#include "Player.h"
#include "Enemy.h"
#include "CombatMath.h"
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
      baseStats_{maxHp, 10, CombatMath::Balance::BasePlayerSpeed, 20, 0, 0, 5, 150},
      totalStats_(baseStats_), equipped_{nullptr, nullptr, nullptr, nullptr, nullptr} {}

Player::~Player() = default;

void Player::takeDamage(int amount) {
    if (amount <= 0) return;
    int damageReduction = 0;
    for (const auto& status : statuses_) {
        if (status.type == StatusType::DamageReduction) damageReduction += status.potency;
    }
    amount = amount * (100 - (std::min)(CombatMath::Balance::MaximumDamageReduction, damageReduction)) / 100;
    if (amount <= 0) return;
    amount = AbsorbDamage(amount);
    if (amount <= 0) return;
    const auto activeSets = GetActiveSets();
    const bool verdigrisSet = std::find(activeSets.begin(), activeSets.end(), "Verdigris Matron") != activeSets.end();
    const int healthBefore = hp_;
    hp_ -= amount;
    if (hp_ < 0) hp_ = 0;
    if (verdigrisSet && healthBefore > hp_) {
        const int recovery = healthBefore - hp_;
        heal(recovery / 2);
        AddBarrier(recovery / 4);
    }
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
    int healResistance = 0;
    for (const auto& status : statuses_) {
        if (status.type == StatusType::HealResistance) healResistance += status.potency;
    }
    amount = amount * (100 - (std::min)(100, healResistance)) / 100;
    if (amount <= 0) return;
    const int healingPotency = (std::min)(CombatMath::Balance::MaximumHealingPotency,
        getHealingPotency());
    const int healingReceived = (std::min)(CombatMath::Balance::MaximumHealingPotency,
        totalStats_.healingReceived);
    amount = amount * (100 + healingPotency) / 100;
    amount = amount * (100 + healingReceived) / 100;
    hp_ += amount;
    if (hp_ > maxHp_) hp_ = maxHp_;
}

void Player::RestoreHealth(int amount) {
    hp_ = (std::max)(0, (std::min)(maxHp_, amount));
}

int Player::GetBarrier() const { return barrier_; }

int Player::AddBarrier(int amount, int duration) {
    if (amount <= 0) return barrier_;
    amount = amount * (100 + (std::min)(CombatMath::Balance::MaximumBarrierPotency, totalStats_.barrierPotency)) / 100;
    barrier_ += amount;
    if (duration > 0) statuses_.push_back({StatusType::Barrier, duration, amount, 0});
    return barrier_;
}

int Player::AbsorbDamage(int amount) {
    if (amount <= 0) return 0;
    const int absorbed = (std::min)(barrier_, amount);
    barrier_ -= absorbed;
    return amount - absorbed;
}

void Player::ReduceBarrier(int amount) {
    if (amount > 0) barrier_ = (std::max)(0, barrier_ - amount);
}

bool Player::isAlive() const { return hp_ > 0; }
const std::string& Player::getName() const { return name_; }
void Player::SetName(const std::string& name) {
    if (!name.empty()) name_ = name;
}
int Player::getHP() const { return hp_; }
int Player::getMaxHP() const { return maxHp_; }
int Player::getAttack() const {
    return (std::max)(1, totalStats_.atk * (100 + SumStatusBonus(StatusType::AttackUp) - SumStatusBonus(StatusType::AttackDown)) / 100);
}
int Player::getDefense() const {
    return (std::max)(0, totalStats_.def * (100 + SumStatusBonus(StatusType::DefenseUp) - SumStatusBonus(StatusType::DefenseDown)) / 100);
}
int Player::getSP() const { return sp_; }
int Player::getSpeed() const {
    int slow = SumStatusBonus(StatusType::Slow);
    const int speedUp = SumStatusBonus(StatusType::SpeedUp);
    return (std::max)(1, totalStats_.spd * (100 + speedUp - (std::min)(75, slow)) / 100);
}
int Player::getAccuracy() const { return (std::max)(0, totalStats_.acc + SumStatusBonus(StatusType::AccuracyUp) - SumStatusBonus(StatusType::AccuracyDown)); }
int Player::getResistance() const { return (std::min)(100, totalStats_.res); }
int Player::getCritRate() const { return (std::max)(0, (std::min)(100, totalStats_.critRate + SumStatusBonus(StatusType::CritUp))); }
int Player::getCritDamage() const { return (std::max)(0, totalStats_.critDamage + SumStatusBonus(StatusType::CritDamageUp)); }
int Player::getEvasion() const { return totalStats_.evasion; }
int Player::getHealingPotency() const { return totalStats_.healingPotency + SumStatusBonus(StatusType::HealingPotencyUp); }
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
    baseStats_.hp = 100 + levelOffset * 25;
    baseStats_.def = 10 + levelOffset * 10;
    baseStats_.atk = 20 + levelOffset * 15;
    baseStats_.spd = CombatMath::Balance::BasePlayerSpeed;
    RecalculateStats();
    maxHp_ = totalStats_.hp;
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
        const bool hasCompleteSet = pieces == 5;
        if (hasCompleteSet) activeSets.push_back(item->getSetName());
    }
    return activeSets;
}

bool Player::UseConsumable(const std::string& itemName, Enemy* target) {
    Item* item = inventory_.FindItem(itemName);
    if (item == nullptr || item->getCategory() != ItemCategory::Consumable) return false;
    const std::string& effect = item->getEffect();
    if (effect.find("restore_35") != std::string::npos) {
        heal(maxHp_ * 35 / 100);
        statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
            return status.type == StatusType::Bleed;
        }), statuses_.end());
    }
    else if (effect.find("restore_") == 0) {
        const std::size_t start = 8;
        const std::size_t end = effect.find('_', start);
        const int percent = end == std::string::npos ? 0 : std::stoi(effect.substr(start, end - start));
        if (percent <= 0) return false;
        heal(maxHp_ * percent / 100);
    }
    else if (effect == "remove_all_debuffs") CleanseNegativeStatuses();
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

int Player::CleanseNegativeStatuses() {
    const auto oldSize = statuses_.size();
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
        return status.type == StatusType::Stun || status.type == StatusType::Bleed || status.type == StatusType::Burn ||
            status.type == StatusType::Poison || status.type == StatusType::Slow || status.type == StatusType::VerdigrisMark ||
            status.type == StatusType::AttackDown || status.type == StatusType::DefenseDown || status.type == StatusType::AccuracyDown;
    }), statuses_.end());
    return static_cast<int>(oldSize - statuses_.size());
}

int Player::SumStatusBonus(StatusType type) const {
    int bonus = 0;
    for (const auto& status : statuses_) if (status.type == type) bonus += status.potency;
    return bonus;
}

bool Player::HasStatus(StatusType type) const {
    for (const auto& status : statuses_) if (status.type == type) return true;
    return false;
}

void Player::ApplyStatus(const StatusEffect& status, int accuracy) {
    StatusEffect adjusted = status;
    if (adjusted.type == StatusType::Stun || adjusted.type == StatusType::Bleed || adjusted.type == StatusType::Burn ||
        adjusted.type == StatusType::Poison || adjusted.type == StatusType::Slow || adjusted.type == StatusType::VerdigrisMark ||
        adjusted.type == StatusType::AttackDown || adjusted.type == StatusType::DefenseDown || adjusted.type == StatusType::AccuracyDown) {
        const int resistance = (std::min)(CombatMath::Balance::MaximumResistance, totalStats_.res);
        const unsigned roll = static_cast<unsigned>((hp_ * 17 + maxHp_ * 31 + static_cast<int>(adjusted.type) * 13) % 100);
        if (roll >= static_cast<unsigned>(CombatMath::AccuracyChance(100, accuracy, resistance))) return;
        adjusted.turns = (std::max)(1, adjusted.turns * (100 - (std::min)(CombatMath::Balance::MaximumDebuffReduction, totalStats_.debuffDurationReduction)) / 100);
    }
    if (GetStatusStackPolicy(adjusted.type) != StackPolicy::IndependentStacks) {
        for (auto& existing : statuses_) {
            if (existing.type == adjusted.type) {
                existing = adjusted;
                return;
            }
        }
    }
    statuses_.push_back(adjusted);
}

void Player::ProcessStatuses() {
    int expiredBarrier = 0;
    for (auto& status : statuses_) {
        const int dotReduction = (std::min)(75, SumStatusBonus(StatusType::DotReduction));
        const int dotUp = SumStatusBonus(StatusType::DotUp);
        if (status.type == StatusType::Bleed) takeDamage((std::max)(1, status.sourceAttack * 30 * (100 + status.dotPotency + dotUp) * (100 - dotReduction) / 1000000));
        else if (status.type == StatusType::Burn) takeDamage((std::max)(1, status.sourceAttack * 60 * (100 + status.dotPotency + dotUp) * (100 - dotReduction) / 1000000));
        else if (status.type == StatusType::Poison) takeDamage((std::max)(1, maxHp_ * 5 * (100 + status.dotPotency + dotUp) * (100 - dotReduction) / 1000000));
        else if (status.type == StatusType::HealOverTime) heal(status.potency);
        --status.turns;
        if (status.type == StatusType::Barrier && status.turns <= 0) expiredBarrier += status.potency;
    }
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
        return status.turns <= 0;
    }), statuses_.end());
    if (expiredBarrier > 0) ReduceBarrier(expiredBarrier);
}

void Player::ClearStatuses() {
    statuses_.clear();
    barrier_ = 0;
}

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
        if (item->getMainStat() == "hp") totalStats_.hp += item->getMainStatValue();
        else if (item->getMainStat() == "def") totalStats_.def += item->getMainStatValue();
        else if (item->getMainStat() == "spd") totalStats_.spd += item->getMainStatValue();
        else if (item->getMainStat() == "atk") totalStats_.atk += item->getMainStatValue();
        for (const auto& substat : item->getSubstats()) {
            int baseValue = 0;
            int* totalValue = nullptr;
            switch (substat.first) {
            case ItemStat::HP: baseValue = baseStats_.hp; totalValue = &totalStats_.hp; break;
            case ItemStat::DEF: baseValue = baseStats_.def; totalValue = &totalStats_.def; break;
            case ItemStat::SPD: baseValue = baseStats_.spd; totalValue = &totalStats_.spd; break;
            case ItemStat::ATK: baseValue = baseStats_.atk; totalValue = &totalStats_.atk; break;
            case ItemStat::Accuracy: baseValue = 100; totalValue = &totalStats_.acc; break;
            case ItemStat::Resistance: baseValue = 100; totalValue = &totalStats_.res; break;
            case ItemStat::CritRate: baseValue = 100; totalValue = &totalStats_.critRate; break;
            case ItemStat::CritDamage: baseValue = 100; totalValue = &totalStats_.critDamage; break;
            case ItemStat::Evasion: baseValue = 100; totalValue = &totalStats_.evasion; break;
            case ItemStat::HealingPotency: totalValue = &totalStats_.healingPotency; break;
            case ItemStat::BarrierPotency: totalValue = &totalStats_.barrierPotency; break;
            case ItemStat::DotPotency: totalValue = &totalStats_.dotPotency; break;
            case ItemStat::Lifesteal: totalValue = &totalStats_.lifesteal; break;
            case ItemStat::DebuffDurationReduction: totalValue = &totalStats_.debuffDurationReduction; break;
            }
            if (totalValue != nullptr) *totalValue += baseValue > 0 ? baseValue * substat.second / 100 : substat.second;
        }
        const int percent = item->getMainStatPercent();
        if (percent > 0) {
            if (item->getMainStat() == "hp") totalStats_.hpPercent += percent;
            else if (item->getMainStat() == "def") totalStats_.defPercent += percent;
            else if (item->getMainStat() == "spd") totalStats_.spdPercent += percent;
            else if (item->getMainStat() == "atk") totalStats_.atkPercent += percent;
            else if (item->getMainStat() == "acc") totalStats_.accuracyPercent += percent;
            else if (item->getMainStat() == "res") totalStats_.resistancePercent += percent;
            else if (item->getMainStat() == "crit") totalStats_.critRatePercent += percent;
        }
    }

    for (const std::string& setName : GetActiveSets()) {
        if (setName == "Verdigris Matron") {
            totalStats_.healingPotency += 100;
            totalStats_.lifesteal += 50;
            totalStats_.barrierPotency += 25;
        } else if (setName == "Shimmer Reliquary") {
            totalStats_.barrierPotency += 75;
            totalStats_.healingReceived += 50;
            totalStats_.critRate += 25;
        } else if (setName == "Oracle Automaton") {
            totalStats_.atk += 50;
            totalStats_.acc += 30;
            totalStats_.critDamage += 75;
        } else if (setName == "Shimmer Ascendant") {
            totalStats_.atk += 15;
            totalStats_.acc += 15;
        } else if (setName == "Automaton Overlord") {
            totalStats_.acc += 20;
            totalStats_.critDamage += 20;
        }
    }
    totalStats_.hp = CombatMath::ApplyPercent(totalStats_.hp, totalStats_.hpPercent);
    totalStats_.def = CombatMath::ApplyPercent(totalStats_.def, totalStats_.defPercent);
    totalStats_.spd += totalStats_.spdPercent;
    totalStats_.atk = CombatMath::ApplyPercent(totalStats_.atk, totalStats_.atkPercent);
    totalStats_.acc = CombatMath::ClampPercent(totalStats_.acc + totalStats_.accuracyPercent);
    totalStats_.res = CombatMath::ClampPercent(totalStats_.res + totalStats_.resistancePercent);
    totalStats_.critRate = CombatMath::ClampPercent(totalStats_.critRate + totalStats_.critRatePercent);
    totalStats_.critDamage = CombatMath::ApplyPercent(totalStats_.critDamage, totalStats_.critDamagePercent);
    maxHp_ = totalStats_.hp;
    if (hp_ > maxHp_) hp_ = maxHp_;
}
