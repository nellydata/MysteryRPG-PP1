#include "ConsumableEffects.h"
#include "Area.h"
#include "CombatManager.h"
#include "CombatMath.h"
#include "Enemy.h"
#include "Player.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>

namespace {
const std::array<ConsumableEffectDefinition, 12> Effects{{
    {"restore_20_percent_hp", ConsumableEffectKind::RestoreHealth, ConsumableContext::Any, 20, 0},
    {"restore_35_percent_hp_and_cleanse_bleed", ConsumableEffectKind::RestoreAndCleanseBleed, ConsumableContext::Any, 35, 0},
    {"restore_50_percent_hp", ConsumableEffectKind::RestoreHealth, ConsumableContext::Any, 50, 0},
    {"increase_acc_by_10_percent_for_3_turns", ConsumableEffectKind::AccuracyUp, ConsumableContext::Combat, 10, 3},
    {"increase_def_by_15_percent_for_3_turns", ConsumableEffectKind::DefenseUp, ConsumableContext::Combat, 15, 3},
    {"reduce_shimmer_skill_cooldowns_by_1", ConsumableEffectKind::ReduceCooldowns, ConsumableContext::Combat, 1, 0},
    {"remove_all_debuffs", ConsumableEffectKind::CleanseNegative, ConsumableContext::Any, 0, 0},
    {"remove_burn_and_poison", ConsumableEffectKind::CleanseBurnPoison, ConsumableContext::Any, 0, 0},
    {"apply_burn_60_percent_atk", ConsumableEffectKind::ApplyBurn, ConsumableContext::Combat, 60, 3},
    {"deal_flat_50_damage", ConsumableEffectKind::FlatDamage, ConsumableContext::Combat, 50, 0},
    {"illuminate_dark_rooms_for_30_seconds", ConsumableEffectKind::Illuminate, ConsumableContext::Exploration, 0, 30},
    {"lubricate_mechanisms_unlocking_rusted_doors", ConsumableEffectKind::UnlockRustedLock, ConsumableContext::Exploration, 0, 0}
}};

int RestorationAmount(const Player& player, int percent) {
    const auto amount = static_cast<std::int64_t>(player.getMaxHP()) * percent / 100;
    return static_cast<int>((std::min)(amount, static_cast<std::int64_t>((std::numeric_limits<int>::max)())));
}

bool CanRestoreHealth(const Player& player, int amount) {
    if (!player.isAlive() || player.getHP() >= player.getMaxHP()) return false;
    int resistance = 0;
    for (const auto& status : player.GetStatuses())
        if (status.type == StatusType::HealResistance) resistance += status.potency;
    std::int64_t effective = static_cast<std::int64_t>(amount) * (100 - (std::clamp)(resistance, 0, 100)) / 100;
    effective = effective * (100 + (std::clamp)(player.getHealingPotency(), 0, CombatMath::Balance::MaximumHealingPotency)) / 100;
    effective = effective * (100 + (std::clamp)(player.getStats().healingReceived, 0, CombatMath::Balance::MaximumHealingPotency)) / 100;
    return effective > 0;
}

bool WouldImproveStatus(const Player& player, StatusType type, int potency, int duration) {
    for (const auto& status : player.GetStatuses()) {
        if (status.type != type) continue;
        // Do not replace a stronger buff with a weaker consumable effect.
        if (status.potency > potency) return false;
        return status.potency < potency || status.turns < duration;
    }
    return true;
}
}

const ConsumableEffectDefinition* FindConsumableEffect(const std::string& effectId) {
    for (const auto& effect : Effects) if (effectId == effect.id) return &effect;
    return nullptr;
}

bool ValidateConsumableEffects(const std::vector<ItemDefinition>& definitions, std::string* error) {
    for (const auto& item : definitions) {
        if (item.category != ItemCategory::Consumable || FindConsumableEffect(item.effect) == nullptr) {
            if (error) *error = "Unsupported consumable effect '" + item.effect + "' in " + item.name;
            return false;
        }
    }
    return true;
}

bool IsNegativeConsumableStatus(StatusType type) {
    return StatusRules::Negative(type);
}

bool Player::UseConsumable(const std::string& itemName, Enemy* target, CombatManager* combat, Area* area) {
    const Item* item = inventory_.FindUsableItem(itemName);
    return item != nullptr && UseConsumable(item->getInstanceId(), target, combat, area);
}

bool Player::UseConsumable(unsigned long long instanceId, Enemy* target, CombatManager* combat, Area* area) {
    if (!isAlive() || !inventory_.CanRemoveItem(instanceId)) return false;
    const Item* item = inventory_.FindItem(instanceId);
    if (item == nullptr || item->getCategory() != ItemCategory::Consumable) return false;
    const auto* effect = FindConsumableEffect(item->getEffect());
    if (effect == nullptr || (target != nullptr && !target->isAlive())) return false;
    if (combat != nullptr && !combat->CanUseConsumableNow(*this, target)) return false;
    if (combat == nullptr && target != nullptr) return false;
    if (combat == nullptr && area != nullptr && area->hasEnemies()) return false;
    if (effect->context == ConsumableContext::Combat && combat == nullptr) return false;
    if (effect->context == ConsumableContext::Exploration &&
        (combat != nullptr || area == nullptr || area->hasEnemies())) return false;

    const int restoration = RestorationAmount(*this, effect->magnitude);
    bool meaningful = false;
    switch (effect->kind) {
    case ConsumableEffectKind::RestoreHealth:
        meaningful = CanRestoreHealth(*this, restoration); break;
    case ConsumableEffectKind::RestoreAndCleanseBleed:
        meaningful = CanRestoreHealth(*this, restoration) || HasStatus(StatusType::Bleed); break;
    case ConsumableEffectKind::AccuracyUp:
        meaningful = WouldImproveStatus(*this, StatusType::AccuracyUp, effect->magnitude, effect->duration); break;
    case ConsumableEffectKind::DefenseUp:
        meaningful = WouldImproveStatus(*this, StatusType::DefenseUp, effect->magnitude, effect->duration); break;
    case ConsumableEffectKind::ReduceCooldowns:
        meaningful = combat->HasActiveShimmerCooldowns(); break;
    case ConsumableEffectKind::CleanseNegative:
        meaningful = std::any_of(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) { return IsNegativeConsumableStatus(status.type); }); break;
    case ConsumableEffectKind::CleanseBurnPoison:
        meaningful = HasStatus(StatusType::Burn) || HasStatus(StatusType::Poison); break;
    case ConsumableEffectKind::ApplyBurn: case ConsumableEffectKind::FlatDamage:
        meaningful = target != nullptr; break;
    case ConsumableEffectKind::Illuminate:
        meaningful = area->CanBeIlluminated() && !area->IsIlluminated(); break;
    case ConsumableEffectKind::UnlockRustedLock:
        meaningful = area->HasRustedLock(); break;
    }
    if (!meaningful) return false;

    // Preserve metadata before removing this exact instance. Validation above
    // performs no gameplay mutations; removal failure also leaves effects untouched.
    if (!inventory_.RemoveItem(instanceId)) return false;
    const auto cleanse = [&](auto predicate) {
        statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), predicate), statuses_.end());
    };
    switch (effect->kind) {
    case ConsumableEffectKind::RestoreHealth:
        heal(restoration); break;
    case ConsumableEffectKind::RestoreAndCleanseBleed:
        heal(restoration);
        cleanse([](const StatusEffect& status) { return status.type == StatusType::Bleed; }); break;
    case ConsumableEffectKind::AccuracyUp:
        ApplyStatus({StatusType::AccuracyUp, effect->duration, effect->magnitude}); break;
    case ConsumableEffectKind::DefenseUp:
        ApplyStatus({StatusType::DefenseUp, effect->duration, effect->magnitude}); break;
    case ConsumableEffectKind::ReduceCooldowns:
        combat->ReduceShimmerCooldowns(effect->magnitude); break;
    case ConsumableEffectKind::CleanseNegative:
        cleanse([](const StatusEffect& status) { return IsNegativeConsumableStatus(status.type); }); break;
    case ConsumableEffectKind::CleanseBurnPoison:
        cleanse([](const StatusEffect& status) { return status.type == StatusType::Burn || status.type == StatusType::Poison; }); break;
    case ConsumableEffectKind::ApplyBurn:
        target->ApplyStatus({StatusType::Burn, effect->duration, 0, getAttack(), getDotPotency(), getDefense(), getName(), GetCombatantId()}, getAccuracy()); break;
    case ConsumableEffectKind::FlatDamage:
        target->takeDamage(effect->magnitude); break;
    case ConsumableEffectKind::Illuminate:
        area->IlluminateFor(std::chrono::seconds(effect->duration)); break;
    case ConsumableEffectKind::UnlockRustedLock:
        area->UnlockRustedLock(); break;
    }
    return true;
}
