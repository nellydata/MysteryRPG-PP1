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
    void removeDeadEnemies();

    void AddInteractable(const std::string& interactable);
    void SetNPC(bool hasNPC);
    void SetClues(bool hasClues);
    void SetItems(bool hasItems);
    void SetSpawnCondition(EnemySpawnCondition condition);

    const std::vector<std::string>& GetInteractables() const;
    bool HasNPC() const;
    bool HasClues() const;
    bool HasItems() const;
    EnemySpawnCondition GetSpawnCondition() const;
    bool ConsumeClue();
    bool ConsumeItem();

private:
    std::string name_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::string> interactables_;
    bool hasNPC_;
    bool hasClues_;
    bool hasItems_;
    EnemySpawnCondition spawnCondition_;
};
