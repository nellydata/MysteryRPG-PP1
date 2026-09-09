#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Area;
class CombatManager;
class Enemy;
class Player;
class ProgressionManager;

class ConsoleUI {
public:
    void RenderDashboard(const Player&, const ProgressionManager&, const std::vector<Area>&,
                         const std::string& context) const;
    void RenderRoom(const Area&, const Player&, const ProgressionManager&) const;
    void RenderCombat(const Player&, const ProgressionManager&, const Enemy&, const CombatManager&) const;
    void PresentEvents(const CombatManager&) const;
    void ShowBattleHistory(const CombatManager&) const;
    void ShowDefeat(const Player&, const CombatManager&) const;
    void ManageInventory(Player&, CombatManager* combat = nullptr, Area* area = nullptr) const;
    void ManageShimmerSkills(ProgressionManager&) const;
    void ShowMap(const std::vector<Area>&) const;
    void ShowMessage(const std::string&) const;
    void PresentLevelUpRewards(ProgressionManager&) const;
    bool ChoosePendingShimmerSkill(ProgressionManager&) const;
    int Prompt(const std::string&, int minimum, int maximum) const;
    static void SetFast(bool fast);
    static bool IsFast();

private:
    mutable std::uint64_t lastEventSequence_ = 0;
    void PrintPlayerPanel(const Player&, const ProgressionManager&) const;
};
