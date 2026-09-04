#pragma once

#include <string>

class Player;
class Enemy;

class ShimmerSkill {
public:
	ShimmerSkill(const std::string& name, int cost);
	virtual ~ShimmerSkill();

	virtual void apply(Player& player, Enemy& enemy) = 0;

	const std::string& getName() const;
	int getCost() const;

protected:
	std::string name_;
	int cost_;
};
