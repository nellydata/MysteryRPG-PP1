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

    player.ProcessStatuses();
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
        AddEvent(CombatEvent::Kind::Info, "Shimmer skills are not available yet.");
        return false;
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
    AddEvent(CombatEvent::Kind::Damage, enemy.getName() + " inflicted " + std::to_string(damage) + " damage!", damage);
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
