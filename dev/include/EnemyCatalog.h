#pragma once

#include "GameplayTypes.h"
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <functional>

struct EnemySelectionTrace {
	EnemyTier tier = EnemyTier::Basic;
	std::string enemyId;
	int level = 1;
	std::string source;
};

// An opt-in sink keeps statistical debug runs useful without flooding the UI.
void SetEnemySelectionTraceSink(std::function<void(const EnemySelectionTrace&)> sink);
std::array<double, 4> GetEnemyTierWeights(int level);

const EnemyDefinition* FindEnemyDefinition(const std::string& name);
const std::vector<EnemyDefinition>& GetEnemyCatalogue();
std::unique_ptr<class Enemy> CreateCatalogueEnemy(const std::string& name, int level);
std::unique_ptr<class Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level, bool allowElite = true, bool allowBoss = false);
std::unique_ptr<class Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level,
	EnemyFaction faction, EnemyTier maximumTier);
std::unique_ptr<class Enemy> CreateRoomEnemy(const RoomDefinition& room, unsigned seed, int level, bool afterClear);
bool HasEligibleRoomEnemy(const RoomDefinition& room, int level, bool afterClear);
std::unique_ptr<class Enemy> CreateFirstClearBoss(const RoomDefinition& room);
bool ValidateEnemyCatalogue(std::string* error = nullptr);

const char* EnemyFactionName(EnemyFaction faction);
const char* EnemyTierName(EnemyTier tier);
