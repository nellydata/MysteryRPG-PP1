#pragma once

#include <string>

class Enemy {
public:
    Enemy(const std::string& name, int hp, int atk, int xpReward = 15);
    virtual ~Enemy();

    virtual void takeDamage(int amount);
    bool isAlive() const;
    const std::string& getName() const;
    int getHP() const;
    int getAtk() const;
    int getXPReward() const;

protected:
    std::string name_;
    int hp_;
    int atk_;
    int xpReward_;
};
