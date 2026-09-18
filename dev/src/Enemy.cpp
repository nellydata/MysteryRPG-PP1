#include "Enemy.h"
#include "CombatMath.h"
#include "ProgressionManager.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
int ScaleStat(int base, double growth, int level, int cap) {
	const int relativeLevel = (std::max)(0, (std::min)(100, level) - 1);
	// Match the player's linear early stat growth, with gradual later growth.
	// The 0.5 coefficient keeps Rare/Epic enemies killable in 4 player actions
	// through the early progression window (levels 1-10) while preserving the
	// intended tier identity at higher levels.
	const double levelScale = 0.35 * (1.0 + 0.5 * relativeLevel);
	const int scaled = static_cast<int>(std::lround((std::clamp)(
        base * levelScale * std::pow(growth, relativeLevel / 25.0), 1.0, static_cast<double>(cap))));
	return (std::max)(1, (std::min)(cap, scaled));
}

int NormalizeSpeed(int legacySpeed, EnemyFaction faction) {
	const int factionAdjustment = faction == EnemyFaction::Automata ? 5 :
		faction == EnemyFaction::Devotee ? -3 : 0;
	return (std::max)(CombatMath::Balance::MinimumEnemySpeed,
		(std::min)(CombatMath::Balance::MaximumEnemySpeed, CombatMath::Saturate(
            75LL + static_cast<std::int64_t>(legacySpeed) * 35 / 100 + factionAdjustment)));
}

}

void Enemy::heal(int amount) {
    lastHealResult_ = StatusRules::Heal(hp_, maxHp_, amount, SumStatusBonus(StatusType::HealResistance),
        SumStatusBonus(StatusType::HealingPotencyUp), 0);
}

std::vector<std::pair<StatusType, int>>& Enemy::EmptyDamageEvents() {
	static std::vector<std::pair<StatusType, int>> events;
	return events;
}

int& Enemy::EmptyHealing() {
	static int healing = 0;
	return healing;
}

Enemy::Enemy(const EnemyDefinition& definition, int level, bool firstClearBoss,
             bool farmBoss, std::int64_t xpOverride)
	: name_(definition.name),
	  hp_(ScaleStat(definition.hpBase, definition.tier == EnemyTier::Boss ? 1.45 : definition.tier == EnemyTier::Elite ? 1.35 : 1.25, level, 250000)),
	  maxHp_(hp_),
	  atk_(ScaleStat(definition.atkBase, definition.tier == EnemyTier::Boss ? 1.22 : definition.tier == EnemyTier::Elite ? 1.18 : 1.14, level, 5000)),
	  xpReward_(xpOverride >= 0 ? xpOverride : CalculateEnemyXP(level, static_cast<int>(definition.tier))),
	  defense_(ScaleStat(definition.defBase, definition.tier == EnemyTier::Boss ? 1.22 : definition.tier == EnemyTier::Elite ? 1.18 : 1.14, level, 5000)),
	  speed_(NormalizeSpeed(definition.speed, definition.faction)),
	  phaseBaseAtk_(atk_), phaseBaseDefense_(defense_), phaseBaseSpeed_(speed_),
	  level_((std::max)(1, (std::min)(100, level))), resistance_(0), evasion_(0),
	  critRate_(5), critDamage_(150), accuracy_(100),
	  faction_(definition.faction), tier_(definition.tier), skills_(definition.skills),
		phase_(1), xpRewardClaimed_(false),
		lootRewardClaimed_(false), firstClearBoss_(firstClearBoss), farmBoss_(farmBoss) {
	skillCooldowns_.assign(skills_.size(), 0);
}

const std::vector<StatusEffect>& Enemy::GetStatuses() const { return statuses_; }

Enemy::~Enemy() = default;

void Enemy::takeDamage(int amount) {
    lastDamageResult_ = StatusRules::Damage(hp_, statuses_, amount, 0,
        (std::min)(CombatMath::Balance::MaximumDamageReduction, SumStatusBonus(StatusType::DamageReduction)));
}
const DamageResolution& Enemy::ApplyDirectDamage(int rawDamage, bool ignoreDefense) {
    const int requested = (std::max)(0, rawDamage);
    const int afterDefense = ignoreDefense ? requested : CombatMath::Mitigate(requested, getDefense());
    takeDamage(afterDefense);
    if (lastDamageResult_.hpDamage || lastDamageResult_.barrierAbsorbed || isAlive())
        lastDamageResult_.mitigatedDamage += requested - afterDefense;
    lastDamageResult_.requestedDamage = requested;
    return lastDamageResult_;
}
int Enemy::ApplyDamageOverTime(int amount) {
    lastDamageResult_ = StatusRules::Damage(hp_, statuses_, amount, 0,
        (std::min)(75, SumStatusBonus(StatusType::DotReduction)));
    return lastDamageResult_.hpDamage;
}
std::vector<StatusDamageResolution> Enemy::DetonateDots(std::uint64_t sourceId) {
    std::vector<StatusDamageResolution> results;
    if (!isAlive()) return results;
    const auto active = statuses_;
    for (const auto& status : active) {
        if (!isAlive()) break;
        if (status.sourceId != sourceId || (status.type != StatusType::Bleed &&
            status.type != StatusType::Burn && status.type != StatusType::Poison)) continue;
        const int tick = StatusRules::DotDamage(status, maxHp_, active);
        for (int remaining = 0; remaining < status.turns && isAlive(); ++remaining) {
            ApplyDamageOverTime(tick);
            results.push_back({status.type, lastDamageResult_, status.source, status.sourceId});
        }
    }
    statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [&](const StatusEffect& status) {
        return status.sourceId == sourceId && (status.type == StatusType::Bleed ||
            status.type == StatusType::Burn || status.type == StatusType::Poison);
    }), statuses_.end());
    return results;
}
void Enemy::BeginStatusTurn(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
    if (statusTurnOpen_) return;
    statusTurnOpen_ = true;
    statusTurnSnapshot_.clear();
    statusDamageResults_.clear();
    damageEvents.clear();
    healing = 0;
    const auto active = statuses_;
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
void Enemy::EndStatusTurn() {
    if (!statusTurnOpen_) return;
    StatusRules::FinishTurn(statuses_, statusTurnSnapshot_);
    statusTurnOpen_ = false;
}
void Enemy::ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
    BeginStatusTurn(damageEvents, healing);
    EndStatusTurn();
}
bool Enemy::isAlive() const { return hp_ > 0; }
const std::string& Enemy::getName() const { return name_; }
int Enemy::getHP() const { return hp_; }
int Enemy::getMaxHP() const { return maxHp_; }
int Enemy::SumStatusBonus(StatusType type) const {
    return StatusRules::Sum(statuses_, type);
}
int Enemy::getAtk() const { return (std::max)(1, CombatMath::Scale(atk_, CombatMath::Saturate(100LL + SumStatusBonus(StatusType::AttackUp) - SumStatusBonus(StatusType::AttackDown)))); }
std::int64_t Enemy::getXPReward() const { return xpReward_; }
bool Enemy::CanReceiveDefeatRewards() const { return !isAlive() && !rewardsForfeited_; }
bool Enemy::HasClaimedXPReward() const { return xpRewardClaimed_; }
bool Enemy::HasClaimedLootReward() const { return lootRewardClaimed_; }
void Enemy::ForfeitRewards() { rewardsForfeited_ = true; }
bool Enemy::ClaimXPReward() {
	if (!CanReceiveDefeatRewards() || xpRewardClaimed_) return false;
	xpRewardClaimed_ = true;
	return true;
}
bool Enemy::ClaimLootReward() {
	if (!CanReceiveDefeatRewards() || lootRewardClaimed_) return false;
	lootRewardClaimed_ = true;
	return true;
}
bool Enemy::IsFirstClearBoss() const { return firstClearBoss_; }
bool Enemy::IsFarmBoss() const { return farmBoss_; }
const DamageResolution& Enemy::GetLastDamageResult() const { return lastDamageResult_; }
const std::vector<StatusDamageResolution>& Enemy::GetStatusDamageResults() const { return statusDamageResults_; }
const std::vector<StatusApplicationResolution>& Enemy::GetStatusApplications() const { return statusApplications_; }
int Enemy::getDefense() const { return CombatMath::Scale(defense_, CombatMath::Saturate(100LL + SumStatusBonus(StatusType::DefenseUp) - SumStatusBonus(StatusType::DefenseDown))); }
int Enemy::getSpeed() const {
	const int slow = SumStatusBonus(StatusType::Slow);
    const int ordinary = CombatMath::Scale(speed_, CombatMath::Saturate(100LL +
        SumStatusBonus(StatusType::SpeedUp) - (std::min)(75, slow)));
    return (std::max)(1, artifactSpeedSuppressed_ ? CombatMath::Scale(ordinary, 75) : ordinary);
}
int Enemy::getLevel() const { return level_; }
int Enemy::getResistance() const { return resistance_; }
int Enemy::getCritRate() const { return (std::min)(100, CombatMath::Saturate(static_cast<std::int64_t>(critRate_) + SumStatusBonus(StatusType::CritUp))); }
int Enemy::getCritDamage() const { return CombatMath::Saturate(static_cast<std::int64_t>(critDamage_) + SumStatusBonus(StatusType::CritDamageUp)); }
int Enemy::getAccuracy() const {
	const int bonus = SumStatusBonus(StatusType::AccuracyUp) - SumStatusBonus(StatusType::AccuracyDown);
	return CombatMath::Saturate(static_cast<std::int64_t>(accuracy_) + bonus);
}
int Enemy::getEvasion() const { return evasion_; }
EnemyFaction Enemy::getFaction() const { return faction_; }
EnemyTier Enemy::getTier() const { return tier_; }
const std::vector<EnemySkillDefinition>& Enemy::GetSkills() const { return skills_; }
int Enemy::GetSkillCooldown(std::size_t index) const {
	return index < skillCooldowns_.size() ? skillCooldowns_[index] : 0;
}
bool Enemy::IsSkillReady(std::size_t index) const { return index < skills_.size() && GetSkillCooldown(index) == 0; }
void Enemy::StartSkillCooldown(std::size_t index) {
	if (index < skillCooldowns_.size() && index < skills_.size()) skillCooldowns_[index] = skills_[index].cooldown;
}
void Enemy::TickSkillCooldowns() {
	for (int& cooldown : skillCooldowns_) if (cooldown > 0) --cooldown;
}
int Enemy::GetPhase() const { return phase_; }
bool Enemy::UpdatePhase() {
	if (!isAlive() || tier_ != EnemyTier::Boss) return false;
	const int healthPercent = maxHp_ > 0 ? hp_ * 100 / maxHp_ : 0;
	const int newPhase = healthPercent <= 30 ? 3 : healthPercent <= 70 ? 2 : 1;
	if (newPhase <= phase_) return false;
	phase_ = newPhase;
	const int multiplier = phase_ == 2 ? 120 : 140;
	atk_ = phaseBaseAtk_ * multiplier / 100;
	defense_ = phaseBaseDefense_ * multiplier / 100;
	speed_ = phaseBaseSpeed_ * multiplier / 100;
	return true;
}

bool Enemy::HasStatus(StatusType type) const {
	for (const auto& status : statuses_) if (status.type == type) return true;
	return false;
}

bool Enemy::ApplyStatus(const StatusEffect& status, int accuracy) {
    return StatusRules::Apply(statuses_, statusApplications_, status, isAlive(), accuracy,
        getResistance(), 0, statusRandom_);
}
int Enemy::CleanseNegativeStatuses() { return StatusRules::Cleanse(statuses_); }
void Enemy::ClearStatuses() {
    statuses_.clear();
    statusTurnSnapshot_.clear();
    statusTurnOpen_ = false;
}
