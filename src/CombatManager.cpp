#include "CombatManager.h"
#include "Enemy.h"
#include <iostream>

CombatManager::CombatManager() = default;
CombatManager::~CombatManager() = default;

void CombatManager::Attack(Enemy& enemy, int damage) {
	enemy.takeDamage(damage);
	std::cout << "You hit " << enemy.getName() << " for " << damage << " damage.\n";
	if (!enemy.isAlive()) {
		std::cout << enemy.getName() << " is defeated.\n";
	}
}

void CombatManager::EnemyAttack(Enemy& enemy) {
	std::cout << enemy.getName() << " attacks for " << enemy.getAtk() << " damage.\n";
}
