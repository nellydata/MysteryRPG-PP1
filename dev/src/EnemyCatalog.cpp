#include "EnemyCatalog.h"
#include "Enemy.h"
#include <algorithm>
#include <cmath>

namespace {
EnemySkillDefinition Skill(const char* name, const char* effect, int multiplier, int cooldown, int duration = 0) {
	return {name, effect, multiplier, cooldown, duration};
}

namespace {
const WeightedEnemyEntry* SelectPoolEntry(const std::vector<WeightedEnemyEntry>& pool, unsigned seed) {
	int total = 0;
	for (const auto& entry : pool) total += (std::max)(0, entry.weight);
	if (total <= 0) return nullptr;
	int roll = static_cast<int>(seed % static_cast<unsigned>(total));
	for (const auto& entry : pool) {
		if (roll < entry.weight) return &entry;
		roll -= entry.weight;
	}
	return nullptr;
}
}

std::vector<EnemyDefinition> BuildCatalogue() {
	return {
		{"Devotee Acolyte", EnemyFaction::Devotee, EnemyTier::Basic, 800, 24, 40, 50, 35,
			{Skill("Purify Lash", "cleanse_self", 100, 2), Skill("Weak Prayer", "heal_self", 0, 3)}, "Sanctity Logic", 75, 20, 0},
		{"Devotee Penitent", EnemyFaction::Devotee, EnemyTier::Rare, 1200, 90, 60, 75, 55,
			{Skill("Blood Offering", "bleed", 140, 3, 2), Skill("Pain Conversion", "attack_up", 0, 4, 2)}, "Sanctity Logic", 20, 60, 10},
		{"Devotee Flagellant", EnemyFaction::Devotee, EnemyTier::Rare, 1300, 85, 70, 80, 58,
			{Skill("Flagellation", "def_down", 120, 3, 2), Skill("Purity Chant", "cleanse_self", 0, 4)}, "Sanctity Logic", 20, 60, 10},
		{"Choir Ascetic", EnemyFaction::Devotee, EnemyTier::Epic, 1800, 120, 100, 110, 85,
			{Skill("Ascetic Strike", "accuracy_down", 180, 3, 2), Skill("Choir Mend", "heal_resistance", 0, 5, 2)}, "Sanctity Logic", 10, 70, 20},
		{"Benediction Sister", EnemyFaction::Devotee, EnemyTier::Epic, 2000, 110, 120, 115, 90,
			{Skill("Blessed Flame", "burn", 0, 4, 2), Skill("Benediction", "dot_reduction", 0, 5, 2)}, "Sanctity Logic", 10, 70, 20},
		{"Automata Unit-01", EnemyFaction::Automata, EnemyTier::Basic, 900, 28, 50, 60, 40,
			{Skill("Precision Shot", "accuracy_up", 110, 2, 2), Skill("Recalibrate", "extra_turn", 0, 3)}, "Precision Logic", 75, 20, 0},
		{"Automata Sharpsight", EnemyFaction::Automata, EnemyTier::Rare, 1400, 110, 70, 85, 60,
			{Skill("Sharpsight Beam", "crit_up", 150, 3, 2), Skill("Target Lock", "accuracy_floor", 0, 4, 2)}, "Precision Logic", 20, 60, 10},
		{"Automata Striker", EnemyFaction::Automata, EnemyTier::Rare, 1500, 120, 80, 90, 65,
			{Skill("Strikeframe", "extra_turn", 130, 3), Skill("Overclock", "attack_up", 0, 4, 2)}, "Precision Logic", 20, 60, 10},
		{"Automata Vanguard", EnemyFaction::Automata, EnemyTier::Epic, 2200, 150, 120, 110, 95,
			{Skill("Vanguard Slash", "crit_damage_up", 200, 4, 2), Skill("Reinforce", "barrier", 0, 5)}, "Precision Logic", 10, 70, 20},
		{"Automata Oracle Unit", EnemyFaction::Automata, EnemyTier::Epic, 2400, 160, 130, 120, 105,
			{Skill("Oracle Strike", "guaranteed_crit", 260, 5), Skill("Predictive Matrix", "accuracy_floor", 0, 6, 2)}, "Precision Logic", 10, 70, 20},
		{"Verdigris Thrall", EnemyFaction::Verdigris, EnemyTier::Basic, 850, 26, 45, 40, 35,
			{Skill("Toxic Swipe", "poison", 0, 2, 1), Skill("Rotting Chant", "dot_up", 0, 3, 2)}, "Corruption Logic", 75, 20, 0},
		{"Verdigris Rotbearer", EnemyFaction::Verdigris, EnemyTier::Rare, 1300, 100, 70, 70, 55,
			{Skill("Rotburst", "poison", 0, 4, 3), Skill("Toxic Blessing", "dot_up", 0, 5, 2)}, "Corruption Logic", 20, 60, 10},
		{"Verdigris Bleedcaller", EnemyFaction::Verdigris, EnemyTier::Rare, 1400, 110, 75, 75, 60,
			{Skill("Bloodcall", "bleed", 0, 4, 3), Skill("Hemorrhage", "dot_up", 0, 5, 2)}, "Corruption Logic", 20, 60, 10},
		{"Verdigris Pyre Sister", EnemyFaction::Verdigris, EnemyTier::Epic, 2100, 150, 110, 105, 90,
			{Skill("Pyrebrand", "burn", 0, 4, 3), Skill("Corrupted Flame", "dot_up", 0, 5, 2)}, "Corruption Logic", 10, 70, 20},
		{"Verdigris Matron's Hand", EnemyFaction::Verdigris, EnemyTier::Epic, 2300, 160, 120, 115, 100,
			{Skill("Tri-Corrupt", "tri_dot", 0, 5, 2), Skill("Toxic Benediction", "dot_up", 0, 6, 2)}, "Corruption Logic", 10, 70, 20},
		{"Choir Warden", EnemyFaction::Devotee, EnemyTier::Elite, 3000, 180, 150, 130, 150,
			{Skill("Warden's Verdict", "def_down", 300, 4, 3), Skill("Choir Shield", "barrier", 0, 5), Skill("Sanctified Pulse", "heal_cleanse", 0, 4)}, "Holy Retaliation", 60, 30, 10},
		{"Ascetic Purifier", EnemyFaction::Devotee, EnemyTier::Elite, 3200, 170, 160, 125, 155,
			{Skill("Purifier's Flame", "burn", 0, 4, 3), Skill("Ascetic Chant", "heal_resistance", 0, 5, 3), Skill("Judgment Beam", "accuracy_down", 280, 4, 2)}, "DOT Cleanse", 60, 30, 10},
		{"Strikeframe-X", EnemyFaction::Automata, EnemyTier::Elite, 3500, 200, 160, 140, 170,
			{Skill("Strikeframe Overdrive", "extra_turn", 320, 4), Skill("Precision Matrix", "accuracy_crit_up", 0, 5, 3), Skill("Core Burst", "crit_damage_up", 260, 4, 2)}, "Overclock Surge", 60, 30, 10},
		{"Oracle-VII", EnemyFaction::Automata, EnemyTier::Elite, 3600, 210, 170, 145, 180,
			{Skill("Oracle Beam", "guaranteed_crit", 350, 5), Skill("Predictive Shield", "barrier_accuracy", 0, 5, 3), Skill("Refraction Scan", "accuracy_down", 0, 4, 2)}, "Future Sight", 60, 30, 10},
		{"Rot Herald", EnemyFaction::Verdigris, EnemyTier::Elite, 3400, 190, 150, 135, 165,
			{Skill("Rot Nova", "poison", 0, 4, 3), Skill("Hemorrhage Lash", "bleed", 0, 4, 3), Skill("Corrupted Blessing", "dot_up", 0, 5, 2)}, "Toxic Aura", 60, 30, 10},
		{"Pyre Matron", EnemyFaction::Verdigris, EnemyTier::Elite, 3600, 200, 160, 140, 175,
			{Skill("Pyre Collapse", "burn", 0, 4, 3), Skill("Tri-Corrupt Burst", "tri_dot", 0, 5, 2), Skill("Verdigris Chant", "dot_up", 0, 6, 2)}, "Corruption Spread", 60, 30, 10},
		{"Matron of the Catacombs", EnemyFaction::Devotee, EnemyTier::Boss, 5000, 200, 180, 140, 300,
			{Skill("Catacomb Judgment", "def_down", 350, 4, 3), Skill("Sanctified Barrier", "barrier", 0, 5), Skill("Choir's Wrath", "burn_bleed", 0, 6, 3)}, "Boss Phases", 20, 70, 10},
		{"Overlord-Prime", EnemyFaction::Automata, EnemyTier::Boss, 6000, 250, 200, 160, 350,
			{Skill("Prime Execution", "crit_damage_up", 450, 5, 3), Skill("Overclock Core", "extra_turn", 0, 4), Skill("Prime Directive", "accuracy_up", 0, 6, 3)}, "Boss Phases", 20, 70, 10},
		{"Matron of Rot", EnemyFaction::Verdigris, EnemyTier::Boss, 6500, 260, 210, 150, 375,
			{Skill("Rot Nova", "poison", 0, 5, 4), Skill("Hemorrhage Wave", "bleed", 0, 5, 4), Skill("Pyre Collapse", "burn", 0, 5, 4), Skill("Toxic Benediction", "dot_up", 0, 6, 2)}, "Boss Phases", 20, 70, 10},
		{"Sister Caligo, Keeper of the Font", EnemyFaction::Devotee, EnemyTier::Boss, 4200, 175, 170, 145, 260,
			{Skill("Caligo's Benediction", "heal_cleanse", 0, 4), Skill("Font's Judgment", "def_down", 300, 4, 3), Skill("Drowning Litany", "burn_bleed", 0, 6, 3)}, "Sanctuary Phases", 20, 70, 10},
		{"The Reliquary Engine", EnemyFaction::Automata, EnemyTier::Boss, 5600, 240, 220, 125, 340,
			{Skill("Vault Protocol", "barrier", 0, 5), Skill("Reliquary Beam", "guaranteed_crit", 360, 5), Skill("Overload Cascade", "extra_turn", 0, 4)}, "Vault Phases", 20, 70, 10}
	};
}

}

const std::vector<EnemyDefinition>& GetEnemyCatalogue() {
	static const std::vector<EnemyDefinition> catalogue = BuildCatalogue();
	return catalogue;
}

std::unique_ptr<Enemy> CreateRoomEnemy(const RoomDefinition& room, unsigned seed, int level, bool afterClear) {
	const auto& pool = afterClear ? room.enemiesAfterClear : room.enemiesBeforeClear;
	const auto* entry = SelectPoolEntry(pool, seed);
	return entry == nullptr ? nullptr : CreateCatalogueEnemy(entry->enemyId, level);
}

bool ValidateEnemyCatalogue(std::string* error) {
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.name.empty() || definition.hpBase <= 0 || definition.atkBase <= 0) {
			if (error) *error = "Invalid enemy definition: " + definition.name;
			return false;
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
	std::vector<const EnemyDefinition*> candidates;
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.tier == EnemyTier::Boss && !allowBoss) continue;
		if (definition.tier == EnemyTier::Elite && !allowElite) continue;
		if (definition.tier == EnemyTier::Boss || definition.tier == EnemyTier::Elite || definition.tier == EnemyTier::Basic || definition.tier == EnemyTier::Rare || definition.tier == EnemyTier::Epic)
			candidates.push_back(&definition);
	}
	if (candidates.empty()) return nullptr;
	return std::make_unique<Enemy>(*candidates[seed % candidates.size()], level);
}

std::unique_ptr<Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level,
	EnemyFaction faction, EnemyTier maximumTier) {
	std::vector<const EnemyDefinition*> candidates;
	const int clampedLevel = (std::max)(1, (std::min)(100, level));
	for (const auto& definition : GetEnemyCatalogue()) {
		if (definition.faction != faction || static_cast<int>(definition.tier) > static_cast<int>(maximumTier)) continue;
		int weight = 0;
		switch (definition.tier) {
		case EnemyTier::Basic: weight = (clampedLevel >= 80) ? 0 : (100 - clampedLevel); break;
		case EnemyTier::Rare: weight = (std::max)(1, 100 - clampedLevel / 2); break;
		case EnemyTier::Epic: weight = (std::max)(1, clampedLevel / 3); break;
		case EnemyTier::Elite: weight = (std::max)(1, (clampedLevel - 20) / 2); break;
		case EnemyTier::Boss: weight = 0; break;
		}
		for (int copy = 0; copy < weight; ++copy) candidates.push_back(&definition);
	}
	if (candidates.empty()) return nullptr;
	return std::make_unique<Enemy>(*candidates[seed % candidates.size()], level);
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