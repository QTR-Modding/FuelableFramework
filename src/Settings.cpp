#include "Settings.h"
#include "CLibUtilsQTR/PresetHelpers/PresetHelpersTXT.hpp"

namespace {
    std::vector<RE::FormID> ResolveFormIDs(const std::string& value, bool forceEditorID) {
        {
            std::shared_lock lock(PresetHelpers::formGroups_mutex_);
            if (const auto group = PresetHelpers::formGroups.find(value); group != PresetHelpers::formGroups.end()) {
                std::vector<RE::FormID> forms(group->second.begin(), group->second.end());
                std::ranges::sort(forms);
                return forms;
            }
        }
        if (forceEditorID) {
            if (auto* form = FormReader::GetFormByID(0, value)) return {form->GetFormID()};
            return {};
        }
        if (const auto form = FormReader::GetFormEditorIDFromString(value)) return {form};
        return {};
    }

    void AddSource(LightSource entry, const std::filesystem::path& file,
                   std::vector<LightSource>& sources, std::vector<std::filesystem::path>& origins) {
        const auto found = std::ranges::find_if(sources, [&](const LightSource& existing) {
            return existing.formid == entry.formid && existing.fuel == entry.fuel;
        });
        if (found != sources.end()) {
            const auto index = static_cast<std::size_t>(found - sources.begin());
            logger::warn("Configuration {} replaces source {:08X}/fuel {:08X} from {}.",
                         file.string(), entry.formid, entry.fuel, origins[index].string());
            *found = std::move(entry);
            origins[index] = file;
        } else {
            sources.push_back(std::move(entry));
            origins.push_back(file);
        }
    }

    SourceOptions LoadSourceOptions(const CSimpleIniA& ini) {
        SourceOptions options;
        options.auto_refuel = ini.GetBoolValue("Refueling", "AutoRefuel", options.auto_refuel);
        options.lorebox_enabled = ini.GetBoolValue("LoreBox", "Enabled", options.lorebox_enabled);
        options.lorebox_text = ini.GetValue("LoreBox", "Text", options.lorebox_text.c_str());
        options.skyprompt_enabled = ini.GetBoolValue("SkyPrompt", "Enabled", options.skyprompt_enabled);
        options.prompt_text = ini.GetValue("SkyPrompt", "Text", options.prompt_text.c_str());
        options.compatible_keys = Utilities::ParseHandshakeKeys(ini.GetValue("SkyPrompt", "CompatibleKeys", ""));
        return options;
    }

    void LoadSources(const CSimpleIniA& ini, const std::filesystem::path& file,
                     std::vector<LightSource>& sources, std::vector<std::filesystem::path>& origins) {
        CSimpleIniA::TNamesDepend names;
        ini.GetAllKeys(Settings::InISections[0], names);
        const auto options = LoadSourceOptions(ini);
        const bool forceEditorID = ini.GetBoolValue("Other Stuff", "ForceEditorID", Settings::force_editor_id) &&
                                   Settings::po3installed;
        for (const auto& key : names) {
            const char* source = ini.GetValue(Settings::InISections[0], key.pItem);
            const char* fuel = ini.GetValue(Settings::InISections[1], key.pItem);
            if (!source || !*source || !fuel || !*fuel) {
                logger::warn("Missing source or fuel for {} in {}. Skipping.", key.pItem, file.string());
                continue;
            }
            const char* durationText = ini.GetValue(Settings::InISections[2], key.pItem);
            const double duration = durationText && *durationText ?
                ini.GetDoubleValue(Settings::InISections[2], key.pItem, 0.0) : std::strtod(Settings::default_duration, nullptr);
            if (!std::isfinite(duration) || duration <= 0.0 || duration > std::numeric_limits<float>::max()) {
                logger::warn("Invalid duration for {} in {}. Skipping.", key.pItem, file.string());
                continue;
            }
            const auto sourceIDs = ResolveFormIDs(source, forceEditorID);
            const auto fuelIDs = ResolveFormIDs(fuel, forceEditorID);
            if (sourceIDs.empty() || fuelIDs.empty()) {
                logger::warn("Empty group or unresolved source/fuel for {} in {}. Skipping.", key.pItem, file.string());
                continue;
            }
            for (const auto sourceID : sourceIDs) {
                for (const auto fuelID : fuelIDs) {
                    LightSource entry(sourceID, static_cast<float>(duration), fuelID, options);
                    if (entry.init_failed) {
                        logger::warn("Invalid source {:08X}/fuel {:08X} for {} in {}. Skipping.",
                                     sourceID, fuelID, key.pItem, file.string());
                        continue;
                    }
                    AddSource(std::move(entry), file, sources, origins);
                    logger::info("Loaded source {:08X} with fuel {:08X} and duration {} from {}.",
                                 sourceID, fuelID, duration, file.string());
                }
            }
        }
    }
}

std::vector<LightSource> Settings::LoadINISettings() {
    logger::info("Loading ini settings");
    if (Utilities::IsPo3Installed()) {
        logger::info("powerofthree's Tweaks is installed. Enabling EditorID support.");
        po3installed = true;
    } else {
        logger::info("powerofthree's Tweaks is not installed. Disabling EditorID support.");
        po3installed = false;
    }

    PresetHelpers::TXT_Helpers::GatherForms("Data/SKSE/Plugins/FuelableFramework/formGroups");

    CSimpleIniA ini;
    std::vector<LightSource> lightSources;
    std::vector<std::filesystem::path> origins;

    ini.SetUnicode();
    ini.LoadFile(Utilities::path);

    // Create Sections with defaults if they don't exist
    for (int i = 0; i < InISections.size(); ++i) {
        
        logger::info("Checking section {}", InISections[i]);
        if (ini.SectionExists(InISections[i])) continue;

        logger::info("Section {} does not exist. Creating it.", InISections[i]);
        ini.SetValue(InISections[i], nullptr, nullptr);
        logger::info("Setting default keys for section {}", InISections[i]);
        if (i < 3) {
            ini.SetValue(InISections[i], InIDefaultKeys[i], nullptr, section_comments[i].c_str());
            logger::info("Default values set for section {}", InISections[i]);
        } else if (i==3) {
            logger::info("Creating Other Stuff section");
            for (auto it = other_stuff_defaults.begin(); it != other_stuff_defaults.end(); ++it) {
                auto it2 = it->second.begin();
                ini.SetBoolValue(InISections[i], it->first, it2->first, it2->second);
            }
        } else if (i == 4) {
            logger::info("Creating QuickLight Support section");
            ini.SetValue(InISections[i], "Enabled", "false");
            ini.SetValue(InISections[i], "Fuel", "0");
            ini.SetValue(InISections[i], "Duration", default_duration);
            //ini.SetValue(InISections[i], "RadiusX", "700");
            //ini.SetValue(InISections[i], "RadiusY", "700");
            //ini.SetValue(InISections[i], "RadiusZ", "700");
        }
    }

    // Sections: QuickLight Support
    ql_enabled = ini.GetBoolValue(InISections[4], "Enabled", false);
    if (ql_enabled) {
        const auto temp_fuel_id = std::string(ini.GetValue(InISections[4], "Fuel"));
        ql_fuel = FormReader::GetFormEditorIDFromString(temp_fuel_id);
        ql_formid = FormReader::GetFormEditorIDFromString(ql_editorid);
        if (!ql_formid || !ql_fuel) {
            logger::error("QuickLight is enabled but formid or fuel is invalid. Disabling QuickLight.");
            ql_enabled = false;
        }
        else if (!ql_cost.Initialize(RE::TESForm::LookupByID(ql_fuel))) {
            logger::error("QuickLight is enabled but fuel formid is invalid. Disabling QuickLight.");
            ql_enabled = false;
        }
        else {
            ql_duration = std::stof(ini.GetValue(InISections[4], "Duration"));
            if (ql_duration < 0.1f) {
                logger::warn("QuickLight duration is invalid. Using default value.");
                ql_duration = std::stof(default_duration);
                ini.SetValue(InISections[4], "Duration", default_duration);
            }
            /*const auto temp_radius_x = std::clamp(std::stof(ini.GetValue(InISections[4], "RadiusX","700")), 100.f, 1500.f);
            const auto temp_radius_y = std::clamp(std::stof(ini.GetValue(InISections[4], "RadiusY","700")), 100.f, 1500.f);
            const auto temp_radius_z = std::clamp(std::stof(ini.GetValue(InISections[4], "RadiusZ","700")), 100.f, 1500.f);
            ql_radius = Float3(temp_radius_x, temp_radius_y, temp_radius_z);
            logger::info("QuickLight enabled with formid: {}, duration: {}, radius: x {}, y {}, z {}", ql_formid,
                         ql_duration, ql_radius.x, ql_radius.y, ql_radius.z);*/
        }
    };
    

    // Sections: Other stuff
    // get from user
    CSimpleIniA::TNamesDepend other_stuff_userkeys;
    ini.GetAllKeys(InISections[3], other_stuff_userkeys);
    for (CSimpleIniA::TNamesDepend::const_iterator it = other_stuff_userkeys.begin(); it != other_stuff_userkeys.end();
         ++it) {
        for (auto it2 = other_stuff_defaults.begin(); it2 != other_stuff_defaults.end(); ++it2) {
            if (static_cast<std::string_view>(it->pItem) == static_cast<std::string_view>(it2->first)) {
                logger::info("Found key: {}", it->pItem);
                auto it3 = it2->second.begin();
                auto val = ini.GetBoolValue(InISections[3], it->pItem, it3->first);
                ini.SetBoolValue(InISections[3], it->pItem, val, it3->second);
                break;
            }
        }
    }

    // set stuff which is not found
    for (auto it = other_stuff_defaults.begin(); it != other_stuff_defaults.end(); ++it) {
        if (ini.KeyExists(InISections[3], it->first)) continue;
        auto it2 = it->second.begin();
        ini.SetBoolValue(InISections[3], it->first, it2->first, it2->second);
    }

    force_editor_id = ini.GetBoolValue(InISections[3],
                                       OtherStuffDefKeys[0]);  // logger::info("force_editor_id: {}", force_editor_id);
    enabled_plyrmsg = ini.GetBoolValue(InISections[3],
                                       OtherStuffDefKeys[1]);  // logger::info("enabled_plyrmsg: {}", enabled_plyrmsg);
    enabled_remainingmsg = ini.GetBoolValue(
        InISections[3], OtherStuffDefKeys[2]);  // logger::info("enabled_remainingmsg: {}", enabled_remainingmsg);
    enabled_err_msgbox = ini.GetBoolValue(
        InISections[3], OtherStuffDefKeys[3]);  // logger::info("enabled_err_msgbox: {}", enabled_err_msgbox);

    LoadSources(ini, Utilities::path, lightSources, origins);
    ini.SaveFile(Utilities::path);

    constexpr auto sources_folder = L"Data/SKSE/Plugins/FuelableFramework";
    std::error_code error;
    if (std::filesystem::exists(sources_folder, error)) {
        std::vector<std::filesystem::path> files;
        for (std::filesystem::recursive_directory_iterator it(sources_folder, error), end;
             !error && it != end; it.increment(error)) {
            const bool regular = it->is_regular_file(error);
            if (error) break;
            if (regular && _wcsicmp(it->path().extension().c_str(), L".ini") == 0) {
                files.push_back(it->path());
            }
        }
        if (error) logger::error("Failed to read configuration folder: {}", error.message());
        std::ranges::sort(files);
        for (const auto& file : files) {
            CSimpleIniA source_ini;
            source_ini.SetUnicode();
            if (source_ini.LoadFile(file.c_str()) < 0) {
                logger::error("Failed to load configuration: {}", file.string());
                continue;
            }
            LoadSources(source_ini, file, lightSources, origins);
        }
    } else if (error) {
        logger::error("Failed to read configuration folder: {}", error.message());
    }

    return lightSources;
}
