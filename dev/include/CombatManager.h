#pragma once

#include <memory>
#include <string>
#include <vector>

class Player;
class Enemy;
class ShimmerSkill;
class ProgressionManager;

enum class CombatAction {
    Attack,
    Item,
    Shimmer,
    Flee
};

class CombatManager {
public:
    struct CombatEvent {
        enum class Kind {
            Turn, Attack, Skill, Damage, DotTick, Heal, Shield, Buff, Debuff,
            Cleanse, ExtraTurn, PhaseTransition, Shimmer, Mutation, Death,
            Revive, Environment, Status, Info
        };
        Kind kind;
        std::string message;
        int value = 0;
    };
    CombatManager();
    ~CombatManager();

    void BeginEncounter(Enemy& enemy);
    void PrepareTurn(Player& player, Enemy& enemy);
    bool resolve(CombatAction action, Player& player, Enemy& enemy,
                 ShimmerSkill* skill = nullptr,
                 ProgressionManager* progression = nullptr);
    bool UseConsumable(const std::string& itemName, Player& player, Enemy& enemy);
    void enemyTurn(Player& player, Enemy& enemy);
    const ShimmerSkill* GetDefaultSkill() const;
    bool IsPlayerTurn() const;
    bool HasUsedConsumableThisTurn() const;
    const std::vector<CombatEvent>& GetEvents() const;
    void ClearEvents();

private:
    bool interrogate(Player& player, Enemy& enemy,
                     ProgressionManager* progression);

    bool blocked_;
    bool weaknessExposed_;
    int interrogationAttempts_;
    int turnNumber_;
    bool playerTurn_;
    int playerGauge_;
    int enemyGauge_;
    Enemy* activeEnemy_;
    std::unique_ptr<ShimmerSkill> defaultSkill_;
    std::vector<CombatEvent> events_;
    bool consumableUsedThisTurn_;

    void AddEvent(CombatEvent::Kind kind, const std::string& message, int value = 0);
};
