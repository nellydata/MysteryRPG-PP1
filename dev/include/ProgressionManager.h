#pragma once

#include <set>
#include <string>
#include <vector>

class Player;

class ProgressionManager {
public:
    ProgressionManager();

    void GrantXP(int amount);
    void GrantClueXP();
    bool CheckLevelUp(Player& player);
    int GetLevel() const;
    void SetMaxLevel(Player& player);
    int GetXP() const;
    int GetXPToNextLevel() const;
    bool UnlockCrux(const std::string& itemName);
    bool HasCrux(const std::string& itemName) const;
    bool UnlockShimmerSkill(const std::string& skillName);
    bool HasShimmerSkill(const std::string& skillName) const;
    std::vector<std::string> GetUnlockedShimmerSkills() const;
    std::vector<std::string> GetEquippedShimmerSkills() const;
    int GetShimmerPoints() const;
    bool EquipShimmerSkill(const std::string& skillName, int cost);
    bool UnequipShimmerSkill(const std::string& skillName, int cost);
    bool UnlockShimmerFromTrigger(const std::string& trigger);
    std::string RollShimmerSkill();
    bool HasPendingShimmerChoice() const;
    const std::vector<std::string>& GetPendingShimmerChoices() const;
    bool SelectPendingShimmerSkill(const std::string& skillName);
    std::vector<std::string> ConsumeLevelUpMessages();

private:
    int level_;
    int xp_;
    int xpToNextLevel_;
    std::set<std::string> cruxItems_;
    std::set<std::string> shimmerSkills_;
    std::set<std::string> equippedShimmerSkills_;
    int shimmerPointsUsed_;
    std::vector<std::string> pendingShimmerChoices_;
    std::vector<std::string> levelUpMessages_;

    void QueueLevelUpChoice();
    void AddLevelUpEquipmentRewards(Player& player);
};
