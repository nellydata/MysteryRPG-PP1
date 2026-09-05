#include "EncounterManager.h"
#include "Area.h"
#include "Automata.h"
#include "Boss.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
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

void PrintStatus(const Player& player, const ProgressionManager& progression) {
    std::cout << "Status: HP " << player.getHP() << "/" << player.getMaxHP()
              << " | Attack " << player.getAttack()
              << " | Defense " << player.getDefense()
              << " | Level " << progression.GetLevel()
              << " | XP " << progression.GetXP() << "\n";
}

void InspectEnvironment(Area& area, Player& player, ProgressionManager& progression) {
    const auto& interactables = area.GetInteractables();
    std::cout << "What do you inspect?\n";
    for (std::size_t index = 0; index < interactables.size(); ++index) {
        std::cout << index + 1 << ". " << interactables[index] << "\n";
    }
    std::cout << "0. Back\n> ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "That is not a valid choice.\n";
        return;
    }
    if (choice == 0) return;
    if (choice < 1 || choice > static_cast<int>(interactables.size())) {
        std::cout << "That is not a valid interactable.\n";
        return;
    }

    const std::string& object = interactables[choice - 1];
    std::cout << "You inspect the " << object << ".\n";
    if (object == "Figure in the shadows" && area.HasNPC()) {
        std::cout << "You see an NPC here. Interaction system coming soon.\n";
    }
    if (area.ConsumeClue()) {
        std::cout << "You uncover a clue hidden in the details.\n";
        player.AddClue({"A fragment recovered from the " + object + "."});
        progression.GrantXP(10);
        progression.CheckLevelUp(player);
    } else {
        std::cout << "Nothing new is revealed.\n";
    }
}

void SearchRoom(Area& area, Player& player) {
    if (!area.ConsumeItem()) {
        std::cout << "You find nothing useful.\n";
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
    std::cout << "You found something useful. It was added to your inventory.\n";
}
}

EncounterManager::EncounterManager() = default;

bool EncounterManager::Explore(Area& area, Player& player,
                               ProgressionManager& progression,
                               CombatManager& combat) {
    const unsigned seed = RoomSeed(area.getName());
    if (!area.hasEnemies()
        && static_cast<int>(seed % 100) < SpawnChance(area.GetSpawnCondition())) {
        SpawnEnemy(area, seed);
        std::cout << "\nA hostile presence emerges from the room.\n"
                  << "Combat begins.\n";
    }

    while (player.isAlive()) {
        Enemy* enemy = area.getFirstAliveEnemy();
        if (enemy != nullptr) {
            combat.BeginEncounter(*enemy);
            std::cout << "\nCombat with " << enemy->getName() << ".\n";
            PrintStatus(player, progression);
            std::cout << "Enemy HP: " << enemy->getHP() << "/" << enemy->getMaxHP() << "\n"
                      << "1. Attack\n2. Block\n3. Shimmer\n"
                      << "4. Interrogate\n5. Expose Weakness\n6. Flee\n> ";

            int choice = 0;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                std::cout << "Enter a combat choice from 1 to 6.\n";
                continue;
            }

            if (choice == 6) {
                combat.resolve(CombatAction::Flee, player, *enemy);
                return false;
            }

            CombatAction action;
            switch (choice) {
            case 1: action = CombatAction::Attack; break;
            case 2: action = CombatAction::Block; break;
            case 3: action = CombatAction::Shimmer; break;
            case 4: action = CombatAction::Interrogate; break;
            case 5: action = CombatAction::ExposeWeakness; break;
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

        std::cout << "\n" << area.getName() << "\n";
        PrintStatus(player, progression);
        std::cout << "1. Inspect environment\n2. Search room\n"
                  << "3. Open inventory\n0. Leave room\n> ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Enter one of the displayed choices.\n";
            continue;
        }
        switch (choice) {
        case 1: InspectEnvironment(area, player, progression); break;
        case 2: SearchRoom(area, player); break;
        case 3: player.PrintInventory(); break;
        case 0: return true;
        default: std::cout << "That action is not available.\n"; break;
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
