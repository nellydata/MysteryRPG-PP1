#pragma once

#include "GameplayTypes.h"
#include <memory>
#include <string>
#include <vector>

class Area;
class CombatManager;
class Player;
class ProgressionManager;
class Enemy;

enum class InvestigationEventType {
    FountainDiscovery, EnemyEncounter, ItemDiscovery, ExplorationXP,
    ClueDiscovery, Beneficial, Hazard, Quiet
};

struct InvestigationResult {
    InvestigationEventType type = InvestigationEventType::Quiet;
    std::string narrative;
    std::string discoveryId;
    std::string enemyId;
    std::vector<ItemDefinition> items;
    std::unique_ptr<Enemy> spawnedEnemy;
    std::int64_t xpAwarded = 0;
    int hpDelta = 0;
    bool fountainUnlocked = false;
    bool primaryResolved = false;
};

class EncounterManager {
public:
    EncounterManager();

    InvestigationResult ResolveInvestigation(Area& area, Player& player,
                                              ProgressionManager& progression);

    bool Explore(Area& area, Player& player, ProgressionManager& progression,
                 CombatManager& combat);
    bool Explore(Area& area, Player& player, ProgressionManager& progression);
    bool explore(Area& area, Player& player, CombatManager& combat,
                 ProgressionManager& progression);
};
