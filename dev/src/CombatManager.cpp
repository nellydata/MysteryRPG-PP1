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
}

CombatManager::CombatManager()
    : blocked_(false), weaknessExposed_(false), interrogationAttempts_(0),
      turnNumber_(0), playerTurn_(true), activeEnemy_(nullptr),
      defaultSkill_(ShimmerSkill::Create("Pulsus Luminis")) {}

CombatManager::~CombatManager() = default;

const ShimmerSkill* CombatManager::GetDefaultSkill() const { return defaultSkill_.get(); }

void CombatManager::BeginEncounter(Enemy& enemy) {
    if (activeEnemy_ == &enemy) return;
    activeEnemy_ = &enemy;
    blocked_ = false;
    turnNumber_ = 0;
    playerTurn_ = true;
    weaknessExposed_ = false;
    interrogationAttempts_ = 0;
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    BeginEncounter(enemy);
    blocked_ = false;

    player.ProcessStatuses();
    enemy.ProcessStatuses();
    if (!player.isAlive() || !enemy.isAlive()) return false;
    if (!playerTurn_) return false;
    ++turnNumber_;

    switch (action) {
    case CombatAction::Attack: {
        const unsigned roll = static_cast<unsigned>(turnNumber_ * 37 + player.getAttack() * 11 + enemy.getHP()) % 100;
        if (roll < static_cast<unsigned>(enemy.getEvasion())) {
            std::cout << player.getName() << " attacks " << enemy.getName() << ", but the strike is evaded.\n";
            playerTurn_ = false;
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
        std::cout << "Adelaide attacks " << enemy.getName()
                  << " for " << damage << (critical ? " critical" : "") << " damage.\n";
        if (!enemy.isAlive() && progression != nullptr) {
            progression->GrantXP(enemy.getXPReward());
            progression->CheckLevelUp(player);
        }
        if (enemy.isAlive()) playerTurn_ = false;
        return true;
    }
    case CombatAction::Shimmer:
        if (skill == nullptr) skill = defaultSkill_.get();
        if (skill == nullptr || !skill->IsReady()) {
            std::cout << "The Shimmer is not available yet.\n";
            return false;
        }
        skill->apply(player, enemy);
        skill->ResetCooldown();
        if (enemy.isAlive()) playerTurn_ = false;
        return true;
    case CombatAction::Interrogate: {
        const bool result = interrogate(player, enemy, progression);
        if (result && enemy.isAlive()) playerTurn_ = false;
        return result;
    }
    case CombatAction::ExposeWeakness:
        weaknessExposed_ = true;
        playerTurn_ = false;
        std::cout << "A weakness in " << enemy.getName() << " is exposed.\n";
        return true;
    case CombatAction::Flee:
        std::cout << "Adelaide retreats from the encounter.\n";
        return true;
    }

    return false;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;

    if (enemy.HasStatus(StatusType::Stun)) {
        std::cout << enemy.getName() << " loses its turn to the stun.\n";
        playerTurn_ = true;
        return;
    }
    if (player.HasStatus(StatusType::Barrier)) {
        player.ClearStatuses();
        std::cout << "The Shimmer barrier negates the attack.\n";
        playerTurn_ = true;
        return;
    }
    const int attackDamage = enemy.getAtk();
    const int damage = std::max(0, static_cast<int>(std::lround(
        static_cast<double>(attackDamage) * 100.0 / (100.0 + player.getDefense()))));
    player.takeDamage(damage);
    std::cout << enemy.getName() << " attacks for " << damage << " damage.\n";
    if (defaultSkill_ != nullptr) defaultSkill_->TickCooldown();
    playerTurn_ = true;
}

bool CombatManager::IsPlayerTurn() const { return playerTurn_; }

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
