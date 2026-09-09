#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

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
	int healingReceived = 0;
	int barrierPotency = 0;
	int dotPotency = 0;
	int lifesteal = 0;
	int debuffDurationReduction = 0;
	int hpPercent = 0;
	int defPercent = 0;
	int spdPercent = 0;
	int atkPercent = 0;
	int accuracyPercent = 0;
	int resistancePercent = 0;
	int critRatePercent = 0;
	int critDamagePercent = 0;
	int damagePercent = 0;
	int damageTakenPercent = 0;
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
	Barrier,
	AttackUp,
	AttackDown,
	DefenseUp,
	DefenseDown,
	SpeedUp,
	AccuracyUp,
	AccuracyDown,
	CritUp,
	CritDamageUp,
	GuaranteedCrit,
	DotUp,
	HealingPotencyUp,
	DotReduction,
	HealResistance,
	AccuracyFloor
};

struct StatusEffect {
	StatusType type;
	int turns = 1;
	int potency = 0;
	int sourceAttack = 0;
	int dotPotency = 0;
	int sourceDefense = 0;
	std::string source;
};

enum class StackPolicy {
	IndependentStacks,
	RefreshDuration,
	ExtendDuration,
	ReplaceIfStronger,
	Unique
};

inline StackPolicy GetStatusStackPolicy(StatusType type) {
	switch (type) {
	case StatusType::Burn:
	case StatusType::Bleed:
	case StatusType::Poison:
		return StackPolicy::IndependentStacks;
	case StatusType::HealOverTime:
	case StatusType::Stun:
	case StatusType::Slow:
	case StatusType::VerdigrisMark:
	case StatusType::DamageReduction:
	case StatusType::Barrier:
	case StatusType::AttackUp:
	case StatusType::AttackDown:
	case StatusType::DefenseUp:
	case StatusType::DefenseDown:
	case StatusType::AccuracyUp:
	case StatusType::AccuracyDown:
	case StatusType::CritUp:
	case StatusType::CritDamageUp:
	case StatusType::GuaranteedCrit:
	case StatusType::DotUp:
	case StatusType::HealingPotencyUp:
	case StatusType::HealResistance:
	case StatusType::AccuracyFloor:
		return StackPolicy::RefreshDuration;
	}
	return StackPolicy::Unique;
}

enum class Rarity { Common, Uncommon, Rare, Epic, Legendary, Artifact };
enum class EquipSlot { Head, Torso, Boots, Hand, Accessory, None };
enum class ItemCategory { Equippable, Consumable, Clue, Crux, ShimmerSkill };

enum class EnemyFaction { Devotee, Automata, Verdigris };
enum class EnemyTier { Basic, Rare, Epic, Elite, Boss };
enum class EnemyRole { Balanced, Tank, Striker, Speedster, Support, Controller, DotCaster, Executioner };
enum class RoomType { Sanctuary, Entrance, Archive, Chapel, Storage };
enum class ConsumableProfile { Restorative, Devotional, OccultMechanica, Ritual, SupplyCache };
enum class EquipmentFamily { Plain, Choir, Occultist, Shimmer, Verdigris, Mechanica, Ironbound, Sanctum, Martyr, Clockwork };

struct WeightedEnemyEntry {
	std::string enemyId;
	int weight = 1;
	int minimumLevel = 1;
};

struct RoomDefinition {
	std::string id;
	std::string displayName;
	RoomType type = RoomType::Entrance;
	std::vector<WeightedEnemyEntry> enemiesBeforeClear;
	std::vector<WeightedEnemyEntry> enemiesAfterClear;
	std::string bossId;
	int firstBossSpawnChance = 50;
	int farmBossStartingChance = 8;
	ConsumableProfile consumableProfile = ConsumableProfile::Devotional;
	std::string nextRoomId;
	int firstClearBossLevel = 1;
	int intendedProgressionLevel = 1;
	std::int64_t firstClearBossXP = 0;
};

struct LootContext {
	int playerLevel = 1;
	std::string roomId;
	std::string enemyId;
	EnemyFaction faction = EnemyFaction::Devotee;
	EnemyTier enemyTier = EnemyTier::Basic;
	bool bossFirstClear = false;
	bool bossFarmEcho = false;
	bool guaranteedBasicGear = false;
	bool fountainFirstClear = false;
};

struct DropProfile {
	int equipmentChance = 0;
	int consumableChance = 0;
	int guaranteedEquipment = 0;
	int bonusEquipmentChance = 0;
};

struct EnemySkillDefinition {
	std::string name;
	std::string effect;
	int multiplier = 100;
	int cooldown = 0;
	int duration = 0;
	int statusPotency = 20;
};

struct EnemyDefinition {
	std::string name;
	EnemyFaction faction = EnemyFaction::Devotee;
	EnemyTier tier = EnemyTier::Basic;
	int hpBase = 1;
	int atkBase = 1;
	int defBase = 0;
	int speed = 1;
	int xpReward = 15;
	std::vector<EnemySkillDefinition> skills;
	std::string passive;
	int rareDropChance = 0;
	int epicDropChance = 0;
	int legendaryDropChance = 0;
	EnemyRole role = EnemyRole::Balanced;
	std::string lootTag;
	int minimumLevel = 1;
};

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
	int mainStatPercent = 0;
	std::vector<ItemStat> allowedSubstats;
	std::vector<ItemStat> forbiddenStats;
	std::string setName;
	std::string passive;
	std::string setPassive;
	int mainStatValue = 0;
	std::vector<std::pair<ItemStat, int>> substats;
	EquipmentFamily family = EquipmentFamily::Plain;
};
