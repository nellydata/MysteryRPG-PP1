#pragma once

#include "GameplayTypes.h"
#include "Enemy.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

class Area;
class CombatManager;
class Player;
class ProgressionManager;
class MapManager;

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

struct VictoryResult {
    bool resolved = false;
    bool firstClear = false;
    std::vector<ItemDefinition> items;
    std::vector<std::string> messages;
};

class EncounterManager {
public:
    EncounterManager();

    InvestigationResult ResolveInvestigation(Area& area, Player& player,
                                              ProgressionManager& progression);
    InvestigationResult ResolveInvestigation(Area& area, Player& player,
                                              ProgressionManager& progression, unsigned seed);
    VictoryResult ResolveVictory(Area& area, Player& player, ProgressionManager& progression,
                                MapManager& map, Enemy& enemy, std::optional<unsigned> seed = std::nullopt);
    int GetBasicGearMisses() const { return basicGearMisses_; }

    bool Explore(Area& area, Player& player, ProgressionManager& progression,
                 CombatManager& combat, MapManager* map = nullptr);
    bool Explore(Area& area, Player& player, ProgressionManager& progression);
    bool explore(Area& area, Player& player, CombatManager& combat,
                 ProgressionManager& progression);
private:
    int basicGearMisses_ = 0;
};
