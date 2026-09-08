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
};

struct StatusDamageResolution {
    StatusType type;
    DamageResolution damage;
};

struct StatusApplicationResolution {
    StatusEffect status;
    bool applied = false;
};
