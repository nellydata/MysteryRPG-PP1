#include "ProgressionManager.h"
#include "Player.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <iostream>
#include <random>

ProgressionManager::ProgressionManager()
    : level_(1), xp_(0), xpToNextLevel_(100), shimmerPointsUsed_(0) {}

void ProgressionManager::GrantXP(int amount) {
    if (amount > 0) xp_ += amount;
}

void ProgressionManager::GrantClueXP() {
    GrantXP((std::max)(1, xpToNextLevel_ / 4));
}

bool ProgressionManager::UnequipShimmerSkill(const std::string& skillName, int cost) {
    if (cost <= 0 || equippedShimmerSkills_.erase(skillName) == 0) return false;
    shimmerPointsUsed_ = (std::max)(0, shimmerPointsUsed_ - cost);
    return true;
}

bool ProgressionManager::CheckLevelUp(Player& player) {
    bool leveledUp = false;

    while (level_ < 100 && xpToNextLevel_ > 0 && xp_ >= xpToNextLevel_) {
        xp_ -= xpToNextLevel_;
        ++level_;
        player.SetLevelStats(level_);
        player.RestoreHealth(player.getMaxHP());
        xpToNextLevel_ = static_cast<int>(xpToNextLevel_ * 1.10f + 0.5f);
        const std::string reward = RollShimmerSkill();
        if (!reward.empty()) UnlockShimmerSkill(reward);
        leveledUp = true;

        std::cout << "Level up! Adelaide is now level " << level_ << ".\n"
                   << "Recovered to full health and discovered Shimmer skill " << reward << ".\n";
    }

    return leveledUp;
}

std::string ProgressionManager::RollShimmerSkill() {
    static std::mt19937 generator(0x4D494341u);
    static const std::vector<std::pair<int, int>> costs = {{2, 45}, {3, 30}, {4, 17}, {5, 8}};
    std::uniform_int_distribution<int> roll(1, 100);
    const int targetCost = [&]() {
        int value = roll(generator);
        for (const auto& entry : costs) {
            if (value <= entry.second) return entry.first;
            value -= entry.second;
        }
        return 2;
    }();
    std::vector<std::string> candidates;
    static const std::vector<std::string> names = {
        "Ray of Mercy", "Prism Step", "Gleam Needle", "Phragmós Phōtós", "Therapeía Anakláseōs", "Anástasis Phōtós", "Phthorá Phōtós",
        "Phōteinḕ Kátharsis", "Phōteinòs Thṓrax", "Refractio Ultima", "Lumen Spiculum",
        "Lux Vortex", "Siphon Lux", "Lux Aegis", "Refraction Cataclysm", "Lux Ascension"
    };
    for (const auto& name : names) {
        auto skill = ShimmerSkill::Create(name);
        if (skill != nullptr && skill->getEquipCost() == targetCost && !HasShimmerSkill(name)) candidates.push_back(name);
    }
    if (candidates.empty()) {
        for (const auto& name : names) if (!HasShimmerSkill(name)) return name;
        return {};
    }
    return candidates[static_cast<std::size_t>(roll(generator) % candidates.size())];
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

std::vector<std::string> ProgressionManager::GetUnlockedShimmerSkills() const {
    return std::vector<std::string>(shimmerSkills_.begin(), shimmerSkills_.end());
}

std::vector<std::string> ProgressionManager::GetEquippedShimmerSkills() const {
    return std::vector<std::string>(equippedShimmerSkills_.begin(), equippedShimmerSkills_.end());
}

int ProgressionManager::GetShimmerPoints() const { return 12 - shimmerPointsUsed_; }
bool ProgressionManager::EquipShimmerSkill(const std::string& skillName, int cost) {
    if (!HasShimmerSkill(skillName) || cost <= 0 || equippedShimmerSkills_.count(skillName) != 0 || shimmerPointsUsed_ + cost > 12) return false;
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
