#ifndef PRESETVST3_H
#define PRESETVST3_H

#include <string>
#include <Windows.h>

#ifndef UNICODE
#define UNICODE_OFF
#endif
#include "public.sdk/source/common/memorystream.h"

#include "Preset.h"

namespace VSTHost {
class PluginVST3;
class PresetVST3 : public Preset {
public:
	static const std::string kExtension;
	PresetVST3(PluginVST3& p);
	~PresetVST3();
	bool Load() override;
	bool Load(const std::string& path) override;
	bool Save() override;
	bool Save(const std::string& path) override;
private:
	void SetState() override;
	void GetState() override;
	Steinberg::MemoryStream edit_stream, processor_stream;
	Steinberg::FUID fuid;
	PluginVST3& plugin;
};
} // namespace

#endif