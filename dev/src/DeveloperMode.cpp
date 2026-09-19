#include "DeveloperMode.h"
#include "ContentCatalog.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include <algorithm>
#include <cctype>
#include <memory>
#include <random>
#include <stdexcept>

namespace DeveloperMode {
bool IsDeveloperName(const std::string& submitted) {
    const auto first = submitted.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return true;
    const auto last = submitted.find_last_not_of(" \t\r\n");
    std::string normalized = submitted.substr(first, last - first + 1);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char letter) {
        return static_cast<char>(std::tolower(letter));
    });
    return normalized == "adelaide";
}

void Initialize(Player& player, ProgressionManager& progression, MapManager& map) {
    progression.SetMaxLevel(player);
    player.RestoreHealth(player.getMaxHP());
    for (Area& room : map.GetRooms()) room.UnlockDeveloperContent();
    for (const auto& definition : BuildEquippableCatalog())
        if (!player.FindItem(definition.name)) player.AddItem(std::make_unique<Item>(definition));
    for (const auto& definition : BuildConsumableCatalog())
        if (!player.FindItem(definition.name)) player.AddItem(std::make_unique<Item>(definition));
    for (const auto& definition : BuildClueCatalog())
        if (!player.FindItem(definition.name)) player.AddItem(std::make_unique<Item>(definition));
    for (const auto& definition : BuildCruxCatalog()) {
        if (!player.FindItem(definition.name)) player.AddItem(std::make_unique<Item>(definition));
        progression.UnlockCrux(definition.name);
    }
    for (const auto& name : BuildLatinShimmerSkillNames()) progression.UnlockShimmerSkill(name);
    player.RestoreHealth(player.getMaxHP());
}

std::vector<ItemDefinition> RollRoom(const RoomDefinition& room, int level,
                                     DeveloperDropSource source, unsigned seed) {
    if (source == DeveloperDropSource::Investigation)
        return GenerateInvestigationLoot(room.consumableProfile, level, room.id, seed, true);
    LootContext context;
    context.playerLevel = level;
    context.roomId = room.id;
    context.enemyTier = source == DeveloperDropSource::Basic ? EnemyTier::Basic :
        source == DeveloperDropSource::Rare ? EnemyTier::Rare :
        source == DeveloperDropSource::Epic ? EnemyTier::Epic :
        source == DeveloperDropSource::Elite ? EnemyTier::Elite : EnemyTier::Boss;
    if (source == DeveloperDropSource::Boss && room.bossId.empty())
        throw std::invalid_argument("This room has no boss drop profile");
    context.enemyId = source == DeveloperDropSource::Boss ? room.bossId :
        !room.enemiesAfterClear.empty() ? room.enemiesAfterClear.front().enemyId : "Devotee Acolyte";
    if (const auto* enemy = FindEnemyDefinition(context.enemyId)) context.faction = enemy->faction;
    context.bossFarmEcho = source == DeveloperDropSource::Boss;
    return GenerateLoot(context, seed);
}

DeveloperDropSummary SampleRoom(const RoomDefinition& room, int level,
                                DeveloperDropSource source, int rolls, unsigned seed) {
    if (rolls < 1 || rolls > 100000) throw std::invalid_argument("Sample count must be 1..100000");
    DeveloperDropSummary summary;
    summary.rolls = rolls;
    std::mt19937 seeds(seed);
    for (int index = 0; index < rolls; ++index) {
        const auto items = RollRoom(room, level, source, seeds());
        for (const auto& item : items) {
            if (item.category == ItemCategory::Equippable) {
                ++summary.gear;
                ++summary.rarity[static_cast<int>(item.rarity)];
            } else if (item.category == ItemCategory::Consumable) ++summary.consumables;
        }
    }
    return summary;
}
}
