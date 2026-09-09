#pragma once

#include "GameplayTypes.h"
#include <algorithm>
#include <cmath>
#include <random>

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

inline int ApplyPercent(int value, int percent) {
	return static_cast<int>(std::lround(value * (1.0 + percent / 100.0)));
}

inline int Mitigate(int damage, int defense) {
	const double factor = static_cast<double>(Balance::DefenseConstant) /
		((std::max)(0, defense) + Balance::DefenseConstant);
	return (std::max)(1, static_cast<int>(std::lround(damage * factor)));
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
	const int adjusted = static_cast<int>(std::lround(baseChance * (1.0 + (accuracy - resistance) / 100.0)));
	return (std::max)(15, (std::min)(100, adjusted));
}
}
