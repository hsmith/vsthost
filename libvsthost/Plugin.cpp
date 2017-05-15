#include "Plugin.h"

#include "Preset.h"
#include "PluginWindow.h"

namespace VSTHost {
Plugin::Plugin(HMODULE m) : module(m) {

}

Plugin::~Plugin() {
	if (module) {
		//::Sleep(100);
		::FreeLibrary(module);
	}
}

std::string Plugin::GetPluginFileName() const {
	std::string ret = GetPluginPath();
	std::string::size_type pos = 0;
	if ((pos = ret.find_last_of('\\')) != std::string::npos)
		ret = ret.substr(pos + 1);
	return ret;
}

std::string Plugin::GetPluginDirectory() const {
	std::string ret = GetPluginPath();
	std::string::size_type pos = 0;
	if ((pos = ret.find_last_of('\\')) != std::string::npos)
		ret = ret.substr(0, pos);
	return ret;
}

std::string Plugin::GetPluginPath() const {
	char buf[MAX_PATH] = { 0 };
	::GetModuleFileNameA(module, buf, MAX_PATH);
	return std::string(buf);;
}

void Plugin::SetActive(bool active_) {
	if (active != active_) {
		std::lock_guard<std::mutex> lock(plugin_lock);
		if (active_)
			Resume();
		else
			Suspend();
	}
}

bool Plugin::IsActive() const {
	return active;
}

bool Plugin::IsBypassed() const {
	return bypass;
}

bool Plugin::HasEditor() const {
	return has_editor;
}

bool Plugin::SaveState() {
	if (state)
		return state->Save();
	return false;
}

bool Plugin::LoadState() {
	if (state)
		return state->Load();
	return false;
}

bool Plugin::SaveState(const std::string& path) {
	if (state)
		return state->Save(path);
	return false;
}

bool Plugin::LoadState(const std::string& path) {
	if (state)
		return state->Load(path);
	return false;
}

Steinberg::uint32 Plugin::GetChannelCount() {
	return static_cast<Steinberg::uint32>(Steinberg::Vst::SpeakerArr::getChannelCount(speaker_arrangement));
}
} // namespace