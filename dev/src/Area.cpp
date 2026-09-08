#include "Area.h"
#include "Enemy.h"
#include <algorithm>

Area::Area(const std::string& name)
    : name_(name), hasNPC_(false),
      spawnCondition_(EnemySpawnCondition::None), unlocked_(false), searchCount_(0),
      discoveredNPC_(false), bossDefeated_(false), bossEncountered_(false), healingUnlocked_(false), healingRoom_(false) {}

Area::~Area() = default;

void Area::addEnemy(std::unique_ptr<Enemy> enemy) {
    if (enemy) enemies_.push_back(std::move(enemy));
}

bool Area::AreAllRequiredInteractablesRevealed() const {
    return visibleInteractables_.size() >= interactables_.size();
}

const std::vector<std::unique_ptr<Enemy>>& Area::GetEnemies() const { return enemies_; }

bool Area::IsUnlocked() const { return unlocked_; }
void Area::SetUnlocked(bool unlocked) { unlocked_ = unlocked; }
int Area::GetSearchCount() const { return searchCount_; }
void Area::RecordSearch() { ++searchCount_; }

bool Area::RevealNextInteractable() {
    if (visibleInteractables_.size() >= interactables_.size()) return false;
    visibleInteractables_.push_back(interactables_[visibleInteractables_.size()]);
    return true;
}

const std::vector<std::string>& Area::GetVisibleInteractables() const {
    return visibleInteractables_;
}

bool Area::IsFullySearched() const {
    return visibleInteractables_.size() >= interactables_.size()
        && (!hasNPC_ || discoveredNPC_);
}

bool Area::HasDiscoveredNPC() const { return discoveredNPC_; }
void Area::DiscoverNPC() { if (hasNPC_) discoveredNPC_ = true; }
bool Area::HasBossBeenDefeated() const { return bossDefeated_; }
void Area::SetBossDefeated(bool defeated) { bossDefeated_ = defeated; }
bool Area::IsHealingUnlocked() const { return healingUnlocked_; }
bool Area::IsHealingRoom() const { return healingRoom_; }
void Area::SetHealingUnlocked(bool unlocked) { healingUnlocked_ = unlocked; }
void Area::SetHealingRoom(bool healingRoom) { healingRoom_ = healingRoom; }
bool Area::HasBossBeenEncountered() const { return bossEncountered_; }
void Area::MarkBossEncountered() { bossEncountered_ = true; }
void Area::SetCleared(bool cleared) {
    if (!cleared) return;
    unlocked_ = true;
    visibleInteractables_ = interactables_;
    discoveredNPC_ = hasNPC_;
    bossDefeated_ = true;
    bossEncountered_ = false;
    if (healingRoom_) healingUnlocked_ = true;
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

void Area::ClearEnemies() {
    enemies_.clear();
}

void Area::AddInteractable(const std::string& interactable) {
    if (!interactable.empty()) interactables_.push_back(interactable);
}

void Area::SetNPC(bool hasNPC) { hasNPC_ = hasNPC; }
void Area::SetClues(bool hasClues) {
    cluePool_.Set(hasClues, static_cast<int>(interactables_.size()));
}
void Area::SetItems(bool hasItems) {
    itemPool_.Set(hasItems, static_cast<int>(interactables_.size()));
}
void Area::SetSpawnCondition(EnemySpawnCondition condition) { spawnCondition_ = condition; }

const std::vector<std::string>& Area::GetInteractables() const { return interactables_; }
bool Area::HasNPC() const { return hasNPC_; }
bool Area::HasClues() const { return cluePool_.Has(); }
bool Area::HasItems() const { return itemPool_.Has(); }
bool Area::HasAllClues() const { return cluePool_.HasAll(); }
bool Area::HasAllItems() const { return itemPool_.HasAll(); }
EnemySpawnCondition Area::GetSpawnCondition() const { return spawnCondition_; }

bool Area::ConsumeClue() {
    return cluePool_.Consume();
}

bool Area::ConsumeItem() {
    return itemPool_.Consume();
}
