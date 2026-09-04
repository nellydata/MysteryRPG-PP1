#pragma once

#include <string>
#include <vector>
#include <memory>

class Enemy;

class Area {
public:
    Area(const std::string& name = "");
    ~Area();

    void addEnemy(std::unique_ptr<Enemy> enemy);
    bool hasEnemies() const;
    const std::string& getName() const;

    Enemy* getFirstAliveEnemy();
    void removeDeadEnemies();

private:
    std::string name_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
};
