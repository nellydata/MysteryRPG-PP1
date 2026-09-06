#include "ShimmerSkill.h"
#include "Enemy.h"
#include "Player.h"
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
			player.ApplyStatus({StatusType::HealOverTime, duration_, player.getMaxHP() * 20 / 100, 0});
		} else if (effect_ == "cleanse_resist") {
			player.ClearStatuses();
			player.ApplyStatus({StatusType::DamageReduction, duration_, 30, 0});
		} else if (effect_ == "defensive_buff") {
			player.AddBarrier(player.getMaxHP() * powerPercent_ / 100, duration_);
			player.ApplyStatus({StatusType::DamageReduction, duration_, 20, 0});
		} else if (effect_ == "damage_ignore") {
			enemy.takeDamage(player.getAttack() * powerPercent_ / 100);
		} else if (effect_ == "damage_burn") {
			enemy.takeDamage(player.getAttack() * powerPercent_ / 100);
			enemy.ApplyStatus({StatusType::Burn, duration_, 0, player.getAttack(), 50}, player.getAccuracy());
		} else if (effect_ == "damage_tridot") {
			enemy.takeDamage(player.getAttack() * powerPercent_ / 100);
			for (StatusType type : {StatusType::Burn, StatusType::Bleed, StatusType::Poison})
				enemy.ApplyStatus({type, duration_, 0, player.getAttack(), 40}, player.getAccuracy());
		} else if (effect_ == "damage_lifesteal") {
			const int damage = player.getAttack() * powerPercent_ / 100;
			enemy.takeDamage(damage);
			player.heal(damage * 25 / 100);
		} else if (effect_ == "damage_defdown") {
			enemy.takeDamage(player.getAttack() * powerPercent_ / 100);
			enemy.ApplyStatus({StatusType::VerdigrisMark, duration_, 60, player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "stat_boost") {
			player.ModifyStats({0, 0, 50, 50, 0, 0, 20, 0});
		} else if (effect_ == "revive") {
			player.ApplyStatus({StatusType::HealOverTime, 99, player.getMaxHP() / 2, 0});
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
	if (skillName == "Phragmós Phōtós" || skillName == "Velum Radiāns" || skillName == "Requiem Mechanicum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Gain a barrier equal to 1500% of max HP for 3 turns.", "barrier", 1500, 3);
	if (skillName == "Ray of Mercy") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Heal 20% max HP and gain a brief restorative pulse.", "heal_hot", 20, 1);
	if (skillName == "Prism Step") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 4, "Gain a 10% damage reduction barrier for 1 turn.", "defensive_buff", 100, 1);
	if (skillName == "Gleam Needle") return std::make_unique<DefinedShimmerSkill>(skillName, 1, 3, "Deal 180% ATK as a precise Shimmer strike.", "damage_ignore", 180, 0);
	if (skillName == "Therapeía Anakláseōs" || skillName == "Lux Sanctorum") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 8, "Heal 70% max HP and gain a 20% max HP HoT for 3 turns.", "heal_hot", 70, 3);
	if (skillName == "Anástasis Phōtós") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 0, "Prepare a once-per-battle 50% max HP revival.", "revive", 50, 0);
	if (skillName == "Phthorá Phōtós") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 7, "Reduce enemy ATK and DEF by 60% for 4 turns.", "damage_defdown", 600, 4);
	if (skillName == "Phōteinḕ Kátharsis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 5, "Cleanse debuffs and gain 30% damage resistance for 3 turns.", "cleanse_resist", 0, 3);
	if (skillName == "Phōteinòs Thṓrax") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 500% max HP barrier and 20% damage reduction for 3 turns.", "defensive_buff", 500, 3);
	if (skillName == "Refractio Ultima") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Deal 1000% ATK as defense-ignoring damage.", "damage_ignore", 1000, 0);
	if (skillName == "Lumen Spiculum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 600% ATK and apply Burn for 3 turns.", "damage_burn", 600, 3);
	if (skillName == "Lux Vortex") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal 500% ATK and apply Burn, Bleed, and Poison.", "damage_tridot", 500, 3);
	if (skillName == "Siphon Lux") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 0, "Heal for 25% of damage dealt.", "damage_lifesteal", 100, 0);
	if (skillName == "Lux Aegis") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Gain a 2000% max HP barrier and 20% damage reduction.", "defensive_buff", 2000, 3);
	if (skillName == "Refraction Cataclysm") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 12, "Deal 1500% ATK as defense-ignoring damage.", "damage_ignore", 1500, 3);
	if (skillName == "Lux Ascension" || skillName == "Onus Coruscum" || skillName == "Resonantia Chori") return std::make_unique<DefinedShimmerSkill>(skillName, 5, 10, "Gain major ATK, DEF, SPD, and critical buffs for 3 turns.", "stat_boost", 0, 3);
	if (skillName == "Pulsus Luminis") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal a focused 250% ATK Shimmer strike.", "damage_ignore", 250, 0);
	if (skillName == "Gradus Coruscus" || skillName == "Mutatio Horologii") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Accelerate the next combat action.", "stat_boost", 0, 1);
	if (skillName == "Lucerna Echo" || skillName == "Catena Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 400% ATK and attempt to stun the enemy.", "damage_burn", 400, 2);
	if (skillName == "Fulgur Aurorae") return std::make_unique<DefinedShimmerSkill>(skillName, 4, 8, "Deal a heavy 700% ATK Shimmer strike.", "damage_ignore", 700, 0);
	if (skillName == "Flos Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, 6, "Deal 600% ATK and apply Burn.", "damage_burn", 600, 3);
	if (skillName == "Scutum Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 4, "Gain a 500% max HP barrier.", "defensive_buff", 500, 3);
	if (skillName == "Nota Luminosa" || skillName == "Revelatio Occulta") return std::make_unique<DefinedShimmerSkill>(skillName, 2, 5, "Deal 350% ATK and mark the enemy.", "damage_defdown", 350, 3);
	return nullptr;
}
