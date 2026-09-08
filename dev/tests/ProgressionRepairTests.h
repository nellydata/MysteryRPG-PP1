#pragma once

#include "MapManager.h"
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>

namespace ProgressionRepairValidation {

inline void GiveStarterEquipment(Player& player) {
    player.SetLevelStats(1);
    for (const auto& definition : BuildEquippableCatalog()) {
        if (definition.name != "plain_rosary" && definition.name != "linen_veil") continue;
        player.AddItem(std::make_unique<Item>(definition));
        if (definition.name == "plain_rosary") player.Equip(*player.GetInventory().GetItems().back().item);
    }
    for (const auto& definition : BuildConsumableCatalog())
        if (definition.name == "cellar_rations" || definition.name == "purification_salts")
            player.AddItem(std::make_unique<Item>(definition));
    player.RestoreHealth(player.getMaxHP());
}

inline void ConfigureRoom(Area& room, bool repeat = false) {
    room.AddInteractable("Broken statue");
    room.AddInteractable("Old banner");
    room.SetClues(!repeat);
    room.SetItems(!repeat);
    room.SetSpawnCondition(EnemySpawnCondition::Low);
    if (repeat) {
        while (room.RevealNextInteractable()) {}
        room.SetBossDefeated(true);
    }
}

inline int ItemCount(const Player& player) {
    int count = 0;
    for (const auto& entry : player.GetInventory().GetItems()) count += entry.quantity;
    return count;
}

inline void AssertPercent(int observed, int samples, double expected, double tolerance = 0.8) {
    assert(std::abs(100.0 * observed / samples - expected) < tolerance);
}

inline void TestEnemyDistribution() {
    constexpr int samples = 100000;
    std::mt19937 seeds(0xC0FFEEu);
    std::cout << std::fixed << std::setprecision(3);
    int traceCount = 0;
    SetEnemySelectionTraceSink([&](const EnemySelectionTrace& trace) {
        assert(!trace.enemyId.empty() && !trace.source.empty() && trace.level >= 1);
        if (traceCount++ < 7)
            std::cout << "TRACE tier=" << EnemyTierName(trace.tier) << " enemy=" << trace.enemyId
                      << " level=" << trace.level << " source=" << trace.source << '\n';
    });
    for (const int level : {1, 5, 10, 20, 40, 70, 100}) {
        std::array<int, 4> counts{};
        for (int draw = 0; draw < samples; ++draw) {
            auto enemy = CreateRandomCatalogueEnemy(seeds(), level);
            assert(enemy && enemy->getTier() != EnemyTier::Boss);
            assert(FindEnemyDefinition(enemy->getName())->minimumLevel <= level);
            ++counts[static_cast<std::size_t>(enemy->getTier())];
        }
        const auto expected = GetEnemyTierWeights(level);
        std::cout << "TIERS level=" << level << " samples=" << samples;
        for (std::size_t tier = 0; tier < counts.size(); ++tier) {
            AssertPercent(counts[tier], samples, expected[tier]);
            std::cout << ' ' << EnemyTierName(static_cast<EnemyTier>(tier)) << '=' << 100.0 * counts[tier] / samples;
        }
        std::cout << '\n';
    }
    assert(traceCount == samples * 7);
    SetEnemySelectionTraceSink({});

    const auto low = GetEnemyTierWeights(5);
    const auto high = GetEnemyTierWeights(10);
    const auto middle = GetEnemyTierWeights(7);
    for (std::size_t tier = 0; tier < 4; ++tier)
        assert(std::abs(middle[tier] - (low[tier] + (high[tier] - low[tier]) * 0.4)) < 0.0001);

    RoomDefinition sparse;
    sparse.id = "Sparse eligibility fixture";
    sparse.enemiesBeforeClear = {{"Devotee Acolyte", 1}, {"Matron of the Catacombs", 100000}};
    for (int draw = 0; draw < 1000; ++draw) {
        auto enemy = CreateRoomEnemy(sparse, seeds(), 100, false);
        assert(enemy && enemy->getName() == "Devotee Acolyte");
        auto faction = CreateRandomCatalogueEnemy(seeds(), 5, EnemyFaction::Automata, EnemyTier::Epic);
        assert(faction && faction->getFaction() == EnemyFaction::Automata);
        assert(FindEnemyDefinition(faction->getName())->minimumLevel <= 5);
    }
    for (const auto& room : MapManager::GetFloor1Definitions()) {
        if (room.type == RoomType::Sanctuary) continue;
        for (int level : {1, 5, 10, 20}) {
            for (int draw = 0; draw < 100; ++draw) {
                auto enemy = CreateRoomEnemy(room, seeds(), level, false);
                if (!HasEligibleRoomEnemy(room, level, false)) { assert(!enemy); continue; }
                assert(enemy && enemy->getTier() != EnemyTier::Boss);
                bool listed = false;
                for (const auto& entry : room.enemiesBeforeClear) if (entry.enemyId == enemy->getName()) listed = true;
                assert(listed && FindEnemyDefinition(enemy->getName())->minimumLevel <= level);
            }
        }
    }
}

inline void TestInvestigationDistributionAndMutation() {
    constexpr int samples = 20000;
    EncounterManager encounters;
    std::mt19937 seeds(0x1A2B3C4Du);
    std::array<unsigned, 8> outcomeSeeds{};
    std::array<bool, 8> seen{};
    for (bool repeat : {false, true}) {
        std::array<int, 8> counts{};
        int consumables = 0, equipment = 0;
        for (int draw = 0; draw < samples; ++draw) {
            Area room("Entrance Hall");
            ConfigureRoom(room, repeat);
            Player player;
            player.takeDamage(20);
            ProgressionManager progression;
            const unsigned seed = seeds();
            const int beforeHP = player.getHP();
            auto result = encounters.ResolveInvestigation(room, player, progression, seed);
            assert(result.primaryResolved && result.type != InvestigationEventType::Quiet);
            const auto index = static_cast<std::size_t>(result.type);
            ++counts[index];
            if (!repeat) { outcomeSeeds[index] = seed; seen[index] = true; }
            assert(progression.GetXP() == result.xpAwarded);
            assert(player.getHP() - beforeHP == result.hpDelta);
            if (result.type == InvestigationEventType::EnemyEncounter) assert(result.spawnedEnemy);
            if (result.type == InvestigationEventType::ItemDiscovery) {
                assert(!result.items.empty() && ItemCount(player) == static_cast<int>(result.items.size()));
                for (const auto& item : result.items) {
                    assert(player.GetInventory().GetQuantity(item.name) >= 1);
                    if (item.category == ItemCategory::Consumable) ++consumables;
                    else { assert(item.category == ItemCategory::Equippable); ++equipment; }
                }
            }
            if (result.type == InvestigationEventType::ExplorationXP || result.type == InvestigationEventType::ClueDiscovery)
                assert(result.xpAwarded > 0);
        }
        const std::array<double, 8> expected = repeat
            ? std::array<double, 8>{0, 30, 42, 20, 0, 4, 4, 0}
            : std::array<double, 8>{0, 25, 35, 18, 12, 6, 4, 0};
        std::cout << "INVESTIGATION repeat=" << repeat << " samples=" << samples;
        for (std::size_t i = 0; i < counts.size(); ++i) {
            AssertPercent(counts[i], samples, expected[i], 1.25);
            std::cout << " event" << i << '=' << 100.0 * counts[i] / samples;
        }
        AssertPercent(consumables, consumables + equipment, repeat ? 80.0 : 70.0, 2.0);
        std::cout << " consumable=" << 100.0 * consumables / (consumables + equipment) << '\n';
    }

    for (int outcome = 1; outcome <= 6; ++outcome) {
        assert(seen[static_cast<std::size_t>(outcome)]);
        Area first("Entrance Hall"), second("Entrance Hall");
        ConfigureRoom(first); ConfigureRoom(second);
        Player a, b;
        a.takeDamage(20); b.takeDamage(20);
        ProgressionManager pa, pb;
        const auto ra = encounters.ResolveInvestigation(first, a, pa, outcomeSeeds[outcome]);
        const auto rb = encounters.ResolveInvestigation(second, b, pb, outcomeSeeds[outcome]);
        assert(ra.type == rb.type && static_cast<int>(ra.type) == outcome);
        assert(ra.narrative == rb.narrative && ra.enemyId == rb.enemyId);
        assert(ra.xpAwarded == rb.xpAwarded && pa.GetXP() == pb.GetXP());
        assert(ra.items.size() == rb.items.size());
        if (!ra.items.empty()) {
            assert(ra.items.front().name == rb.items.front().name);
            assert(ra.items.front().substats == rb.items.front().substats);
        }
    }

    Area pity("Entrance Hall");
    ConfigureRoom(pity);
    Player player;
    ProgressionManager progression;
    for (int failed = 0; failed < 4; ++failed) {
        auto result = encounters.ResolveInvestigation(pity, player, progression,
            outcomeSeeds[static_cast<int>(InvestigationEventType::ItemDiscovery)]);
        assert(result.type == InvestigationEventType::ItemDiscovery);
        assert(pity.GetDiscoveryFailures() == failed + 1);
    }
    auto forced = encounters.ResolveInvestigation(pity, player, progression,
        outcomeSeeds[static_cast<int>(InvestigationEventType::ItemDiscovery)]);
    assert(forced.type == InvestigationEventType::ClueDiscovery && pity.GetDiscoveryFailures() == 0);

    Area exhausted("Entrance Hall");
    ConfigureRoom(exhausted, true);
    int items = 0, xp = 0;
    for (int draw = 0; draw < 200; ++draw) {
        player.RestoreHealth(player.getMaxHP());
        auto result = encounters.ResolveInvestigation(exhausted, player, progression, seeds());
        items += result.type == InvestigationEventType::ItemDiscovery;
        xp += result.type == InvestigationEventType::ExplorationXP;
        assert(result.type != InvestigationEventType::Quiet && result.type != InvestigationEventType::ClueDiscovery);
    }
    assert(items > 50 && xp > 15);
}

struct FightMetrics { int actions = 0; int enemyActions = 0; int damageReceived = 0; int maximumHit = 0; bool victory = false; };

inline FightMetrics Fight(Player& player, Enemy& enemy, ProgressionManager* progression = nullptr) {
    FightMetrics metrics;
    CombatManager combat;
    combat.BeginEncounter(enemy);
    combat.PrepareTurn(player, enemy);
    for (int guard = 0; guard < 500 && player.isAlive() && enemy.isAlive(); ++guard) {
        const int before = player.getHP();
        if (combat.IsPlayerTurn()) { ++metrics.actions; assert(combat.resolve(CombatAction::Attack, player, enemy, nullptr, progression)); }
        else { ++metrics.enemyActions; combat.enemyTurn(player, enemy, progression); }
        const int damage = (std::max)(0, before - player.getHP());
        metrics.damageReceived += damage;
        metrics.maximumHit = (std::max)(metrics.maximumHit, damage);
    }
    assert(!player.isAlive() || !enemy.isAlive());
    metrics.victory = !enemy.isAlive();
    return metrics;
}

inline void TestEarlyFights() {
    for (int level : {1, 5, 10}) {
        for (const char* name : {"Devotee Acolyte", "Automata Unit-01", "Devotee Penitent", "Devotee Flagellant"}) {
            Player player;
            GiveStarterEquipment(player);
            player.SetLevelStats(level);
            player.RestoreHealth(player.getMaxHP());
            auto enemy = CreateCatalogueEnemy(name, level);
            std::cout << "FIGHT level=" << level << " playerHP=" << player.getMaxHP() << " playerATK=" << player.getAttack()
                      << " playerDEF=" << player.getDefense() << " enemy=" << name << " hp=" << enemy->getMaxHP()
                      << " atk=" << enemy->getAtk() << " def=" << enemy->getDefense() << " xp=" << enemy->getXPReward();
            const auto metrics = Fight(player, *enemy);
            std::cout << " actions=" << metrics.actions << " enemyActions=" << metrics.enemyActions
                      << " damage=" << metrics.damageReceived << " maxHit=" << metrics.maximumHit << " win=" << metrics.victory << '\n';
            assert(metrics.victory);
            if (enemy->getTier() == EnemyTier::Basic) assert(metrics.actions >= 2 && metrics.actions <= 4);
            if (level == 1 && enemy->getTier() == EnemyTier::Rare) assert(metrics.maximumHit < player.getMaxHP() / 2);
        }
    }
}

inline void TestFixedBossAndFirstClear() {
    MapManager map;
    EncounterManager encounters;
    const auto* definition = map.GetRoomDefinition("Entrance Hall");
    assert(definition && definition->firstBossSpawnChance == 50);
    auto reference = CreateFirstClearBoss(*definition);
    assert(reference && reference->IsFirstClearBoss() && !reference->IsFarmBoss());
    std::cout << "BOSS id=" << reference->getName() << " fixedLevel=" << reference->getLevel() << " hp=" << reference->getMaxHP()
              << " atk=" << reference->getAtk() << " def=" << reference->getDefense() << " speed=" << reference->getSpeed()
              << " firstXP=" << reference->getXPReward() << '\n';
    std::mt19937 seeds(0xB055u);
    for (int level : {1, 8, 20}) {
        Player player;
        ProgressionManager progression;
        while (progression.GetLevel() < level) { progression.GrantXP(progression.GetXPToNextLevel()); progression.CheckLevelUp(player); }
        bool found = false;
        for (int draw = 0; draw < 100 && !found; ++draw) {
            Area room("Entrance Hall");
            ConfigureRoom(room, true);
            room.SetBossDefeated(false);
            auto result = encounters.ResolveInvestigation(room, player, progression, seeds());
            if (!result.spawnedEnemy || !result.spawnedEnemy->IsFirstClearBoss()) continue;
            found = true;
            assert(result.spawnedEnemy->getLevel() == reference->getLevel());
            assert(result.spawnedEnemy->getMaxHP() == reference->getMaxHP());
            assert(result.spawnedEnemy->getAtk() == reference->getAtk());
            assert(result.spawnedEnemy->getDefense() == reference->getDefense());
            assert(result.spawnedEnemy->getSpeed() == reference->getSpeed());
        }
        assert(found);
    }
    int bosses = 0;
    for (int draw = 0; draw < 10000; ++draw) {
        Area room("Entrance Hall"); ConfigureRoom(room, true); room.SetBossDefeated(false);
        Player player; ProgressionManager progression;
        auto result = encounters.ResolveInvestigation(room, player, progression, seeds());
        bosses += result.spawnedEnemy && result.spawnedEnemy->IsFirstClearBoss();
    }
    AssertPercent(bosses, 10000, 50.0, 1.5);
    std::cout << "BOSS_ELIGIBILITY samples=10000 firstClearPercent=" << bosses / 100.0 << '\n';

    Area& entrance = map.GetRooms()[1];
    Area& archive = map.GetRooms()[2];
    Player player; ProgressionManager progression;
    assert(!encounters.ResolveVictory(entrance, player, progression, map, *reference).resolved);
    assert(!entrance.HasBossBeenDefeated() && !archive.IsUnlocked());
    player.takeDamage(player.getMaxHP());
    reference->takeDamage(reference->getHP());
    assert(!encounters.ResolveVictory(entrance, player, progression, map, *reference).resolved);
    assert(!entrance.HasBossBeenDefeated() && !archive.IsUnlocked());
    player.RestoreHealth(player.getMaxHP());
    auto retry = CreateFirstClearBoss(*definition);
    assert(retry && retry->getMaxHP() == reference->getMaxHP());
    retry->takeDamage(retry->getHP());
    const auto first = encounters.ResolveVictory(entrance, player, progression, map, *retry);
    assert(first.resolved && first.firstClear && entrance.HasBossBeenDefeated() && archive.IsUnlocked());
    const auto xpBefore = progression.GetXP();
    const auto levelBefore = progression.GetLevel();
    const int itemsBefore = ItemCount(player);
    assert(!encounters.ResolveVictory(entrance, player, progression, map, *retry).resolved);
    assert(progression.GetXP() == xpBefore && progression.GetLevel() == levelBefore && ItemCount(player) == itemsBefore);
    int farmHP1 = 0, farmHP20 = 0;
    for (int level : {1, 20}) {
        bool foundFarm = false;
        for (int draw = 0; draw < 2000 && !foundFarm; ++draw) {
            auto enemy = CreateRoomEnemy(*definition, seeds(), level, true);
            if (!enemy || !enemy->IsFarmBoss()) continue;
            foundFarm = true;
            assert(!enemy->IsFirstClearBoss() && enemy->getLevel() == level);
            if (level == 1) farmHP1 = enemy->getMaxHP(); else farmHP20 = enemy->getMaxHP();
            enemy->takeDamage(enemy->getHP());
            const auto farm = encounters.ResolveVictory(entrance, player, progression, map, *enemy);
            assert(farm.resolved && !farm.firstClear && farm.messages.empty());
        }
        assert(foundFarm);
    }
    assert(farmHP20 > farmHP1);
    std::cout << "FARM hpLevel1=" << farmHP1 << " hpLevel20=" << farmHP20 << '\n';
}

inline double EquipmentScore(const Item& item) {
    const auto& stats = item.getStats();
    return stats.atk * 4.0 + stats.atkPercent * 4.0 + stats.hp * 0.25 + stats.hpPercent
        + stats.def * 0.5 + stats.defPercent * 0.5 + stats.spd * 2.0 + stats.spdPercent * 2.0
        + stats.critRate + stats.critDamage * 0.3 + stats.acc * 0.3 + stats.evasion;
}

inline void EquipUpgrades(Player& player) {
    for (const auto& entry : player.GetInventory().GetItems()) {
        if (!entry->isEquippable() || entry->getSlot() == EquipSlot::None) continue;
        const Item* equipped = player.GetEquipped(entry->getSlot());
        if (!equipped || EquipmentScore(*entry.item) > EquipmentScore(*equipped)) player.Equip(*entry.item);
    }
}

inline void SimulateFreshProgression() {
    constexpr int runs = 32;
    struct Totals { double investigations = 0, battles = 0, wins = 0, itemEvents = 0, xpEvents = 0, defeats = 0, recoveries = 0, actions = 0, bossWithdrawals = 0; };
    std::array<Totals, 3> totals{};
    const int targets[] = {2, 5, 10};
    int readyBossVictories = 0;
    for (int run = 0; run < runs; ++run) {
        Player player; GiveStarterEquipment(player);
        ProgressionManager progression; EncounterManager encounters; MapManager map;
        Area& fountain = map.GetRooms()[0];
        Area& room = map.GetRooms()[1];
        std::mt19937 seeds(0xA11CEu + run);
        Totals counts;
        auto guardian = encounters.ResolveInvestigation(fountain, player, progression, seeds());
        std::cout << "DEBUG run=" << run << " guardian=" << (guardian.spawnedEnemy ? guardian.spawnedEnemy->getName() : "null")
                  << " playerHP=" << player.getHP() << "/" << player.getMaxHP() << " level=" << progression.GetLevel() << '\n';
        assert(guardian.spawnedEnemy);
        auto guardianFight = Fight(player, *guardian.spawnedEnemy, &progression);
        std::cout << "DEBUG run=" << run << " guardianFight victory=" << guardianFight.victory
                  << " actions=" << guardianFight.actions << " damage=" << guardianFight.damageReceived << '\n';
        assert(guardianFight.victory);
        encounters.ResolveVictory(fountain, player, progression, map, *guardian.spawnedEnemy);
        std::cout << "DEBUG run=" << run << " healingUnlocked=" << fountain.IsHealingUnlocked()
                  << " roomUnlocked=" << room.IsUnlocked() << " level=" << progression.GetLevel() << '\n';
        assert(fountain.IsHealingUnlocked() && room.IsUnlocked());
        ++counts.battles; ++counts.wins; counts.actions += guardianFight.actions;
        int target = 0;
        int loopGuard = 0;
        while (progression.GetLevel() < 10 && counts.investigations < 2000) {
            if (++loopGuard % 25 == 0)
                std::cout << "DEBUG run=" << run << " loop=" << loopGuard << " level=" << progression.GetLevel()
                          << " investigations=" << counts.investigations << " playerHP=" << player.getHP() << "/" << player.getMaxHP()
                          << " xp=" << progression.GetXP() << "/" << progression.GetXPToNextLevel()
                          << " defeats=" << counts.defeats << " itemEvents=" << counts.itemEvents << " xpEvents=" << counts.xpEvents << '\n';
            if (player.getHP() < player.getMaxHP() * 60 / 100) {
                player.ClearStatuses(); player.RestoreHealth(player.getMaxHP()); ++counts.recoveries;
            }
            auto result = encounters.ResolveInvestigation(room, player, progression, seeds());
            ++counts.investigations;
            counts.itemEvents += result.type == InvestigationEventType::ItemDiscovery;
            counts.xpEvents += result.xpAwarded > 0;
            if (result.spawnedEnemy) {
                if (result.spawnedEnemy->IsFirstClearBoss()) {
                    // Follow the actual flee action, including any faster boss opening.
                    CombatManager combat;
                    combat.BeginEncounter(*result.spawnedEnemy); combat.PrepareTurn(player, *result.spawnedEnemy);
                    while (player.isAlive() && !combat.IsPlayerTurn()) combat.enemyTurn(player, *result.spawnedEnemy);
                    if (player.isAlive()) {
                        std::cout << "DEBUG run=" << run << " bossFlee playerTurn=" << combat.IsPlayerTurn()
                                  << " playerHP=" << player.getHP() << " boss=" << result.spawnedEnemy->getName() << '\n';
                        assert(combat.resolve(CombatAction::Flee, player, *result.spawnedEnemy));
                    }
                    ++counts.bossWithdrawals;
                } else {
                    ++counts.battles;
                    const auto fight = Fight(player, *result.spawnedEnemy, &progression);
                    counts.actions += fight.actions;
                    if (fight.victory) { ++counts.wins; encounters.ResolveVictory(room, player, progression, map, *result.spawnedEnemy); }
                }
            }
            if (!player.isAlive()) {
                ++counts.defeats; ++counts.recoveries;
                player.ClearStatuses(); player.RestoreHealth(player.getMaxHP());
                assert(!room.HasBossBeenDefeated());
            }
            progression.CheckLevelUp(player);
            EquipUpgrades(player);
            while (progression.HasPendingShimmerChoice()) {
                if (progression.GetPendingShimmerChoices().empty()) progression.ResolvePendingShimmerChoice();
                else progression.SelectPendingShimmerSkill(progression.GetPendingShimmerChoices().front());
            }
            while (target < 3 && progression.GetLevel() >= targets[target]) {
                totals[target].investigations += counts.investigations;
                totals[target].battles += counts.battles;
                totals[target].wins += counts.wins;
                totals[target].itemEvents += counts.itemEvents;
                totals[target].xpEvents += counts.xpEvents;
                totals[target].defeats += counts.defeats;
                totals[target].recoveries += counts.recoveries;
                totals[target].actions += counts.actions;
                totals[target].bossWithdrawals += counts.bossWithdrawals;
                ++target;
            }
        }
        std::cout << "DEBUG run=" << run << " level=" << progression.GetLevel()
                  << " investigations=" << counts.investigations << " target=" << target
                  << " itemEvents=" << counts.itemEvents << " xpEvents=" << counts.xpEvents << '\n';
        assert(target == 3 && !room.HasBossBeenDefeated() && counts.itemEvents > 0 && counts.xpEvents > 0);
        player.ClearStatuses(); player.RestoreHealth(player.getMaxHP());
        auto boss = CreateFirstClearBoss(*map.GetRoomDefinition(room));
        const auto readyFight = Fight(player, *boss, &progression);
        if (readyFight.victory) {
            ++readyBossVictories;
            assert(encounters.ResolveVictory(room, player, progression, map, *boss).firstClear);
            assert(map.GetRooms()[2].IsUnlocked());
        }
    }
    for (int target = 0; target < 3; ++target) {
        const auto& t = totals[target];
        std::cout << "PROGRESSION runs=" << runs << " target=" << targets[target] << " investigations=" << t.investigations / runs
                  << " ordinaryBattles=" << t.battles / runs << " wins=" << t.wins / runs << " itemEvents=" << t.itemEvents / runs
                  << " xpEvents=" << t.xpEvents / runs << " defeats=" << t.defeats / runs << " fountainRecoveries=" << t.recoveries / runs
                  << " playerActions=" << t.actions / runs << " bossWithdrawals=" << t.bossWithdrawals / runs << '\n';
    }
    std::cout << "READY_BOSS level=10 victories=" << readyBossVictories << '/' << runs << '\n';
    assert(readyBossVictories >= runs * 3 / 4);
    assert(totals[0].investigations / runs < 40 && totals[2].investigations / runs < 500);
}

inline void Run() {
    TestEnemyDistribution();
    TestInvestigationDistributionAndMutation();
    TestEarlyFights();
    TestFixedBossAndFirstClear();
    SimulateFreshProgression();
    std::cout << "Progression repair validation passed.\n";
}
}
