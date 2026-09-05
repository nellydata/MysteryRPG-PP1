#include "ContentCatalog.h"

namespace {
void SetSlotRules(ItemDefinition& item, EquipSlot slot, const std::string& mainStat) {
	item.mainStat = mainStat;
	switch (slot) {
	case EquipSlot::Head:
		item.theme = "insight, clarity, protection, perception";
		item.allowedSubstats = {ItemStat::CritRate, ItemStat::CritDamage, ItemStat::Resistance, ItemStat::Accuracy};
		item.forbiddenStats = {ItemStat::ATK, ItemStat::SPD, ItemStat::DotPotency, ItemStat::Evasion};
		break;
	case EquipSlot::Torso:
		item.theme = "protection, stability, purity, endurance";
		item.allowedSubstats = {ItemStat::Resistance, ItemStat::BarrierPotency, ItemStat::HealingPotency, ItemStat::DebuffDurationReduction};
		item.forbiddenStats = {ItemStat::ATK, ItemStat::CritRate, ItemStat::CritDamage, ItemStat::Accuracy, ItemStat::DotPotency, ItemStat::Evasion, ItemStat::SPD};
		break;
	case EquipSlot::Hand:
		item.theme = "offense, aggression, precision, execution";
		item.allowedSubstats = {ItemStat::CritRate, ItemStat::CritDamage, ItemStat::Accuracy, ItemStat::DotPotency, ItemStat::Lifesteal};
		item.forbiddenStats = {ItemStat::DEF, ItemStat::HP, ItemStat::Resistance, ItemStat::BarrierPotency, ItemStat::HealingPotency, ItemStat::SPD, ItemStat::Evasion};
		break;
	case EquipSlot::Boots:
		item.theme = "movement, agility, positioning";
		item.allowedSubstats = {ItemStat::Evasion, ItemStat::Accuracy, ItemStat::ATK, ItemStat::DEF};
		item.forbiddenStats = {ItemStat::CritRate, ItemStat::CritDamage, ItemStat::DotPotency, ItemStat::HealingPotency, ItemStat::BarrierPotency, ItemStat::Resistance, ItemStat::HP};
		break;
	case EquipSlot::Accessory:
		item.theme = "spiritual power, amplification, utility";
		item.allowedSubstats = {ItemStat::CritRate, ItemStat::CritDamage, ItemStat::Accuracy, ItemStat::Resistance, ItemStat::HealingPotency, ItemStat::BarrierPotency, ItemStat::DotPotency};
		item.forbiddenStats = {ItemStat::SPD, ItemStat::Evasion};
		break;
	default: break;
	}
}

void AddEquippables(std::vector<ItemDefinition>& result, EquipSlot slot, const char* const* names,
					int count, Rarity rarity, const std::string& mainStat, int value) {
	for (int i = 0; i < count; ++i) {
		ItemDefinition item;
		item.name = names[i];
		item.description = "Micare " + mainStat + " equipment.";
		item.category = ItemCategory::Equippable;
		item.rarity = rarity;
		item.slot = slot;
		item.mainStat = mainStat;
		if (mainStat == "hp") item.stats.hp = value;
		else if (mainStat == "def") item.stats.def = value;
		else if (mainStat == "spd") item.stats.spd = value;
		else if (mainStat == "atk") item.stats.atk = value;
		SetSlotRules(item, slot, mainStat);
		result.push_back(item);
	}
}
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
	const char* handCommon[] = {"plain_rosary","cellar_bible","worn_crucible","simple_ritual_staff","dusty_prayer_book","basic_choir_tome","old_ritual_blade","faded_sanctum_relic","thin_silk_gloves","plain_vestment_staff"};
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
	const char* accessoryCommon[] = {"worn_rosary", "tarnished_iron_ring", "cracked_automaton_charm", "old_choir_pendant", "brass_prayer_token"};
	const char* accessoryUncommon[] = {"blessed_rosary", "reinforced_iron_ring", "polished_automaton_charm", "choir_ember_pendant"};
	const char* accessoryRare[] = {"ceremonial_rosary_of_grace", "ironbound_ring", "automaton_combat_charm", "choir_flame_pendant"};
	const char* accessoryEpic[] = {"rosary_of_benediction", "ironheart_ring", "automaton_strike_charm", "choir_inferno_pendant"};
	const char* accessoryLegendary[] = {"choir_verdigris_rosary", "shimmer_ascendant_ring", "automaton_command_charm"};
	AddEquippables(result, EquipSlot::Accessory, accessoryCommon, 5, Rarity::Common, "hp", 5);
	AddEquippables(result, EquipSlot::Accessory, accessoryUncommon, 4, Rarity::Uncommon, "hp", 10);
	AddEquippables(result, EquipSlot::Accessory, accessoryRare, 4, Rarity::Rare, "hp", 15);
	AddEquippables(result, EquipSlot::Accessory, accessoryEpic, 4, Rarity::Epic, "hp", 20);
	AddEquippables(result, EquipSlot::Accessory, accessoryLegendary, 3, Rarity::Legendary, "hp", 25);
	for (auto& item : result) {
		if (item.rarity == Rarity::Legendary) {
			if (item.name.find("choir") != std::string::npos) item.setName = "Choir of Verdigris";
			else if (item.name.find("shimmer") != std::string::npos) item.setName = "Shimmer Ascendant";
			else if (item.name.find("automaton") != std::string::npos || item.name.find("clockwork") != std::string::npos) item.setName = "Automaton Overlord";
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
	for (auto& item : result) {
		if (item.rarity != Rarity::Artifact) continue;
		SetSlotRules(item, item.slot, item.slot == EquipSlot::Head ? "hp" :
			item.slot == EquipSlot::Torso ? "def" :
			item.slot == EquipSlot::Boots ? "spd" : "atk");
		if (item.name.find("verdigris") != std::string::npos) item.setName = "Verdigris Matron";
		else if (item.name.find("choir") != std::string::npos) item.setName = "Shimmer Reliquary";
		else if (item.name.find("first_light") != std::string::npos) item.setName = "Oracle Automaton";
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
