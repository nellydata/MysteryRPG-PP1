#include "Player.h"

Player::Player(const std::string& name, int maxHp, int maxSp)
    : name_(name), hp_(maxHp), maxHp_(maxHp), attack_(6), defense_(0),
      sp_(maxSp), maxSp_(maxSp) {}

Player::~Player() = default;

void Player::takeDamage(int amount) {
    if (amount <= 0) return;
    hp_ -= amount;
    if (hp_ < 0) hp_ = 0;
}

void Player::heal(int amount) {
    if (amount <= 0) return;
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
}

bool Player::isAlive() const { return hp_ > 0; }
const std::string& Player::getName() const { return name_; }
int Player::getHP() const { return hp_; }
int Player::getMaxHP() const { return maxHp_; }
int Player::getAttack() const { return attack_; }
int Player::getDefense() const { return defense_; }
int Player::getSP() const { return sp_; }
std::size_t Player::GetInventorySize() const { return inventory_.size(); }

void Player::increaseMaxHP(int amount) {
    if (amount > 0) maxHp_ += amount;
}

void Player::increaseAttack(int amount) {
    if (amount > 0) attack_ += amount;
}

void Player::increaseDefense(int amount) {
    if (amount > 0) defense_ += amount;
}

void Player::spendSP(int amount) {
    if (amount <= 0) return;
    sp_ -= amount;
    if (sp_ < 0) sp_ = 0;
}

void Player::gainSP(int amount) {
    if (amount <= 0) return;
    sp_ += amount;
    if (sp_ > maxSp_) sp_ = maxSp_;
}

void Player::AddItem(std::unique_ptr<Item> item) {
    if (item) inventory_.push_back(std::move(item));
}
