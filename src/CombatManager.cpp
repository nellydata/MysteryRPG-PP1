#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ShimmerSkill.h"
#include <iostream>

CombatManager::CombatManager() : blocked_(false), weaknessExposed_(false) {}
CombatManager::~CombatManager() = default;

bool CombatManager::resolve(CombatAction action, Player& player, Enemy& enemy,
                            ShimmerSkill* skill, int itemHealing) {
    blocked_ = false;

    switch (action) {
    case CombatAction::Attack:
        enemy.takeDamage(6 + (weaknessExposed_ ? 2 : 0));
        std::cout << "Adelaide attacks " << enemy.getName() << ".\n";
        return true;
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
        if (itemHealing <= 0 || player.getHP() <= 0) return false;
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
    player.takeDamage(damage);
    blocked_ = false;
    std::cout << enemy.getName() << " attacks for " << damage << " damage.\n";
}
