#pragma once

#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "EncounterManager.h"
#include "EnemyCatalog.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <string>

namespace LootEconomyValidation {

inline int GearCount(const std::vector<ItemDefinition>& loot) {
    int count = 0;
    for (const auto& item : loot) count += item.category == ItemCategory::Equippable;
    return count;
}

inline bool SameRoll(const ItemDefinition& first, const ItemDefinition& second) {
    return first.name == second.name && first.category == second.category && first.rarity == second.rarity
        && first.slot == second.slot && first.mainStat == second.mainStat
        && first.mainStatValue == second.mainStatValue && first.mainStatPercent == second.mainStatPercent
        && first.substats == second.substats && first.uniqueEffect == second.uniqueEffect;
}

inline Rarity MaximumRarity(int level, bool boss) {
    if (boss && level >= 70) return Rarity::Artifact;
    if (boss && level >= 40) return Rarity::Legendary;
    if (level >= 20) return Rarity::Epic;
    if (level >= 10) return Rarity::Rare;
    if (level >= 5) return Rarity::Uncommon;
    return Rarity::Common;
}

inline void CheckRarityGate(const ItemDefinition& item, int level, bool boss) {
    if (item.category != ItemCategory::Equippable) return;
    assert(item.slot != EquipSlot::None);
    assert(static_cast<int>(item.rarity) <= static_cast<int>(MaximumRarity(level, boss)));
    assert(item.rarity >= Rarity::Common);
}

inline void CheckFrequency(int count, int samples, double chance, double tolerance = 0.008) {
    assert(samples > 0);
    assert(std::abs(static_cast<double>(count) / samples - chance) < tolerance);
}

inline void StatisticalDropTable() {
    struct Configuration { EnemyTier tier; int level; int guaranteed; double extraChance; double consumableChance; };
    const Configuration configurations[] = {
        {EnemyTier::Basic, 1, 0, .85, .30},
        {EnemyTier::Basic, 15, 0, .85 + .15 * 14.0 / 29.0, .30},
        {EnemyTier::Basic, 30, 0, 1.0, .30},
        {EnemyTier::Rare, 30, 1, .20, .40},
        {EnemyTier::Epic, 30, 1, .50, .50},
        {EnemyTier::Elite, 30, 2, .35, .65},
        {EnemyTier::Boss, 70, 3, .50, 1.0},
        {EnemyTier::Boss, 80, 3, .50, 1.0},
        {EnemyTier::Boss, 100, 3, .50, 1.0}
    };
    constexpr int samples = 100000;
    std::mt19937 seeds(0x1007EC0u);
    bool observedIndependentDuplicateRoll = false;
    for (const auto& config : configurations) {
        LootContext context;
        context.playerLevel = config.level;
        context.enemyTier = config.tier;
        context.roomId = "Entrance Hall";
        int totalGear = 0, consumableVictories = 0, extraVictories = 0, extraAndConsumable = 0;
        std::array<int, 6> rarities{};
        for (int draw = 0; draw < samples; ++draw) {
            const unsigned seed = seeds();
            const auto loot = GenerateLoot(context, seed);
            int gear = 0, consumables = 0;
            for (const auto& item : loot) {
                assert(item.category == ItemCategory::Equippable || item.category == ItemCategory::Consumable);
                if (item.category == ItemCategory::Consumable) { ++consumables; continue; }
                ++gear;
                ++rarities[static_cast<std::size_t>(item.rarity)];
                CheckRarityGate(item, config.level, config.tier == EnemyTier::Boss);
            }
            assert(gear >= config.guaranteed && gear <= config.guaranteed + 1);
            assert(consumables <= 1);
            if (config.extraChance == 1.0) assert(gear == config.guaranteed + 1);
            if (config.consumableChance == 1.0) assert(consumables == 1);
            totalGear += gear;
            consumableVictories += consumables;
            const bool extra = gear > config.guaranteed;
            extraVictories += extra;
            extraAndConsumable += extra && consumables > 0;
            if (draw < 16) {
                const auto replay = GenerateLoot(context, seed);
                assert(loot.size() == replay.size());
                for (std::size_t item = 0; item < loot.size(); ++item) assert(SameRoll(loot[item], replay[item]));
            }
            if (!observedIndependentDuplicateRoll && gear > 1) {
                for (std::size_t first = 0; first < loot.size(); ++first)
                    for (std::size_t second = first + 1; second < loot.size(); ++second)
                        if (loot[first].category == ItemCategory::Equippable && loot[first].name == loot[second].name
                            && !SameRoll(loot[first], loot[second])) observedIndependentDuplicateRoll = true;
            }
        }
        CheckFrequency(extraVictories, samples, config.extraChance);
        CheckFrequency(consumableVictories, samples, config.consumableChance);
        // A reused decision roll can match both marginals while correlating the rewards.
        CheckFrequency(extraAndConsumable, samples, config.extraChance * config.consumableChance);
        if (config.tier == EnemyTier::Boss) {
            const double artifactChance = config.level == 70 ? .01 : config.level == 80 ? .02 : .05;
            CheckFrequency(rarities[static_cast<int>(Rarity::Artifact)], totalGear, artifactChance, .002);
            if (config.level == 100) {
                assert(rarities[static_cast<int>(Rarity::Common)] == 0 && rarities[static_cast<int>(Rarity::Uncommon)] == 0);
                CheckFrequency(rarities[static_cast<int>(Rarity::Rare)], totalGear, .05, .004);
                CheckFrequency(rarities[static_cast<int>(Rarity::Epic)], totalGear, .65, .004);
                CheckFrequency(rarities[static_cast<int>(Rarity::Legendary)], totalGear, .25, .004);
            }
        }
        std::cout << std::fixed << std::setprecision(3) << "LOOT tier=" << EnemyTierName(config.tier)
                  << " level=" << config.level << " seededVictories=" << samples << " meanGear=" << 1.0 * totalGear / samples
                  << " extraPercent=" << 100.0 * extraVictories / samples
                  << " consumablePercent=" << 100.0 * consumableVictories / samples
                  << " jointPercent=" << 100.0 * extraAndConsumable / samples
                  << " Rare=" << 100.0 * rarities[static_cast<int>(Rarity::Rare)] / totalGear
                  << " Epic=" << 100.0 * rarities[static_cast<int>(Rarity::Epic)] / totalGear
                  << " Legendary=" << 100.0 * rarities[static_cast<int>(Rarity::Legendary)] / totalGear
                  << " Artifact=" << 100.0 * rarities[static_cast<int>(Rarity::Artifact)] / totalGear << '\n';
    }
    assert(observedIndependentDuplicateRoll);
}

inline void RarityGatesAndFirstClearQuantities() {
    std::mt19937 seeds(0x6A7E5u);
    for (int level : {1, 4, 5, 9, 10, 19, 20, 39, 40, 69, 70, 100}) {
        for (bool boss : {false, true}) {
            LootContext context;
            context.playerLevel = level;
            context.enemyTier = boss ? EnemyTier::Boss : EnemyTier::Elite;
            bool sawMaximum = false;
            for (int draw = 0; draw < 2000; ++draw) {
                for (const auto& item : GenerateLoot(context, seeds())) {
                    CheckRarityGate(item, level, boss);
                    if (item.category == ItemCategory::Equippable && item.rarity == MaximumRarity(level, boss)) sawMaximum = true;
                }
                for (const auto& item : GenerateInvestigationLoot(ConsumableProfile::SupplyCache, level, "Storage Room", seeds(), true))
                    CheckRarityGate(item, level, false);
            }
            assert(sawMaximum);
        }
    }
    for (bool firstClear : {false, true}) {
        LootContext boss;
        boss.playerLevel = 8;
        boss.enemyTier = EnemyTier::Boss;
        boss.bossFirstClear = firstClear;
        boss.bossFarmEcho = !firstClear;
        bool sawThree = false, sawFour = false;
        for (unsigned seed = 0; seed < 256; ++seed) {
            const auto loot = GenerateLoot(boss, seed);
            const int gear = GearCount(loot);
            assert(gear == 3 || gear == 4);
            assert(static_cast<int>(loot.size()) == gear + 1);
            sawThree |= gear == 3;
            sawFour |= gear == 4;
        }
        assert(sawThree && sawFour);
    }
}

inline unsigned BasicMissSeed(int playerLevel) {
    LootContext context;
    context.playerLevel = playerLevel;
    context.enemyTier = EnemyTier::Basic;
    context.roomId = "Entrance Hall";
    context.enemyId = "Devotee Acolyte";
    for (unsigned seed = 0; seed < 100000; ++seed) if (GearCount(GenerateLoot(context, seed)) == 0) return seed;
    assert(false && "Low-level Basic reward table must include an ordinary gear miss");
    return 0;
}

inline std::unique_ptr<Enemy> RewardFixture(EnemyTier tier = EnemyTier::Basic) {
    const auto* acolyte = FindEnemyDefinition("Devotee Acolyte");
    assert(acolyte);
    EnemyDefinition definition = *acolyte;
    definition.tier = tier;
    // Zero fixture XP isolates pity from incidental level changes. Explicit
    // level-up tests below still use the public progression mutation route.
    return std::make_unique<Enemy>(definition, 1, false, false, 0);
}

inline void SessionBasicPity() {
    EncounterManager encounters;
    Player player;
    ProgressionManager progression;
    MapManager map;
    Area& room = map.GetRooms()[1];
    auto first = RewardFixture();
    first->takeDamage(first->getHP());
    const auto firstResult = encounters.ResolveVictory(room, player, progression, map, *first, BasicMissSeed(1));
    assert(firstResult.resolved && GearCount(firstResult.items) == 0 && encounters.GetBasicGearMisses() == 1);
    const auto countAfterFirst = player.GetInventory().GetItemCount();
    assert(!encounters.ResolveVictory(room, player, progression, map, *first, BasicMissSeed(1)).resolved);
    assert(player.GetInventory().GetItemCount() == countAfterFirst && encounters.GetBasicGearMisses() == 1);
    for (const auto tier : {EnemyTier::Rare, EnemyTier::Epic, EnemyTier::Elite, EnemyTier::Boss}) {
        auto other = RewardFixture(tier);
        other->takeDamage(other->getHP());
        assert(encounters.ResolveVictory(room, player, progression, map, *other, 71u).resolved);
        assert(encounters.GetBasicGearMisses() == 1);
    }
    auto living = RewardFixture();
    assert(!encounters.ResolveVictory(room, player, progression, map, *living, 1u).resolved);
    assert(encounters.GetBasicGearMisses() == 1);
    auto defeatedWhileDead = RewardFixture();
    defeatedWhileDead->takeDamage(defeatedWhileDead->getHP());
    player.takeDamage(player.getHP());
    assert(!encounters.ResolveVictory(room, player, progression, map, *defeatedWhileDead, 1u).resolved);
    assert(encounters.GetBasicGearMisses() == 1);
    player.RestoreHealth(player.getMaxHP());
    auto fled = RewardFixture();
    CombatManager combat;
    combat.BeginEncounter(player, *fled, progression.GetLevel());
    combat.PrepareTurn(player, *fled);
    assert(combat.IsPlayerTurn() && combat.resolve(CombatAction::Flee, player, *fled));
    fled->takeDamage(fled->getHP());
    assert(!encounters.ResolveVictory(room, player, progression, map, *fled, 1u).resolved);
    assert(encounters.GetBasicGearMisses() == 1);

    bool itemEvent = false, xpEvent = false;
    for (unsigned seed = 0; seed < 500 && (!itemEvent || !xpEvent); ++seed) {
        Area investigationRoom("Loot pity investigation fixture");
        investigationRoom.SetSpawnCondition(EnemySpawnCondition::None);
        player.RestoreHealth(player.getMaxHP());
        const auto investigation = encounters.ResolveInvestigation(investigationRoom, player, progression, seed);
        itemEvent |= investigation.type == InvestigationEventType::ItemDiscovery;
        xpEvent |= investigation.type == InvestigationEventType::ExplorationXP;
        assert(encounters.GetBasicGearMisses() == 1);
    }
    assert(itemEvent && xpEvent);
    if (progression.GetXP() < progression.GetXPToNextLevel())
        progression.GrantXP(progression.GetXPToNextLevel() - progression.GetXP());
    assert(progression.CheckLevelUp(player));
    assert(progression.GetLevel() >= 2 && encounters.GetBasicGearMisses() == 1);
    const auto missSeed = BasicMissSeed(progression.GetLevel());
    auto second = RewardFixture();
    second->takeDamage(second->getHP());
    const auto secondResult = encounters.ResolveVictory(room, player, progression, map, *second, missSeed);
    assert(secondResult.resolved && GearCount(secondResult.items) == 0 && encounters.GetBasicGearMisses() == 2);
    auto interveningRare = RewardFixture(EnemyTier::Rare);
    interveningRare->takeDamage(interveningRare->getHP());
    assert(encounters.ResolveVictory(room, player, progression, map, *interveningRare, 71u).resolved);
    assert(encounters.GetBasicGearMisses() == 2);
    auto third = RewardFixture();
    third->takeDamage(third->getHP());
    const auto thirdResult = encounters.ResolveVictory(room, player, progression, map, *third, missSeed);
    assert(thirdResult.resolved && GearCount(thirdResult.items) == 1 && encounters.GetBasicGearMisses() == 0);
    const auto afterPity = player.GetInventory().GetItemCount();
    assert(!encounters.ResolveVictory(room, player, progression, map, *third, missSeed).resolved);
    assert(player.GetInventory().GetItemCount() == afterPity && encounters.GetBasicGearMisses() == 0);
    auto next = RewardFixture();
    next->takeDamage(next->getHP());
    assert(GearCount(encounters.ResolveVictory(room, player, progression, map, *next, missSeed).items) == 0);
    assert(encounters.GetBasicGearMisses() == 1);
    EncounterManager separateSession;
    assert(separateSession.GetBasicGearMisses() == 0);
}

inline void FirstFountainGuarantee() {
    MapManager map;
    Area& fountain = map.GetRooms()[0];
    Player player;
    ProgressionManager progression;
    EncounterManager encounters;
    assert(!fountain.IsHealingUnlocked() && !map.GetRooms()[1].IsUnlocked());
    auto discovery = encounters.ResolveInvestigation(fountain, player, progression, 17u);
    assert(discovery.type == InvestigationEventType::FountainDiscovery && discovery.spawnedEnemy);
    fountain.addEnemy(std::move(discovery.spawnedEnemy));
    Enemy* guardian = fountain.getFirstAliveEnemy();
    assert(guardian);
    const auto rewardXP = guardian->getXPReward();
    guardian->takeDamage(guardian->getHP());
    const auto missSeed = BasicMissSeed(1);
    const auto firstClear = encounters.ResolveVictory(fountain, player, progression, map, *guardian, missSeed);
    assert(firstClear.resolved && GearCount(firstClear.items) == 1);
    for (const auto& item : firstClear.items)
        if (item.category == ItemCategory::Equippable) assert(item.rarity == Rarity::Common);
    assert(fountain.IsHealingUnlocked() && map.GetRooms()[1].IsUnlocked());
    assert(progression.GetXP() == rewardXP && progression.GetLevel() == 1);
    assert(encounters.GetBasicGearMisses() == 0);
    const auto inventoryCount = player.GetInventory().GetItemCount();
    assert(!encounters.ResolveVictory(fountain, player, progression, map, *guardian, missSeed).resolved);
    assert(player.GetInventory().GetItemCount() == inventoryCount && progression.GetXP() == rewardXP);
    fountain.removeDeadEnemies();
    auto later = RewardFixture();
    later->takeDamage(later->getHP());
    const auto repeat = encounters.ResolveVictory(fountain, player, progression, map, *later, missSeed);
    assert(repeat.resolved && GearCount(repeat.items) == 0);
    assert(encounters.GetBasicGearMisses() == 1 && fountain.IsHealingUnlocked() && map.GetRooms()[1].IsUnlocked());
}
}

inline void RunLootEconomyTests() {
    LootEconomyValidation::StatisticalDropTable();
    LootEconomyValidation::RarityGatesAndFirstClearQuantities();
    LootEconomyValidation::SessionBasicPity();
    LootEconomyValidation::FirstFountainGuarantee();
    std::cout << "Loot economy regressions passed.\n";
}
