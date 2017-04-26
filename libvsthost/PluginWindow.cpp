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
	if (RegisterWC(kClassName)) {
		menu = CreateMenu();
		wnd = ::CreateWindowA(kClassName, host_ctrl->GetPluginName(index).c_str(), 
			WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, rect.left, rect.top, rect.right, rect.bottom,
			parent, menu, ::GetModuleHandle(NULL), static_cast<LPVOID>(this));
		if (wnd && host_ctrl->HasEditor(index)) {
			host_ctrl->CreateEditor(index, wnd);
			FixSize();
		}
		return wnd != NULL;
	}
	else
		return false;
}

void PluginWindow::Show() {
	if (wnd && host_ctrl->HasEditor(index)) {
		host_ctrl->ShowEditor(index);
		if (!size_fixed) {
			FixSize();
			size_fixed = true;
		}
		Window::Show();
	}
}

void PluginWindow::Hide() {
	if (wnd && host_ctrl->HasEditor(index)) {
		host_ctrl->HideEditor(index);
		Window::Hide();
	}
}

void PluginWindow::FixSize() {
	const auto w = host_ctrl->GetPluginEditorWidth(index), h = host_ctrl->GetPluginEditorHeight(index);
	RECT client_rect{};
	::GetClientRect(wnd, &client_rect);
	if (client_rect.right - client_rect.left != w || client_rect.bottom - client_rect.top != h) {
		client_rect.right = client_rect.left + w;
		client_rect.bottom = client_rect.top + h;
		::AdjustWindowRect(&client_rect, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);
		client_rect.bottom += ::GetSystemMetrics(SM_CYMENU);
		if (client_rect.left < 0) {
			client_rect.right -= client_rect.left;
			client_rect.left -= client_rect.left;
		}
		if (client_rect.top < 0) {
			client_rect.bottom -= client_rect.top;
			client_rect.top -= client_rect.top;
		}
		::SetWindowPos(wnd, NULL, rect.left, rect.top, rect.right, rect.bottom, NULL);
	}
}

void PluginWindow::MovedUp() {
	index--;
}

void PluginWindow::MovedDown() {
	index++;
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