#include "PluginVST3.h"

#include <string>
#include <cstring>

#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/ipluginbase.h"
DEF_CLASS_IID(Steinberg::IPluginFactory2)
DEF_CLASS_IID(Steinberg::Vst::IComponent)
DEF_CLASS_IID(Steinberg::Vst::IComponentHandler)
DEF_CLASS_IID(Steinberg::Vst::IAudioProcessor)
DEF_CLASS_IID(Steinberg::Vst::IEditController)
DEF_CLASS_IID(Steinberg::Vst::IConnectionPoint)
DEF_CLASS_IID(Steinberg::Vst::IUnitInfo)
DEF_CLASS_IID(Steinberg::IBStream)

#include "ParameterChanges.h"
#include "PresetVST3.h"

extern "C" typedef bool (PLUGIN_API *VST3ExitProc)();

namespace VSTHost {
PluginVST3::PluginVST3(HMODULE m, Steinberg::IPluginFactory* f, Steinberg::FUnknown* c)
	: Plugin(m), factory(f), class_index(0), context(c) {
	pd.inputs = nullptr;
	pd.outputs = nullptr;
	pd.inputParameterChanges = nullptr;
	pd.outputParameterChanges = nullptr;
	Steinberg::PClassInfo ci;
	Steinberg::tresult result;
	for (decltype(factory->countClasses()) i = 0; i < factory->countClasses(); ++i) {
		class_index = i;
		factory->getClassInfo(class_index, &ci);
		result = factory->createInstance(ci.cid, FUnknown::iid, reinterpret_cast<void**>(&plugin));
		if (result == Steinberg::kResultOk && plugin) {
			result = factory->createInstance(ci.cid, Steinberg::Vst::IComponent::iid, reinterpret_cast<void**>(&processor_component));
			if (result == Steinberg::kResultOk && processor_component) {
				processor_component_initialized = processor_component->initialize(context) == Steinberg::kResultOk;
				result = processor_component->queryInterface(Steinberg::Vst::IEditController::iid, reinterpret_cast<void**>(&edit_controller));
				if (result != Steinberg::kResultOk) {
					separated = true;
					Steinberg::FUID controllerCID;
					if (processor_component->getControllerClassId(controllerCID) == Steinberg::kResultTrue && controllerCID.isValid())
						result = factory->createInstance(controllerCID, Steinberg::Vst::IEditController::iid, (void**)&edit_controller);
				}
				if (processor_component_initialized && result == Steinberg::kResultOk) {
					edit_controller_initialized = edit_controller->initialize(context) == Steinberg::kResultOk;
					result = processor_component->queryInterface(Steinberg::Vst::IAudioProcessor::iid, reinterpret_cast<void**>(&audio));
					Steinberg::Vst::BusInfo bi_in{}, bi_out{};
					Steinberg::Vst::MediaType mt = Steinberg::Vst::MediaTypes::kAudio;
					for (Steinberg::int32 i = 0; i < processor_component->getBusCount(mt, Steinberg::Vst::BusDirections::kInput); ++i) {
						processor_component->getBusInfo(mt, Steinberg::Vst::BusDirections::kInput, i, bi_in);
						if (bi_out.channelCount == GetChannelCount() && bi_in.busType == Steinberg::Vst::BusTypes::kMain)
							break;
					}
					for (Steinberg::int32 i = 0; i < processor_component->getBusCount(mt, Steinberg::Vst::BusDirections::kOutput); ++i) {
						processor_component->getBusInfo(mt, Steinberg::Vst::BusDirections::kOutput, i, bi_out);
						if (bi_out.channelCount == GetChannelCount() && bi_out.busType == Steinberg::Vst::BusTypes::kMain)
							break;
					}
					Steinberg::Vst::SpeakerArrangement sa = Steinberg::Vst::SpeakerArr::kStereo;
					can_stereo = (bi_out.channelCount == GetChannelCount() && bi_in.channelCount == GetChannelCount())
						|| audio->setBusArrangements(&sa, 1, &sa, 1) == Steinberg::kResultOk;
				}
			}
		}
		if (IsValid() == IsValidCodes::kValid && result == Steinberg::kResultOk)
			break;
		if (processor_component_initialized) {
			processor_component->terminate();
			processor_component_initialized = false;
			if (edit_controller_initialized) {
				edit_controller->terminate();
				edit_controller_initialized = false;
			}
		}
		processor_component = nullptr;
		edit_controller = nullptr;
		audio = nullptr;
		can_stereo = false;
		separated = false;
	}
}

PluginVST3::~PluginVST3() {
	SetActive(false);
	if (plugin_view) {
		plugin_view->removed();
		plugin_view->release();
	}
	state.reset(); // state has to be destroyed before the rest of the plugin is freed
	if (connection_point_component && connection_point_controller) {
		connection_point_component->disconnect(connection_point_controller);
		connection_point_controller->disconnect(connection_point_component);
		connection_point_component->release();
		connection_point_controller->release();
	}
	if (audio)
		audio->release();
	if (unit_info)
		unit_info->release();
	if (edit_controller) {
		edit_controller->setComponentHandler(0);
		//if (edit_controller_initialized)
		//	edit_controller->terminate();
		//edit_controller->release();
	}
	if (processor_component && separated) {
		//if (processor_component_initialized)
		//	processor_component->terminate();
		//processor_component->release();
	}
	if (plugin)
		plugin->release();
	if (factory)
		factory->release();
	void* exitProc = nullptr;
	exitProc = ::GetProcAddress(module, "ExitDll");
	if (exitProc)
		static_cast<VST3ExitProc>(exitProc)();
}

Plugin::IsValidCodes PluginVST3::IsValid() const {
	Steinberg::IPluginFactory2* factory2 = nullptr;
	factory->queryInterface(Steinberg::IPluginFactory2::iid, reinterpret_cast<void**>(&factory2));
	if (factory2) {
		Steinberg::PClassInfo2 ci2;
		factory2->getClassInfo2(class_index, &ci2);
		factory2->release();
		std::string subcategory(ci2.subCategories, ci2.kSubCategoriesSize);
		if (!std::strcmp(ci2.category, "Audio Module Class") && subcategory.find("Fx") != std::string::npos) {
			if (can_stereo) {
				if (edit_controller && audio && processor_component)
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

void PluginVST3::Initialize(Steinberg::Vst::TSamples bs, Steinberg::Vst::SampleRate sr) {
	block_size = bs;
	sample_rate = sr;

	// set component handler for edit controller
	edit_controller->setComponentHandler(this);

	// check for bypass parameter (soft bypass) and for preset change parameter
	Steinberg::Vst::ParameterInfo pi;
	static Steinberg::Vst::ParamID kNoParamId = -1;
	for (Steinberg::int32 i = 0; i < edit_controller->getParameterCount() && (bypass_param_id == -1 || program_change_param_id == -1); ++i) {
		edit_controller->getParameterInfo(i, pi);
		if (pi.flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsBypass)
			bypass_param_id = pi.id;
		else if (pi.flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsProgramChange) {
			program_change_param_id = pi.id;
			program_change_param_idx = i;
		}
	}

	// establish program count
	edit_controller->queryInterface(Steinberg::Vst::IUnitInfo::iid, reinterpret_cast<void**>(&unit_info));
	if (unit_info) {
		const auto program_list_count = unit_info->getProgramListCount();
		Steinberg::Vst::ProgramListInfo prog_list{};
		if (program_list_count == 0 && unit_info->getProgramListInfo(0, prog_list) == Steinberg::kResultOk)
			program_count = prog_list.programCount;
		else if (program_list_count > 0) {
			Steinberg::Vst::UnitInfo unit{};
			Steinberg::int32 i = 0;
			while (i < unit_info->getUnitCount() && unit_info->getUnitInfo(i, unit) == Steinberg::kResultTrue && unit.id != Steinberg::Vst::kRootUnitId)
				++i; // there has got to be a root unit id if getUnitCount returns more than zero
			program_list_root = unit.programListId;
			i = 0;
			while (i < program_list_count && unit_info->getProgramListInfo(i, prog_list) == Steinberg::kResultTrue) {
				if (prog_list.id == program_list_root) {
					program_count = prog_list.programCount;
					break;
				}
				++i;
			}
		}
	}

	// setup audio stuff
	Steinberg::Vst::ProcessSetup ps{};
	if (audio->canProcessSampleSize(Steinberg::Vst::kSample32) == Steinberg::kResultOk)
		ps.symbolicSampleSize = Steinberg::Vst::kSample32;
	else
		ps.symbolicSampleSize = Steinberg::Vst::kSample64;
	ps.processMode = Steinberg::Vst::kRealtime;
	ps.sampleRate = sample_rate;
	ps.maxSamplesPerBlock = block_size;
	audio->setupProcessing(ps);

	// activate buses
	Steinberg::Vst::BusInfo bi;
	Steinberg::Vst::MediaType mt = Steinberg::Vst::MediaTypes::kAudio;
	for (Steinberg::int32 i = 0; i < processor_component->getBusCount(mt, Steinberg::Vst::BusDirections::kInput); ++i) {
		processor_component->getBusInfo(mt, Steinberg::Vst::BusDirections::kInput, i, bi);
		if (bi.channelCount == GetChannelCount() && bi.busType == Steinberg::Vst::BusTypes::kMain) {
			processor_component->activateBus(mt, Steinberg::Vst::BusDirections::kInput, i, true);
			break;
		}
	}
	for (Steinberg::int32 i = 0; i < processor_component->getBusCount(mt, Steinberg::Vst::BusDirections::kOutput); ++i) {
		processor_component->getBusInfo(mt, Steinberg::Vst::BusDirections::kOutput, i, bi);
		if (bi.channelCount == GetChannelCount() && bi.busType == Steinberg::Vst::BusTypes::kMain) {
			processor_component->activateBus(mt, Steinberg::Vst::BusDirections::kOutput, i, true);
			break;
		}
	}

	// setup process data
	if (audio->canProcessSampleSize(Steinberg::Vst::kSample32) == Steinberg::kResultOk)
		pd.symbolicSampleSize = Steinberg::Vst::kSample32;
	else
		pd.symbolicSampleSize = Steinberg::Vst::kSample64;
	pd.numSamples = block_size;
	pd.processMode = Steinberg::Vst::kRealtime;
	pd.numInputs = 1;
	pd.numOutputs = 1;
	bus_in = std::make_unique<Steinberg::Vst::AudioBusBuffers>(Steinberg::Vst::AudioBusBuffers());
	bus_out = std::make_unique<Steinberg::Vst::AudioBusBuffers>(Steinberg::Vst::AudioBusBuffers());
	bus_in->numChannels = GetChannelCount();
	bus_out->numChannels = GetChannelCount();
	pd.inputs = bus_in.get();
	pd.outputs = bus_out.get();
	pd.inputEvents = nullptr;
	pd.outputEvents = nullptr;
	pd.processContext = nullptr;
	// create parameter changes objects
	pc_in = std::make_unique<ParameterChanges>(ParameterChanges(edit_controller));
	pc_out = std::make_unique<ParameterChanges>(ParameterChanges(edit_controller));
	pd.inputParameterChanges = pc_in.get();
	pd.outputParameterChanges = pc_out.get();

	SetActive(true);

	// synchronize controller and processor
	processor_component->queryInterface(Steinberg::Vst::IConnectionPoint::iid, (void**)&connection_point_component);
	edit_controller->queryInterface(Steinberg::Vst::IConnectionPoint::iid, (void**)&connection_point_controller);
	if (connection_point_component && connection_point_controller) {
		connection_point_component->connect(connection_point_controller);
		connection_point_controller->connect(connection_point_component);
	}
	
	Steinberg::MemoryStream stream;
	if (processor_component->getState(&stream) == Steinberg::kResultTrue) {
		stream.seek(0, Steinberg::IBStream::kIBSeekSet, 0);
		edit_controller->setComponentState(&stream);
	}
	
	// create plugin state module
	state = std::unique_ptr<Preset>(new PresetVST3(*this));

	// check if plugin has editor and remember it
	plugin_view = edit_controller->createView(Steinberg::Vst::ViewType::kEditor);
	has_editor = plugin_view != nullptr;
}

std::basic_string<TCHAR> PluginVST3::GetPluginName() const {
	Steinberg::PClassInfo ci;
	factory->getClassInfo(class_index, &ci); 
	std::string tmp(ci.name); // for some reason ci.name is single byte ASCII too
	return std::basic_string<TCHAR>(tmp.begin(), tmp.end());
}

std::string PluginVST3::GetPluginNameA() const {
	Steinberg::PClassInfo ci;
	factory->getClassInfo(class_index, &ci);
	return std::string(ci.name);
}

void PluginVST3::Process(Steinberg::Vst::Sample32** input, Steinberg::Vst::Sample32** output, Steinberg::Vst::TSamples block_size) {
	if (IsActive() && !BypassProcess()) {
		std::lock_guard<std::mutex> lock(plugin_lock);
		pd.inputs->channelBuffers32 = input;
		pd.outputs->channelBuffers32 = output;
		pd.numSamples = block_size;
		{
			std::lock_guard<std::mutex> lock(queue_lock);
			if (current_queue && current_queue->GetIndex() == -1)
				pd.inputParameterChanges->addParameterData(current_queue->getParameterId(), current_param_idx);
		}
		audio->process(pd);
		if (!bypass) {
			ProcessOutputParameterChanges();
			dynamic_cast<ParameterChanges*>(pd.inputParameterChanges)->ClearQueue();
			//offset = 0;
		}
	}
	else {
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(static_cast<void*>(output[i]), static_cast<void*>(input[i]), sizeof(input[0][0]) * block_size);
	}
}

void PluginVST3::SetBlockSize(Steinberg::Vst::TSamples bs) {
	bool was_active;
	if (was_active = IsActive())
		SetActive(false);
	block_size = bs;
	Steinberg::Vst::ProcessSetup ps;
	ps.symbolicSampleSize = Steinberg::Vst::kSample32;
	ps.processMode = Steinberg::Vst::kRealtime;
	ps.sampleRate = sample_rate;
	ps.maxSamplesPerBlock = block_size;
	audio->setupProcessing(ps);
	pd.numSamples = block_size;
	if (was_active)
		SetActive(true);
}

void PluginVST3::SetSampleRate(Steinberg::Vst::SampleRate sr) {
	bool was_active;
	if (was_active = IsActive())
		SetActive(false);
	sample_rate = sr;
	Steinberg::Vst::ProcessSetup ps;
	ps.symbolicSampleSize = Steinberg::Vst::kSample32;
	ps.processMode = Steinberg::Vst::kRealtime;
	ps.sampleRate = sample_rate;
	ps.maxSamplesPerBlock = block_size;
	audio->setupProcessing(ps);
	if (was_active)
		SetActive(true);
}

Steinberg::uint32 PluginVST3::GetProgramCount() const {
	return program_count;
}

void PluginVST3::SetProgram(Steinberg::uint32 id) {
	if (id < program_count && program_change_param_id != -1) {
		Steinberg::Vst::ParameterInfo param_info{};
		if (edit_controller->getParameterInfo(program_change_param_idx, param_info) == Steinberg::kResultTrue) {
			if (param_info.stepCount > 0 && id <= static_cast<Steinberg::uint32>(param_info.stepCount)) {
				auto value = static_cast<Steinberg::Vst::ParamValue>(id) / static_cast<Steinberg::Vst::ParamValue>(param_info.stepCount);
				SetParameter(program_change_param_id, value);
			}
		}
	}
}

std::basic_string<TCHAR> PluginVST3::GetProgramName(Steinberg::uint32 id) {
	Steinberg::Vst::ProgramListInfo list_info{};
	if (unit_info->getProgramListInfo(0, list_info) == Steinberg::kResultTrue) {
		Steinberg::Vst::String128 tmp = { 0 };
		if (unit_info->getProgramName(list_info.id, id, tmp) == Steinberg::kResultTrue) {
			Steinberg::String str(tmp);
#ifdef UNICODE
			return str.text16();
#else
			return str.text8();
#endif
		}
	}
	return std::basic_string<TCHAR>();
}

std::string PluginVST3::GetProgramNameA(Steinberg::uint32 id) {
	Steinberg::Vst::ProgramListInfo list_info{};
	if (unit_info->getProgramListInfo(0, list_info) == Steinberg::kResultTrue) {
		Steinberg::Vst::String128 tmp = { 0 };
		if (unit_info->getProgramName(list_info.id, id, tmp) == Steinberg::kResultTrue) {
			Steinberg::String str(tmp);
			return str.text8();
		}
	}
	return "";
}

std::string PluginVST3::GetPresetExtension() {
	return PresetVST3::kExtension;
}

Steinberg::uint32 PluginVST3::GetParameterCount() const {
	return edit_controller->getParameterCount();
}

Steinberg::Vst::ParamValue PluginVST3::GetParameter(Steinberg::Vst::ParamID id) const {
	return edit_controller->getParamNormalized(id);
}

void PluginVST3::SetParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) {
	beginEdit(id);
	edit_controller->setParamNormalized(id, value);
	performEdit(id, value);
	endEdit(id);
}

void PluginVST3::SetBypass(bool bypass_) {
	if (bypass != bypass_) {
		bypass = bypass_;
		if (bypass_param_id != -1) {
			Steinberg::Vst::ParamValue value = static_cast<Steinberg::Vst::ParamValue>(bypass);
			SetParameter(bypass_param_id, value);
		}
	}
}

bool PluginVST3::BypassProcess() const {			// wywolanie process omijaj tylko wtedy, jak
	return bypass && bypass_param_id == -1;	// bypass == true i nie znaleziono parametru "bypass"
}

bool PluginVST3::HasEditor() const {
	return has_editor;
}

void PluginVST3::CreateEditor(HWND hwnd) {
	if (HasEditor())
		plugin_view->attached(hwnd, Steinberg::kPlatformTypeHWND);
}

Steinberg::uint32 PluginVST3::GetEditorHeight() {
	if (HasEditor()) {
		Steinberg::ViewRect vr;
		plugin_view->getSize(&vr);
		return vr.getHeight();
	}
	return 0;
}

Steinberg::uint32 PluginVST3::GetEditorWidth() {
	if (HasEditor()) {
		Steinberg::ViewRect vr;
		plugin_view->getSize(&vr);
		return vr.getWidth();
	}
	return 0;
}


void PluginVST3::ShowEditor() {
	if (HasEditor())
		plugin_view->onFocus(true);
}

void PluginVST3::HideEditor() {
	if (HasEditor())
		plugin_view->onFocus(false);
}

Steinberg::tresult PLUGIN_API PluginVST3::beginEdit(Steinberg::Vst::ParamID id) {
	current_queue = dynamic_cast<ParameterChanges*>(pd.inputParameterChanges)->GetQueue(id);
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API PluginVST3::performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) {
	Steinberg::int32 index;
	if (current_queue)
		current_queue->addPoint(offset++, valueNormalized, index);
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API PluginVST3::endEdit(Steinberg::Vst::ParamID id) {
	if (current_queue) {
		pd.inputParameterChanges->addParameterData(id, current_param_idx);
		{
			std::lock_guard<std::mutex> lock(queue_lock);
			current_queue = nullptr;
			current_param_idx = -1;
		}
		offset = 0;
	}
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API PluginVST3::restartComponent(Steinberg::int32 flags) {
	return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API PluginVST3::queryInterface(const Steinberg::TUID _iid, void** obj) {
	QUERY_INTERFACE(_iid, obj, Steinberg::FUnknown::iid, Steinberg::Vst::IComponentHandler)
	QUERY_INTERFACE(_iid, obj, Steinberg::Vst::IComponentHandler::iid, Steinberg::Vst::IComponentHandler)
	*obj = 0;
	return Steinberg::kNoInterface;
}

Steinberg::uint32 PLUGIN_API PluginVST3::addRef() {
	return 1;
}

Steinberg::uint32 PLUGIN_API PluginVST3::release() {
	return 1;
}

void PluginVST3::Resume() {
	processor_component->setActive(true);
	StartProcessing();
	active = true;
}

void PluginVST3::Suspend() {
	StopProcessing();
	processor_component->setActive(false);
	active = false;
}

void PluginVST3::StartProcessing() {
	audio->setProcessing(true);
}

void PluginVST3::StopProcessing() {
	audio->setProcessing(false);
}

void PluginVST3::ProcessOutputParameterChanges() {
	auto pc = dynamic_cast<ParameterChanges*>(pd.outputParameterChanges);
	for (unsigned i = 0; i < static_cast<unsigned>(pc->getParameterCount()); ++i) {
		auto q = pc->getParameterData(i);
		Steinberg::Vst::ParamValue value;
		Steinberg::int32 offset;
		q->getPoint(q->getPointCount() - 1, offset, value);
		edit_controller->setParamNormalized(q->getParameterId(), value);
	}
	pc->ClearQueue();
}
} // namespace