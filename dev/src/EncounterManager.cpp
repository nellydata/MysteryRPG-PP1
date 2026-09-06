#include "EncounterManager.h"
#include "Area.h"
#include "Automata.h"
#include "Boss.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
#include "Enemy.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Nun.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <iostream>
#include <memory>

namespace {
unsigned RoomSeed(const std::string& roomName) {
    unsigned seed = 2166136261u;
    for (unsigned char character : roomName) {
        seed ^= character;
        seed *= 16777619u;
    }
    return seed;
}

int SpawnChance(EnemySpawnCondition condition) {
    switch (condition) {
    case EnemySpawnCondition::None: return 0;
    case EnemySpawnCondition::Low: return 20;
    case EnemySpawnCondition::Medium: return 50;
    case EnemySpawnCondition::High: return 80;
    }
    return 0;
}

void SpawnEnemy(Area& area, unsigned seed) {
    const int level = 1 + static_cast<int>((seed / 3) % 5);
    MapManager map;
    const RoomDefinition* definition = map.GetRoomDefinition(area);
    if (definition != nullptr) {
        if (auto enemy = CreateRoomEnemy(*definition, seed, level, area.HasBossBeenDefeated())) area.addEnemy(std::move(enemy));
    } else if (auto enemy = CreateRandomCatalogueEnemy(seed, level, false, false))
        area.addEnemy(std::move(enemy));
}

void InspectEnvironment(Area& area, Player& player, ProgressionManager& progression,
                        ConsoleUI& ui) {
    const auto& interactables = area.GetVisibleInteractables();
    std::cout << "\nObjects:\n";
    for (std::size_t index = 0; index < interactables.size(); ++index)
        std::cout << "  " << index + 1 << ". " << interactables[index] << "\n";
    const int choice = ui.Prompt("Choose an object to inspect, or 0 to return.", 0,
                                 static_cast<int>(interactables.size()));
    if (choice == 0) return;
    if (choice < 1 || choice > static_cast<int>(interactables.size())) {
        ui.ShowMessage("That is not a valid interactable.");
        return;
    }

    const std::string& object = interactables[choice - 1];
    std::cout << "You inspect the " << object << ".\n";
    area.RecordSearch();
    if (object == "Figure in the shadows" && area.HasNPC()) {
        ui.ShowMessage("You see an NPC here. Interaction system coming soon.");
    }
    if (area.ConsumeClue()) {
        ui.ShowMessage("You uncover a clue hidden in the details.");
        player.AddClue({"A fragment recovered from the " + object + "."});
        progression.GrantXP(10);
        progression.CheckLevelUp(player);
    } else {
        ui.ShowMessage("Nothing new is revealed.");
    }
    if (area.RevealNextInteractable()) ui.ShowMessage("Another interaction becomes visible.");
    if (area.HasNPC() && !area.HasDiscoveredNPC() && area.IsFullySearched()) area.DiscoverNPC();
}

void SearchRoom(Area& area, Player& player) {
    if (!area.ConsumeItem()) {
        std::cout << "\nYou find nothing useful.\n";
        return;
    }

    MapManager map;
    const RoomDefinition* definition = map.GetRoomDefinition(area);
    const std::string& room = area.getName();
    const auto clues = BuildClueCatalog();
    if (room == "Archive Room") {
        for (const auto& definition : clues) {
            if (definition.name == "choir_logbook") {
                player.AddItem(std::make_unique<Item>(definition));
                break;
            }
        }
    } else {
        const ItemDefinition reward = BuildInvestigationConsumable(
            definition == nullptr ? ConsumableProfile::SupplyCache : definition->consumableProfile, 1);
        if (!reward.name.empty()) player.AddItem(std::make_unique<Item>(reward));
    }
    std::cout << "\nYou found something useful. It was added to your inventory.\n";
}
}

EncounterManager::EncounterManager() = default;

bool EncounterManager::Explore(Area& area, Player& player,
                               ProgressionManager& progression,
                               CombatManager& combat) {
    ConsoleUI ui;
    const unsigned seed = RoomSeed(area.getName());
    if (area.GetVisibleInteractables().empty()) area.RevealNextInteractable();
    if (!area.hasEnemies()
        && static_cast<int>(seed % 100) < SpawnChance(area.GetSpawnCondition())) {
        SpawnEnemy(area, seed);
        ui.ShowMessage("A hostile presence emerges from the room. Combat begins.");
    }

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
                if (!combat.UseConsumable(consumables[static_cast<std::size_t>(itemChoice - 1)], player, *enemy))
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
                    combat.enemyTurn(player, *enemy);
            }
            continue;
        }

        ui.RenderRoom(area, player, progression);
        const int choice = ui.Prompt("Choose an exploration action.", 0, 4);
        switch (choice) {
        case 1: InspectEnvironment(area, player, progression, ui); break;
        case 2: SearchRoom(area, player); break;
        case 3:
            ui.ShowInventory(player);
            ui.Prompt("Press 0 to return.", 0, 0);
            break;
        case 4:
            ui.ShowMessage("The floor map is etched into the cathedral ledger. Return to the room selection to view it.");
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
