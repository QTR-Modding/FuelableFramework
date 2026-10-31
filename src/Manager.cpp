#include "Manager.h"

void LightSourceManager::Init() {
    Clear();
    for (auto& source : sources) {
        source.remaining = 0.0f;
        source.elapsed = 0.0f;
    }

    current_source = nullptr;
    is_burning = false;
    // allow_equip_event_sink = true;

    __restart = true;

    logger::info("LightSourceManager initialized.");
}
void LightSourceManager::UpdateLoop() {
    std::lock_guard lock(state_mutex);
    if (!current_source) return;
    const auto cal = RE::Calendar::GetSingleton();
    if (RE::UI::GetSingleton()->GameIsPaused()) return;
    current_source->elapsed = cal->GetHoursPassed() - __start_h;
    if (!HasFuel(current_source)) NoFuel();
}

float LightSourceManager::GetRemaining(const LightSource* src) const {
    std::lock_guard lock(state_mutex);
    return std::max(0.0f, src->remaining - src->elapsed);
}

LightSource* LightSourceManager::FindSource(RE::FormID source, RE::FormID fuel) {
    auto found = std::ranges::find_if(sources, [=](const LightSource& entry) {
        return entry.formid == source && entry.fuel == fuel;
    });
    return found == sources.end() ? nullptr : &*found;
}

bool LightSourceManager::Refuel(LightSource& source) {
    std::lock_guard lock(state_mutex);
    if (!source.cost.Consume(RE::PlayerCharacter::GetSingleton())) return false;
    const bool resume = current_source == &source && is_burning && isRunning();
    if (resume) PauseBurn();
    source.remaining = std::max(0.0f, source.remaining) + source.duration;
    if (Settings::enabled_plyrmsg) MsgBoxesNotifs::InGame::Refuel(source.GetName(), source.GetFuelName());
    if (resume) UnPauseBurn();
    return true;
}
bool LightSourceManager::SetSourceWithRemaningFuel(RE::FormID eqp_obj) {
    for (auto& src : sources) {
        if (eqp_obj == src.formid && HasFuel(&src)) {
            current_source = &src;
            logger::trace("Remainingli olan bi tane buldum: {} {}", src.remaining, current_source->remaining);
            return true;
        }
    }
    return false;
}

bool LightSourceManager::SetSource(RE::FormID eqp_obj, RE::FormID fuel_obj) {
    logger::trace("{}", fuel_obj);
    // fuel_obj dayatmasi varsa onu oncelikle
    if (fuel_obj) {
        for (auto& src : sources) {
            if (eqp_obj == src.formid && fuel_obj == src.fuel) {
                current_source = &src;
                return true;
            }
        }
    }
    // yoksa remaining fuel olanlardan birini bul
    if (SetSourceWithRemaningFuel(eqp_obj)) return true;
    // yoksa ilk buldugunu al
    for (auto& src : sources) {
        if (eqp_obj == src.formid) {
            current_source = &src;
            return true;
        }
    }
    logger::error("Did not find a match!!!");
    return false;
}

void LightSourceManager::LogRemainings() {
    for (auto& src : sources) {
        logger::info("Remaining hours for {}: {}", src.GetName(), src.remaining);
    }
}

void LightSourceManager::Reset() {
    std::lock_guard lock(state_mutex);
    Stop();
    Init();
}
void LightSourceManager::SendData() {
    std::lock_guard lock(state_mutex);
    for (auto& src : sources) {
        SetData({src.formid, src.fuel}, src.remaining);
    }
}
void LightSourceManager::ReceiveData() {
    std::lock_guard lock(state_mutex);
    for (auto& src : sources) {
        for (const auto& [formId2, value] : m_Data) {
            if (formId2.outerKey == src.formid && formId2.innerKey == src.fuel) {
                src.remaining = value;
                break;
            }
        }
    }
}
void LightSourceManager::HandleEquip(RE::FormID a_formid, bool equipped) {
    std::lock_guard lock(state_mutex);
    if (equipped) {
        if (IsCurrentSource(a_formid)) return;
        if (!SetSource(a_formid)) logger::info("Failed to set source. Something is terribly wrong!!!");
        StartBurn();
    } else {
        if (!IsCurrentSource(a_formid)) return;
        StopBurn();
    }
};

float LightSourceManager::PauseBurn() {
    std::lock_guard lock(state_mutex);
    Stop();
    if (!current_source) return 0.0f;
    if (is_burning && !__restart) {
        current_source->elapsed = RE::Calendar::GetSingleton()->GetHoursPassed() - __start_h;
    }
    current_source->remaining -= current_source->elapsed;
    current_source->elapsed = 0.f;
    __restart = true;
    return current_source->remaining;  // sadece nofuel icinde kullanmak icin
};

void LightSourceManager::UnPauseBurn() {
    std::lock_guard lock(state_mutex);
    __start_h = RE::Calendar::GetSingleton()->GetHoursPassed();
    __restart = false;
    Start();
}

void LightSourceManager::NoFuel() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* exhausted = current_source;
    const float overrun = std::min(0.0f, PauseBurn());
    exhausted->remaining = 0.0f;
    std::vector<std::string_view> requirements;
    for (auto& source : sources) {
        if (source.formid != exhausted->formid) continue;
        requirements.push_back(source.GetFuelName());
        if (HasFuel(&source) || (source.options.auto_refuel && Refuel(source))) {
            source.remaining += overrun;
            current_source = &source;
            UnPauseBurn();
            return;
        }
    }
    is_burning = false;
    current_source = nullptr;
    if (Settings::enabled_plyrmsg) {
        MsgBoxesNotifs::InGame::NoFuel(exhausted->GetName(), Utilities::String::join(requirements, " or "));
    }
    RE::ActorEquipManager::GetSingleton()->UnequipObject(player, exhausted->GetBoundObject(), nullptr, 1,
                                                       nullptr, true, false, false);
}
void LightSourceManager::ShowRemaining() {
    if (!Settings::enabled_remainingmsg) return;
    int _remaining = Utilities::Round2Int(current_source->remaining);
    MsgBoxesNotifs::InGame::Remaining(_remaining, GetName());
};

bool LightSourceManager::IsValidSource(RE::FormID eqp_obj) {
    for (auto& src : sources) {
        if (eqp_obj == src.formid) {
            return true;
        }
    }
    return false;
}
bool LightSourceManager::IsCurrentSource(RE::FormID eqp_obj) {
    if (!current_source) return false;
    return eqp_obj == current_source->formid;
}

void LightSourceManager::StartBurn() {
    std::lock_guard lock(state_mutex);
    if (is_burning) return;
    if (!current_source) {
        logger::error("No current source!!No current source!!No current source!!No current source!!");
        MsgBoxesNotifs::Windows::GeneralErr();
        return;
    }
    ShowRemaining();
    is_burning = true;
    UnPauseBurn();
}
void LightSourceManager::StopBurn() {
    std::lock_guard lock(state_mutex);
    PauseBurn();
    is_burning = false;
    current_source = nullptr;
};
