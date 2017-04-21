#include "HostWindow.h"

#include <iostream>

#include "PluginVST2.h"
#include "PluginVST3.h"
#include "PluginWindow.h"
#include "PluginManager.h"

namespace VSTHost {
const TCHAR* HostWindow::button_labels[Items::BUTTON_COUNT] = { 
	TEXT("Add"), TEXT("Delete"), TEXT("Move Up"), TEXT("Move Down"), 
	TEXT("Show Editor"), TEXT("Save List") };
const TCHAR* HostWindow::kClassName = TEXT("HostWindow");
const TCHAR* HostWindow::kCaption = TEXT("HostWindow");
const int HostWindow::kWindowWidth = 150;
const int HostWindow::kWindowHeight = 120;
const int HostWindow::kListWidth = 150;
const int HostWindow::kListHeight = 210;
const int HostWindow::kButtonWidth = 120;
const int HostWindow::kButtonHeight = 30;
bool HostWindow::registered = false;

HostWindow::HostWindow(IHostController* hc) : Window(kWindowWidth, kWindowHeight), font(NULL), host_ctrl(hc) {
	
}

HostWindow::~HostWindow() {
	if (observing)
		host_ctrl->UnregisterObserver(this);
	if (font)
		::DeleteObject(font);
	for (auto b : buttons)
		if (b != NULL)
			::DestroyWindow(b);
	if (plugin_list)
		::DestroyWindow(plugin_list);
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
			// initialize open file dialog
			ofn = std::make_unique<OPENFILENAMEA>(OPENFILENAMEA());
			ofn->lStructSize = sizeof(*ofn);
			ofn->hwndOwner = wnd;
			ofn->lpstrFilter = "VST Plugins (*.dll, *.vst3)\0*.dll;*.vst3\0VST2 Plugins (*.dll)\0*.dll\0VST3 Plugins (*.vst3)\0*.vst3\0";
			ofn->nMaxFile = MAX_PATH;
			ofn->lpstrInitialDir = Host::kPluginDirectory;
			ofn->Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
			host_ctrl->RegisterObserver(this);
			observing = true;
			OnInitialization();
		}
		return 0 != wnd;
	}
	else return false;
}

void HostWindow::OnInitialization() {
	plugin_windows.clear();
	PopulatePluginList();
	if (host_ctrl->GetPluginCount()) {
		for (std::uint32_t i = 0; i < host_ctrl->GetPluginCount(); ++i)
			plugin_windows.emplace_back(new PluginWindow(host_ctrl, i));
		Select(0);
		SetButtons();
	}
}

void HostWindow::PopulatePluginList() {
	SendMessage(plugin_list, LB_RESETCONTENT, NULL, NULL);
	for (std::uint32_t i = 0; i < host_ctrl->GetPluginCount(); ++i) {
		auto pos = SendMessageA(plugin_list, LB_ADDSTRING, 0, (LPARAM)host_ctrl->GetPluginName(i).c_str());
		SendMessage(plugin_list, LB_SETITEMDATA, pos, (LPARAM)i);
	}
}

void HostWindow::OnCreate(HWND hWnd) {
	plugin_list = CreateWindow(TEXT("listbox"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | LBS_NOTIFY,
		20, 20, kListWidth, kListHeight, hWnd, (HMENU)Items::PluginList, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
	buttons[Items::BUTTON_COUNT - 1] = CreateWindow(TEXT("button"), button_labels[Items::BUTTON_COUNT - 1], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		20, 25 + kListHeight, kButtonWidth + 20 + kListWidth, kButtonHeight, hWnd, 
		(HMENU)(Items::BUTTON_COUNT - 1), (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
	for (Steinberg::int64 i = Items::Add; i < Items::BUTTON_COUNT - 1; ++i) {
		buttons[i] = CreateWindow(TEXT("button"), button_labels[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			20 + kListWidth + 20, 20 + i * 40, kButtonWidth, kButtonHeight, hWnd, (HMENU)i,
			(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
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
					if (HIWORD(wParam) == BN_CLICKED) {
						char filename[MAX_PATH]{};
						ofn->lpstrFile = filename;
						if (::GetOpenFileNameA(ofn.get()))
							host_ctrl->AddPlugin(std::string(filename));
					}
					break;
				case Items::Delete:
					if (HIWORD(wParam) == BN_CLICKED)
						host_ctrl->DeletePlugin(GetSelection());
					break;
				case Items::Up:
					if (HIWORD(wParam) == BN_CLICKED)
						host_ctrl->MoveUp(GetSelection());
					break;
				case Items::Down:
					if (HIWORD(wParam) == BN_CLICKED)
						host_ctrl->MoveDown(GetSelection());
					break;
				case Items::PluginList:
					if (HIWORD(wParam) == LBN_SELCHANGE)
						SetButtons();
					break;
				case Items::Show:
					if (HIWORD(wParam) == BN_CLICKED) {
						const auto selected = GetSelection();
						if (plugin_windows[selected]->IsVisible())
							plugin_windows[selected]->Hide();
						else
							plugin_windows[selected]->Show();
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

void HostWindow::Select(std::uint32_t idx) {
	SendMessage(plugin_list, LB_SETCURSEL, idx, 0);
	SetFocus(plugin_list);
}

void HostWindow::SetButtons() {
	const auto count = host_ctrl->GetPluginCount(), selected = GetSelection();
	::EnableWindow(buttons[Items::Up], count >= 2 && selected > 0);
	::EnableWindow(buttons[Items::Down], count >= 2 && selected >= 0 && selected < count - 1);
	::EnableWindow(buttons[Items::Delete], count > 0);
	if (count > 0 && selected >= 0 && host_ctrl->HasEditor(selected))
	{
		::EnableWindow(buttons[Items::Show], true);
		if (plugin_windows[selected]->IsVisible())
			::SendMessageA(buttons[Items::Show], WM_SETTEXT, NULL, (LPARAM)"Hide Editor");
		else
			::SendMessageA(buttons[Items::Show], WM_SETTEXT, NULL, (LPARAM)"Show Editor");
	}
	else
	{
		::EnableWindow(buttons[Items::Show], false);
		::SendMessageA(buttons[Items::Show], WM_SETTEXT, NULL, (LPARAM)"Show Editor");
	}
}

std::uint32_t HostWindow::GetSelection() {
	return static_cast<std::uint32_t>(SendMessage(plugin_list, LB_GETCURSEL, NULL, NULL));
}

void HostWindow::OnPluginAdded(std::uint32_t idx) {
	plugin_windows.emplace_back(new PluginWindow(host_ctrl, idx));
	auto pos = SendMessageA(plugin_list, LB_ADDSTRING, 0, (LPARAM)host_ctrl->GetPluginName(idx).c_str());
	SendMessage(plugin_list, LB_SETITEMDATA, pos, (LPARAM)idx);
	Select(idx);
	SetButtons();
}

void HostWindow::OnPluginDeleted(std::uint32_t idx) {
	if (plugin_windows[idx]->IsVisible())
		plugin_windows[idx]->Hide();
	plugin_windows.erase(plugin_windows.begin() + idx);
	PopulatePluginList();

	const auto count = host_ctrl->GetPluginCount();
	if (count > 0)
	{
		if (idx == count)
			Select(idx - 1);
		else
			Select(idx);
	}
	else
		SendMessage(plugin_list, LB_SETCURSEL, -1, 0);
	SetButtons();
}

void HostWindow::OnListLoaded() {
	OnInitialization();
}

void HostWindow::OnMovedUp(std::uint32_t idx) {
	plugin_windows[idx]->MovedUp();
	plugin_windows[idx - 1]->MovedDown();
	using std::swap;
	swap(plugin_windows[idx], plugin_windows[idx - 1]);
	PopulatePluginList();

	Select(idx - 1);
	SetButtons();
}

void HostWindow::OnMovedDown(std::uint32_t idx) {
	plugin_windows[idx]->MovedDown();
	plugin_windows[idx + 1]->MovedUp();
	using std::swap;
	swap(plugin_windows[idx], plugin_windows[idx + 1]);
	PopulatePluginList();

	Select(idx + 1);
	SetButtons();
}

void HostWindow::OnEditorShown(std::uint32_t idx) {
	if (idx == GetSelection())
		::SendMessageA(buttons[Items::Show], WM_SETTEXT, NULL, (LPARAM)"Hide Editor");
}

void HostWindow::OnEditorHidden(std::uint32_t idx) {
	if (idx == GetSelection())
		::SendMessageA(buttons[Items::Show], WM_SETTEXT, NULL, (LPARAM)"Show Editor");
}

void HostWindow::OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
	plugin_windows[plugin_idx]->PresetSet(preset_idx);
}

void HostWindow::OnBypassSet(std::uint32_t idx, bool bypass) {
	plugin_windows[idx]->BypassSet(bypass);
}

void HostWindow::OnActiveSet(std::uint32_t idx, bool active) {
	plugin_windows[idx]->ActiveSet(active);
}

void HostWindow::OnStateLoaded(std::uint32_t idx) {
	plugin_windows[idx]->StateLoaded();
}
} // namespace