#pragma once

#include "GameplayTypes.h"
#include <string>
#include <vector>

enum class ConsumableEffectKind {
    RestoreHealth, RestoreAndCleanseBleed, AccuracyUp, DefenseUp,
    ReduceCooldowns, CleanseNegative, CleanseBurnPoison, ApplyBurn,
    FlatDamage, Illuminate, UnlockRustedLock
};

enum class ConsumableContext { Any, Combat, Exploration };

struct ConsumableEffectDefinition {
    const char* id;
    ConsumableEffectKind kind;
    ConsumableContext context;
    int magnitude;
    int duration;
};

const ConsumableEffectDefinition* FindConsumableEffect(const std::string& effectId);
bool ValidateConsumableEffects(const std::vector<ItemDefinition>& definitions, std::string* error = nullptr);
bool IsNegativeConsumableStatus(StatusType type);
