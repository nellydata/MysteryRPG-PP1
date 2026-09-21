#include "ShimmerSkill.h"
#include "Enemy.h"
#include "Player.h"
#include "CombatMath.h"
#include <algorithm>
#include <cstdint>

namespace {
int DirectShimmerDamage(const Player& player, int powerPercent) {
	// Catalogue powers and the set multiplier are bounded; promote before all
	// multiplication and round only once, after both percentage modifiers.
	const std::int64_t raw = static_cast<std::int64_t>(player.getAttack()) * powerPercent *
		player.GetShimmerDamageMultiplierPercent() / 10000;
	return (std::max)(1, CombatMath::Saturate(raw));
}

class DefinedShimmerSkill final : public ShimmerSkill {
public:
	DefinedShimmerSkill(const std::string& name, int equipCost, int cooldown, const std::string& description,
		const std::string& effect, int powerPercent = 0, int duration = 0,
		const CombatBuff& combatBuff = CombatBuff{}, const EffectMetadata& metadata = EffectMetadata{})
		: ShimmerSkill(name, equipCost, cooldown, description, effect, powerPercent, duration, combatBuff, metadata) {}

	void apply(Player& player, Enemy& enemy) override {
		if (!player.isAlive() || (targetType_ != TargetType::Self && !enemy.isAlive())) return;
		const auto selfStatus = [&](StatusType type, int duration, int potency) {
			player.ApplyStatus({type, duration, potency, player.getAttack(), player.getDotPotency(),
				player.getDefense(), player.getName(), player.GetCombatantId(), 0,
				effect_ == "stat_boost" && effectMetadata_.normalActionsOnly});
		};
		const auto enemyStatus = [&](StatusType type, int duration, int potency, int authoredDotPotency = 0) {
			if (!enemy.isAlive()) return;
			enemy.ApplyStatus({type, duration, potency, player.getAttack(),
				CombatMath::Saturate(static_cast<std::int64_t>(authoredDotPotency) + player.getDotPotency()),
				player.getDefense(), player.getName(), player.GetCombatantId()}, player.getAccuracy());
		};
		if (effect_ == "barrier") {
			player.AddBarrier(CombatMath::Scale(player.getMaxHP(), powerPercent_), duration_);
		} else if (effect_ == "heal_hot") {
			player.heal(CombatMath::Scale(player.getMaxHP(), powerPercent_));
			selfStatus(StatusType::HealOverTime, duration_, CombatMath::Scale(player.getMaxHP(), effectMetadata_.healOverTimePercent));
		} else if (effect_ == "cleanse_resist") {
			player.CleanseNegativeStatuses();
			selfStatus(StatusType::DamageReduction, duration_, effectMetadata_.statusPotency);
		} else if (effect_ == "defensive_buff") {
			player.AddBarrier(CombatMath::Scale(player.getMaxHP(), powerPercent_), duration_);
			selfStatus(StatusType::DamageReduction, duration_, effectMetadata_.statusPotency);
		} else if (effect_ == "damage_ignore") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_), true);
		} else if (effect_ == "damage_burn") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_));
			enemyStatus(StatusType::Burn, duration_, 0, effectMetadata_.dotPotency);
		} else if (effect_ == "damage_stun") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_));
			enemyStatus(StatusType::Stun, duration_ > 0 ? duration_ : 1, 0);
		} else if (effect_ == "damage_tridot") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_));
			for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison})
				enemyStatus(type, duration_, 0, effectMetadata_.dotPotency);
		} else if (effect_ == "damage_lifesteal") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_));
			player.heal(CombatMath::Scale(enemy.GetLastDamageResult().hpDamage, 25));
		} else if (effect_ == "damage_defdown") {
			enemy.ApplyDirectDamage(DirectShimmerDamage(player, powerPercent_));
			enemyStatus(StatusType::DefenseDown, duration_, effectMetadata_.statusPotency);
		} else if (effect_ == "stat_boost") {
			const auto buff = [&](StatusType type, int potency) {
				if (potency > 0) selfStatus(type, duration_, potency);
			};
			buff(StatusType::AttackUp, combatBuff_.attackPercent);
			buff(StatusType::DefenseUp, combatBuff_.defensePercent);
			buff(StatusType::SpeedUp, combatBuff_.speedPercent);
			buff(StatusType::CritUp, combatBuff_.critRatePoints);
			buff(StatusType::CritDamageUp, combatBuff_.critDamagePoints);
			buff(StatusType::DamageReduction, combatBuff_.damageReductionPoints);
		}
	}

};
}

ShimmerSkill::ShimmerSkill(const std::string& name, int equipCost, int cooldown,
	const std::string& description, const std::string& effect)
	: ShimmerSkill(name, equipCost, cooldown, description, effect, 100, 0) {}

ShimmerSkill::ShimmerSkill(const std::string& name, int equipCost, int cooldown, const std::string& description,
	const std::string& effect, int powerPercent, int duration)
	: ShimmerSkill(name, equipCost, cooldown, description, effect, powerPercent, duration, CombatBuff{}, EffectMetadata{}) {}

ShimmerSkill::ShimmerSkill(const std::string& name, int equipCost, int cooldown, const std::string& description,
	const std::string& effect, int powerPercent, int duration, const CombatBuff& combatBuff,
	const EffectMetadata& effectMetadata)
	: name_(name), description_(description), effect_(effect), equipCost_(equipCost), cooldown_(cooldown),
	  powerPercent_(powerPercent), duration_(duration), combatBuff_(combatBuff), effectMetadata_(effectMetadata), remainingCooldown_(0) {
	if (effect_ == "barrier" || effect_ == "heal_hot" || effect_ == "cleanse_resist" || effect_ == "defensive_buff" || effect_ == "stat_boost")
		targetType_ = TargetType::Self;
	}

ShimmerSkill::ShimmerSkill(const std::string& name, int cooldown, const std::string& description, const std::string& effect)
	: ShimmerSkill(name, cooldown, cooldown, description, effect, 100, 0) {}

ShimmerSkill::~ShimmerSkill() = default;

const std::string& ShimmerSkill::getName() const { return name_; }
const std::string& ShimmerSkill::getDescription() const { return description_; }
const std::string& ShimmerSkill::getEffect() const { return effect_; }
int ShimmerSkill::getCost() const { return equipCost_; }
int ShimmerSkill::getEquipCost() const { return equipCost_; }
int ShimmerSkill::getCooldown() const { return cooldown_; }
int ShimmerSkill::getPowerPercent() const { return powerPercent_; }
int ShimmerSkill::getDuration() const { return duration_; }
const ShimmerSkill::CombatBuff& ShimmerSkill::getCombatBuff() const { return combatBuff_; }
const ShimmerSkill::EffectMetadata& ShimmerSkill::getEffectMetadata() const { return effectMetadata_; }
ShimmerSkill::TargetType ShimmerSkill::getTargetType() const { return targetType_; }
int ShimmerSkill::getRemainingCooldown() const { return remainingCooldown_; }
bool ShimmerSkill::IsReady() const { return remainingCooldown_ == 0; }
void ShimmerSkill::TickCooldown() { if (remainingCooldown_ > 0) --remainingCooldown_; }
void ShimmerSkill::ResetCooldown() { remainingCooldown_ = cooldown_; }

std::unique_ptr<ShimmerSkill> ShimmerSkill::Create(const std::string& skillName) {
	if (skillName == "Claustrum Lucis" || skillName == "Velum Radiāns" || skillName == "Requiem Mechanicum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Gain a 1500% max HP barrier for 3 turns.", "barrier", 1500, 3);
	if (skillName == "Radius Misericordiae") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Restore 500% max HP and gain a 100% max HP restorative pulse for 2 turns.", "heal_hot", 500, 2, CombatBuff{}, EffectMetadata{0, 0, 100});
	if (skillName == "Gradus Prismatis") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 4, "Gain a 500% max HP barrier and 20% damage reduction for 2 turns.", "defensive_buff", 500, 2, CombatBuff{}, EffectMetadata{20});
	if (skillName == "Acus Fulgoris") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Deal 500% ATK as a defense-ignoring Shimmer strike.", "damage_ignore", 500, 0);
	if (skillName == "Curatio Refractionis" || skillName == "Lux Sanctorum") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 8, "Restore 3000% max HP and gain a 600% max HP HoT for 3 turns.", "heal_hot", 3000, 3, CombatBuff{}, EffectMetadata{0, 0, 600});
	if (skillName == "Praesidium Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 8, "Gain a one-turn emergency barrier equal to 3500% max HP.", "barrier", 3500, 1);
	if (skillName == "Corruptio Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 7, "Deal 2200% ATK and reduce enemy DEF by 220% for 4 turns.", "damage_defdown", 2200, 4, CombatBuff{}, EffectMetadata{220});
	if (skillName == "Purgatio Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 5, "Cleanse debuffs and gain 30% damage resistance for 3 turns.", "cleanse_resist", 0, 3, CombatBuff{}, EffectMetadata{30});
	if (skillName == "Lorica Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 1000% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 1000, 3, CombatBuff{}, EffectMetadata{20});
	if (skillName == "Refractio Ultima") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Deal 3200% ATK as defense-ignoring damage.", "damage_ignore", 3200, 0);
	if (skillName == "Lumen Spiculum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1500% ATK and apply Burn for 3 turns.", "damage_burn", 1500, 3, CombatBuff{}, EffectMetadata{0, 50});
	if (skillName == "Vortex Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal 2100% ATK and apply Burn, Bleed, and Poison for 3 turns.", "damage_tridot", 2100, 3, CombatBuff{}, EffectMetadata{0, 40});
	if (skillName == "Siphon Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 6, "Deal 1800% ATK and restore 25% of damage dealt.", "damage_lifesteal", 1800, 0);
	if (skillName == "Aegis Lucis") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Gain a 3000% max HP barrier and 30% damage reduction for 3 turns.", "defensive_buff", 3000, 3, CombatBuff{}, EffectMetadata{30});
	if (skillName == "Cataclysmus Refractionis") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Deal 3600% ATK as defense-ignoring damage.", "damage_ignore", 3600, 0);
	if (skillName == "Ascensio Lucis" || skillName == "Onus Coruscum" || skillName == "Resonantia Chori") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Gain 25% ATK, DEF and SPD, 15pp critical rate, 25pp critical damage and 15pp damage reduction for 3 turns.", "stat_boost", 0, 3, CombatBuff{25, 25, 25, 15, 25, 15, 0});
	if (skillName == "Pulsus Luminis") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal 900% ATK as a focused Shimmer strike.", "damage_ignore", 900, 0);
	// The existing authored 800 value was previously unused damage metadata.
	// Interpret it explicitly as next-action gauge progress, separately from
	// the approved 40% core-stat buff. It is a single cast effect, not a tick.
	if (skillName == "Gradus Coruscus" || skillName == "Mutatio Horologii") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain 40% ATK, DEF and SPD for your next 2 normal actions; immediately add 800/1000 gauge progress toward your next action.", "stat_boost", 0, 2, CombatBuff{40, 40, 40, 0, 0, 0, 800}, EffectMetadata{0, 0, 0, true});
	if (skillName == "Echo Lucernae" || skillName == "Catena Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1300% ATK and attempt to stun the enemy for 2 turns.", "damage_stun", 1300, 2);
	if (skillName == "Fulgur Aurorae") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal 2000% ATK as a heavy defense-ignoring strike.", "damage_ignore", 2000, 0);
	if (skillName == "Flos Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1500% ATK and apply Burn for 3 turns.", "damage_burn", 1500, 3, CombatBuff{}, EffectMetadata{0, 50});
	if (skillName == "Scutum Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 1000% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 1000, 3, CombatBuff{}, EffectMetadata{20});
	if (skillName == "Nota Luminosa" || skillName == "Revelatio Occulta") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal 900% ATK and reduce enemy DEF by 90% for 3 turns.", "damage_defdown", 900, 3, CombatBuff{}, EffectMetadata{90});
	return nullptr;
}
