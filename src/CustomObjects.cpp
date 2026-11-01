#include "CustomObjects.h"

bool FuelCost::Initialize(RE::TESForm* form) {
    ingredients.clear();
    requirements.clear();
    if (!form) return false;
    if (auto* item = form->As<RE::TESBoundObject>()) {
        ingredients.push_back({item, 1});
    } else if (auto* recipe = form->As<RE::BGSConstructibleObject>()) {
        bool valid = true;
        recipe->requiredItems.ForEachContainerObject([&](RE::ContainerObject& entry) {
            if (!entry.obj || entry.count <= 0) {
                valid = false;
                return RE::BSContainer::ForEachResult::kStop;
            }
            auto existing = std::ranges::find(ingredients, entry.obj, &Ingredient::object);
            if (existing == ingredients.end()) {
                ingredients.push_back({entry.obj, entry.count});
            } else {
                if (entry.count > std::numeric_limits<std::int32_t>::max() - existing->count) {
                    valid = false;
                    return RE::BSContainer::ForEachResult::kStop;
                }
                existing->count += entry.count;
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });
        if (!valid) {
            ingredients.clear();
            return false;
        }
    }
    if (ingredients.empty()) return false;
    for (const auto& ingredient : ingredients) {
        if (!requirements.empty()) requirements += ", ";
        requirements += ingredient.object->GetName();
        requirements += std::format(" ({})", ingredient.count);
    }
    return true;
}

bool FuelCost::CanConsume(RE::PlayerCharacter* actor) const {
    return !ingredients.empty() && std::ranges::all_of(ingredients, [actor](const Ingredient& entry) {
        return actor->GetItemCount(entry.object) >= entry.count;
    });
}

bool FuelCost::Consume(RE::PlayerCharacter* actor) const {
    if (!CanConsume(actor)) return false;
    for (const auto& ingredient : ingredients) {
        actor->RemoveItem(ingredient.object, ingredient.count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
    }
    return true;
}
