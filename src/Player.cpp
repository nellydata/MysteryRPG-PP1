#include "Player.h"

Player::Player(const std::string& name, int maxHp, int maxSp)
	: name_(name), hp_(maxHp), maxHp_(maxHp), sp_(maxSp), maxSp_(maxSp), level_(1) {}

Player::~Player() = default;

void Player::takeDamage(int amount) {
	hp_ -= amount;
	if (hp_ < 0) hp_ = 0;
}

void Player::heal(int amount) {
	hp_ += amount;
	if (hp_ > maxHp_) hp_ = maxHp_;
}

bool Player::isAlive() const { return hp_ > 0; }
const std::string& Player::getName() const { return name_; }
int Player::getHP() const { return hp_; }
int Player::getSP() const { return sp_; }

void Player::spendSP(int amount) {
	sp_ -= amount;
	if (sp_ < 0) sp_ = 0;
}

void Player::gainSP(int amount) {
	sp_ += amount;
	if (sp_ > maxSp_) sp_ = maxSp_;
}
