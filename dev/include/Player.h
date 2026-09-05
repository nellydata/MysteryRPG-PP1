#pragma once

#include <string>
#include <vector>
#include "Clue.h"
#include "GameplayTypes.h"
#include "Item.h"
#include "Inventory.h"

class Player {
public:
    Player(const std::string& name = "Adelaide", int maxHp = 20, int maxSp = 10);
    ~Player();

    void takeDamage(int amount);
    void heal(int amount);
    bool isAlive() const;

    const std::string& getName() const;
    void SetName(const std::string& name);
    int getHP() const;
    int getMaxHP() const;
    int getAttack() const;
    int getDefense() const;
    int getSP() const;
    int getSpeed() const;
    int getAccuracy() const;
    int getResistance() const;
    int getCritRate() const;
    int getCritDamage() const;
    int getEvasion() const;
    int getHealingPotency() const;
    int getBarrierPotency() const;
    int getDotPotency() const;
    int getLifesteal() const;
    int getDebuffDurationReduction() const;
    int getDisplayedAccuracy() const;
    int getDisplayedResistance() const;
    int getDisplayedCritRate() const;
    int getDisplayedEvasion() const;
    const Stats& getStats() const;
    std::size_t GetInventorySize() const;

    void increaseMaxHP(int amount);
    void increaseAttack(int amount);
    void increaseDefense(int amount);
    void SetLevelStats(int level);
    void spendSP(int amount);
    void gainSP(int amount);
    void AddItem(std::unique_ptr<Item> item);
    void AddClue(const Clue& clue);
    void PrintInventory() const;
    bool Equip(Item& item);
    bool ToggleEquip(Item& item);
    bool UseConsumable(const std::string& itemName, class Enemy* target = nullptr);
    bool HasStatus(StatusType type) const;
    void ApplyStatus(const StatusEffect& status);
    void ProcessStatuses();
    void ClearStatuses();
    void ModifyStats(const Stats& delta);
    const Inventory& GetInventory() const;
    Inventory& GetInventory();
    Item* FindItem(const std::string& itemName);
    const std::vector<StatusEffect>& GetStatuses() const;
    const Item* GetEquipped(EquipSlot slot) const;
    std::vector<std::string> GetActiveSets() const;

private:
    std::string name_;
    int hp_;
    int maxHp_;
    Stats baseStats_;
    Stats totalStats_;
    int sp_;
    int maxSp_;
    Inventory inventory_;
    std::vector<StatusEffect> statuses_;
    Item* equipped_[5];

    void RecalculateStats();
};
