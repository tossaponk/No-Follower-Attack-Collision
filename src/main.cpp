﻿#include "NFAC/NFAC.h"

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "loki_NoFollowerAttackCollision.log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info("loki_NoFollowerAttackCollision v1.0.0");

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "loki_NoFollowerAttackCollision";
	a_info->version = 1;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {

	auto ptr = Loki::NoFollowerAttackCollision::GetSingleton();

	switch (message->type) {
		case SKSE::MessagingInterface::kDataLoaded: {
			ptr->InstallInputSink();
			break;
		}
		case SKSE::MessagingInterface::kNewGame: {
			break;
		}
		case SKSE::MessagingInterface::kPostLoadGame: {
			break;
		}
		case SKSE::MessagingInterface::kPostLoad: {
			ptr->InstallMeleeHook();
			ptr->InstallSweepHook();
			ptr->InstallArrowHook();
			ptr->InstallVaildTargetHook();
			ptr->InstallMagicHitHook();
			//ptr->InstallMagicApplyHook();
			//ptr->InstallOnHitEventSink();
			break;
		}
		case SKSE::MessagingInterface::kPostPostLoad: {
			break;
		}
		default:
			break;
	}

}

#ifdef SKYRIM_SUPPORT_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() 
{
	SKSE::PluginVersionData v;

	v.PluginVersion(1);
	v.PluginName("loki_NoFollowerAttackCollision"sv);

	v.UsesAddressLibrary(true);
	//v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#endif

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse)
{
	logger::info("NoFollowerAttackCollision loaded");
	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(64);

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", SKSEMessageHandler)) { // add callbacks for TrueHUD
		return false;
	}

	return true;
}


// 0x627930 + 0x2DD     37650	627930
// 0x627930 + 0x374     37650	627930
// 0x62B870 + 0xDD      37689	62b870