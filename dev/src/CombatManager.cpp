#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <cmath>
#include "ShimmerSkill.h"
#include "CombatMath.h"
#include <algorithm>
#include <string>
#include <memory>

namespace {
unsigned InterrogationRoll(const Enemy& enemy, int playerLevel, int attempt) {
    unsigned value = 2166136261u;
    const std::string& name = enemy.getName();
    for (unsigned char character : name) {
        value ^= character;
        value *= 16777619u;
    }

    value ^= static_cast<unsigned>(enemy.getHP() * 31 + playerLevel * 17 + attempt * 13);
    return value % 100;
}

const char* StatusDisplayName(StatusType type) {
    switch (type) {
    case StatusType::Bleed: return "Bleed";
    case StatusType::Burn: return "Burn";
    case StatusType::Poison: return "Poison";
    case StatusType::Stun: return "Stun";
    case StatusType::HealOverTime: return "Healing-over-time";
    case StatusType::Slow: return "Slow";
    case StatusType::VerdigrisMark: return "Verdigris Mark";
    case StatusType::DamageReduction: return "Damage Reduction";
    case StatusType::Barrier: return "Barrier";
    case StatusType::AttackUp: return "Attack Up";
    case StatusType::AccuracyUp: return "Accuracy Up";
    case StatusType::CritUp: return "Critical Rate Up";
    case StatusType::CritDamageUp: return "Critical Damage Up";
    case StatusType::GuaranteedCrit: return "Guaranteed Critical";
    case StatusType::DotUp: return "DOT Up";
    case StatusType::DotReduction: return "DOT Reduction";
    case StatusType::HealResistance: return "Healing Resistance";
    case StatusType::AccuracyFloor: return "Accuracy Floor";
    }
    return "Status";
}

int InterrogationChance(const Enemy& enemy, int playerLevel) {
    const int healthPercent = enemy.getMaxHP() > 0
        ? enemy.getHP() * 100 / enemy.getMaxHP()
        : 0;
    int chance = 35;

    if (healthPercent <= 25) chance += 25;
    else if (healthPercent <= 50) chance += 15;

    const std::string& type = enemy.getName();
    if (type.find("Automata") != std::string::npos) chance -= 10;
    else if (type.find("Nun") != std::string::npos) chance += 5;
    else if (type.find("Boss") != std::string::npos) chance -= 15;
    else if (type.find("Husk") != std::string::npos) chance += 10;

    chance += std::min(playerLevel - 1, 5) * 3;
    return std::max(10, std::min(chance, 85));
}

void AccumulateGaugesUntilReady(int& playerGauge, int& enemyGauge,
                                int playerSpeed, int enemySpeed) {
    for (int step = 0; step < 10000 && playerGauge < 1000 && enemyGauge < 1000; ++step) {
        playerGauge += (std::max)(1, playerSpeed);
        enemyGauge += (std::max)(1, enemySpeed);
    }
}
}

void CombatManager::AdvanceTurn(Player& player, Enemy& enemy) {
    AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
    playerTurn_ = playerGauge_ >= 1000;
    if (playerTurn_ && !groupEnemyTurn_) StartPlayerTurn(player);
}

void CombatManager::ResolveDefeatReward(Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (progression != nullptr && !enemy.isAlive() && enemy.ClaimXPReward()) {
        progression->GrantXP(enemy.getXPReward());
        progression->CheckLevelUp(player);
        AddEvent(CombatEvent::Kind::Info, "XP gained: +" + std::to_string(enemy.getXPReward()) + ".");
    }
}

void CombatManager::StartPlayerTurn(Player& player) {
    if (!player.isAlive()) return;
    consumableUsedThisTurn_ = false;
    for (auto& cooldown : shimmerCooldowns_) if (cooldown.second > 0) --cooldown.second;
    if (defaultSkill_ != nullptr) defaultSkill_->TickCooldown();
    AddEvent(CombatEvent::Kind::Turn, player.getName() + "'s turn.");
    const int healthBefore = player.getHP();
    player.ProcessStatuses();
    const int damage = healthBefore - player.getHP();
    if (damage > 0) AddEvent(CombatEvent::Kind::DotTick, "Ongoing effects deal " + std::to_string(damage) + " damage to " + player.getName() + ".", damage);
    const int healing = player.getHP() - healthBefore;
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(healing) + " HP from HoT.", healing);
    if (!player.isAlive()) AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
}

CombatManager::CombatManager()
    : blocked_(false), weaknessExposed_(false), interrogationAttempts_(0),
      turnNumber_(0), playerTurn_(true), activeEnemy_(nullptr),
      playerGauge_(0), enemyGauge_(0),
      consumableUsedThisTurn_(false),
      defaultSkill_(ShimmerSkill::Create("Pulsus Luminis")) {}

CombatManager::~CombatManager() = default;

const ShimmerSkill* CombatManager::GetDefaultSkill() const { return defaultSkill_.get(); }

const std::vector<CombatManager::CombatEvent>& CombatManager::GetEvents() const { return events_; }
void CombatManager::ClearEvents() { events_.clear(); }
void CombatManager::ResetEncounter() {
    activeEnemy_ = nullptr;
    blocked_ = false;
    weaknessExposed_ = false;
    interrogationAttempts_ = 0;
    turnNumber_ = 0;
    playerTurn_ = true;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    consumableUsedThisTurn_ = false;
    groupEnemyTurn_ = false;
    currentEnemyParty_.clear();
    shimmerCooldowns_.clear();
    events_.clear();
}
void CombatManager::RecordInfo(const std::string& message) { AddEvent(CombatEvent::Kind::Info, message); }
int CombatManager::GetSkillCooldown(const std::string& skillName) const {
    const auto found = shimmerCooldowns_.find(skillName);
    return found == shimmerCooldowns_.end() ? 0 : found->second;
}
void CombatManager::AddEvent(CombatEvent::Kind kind, const std::string& message, int value) {
    events_.push_back({kind, message, value});
    if (events_.size() > 40) events_.erase(events_.begin());
}

void CombatManager::PrepareTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;
    if (playerGauge_ == 0 && enemyGauge_ == 0 && !playerTurn_) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        if (playerTurn_) StartPlayerTurn(player);
    }
    if (playerTurn_ && player.isAlive()) AddEvent(CombatEvent::Kind::Info, "Waiting for " + player.getName() + " to choose an action.");
}

void CombatManager::ResumeAfterRespawn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;
    playerGauge_ = 1000;
    enemyGauge_ = 0;
    playerTurn_ = true;
    consumableUsedThisTurn_ = false;
    StartPlayerTurn(player);
    AddEvent(CombatEvent::Kind::Info, player.getName() + " returns from the fountain and may act.");
}

void CombatManager::BeginEncounter(Enemy& enemy) {
    if (activeEnemy_ == &enemy) return;
    activeEnemy_ = &enemy;
    blocked_ = false;
    turnNumber_ = 0;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    playerTurn_ = false;
    weaknessExposed_ = false;
    interrogationAttempts_ = 0;
    consumableUsedThisTurn_ = false;
    shimmerCooldowns_.clear();
    events_.clear();
    AddEvent(CombatEvent::Kind::Info, "Combat begins against " + enemy.getName() + ".");
}

void CombatManager::ContinueEncounter(Enemy& enemy) {
    activeEnemy_ = &enemy;
    playerGauge_ = 0;
    enemyGauge_ = 0;
    playerTurn_ = false;
    consumableUsedThisTurn_ = false;
    AddEvent(CombatEvent::Kind::Info, enemy.getName() + " enters the battle.");
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    if (activeEnemy_ == nullptr) BeginEncounter(enemy);
    blocked_ = false;
    if (turnNumber_ == 0 && playerGauge_ == 0 && enemyGauge_ == 0) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
    }

    if (!player.isAlive() || !enemy.isAlive()) return false;
    if (!playerTurn_) return false;
    ++turnNumber_;

    switch (action) {
    case CombatAction::Attack: {
        AddEvent(CombatEvent::Kind::Attack, player.getName() + " uses Attack on " + enemy.getName() + ".");
        const unsigned hitRoll = static_cast<unsigned>(turnNumber_ * 37 + player.getAttack() * 11 + enemy.getHP()) % 100;
        const int hitChance = CombatMath::AccuracyChance(100, player.getAccuracy(), enemy.getEvasion());
        if (hitRoll >= static_cast<unsigned>(hitChance)) {
            AddEvent(CombatEvent::Kind::Info, player.getName() + " attacks " + enemy.getName() + ", but the strike is evaded.");
            playerGauge_ -= 1000;
            playerTurn_ = false;
            AddEvent(CombatEvent::Kind::Info, player.getName() + "'s attack was evaded.");
            AdvanceTurn(player, enemy);
            return true;
        }
        const unsigned critRoll = static_cast<unsigned>(turnNumber_ * 53 + player.getAttack() * 7 + enemy.getHP() * 3) % 100;
        const bool critical = critRoll < static_cast<unsigned>((std::min)(100, player.getCritRate()));
        const int rawDamage = critical
            ? player.getAttack() * player.getCritDamage() / 100
            : player.getAttack();
        const int damage = CombatMath::Mitigate(rawDamage, enemy.getDefense()) + (weaknessExposed_ ? 2 : 0);
        enemy.takeDamage(damage);
        AddEvent(CombatEvent::Kind::Damage, player.getName() + " deals " + std::to_string(damage) + " damage to " + enemy.getName() + ".", damage);
        if (player.getLifesteal() > 0) {
            const int recovery = damage * (std::min)(CombatMath::Balance::MaximumLifesteal, player.getLifesteal()) / 100;
            if (recovery > 0) {
                player.heal(recovery);
                AddEvent(CombatEvent::Kind::Heal, player.getName() + " recovers " + std::to_string(recovery) + " HP from lifesteal.", recovery);
            }
        }
        if (enemy.UpdatePhase()) {
            AddEvent(CombatEvent::Kind::Info, "Boss enters Phase " + std::to_string(enemy.GetPhase()) + ".");
        }
        AddEvent(CombatEvent::Kind::Info, player.getName() + " attacks " + enemy.getName()
                  + " for " + std::to_string(damage) + (critical ? " [CRITICAL]" : "") + " damage.");
        if (!enemy.isAlive() && progression != nullptr) {
            ResolveDefeatReward(player, enemy, progression);
        }
        playerGauge_ -= 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return true;
    }
    case CombatAction::Shimmer:
    {
        if (skill == nullptr) {
            AddEvent(CombatEvent::Kind::Info, "Select an equipped Shimmer skill first.");
            return false;
        }
        const std::string effect = skill->getEffect();
        const int remainingCooldown = shimmerCooldowns_[skill->getName()];
        if (remainingCooldown > 0) {
            AddEvent(CombatEvent::Kind::Info, skill->getName() + " is recharging for " + std::to_string(remainingCooldown) + (remainingCooldown == 1 ? " turn." : " turns."));
            return false;
        }
        const int enemyHealthBefore = enemy.getHP();
        const int playerHealthBefore = player.getHP();
        AddEvent(CombatEvent::Kind::Shimmer, player.getName() + " uses Shimmer skill " + skill->getName() + " on " + enemy.getName() + ". Effect: " + effect + ".");
        skill->apply(player, enemy);

        const int dealt = enemyHealthBefore - enemy.getHP();
        const int healed = player.getHP() - playerHealthBefore;
        if (dealt > 0) AddEvent(CombatEvent::Kind::Damage, "Shimmer dealt " + std::to_string(dealt) + " damage.", dealt);
        if (healed > 0) AddEvent(CombatEvent::Kind::Heal, "Shimmer restored " + std::to_string(healed) + " HP.", healed);
        if (effect.find("barrier") != std::string::npos) AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains a Shimmer barrier.");
        if (effect.find("stun") != std::string::npos && enemy.HasStatus(StatusType::Stun)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " is stunned by Shimmer.");
        if (effect.find("burn") != std::string::npos && enemy.HasStatus(StatusType::Burn)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " suffers Burn from Shimmer.");
        if (effect.find("mark") != std::string::npos && enemy.HasStatus(StatusType::VerdigrisMark)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " is marked by Shimmer.");
        if (effect == "stat_boost") AddEvent(CombatEvent::Kind::Info, player.getName() + " gains a major Shimmer stat increase.");
        if (effect == "extra_turn") AddEvent(CombatEvent::Kind::Info, player.getName() + " accelerates the next combat action.");
        if (dealt == 0 && healed == 0 && effect.find("barrier") == std::string::npos &&
            effect.find("stun") == std::string::npos && effect.find("burn") == std::string::npos &&
            effect.find("mark") == std::string::npos && effect != "stat_boost" && effect != "extra_turn") {
            AddEvent(CombatEvent::Kind::Info, "Shimmer effect resolved: " + effect + ".");
        }
        shimmerCooldowns_[skill->getName()] = skill->getCooldown() + 1;
        if (!enemy.isAlive() && progression != nullptr) {
            ResolveDefeatReward(player, enemy, progression);
        }
        playerGauge_ -= 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return true;
    }
    case CombatAction::Item: {
        if (consumableUsedThisTurn_) {
            AddEvent(CombatEvent::Kind::Info, "Only one consumable can be used per turn.");
            return false;
        }
        AddEvent(CombatEvent::Kind::Info, "Select a consumable to use.");
        return false;
    }
    case CombatAction::Flee:
        AddEvent(CombatEvent::Kind::Info, player.getName() + " retreats from the encounter.");
        return true;
    }

    return false;
}

bool CombatManager::UseConsumable(const std::string& itemName, Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!playerTurn_ || consumableUsedThisTurn_) {
        AddEvent(CombatEvent::Kind::Info, "Only one consumable can be used per turn.");
        return false;
    }
    const int healthBefore = player.getHP();
    const int enemyHealthBefore = enemy.getHP();
    if (!player.UseConsumable(itemName, &enemy)) {
        AddEvent(CombatEvent::Kind::Info, "That consumable cannot be used in battle.");
        return false;
    }
    consumableUsedThisTurn_ = true;
    AddEvent(CombatEvent::Kind::Info, player.getName() + " uses " + itemName + ".");
    const int healing = player.getHP() - healthBefore;
    const int damage = enemyHealthBefore - enemy.getHP();
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " heals " + std::to_string(healing) + " HP.", healing);
    if (damage > 0) AddEvent(CombatEvent::Kind::Damage, player.getName() + " deals " + std::to_string(damage) + " damage to " + enemy.getName() + ".", damage);
    ResolveDefeatReward(player, enemy, progression);
    playerGauge_ -= 1000;
    playerTurn_ = false;
    AdvanceTurn(player, enemy);
    return true;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, ProgressionManager* progression) {
    if (!enemy.isAlive()) {
        ResolveDefeatReward(player, enemy, progression);
        return;
    }
    enemy.TickSkillCooldowns();

    if (enemyGauge_ < 1000) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        if (playerTurn_ && !groupEnemyTurn_) return;
    }

    AddEvent(CombatEvent::Kind::Turn, enemy.getName() + "'s turn.");
    std::vector<std::pair<StatusType, int>> enemyDamageEvents;
    int enemyHealing = 0;
    enemy.ProcessStatuses(enemyDamageEvents, enemyHealing);
    for (const auto& tick : enemyDamageEvents) AddEvent(CombatEvent::Kind::DotTick, StatusDisplayName(tick.first) + std::string(" deals ") + std::to_string(tick.second) + " damage to " + enemy.getName() + ".", tick.second);
    if (enemyHealing > 0) AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " heals " + std::to_string(enemyHealing) + " HP from HoT.", enemyHealing);
    if (!enemy.isAlive()) return;
    if (enemy.HasStatus(StatusType::Stun)) {
        AddEvent(CombatEvent::Kind::Info, enemy.getName() + " loses its turn to Stun.");
        enemyGauge_ -= 1000;
        return;
    }
    consumableUsedThisTurn_ = false;
    const auto& skills = enemy.GetSkills();
    for (std::size_t index = 0; index < skills.size(); ++index) {
        const auto& skill = skills[index];
        if (!enemy.IsSkillReady(index)) continue;
        if (skill.effect == "heal_self" && enemy.getHP() >= enemy.getMaxHP() / 2) continue;
        if (skill.effect == "cleanse_self" && enemy.GetStatuses().empty()) continue;
        AddEvent(CombatEvent::Kind::Skill, enemy.getName() + " uses " + skill.name + ".");
        const bool grantsExtraTurn = skill.effect == "extra_turn";
        Enemy* allyTarget = nullptr;
        if (skill.effect.find("ally") != std::string::npos) {
            for (Enemy* ally : currentEnemyParty_) {
                if (ally != nullptr && ally != &enemy && ally->isAlive()) { allyTarget = ally; break; }
            }
        }
        if (allyTarget != nullptr && skill.effect.find("heal") != std::string::npos) {
            const int before = allyTarget->getHP();
            allyTarget->heal((std::max)(1, allyTarget->getMaxHP() * 15 / 100));
            AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " heals ally " + allyTarget->getName() + " for " + std::to_string(allyTarget->getHP() - before) + " HP.");
        }
        const int damage = skill.multiplier > 0
                ? CombatMath::Mitigate((std::max)(1, enemy.getAtk() * skill.multiplier / 100), player.getDefense())
            : 0;
        if (damage > 0) {
            const int finalDamage = player.AbsorbDamage(damage);
            player.takeDamage(finalDamage);
            AddEvent(CombatEvent::Kind::Damage, enemy.getName() + " deals " + std::to_string(finalDamage) + " damage to " + player.getName() + ".", finalDamage);
        }
        if (skill.effect == "bleed" || skill.effect == "burn" || skill.effect == "poison") {
            StatusType type = skill.effect == "bleed" ? StatusType::Bleed : skill.effect == "burn" ? StatusType::Burn : StatusType::Poison;
            const bool hadStatus = player.HasStatus(type);
            player.ApplyStatus({type, skill.duration > 0 ? skill.duration : 2, 0, enemy.getAtk(), 0, enemy.getDefense()}, enemy.getAccuracy());
            AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " inflicts " + std::string(StatusDisplayName(type)) + " on " + player.getName() + " for " +
                std::to_string(skill.duration > 0 ? skill.duration : 2) + " turns.");
        } else if (skill.effect == "tri_dot" || skill.effect == "burn_bleed") {
            const int duration = skill.duration > 0 ? skill.duration : 2;
            for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison}) {
                if (skill.effect == "burn_bleed" && type == StatusType::Poison) continue;
                const bool hadStatus = player.HasStatus(type);
                player.ApplyStatus({type, duration, 0, enemy.getAtk(), 0, enemy.getDefense()}, enemy.getAccuracy());
                if (!hadStatus && player.HasStatus(type)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " inflicts " + std::string(StatusDisplayName(type)) + " on " + player.getName() + " for " + std::to_string(duration) + " turns.");
            }
        } else if (skill.effect == "def_down" || skill.effect == "accuracy_down") {
            const bool hadStatus = player.HasStatus(StatusType::VerdigrisMark);
            player.ApplyStatus({StatusType::VerdigrisMark, skill.duration > 0 ? skill.duration : 2, 0, enemy.getAtk(), 0, enemy.getDefense()}, enemy.getAccuracy());
            if (!hadStatus && player.HasStatus(StatusType::VerdigrisMark)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " suffers Verdigris Mark for " + std::to_string(skill.duration > 0 ? skill.duration : 2) + " turns!");
        } else if (skill.effect == "heal_self" || skill.effect == "heal_cleanse") {
            const int amount = (std::max)(1, enemy.getMaxHP() * 15 / 100);
            const int before = enemy.getHP();
            enemy.heal(amount);
            AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " healed for " + std::to_string(enemy.getHP() - before) + ".", enemy.getHP() - before);
            if (skill.effect == "heal_cleanse") {
                enemy.ClearStatuses();
                AddEvent(CombatEvent::Kind::Cleanse, enemy.getName() + " cleanses its negative statuses.");
            }
        } else if (skill.effect == "cleanse_self") {
            enemy.ClearStatuses();
            AddEvent(CombatEvent::Kind::Cleanse, enemy.getName() + " cleanses its negative statuses.");
        } else if (skill.effect == "barrier") {
            enemy.ApplyStatus({StatusType::Barrier, skill.duration > 0 ? skill.duration : 2, enemy.getMaxHP() / 5});
            AddEvent(CombatEvent::Kind::Shield, enemy.getName() + " gains a barrier.");
        } else if (skill.effect == "attack_up" || skill.effect == "accuracy_up" || skill.effect == "crit_up" ||
                   skill.effect == "crit_damage_up" || skill.effect == "guaranteed_crit" || skill.effect == "dot_up" ||
                skill.effect == "heal_resistance" || skill.effect == "accuracy_floor" ||
                   skill.effect == "dot_reduction" || skill.effect == "accuracy_crit_up") {
            StatusType status = StatusType::AttackUp;
            if (skill.effect == "accuracy_up") status = StatusType::AccuracyUp;
            else if (skill.effect == "crit_up") status = StatusType::CritUp;
            else if (skill.effect == "crit_damage_up") status = StatusType::CritDamageUp;
            else if (skill.effect == "guaranteed_crit") status = StatusType::GuaranteedCrit;
            else if (skill.effect == "dot_up") status = StatusType::DotUp;
            else if (skill.effect == "heal_resistance") status = StatusType::HealResistance;
            else if (skill.effect == "accuracy_floor") status = StatusType::AccuracyFloor;
            else if (skill.effect == "dot_reduction") status = StatusType::DotReduction;
            else if (skill.effect == "accuracy_crit_up") {
                enemy.ApplyStatus({StatusType::AccuracyUp, skill.duration > 0 ? skill.duration : 2, 25});
                enemy.ApplyStatus({StatusType::CritUp, skill.duration > 0 ? skill.duration : 2, 20});
                AddEvent(CombatEvent::Kind::Buff, enemy.getName() + " gains Accuracy Up and Critical Rate Up.");
                goto finish_enemy_skill;
            }
            enemy.ApplyStatus({status, skill.duration > 0 ? skill.duration : 2, skill.multiplier > 0 ? skill.multiplier / 2 : 20});
            AddEvent(CombatEvent::Kind::Buff, enemy.getName() + " gains " + StatusDisplayName(status) + ".");
        }
        if (skill.effect == "extra_turn") AddEvent(CombatEvent::Kind::ExtraTurn, enemy.getName() + " gains an extra turn.");
    finish_enemy_skill:
        if (!enemy.isAlive()) AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
        enemy.StartSkillCooldown(index);
        enemyGauge_ -= 1000;
        if (grantsExtraTurn) enemyGauge_ = 1000;
        playerTurn_ = false;
        AdvanceTurn(player, enemy);
        return;
    }

    const int attackDamage = enemy.getAtk();
    const int hitRoll = (turnNumber_ * 41 + enemy.getAtk() * 7 + player.getHP()) % 100;
    const int hitChance = CombatMath::AccuracyChance(100, enemy.getAccuracy(), player.getEvasion());
    if (hitRoll >= hitChance) {
        AddEvent(CombatEvent::Kind::Info, enemy.getName() + " misses " + player.getName() + ".");
        enemyGauge_ -= 1000;
        return;
    }
    const bool critical = enemy.HasStatus(StatusType::GuaranteedCrit) ||
        ((enemy.getCritRate() + (enemy.HasStatus(StatusType::CritUp) ? 20 : 0)) > hitRoll);
    const int rawDamage = critical ? attackDamage * (enemy.getCritDamage() + (enemy.HasStatus(StatusType::CritDamageUp) ? 50 : 0)) / 100 : attackDamage;
    const int damage = CombatMath::Mitigate(rawDamage, player.getDefense());
    const int finalDamage = player.AbsorbDamage(damage);
    player.takeDamage(finalDamage);
    AddEvent(CombatEvent::Kind::Attack, enemy.getName() + " attacks " + player.getName() + ".", finalDamage);
    AddEvent(CombatEvent::Kind::Damage, enemy.getName() + " deals " + std::to_string(finalDamage) + " damage to " + player.getName() + ".", finalDamage);
    if (!player.isAlive()) AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
    AddEvent(CombatEvent::Kind::Info, enemy.getName() + " attacks for " + std::to_string(damage)
        + (critical ? " [CRITICAL]" : "") + " damage.");
    enemyGauge_ -= 1000;
    playerTurn_ = false;
    AdvanceTurn(player, enemy);
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy, const std::vector<Enemy*>& enemies, ProgressionManager* progression) {
    std::vector<Enemy*> order;
    for (Enemy* candidate : enemies) if (candidate != nullptr && candidate->isAlive()) order.push_back(candidate);
    std::sort(order.begin(), order.end(), [](const Enemy* left, const Enemy* right) {
        if (left->getSpeed() != right->getSpeed()) return left->getSpeed() > right->getSpeed();
        return left->getName() < right->getName();
    });
    groupEnemyTurn_ = true;
    playerTurn_ = false;
    for (Enemy* candidate : order) {
        if (!player.isAlive()) break;
        enemyGauge_ = 1000;
        enemyTurn(player, *candidate, progression);
        playerTurn_ = false;
    }
    groupEnemyTurn_ = false;
    enemyGauge_ = 0;
    playerGauge_ = 1000;
    if (player.isAlive()) {
        playerTurn_ = true;
        StartPlayerTurn(player);
    }
}

bool CombatManager::IsPlayerTurn() const { return playerTurn_; }
bool CombatManager::HasUsedConsumableThisTurn() const { return consumableUsedThisTurn_; }

bool CombatManager::interrogate(Player& player, Enemy& enemy,
                                ProgressionManager* progression) {
    ++interrogationAttempts_;
    const int playerLevel = progression != nullptr ? progression->GetLevel() : 1;
    const int chance = InterrogationChance(enemy, playerLevel);
    const unsigned roll = InterrogationRoll(enemy, playerLevel, interrogationAttempts_);

    AddEvent(CombatEvent::Kind::Info, "Interrogation success chance: " + std::to_string(chance) + "%.");
    if (roll >= static_cast<unsigned>(chance)) {
        AddEvent(CombatEvent::Kind::Info, enemy.getName() + " refuses to reveal anything.");
        return true;
    }

    if (!enemy.ClaimInvestigationReward()) {
        AddEvent(CombatEvent::Kind::Info, enemy.getName() + " has no new lore to reveal.");
        return true;
    }
    AddEvent(CombatEvent::Kind::Info, enemy.getName() + " reveals a fragment of cathedral lore.");
    if (progression != nullptr) {
        progression->GrantXP(10);
        progression->CheckLevelUp(player);
    }
    return true;
}
