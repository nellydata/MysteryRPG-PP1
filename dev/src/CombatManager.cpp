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
      playerGauge_(0), enemyGauge_(0),
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
        while (playerGauge_ < 1000 && enemyGauge_ < 1000) {
            playerGauge_ += (std::max)(1, player.getSpeed());
            enemyGauge_ += (std::max)(1, enemy.getSpeed());
        }
        playerTurn_ = playerGauge_ >= 1000;
    }
    while (!playerTurn_ && player.isAlive() && enemy.isAlive()) enemyTurn(player, enemy);
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
    events_.clear();
    AddEvent(CombatEvent::Kind::Info, "Combat begins against " + enemy.getName() + ".");
}

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, ProgressionManager* progression) {
    BeginEncounter(enemy);
    blocked_ = false;
    if (turnNumber_ == 0 && playerGauge_ == 0 && enemyGauge_ == 0) {
        while (playerGauge_ < 1000 && enemyGauge_ < 1000) {
            playerGauge_ += (std::max)(1, player.getSpeed());
            enemyGauge_ += (std::max)(1, enemy.getSpeed());
        }
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
        if (enemy.isAlive()) playerTurn_ = false;
        playerGauge_ -= 1000;
        return true;
    }
    case CombatAction::Shimmer:
    {
        if (skill == nullptr) skill = defaultSkill_.get();
        if (skill == nullptr || !skill->IsReady()) {
            std::cout << "The Shimmer is not available yet.\n";
            return false;
        }
        const int enemyHpBefore = enemy.getHP();
        const int playerHpBefore = player.getHP();
        const bool enemyHadStun = enemy.HasStatus(StatusType::Stun);
        const bool playerHadBarrier = player.HasStatus(StatusType::Barrier);
        const std::size_t enemyStatusCount = enemy.GetStatuses().size();
        const std::size_t playerStatusCount = player.GetStatuses().size();
        skill->apply(player, enemy);
        const int damage = enemyHpBefore - enemy.getHP();
        const int healing = player.getHP() - playerHpBefore;
        if (damage > 0) AddEvent(CombatEvent::Kind::Damage, player.getName() + " inflicted " + std::to_string(damage) + " damage!", damage);
        if (healing > 0) AddEvent(CombatEvent::Kind::Heal, player.getName() + " healed for " + std::to_string(healing) + ".", healing);
        auto statusName = [](StatusType type) {
            switch (type) {
            case StatusType::Stun: return std::string("Stun");
            case StatusType::Bleed: return std::string("Bleed");
            case StatusType::Burn: return std::string("Burn");
            case StatusType::Poison: return std::string("Poison");
            case StatusType::Slow: return std::string("Slow");
            case StatusType::VerdigrisMark: return std::string("Verdigris mark");
            default: return std::string("Status");
            }
        };
        if (enemy.GetStatuses().size() > enemyStatusCount) {
            const auto& status = enemy.GetStatuses().back();
            AddEvent(CombatEvent::Kind::Status, enemy.getName() + " is affected by " + statusName(status.type) +
                " for " + std::to_string(status.turns) + " turns!");
        }
        if (player.GetStatuses().size() > playerStatusCount) {
            const auto& status = player.GetStatuses().back();
            AddEvent(CombatEvent::Kind::Status, player.getName() + " is affected by " + statusName(status.type) +
                " for " + std::to_string(status.turns) + " turns!");
        }
        if (!playerHadBarrier && player.HasStatus(StatusType::Barrier)) AddEvent(CombatEvent::Kind::Shield, player.getName() + " gained a shield.");
        skill->ResetCooldown();
        if (enemy.isAlive()) playerTurn_ = false;
        playerGauge_ -= 1000;
        return true;
    }
    case CombatAction::Interrogate: {
        const bool result = interrogate(player, enemy, progression);
        if (result && enemy.isAlive()) playerTurn_ = false;
        if (result) AddEvent(CombatEvent::Kind::Info, player.getName() + " interrogated " + enemy.getName() + ".");
        return result;
    }
    case CombatAction::ExposeWeakness:
        weaknessExposed_ = true;
        AddEvent(CombatEvent::Kind::Info, enemy.getName() + "'s weakness was exposed.");
        playerTurn_ = false;
        AddEvent(CombatEvent::Kind::Status, enemy.getName() + " is stunned and loses its turn!");
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

    AddEvent(CombatEvent::Kind::Turn, enemy.getName() + "'s turn.");

    if (enemy.HasStatus(StatusType::Stun)) {
        std::cout << enemy.getName() << " loses its turn to the stun.\n";
        enemyGauge_ -= 1000;
        playerTurn_ = false;
        while (playerGauge_ < 1000 && enemyGauge_ < 1000) {
            playerGauge_ += (std::max)(1, player.getSpeed());
            enemyGauge_ += (std::max)(1, enemy.getSpeed());
        }
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
    while (playerGauge_ < 1000 && enemyGauge_ < 1000) {
        playerGauge_ += (std::max)(1, player.getSpeed());
        enemyGauge_ += (std::max)(1, enemy.getSpeed());
    }
    playerTurn_ = playerGauge_ >= 1000;
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
