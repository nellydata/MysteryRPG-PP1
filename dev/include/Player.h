#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Clue.h"
#include "Item.h"

class Player {
public:
    Player(const std::string& name = "Adelaide", int maxHp = 30, int maxSp = 10);
    ~Player();

    void takeDamage(int amount);
    void heal(int amount);
    bool isAlive() const;

    const std::string& getName() const;
    int getHP() const;
    int getMaxHP() const;
    int getAttack() const;
    int getDefense() const;
    int getSP() const;
    std::size_t GetInventorySize() const;

    void increaseMaxHP(int amount);
    void increaseAttack(int amount);
    void increaseDefense(int amount);
    void spendSP(int amount);
    void gainSP(int amount);
    void AddItem(std::unique_ptr<Item> item);
    void AddClue(const Clue& clue);
    void PrintInventory() const;

private:
    std::string name_;
    int hp_;
    int maxHp_;
    int attack_;
    int defense_;
    int sp_;
    int maxSp_;
    std::vector<std::unique_ptr<Item>> inventory_;
    std::vector<Clue> clues_;
};
