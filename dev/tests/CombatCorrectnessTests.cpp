#include "Enemy.h"
#include "Player.h"
#include "EnemyCatalog.h"
#include "ContentCatalog.h"
#include "ProgressionManager.h"
#include <cassert>

namespace {
EnemyDefinition TestEnemyDefinition() {
	EnemyDefinition definition;
	definition.name = "Test Enemy";
	definition.hpBase = 100;
	definition.atkBase = 20;
	definition.defBase = 10;
	definition.speed = 100;
	return definition;
}
}

int main() {
	{
		Player player("Test", 100, 10);
		player.AddBarrier(20);
		player.takeDamage(30);
		assert(player.getHP() == 90);
		assert(player.GetBarrier() == 0);
	}

	{
		Player player("Test", 100, 10);
		player.takeDamage(50);
		player.ApplyStatus({StatusType::HealingPotencyUp, 2, 50});
		player.heal(10);
		assert(player.getHP() == 65);
	}

	{
		Player player("Test", 100, 10);
		player.AddBarrier(20, 1);
		player.ProcessStatuses();
		assert(player.GetBarrier() == 0);
	}

	{
		Player player("Test", 100, 10);
		player.ApplyStatus({StatusType::Bleed, 2, 0, 10});
		player.ProcessStatuses();
		assert(player.getHP() < 100);
		const int afterFirstTick = player.getHP();
		player.ProcessStatuses();
		assert(player.getHP() < afterFirstTick);
		assert(!player.HasStatus(StatusType::Bleed));
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		enemy.ApplyStatus({StatusType::Barrier, 2, 20});
		const int before = enemy.getHP();
		enemy.takeDamage(30);
		assert(enemy.getHP() == before - 10);
		assert(!enemy.HasStatus(StatusType::Barrier));
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		const int before = enemy.getHP();
		enemy.ApplyStatus({StatusType::Burn, 1, 0, 100});
		enemy.ProcessStatuses();
		assert(enemy.getHP() < before);
		assert(!enemy.HasStatus(StatusType::Burn));
	}

	{
		Player player("Test", 100, 10);
		player.takeDamage(90);
		player.ApplyStatus({StatusType::DotUp, 2, 100});
		player.ApplyStatus({StatusType::Bleed, 2, 0, 1000});
		const int before = player.getHP();
		player.ProcessStatuses();
		assert(before - player.getHP() >= 6);
	}

	{
		const auto* flagellant = FindEnemyDefinition("Devotee Flagellant");
		assert(flagellant != nullptr);
		assert(flagellant->skills.front().effect == "def_down");
		assert(flagellant->skills.front().multiplier == 180);
		std::string error;
		assert(ValidateEnemyCatalogue(&error));
	}

	{
		const auto consumables = BuildConsumableCatalog();
		int rations = 0;
		int bandages = 0;
		for (const auto& item : consumables) {
			if (item.name == "cellar_rations") rations = 20;
			if (item.name == "blessed_bandages") bandages = 35;
		}
		assert(bandages > rations);
		assert(CalculateInvestigationXP(10, 3) > CalculateInvestigationXP(1, 1));
	}

	return 0;
}
