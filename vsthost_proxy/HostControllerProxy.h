#pragma once

using System::String;
using System::UInt32;
using System::IntPtr;

namespace VSTHost {
	class IHostController;
	ref class HostObserverProxyWrapper;
	public ref class HostControllerProxy
	{
	internal:
		HostControllerProxy(IHostController* hc_);
	public:
		~HostControllerProxy();
		!HostControllerProxy();
		bool LoadPluginList(String^ path);
		bool SavePluginList(String^ path);
		bool LoadPluginList();
		bool SavePluginList();
		UInt32 GetPluginCount();
		bool AddPlugin(String^ path);
		void DeletePlugin(UInt32 idx);
		void MoveUp(UInt32 idx);
		void MoveDown(UInt32 idx);
		String^ GetPluginName(UInt32 idx);
		bool HasEditor(UInt32 idx);
		void CreateEditor(UInt32 idx, IntPtr hwnd);
		UInt32 GetPluginEditorHeight(UInt32 idx);
		UInt32 GetPluginEditorWidth(UInt32 idx);
		void ShowEditor(UInt32 idx);
		void HideEditor(UInt32 idx);
		bool IsBypassed(UInt32 idx);
		void SetBypass(UInt32 idx, bool bypass);
		bool IsActive(UInt32 idx);
		void SetActive(UInt32 idx, bool active);
		UInt32 GetPluginPresetCount(UInt32 idx);
		String^ GetPluginPresetName(UInt32 plugin_idx, UInt32 preset_idx);
		void SetPluginPreset(UInt32 plugin_idx, UInt32 preset_idx);
		String^ GetPluginPresetExtension(UInt32 idx);
		bool SavePreset(UInt32 idx);
		bool LoadPreset(UInt32 idx);
		bool SavePreset(UInt32 idx, String^ path);
		bool LoadPreset(UInt32 idx, String^ path);
		void RegisterObserver(HostObserverProxyWrapper^ o);
		void UnregisterObserver(HostObserverProxyWrapper^ o);
	private:
		IHostController* hc;
	};
}