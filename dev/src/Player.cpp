#include "Player.h"
#include <iostream>

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
std::size_t Player::GetInventorySize() const { return inventory_.size() + clues_.size(); }

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

void Player::AddClue(const Clue& clue) {
    clues_.push_back(clue);
}

void Player::PrintInventory() const {
    std::cout << "\nItems:\n";
    if (inventory_.empty()) {
        std::cout << "- None\n";
    } else {
        for (const auto& item : inventory_) {
            std::cout << "- " << item->getName() << ": "
                      << item->getDescription() << "\n";
        }
    }

    std::cout << "Clues:\n";
    if (clues_.empty()) {
        std::cout << "- None\n";
    } else {
        for (const auto& clue : clues_) {
            std::cout << "- " << clue.description << "\n";
        }
    }
}
