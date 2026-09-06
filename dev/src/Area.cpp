#include "Area.h"
#include "Enemy.h"
#include <algorithm>

Area::Area(const std::string& name)
    : name_(name), hasNPC_(false), hasClues_(false), hasItems_(false), cluesRemaining_(0), itemsRemaining_(0),
      spawnCondition_(EnemySpawnCondition::None), unlocked_(false), searchCount_(0),
      discoveredNPC_(false), bossDefeated_(false), bossEncountered_(false), healingUnlocked_(false) {}

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
void Area::SetHealingUnlocked(bool unlocked) { healingUnlocked_ = unlocked; }
bool Area::HasBossBeenEncountered() const { return bossEncountered_; }
void Area::MarkBossEncountered() { bossEncountered_ = true; }
void Area::SetCleared(bool cleared) {
    if (!cleared) return;
    unlocked_ = true;
    visibleInteractables_ = interactables_;
    discoveredNPC_ = hasNPC_;
    bossDefeated_ = true;
    bossEncountered_ = false;
    if (name_ == "Fountain") healingUnlocked_ = true;
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
void Area::SetClues(bool hasClues) { hasClues_ = hasClues; cluesRemaining_ = hasClues ? static_cast<int>(interactables_.size()) : 0; }
void Area::SetItems(bool hasItems) { hasItems_ = hasItems; itemsRemaining_ = hasItems ? static_cast<int>(interactables_.size()) : 0; }
void Area::SetSpawnCondition(EnemySpawnCondition condition) { spawnCondition_ = condition; }

const std::vector<std::string>& Area::GetInteractables() const { return interactables_; }
bool Area::HasNPC() const { return hasNPC_; }
bool Area::HasClues() const { return hasClues_; }
bool Area::HasItems() const { return hasItems_; }
bool Area::HasAllClues() const { return cluesRemaining_ == 0; }
bool Area::HasAllItems() const { return itemsRemaining_ == 0; }
EnemySpawnCondition Area::GetSpawnCondition() const { return spawnCondition_; }

bool Area::ConsumeClue() {
    if (!hasClues_ || cluesRemaining_ <= 0) return false;
    --cluesRemaining_;
    hasClues_ = cluesRemaining_ > 0;
    return true;
}

bool Area::ConsumeItem() {
    if (!hasItems_ || itemsRemaining_ <= 0) return false;
    --itemsRemaining_;
    hasItems_ = itemsRemaining_ > 0;
    return true;
}
