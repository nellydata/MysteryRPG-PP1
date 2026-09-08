#pragma once

#include "Enemy.h"
#include <memory>
#include <string>
#include <vector>

enum class EnemySpawnCondition {
    None,
    Low,
    Medium,
    High
};

class Area {
public:
    Area(const std::string& name = "");
    ~Area();
    Area(const Area&) = delete;
    Area& operator=(const Area&) = delete;
    Area(Area&&) noexcept = default;
    Area& operator=(Area&&) noexcept = default;

    void addEnemy(std::unique_ptr<Enemy> enemy);
    bool hasEnemies() const;
    const std::string& getName() const;

    Enemy* getFirstAliveEnemy();
    const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const;
    void removeDeadEnemies();
    void ClearEnemies();

    void AddInteractable(const std::string& interactable);
    void SetNPC(bool hasNPC);
    void SetClues(bool hasClues);
    void SetItems(bool hasItems);
    void SetSpawnCondition(EnemySpawnCondition condition);

    const std::vector<std::string>& GetInteractables() const;
    bool HasNPC() const;
    bool HasClues() const;
    bool HasItems() const;
    bool HasAllClues() const;
    bool HasAllItems() const;
    EnemySpawnCondition GetSpawnCondition() const;
    bool ConsumeClue();
    bool ConsumeItem();
    bool IsUnlocked() const;
    void SetUnlocked(bool unlocked);
    int GetSearchCount() const;
    void RecordSearch();
    bool RevealNextInteractable();
    const std::vector<std::string>& GetVisibleInteractables() const;
    bool IsFullySearched() const;
    bool AreAllRequiredInteractablesRevealed() const;
    bool HasDiscoveredNPC() const;
    void DiscoverNPC();
    bool HasBossBeenDefeated() const;
    void SetBossDefeated(bool defeated);
    bool IsHealingUnlocked() const;
    bool IsHealingRoom() const;
    void SetHealingUnlocked(bool unlocked);
    void SetHealingRoom(bool healingRoom);
    bool HasBossBeenEncountered() const;
    void MarkBossEncountered();
    void SetCleared(bool cleared);

private:
    struct ResourcePool {
        bool available = false;
        int remaining = 0;

        void Set(bool hasResource, int count) {
            available = hasResource;
            remaining = hasResource ? count : 0;
        }

        bool Has() const { return available; }
        bool HasAll() const { return remaining == 0; }

        bool Consume() {
            if (!available || remaining <= 0) return false;
            --remaining;
            available = remaining > 0;
            return true;
        }
    };

    std::string name_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::string> interactables_;
    bool hasNPC_;
    EnemySpawnCondition spawnCondition_;
    bool unlocked_;
    int searchCount_;
    std::vector<std::string> visibleInteractables_;
    bool discoveredNPC_;
    bool bossDefeated_;
    bool bossEncountered_;
    bool healingUnlocked_;
    bool healingRoom_;
    ResourcePool cluePool_;
    ResourcePool itemPool_;
};
