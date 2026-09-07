#include "MapManager.h"
#include <iostream>
#include <stdexcept>

MapManager::MapManager() {
    GenerateFloor1();
}

const std::vector<RoomDefinition>& MapManager::GetFloor1Definitions() {
    static const std::vector<RoomDefinition> definitions = {
        {"Fountain", "Fountain", RoomType::Sanctuary,
            {{"Devotee Acolyte", 60}, {"Devotee Penitent", 25}, {"Devotee Flagellant", 15}},
            {{"Devotee Acolyte", 70}, {"Devotee Penitent", 30}}, "Choir Warden", 50, 8, ConsumableProfile::Restorative, "Entrance Hall"},
        {"Entrance Hall", "Entrance Hall", RoomType::Entrance,
            {{"Devotee Acolyte", 30}, {"Devotee Penitent", 20}, {"Devotee Flagellant", 15}, {"Choir Ascetic", 10}, {"Benediction Sister", 10}, {"Automata Unit-01", 5}},
            {{"Devotee Acolyte", 25}, {"Devotee Penitent", 20}, {"Devotee Flagellant", 15}, {"Choir Ascetic", 15}, {"Benediction Sister", 15}, {"Choir Warden", 10}}, "Matron of the Catacombs", 50, 8, ConsumableProfile::Devotional, "Archive Room"},
        {"Archive Room", "Archive Room", RoomType::Archive,
            {{"Automata Unit-01", 30}, {"Automata Sharpsight", 20}, {"Automata Striker", 20}, {"Automata Vanguard", 15}, {"Automata Oracle Unit", 10}, {"Oracle-VII", 5}},
            {{"Automata Unit-01", 20}, {"Automata Striker", 20}, {"Automata Sharpsight", 15}, {"Automata Vanguard", 20}, {"Automata Oracle Unit", 15}, {"Oracle-VII", 10}}, "Overlord-Prime", 50, 8, ConsumableProfile::OccultMechanica, "Chapel"},
        {"Chapel", "Chapel", RoomType::Chapel,
            {{"Devotee Flagellant", 12}, {"Benediction Sister", 12}, {"Verdigris Rotbearer", 15}, {"Verdigris Bleedcaller", 15}, {"Choir Ascetic", 12}, {"Verdigris Pyre Sister", 10}, {"Rot Herald", 6}, {"Pyre Matron", 6}},
            {{"Benediction Sister", 10}, {"Verdigris Rotbearer", 12}, {"Verdigris Bleedcaller", 12}, {"Verdigris Pyre Sister", 12}, {"Rot Herald", 10}, {"Pyre Matron", 10}, {"Ascetic Purifier", 7}, {"Choir Ascetic", 10}}, "Matron of Rot", 50, 8, ConsumableProfile::Ritual, "Storage Room"},
        {"Storage Room", "Storage Room", RoomType::Storage,
            {{"Automata Unit-01", 25}, {"Automata Striker", 20}, {"Automata Vanguard", 15}, {"Automata Sharpsight", 15}, {"Verdigris Thrall", 15}, {"Verdigris Rotbearer", 10}},
            {{"Automata Striker", 20}, {"Automata Vanguard", 20}, {"Automata Sharpsight", 15}, {"Automata Oracle Unit", 15}, {"Verdigris Thrall", 15}, {"Verdigris Rotbearer", 15}}, "The Reliquary Engine", 50, 8, ConsumableProfile::SupplyCache, ""}
    };
    return definitions;
}

const RoomDefinition* MapManager::GetRoomDefinition(const Area& area) const { return GetRoomDefinition(area.getName()); }
const RoomDefinition* MapManager::GetRoomDefinition(const std::string& roomId) const {
    for (const auto& definition : GetFloor1Definitions()) if (definition.id == roomId) return &definition;
    return nullptr;
}

void MapManager::GenerateFloor1() {
    rooms_.clear();

    rooms_.emplace_back("Fountain");
    Area& fountain = rooms_.back();
    fountain.SetUnlocked(true);
    fountain.SetHealingRoom(true);
    fountain.AddInteractable("The restorative cathedral fountain");
    fountain.SetSpawnCondition(EnemySpawnCondition::Low);

    rooms_.emplace_back("Entrance Hall");
    Area& entrance = rooms_.back();
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
