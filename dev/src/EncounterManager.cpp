#include "EncounterManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
#include "ConsoleFormat.h"
#include "Enemy.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <random>
#include <stdexcept>

namespace {
}

EncounterManager::EncounterManager() = default;

InvestigationResult EncounterManager::ResolveInvestigation(Area& area, Player& player,
                                                           ProgressionManager& progression) {
    if (!player.isAlive() || area.hasEnemies()) {
        InvestigationResult result;
        result.narrative = "Resolve the current encounter before investigating.";
        return result;
    }
    return ResolveInvestigation(area, player, progression, area.NextInvestigationSeed());
}

InvestigationResult EncounterManager::ResolveInvestigation(Area& area, Player& player,
                                                           ProgressionManager& progression, unsigned seed) {
    InvestigationResult result;
    if (!player.isAlive() || area.hasEnemies()) {
        result.narrative = "Resolve the current encounter before investigating.";
        return result;
    }
    area.RecordSearch();
    std::mt19937 rng(seed);
    MapManager map;
    const RoomDefinition* definition = map.GetRoomDefinition(area);
    const int level = progression.GetLevel();
    if (area.IsHealingRoom() && !area.IsHealingUnlocked()) {
        result.type = InvestigationEventType::FountainDiscovery;
        result.enemyId = "Devotee Acolyte";
        result.spawnedEnemy = CreateCatalogueEnemy(result.enemyId, 1);
        result.narrative = "A weak guardian rises from the sealed Fountain. Defeat it to purify the sanctuary.";
        result.primaryResolved = result.spawnedEnemy != nullptr;
        return result;
    }

    const bool cluesEligible = area.HasClues() || !area.AreAllRequiredInteractablesRevealed();
    const bool repeat = !cluesEligible || area.HasBossBeenDefeated();
    const bool bossEligible = definition && !definition->bossId.empty() && !cluesEligible && !area.HasBossBeenDefeated();
    if (bossEligible && std::uniform_int_distribution<int>(0, 99)(rng) < definition->firstBossSpawnChance) {
        result.type = InvestigationEventType::EnemyEncounter;
        result.enemyId = definition->bossId;
        result.spawnedEnemy = CreateFirstClearBoss(*definition);
        result.narrative = "A major presence emerges: " + result.enemyId + ". Fixed level " +
            std::to_string(definition->firstClearBossLevel) + "; prepare around level " +
            std::to_string(definition->intendedProgressionLevel) + ". You can withdraw and continue investigating.";
        area.MarkBossEncountered();
        result.primaryResolved = result.spawnedEnemy != nullptr;
        return result;
    }

    // Eligibility is resolved before the draw; each event has exactly one handler.
    const bool enemiesEligible = area.GetSpawnCondition() != EnemySpawnCondition::None && definition &&
        HasEligibleRoomEnemy(*definition, level, area.HasBossBeenDefeated());
    const double weights[] = {enemiesEligible ? (repeat ? 30.0 : 25.0) : 0.0,
        repeat ? 42.0 : 35.0, repeat ? 20.0 : 18.0,
        cluesEligible ? 12.0 : 0.0, repeat ? 4.0 : 6.0, 4.0};
    const int choice = cluesEligible && area.GetDiscoveryFailures() >= 4 ? 3 :
        std::discrete_distribution<int>(std::begin(weights), std::end(weights))(rng);
    if (cluesEligible) area.RecordDiscoveryAttempt(choice == 3);
    result.primaryResolved = true;
    switch (choice) {
    case 0:
        result.type = InvestigationEventType::EnemyEncounter;
        result.spawnedEnemy = CreateRoomEnemy(*definition, rng(), level, area.HasBossBeenDefeated());
        // A room marked eligible should always fill its pool, but an empty or
        // exhausted pool must not crash the run: fall back to a safe default
        // guardian so the encounter still resolves with a valid enemy.
        if (!result.spawnedEnemy) result.spawnedEnemy = CreateCatalogueEnemy("Devotee Acolyte", level);
        if (!result.spawnedEnemy) throw std::logic_error("No eligible enemy could be constructed for an encounter");
        result.enemyId = result.spawnedEnemy->getName();
        result.narrative = "A hostile presence emerges: " + result.enemyId + " (" +
            EnemyTierName(result.spawnedEnemy->getTier()) + ", level " + std::to_string(result.spawnedEnemy->getLevel()) + ").";
        break;
    case 1:
        result.type = InvestigationEventType::ItemDiscovery;
        result.items = GenerateInvestigationLoot(definition ? definition->consumableProfile : ConsumableProfile::SupplyCache,
            level, area.getName(), rng(), repeat);
        if (result.items.empty()) throw std::logic_error("Item event produced no reward");
        for (const auto& item : result.items) player.AddItem(std::make_unique<Item>(item));
        // Finite cache accounting does not control repeatable rewards.
        area.ConsumeItem();
        result.narrative = "You discover supplies hidden in the room.";
        break;
    case 2:
        result.type = InvestigationEventType::ExplorationXP;
        result.xpAwarded = (std::max)(std::int64_t{1}, XPRequirementForLevel(level) *
            std::uniform_int_distribution<int>(8, 12)(rng) / 100);
        result.narrative = "You piece together a useful detail about the cathedral.";
        break;
    case 3: {
        const bool consumed = area.ConsumeClue();
        const bool revealed = area.RevealNextInteractable();
        if (!consumed && !revealed) throw std::logic_error("Ineligible discovery selected");
        result.type = InvestigationEventType::ClueDiscovery;
        result.discoveryId = area.getName() + ":discovery:" + std::to_string(area.GetSearchCount());
        result.xpAwarded = CalculateInvestigationXP(level, InvestigationAreaDepth(area.getName()));
        result.narrative = "You recover a meaningful fragment from the room.";
        if (!area.HasClues() && area.AreAllRequiredInteractablesRevealed()) {
            result.xpAwarded += XPRequirementForLevel(level) * 20 / 100;
            result.narrative += " The room's required discoveries are complete; its guardian may now appear.";
        }
        break;
    }
    case 4: {
        result.type = InvestigationEventType::Beneficial;
        const int before = player.getHP();
        player.heal((std::max)(1, player.getMaxHP() * 10 / 100));
        result.hpDelta = player.getHP() - before;
        result.narrative = "A clean restorative cache helps you recover " + std::to_string(result.hpDelta) + " HP.";
        break;
    }
    case 5: {
        result.type = InvestigationEventType::Hazard;
        const int before = player.getHP();
        player.takeDamage((std::max)(1, player.getMaxHP() * 5 / 100));
        result.hpDelta = player.getHP() - before;
        result.narrative = "Falling masonry catches you by surprise.";
        break;
    }
    }
    if (result.xpAwarded > 0) progression.GrantXP(result.xpAwarded);
    return result;
}

VictoryResult EncounterManager::ResolveVictory(Area& area, Player& player, ProgressionManager& progression,
                                               MapManager& map, Enemy& enemy, std::optional<unsigned> seed) {
    VictoryResult result;
    if (!player.isAlive()) {
        enemy.ForfeitRewards();
        return result;
    }
    if (!enemy.CanReceiveDefeatRewards() || enemy.HasClaimedLootReward()) return result;
    const RoomDefinition* room = map.GetRoomDefinition(area);
    const bool configuredBoss = room && enemy.getName() == room->bossId && enemy.IsFirstClearBoss();
    result.firstClear = configuredBoss && !area.HasBossBeenDefeated();
    const auto acceptedXP = CombatManager::DeliverDefeatXP(player, enemy, progression);
    if (acceptedXP > 0) result.messages.push_back("XP gained: +" + std::to_string(acceptedXP) + ".");

    LootContext context;
    context.playerLevel = progression.GetLevel();
    context.roomId = area.getName();
    context.enemyId = enemy.getName();
    context.faction = enemy.getFaction();
    context.enemyTier = enemy.getTier();
    context.bossFirstClear = result.firstClear;
    context.bossFarmEcho = enemy.IsFarmBoss();
    context.fountainFirstClear = area.IsHealingRoom() && !area.IsHealingUnlocked();
    context.guaranteedBasicGear = enemy.getTier() == EnemyTier::Basic && basicGearMisses_ >= 2;
    result.items = GenerateLoot(context, seed ? *seed : area.NextInvestigationSeed());
    std::vector<std::unique_ptr<Item>> pendingItems;
    pendingItems.reserve(result.items.size());
    for (const auto& item : result.items) pendingItems.push_back(std::make_unique<Item>(item));
    std::vector<unsigned long long> deliveredItems;
    deliveredItems.reserve(pendingItems.size());
    try {
        for (auto& item : pendingItems) {
            const auto instanceId = item->getInstanceId();
            player.AddItem(std::move(item));
            deliveredItems.push_back(instanceId);
        }
    } catch (...) {
        // Only this reward's newly inserted, unequipped items are rolled back.
        for (const auto instanceId : deliveredItems) player.GetInventory().RemoveItem(instanceId);
        throw;
    }
    // Empty loot is also a successfully resolved roll. Commit only after every
    // generated item arrived; first-clear flags and unlocks follow delivery.
    enemy.ClaimLootReward();
    if (enemy.getTier() == EnemyTier::Basic) {
        const bool gear = std::any_of(result.items.begin(), result.items.end(), [](const ItemDefinition& item) {
            return item.category == ItemCategory::Equippable;
        });
        basicGearMisses_ = gear ? 0 : (std::min)(2, basicGearMisses_ + 1);
    }
    result.resolved = true;
    bool unlockNext = false;
    if (area.IsHealingRoom() && !area.IsHealingUnlocked() && !area.hasEnemies()) {
        area.SetHealingUnlocked(true);
        unlockNext = true;
        result.messages.push_back("The Fountain is purified. Full healing is now available.");
    }
    if (result.firstClear) {
        area.SetBossDefeated(true);
        unlockNext = true;
        if (progression.UnlockShimmerFromTrigger(enemy.getName()))
            result.messages.push_back("Shimmer skill unlocked from " + enemy.getName() + ".");
        result.messages.push_back("First clear: " + enemy.getName() + ".");
    }
    if (unlockNext && room && !room->nextRoomId.empty()) {
        for (Area& next : map.GetRooms()) if (next.getName() == room->nextRoomId && !next.IsUnlocked()) {
            next.SetUnlocked(true);
            result.messages.push_back(room->nextRoomId + " is now unlocked.");
        }
    }
    return result;
}

void EncounterManager::BeginEncounter(Player& player, Enemy& enemy,
                                       ProgressionManager& progression, CombatManager& combat) {
    combat.BeginEncounter(player, enemy, progression.GetLevel());
    combat.PrepareTurn(player, enemy);
}

void EncounterManager::UseConsumable(Player& player, Enemy& enemy,
                                     ProgressionManager& progression, CombatManager& combat,
                                     ConsoleUI& ui) {
    std::vector<unsigned long long> consumables;
    for (const auto& entry : player.GetInventory().GetItems())
        if (entry->getCategory() == ItemCategory::Consumable && entry.quantity > 0)
            consumables.push_back(entry->getInstanceId());
    if (consumables.empty()) {
        ui.ShowMessage("No consumables available.");
        return;
    }

    constexpr std::size_t pageSize = 12;
    std::size_t page = 0;
    while (std::cin) {
        const std::size_t first = page * pageSize;
        const std::size_t count = (std::min)(pageSize, consumables.size() - first);
        ui.ShowMessage("Consumables page " + std::to_string(page + 1) + "/" +
            std::to_string((consumables.size() + pageSize - 1) / pageSize) +
            ". 13 next page, 14 previous page.");
        for (std::size_t offset = 0; offset < count; ++offset) {
            const auto instanceId = consumables[first + offset];
            ui.ShowMessage(std::to_string(offset + 1) + ". " +
                player.GetInventory().FindItem(instanceId)->getName() + " x" +
                std::to_string(player.GetInventory().GetQuantity(instanceId)) +
                (player.GetInventory().IsLocked(instanceId) ? " [LOCKED]" : ""));
        }
        const int selected = ui.Prompt("Choose consumable, 0 cancel:", 0, 14);
        if (!selected || !std::cin) return;
        if (selected == 13) {
            if (first + count < consumables.size()) ++page;
            else ui.ShowMessage("No next consumable page.");
            continue;
        }
        if (selected == 14) {
            if (page > 0) --page;
            continue;
        }
        if (selected > static_cast<int>(count)) {
            ui.ShowMessage("That consumable is not on this page.");
            continue;
        }
        combat.UseConsumable(consumables[first + selected - 1], player, enemy, &progression);
        return;
    }
}

void EncounterManager::UseShimmer(Player& player, Enemy& enemy,
                                  ProgressionManager& progression, CombatManager& combat,
                                  ConsoleUI& ui) {
    const auto equipped = progression.GetEquippedShimmerSkills();
    if (equipped.empty()) {
        ui.ShowMessage("Equip a Shimmer skill from the room's Skills menu first.");
        return;
    }
    for (std::size_t index = 0; index < equipped.size(); ++index)
    {
        const auto skill = ShimmerSkill::Create(equipped[index]);
        const int cooldown = combat.GetSkillCooldown(equipped[index]);
        std::cout << '[' << index + 1 << "] " << equipped[index]
                  << "  Cost " << (skill ? skill->getCost() : 0)
                  << "  [" << ConsoleFormat::CooldownText(cooldown) << "]\n";
        if (skill) std::cout << "    " << skill->getDescription() << '\n';
    }
    const int selected = ui.Prompt("Choose combat Shimmer, 0 cancel:", 0,
                                   static_cast<int>(equipped.size()));
    if (!selected) return;
    auto skill = ShimmerSkill::Create(equipped[selected - 1]);
    if (!skill) { ui.ShowMessage(ConsoleFormat::Error("That Shimmer definition is unavailable.")); return; }
    if (combat.GetSkillCooldown(skill->getName()) > 0) {
        ui.ShowMessage(ConsoleFormat::Warning(skill->getName() + " is still on cooldown."));
        return;
    }
    combat.resolve(CombatAction::Shimmer, player, enemy, skill.get(), &progression);
}

bool EncounterManager::HandlePlayerTurn(Player& player, Enemy& enemy,
                                        ProgressionManager& progression, CombatManager& combat,
                                        ConsoleUI& ui) {
    const int choice = ui.Prompt("Combat action:", 1, 6);
    if (!std::cin) return false;
    if (choice == 5) {
        ui.ShowBattleHistory(combat);
    } else if (choice == 6) {
        ConsoleUI::SetFast(!ConsoleUI::IsFast());
        ui.ShowMessage(ConsoleUI::IsFast() ? "Fast presentation enabled." : "Normal presentation enabled.");
    } else if (choice == 4) {
        combat.resolve(CombatAction::Flee, player, enemy);
        ui.PresentEvents(combat);
        return true;
    } else if (choice == 2) {
        UseConsumable(player, enemy, progression, combat, ui);
    } else if (choice == 3) {
        UseShimmer(player, enemy, progression, combat, ui);
    } else {
        combat.resolve(CombatAction::Attack, player, enemy, nullptr, &progression);
    }
    return false;
}

void EncounterManager::CompleteVictory(Area& area, Player& player, Enemy& enemy,
                                       ProgressionManager& progression, CombatManager& combat,
                                       MapManager& map, ConsoleUI& ui) {
    const auto victory = ResolveVictory(area, player, progression, map, enemy);
    for (const auto& message : victory.messages) combat.RecordReward(message);
    for (const auto& item : victory.items) combat.RecordReward("Found: " + item.name + ".");
    combat.CompleteEncounter("victory");
    ui.PresentEvents(combat);
    CleanupEncounter(area, player, combat, false);
    area.removeDeadEnemies();

    const auto* room = map.GetRoomDefinition(area);
    if (room && room->nextRoomId.empty() && area.HasBossBeenDefeated())
        ui.ShowMessage("Floor I cleared. The cathedral falls silent.");
    ui.PresentLevelUpRewards(progression);
}

void EncounterManager::ProcessInvestigation(Area& area, Player& player,
                                            ProgressionManager& progression, CombatManager& combat,
                                            ConsoleUI& ui) {
    auto result = ResolveInvestigation(area, player, progression);
    ui.ShowMessage(result.narrative);
    if (result.xpAwarded > 0)
        ui.ShowMessage("Investigation gained " + std::to_string(result.xpAwarded) + " XP.");
    for (const auto& item : result.items) ui.ShowMessage("Found: " + item.name + ".");
    if (result.hpDelta < 0)
        ui.ShowMessage("Investigation damage: " + std::to_string(-result.hpDelta) + " HP.");
    if (result.spawnedEnemy) area.addEnemy(std::move(result.spawnedEnemy));
    if (!player.isAlive()) combat.RecordEnvironmentalDefeat("Falling masonry", player);
    progression.CheckLevelUp(player);
    if (player.isAlive()) ui.PresentLevelUpRewards(progression);
}

void EncounterManager::CleanupEncounter(Area& area, Player& player, CombatManager& combat,
                                        bool clearEnemies) {
    combat.ResetEncounter();
    player.ClearStatuses();
    if (clearEnemies) area.ClearEnemies();
}

void EncounterManager::HandleDefeat(Area& area, Player& player, CombatManager& combat,
                                    ConsoleUI& ui) {
    combat.CompleteEncounter("defeat");
    ui.ShowDefeat(player, combat);
    // Keep the completed event snapshot; reset only temporary combat state after acknowledgment.
    CleanupEncounter(area, player, combat, true);
}

bool EncounterManager::Explore(Area& area, Player& player, ProgressionManager& progression,
                               CombatManager& combat, MapManager* map) {
    ConsoleUI ui;
    MapManager fallbackMap;
    MapManager& activeMap = map ? *map : fallbackMap;
    Enemy* encounterEnemy = nullptr;
    while (std::cin && player.isAlive()) {
        Enemy* enemy = area.getFirstAliveEnemy();
        if (enemy) {
            if (encounterEnemy != enemy) {
                BeginEncounter(player, *enemy, progression, combat);
                encounterEnemy = enemy;
            }
            ui.PresentEvents(combat);
            if (!player.isAlive()) break;
            ui.RenderCombat(player, progression, *enemy, combat);
            if (!combat.IsPlayerTurn()) {
                combat.enemyTurn(player, *enemy, &progression);
            } else if (HandlePlayerTurn(player, *enemy, progression, combat, ui)) {
                CleanupEncounter(area, player, combat, true);
                encounterEnemy = nullptr;
                continue;
            }
            ui.PresentEvents(combat);
            if (!player.isAlive()) break;
            if (!enemy->isAlive()) {
                CompleteVictory(area, player, *enemy, progression, combat, activeMap, ui);
                encounterEnemy = nullptr;
            }
            continue;
        }
        ui.RenderRoom(area, player, progression);
        const int maximum = area.IsHealingRoom() && area.IsHealingUnlocked() ? 7 : 6;
        const int choice = ui.Prompt("Room action:", 0, maximum);
        if (!std::cin || choice == 0) return true;
        if (choice == 3) { ui.ManageInventory(player, nullptr, &area); continue; }
        if (choice == 4) { ui.ShowMap(activeMap.GetRooms()); continue; }
        if (choice == 5) { ui.ManageShimmerSkills(progression); continue; }
        if (choice == 6) { ui.ShowBattleHistory(combat); continue; }
        if (choice == 7) {
            player.RestoreHealth(player.getMaxHP());
            ui.ShowMessage("The Fountain restores you to full health.");
            continue;
        }
        ProcessInvestigation(area, player, progression, combat, ui);
    }
    if (!player.isAlive()) HandleDefeat(area, player, combat, ui);
    else CleanupEncounter(area, player, combat, true);
    return false;
}

bool EncounterManager::Explore(Area& area, Player& player, ProgressionManager& progression) {
    CombatManager combat;
    return Explore(area, player, progression, combat);
}

bool EncounterManager::explore(Area& area, Player& player, CombatManager& combat, ProgressionManager& progression) {
    return Explore(area, player, progression, combat);
}
