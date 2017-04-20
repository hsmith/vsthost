#include "HostWindow.h"

#include <iostream>

#include "PluginVST2.h"
#include "PluginVST3.h"
#include "PluginVST3Window.h"
#include "PluginVST2Window.h"
#include "PluginManager.h"

namespace VSTHost {
const TCHAR* HostWindow::button_labels[Items::BUTTON_COUNT] = { 
	TEXT("Add"), TEXT("Delete"), TEXT("Move Up"), TEXT("Move Down"), 
	TEXT("Show Editor"), TEXT("Hide Editor"), TEXT("Save List") };
const TCHAR* HostWindow::kClassName = TEXT("HostWindow");
const TCHAR* HostWindow::kCaption = TEXT("HostWindow");
const int HostWindow::kWindowWidth = 150;
const int HostWindow::kWindowHeight = 160;
const int HostWindow::kListWidth = 150;
const int HostWindow::kListHeight = 250;
const int HostWindow::kButtonWidth = 120;
const int HostWindow::kButtonHeight = 30;
bool HostWindow::registered = false;

HostWindow::HostWindow(IHostController* hc) : Window(kWindowWidth, kWindowHeight), font(NULL), host_ctrl(hc) {
	host_ctrl->RegisterObserver(this);
}

HostWindow::~HostWindow() {
	host_ctrl->UnregisterObserver(this);
	if (font)
		::DeleteObject(font);
	for (auto b : buttons)
		if (b != NULL)
			::DestroyWindow(b);
	if (plugin_list)
		::DestroyWindow(plugin_list);
	// wnd freed through base class dtor
}

bool HostWindow::RegisterWC(const TCHAR* class_name) {
	if (!registered)
		registered = Window::RegisterWC(class_name);
	return registered;
}

bool HostWindow::Initialize(HWND parent) {
	if (RegisterWC(kClassName)) {
		wnd = CreateWindow(kClassName, kCaption, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			rect.left, rect.top, rect.right, rect.bottom, parent, NULL, GetModuleHandle(NULL), reinterpret_cast<LPVOID>(this));
		if (wnd) {
			SetFont();
			PopulatePluginList();
			SelectPlugin(0);
		}
		return 0 != wnd;
	}
	else return false;
}

void HostWindow::OnCreate(HWND hWnd) {
	plugin_list = CreateWindow(TEXT("listbox"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | LBS_NOTIFY,
		20, 20, kListWidth, kListHeight, hWnd, (HMENU)Items::PluginList, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
	buttons[Items::BUTTON_COUNT - 1] = CreateWindow(TEXT("button"), button_labels[Items::BUTTON_COUNT - 1], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		20, 25 + kListHeight, kButtonWidth + 20 + kListWidth, kButtonHeight, hWnd, 
		(HMENU)(Items::BUTTON_COUNT - 1), (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
	for (Steinberg::int64 i = Items::Add; i < Items::BUTTON_COUNT - 1; ++i) {
		buttons[i] = CreateWindow(TEXT("button"), button_labels[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			20 + kListWidth + 20, 20 + i * 40, kButtonWidth, kButtonHeight, hWnd, reinterpret_cast<HMENU>(i),
			reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hWnd, GWLP_HINSTANCE)), NULL);
	}
}


LRESULT CALLBACK HostWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_CREATE:
			OnCreate(hWnd);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case Items::Add:
					if (HIWORD(wParam) == BN_CLICKED)
						OpenDialog();
					break;
				case Items::Delete:
					if (HIWORD(wParam) == BN_CLICKED) {
						const auto sel = GetPluginSelection();
						const auto count = host_ctrl->GetPluginCount();
						if (sel < count) {
							host_ctrl->DeletePlugin(sel);
							PopulatePluginList();
							if (sel == count - 1)
								SelectPlugin(sel - 1);
							else
								SelectPlugin(sel);
						}
					}
					break;
				case Items::Up:
					if (HIWORD(wParam) == BN_CLICKED) {
						const auto sel = GetPluginSelection();
						if (sel > 0 && sel < host_ctrl->GetPluginCount()) {
							host_ctrl->MoveUp(sel);
							PopulatePluginList();
							SelectPlugin(sel - 1);
						}
					}
					break;
				case Items::Down:
					if (HIWORD(wParam) == BN_CLICKED) {
						const auto sel = GetPluginSelection();
						if (sel < host_ctrl->GetPluginCount() - 1) {
							host_ctrl->MoveDown(sel);
							PopulatePluginList();
							SelectPlugin(sel + 1);
						}
					}
					break;
				case Items::PluginList:
					if (HIWORD(wParam) == LBN_SELCHANGE)
						SelectPlugin(GetPluginSelection());
					break;
				case Items::Show:
					if (HIWORD(wParam) == BN_CLICKED) {
						const auto sel = GetPluginSelection();
						if (sel < host_ctrl->GetPluginCount() && host_ctrl->HasEditor(sel)) {
							host_ctrl->ShowEditor(sel);
							EnableWindow(buttons[Items::Show], false);
							EnableWindow(buttons[Items::Hide], true);
						}
					}
					break;
				case Items::Hide:
					if (HIWORD(wParam) == BN_CLICKED) {
						auto sel = GetPluginSelection();
						if (sel < host_ctrl->GetPluginCount() && host_ctrl->HasEditor(sel)) {
							host_ctrl->HideEditor(sel);
							EnableWindow(buttons[Items::Show], true);
							EnableWindow(buttons[Items::Hide], false);
						}
					}
					break;
				case Items::Save:
					if (HIWORD(wParam) == BN_CLICKED)
						host_ctrl->SavePluginList();
				default:
					break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void HostWindow::PopulatePluginList() {
	if (plugin_list) {
		SendMessage(plugin_list, LB_RESETCONTENT, NULL, NULL);
		for (decltype(host_ctrl->GetPluginCount()) i = 0; i < host_ctrl->GetPluginCount(); ++i) {
			int pos = SendMessageA(plugin_list, LB_ADDSTRING, 0, (LPARAM)host_ctrl->GetPluginName(i).c_str());
			SendMessage(plugin_list, LB_SETITEMDATA, pos, (LPARAM)i);
		}
	}
}

void HostWindow::SetFont() {
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	font = CreateFontIndirect(&metrics.lfMessageFont);
	SendMessage(wnd, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
	SendMessage(plugin_list, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
	for (int i = Items::Add; i < Items::BUTTON_COUNT; ++i)
		SendMessage(buttons[i], WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
}

void HostWindow::OpenDialog() {
	char filename[MAX_PATH]{};
	if (!ofn) {
		ofn = std::make_unique<OPENFILENAMEA>(OPENFILENAMEA());
		ofn->lStructSize = sizeof(*ofn);
		ofn->hwndOwner = wnd;
		ofn->lpstrFilter = "VST Plugins (*.dll, *.vst3)\0*.dll;*.vst3\0VST2 Plugins (*.dll)\0*.dll\0VST3 Plugins (*.vst3)\0*.vst3\0";
		ofn->nMaxFile = sizeof(filename);
		ofn->lpstrInitialDir = Plugin::kPluginDirectory.c_str();
		ofn->Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	}
	ofn->lpstrFile = filename;
	if (::GetOpenFileNameA(ofn.get())) {
		const auto count = host_ctrl->GetPluginCount();
		if (host_ctrl->AddPlugin(std::string(filename))) {
			host_ctrl->CreateEditor(host_ctrl->GetPluginCount() - 1, NULL);
			PopulatePluginList();
			SelectPlugin(count);
		}
	}
}

void HostWindow::SelectPlugin(std::uint32_t i) {
	if (plugin_list) {
		SendMessage(plugin_list, LB_SETCURSEL, i, 0);
		SetFocus(plugin_list);
		auto count = host_ctrl->GetPluginCount();
		if (count >= 2) {
			EnableWindow(buttons[Items::Up], i > 0);
			EnableWindow(buttons[Items::Down], i < count - 1);
		}
		else {
			EnableWindow(buttons[Items::Up], false);
			EnableWindow(buttons[Items::Down], false);
		}
		if (count == 0) {
			EnableWindow(buttons[Items::Show], false);
			EnableWindow(buttons[Items::Hide], false);
			EnableWindow(buttons[Items::Delete], false);
		}
		else {
			EnableWindow(buttons[Items::Show], host_ctrl->HasEditor(i) && !host_ctrl->IsEditorShown(i));
			EnableWindow(buttons[Items::Hide], host_ctrl->HasEditor(i) && host_ctrl->IsEditorShown(i));
			EnableWindow(buttons[Items::Delete], true);
		}
	}
}

std::uint32_t HostWindow::GetPluginSelection() {
	if (plugin_list)
		return static_cast<std::uint32_t>(SendMessage(plugin_list, LB_GETCURSEL, NULL, NULL));
	else 
		return -1;
}

void HostWindow::CreateEditors() {
	for (decltype(host_ctrl->GetPluginCount()) i = 0; i < host_ctrl->GetPluginCount(); ++i)
		host_ctrl->CreateEditor(i, NULL);
}

void HostWindow::OnPluginAdded(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnPluginDeleted(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnListLoaded() {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnMovedUp(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnMovedDown(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnEditorShown(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnEditorHidden(std::uint32_t idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnBypassSet(std::uint32_t idx, bool bypass) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}

void HostWindow::OnActiveSet(std::uint32_t idx, bool active) {
	MessageBoxA(NULL, __func__, NULL, NULL);
}
} // namespace