#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <cmath>
#include "ShimmerSkill.h"
#include <algorithm>
#include <iostream>
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
    while (playerGauge < 1000 && enemyGauge < 1000) {
        playerGauge += (std::max)(1, playerSpeed);
        enemyGauge += (std::max)(1, enemySpeed);
    }
}
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
void CombatManager::AddEvent(CombatEvent::Kind kind, const std::string& message, int value) {
    events_.push_back({kind, message, value});
    if (events_.size() > 40) events_.erase(events_.begin());
}

void CombatManager::PrepareTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;
    if (turnNumber_ == 0 && playerGauge_ == 0 && enemyGauge_ == 0) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
    }
    while (!playerTurn_ && player.isAlive() && enemy.isAlive() && enemyGauge_ >= 1000) enemyTurn(player, enemy);
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
    events_.clear();
    AddEvent(CombatEvent::Kind::Info, "Combat begins against " + enemy.getName() + ".");
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    BeginEncounter(enemy);
    blocked_ = false;
    if (turnNumber_ == 0 && playerGauge_ == 0 && enemyGauge_ == 0) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
    }

    const int playerHealthBeforeStatuses = player.getHP();
    const auto statusesBefore = player.GetStatuses();
    player.ProcessStatuses();
    for (const auto& status : statusesBefore) {
        const int healthLost = playerHealthBeforeStatuses - player.getHP();
        if (healthLost > 0 && (status.type == StatusType::Bleed || status.type == StatusType::Burn || status.type == StatusType::Poison)) {
            AddEvent(CombatEvent::Kind::DotTick, player.getName() + " takes " + std::to_string(healthLost) + " " + std::string(StatusDisplayName(status.type)) + " damage.", healthLost);
            break;
        }
    }
    if (!player.isAlive()) AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen from status damage.");
    enemy.ProcessStatuses();
    if (!player.isAlive() || !enemy.isAlive()) return false;
    if (!playerTurn_) return false;
    ++turnNumber_;
    AddEvent(CombatEvent::Kind::Turn, player.getName() + "'s turn.");

    switch (action) {
    case CombatAction::Attack: {
        const unsigned roll = static_cast<unsigned>(turnNumber_ * 37 + player.getAttack() * 11 + enemy.getHP()) % 100;
        if (roll < static_cast<unsigned>(enemy.getEvasion())) {
            std::cout << player.getName() << " attacks " << enemy.getName() << ", but the strike is evaded.\n";
            playerTurn_ = false;
            AddEvent(CombatEvent::Kind::Info, player.getName() + "'s attack was evaded.");
            return true;
        }
        const bool critical = roll < static_cast<unsigned>((std::min)(100, player.getCritRate()));
        const int rawDamage = critical
            ? player.getAttack() * player.getCritDamage() / 100
            : player.getAttack();
        const int damage = std::max(1, static_cast<int>(std::lround(
            static_cast<double>(rawDamage) * 100.0 / (100.0 + enemy.getDefense())))
            + (weaknessExposed_ ? 2 : 0));
        enemy.takeDamage(damage);
        AddEvent(CombatEvent::Kind::Damage, player.getName() + " inflicted " + std::to_string(damage) + " damage!", damage);
        if (enemy.UpdatePhase()) {
            AddEvent(CombatEvent::Kind::Info, "Boss enters Phase " + std::to_string(enemy.GetPhase()) + ".");
        }
        std::cout << "Adelaide attacks " << enemy.getName()
                  << " for " << damage << (critical ? " critical" : "") << " damage.\n";
        if (!enemy.isAlive() && progression != nullptr) {
            progression->GrantXP(enemy.getXPReward());
            progression->CheckLevelUp(player);
        }
        playerGauge_ -= 1000;
        playerTurn_ = enemy.isAlive() && playerGauge_ >= 1000;
        return true;
    }
    case CombatAction::Shimmer:
    {
        if (skill == nullptr) {
            AddEvent(CombatEvent::Kind::Info, "Select an equipped Shimmer skill first.");
            return false;
        }
        const std::string effect = skill->getEffect();
        const int enemyHealthBefore = enemy.getHP();
        const int playerHealthBefore = player.getHP();
        AddEvent(CombatEvent::Kind::Shimmer, player.getName() + " invokes a Shimmer resonance.");

        if (effect.find("heal") != std::string::npos || effect.find("restore") != std::string::npos) {
            const int healAmount = (std::max)(1, player.getMaxHP() * 20 / 100);
            player.heal(healAmount);
        }
        if (effect.find("barrier") != std::string::npos || effect.find("shield") != std::string::npos) {
            player.ApplyStatus({StatusType::Barrier, 1, 0, 0});
            AddEvent(CombatEvent::Kind::Shield, player.getName() + " gains a barrier.");
        }
        if (effect.find("damage") != std::string::npos || effect.find("strike") != std::string::npos ||
            effect.find("attack") != std::string::npos || effect.find("burn") != std::string::npos) {
            const int rawDamage = (std::max)(1, player.getAttack() * 130 / 100);
            const int shimmerDamage = std::max(1, static_cast<int>(std::lround(
                static_cast<double>(rawDamage) * 100.0 / (100.0 + enemy.getDefense()))));
            enemy.takeDamage(shimmerDamage);
        }

        const int dealt = enemyHealthBefore - enemy.getHP();
        const int healed = player.getHP() - playerHealthBefore;
        if (dealt > 0) AddEvent(CombatEvent::Kind::Damage, "Shimmer dealt " + std::to_string(dealt) + " damage.", dealt);
        if (healed > 0) AddEvent(CombatEvent::Kind::Heal, "Shimmer restored " + std::to_string(healed) + " HP.", healed);
        if (!enemy.isAlive() && progression != nullptr) {
            progression->GrantXP(enemy.getXPReward());
            progression->CheckLevelUp(player);
        }
        playerGauge_ -= 1000;
        playerTurn_ = enemy.isAlive() && playerGauge_ >= 1000;
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
        std::cout << "Adelaide retreats from the encounter.\n";
        return true;
    }

    return false;
}

bool CombatManager::UseConsumable(const std::string& itemName, Player& player, Enemy& enemy) {
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
    AddEvent(CombatEvent::Kind::Info, player.getName() + " used " + itemName + ".");
    const int healing = player.getHP() - healthBefore;
    const int damage = enemyHealthBefore - enemy.getHP();
    if (healing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " healed for " + std::to_string(healing) + ".", healing);
    if (damage > 0) AddEvent(CombatEvent::Kind::Damage, player.getName() + " inflicted " + std::to_string(damage) + " damage!", damage);
    return true;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;

    if (enemyGauge_ < 1000) {
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        if (playerTurn_) return;
    }

    AddEvent(CombatEvent::Kind::Turn, enemy.getName() + "'s turn.");
    consumableUsedThisTurn_ = false;
    const auto& skills = enemy.GetSkills();
    for (std::size_t index = 0; index < skills.size(); ++index) {
        const auto& skill = skills[index];
        if (!enemy.IsSkillReady(index)) continue;
        if (skill.effect == "heal_self" && enemy.getHP() >= enemy.getMaxHP() / 2) continue;
        if (skill.effect == "cleanse_self" && enemy.GetStatuses().empty()) continue;
        const int damage = skill.multiplier > 0
            ? (std::max)(1, enemy.getAtk() * skill.multiplier / 100)
            : 0;
        if (damage > 0) {
            player.takeDamage((std::max)(0, static_cast<int>(std::lround(
                static_cast<double>(damage) * 100.0 / (100.0 + player.getDefense())))));
            AddEvent(CombatEvent::Kind::Skill, enemy.getName() + " uses " + skill.name + ". " + enemy.getName() + " dealt " + std::to_string(damage) +
                " damage to " + player.getName() + " with " + skill.name + ".", damage);
        }
        if (skill.effect == "bleed" || skill.effect == "burn" || skill.effect == "poison") {
            StatusType type = skill.effect == "bleed" ? StatusType::Bleed : skill.effect == "burn" ? StatusType::Burn : StatusType::Poison;
            const bool hadStatus = player.HasStatus(type);
            player.ApplyStatus({type, skill.duration > 0 ? skill.duration : 2, 0, enemy.getAtk()});
            if (!hadStatus && player.HasStatus(type)) AddEvent(CombatEvent::Kind::Debuff, player.getName() + " suffers " + std::string(StatusDisplayName(type)) + " for " +
                std::to_string(skill.duration > 0 ? skill.duration : 2) + " turns!");
        } else if (skill.effect == "tri_dot" || skill.effect == "burn_bleed") {
            const int duration = skill.duration > 0 ? skill.duration : 2;
            for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison}) {
                if (skill.effect == "burn_bleed" && type == StatusType::Poison) continue;
                const bool hadStatus = player.HasStatus(type);
                player.ApplyStatus({type, duration, 0, enemy.getAtk()});
                if (!hadStatus && player.HasStatus(type)) AddEvent(CombatEvent::Kind::Debuff, player.getName() + " suffers " + std::string(StatusDisplayName(type)) + " for " + std::to_string(duration) + " turns!");
            }
        } else if (skill.effect == "def_down" || skill.effect == "accuracy_down") {
            const bool hadStatus = enemy.HasStatus(StatusType::VerdigrisMark);
            enemy.ApplyStatus({StatusType::VerdigrisMark, skill.duration > 0 ? skill.duration : 2, 0, enemy.getAtk()}, player.getAccuracy());
            if (!hadStatus && enemy.HasStatus(StatusType::VerdigrisMark)) AddEvent(CombatEvent::Kind::Debuff, enemy.getName() + " suffers Verdigris Mark for " + std::to_string(skill.duration > 0 ? skill.duration : 2) + " turns!");
        } else if (skill.effect == "heal_self" || skill.effect == "heal_cleanse") {
            const int amount = (std::max)(1, enemy.getMaxHP() * 15 / 100);
            const int before = enemy.getHP();
            enemy.heal(amount);
            AddEvent(CombatEvent::Kind::Heal, enemy.getName() + " healed for " + std::to_string(enemy.getHP() - before) + ".", enemy.getHP() - before);
        }
        if (!enemy.isAlive()) AddEvent(CombatEvent::Kind::Death, enemy.getName() + " has been defeated.");
        enemy.StartSkillCooldown(index);
        enemy.TickSkillCooldowns();
        enemyGauge_ -= 1000;
        playerTurn_ = false;
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        return;
    }

    if (enemy.HasStatus(StatusType::Stun)) {
        std::cout << enemy.getName() << " loses its turn to the stun.\n";
        enemyGauge_ -= 1000;
        playerTurn_ = false;
        AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
        playerTurn_ = playerGauge_ >= 1000;
        return;
    }
    if (player.HasStatus(StatusType::Barrier)) {
        player.ClearStatuses();
        std::cout << "The Shimmer barrier negates the attack.\n";
        playerTurn_ = true;
        AddEvent(CombatEvent::Kind::Shield, player.getName() + "'s shield blocked the attack.");
        return;
    }
    const int attackDamage = enemy.getAtk();
    const int damage = std::max(0, static_cast<int>(std::lround(
        static_cast<double>(attackDamage) * 100.0 / (100.0 + player.getDefense()))));
    player.takeDamage(damage);
    AddEvent(CombatEvent::Kind::Attack, enemy.getName() + " uses Basic Attack and inflicted " + std::to_string(damage) + " damage!", damage);
    if (!player.isAlive()) AddEvent(CombatEvent::Kind::Death, player.getName() + " has fallen.");
    std::cout << enemy.getName() << " attacks for " << damage << " damage.\n";
    if (defaultSkill_ != nullptr) defaultSkill_->TickCooldown();
    enemyGauge_ -= 1000;
    playerTurn_ = false;
    AccumulateGaugesUntilReady(playerGauge_, enemyGauge_, player.getSpeed(), enemy.getSpeed());
    playerTurn_ = playerGauge_ >= 1000;
}

bool CombatManager::IsPlayerTurn() const { return playerTurn_; }
bool CombatManager::HasUsedConsumableThisTurn() const { return consumableUsedThisTurn_; }

bool CombatManager::interrogate(Player& player, Enemy& enemy,
                                ProgressionManager* progression) {
    ++interrogationAttempts_;
    const int playerLevel = progression != nullptr ? progression->GetLevel() : 1;
    const int chance = InterrogationChance(enemy, playerLevel);
    const unsigned roll = InterrogationRoll(enemy, playerLevel, interrogationAttempts_);

    std::cout << "Interrogation success chance: " << chance << "%.\n";
    if (roll >= static_cast<unsigned>(chance)) {
        std::cout << enemy.getName() << " refuses to reveal anything.\n";
        return true;
    }

    std::cout << enemy.getName() << " reveals a fragment of cathedral lore.\n";
    if (progression != nullptr) {
        progression->GrantXP(10);
        progression->CheckLevelUp(player);
    }
    return true;
}
