#include "Player.h"
#include "Enemy.h"
#include "CombatMath.h"
#include "EquipmentStats.h"
#include <iostream>
#include <algorithm>
#include <set>

namespace {
bool ViolatesTheme(const Item& item) {
    for (const ItemStat stat : item.getForbiddenStats()) {
        if (EquipmentStats::HasStat(item.GetStatBonuses(), stat)) return true;
    }
    return false;
}
}

Player::Player(const std::string& name, int maxHp, int maxSp)
    : name_(name), hp_(maxHp), maxHp_(maxHp), sp_(maxSp), maxSp_(maxSp),
      baseStats_{maxHp, 10, CombatMath::Balance::BasePlayerSpeed, 20, 0, 0, 5, 150},
      totalStats_(baseStats_), equipped_{nullptr, nullptr, nullptr, nullptr, nullptr} {}

Player::~Player() = default;

const std::vector<StatusApplicationResolution>& Player::GetStatusApplications() const { return statusApplications_; }
const DamageResolution& Player::GetLastDamageResult() const { return lastDamageResult_; }
const std::vector<StatusDamageResolution>& Player::GetStatusDamageResults() const { return statusDamageResults_; }

void Player::takeDamage(int amount) {
    lastDamageResult_ = {};
    lastDamageResult_.requestedDamage = (std::max)(0, amount);
    lastDamageResult_.remainingHP = hp_;
    if (amount <= 0) return;
    int damageReduction = 0;
    for (const auto& status : statuses_) {
        if (status.type == StatusType::DamageReduction) damageReduction += status.potency;
    }
    amount = amount * (100 - (std::min)(CombatMath::Balance::MaximumDamageReduction, damageReduction)) / 100;
    const int barrierBefore = barrier_;
    amount = AbsorbDamage(amount);
    lastDamageResult_.barrierAbsorbed = barrierBefore - barrier_;
    lastDamageResult_.barrierBroken = barrierBefore > 0 && barrier_ == 0;
    const int healthBefore = hp_;
    hp_ = (std::max)(0, hp_ - amount);
    lastDamageResult_.hpDamage = healthBefore - hp_;
    lastDamageResult_.remainingHP = hp_;
    if (HasActiveSet("Verdigris Matron") && healthBefore > hp_) {
        const int recovery = healthBefore - hp_;
        const int beforeHeal = hp_;
        const int beforeBarrier = barrier_;
        heal(recovery / 2);
        AddBarrier(recovery / 4);
        lastDamageResult_.healing = hp_ - beforeHeal;
        lastDamageResult_.barrierGained = barrier_ - beforeBarrier;
    }
}

bool Player::ToggleEquip(Item& item) {
    if (!item.isEquippable() || static_cast<int>(item.getSlot()) < 0 ||
        static_cast<int>(item.getSlot()) >= 5 || ViolatesTheme(item) || !inventory_.Contains(item.getInstanceId()) ||
        inventory_.FindItem(item.getInstanceId()) != &item) return false;
    const int slot = static_cast<int>(item.getSlot());
    if (equipped_[slot] == &item) {
        equipped_[slot] = nullptr;
        inventory_.UnprotectItem(item.getInstanceId());
    } else {
        if (equipped_[slot] != nullptr) inventory_.UnprotectItem(equipped_[slot]->getInstanceId());
        equipped_[slot] = &item;
        inventory_.ProtectItem(item.getInstanceId());
    }
    RecalculateStats();
    return true;
}

void Player::heal(int amount) {
    if (amount <= 0 || !isAlive()) return;
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
    int remainingAbsorbed = absorbed;
    for (auto& status : statuses_) {
        if (remainingAbsorbed <= 0) break;
        if (status.type != StatusType::Barrier) continue;
        const int fromStatus = (std::min)(status.potency, remainingAbsorbed);
        status.potency -= fromStatus;
        remainingAbsorbed -= fromStatus;
    }
    barrier_ -= absorbed;
    return amount - absorbed;
}

int Player::ApplyDamageOverTime(int amount) {
    lastDamageResult_ = {};
    lastDamageResult_.requestedDamage = (std::max)(0, amount);
    const int barrierBefore = barrier_;
    const int remaining = AbsorbDamage(amount);
    const int healthBefore = hp_;
    hp_ = (std::max)(0, hp_ - remaining);
    lastDamageResult_.barrierAbsorbed = barrierBefore - barrier_;
    lastDamageResult_.barrierBroken = barrierBefore > 0 && barrier_ == 0;
    lastDamageResult_.hpDamage = healthBefore - hp_;
    lastDamageResult_.remainingHP = hp_;
    return lastDamageResult_.hpDamage;
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
    return EffectiveStatsFor(totalStats_).atk;
}
int Player::getDefense() const {
    return EffectiveStatsFor(totalStats_).def;
}
int Player::getSP() const { return sp_; }
int Player::getSpeed() const {
    return EffectiveStatsFor(totalStats_).spd;
}
int Player::getAccuracy() const { return EffectiveStatsFor(totalStats_).acc; }
int Player::getResistance() const { return EffectiveStatsFor(totalStats_).res; }
int Player::getCritRate() const { return EffectiveStatsFor(totalStats_).critRate; }
int Player::getCritDamage() const { return EffectiveStatsFor(totalStats_).critDamage; }
int Player::getEvasion() const { return EffectiveStatsFor(totalStats_).evasion; }
int Player::getHealingPotency() const { return EffectiveStatsFor(totalStats_).healingPotency; }
int Player::getBarrierPotency() const { return EffectiveStatsFor(totalStats_).barrierPotency; }
int Player::getDotPotency() const { return EffectiveStatsFor(totalStats_).dotPotency; }
int Player::getLifesteal() const { return EffectiveStatsFor(totalStats_).lifesteal; }
int Player::getDebuffDurationReduction() const { return EffectiveStatsFor(totalStats_).debuffDurationReduction; }
int Player::getDisplayedAccuracy() const { return getAccuracy(); }
int Player::getDisplayedResistance() const { return getResistance(); }
int Player::getDisplayedCritRate() const { return getCritRate(); }
int Player::getDisplayedEvasion() const { return getEvasion(); }
const Stats& Player::getStats() const { return totalStats_; }
Stats Player::GetEffectiveStats() const { return EffectiveStatsFor(totalStats_); }
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
    if (!item.isEquippable() || static_cast<int>(item.getSlot()) < 0 ||
        static_cast<int>(item.getSlot()) >= 5 || ViolatesTheme(item) || !inventory_.Contains(item.getInstanceId()) ||
        inventory_.FindItem(item.getInstanceId()) != &item) return false;
    const int slot = static_cast<int>(item.getSlot());
    if (equipped_[slot] != nullptr) inventory_.UnprotectItem(equipped_[slot]->getInstanceId());
    equipped_[slot] = &item;
    inventory_.ProtectItem(item.getInstanceId());
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
        const bool hasCompleteSet = pieces >= 5;
        if (hasCompleteSet) activeSets.push_back(item->getSetName());
    }
    return activeSets;
}

bool Player::HasActiveSet(const std::string& setName) const {
    for (const std::string& activeSet : GetActiveSets()) {
        if (activeSet == setName) return true;
    }
    return false;
}

int Player::GetShimmerDamageMultiplierPercent() const {
    return HasActiveSet("Shimmer Ascendant") ? 125 : 100;
}

bool Player::HasBarrierAbsorptionSet() const {
    return HasActiveSet("Shimmer Reliquary");
}

void Player::StartBattleEffects() {
    if (HasActiveSet("Choir of Verdigris")) AddBarrier(maxHp_ / 4);
}

int Player::CleanseNegativeStatuses() {
    const auto oldSize = statuses_.size();
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
        return status.type == StatusType::Stun || status.type == StatusType::Bleed || status.type == StatusType::Burn ||
            status.type == StatusType::Poison || status.type == StatusType::Slow || status.type == StatusType::VerdigrisMark ||
            status.type == StatusType::AttackDown || status.type == StatusType::DefenseDown || status.type == StatusType::AccuracyDown ||
            status.type == StatusType::HealResistance || status.type == StatusType::DotReduction;
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

bool Player::ApplyStatus(const StatusEffect& status, int accuracy) {
    statusApplications_.push_back({status, false});
    StatusEffect adjusted = status;
    if (adjusted.type == StatusType::Stun || adjusted.type == StatusType::Bleed || adjusted.type == StatusType::Burn ||
        adjusted.type == StatusType::Poison || adjusted.type == StatusType::Slow || adjusted.type == StatusType::VerdigrisMark ||
        adjusted.type == StatusType::AttackDown || adjusted.type == StatusType::DefenseDown || adjusted.type == StatusType::AccuracyDown) {
        const int resistance = (std::min)(CombatMath::Balance::MaximumResistance, totalStats_.res);
        const unsigned roll = static_cast<unsigned>((hp_ * 17 + maxHp_ * 31 + static_cast<int>(adjusted.type) * 13) % 100);
        if (roll >= static_cast<unsigned>(CombatMath::AccuracyChance(100, accuracy, resistance))) return false;
        adjusted.turns = (std::max)(1, adjusted.turns * (100 - (std::min)(CombatMath::Balance::MaximumDebuffReduction, totalStats_.debuffDurationReduction)) / 100);
    }
    if (GetStatusStackPolicy(adjusted.type) != StackPolicy::IndependentStacks) {
        for (auto& existing : statuses_) {
            if (existing.type == adjusted.type) {
                existing = adjusted;
                statusApplications_.back() = {adjusted, true};
                return true;
            }
        }
    }
    statuses_.push_back(adjusted);
    statusApplications_.back() = {adjusted, true};
    return true;
}

std::vector<std::pair<StatusType, int>>& Player::EmptyDamageEvents() {
    static std::vector<std::pair<StatusType, int>> events;
    events.clear();
    return events;
}

void Player::ProcessStatuses() {
    auto& damageEvents = EmptyDamageEvents();
    int& healing = EmptyHealing();
    ProcessStatuses(damageEvents, healing);
}

int& Player::EmptyHealing() {
    static int healing = 0;
    healing = 0;
    return healing;
}

void Player::ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
    statusDamageResults_.clear();
    damageEvents.clear();
    healing = 0;
    int expiredBarrier = 0;
    for (auto& status : statuses_) {
        const int dotReduction = (std::min)(75, SumStatusBonus(StatusType::DotReduction));
        const int dotUp = SumStatusBonus(StatusType::DotUp);
        int damage = 0;
        if (status.type == StatusType::Bleed) {
            damage = status.sourceAttack * 30 / 100;
            damage = damage * (100 + status.dotPotency + dotUp) / 100;
            damage = damage * (100 - dotReduction) / 100;
        } else if (status.type == StatusType::Burn) {
            damage = status.sourceAttack * 60 / 100;
            damage = damage * (100 + status.dotPotency + dotUp) / 100;
            damage = damage * (100 - dotReduction) / 100;
        } else if (status.type == StatusType::Poison) {
            damage = maxHp_ * 5 / 100;
            damage = damage * (100 + status.dotPotency + dotUp) / 100;
            damage = damage * (100 - dotReduction) / 100;
        }
        if (status.type == StatusType::HealOverTime) healing += status.potency;
        if (damage > 0) {
            ApplyDamageOverTime(damage);
            statusDamageResults_.push_back({status.type, lastDamageResult_});
            damageEvents.emplace_back(status.type, damage);
        }
        --status.turns;
        if (status.type == StatusType::Barrier && status.turns <= 0) expiredBarrier += status.potency;
    }
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
        return status.turns <= 0;
    }), statuses_.end());
    if (expiredBarrier > 0) barrier_ = (std::max)(0, barrier_ - expiredBarrier);
    const int beforeHealing = hp_;
    if (healing > 0) heal(healing);
    healing = hp_ - beforeHealing;
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
    if (static_cast<int>(slot) < 0 || static_cast<int>(slot) >= 5) return nullptr;
    return equipped_[static_cast<int>(slot)];
}

void Player::RecalculateStats() {
    const Item* equipment[5];
    for (int slot = 0; slot < 5; ++slot) equipment[slot] = equipped_[slot];
    totalStats_ = CalculateEquipmentStats(equipment);
    maxHp_ = totalStats_.hp;
    if (hp_ > maxHp_) hp_ = maxHp_;
}

Stats Player::CalculateEquipmentStats(const Item* const equipment[5]) const {
    Stats total = baseStats_;
    std::set<std::string> sets;
    for (int slot = 0; slot < 5; ++slot) {
        if (!equipment[slot]) continue;
        EquipmentStats::Add(total, equipment[slot]->GetStatBonuses());
        if (!equipment[slot]->getSetName().empty()) sets.insert(equipment[slot]->getSetName());
    }
    // Keep the existing five-piece set behavior until the separate approved
    // set/Artifact phase; previews use the same counted loadout as equipment.
    for (const auto& setName : sets) {
        int pieces = 0;
        for (int slot = 0; slot < 5; ++slot)
            if (equipment[slot] && equipment[slot]->getSetName() == setName) ++pieces;
        if (pieces < 5) continue;
        if (setName == "Choir of Verdigris") total.healingReceived += 25;
        else if (setName == "Automaton Overlord") { total.acc = 100; total.critDamage += 50; }
        else if (setName == "Verdigris Matron") { total.healingPotency += 100; total.lifesteal += 50; total.barrierPotency += 25; }
        else if (setName == "Shimmer Reliquary") { total.barrierPotency += 50; total.healingReceived += 50; total.critRate += 25; }
        else if (setName == "Oracle Automaton") { total.atk += 50; total.acc += 30; total.critDamage += 75; }
        else if (setName == "Shimmer Ascendant") { total.atk += 15; total.acc += 15; }
    }
    return EquipmentStats::Finalize(total);
}

Stats Player::EffectiveStatsFor(const Stats& stats) const {
    Stats result = stats;
    const auto clamp = [](int value, int maximum) { return (std::max)(0, (std::min)(maximum, value)); };
    result.atk = (std::max)(1, EquipmentStats::ApplyPercent(stats.atk, SumStatusBonus(StatusType::AttackUp) - SumStatusBonus(StatusType::AttackDown)));
    result.def = EquipmentStats::ApplyPercent(stats.def, SumStatusBonus(StatusType::DefenseUp) - SumStatusBonus(StatusType::DefenseDown));
    result.spd = (std::max)(1, EquipmentStats::ApplyPercent(stats.spd, SumStatusBonus(StatusType::SpeedUp) - (std::min)(75, SumStatusBonus(StatusType::Slow))));
    result.acc = clamp(EquipmentStats::SaturatingAdd(stats.acc, SumStatusBonus(StatusType::AccuracyUp) - SumStatusBonus(StatusType::AccuracyDown)), 100);
    result.res = clamp(stats.res, CombatMath::Balance::MaximumResistance);
    result.critRate = clamp(EquipmentStats::SaturatingAdd(stats.critRate, SumStatusBonus(StatusType::CritUp)), CombatMath::Balance::MaximumCritRate);
    result.critDamage = (std::max)(0, EquipmentStats::SaturatingAdd(stats.critDamage, SumStatusBonus(StatusType::CritDamageUp)));
    result.evasion = clamp(stats.evasion, CombatMath::Balance::MaximumEvasion);
    result.healingPotency = clamp(EquipmentStats::SaturatingAdd(stats.healingPotency, SumStatusBonus(StatusType::HealingPotencyUp)), CombatMath::Balance::MaximumHealingPotency);
    result.healingReceived = clamp(stats.healingReceived, CombatMath::Balance::MaximumHealingPotency);
    result.barrierPotency = clamp(stats.barrierPotency, CombatMath::Balance::MaximumBarrierPotency);
    result.dotPotency = clamp(stats.dotPotency, CombatMath::Balance::MaximumDotPotency);
    result.lifesteal = clamp(stats.lifesteal, CombatMath::Balance::MaximumLifesteal);
    result.debuffDurationReduction = clamp(stats.debuffDurationReduction, CombatMath::Balance::MaximumDebuffReduction);
    return result;
}

Stats Player::PreviewEquipment(const Item& item) const {
    if (!item.isEquippable() || static_cast<int>(item.getSlot()) < 0 || static_cast<int>(item.getSlot()) >= 5 || ViolatesTheme(item))
        return GetEffectiveStats();
    const Item* equipment[5];
    for (int slot = 0; slot < 5; ++slot) equipment[slot] = equipped_[slot];
    equipment[static_cast<int>(item.getSlot())] = &item;
    return EffectiveStatsFor(CalculateEquipmentStats(equipment));
}

Stats Player::PreviewUnequip(EquipSlot slot) const {
    if (static_cast<int>(slot) < 0 || static_cast<int>(slot) >= 5) return GetEffectiveStats();
    const Item* equipment[5];
    for (int index = 0; index < 5; ++index) equipment[index] = equipped_[index];
    equipment[static_cast<int>(slot)] = nullptr;
    return EffectiveStatsFor(CalculateEquipmentStats(equipment));
}
