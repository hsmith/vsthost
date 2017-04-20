#include "HostObserver.h"

namespace VSTHost {
HostObserver::HostObserver() {

}

HostObserver::~HostObserver() {

}

void HostSubject::Notify(HostEvent e, std::uint32_t v1, std::uint32_t v2) {
	switch (e) {
		case HostEvent::Added:
			for (auto o : observers)
				o->OnPluginAdded(v1);
			break;
		case HostEvent::Deleted:
			for (auto o : observers)
				o->OnPluginDeleted(v1);
			break;
		case HostEvent::ListLoaded:
			for (auto o : observers)
				o->OnListLoaded();
			break;
		case HostEvent::MovedUp:
			for (auto o : observers)
				o->OnMovedUp(v1);
			break;
		case HostEvent::MovedDown:
			for (auto o : observers)
				o->OnMovedDown(v1);
			break;
		case HostEvent::EditorShown:
			for (auto o : observers)
				o->OnEditorShown(v1);
			break;
		case HostEvent::EditorHidden:
			for (auto o : observers)
				o->OnEditorHidden(v1);
			break;
		case HostEvent::PresetSet:
			for (auto o : observers)
				o->OnPresetSet(v1, v2);
			break;
		case HostEvent::BypassSet:
			for (auto o : observers)
				o->OnBypassSet(v1, v2 != 0);
			break;
		case HostEvent::ActiveSet:
			for (auto o : observers)
				o->OnActiveSet(v1, v2 != 0);
			break;
		default:
			break;
	}
}

HostSubject::HostSubject() {

}

void HostSubject::Register(HostObserver* o) {
	if (o)
		observers.push_back(o);
}

void HostSubject::Unregister(HostObserver* o) {
	for (auto it = observers.begin(); it != observers.end(); ++it)
		if (*it == o) {
			observers.erase(it);
			break;
		}
}
} // namespace