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
};
