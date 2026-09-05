#include "Player.h"
#include "Enemy.h"
#include <iostream>
#include <algorithm>

namespace {
bool HasValue(const Stats& stats, ItemStat stat) {
    switch (stat) {
    case ItemStat::HP: return stats.hp != 0;
    case ItemStat::DEF: return stats.def != 0;
    case ItemStat::SPD: return stats.spd != 0;
    case ItemStat::ATK: return stats.atk != 0;
    case ItemStat::Accuracy: return stats.acc != 0;
    case ItemStat::Resistance: return stats.res != 0;
    case ItemStat::CritRate: return stats.critRate != 0;
    case ItemStat::CritDamage: return stats.critDamage != 0;
    case ItemStat::Evasion: return stats.evasion != 0;
    case ItemStat::HealingPotency: return stats.healingPotency != 0;
    case ItemStat::BarrierPotency: return stats.barrierPotency != 0;
    case ItemStat::DotPotency: return stats.dotPotency != 0;
    case ItemStat::Lifesteal: return stats.lifesteal != 0;
    case ItemStat::DebuffDurationReduction: return stats.debuffDurationReduction != 0;
    }
    return false;
}

bool ViolatesTheme(const Item& item) {
    for (const ItemStat stat : item.getForbiddenStats()) {
        if (HasValue(item.getStats(), stat)) return true;
    }
    return false;
}
}

Player::Player(const std::string& name, int maxHp, int maxSp)
    : name_(name), hp_(maxHp), maxHp_(maxHp), sp_(maxSp), maxSp_(maxSp),
      baseStats_{maxHp, 10, 10, 20, 0, 0, 5, 150},
      totalStats_(baseStats_), equipped_{nullptr, nullptr, nullptr, nullptr, nullptr} {}

Player::~Player() = default;

void Player::takeDamage(int amount) {
    if (amount <= 0) return;
    hp_ -= amount;
    if (hp_ < 0) hp_ = 0;
}

bool Player::ToggleEquip(Item& item) {
    if (!item.isEquippable() || item.getSlot() == EquipSlot::None || ViolatesTheme(item)) return false;
    const int slot = static_cast<int>(item.getSlot());
    if (equipped_[slot] == &item) equipped_[slot] = nullptr;
    else equipped_[slot] = &item;
    RecalculateStats();
    return true;
}

void Player::heal(int amount) {
    if (amount <= 0) return;
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
}

bool Player::isAlive() const { return hp_ > 0; }
const std::string& Player::getName() const { return name_; }
void Player::SetName(const std::string& name) {
    if (!name.empty()) name_ = name;
}
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
int Player::getEvasion() const { return totalStats_.evasion; }
int Player::getHealingPotency() const { return totalStats_.healingPotency; }
int Player::getBarrierPotency() const { return totalStats_.barrierPotency; }
int Player::getDotPotency() const { return totalStats_.dotPotency; }
int Player::getLifesteal() const { return totalStats_.lifesteal; }
int Player::getDebuffDurationReduction() const { return totalStats_.debuffDurationReduction; }
int Player::getDisplayedAccuracy() const { return (std::min)(100, totalStats_.acc); }
int Player::getDisplayedResistance() const { return (std::min)(100, totalStats_.res); }
int Player::getDisplayedCritRate() const { return (std::min)(100, totalStats_.critRate); }
int Player::getDisplayedEvasion() const { return (std::min)(80, totalStats_.evasion); }
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

void Player::SetLevelStats(int level) {
    const int levelOffset = (std::max)(0, level - 1);
    const int previousMaxHp = maxHp_;
    baseStats_.hp = 20 + levelOffset * 5;
    baseStats_.def = 10 + levelOffset * 10;
    baseStats_.atk = 20 + levelOffset * 15;
    baseStats_.spd = 10;
    RecalculateStats();
    hp_ += maxHp_ - previousMaxHp;
    if (hp_ > maxHp_) hp_ = maxHp_;
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
    if (!item.isEquippable() || item.getSlot() == EquipSlot::None || ViolatesTheme(item)) return false;
    equipped_[static_cast<int>(item.getSlot())] = &item;
    RecalculateStats();
    return true;
}

std::vector<std::string> Player::GetActiveSets() const {
    std::vector<std::string> activeSets;
    for (const Item* item : equipped_) {
        if (item == nullptr || item->getSetName().empty()) continue;
        bool alreadySeen = false;
        for (const std::string& setName : activeSets) {
            if (setName == item->getSetName()) {
                alreadySeen = true;
                break;
            }
        }
        if (alreadySeen) continue;

        int pieces = 0;
        for (const Item* equipped : equipped_) {
            if (equipped != nullptr && equipped->getSetName() == item->getSetName()) ++pieces;
        }
        if (pieces == 5) activeSets.push_back(item->getSetName());
    }
    return activeSets;
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
        if (status.type == StatusType::Bleed) takeDamage(status.sourceAttack * 30 * (100 + status.dotPotency) / 10000);
        else if (status.type == StatusType::Burn) takeDamage(status.sourceAttack * 60 * (100 + status.dotPotency) / 10000);
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
    baseStats_.evasion += delta.evasion;
    baseStats_.healingPotency += delta.healingPotency;
    baseStats_.healingReceived += delta.healingReceived;
    baseStats_.barrierPotency += delta.barrierPotency;
    baseStats_.dotPotency += delta.dotPotency;
    baseStats_.lifesteal += delta.lifesteal;
    baseStats_.debuffDurationReduction += delta.debuffDurationReduction;
    RecalculateStats();
}

const Inventory& Player::GetInventory() const { return inventory_; }
Inventory& Player::GetInventory() { return inventory_; }
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
        totalStats_.evasion += stats.evasion;
        totalStats_.healingPotency += stats.healingPotency;
        totalStats_.healingReceived += stats.healingReceived;
        totalStats_.barrierPotency += stats.barrierPotency;
        totalStats_.dotPotency += stats.dotPotency;
        totalStats_.lifesteal += stats.lifesteal;
        totalStats_.debuffDurationReduction += stats.debuffDurationReduction;
        const int percent = item->getMainStatPercent();
        if (percent > 0) {
            int baseValue = 0;
            int* totalValue = nullptr;
            if (item->getMainStat() == "hp") { baseValue = baseStats_.hp; totalValue = &totalStats_.hp; }
            else if (item->getMainStat() == "def") { baseValue = baseStats_.def; totalValue = &totalStats_.def; }
            else if (item->getMainStat() == "spd") { baseValue = baseStats_.spd; totalValue = &totalStats_.spd; }
            else if (item->getMainStat() == "atk") { baseValue = baseStats_.atk; totalValue = &totalStats_.atk; }
            else if (item->getMainStat() == "acc") { baseValue = 100; totalValue = &totalStats_.acc; }
            if (totalValue != nullptr) *totalValue += baseValue * percent / 100;
        }
    }

    for (const std::string& setName : GetActiveSets()) {
        if (setName == "Shimmer Ascendant") {
            totalStats_.atk += 15;
            totalStats_.acc += 15;
        } else if (setName == "Automaton Overlord") {
            totalStats_.acc += 20;
            totalStats_.critDamage += 20;
        } else if (setName == "Oracle Automaton") {
            totalStats_.acc += 10;
            totalStats_.critDamage += 10;
        }
    }
    maxHp_ = totalStats_.hp;
    if (hp_ > maxHp_) hp_ = maxHp_;
}
