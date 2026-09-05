#pragma once

class Area;
class CombatManager;
class Player;
class ProgressionManager;

class EncounterManager {
public:
    EncounterManager();

    bool Explore(Area& area, Player& player, ProgressionManager& progression,
                 CombatManager& combat);
    bool Explore(Area& area, Player& player, ProgressionManager& progression);
    bool explore(Area& area, Player& player, CombatManager& combat,
                 ProgressionManager& progression);
};
