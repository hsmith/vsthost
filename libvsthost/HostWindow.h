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
	enum Items {
		Add = 0, Delete, Up, Down, Show, Hide, Save, BUTTON_COUNT, PluginList
	};
	static const TCHAR* button_labels[Items::BUTTON_COUNT];
	static const TCHAR* kClassName;
	static const TCHAR* kCaption;
	static const int kWindowWidth, kWindowHeight;
	static const int kListWidth, kListHeight;
	static const int kButtonWidth, kButtonHeight;
	void OnCreate(HWND hWnd) override;
	void SetFont();
	std::uint32_t GetPluginSelection();
public:
	HostWindow(IHostController* hc);
	//HostWindow(const HostWindow& hw) : Window(hw) {}
	~HostWindow();
	bool Initialize(HWND parent) override;
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool RegisterWC(const TCHAR* class_name) override;
	void CreateEditors();
	void PopulatePluginList();
	void SelectPlugin(std::uint32_t i);
	void OpenDialog();
private:
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
	static bool registered;
	HFONT font;
	HWND plugin_list;
	HWND buttons[Items::BUTTON_COUNT];
	std::unique_ptr<OPENFILENAMEA> ofn;
	std::unique_ptr<IHostController> host_ctrl;
};
} // namespace

#endif