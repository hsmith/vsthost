#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include <memory>

#include "Window.h"
#include "Host.h"

namespace VSTHost {
class Plugin;
class HostWindow;
class PluginWindow : public Window {
	friend class HostWindow;
private:
	enum MenuItem {
		Bypass = 10000, Active, Close, 
		State, Save, Load, SaveAs, LoadFrom, 
		Presets, Preset = 20000
	};
	static const TCHAR* kClassName;
	static bool registered;
	HMENU menu;
	HMENU CreateMenu();
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	bool RegisterWC(const TCHAR* class_name) override;
public:
	PluginWindow(std::shared_ptr<IHostController>& hc, std::uint32_t idx);
	~PluginWindow();
	bool Initialize(HWND parent = NULL) override;
	void Show() override;
	void Hide() override;
private:
	void FixSize();
	void MovedUp();
	void MovedDown();
	void PresetSet(std::uint32_t idx);
	void BypassSet(bool bypass);
	void ActiveSet(bool active);
	void StateLoaded();
	std::uint32_t index;
	std::shared_ptr<IHostController> host_ctrl;
	bool size_fixed{ false };
	std::uint32_t last_checked;
};
} // namespace

#endif