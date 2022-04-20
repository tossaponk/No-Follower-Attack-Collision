#pragma once

#define NOMINMAX
//_CRT_SECURE_NO_WARNINGS
#define TRUEHUD_API_COMMONLIB
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "xbyak/xbyak.h"
//_CRT_SECURE_NO_WARNINGS;

//#include "C:/dev/steamworks_sdk_152/sdk/public/steam/steam_api.h"
//#pragma comment(lib, "steam_api64.lib");
//#define NOMINMAX
//#include <Windows.h>

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef SKSE_SUPPORT_XBYAK
[[nodiscard]] void* allocate(Xbyak::CodeGenerator& a_code);
#endif

using namespace std::literals;

namespace logger = SKSE::log;

#define DLLEXPORT __declspec(dllexport)

namespace stl
{
	//using nonstd::span;
	using SKSE::stl::report_and_fail;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}