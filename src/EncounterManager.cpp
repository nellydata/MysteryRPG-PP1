#include "EncounterManager.h"
#include "Area.h"
#include "Automata.h"
#include "Boss.h"
#include "CombatManager.h"
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
    case 0:
        area.addEnemy(std::make_unique<Automata>());
        break;
    case 1:
        area.addEnemy(std::make_unique<Nun>());
        break;
    default:
        area.addEnemy(std::make_unique<Enemy>("Shimmer Husk", 10, 3));
        break;
    }
}

void InspectEnvironment(Area& area, Player& player, ProgressionManager& progression) {
    const auto& interactables = area.GetInteractables();
    if (interactables.empty()) {
        std::cout << "There is nothing obvious to inspect.\n";
        return;
    }

    std::cout << "Inspect:\n";
    for (std::size_t index = 0; index < interactables.size(); ++index) {
        std::cout << index + 1 << ". " << interactables[index] << "\n";
    }
    std::cout << "> ";

    int choice = 0;
    if (!(std::cin >> choice) || choice < 1
        || choice > static_cast<int>(interactables.size())) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "That is not a valid interactable.\n";
        return;
    }

    std::cout << "You inspect the " << interactables[choice - 1] << ".\n";
    if (area.ConsumeClue()) {
        std::cout << "A hidden clue emerges from the architecture.\n";
        progression.GrantXP(10);
        progression.CheckLevelUp(player);
    } else {
        std::cout << "Nothing new is revealed.\n";
    }
}

void SearchItems(Area& area, Player& player) {
    if (!area.ConsumeItem()) {
        std::cout << "There are no undiscovered items here.\n";
        return;
    }

    player.AddItem(std::make_unique<Item>("Cathedral Token", "A recovered cathedral keepsake."));
    std::cout << "You found a Cathedral Token. Items are collected but cannot be used yet.\n";
}

void TalkToNPC(const Area& area) {
    if (area.HasNPC()) {
        std::cout << "You see an NPC here. Interaction system coming soon.\n";
    } else {
        std::cout << "There is no one here to speak with.\n";
    }
}
}

EncounterManager::EncounterManager() = default;

bool EncounterManager::Explore(Area& area, Player& player,
                               ProgressionManager& progression,
                               CombatManager& combat) {
    const unsigned seed = RoomSeed(area.getName());
    const bool shouldSpawn = static_cast<int>(seed % 100)
        < SpawnChance(area.GetSpawnCondition());
    if (shouldSpawn && !area.hasEnemies()) SpawnEnemy(area, seed);

    while (player.isAlive()) {
        Enemy* enemy = area.getFirstAliveEnemy();
        const bool inCombat = enemy != nullptr;

        std::cout << "\n" << area.getName() << "\n";
        if (inCombat) {
            std::cout << "Enemy: " << enemy->getName() << " (HP " << enemy->getHP() << ")\n"
                      << "Your HP: " << player.getHP() << " | SP: " << player.getSP() << "\n"
                      << "1. Attack\n2. Block\n3. Interrogate\n"
                      << "4. Expose Weakness\n5. Use Item\n6. Flee\n"
                      << "7. Inspect environment\n8. Search for items\n9. Speak to NPC\n0. Leave room\n> ";
        } else {
            std::cout << "1. Inspect environment\n2. Search for items\n"
                      << "3. Speak to NPC\n0. Leave room\n> ";
        }

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Enter one of the displayed choices.\n";
            continue;
        }

        if (choice == 0) return !inCombat && player.isAlive();
        if ((!inCombat && choice == 1) || (inCombat && choice == 7)) {
            InspectEnvironment(area, player, progression);
            continue;
        }
        if ((!inCombat && choice == 2) || (inCombat && choice == 8)) {
            SearchItems(area, player);
            continue;
        }
        if ((!inCombat && choice == 3) || (inCombat && choice == 9)) {
            TalkToNPC(area);
            continue;
        }
        if (!inCombat) {
            std::cout << "That action is not available.\n";
            continue;
        }

        CombatAction action;
        switch (choice) {
        case 1: action = CombatAction::Attack; break;
        case 2: action = CombatAction::Block; break;
        case 3: action = CombatAction::Interrogate; break;
        case 4: action = CombatAction::ExposeWeakness; break;
        case 5: action = CombatAction::Item; break;
        case 6: return false;
        default:
            std::cout << "That action is not available.\n";
            continue;
        }

        if (!combat.resolve(action, player, *enemy, nullptr, 5, &progression)) continue;
        const bool defeated = !enemy->isAlive();
        if (defeated) {
            area.removeDeadEnemies();
        } else {
            combat.enemyTurn(player, *enemy);
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
