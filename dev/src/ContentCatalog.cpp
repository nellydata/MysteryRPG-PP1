#include "ContentCatalog.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include "CombatMath.h"
#include "EnemyCatalog.h"
#include "MapManager.h"
#include "EquipmentStats.h"
#include "ConsumableEffects.h"
#include <set>

namespace {
EquipmentFamily FamilyForName(const std::string& name) {
	if (name.find("verdigris") != std::string::npos) return EquipmentFamily::Verdigris;
	if (name.find("mechan") != std::string::npos || name.find("clockwork") != std::string::npos) return EquipmentFamily::Mechanica;
	if (name.find("choir") != std::string::npos) return EquipmentFamily::Choir;
	if (name.find("shimmer") != std::string::npos) return EquipmentFamily::Shimmer;
	if (name.find("occult") != std::string::npos) return EquipmentFamily::Occultist;
	if (name.find("iron") != std::string::npos) return EquipmentFamily::Ironbound;
	return EquipmentFamily::Plain;
}

void SetSlotRules(ItemDefinition& item, EquipSlot slot, const std::string& mainStat) {
	item.mainStat = mainStat;
	switch (slot) {
	case EquipSlot::Head:
		item.theme = "insight, clarity, protection, perception";
		item.allowedSubstats = {ItemStat::CritRate, ItemStat::CritDamage, ItemStat::ATK, ItemStat::DEF, ItemStat::HP};
		break;
	case EquipSlot::Torso:
		item.theme = "protection, stability, purity, endurance";
		item.allowedSubstats = {ItemStat::HP, ItemStat::DEF, ItemStat::Resistance, ItemStat::ATK};
		break;
	case EquipSlot::Hand:
		item.theme = "offense, aggression, precision, execution";
		item.allowedSubstats = {ItemStat::ATK, ItemStat::CritRate, ItemStat::CritDamage, ItemStat::Accuracy};
		break;
	case EquipSlot::Boots:
		item.theme = "movement, agility, positioning";
		item.allowedSubstats = {ItemStat::Evasion, ItemStat::Accuracy, ItemStat::ATK, ItemStat::DEF, ItemStat::SPD};
		break;
	case EquipSlot::Accessory:
		item.theme = "spiritual power, amplification, utility";
		item.allowedSubstats = {ItemStat::ATK, ItemStat::DEF, ItemStat::HP, ItemStat::Resistance, ItemStat::DotPotency, ItemStat::HealingPotency, ItemStat::BarrierPotency};
		break;
	default: break;
	}

			if (!item.setName.empty()) item.theme = item.setName + ": " + item.theme;
}

unsigned TemplateSeed(const std::string& name) {
	unsigned seed = 2166136261u;
	for (unsigned char character : name) seed = (seed ^ character) * 16777619u;
	return seed;
}

bool AllowedSubstat(EquipSlot slot, ItemStat stat) {
	static const auto pools = []() {
		std::array<std::vector<ItemStat>, 5> result;
		for (int index = 0; index < 5; ++index) {
			ItemDefinition rules;
			SetSlotRules(rules, static_cast<EquipSlot>(index), "");
			result[index] = rules.allowedSubstats;
		}
		return result;
	}();
	const int index = static_cast<int>(slot);
	return index >= 0 && index < 5 && std::find(pools[index].begin(), pools[index].end(), stat) != pools[index].end();
}

bool AllowedMain(EquipSlot slot, const std::string& key) {
	const auto rule = EquipmentStats::ForMain(key);
	if (!rule.flat || slot == EquipSlot::None || static_cast<int>(slot) < 0 || static_cast<int>(slot) >= 5) return false;
	if (slot == EquipSlot::Accessory) return rule.flat != &Stats::spd;
	if (rule.flat == &Stats::hp) return slot == EquipSlot::Head || slot == EquipSlot::Torso;
	if (rule.flat == &Stats::def) return slot == EquipSlot::Torso;
	if (rule.flat == &Stats::atk) return slot == EquipSlot::Hand;
	if (rule.flat == &Stats::spd || rule.flat == &Stats::evasion) return slot == EquipSlot::Boots;
	if (rule.flat == &Stats::acc) return slot == EquipSlot::Head || slot == EquipSlot::Hand;
	if (rule.flat == &Stats::res) return slot == EquipSlot::Head || slot == EquipSlot::Torso;
	if (rule.flat == &Stats::critRate || rule.flat == &Stats::critDamage) return slot == EquipSlot::Head || slot == EquipSlot::Hand;
	return false;
}

void AddEquippables(std::vector<ItemDefinition>& result, EquipSlot slot, const char* const* names,
					int count, Rarity rarity, const std::string& mainStat, int value) {
	(void)value; // Legacy catalogue call-site parameter; ranges are authoritative.
	for (int i = 0; i < count; ++i) {
		ItemDefinition item;
		const std::string name = names[i];
		std::string resolvedMainStat = mainStat;
		if (slot == EquipSlot::Head && (name.find("automaton") != std::string::npos || name.find("shimmer") != std::string::npos)) resolvedMainStat = "acc";
		if (slot == EquipSlot::Accessory) {
			if (name.find("automaton") != std::string::npos || name.find("shimmer_ascendant") != std::string::npos) resolvedMainStat = "atk";
			else if (name.find("iron") != std::string::npos) resolvedMainStat = "def";
			else resolvedMainStat = "hp";
		}
		item.name = name;
		item.family = FamilyForName(name);
		item.description = "Micare " + resolvedMainStat + " equipment.";
		item.category = ItemCategory::Equippable;
		item.rarity = rarity;
		item.slot = slot;
		item.mainStat = resolvedMainStat;
		SetSlotRules(item, slot, resolvedMainStat);
		if (rarity == Rarity::Artifact) {
			if (slot == EquipSlot::Head) { item.passive = "Critical hits reveal enemies and restore 8% HP"; item.stats.critRate += 8; item.stats.critDamage += 20; }
			else if (slot == EquipSlot::Torso) { item.passive = "Reduce incoming damage by 12% and gain a barrier after taking damage"; item.stats.res += 10; item.stats.barrierPotency += 18; }
			else if (slot == EquipSlot::Hand) { item.passive = "Attacks apply a stacking mark and deal bonus damage over time"; item.stats.dotPotency += 18; item.stats.lifesteal += 6; }
			else if (slot == EquipSlot::Boots) { item.passive = "After three actions, gain a bonus action and evade the next hit"; item.stats.evasion += 10; item.stats.spd += 4; }
			else if (slot == EquipSlot::Accessory) { item.passive = "Shimmer skills amplify all rolled stats for two turns"; item.stats.acc += 8; item.stats.healingPotency += 15; }
		} else if (rarity == Rarity::Epic) {
			if (slot == EquipSlot::Head) { item.passive = "Crit rate +5%"; item.stats.critRate += 5; }
			else if (slot == EquipSlot::Torso) { item.passive = "Healing received +10%"; item.stats.healingReceived += 10; }
			else if (slot == EquipSlot::Hand) { item.passive = "DOT potency +10%"; item.stats.dotPotency += 10; }
			else if (slot == EquipSlot::Boots) { item.passive = "Accuracy +18%"; item.stats.acc += 18; }
			else if (slot == EquipSlot::Accessory) { item.passive = "Accuracy +18%"; item.stats.acc += 18; }
		}
		result.push_back(RollLootInstance(item, TemplateSeed(name)));
	}
}
}

int GetEquipmentSubstatCount(Rarity rarity) {
	const int tier = static_cast<int>(rarity);
	if (tier < 0 || tier > 5) throw std::invalid_argument("Unknown equipment rarity");
	return (std::min)(4, tier);
}

std::pair<int, int> GetEquipmentMainStatRange(const std::string& mainStat, Rarity rarity) {
	const int tier = static_cast<int>(rarity);
	const auto rule = EquipmentStats::ForMain(mainStat);
	if (tier < 0 || tier > 5 || !rule.flat) throw std::invalid_argument("Unknown equipment main stat or rarity");
	static const std::pair<int, int> core[] = {{8, 12}, {12, 16}, {16, 22}, {22, 28}, {28, 34}, {34, 40}};
	static const std::pair<int, int> speed[] = {{3, 5}, {5, 8}, {8, 12}, {12, 16}, {16, 20}, {20, 25}};
	static const std::pair<int, int> crit[] = {{2, 4}, {4, 6}, {6, 9}, {9, 12}, {12, 15}, {15, 18}};
	static const std::pair<int, int> critDamage[] = {{5, 8}, {8, 12}, {12, 18}, {18, 25}, {25, 32}, {32, 40}};
	static const std::pair<int, int> accuracy[] = {{5, 8}, {8, 12}, {12, 18}, {18, 24}, {24, 30}, {30, 36}};
	static const std::pair<int, int> potency[] = {{8, 12}, {12, 18}, {18, 25}, {25, 32}, {32, 40}, {40, 50}};
	if (rule.corePercentRoll) return core[tier];
	if (rule.flat == &Stats::spd) return speed[tier];
	if (rule.flat == &Stats::critRate || rule.flat == &Stats::evasion) return crit[tier];
	if (rule.flat == &Stats::critDamage) return critDamage[tier];
	if (rule.flat == &Stats::acc || rule.flat == &Stats::res) return accuracy[tier];
	return potency[tier];
}

bool ValidateEquipmentDefinition(const ItemDefinition& item, std::string* error) {
	const auto fail = [&](const std::string& reason) {
		if (error) *error = item.name + ": " + reason;
		return false;
	};
	if (item.name.empty() || item.category != ItemCategory::Equippable || !AllowedMain(item.slot, item.mainStat))
		return fail("invalid equipment name, slot, or main stat");
	if (static_cast<int>(item.rarity) < 0 || static_cast<int>(item.rarity) > 5) return fail("invalid rarity");
	const auto main = EquipmentStats::ForMain(item.mainStat);
	const auto range = GetEquipmentMainStatRange(item.mainStat, item.rarity);
	const int mainValue = main.corePercentRoll ? item.mainStatPercent : item.mainStatValue;
	if (mainValue < range.first || mainValue > range.second ||
		(main.corePercentRoll ? item.mainStatValue != 0 : item.mainStatPercent != 0)) return fail("main stat has invalid units or range");
	if (item.substats.size() != static_cast<std::size_t>(GetEquipmentSubstatCount(item.rarity))) return fail("incorrect substat count");
	std::set<ItemStat> seen;
	for (const auto& substat : item.substats) {
		const auto subRange = CombatMath::SubstatRange(substat.first, item.rarity);
		if (!AllowedSubstat(item.slot, substat.first) ||
			std::find(item.allowedSubstats.begin(), item.allowedSubstats.end(), substat.first) == item.allowedSubstats.end() ||
			!seen.insert(substat.first).second || substat.second < subRange.first || substat.second > subRange.second)
			return fail("illegal, duplicate, or out-of-range substat");
	}
	for (ItemStat forbidden : item.forbiddenStats) {
		if (EquipmentStats::HasStat(item.stats, forbidden) ||
			EquipmentStats::ForStat(forbidden).flat == main.flat || seen.count(forbidden)) return fail("forbidden stat is present");
	}
	if (item.rarity == Rarity::Artifact && item.uniqueEffect.empty()) return fail("Artifact requires an authored unique effect");
	return true;
}

std::vector<ItemDefinition> GenerateInvestigationLoot(ConsumableProfile profile, int playerLevel,
                                                       const std::string& roomId, unsigned seed, bool repeat) {
    std::mt19937 rng(seed);
    const int level = (std::max)(1, playerLevel);
    const bool consumable = std::uniform_int_distribution<int>(0, 99)(rng) < (repeat ? 80 : 70);
    if (consumable) {
        // Room source profiles explicitly select their appropriate restorative/resource reward.
        const auto item = BuildInvestigationConsumable(profile, level);
        if (item.name.empty()) throw std::logic_error("Missing investigation consumable profile");
        return {RollLootInstance(item, rng())};
    }
    static const auto catalogue = BuildEquippableCatalog();
    std::vector<const ItemDefinition*> pools[4];
    for (const auto& item : catalogue) {
        const int rarity = static_cast<int>(item.rarity);
        if (rarity >= 0 && rarity < 4) pools[rarity].push_back(&item);
    }
    double weights[] = {level < 5 ? 100.0 : 20.0, level >= 5 ? 60.0 : 0.0,
        level >= 10 ? 35.0 : 0.0, level >= 20 ? 20.0 : 0.0};
    for (int rarity = 0; rarity < 4; ++rarity) if (pools[rarity].empty()) weights[rarity] = 0;
    if (weights[0] + weights[1] + weights[2] + weights[3] <= 0)
        throw std::logic_error("No eligible investigation equipment");
    const int rarity = std::discrete_distribution<int>(std::begin(weights), std::end(weights))(rng);
    const auto& pool = pools[rarity];
    const auto* item = pool[std::uniform_int_distribution<std::size_t>(0, pool.size() - 1)(rng)];
    return {RollLootInstance(*item, rng())};
}

bool ValidateMicareContent(std::string* error) {
	if (!ValidateContentCatalog(error)) return false;
	if (!ValidateEnemyCatalogue(error)) return false;
	for (const auto& room : MapManager::GetFloor1Definitions()) {
		if (room.id.empty() || (room.id != "Fountain" && room.bossId.empty()) || room.firstBossSpawnChance < 0 || room.firstBossSpawnChance > 100) {
			if (error) *error = "Room profile has invalid boss progression: " + room.id;
			return false;
		}
		if (!room.bossId.empty() && FindEnemyDefinition(room.bossId) == nullptr) {
			if (error) *error = "Room boss is missing from enemy catalogue: " + room.bossId;
			return false;
		}
		if (!room.bossId.empty() && (room.firstClearBossLevel < 1 || room.firstClearBossLevel > 100 ||
			room.intendedProgressionLevel < 1 || room.intendedProgressionLevel > 100 || room.firstClearBossXP <= 0 ||
			room.farmBossStartingChance < 0 || room.farmBossStartingChance > 100)) {
			if (error) *error = "Invalid fixed boss profile: " + room.id;
			return false;
		}
		for (const auto& entry : room.enemiesBeforeClear) if (FindEnemyDefinition(entry.enemyId) == nullptr) {
			if (error) *error = "Room pool enemy is missing: " + entry.enemyId;
			return false;
		}
		for (const auto& entry : room.enemiesAfterClear) if (FindEnemyDefinition(entry.enemyId) == nullptr) {
			if (error) *error = "Farm pool enemy is missing: " + entry.enemyId;
			return false;
		}
	}
	return true;
}

std::string InvestigationConsumableName(ConsumableProfile profile, int playerLevel) {
	if (playerLevel >= 20) {
		switch (profile) {
		case ConsumableProfile::Restorative: return "lumen_draught";
		case ConsumableProfile::OccultMechanica: return "shimmer_vial";
		case ConsumableProfile::Ritual: return "ritual_powder";
		default: break;
		}
	}
	switch (profile) {
	case ConsumableProfile::Restorative: return "blessed_bandages";
	case ConsumableProfile::OccultMechanica: return "mechanical_charge";
	case ConsumableProfile::Ritual: return "ritual_powder";
	case ConsumableProfile::SupplyCache:
	case ConsumableProfile::Devotional: return playerLevel >= 10 ? "blessed_bandages" : "cellar_rations";
	}
	return "cellar_rations";
}

ItemDefinition BuildInvestigationConsumable(ConsumableProfile profile, int playerLevel) {
	const std::string name = InvestigationConsumableName(profile, playerLevel);
	for (auto definition : BuildConsumableCatalog()) {
		if (definition.name != name) continue;
		return definition;
	}
	return {};
}

double BossArtifactChance(int playerLevel) {
	if (playerLevel < 70) return 0.0;
	if (playerLevel < 80) return 1.0 + (playerLevel - 70) * .1;
	return 2.0 + ((std::min)(100, playerLevel) - 80) * .15;
}

std::vector<ItemDefinition> GenerateLoot(const LootContext& context, unsigned seed) {
	std::vector<ItemDefinition> result;
	std::mt19937 rng(seed);
	static const auto equipment = BuildEquippableCatalog();
	static const auto pools = [] {
		std::array<std::vector<const ItemDefinition*>, 6> result;
		for (const auto& item : equipment) result[static_cast<int>(item.rarity)].push_back(&item);
		return result;
	}();
	static const auto consumables = BuildConsumableCatalog();
	const bool boss = context.enemyTier == EnemyTier::Boss;
	const bool elite = context.enemyTier == EnemyTier::Elite;
	const int level = (std::max)(1, context.playerLevel);
	int guaranteed = 0;
	double extraChance = 0;
	double consumableChance = 0;
	switch (context.enemyTier) {
	case EnemyTier::Basic:
		extraChance = .85 + .15 * ((std::min)(30, level) - 1) / 29.0;
		consumableChance = .30; break;
	case EnemyTier::Rare: guaranteed = 1; extraChance = .20; consumableChance = .40; break;
	case EnemyTier::Epic: guaranteed = 1; extraChance = .50; consumableChance = .50; break;
	case EnemyTier::Elite: guaranteed = 2; extraChance = .35; consumableChance = .65; break;
	case EnemyTier::Boss: guaranteed = 3; extraChance = .50; consumableChance = 1.0; break;
	default: throw std::invalid_argument("Unknown enemy loot tier");
	}
	// These decisions have their own draws, independent of one another and of
	// all rarity/template/stat draws. Pity never changes the consumable draw.
	const bool extra = std::bernoulli_distribution(extraChance)(rng);
	const bool consumable = std::bernoulli_distribution(consumableChance)(rng);
	int count = guaranteed + (extra ? 1 : 0);
	if (context.fountainFirstClear) count = 1;
	else if (context.enemyTier == EnemyTier::Basic && context.guaranteedBasicGear) count = 1;
	for (int drop = 0; drop < count; ++drop) {
		const double rarityRoll = std::uniform_real_distribution<double>(0.0, 100.0)(rng);
		Rarity rarity = Rarity::Common;
		// Reported interpolation: Artifact replaces Legendary within their
		// existing combined 30% bucket (1% at 70, 2% at 80, 5% at 100).
		const double artifactChance = BossArtifactChance(level);
		if (!context.fountainFirstClear) {
			if (boss && level >= 70 && rarityRoll < artifactChance) rarity = Rarity::Artifact;
			else if (boss && level >= 40 && rarityRoll < 30) rarity = Rarity::Legendary;
			else if (level >= 20 && rarityRoll < (boss ? 95 : elite ? 75 : 45)) rarity = Rarity::Epic;
			else if (level >= 10 && rarityRoll < (boss ? 100 : elite ? 95 : 80)) rarity = Rarity::Rare;
			else rarity = level >= 5 ? Rarity::Uncommon : Rarity::Common;
		}
		const auto& candidates = pools[static_cast<int>(rarity)];
		if (candidates.empty()) throw std::logic_error("Missing eligible enemy loot equipment");
		const auto* item = candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(rng)];
		result.push_back(RollLootInstance(*item, rng()));
	}
	if (consumable) {
		if (consumables.empty()) throw std::logic_error("Missing enemy loot consumables");
		result.push_back(consumables[std::uniform_int_distribution<std::size_t>(0, consumables.size() - 1)(rng)]);
	}
	return result;
}

ItemDefinition RollLootInstance(ItemDefinition item, unsigned seed) {
	if (item.category != ItemCategory::Equippable) return item;
	if (!AllowedMain(item.slot, item.mainStat)) throw std::invalid_argument("Invalid equipment main stat: " + item.name);
	const auto range = GetEquipmentMainStatRange(item.mainStat, item.rarity);
	std::mt19937 rng(seed);
	const int mainRoll = std::uniform_int_distribution<int>(range.first, range.second)(rng);
	item.mainStatValue = 0;
	item.mainStatPercent = 0;
	const auto main = EquipmentStats::ForMain(item.mainStat);
	if (main.corePercentRoll) item.mainStatPercent = mainRoll;
	else item.mainStatValue = mainRoll;
	item.substats.clear();
	std::vector<ItemStat> eligible;
	for (ItemStat stat : item.forbiddenStats) {
		if (EquipmentStats::HasStat(item.stats, stat) || EquipmentStats::ForStat(stat).flat == main.flat)
			throw std::invalid_argument("Forbidden fixed or main stat: " + item.name);
	}
	for (ItemStat stat : item.allowedSubstats) {
		if (AllowedSubstat(item.slot, stat) &&
			std::find(item.forbiddenStats.begin(), item.forbiddenStats.end(), stat) == item.forbiddenStats.end() &&
			std::find(eligible.begin(), eligible.end(), stat) == eligible.end()) eligible.push_back(stat);
	}
	const int count = GetEquipmentSubstatCount(item.rarity);
	if (eligible.size() < static_cast<std::size_t>(count)) throw std::invalid_argument("Too few legal substats: " + item.name);
	std::shuffle(eligible.begin(), eligible.end(), rng);
	for (int index = 0; index < count; ++index) {
		const auto subRange = CombatMath::SubstatRange(eligible[index], item.rarity);
		item.substats.emplace_back(eligible[index], std::uniform_int_distribution<int>(subRange.first, subRange.second)(rng));
	}
	return item;
}

bool ValidateContentCatalog(std::string* error) {
	const auto validate = [](const std::vector<ItemDefinition>& items) {
		for (const auto& item : items) if (item.name.empty()) return false;
		return true;
	};
	if (!validate(BuildEquippableCatalog()) || !validate(BuildConsumableCatalog()) || !validate(BuildClueCatalog()) || !validate(BuildCruxCatalog())) {
		if (error) *error = "Content catalogue contains an unnamed item.";
		return false;
	}
	for (const auto& item : BuildEquippableCatalog()) if (!ValidateEquipmentDefinition(item, error)) return false;
	if (!ValidateConsumableEffects(BuildConsumableCatalog(), error)) return false;
	return true;
}

std::vector<ItemDefinition> BuildEquippableCatalog() {
	std::vector<ItemDefinition> result;
	const char* headCommon[] = {"linen_veil","cellar_hood","plain_sisters_bonnet","dusty_prayer_shawl","worn_lace_headwrap","simple_choir_cap","old_ritual_band","faded_mourning_veil","basic_attendants_hood","thin_silk_headcloth"};
	const char* headUncommon[] = {"verdigris_edged_veil","shimmer_thread_bonnet","choirmistress_lace_hood","gilded_prayer_shawl","mechanized_hairpin","hollow_choir_headwrap","wax_sealed_sisters_cap"};
	const char* headRare[] = {"veil_of_the_luminous_choir","occultists_mourning_hood","shimmer_infused_headband","verdigris_crownlet","mechanica_ritual_circlet"};
	const char* headEpic[] = {"choirs_resonant_halo","veil_of_the_verdigris_saint","shimmer_woven_diadem","clockwork_sisters_hood"};
	const char* headLegendary[] = {"radiant_oath_veil","crown_of_the_broken_choir","lumen_forged_headpiece"};
	const char* torsoCommon[] = {"plain_seraphic_dress","cellar_attire","worn_convent_robe","simple_habit_garment","dusty_linen_gown","basic_choir_dress","old_ritual_robe","faded_sanctum_gown","thin_silk_attire","plain_vestment"};
	const char* torsoUncommon[] = {"verdigris_trimmed_robe","shimmer_thread_gown","choirmistress_attire","gilded_sanctum_dress","mechanized_corset","hollow_choir_robe","wax_sealed_vestment"};
	const char* torsoRare[] = {"dress_of_the_luminous_choir","occultists_ritual_gown","shimmer_infused_vestment","verdigris_sanctum_attire","mechanica_ritual_garment"};
	const char* torsoEpic[] = {"choirs_resonant_vestment","robes_of_the_verdigris_saint","shimmer_woven_gown","clockwork_sisters_vestment"};
	const char* torsoLegendary[] = {"radiant_oath_vestment","garment_of_the_broken_choir","lumen_forged_robe"};
	const char* bootsCommon[] = {"plain_leather_shoes","cellar_sandals","worn_convent_boots","simple_habit_shoes","dusty_lace_boots","basic_choir_sandals","old_ritual_shoes","faded_sanctum_boots","thin_silk_slippers","plain_vestment_shoes"};
	const char* bootsUncommon[] = {"verdigris_trimmed_boots","shimmer_thread_sandals","choirmistress_lace_boots","gilded_sanctum_shoes","mechanized_step_boots","hollow_choir_slippers","wax_sealed_sandals"};
	const char* bootsRare[] = {"boots_of_the_luminous_choir","occultists_ritual_boots","shimmer_infused_slippers","verdigris_sanctum_sandals","mechanica_ritual_boots"};
	const char* bootsEpic[] = {"choirs_resonant_boots","boots_of_the_verdigris_saint","shimmer_woven_sandals","clockwork_sisters_step"};
	const char* bootsLegendary[] = {"radiant_oath_boots","steps_of_the_broken_choir","lumen_forged_sandals"};
	const char* handCommon[] = {"worn_ritual_dagger","cellar_bible","worn_crucible","simple_ritual_staff","dusty_prayer_book","basic_choir_tome","old_ritual_blade","faded_sanctum_relic","thin_silk_gloves","plain_vestment_staff"};
	const char* handUncommon[] = {"verdigris_etched_rosary","shimmer_thread_bible","choirmistress_ritual_blade","gilded_prayer_tome","mechanized_relic_staff","hollow_choir_crucible","wax_sealed_ritual_gloves"};
	const char* handRare[] = {"tome_of_the_luminous_choir","occultists_blood_blade","shimmer_infused_crucible","verdigris_sanctum_staff","mechanica_ritual_tome"};
	const char* handEpic[] = {"choirs_resonant_relic","blade_of_the_verdigris_saint","shimmer_woven_crucible","clockwork_sisters_staff"};
	const char* handLegendary[] = {"radiant_oath_blade","relic_of_the_broken_choir","lumen_forged_crucible"};
	AddEquippables(result, EquipSlot::Head, headCommon, 10, Rarity::Common, "hp", 5);
	AddEquippables(result, EquipSlot::Head, headUncommon, 7, Rarity::Uncommon, "hp", 10);
	AddEquippables(result, EquipSlot::Head, headRare, 5, Rarity::Rare, "hp", 15);
	AddEquippables(result, EquipSlot::Head, headEpic, 4, Rarity::Epic, "hp", 20);
	AddEquippables(result, EquipSlot::Head, headLegendary, 3, Rarity::Legendary, "hp", 25);
	AddEquippables(result, EquipSlot::Torso, torsoCommon, 10, Rarity::Common, "def", 2);
	AddEquippables(result, EquipSlot::Torso, torsoUncommon, 7, Rarity::Uncommon, "def", 4);
	AddEquippables(result, EquipSlot::Torso, torsoRare, 5, Rarity::Rare, "def", 6);
	AddEquippables(result, EquipSlot::Torso, torsoEpic, 4, Rarity::Epic, "def", 8);
	AddEquippables(result, EquipSlot::Torso, torsoLegendary, 3, Rarity::Legendary, "def", 10);
	AddEquippables(result, EquipSlot::Boots, bootsCommon, 10, Rarity::Common, "spd", 1);
	AddEquippables(result, EquipSlot::Boots, bootsUncommon, 7, Rarity::Uncommon, "spd", 2);
	AddEquippables(result, EquipSlot::Boots, bootsRare, 5, Rarity::Rare, "spd", 3);
	AddEquippables(result, EquipSlot::Boots, bootsEpic, 4, Rarity::Epic, "spd", 4);
	AddEquippables(result, EquipSlot::Boots, bootsLegendary, 3, Rarity::Legendary, "spd", 5);
	AddEquippables(result, EquipSlot::Hand, handCommon, 10, Rarity::Common, "atk", 2);
	AddEquippables(result, EquipSlot::Hand, handUncommon, 7, Rarity::Uncommon, "atk", 4);
	AddEquippables(result, EquipSlot::Hand, handRare, 5, Rarity::Rare, "atk", 6);
	AddEquippables(result, EquipSlot::Hand, handEpic, 4, Rarity::Epic, "atk", 8);
	AddEquippables(result, EquipSlot::Hand, handLegendary, 3, Rarity::Legendary, "atk", 10);
	const char* accessoryCommon[] = {"plain_rosary", "tarnished_iron_ring", "cracked_automaton_charm", "old_choir_pendant", "brass_prayer_token"};
	const char* accessoryUncommon[] = {"blessed_rosary", "reinforced_iron_ring", "polished_automaton_charm", "choir_ember_pendant"};
	const char* accessoryRare[] = {"ceremonial_rosary_of_grace", "ironbound_ring", "automaton_combat_charm", "choir_flame_pendant"};
	const char* accessoryEpic[] = {"rosary_of_benediction", "ironheart_ring", "automaton_strike_charm", "choir_inferno_pendant"};
	const char* accessoryLegendary[] = {"radiant_oath_reliquary", "broken_choir_reliquary", "lumen_forged_reliquary"};
	AddEquippables(result, EquipSlot::Accessory, accessoryCommon, 5, Rarity::Common, "hp", 5);
	AddEquippables(result, EquipSlot::Accessory, accessoryUncommon, 4, Rarity::Uncommon, "hp", 10);
	AddEquippables(result, EquipSlot::Accessory, accessoryRare, 4, Rarity::Rare, "hp", 15);
	AddEquippables(result, EquipSlot::Accessory, accessoryEpic, 4, Rarity::Epic, "hp", 20);
	AddEquippables(result, EquipSlot::Accessory, accessoryLegendary, 3, Rarity::Legendary, "hp", 25);
	for (auto& item : result) {
		if (item.rarity == Rarity::Legendary) {
			if (item.name.find("broken_choir") != std::string::npos) {
				item.setName = "Choir of Verdigris";
				item.setPassive = "Five-piece: gain a barrier at battle start and increase healing received by 25%.";
			} else if (item.name.find("lumen_forged") != std::string::npos) {
				item.setName = "Shimmer Ascendant";
				item.setPassive = "Five-piece: Shimmer skills deal 25% more damage and restore 5% max HP.";
			} else if (item.name.find("radiant_oath") != std::string::npos) {
				item.setName = "Automaton Overlord";
				item.setPassive = "Five-piece: accuracy is perfected, critical damage rises by 50%, and every third action grants a bonus turn.";
			}
		}
	}
	result.push_back({"verdigris_halo_of_saint_adelaide", "Artifact headgear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Head, {12}, {}, "shimmer_cooldown_reduced_by_1"});
	result.push_back({"choirs_eternal_mourning_veil", "Artifact headgear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Head, {12}, {}, "immune_to_stun_once_per_battle"});
	result.push_back({"locket_crown_of_the_first_light", "Artifact headgear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Head, {12}, {}, "crit_hits_restore_5_percent_hp"});
	result.push_back({"verdigris_saint_vestment", "Artifact torso gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Torso, {0, 12}, {}, "reduces_damage_taken_by_10_percent"});
	result.push_back({"choirs_eternal_ritual_gown", "Artifact torso gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Torso, {0, 12}, {}, "immune_to_bleed"});
	result.push_back({"first_light_sanctum_attire", "Artifact torso gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Torso, {0, 12}, {}, "gain_5_percent_def_when_hit"});
	result.push_back({"verdigris_saint_step", "Artifact boots.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Boots, {0, 0, 12}, {}, "gain_extra_turn_every_12_actions"});
	result.push_back({"choirs_eternal_boots", "Artifact boots.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Boots, {0, 0, 12}, {}, "immune_to_slow"});
	result.push_back({"first_light_sandals", "Artifact boots.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Boots, {0, 0, 12}, {}, "spd_increases_by_1_each_turn"});
	result.push_back({"verdigris_saint_relic", "Artifact hand gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Hand, {0, 0, 0, 12}, {}, "attacks_apply_verdigris_mark_dealing_bonus_damage_over_time"});
	result.push_back({"choirs_eternal_tome", "Artifact hand gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Hand, {0, 0, 0, 12}, {}, "first_attack_each_battle_is_guaranteed_crit"});
	result.push_back({"first_light_crucible", "Artifact hand gear.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Hand, {0, 0, 0, 12}, {}, "gain_5_percent_atk_every_time_you_use_a_shimmer_skill"});
	result.push_back({"verdigris_saint_reliquary", "Artifact accessory.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Accessory, {12}, {}, "verdigris_set_lifesteal"});
	result.push_back({"choirs_eternal_reliquary", "Artifact accessory.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Accessory, {12}, {}, "choir_set_barrier"});
	result.push_back({"first_light_reliquary", "Artifact accessory.", ItemCategory::Equippable, Rarity::Artifact, EquipSlot::Accessory, {12}, {}, "first_light_set_shimmer"});
	for (auto& item : result) {
		if (item.rarity != Rarity::Artifact) continue;
		const std::string artifactMainStat = item.slot == EquipSlot::Boots ? "spd" : item.slot == EquipSlot::Torso ? "def" : item.slot == EquipSlot::Head ? "hp" : item.slot == EquipSlot::Accessory ? "hp" : "atk";
		SetSlotRules(item, item.slot, artifactMainStat);
		item = RollLootInstance(item, TemplateSeed(item.name));
		if (item.name.find("verdigris") != std::string::npos) item.setName = "Verdigris Matron";
		else if (item.name.find("choir") != std::string::npos) item.setName = "Shimmer Reliquary";
		else if (item.name.find("first_light") != std::string::npos) item.setName = "Oracle Automaton";
		item.passive = item.uniqueEffect;
		item.theme = item.setName + ": " + item.theme;
		if (item.setName == "Verdigris Matron") item.setPassive = "Five-piece: recover 25% max HP each turn, recover 50% of damage taken, convert excess recovery into barrier, and gain 100% healing potency.";
		else if (item.setName == "Shimmer Reliquary") item.setPassive = "Five-piece: the first Shimmer skill each turn costs no cooldown and barriers absorb 50% more damage.";
		else if (item.setName == "Oracle Automaton") item.setPassive = "Five-piece: gain 30% accuracy, 75% critical damage, and a chance to repeat offensive actions.";
	}
	return result;
}

std::vector<ItemDefinition> BuildConsumableCatalog() {
	return {
		{"cellar_rations", "Restore 20 percent HP.", ItemCategory::Consumable, Rarity::Common, EquipSlot::None, {}, "restore_20_percent_hp", {}},
		{"blessed_bandages", "Restore 35 percent HP and cleanse bleed.", ItemCategory::Consumable, Rarity::Common, EquipSlot::None, {}, "restore_35_percent_hp_and_cleanse_bleed", {}},
		{"lumen_draught", "Restore 50 percent HP.", ItemCategory::Consumable, Rarity::Rare, EquipSlot::None, {}, "restore_50_percent_hp", {}},
		{"choir_incense", "Increase ACC by 10 percent for 3 turns.", ItemCategory::Consumable, Rarity::Uncommon, EquipSlot::None, {}, "increase_acc_by_10_percent_for_3_turns", {}},
		{"verdigris_tincture", "Increase DEF by 15 percent for 3 turns.", ItemCategory::Consumable, Rarity::Uncommon, EquipSlot::None, {}, "increase_def_by_15_percent_for_3_turns", {}},
		{"shimmer_vial", "Reduce shimmer skill cooldown by 1.", ItemCategory::Consumable, Rarity::Rare, EquipSlot::None, {}, "reduce_shimmer_skill_cooldowns_by_1", {}},
		{"purification_salts", "Remove all debuffs.", ItemCategory::Consumable, Rarity::Uncommon, EquipSlot::None, {}, "remove_all_debuffs", {}},
		{"sanctum_water", "Remove burn and poison.", ItemCategory::Consumable, Rarity::Uncommon, EquipSlot::None, {}, "remove_burn_and_poison", {}},
		{"ritual_powder", "Apply burn for 60 percent of ATK.", ItemCategory::Consumable, Rarity::Rare, EquipSlot::None, {}, "apply_burn_60_percent_atk", {}},
		{"mechanical_charge", "Deal flat 50 damage.", ItemCategory::Consumable, Rarity::Rare, EquipSlot::None, {}, "deal_flat_50_damage", {}},
		{"shimmer_capsule", "Illuminate dark rooms for 30 seconds.", ItemCategory::Consumable, Rarity::Common, EquipSlot::None, {}, "illuminate_dark_rooms_for_30_seconds", {}},
		{"verdigris_oil", "Lubricate mechanisms and unlock rusted doors.", ItemCategory::Consumable, Rarity::Common, EquipSlot::None, {}, "lubricate_mechanisms_unlocking_rusted_doors", {}}
	};
}

std::vector<ItemDefinition> BuildClueCatalog() {
	return {
		{"storage_key", "Opens storage room doors.", ItemCategory::Clue, Rarity::Common, EquipSlot::None, {}, "opens_storage_room_doors", {}},
		{"cellar_master_key", "Opens all cellar doors.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "opens_all_cellar_doors", {}},
		{"choir_chest_key", "Opens locked chests in the choir wing.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "opens_locked_chests_in_choir_wing", {}},
		{"glass_cutter", "Cuts through brittle windows.", ItemCategory::Clue, Rarity::Common, EquipSlot::None, {}, "cuts_through_brittle_windows", {}},
		{"ritual_candle", "Activates occult seals.", ItemCategory::Clue, Rarity::Common, EquipSlot::None, {}, "activates_occult_seals", {}},
		{"shimmer_conduit", "Powers Shimmer mechanisms.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "powers_shimmer_mechanisms", {}},
		{"automata_core_fragment", "Repairs mechanical doors.", ItemCategory::Clue, Rarity::Uncommon, EquipSlot::None, {}, "repairs_mechanical_doors", {}},
		{"verdigris_gear", "Activates clockwork devices.", ItemCategory::Clue, Rarity::Uncommon, EquipSlot::None, {}, "activates_clockwork_devices", {}},
		{"broken_sigil_plate", "Required for occult puzzle completion.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "required_for_occult_puzzle_completion", {}},
		{"choir_logbook", "Reveals room patterns.", ItemCategory::Clue, Rarity::Common, EquipSlot::None, {}, "reveals_room_patterns", {}},
		{"sanctum_blueprints", "Reveal hidden passages.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "reveals_hidden_passages", {}},
		{"ritual_notes", "Reveal occult mechanism solutions.", ItemCategory::Clue, Rarity::Rare, EquipSlot::None, {}, "reveals_occult_mechanism_solutions", {}}
	};
}

std::vector<ItemDefinition> BuildCruxCatalog() {
	return {
		{"first_nuns_diary", "Unlocks the first major story branch.", ItemCategory::Crux, Rarity::Legendary, EquipSlot::None, {}, "unlocks_first_major_story_branch", {}},
		{"adelaide_case_file", "Reveals the player's backstory and unlocks a new zone.", ItemCategory::Crux, Rarity::Legendary, EquipSlot::None, {}, "reveals_player_backstory_and_unlocks_new_zone", {}},
		{"broken_oath", "Changes the endgame route.", ItemCategory::Crux, Rarity::Legendary, EquipSlot::None, {}, "changes_endgame_route", {}},
		{"shimmering_locket", "Activates the Shimmer gate.", ItemCategory::Crux, Rarity::Legendary, EquipSlot::None, {}, "activates_shimmer_gate", {}},
		{"verdigris_sigil", "Opens the mechanical wing.", ItemCategory::Crux, Rarity::Legendary, EquipSlot::None, {}, "opens_mechanical_wing", {}},
		{"choirs_black_box", "Reveals the final truth and unlocks the final boss.", ItemCategory::Crux, Rarity::Artifact, EquipSlot::None, {}, "reveals_final_truth_and_unlocks_final_boss", {}}
	};
}

std::vector<std::string> BuildLatinShimmerSkillNames() {
	return {
		"Pulsus Luminis", "Velum Radiāns", "Gradus Coruscus", "Lucerna Echo", "Fulgur Aurorae", "Flos Aeruginis",
		"Resonantia Chori", "Requiem Mechanicum", "Scutum Coruscum", "Nota Luminosa", "Revelatio Occulta",
		"Catena Aeruginis", "Onus Coruscum", "Lux Sanctorum", "Mutatio Horologii", "Phragmós Phōtós",
		"Ray of Mercy", "Prism Step", "Gleam Needle", "Therapeía Anakláseōs", "Anástasis Phōtós",
		"Phthorá Phōtós", "Phōteinḕ Kátharsis", "Phōteinòs Thṓrax", "Refractio Ultima", "Lumen Spiculum",
		"Lux Vortex", "Siphon Lux", "Lux Aegis", "Refraction Cataclysm", "Lux Ascension"
	};
}
