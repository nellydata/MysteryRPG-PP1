#pragma once

#include <memory>

class Player;
class Enemy;
class ShimmerSkill;
class ProgressionManager;

enum class CombatAction {
    Attack,
    Block,
    Shimmer,
    Interrogate,
    ExposeWeakness,
    Flee
};

class CombatManager {
public:
    CombatManager();
    ~CombatManager();

    void BeginEncounter(Enemy& enemy);
    bool resolve(CombatAction action, Player& player, Enemy& enemy,
                 ShimmerSkill* skill = nullptr,
                 ProgressionManager* progression = nullptr);
    void enemyTurn(Player& player, Enemy& enemy);
    const ShimmerSkill* GetDefaultSkill() const;

private:
    bool interrogate(Player& player, Enemy& enemy,
                     ProgressionManager* progression);

    bool blocked_;
    bool weaknessExposed_;
    int interrogationAttempts_;
    int turnNumber_;
    Enemy* activeEnemy_;
    std::unique_ptr<ShimmerSkill> defaultSkill_;
};
