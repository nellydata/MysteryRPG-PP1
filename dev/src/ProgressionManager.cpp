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
        player.increaseMaxHP(5);
        player.increaseDefense(1);
        player.heal(5);
        leveledUp = true;

        std::cout << "Level up! Adelaide is now level " << level_ << ".\n"
                   << "Max HP +5, Defense +1, HP restored +5.\n";
    }

    return leveledUp;
}

int ProgressionManager::GetLevel() const { return level_; }
int ProgressionManager::GetXP() const { return xp_; }

bool ProgressionManager::UnlockCrux(const std::string& itemName) {
    return !itemName.empty() && cruxItems_.insert(itemName).second;
}

bool ProgressionManager::HasCrux(const std::string& itemName) const {
    return cruxItems_.find(itemName) != cruxItems_.end();
}

bool ProgressionManager::UnlockShimmerSkill(const std::string& skillName) {
    return !skillName.empty() && shimmerSkills_.insert(skillName).second;
}

bool ProgressionManager::HasShimmerSkill(const std::string& skillName) const {
    return shimmerSkills_.find(skillName) != shimmerSkills_.end();
}
