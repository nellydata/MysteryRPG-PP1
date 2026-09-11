#pragma once

#include <string>
#include <vector>
#include "Clue.h"
#include "GameplayTypes.h"
#include "CombatResolution.h"
#include "Item.h"
#include "Inventory.h"
#include "StatusRules.h"

class CombatManager;
class Area;

class Player {
public:
    Player(const std::string& name = "Adelaide", int maxHp = 100, int maxSp = 10);
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void takeDamage(int amount);
    const DamageResolution& ApplyDirectDamage(int rawDamage, bool ignoreDefense = false);
    const HealResolution& GetLastHealResult() const { return lastHealResult_; }
    std::uint64_t GetCombatantId() const { return combatantId_; }
    void SetStatusSeed(unsigned seed) { statusRandom_.Seed(seed); }
    std::uint64_t GetStatusRandomDrawCount() const { return statusRandom_.DrawCount(); }
    const DamageResolution& GetLastDamageResult() const;
    const std::vector<StatusDamageResolution>& GetStatusDamageResults() const;
    const std::vector<StatusApplicationResolution>& GetStatusApplications() const;
    void heal(int amount);
    void RestoreHealth(int amount);
    int GetBarrier() const;
    int AddBarrier(int amount, int duration = 0);
    int AbsorbDamage(int amount);
    int ApplyDamageOverTime(int amount);
    void ReduceBarrier(int amount);
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
    Stats GetEffectiveStats() const;
    Stats PreviewEquipment(const Item& item) const;
    Stats PreviewUnequip(EquipSlot slot) const;
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
    bool UseConsumable(const std::string& itemName, class Enemy* target = nullptr,
                       CombatManager* combat = nullptr, Area* area = nullptr);
    bool UseConsumable(unsigned long long instanceId, class Enemy* target = nullptr,
                       CombatManager* combat = nullptr, Area* area = nullptr);
    bool HasStatus(StatusType type) const;
    bool ApplyStatus(const StatusEffect& status, int accuracy = 100);
    void ProcessStatuses();
    void BeginStatusTurn(std::vector<std::pair<StatusType, int>>& damageEvents = EmptyDamageEvents(), int& healing = EmptyHealing());
    void EndStatusTurn();
    void ProcessStatuses(std::vector<std::pair<StatusType, int>>& damageEvents, int& healing);
    void ClearStatuses();
    int CleanseNegativeStatuses();
    void ModifyStats(const Stats& delta);
    const Inventory& GetInventory() const;
    Inventory& GetInventory();
    Item* FindItem(const std::string& itemName);
    const std::vector<StatusEffect>& GetStatuses() const;
    const Item* GetEquipped(EquipSlot slot) const;
    int GetSetPieceCount(const std::string& setName) const;
    std::vector<std::string> GetActiveSets() const;
    bool HasActiveSet(const std::string& setName) const;
    int GetShimmerDamageMultiplierPercent() const;
    bool HasBarrierAbsorptionSet() const;
    void StartBattleEffects();

private:
    DamageResolution lastDamageResult_;
    HealResolution lastHealResult_;
    const std::uint64_t combatantId_ = StatusRules::NextIdentity();
    RandomStream statusRandom_;
    std::vector<std::uint64_t> statusTurnSnapshot_;
    bool statusTurnOpen_ = false;
    std::vector<StatusDamageResolution> statusDamageResults_;
    std::vector<StatusApplicationResolution> statusApplications_;
    std::string name_;
    int hp_;
    int maxHp_;
    Stats baseStats_;
    Stats totalStats_;
    int sp_;
    int maxSp_;
    int barrier_ = 0;
    Inventory inventory_;
    std::vector<StatusEffect> statuses_;
    Item* equipped_[5];

    void RecalculateStats();
    Stats CalculateEquipmentStats(const Item* const equipment[5]) const;
    Stats EffectiveStatsFor(const Stats& stats) const;
    int SumStatusBonus(StatusType type) const;
    static std::vector<std::pair<StatusType, int>>& EmptyDamageEvents();
    static int& EmptyHealing();
};
