#include "PluginManager.h"

#include <iostream>
#include <fstream>
#include <windows.h>

#include "Plugin.h"
#include "PluginLoader.h"

namespace VSTHost {
PluginManager::PluginManager(Steinberg::Vst::TSamples bs, Steinberg::Vst::SampleRate sr, Steinberg::Vst::SpeakerArrangement sa, Steinberg::FUnknown* context)
	: def_block_size(bs)
	, def_sample_rate(sr)
	, def_speaker_arrangement(sa)
	, vst3_context(context) {

}

PluginManager::IndexType PluginManager::Size() const {
	return plugins.size();
}

Plugin& PluginManager::GetAt(PluginManager::IndexType i) const {
	return *plugins[i];
}

Plugin& PluginManager::operator[](IndexType i) const {
	return GetAt(i);
}

Plugin& PluginManager::Front() const {
	return *plugins[0]; // checking whether there is at least 1 plugin is on the user
}

Plugin& PluginManager::Back() const {
	return *plugins[Size() - 1]; // same
}

std::mutex& PluginManager::GetLock() {
	return manager_lock;
}

bool PluginManager::Add(const std::string& path) {
	auto plugin = PluginLoader::Load(path, vst3_context, def_speaker_arrangement);
	if (plugin) { // host now owns what plugin points at
		try {
			plugin->Initialize(def_block_size, def_sample_rate);
			if (!plugin->SetSpeakerArrangement(def_speaker_arrangement)) {
				std::cout << plugin->GetPluginName() << " did not accept the speaker arrangement!" << std::endl;
				return false;
			}
			plugin->LoadState();
			plugins.push_back(std::move(plugin));
			std::lock_guard<std::mutex> lock(manager_lock);
			queue.push_back(plugins.back().get());
			std::cout << "Loaded " << path << "." << std::endl;
		}
		catch (...) {
			std::cout << "Error initializing plugin." << std::endl;
		}
		return true;
	}
	return false;
}

void PluginManager::Delete(IndexType i) {
	if (i < Size()) {
		{
			std::lock_guard<std::mutex> lock(manager_lock);
			auto it = queue.begin();
			while (*it != plugins[i].get())
				it++;
			if (it != queue.end())
				queue.erase(it);
		}
		try {
			plugins.erase(plugins.begin() + i);
		}
		catch (...) {
			plugins[i].release();
			plugins.erase(plugins.begin() + i);
			std::cout << "Error unloading plugin." << std::endl;
		}
	}
}

void PluginManager::Swap(IndexType i, IndexType j) {
	if (i < Size() && j < Size()) {
		std::swap(plugins[i], plugins[j]);
		RecreateQueue();
	}
}

bool PluginManager::LoadPluginList(const std::string& path) {
	{
		std::lock_guard<std::mutex> lock(manager_lock);
		queue.clear();
	}
	plugins.clear();
	std::string line;
	std::ifstream list(path);
	if (list.is_open()) {
		while (getline(list, line))
			if (!line.empty())
				Add(line);
		list.close();
		return true;
	}
	else
		std::cout << " Could not open " << path << '.' << std::endl;
	return false;
}

bool PluginManager::SavePluginList(const std::string& path) const {
	char abs[MAX_PATH]{};
	_fullpath(abs, ".", MAX_PATH);
	std::string absolute(abs);
	std::ofstream list(path, std::ofstream::out | std::ofstream::trunc);
	if (list.is_open()) {
		for (decltype(Size()) i = 0; i < Size(); ++i) {
			std::string relative = GetRelativePath(GetAt(i).GetPluginPath());
			if (!relative.empty())
				list << relative << std::endl;
			else
				list << GetAt(i).GetPluginPath();
		}
		list.close();
		return true;
	}
	return false;
}

void PluginManager::SetBlockSize(Steinberg::Vst::TSamples bs) {
	def_block_size = bs;
	for (auto& p : plugins)
		p->SetBlockSize(def_block_size);
}

void PluginManager::SetSampleRate(Steinberg::Vst::SampleRate sr) {
	def_sample_rate = sr;
	for (auto& p : plugins)
		p->SetSampleRate(def_sample_rate);
}

void PluginManager::SetSpeakerArrangement(Steinberg::Vst::SpeakerArrangement sa) {
	def_speaker_arrangement = sa;
	using idx_t = decltype(Size());
	std::vector<idx_t> to_delete;
	for (idx_t i = 0; i < Size(); ++i)
		if (!plugins[i]->SetSpeakerArrangement(def_speaker_arrangement)) {
			to_delete.push_back(i); // registering which plugins did not accept new SpeakerArrangement
			std::cout << plugins[i]->GetPluginName() << " did not accept new speaker arrangement." << std::endl;
		}
	if (to_delete.size()) { // to unload these plugins, going from back to front to not disrupt indices
		for (auto it = to_delete.crbegin(); it != to_delete.crend(); ++it) { 
			auto idx = *it; // code is duplicated b/o a deadlock
			try {			// TODO: make it better
				plugins.erase(plugins.begin() + idx);
			}
			catch (...) {
				plugins[idx].release();
				plugins.erase(plugins.begin() + idx);
				std::cout << "Error unloading plugin." << std::endl;
			}
		}
		queue.clear();
		for (auto& p : plugins)
			if (p->IsActive() && !p->IsBypassed())
				queue.push_back(p.get());
	}
}

std::vector<Plugin*>& PluginManager::GetQueue() {
	return queue;
}

void PluginManager::RemoveFromQueue(IndexType i) {
	if (i < Size()) {
		std::lock_guard<std::mutex> lock(manager_lock);
		auto it = queue.begin();
		while (it != queue.end() && *it != plugins[i].get())
			it++;
		if (it != queue.end())
			queue.erase(it);
	}
}

void PluginManager::AddToQueue(IndexType i) {
	RecreateQueue(); // lazy way
}

void PluginManager::RecreateQueue() {
	std::lock_guard<std::mutex> lock(manager_lock);
	queue.clear();
	for (auto& p : plugins)
		if (p->IsActive() && !p->IsBypassed())
			queue.push_back(p.get());
}

std::string PluginManager::GetRelativePath(const std::string& absolute) const {
	const auto current_directory = GetAbsolutePath(".");
	if (current_directory[0] == absolute[0]) {
		std::string temp = current_directory, result = ".\\", prefix;
		auto parent = [](std::string& s) {
			std::string::size_type pos = 0;
			if ((pos = s.find_last_of("\\")) != std::string::npos) {
				s = s.substr(0, pos);
			}
			else {
				pos = s.find_last_of(":");
				s = s.substr(0, pos);
			}
		};
		std::string::size_type pos = 0;
		while ((pos = absolute.find(temp)) == std::string::npos) {
			prefix.append("..\\");
			parent(temp);
		}
		result.append(prefix).append(absolute.substr(temp.size() + 1));
		return result;
	}
	else
		return "";
}

std::string PluginManager::GetAbsolutePath(const std::string& relative) const {
	char tmp[MAX_PATH]{};
	_fullpath(tmp, relative.c_str(), MAX_PATH);
	return std::string(tmp);
}
} // namespace