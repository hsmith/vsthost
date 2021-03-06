#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <string>
#include <memory>

#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/base/funknown.h"

namespace VSTHost {
class Plugin;
class PluginLoader {
public:
	static std::unique_ptr<Plugin> Load(const std::string& path, Steinberg::FUnknown* context, Steinberg::Vst::SpeakerArrangement sa);
};
} // namespace

#endif