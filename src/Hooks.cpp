#include "Hooks.h"

using namespace Hooks;

void MagicCastHook::InstallHook() {
    REL::Relocation<std::uintptr_t> CasterVtbl{RE::VTABLE_LightEffect[0]};
    BaseFunction_Start = CasterVtbl.write_vfunc(0x14, Start);
    BaseFunction_Update = CasterVtbl.write_vfunc(0x04, Update);
    BaseFunction_Stop = CasterVtbl.write_vfunc(0x15, Stop);

    //last_radius = Settings::ql_radius;
}

void MagicCastHook::Start(RE::LightEffect* a_eff) {
    if (!a_eff || a_eff->GetBaseObject()->GetFormID() != Settings::ql_formid) return BaseFunction_Start(a_eff);
    logger::trace("aaaQLLightSelf detected. Duration: {}", Settings::ql_duration);
    if (ql_depleted) {
        if (Settings::ql_cost.Consume(RE::PlayerCharacter::GetSingleton())) {
            ql_elapsed_t = 0.f;
            ql_depleted = false;
        } else {
            NoFuelQL(a_eff);
            return BaseFunction_Start(a_eff);
        }
    }
    ql_last_t = RE::Calendar::GetSingleton()->GetHoursPassed();
    return BaseFunction_Start(a_eff);
}

void MagicCastHook::Update(RE::LightEffect* a_eff, float a_delta) {
    
    if (!a_eff || a_eff->GetBaseObject()->GetFormID() != Settings::ql_formid) return BaseFunction_Update(a_eff, a_delta);
    
    ql_total_delta += a_delta;
    if (ql_total_delta < 0.5f) return BaseFunction_Update(a_eff, a_delta);
    
    ql_total_delta = 0.f;
    const auto temp_t = RE::Calendar::GetSingleton()->GetHoursPassed();
    const auto temp_elapsed = temp_t - ql_last_t;  // hours
    ql_elapsed_t += temp_elapsed;
    ql_last_t = temp_t;
    logger::trace("Elapsed: {}", ql_elapsed_t);

    if (ql_elapsed_t >= Settings::ql_duration || ql_depleted) {
        if (Settings::ql_cost.Consume(RE::PlayerCharacter::GetSingleton())) {
            ql_elapsed_t = 0.f;
        } 
        else NoFuelQL(a_eff);
    }

    return BaseFunction_Update(a_eff, a_delta);
}

void MagicCastHook::Stop(RE::LightEffect* a_eff) {
    if (!a_eff || a_eff->GetBaseObject()->GetFormID() != Settings::ql_formid) return BaseFunction_Stop(a_eff);
    logger::trace("aaaQLLightSelf stopped.");
    ql_total_delta = 0.f;
    return BaseFunction_Stop(a_eff);
}

void NoFuelQL(RE::LightEffect* a_eff) {
    a_eff->Dispel(false);
    ql_depleted = true;
    if (Settings::enabled_plyrmsg) {
        MsgBoxesNotifs::InGame::NoFuel("Quick Light", Settings::ql_cost.requirements);
    }
}

void Hooks::InstallHooks(){
    if (Settings::ql_enabled) MagicCastHook::InstallHook();
    logger::info("MagicCastHook installed.");
};
