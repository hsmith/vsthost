#include "VSTPlugin.h"

#include "VSTPreset.h"
#include "VSTPluginGUI.h"

using namespace std;

VSTPlugin::VSTPlugin(HMODULE m, AEffect* p) : Plugin(m), plugin(p) {

}

VSTPlugin::~VSTPlugin() {
	if (module) 
		::FreeLibrary(module);
}

bool VSTPlugin::IsValid() {
	VstPlugCategory c = static_cast<VstPlugCategory>(Dispatcher(effGetPlugCategory));
	return plugin->magic == kEffectMagic && c != kPlugCategSynth && c >= kPlugCategUnknown && c <= kPlugCategRestoration;
}

void VSTPlugin::Initialize() {
	Dispatcher(AEffectOpcodes::effOpen);
	SetSampleRate(sample_rate);
	SetBlockSize(block_size);
	UpdateSpeakerArrangement();
	state = new VSTPreset(plugin);
	plugin->resvd1 = reinterpret_cast<VstIntPtr>(this);
	soft_bypass = CanDo("bypass");
	SetActive(true);
}

std::string VSTPlugin::GetPluginName() {
	TCHAR name[kVstMaxProductStrLen + 1] = { 0 };
	if (Dispatcher(effGetEffectName, 0, 0, (void *)name));
	else if (Dispatcher(effGetProductString, 0, 0, (void *)name));
	else {
		GetModuleFileName(module, name, kVstMaxProductStrLen + 1);
		std::string tmp(name);
		std::string::size_type pos = 0;
		if ((pos = tmp.find_last_of('\\')) != std::string::npos)
			tmp = tmp.substr(pos + 1);
		if ((pos = tmp.find_last_of('.')) != std::string::npos)
			tmp = tmp.substr(0, pos);
		return tmp;
	}
	return std::string(name);
}

void VSTPlugin::Process(Steinberg::Vst::Sample32** input, Steinberg::Vst::Sample32** output) {
	if (IsActive()) {
		if (BypassProcess()) // hard bypass
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(static_cast<void*>(output[i]), static_cast<void*>(input[i]), sizeof(input[0][0]));
		else {
			StartProcessing();
			if (0 != (plugin->flags & VstAEffectFlags::effFlagsCanReplacing))
				plugin->processReplacing(plugin, input, output, block_size);
			else
				plugin->process(plugin, input, output, block_size);
			StopProcessing();
		}
	}
}

void VSTPlugin::UpdateBlockSize() {
	bool was_active;
	if (was_active = IsActive())
		SetActive(false);
	Dispatcher(AEffectOpcodes::effSetBlockSize, 0, static_cast<int>(block_size));
	if (was_active)
		SetActive(true);
}

void VSTPlugin::UpdateSampleRate() {
	bool was_active;
	if (was_active = IsActive())
		SetActive(false);
	Dispatcher(AEffectOpcodes::effSetSampleRate, 0, static_cast<float>(sample_rate));
	if (was_active)
		SetActive(true);
}

void VSTPlugin::UpdateSpeakerArrangement() {
	if (GetVSTVersion() < 2300)
		return;
	bool was_active;
	if (was_active = IsActive())
		SetActive(false);
	VstSpeakerArrangement in{}, out{};
	if (speaker_arrangement == Steinberg::Vst::SpeakerArr::kMono) {
		in.numChannels = 1;
		in.type = VstSpeakerArrangementType::kSpeakerArrMono;
		in.speakers[0].type = VstSpeakerType::kSpeakerM;
		in.speakers[0].name[0] = 'M';
		in.speakers[0].name[1] = '\0';
		out.numChannels = 1;
		out.type = VstSpeakerArrangementType::kSpeakerArrMono;
		out.speakers[0].type = VstSpeakerType::kSpeakerM;
		out.speakers[0].name[0] = 'M';
		out.speakers[0].name[1] = '\0';
	}
	else {
		in.numChannels = 2;
		in.type = VstSpeakerArrangementType::kSpeakerArrStereo;
		in.speakers[0].type = VstSpeakerType::kSpeakerL;
		in.speakers[0].name[0] = 'L';
		in.speakers[0].name[1] = '\0';
		in.speakers[1].type = VstSpeakerType::kSpeakerR;
		in.speakers[1].name[0] = 'R';
		in.speakers[1].name[1] = '\0';
		out.numChannels = 2;
		out.type = VstSpeakerArrangementType::kSpeakerArrStereo;
		out.speakers[0].type = VstSpeakerType::kSpeakerL;
		out.speakers[0].name[0] = 'L';
		out.speakers[0].name[1] = '\0';
		out.speakers[1].type = VstSpeakerType::kSpeakerR;
		out.speakers[1].name[0] = 'R';
		out.speakers[1].name[1] = '\0';

	}
	Dispatcher(AEffectXOpcodes::effSetSpeakerArrangement, 0, reinterpret_cast<Steinberg::int32>(&in), &out);
	if (was_active)
		SetActive(true);
}

Steinberg::int32 VSTPlugin::GetProgramCount() {
	return plugin->numPrograms;
}

void VSTPlugin::SetProgram(Steinberg::int32 id) {
	Dispatcher(AEffectXOpcodes::effBeginSetProgram);
	Dispatcher(AEffectOpcodes::effSetProgram, 0, id);
	Dispatcher(AEffectXOpcodes::effEndSetProgram);
}

Steinberg::int32 VSTPlugin::GetParameterCount() {
	return plugin->numParams;
}

Steinberg::Vst::ParamValue VSTPlugin::GetParameter(Steinberg::Vst::ParamID id) {
	return plugin->getParameter(plugin, id);
}

void VSTPlugin::SetParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) {
	plugin->setParameter(plugin, id, static_cast<float>(value));
}

void VSTPlugin::SetBypass(bool bypass_) {
	if (bypass != bypass_) {
		bypass = bypass_;
		if (soft_bypass)
			Dispatcher(AEffectXOpcodes::effSetBypass, 0, bypass);
	}
}

bool VSTPlugin::BypassProcess() {	// wywolanie process omijaj tylko wtedy, jak
	return bypass && !soft_bypass;	// bypass == true i wtyczka nie obsluguje soft bypass
}

bool VSTPlugin::HasEditor() {
	return static_cast<bool>(plugin->flags & effFlagsHasEditor);
}

void VSTPlugin::CreateEditor(HWND hWnd) {
	if (!gui && HasEditor()) {
		gui = new VSTPluginGUI(*this, plugin);
		gui->Initialize(hWnd);
	}
}

VstIntPtr VSTCALLBACK VSTPlugin::HostCallbackWrapper(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt) {
	if (opcode == AudioMasterOpcodes::audioMasterVersion)
		return 2400;
	VSTPlugin* plugin = reinterpret_cast<VSTPlugin*>(effect->resvd1);
	if (plugin)
		return plugin->HostCallback(effect, opcode, index, value, ptr, opt);
	return 0;
}

void VSTPlugin::PrintPrograms() {
	char ProgramName[kVstMaxProgNameLen + 1] = { 0 };
	int currentProgram = Dispatcher(AEffectOpcodes::effGetProgram);
	bool programChanged = false;
	for (decltype(GetProgramCount()) i = 0; i < GetProgramCount(); i++) {
		if (Dispatcher(effGetProgramNameIndexed, i, 0, ProgramName)) std::cout << ProgramName << std::endl;
		else {
			Dispatcher(AEffectOpcodes::effSetProgram, 0, i);
			Dispatcher(AEffectOpcodes::effGetProgramName, 0, 0, ProgramName);
			if (!programChanged) programChanged = true;
		}
	}
	if (programChanged) Dispatcher(AEffectOpcodes::effSetProgram, 0, currentProgram);
}

void VSTPlugin::PrintParameters() {	// + 1, bo wyjatki wyrzucalo
	char ParamLabel[kVstMaxParamStrLen + 1] = { 0 };
	char ParamDisplay[kVstMaxParamStrLen + 1] = { 0 };
	char ParamName[kVstMaxParamStrLen + 1] = { 0 };
	VstParameterProperties properties;
	for (decltype(GetParameterCount()) i = 0; i < GetParameterCount(); i++) {
		Dispatcher(effGetParamLabel, i, 0, ParamLabel);
		Dispatcher(effGetParamDisplay, i, 0, ParamDisplay);
		Dispatcher(effGetParamName, i, 0, ParamName);
		std::cout << "Parameter " << i << ":" << endl
			<< "Label: " << ParamLabel << endl
			<< "Display: " << ParamDisplay << endl
			<< "Name: " << ParamName << endl
			<< "Value: " << GetParameter(i) << endl;
		if (Dispatcher(effGetParameterProperties, i, 0, &properties)) {
			for (int j = 0; j <= 6; j++) {
				cout << "Flag ";
				switch (1 << i) {
				case kVstParameterIsSwitch:
					cout << "kVstParameterIsSwitch: " << (properties.flags & (1 << i) ? "Yes" : "No") << endl;
					break;
				case kVstParameterUsesIntegerMinMax:
					cout << "kVstParameterUsesIntegerMinMax: ";
					if (properties.flags & (1 << i)) {
						cout << "Yes" << endl
							<< "minInteger: " << properties.minInteger << endl
							<< "maxInteger: " << properties.maxInteger << endl;
					}
					else cout << "No" << endl;
					break;
				case kVstParameterUsesFloatStep:
					cout << "kVstParameterUsesFloatStep: ";
					if (properties.flags & (1 << i)) {
						cout << "Yes" << endl
							<< "stepFloat: " << properties.stepFloat << endl
							<< "smallStepFloat: " << properties.smallStepFloat << endl
							<< "largeStepFloat: " << properties.largeStepFloat << endl;
					}
					else cout << "No" << endl;
					break;
				case kVstParameterUsesIntStep:
					cout << "kVstParameterUsesIntStep: ";
					if (properties.flags & (1 << i)) {
						cout << "Yes" << endl
							<< "largeStepInteger: " << properties.largeStepInteger << endl;
					}
					else cout << "No" << endl;
					break;
				case kVstParameterSupportsDisplayIndex:
					cout << "kVstParameterSupportsDisplayIndex: ";
					if (properties.flags & (1 << i)) {
						cout << "Yes" << endl
							<< "displayIndex: " << properties.displayIndex << endl;
					}
					else cout << "No" << endl;
					break;
				case kVstParameterSupportsDisplayCategory:
					cout << "kVstParameterSupportsDisplayCategory: ";
					if (properties.flags & (1 << i)) {
						cout << "Yes" << endl
							<< "category: " << properties.category << endl
							<< "numParametersInCategory: " << properties.numParametersInCategory << endl
							<< "categoryLabel: " << properties.categoryLabel << endl;
					}
					else cout << "No" << endl;
					break;
				case kVstParameterCanRamp:
					cout << "kVstParameterCanRamps: " << (properties.flags & (1 << i) ? "Yes" : "No") << endl;
					break;
				}
			}
		}
	}
}

void VSTPlugin::PrintCanDos() {
	char *PlugCanDos[] = { "sendVstEvents", "sendVstMidiEvent", "receiveVstEvents", "receiveVstMidiEvent",
		"receiveVstTimeInfo", "offline", "midiProgramNames", "bypass" };
	for (int i = 0; i < (sizeof(PlugCanDos) / sizeof(char *)); i++) {
		cout << PlugCanDos[i] << ": " << (CanDo(PlugCanDos[i]) ? "Yes" : "No") << endl;
	}
}

void VSTPlugin::PrintInfo() {
	char *separator = "================\n";
	char EffectName[kVstMaxEffectNameLen + 1] = { 0 };
	char VendorString[kVstMaxVendorStrLen + 1] = { 0 };
	char ProductString[kVstMaxProductStrLen + 1] = { 0 };
	Dispatcher(effGetEffectName, 0, 0, (void *)EffectName);
	Dispatcher(effGetVendorString, 0, 0, (void *)VendorString);
	Dispatcher(effGetProductString, 0, 0, (void *)ProductString);
	VstInt32 VendorVersion = GetVendorVersion();
	VstInt32 VSTVersion = GetVSTVersion();
	cout << "VST Plugin " << EffectName << endl
		<< "Version: " << GetVersion() << endl
		<< "Vendor: " << VendorString << endl
		<< "Product: " << ProductString << endl
		<< "Vendor Version: " << VendorVersion << endl
		<< "UniqueID: " << plugin->uniqueID << endl
		<< "VST Version: " << VSTVersion << endl
		<< separator << "Flags: " << endl;
	for (int i = 0; i < 13; i++) {
		if (i == 6 || i == 7) continue;
		switch (1 << i) {
		case effFlagsHasEditor:
			std::cout << "effFlagsHasEditor: ";
			break;
		case effFlagsCanReplacing:
			std::cout << "effFlagsCanReplacing: ";
			break;
		case effFlagsProgramChunks:
			std::cout << "effFlagsProgramChunks: ";
			break;
		case effFlagsIsSynth:
			std::cout << "effFlagsIsSynth: ";
			break;
		case effFlagsNoSoundInStop:
			std::cout << "effFlagsNoSoundInStop: ";
			break;
		case effFlagsCanDoubleReplacing:
			std::cout << "effFlagsCanDoubleReplacing (VST 2.4): ";
			break;
		case effFlagsHasClip:
			std::cout << "effFlagsHasClip (Deprecated): ";
			break;
		case effFlagsHasVu:
			std::cout << "effFlagsHasVu (Deprecated): ";
			break;
		case effFlagsCanMono:
			std::cout << "effFlagsCanMono (Deprecated): ";
			break;
		case effFlagsExtIsAsync:
			std::cout << "effFlagsExtIsAsync (Deprecated): ";
			break;
		case effFlagsExtHasBuffer:
			std::cout << "effFlagsExtHasBuffer (Deprecated): ";
			break;
		}
		std::cout << (plugin->flags & (1 << i) ? "Yes" : "No") << std::endl;
	}
	cout << separator << "Presets(" << GetProgramCount() << "):" << endl;
	PrintPrograms();
	cout << separator << "Parameters(" << GetParameterCount() << "):" << endl;
	//PrintParameters();
	cout << separator << "Inputs: " << plugin->numInputs << endl
		<< "Outputs: " << plugin->numOutputs << endl
		<< separator << "CanDo:" << endl;
	PrintCanDos();
	cout << separator;
}

void VSTPlugin::Resume() {
	Dispatcher(AEffectOpcodes::effMainsChanged, 0, true);
	StopProcessing();
	active = true;
}

void VSTPlugin::Suspend() {
	StopProcessing();
	Dispatcher(AEffectOpcodes::effMainsChanged, 0, false);
	active = false;
}

void VSTPlugin::StartProcessing() {
	Dispatcher(AEffectXOpcodes::effStartProcess);
}

void VSTPlugin::StopProcessing() {
	Dispatcher(AEffectXOpcodes::effStopProcess);
}

VstIntPtr VSTCALLBACK VSTPlugin::Dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
	return plugin->dispatcher(plugin, opcode, index, value, ptr, opt);
}

VstIntPtr VSTCALLBACK VSTPlugin::HostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt) {
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
			char buf[128]{};
			auto length = GetModuleFileName(module, buf, 128);
			std::string tmp(buf, length);
			std::string::size_type pos = 0;
			if ((pos = tmp.find_last_of('\\')) != std::string::npos) {
				std::memcpy(ptr, tmp.c_str(), tmp.size());
				return 1;
			}
			else 
				return 0;
		}
		case AudioMasterOpcodesX::audioMasterUpdateDisplay:
			if (gui)
				;//gui->Update();
			return 1;
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

bool VSTPlugin::CanDo(const char *canDo) {
	return (Dispatcher(effCanDo, 0, 0, (void *)canDo) != 0);
}

int VSTPlugin::GetVendorVersion() {
	return Dispatcher(effGetVendorVersion);
}

int VSTPlugin::GetVSTVersion() {
	return Dispatcher(effGetVstVersion);
}