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
	HostImpl(std::int64_t block_size, double sample_rate, SpeakerArrangement sa)
	: block_size(block_size)
	, sample_rate(sample_rate)
	, speaker_arrangement(GetVSTSpeakerArrangement(sa))
	, plugins(block_size, sample_rate, speaker_arrangement, UnknownCast()) {
		AllocateBuffers();
	}

	~HostImpl() {

	}

	void Process(float** input, float** output, std::int64_t block_size) {
		auto queue = plugins.GetQueue();
		if (queue.size()) {
			std::lock_guard<std::mutex> lock(plugins.GetLock());
			if (queue.size() == 1) {
				queue.front()->Process(input, output, block_size);
			}
			else if (queue.size() > 1) {
				queue.front()->Process(input, buffers[1], block_size);
				unsigned i, last_processed = 1;
				for (i = 1; i < queue.size() - 1; i++) {
					last_processed = (i + 1) % 2;
					queue[i]->Process(buffers[i % 2], buffers[last_processed], block_size);
				}
				queue.back()->Process(buffers[last_processed], output, block_size);
			}
		}
		else
			for (unsigned i = 0; i < GetChannelCount(); ++i)
				std::memcpy(output[i], input[i], sizeof(input[0][0]) * block_size);
	}

	void ProcessReplace(float** input_output, std::int64_t block_size) {
		auto queue = plugins.GetQueue();
		if (queue.size()) {
			std::lock_guard<std::mutex> lock(plugins.GetLock());
			for (auto &p : queue)
				p->Process(input_output, input_output, block_size);
		}
	}

	void Process(char* input, char* output, std::int64_t block_size) {
		Process(reinterpret_cast<std::int16_t*>(input), reinterpret_cast<std::int16_t*>(output), block_size);
	}

	void ProcessReplace(char* input_output, std::int64_t block_size) {
		ProcessReplace(reinterpret_cast<std::int16_t*>(input_output), block_size);
	}

	void Process(std::int16_t* input, std::int16_t* output, std::int64_t block_size) {
		auto queue = plugins.GetQueue();
		if (queue.size()) {
			ConvertFrom16Bits(input, buffers[0]);
			ProcessReplace(buffers[0], block_size);
			ConvertTo16Bits(buffers[0], output);
		}
		else
			std::memcpy(output, input, block_size * 2 * GetChannelCount());
	}

	void ProcessReplace(std::int16_t* input_output, std::int64_t block_size) {
		auto queue = plugins.GetQueue();
		if (queue.size()) {
			ConvertFrom16Bits(input_output, buffers[0]);
			ProcessReplace(buffers[0], block_size);
			ConvertTo16Bits(buffers[0], input_output);
		}
	}

	void SetSampleRate(Steinberg::Vst::SampleRate sr) {
		if (sr != sample_rate) {
			sample_rate = sr;
			plugins.SetSampleRate(sample_rate);
		}
	}

	void SetBlockSize(Steinberg::Vst::TSamples bs) {
		if (bs != block_size) {
			block_size = bs;
			std::lock_guard<std::mutex> lock(plugins.GetLock());
			plugins.SetBlockSize(block_size);
			AllocateBuffers();
		}
	}

	Steinberg::Vst::SpeakerArrangement GetVSTSpeakerArrangement(SpeakerArrangement sa) {
		switch (sa) {
		case SpeakerArrangement::Mono: return Steinberg::Vst::SpeakerArr::kMono;
		case SpeakerArrangement::Stereo: return Steinberg::Vst::SpeakerArr::kStereo;
		case SpeakerArrangement::Surround_5_1: return Steinberg::Vst::SpeakerArr::k51;
		case SpeakerArrangement::Surround_7_1: return Steinberg::Vst::SpeakerArr::k71Cine; // ?
		default: return Steinberg::Vst::SpeakerArr::kEmpty;
		}
	}

	void SetSpeakerArrangement(SpeakerArrangement sa) {
		auto sa_vst = GetVSTSpeakerArrangement(sa);
		if (sa_vst != speaker_arrangement) {
			speaker_arrangement = sa_vst;
			std::lock_guard<std::mutex> lock(plugins.GetLock());
			plugins.SetSpeakerArrangement(sa_vst);
			AllocateBuffers();
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
			if (IsActive(idx))
				bypass ? plugins.RemoveFromQueue(idx) : plugins.AddToQueue(idx);
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
			if (!IsBypassed(idx) && active)
				plugins.AddToQueue(idx);
			else if (IsBypassed(idx) && !active)
				plugins.RemoveFromQueue(idx);
			active && !IsBypassed(idx) ? plugins.AddToQueue(idx) : plugins.RemoveFromQueue(idx);
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
		for (auto i = 0u; i < 2; ++i) {
			smart_buffers[i].clear();
			buffers_ptrs[i].clear();
			for (auto j = 0u; j < GetChannelCount(); ++j) {
				smart_buffers[i].push_back(std::move(std::make_unique<Steinberg::Vst::Sample32[]>(block_size)));
				buffers_ptrs[i].push_back(smart_buffers[i][j].get());
			}
			buffers[i] = buffers_ptrs[i].data();
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

	PluginManager plugins;
	std::unique_ptr<HostWindow> gui;
	std::thread ui_thread;
	std::mutex processing_lock;

	Steinberg::Vst::TSamples block_size;
	Steinberg::Vst::SampleRate sample_rate;
	Steinberg::Vst::SpeakerArrangement speaker_arrangement;

	Steinberg::uint32 GetChannelCount() {
		return static_cast<Steinberg::uint32>(Steinberg::Vst::SpeakerArr::getChannelCount(speaker_arrangement));
	}

	Steinberg::Vst::Sample32** buffers[2]{};
	std::vector<Steinberg::Vst::Sample32*> buffers_ptrs[2];
	std::vector<std::unique_ptr<Steinberg::Vst::Sample32[]>> smart_buffers[2];
};

Host::Host(std::int64_t max_num_samples, double sample_rate, SpeakerArrangement sa) 
	: impl(new Host::HostImpl(max_num_samples, sample_rate, sa)) {

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

void Host::ProcessReplace(float** input_output, std::int64_t num_samples) {
	impl->ProcessReplace(input_output, num_samples);
}

void Host::ProcessReplace(char* input_output, std::int64_t num_samples) {
	impl->ProcessReplace(input_output, num_samples);
}

void Host::ProcessReplace(std::int16_t* input_output, std::int64_t num_samples) {
	impl->ProcessReplace(input_output, num_samples);
}

void Host::SetSampleRate(double sr) {
	impl->SetSampleRate(sr);
}

void Host::SetBlockSize(std::int64_t bs) {
	impl->SetBlockSize(bs);
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

	std::weak_ptr<Host::HostImpl> host_weak;
};

HostController::HostController(std::shared_ptr<Host::HostImpl>& impl) 
	: host_weak(impl) {

}

bool HostController::LoadPluginList(const std::string& path) {
	if (auto host = host_weak.lock())
		return host->LoadPluginList(path);
	return false;
}

bool HostController::SavePluginList(const std::string& path) const {
	if (auto host = host_weak.lock())
		return host->SavePluginList(path);
	return false;
}

bool HostController::LoadPluginList() {
	if (auto host = host_weak.lock())
		return host->LoadPluginList();
	return false;
}

bool HostController::SavePluginList() const {
	if (auto host = host_weak.lock())
		return host->SavePluginList();
	return false;
}

std::uint32_t HostController::GetPluginCount() const {
	if (auto host = host_weak.lock())
		return host->GetPluginCount();
	return 0;
}

bool HostController::AddPlugin(const std::string& path) {
	if (auto host = host_weak.lock())
		return host->AddPlugin(path);
	return false;
}

void HostController::DeletePlugin(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->DeletePlugin(idx);
}

void HostController::MoveUp(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->MoveUp(idx);
}

void HostController::MoveDown(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->MoveDown(idx);
}

std::string HostController::GetPluginName(std::uint32_t idx) const {
	if (auto host = host_weak.lock())
		return host->GetPluginName(idx);
	return "";
}

bool HostController::HasEditor(std::uint32_t idx) const {
	if (auto host = host_weak.lock())
		return host->HasEditor(idx);
	return false;
}

void HostController::CreateEditor(std::uint32_t idx, HWND hwnd) {
	if (auto host = host_weak.lock())
		return host->CreateEditor(idx, hwnd);
}

std::uint32_t HostController::GetPluginEditorHeight(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->GetPluginEditorHeight(idx);
	return 0;
}

std::uint32_t HostController::GetPluginEditorWidth(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->GetPluginEditorWidth(idx);
	return 0;
}

void HostController::ShowEditor(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->ShowEditor(idx);
}

void HostController::HideEditor(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->HideEditor(idx);
}

bool HostController::IsBypassed(std::uint32_t idx) const {
	if (auto host = host_weak.lock())
		return host->IsBypassed(idx);
	return false;
}

void HostController::SetBypass(std::uint32_t idx, bool bypass) {
	if (auto host = host_weak.lock())
		return host->SetBypass(idx, bypass);
}

bool HostController::IsActive(std::uint32_t idx) const {
	if (auto host = host_weak.lock())
		return host->IsActive(idx);
	return false;
}

void HostController::SetActive(std::uint32_t idx, bool active) {
	if (auto host = host_weak.lock())
		return host->SetActive(idx, active);
}

std::uint32_t HostController::GetPluginPresetCount(std::uint32_t idx) const {
	if (auto host = host_weak.lock())
		return host->GetPluginPresetCount(idx);
	return 0;
}

std::string HostController::GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	if (auto host = host_weak.lock())
		return host->GetPluginPresetName(plugin_idx, preset_idx);
	return "";
}

void HostController::SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	if (auto host = host_weak.lock())
		return host->SetPluginPreset(plugin_idx, preset_idx);
}

std::string HostController::GetPluginPresetExtension(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->plugins[idx].GetPresetExtension();
	return "";
}

bool HostController::SavePreset(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->SavePreset(idx);
	return false;
}

bool HostController::LoadPreset(std::uint32_t idx) {
	if (auto host = host_weak.lock())
		return host->LoadPreset(idx);
	return false;
}

bool HostController::SavePreset(std::uint32_t idx, const std::string& path) {
	if (auto host = host_weak.lock())
		return host->SavePreset(idx, path);
	return false;
}

bool HostController::LoadPreset(std::uint32_t idx, const std::string& path) {
	if (auto host = host_weak.lock())
		return host->LoadPreset(idx, path);
	return false;
}

void HostController::RegisterObserver(HostObserver* o) {
	if (auto host = host_weak.lock())
		return host->Register(o);
}

void HostController::UnregisterObserver(HostObserver* o) {
	if (auto host = host_weak.lock())
		return host->Unregister(o);
}

IHostController* Host::GetController() {
	return new HostController(impl);
}
} // namespace