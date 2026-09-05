#include "GameManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "Item.h"
#include <iostream>

namespace {
void AddNamedItem(Player& player, const std::vector<ItemDefinition>& definitions,
                  const std::string& name) {
    for (const auto& definition : definitions) {
        if (definition.name == name) {
            player.AddItem(std::make_unique<Item>(definition));
            return;
        }
    }
}
}

namespace {
void PrintStatus(const Player& player, const ProgressionManager& progression) {
    std::cout << "Status: HP " << player.getHP() << "/" << player.getMaxHP()
              << " | Attack " << player.getAttack()
              << " | Defense " << player.getDefense()
              << " | Speed " << player.getSpeed()
              << " | Crit " << player.getCritRate() << "%"
              << " | Level " << progression.GetLevel()
              << " | XP " << progression.GetXP() << "\n";
}
}

void GameManager::Run() {
    Player player("Adelaide", 30, 10);
    MapManager mapManager;
    CombatManager combat;
    EncounterManager encounterManager;
    ProgressionManager progression;
    const auto equipment = BuildEquippableCatalog();
    const auto consumables = BuildConsumableCatalog();
    const auto clues = BuildClueCatalog();
    const auto cruxItems = BuildCruxCatalog();
    AddNamedItem(player, equipment, "plain_rosary");
    AddNamedItem(player, equipment, "linen_veil");
    AddNamedItem(player, consumables, "cellar_rations");
    AddNamedItem(player, consumables, "purification_salts");
    AddNamedItem(player, clues, "storage_key");
    AddNamedItem(player, cruxItems, "first_nuns_diary");
    if (Item* hand = player.FindItem("plain_rosary")) player.Equip(*hand);

    std::cout << "MICARE\n"
              << "The cathedral is silent except for the pulse of the Shimmer.\n";

    while (player.isAlive()) {
        std::cout << "\n";
        PrintStatus(player, progression);
        std::cout << "Choose a room:\n";
        const auto& rooms = mapManager.GetRooms();
        for (std::size_t index = 0; index < rooms.size(); ++index) {
            std::cout << index + 1 << ". " << rooms[index].getName() << "\n";
        }
        std::cout << "5. Open inventory\n0. Leave the cathedral\n> ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Enter one of the displayed choices.\n";
            continue;
        }
        if (choice == 0) break;
        if (choice == 5) {
            player.PrintInventory();
            continue;
        }
        if (choice < 1 || choice > static_cast<int>(rooms.size())) {
            std::cout << "That room is not available.\n";
            continue;
        }

        Area& selectedRoom = mapManager.SelectRoom(choice - 1);
        mapManager.DescribeRoom(selectedRoom);
        encounterManager.Explore(selectedRoom, player, progression, combat);
    }

    std::cout << "\n";
    PrintStatus(player, progression);
    if (!player.isAlive()) {
        std::cout << "The Shimmer claims Adelaide.\n";
    } else {
        std::cout << "Adelaide leaves the cathedral for now.\n";
    }
}
