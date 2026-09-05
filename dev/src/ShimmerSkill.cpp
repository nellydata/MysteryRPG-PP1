#include "ShimmerSkill.h"
#include "Enemy.h"
#include "Player.h"
#include <algorithm>

namespace {
class DefinedShimmerSkill final : public ShimmerSkill {
public:
	DefinedShimmerSkill(const std::string& name, int cooldown, const std::string& description, const std::string& effect)
		: ShimmerSkill(name, cooldown, description, effect) {}

	void apply(Player& player, Enemy& enemy) override {
		if (effect_ == "damage_reveal") enemy.takeDamage(player.getAttack() * 250 / 100);
		else if (effect_ == "damage_burn") {
			enemy.takeDamage(player.getAttack() * 220 / 100);
			enemy.ApplyStatus({StatusType::Burn, 3, 0, player.getAttack(), player.getDotPotency()}, player.getAccuracy());
		} else if (effect_ == "damage_mark") {
			enemy.ApplyStatus({StatusType::VerdigrisMark, 3, player.getAttack() / 2, player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "heal_cleanse") {
			player.heal(player.getMaxHP() * 30 / 100);
			player.ClearStatuses();
		} else if (effect_ == "barrier") {
			player.ApplyStatus({StatusType::Barrier, 1, 1, 0});
		} else if (effect_ == "damage_stun") {
			enemy.takeDamage(player.getAttack() * 240 / 100);
			enemy.ApplyStatus({StatusType::Stun, 1, 0, player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "damage") {
			enemy.takeDamage(player.getAttack() * 3);
		} else if (effect_ == "defense_break") {
			enemy.ApplyStatus({StatusType::VerdigrisMark, 3, 0, player.getAttack()}, player.getAccuracy());
		} else if (effect_ == "extra_turn") {
			player.ApplyStatus({StatusType::HealOverTime, 1, 0, 0});
		} else if (effect_ == "stat_boost") {
			player.ModifyStats({0, 0, 50, 0, 50, 50, 0, 0});
		}
	}

};
}

ShimmerSkill::ShimmerSkill(const std::string& name, int cost, const std::string& description, const std::string& effect)
	: name_(name), description_(description), effect_(effect), cooldown_(cost), remainingCooldown_(0) {}

ShimmerSkill::~ShimmerSkill() = default;

const std::string& ShimmerSkill::getName() const { return name_; }
const std::string& ShimmerSkill::getDescription() const { return description_; }
const std::string& ShimmerSkill::getEffect() const { return effect_; }
int ShimmerSkill::getCost() const { return cooldown_; }
int ShimmerSkill::getCooldown() const { return cooldown_; }
int ShimmerSkill::getRemainingCooldown() const { return remainingCooldown_; }
bool ShimmerSkill::IsReady() const { return remainingCooldown_ == 0; }
void ShimmerSkill::TickCooldown() { if (remainingCooldown_ > 0) --remainingCooldown_; }
void ShimmerSkill::ResetCooldown() { remainingCooldown_ = cooldown_; }

std::unique_ptr<ShimmerSkill> ShimmerSkill::Create(const std::string& skillName) {
	if (skillName == "Pulsus Luminis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "A focused Shimmer strike that reveals the target.", "damage_reveal");
	if (skillName == "Velum Radiāns") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "Creates a barrier that protects the player for one turn.", "barrier");
	if (skillName == "Gradus Coruscus") return std::make_unique<DefinedShimmerSkill>(skillName, 2, "Accelerates the player with a restorative pulse.", "extra_turn");
	if (skillName == "Lucerna Echo") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "Strikes the enemy and attempts to stun it.", "damage_stun");
	if (skillName == "Fulgur Aurorae") return std::make_unique<DefinedShimmerSkill>(skillName, 5, "Deals a heavy Shimmer attack.", "damage");
	if (skillName == "Flos Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "Strikes the enemy and applies Burn.", "damage_burn");
	if (skillName == "Resonantia Chori") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "Raises the player's combat accuracy and resistance.", "stat_boost");
	if (skillName == "Requiem Mechanicum") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "Deploys a defensive barrier.", "barrier");
	if (skillName == "Scutum Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "Deploys a short defensive barrier.", "barrier");
	if (skillName == "Nota Luminosa") return std::make_unique<DefinedShimmerSkill>(skillName, 2, "Marks the enemy for increased vulnerability.", "damage_mark");
	if (skillName == "Revelatio Occulta") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "Deals damage and exposes hidden weaknesses.", "damage_reveal");
	if (skillName == "Catena Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "Damages the enemy and attempts to stun it.", "damage_stun");
	if (skillName == "Onus Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 5, "Strengthens the player's combat attributes.", "stat_boost");
	if (skillName == "Lux Sanctorum") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "Restores health and cleanses player statuses.", "heal_cleanse");
	if (skillName == "Mutatio Horologii") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "Accelerates the player's next combat action.", "extra_turn");
	return nullptr;
}
