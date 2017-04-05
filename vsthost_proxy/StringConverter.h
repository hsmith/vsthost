#pragma once

#include <string>

namespace VSTHost {
	class StringConverter {
	public:
		static std::string Convert(System::String^ str) {
			System::IntPtr c_str = System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str);
			std::string ret((const char*)c_str.ToPointer());
			System::Runtime::InteropServices::Marshal::FreeHGlobal(c_str);
			return ret;
		}
		static System::String^ Convert(const std::string& str) {
			return gcnew System::String(str.c_str());
		}
	};
}