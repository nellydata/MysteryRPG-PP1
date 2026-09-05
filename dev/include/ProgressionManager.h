#pragma once

#include <set>
#include <string>

class Player;

class ProgressionManager {
public:
    ProgressionManager();

    void GrantXP(int amount);
    bool CheckLevelUp(Player& player);
    int GetLevel() const;
    void SetMaxLevel(Player& player);
    int GetXP() const;
    int GetXPToNextLevel() const;
    bool UnlockCrux(const std::string& itemName);
    bool HasCrux(const std::string& itemName) const;
    bool UnlockShimmerSkill(const std::string& skillName);
    bool HasShimmerSkill(const std::string& skillName) const;
    int GetShimmerPoints() const;
    bool EquipShimmerSkill(const std::string& skillName, int cost);
    bool UnlockShimmerFromTrigger(const std::string& trigger);

private:
    int level_;
    int xp_;
    int xpToNextLevel_;
    std::set<std::string> cruxItems_;
    std::set<std::string> shimmerSkills_;
    std::set<std::string> equippedShimmerSkills_;
    int shimmerPointsUsed_;
};
