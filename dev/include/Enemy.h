#pragma once

#include <string>
#include <vector>
#include "GameplayTypes.h"

class Enemy {
public:
    Enemy(const std::string& name, int hp, int atk, int xpReward = 15, int level = 1);
    virtual ~Enemy();

    virtual void takeDamage(int amount);
    bool isAlive() const;
    const std::string& getName() const;
    int getHP() const;
    int getMaxHP() const;
    int getAtk() const;
    int getXPReward() const;
    int getDefense() const;
    int getSpeed() const;
    int getLevel() const;
    bool HasStatus(StatusType type) const;
    void ApplyStatus(const StatusEffect& status);
    void ProcessStatuses();
    void ClearStatuses();

protected:
    std::string name_;
    int hp_;
    int maxHp_;
    int atk_;
    int xpReward_;
    int defense_;
    int speed_;
    int level_;
    std::vector<StatusEffect> statuses_;
};
