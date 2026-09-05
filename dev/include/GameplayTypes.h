#pragma once

#include <string>
#include <vector>

struct Stats {
	int hp = 0;
	int def = 0;
	int spd = 0;
	int atk = 0;
	int acc = 0;
	int res = 0;
	int critRate = 0;
	int critDamage = 0;
	int evasion = 0;
	int healingPotency = 0;
	int barrierPotency = 0;
	int dotPotency = 0;
	int lifesteal = 0;
	int debuffDurationReduction = 0;
};

enum class StatusType {
	Stun,
	Bleed,
	Burn,
	Poison,
	HealOverTime,
	Slow,
	VerdigrisMark,
	DamageReduction,
	Barrier
};

struct StatusEffect {
	StatusType type;
	int turns = 1;
	int potency = 0;
	int sourceAttack = 0;
	int dotPotency = 0;
};

enum class Rarity { Common, Uncommon, Rare, Epic, Legendary, Artifact };
enum class EquipSlot { Head, Torso, Boots, Hand, Accessory, None };
enum class ItemCategory { Equippable, Consumable, Clue, Crux, ShimmerSkill };

enum class ItemStat {
	HP,
	DEF,
	SPD,
	ATK,
	Accuracy,
	Resistance,
	CritRate,
	CritDamage,
	Evasion,
	HealingPotency,
	BarrierPotency,
	DotPotency,
	Lifesteal,
	DebuffDurationReduction
};

struct ItemDefinition {
	std::string name;
	std::string description;
	ItemCategory category = ItemCategory::Consumable;
	Rarity rarity = Rarity::Common;
	EquipSlot slot = EquipSlot::None;
	Stats stats;
	std::string effect;
	std::string uniqueEffect;
	std::string theme;
	std::string mainStat;
	std::vector<ItemStat> allowedSubstats;
	std::vector<ItemStat> forbiddenStats;
	std::string setName;
	std::string passive;
	std::string setPassive;
};
