#include "ContentCatalog.h"
#include "Item.h"
#include "Player.h"
#include "EquipmentEconomyTests.h"
#include "TestDiagnostics.h"
#include <cassert>
#include <memory>
#include <set>

int main() {
	UseConsoleTestDiagnostics();
	RunEquipmentEconomyTests();
	const auto catalog = BuildEquippableCatalog();
	assert(!catalog.empty());

	std::set<unsigned long long> instanceIds;
	int equipmentCount = 0;
	for (const auto& definition : catalog) {
		if (definition.slot == EquipSlot::None) continue;
		const ItemDefinition firstRoll = RollLootInstance(definition, 101);
		const ItemDefinition secondRoll = RollLootInstance(definition, 202);
		Item first(firstRoll);
		Item second(secondRoll);
		assert(first.isEquippable());
		assert(second.isEquippable());
		assert(first.getInstanceId() != second.getInstanceId());
		instanceIds.insert(first.getInstanceId());
		instanceIds.insert(second.getInstanceId());
		++equipmentCount;

		if (definition.rarity == Rarity::Common) assert(first.getSubstats().empty());
		if (definition.rarity == Rarity::Epic) assert(first.getSubstats().size() == 3);
	}
	assert(equipmentCount > 0);
	assert(instanceIds.size() == static_cast<std::size_t>(equipmentCount * 2));

	ItemDefinition boots;
	boots.name = "canonical_boots";
	boots.category = ItemCategory::Equippable;
	boots.slot = EquipSlot::Boots;
	boots.mainStat = "spd";
	boots.mainStatValue = 50;
	Player player("Equipment test", 100, 10);
	player.AddItem(std::make_unique<Item>(boots));
	Item* bootsItem = player.GetInventory().GetItems().back().item.get();
	player.Equip(*bootsItem);
	assert(player.getSpeed() == 150);
	assert(!player.GetInventory().DiscardItem(bootsItem->getInstanceId()));
	player.ToggleEquip(*bootsItem);
	assert(player.GetInventory().DiscardItem(bootsItem->getInstanceId()));

	ItemDefinition speedPercent;
	speedPercent.name = "speed_percent_boots";
	speedPercent.category = ItemCategory::Equippable;
	speedPercent.slot = EquipSlot::Boots;
	speedPercent.mainStat = "spd";
	speedPercent.mainStatPercent = 25;
	Player percentagePlayer("Percentage test", 100, 10);
	percentagePlayer.AddItem(std::make_unique<Item>(speedPercent));
	Item* percentageItem = percentagePlayer.GetInventory().GetItems().back().item.get();
	percentagePlayer.Equip(*percentageItem);
	assert(percentagePlayer.getSpeed() == 125);

	LootContext context;
	context.playerLevel = 25;
	context.enemyTier = EnemyTier::Elite;
	const auto loot = GenerateLoot(context, 7331);
	for (const auto& item : loot) {
		assert(item.category == ItemCategory::Equippable || item.category == ItemCategory::Consumable);
		if (item.category == ItemCategory::Equippable) assert(item.slot != EquipSlot::None);
	}
	return 0;
}
