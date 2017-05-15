#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/base/funknown.h"

#include "Host.h"

namespace VSTHost {
class Plugin;
class PluginLoader;
class PluginManager {
	using IndexType = std::vector<std::unique_ptr<Plugin>>::size_type;
public:
	PluginManager(Steinberg::Vst::TSamples bs, Steinberg::Vst::SampleRate sr, Steinberg::Vst::SpeakerArrangement sa, Steinberg::FUnknown* context);
	IndexType Size() const;
	Plugin& GetAt(IndexType i) const;
	Plugin& operator[](IndexType i) const;
	Plugin& Front() const;
	Plugin& Back() const;
	std::mutex& GetLock();
	bool Add(const std::string& path);
	void Delete(IndexType i);
	void Swap(IndexType i, IndexType j);

	bool LoadPluginList(const std::string& path = Host::kPluginList);
	bool SavePluginList(const std::string& path = Host::kPluginList) const;

	void SetBlockSize(Steinberg::Vst::TSamples bs);
	void SetSampleRate(Steinberg::Vst::SampleRate sr);
	void SetSpeakerArrangement(Steinberg::Vst::SpeakerArrangement sa);

	std::vector<Plugin*>& GetQueue();
	void RemoveFromQueue(IndexType i);
	void AddToQueue(IndexType i);
private:
	void RecreateQueue();
	std::string GetRelativePath(const std::string& absolute) const;
	std::string GetAbsolutePath(const std::string& relative) const;
	Steinberg::Vst::TSamples def_block_size;		// default block size & sample rate
	Steinberg::Vst::SampleRate def_sample_rate;		// for new plugins
	Steinberg::Vst::SpeakerArrangement def_speaker_arrangement;
	Steinberg::FUnknown* vst3_context;
	std::vector<std::unique_ptr<Plugin>> plugins;
	std::mutex manager_lock; // used to not delete an element while iterating over all contents
	std::vector<Plugin*> queue;
};
} // namespace

#endif