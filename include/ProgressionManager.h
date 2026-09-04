#pragma once

class Player;

class ProgressionManager {
public:
    ProgressionManager();

    void GrantXP(int amount);
    bool CheckLevelUp(Player& player);
    int GetLevel() const;
    int GetXP() const;

private:
    int level_;
    int xp_;
    int xpToNextLevel_;
};
