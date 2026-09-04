#include "Enemy.h"

Enemy::Enemy(const std::string& name, int hp, int atk, int xpReward)
	: name_(name), hp_(hp), atk_(atk), xpReward_(xpReward) {}

Enemy::~Enemy() = default;

void Enemy::takeDamage(int amount) {
	if (amount <= 0) return;
	hp_ -= amount;
	if (hp_ < 0) hp_ = 0;
}

bool Enemy::isAlive() const { return hp_ > 0; }
const std::string& Enemy::getName() const { return name_; }
int Enemy::getHP() const { return hp_; }
int Enemy::getAtk() const { return atk_; }
int Enemy::getXPReward() const { return xpReward_; }
