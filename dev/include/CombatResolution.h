#pragma once

#include "GameplayTypes.h"

// Captured at the mutation site, before any reactive healing or barrier gain.
struct DamageResolution {
    int requestedDamage = 0;
    int barrierAbsorbed = 0;
    int hpDamage = 0;
    int remainingHP = 0;
    int healing = 0;
    int barrierGained = 0;
    bool barrierBroken = false;
    int mitigatedDamage = 0;
    int overkillDamage = 0;
};

struct HealResolution {
    int requestedHealing = 0;
    int effectiveHealing = 0;
    int hpRestored = 0;
    int overheal = 0;
    int remainingHP = 0;
    int barrierGained = 0;
};

struct StatusDamageResolution {
    StatusType type;
    DamageResolution damage;
    std::string source;
    std::uint64_t sourceId = 0;
};

struct StatusApplicationResolution {
    StatusEffect status;
    bool applied = false;
    bool resisted = false;
};
