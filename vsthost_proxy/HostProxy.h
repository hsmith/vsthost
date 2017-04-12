#pragma once

using System::String;
using System::Byte;
using System::Int16;
using System::Int64;

namespace VSTHost {
	class Host;
	ref class HostControllerProxy;
	public ref class HostProxy
	{
	public:
		HostProxy(Int64 max_num_samples, double sample_rate);
		~HostProxy();
		!HostProxy();
		void Process(array<array<float>^>^ input, array<array<float>^>^ output, Int64 num_samples);
		void Process(array<Byte>^ input, array<Byte>^ output, Int64 num_samples);
		void Process(array<Int16>^ input, array<Int16>^ output, Int64 num_samples);
		void SetSampleRate(double sr);
		void SetBlockSize(Int64 bs);
		void CreateGUIThread();
		void CreateGUI();
		bool LoadPluginList(String^ path);
		bool SavePluginList(String^ path);
		bool LoadPluginList();
		bool SavePluginList();
		HostControllerProxy^ GetController();
	private:
		Host* host;
		Int64 max_samples;
		float** in_f;
		float** out_f;
		int channels;
		char* in_c;
		char* out_c;
		short* in_s;
		short* out_s;
	};
}
