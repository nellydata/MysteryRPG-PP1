#pragma once

class Player;

class ProgressionManager {
public:
    ProgressionManager();

    void awardXP(Player& player, int amount);
    int getXP() const;
    int getLevel() const;
    int getXPToNextLevel() const;

private:
    void levelUp(Player& player);

    int xp_;
    int level_;
};
