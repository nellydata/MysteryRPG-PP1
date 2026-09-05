#include "GameManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
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

void GameManager::Run() {
    Player player("Adelaide", 30, 10);
    MapManager mapManager;
    CombatManager combat;
    EncounterManager encounterManager;
    ProgressionManager progression;
    ConsoleUI ui;
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

    while (player.isAlive()) {
        const auto& rooms = mapManager.GetRooms();
        ui.RenderDashboard(player, progression, rooms, "Select a destination or panel.");
        const int choice = ui.Prompt("Select a destination or panel.", 0,
                                     ui.IsDeveloperMode() ? 8 : 7);
        if (choice == -1) {
            ui.ShowMessage("Enter one of the displayed choices.");
            ui.Prompt("Press 0 to continue.", 0, 0);
            continue;
        }
        if (choice == 0) break;
        if (choice == 5) {
            ui.ShowInventory(player);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == 6) {
            ui.ShowShimmerSkills(combat);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == 7) {
            ui.ShowMap(rooms);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == 8 && ui.IsDeveloperMode()) {
            ui.ShowDeveloperPanel();
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice < 1 || choice > static_cast<int>(rooms.size())) {
            ui.ShowMessage("That room is not available.");
            ui.Prompt("Press 0 to continue.", 0, 0);
            continue;
        }

        Area& selectedRoom = mapManager.SelectRoom(choice - 1);
        encounterManager.Explore(selectedRoom, player, progression, combat);
    }

    ui.Clear();
    ui.RenderDashboard(player, progression, mapManager.GetRooms(), "SESSION COMPLETE");
    if (!player.isAlive()) {
        ui.ShowMessage("The Shimmer claims Adelaide.");
    } else {
        ui.ShowMessage("Adelaide leaves the cathedral for now.");
    }
}
