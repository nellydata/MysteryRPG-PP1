#include "MapManager.h"
#include <iostream>
#include <stdexcept>

namespace {
const char* SpawnConditionText(EnemySpawnCondition condition) {
    switch (condition) {
    case EnemySpawnCondition::None: return "None (0%)";
    case EnemySpawnCondition::Low: return "Low (20%)";
    case EnemySpawnCondition::Medium: return "Medium (50%)";
    case EnemySpawnCondition::High: return "High (80%)";
    }
    return "Unknown";
}
}

MapManager::MapManager() {
    GenerateFloor1();
}

void MapManager::GenerateFloor1() {
    rooms_.clear();

    rooms_.emplace_back("Entrance Hall");
    Area& entrance = rooms_.back();
    entrance.AddInteractable("Broken statue");
    entrance.AddInteractable("Old banner");
    entrance.SetClues(true);
    entrance.SetSpawnCondition(EnemySpawnCondition::Low);

    rooms_.emplace_back("Archive Room");
    Area& archive = rooms_.back();
    archive.AddInteractable("Dusty bookshelf");
    archive.AddInteractable("Locked cabinet");
    archive.SetNPC(true);
    archive.SetClues(true);
    archive.SetItems(true);
    archive.SetSpawnCondition(EnemySpawnCondition::Medium);

    rooms_.emplace_back("Chapel");
    Area& chapel = rooms_.back();
    chapel.AddInteractable("Altar");
    chapel.AddInteractable("Prayer bench");
    chapel.SetNPC(true);
    chapel.SetClues(true);
    chapel.SetSpawnCondition(EnemySpawnCondition::High);

    rooms_.emplace_back("Storage Room");
    Area& storage = rooms_.back();
    storage.AddInteractable("Crates");
    storage.AddInteractable("Supply shelf");
    storage.SetItems(true);
    storage.SetSpawnCondition(EnemySpawnCondition::None);
}

std::vector<Area>& MapManager::GetRooms() { return rooms_; }
const std::vector<Area>& MapManager::GetRooms() const { return rooms_; }

Area& MapManager::SelectRoom(int index) {
    if (index < 0 || index >= static_cast<int>(rooms_.size())) {
        throw std::out_of_range("Room index is out of range.");
    }
    return rooms_[index];
}

void MapManager::DescribeRoom(const Area& area) const {
    std::cout << "\n" << area.getName() << "\n"
              << "NPC present: " << (area.HasNPC() ? "Yes" : "No") << "\n"
              << "Clues present: " << (area.HasClues() ? "Yes" : "No") << "\n"
              << "Items present: " << (area.HasItems() ? "Yes" : "No") << "\n"
              << "Enemy spawn chance: " << SpawnConditionText(area.GetSpawnCondition()) << "\n"
              << "Interactables:\n";

    for (const auto& interactable : area.GetInteractables()) {
        std::cout << "- " << interactable << "\n";
    }
}
