#include "Enemy.h"
#include "CombatMath.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
int ScaleStat(int base, double growth, int level, int cap) {
	const int relativeLevel = (std::max)(0, (std::min)(100, level) - 1);
	const double levelScale = 0.35 + 0.65 * (relativeLevel / 10.0);
	const int scaled = static_cast<int>(std::lround(base * levelScale * std::pow(growth, relativeLevel / 10.0)));
	return (std::max)(1, (std::min)(cap, scaled));
}

int NormalizeSpeed(int legacySpeed, EnemyFaction faction) {
	const int factionAdjustment = faction == EnemyFaction::Automata ? 5 :
		faction == EnemyFaction::Devotee ? -3 : 0;
	return (std::max)(CombatMath::Balance::MinimumEnemySpeed,
		(std::min)(CombatMath::Balance::MaximumEnemySpeed, 75 + legacySpeed * 35 / 100 + factionAdjustment));
}

}

void Enemy::heal(int amount) {
	if (amount <= 0) return;
	for (const auto& status : statuses_) if (status.type == StatusType::HealResistance) amount = amount * (100 - status.potency) / 100;
	hp_ = (std::min)(maxHp_, hp_ + amount);
}

std::vector<std::pair<StatusType, int>>& Enemy::EmptyDamageEvents() {
	static std::vector<std::pair<StatusType, int>> events;
	return events;
}

int& Enemy::EmptyHealing() {
	static int healing = 0;
	return healing;
}

Enemy::Enemy(const EnemyDefinition& definition, int level)
	: name_(definition.name),
	  hp_(ScaleStat(definition.hpBase, definition.tier == EnemyTier::Boss ? 1.45 : definition.tier == EnemyTier::Elite ? 1.35 : 1.25, level, 250000)),
	  maxHp_(hp_),
	  atk_(ScaleStat(definition.atkBase, definition.tier == EnemyTier::Boss ? 1.22 : definition.tier == EnemyTier::Elite ? 1.18 : 1.14, level, 5000)),
	  xpReward_(definition.xpReward * (std::max)(1, level)),
	  defense_(ScaleStat(definition.defBase, definition.tier == EnemyTier::Boss ? 1.22 : definition.tier == EnemyTier::Elite ? 1.18 : 1.14, level, 5000)),
	  speed_(NormalizeSpeed(definition.speed, definition.faction)), level_(level), resistance_(0), evasion_(0),
	  critRate_(5), critDamage_(150), accuracy_(100),
	  faction_(definition.faction), tier_(definition.tier), skills_(definition.skills),
		phase_(1) {
	skillCooldowns_.assign(skills_.size(), 0);
}

const std::vector<StatusEffect>& Enemy::GetStatuses() const { return statuses_; }

Enemy::~Enemy() = default;

void Enemy::takeDamage(int amount) {
	if (amount <= 0) return;
	hp_ -= amount;
	if (hp_ < 0) hp_ = 0;
}

void Enemy::ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing) {
	healing = 0;
	for (auto& status : statuses_) {
		int damage = 0;
		if (status.type == StatusType::Bleed) damage = status.sourceAttack * 30 * (100 + status.dotPotency) / 10000;
		else if (status.type == StatusType::Burn) damage = status.sourceAttack * 60 * (100 + status.dotPotency) / 10000;
		else if (status.type == StatusType::Poison) damage = maxHp_ * 5 * (100 + status.dotPotency) / 10000;
		else if (status.type == StatusType::HealOverTime) healing += status.potency;
		if (damage > 0) {
			takeDamage(damage);
			damageEvents.emplace_back(status.type, damage);
		}
		--status.turns;
	}
	statuses_.erase(std::remove_if(statuses_.begin(), statuses_.end(), [](const StatusEffect& status) {
		return status.turns <= 0;
	}), statuses_.end());
	if (healing > 0) heal(healing);
}

bool Enemy::isAlive() const { return hp_ > 0; }
const std::string& Enemy::getName() const { return name_; }
int Enemy::getHP() const { return hp_; }
int Enemy::getMaxHP() const { return maxHp_; }
int Enemy::SumStatusBonus(StatusType type) const {
	int bonus = 0;
	for (const auto& status : statuses_) if (status.type == type) bonus += status.potency;
	return bonus;
}
int Enemy::getAtk() const { return atk_ * (100 + SumStatusBonus(StatusType::AttackUp)) / 100; }
int Enemy::getXPReward() const { return xpReward_; }
int Enemy::getDefense() const { return defense_; }
int Enemy::getSpeed() const {
	const int slow = SumStatusBonus(StatusType::Slow);
	return (std::max)(1, speed_ * (100 - (std::min)(75, slow)) / 100);
}
int Enemy::getLevel() const { return level_; }
int Enemy::getResistance() const { return resistance_; }
int Enemy::getCritRate() const { return critRate_; }
int Enemy::getCritDamage() const { return critDamage_; }
int Enemy::getAccuracy() const {
	const int bonus = SumStatusBonus(StatusType::AccuracyUp);
	return (std::min)(100, accuracy_ + bonus);
}
int Enemy::getEvasion() const { return evasion_; }
EnemyFaction Enemy::getFaction() const { return faction_; }
EnemyTier Enemy::getTier() const { return tier_; }
const std::vector<EnemySkillDefinition>& Enemy::GetSkills() const { return skills_; }
int Enemy::GetSkillCooldown(std::size_t index) const {
	return index < skillCooldowns_.size() ? skillCooldowns_[index] : 0;
}
bool Enemy::IsSkillReady(std::size_t index) const { return GetSkillCooldown(index) == 0; }
void Enemy::StartSkillCooldown(std::size_t index) {
	if (index < skillCooldowns_.size() && index < skills_.size()) skillCooldowns_[index] = skills_[index].cooldown + 1;
}
void Enemy::TickSkillCooldowns() {
	for (int& cooldown : skillCooldowns_) if (cooldown > 0) --cooldown;
}
int Enemy::GetPhase() const { return phase_; }
bool Enemy::UpdatePhase() {
	if (tier_ != EnemyTier::Boss) return false;
	const int healthPercent = maxHp_ > 0 ? hp_ * 100 / maxHp_ : 0;
	const int newPhase = healthPercent <= 30 ? 3 : healthPercent <= 70 ? 2 : 1;
	if (newPhase <= phase_) return false;
	phase_ = newPhase;
	if (phase_ == 2) {
		atk_ = atk_ * 120 / 100;
		defense_ = defense_ * 120 / 100;
		speed_ = speed_ * 120 / 100;
	} else {
		atk_ = atk_ * 140 / 100;
		defense_ = defense_ * 125 / 100;
		speed_ = speed_ * 125 / 100;
	}
	return true;
}

bool Enemy::HasStatus(StatusType type) const {
	for (const auto& status : statuses_) if (status.type == type) return true;
	return false;
}

bool Enemy::ApplyStatus(const StatusEffect& status, int accuracy) {
	const unsigned roll = static_cast<unsigned>((hp_ * 17 + level_ * 31 + static_cast<int>(status.type) * 13) % 100);
	if (GetStatusStackPolicy(status.type) != StackPolicy::IndependentStacks) {
		const int hitChance = CombatMath::AccuracyChance(100, accuracy, resistance_);
		if (roll >= static_cast<unsigned>(hitChance)) return false;
	}
	if (status.type != StatusType::HealOverTime &&
		GetStatusStackPolicy(status.type) != StackPolicy::IndependentStacks) {
		for (auto& existing : statuses_) {
			if (existing.type == status.type) {
				existing = status;
				return true;
			}
		}
	}
	statuses_.push_back(status);
	return true;
}

void Enemy::ClearStatuses() { statuses_.clear(); }
