#include "ProgressionManager.h"
#include "Player.h"
#include <iostream>

ProgressionManager::ProgressionManager()
    : level_(1), xp_(0), xpToNextLevel_(100), shimmerPointsUsed_(0) {}

void ProgressionManager::GrantXP(int amount) {
    if (amount > 0) xp_ += amount;
}

bool ProgressionManager::CheckLevelUp(Player& player) {
    bool leveledUp = false;

    while (level_ < 100 && xpToNextLevel_ > 0 && xp_ >= xpToNextLevel_) {
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
    // Keep the threshold valid so future XP rewards cannot create an endless
    // level-up loop while the player remains at the completion-mode level.
    xpToNextLevel_ = 100;
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

int ProgressionManager::GetShimmerPoints() const { return 10 - shimmerPointsUsed_; }
bool ProgressionManager::EquipShimmerSkill(const std::string& skillName, int cost) {
    if (!HasShimmerSkill(skillName) || cost <= 0 || equippedShimmerSkills_.count(skillName) != 0 || shimmerPointsUsed_ + cost > 10) return false;
    equippedShimmerSkills_.insert(skillName);
    shimmerPointsUsed_ += cost;
    return true;
}

bool ProgressionManager::UnlockShimmerFromTrigger(const std::string& trigger) {
    static const std::pair<const char*, const char*> unlocks[] = {
        {"mirror", "Phragmós Phōtós"}, {"Matron of the Catacombs", "Therapeía Anakláseōs"},
        {"Catacombs", "Anástasis Phōtós"}, {"corpse", "Siphon Lux"},
        {"statue", "Phthorá Phōtós"}, {"Overlord-Prime", "Refractio Ultima"},
        {"light-spear", "Lumen Spiculum"}, {"relic", "Phōteinḕ Kátharsis"},
        {"vortex", "Lux Vortex"}, {"Oracle Core", "Phōteinòs Thṓrax"}
    };
    for (const auto& unlock : unlocks) if (trigger.find(unlock.first) != std::string::npos) return UnlockShimmerSkill(unlock.second);
    return false;
}
