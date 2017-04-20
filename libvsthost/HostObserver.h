#ifndef HOSTOBSERVER_H
#define HOSTOBSERVER_H

#include <vector>
#include <cstdint>

namespace VSTHost {
class HostObserver;
class HostSubject {
public:
	enum HostEvent {
		Added,
		Deleted,
		ListLoaded,
		MovedUp,
		MovedDown,
		EditorShown,
		EditorHidden,
		PresetSet,
		BypassSet,
		ActiveSet,
		StateLoaded
	};
	void Notify(HostEvent e, std::uint32_t v1 = 0, std::uint32_t v2 = 0);
	HostSubject();
	void Register(HostObserver* o);
	void Unregister(HostObserver* o);
private:
	std::vector<HostObserver*> observers;
};

class HostObserver {
	friend void HostSubject::Notify(HostEvent, std::uint32_t, std::uint32_t);
public:
	HostObserver();
	virtual ~HostObserver();
protected:
	virtual void OnPluginAdded(std::uint32_t idx) = 0;
	virtual void OnPluginDeleted(std::uint32_t idx) = 0;
	virtual void OnListLoaded() = 0;
	virtual void OnMovedUp(std::uint32_t idx) = 0;
	virtual void OnMovedDown(std::uint32_t idx) = 0;
	virtual void OnEditorShown(std::uint32_t idx) = 0;
	virtual void OnEditorHidden(std::uint32_t idx) = 0;
	virtual void OnPresetSet(std::uint32_t plugin_idx, std::uint32_t preset_idx) = 0;
	virtual void OnBypassSet(std::uint32_t idx, bool bypass) = 0;
	virtual void OnActiveSet(std::uint32_t idx, bool active) = 0;
	virtual void OnStateLoaded(std::uint32_t idx) = 0;
};
} // namespace

#endif