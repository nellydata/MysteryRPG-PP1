#include "Enemy.h"
#include <algorithm>

Enemy::Enemy(const std::string& name, int hp, int atk, int xpReward)
	: name_(name), hp_(hp), maxHp_(hp), atk_(atk), xpReward_(xpReward),
	  defense_(0), speed_(5) {}

Enemy::~Enemy() = default;

void Enemy::takeDamage(int amount) {
	if (amount <= 0) return;
	hp_ -= amount;
	if (hp_ < 0) hp_ = 0;
}

bool Enemy::isAlive() const { return hp_ > 0; }
const std::string& Enemy::getName() const { return name_; }
int Enemy::getHP() const { return hp_; }
int Enemy::getMaxHP() const { return maxHp_; }
int Enemy::getAtk() const { return atk_; }
int Enemy::getXPReward() const { return xpReward_; }
int Enemy::getDefense() const { return defense_; }
int Enemy::getSpeed() const { return speed_; }

bool Enemy::HasStatus(StatusType type) const {
	for (const auto& status : statuses_) if (status.type == type) return true;
	return false;
}

void Enemy::ApplyStatus(const StatusEffect& status) {
	if (status.type != StatusType::HealOverTime && HasStatus(status.type)) return;
	statuses_.push_back(status);
}

void Enemy::ProcessStatuses() {
	for (auto& status : statuses_) {
		if (status.type == StatusType::Bleed) takeDamage(status.sourceAttack * 30 / 100);
		else if (status.type == StatusType::Burn) takeDamage(status.sourceAttack * 60 / 100);
		else if (status.type == StatusType::Poison) takeDamage(maxHp_ * 5 / 100);
		--status.turns;
	}
	statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
		return status.turns <= 0;
	}), statuses_.end());
}

void Enemy::ClearStatuses() { statuses_.clear(); }
