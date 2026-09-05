#pragma once

#include "GameplayTypes.h"
#include <memory>
#include <string>
#include <vector>

const EnemyDefinition* FindEnemyDefinition(const std::string& name);
const std::vector<EnemyDefinition>& GetEnemyCatalogue();
std::unique_ptr<class Enemy> CreateCatalogueEnemy(const std::string& name, int level);
std::unique_ptr<class Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level, bool allowElite = true, bool allowBoss = false);
std::unique_ptr<class Enemy> CreateRandomCatalogueEnemy(unsigned seed, int level,
	EnemyFaction faction, EnemyTier maximumTier);

const char* EnemyFactionName(EnemyFaction faction);
const char* EnemyTierName(EnemyTier tier);