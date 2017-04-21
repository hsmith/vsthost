#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H
#include "Window.h"

#include <vector>
#include <memory>

#include "Host.h"
#include "HostObserver.h"

namespace VSTHost {
class Plugin;
class PluginWindow;
class PluginManager;
class HostWindow : public Window, public HostObserver {
private:
	enum Items {
		Add = 0, Delete, Up, Down, Show, Save, BUTTON_COUNT, PluginList
	};
	static const TCHAR* button_labels[Items::BUTTON_COUNT];
	static const TCHAR* kClassName;
	static const TCHAR* kCaption;
	static const int kWindowWidth, kWindowHeight;
	static const int kListWidth, kListHeight;
	static const int kButtonWidth, kButtonHeight;
	static bool registered;
	HFONT font;
	HWND plugin_list;
	HWND buttons[Items::BUTTON_COUNT];
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool RegisterWC(const TCHAR* class_name) override;
	void OnCreate(HWND hWnd) override;
public:
	HostWindow(IHostController* hc);
	~HostWindow();
	bool Initialize(HWND parent) override;
private:
	void SetFont();
	void OnInitialization();
	void PopulatePluginList();
	std::uint32_t GetSelection();
	void Select(std::uint32_t idx);
	void SetButtons();
	void OnPluginAdded(std::uint32_t idx) override;
	void OnPluginDeleted(std::uint32_t idx) override;
	void OnListLoaded() override;
	void OnMovedUp(std::uint32_t idx) override;
	void OnMovedDown(std::uint32_t idx) override;
	void OnEditorShown(std::uint32_t idx) override;
	void OnEditorHidden(std::uint32_t idx) override;
	void OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) override;
	void OnBypassSet(std::uint32_t idx, bool bypass) override;
	void OnActiveSet(std::uint32_t idx, bool active) override;
	void OnStateLoaded(std::uint32_t idx) override;

	std::unique_ptr<OPENFILENAMEA> ofn;
	std::shared_ptr<IHostController> host_ctrl;
	std::vector<std::unique_ptr<PluginWindow>> plugin_windows;
	bool observing{ false };
};
} // namespace

#endif