#include "ProgressionManager.h"
#include "Player.h"
#include <iostream>

ProgressionManager::ProgressionManager()
    : level_(1), xp_(0), xpToNextLevel_(100) {}

void ProgressionManager::GrantXP(int amount) {
    if (amount > 0) xp_ += amount;
}

bool ProgressionManager::CheckLevelUp(Player& player) {
    bool leveledUp = false;

    while (xp_ >= xpToNextLevel_) {
        xp_ -= xpToNextLevel_;
        ++level_;
        player.increaseMaxHP(2);
        player.increaseAttack(1);
        player.increaseDefense(1);
        player.heal(5);
        leveledUp = true;

        std::cout << "Level up! Adelaide is now level " << level_ << ".\n"
                  << "Max HP +2, Attack +1, Defense +1, HP restored +5.\n";
    }

    return leveledUp;
}

int ProgressionManager::GetLevel() const { return level_; }
int ProgressionManager::GetXP() const { return xp_; }
