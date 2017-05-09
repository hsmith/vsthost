#define NOMINMAX // kolizja makra MAX z windows.h oraz std::numeric_limits<T>::max()
#include "Host.h"

#include <limits>
#include <cstring>
#include <thread>
#include <array>

#include "base/source/fstring.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
DEF_CLASS_IID(Steinberg::Vst::IHostApplication)

#include "HostObserver.h"
#include "HostWindow.h"
#include "Plugin.h"
#include "PluginManager.h"

namespace VSTHost {
class Host::HostImpl : public HostSubject, public Steinberg::Vst::IHostApplication {
	friend class HostController;
public:
	HostImpl(std::int64_t block_size, double sample_rate)
	: block_size(block_size), sample_rate(sample_rate), plugins(block_size, sample_rate, UnknownCast()) {
		AllocateBuffers();
	}

	~HostImpl() {

	}

	void Process(float** input, float** output, std::int64_t block_size) {
		std::lock_guard<std::mutex> lock(plugins.GetLock());
		if (plugins.Size() == 1) {
			plugins.Front().Process(input, output, block_size);
		}
		else if (plugins.Size() > 1) {
			plugins.Front().Process(input, buffers[1], block_size);
			unsigned i, last_processed = 1;
			for (i = 1; i < plugins.Size() - 1; i++) {
				last_processed = (i + 1) % 2;
				plugins[i].Process(buffers[i % 2], buffers[last_processed], block_size);
			}
			plugins.Back().Process(buffers[last_processed], output, block_size);
		}
		else {
			for (unsigned i = 0; i < GetChannelCount(); ++i) {
				std::memcpy(output[i], input[i], sizeof(input[0][0]) * block_size);
			}
		}
	}

	void Process(char* input, char* output, std::int64_t block_size) {
		Process(reinterpret_cast<std::int16_t*>(input), reinterpret_cast<std::int16_t*>(output), block_size);
	}

	void Process(std::int16_t* input, std::int16_t* output, std::int64_t block_size) {
		std::lock_guard<std::mutex> lock(plugins.GetLock());
		if (plugins.Size() == 0) {
			std::memcpy(output, input, block_size * 2 * GetChannelCount());
			return;
		}
		ConvertFrom16Bits(input, buffers[0]);
		if (plugins.Size() == 1) {
			plugins.Front().Process(buffers[0], buffers[1], block_size);
			ConvertTo16Bits(buffers[1], output);
		}
		else if (plugins.Size() > 1) {
			unsigned last_processed = 0;			// remember where the most recently processed buffer is,
			for (unsigned i = 0; i < plugins.Size(); ++i) {	// so that it could be moved to output.
				last_processed = (i + 1) % 2;
				plugins[i].Process(buffers[i % 2], buffers[last_processed], block_size);
			}
			ConvertTo16Bits(buffers[last_processed], output);
		}
	}

	void SetSampleRate(Steinberg::Vst::SampleRate sr) {
		sample_rate = sr;
		plugins.SetSampleRate(sample_rate);
		for (decltype(plugins.Size()) i = 0; i < plugins.Size(); ++i)
			plugins.GetAt(i).SetSampleRate(sample_rate);
	}

	void SetBlockSize(Steinberg::Vst::TSamples bs) {
		if (bs != block_size) {
			block_size = bs;
			plugins.SetBlockSize(block_size);
			AllocateBuffers();
			for (decltype(plugins.Size()) i = 0; i < plugins.Size(); ++i)
				plugins[i].SetBlockSize(block_size);
		}
	}

	void CreateGUIThread(IHostController* hc) {
		std::thread gui_thread(&Host::HostImpl::CreateGUI, this, hc);
		gui_thread.detach();
	}

	void CreateGUI(IHostController* hc) {
		gui.reset(new HostWindow(hc));
		gui->Initialize(NULL);
		gui->Go();
	}

	bool LoadPluginList(const std::string& path) {
		if (plugins.LoadPluginList(path)) {
			Notify(HostEvent::ListLoaded);
			return true;
		}
		return false;
	}

	bool SavePluginList(const std::string& path) const {
		return plugins.SavePluginList(path);
	}

	bool LoadPluginList() {
		if (plugins.LoadPluginList()) {
			Notify(HostEvent::ListLoaded);
			return true;
		}
		return false;
	}

	bool SavePluginList() const {
		return plugins.SavePluginList();
	}

	Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override {
		Steinberg::String str("VSTHost");
		str.copyTo16(name, 0, 127);
		return Steinberg::kResultTrue;
	}

	Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid, Steinberg::TUID iid, void** obj) override {
		*obj = nullptr;
		return Steinberg::kResultFalse;
	}

	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override {
		QUERY_INTERFACE(_iid, obj, Steinberg::FUnknown::iid, Steinberg::Vst::IHostApplication)
		QUERY_INTERFACE(_iid, obj, Steinberg::Vst::IHostApplication::iid, Steinberg::Vst::IHostApplication)
		*obj = 0;
		return Steinberg::kNoInterface;
	}

	Steinberg::uint32 PLUGIN_API addRef() override {
		return 1;
	}

	Steinberg::uint32 PLUGIN_API release() override {
		return 1;
	}

private:
	// for hostcontroller and observability
	std::uint32_t GetPluginCount() const {
		return static_cast<std::uint32_t>(plugins.Size());
	}

	bool AddPlugin(const std::string& path) {
		if (plugins.Add(path)) {
			Notify(HostEvent::Added, static_cast<std::uint32_t>(plugins.Size() - 1));
			return true;
		}
		return false;
	}

	void DeletePlugin(std::uint32_t idx) {
		if (idx < plugins.Size()) {
			plugins.Delete(idx);
			Notify(HostEvent::Deleted, idx);
		}
	}

	void MoveUp(std::uint32_t idx) {
		if (idx < plugins.Size() && idx > 0) {
			plugins.Swap(idx, idx - 1);
			Notify(HostEvent::MovedUp, idx);
		}
	}

	void MoveDown(std::uint32_t idx) {
		if (plugins.Size() > 0 && idx < plugins.Size() - 1) {
			plugins.Swap(idx, idx + 1);
			Notify(HostEvent::MovedDown, idx);
		}
	}

	std::string GetPluginName(std::uint32_t idx) const {
		if (idx < plugins.Size())
			return plugins[idx].GetPluginNameA();
		return "";
	}

	bool HasEditor(std::uint32_t idx) const {
		if (idx < plugins.Size())
			return plugins[idx].HasEditor();
		return false;
	}

	void CreateEditor(std::uint32_t idx, HWND hwnd) {
		if (idx < plugins.Size())
			plugins[idx].CreateEditor(hwnd);
	}

	std::uint32_t GetPluginEditorHeight(std::uint32_t idx) {
		if (idx < plugins.Size())
			return plugins[idx].GetEditorHeight();
		else
			return 0;
	}

	std::uint32_t GetPluginEditorWidth(std::uint32_t idx) {
		if (idx < plugins.Size())
			return plugins[idx].GetEditorWidth();
		else
			return 0;
	}

	void ShowEditor(std::uint32_t idx) {
		if (idx < plugins.Size()) {
			plugins[idx].ShowEditor();
			Notify(HostEvent::EditorShown, idx);
		}
	}

	void HideEditor(std::uint32_t idx) {
		if (idx < plugins.Size()) {
			plugins[idx].HideEditor();
			Notify(HostEvent::EditorHidden, idx);
		}
	}

	bool IsBypassed(std::uint32_t idx) const {
		if (idx < plugins.Size())
			return plugins[idx].IsBypassed();
		return false;
	}

	void SetBypass(std::uint32_t idx, bool bypass) {
		if (idx < plugins.Size()) {
			plugins[idx].SetBypass(bypass);
			Notify(HostEvent::BypassSet, idx, bypass);
		}
	}

	bool IsActive(std::uint32_t idx) const {
		if (idx < plugins.Size())
			return plugins[idx].IsActive();
		return false;
	}

	void SetActive(std::uint32_t idx, bool active) {
		if (idx < plugins.Size()) {
			plugins[idx].SetActive(active);
			Notify(HostEvent::ActiveSet, idx, active);
		}
	}

	std::uint32_t GetPluginPresetCount(std::uint32_t idx) const {
		if (idx < plugins.Size())
			return plugins[idx].GetProgramCount();
		return 0;
	}

	std::string GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
		if (plugin_idx < plugins.Size() && preset_idx < plugins[plugin_idx].GetProgramCount())
			return plugins[plugin_idx].GetProgramNameA(preset_idx);
		return "";
	}

	void SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
		if (plugin_idx < plugins.Size() && preset_idx < plugins[plugin_idx].GetProgramCount()) {
			plugins[plugin_idx].SetProgram(preset_idx);
			Notify(HostEvent::PresetSet, plugin_idx, preset_idx);
		}
	}

	bool SavePreset(std::uint32_t idx) {
		if (idx < plugins.Size())
			return plugins[idx].SaveState();
		return false;
	}

	bool LoadPreset(std::uint32_t idx) {
		if (idx < plugins.Size() && plugins[idx].LoadState()) {
			Notify(HostEvent::StateLoaded, idx);
			return true;
		}
		return false;
	}

	bool SavePreset(std::uint32_t idx, const std::string& path) {
		if (idx < plugins.Size())
			return plugins[idx].SaveState(path);
		return false;
	}

	bool LoadPreset(std::uint32_t idx, const std::string& path) {
		if (idx < plugins.Size() && plugins[idx].LoadState(path)) {
			Notify(HostEvent::StateLoaded, idx);
			return true;
		}
		return false;
	}

	void AllocateBuffers() {
		unsigned i = 0;
		for (auto& buffer : smart_buffers) {
			unsigned j = 0;
			for (auto& channel : buffer) {
				channel.reset(new Steinberg::Vst::Sample32[block_size]);
				buffers[i][j++] = channel.get();
			}
			++i;
		}
	}

	void ConvertFrom16Bits(std::int8_t* input, float** output) {
		ConvertFrom16Bits(reinterpret_cast<std::int16_t*>(input), output);
	}

	void ConvertFrom16Bits(std::int16_t* input, float** output) {
		for (unsigned i = 0, in_i = 0; i < block_size; ++i)
			for (unsigned j = 0; j < GetChannelCount(); ++j, ++in_i)
				output[j][i] = input[in_i] / static_cast<float>(std::numeric_limits<std::int16_t>::max());
	}

	void ConvertTo16Bits(float** input, std::int8_t* output) {
		ConvertTo16Bits(input, reinterpret_cast<std::int16_t*>(output));
	}

	void ConvertTo16Bits(float** input, std::int16_t* output) {
		for (unsigned i = 0, out_i = 0; i < block_size; ++i)
			for (unsigned j = 0; j < GetChannelCount(); ++j, ++out_i)
				if (input[j][i] > 1)
					output[out_i] = std::numeric_limits<std::int16_t>::max();
				else if (input[j][i] < -1)
					output[out_i] = std::numeric_limits<std::int16_t>::min();
				else
					output[out_i] = static_cast<std::int16_t>(input[j][i] * std::numeric_limits<std::int16_t>::max());
	}

	Steinberg::FUnknown* UnknownCast() {
		return static_cast<Steinberg::FUnknown *>(this);
	}

	constexpr static Steinberg::uint32 channels = 2u;
	constexpr static Steinberg::uint32 GetChannelCount() {
		return channels; // only stereo
	}


	PluginManager plugins;
	std::unique_ptr<HostWindow> gui;
	std::thread ui_thread;
	std::mutex processing_lock;

	Steinberg::Vst::TSamples block_size;
	Steinberg::Vst::SampleRate sample_rate;
	Steinberg::Vst::Sample32* buffers[2][channels /*GetChannelCount()*/]{};
	std::array<std::array<std::unique_ptr<Steinberg::Vst::Sample32[]>, channels /*GetChannelCount()*/>, 2> smart_buffers;
};

Host::Host(std::int64_t max_num_samples, double sample_rate) : impl(new Host::HostImpl(max_num_samples, sample_rate)) {

}

Host::~Host() {

}

void Host::Process(float** input, float** output, std::int64_t num_samples) {
	impl->Process(input, output, num_samples);
}

void Host::Process(char* input, char* output, std::int64_t num_samples) {
	impl->Process(input, output, num_samples);
}

void Host::Process(std::int16_t* input, std::int16_t* output, std::int64_t num_samples) {
	impl->Process(input, output, num_samples);
}

void Host::SetSampleRate(double sr) {
	impl->SetSampleRate(sr);
}

void Host::SetBlockSize(std::int64_t bs) {
	impl->SetSampleRate(bs);
}

void Host::CreateGUIThread() {
	impl->CreateGUIThread(GetController());
}

void Host::CreateGUI() {
	impl->CreateGUI(GetController());
}

bool Host::LoadPluginList(const std::string& path) {
	return impl->LoadPluginList(path);
}

bool Host::SavePluginList(const std::string& path) const {
	return impl->SavePluginList(path);
}

bool Host::LoadPluginList() {
	return impl->LoadPluginList();
}

bool Host::SavePluginList() const {
	return impl->SavePluginList();
}

class HostController : public IHostController {
public:
	bool LoadPluginList(const std::string& path) override;
	bool SavePluginList(const std::string& path) const override;
	bool LoadPluginList() override;
	bool SavePluginList() const override;
	std::uint32_t GetPluginCount() const override;
	bool AddPlugin(const std::string& path) override;
	void DeletePlugin(std::uint32_t idx) override;
	void MoveUp(std::uint32_t idx) override;
	void MoveDown(std::uint32_t idx) override;
	std::string GetPluginName(std::uint32_t idx) const override;
	bool HasEditor(std::uint32_t idx) const override;
	void CreateEditor(std::uint32_t idx, HWND hwnd) override;
	std::uint32_t GetPluginEditorHeight(std::uint32_t idx) override;
	std::uint32_t GetPluginEditorWidth(std::uint32_t idx) override;
	void ShowEditor(std::uint32_t idx) override;
	void HideEditor(std::uint32_t idx) override;
	bool IsBypassed(std::uint32_t idx) const override;
	void SetBypass(std::uint32_t idx, bool bypass) override;
	bool IsActive(std::uint32_t idx) const override;
	void SetActive(std::uint32_t idx, bool active) override;
	std::uint32_t GetPluginPresetCount(std::uint32_t idx) const override;
	std::string GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx) override;
	void SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx) override;
	std::string HostController::GetPluginPresetExtension(std::uint32_t idx) override;
	bool SavePreset(std::uint32_t idx) override;
	bool LoadPreset(std::uint32_t idx) override;
	bool SavePreset(std::uint32_t idx, const std::string& path) override;
	bool LoadPreset(std::uint32_t idx, const std::string& path) override;
	void RegisterObserver(HostObserver* o) override;
	void UnregisterObserver(HostObserver* o) override;
private:
	friend IHostController* Host::GetController();
	HostController(std::shared_ptr<Host::HostImpl>& impl);

	std::shared_ptr<Host::HostImpl> host;
};

HostController::HostController(std::shared_ptr<Host::HostImpl>& impl) : host(impl) {

}

bool HostController::LoadPluginList(const std::string& path) {
	return host->LoadPluginList(path);
}

bool HostController::SavePluginList(const std::string& path) const {
	return host->SavePluginList(path);
}

bool HostController::LoadPluginList() {
	return host->LoadPluginList();
}

bool HostController::SavePluginList() const {
	return host->SavePluginList();
}

std::uint32_t HostController::GetPluginCount() const {
	return host->GetPluginCount();
}

bool HostController::AddPlugin(const std::string& path) {
	return host->AddPlugin(path);
}

void HostController::DeletePlugin(std::uint32_t idx) {
	return host->DeletePlugin(idx);
}

void HostController::MoveUp(std::uint32_t idx) {
	return host->MoveUp(idx);
}

void HostController::MoveDown(std::uint32_t idx) {
	return host->MoveDown(idx);
}

std::string HostController::GetPluginName(std::uint32_t idx) const {
	return host->GetPluginName(idx);
}

bool HostController::HasEditor(std::uint32_t idx) const {
	return host->HasEditor(idx);
}

void HostController::CreateEditor(std::uint32_t idx, HWND hwnd) {
	return host->CreateEditor(idx, hwnd);
}

std::uint32_t HostController::GetPluginEditorHeight(std::uint32_t idx) {
	return host->GetPluginEditorHeight(idx);
}

std::uint32_t HostController::GetPluginEditorWidth(std::uint32_t idx) {
	return host->GetPluginEditorWidth(idx);
}

void HostController::ShowEditor(std::uint32_t idx) {
	return host->ShowEditor(idx);
}

void HostController::HideEditor(std::uint32_t idx) {
	return host->HideEditor(idx);
}

bool HostController::IsBypassed(std::uint32_t idx) const {
	return host->IsBypassed(idx);
}

void HostController::SetBypass(std::uint32_t idx, bool bypass) {
	return host->SetBypass(idx, bypass);
}

bool HostController::IsActive(std::uint32_t idx) const {
	return host->IsActive(idx);
}

void HostController::SetActive(std::uint32_t idx, bool active) {
	return host->SetActive(idx, active);
}

std::uint32_t HostController::GetPluginPresetCount(std::uint32_t idx) const {
	return host->GetPluginPresetCount(idx);
}

std::string HostController::GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	return host->GetPluginPresetName(plugin_idx, preset_idx);
}

void HostController::SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	return host->SetPluginPreset(plugin_idx, preset_idx);
}

std::string HostController::GetPluginPresetExtension(std::uint32_t idx) {
	return host->plugins[idx].GetPresetExtension();
}

bool HostController::SavePreset(std::uint32_t idx) {
	return host->SavePreset(idx);
}

bool HostController::LoadPreset(std::uint32_t idx) {
	return host->LoadPreset(idx);
}

bool HostController::SavePreset(std::uint32_t idx, const std::string& path) {
	return host->SavePreset(idx, path);
}

bool HostController::LoadPreset(std::uint32_t idx, const std::string& path) {
	return host->LoadPreset(idx, path);
}

void HostController::RegisterObserver(HostObserver* o) {
	return host->Register(o);
}

void HostController::UnregisterObserver(HostObserver* o) {
	return host->Unregister(o);
}

IHostController* Host::GetController() {
	return new HostController(impl);
}
} // namespace