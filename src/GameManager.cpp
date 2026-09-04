#include "GameManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <iostream>

void GameManager::Run() {
	Player player("Adelaide", 30, 10);
	MapManager mapManager;
	CombatManager combat;
	EncounterManager encounterManager;
	ProgressionManager progression;

	std::cout << "MICARE\n"
			  << "The cathedral is silent except for the pulse of the Shimmer.\n";

	while (player.isAlive()) {
		std::cout << "\nChoose a room:\n";
		const auto& rooms = mapManager.GetRooms();
		for (std::size_t index = 0; index < rooms.size(); ++index) {
			std::cout << index + 1 << ". " << rooms[index].getName() << "\n";
		}
		std::cout << "0. Leave the cathedral\n> ";

		int choice = 0;
		if (!(std::cin >> choice)) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			std::cout << "Enter one of the displayed choices.\n";
			continue;
		}
		if (choice == 0) break;
		if (choice < 1 || choice > static_cast<int>(rooms.size())) {
			std::cout << "That room is not available.\n";
			continue;
		}

		Area& selectedRoom = mapManager.SelectRoom(choice - 1);
		mapManager.DescribeRoom(selectedRoom);
		encounterManager.Explore(selectedRoom, player, progression, combat);

		std::cout << "\nLevel: " << progression.GetLevel()
				  << " | XP: " << progression.GetXP()
				  << " | HP: " << player.getHP()
				  << " | Inventory: " << player.GetInventorySize() << "\n";
	}

	if (!player.isAlive()) {
		std::cout << "The Shimmer claims Adelaide.\n";
	} else {
		std::cout << "Adelaide leaves the cathedral for now.\n";
	}
}
