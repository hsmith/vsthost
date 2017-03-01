#ifndef HOST_H
#define HOST_H

#include <string>
#include <memory>
#include <cstdint>
#include <windows.h>

namespace VSTHost {
class HostController;
class Host {
friend HostController;
public:
	Host(std::int64_t max_num_samples, double sample_rate); // max_num_samples is maximum number of samples per channel
	~Host();
	void Process(float** input, float** output, std::int64_t num_samples); // num_samples - samples per channel
	void Process(char* input, char* output, std::int64_t num_samples);
	void Process(std::int16_t* input, std::int16_t* output, std::int64_t num_samples);
	void SetSampleRate(double sr);
	void SetBlockSize(std::int64_t bs);
	void CreateGUIThread();
	void CreateGUI();
	bool LoadPluginList(const std::string& path);
	bool SavePluginList(const std::string& path) const;
	bool LoadPluginList();
	bool SavePluginList() const;
	HostController GetController();
private:
	class HostImpl;
	std::shared_ptr<HostImpl> impl;
};

class HostController {
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
friend HostController Host::GetController();
	HostController(std::shared_ptr<Host::HostImpl> impl);
	std::shared_ptr<Host::HostImpl> host;
};
} // namespace

#endif