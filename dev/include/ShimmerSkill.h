#pragma once

#include <string>
#include <memory>

class Player;
class Enemy;

class ShimmerSkill {
public:
	ShimmerSkill(const std::string& name, int cooldown);
	virtual ~ShimmerSkill();

	virtual void apply(Player& player, Enemy& enemy) = 0;

	const std::string& getName() const;
	int getCost() const;
	int getCooldown() const;
	bool IsReady() const;
	void TickCooldown();
	void ResetCooldown();
	static std::unique_ptr<ShimmerSkill> Create(const std::string& skillName);

protected:
	std::string name_;
	int cooldown_;
	int remainingCooldown_;
};
