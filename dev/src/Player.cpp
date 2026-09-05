#include "Player.h"
#include "Enemy.h"
#include <iostream>
#include <algorithm>

Player::Player(const std::string& name, int maxHp, int maxSp)
    : name_(name), hp_(maxHp), maxHp_(maxHp), sp_(maxSp), maxSp_(maxSp),
      baseStats_{maxHp, 0, 5, 6, 0, 0, 5, 150},
      totalStats_(baseStats_), equipped_{nullptr, nullptr, nullptr, nullptr} {}

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
int Player::getAttack() const { return totalStats_.atk; }
int Player::getDefense() const { return totalStats_.def; }
int Player::getSP() const { return sp_; }
int Player::getSpeed() const { return totalStats_.spd; }
int Player::getAccuracy() const { return totalStats_.acc; }
int Player::getResistance() const { return totalStats_.res; }
int Player::getCritRate() const { return totalStats_.critRate; }
int Player::getCritDamage() const { return totalStats_.critDamage; }
const Stats& Player::getStats() const { return totalStats_; }
std::size_t Player::GetInventorySize() const {
    return inventory_.GetItems().size() + inventory_.GetClues().size();
}

void Player::increaseMaxHP(int amount) {
    if (amount > 0) {
        baseStats_.hp += amount;
        RecalculateStats();
        hp_ += amount;
    }
}

void Player::increaseAttack(int amount) {
    if (amount > 0) { baseStats_.atk += amount; RecalculateStats(); }
}

void Player::increaseDefense(int amount) {
    if (amount > 0) { baseStats_.def += amount; RecalculateStats(); }
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
    inventory_.AddItem(std::move(item));
}

void Player::AddClue(const Clue& clue) {
    inventory_.AddClue(clue);
}

void Player::PrintInventory() const {
    std::cout << "\nItems:\n";
    if (inventory_.GetItems().empty()) {
        std::cout << "- None\n";
    } else {
        for (const auto& item : inventory_.GetItems()) {
            std::cout << "- " << item->getName() << ": "
                      << item->getDescription() << "\n";
        }
    }

    std::cout << "Clues:\n";
    if (inventory_.GetClues().empty()) {
        std::cout << "- None\n";
    } else {
        for (const auto& clue : inventory_.GetClues()) {
            std::cout << "- " << clue.description << "\n";
        }
    }
}

bool Player::Equip(Item& item) {
    if (!item.isEquippable() || item.getSlot() == EquipSlot::None) return false;
    equipped_[static_cast<int>(item.getSlot())] = &item;
    RecalculateStats();
    return true;
}

bool Player::UseConsumable(const std::string& itemName, Enemy* target) {
    Item* item = inventory_.FindItem(itemName);
    if (item == nullptr || item->getCategory() != ItemCategory::Consumable) return false;
    const std::string& effect = item->getEffect();
    if (effect.find("restore_20") != std::string::npos) heal(maxHp_ * 20 / 100);
    else if (effect.find("restore_35") != std::string::npos) heal(maxHp_ * 35 / 100);
    else if (effect.find("restore_50") != std::string::npos) heal(maxHp_ * 50 / 100);
    else if (effect == "remove_all_debuffs") ClearStatuses();
    else if (effect == "remove_burn_and_poison") {
        statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
            return status.type == StatusType::Burn || status.type == StatusType::Poison;
        }), statuses_.end());
    } else if (effect.find("deal_flat_50") != std::string::npos && target != nullptr) {
        target->takeDamage(50);
    } else if (effect.find("apply_burn") != std::string::npos && target != nullptr) {
        target->ApplyStatus({StatusType::Burn, 3, 0, getAttack()});
    } else return false;
    inventory_.RemoveItem(itemName);
    return true;
}

bool Player::HasStatus(StatusType type) const {
    for (const auto& status : statuses_) if (status.type == type) return true;
    return false;
}

void Player::ApplyStatus(const StatusEffect& status) {
    if (status.type != StatusType::HealOverTime && HasStatus(status.type)) return;
    statuses_.push_back(status);
}

void Player::ProcessStatuses() {
    for (auto& status : statuses_) {
        if (status.type == StatusType::Bleed) takeDamage(status.sourceAttack * 30 / 100);
        else if (status.type == StatusType::Burn) takeDamage(status.sourceAttack * 60 / 100);
        else if (status.type == StatusType::Poison) takeDamage(maxHp_ * 5 / 100);
        else if (status.type == StatusType::HealOverTime) heal(status.potency);
        --status.turns;
    }
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
        return status.turns <= 0;
    }), statuses_.end());
}

void Player::ClearStatuses() { statuses_.clear(); }

void Player::ModifyStats(const Stats& delta) {
    baseStats_.atk += delta.atk;
    baseStats_.def += delta.def;
    baseStats_.spd += delta.spd;
    baseStats_.acc += delta.acc;
    baseStats_.res += delta.res;
    baseStats_.critRate += delta.critRate;
    baseStats_.critDamage += delta.critDamage;
    RecalculateStats();
}

const Inventory& Player::GetInventory() const { return inventory_; }
Item* Player::FindItem(const std::string& itemName) { return inventory_.FindItem(itemName); }
const std::vector<StatusEffect>& Player::GetStatuses() const { return statuses_; }
const Item* Player::GetEquipped(EquipSlot slot) const {
    if (slot == EquipSlot::None) return nullptr;
    return equipped_[static_cast<int>(slot)];
}

void Player::RecalculateStats() {
    totalStats_ = baseStats_;
    for (const Item* item : equipped_) {
        if (item == nullptr) continue;
        const Stats& stats = item->getStats();
        totalStats_.hp += stats.hp;
        totalStats_.def += stats.def;
        totalStats_.spd += stats.spd;
        totalStats_.atk += stats.atk;
        totalStats_.acc += stats.acc;
        totalStats_.res += stats.res;
        totalStats_.critRate += stats.critRate;
        totalStats_.critDamage += stats.critDamage;
    }
    maxHp_ = totalStats_.hp;
    if (hp_ > maxHp_) hp_ = maxHp_;
}
