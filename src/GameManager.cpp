#include "GameManager.h"
#include "Player.h"
#include "MapManager.h"
#include "CombatManager.h"
#include "Area.h"
#include "Enemy.h"
#include <memory>

void GameManager::Run() {
	auto player = std::make_unique<Player>("Adelaide", 30, 10);
	MapManager map;
	CombatManager combat;

	std::cout << "Entering the cathedral...\n";
	auto area = map.generateArea("Entrance Hall");

	while (area->hasEnemies() && player->isAlive()) {
		Enemy* enemy = area->getFirstAliveEnemy();
		if (!enemy) break;

		std::cout << "You encounter: " << enemy->getName()
				  << " (HP: " << enemy->getHP() << ")\n";
		combat.Attack(*enemy, 6);

		if (!enemy->isAlive()) {
			area->removeDeadEnemies();
			continue;
		}

		combat.EnemyAttack(*enemy);
		player->takeDamage(enemy->getAtk());
		std::cout << "Your HP: " << player->getHP() << "\n";
	}

	std::cout << (player->isAlive() ? "Area cleared.\n" : "You have been defeated.\n");
}
