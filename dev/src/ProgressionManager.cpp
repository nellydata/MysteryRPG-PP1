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
        player.SetLevelStats(level_);
        xpToNextLevel_ = static_cast<int>(xpToNextLevel_ * 1.10f + 0.5f);
        leveledUp = true;

        std::cout << "Level up! Adelaide is now level " << level_ << ".\n"
                   << "Max HP +5, Defense +10, Attack +15.\n";
    }

    return leveledUp;
}

int ProgressionManager::GetLevel() const { return level_; }
int ProgressionManager::GetXP() const { return xp_; }
int ProgressionManager::GetXPToNextLevel() const { return xpToNextLevel_; }
void ProgressionManager::SetMaxLevel(Player& player) {
    level_ = 100;
    xp_ = 0;
    xpToNextLevel_ = 0;
    player.SetLevelStats(level_);
    player.heal(player.getMaxHP());
}

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
