#pragma once
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <cassert>
#include <iostream>
#include <cstdlib>

// Separate integration fixture: the fresh-console run never calls these APIs.
inline void RunStorageFarmingTests() {
    MapManager map;
    Area& storage = map.GetRooms().at(4);
    assert(storage.getName() == "Storage Room");
    if (storage.GetSpawnCondition() == EnemySpawnCondition::None) {
        std::cerr << "Storage regression FAILED: the configured ordinary/farming pool is disabled.\n";
        std::exit(EXIT_FAILURE);
    }
    storage.SetCleared(true);
    Player player("Storage regression");
    ProgressionManager progression;
    EncounterManager encounters;
    int enemies = 0, bosses = 0, items = 0, xp = 0, beneficial = 0, hazards = 0;
    constexpr int trials = 10000;
    for (unsigned seed = 0; seed < trials; ++seed) {
        player.RestoreHealth(player.getMaxHP());
        const auto result = encounters.ResolveInvestigation(storage, player, progression, seed);
        if (result.spawnedEnemy) {
            ++enemies;
            assert(!result.spawnedEnemy->IsFirstClearBoss());
            if (result.spawnedEnemy->getTier() == EnemyTier::Boss) {
                ++bosses;
                assert(result.spawnedEnemy->getName() == "The Reliquary Engine");
                assert(result.spawnedEnemy->IsFarmBoss());
            }
        } else if (result.type == InvestigationEventType::ItemDiscovery) ++items;
        else if (result.type == InvestigationEventType::ExplorationXP) ++xp;
        else if (result.type == InvestigationEventType::Beneficial) ++beneficial;
        else if (result.type == InvestigationEventType::Hazard) ++hazards;
        assert(result.primaryResolved);
    }
    assert(enemies > 2750 && enemies < 3250);
    assert(bosses > 170 && bosses < 320);
    assert(items > 3900 && items < 4500);
    assert(xp > 1750 && xp < 2250);
    assert(beneficial > 300 && beneficial < 500);
    assert(hazards > 300 && hazards < 500);
    const auto* definition = map.GetRoomDefinition(storage);
    assert(definition->firstBossSpawnChance == 50);
    assert(definition->farmBossStartingChance == 8);
    assert(definition->firstClearBossLevel == 30 && definition->firstClearBossXP == 1600);
    std::cout << "Storage farming passed: 10000 seeded real investigations; enemy=" << enemies
              << " farmBoss=" << bosses << " item=" << items << " XP=" << xp
              << " beneficial=" << beneficial << " hazard=" << hazards << '\n';
}
