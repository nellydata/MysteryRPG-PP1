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
    : name_(name), hp_((std::max)(1, maxHp)), maxHp_(hp_),
      sp_((std::max)(0, maxSp)), maxSp_(sp_),
      baseStats_{hp_, 10, CombatMath::Balance::BasePlayerSpeed, 20, 0, 0, 5, 150},
      totalStats_(baseStats_), equipped_{nullptr, nullptr, nullptr, nullptr, nullptr} {}

Player::~Player() = default;

const std::vector<StatusApplicationResolution>& Player::GetStatusApplications() const { return statusApplications_; }
const DamageResolution& Player::GetLastDamageResult() const { return lastDamageResult_; }
const std::vector<StatusDamageResolution>& Player::GetStatusDamageResults() const { return statusDamageResults_; }

void Player::takeDamage(int amount) {
    lastDamageResult_ = StatusRules::Damage(hp_, statuses_, amount, 0,
        (std::min)(CombatMath::Balance::MaximumDamageReduction, SumStatusBonus(StatusType::DamageReduction)));
    barrier_ = StatusRules::Barriers(statuses_);
    if (isAlive() && HasActiveSet("Verdigris Matron") && lastDamageResult_.hpDamage > 0) {
        const int recovery = lastDamageResult_.hpDamage;
        const int beforeHeal = hp_;
        const int beforeBarrier = barrier_;
        heal(recovery / 2);
        AddBarrier(recovery / 4);
        lastDamageResult_.healing = hp_ - beforeHeal;
        lastDamageResult_.barrierGained = barrier_ - beforeBarrier;
        lastDamageResult_.remainingHP = hp_;
    }
}

const DamageResolution& Player::ApplyDirectDamage(int rawDamage, bool ignoreDefense) {
    const int requested = (std::max)(0, rawDamage);
    const int afterDefense = ignoreDefense ? requested : CombatMath::Mitigate(requested, getDefense());
    takeDamage(afterDefense);
    if (lastDamageResult_.hpDamage || lastDamageResult_.barrierAbsorbed || isAlive())
        lastDamageResult_.mitigatedDamage += requested - afterDefense;
    lastDamageResult_.requestedDamage = requested;
    return lastDamageResult_;
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
    lastHealResult_ = StatusRules::Heal(hp_, maxHp_, amount, SumStatusBonus(StatusType::HealResistance),
        getHealingPotency(), totalStats_.healingReceived);
}

void Player::RestoreHealth(int amount) {
    hp_ = (std::max)(0, (std::min)(maxHp_, amount));
}

int Player::GetBarrier() const { return barrier_; }

int Player::AddBarrier(int amount, int duration) {
    if (amount <= 0 || !isAlive() || duration < 0) return barrier_;
    amount = CombatMath::Scale(amount, 100 + getBarrierPotency());
    ApplyStatus({StatusType::Barrier, duration, amount, 0, 0, 0, getName(), GetCombatantId()});
    return barrier_;
}

int Player::AbsorbDamage(int amount) {
    const int remaining = StatusRules::Absorb(statuses_, amount);
    barrier_ = StatusRules::Barriers(statuses_);
    return remaining;
}

int Player::ApplyDamageOverTime(int amount) {
    lastDamageResult_ = StatusRules::Damage(hp_, statuses_, amount, 0,
        (std::min)(75, SumStatusBonus(StatusType::DotReduction)));
    barrier_ = StatusRules::Barriers(statuses_);
    return lastDamageResult_.hpDamage;
}

void Player::ReduceBarrier(int amount) {
    if (amount > 0) (void)AbsorbDamage(amount);
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
        const int beforeBase = baseStats_.hp;
        const int beforeHP = hp_;
        baseStats_.hp = CombatMath::Saturate(static_cast<std::int64_t>(baseStats_.hp) + amount);
        RecalculateStats();
        if (beforeHP > 0) hp_ = (std::min)(maxHp_, CombatMath::Saturate(
            static_cast<std::int64_t>(beforeHP) + baseStats_.hp - beforeBase));
    }
}

void Player::increaseAttack(int amount) {
    if (amount > 0) { baseStats_.atk = CombatMath::Saturate(static_cast<std::int64_t>(baseStats_.atk) + amount); RecalculateStats(); }
}

void Player::increaseDefense(int amount) {
    if (amount > 0) { baseStats_.def = CombatMath::Saturate(static_cast<std::int64_t>(baseStats_.def) + amount); RecalculateStats(); }
}

void Player::SetLevelStats(int level) {
    const int levelOffset = (std::clamp)(level, 1, 100) - 1;
    const int previousMaxHp = maxHp_;
    const int previousHP = hp_;
    baseStats_.hp = 100 + levelOffset * 25;
    baseStats_.def = 10 + levelOffset * 10;
    baseStats_.atk = 20 + levelOffset * 15;
    baseStats_.spd = CombatMath::Balance::BasePlayerSpeed;
    RecalculateStats();
    hp_ = previousHP > 0 && maxHp_ > 0 ? (std::clamp)(CombatMath::Saturate(
        static_cast<std::int64_t>(previousHP) + maxHp_ - previousMaxHp), 1, maxHp_) : 0;
}

void Player::spendSP(int amount) {
    if (amount <= 0) return;
    sp_ -= amount;
    if (sp_ < 0) sp_ = 0;
}

void Player::gainSP(int amount) {
    if (amount <= 0) return;
    sp_ = (std::min)(maxSp_, CombatMath::Saturate(static_cast<std::int64_t>(sp_) + amount));
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
    return StatusRules::Cleanse(statuses_);
}

int Player::SumStatusBonus(StatusType type) const {
    return StatusRules::Sum(statuses_, type);
}

bool Player::HasStatus(StatusType type) const {
    for (const auto& status : statuses_) if (status.type == type) return true;
    return false;
}

bool Player::ApplyStatus(const StatusEffect& status, int accuracy) {
    const bool applied = StatusRules::Apply(statuses_, statusApplications_, status, isAlive(),
        accuracy, getResistance(), getDebuffDurationReduction(), statusRandom_);
    barrier_ = StatusRules::Barriers(statuses_);
    return applied;
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

void Player::BeginStatusTurn(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
    if (statusTurnOpen_) return;
    statusTurnOpen_ = true;
    statusTurnSnapshot_.clear();
    statusDamageResults_.clear();
    damageEvents.clear();
    healing = 0;
    const auto active = statuses_; // Mutation-safe source snapshots, including Mark.
    for (const auto& status : active) if (status.turns > 0) statusTurnSnapshot_.push_back(status.applicationId);
    for (const auto& status : active) {
        if (!isAlive()) break;
        const int damage = StatusRules::DotDamage(status, maxHp_, active);
        if (damage > 0) {
            ApplyDamageOverTime(damage);
            statusDamageResults_.push_back({status.type, lastDamageResult_, status.source, status.sourceId});
            damageEvents.emplace_back(status.type, lastDamageResult_.hpDamage);
        }
        if (status.type == StatusType::HealOverTime) healing = CombatMath::Saturate(static_cast<std::int64_t>(healing) + status.potency);
    }
    heal(healing);
    healing = lastHealResult_.hpRestored;
}

void Player::EndStatusTurn() {
    if (!statusTurnOpen_) return;
    StatusRules::FinishTurn(statuses_, statusTurnSnapshot_);
    barrier_ = StatusRules::Barriers(statuses_);
    statusTurnOpen_ = false;
}

void Player::ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
    BeginStatusTurn(damageEvents, healing);
    EndStatusTurn();
}
void Player::ClearStatuses() {
    statuses_.clear();
    barrier_ = 0;
    statusTurnSnapshot_.clear();
    statusTurnOpen_ = false;
}

void Player::ModifyStats(const Stats& delta) {
    EquipmentStats::Add(baseStats_, delta);
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
        Stats bonus;
        if (setName == "Choir of Verdigris") bonus.healingReceived = 25;
        else if (setName == "Automaton Overlord") { total.acc = 100; bonus.critDamage = 50; }
        else if (setName == "Verdigris Matron") { bonus.healingPotency = 100; bonus.lifesteal = 50; bonus.barrierPotency = 25; }
        else if (setName == "Shimmer Reliquary") { bonus.barrierPotency = 50; bonus.healingReceived = 50; bonus.critRate = 25; }
        else if (setName == "Oracle Automaton") { bonus.atk = 50; bonus.acc = 30; bonus.critDamage = 75; }
        else if (setName == "Shimmer Ascendant") { bonus.atk = 15; bonus.acc = 15; }
        EquipmentStats::Add(total, bonus);
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
    result.dotPotency = clamp(EquipmentStats::SaturatingAdd(stats.dotPotency, SumStatusBonus(StatusType::DotUp)), CombatMath::Balance::MaximumDotPotency);
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
