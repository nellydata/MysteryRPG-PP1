#include "Enemy.h"
#include <algorithm>
#include <string>

Enemy::Enemy(const std::string& name, int hp, int atk, int xpReward, int level)
	: name_(name), hp_(hp), maxHp_(hp), atk_(atk), xpReward_(xpReward),
	  defense_(0), speed_(5), level_(level), resistance_(0), evasion_(0) {
	int baseSpeed = 10;
	double growth = 1.0;
	if (name.find("Automata") != std::string::npos) baseSpeed = 8;
	else if (name.find("Buff Nun") != std::string::npos) { baseSpeed = 14; growth = 2.0; }
	else if (name.find("Nun") != std::string::npos) { baseSpeed = 12; growth = 1.5; }
	speed_ = baseSpeed + static_cast<int>(growth * (std::max)(0, level_ - 1));
}

const std::vector<StatusEffect>& Enemy::GetStatuses() const { return statuses_; }

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
int Enemy::getLevel() const { return level_; }
int Enemy::getResistance() const { return resistance_; }
int Enemy::getEvasion() const { return evasion_; }

bool Enemy::HasStatus(StatusType type) const {
	for (const auto& status : statuses_) if (status.type == type) return true;
	return false;
}

bool Enemy::ApplyStatus(const StatusEffect& status, int accuracy) {
	const unsigned roll = static_cast<unsigned>((hp_ * 17 + level_ * 31 + static_cast<int>(status.type) * 13) % 100);
	if (status.type != StatusType::HealOverTime && roll >= static_cast<unsigned>((std::max)(0, accuracy - resistance_))) return false;
	if (status.type != StatusType::HealOverTime && HasStatus(status.type)) return false;
	statuses_.push_back(status);
	return true;
}

void Enemy::ProcessStatuses() {
	for (auto& status : statuses_) {
		if (status.type == StatusType::Bleed) takeDamage(status.sourceAttack * 30 * (100 + status.dotPotency) / 10000);
		else if (status.type == StatusType::Burn) takeDamage(status.sourceAttack * 60 * (100 + status.dotPotency) / 10000);
		else if (status.type == StatusType::Poison) takeDamage(maxHp_ * 5 / 100);
		--status.turns;
	}
	statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
		return status.turns <= 0;
	}), statuses_.end());
}

void Enemy::ClearStatuses() { statuses_.clear(); }
