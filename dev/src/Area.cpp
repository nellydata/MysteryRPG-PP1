#include "Area.h"
#include "Enemy.h"
#include <algorithm>

Area::Area(const std::string& name)
    : name_(name), hasNPC_(false), hasClues_(false), hasItems_(false),
      spawnCondition_(EnemySpawnCondition::None) {}

Area::~Area() = default;

void Area::addEnemy(std::unique_ptr<Enemy> enemy) {
    if (enemy) enemies_.push_back(std::move(enemy));
}

bool Area::hasEnemies() const {
    for (const auto& enemy : enemies_) {
        if (enemy && enemy->isAlive()) return true;
    }
    return false;
}

const std::string& Area::getName() const { return name_; }

Enemy* Area::getFirstAliveEnemy() {
    for (auto& enemy : enemies_) {
        if (enemy && enemy->isAlive()) return enemy.get();
    }
    return nullptr;
}

void Area::removeDeadEnemies() {
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& enemy) {
            return !enemy || !enemy->isAlive();
        }), enemies_.end());
}

void Area::AddInteractable(const std::string& interactable) {
    if (!interactable.empty()) interactables_.push_back(interactable);
}

void Area::SetNPC(bool hasNPC) { hasNPC_ = hasNPC; }
void Area::SetClues(bool hasClues) { hasClues_ = hasClues; }
void Area::SetItems(bool hasItems) { hasItems_ = hasItems; }
void Area::SetSpawnCondition(EnemySpawnCondition condition) { spawnCondition_ = condition; }

const std::vector<std::string>& Area::GetInteractables() const { return interactables_; }
bool Area::HasNPC() const { return hasNPC_; }
bool Area::HasClues() const { return hasClues_; }
bool Area::HasItems() const { return hasItems_; }
EnemySpawnCondition Area::GetSpawnCondition() const { return spawnCondition_; }

bool Area::ConsumeClue() {
    if (!hasClues_) return false;
    hasClues_ = false;
    return true;
}

bool Area::ConsumeItem() {
    if (!hasItems_) return false;
    hasItems_ = false;
    return true;
}
