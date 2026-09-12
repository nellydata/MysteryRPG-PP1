#include "Item.h"
#include "EquipmentStats.h"
#include <vector>

unsigned long long Item::nextInstanceId_ = 0;

Item::Item(const std::string& name, const std::string& description)
	: name_(name), description_(description), category_(ItemCategory::Consumable),
	  rarity_(Rarity::Common), slot_(EquipSlot::None), mainStatValue_(0), mainStatPercent_(0), instanceId_(++nextInstanceId_) {}

Item::Item(const ItemDefinition& definition)
	: name_(definition.name), description_(definition.description),
	  category_(definition.category), rarity_(definition.rarity),
	  slot_(definition.slot), stats_(definition.stats), effect_(definition.effect),
	  uniqueEffect_(definition.uniqueEffect), theme_(definition.theme),
	  mainStat_(definition.mainStat), mainStatValue_(definition.mainStatValue), mainStatPercent_(definition.mainStatPercent), allowedSubstats_(definition.allowedSubstats), substats_(definition.substats),
	  forbiddenStats_(definition.forbiddenStats), setName_(definition.setName),
	  passive_(definition.passive), setPassive_(definition.setPassive), instanceId_(++nextInstanceId_) {}

Item::~Item() = default;

const std::string& Item::getName() const { return name_; }
const std::string& Item::getDescription() const { return description_; }
ItemCategory Item::getCategory() const { return category_; }
Rarity Item::getRarity() const { return rarity_; }
EquipSlot Item::getSlot() const { return slot_; }
const Stats& Item::getStats() const { return stats_; }
Stats Item::GetStatBonuses() const {
    Stats result = stats_;
    const auto main = EquipmentStats::ForMain(mainStat_);
    if (main.flat) {
        result.*main.flat = EquipmentStats::SaturatingAdd(result.*main.flat, mainStatValue_);
        auto destination = main.percent ? main.percent : main.flat;
        result.*destination = EquipmentStats::SaturatingAdd(result.*destination, mainStatPercent_);
    }
    for (const auto& substat : substats_) {
        const auto rule = EquipmentStats::ForStat(substat.first);
        auto destination = rule.corePercentRoll ? rule.percent : rule.flat;
        if (destination) result.*destination = EquipmentStats::SaturatingAdd(result.*destination, substat.second);
    }
    return result;
}

std::vector<std::string> Item::GetStatLines() const {
    std::vector<std::string> lines;
    const auto append = [&](const std::string& source, const char* label, int value, const char* unit) {
        if (value != 0) lines.push_back(source + " " + label + (value >= 0 ? " +" : " ") + std::to_string(value) + unit);
    };
    const auto main = EquipmentStats::ForMain(mainStat_);
    if (main.flat) {
        append("Main:", main.label, mainStatValue_, main.percent ? " flat" : " pp");
        append("Main:", main.label, mainStatPercent_, main.percent ? "%" : " pp");
    }
    for (const auto& substat : substats_) {
        const auto rule = EquipmentStats::ForStat(substat.first);
        append("Sub:", rule.label, substat.second,
            rule.corePercentRoll ? "%" : substat.first == ItemStat::SPD ? " flat" : " pp");
    }
    static const char* labels[] = {"HP", "DEF", "SPD", "ATK", "ACC", "RES", "CritRate", "CritDamage", "EVA",
        "HealingPotency", "HealingReceived", "BarrierPotency", "DOTPotency", "Lifesteal", "DebuffDurationReduction",
        "HP", "DEF", "SPD", "ATK", "ACC", "RES", "CritRate", "CritDamage", "DirectDamage", "DamageTaken"};
    const auto& fields = EquipmentStats::Fields();
    for (std::size_t index = 0; index < fields.size(); ++index)
        append("Fixed:", labels[index], stats_.*fields[index], index < 4 ? " flat" :
            ((index >= 15 && index <= 18) || index >= 23) ? "%" : " pp");
    return lines;
}

int Item::GetStatContribution(ItemStat stat, const Stats& referenceStats) const {
    const auto rule = EquipmentStats::ForStat(stat);
    if (!rule.flat) return 0;
    const Stats bonus = GetStatBonuses();
    if (rule.percent) return EquipmentStats::ApplyPercent(
        EquipmentStats::SaturatingAdd(referenceStats.*rule.flat, bonus.*rule.flat), bonus.*rule.percent) - referenceStats.*rule.flat;
    return EquipmentStats::Finalize(bonus).*rule.flat;
}
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
unsigned long long Item::getInstanceId() const { return instanceId_; }
