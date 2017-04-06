#include "Stdafx.h"
#include "HostControllerProxy.h"
#include "StringConverter.h"
#include "libvsthost/Host.h"

namespace VSTHost {

	HostControllerProxy::HostControllerProxy(IHostController* hc_) :
		hc (hc_) {
	}

	HostControllerProxy::~HostControllerProxy() {
		delete hc;
	}

	bool HostControllerProxy::LoadPluginList(String^ path) {
		return hc->LoadPluginList(StringConverter::Convert(path));
	}

	bool HostControllerProxy::SavePluginList(String^ path) {
		return hc->SavePluginList(StringConverter::Convert(path));
	}

	bool HostControllerProxy::LoadPluginList() {
		return hc->LoadPluginList();
	}

	bool HostControllerProxy::SavePluginList() {
		return hc->SavePluginList();
	}

	UInt32 HostControllerProxy::GetPluginCount() {
		return hc->GetPluginCount();
	}

	bool HostControllerProxy::AddPlugin(String^ path) {
		return hc->AddPlugin(StringConverter::Convert(path));
	}

	void HostControllerProxy::DeletePlugin(UInt32 idx) {
		return hc->DeletePlugin(idx);
	}

	void HostControllerProxy::MoveUp(UInt32 idx) {
		return hc->MoveUp(idx);
	}

	void HostControllerProxy::MoveDown(UInt32 idx) {
		return hc->MoveDown(idx);
	}

	String^ HostControllerProxy::GetPluginName(UInt32 idx) {
		return StringConverter::Convert(hc->GetPluginName(idx));
	}

	bool HostControllerProxy::HasEditor(UInt32 idx) {
		return hc->HasEditor(idx);
	}

	void HostControllerProxy::CreateEditor(UInt32 idx, IntPtr hwnd) {
#ifdef _WIN64
		return hc->CreateEditor(idx, (HWND)hwnd.ToInt64());
#else
		return hc->CreateEditor(idx, (HWND)hwnd.ToInt32());
#endif
	}

	UInt32 HostControllerProxy::GetPluginEditorHeight(UInt32 idx) {
		return hc->GetPluginEditorHeight(idx);
	}

	UInt32 HostControllerProxy::GetPluginEditorWidth(UInt32 idx) {
		return hc->GetPluginEditorWidth(idx);
	}

	void HostControllerProxy::ShowEditor(UInt32 idx) {
		return hc->ShowEditor(idx);
	}

	void HostControllerProxy::HideEditor(UInt32 idx) {
		return hc->HideEditor(idx);
	}

	bool HostControllerProxy::IsEditorShown(UInt32 idx) {
		return hc->IsEditorShown(idx);
	}

	bool HostControllerProxy::IsBypassed(UInt32 idx) {
		return hc->IsBypassed(idx);
	}

	void HostControllerProxy::SetBypass(UInt32 idx, bool bypass) {
		return hc->SetBypass(idx, bypass);
	}

	bool HostControllerProxy::IsActive(UInt32 idx) {
		return hc->IsActive(idx);
	}

	void HostControllerProxy::SetActive(UInt32 idx, bool active) {
		return hc->SetActive(idx, active);
	}

	UInt32 HostControllerProxy::GetPluginPresetCount(UInt32 idx) {
		return hc->GetPluginPresetCount(idx);
	}

	String^ HostControllerProxy::GetPluginPresetName(UInt32 plugin_idx, UInt32 preset_idx) {
		return StringConverter::Convert(hc->GetPluginPresetName(plugin_idx, preset_idx));
	}

	void HostControllerProxy::SetPluginPreset(UInt32 plugin_idx, UInt32 preset_idx) {
		return hc->SetPluginPreset(plugin_idx, preset_idx);
	}

	bool HostControllerProxy::SavePreset(UInt32 idx) {
		return hc->SavePreset(idx);
	}

	bool HostControllerProxy::LoadPreset(UInt32 idx) {
		return hc->LoadPreset(idx);
	}

	bool HostControllerProxy::SavePreset(UInt32 idx, String^ path) {
		return hc->SavePreset(idx, StringConverter::Convert(path));
	}

	bool HostControllerProxy::LoadPreset(UInt32 idx, String^ path) {
		return hc->LoadPreset(idx, StringConverter::Convert(path));
	}

}