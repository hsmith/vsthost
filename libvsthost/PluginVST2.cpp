#include "PluginVST2.h"

#include <iostream>

#include "pluginterfaces\vst2.x\aeffectx.h"

#include "PresetVST2.h"

namespace VSTHost {
PluginVST2::PluginVST2(HMODULE m, AEffect* p, Steinberg::Vst::SpeakerArrangement sa)
	: Plugin(m, sa)
	, plugin(p) {
	plugin->resvd1 = reinterpret_cast<VstIntPtr>(this);

	// try setting correct number of inputs and outputs if there's a need
	SetActive(false);
	const auto cc = GetChannelCount();
	if (static_cast<Steinberg::uint32>(plugin->numInputs) < cc || 
		static_cast<Steinberg::uint32>(plugin->numInputs) < cc) {
		VstSpeakerArrangement in{}, out{};
		GetVST2SpeakerArrangement(in, speaker_arrangement);
		GetVST2SpeakerArrangement(out, speaker_arrangement);
		auto ret = Dispatcher(AEffectXOpcodes::effSetSpeakerArrangement, 0, reinterpret_cast<VstIntPtr>(&in), &out);
		if (!ret && (static_cast<Steinberg::uint32>(plugin->numInputs) < cc || 
					static_cast<Steinberg::uint32>(plugin->numOutputs) < cc)) {
			VstSpeakerArrangement* in_test = nullptr, *out_test = nullptr;
			if (GetVSTVersion() > 2300 && Dispatcher(AEffectXOpcodes::effGetSpeakerArrangement, 0, reinterpret_cast<VstIntPtr>(in_test), out_test)
				&& in_test && in_test->numChannels == GetChannelCount() && out_test && out_test->numChannels == GetChannelCount())
				proper_in_out_num = true;
		}
		else
			proper_in_out_num = true;
	}
	else
		proper_in_out_num = true;
}

PluginVST2::~PluginVST2() {
	SetActive(false);
	state.reset(); // state has to be destroyed before the rest of the plugin is freed
	Dispatcher(AEffectOpcodes::effClose);
	// turns out offClose opcode handles freeing AEffect object and I musn't do that
	plugin.release();
}

Plugin::IsValidCodes PluginVST2::IsValid() const {
	if (plugin) {
		VstPlugCategory c = static_cast<VstPlugCategory>(plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetPlugCategory, 0, 0, nullptr, 0.));
		if (c != kPlugCategSynth && c >= kPlugCategUnknown && c <= kPlugCategRestoration) {
			if (proper_in_out_num) {
				if (plugin->magic == kEffectMagic)
					return IsValidCodes::kValid;
				else
					return IsValidCodes::kInvalid;
			}
			else
				return IsValidCodes::kWrongInOutNum;
		}
		else
			return IsValidCodes::kIsNotEffect;
	}
	else
		return IsValidCodes::kInvalid;
}

void PluginVST2::Initialize(Steinberg::Vst::TSamples bs, Steinberg::Vst::SampleRate sr) {
	sample_rate = sr;
	block_size = bs;
	Dispatcher(AEffectOpcodes::effOpen);
	Dispatcher(AEffectOpcodes::effSetSampleRate, 0, 0, nullptr, static_cast<float>(sample_rate));
	Dispatcher(AEffectOpcodes::effSetBlockSize, 0, static_cast<VstIntPtr>(block_size));
	state = std::unique_ptr<Preset>(new PresetVST2(*this));
	soft_bypass = CanDo("bypass");
	has_editor = static_cast<bool>(plugin->flags & effFlagsHasEditor);
	SetActive(true);
}

std::basic_string<TCHAR> PluginVST2::GetPluginName() const {
	char name[kVstMaxProductStrLen] = { 0 }; // vst2 does not support unicode
	if (plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetEffectName, 0, 0, reinterpret_cast<void*>(name), 0.) ||
		plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetProductString, 0, 0, reinterpret_cast<void*>(name), 0.)) {
		std::string tmp(name);
		return std::basic_string<TCHAR>(tmp.begin(), tmp.end());
	}
	else {
		TCHAR buf[MAX_PATH] = { 0 };
		GetModuleFileName(module, buf, MAX_PATH);
		std::basic_string<TCHAR> tmp(buf);
		std::basic_string<TCHAR>::size_type pos = 0;
		if ((pos = tmp.find_last_of('\\')) != std::basic_string<TCHAR>::npos)
			tmp = tmp.substr(pos + 1);
		if ((pos = tmp.find_last_of('.')) != std::basic_string<TCHAR>::npos)
			tmp = tmp.substr(0, pos);
		return tmp;
	}
}

std::string PluginVST2::GetPluginNameA() const {
	std::string ret;
	char name[kVstMaxProductStrLen] = { 0 }; // vst2 does not support unicode
	if (plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetEffectName, 0, 0, reinterpret_cast<void*>(name), 0.) ||
		plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetProductString, 0, 0, reinterpret_cast<void*>(name), 0.)) {
		ret = name;
	}
	else {
		char buf[MAX_PATH] = { 0 };
		GetModuleFileNameA(module, buf, MAX_PATH);
		ret = buf;
		std::string::size_type pos = 0;
		if ((pos = ret.find_last_of('\\')) != std::string::npos)
			ret = ret.substr(pos + 1);
		if ((pos = ret.find_last_of('.')) != std::string::npos)
			ret = ret.substr(0, pos);
	}
	return ret;
}

void PluginVST2::Process(Steinberg::Vst::Sample32** input, Steinberg::Vst::Sample32** output, Steinberg::Vst::TSamples block_size) {
	if (BypassProcess())
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(static_cast<void*>(output[i]), static_cast<void*>(input[i]), sizeof(input[0][0]) * block_size);
	else {
		std::lock_guard<std::mutex> lock(plugin_lock);
		StartProcessing();
		plugin->processReplacing(plugin.get(), input, output, block_size);
		StopProcessing();
	}
}

void PluginVST2::ProcessReplace(Steinberg::Vst::Sample32** input_output, Steinberg::Vst::TSamples block_size) {
	if (BypassProcess())
		return;
	std::lock_guard<std::mutex> lock(plugin_lock);
	StartProcessing();
	plugin->processReplacing(plugin.get(), input_output, input_output, block_size);
	StopProcessing();
}

void PluginVST2::SetBlockSize(Steinberg::Vst::TSamples bs) {
	if (bs != block_size) {
		bool was_active;
		if (was_active = IsActive())
			SetActive(false);
		block_size = bs;
		Dispatcher(AEffectOpcodes::effSetBlockSize, 0, static_cast<VstIntPtr>(block_size));
		if (was_active)
			SetActive(true);
	}
}

void PluginVST2::SetSampleRate(Steinberg::Vst::SampleRate sr) {
	if (sr != sample_rate) {
		bool was_active;
		if (was_active = IsActive())
			SetActive(false);
		sample_rate = sr;
		Dispatcher(AEffectOpcodes::effSetSampleRate, 0, 0, nullptr, static_cast<float>(sample_rate));
		if (was_active)
			SetActive(true);
	}
}

bool PluginVST2::SetSpeakerArrangement(Steinberg::Vst::SpeakerArrangement sa) {
	if (sa != speaker_arrangement) {
		bool was_active;
		if (was_active = IsActive())
			SetActive(false);
		VstSpeakerArrangement in{}, out{};
		GetVST2SpeakerArrangement(in, speaker_arrangement);
		GetVST2SpeakerArrangement(out, speaker_arrangement);
		auto ret = Dispatcher(AEffectXOpcodes::effSetSpeakerArrangement, 0, reinterpret_cast<VstIntPtr>(&in), &out);
		if (was_active)
			SetActive(true);
		return ret != 0;
	}
	else
		return true;
}

Steinberg::uint32 PluginVST2::GetProgramCount() const {
	return static_cast<Steinberg::uint32>(plugin->numPrograms);
}

void PluginVST2::SetProgram(Steinberg::uint32 id) {
	Dispatcher(AEffectXOpcodes::effBeginSetProgram);
	Dispatcher(AEffectOpcodes::effSetProgram, 0, id);
	Dispatcher(AEffectXOpcodes::effEndSetProgram);
}

std::basic_string<TCHAR> PluginVST2::GetProgramName(Steinberg::uint32 id) {
	auto s = GetProgramNameA(id); // VST2 program names are in ASCII only
	return std::basic_string<TCHAR>(s.begin(), s.end());
}

std::string PluginVST2::GetProgramNameA(Steinberg::uint32 id) {
	char tmp[kVstMaxProgNameLen + 1] = { 0 };
	auto current_program = static_cast<Steinberg::uint32>(Dispatcher(AEffectOpcodes::effGetProgram));
	bool program_changed = false;
	if (!Dispatcher(AEffectXOpcodes::effGetProgramNameIndexed, id, 0, tmp)) {
		SetProgram(id);
		Dispatcher(AEffectOpcodes::effGetProgramName, 0, 0, tmp);
		program_changed = true;
	}
	if (program_changed)
		SetProgram(current_program);
	return std::string(tmp);
}

Steinberg::uint32 PluginVST2::GetParameterCount() const {
	return plugin->numParams;
}

Steinberg::Vst::ParamValue PluginVST2::GetParameter(Steinberg::Vst::ParamID id) const {
	return plugin->getParameter(plugin.get(), id);
}

void PluginVST2::SetParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) {
	plugin->setParameter(plugin.get(), id, static_cast<float>(value));
}

std::string PluginVST2::GetPresetExtension() {
	return PresetVST2::kExtension;
}

void PluginVST2::SetBypass(bool bypass_) {
	if (bypass != bypass_) {
		std::lock_guard<std::mutex> lock(plugin_lock);
		bypass = bypass_;
		if (soft_bypass)
			Dispatcher(AEffectXOpcodes::effSetBypass, 0, bypass);
	}
}

bool PluginVST2::BypassProcess() const {	// wywolanie process omijaj tylko wtedy, jak
	return (bypass && !soft_bypass) || !active;	// bypass == true i wtyczka nie obsluguje soft bypass
}

bool PluginVST2::HasEditor() const {
	return static_cast<bool>(plugin->flags & effFlagsHasEditor);
}

void PluginVST2::CreateEditor(HWND hwnd) {
	this->hwnd = hwnd;
}

Steinberg::uint32 PluginVST2::GetEditorHeight() {
	if (HasEditor()) {
		ERect* erect = nullptr;
		Dispatcher(AEffectOpcodes::effEditGetRect, 0, 0, &erect);
		if (erect)
			return erect->bottom - erect->top;
	}
	return 0;
}

Steinberg::uint32 PluginVST2::GetEditorWidth() {
	if (HasEditor()) {
		ERect* erect = nullptr;
		Dispatcher(AEffectOpcodes::effEditGetRect, 0, 0, &erect);
		if (erect)
			return erect->right - erect->left;
	}
	return 0;
}

void PluginVST2::ShowEditor() {
	if (HasEditor())
		Dispatcher(AEffectOpcodes::effEditOpen, 0, 0, hwnd);
}

void PluginVST2::HideEditor() {
	if (HasEditor())
		Dispatcher(AEffectOpcodes::effEditClose, 0, 0, hwnd);
}

VstIntPtr VSTCALLBACK PluginVST2::HostCallbackWrapper(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
	if (opcode == AudioMasterOpcodes::audioMasterVersion)
		return 2400;
	PluginVST2* plugin = reinterpret_cast<PluginVST2*>(effect->resvd1);
	if (plugin)
		return plugin->HostCallback(effect, opcode, index, value, ptr, opt);
	return 0;
}

void PluginVST2::Resume() {
	Dispatcher(AEffectOpcodes::effMainsChanged, 0, true);
	StopProcessing();
	active = true;
}

void PluginVST2::Suspend() {
	StopProcessing();
	Dispatcher(AEffectOpcodes::effMainsChanged, 0, false);
	active = false;
}

void PluginVST2::StartProcessing() {
	Dispatcher(AEffectXOpcodes::effStartProcess);
}

void PluginVST2::StopProcessing() {
	Dispatcher(AEffectXOpcodes::effStopProcess);
}

VstIntPtr VSTCALLBACK PluginVST2::Dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
	return plugin->dispatcher(plugin.get(), opcode, index, value, ptr, opt);
}

VstIntPtr VSTCALLBACK PluginVST2::HostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
	switch (opcode) {
		case AudioMasterOpcodes::audioMasterVersion:
			return 2400;
		case AudioMasterOpcodes::audioMasterIdle:
			return 0; // plugin gives idle time to host
		case AudioMasterOpcodesX::audioMasterGetTime:
		case AudioMasterOpcodesX::audioMasterProcessEvents:
		case AudioMasterOpcodesX::audioMasterIOChanged:
		case AudioMasterOpcodesX::audioMasterSizeWindow:
			return 0; // unsupported
		case AudioMasterOpcodesX::audioMasterGetSampleRate:
			return static_cast<VstIntPtr>(sample_rate);
		case AudioMasterOpcodesX::audioMasterGetBlockSize:
			return static_cast<VstIntPtr>(block_size);
		case AudioMasterOpcodesX::audioMasterGetInputLatency:
		case AudioMasterOpcodesX::audioMasterGetOutputLatency:
		case AudioMasterOpcodesX::audioMasterGetCurrentProcessLevel:
		case AudioMasterOpcodesX::audioMasterGetAutomationState:
		case AudioMasterOpcodesX::audioMasterOfflineStart:
		case AudioMasterOpcodesX::audioMasterOfflineRead:
		case AudioMasterOpcodesX::audioMasterOfflineWrite:
		case AudioMasterOpcodesX::audioMasterOfflineGetCurrentPass:
		case AudioMasterOpcodesX::audioMasterOfflineGetCurrentMetaPass:
			return 0; // unsupported
		case AudioMasterOpcodesX::audioMasterGetVendorString:
			std::memcpy(ptr, "jperek", 7); // kVstMaxVendorStrLen
			return 1;
		case AudioMasterOpcodesX::audioMasterGetProductString:
			std::memcpy(ptr, "VSTHost", 8); // kVstMaxProductStrLen
			return 1;
		case AudioMasterOpcodesX::audioMasterGetVendorVersion:
			return 1000;
		case AudioMasterOpcodesX::audioMasterVendorSpecific:
			return 0;
		case AudioMasterOpcodesX::audioMasterCanDo:
			if (std::strcmp(static_cast<char*>(ptr), "sendVstEvents") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "sendVstMidiEvent") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "sendVstTimeInfo") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "receiveVstEvents") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "receiveVstMidiEvent") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "reportConnectionChanges") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "acceptIOChanges") == 0)
				return 1;
			else if (std::strcmp(static_cast<char*>(ptr), "sizeWindow") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "offline") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "openFileSelector") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "closeFileSelector") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "startStopProcess") == 0)
				return 1;
			else if (std::strcmp(static_cast<char*>(ptr), "shellCategory") == 0)
				return -1;
			else if (std::strcmp(static_cast<char*>(ptr), "sendVstMidiEventFlagIsRealtime") == 0)
				return -1;
			return 0;
		case AudioMasterOpcodesX::audioMasterGetLanguage:
			return VstHostLanguage::kVstLangEnglish;
		case AudioMasterOpcodesX::audioMasterGetDirectory: {
			auto buf = GetPluginDirectory();
			std::memcpy(ptr, buf.c_str(), buf.size());
			return 1;
		}
		case AudioMasterOpcodesX::audioMasterUpdateDisplay:
			return 0;
		case AudioMasterOpcodesX::audioMasterBeginEdit:
			//index
			return 1;
		case AudioMasterOpcodesX::audioMasterEndEdit:
			//index
			return 1;
		case AudioMasterOpcodesX::audioMasterOpenFileSelector:
		case AudioMasterOpcodesX::audioMasterCloseFileSelector:
		default:
			return 0;
	}
}

bool PluginVST2::CanDo(const char *canDo) const {
	return (plugin->dispatcher(plugin.get(), AEffectXOpcodes::effCanDo, 0, 0, (void *)canDo, 0.) != 0);
}

Steinberg::int32 PluginVST2::GetVendorVersion() const {
	return plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetVendorVersion, 0, 0, nullptr, 0.);
}

Steinberg::int32 PluginVST2::GetVSTVersion() const {
	return plugin->dispatcher(plugin.get(), AEffectXOpcodes::effGetVstVersion, 0, 0, nullptr, 0.);
}

Steinberg::int32 PluginVST2::GetFlags() const {
	return plugin->flags;
}

VstInt32 PluginVST2::GetVST2SpeakerArrangementType(Steinberg::Vst::SpeakerArrangement sa) {
	switch (sa) {
		case Steinberg::Vst::SpeakerArr::kMono:
			return ::kSpeakerArrMono;
		case Steinberg::Vst::SpeakerArr::kStereo:
			return ::kSpeakerArrStereo;
		case Steinberg::Vst::SpeakerArr::kStereoSurround:
			return ::kSpeakerArrStereoSurround;
		case Steinberg::Vst::SpeakerArr::kStereoCenter:
			return ::kSpeakerArrStereoCenter;
		case Steinberg::Vst::SpeakerArr::kStereoSide:
			return ::kSpeakerArrStereoSide;
		case Steinberg::Vst::SpeakerArr::kStereoCLfe:
			return ::kSpeakerArrStereoCLfe;
		case Steinberg::Vst::SpeakerArr::k30Cine:
			return ::kSpeakerArr30Cine;
		case Steinberg::Vst::SpeakerArr::k30Music:
			return ::kSpeakerArr30Music;
		case Steinberg::Vst::SpeakerArr::k31Cine:
			return ::kSpeakerArr31Cine;
		case Steinberg::Vst::SpeakerArr::k31Music:
			return ::kSpeakerArr31Music;
		case Steinberg::Vst::SpeakerArr::k40Cine:
			return ::kSpeakerArr40Cine;
		case Steinberg::Vst::SpeakerArr::k40Music:
			return ::kSpeakerArr40Music;
		case Steinberg::Vst::SpeakerArr::k41Cine:
			return ::kSpeakerArr41Cine;
		case Steinberg::Vst::SpeakerArr::k41Music:
			return ::kSpeakerArr41Music;
		case Steinberg::Vst::SpeakerArr::k50:
			return ::kSpeakerArr50;
		case Steinberg::Vst::SpeakerArr::k51:
			return ::kSpeakerArr51;
		case Steinberg::Vst::SpeakerArr::k60Cine:
			return ::kSpeakerArr60Cine;
		case Steinberg::Vst::SpeakerArr::k60Music:
			return ::kSpeakerArr60Music;
		case Steinberg::Vst::SpeakerArr::k61Cine:
			return ::kSpeakerArr61Cine;
		case Steinberg::Vst::SpeakerArr::k61Music:
			return ::kSpeakerArr61Music;
		case Steinberg::Vst::SpeakerArr::k70Cine:
			return ::kSpeakerArr70Cine;
		case Steinberg::Vst::SpeakerArr::k70Music:
			return ::kSpeakerArr70Music;
		case Steinberg::Vst::SpeakerArr::k71Cine:
			return ::kSpeakerArr71Cine;
		case Steinberg::Vst::SpeakerArr::k71Music:
			return ::kSpeakerArr71Music;
		case Steinberg::Vst::SpeakerArr::k80Cine:
			return ::kSpeakerArr80Cine;
		case Steinberg::Vst::SpeakerArr::k80Music:
			return ::kSpeakerArr80Music;
		case Steinberg::Vst::SpeakerArr::k81Cine:
			return ::kSpeakerArr81Cine;
		case Steinberg::Vst::SpeakerArr::k81Music:
			return ::kSpeakerArr81Music;
		case Steinberg::Vst::SpeakerArr::k102:
			return ::kSpeakerArr102;
		default:
			return ::kSpeakerArrEmpty;
	}
}

VstSpeakerType PluginVST2::GetVST2SpeakerType(Steinberg::Vst::Speaker speaker) {
	switch (speaker) {
		case Steinberg::Vst::kSpeakerM:
			return ::kSpeakerM;
		case Steinberg::Vst::kSpeakerL:
			return ::kSpeakerL;
		case Steinberg::Vst::kSpeakerR:
			return ::kSpeakerR;
		case Steinberg::Vst::kSpeakerC:
			return ::kSpeakerC;
		case Steinberg::Vst::kSpeakerLfe:
			return ::kSpeakerLfe;
		case Steinberg::Vst::kSpeakerLs:
			return ::kSpeakerLs;
		case Steinberg::Vst::kSpeakerRs:
			return ::kSpeakerRs;
		case Steinberg::Vst::kSpeakerLc:
			return ::kSpeakerLc;
		case Steinberg::Vst::kSpeakerRc:
			return ::kSpeakerRc;
		case Steinberg::Vst::kSpeakerS:
			return ::kSpeakerS;
		case Steinberg::Vst::kSpeakerSl:
			return ::kSpeakerSl;
		case Steinberg::Vst::kSpeakerSr:
			return ::kSpeakerSr;
		case Steinberg::Vst::kSpeakerTc:
			return ::kSpeakerTm;
		case Steinberg::Vst::kSpeakerTfl:
			return ::kSpeakerTfl;
		case Steinberg::Vst::kSpeakerTfc:
			return ::kSpeakerTfc;
		case Steinberg::Vst::kSpeakerTfr:
			return ::kSpeakerTfr;
		case Steinberg::Vst::kSpeakerTrl:
			return ::kSpeakerTrl;
		case Steinberg::Vst::kSpeakerTrc:
			return ::kSpeakerTrc;
		case Steinberg::Vst::kSpeakerTrr:
			return ::kSpeakerTrr;
		case Steinberg::Vst::kSpeakerLfe2:
			return ::kSpeakerLfe2;
	}
	return ::kSpeakerUndefined;
}

void PluginVST2::GetVST2SpeakerArrangement(VstSpeakerArrangement& sa_vst2, Steinberg::Vst::SpeakerArrangement sa_vst3) {
	sa_vst2.type = GetVST2SpeakerArrangementType(sa_vst3);
	sa_vst2.numChannels = Steinberg::Vst::SpeakerArr::getChannelCount(sa_vst3);
	if (sa_vst2.numChannels > 8)
		return;

	static constexpr char* speaker_names[] = { "M", "L", "R", "C", "Lfe", "Ls", "Rs", "Lc", 
		"Rc", "Cs", "Sl", "Sr", "Tm", "Tfl", "Tfc", "Tfr", "Trl", "Trc", "Trr", "Lfe2"};
	static constexpr auto speaker_names_size = sizeof(speaker_names) / sizeof(speaker_names[0]);

	Steinberg::Vst::Speaker test_speaker = Steinberg::Vst::kSpeakerL;
	for (Steinberg::uint32 i = 0; i < static_cast<Steinberg::uint32>(sa_vst2.numChannels); ++i) {
		Steinberg::Vst::Speaker speaker = 0;
		while (speaker == 0 && test_speaker != 0) {
			if (sa_vst3 & test_speaker)
				speaker = test_speaker;
			test_speaker <<= 1;
		}
		if (speaker) {
			const auto type = GetVST2SpeakerType(speaker);
			sa_vst2.speakers[i].type = type;
			if (type >= 0 && type < speaker_names_size)
				strncpy(sa_vst2.speakers[i].name, speaker_names[type], kVstMaxNameLen);
			else
				strncpy(sa_vst2.speakers[i].name, std::to_string(i + 1).c_str(), kVstMaxNameLen);
		}
	}
}
} // namespace