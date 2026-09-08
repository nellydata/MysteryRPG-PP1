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
    Player player("Adelaide", 20, 10);
    player.SetLevelStats(1);
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

    player.RestoreHealth(player.getMaxHP());
    progression.UnlockShimmerSkill("Pulsus Luminis");

    while (player.isAlive()) {
        const auto& rooms = mapManager.GetRooms();
        ui.RenderDashboard(player, progression, rooms, "Select a destination or panel.");
        const int panelBase = static_cast<int>(rooms.size()) + 1;
        const int choice = ui.Prompt("Select a destination or panel.", 0,
                                    ui.IsDeveloperMode() ? panelBase + 3 : panelBase + 2);
        if (choice == -1) {
            ui.ShowMessage("Enter one of the displayed choices.");
            ui.Prompt("Press 0 to continue.", 0, 0);
            continue;
        }
        if (choice == 0) break;
        if (choice == panelBase) {
            ui.ShowInventory(player);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == panelBase + 1) {
            ui.ShowShimmerSkills(combat);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == panelBase + 2) {
            ui.ShowMap(rooms);
            ui.Prompt("Press 0 to return.", 0, 0);
            continue;
        }
        if (choice == panelBase + 3 && ui.IsDeveloperMode()) {
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
        if (!selectedRoom.IsUnlocked()) {
            ui.ShowMessage("That room is locked.");
            ui.Prompt("Press 0 to continue.", 0, 0);
            continue;
        }
        encounterManager.Explore(selectedRoom, player, progression, combat, &mapManager);
    }

    ui.Clear();
    ui.RenderDashboard(player, progression, mapManager.GetRooms(), "SESSION COMPLETE");
    if (!player.isAlive()) {
        ui.ShowMessage("The Shimmer claims Adelaide.");
    } else {
        ui.ShowMessage("Adelaide leaves the cathedral for now.");
    }
}
