#include "EncounterManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
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

namespace {
}

EncounterManager::EncounterManager() = default;

InvestigationResult EncounterManager::ResolveInvestigation(Area& area, Player& player,
                                                           ProgressionManager& progression) {
    InvestigationResult result;
    if (!player.isAlive() || area.hasEnemies()) {
        result.narrative = area.hasEnemies() ? "Combat is still unresolved." : "You cannot investigate while incapacitated.";
        return result;
    }

    area.RecordSearch();
    const unsigned seed = area.NextInvestigationSeed();
    const int roll = static_cast<int>(seed % 100);
    MapManager map;
    const RoomDefinition* definition = map.GetRoomDefinition(area);
    const int level = progression.GetLevel();

    if (area.getName() == "Fountain" && !area.IsHealingUnlocked()) {
        result.type = InvestigationEventType::FountainDiscovery;
        result.narrative = "A weak guardian rises from the sealed Fountain. Defeat it to purify the sanctuary.";
        result.enemyId = "Devotee Acolyte";
        result.spawnedEnemy = CreateCatalogueEnemy(result.enemyId, (std::max)(1, level));
        result.primaryResolved = result.spawnedEnemy != nullptr;
        return result;
    }

    const bool requiredDiscovery = !area.AreAllRequiredInteractablesRevealed() || area.HasClues();
    if (requiredDiscovery && area.GetSearchCount() >= 5) {
        if (area.ConsumeClue()) {
            result.type = InvestigationEventType::ClueDiscovery;
            result.discoveryId = area.getName() + ":clue:" + std::to_string(area.GetSearchCount());
            result.xpAwarded = CalculateInvestigationXP(level, InvestigationAreaDepth(area.getName()));
            progression.GrantXP(result.xpAwarded);
            area.RevealNextInteractable();
            result.narrative = "You recover a meaningful fragment from the room.";
            result.primaryResolved = true;
            return result;
        }
        if (area.RevealNextInteractable()) {
            result.type = InvestigationEventType::ClueDiscovery;
            result.discoveryId = area.GetVisibleInteractables().back();
            result.narrative = "A previously hidden interaction is revealed.";
            result.primaryResolved = true;
            return result;
        }
    }

    const bool bossEligible = definition != nullptr && !definition->bossId.empty() &&
        area.AreAllRequiredInteractablesRevealed() && area.HasAllClues();
    if (bossEligible && !area.HasBossBeenDefeated() && roll < definition->firstBossSpawnChance) {
        area.MarkBossEncountered();
        result.type = InvestigationEventType::EnemyEncounter;
        result.enemyId = definition->bossId;
        result.spawnedEnemy = CreateCatalogueEnemy(result.enemyId, (std::max)(1, level));
        result.narrative = "A major presence emerges from the room.";
        result.primaryResolved = result.spawnedEnemy != nullptr;
        return result;
    }

    const bool repeat = area.AreAllRequiredInteractablesRevealed() && !area.HasClues();
    const int enemyWeight = repeat ? 30 : 25;
    const int itemWeight = repeat ? 42 : 35;
    const int xpWeight = repeat ? 20 : 18;
    const int clueWeight = repeat ? 0 : 12;
    const int specialWeight = repeat ? 4 : 6;
    const int hazardWeight = 4;
    const bool enemiesEligible = area.GetSpawnCondition() != EnemySpawnCondition::None;
    const bool itemsEligible = area.HasItems();
    const bool cluesEligible = area.HasClues() || !area.AreAllRequiredInteractablesRevealed();
    const int total = (enemiesEligible ? enemyWeight : 0) + (itemsEligible ? itemWeight : 0) +
        xpWeight + (cluesEligible ? clueWeight : 0) + specialWeight + hazardWeight;
    int choice = static_cast<int>((seed / 17) % static_cast<unsigned>(total));

    if (enemiesEligible && (choice -= enemyWeight) < 0) {
        result.type = InvestigationEventType::EnemyEncounter;
        result.narrative = "A hostile presence emerges from the darkness.";
        const EnemyFaction faction = area.getName() == "Archive Room" ? EnemyFaction::Automata :
            area.getName() == "Chapel" ? EnemyFaction::Verdigris : EnemyFaction::Devotee;
        const EnemyTier tier = level >= 20 ? EnemyTier::Elite : level >= 6 ? EnemyTier::Epic : EnemyTier::Rare;
        result.spawnedEnemy = CreateRandomCatalogueEnemy(seed, (std::max)(1, level), faction, tier);
        result.primaryResolved = result.spawnedEnemy != nullptr;
        return result;
    }
    if (itemsEligible && (choice -= itemWeight) < 0) {
        result.type = InvestigationEventType::ItemDiscovery;
        area.ConsumeItem();
        result.items = GenerateInvestigationLoot(definition == nullptr ? ConsumableProfile::SupplyCache : definition->consumableProfile,
            level, area.getName(), seed);
        result.narrative = result.items.empty() ? "You uncover an empty compartment." : "You discover supplies hidden in the room.";
        if (result.items.empty()) {
            result.type = InvestigationEventType::ExplorationXP;
            result.xpAwarded = (std::max)(std::int64_t{1}, XPRequirementForLevel(level) * 5 / 100);
            progression.GrantXP(result.xpAwarded);
            result.narrative = "The cache is empty, but the search reveals a useful detail.";
        }
        result.primaryResolved = true;
        return result;
    }
    if ((choice -= xpWeight) < 0) {
        result.type = InvestigationEventType::ExplorationXP;
        const std::int64_t requirement = XPRequirementForLevel(level);
        result.xpAwarded = (std::max)(std::int64_t{1}, requirement * (5 + static_cast<int>((seed / 29) % 6)) / 100);
        progression.GrantXP(result.xpAwarded);
        result.narrative = "You piece together a useful detail about the cathedral.";
        result.primaryResolved = true;
        return result;
    }
    if (cluesEligible && (choice -= clueWeight) < 0 && area.ConsumeClue()) {
        result.type = InvestigationEventType::ClueDiscovery;
        result.discoveryId = area.getName() + ":clue:" + std::to_string(area.GetSearchCount());
        result.xpAwarded = CalculateInvestigationXP(level, InvestigationAreaDepth(area.getName()));
        progression.GrantXP(result.xpAwarded);
        area.RevealNextInteractable();
        result.narrative = "A clue is recovered from the room's details.";
        result.primaryResolved = true;
        return result;
    }
    if (!area.AreAllRequiredInteractablesRevealed() && area.RevealNextInteractable()) {
        result.type = InvestigationEventType::ClueDiscovery;
        result.discoveryId = area.getName() + ":object:" + std::to_string(area.GetVisibleInteractables().size() - 1);
        result.narrative = "A previously hidden interaction is revealed.";
        result.primaryResolved = true;
        return result;
    }
    if ((choice -= specialWeight) < 0) {
        result.type = InvestigationEventType::Beneficial;
        player.heal((std::max)(1, player.getMaxHP() * 10 / 100));
        result.hpDelta = player.getMaxHP() * 10 / 100;
        result.narrative = "A clean restorative cache helps you recover.";
        result.primaryResolved = true;
        return result;
    }

    result.type = InvestigationEventType::Hazard;
    const int before = player.getHP();
    player.takeDamage((std::max)(1, player.getMaxHP() * 5 / 100));
    result.hpDelta = player.getHP() - before;
    result.narrative = "Falling masonry catches you by surprise.";
    result.primaryResolved = true;
    return result;
}

bool EncounterManager::Explore(Area& area, Player& player,
                               ProgressionManager& progression,
                               CombatManager& combat) {
    ConsoleUI ui;
    if (area.GetVisibleInteractables().empty()) area.RevealNextInteractable();

    while (player.isAlive()) {
        Enemy* enemy = area.getFirstAliveEnemy();
        if (enemy != nullptr) {
            combat.BeginEncounter(*enemy);
            combat.PrepareTurn(player, *enemy);
            ui.RenderCombat(player, progression, *enemy, combat);
            const int choice = ui.Prompt("Choose a combat action.", 1, 4);

            if (choice == 4) {
                combat.resolve(CombatAction::Flee, player, *enemy);
                return false;
            }

            if (choice == 2) {
                std::vector<std::string> consumables;
                for (const auto& item : player.GetInventory().GetItems()) {
                    if (item->getCategory() != ItemCategory::Consumable) continue;
                    if (player.GetInventory().GetQuantity(item->getName()) <= 0) continue;
                    bool duplicate = false;
                    for (const auto& name : consumables) {
                        if (name == item->getName()) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) consumables.push_back(item->getName());
                }
                if (consumables.empty()) {
                    ui.ShowMessage("No consumables are available.");
                    continue;
                }
                std::cout << "\nConsumables:\n";
                for (std::size_t i = 0; i < consumables.size(); ++i)
                    std::cout << "  " << i + 1 << ". " << consumables[i] << "\n";
                const int itemChoice = ui.Prompt("Choose a consumable, or 0 to cancel.", 0,
                                                 static_cast<int>(consumables.size()));
                if (itemChoice == 0) continue;
                if (!combat.UseConsumable(consumables[static_cast<std::size_t>(itemChoice - 1)], player, *enemy, &progression))
                    ui.ShowMessage("That consumable cannot be used right now.");
                continue;
            }

            if (choice == 3) {
                const auto equipped = progression.GetEquippedShimmerSkills();
                if (equipped.empty()) {
                    ui.ShowMessage("No Shimmer skills are equipped.");
                    continue;
                }
                std::cout << "\nEquipped Shimmer skills:\n";
                for (std::size_t i = 0; i < equipped.size(); ++i)
                    std::cout << "  " << i + 1 << ". " << equipped[i] << "\n";
                const int skillChoice = ui.Prompt("Choose a skill, or 0 to cancel.", 0,
                                                 static_cast<int>(equipped.size()));
                if (skillChoice == 0) continue;
                auto selectedSkill = ShimmerSkill::Create(equipped[static_cast<std::size_t>(skillChoice - 1)]);
                if (!selectedSkill || !combat.resolve(CombatAction::Shimmer, player, *enemy, selectedSkill.get(), &progression))
                    ui.ShowMessage("That Shimmer skill cannot be used right now.");
                continue;
            }

            CombatAction action;
            switch (choice) {
            case 1: action = CombatAction::Attack; break;
            case 3: continue;
            default:
                std::cout << "That is not a combat action.\n";
                continue;
            }

            if (!combat.resolve(action, player, *enemy, nullptr, &progression)) continue;
            if (!enemy->isAlive()) {
                area.removeDeadEnemies();
            } else {
                while (!combat.IsPlayerTurn() && player.isAlive() && enemy->isAlive())
                {
                    combat.enemyTurn(player, *enemy, &progression);
                    ui.RenderCombat(player, progression, *enemy, combat);
                    if (!player.isAlive()) break;
                }
            }
            for (const auto& message : progression.ConsumeLevelUpMessages()) ui.ShowMessage(message);
            while (progression.HasPendingShimmerChoice()) {
                if (!ui.ChoosePendingShimmerSkill(progression)) break;
            }
            continue;
        }

        ui.RenderRoom(area, player, progression);
        const int maximumChoice = area.getName() == "Fountain" && area.IsHealingUnlocked() ? 5 : 4;
        const int choice = ui.Prompt("Choose an exploration action.", 0, maximumChoice);
        switch (choice) {
        case 1:
        case 2: {
            InvestigationResult result = ResolveInvestigation(area, player, progression);
            ui.ShowMessage(result.narrative);
            if (result.xpAwarded > 0) ui.ShowMessage("Investigation gained " + std::to_string(result.xpAwarded) + " XP.");
            for (const auto& item : result.items) {
                player.AddItem(std::make_unique<Item>(item));
                ui.ShowMessage("Found: " + item.name + ".");
            }
            if (result.hpDelta < 0) ui.ShowMessage("You lose " + std::to_string(-result.hpDelta) + " HP. Current HP: " +
                std::to_string(player.getHP()) + "/" + std::to_string(player.getMaxHP()) + ".");
            if (result.spawnedEnemy != nullptr) {
                area.addEnemy(std::move(result.spawnedEnemy));
                ui.ShowMessage("Combat begins.");
            }
            progression.CheckLevelUp(player);
            break;
        }
        case 3:
            ui.ShowInventory(player);
            ui.Prompt("Press 0 to return.", 0, 0);
            break;
        case 4:
            ui.ShowMessage("The floor map is etched into the cathedral ledger. Return to the room selection to view it.");
            break;
        case 5:
            player.RestoreHealth(player.getMaxHP());
            ui.ShowMessage("The Fountain restores Adelaide to full health.");
            break;
        case 0: return true;
        default: ui.ShowMessage("That action is not available."); break;
        }
    }

    return false;
}

bool EncounterManager::Explore(Area& area, Player& player,
                               ProgressionManager& progression) {
    CombatManager combat;
    return Explore(area, player, progression, combat);
}

bool EncounterManager::explore(Area& area, Player& player,
                               CombatManager& combat,
                               ProgressionManager& progression) {
    return Explore(area, player, progression, combat);
}
