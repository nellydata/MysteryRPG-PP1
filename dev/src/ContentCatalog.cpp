#include "ContentCatalog.h"
#include <algorithm>
#include <random>
#include "CombatMath.h"
#include "EnemyCatalog.h"
#include "MapManager.h"

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

int PercentMainStatValue(Rarity rarity) {
	switch (rarity) {
	case Rarity::Common: return 5;
	case Rarity::Uncommon: return 10;
	case Rarity::Rare: return 15;
	case Rarity::Epic: return 20;
	case Rarity::Legendary: return 25;
	case Rarity::Artifact: return 30;
	}
	return 5;
}

int MainHPValue(Rarity rarity) {
	switch (rarity) {
	case Rarity::Common: return 1000;
	case Rarity::Uncommon: return 1500;
	case Rarity::Rare: return 2000;
	case Rarity::Epic: return 3000;
	case Rarity::Legendary: return 4000;
	case Rarity::Artifact: return 5000;
	}
	return 1000;
}

int MainDEFValue(Rarity rarity) {
	switch (rarity) {
	case Rarity::Common: return 75;
	case Rarity::Uncommon: return 100;
	case Rarity::Rare: return 150;
	case Rarity::Epic: return 200;
	case Rarity::Legendary: return 250;
	case Rarity::Artifact: return 300;
	}
	return 75;
}

int MainStatValue(Rarity rarity, int value) {
	(void)value;
	switch (rarity) {
	case Rarity::Common: return 100;
	case Rarity::Uncommon: return 150;
	case Rarity::Rare: return 200;
	case Rarity::Epic: return 300;
	case Rarity::Legendary: return 400;
	case Rarity::Artifact: return 500;
	}
	return 100;
}

int SubstatCount(Rarity rarity) {
	switch (rarity) {
	case Rarity::Uncommon: return 1;
	case Rarity::Rare: return 1;
	case Rarity::Epic: return 3;
	case Rarity::Legendary: return 4;
	case Rarity::Artifact: return 4;
	default: return 0;
	}
}

void AddSubstat(ItemDefinition& item, ItemStat stat, int value) {
	item.substats.emplace_back(stat, value);
}

int SpeedMainStatValue(Rarity rarity, int value) {
	(void)value;
	switch (rarity) {
	case Rarity::Common: return 5;
	case Rarity::Uncommon: return 40;
	case Rarity::Rare: return 60;
	case Rarity::Epic: return 80;
	case Rarity::Legendary: return 100;
	case Rarity::Artifact: return 120;
	}
	return 20;
}

void AddEquippables(std::vector<ItemDefinition>& result, EquipSlot slot, const char* const* names,
					int count, Rarity rarity, const std::string& mainStat, int value) {
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
		const int mainStatValue = slot == EquipSlot::Boots ? SpeedMainStatValue(rarity, value)
			: resolvedMainStat == "hp" ? MainHPValue(rarity)
			: resolvedMainStat == "def" ? MainDEFValue(rarity)
			: MainStatValue(rarity, value);
		if (slot == EquipSlot::Boots) {
			item.mainStatValue = mainStatValue;
			item.stats.spd = mainStatValue;
		}

		else if (resolvedMainStat == "acc" || resolvedMainStat == "res") item.mainStatPercent = PercentMainStatValue(rarity);
		else if (resolvedMainStat == "hp") item.mainStatPercent = static_cast<int>(rarity) * 5 + 5;
		else item.mainStatValue = mainStatValue;
		const int substatCount = SubstatCount(rarity);
		for (int roll = 0; roll < substatCount && !item.allowedSubstats.empty(); ++roll) {
			const ItemStat stat = item.allowedSubstats[static_cast<std::size_t>((i + roll * 3) % item.allowedSubstats.size())];
			AddSubstat(item, stat, CombatMath::SubstatRoll(stat, rarity,
				static_cast<unsigned>(i * 17 + roll * 31 + static_cast<int>(rarity) * 13)));
		}
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
		result.push_back(item);
	}
}
}

bool ValidateMicareContent(std::string* error) {
	if (!ValidateContentCatalog(error)) return false;
	if (!ValidateEnemyCatalogue(error)) return false;
	for (const auto& room : MapManager::GetFloor1Definitions()) {
		if (room.id.empty() || room.bossId.empty() || room.firstBossSpawnChance != 50) {
			if (error) *error = "Room profile has invalid boss progression: " + room.id;
			return false;
		}
		if (FindEnemyDefinition(room.bossId) == nullptr) {
			if (error) *error = "Room boss is missing from enemy catalogue: " + room.bossId;
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
		if (definition.effect.find("restore_") == 0) {
			const int percent = (std::min)(80, 20 + (std::max)(0, playerLevel - 1) / 4);
			definition.effect = "restore_" + std::to_string(percent) + "_percent_hp";
			definition.description = "Restore " + std::to_string(percent) + " percent HP.";
		}
		return definition;
	}
	return {};
}

std::vector<ItemDefinition> GenerateLoot(const LootContext& context, unsigned seed) {
	std::vector<ItemDefinition> result;
	const bool boss = context.enemyTier == EnemyTier::Boss;
	const bool elite = context.enemyTier == EnemyTier::Elite;
	const int level = (std::max)(1, context.playerLevel);
	const int count = boss ? (context.bossFirstClear ? 4 : 3) : elite ? 2 : level >= 20 ? 2 : 1;
	for (int drop = 0; drop < count; ++drop) {
		const unsigned roll = seed + static_cast<unsigned>(drop * 41);
		const int dropChance = boss || elite ? 100 : (std::min)(90, 15 + level);
		if (static_cast<int>(roll % 100) >= dropChance) continue;
		const int rarityRoll = static_cast<int>((roll / 7) % 100);
		Rarity rarity = Rarity::Common;
		if (boss) rarity = rarityRoll < (context.bossFirstClear ? 35 : 20) ? Rarity::Artifact : rarityRoll < 70 ? Rarity::Legendary : rarityRoll < 92 ? Rarity::Epic : Rarity::Rare;
		else if (elite) rarity = rarityRoll < 20 ? Rarity::Epic : rarityRoll < 70 ? Rarity::Epic : Rarity::Rare;
		else rarity = rarityRoll < (level / 5) ? Rarity::Epic : rarityRoll < 35 ? Rarity::Rare : rarityRoll < 75 ? Rarity::Uncommon : Rarity::Common;
		std::vector<ItemDefinition> candidates;
		for (const auto& item : BuildEquippableCatalog()) if (item.rarity == rarity) candidates.push_back(item);
		if (!candidates.empty()) result.push_back(RollLootInstance(candidates[(roll / 13) % candidates.size()], roll + 97));
	}
	return result;
}

ItemDefinition RollLootInstance(ItemDefinition item, unsigned seed) {
	const int variation = 90 + static_cast<int>(seed % 21);
	if (item.mainStatValue > 0) item.mainStatValue = (std::max)(1, item.mainStatValue * variation / 100);
	if (item.mainStatPercent > 0) item.mainStatPercent = (std::max)(1, item.mainStatPercent * variation / 100);
	item.stats.hp = item.stats.hp * variation / 100;
	item.stats.def = item.stats.def * variation / 100;
	item.stats.spd = item.stats.spd * variation / 100;
	item.stats.atk = item.stats.atk * variation / 100;
	item.substats.clear();
	for (std::size_t index = 0; index < item.allowedSubstats.size() && index < static_cast<std::size_t>(SubstatCount(item.rarity)); ++index) {
		const ItemStat stat = item.allowedSubstats[(index + seed) % item.allowedSubstats.size()];
		AddSubstat(item, stat, CombatMath::SubstatRoll(stat, item.rarity, seed + static_cast<unsigned>(index * 37)));
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
		item.stats = {};
		item.mainStatValue = 0;
		item.mainStatPercent = 0;
		item.substats.clear();
		item.mainStat = artifactMainStat;
		const int artifactMain = artifactMainStat == "spd" ? SpeedMainStatValue(Rarity::Artifact, 0)
			: artifactMainStat == "def" ? MainDEFValue(Rarity::Artifact)
			: artifactMainStat == "hp" ? PercentMainStatValue(Rarity::Artifact) : MainStatValue(Rarity::Artifact, 0);
		if (artifactMainStat == "hp") item.mainStatPercent = artifactMain;
		else {
			item.mainStatValue = artifactMain;
			if (artifactMainStat == "def") item.stats.def = artifactMain;
			else if (artifactMainStat == "spd") item.stats.spd = artifactMain;
			else item.stats.atk = artifactMain;
		}
		for (int roll = 0; roll < SubstatCount(Rarity::Artifact); ++roll) {
			if (!item.allowedSubstats.empty()) {
				const ItemStat stat = item.allowedSubstats[static_cast<std::size_t>(roll % item.allowedSubstats.size())];
				AddSubstat(item, stat, CombatMath::SubstatRoll(stat, Rarity::Artifact,
					static_cast<unsigned>(roll * 31 + static_cast<int>(item.slot) * 17)));
			}
		}
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
	return {"Pulsus Luminis", "Velum Radiāns", "Gradus Coruscus", "Lucerna Echo", "Fulgur Aurorae", "Flos Aeruginis", "Resonantia Chori", "Requiem Mechanicum", "Scutum Coruscum", "Nota Luminosa", "Revelatio Occulta", "Catena Aeruginis", "Onus Coruscum", "Lux Sanctorum", "Mutatio Horologii"};
}
