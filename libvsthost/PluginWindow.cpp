#include "PluginWindow.h"

#include "Plugin.h"

namespace VSTHost {
const TCHAR* PluginWindow::kClassName = TEXT("PluginWindow");
bool PluginWindow::registered = false;

PluginWindow::PluginWindow(std::shared_ptr<IHostController> hc, std::uint32_t idx) 
	: Window(200, 300), menu(NULL), index(idx), host_ctrl(hc), last_checked(host_ctrl->GetPluginPresetCount(index)) {
	
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
			if (LOWORD(wParam) >= MenuItem::Preset && LOWORD(wParam) - MenuItem::Preset < host_ctrl->GetPluginPresetCount(index)) {
				host_ctrl->SetPluginPreset(index, LOWORD(wParam) - MenuItem::Preset);
				break;
			}
			switch (LOWORD(wParam)) {
				case MenuItem::Bypass: {
					host_ctrl->SetBypass(index, !host_ctrl->IsBypassed(index));
					break;
				}
				case MenuItem::Active: {
					host_ctrl->SetActive(index, !host_ctrl->IsActive(index));
					break;
				}
				case MenuItem::Close:
					Hide();
					break;
				case MenuItem::Load:
					host_ctrl->LoadPreset(index);
					break;
				case MenuItem::Save:
					host_ctrl->SavePreset(index);
					break;
				default:
					break;
			}
			break;
		case WM_DESTROY:
			host_ctrl->HideEditor(index);
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
			CheckMenuItem(menu, MenuItem::Bypass, host_ctrl->IsBypassed(index) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(menu, MenuItem::Active, host_ctrl->IsActive(index) ? MF_CHECKED : MF_UNCHECKED);
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
		::SetWindowPos(wnd, NULL, client_rect.left, client_rect.top, client_rect.right, client_rect.bottom, NULL);
	}
}

void PluginWindow::MovedUp() {
	index--;
}

void PluginWindow::MovedDown() {
	index++;
}

void PluginWindow::PresetSet(std::uint32_t idx) {
	if (last_checked < host_ctrl->GetPluginPresetCount(index))
		CheckMenuItem(menu, MenuItem::Preset + last_checked, MF_UNCHECKED);
	CheckMenuItem(menu, MenuItem::Preset + idx, MF_CHECKED);
	last_checked = idx;
}

void PluginWindow::BypassSet(bool bypass) {
	CheckMenuItem(menu, MenuItem::Bypass, bypass ? MF_CHECKED : MF_UNCHECKED);
}

void PluginWindow::ActiveSet(bool active) {
	CheckMenuItem(menu, MenuItem::Active, active ? MF_CHECKED : MF_UNCHECKED);
}

void PluginWindow::StateLoaded() {
	if (last_checked < host_ctrl->GetPluginPresetCount(index)) {
		CheckMenuItem(menu, MenuItem::Preset + last_checked, MF_UNCHECKED);
		last_checked = host_ctrl->GetPluginPresetCount(index);
	}
	Refresh();
}

HMENU PluginWindow::CreateMenu() {
	HMENU hmenu = ::CreateMenu();
	// plugin submenu
	HMENU hplugin = ::CreateMenu();
	::AppendMenu(hplugin, MF_STRING, MenuItem::Bypass, TEXT("Bypass"));
	::AppendMenu(hplugin, MF_STRING, MenuItem::Active, TEXT("Active"));
	::AppendMenu(hplugin, MF_STRING, MenuItem::Close, TEXT("Close"));
	::AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hplugin, TEXT("Plugin"));
	// state submenu
	HMENU hstate = ::CreateMenu();
	::AppendMenu(hstate, MF_STRING, MenuItem::Save, TEXT("Save"));
	::AppendMenu(hstate, MF_STRING, MenuItem::Load, TEXT("Load"));
	::AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hstate, TEXT("State"));
	// preset submenu
	HMENU hpresets = ::CreateMenu();
	for (Steinberg::uint32 i = 0; i < host_ctrl->GetPluginPresetCount(index); ++i)
		::AppendMenu(hpresets, MF_STRING, MenuItem::Preset + i, host_ctrl->GetPluginPresetName(index, i).c_str());
	::AppendMenu(hmenu, host_ctrl->GetPluginPresetCount(index) > 0 ? MF_POPUP : MF_POPUP | MF_GRAYED, (UINT_PTR)hpresets, TEXT("Preset"));
	return hmenu;
}
} // namespace