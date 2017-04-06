#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H

#include "Window.h"

namespace VSTHost {
class Plugin;
class PluginWindow : public Window {
public:
	PluginWindow(int width, int height, Plugin& p);
	virtual ~PluginWindow();
	virtual bool Initialize(HWND parent = NULL) = 0;
	virtual void SetRect() = 0;
	bool IsActive() const;
	HWND GetHWND() const;
protected:
	enum MenuItem {
		Bypass = 10000, Active, Close, 
		State, Load, Save, 
		Presets, Preset = 20000
	};
	void ApplyOffset();
	virtual HMENU CreateMenu() const = 0;
	HMENU menu;
	static const TCHAR* kClassName;
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	bool RegisterWC(const TCHAR* class_name) override;
	static bool registered;
	static int static_offset;
	int offset;
	int size_x{ 0 }, size_y{ 0 };
	bool is_active{ false };
	Plugin& plugin;
};
} // namespace

#endif