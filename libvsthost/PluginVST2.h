#ifndef PLUGINVST2_H
#define PLUGINVST2_H

#ifndef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 0
#endif

#include "pluginterfaces\base\fplatform.h"
#include "pluginterfaces\vst2.x\aeffect.h"

#include "Plugin.h"
#include "PluginLoader.h"

struct VstSpeakerArrangement;
enum VstSpeakerType;
namespace VSTHost {
class PresetVST3;
class PluginVST2 : public Plugin {
	friend class PresetVST2;
	friend std::unique_ptr<Plugin> PluginLoader::Load(const std::string& path, Steinberg::FUnknown* context, Steinberg::Vst::SpeakerArrangement sa);
	PluginVST2(HMODULE m, AEffect* p, Steinberg::Vst::SpeakerArrangement sa);
public:
	~PluginVST2();
	// basic plugin interface
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
	std::string GetPresetExtension() override;
	// parameters
	Steinberg::uint32 GetParameterCount() const override;
	Steinberg::Vst::ParamValue GetParameter(Steinberg::Vst::ParamID id) const override;
	void SetParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) override;
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
	// vst2 callback procedure wrapper
	static VstIntPtr VSTCALLBACK HostCallbackWrapper(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
private:
	void Resume() override;
	void Suspend() override;
	void StartProcessing() override;
	void StopProcessing() override;
	// vst2 specific
	HWND hwnd; // editor windows handle
	VstIntPtr VSTCALLBACK Dispatcher(VstInt32 opcode, VstInt32 index = 0, VstIntPtr value = 0, void* ptr = nullptr, float opt = 0.);
	VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
	bool CanDo(const char* canDo) const;
	Steinberg::int32 GetVendorVersion() const;
	Steinberg::int32 GetVSTVersion() const;
	Steinberg::int32 GetFlags() const;
	VstInt32 GetVST2SpeakerArrangementType(Steinberg::Vst::SpeakerArrangement sa);
	VstSpeakerType GetVST2SpeakerType(Steinberg::Vst::Speaker speaker);
	void GetVST2SpeakerArrangement(VstSpeakerArrangement& sa_vst2, Steinberg::Vst::SpeakerArrangement sa_vst3);
	// soft bypass
	bool soft_bypass{ false };
	std::unique_ptr<AEffect> plugin;
};
} // namespace

#endif