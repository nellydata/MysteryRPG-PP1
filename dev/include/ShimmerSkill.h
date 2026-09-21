#pragma once

#include <string>
#include <memory>

class Player;
class Enemy;

class ShimmerSkill {
public:
	enum class TargetType { Self, SingleEnemy, AllEnemies };
	struct CombatBuff {
		int attackPercent = 0;
		int defensePercent = 0;
		int speedPercent = 0;
		int critRatePoints = 0;
		int critDamagePoints = 0;
		int damageReductionPoints = 0;
		// One successful cast adds this much progress toward the next action.
		// CombatManager owns the 1000-point threshold and applies the progress.
		int nextActionGauge = 0;
	};
	struct EffectMetadata {
		int statusPotency = 0;
		int dotPotency = 0;
		int healOverTimePercent = 0;
		bool normalActionsOnly = false;
	};
	ShimmerSkill(const std::string& name, int equipCost, int cooldown,
		const std::string& description = "", const std::string& effect = "");
	ShimmerSkill(const std::string& name, int equipCost, int cooldown,
		const std::string& description, const std::string& effect, int powerPercent, int duration);
	ShimmerSkill(const std::string& name, int equipCost, int cooldown,
		const std::string& description, const std::string& effect, int powerPercent, int duration,
		const CombatBuff& combatBuff, const EffectMetadata& effectMetadata = EffectMetadata{});
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
	const CombatBuff& getCombatBuff() const;
	const EffectMetadata& getEffectMetadata() const;
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
	CombatBuff combatBuff_;
	EffectMetadata effectMetadata_;
	TargetType targetType_ = TargetType::SingleEnemy;
	int remainingCooldown_;
};
