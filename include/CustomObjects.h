#pragma once
#include "Utils.h"

//struct Float3 {
//    float x, y, z;
//
//    Float3(float x, float y, float z) : x(x), y(y), z(z){
//        x = std::max(x, 0.f);
//        y = std::max(y, 0.f);
//        z = std::max(z, 0.f);
//    };
//
//    Float3() : x(0.f), y(0.f), z(0.f) {}
//
//    void operator=(const Float3& other) {
//        x = std::max(other.x,0.f);
//        y = std::max(other.y,0.f);
//        z = std::max(other.z,0.f);
//    };
//    
//    void operator=(const RE::NiPoint3& other) {
//        x = std::max(other.x, 0.f);
//        y = std::max(other.y, 0.f);
//        z = std::max(other.z, 0.f);
//    };
//
//    float GetLength() const { return std::sqrt(x * x + y * y + z * z); };
//};

struct FormID2 {
    uint32_t outerKey;
    uint32_t innerKey;

    // Comparison operator for using Key as std::map key
    bool operator<(const FormID2& other) const {
        return outerKey < other.outerKey || (outerKey == other.outerKey && innerKey < other.innerKey);
    }
};

struct SourceOptions {
    bool auto_refuel = true;
    bool lorebox_enabled = true;
    bool skyprompt_enabled = true;
    std::string lorebox_text = "Remaining: {remaining}. Requires: {requirements}.";
    std::string prompt_text = "Refuel ({requirements})";
    std::vector<std::uint64_t> compatible_keys;
};

struct FuelCost {
    struct Ingredient {
        RE::TESBoundObject* object;
        std::int32_t count;
    };
    std::vector<Ingredient> ingredients;
    std::string requirements;

    bool Initialize(RE::TESForm* form);
    bool CanConsume(RE::PlayerCharacter* actor) const;
    bool Consume(RE::PlayerCharacter* actor) const;
};

struct LightSource {
    float duration;
    RE::FormID formid;
    RE::FormID fuel;
    SourceOptions options;
    FuelCost cost;
    RE::TESBoundObject* object = nullptr;

    bool init_failed=false;

    LightSource(RE::FormID id, float duration, RE::FormID fuelid, const SourceOptions& source_options = {})
        : duration(duration), formid(id), fuel(fuelid), options(source_options),
          object(FormReader::GetFormByID<RE::TESBoundObject>(id)) {
        init_failed = !object || !cost.Initialize(FormReader::GetFormByID(fuel));
        if (init_failed) {
            logger::error("Failed to initialize source {:08X} with fuel {:08X}", formid, fuel);
        }
    }

    bool operator < (const LightSource& other) const { return formid < other.formid; }


    float remaining = 0.f;
    float elapsed = 0.f;

    std::string_view GetName() const {
        return object->GetName();
    };

    std::string_view GetFuelName() const {
        return cost.requirements;
    };

    RE::TESBoundObject* GetBoundObject() const { return object; };


};

struct UISource {
    std::string name;
    std::string fuel_name;
    std::string duration;
};
