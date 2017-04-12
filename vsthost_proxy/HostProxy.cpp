#include "Stdafx.h"
#include "HostProxy.h"
#include "libvsthost/Host.h"
#include "HostControllerProxy.h"
#include "StringConverter.h"


using namespace System;

namespace VSTHost {

	HostProxy::HostProxy(Int64 max_num_samples, double sample_rate) :
		host(new Host(max_num_samples, sample_rate)),
		max_samples(max_num_samples),
		in_f(nullptr),
		out_f(nullptr),
		channels(0),
		in_c(nullptr),
		out_c(nullptr),
		in_s(nullptr),
		out_s(nullptr) {

	}

	HostProxy::~HostProxy() {
		this->!HostProxy();
	}

	HostProxy::!HostProxy() {
		delete host;
		if (in_f) {
			for (int i = 0; i < channels; ++i) {
				delete[] in_f[i];
				delete[] out_f[i];
			}
			delete[] in_f;
			delete[] out_f;
		}
		if (in_c) {
			delete[] in_c;
			delete[] out_c;
		}
		if (in_s) {
			delete[] in_s;
			delete[] out_s;
		}
	}

	void HostProxy::Process(array<array<float>^>^ input, array<array<float>^>^ output, Int64 num_samples) {
		if (!in_f) {
			channels = input->Length; // assert input->Length == output->Length?
			in_f = new float*[channels];
			out_f = new float*[channels];
			for (int i = 0; i < channels; ++i) {
				in_f[i] = new float[max_samples];
				out_f[i] = new float[max_samples];
			}
		}
		for (int i = 0; i < channels; ++i)
			System::Runtime::InteropServices::Marshal::Copy(input[i], 0, IntPtr(in_f[i]), input[i]->Length);
		host->Process(in_f, out_f, num_samples);
		for (int i = 0; i < channels; ++i)
			System::Runtime::InteropServices::Marshal::Copy(IntPtr(out_f[i]), output[i], 0, input[i]->Length);
	}

	void HostProxy::Process(array<Byte>^ input, array<Byte>^ output, Int64 num_samples) {
		if (!in_c) {
			in_c = new char[max_samples * 2 * 2]; // stereo 16 bit
			out_c = new char[max_samples * 2 * 2];
		}
		const int size = input->Length;
		System::Runtime::InteropServices::Marshal::Copy(input, 0, IntPtr(in_c), size);
		host->Process(in_c, out_c, num_samples);
		System::Runtime::InteropServices::Marshal::Copy(IntPtr(out_c), output, 0, size);
	}

	void HostProxy::Process(array<Int16>^ input, array<Int16>^ output, Int64 num_samples) {
		if (!in_s) {
			in_s = new short[max_samples * 2];
			out_s = new short[max_samples * 2];
		}
		const int size = input->Length;
		System::Runtime::InteropServices::Marshal::Copy(input, 0, IntPtr(in_s), size);
		host->Process(in_s, out_s, num_samples);
		System::Runtime::InteropServices::Marshal::Copy(IntPtr(out_s), output, 0, size);
	}

	void HostProxy::SetSampleRate(double sr) {
		host->SetSampleRate(sr);
	}

	void HostProxy::SetBlockSize(Int64 bs) {
		host->SetBlockSize(bs);
	}

	void HostProxy::CreateGUIThread() {
		host->CreateGUIThread();
	}

	void HostProxy::CreateGUI() {
		host->CreateGUI();
	}

	bool HostProxy::LoadPluginList(String^ path) {
		return host->LoadPluginList(StringConverter::Convert(path));
	}

	bool HostProxy::SavePluginList(String^ path) {
		return host->SavePluginList(StringConverter::Convert(path));
	}

	bool HostProxy::LoadPluginList() {
		return host->LoadPluginList();
	}

	bool HostProxy::SavePluginList() {
		return host->SavePluginList();
	}

	HostControllerProxy^ HostProxy::GetController() {
		return gcnew HostControllerProxy(host->GetController());
	}

}