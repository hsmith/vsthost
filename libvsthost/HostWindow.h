#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H
#include "Window.h"

#include <vector>
#include <memory>

#include "Host.h"

namespace VSTHost {
class Plugin;
class PluginWindow;
class PluginManager;
class HostWindow : public Window {
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
	~HostWindow();
	bool Initialize(HWND parent) override;
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool RegisterWC(const TCHAR* class_name) override;
	void CreateEditors();
	void PopulatePluginList();
	void SelectPlugin(std::uint32_t i);
	void OpenDialog();
private:
	static bool registered;
	HFONT font;
	HWND plugin_list;
	HWND buttons[Items::BUTTON_COUNT];
	std::unique_ptr<OPENFILENAMEA> ofn;
	std::unique_ptr<IHostController> host_ctrl;
};
} // namespace

#endif