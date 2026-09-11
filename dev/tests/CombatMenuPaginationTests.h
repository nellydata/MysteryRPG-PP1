#pragma once
#include "CombatManager.h"
#include "ConsoleUI.h"
#include "ContentCatalog.h"
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <algorithm>
#include <cassert>
#include <sstream>

inline void RunCombatMenuPaginationTests() {
    Player player("Pagination player");
    ProgressionManager progression;
    CombatManager combat(41);
    EncounterManager encounters;
    MapManager map;
    Area room("Consumable pagination fixture");
    EnemyDefinition definition;
    definition.name = "Pagination opponent";
    definition.hpBase = 10000; definition.atkBase = 1; definition.defBase = 1; definition.speed = 1;
    room.addEnemy(std::make_unique<Enemy>(definition, 1));
    const auto catalogue = BuildConsumableCatalog();
    const auto ration = std::find_if(catalogue.begin(), catalogue.end(), [](const ItemDefinition& item) {
        return item.name == "cellar_rations";
    });
    assert(ration != catalogue.end());
    std::vector<unsigned long long> instances;
    for (int index = 0; index < 25; ++index) {
        auto item = std::make_unique<Item>(*ration);
        instances.push_back(item->getInstanceId());
        player.AddItem(std::move(item));
    }
    player.GetInventory().SetLocked(instances[12], true);
    player.takeDamage(40);
    ConsoleUI::SetFast(true);
    // Cancel; next/previous; reject locked second-page entry; reject nonexistent
    // final-page entry; use the exact 25th instance; flee and leave normally.
    std::istringstream input("2\n0\n2\n13\n14\n13\n1\n2\n13\n13\n2\n1\n4\n0\n");
    std::ostringstream output;
    auto* previousIn = std::cin.rdbuf(input.rdbuf());
    auto* previousOut = std::cout.rdbuf(output.rdbuf());
    encounters.Explore(room, player, progression, combat, &map);
    std::cin.rdbuf(previousIn); std::cout.rdbuf(previousOut); std::cin.clear();
    const auto text = output.str();
    assert(text.find("Consumables page 1/3") != std::string::npos);
    assert(text.find("Consumables page 2/3") != std::string::npos);
    assert(text.find("Consumables page 3/3") != std::string::npos);
    assert(text.find("That consumable is not on this page.") != std::string::npos);
    assert(text.find("locked, unavailable, or has no effect") != std::string::npos);
    assert(player.GetInventory().GetItems().size() == 24);
    assert(!player.GetInventory().Contains(instances.back()));
    for (std::size_t index = 0; index < instances.size() - 1; ++index)
        assert(player.GetInventory().Contains(instances[index]));
    assert(player.GetInventory().IsLocked(instances[12]));
    assert(player.getHP() == 79 && progression.GetXP() == 0);
    int actions = 0;
    for (const auto& event : combat.GetEvents())
        actions += event.message == "Pagination player uses cellar_rations on Pagination player.";
    assert(actions == 1);
    std::cout << "Combat consumable pagination passed: 25 exact instances, three pages, next/previous/cancel, locked and invalid choices, one final-page use.\n";
}
