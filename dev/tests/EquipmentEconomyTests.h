#pragma once
#include "ContentCatalog.h"
#include "EquipmentStats.h"
#include "Player.h"
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace EquipmentEconomyValidation {
inline void SameStats(const Stats& a, const Stats& b) {
    for (auto field : EquipmentStats::Fields()) assert(a.*field == b.*field);
}

inline void CatalogueRolls() {
    assert(BossArtifactChance(69) == 0.0 && BossArtifactChance(70) == 1.0);
    assert(BossArtifactChance(75) == 1.5 && BossArtifactChance(80) == 2.0);
    assert(BossArtifactChance(90) == 3.5 && BossArtifactChance(100) == 5.0);
    assert(BossArtifactChance(1000) == 5.0);
    static_assert(!std::is_copy_constructible<Item>::value && !std::is_move_constructible<Item>::value,
        "Owned item instance IDs must not be copied");
    const auto catalogue = BuildEquippableCatalog();
    std::array<bool, 6> rarities{};
    std::array<bool, 5> slots{};
    bool independentMainAndSubstat = false;
    int checked = 0;
    for (const auto& definition : catalogue) {
        std::string error;
        assert(ValidateEquipmentDefinition(definition, &error));
        rarities[static_cast<int>(definition.rarity)] = true;
        slots[static_cast<int>(definition.slot)] = true;
        const auto first = RollLootInstance(definition, 17u);
        for (unsigned seed = 0; seed < 64; ++seed) {
            const auto roll = RollLootInstance(definition, seed);
            const auto replay = RollLootInstance(definition, seed);
            assert(ValidateEquipmentDefinition(roll, &error));
            SameStats(roll.stats, definition.stats);
            assert(roll.name == definition.name && roll.uniqueEffect == definition.uniqueEffect &&
                roll.setName == definition.setName && roll.passive == definition.passive &&
                roll.setPassive == definition.setPassive && roll.theme == definition.theme &&
                roll.forbiddenStats == definition.forbiddenStats);
            assert(roll.mainStatValue == replay.mainStatValue && roll.mainStatPercent == replay.mainStatPercent &&
                roll.substats == replay.substats);
            if (roll.mainStatValue == first.mainStatValue && roll.mainStatPercent == first.mainStatPercent &&
                roll.substats != first.substats) independentMainAndSubstat = true;
            ++checked;
        }
    }
    for (bool found : rarities) assert(found);
    for (bool found : slots) assert(found);
    assert(independentMainAndSubstat);

    const std::pair<int, int> core[] = {{8,12},{12,16},{16,22},{22,28},{28,34},{34,40}};
    const std::pair<int, int> spd[] = {{3,5},{5,8},{8,12},{12,16},{16,20},{20,25}};
    const std::pair<int, int> crit[] = {{2,4},{4,6},{6,9},{9,12},{12,15},{15,18}};
    const std::pair<int, int> critDamage[] = {{5,8},{8,12},{12,18},{18,25},{25,32},{32,40}};
    const std::pair<int, int> acc[] = {{5,8},{8,12},{12,18},{18,24},{24,30},{30,36}};
    const std::pair<int, int> potency[] = {{8,12},{12,18},{18,25},{25,32},{32,40},{40,50}};
    for (int tier = 0; tier < 6; ++tier) {
        const auto rarity = static_cast<Rarity>(tier);
        for (const auto* stat : {"hp", "atk", "def"}) assert(GetEquipmentMainStatRange(stat, rarity) == core[tier]);
        assert(GetEquipmentMainStatRange("spd", rarity) == spd[tier]);
        for (const auto* stat : {"crit_rate", "evasion"}) assert(GetEquipmentMainStatRange(stat, rarity) == crit[tier]);
        assert(GetEquipmentMainStatRange("crit_damage", rarity) == critDamage[tier]);
        for (const auto* stat : {"acc", "res"}) assert(GetEquipmentMainStatRange(stat, rarity) == acc[tier]);
        for (const auto* stat : {"healing", "barrier", "dot"}) assert(GetEquipmentMainStatRange(stat, rarity) == potency[tier]);
    }
    auto forbidden = catalogue.front();
    forbidden.forbiddenStats = {ItemStat::HP};
    assert(!ValidateEquipmentDefinition(forbidden));
    bool rejected = false;
    try { (void)RollLootInstance(forbidden, 9); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    for (const auto& item : catalogue) if (item.rarity == Rarity::Rare) {
        auto invalid = item;
        invalid.substats[1] = invalid.substats[0];
        assert(!ValidateEquipmentDefinition(invalid));
        invalid = item;
        invalid.mainStat = "unknown_stat";
        assert(!ValidateEquipmentDefinition(invalid));
        break;
    }
    std::cout << "Equipment rolls: " << checked << " validated across all catalogue items and rarities.\n";
}

inline ItemDefinition Fixture(EquipSlot slot, const char* main, int value) {
    ItemDefinition item;
    item.name = "authored stat fixture " + std::to_string(static_cast<int>(slot));
    item.category = ItemCategory::Equippable;
    item.slot = slot;
    item.mainStat = main;
    if (slot == EquipSlot::Head || slot == EquipSlot::Torso || slot == EquipSlot::Hand) item.mainStatPercent = value;
    else item.mainStatValue = value;
    return item;
}

inline void AggregationAndPreview() {
    Player player;
    assert(player.GetEquipped(static_cast<EquipSlot>(-1)) == nullptr);
    assert(player.GetEquipped(static_cast<EquipSlot>(99)) == nullptr);
    player.takeDamage(11);
    auto head = Fixture(EquipSlot::Head, "hp", 10);
    head.stats.hp = 20; head.stats.critRate = 4;
    head.substats = {{ItemStat::HP, 3}, {ItemStat::CritRate, 3}};
    auto torso = Fixture(EquipSlot::Torso, "def", 20);
    torso.stats.def = 7; torso.stats.atk = 5;
    torso.substats = {{ItemStat::HP, 7}, {ItemStat::DEF, 5}};
    auto hand = Fixture(EquipSlot::Hand, "atk", 10);
    hand.stats.atkPercent = 12; hand.stats.critDamage = 20;
    hand.substats = {{ItemStat::ATK, 8}};
    auto boots = Fixture(EquipSlot::Boots, "spd", 5);
    boots.stats.spd = 3; boots.stats.spdPercent = 10;
    boots.substats = {{ItemStat::SPD, 2}};
    auto accessory = Fixture(EquipSlot::Accessory, "acc", 6);
    accessory.stats.acc = 2;
    accessory.substats = {{ItemStat::Accuracy, 4}};
    for (const auto& definition : {head, torso, hand, boots, accessory}) {
        auto item = std::make_unique<Item>(definition);
        Item* selected = item.get();
        player.AddItem(std::move(item));
        const auto statsBefore = player.GetEffectiveStats();
        const auto preview = player.PreviewEquipment(*selected);
        SameStats(statsBefore, player.GetEffectiveStats());
        assert(player.getHP() == 89 && player.GetEquipped(definition.slot) == nullptr);
        assert(player.Equip(*selected));
        SameStats(preview, player.GetEffectiveStats());
        assert(player.getHP() == 89);
    }
    const auto stats = player.GetEffectiveStats();
    assert(stats.hp == 144 && stats.atk == 33 && stats.def == 21 && stats.spd == 121);
    assert(stats.acc == 12 && stats.critRate == 12 && stats.critDamage == 170);
    assert(player.getMaxHP() == stats.hp && player.getAttack() == stats.atk &&
        player.getDefense() == stats.def && player.getSpeed() == stats.spd &&
        player.getDisplayedAccuracy() == stats.acc && player.getDisplayedCritRate() == stats.critRate);
    player.ApplyStatus({StatusType::AttackUp, 3, 40});
    player.ApplyStatus({StatusType::DefenseUp, 3, 15});
    const auto buffed = player.GetEffectiveStats();
    assert(buffed.atk == 46 && buffed.def == 24);
    const Item* worn = player.GetEquipped(EquipSlot::Head);
    const auto withoutHead = player.PreviewUnequip(EquipSlot::Head);
    assert(player.getHP() == 89 && player.GetEquipped(EquipSlot::Head) == worn);
    assert(player.ToggleEquip(*player.GetInventory().FindItem(worn->getInstanceId())));
    SameStats(withoutHead, player.GetEffectiveStats());
    assert(player.getHP() == 89);
    const auto lines = Item(hand).GetStatLines();
    assert(std::find(lines.begin(), lines.end(), "Main: ATK +10%") != lines.end());
    assert(std::find(lines.begin(), lines.end(), "Sub: ATK +8%") != lines.end());
    assert(std::find(lines.begin(), lines.end(), "Fixed: ATK +12%") != lines.end());

    Player starter;
    for (const auto& definition : BuildEquippableCatalog()) if (definition.name == "cellar_bible") {
        starter.AddItem(std::make_unique<Item>(definition));
        assert(starter.Equip(*starter.FindItem(definition.name)));
        assert(starter.getAttack() == 22);
    }
    Inventory inventory;
    auto percent = Fixture(EquipSlot::Hand, "atk", 20);
    percent.name = "percentage";
    auto flat = Fixture(EquipSlot::Hand, "atk", 0);
    flat.name = "flat"; flat.stats.atk = 50;
    inventory.AddItem(std::make_unique<Item>(flat));
    inventory.AddItem(std::make_unique<Item>(percent));
    Stats reference;
    reference.atk = 400;
    const auto sorted = inventory.GetSortedItems(InventorySort::HighestStat, ItemStat::ATK, reference);
    assert(sorted[0]->getName() == "percentage" && sorted[0]->GetStatContribution(ItemStat::ATK, reference) == 80);
    reference.atk = 20;
    assert(inventory.GetSortedItems(InventorySort::HighestStat, ItemStat::ATK, reference)[0]->getName() == "flat");
}
}

inline void RunEquipmentEconomyTests() {
    EquipmentEconomyValidation::CatalogueRolls();
    EquipmentEconomyValidation::AggregationAndPreview();
    std::cout << "Equipment percentage, fixed bonus, preview, display and sorting regressions passed.\n";
}
