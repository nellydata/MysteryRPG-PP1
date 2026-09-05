#include "MapManager.h"
#include <iostream>
#include <stdexcept>

MapManager::MapManager() {
    GenerateFloor1();
}

void MapManager::GenerateFloor1() {
    rooms_.clear();

    rooms_.emplace_back("Entrance Hall");
    Area& entrance = rooms_.back();
    entrance.SetUnlocked(true);
    entrance.AddInteractable("Broken statue");
    entrance.AddInteractable("Old banner");
    entrance.SetClues(true);
    entrance.SetItems(true);
    entrance.SetSpawnCondition(EnemySpawnCondition::Low);

    rooms_.emplace_back("Archive Room");
    Area& archive = rooms_.back();
    archive.AddInteractable("Dusty bookshelf");
    archive.AddInteractable("Locked cabinet");
    archive.AddInteractable("Figure in the shadows");
    archive.SetNPC(true);
    archive.SetClues(true);
    archive.SetItems(true);
    archive.SetSpawnCondition(EnemySpawnCondition::Medium);

    rooms_.emplace_back("Chapel");
    Area& chapel = rooms_.back();
    chapel.AddInteractable("Altar");
    chapel.AddInteractable("Prayer bench");
    chapel.AddInteractable("Figure in the shadows");
    chapel.SetNPC(true);
    chapel.SetClues(true);
    chapel.SetItems(true);
    chapel.SetSpawnCondition(EnemySpawnCondition::High);

    rooms_.emplace_back("Storage Room");
    Area& storage = rooms_.back();
    storage.AddInteractable("Crates");
    storage.AddInteractable("Supply shelf");
    storage.SetClues(true);
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
    std::cout << "\nYou enter the " << area.getName() << ".\n"
              << "Objects of interest:\n";
    for (const auto& interactable : area.GetInteractables()) {
        std::cout << "- " << interactable << "\n";
    }
}
