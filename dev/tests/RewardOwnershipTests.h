#pragma once

#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "EncounterManager.h"
#include "Enemy.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace RewardOwnershipValidation {

inline EnemyDefinition TargetDefinition() {
    EnemyDefinition definition;
    definition.name = "Reward ownership target";
    definition.hpBase = 12;
    definition.atkBase = 1;
    definition.defBase = 0;
    definition.speed = 40;
    return definition;
}

inline std::size_t XPEventCount(const CombatManager& combat) {
    std::size_t count = 0;
    for (const auto& event : combat.GetEvents())
        if (event.kind == CombatManager::CombatEvent::Kind::Reward && event.message.find("XP gained:") == 0) ++count;
    return count;
}

inline void TestKillPath(int path, bool combatDeliversXP) {
    Player player;
    ProgressionManager progression;
    CombatManager combat;
    EncounterManager encounters;
    MapManager map;
    Area area("Entrance Hall");
    auto definition = TargetDefinition();
    if (path == 2) definition.speed = 300; // DOT victim genuinely owns the opening turn.
    Enemy enemy(definition, 1);
    combat.BeginEncounter(player, enemy, 1);
    combat.PrepareTurn(player, enemy);
    auto* recipient = combatDeliversXP ? &progression : nullptr;
    if (path == 0) {
        assert(combat.resolve(CombatAction::Attack, player, enemy, nullptr, recipient));
    } else if (path == 1) {
        auto skill = ShimmerSkill::Create("Gleam Needle");
        assert(skill && combat.resolve(CombatAction::Shimmer, player, enemy, skill.get(), recipient));
    } else if (path == 2) {
        assert(!combat.IsPlayerTurn());
        assert(enemy.ApplyStatus({StatusType::Burn, 2, 0, 100}));
        combat.enemyTurn(player, enemy, recipient);
    } else {
        for (const auto& item : BuildConsumableCatalog())
            if (item.name == "mechanical_charge") player.AddItem(std::make_unique<Item>(item));
        assert(player.GetInventory().GetQuantity("mechanical_charge") == 1);
        assert(combat.UseConsumable("mechanical_charge", player, enemy, recipient));
        assert(player.GetInventory().GetQuantity("mechanical_charge") == 0);
    }
    assert(!enemy.isAlive() && player.isAlive());
    assert(progression.GetXP() == (combatDeliversXP ? enemy.getXPReward() : 0));
    assert(enemy.HasClaimedXPReward() == combatDeliversXP);
    assert(!enemy.HasClaimedLootReward());
    assert(XPEventCount(combat) == (combatDeliversXP ? 1u : 0u));

    // Shared resolution remains valid for an enemy supplied directly by tests,
    // as well as one owned by Area. The fallback emits the sole missing XP event.
    const auto victory = encounters.ResolveVictory(area, player, progression, map, enemy);
    assert(victory.resolved);
    for (const auto& message : victory.messages) combat.RecordReward(message);
    assert(progression.GetXP() == enemy.getXPReward());
    assert(enemy.HasClaimedXPReward() && enemy.HasClaimedLootReward());
    assert(XPEventCount(combat) == 1);
    const auto itemCount = player.GetInventory().GetItemCount();
    const auto eventCount = combat.GetEvents().size();
    assert(!encounters.ResolveVictory(area, player, progression, map, enemy).resolved);
    assert(CombatManager::DeliverDefeatXP(player, enemy, progression) == 0);
    combat.enemyTurn(player, enemy, &progression);
    assert(progression.GetXP() == enemy.getXPReward());
    assert(player.GetInventory().GetItemCount() == itemCount);
    assert(combat.GetEvents().size() == eventCount);
}

inline void TestUnresolvedAndDeadPlayer() {
    Player player;
    ProgressionManager progression;
    EncounterManager encounters;
    MapManager map;
    Area area("Entrance Hall");
    Enemy enemy(TargetDefinition(), 1);
    assert(CombatManager::DeliverDefeatXP(player, enemy, progression) == 0);
    assert(!encounters.ResolveVictory(area, player, progression, map, enemy).resolved);
    assert(!enemy.HasClaimedXPReward() && !enemy.HasClaimedLootReward());
    assert(player.GetInventory().GetItemCount() == 0 && progression.GetXP() == 0);
    enemy.takeDamage(enemy.getHP());
    player.takeDamage(player.getHP());
    assert(CombatManager::DeliverDefeatXP(player, enemy, progression) == 0);
    assert(!encounters.ResolveVictory(area, player, progression, map, enemy).resolved);
    player.RestoreHealth(player.getMaxHP());
    assert(!enemy.CanReceiveDefeatRewards());
    assert(!encounters.ResolveVictory(area, player, progression, map, enemy).resolved);
    assert(!enemy.HasClaimedXPReward() && !enemy.HasClaimedLootReward());
    assert(progression.GetXP() == 0 && player.GetInventory().GetItemCount() == 0);

    Enemy fresh(TargetDefinition(), 1);
    fresh.takeDamage(fresh.getHP());
    const auto victory = encounters.ResolveVictory(area, player, progression, map, fresh);
    assert(victory.resolved && fresh.HasClaimedXPReward());
    assert(progression.GetXP() == fresh.getXPReward());
}

inline void TestFledBossAndRetry() {
    Player player;
    ProgressionManager progression;
    CombatManager combat;
    EncounterManager encounters;
    MapManager map;
    Area& area = map.GetRooms()[1];
    Area& nextRoom = map.GetRooms()[2];
    const auto* room = map.GetRoomDefinition(area);
    assert(room && !nextRoom.IsUnlocked());
    auto boss = CreateFirstClearBoss(*room);
    combat.BeginEncounter(player, *boss, 1);
    combat.PrepareTurn(player, *boss);
    if (!combat.IsPlayerTurn()) combat.enemyTurn(player, *boss, &progression);
    assert(player.isAlive() && combat.IsPlayerTurn());
    assert(combat.resolve(CombatAction::Flee, player, *boss));
    assert(!combat.resolve(CombatAction::Attack, player, *boss, nullptr, &progression));
    const auto health = player.getHP();
    combat.enemyTurn(player, *boss, &progression);
    assert(player.getHP() == health);
    boss->takeDamage(boss->getHP());
    assert(!encounters.ResolveVictory(area, player, progression, map, *boss).resolved);
    combat.ResetEncounter();
    combat.BeginEncounter(player, *boss, 1); // Re-entering the same instance cannot restore rewards.
    assert(CombatManager::DeliverDefeatXP(player, *boss, progression) == 0);
    assert(!encounters.ResolveVictory(area, player, progression, map, *boss).resolved);
    assert(!area.HasBossBeenDefeated() && !nextRoom.IsUnlocked());
    assert(progression.GetXP() == 0 && player.GetInventory().GetItemCount() == 0);

    auto retry = CreateFirstClearBoss(*room);
    retry->takeDamage(retry->getHP());
    const auto victory = encounters.ResolveVictory(area, player, progression, map, *retry);
    assert(victory.resolved && victory.firstClear);
    assert(area.HasBossBeenDefeated() && nextRoom.IsUnlocked());
    const auto xp = progression.GetXP();
    const auto level = progression.GetLevel();
    const auto items = player.GetInventory().GetItemCount();
    assert(!encounters.ResolveVictory(area, player, progression, map, *retry).resolved);
    assert(progression.GetXP() == xp && progression.GetLevel() == level);
    assert(player.GetInventory().GetItemCount() == items);
}

inline void TestDefeatAndLevelCap() {
    Player player;
    ProgressionManager progression;
    CombatManager combat;
    Enemy enemy(TargetDefinition(), 1);
    combat.BeginEncounter(player, enemy, 1);
    player.takeDamage(player.getHP());
    combat.CompleteEncounter("defeat");
    combat.ResetEncounter();
    player.RestoreHealth(player.getMaxHP());
    enemy.takeDamage(enemy.getHP());
    assert(CombatManager::DeliverDefeatXP(player, enemy, progression) == 0);
    assert(!enemy.HasClaimedXPReward());

    progression.SetMaxLevel(player);
    Enemy capped(TargetDefinition(), 1);
    capped.takeDamage(capped.getHP());
    const auto xp = progression.GetXP();
    assert(CombatManager::DeliverDefeatXP(player, capped, progression) == 0);
    assert(capped.HasClaimedXPReward());
    assert(progression.GetXP() == xp && progression.GetLevel() == 100);
}

} // namespace RewardOwnershipValidation

inline void RunRewardOwnershipTests() {
    for (int path = 0; path < 4; ++path) {
        RewardOwnershipValidation::TestKillPath(path, true);
        RewardOwnershipValidation::TestKillPath(path, false);
    }
    RewardOwnershipValidation::TestUnresolvedAndDeadPlayer();
    RewardOwnershipValidation::TestFledBossAndRetry();
    RewardOwnershipValidation::TestDefeatAndLevelCap();
    std::cout << "Reward ownership tests passed: eight combat/fallback kill paths, duplicate guards, defeat/flee/retry and level cap.\n";
}
