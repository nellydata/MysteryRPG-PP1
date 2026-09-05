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
};

enum class Rarity { Common, Uncommon, Rare, Epic, Legendary, Artifact };
enum class EquipSlot { Head, Torso, Boots, Hand, None };
enum class ItemCategory { Equippable, Consumable, Clue, Crux, ShimmerSkill };

struct ItemDefinition {
	std::string name;
	std::string description;
	ItemCategory category = ItemCategory::Consumable;
	Rarity rarity = Rarity::Common;
	EquipSlot slot = EquipSlot::None;
	Stats stats;
	std::string effect;
	std::string uniqueEffect;
};
