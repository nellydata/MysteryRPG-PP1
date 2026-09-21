#pragma once

#include "GameplayTypes.h"
#include <array>
#include <string>
#include <vector>

class MapManager;
class Player;
class ProgressionManager;

enum class DeveloperDropSource { Investigation, Basic, Rare, Epic, Elite, Boss };

struct DeveloperDropSummary {
    int rolls = 0;
    int gear = 0;
    int consumables = 0;
    std::array<int, 6> rarity{};
};

namespace DeveloperMode {
bool IsDeveloperName(const std::string& submitted);
void Initialize(Player& player, ProgressionManager& progression, MapManager& map);
std::vector<ItemDefinition> RollRoom(const RoomDefinition& room, int level,
                                     DeveloperDropSource source, unsigned seed);
DeveloperDropSummary SampleRoom(const RoomDefinition& room, int level,
                                DeveloperDropSource source, int rolls, unsigned seed);
}
