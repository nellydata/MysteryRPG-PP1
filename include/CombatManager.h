#pragma once

class Player;
class Enemy;

class CombatManager {
public:
    CombatManager();
    ~CombatManager();

    void Attack(Enemy& enemy, int damage);
    void EnemyAttack(Enemy& enemy);
};
