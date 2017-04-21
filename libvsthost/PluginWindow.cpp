#include "PluginWindow.h"

#include "Plugin.h"

namespace VSTHost {
const TCHAR* PluginWindow::kClassName = TEXT("PluginWindow");
bool PluginWindow::registered = false;

PluginWindow::PluginWindow(std::shared_ptr<IHostController> hc, std::uint32_t idx) 
	: Window(200, 300), menu(NULL), index(idx), host_ctrl(hc) {
	
}

PluginWindow::~PluginWindow() {
	if (menu)
		DestroyMenu(menu);
}



bool PluginWindow::RegisterWC(const TCHAR* class_name) {
	if (!registered)
		registered = Window::RegisterWC(class_name);
	return registered;
}

LRESULT CALLBACK PluginWindow::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
		case WM_CLOSE:
			Hide();
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) >= MenuItem::Preset) {
				//plugin.SetProgram(LOWORD(wParam) - MenuItem::Preset);
				break;
			}
			switch (LOWORD(wParam)) {
				case MenuItem::Bypass: {
					if (menu) {
						//CheckMenuItem(menu, MenuItem::Bypass, plugin.IsBypassed() ? MF_UNCHECKED : MF_CHECKED);
						//plugin.SetBypass(!plugin.IsBypassed());
					}
					break;
				}
				case MenuItem::Active: {
					if (menu) {
						//CheckMenuItem(menu, MenuItem::Active, plugin.IsActive() ? MF_UNCHECKED : MF_CHECKED);
						//plugin.SetActive(!plugin.IsActive());
					}
					break;
				}
				case MenuItem::Close:
					Hide();
					break;
				case MenuItem::Load:
					//plugin.LoadState();
					InvalidateRect(hWnd, NULL, false);
					break;
				case MenuItem::Save:
					//plugin.SaveState();
					break;
				default:
					break;
			}
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

bool PluginWindow::Initialize(HWND parent) {
	return false;
}

void PluginWindow::Show() {

}

void PluginWindow::Hide() {

}

void PluginWindow::MovedUp() {

}

void PluginWindow::MovedDown() {

}

void PluginWindow::PresetSet(std::uint32_t idx) {

}

void PluginWindow::BypassSet(bool bypass) {

}

void PluginWindow::ActiveSet(bool active) {

}

void PluginWindow::StateLoaded() {

}

HMENU PluginWindow::CreateMenu() {
	return NULL;
}
} // namespace