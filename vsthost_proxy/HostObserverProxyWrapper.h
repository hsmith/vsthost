#pragma once
#pragma managed(push, off)
#pragma managed(pop)

#include "libvsthost/HostObserver.h"

using System::UInt32;
using namespace System;
#include <vcclr.h>  

namespace VSTHost {
	class HostObserver;
	ref class HostObserverProxyWrapper;
	public class HostObserverProxy : public HostObserver {
	public:
		HostObserverProxy(HostObserverProxyWrapper^ hopw);
		void OnPluginAdded(std::uint32_t idx) override;
		void OnPluginDeleted(std::uint32_t idx) override;
		void OnListLoaded() override;
		void OnMovedUp(std::uint32_t idx) override;
		void OnMovedDown(std::uint32_t idx) override;
		void OnEditorShown(std::uint32_t idx) override;
		void OnEditorHidden(std::uint32_t idx) override;
		void OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) override;
		void OnBypassSet(std::uint32_t idx, bool bypass) override;
	private:
		gcroot<HostObserverProxyWrapper^> wrapper;
	};

	public ref class HostObserverProxyWrapper abstract {
	public:
		HostObserverProxyWrapper();
		~HostObserverProxyWrapper();
		virtual void OnPluginAdded(UInt32 idx) = 0;
		virtual void OnPluginDeleted(UInt32 idx) = 0;
		virtual void OnListLoaded() = 0;
		virtual void OnMovedUp(UInt32 idx) = 0;
		virtual void OnMovedDown(UInt32 idx) = 0;
		virtual void OnEditorShown(UInt32 idx) = 0;
		virtual void OnEditorHidden(UInt32 idx) = 0;
		virtual void OnPresetSet(UInt32 plugin_idx, UInt32 preset_idx) = 0;
		virtual void OnBypassSet(UInt32 idx, bool bypass) = 0;
	internal:
		HostObserver* GetHostObserver();
	private:
		HostObserver* ho;
	};
}