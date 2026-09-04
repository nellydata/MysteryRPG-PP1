#include "MapManager.h"
#include "Area.h"
#include "Automata.h"
#include "Boss.h"
#include "Nun.h"
#include <functional>
#include <memory>

MapManager::MapManager() = default;
MapManager::~MapManager() = default;

std::unique_ptr<Area> MapManager::generateArea(const std::string& name) {
    auto area = std::make_unique<Area>(name);
    const std::size_t seed = std::hash<std::string>{}(name);
    const int enemyCount = 2 + static_cast<int>(seed % 3);

    for (int index = 0; index < enemyCount; ++index) {
        switch ((seed + static_cast<std::size_t>(index)) % 3) {
        case 0:
            area->addEnemy(std::make_unique<Automata>());
            break;
        case 1:
            area->addEnemy(std::make_unique<Nun>());
            break;
        default:
            area->addEnemy(std::make_unique<Enemy>("Shimmer Husk", 10, 3));
            break;
        }
    }

    if (name == "Sanctum") {
        area->addEnemy(std::make_unique<Boss>());
    }

    return area;
}
