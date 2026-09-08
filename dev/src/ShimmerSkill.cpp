#include "ShimmerSkill.h"
#include "Enemy.h"
#include "Player.h"
#include "CombatMath.h"
#include <algorithm>

namespace {
class DefinedShimmerSkill final : public ShimmerSkill {
public:
	DefinedShimmerSkill(const std::string& name, int equipCost, int cooldown, const std::string& description,
		const std::string& effect, int powerPercent = 0, int duration = 0)
		: ShimmerSkill(name, equipCost, cooldown, description, effect, powerPercent, duration) {}

	void apply(Player& player, Enemy& enemy) override {
		if (effect_ == "barrier") {
			player.AddBarrier(player.getMaxHP() * powerPercent_ / 100, duration_);
		} else if (effect_ == "heal_hot") {
			player.heal(player.getMaxHP() * powerPercent_ / 100);
			player.ApplyStatus({StatusType::HealOverTime, duration_, player.getMaxHP() * powerPercent_ / 500, 0});
		} else if (effect_ == "cleanse_resist") {
			player.CleanseNegativeStatuses();
			player.ApplyStatus({StatusType::DamageReduction, duration_, 30, 0});
		} else if (effect_ == "defensive_buff") {
			player.AddBarrier(player.getMaxHP() * powerPercent_ / 100, duration_);
			player.ApplyStatus({StatusType::DamageReduction, duration_, (std::max)(20, powerPercent_ / 100), 0});
		} else if (effect_ == "damage_ignore") {
			enemy.takeDamage((std::max)(1, player.getAttack() * powerPercent_ / 100));
		} else if (effect_ == "damage_burn") {
			enemy.takeDamage(CombatMath::Mitigate((std::max)(1, player.getAttack() * powerPercent_ * player.GetShimmerDamageMultiplierPercent() / 10000), enemy.getDefense()));
				enemy.ApplyStatus({StatusType::Burn, duration_, 0, player.getAttack(), 50, player.getDefense()}, player.getAccuracy());
		} else if (effect_ == "damage_stun") {
			enemy.takeDamage(CombatMath::Mitigate((std::max)(1, player.getAttack() * powerPercent_ * player.GetShimmerDamageMultiplierPercent() / 10000), enemy.getDefense()));
			enemy.ApplyStatus({StatusType::Stun, duration_ > 0 ? duration_ : 1, 0, player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "damage_tridot") {
			enemy.takeDamage(CombatMath::Mitigate((std::max)(1, player.getAttack() * powerPercent_ * player.GetShimmerDamageMultiplierPercent() / 10000), enemy.getDefense()));
			for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison})
				enemy.ApplyStatus({type, duration_, 0, player.getAttack(), 40, player.getDefense()}, player.getAccuracy());
		} else if (effect_ == "damage_lifesteal") {
			const int damage = CombatMath::Mitigate((std::max)(1, player.getAttack() * powerPercent_ * player.GetShimmerDamageMultiplierPercent() / 10000), enemy.getDefense());
			enemy.takeDamage(damage);
			player.heal(damage * 25 / 100);
		} else if (effect_ == "damage_defdown") {
			enemy.takeDamage(CombatMath::Mitigate((std::max)(1, player.getAttack() * powerPercent_ * player.GetShimmerDamageMultiplierPercent() / 10000), enemy.getDefense()));
			enemy.ApplyStatus({StatusType::DefenseDown, duration_, (std::max)(20, powerPercent_ / 10), player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "stat_boost") {
			const int duration = duration_ > 0 ? duration_ : 2;
			player.ApplyStatus({StatusType::AttackUp, duration, 25});
			player.ApplyStatus({StatusType::DefenseUp, duration, 25});
			player.ApplyStatus({StatusType::SpeedUp, duration, 25});
			player.ApplyStatus({StatusType::CritUp, duration, 15});
			player.ApplyStatus({StatusType::CritDamageUp, duration, 25});
			player.ApplyStatus({StatusType::DamageReduction, duration, 15});
		} else if (effect_ == "revive") {
			player.AddBarrier(player.getMaxHP() * powerPercent_ / 100, 1);
		}
	}

};
}

ShimmerSkill::ShimmerSkill(const std::string& name, int equipCost, int cooldown, const std::string& description,
	const std::string& effect, int powerPercent, int duration)
	: name_(name), description_(description), effect_(effect), equipCost_(equipCost), cooldown_(cooldown),
	  powerPercent_(powerPercent), duration_(duration), remainingCooldown_(0) {
	if (effect_ == "barrier" || effect_ == "heal_hot" || effect_ == "cleanse_resist" || effect_ == "defensive_buff" || effect_ == "stat_boost" || effect_ == "revive")
		targetType_ = TargetType::Self;
	else if (effect_ == "all_damage" || effect_ == "all_burn") targetType_ = TargetType::AllEnemies;
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
ShimmerSkill::TargetType ShimmerSkill::getTargetType() const { return targetType_; }
int ShimmerSkill::getRemainingCooldown() const { return remainingCooldown_; }
bool ShimmerSkill::IsReady() const { return remainingCooldown_ == 0; }
void ShimmerSkill::TickCooldown() { if (remainingCooldown_ > 0) --remainingCooldown_; }
void ShimmerSkill::ResetCooldown() { remainingCooldown_ = cooldown_; }

std::unique_ptr<ShimmerSkill> ShimmerSkill::Create(const std::string& skillName) {
	if (skillName == "Phragmós Phōtós" || skillName == "Velum Radiāns" || skillName == "Requiem Mechanicum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Gain a 1500% max HP barrier for 3 turns.", "barrier", 1500, 3);
	if (skillName == "Ray of Mercy") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Restore 500% max HP and gain a 100% max HP restorative pulse for 2 turns.", "heal_hot", 500, 2);
	if (skillName == "Prism Step") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 4, "Gain a 500% max HP barrier and 20% damage reduction for 2 turns.", "defensive_buff", 500, 2);
	if (skillName == "Gleam Needle") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Deal 500% ATK as a precise Shimmer strike.", "damage_ignore", 500, 0);
	if (skillName == "Therapeía Anakláseōs" || skillName == "Lux Sanctorum") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 8, "Restore 3000% max HP and gain a 600% max HP HoT for 3 turns.", "heal_hot", 3000, 3);
	if (skillName == "Anástasis Phōtós") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 8, "Gain a one-turn emergency barrier equal to 3500% max HP.", "revive", 3500, 1);
	if (skillName == "Phthorá Phōtós") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 7, "Deal 2200% ATK and reduce enemy DEF by 220% for 4 turns.", "damage_defdown", 2200, 4);
	if (skillName == "Phōteinḕ Kátharsis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 5, "Cleanse debuffs and gain 30% damage resistance for 3 turns.", "cleanse_resist", 0, 3);
	if (skillName == "Phōteinòs Thṓrax") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 1000% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 1000, 3);
	if (skillName == "Refractio Ultima") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Deal 3200% ATK as defense-ignoring damage.", "damage_ignore", 3200, 0);
	if (skillName == "Lumen Spiculum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1500% ATK and apply Burn for 3 turns.", "damage_burn", 1500, 3);
	if (skillName == "Lux Vortex") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal 2100% ATK and apply Burn, Bleed, and Poison.", "damage_tridot", 2100, 3);
	if (skillName == "Siphon Lux") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 6, "Deal 1800% ATK and restore 25% of damage dealt.", "damage_lifesteal", 1800, 0);
	if (skillName == "Lux Aegis") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Gain a 3000% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 3000, 3);
	if (skillName == "Refraction Cataclysm") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Deal 3600% ATK as defense-ignoring damage.", "damage_ignore", 3600, 0);
	if (skillName == "Lux Ascension" || skillName == "Onus Coruscum" || skillName == "Resonantia Chori") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Gain major ATK, DEF, SPD, critical, and damage-resistance buffs for 3 turns.", "stat_boost", 0, 3);
	if (skillName == "Pulsus Luminis") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal 900% ATK as a focused Shimmer strike.", "damage_ignore", 900, 0);
	if (skillName == "Gradus Coruscus" || skillName == "Mutatio Horologii") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain 40% combat stats and accelerate the next action for 2 turns.", "stat_boost", 800, 2);
	if (skillName == "Lucerna Echo" || skillName == "Catena Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1300% ATK and attempt to stun the enemy for 2 turns.", "damage_stun", 1300, 2);
	if (skillName == "Fulgur Aurorae") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal 2000% ATK as a heavy defense-ignoring strike.", "damage_ignore", 2000, 0);
	if (skillName == "Flos Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 1500% ATK and apply Burn for 3 turns.", "damage_burn", 1500, 3);
	if (skillName == "Scutum Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 1000% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 1000, 3);
	if (skillName == "Nota Luminosa" || skillName == "Revelatio Occulta") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal 900% ATK and reduce enemy DEF by 90% for 3 turns.", "damage_defdown", 900, 3);
	return nullptr;
}
