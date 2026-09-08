#include "ContentCatalog.h"
#include "Item.h"
#include "Player.h"
#include <cassert>
#include <memory>
#include <set>

int main() {
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
	Item bootsItem(boots);
	Player player("Equipment test", 100, 10);
	player.Equip(bootsItem);
	assert(player.getSpeed() == 150);

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
