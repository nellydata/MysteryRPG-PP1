#pragma once

#include <string>
#include "GameplayTypes.h"

class Item {
public:
    Item(const std::string& name = "", const std::string& description = "");
    explicit Item(const ItemDefinition& definition);
    ~Item();

    const std::string& getName() const;
    const std::string& getDescription() const;
    ItemCategory getCategory() const;
    Rarity getRarity() const;
    EquipSlot getSlot() const;
    const Stats& getStats() const;
    const std::string& getEffect() const;
    const std::string& getUniqueEffect() const;
    const std::string& getTheme() const;
    const std::string& getMainStat() const;
    int getMainStatPercent() const;
    const std::vector<ItemStat>& getAllowedSubstats() const;
    const std::vector<ItemStat>& getForbiddenStats() const;
    const std::string& getSetName() const;
    const std::string& getPassive() const;
    const std::string& getSetPassive() const;
    bool isEquippable() const;

private:
    std::string name_;
    std::string description_;
    ItemCategory category_;
    Rarity rarity_;
    EquipSlot slot_;
    Stats stats_;
    std::string effect_;
    std::string uniqueEffect_;
    std::string theme_;
    std::string mainStat_;
    int mainStatPercent_;
    std::vector<ItemStat> allowedSubstats_;
    std::vector<ItemStat> forbiddenStats_;
    std::string setName_;
    std::string passive_;
    std::string setPassive_;
};
