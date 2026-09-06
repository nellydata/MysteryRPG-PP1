#pragma once

#include <algorithm>
#include <cmath>

namespace CombatMath {
inline int ClampPercent(int value) {
	return (std::max)(0, (std::min)(100, value));
}

inline int ApplyPercent(int value, int percent) {
	return static_cast<int>(std::lround(value * (1.0 + percent / 100.0)));
}

inline int Mitigate(int damage, int defense) {
	const double factor = 1000.0 / ((std::max)(0, defense) + 1000.0);
	return (std::max)(1, static_cast<int>(std::lround(damage * factor)));
}

inline int AccuracyChance(int baseChance, int accuracy, int resistance) {
	const int adjusted = static_cast<int>(std::lround(baseChance * (1.0 + (accuracy - resistance) / 100.0)));
	return (std::max)(15, (std::min)(100, adjusted));
}
}