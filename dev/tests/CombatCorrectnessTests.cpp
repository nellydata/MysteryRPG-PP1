#include "Enemy.h"
#include "Player.h"
#include "CombatManager.h"
#include "EnemyCatalog.h"
#include "ContentCatalog.h"
#include "ProgressionManager.h"
#include "EncounterManager.h"
#include "Area.h"
#include "ShimmerSkill.h"
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

void EquipFullSet(Player& player, const std::string& setName) {
	const auto catalog = BuildEquippableCatalog();
	for (const auto& definition : catalog) {
		if (definition.setName == setName) {
			player.AddItem(std::make_unique<Item>(definition));
			player.Equip(*player.GetInventory().GetItems().back().item);
		}
	}
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
		player.AddBarrier(100);
		player.AddBarrier(50, 1);
		player.takeDamage(25);
		assert(player.GetBarrier() == 125);
		player.ProcessStatuses();
		assert(player.GetBarrier() == 100);
		assert(player.getHP() == player.getMaxHP());
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
		Player player("Test", 100, 10);
		player.ApplyStatus({StatusType::Bleed, 1, 0, 20});
		std::vector<std::pair<StatusType, int>> damageEvents;
		int healing = 0;
		player.ProcessStatuses(damageEvents, healing);
		assert(player.getHP() == 94);
		assert(damageEvents.size() == 1);
		assert(damageEvents.front().second == 6);
	}

	{
		Player player("Test", 100, 10);
		player.ApplyStatus({StatusType::AttackUp, 1, 50});
		std::vector<std::pair<StatusType, int>> damageEvents;
		int healing = 0;
		player.ProcessStatuses(damageEvents, healing);
		assert(player.getHP() == player.getMaxHP());
		assert(damageEvents.empty());
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
		enemy.ApplyStatus({StatusType::Burn, 1, 0, 10});
		std::vector<std::pair<StatusType, int>> damageEvents;
		int healing = 0;
		enemy.ProcessStatuses(damageEvents, healing);
		assert(enemy.getHP() == before - 6);
		assert(damageEvents.size() == 1);
		assert(damageEvents.front().second == 6);
		assert(!enemy.HasStatus(StatusType::Burn));
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		enemy.ApplyStatus({StatusType::AttackUp, 1, 50});
		const int before = enemy.getHP();
		std::vector<std::pair<StatusType, int>> damageEvents;
		int healing = 0;
		enemy.ProcessStatuses(damageEvents, healing);
		assert(enemy.getHP() == before);
		assert(damageEvents.empty());
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		enemy.ApplyStatus({StatusType::AttackUp, 2, 25});
		enemy.ApplyStatus({StatusType::Barrier, 2, 20});
		enemy.ApplyStatus({StatusType::Burn, 2, 0, 10});
		enemy.ApplyStatus({StatusType::HealResistance, 2, 25});
		assert(enemy.CleanseNegativeStatuses() == 2);
		assert(enemy.HasStatus(StatusType::AttackUp));
		assert(enemy.HasStatus(StatusType::Barrier));
		assert(!enemy.HasStatus(StatusType::Burn));
		assert(!enemy.HasStatus(StatusType::HealResistance));
	}

	{
		EnemyDefinition bossDefinition = TestEnemyDefinition();
		bossDefinition.tier = EnemyTier::Boss;
		Enemy boss(bossDefinition, 1);
		const int baseAttack = boss.getAtk();
		boss.takeDamage(boss.getMaxHP() * 71 / 100);
		assert(boss.UpdatePhase());
		assert(boss.GetPhase() == 2);
		assert(boss.getAtk() == baseAttack * 120 / 100);
		boss.takeDamage(boss.getHP() - boss.getMaxHP() * 29 / 100);
		assert(boss.UpdatePhase());
		assert(boss.GetPhase() == 3);
		assert(boss.getAtk() == baseAttack * 140 / 100);
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		Player player("Test", 100, 10);
		CombatManager combat;
		combat.BeginEncounter(enemy);
		combat.PrepareTurn(player, enemy);
		assert(combat.IsPlayerTurn());
		assert(combat.resolve(CombatAction::Attack, player, enemy));
		enemy.ApplyStatus({StatusType::Stun, 1, 0});
		const int playerHealth = player.getHP();
		combat.enemyTurn(player, enemy);
		assert(player.getHP() == playerHealth);
		bool sawStunLoss = false;
		for (const auto& event : combat.GetEvents())
			if (event.message.find("loses its turn to Stun") != std::string::npos) sawStunLoss = true;
		assert(sawStunLoss);
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
		Player player("Set Test", 100, 10);
		EquipFullSet(player, "Automaton Overlord");
		assert(player.HasActiveSet("Automaton Overlord"));
		assert(player.getAccuracy() == 100);
		assert(player.getCritDamage() >= 200);
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

	{
		const auto loot = GenerateInvestigationLoot(ConsumableProfile::SupplyCache, 1, "Storage Room", 70);
		assert(loot.size() == 1);
		assert(loot.front().category == ItemCategory::Equippable);
	}

	{
		Area room("Storage Room");
		room.AddInteractable("Supply shelf");
		room.SetSpawnCondition(EnemySpawnCondition::None);
		room.SetClues(false);
		room.SetItems(false);
		Player player("Test", 100, 10);
		ProgressionManager progression;
		EncounterManager encounters;
		const InvestigationResult first = encounters.ResolveInvestigation(room, player, progression);
		const InvestigationResult second = encounters.ResolveInvestigation(room, player, progression);
		assert(first.primaryResolved);
		assert(second.primaryResolved);
		assert(room.GetSearchCount() == 2);
		assert(first.type != InvestigationEventType::FountainDiscovery);
	}

	{
		Area fountain("Fountain");
		fountain.AddInteractable("The restorative cathedral fountain");
		fountain.SetHealingRoom(true);
		Player player("Test", 100, 10);
		ProgressionManager progression;
		EncounterManager encounters;
		const InvestigationResult result = encounters.ResolveInvestigation(fountain, player, progression);
		assert(result.type == InvestigationEventType::FountainDiscovery);
		assert(result.spawnedEnemy != nullptr);
		assert(!fountain.IsHealingUnlocked());
	}

	{
		Area room("Chapel");
		room.AddInteractable("Altar");
		room.SetSpawnCondition(EnemySpawnCondition::None);
		room.SetClues(false);
		room.SetItems(false);
		Player player("Test", 100, 10);
		ProgressionManager progression;
		EncounterManager encounters;
		bool sawHazard = false;
		for (int attempt = 0; attempt < 100 && player.isAlive(); ++attempt) {
			const int before = player.getHP();
			const InvestigationResult result = encounters.ResolveInvestigation(room, player, progression);
			assert(result.primaryResolved);
			if (result.type == InvestigationEventType::Hazard) {
				sawHazard = true;
				assert(before - player.getHP() <= player.getMaxHP() * 12 / 100);
				assert(player.getHP() >= 1);
			}
		}
		assert(sawHazard);
	}

	{
		auto low = ShimmerSkill::Create("Gleam Needle");
		auto mid = ShimmerSkill::Create("Lumen Spiculum");
		auto high = ShimmerSkill::Create("Refractio Ultima");
		assert(low != nullptr && mid != nullptr && high != nullptr);
		assert(low->getCost() == 1 && low->getPowerPercent() >= 500);
		assert(mid->getCost() == 3 && mid->getPowerPercent() >= 1200);
		assert(high->getCost() == 5 && high->getPowerPercent() >= 2600);
		Enemy enemy(TestEnemyDefinition(), 1);
		Player player("Test", 100, 100);
		const int before = enemy.getHP();
		high->apply(player, enemy);
		assert(before - enemy.getHP() >= 1);
	}

	{
		Enemy enemy(TestEnemyDefinition(), 1);
		Player player("Test", 100, 100);
		player.takeDamage(50);
		auto siphon = ShimmerSkill::Create("Siphon Lux");
		assert(siphon != nullptr);
		siphon->apply(player, enemy);
		assert(player.getHP() == 58);
	}

	return 0;
}
