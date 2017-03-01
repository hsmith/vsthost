#include "PluginVST2Window.h"

#include "pluginterfaces\vst2.x\aeffectx.h"

#include "PluginVST2.h"

namespace VSTHost {
PluginVST2Window::PluginVST2Window(PluginVST2& p) : PluginWindow(100, 100, p) {

}

void PluginVST2Window::SetRect() {
	ERect* erect = nullptr;
	if (dynamic_cast<PluginVST2&>(plugin).Dispatcher(AEffectOpcodes::effEditGetRect, 0, 0, &erect)) {
		size_x = erect->right - erect->left;
		size_y = erect->bottom - erect->top;
		rect.left = erect->left;
		rect.right = erect->right;
		rect.top = erect->top;
		rect.bottom = erect->bottom;
		::AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);
		rect.bottom += ::GetSystemMetrics(SM_CYMENU);
		if (rect.left < 0) {
			rect.right -= rect.left;
			rect.left -= rect.left;
		}
		if (rect.top < 0) {
			rect.bottom -= rect.top;
			rect.top -= rect.top;
		}
	}
	else {
		rect.left = 0;
		rect.top = 0;
		rect.right = 600;
		rect.bottom = 300;
	}
	ApplyOffset();
}

bool PluginVST2Window::Initialize(HWND parent) {
	if (RegisterWC(kClassName)) {
		SetRect();
		wnd = ::CreateWindow(kClassName, plugin.GetPluginName().c_str(), WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			rect.left, rect.top, rect.right, rect.bottom, NULL, menu = CreateMenu(), ::GetModuleHandle(NULL), (LPVOID)this);
		return wnd != NULL;
	}
	else 
		return false;
}

void PluginVST2Window::Show() {
	if (wnd) {
		is_active = true;
		ERect* erect = nullptr;
		dynamic_cast<PluginVST2&>(plugin).Dispatcher(AEffectOpcodes::effEditGetRect, 0, 0, &erect);
		if (erect->right - erect->left != size_x || erect->bottom - erect->top != size_y) {
			SetRect();
			::SetWindowPos(wnd, NULL, rect.left, rect.top, rect.right, rect.bottom, NULL);
		}
		Window::Show();
	}
}

void PluginVST2Window::Hide() {
	if (wnd) {
		is_active = false;
		Window::Hide();
	}
}

HMENU PluginVST2Window::CreateMenu() const {
	HMENU hmenu = ::CreateMenu();
	// plugin submenu
	HMENU hplugin = ::CreateMenu();
	::AppendMenu(hplugin, MF_STRING, MenuItem::Bypass, TEXT("Bypass"));
	auto flag = MF_STRING;
	if (plugin.IsActive())
		flag |= MF_CHECKED;
	::AppendMenu(hplugin, flag, MenuItem::Active, TEXT("Active"));
	::AppendMenu(hplugin, MF_STRING, MenuItem::Close, TEXT("Close"));
	::AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hplugin, TEXT("Plugin"));
	// state submenu
	HMENU hstate = ::CreateMenu();
	::AppendMenu(hstate, MF_STRING, MenuItem::Save, TEXT("Save"));
	::AppendMenu(hstate, MF_STRING, MenuItem::Load, TEXT("Load"));
	::AppendMenu(hmenu, MF_POPUP, (UINT_PTR)hstate, TEXT("State"));
	// preset submenu
	HMENU hpresets = ::CreateMenu();
	PluginVST2& p = dynamic_cast<PluginVST2&>(plugin);
	auto currentProgram = p.Dispatcher(AEffectOpcodes::effGetProgram);
	bool programChanged = false;
	for (decltype(plugin.GetProgramCount()) i = 0; i < plugin.GetProgramCount(); ++i) {
		char tmp[kVstMaxProgNameLen + 1] = { 0 };
		p.Dispatcher(AEffectXOpcodes::effBeginSetProgram);
		if (!p.Dispatcher(AEffectXOpcodes::effGetProgramNameIndexed, i, 0, tmp)) {
			p.Dispatcher(AEffectOpcodes::effSetProgram, 0, i);
			p.Dispatcher(AEffectOpcodes::effGetProgramName, 0, 0, tmp);
			if (!programChanged) programChanged = true;
		}
		p.Dispatcher(AEffectXOpcodes::effEndSetProgram);
		::AppendMenuA(hpresets, MF_STRING, MenuItem::Preset + i, tmp);
	}
	if (programChanged) p.Dispatcher(AEffectOpcodes::effSetProgram, 0, currentProgram);
	::AppendMenu(hmenu, plugin.GetProgramCount() > 0 ? MF_POPUP : MF_POPUP | MF_GRAYED, (UINT_PTR)hpresets, TEXT("Plugin"));
	return hmenu;
}
} // namespace