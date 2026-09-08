#include "GameManager.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <iostream>

void GameManager::Run() {
    ConsoleUI ui;
    std::cout << "MICARE - console edition\nInvestigator name (Enter for Adelaide):\n> " << std::flush;
    std::string name;
    if (!std::getline(std::cin, name)) return;
    if (name.empty()) name = "Adelaide";
    Player player(name, 100, 10);
    player.SetLevelStats(1);
    MapManager map;
    CombatManager combat;
    EncounterManager encounters;
    ProgressionManager progression;
    for (const auto& definition : BuildEquippableCatalog()) {
        if (definition.name != "plain_rosary" && definition.name != "linen_veil") continue;
        player.AddItem(std::make_unique<Item>(definition));
        if (definition.name == "plain_rosary") player.Equip(*player.GetInventory().GetItems().back().item);
    }
    for (const auto& definition : BuildConsumableCatalog())
        if (definition.name == "cellar_rations" || definition.name == "purification_salts")
            player.AddItem(std::make_unique<Item>(definition));
    for (const auto& definition : BuildClueCatalog())
        if (definition.name == "storage_key") player.AddItem(std::make_unique<Item>(definition));
    for (const auto& definition : BuildCruxCatalog())
        if (definition.name == "first_nuns_diary") player.AddItem(std::make_unique<Item>(definition));
    progression.UnlockShimmerSkill("Pulsus Luminis");
    progression.EquipShimmerSkill("Pulsus Luminis");
    player.RestoreHealth(player.getMaxHP());
    int forcedRoom = -1;
    while (std::cin) {
        int choice;
        if (forcedRoom >= 0) { choice = forcedRoom + 1; forcedRoom = -1; }
        else {
            ui.RenderDashboard(player, progression, map.GetRooms(), "Investigate, improve your equipment, and challenge each room's guardian.");
            choice = ui.Prompt("Destination or panel:", 0, 9);
        }
        if (!std::cin || choice == 0) break;
        if (choice == 6) { ui.ManageInventory(player); continue; }
        if (choice == 7) { ui.ManageShimmerSkills(progression); continue; }
        if (choice == 8) { ui.ShowMap(map.GetRooms()); continue; }
        if (choice == 9) { ui.ShowBattleHistory(combat); continue; }
        Area& room = map.SelectRoom(choice - 1);
        if (!room.IsUnlocked()) { ui.ShowMessage("That room is locked."); continue; }
        encounters.Explore(room, player, progression, combat, &map);
        if (!std::cin) break;
        if (!player.isAlive()) {
            player.RestoreHealth(player.getMaxHP());
            ui.ShowMessage("You return to the Fountain at full health.");
            forcedRoom = 0;
        }
    }
    ui.ShowMessage("Session complete.");
}
