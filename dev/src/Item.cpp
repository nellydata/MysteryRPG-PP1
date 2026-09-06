#include "Item.h"
#include <vector>

Item::Item(const std::string& name, const std::string& description)
	: name_(name), description_(description), category_(ItemCategory::Consumable),
	  rarity_(Rarity::Common), slot_(EquipSlot::None), mainStatValue_(0), mainStatPercent_(0) {}

Item::Item(const ItemDefinition& definition)
	: name_(definition.name), description_(definition.description),
	  category_(definition.category), rarity_(definition.rarity),
	  slot_(definition.slot), stats_(definition.stats), effect_(definition.effect),
	  uniqueEffect_(definition.uniqueEffect), theme_(definition.theme),
			mainStat_(definition.mainStat), mainStatValue_(definition.mainStatValue), mainStatPercent_(definition.mainStatPercent), allowedSubstats_(definition.allowedSubstats), substats_(definition.substats),
	  forbiddenStats_(definition.forbiddenStats), setName_(definition.setName),
	  passive_(definition.passive), setPassive_(definition.setPassive) {}

Item::~Item() = default;

const std::string& Item::getName() const { return name_; }
const std::string& Item::getDescription() const { return description_; }
ItemCategory Item::getCategory() const { return category_; }
Rarity Item::getRarity() const { return rarity_; }
EquipSlot Item::getSlot() const { return slot_; }
const Stats& Item::getStats() const { return stats_; }
const std::string& Item::getEffect() const { return effect_; }
const std::string& Item::getUniqueEffect() const { return uniqueEffect_; }
const std::string& Item::getTheme() const { return theme_; }
const std::string& Item::getMainStat() const { return mainStat_; }
int Item::getMainStatPercent() const { return mainStatPercent_; }
int Item::getMainStatValue() const { return mainStatValue_; }
const std::vector<std::pair<ItemStat, int>>& Item::getSubstats() const { return substats_; }
const std::vector<ItemStat>& Item::getAllowedSubstats() const { return allowedSubstats_; }
const std::vector<ItemStat>& Item::getForbiddenStats() const { return forbiddenStats_; }
const std::string& Item::getSetName() const { return setName_; }
const std::string& Item::getPassive() const { return passive_; }
const std::string& Item::getSetPassive() const { return setPassive_; }
bool Item::isEquippable() const { return category_ == ItemCategory::Equippable; }
