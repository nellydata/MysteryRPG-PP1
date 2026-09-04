#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <iostream>

CombatManager::CombatManager() : blocked_(false), weaknessExposed_(false) {}
CombatManager::~CombatManager() = default;

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, int itemHealing,
                            ProgressionManager* progression) {
    blocked_ = false;

    switch (action) {
    case CombatAction::Attack: {
        const int damage = player.getAttack() + (weaknessExposed_ ? 2 : 0);
        enemy.takeDamage(damage);
        std::cout << "Adelaide attacks " << enemy.getName()
                  << " for " << damage << " damage.\n";
        if (!enemy.isAlive() && progression != nullptr) {
            progression->GrantXP(enemy.getXPReward());
            progression->CheckLevelUp(player);
        }
        return true;
    }
    case CombatAction::Block:
        blocked_ = true;
        std::cout << "Adelaide braces for the next attack.\n";
        return true;
    case CombatAction::Shimmer:
        if (skill == nullptr || player.getSP() < skill->getCost()) {
            std::cout << "The Shimmer remains out of reach.\n";
            return false;
        }
        player.spendSP(skill->getCost());
        skill->apply(player, enemy);
        return true;
    case CombatAction::Interrogate:
        std::cout << enemy.getName() << " reveals a fragment of cathedral lore.\n";
        return true;
    case CombatAction::ExposeWeakness:
        weaknessExposed_ = true;
        std::cout << "A weakness in " << enemy.getName() << " is exposed.\n";
        return true;
    case CombatAction::Item:
        if (itemHealing <= 0 || !player.isAlive()) return false;
        player.heal(itemHealing);
        std::cout << "Adelaide uses an item and recovers health.\n";
        return true;
    case CombatAction::Flee:
        std::cout << "Adelaide retreats from the encounter.\n";
        return true;
    }

    return false;
}

void CombatManager::enemyTurn(Player& player, Enemy& enemy) {
    if (!enemy.isAlive()) return;

    const int damage = blocked_ ? enemy.getAtk() / 2 : enemy.getAtk();
    const int reducedDamage = damage > player.getDefense()
        ? damage - player.getDefense()
        : 0;
    player.takeDamage(reducedDamage);
    blocked_ = false;
    std::cout << enemy.getName() << " attacks for " << reducedDamage << " damage.\n";
}
