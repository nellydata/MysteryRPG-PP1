#include "MapManager.h"
#include "Area.h"
#include "Enemy.h"
#include <memory>

MapManager::MapManager() = default;
MapManager::~MapManager() = default;

std::unique_ptr<Area> MapManager::generateArea(const std::string& name) {
	auto area = std::make_unique<Area>(name);
	// Placeholder enemies
	area->addEnemy(std::make_unique<Enemy>("Goblin", 8, 2));
	area->addEnemy(std::make_unique<Enemy>("Goblin", 8, 2));
	return area;
}
