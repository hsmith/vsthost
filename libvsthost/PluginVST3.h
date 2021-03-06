#ifndef PLUGINVST3_H
#define PLUGINVST3_H

#undef max
#include <limits>

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstunits.h"

#include "Plugin.h"
#include "PluginLoader.h"

namespace VSTHost {
class PresetVST3;
class Steinberg::IPluginFactory;
struct Steinberg::PClassInfo;
struct Steinberg::PClassInfo2;
class Steinberg::Vst::IComponent;
class Steinberg::Vst::IConnectionPoint;
class ParameterValueQueue;
class ParameterChanges;
class PluginVST3 : public Plugin, public Steinberg::Vst::IComponentHandler {
	friend class PresetVST3;
	friend std::unique_ptr<Plugin> PluginLoader::Load(const std::string& path, Steinberg::FUnknown* context,  Steinberg::Vst::SpeakerArrangement sa);
public:
	// basic plugin interface
	PluginVST3(HMODULE m, Steinberg::IPluginFactory* f, Steinberg::FUnknown* c, Steinberg::Vst::SpeakerArrangement sa);
	~PluginVST3();
	IsValidCodes IsValid() const override;
	void Initialize(Steinberg::Vst::TSamples bs, Steinberg::Vst::SampleRate sr) override;
	std::basic_string<TCHAR> GetPluginName() const override;
	std::string GetPluginNameA() const override;
	void Process(Steinberg::Vst::Sample32** input, Steinberg::Vst::Sample32** output, Steinberg::Vst::TSamples block_size) override;
	void ProcessReplace(Steinberg::Vst::Sample32** input_output, Steinberg::Vst::TSamples block_size) override;
	void SetBlockSize(Steinberg::Vst::TSamples bs) override;
	void SetSampleRate(Steinberg::Vst::SampleRate sr) override;
	bool SetSpeakerArrangement(Steinberg::Vst::SpeakerArrangement sa) override;
	// presets
	Steinberg::uint32 GetProgramCount() const override;
	void SetProgram(Steinberg::uint32 id) override;
	std::basic_string<TCHAR> GetProgramName(Steinberg::uint32 id) override;
	std::string GetProgramNameA(Steinberg::uint32 id) override;
	// parameters
	Steinberg::uint32 GetParameterCount() const override;
	Steinberg::Vst::ParamValue GetParameter(Steinberg::Vst::ParamID id) const override;
	void SetParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) override;
	std::string GetPresetExtension() override;
	// active and bypass flags
	void SetBypass(bool bypass_) override;
	bool BypassProcess() const override;
	// editor
	bool HasEditor() const override;
	void CreateEditor(HWND hwnd) override;
	Steinberg::uint32 GetEditorHeight() override;
	Steinberg::uint32 GetEditorWidth() override;
	void ShowEditor() override;
	void HideEditor() override;
	// vst3 interfaces specific
	Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override;
	Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override;
	Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override;
	Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32 flags) override;
	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override;
	Steinberg::uint32 PLUGIN_API addRef() override;
	Steinberg::uint32 PLUGIN_API release() override;
private:
	void Resume() override;
	void Suspend() override;
	void StartProcessing() override;
	void StopProcessing() override;

	Steinberg::FUnknown* context;

	// soft bypass
	Steinberg::Vst::ParamID bypass_param_id{ std::numeric_limits<std::uint32_t>::max() };
	// vst3 presets handling
	Steinberg::Vst::ParamID program_change_param_id{ std::numeric_limits<std::uint32_t>::max() };
	Steinberg::int32 program_change_param_idx{ -1 };
	Steinberg::Vst::IUnitInfo* unit_info{ nullptr };
	Steinberg::uint32 program_count{ 0 };
	Steinberg::Vst::ProgramListID program_list_root{ Steinberg::Vst::kNoProgramListId };
	// parameter changes
	void ProcessOutputParameterChanges();
	Steinberg::int32 current_param_idx, offset;
	ParameterValueQueue* current_queue;
	std::mutex queue_lock;
	Steinberg::IPlugView* plugin_view{ nullptr };
	// vst3 general
	Steinberg::int32 class_index; // index of the class produced by factory which is valid 
	Steinberg::IPluginFactory* factory;
	Steinberg::FUnknown* plugin;
	Steinberg::Vst::IComponent* processor_component;
	Steinberg::Vst::IEditController* edit_controller;
	bool processor_component_initialized{ false };
	bool edit_controller_initialized{ false };
	// separation between edit controller and processor component classes
	bool separated{ false };
	Steinberg::Vst::IConnectionPoint* connection_point_component{ nullptr };
	Steinberg::Vst::IConnectionPoint* connection_point_controller{ nullptr };
	// audio related
	Steinberg::Vst::IAudioProcessor* audio;
	std::unique_ptr<Steinberg::Vst::AudioBusBuffers> bus_in, bus_out;
	std::unique_ptr<ParameterChanges> pc_in, pc_out;
	Steinberg::Vst::ProcessData pd;
};
} // namespace

#endif