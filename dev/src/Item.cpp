#include "Item.h"

Item::Item(const std::string& name, const std::string& description)
	: name_(name), description_(description), category_(ItemCategory::Consumable),
	  rarity_(Rarity::Common), slot_(EquipSlot::None) {}

Item::Item(const ItemDefinition& definition)
	: name_(definition.name), description_(definition.description),
	  category_(definition.category), rarity_(definition.rarity),
	  slot_(definition.slot), stats_(definition.stats), effect_(definition.effect),
	  uniqueEffect_(definition.uniqueEffect) {}

Item::~Item() = default;

const std::string& Item::getName() const { return name_; }
const std::string& Item::getDescription() const { return description_; }
ItemCategory Item::getCategory() const { return category_; }
Rarity Item::getRarity() const { return rarity_; }
EquipSlot Item::getSlot() const { return slot_; }
const Stats& Item::getStats() const { return stats_; }
const std::string& Item::getEffect() const { return effect_; }
const std::string& Item::getUniqueEffect() const { return uniqueEffect_; }
bool Item::isEquippable() const { return category_ == ItemCategory::Equippable; }
