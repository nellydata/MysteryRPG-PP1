#include "ProgressionManager.h"
#include "Player.h"
#include <iostream>

ProgressionManager::ProgressionManager() : xp_(0), level_(1) {}

void ProgressionManager::awardXP(Player& player, int amount) {
	if (amount <= 0) return;

	xp_ += amount;
	std::cout << "Gained " << amount << " XP.\n";

	while (xp_ >= getXPToNextLevel()) {
		xp_ -= getXPToNextLevel();
		levelUp(player);
	}
}

int ProgressionManager::getXP() const { return xp_; }
int ProgressionManager::getLevel() const { return level_; }
int ProgressionManager::getXPToNextLevel() const { return level_ * 25; }

void ProgressionManager::levelUp(Player& player) {
	++level_;
	player.heal(5);
	player.gainSP(2);
	std::cout << "Adelaide reached level " << level_ << ".\n";
}
