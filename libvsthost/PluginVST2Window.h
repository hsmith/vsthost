#ifndef PLUGINVST2WINDOW_H
#define PLUGINVST2WINDOW_H

#include "PluginWindow.h"

namespace VSTHost {
class PluginVST2;
class PluginVST2Window : public PluginWindow {
public:
	PluginVST2Window(PluginVST2& p);
	~PluginVST2Window() {}
	bool Initialize(HWND parent) override;
	HMENU CreateMenu() const override;
	void Show() override;
	void Hide() override;
	void SetRect() override;
};
} // namespace

#endif