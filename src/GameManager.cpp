#include "GameManager.h"
#include "Area.h"
#include "CombatManager.h"
#include "EncounterManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <memory>

void GameManager::Run() {
	Player player("Adelaide", 30, 10);
	MapManager map;
	CombatManager combat;
	EncounterManager encounters;
	ProgressionManager progression;

	std::cout << "MICARE\n"
			  << "The cathedral is silent except for the pulse of the Shimmer.\n"
			  << "Entering the cathedral...\n";

	auto entrance = map.generateArea("Entrance Hall");
	const bool cleared = encounters.explore(*entrance, player, combat, progression);

	if (cleared) {
		std::cout << "The Entrance Hall is clear. Adelaide reaches level "
				  << progression.getLevel() << ".\n";
	} else if (!player.isAlive()) {
		std::cout << "The Shimmer claims Adelaide.\n";
	} else {
		std::cout << "Adelaide retreats into the cathedral shadows.\n";
	}
}
