#pragma once

#include <string>
#include <vector>
#include <memory>
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

	void increaseMaxHP(int amount);
	void increaseAttack(int amount);
	void increaseDefense(int amount);
	void spendSP(int amount);
	void gainSP(int amount);

private:
	std::string name_;
	int hp_;
	int maxHp_;
	int attack_;
	int defense_;
	int sp_;
	int maxSp_;
	std::vector<std::unique_ptr<Item>> inventory_;
};
