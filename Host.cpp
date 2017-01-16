#include "Host.h"

#include <limits>
#include <cstring>
#include <fstream>
#include <iostream>

#include "base/source/fstring.h"

#include "Plugin.h"
#include "HostWindow.h"
#include "PluginLoader.h"

namespace VSTHost {
const std::string Host::kPluginList{ "vsthost.ini" };

Host::Host(std::int64_t block_size, double sample_rate)
	: block_size(block_size), sample_rate(sample_rate) {
	buffers[0] = nullptr;
	buffers[1] = nullptr;
	AllocateBuffers();
}

Host::~Host() {
	FreeBuffers();
}

bool Host::LoadPlugin(std::string path) {
	auto plugin = PluginLoader::Load(path, block_size, sample_rate);
	if (plugin) { // host now owns what plugin points at
		std::cout << "Loaded " << path << "." << std::endl;
		plugin->Initialize();
		plugins.push_back(std::move(plugin));
		return true;	
	}
	std::cout << "Could not load " << path << "." << std::endl;
	return false;
}

void Host::Process(float** input, float** output) {
	if (plugins.size() == 1 && !plugins.front()->BypassProcess())
		plugins.front()->Process(input, output);
	else if (plugins.size() > 1) {
		if (!plugins.front()->BypassProcess())
			plugins.front()->Process(input, buffers[1]);
		else
			for (unsigned i = 0; i < GetChannelCount(); ++i)
				std::memcpy(input, buffers[1], sizeof(input[0][0]) * block_size);
		unsigned i, last_processed = 1;
		for (i = 1; i < plugins.size() - 1; i++)
			if (!plugins[i]->BypassProcess()) {
				last_processed = (i + 1) % 2;
				plugins[i]->Process(buffers[i % 2], buffers[last_processed]);
			}
		if (!plugins.back()->BypassProcess())
			plugins.back()->Process(buffers[last_processed], output);
		else
			for (unsigned i = 0; i < GetChannelCount(); ++i)
				std::memcpy(buffers[last_processed], output, sizeof(input[0][0]) * block_size);
		/* clearer version, but with copying over all buffers every time
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(input, buffers[0], sizeof(input[0][0]) * block_size);
		unsigned i, last_processed = 0;
		for (i = 0; i < plugins.size() - 1; i++)
			if (!plugins[i]->BypassProcess()) {
				last_processed = (i + 1) % 2;
				plugins[i]->Process(buffers[i % 2], buffers[last_processed]);
			}
		}
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(buffers[last_processed], output, sizeof(input[0][0]) * block_size);
		*/
	}
	else
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			std::memcpy(output[i], input[i], sizeof(input[0][0]) * block_size);
}

void Host::Process(char* input, char* output) { // char != int8_t
	if (std::numeric_limits<char>::min() < 0)
		Process(reinterpret_cast<std::int8_t*>(input), reinterpret_cast<std::int8_t*>(output));
}

void Host::Process(std::int8_t* input, std::int8_t* output) {
	Process(reinterpret_cast<std::int16_t*>(input), reinterpret_cast<std::int16_t*>(output));
}

void Host::Process(std::int16_t* input, std::int16_t* output) {
	ConvertFrom16Bits(input, buffers[0]);
	if (plugins.size() == 1 && !plugins.front()->BypassProcess()) {
		plugins.front()->Process(buffers[0], buffers[1]); // if bypassprocess is true it will just go to memcpy at the bottom
		ConvertTo16Bits(buffers[1], output);
	}
	else if (plugins.size() > 1) {
		unsigned i, last_processed = 0;			// remember where the most recently processed buffer is,
		for (i = 0; i < plugins.size(); i++)	// so that it could be moved to output.
			if (!plugins[i]->BypassProcess()) { // check whether i can bypass calling process,
				last_processed = (i + 1) % 2;	// so that i can omit memcpying the buffers.
				plugins[i]->Process(buffers[i % 2], buffers[last_processed]);
			}
		ConvertTo16Bits(buffers[last_processed], output);
	}
	else
		std::memcpy(output, input, sizeof(input[0]) * block_size * GetChannelCount());
}

void Host::SetSampleRate(double sr) {
	sample_rate = sr;
	for (auto& p : plugins)
		p->SetSampleRate(sample_rate);
}

void Host::SetBlockSize(std::int64_t bs) {
	if (bs != block_size) {
		block_size = bs;
		FreeBuffers();
		AllocateBuffers();
		for (auto& p : plugins)
			p->SetBlockSize(block_size);
	}
}

Steinberg::tresult PLUGIN_API Host::getName(Steinberg::Vst::String128 name) {
	Steinberg::String str("VSTHost");
	str.copyTo16(name, 0, 7);
	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API Host::createInstance(Steinberg::TUID cid, Steinberg::TUID iid, void** obj) {
	*obj = nullptr;
	return Steinberg::kResultFalse;
}

void Host::CreateGUIThread() {
	std::thread gui_thread(&Host::CreateGUI, this);
	gui_thread.detach();
}

void Host::CreateGUI() {
	gui = std::unique_ptr<HostWindow>(new HostWindow(*this));
	gui->Initialize(NULL);
	gui->Go();
}

bool Host::LoadPluginList(std::string path) {
	std::string line;
	std::ifstream list(path);
	if (list.is_open()) {
		while (getline(list, line))
			if (!line.empty())
				LoadPlugin(line);
		for (auto& p : plugins)
			p->LoadStateFromFile();
		list.close();
		return true;
	}
	else
		std::cout << "Could not open " << path << '.' << std::endl;
	return false;
}

bool Host::SavePluginList(std::string path) {
	std::ofstream list(path, std::ofstream::out | std::ofstream::trunc);
	if (list.is_open()) {
		for (auto& p : plugins) {
			list << Plugin::kPluginDirectory + p->GetPluginFileName() << std::endl;
		}
		list.close();
		return true;
	}
	return false;
}

Steinberg::uint32 Host::GetChannelCount() const {
	return 2;
}

void Host::AllocateBuffers() {
	for (auto &b : buffers) {
		if (b)
			FreeBuffers();
		b = new Steinberg::Vst::Sample32*[GetChannelCount()]{};
		for (unsigned i = 0; i < GetChannelCount(); ++i)
			b[i] = new Steinberg::Vst::Sample32[block_size];
	}
}

void Host::FreeBuffers() {
	if (buffers[0] && buffers[1]) {
		for (auto &b : buffers) {
			for (unsigned i = 0; i < GetChannelCount(); ++i)
				if (b[i])
					delete[] b[i];
			delete b;
			b = nullptr;
		}
	}
}

void Host::ConvertFrom16Bits(std::int8_t* input, float** output) {
	ConvertFrom16Bits(reinterpret_cast<std::int16_t*>(input), output);
}

void Host::ConvertFrom16Bits(std::int16_t* input, float** output) {
	for (unsigned i = 0, in_i = 0; i < block_size; ++i)
		for (unsigned j = 0; j < GetChannelCount(); ++j, ++in_i)
			output[j][i] = input[in_i] / static_cast<float>(std::numeric_limits<std::int16_t>::max());
}

void Host::ConvertTo16Bits(float** input, std::int8_t* output) {
	ConvertTo16Bits(input, reinterpret_cast<std::int16_t*>(output));
}

void Host::ConvertTo16Bits(float** input, std::int16_t* output) {
	for (unsigned i = 0, out_i = 0; i < block_size; ++i)
		for (unsigned j = 0; j < GetChannelCount(); ++j, ++out_i)
			if (input[j][i] > 1)
				output[out_i] = std::numeric_limits<std::int16_t>::max();
			else if (input[j][i] < -1)
				output[out_i] = std::numeric_limits<std::int16_t>::min();
			else
				output[out_i] = static_cast<std::int16_t>(input[j][i] * std::numeric_limits<std::int16_t>::max());
}

void Host::SwapPlugins(size_t i, size_t j) {
	if (i < plugins.size() && j < plugins.size()) {
		std::swap(plugins[i], plugins[j]);
	}
}

void Host::DeletePlugin(size_t i) {
	if (i < plugins.size())
		plugins.erase(plugins.begin() + i);
}
} // namespace