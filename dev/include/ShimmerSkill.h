#pragma once

#include <string>
#include <memory>

class Player;
class Enemy;

class ShimmerSkill {
public:
	enum class TargetType { Self, SingleEnemy, AllEnemies };
	ShimmerSkill(const std::string& name, int equipCost, int cooldown,
		const std::string& description = "", const std::string& effect = "");
	ShimmerSkill(const std::string& name, int equipCost, int cooldown,
		const std::string& description, const std::string& effect, int powerPercent, int duration);
	ShimmerSkill(const std::string& name, int cooldown,
		const std::string& description, const std::string& effect);
	virtual ~ShimmerSkill();

	virtual void apply(Player& player, Enemy& enemy) = 0;

	const std::string& getName() const;
	const std::string& getDescription() const;
	const std::string& getEffect() const;
	int getCost() const;
	int getEquipCost() const;
	int getCooldown() const;
	int getPowerPercent() const;
	int getDuration() const;
	TargetType getTargetType() const;
	int getRemainingCooldown() const;
	bool IsReady() const;
	void TickCooldown();
	void ResetCooldown();
	static std::unique_ptr<ShimmerSkill> Create(const std::string& skillName);

protected:
	std::string name_;
	std::string description_;
	std::string effect_;
	int equipCost_;
	int cooldown_;
	int powerPercent_;
	int duration_;
	TargetType targetType_ = TargetType::SingleEnemy;
	int remainingCooldown_;
};
