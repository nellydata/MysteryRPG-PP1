#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

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
    void ContinueEncounter(Enemy& enemy);
    void PrepareTurn(Player& player, Enemy& enemy);
    void PrepareGroupTurn(Player& player, const std::vector<Enemy*>& enemies);
    void ResumeAfterRespawn(Player& player, Enemy& enemy);
    bool resolve(CombatAction action, Player& player, Enemy& enemy,
                 ShimmerSkill* skill = nullptr,
                 ProgressionManager* progression = nullptr);
    bool UseConsumable(const std::string& itemName, Player& player, Enemy& enemy);
    void enemyTurn(Player& player, Enemy& enemy);
    void enemyTurn(Player& player, Enemy& enemy, const std::vector<Enemy*>& enemies);
    const ShimmerSkill* GetDefaultSkill() const;
    bool IsPlayerTurn() const;
    bool HasUsedConsumableThisTurn() const;
    const std::vector<CombatEvent>& GetEvents() const;
    void RecordInfo(const std::string& message);
    int GetSkillCooldown(const std::string& skillName) const;
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
    bool groupEnemyTurn_ = false;
    std::map<std::string, int> shimmerCooldowns_;
    std::vector<Enemy*> currentEnemyParty_;

    void AddEvent(CombatEvent::Kind kind, const std::string& message, int value = 0);
    void StartPlayerTurn(Player& player);
};
