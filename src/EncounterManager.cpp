#include "EncounterManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <iostream>

EncounterManager::EncounterManager() = default;

bool EncounterManager::explore(Area& area, Player& player, CombatManager& combat,
							   ProgressionManager& progression) {
	while (area.hasEnemies() && player.isAlive()) {
		Enemy* enemy = area.getFirstAliveEnemy();
		if (enemy == nullptr) break;

		std::cout << "\n" << area.getName() << "\n"
				  << "Enemy: " << enemy->getName() << " (HP " << enemy->getHP() << ")\n"
				  << "Your HP: " << player.getHP() << " | SP: " << player.getSP() << "\n"
				  << "1. Attack\n2. Block\n3. Interrogate\n"
				  << "4. Expose Weakness\n5. Use Item\n6. Flee\n> ";

		int choice = 0;
		if (!(std::cin >> choice)) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			std::cout << "Enter a number from 1 to 6.\n";
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

		if (action == CombatAction::Interrogate) {
			progression.GrantXP(10);
			progression.CheckLevelUp(player);
		}

		if (!enemy->isAlive()) {
			area.removeDeadEnemies();
			continue;
		}

		combat.enemyTurn(player, *enemy);
	}

	return player.isAlive() && !area.hasEnemies();
}
