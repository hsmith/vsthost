#define NOMINMAX // kolizja makra MAX z windows.h oraz std::numeric_limits<T>::max()
#include "Host.h"

#include <limits>
#include <cstring>
#include <thread>

#ifndef UNICODE
#define UNICODE_OFF
#endif
#include "base/source/fstring.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
DEF_CLASS_IID(Steinberg::Vst::IHostApplication)

#include "HostWindow.h"
#include "Plugin.h"
#include "PluginManager.h"

namespace VSTHost {
class Host::HostImpl : Steinberg::Vst::IHostApplication {
public:
	HostImpl(std::int64_t block_size, double sample_rate)
		: block_size(block_size), sample_rate(sample_rate), plugins(block_size, sample_rate, UnknownCast()) {
		buffers[0] = nullptr;
		buffers[1] = nullptr;
		AllocateBuffers();
	}

	~HostImpl() {
		FreeBuffers();
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
			FreeBuffers();
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
		gui = std::unique_ptr<HostWindow>(new HostWindow(hc));
		gui->Initialize(NULL);
		gui->CreateEditors();
		gui->Go();
	}

	bool LoadPluginList(const std::string& path) {
		return plugins.LoadPluginList(path);
	}

	bool SavePluginList(const std::string& path) const {
		return plugins.SavePluginList(path);
	}

	bool LoadPluginList() {
		return plugins.LoadPluginList();
	}

	bool SavePluginList() const {
		return plugins.SavePluginList();
	}

	Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) {
		Steinberg::String str("VSTHost");
		str.copyTo16(name, 0, 127);
		return Steinberg::kResultTrue;
	}

	Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid, Steinberg::TUID iid, void** obj) {
		*obj = nullptr;
		return Steinberg::kResultFalse;
	}

	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) {
		QUERY_INTERFACE(_iid, obj, Steinberg::FUnknown::iid, Steinberg::Vst::IHostApplication)
		QUERY_INTERFACE(_iid, obj, Steinberg::Vst::IHostApplication::iid, Steinberg::Vst::IHostApplication)
		*obj = 0;
		return Steinberg::kNoInterface;
	}

	Steinberg::uint32 PLUGIN_API addRef() {
		return 1;
	}

	Steinberg::uint32 PLUGIN_API release() {
		return 1;
	}

//private:
	Steinberg::uint32 GetChannelCount() const {
		return 2; // only stereo
	}

	void AllocateBuffers() {
		for (auto &b : buffers) {
			if (b)
				FreeBuffers();
			b = new Steinberg::Vst::Sample32*[GetChannelCount()]{};
			for (unsigned i = 0; i < GetChannelCount(); ++i)
				b[i] = new Steinberg::Vst::Sample32[block_size];
		}
	}

	void FreeBuffers() {
		if (buffers[0] && buffers[1]) {
			for (auto &b : buffers) {
				for (unsigned i = 0; i < GetChannelCount(); ++i)
					if (b[i])
						delete[] b[i];
				delete b;
				b = nullptr;
			}
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
	Steinberg::Vst::Sample32** buffers[2];
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
	bool LoadPluginList(const std::string& path);
	bool SavePluginList(const std::string& path) const;
	bool LoadPluginList();
	bool SavePluginList() const;
	std::uint32_t GetPluginCount() const;
	bool AddPlugin(const std::string& path);
	void DeletePlugin(std::uint32_t idx);
	void MoveUp(std::uint32_t idx);
	void MoveDown(std::uint32_t idx);
	std::string GetPluginName(std::uint32_t idx) const;
	bool HasEditor(std::uint32_t idx) const;
	void CreateEditor(std::uint32_t idx, HWND hwnd);
	std::uint32_t GetPluginEditorHeight(std::uint32_t idx);
	std::uint32_t GetPluginEditorWidth(std::uint32_t idx);
	void ShowEditor(std::uint32_t idx);
	void HideEditor(std::uint32_t idx);
	bool IsEditorShown(std::uint32_t idx) const;
	bool IsBypassed(std::uint32_t idx) const;
	void SetBypass(std::uint32_t idx, bool bypass);
	bool IsActive(std::uint32_t idx) const;
	void SetActive(std::uint32_t idx, bool active);
	std::uint32_t GetPluginPresetCount(std::uint32_t idx) const;
	std::string GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx);
	void SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx);
	bool SavePreset(std::uint32_t idx);
	bool LoadPreset(std::uint32_t idx);
	bool SavePreset(std::uint32_t idx, const std::string& path);
	bool LoadPreset(std::uint32_t idx, const std::string& path);
private:
	friend IHostController* Host::GetController();
	HostController(std::shared_ptr<Host::HostImpl> impl);

	std::shared_ptr<Host::HostImpl> host;
};

HostController::HostController(std::shared_ptr<Host::HostImpl> impl) : host(std::move(impl)) {

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
	return static_cast<std::uint32_t>(host->plugins.Size());
}

bool HostController::AddPlugin(const std::string& path) {
	return host->plugins.Add(path);
}

void HostController::DeletePlugin(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		host->plugins.Delete(idx);
}

void HostController::MoveUp(std::uint32_t idx) {
	if (idx < host->plugins.Size() && idx > 0)
		host->plugins.Swap(idx, idx - 1);
}

void HostController::MoveDown(std::uint32_t idx) {
	if (idx < host->plugins.Size() - 1)
		host->plugins.Swap(idx, idx + 1);
}

std::string HostController::GetPluginName(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].GetPluginNameA();
	return "";
}

bool HostController::HasEditor(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].HasEditor();
	return false;
}

void HostController::CreateEditor(std::uint32_t idx, HWND hwnd) {
	if (idx < host->plugins.Size())
		host->plugins[idx].CreateEditor(hwnd);
}

std::uint32_t HostController::GetPluginEditorHeight(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].GetEditorHeight();
	else
		return 0;
}

std::uint32_t HostController::GetPluginEditorWidth(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].GetEditorWidth();
	else
		return 0;
}

void HostController::ShowEditor(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		host->plugins[idx].ShowEditor();
}

void HostController::HideEditor(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		host->plugins[idx].HideEditor();
}

bool HostController::IsEditorShown(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].IsEditorShown();
	return false;
}

bool HostController::IsBypassed(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].IsBypassed();
	return false;
}

void HostController::SetBypass(std::uint32_t idx, bool bypass) {
	if (idx < host->plugins.Size())
		host->plugins[idx].SetBypass(bypass);
}

bool HostController::IsActive(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].IsActive();
	return false;
}

void HostController::SetActive(std::uint32_t idx, bool active) {
	if (idx < host->plugins.Size())
		host->plugins[idx].SetActive(active);
}

std::uint32_t HostController::GetPluginPresetCount(std::uint32_t idx) const {
	if (idx < host->plugins.Size())
		return host->plugins[idx].GetProgramCount();
	return 0;
}

std::string HostController::GetPluginPresetName(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	if (plugin_idx < host->plugins.Size() && preset_idx < host->plugins[plugin_idx].GetProgramCount())
		return host->plugins[plugin_idx].GetProgramNameA(preset_idx);
	return "";
}

void HostController::SetPluginPreset(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	if (plugin_idx < host->plugins.Size() && preset_idx < host->plugins[plugin_idx].GetProgramCount())
		host->plugins[plugin_idx].SetProgram(preset_idx);
}

bool HostController::SavePreset(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].SaveState();
	return false;
}

bool HostController::LoadPreset(std::uint32_t idx) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].LoadState();
	return false;
}

bool HostController::SavePreset(std::uint32_t idx, const std::string& path) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].SaveState(path);
	return false;
}

bool HostController::LoadPreset(std::uint32_t idx, const std::string& path) {
	if (idx < host->plugins.Size())
		return host->plugins[idx].LoadState(path);
	return false;
}

IHostController* Host::GetController() {
	return new HostController(impl);
}
} // namespace