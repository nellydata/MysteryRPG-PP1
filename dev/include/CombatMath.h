#pragma once

#include "GameplayTypes.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>

namespace CombatMath {
namespace Balance {
constexpr int BasePlayerSpeed = 100;
constexpr int MinimumEnemySpeed = 75;
constexpr int MaximumEnemySpeed = 135;
constexpr int SpecialEnemySpeed = 145;
constexpr int DefenseConstant = 1000;
constexpr int MaximumCritRate = 100;
constexpr int MaximumEvasion = 45;
constexpr int MaximumResistance = 75;
constexpr int MaximumDamageReduction = 75;
constexpr int MaximumLifesteal = 40;
constexpr int MaximumDebuffReduction = 60;
constexpr int MaximumHealingPotency = 150;
constexpr int MaximumBarrierPotency = 200;
constexpr int MaximumDotPotency = 200;
}

inline int ClampPercent(int value) {
	return (std::max)(0, (std::min)(100, value));
}

// Saturation protects the representable state; it is not a gameplay damage cap.
inline int Saturate(std::int64_t value) {
    return static_cast<int>((std::max)(std::int64_t{0},
        (std::min)(static_cast<std::int64_t>((std::numeric_limits<int>::max)()), value)));
}
inline int Scale(int value, int percent) {
    return Saturate(static_cast<std::int64_t>((std::max)(0, value)) * (std::max)(0, percent) / 100);
}
inline int ApplyPercent(int value, int percent) {
    const auto numerator = static_cast<std::int64_t>((std::max)(0, value)) *
        (std::max)(std::int64_t{0}, 100LL + percent);
    return Saturate((numerator + 50) / 100);
}

inline int Mitigate(int damage, int defense) {
    if (damage <= 0) return 0;
    const std::int64_t denominator = static_cast<std::int64_t>((std::max)(0, defense)) + Balance::DefenseConstant;
    return (std::max)(1, Saturate((static_cast<std::int64_t>(damage) * Balance::DefenseConstant + denominator / 2) / denominator));
}

inline std::pair<int, int> SubstatRange(ItemStat stat, Rarity rarity) {
	int minimum = 0;
	int maximum = 0;
	switch (rarity) {
	case Rarity::Uncommon: minimum = 2; maximum = 4; break;
	case Rarity::Rare: minimum = 3; maximum = 5; break;
	case Rarity::Epic: minimum = 5; maximum = 8; break;
	case Rarity::Legendary: minimum = 7; maximum = 12; break;
	case Rarity::Artifact: minimum = 9; maximum = 18; break;
	default: return {0, 0};
	}
	if (stat == ItemStat::SPD) {
		minimum = rarity == Rarity::Uncommon ? 2 : rarity == Rarity::Rare ? 3 : rarity == Rarity::Epic ? 4 : rarity == Rarity::Legendary ? 5 : 7;
		maximum = rarity == Rarity::Uncommon ? 4 : rarity == Rarity::Rare ? 5 : rarity == Rarity::Epic ? 6 : rarity == Rarity::Legendary ? 8 : 10;
	} else if (stat == ItemStat::CritDamage || stat == ItemStat::DotPotency ||
		stat == ItemStat::HealingPotency || stat == ItemStat::BarrierPotency) {
		minimum *= 2;
		maximum *= 2;
	}
	return {minimum, maximum};
}

inline int SubstatRoll(ItemStat stat, Rarity rarity, unsigned seed) {
	const auto range = SubstatRange(stat, rarity);
	std::mt19937 rng(seed);
	return std::uniform_int_distribution<int>(range.first, range.second)(rng);
}

inline int AccuracyChance(int baseChance, int accuracy, int resistance) {
    const auto difference = static_cast<std::int64_t>(accuracy) - resistance;
    const int bounded = static_cast<int>((std::clamp)(difference, -100LL, 100LL));
    const int adjusted = ApplyPercent(baseChance, bounded);
	return (std::max)(15, (std::min)(100, adjusted));
}
}
