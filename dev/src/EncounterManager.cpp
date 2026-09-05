#include "EncounterManager.h"
#include "Area.h"
#include "Automata.h"
#include "Boss.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
#include "Enemy.h"
#include "Item.h"
#include "Nun.h"
#include "Player.h"
#include "ProgressionManager.h"
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
    switch (seed % 3) {
    case 0: area.addEnemy(std::make_unique<Automata>()); break;
    case 1: area.addEnemy(std::make_unique<Nun>()); break;
    default: area.addEnemy(std::make_unique<Enemy>("Shimmer Husk", 10, 3)); break;
    }
}

void InspectEnvironment(Area& area, Player& player, ProgressionManager& progression,
                        ConsoleUI& ui) {
    const auto& interactables = area.GetInteractables();
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
}

void SearchRoom(Area& area, Player& player) {
    if (!area.ConsumeItem()) {
        std::cout << "\nYou find nothing useful.\n";
        return;
    }

    const auto consumables = BuildConsumableCatalog();
    const auto clues = BuildClueCatalog();
    const std::string& room = area.getName();
    if (room == "Archive Room") {
        for (const auto& definition : clues) {
            if (definition.name == "choir_logbook") {
                player.AddItem(std::make_unique<Item>(definition));
                break;
            }
        }
    } else {
        for (const auto& definition : consumables) {
            if (definition.name == "cellar_rations") {
                player.AddItem(std::make_unique<Item>(definition));
                break;
            }
        }
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
    if (!area.hasEnemies()
        && static_cast<int>(seed % 100) < SpawnChance(area.GetSpawnCondition())) {
        SpawnEnemy(area, seed);
        ui.ShowMessage("A hostile presence emerges from the room. Combat begins.");
    }

    while (player.isAlive()) {
        Enemy* enemy = area.getFirstAliveEnemy();
        if (enemy != nullptr) {
            combat.BeginEncounter(*enemy);
            ui.RenderCombat(player, progression, *enemy, combat);
            const int choice = ui.Prompt("Choose a combat action.", 1, 5);

            if (choice == 5) {
                combat.resolve(CombatAction::Flee, player, *enemy);
                return false;
            }

            CombatAction action;
            switch (choice) {
            case 1: action = CombatAction::Attack; break;
            case 2: action = CombatAction::Shimmer; break;
            case 3: action = CombatAction::Interrogate; break;
            case 4: action = CombatAction::ExposeWeakness; break;
            default:
                std::cout << "That is not a combat action.\n";
                continue;
            }

            if (!combat.resolve(action, player, *enemy, nullptr, &progression)) continue;
            if (!enemy->isAlive()) {
                area.removeDeadEnemies();
            } else {
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
