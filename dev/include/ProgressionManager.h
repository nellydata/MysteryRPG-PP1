#pragma once

#include <set>
#include <string>
#include <vector>
#include <cstdint>
#include <deque>

class Player;

std::int64_t XPRequirementForLevel(int level);
std::int64_t CalculateEnemyXP(int enemyLevel, int enemyTier);
std::int64_t CalculateInvestigationXP(int playerLevel, int areaDepth);
int InvestigationAreaDepth(const std::string& areaName);

class ProgressionManager {
public:
    ProgressionManager();

    void GrantXP(std::int64_t amount);
    void GrantClueXP();
    void GrantInvestigationXP(const std::string& areaName);
    bool CheckLevelUp(Player& player);
    int GetLevel() const;
    void SetMaxLevel(Player& player);
    std::int64_t GetXP() const;
    std::int64_t GetXPToNextLevel() const;
    bool UnlockCrux(const std::string& itemName);
    bool HasCrux(const std::string& itemName) const;
    bool UnlockShimmerSkill(const std::string& skillName);
    bool HasShimmerSkill(const std::string& skillName) const;
    std::vector<std::string> GetUnlockedShimmerSkills() const;
    std::vector<std::string> GetEquippedShimmerSkills() const;
    int GetShimmerPoints() const;
    bool EquipShimmerSkill(const std::string& skillName);
    bool UnequipShimmerSkill(const std::string& skillName);
    // Compatibility overloads reject a cost that differs from the catalogue.
    bool EquipShimmerSkill(const std::string& skillName, int cost);
    bool UnequipShimmerSkill(const std::string& skillName, int cost);
    bool UnlockShimmerFromTrigger(const std::string& trigger);
    std::string RollShimmerSkill();
    bool HasPendingShimmerChoice() const;
    const std::vector<std::string>& GetPendingShimmerChoices() const;
    bool SelectPendingShimmerSkill(const std::string& skillName);
    bool ResolvePendingShimmerChoice();
    std::vector<std::string> ConsumeLevelUpMessages();

private:
    int level_;
    std::int64_t xp_;
    std::int64_t xpToNextLevel_;
    std::set<std::string> cruxItems_;
    std::set<std::string> shimmerSkills_;
    std::set<std::string> equippedShimmerSkills_;
    struct LevelUpReward {
        int levelReached = 1;
        std::vector<std::string> shimmerChoices;
    };
    std::deque<LevelUpReward> pendingLevelUpRewards_;
    std::vector<std::string> levelUpMessages_;

    void QueueLevelUpChoice();
    void AddLevelUpEquipmentRewards(Player& player);
};
