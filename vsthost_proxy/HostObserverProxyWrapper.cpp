#include "Stdafx.h"
#include "HostObserverProxyWrapper.h"

namespace VSTHost {
	HostObserverProxy::HostObserverProxy(HostObserverProxyWrapper^ hopw) {
		wrapper = hopw;
	}

	void HostObserverProxy::OnPluginAdded(std::uint32_t idx) {
		wrapper->OnPluginAdded(idx);
	}

	void HostObserverProxy::OnPluginDeleted(std::uint32_t idx) {
		wrapper->OnPluginDeleted(idx);
	}

	void HostObserverProxy::OnListLoaded() {
		wrapper->OnListLoaded();
	}

	void HostObserverProxy::OnMovedUp(std::uint32_t idx) {
		wrapper->OnMovedUp(idx);
	}

	void HostObserverProxy::OnMovedDown(std::uint32_t idx) {
		wrapper->OnMovedDown(idx);
	}

	void HostObserverProxy::OnEditorShown(std::uint32_t idx) {
		wrapper->OnEditorShown(idx);
	}

	void HostObserverProxy::OnEditorHidden(std::uint32_t idx) {
		wrapper->OnEditorHidden(idx);
	}

	void HostObserverProxy::OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) {
		wrapper->OnPresetSet(plugin_idx, preset_idx);
	}

	void HostObserverProxy::OnBypassSet(std::uint32_t idx, bool bypass) {
		wrapper->OnBypassSet(idx, bypass);
	}

	void HostObserverProxy::OnActiveSet(std::uint32_t idx, bool active) {
		wrapper->OnActiveSet(idx, active);
	}

	HostObserverProxyWrapper::HostObserverProxyWrapper() {
		ho = new HostObserverProxy(this);
	}

	HostObserverProxyWrapper::~HostObserverProxyWrapper() {
		this->!HostObserverProxyWrapper();
	}

	HostObserverProxyWrapper::!HostObserverProxyWrapper() {
		if (ho)
			delete ho;
	}

	HostObserver* HostObserverProxyWrapper::GetHostObserver() {
		return ho;
	}
}