#pragma once

class Area;
class Player;
class CombatManager;
class ProgressionManager;

class EncounterManager {
public:
	EncounterManager();

	bool explore(Area& area, Player& player, CombatManager& combat,
				 ProgressionManager& progression);
};
