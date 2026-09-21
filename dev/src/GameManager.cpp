#include "GameManager.h"
#include "CombatManager.h"
#include "ContentCatalog.h"
#include "ConsoleFormat.h"
#include "ConsoleUI.h"
#include "DeveloperMode.h"
#include "EncounterManager.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
void EnableRainbowConsole() {
#ifdef _WIN32
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode))
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void RunDropSimulator(ConsoleUI& ui, Player& player, ProgressionManager& progression,
                      MapManager& map, std::mt19937& random) {
    ui.ShowMessage("Developer drop simulator: raw generator rates; no fights, pity, XP, or room progress.");
    ui.ShowMap(map.GetRooms());
    const int roomChoice = ui.Prompt("Room (1-5, 0 return):", 0, 5);
    if (!std::cin || roomChoice == 0) return;
    const auto& room = MapManager::GetFloor1Definitions().at(static_cast<std::size_t>(roomChoice - 1));
    ui.ShowMessage("Drop source: 1 investigation cache, 2 Basic, 3 Rare, 4 Epic, 5 Elite, 6 boss.");
    const int sourceChoice = ui.Prompt("Source (0 return):", 0, 6);
    if (!std::cin || sourceChoice == 0) return;
    if (sourceChoice == 6 && room.bossId.empty()) {
        ui.ShowMessage("This room has no boss drop profile.");
        return;
    }
    const auto source = static_cast<DeveloperDropSource>(sourceChoice - 1);
    const int action = ui.Prompt("1 roll once and keep items, 2 sample rates without keeping items, 0 return:", 0, 2);
    if (!std::cin || action == 0) return;
    if (action == 1) {
        const auto items = DeveloperMode::RollRoom(room, progression.GetLevel(), source, random());
        for (const auto& definition : items) {
            player.AddItem(std::make_unique<Item>(definition));
            ui.ShowMessage("Rolled and kept: " + definition.name + ".");
        }
        if (items.empty()) ui.ShowMessage("No item on this roll.");
        return;
    }
    const int rolls = ui.Prompt("How many independent rolls (1-10000)?", 1, 10000);
    if (!std::cin) return;
    const auto summary = DeveloperMode::SampleRoom(room, progression.GetLevel(), source, rolls, random());
    std::ostringstream report;
    report << "Sampled " << summary.rolls << " " << room.displayName << " rolls. Gear " << summary.gear
           << " (" << std::fixed << std::setprecision(2) << 100.0 * summary.gear / rolls
           << " per 100 rolls), consumables " << summary.consumables << " ("
           << 100.0 * summary.consumables / rolls << " per 100 rolls).";
    ui.ShowMessage(report.str());
    for (int rarity = 0; rarity < 6; ++rarity) {
        std::ostringstream line;
        line << ConsoleFormat::RarityName(static_cast<Rarity>(rarity)) << ": " << summary.rarity[rarity];
        if (summary.gear > 0)
            line << " (" << std::fixed << std::setprecision(2) <<
                100.0 * summary.rarity[rarity] / summary.gear << "% of gear)";
        ui.ShowMessage(line.str());
    }
}
}

void GameManager::Run() {
    ConsoleUI ui;
    std::cout << "MICARE - console edition\nInvestigator name:\n> " << std::flush;
    std::string name;
    if (!std::getline(std::cin, name)) return;
    const bool developerMode = DeveloperMode::IsDeveloperName(name);
    if (developerMode) { name = "Adelaide"; EnableRainbowConsole(); }
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
    if (developerMode) {
        DeveloperMode::Initialize(player, progression, map);
        ui.ShowMessage("Developer mode active: all rooms, catalogue items and Shimmers unlocked at level 100.");
    }
    player.RestoreHealth(player.getMaxHP());
    std::mt19937 dropRandom(std::random_device{}());
    int forcedRoom = -1;
    while (std::cin) {
        int choice;
        if (forcedRoom >= 0) { choice = forcedRoom + 1; forcedRoom = -1; }
        else {
            ui.RenderDashboard(player, progression, map.GetRooms(), "Investigate, improve your equipment, and challenge each room's guardian.", developerMode);
            choice = ui.Prompt("Destination or panel:", 0, developerMode ? 10 : 9);
        }
        if (!std::cin || choice == 0) break;
        if (choice == 6) { ui.ManageInventory(player); continue; }
        if (choice == 7) { ui.ManageShimmerSkills(progression); continue; }
        if (choice == 8) { ui.ShowMap(map.GetRooms()); continue; }
        if (choice == 9) { ui.ShowBattleHistory(combat); continue; }
        if (choice == 10 && developerMode) {
            RunDropSimulator(ui, player, progression, map, dropRandom);
            continue;
        }
        Area& room = map.SelectRoom(choice - 1);
        if (!room.IsUnlocked()) {
            ui.ShowMessage(ConsoleFormat::Warning("That room is locked; defeat the previous guardian first."));
            continue;
        }
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
