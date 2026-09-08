#include "EnemyCatalog.h"
#include "Enemy.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
EnemySkillDefinition Skill(const char* name, const char* effect, int multiplier, int cooldown, int duration = 0, int statusPotency = 20) {
	return {name, effect, multiplier, cooldown, duration, statusPotency};
}

bool IsKnownEffect(const std::string& effect) {
	static const std::vector<std::string> effects = {
		"cleanse_self", "heal_self", "bleed", "attack_up", "def_down", "accuracy_down", "burn",
		"heal_resistance", "dot_reduction", "accuracy_up", "extra_turn", "crit_up", "accuracy_floor",
		"crit_damage_up", "barrier", "poison", "dot_up", "tri_dot", "accuracy_crit_up", "guaranteed_crit",
		"barrier_accuracy", "heal_cleanse", "burn_bleed"
	};
	return std::find(effects.begin(), effects.end(), effect) != effects.end();
}

std::function<void(const EnemySelectionTrace&)> selectionTraceSink;

struct Candidate {
	const EnemyDefinition* definition;
	int weight;
};

int ClampLevel(int level) { return (std::max)(1, (std::min)(100, level)); }

std::unique_ptr<Enemy> TraceEnemy(std::unique_ptr<Enemy> enemy, const std::string& source) {
	if (enemy && selectionTraceSink)
		selectionTraceSink({enemy->getTier(), enemy->getName(), enemy->getLevel(), source});
	return enemy;
}

const EnemyDefinition* SelectTierThenEnemy(const std::vector<Candidate>& candidates, int level, std::mt19937& generator) {
	if (candidates.empty()) return nullptr;
	std::array<std::vector<Candidate>, 4> tiers;
	for (const auto& candidate : candidates) {
		if (candidate.weight > 0 && candidate.definition->tier != EnemyTier::Boss &&
			candidate.definition->minimumLevel <= ClampLevel(level))
			tiers[static_cast<std::size_t>(candidate.definition->tier)].push_back(candidate);
	}
	auto weights = GetEnemyTierWeights(level);
	double total = 0.0;
	for (std::size_t tier = 0; tier < weights.size(); ++tier) {
		if (tiers[tier].empty()) weights[tier] = 0.0;
		total += weights[tier];
	}
	// A custom room containing only eligible zero-curve tiers still has a valid
	// pool. Redistribute evenly across those tiers instead of inventing an enemy.
	if (total <= 0.0) {
		for (std::size_t tier = 0; tier < weights.size(); ++tier) {
			weights[tier] = tiers[tier].empty() ? 0.0 : 1.0;
			total += weights[tier];
		}
	}
	if (total <= 0.0) return nullptr;
	std::discrete_distribution<std::size_t> pickTier(weights.begin(), weights.end());
	const auto& entries = tiers[pickTier(generator)];
	std::vector<int> entryWeights;
	for (const auto& entry : entries) entryWeights.push_back(entry.weight);
	std::discrete_distribution<std::size_t> pickEntry(entryWeights.begin(), entryWeights.end());
	return entries[pickEntry(generator)].definition;
}

std::vector<Candidate> RoomCandidates(const RoomDefinition& room, int level, bool afterClear) {
	std::vector<Candidate> candidates;
	for (const auto& entry : afterClear ? room.enemiesAfterClear : room.enemiesBeforeClear) {
		const auto* definition = FindEnemyDefinition(entry.enemyId);
		if (definition && entry.weight > 0 && entry.minimumLevel <= ClampLevel(level) &&
			definition->minimumLevel <= ClampLevel(level) && definition->tier != EnemyTier::Boss)
			candidates.push_back({definition, entry.weight});
	}
	return candidates;
}

const EnemyDefinition* EligibleFarmBoss(const RoomDefinition& room, int level, bool afterClear) {
	if (!afterClear || room.farmBossStartingChance <= 0) return nullptr;
	const auto* boss = FindEnemyDefinition(room.bossId);
	return boss && boss->tier == EnemyTier::Boss && boss->minimumLevel <= ClampLevel(level) ? boss : nullptr;
}
}

void SetEnemySelectionTraceSink(std::function<void(const EnemySelectionTrace&)> sink) {
	selectionTraceSink = std::move(sink);
}

std::array<double, 4> GetEnemyTierWeights(int level) {
	struct Knot { int level; std::array<double, 4> weights; };
	static const std::array<Knot, 7> curve = {{
		{1, {90, 10, 0, 0}}, {5, {80, 18, 2, 0}}, {10, {65, 28, 7, 0}},
		{20, {45, 35, 17, 3}}, {40, {25, 35, 30, 10}},
		{70, {10, 25, 40, 25}}, {100, {5, 15, 40, 40}}
	}};
	const int clamped = ClampLevel(level);
	for (std::size_t index = 1; index < curve.size(); ++index) {
		if (clamped > curve[index].level) continue;
		const auto& lower = curve[index - 1];
		const auto& upper = curve[index];
		const double fraction = static_cast<double>(clamped - lower.level) / (upper.level - lower.level);
		std::array<double, 4> result;
		for (std::size_t tier = 0; tier < result.size(); ++tier)
			result[tier] = lower.weights[tier] + fraction * (upper.weights[tier] - lower.weights[tier]);
		return result;
	}
	return curve.back().weights;
}

std::vector<EnemyDefinition> BuildCatalogue() {
	return {
		{"Devotee Acolyte", EnemyFaction::Devotee, EnemyTier::Basic, 155, 24, 18, 50, 35,
			{Skill("Purify Lash", "cleanse_self", 100, 2), Skill("Weak Prayer", "heal_self", 0, 3)}, "Sanctity Logic", 75, 20, 0, EnemyRole::Support, "devotee", 1},
		{"Devotee Penitent", EnemyFaction::Devotee, EnemyTier::Rare, 230, 30, 22, 75, 55,
			{Skill("Blood Offering", "bleed", 125, 3, 2), Skill("Pain Conversion", "attack_up", 0, 4, 2)}, "Sanctity Logic", 20, 60, 10, EnemyRole::DotCaster, "devotee", 1},
		{"Devotee Flagellant", EnemyFaction::Devotee, EnemyTier::Rare, 250, 32, 30, 80, 58,
			{Skill("Flagellation", "def_down", 120, 3, 2, 20), Skill("Purity Chant", "cleanse_self", 0, 4)}, "Sanctity Logic", 20, 60, 10, EnemyRole::Striker, "devotee", 1},
		{"Choir Ascetic", EnemyFaction::Devotee, EnemyTier::Epic, 360, 46, 45, 110, 85,
			{Skill("Ascetic Strike", "accuracy_down", 165, 3, 2), Skill("Choir Mend", "heal_resistance", 0, 5, 2)}, "Sanctity Logic", 10, 70, 20, EnemyRole::Controller, "devotee", 5},
		{"Benediction Sister", EnemyFaction::Devotee, EnemyTier::Epic, 400, 42, 55, 115, 90,
			{Skill("Blessed Flame", "burn", 0, 4, 2), Skill("Benediction", "dot_reduction", 0, 5, 2)}, "Sanctity Logic", 10, 70, 20, EnemyRole::Support, "devotee", 5},
		{"Automata Unit-01", EnemyFaction::Automata, EnemyTier::Basic, 165, 26, 28, 60, 40,
			{Skill("Precision Shot", "accuracy_up", 125, 2, 2), Skill("Recalibrate", "extra_turn", 0, 3)}, "Precision Logic", 75, 20, 0, EnemyRole::Balanced, "automata", 1},
		{"Automata Sharpsight", EnemyFaction::Automata, EnemyTier::Rare, 245, 38, 22, 85, 60,
			{Skill("Sharpsight Beam", "crit_up", 140, 3, 2), Skill("Target Lock", "accuracy_floor", 0, 4, 2)}, "Precision Logic", 20, 60, 10, EnemyRole::Striker, "automata", 1},
		{"Automata Striker", EnemyFaction::Automata, EnemyTier::Rare, 240, 40, 25, 90, 65,
			{Skill("Strikeframe", "extra_turn", 115, 3), Skill("Overclock", "attack_up", 0, 4, 2)}, "Precision Logic", 20, 60, 10, EnemyRole::Speedster, "automata", 1},
		{"Automata Vanguard", EnemyFaction::Automata, EnemyTier::Epic, 440, 50, 65, 110, 95,
			{Skill("Vanguard Slash", "crit_damage_up", 170, 4, 2), Skill("Reinforce", "barrier", 0, 5)}, "Precision Logic", 10, 70, 20, EnemyRole::Tank, "automata", 5},
		{"Automata Oracle Unit", EnemyFaction::Automata, EnemyTier::Epic, 410, 48, 50, 120, 105,
			{Skill("Oracle Strike", "guaranteed_crit", 180, 5), Skill("Predictive Matrix", "accuracy_floor", 0, 6, 2)}, "Precision Logic", 10, 70, 20, EnemyRole::Executioner, "automata", 5},
		{"Verdigris Thrall", EnemyFaction::Verdigris, EnemyTier::Basic, 160, 23, 15, 40, 35,
			{Skill("Toxic Swipe", "poison", 0, 2, 1), Skill("Rotting Chant", "dot_up", 0, 3, 2)}, "Corruption Logic", 75, 20, 0, EnemyRole::DotCaster, "verdigris", 1},
		{"Verdigris Rotbearer", EnemyFaction::Verdigris, EnemyTier::Rare, 250, 34, 28, 70, 55,
			{Skill("Rotburst", "poison", 0, 4, 3), Skill("Toxic Blessing", "dot_up", 0, 5, 2)}, "Corruption Logic", 20, 60, 10, EnemyRole::DotCaster, "verdigris", 1},
		{"Verdigris Bleedcaller", EnemyFaction::Verdigris, EnemyTier::Rare, 245, 37, 24, 75, 60,
			{Skill("Bloodcall", "bleed", 0, 4, 3), Skill("Hemorrhage", "dot_up", 0, 5, 2)}, "Corruption Logic", 20, 60, 10, EnemyRole::DotCaster, "verdigris", 1},
		{"Verdigris Pyre Sister", EnemyFaction::Verdigris, EnemyTier::Epic, 390, 46, 42, 105, 90,
			{Skill("Pyrebrand", "burn", 0, 4, 3), Skill("Corrupted Flame", "dot_up", 0, 5, 2)}, "Corruption Logic", 10, 70, 20, EnemyRole::DotCaster, "verdigris", 5},
		{"Verdigris Matron's Hand", EnemyFaction::Verdigris, EnemyTier::Epic, 425, 48, 48, 115, 100,
			{Skill("Tri-Corrupt", "tri_dot", 0, 5, 2), Skill("Toxic Benediction", "dot_up", 0, 6, 2)}, "Corruption Logic", 10, 70, 20, EnemyRole::DotCaster, "verdigris", 5},
		{"Choir Warden", EnemyFaction::Devotee, EnemyTier::Elite, 650, 65, 75, 130, 150,
			{Skill("Warden's Verdict", "def_down", 200, 4, 3), Skill("Choir Shield", "barrier", 0, 5), Skill("Sanctified Pulse", "heal_cleanse", 0, 4)}, "Holy Retaliation", 60, 30, 10, EnemyRole::Tank, "devotee", 15},
		{"Ascetic Purifier", EnemyFaction::Devotee, EnemyTier::Elite, 620, 62, 80, 125, 155,
			{Skill("Purifier's Flame", "burn", 0, 4, 3), Skill("Ascetic Chant", "heal_resistance", 0, 5, 3), Skill("Judgment Beam", "accuracy_down", 180, 4, 2)}, "DOT Cleanse", 60, 30, 10, EnemyRole::Controller, "devotee", 15},
		{"Strikeframe-X", EnemyFaction::Automata, EnemyTier::Elite, 660, 72, 80, 140, 170,
			{Skill("Strikeframe Overdrive", "extra_turn", 190, 4), Skill("Precision Matrix", "accuracy_crit_up", 0, 5, 3), Skill("Core Burst", "crit_damage_up", 180, 4, 2)}, "Overclock Surge", 60, 30, 10, EnemyRole::Speedster, "automata", 15},
		{"Oracle-VII", EnemyFaction::Automata, EnemyTier::Elite, 680, 74, 85, 145, 180,
			{Skill("Oracle Beam", "guaranteed_crit", 210, 5), Skill("Predictive Shield", "barrier_accuracy", 0, 5, 3), Skill("Refraction Scan", "accuracy_down", 0, 4, 2)}, "Future Sight", 60, 30, 10, EnemyRole::Executioner, "automata", 15},
		{"Rot Herald", EnemyFaction::Verdigris, EnemyTier::Elite, 650, 68, 70, 135, 165,
			{Skill("Rot Nova", "poison", 0, 4, 3), Skill("Hemorrhage Lash", "bleed", 0, 4, 3), Skill("Corrupted Blessing", "dot_up", 0, 5, 2)}, "Toxic Aura", 60, 30, 10, EnemyRole::DotCaster, "verdigris", 15},
		{"Pyre Matron", EnemyFaction::Verdigris, EnemyTier::Elite, 680, 72, 75, 140, 175,
			{Skill("Pyre Collapse", "burn", 0, 4, 3), Skill("Tri-Corrupt Burst", "tri_dot", 0, 5, 2), Skill("Verdigris Chant", "dot_up", 0, 6, 2)}, "Corruption Spread", 60, 30, 10, EnemyRole::DotCaster, "verdigris", 15},
		{"Matron of the Catacombs", EnemyFaction::Devotee, EnemyTier::Boss, 300, 15, 22, 65, 300,
			{Skill("Sanctified Barrier", "barrier", 0, 5), Skill("Catacomb Judgment", "def_down", 140, 4, 2), Skill("Choir's Wrath", "burn_bleed", 0, 6, 2)}, "Boss Phases", 20, 70, 10, EnemyRole::Controller, "devotee", 1},
		{"Overlord-Prime", EnemyFaction::Automata, EnemyTier::Boss, 380, 22, 35, 100, 350,
			{Skill("Prime Directive", "accuracy_up", 0, 6, 3), Skill("Prime Execution", "crit_damage_up", 180, 5, 3), Skill("Overclock Core", "extra_turn", 0, 4)}, "Boss Phases", 20, 70, 10, EnemyRole::Executioner, "automata", 1},
		{"Matron of Rot", EnemyFaction::Verdigris, EnemyTier::Boss, 420, 20, 32, 90, 375,
			{Skill("Toxic Benediction", "dot_up", 0, 6, 2), Skill("Rot Nova", "poison", 0, 5, 3), Skill("Hemorrhage Wave", "bleed", 0, 5, 3), Skill("Pyre Collapse", "burn", 0, 5, 3)}, "Boss Phases", 20, 70, 10, EnemyRole::DotCaster, "verdigris", 1},
		{"Sister Caligo, Keeper of the Font", EnemyFaction::Devotee, EnemyTier::Boss, 340, 18, 28, 85, 260,
			{Skill("Caligo's Benediction", "heal_cleanse", 0, 4), Skill("Font's Judgment", "def_down", 160, 4, 3), Skill("Drowning Litany", "burn_bleed", 0, 6, 2)}, "Sanctuary Phases", 20, 70, 10, EnemyRole::Support, "devotee", 1},
		{"The Reliquary Engine", EnemyFaction::Automata, EnemyTier::Boss, 450, 24, 40, 80, 340,
			{Skill("Vault Protocol", "barrier", 0, 5), Skill("Reliquary Beam", "guaranteed_crit", 190, 5), Skill("Overload Cascade", "extra_turn", 0, 4)}, "Vault Phases", 20, 70, 10, EnemyRole::Tank, "automata", 1}
	};
}

const std::vector<EnemyDefinition>& GetEnemyCatalogue() {
	static const std::vector<EnemyDefinition> catalogue = BuildCatalogue();
	return catalogue;
}

std::unique_ptr<Enemy> CreateRoomEnemy(const RoomDefinition& room, unsigned seed, int level, bool afterClear) {
	std::mt19937 generator(seed);
	const auto candidates = RoomCandidates(room, level, afterClear);
	const auto* farmBoss = EligibleFarmBoss(room, level, afterClear);
	// Boss farming is a separate, explicit encounter roll; it never dilutes the
	// relative Basic/Rare/Epic/Elite curve or enters an uncleared room's pool.
	if (farmBoss && (candidates.empty() || std::uniform_int_distribution<int>(1, 100)(generator) <= room.farmBossStartingChance))
		return TraceEnemy(std::make_unique<Enemy>(*farmBoss, level, false, true), "room farm boss: " + room.id);
	const auto* definition = SelectTierThenEnemy(candidates, level, generator);
	return definition ? TraceEnemy(std::make_unique<Enemy>(*definition, level),
		(afterClear ? "room after clear: " : "room before clear: ") + room.id) : nullptr;
}

bool HasEligibleRoomEnemy(const RoomDefinition& room, int level, bool afterClear) {
	return !RoomCandidates(room, level, afterClear).empty() || EligibleFarmBoss(room, level, afterClear) != nullptr;
}

std::unique_ptr<Enemy> CreateFirstClearBoss(const RoomDefinition& room) {
	const auto* definition = FindEnemyDefinition(room.bossId);
	if (!definition || definition->tier != EnemyTier::Boss) return nullptr;
	return TraceEnemy(std::make_unique<Enemy>(*definition, room.firstClearBossLevel, true, false,
		room.firstClearBossXP), "room first clear: " + room.id);
}

bool ValidateEnemyCatalogue(std::string* error) {
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.name.empty() || definition.hpBase <= 0 || definition.atkBase <= 0 ||
			definition.minimumLevel < 1 || definition.minimumLevel > 100) {
			if (error) *error = "Invalid enemy definition: " + definition.name;
			return false;
		}
		for (const auto& skill : definition.skills) {
			if (skill.multiplier < 0 || skill.statusPotency < 0 || skill.statusPotency > 100) {
				if (error) *error = "Invalid skill potency in " + definition.name;
				return false;
			}
			if (!IsKnownEffect(skill.effect)) {
				if (error) *error = "Unknown enemy effect '" + skill.effect + "' in " + definition.name;
				return false;
			}
		}
	}
	return true;
}

const EnemyDefinition* FindEnemyDefinition(const std::string& name) {
	for (const auto& definition : GetEnemyCatalogue()) if (definition.name == name) return &definition;
	return nullptr;
}

std::unique_ptr<Enemy> CreateCatalogueEnemy(const std::string& name, int level) {
	const EnemyDefinition* definition = FindEnemyDefinition(name);
	if (definition == nullptr) return nullptr;
	return std::make_unique<Enemy>(*definition, level);
}

std::unique_ptr<Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level, bool allowElite, bool allowBoss) {
	std::vector<Candidate> candidates;
	std::vector<const EnemyDefinition*> bosses;
	std::mt19937 generator(seed);
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.minimumLevel > ClampLevel(level)) continue;
		if (definition.tier == EnemyTier::Boss) {
			if (allowBoss) bosses.push_back(&definition);
			continue;
		}
		if (definition.tier == EnemyTier::Elite && !allowElite) continue;
		candidates.push_back({&definition, 1});
	}
	if (!bosses.empty() && std::uniform_int_distribution<int>(1, 100)(generator) <= 8)
		return TraceEnemy(std::make_unique<Enemy>(*bosses[std::uniform_int_distribution<std::size_t>(0, bosses.size() - 1)(generator)],
			level, false, true), "catalogue explicit boss pool");
	const auto* definition = SelectTierThenEnemy(candidates, level, generator);
	return definition ? TraceEnemy(std::make_unique<Enemy>(*definition, level), "catalogue ordinary") : nullptr;
}

std::unique_ptr<Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level,
	EnemyFaction faction, EnemyTier maximumTier) {
	std::vector<Candidate> candidates;
	std::mt19937 generator(seed);
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.faction != faction || definition.tier == EnemyTier::Boss ||
			static_cast<int>(definition.tier) > static_cast<int>(maximumTier) || definition.minimumLevel > ClampLevel(level)) continue;
		candidates.push_back({&definition, 1});
	}
	const auto* definition = SelectTierThenEnemy(candidates, level, generator);
	return definition ? TraceEnemy(std::make_unique<Enemy>(*definition, level),
		"catalogue faction: " + std::string(EnemyFactionName(faction))) : nullptr;
}

const char* EnemyFactionName(EnemyFaction faction) {
	switch (faction) {
	case EnemyFaction::Devotee: return "Devotee";
	case EnemyFaction::Automata: return "Automata";
	case EnemyFaction::Verdigris: return "Verdigris";
	}
	return "Unknown";
}

const char* EnemyTierName(EnemyTier tier) {
	switch (tier) {
	case EnemyTier::Basic: return "Basic";
	case EnemyTier::Rare: return "Rare";
	case EnemyTier::Epic: return "Epic";
	case EnemyTier::Elite: return "Elite";
	case EnemyTier::Boss: return "Boss";
	}
	return "Unknown";
}
