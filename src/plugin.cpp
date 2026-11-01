#include "Events.h"
#include "MCP.h"
#include "Integrations.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    auto* LSM = LightSourceManager::GetSingleton();
    switch (message->type) {
        case SKSE::MessagingInterface::kNewGame:
            if (LSM->sources.empty() && !Settings::ql_enabled) {
                logger::info("No sources found.");
                if (Settings::enabled_err_msgbox) MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            }
            LSM->Reset();
            logger::info("Newgame LSM reset succesful.");
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            LSM->Reset();
            logger::info("Preload LSM reset succesful.");
            break;
        case SKSE::MessagingInterface::kPostLoadGame:
            if (LSM->sources.empty() && !Settings::ql_enabled) {
                logger::info("No sources found (PostLoad).");
                if (Settings::enabled_err_msgbox) MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            } else LSM->LogRemainings();
            if (LSM->current_source) LSM->StartBurn();
            logger::info("Postload LSM succesful.");
            break;
        case SKSE::MessagingInterface::kDataLoaded:
            // Start
            auto sources = Settings::LoadINISettings();
            
            Hooks::InstallHooks();
            logger::info("Hooks installed.");
            
            LSM = LightSourceManager::GetSingleton(sources, 500);
            auto* ourEventSink = OurEventSink::GetSingleton(LSM);
            auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            eventSourceHolder->AddEventSink<RE::TESEquipEvent>(ourEventSink);
            logger::info("Event sinks added.");

            UI::Register(LSM);
            logger::info("MCP registered.");
            Integrations::Install(LSM);
    }
}


void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
#endif
    logger::info("Name of the plugin is {}.", pluginName);
    logger::info("Version of the plugin is {}.", SKSE::PluginDeclaration::GetSingleton()->GetVersion());
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    //spdlog::set_pattern("%v");
    SKSE::Init(skse);
    logger::info("Plugin loaded");
    InitializeSerialization();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
