#pragma once

#include <string>
#include <memory>

class Player;
class Enemy;

class ShimmerSkill {
public:
	ShimmerSkill(const std::string& name, int cooldown,
		const std::string& description = "", const std::string& effect = "");
	virtual ~ShimmerSkill();

	virtual void apply(Player& player, Enemy& enemy) = 0;

	const std::string& getName() const;
	const std::string& getDescription() const;
	const std::string& getEffect() const;
	int getCost() const;
	int getCooldown() const;
	int getRemainingCooldown() const;
	bool IsReady() const;
	void TickCooldown();
	void ResetCooldown();
	static std::unique_ptr<ShimmerSkill> Create(const std::string& skillName);

protected:
	std::string name_;
	std::string description_;
	std::string effect_;
	int cooldown_;
	int remainingCooldown_;
};
