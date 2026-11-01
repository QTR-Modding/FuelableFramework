#pragma once

#include "Serialization.h"
#include "CLibUtilsQTR/Ticker.hpp"

class LightSourceManager : public Ticker, public BaseFormFloat {
    mutable std::recursive_mutex state_mutex;
    
    bool __restart;
    float __start_h;
    
    void Init();

    void UpdateLoop();

    inline std::string_view GetName() { return current_source->GetName(); };
    inline std::string_view GetFuelName() { return current_source->GetFuelName(); };
    inline RE::TESBoundObject* GetBoundObject() { return current_source->GetBoundObject(); };

    bool SetSourceWithRemaningFuel(RE::FormID eqp_obj);

    bool IsCurrentSource(RE::FormID eqp_obj);

    void NoFuel();

    inline bool HasFuel(LightSource* src) { return GetRemaining(src) > 0.0001f; };
    void ShowRemaining();

public:
    float GetRemaining(const LightSource* src) const;
    bool Refuel(LightSource& source);
    LightSource* FindSource(RE::FormID source, RE::FormID fuel);
    LightSourceManager(std::vector<LightSource>& data, std::chrono::milliseconds interval)
        : sources(data), Ticker([this]() {
              SKSE::GetTaskInterface()->AddTask([this] {
                  if (isRunning()) UpdateLoop();
              });
          }, interval) {
        instance = this;
        Init();
    };

    bool is_healthy = true;

    std::vector<LightSource> sources;  // these are not necessarily unique due to support for multiple fuels
    LightSource* current_source;
    bool is_burning;

    static LightSourceManager* GetSingleton(std::vector<LightSource>& data, int u_intervall) {
        static LightSourceManager singleton(data, std::chrono::milliseconds(u_intervall));
        return &singleton;
    }
    static LightSourceManager* GetSingleton() {
        return instance;
    }

    bool IsValidSource(RE::FormID eqp_obj);

    void StartBurn();
    float PauseBurn();
    void UnPauseBurn();
    void StopBurn();
    

    bool SetSource(RE::FormID eqp_obj, RE::FormID fuel_obj = 0);
    void LogRemainings();

    void Reset();

    void SendData();
    void ReceiveData();

    void HandleEquip(RE::FormID a_formid, bool equipped);

private:
    inline static LightSourceManager* instance = nullptr;
};
