#pragma once

class Player;
class Enemy;
class ShimmerSkill;

enum class CombatAction {
    Attack,
    Block,
    Shimmer,
    Interrogate,
    ExposeWeakness,
    Item,
    Flee
};

class CombatManager {
public:
    CombatManager();
    ~CombatManager();

    bool resolve(CombatAction action, Player& player, Enemy& enemy,
                 ShimmerSkill* skill = nullptr, int itemHealing = 0);
    void enemyTurn(Player& player, Enemy& enemy);

private:
    bool blocked_;
    bool weaknessExposed_;
};
