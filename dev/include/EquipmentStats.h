#pragma once

#include "GameplayTypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>

// Equipment units: Stats core fields are fixed flat bonuses; core Percent
// fields are additive percentages of (base + fixed flat). HP/DEF/ATK rolled
// mains and substats add to those percentages. SPD rolls are flat. All
// secondary rolls are percentage points. Final combat modifiers apply after
// equipment aggregation; each core aggregation rounds once, to nearest.
namespace EquipmentStats {
struct Rule {
    const char* label;
    int Stats::*flat;
    int Stats::*percent;
    bool corePercentRoll;
};

inline Rule ForStat(ItemStat stat) {
    switch (stat) {
    case ItemStat::HP: return {"HP", &Stats::hp, &Stats::hpPercent, true};
    case ItemStat::DEF: return {"DEF", &Stats::def, &Stats::defPercent, true};
    case ItemStat::SPD: return {"SPD", &Stats::spd, &Stats::spdPercent, false};
    case ItemStat::ATK: return {"ATK", &Stats::atk, &Stats::atkPercent, true};
    case ItemStat::Accuracy: return {"ACC", &Stats::acc, nullptr, false};
    case ItemStat::Resistance: return {"RES", &Stats::res, nullptr, false};
    case ItemStat::CritRate: return {"CritRate", &Stats::critRate, nullptr, false};
    case ItemStat::CritDamage: return {"CritDamage", &Stats::critDamage, nullptr, false};
    case ItemStat::Evasion: return {"EVA", &Stats::evasion, nullptr, false};
    case ItemStat::HealingPotency: return {"HealingPotency", &Stats::healingPotency, nullptr, false};
    case ItemStat::BarrierPotency: return {"BarrierPotency", &Stats::barrierPotency, nullptr, false};
    case ItemStat::DotPotency: return {"DOTPotency", &Stats::dotPotency, nullptr, false};
    case ItemStat::Lifesteal: return {"Lifesteal", &Stats::lifesteal, nullptr, false};
    case ItemStat::DebuffDurationReduction: return {"DebuffDurationReduction", &Stats::debuffDurationReduction, nullptr, false};
    }
    return {"Unknown", nullptr, nullptr, false};
}

inline Rule ForMain(const std::string& key) {
    if (key == "hp") return ForStat(ItemStat::HP);
    if (key == "def") return ForStat(ItemStat::DEF);
    if (key == "atk") return ForStat(ItemStat::ATK);
    if (key == "spd") return ForStat(ItemStat::SPD);
    if (key == "acc" || key == "accuracy") return ForStat(ItemStat::Accuracy);
    if (key == "res" || key == "resistance") return ForStat(ItemStat::Resistance);
    if (key == "crit" || key == "crit_rate") return ForStat(ItemStat::CritRate);
    if (key == "crit_damage") return ForStat(ItemStat::CritDamage);
    if (key == "eva" || key == "evasion") return ForStat(ItemStat::Evasion);
    if (key == "healing" || key == "healing_potency") return ForStat(ItemStat::HealingPotency);
    if (key == "healing_received") return {"HealingReceived", &Stats::healingReceived, nullptr, false};
    if (key == "barrier" || key == "barrier_potency") return ForStat(ItemStat::BarrierPotency);
    if (key == "dot" || key == "dot_potency") return ForStat(ItemStat::DotPotency);
    if (key == "lifesteal") return ForStat(ItemStat::Lifesteal);
    if (key == "debuff_reduction") return ForStat(ItemStat::DebuffDurationReduction);
    return {"Unknown", nullptr, nullptr, false};
}

inline int SaturatingAdd(int left, int right) {
    const auto sum = static_cast<std::int64_t>(left) + right;
    return static_cast<int>((std::max)(static_cast<std::int64_t>((std::numeric_limits<int>::min)()),
        (std::min)(static_cast<std::int64_t>((std::numeric_limits<int>::max)()), sum)));
}

inline const std::array<int Stats::*, 25>& Fields() {
    static const std::array<int Stats::*, 25> fields = {{
        &Stats::hp, &Stats::def, &Stats::spd, &Stats::atk, &Stats::acc, &Stats::res,
        &Stats::critRate, &Stats::critDamage, &Stats::evasion, &Stats::healingPotency,
        &Stats::healingReceived, &Stats::barrierPotency, &Stats::dotPotency,
        &Stats::lifesteal, &Stats::debuffDurationReduction, &Stats::hpPercent,
        &Stats::defPercent, &Stats::spdPercent, &Stats::atkPercent,
        &Stats::accuracyPercent, &Stats::resistancePercent, &Stats::critRatePercent,
        &Stats::critDamagePercent, &Stats::damagePercent, &Stats::damageTakenPercent
    }};
    return fields;
}

inline void Add(Stats& total, const Stats& addition) {
    for (auto field : Fields()) total.*field = SaturatingAdd(total.*field, addition.*field);
}

inline int ApplyPercent(int value, int percent) {
    const long double scaled = static_cast<long double>(value) * (100.0L + percent) / 100.0L;
    const long double bounded = (std::max)(0.0L, (std::min)(
        static_cast<long double>((std::numeric_limits<int>::max)()), scaled));
    return static_cast<int>(std::llround(bounded));
}

inline Stats Finalize(Stats total) {
    total.hp = ApplyPercent(total.hp, total.hpPercent);
    total.def = ApplyPercent(total.def, total.defPercent);
    total.spd = ApplyPercent(total.spd, total.spdPercent);
    total.atk = ApplyPercent(total.atk, total.atkPercent);
    // Legacy secondary Percent fields are aliases for percentage-point bonuses.
    total.acc = SaturatingAdd(total.acc, total.accuracyPercent);
    total.res = SaturatingAdd(total.res, total.resistancePercent);
    total.critRate = SaturatingAdd(total.critRate, total.critRatePercent);
    total.critDamage = SaturatingAdd(total.critDamage, total.critDamagePercent);
    return total;
}

inline bool HasStat(const Stats& stats, ItemStat stat) {
    const Rule rule = ForStat(stat);
    return rule.flat && (stats.*rule.flat != 0 || (rule.percent && stats.*rule.percent != 0));
}
}
