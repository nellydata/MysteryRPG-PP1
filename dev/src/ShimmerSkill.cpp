#include "ShimmerSkill.h"
#include "Enemy.h"
#include "Player.h"
#include <algorithm>

namespace {
class DefinedShimmerSkill final : public ShimmerSkill {
public:
	DefinedShimmerSkill(const std::string& name, int cooldown, const std::string& effect)
		: ShimmerSkill(name, cooldown), effect_(effect) {}

	void apply(Player& player, Enemy& enemy) override {
		if (effect_ == "damage_reveal") enemy.takeDamage(player.getAttack());
		else if (effect_ == "damage_burn") {
			enemy.takeDamage(player.getAttack());
			enemy.ApplyStatus({StatusType::Burn, 3, 0, player.getAttack()});
		} else if (effect_ == "damage_mark") {
			enemy.ApplyStatus({StatusType::VerdigrisMark, 3, player.getAttack() / 2, player.getAttack()});
		} else if (effect_ == "heal_cleanse") {
			player.heal(player.getMaxHP() * 30 / 100);
			player.ClearStatuses();
		} else if (effect_ == "barrier") {
			player.ApplyStatus({StatusType::Barrier, 1, 1, 0});
		} else if (effect_ == "damage_stun") {
			enemy.takeDamage(player.getAttack());
			enemy.ApplyStatus({StatusType::Stun, 1, 0, player.getAttack()});
		} else if (effect_ == "damage") {
			enemy.takeDamage(player.getAttack() * 2);
		} else if (effect_ == "defense_break") {
			enemy.ApplyStatus({StatusType::VerdigrisMark, 3, 0, player.getAttack()});
		} else if (effect_ == "extra_turn") {
			player.ApplyStatus({StatusType::HealOverTime, 1, 0, 0});
		} else if (effect_ == "stat_boost") {
			player.ModifyStats({0, 0, 1, 0, 10, 10, 0, 0});
		}
	}

private:
	std::string effect_;
};
}

ShimmerSkill::ShimmerSkill(const std::string& name, int cost)
	: name_(name), cooldown_(cost), remainingCooldown_(0) {}

ShimmerSkill::~ShimmerSkill() = default;

const std::string& ShimmerSkill::getName() const { return name_; }
int ShimmerSkill::getCost() const { return cooldown_; }
int ShimmerSkill::getCooldown() const { return cooldown_; }
int ShimmerSkill::getRemainingCooldown() const { return remainingCooldown_; }
bool ShimmerSkill::IsReady() const { return remainingCooldown_ == 0; }
void ShimmerSkill::TickCooldown() { if (remainingCooldown_ > 0) --remainingCooldown_; }
void ShimmerSkill::ResetCooldown() { remainingCooldown_ = cooldown_; }

std::unique_ptr<ShimmerSkill> ShimmerSkill::Create(const std::string& skillName) {
	if (skillName == "Pulsus Luminis") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "damage_reveal");
	if (skillName == "Velum Radiāns") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "barrier");
	if (skillName == "Gradus Coruscus") return std::make_unique<DefinedShimmerSkill>(skillName, 2, "extra_turn");
	if (skillName == "Lucerna Echo") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "damage_stun");
	if (skillName == "Fulgur Aurorae") return std::make_unique<DefinedShimmerSkill>(skillName, 5, "damage");
	if (skillName == "Flos Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "damage_burn");
	if (skillName == "Resonantia Chori") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "stat_boost");
	if (skillName == "Requiem Mechanicum") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "barrier");
	if (skillName == "Scutum Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "barrier");
	if (skillName == "Nota Luminosa") return std::make_unique<DefinedShimmerSkill>(skillName, 2, "damage_mark");
	if (skillName == "Revelatio Occulta") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "damage_reveal");
	if (skillName == "Catena Aeruginis") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "damage_stun");
	if (skillName == "Onus Coruscum") return std::make_unique<DefinedShimmerSkill>(skillName, 5, "stat_boost");
	if (skillName == "Lux Sanctorum") return std::make_unique<DefinedShimmerSkill>(skillName, 4, "heal_cleanse");
	if (skillName == "Mutatio Horologii") return std::make_unique<DefinedShimmerSkill>(skillName, 3, "extra_turn");
	return nullptr;
}
