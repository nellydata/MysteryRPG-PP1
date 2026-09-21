#include "Area.h"
#include "CombatManager.h"
#include "ConsumableEffects.h"
#include "ContentCatalog.h"
#include "EncounterManager.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "StatusRules.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int GearCount(const std::vector<ItemDefinition>& items) {
    int count = 0;
    for (const auto& item : items) if (item.category == ItemCategory::Equippable) ++count;
    return count;
}
void CheckLoot() {
    struct TierExpectation { EnemyTier tier; int guaranteed; double extra; double consumable; };
    const TierExpectation tiers[] = {
        {EnemyTier::Basic, 0, .85, .30}, {EnemyTier::Rare, 1, .20, .40},
        {EnemyTier::Epic, 1, .50, .50}, {EnemyTier::Elite, 2, .35, .65},
        {EnemyTier::Boss, 3, .50, 1.0}
    };
    constexpr int samples = 5000;
    for (const auto& expectation : {std::pair<int, double>{1, .85}, {15, .85 + .15 * 14 / 29.0}, {30, 1.0}}) {
        LootContext basic;
        basic.playerLevel = expectation.first;
        basic.enemyTier = EnemyTier::Basic;
        int gearWins = 0;
        for (unsigned seed = 1; seed <= samples; ++seed)
            gearWins += GearCount(GenerateLoot(basic, seed)) > 0;
        Check(std::abs(gearWins / double(samples) - expectation.second) < .035,
            "Basic gear level scaling");
    }
    for (const auto& tier : tiers) {
        int extras = 0, consumables = 0;
        LootContext context;
        context.playerLevel = tier.tier == EnemyTier::Basic ? 1 : 100;
        context.enemyTier = tier.tier;
        for (unsigned seed = 1; seed <= samples; ++seed) {
            const auto loot = GenerateLoot(context, seed);
            const int gear = GearCount(loot);
            Check(gear >= tier.guaranteed && gear <= tier.guaranteed + 1, "Loot gear count");
            extras += gear > tier.guaranteed;
            for (const auto& item : loot) {
                if (item.category == ItemCategory::Consumable) ++consumables;
                else Check(tier.tier == EnemyTier::Boss || item.rarity <= Rarity::Epic,
                    "Non-boss Legendary/Artifact drop");
            }
        }
        Check(std::abs(extras / double(samples) - tier.extra) < .035, "Loot extra chance");
        Check(std::abs(consumables / double(samples) - tier.consumable) < .035, "Loot consumable chance");
    }
    LootContext boss;
    boss.enemyTier = EnemyTier::Boss;
    boss.playerLevel = 100;
    std::array<int, 6> rarity{};
    int total = 0;
    for (unsigned seed = 1; seed <= samples; ++seed) for (const auto& item : GenerateLoot(boss, seed)) {
        if (item.category != ItemCategory::Equippable) continue;
        ++rarity[static_cast<int>(item.rarity)]; ++total;
    }
    for (const auto& expected : {std::pair<Rarity, double>{Rarity::Rare, .05},
                                  {Rarity::Epic, .65}, {Rarity::Legendary, .25},
                                  {Rarity::Artifact, .05}})
        Check(std::abs(rarity[static_cast<int>(expected.first)] / double(total) - expected.second) < .025,
            "Level-100 boss rarity distribution");
    std::cout << "L100 boss gear sample n=" << total << " (5000 independent encounters): Rare "
        << 100.0 * rarity[static_cast<int>(Rarity::Rare)] / total << "%, Epic "
        << 100.0 * rarity[static_cast<int>(Rarity::Epic)] / total << "%, Legendary "
        << 100.0 * rarity[static_cast<int>(Rarity::Legendary)] / total << "%, Artifact "
        << 100.0 * rarity[static_cast<int>(Rarity::Artifact)] / total << "%\n";
    for (int level : {1, 5, 10, 20, 40, 70}) {
        boss.playerLevel = level;
        const Rarity maximum = level >= 70 ? Rarity::Artifact : level >= 40 ? Rarity::Legendary :
            level >= 20 ? Rarity::Epic : level >= 10 ? Rarity::Rare :
            level >= 5 ? Rarity::Uncommon : Rarity::Common;
        for (unsigned seed = 1; seed <= 1000; ++seed)
            for (const auto& item : GenerateLoot(boss, seed))
                if (item.category == ItemCategory::Equippable)
                    Check(item.rarity <= maximum, "Boss rarity gate");
    }
    for (unsigned seed = 1; seed <= 1000; ++seed)
        for (const auto& item : GenerateInvestigationLoot(ConsumableProfile::SupplyCache,
                100, "Storage Room", seed, true))
            Check(item.rarity <= Rarity::Epic, "Investigation rarity gate");
    std::cout << "Current-source loot rates and rarity gates passed.\n";
}

void CheckPity() {
    MapManager map;
    Area& room = map.GetRooms().at(1);
    Player player;
    ProgressionManager progression;
    EncounterManager encounters;
    LootContext context;
    context.playerLevel = 1;
    context.enemyTier = EnemyTier::Basic;
    unsigned emptySeed = 0;
    for (unsigned seed = 1; seed < 10000; ++seed)
        if (GearCount(GenerateLoot(context, seed)) == 0) { emptySeed = seed; break; }
    Check(emptySeed != 0, "No empty Basic loot seed");
    for (int victory = 1; victory <= 3; ++victory) {
        auto enemy = CreateCatalogueEnemy("Devotee Acolyte", 1);
        Check(enemy != nullptr, "Missing Basic enemy");
        enemy->takeDamage(1000000);
        const auto result = encounters.ResolveVictory(room, player, progression, map, *enemy, emptySeed);
        Check(result.resolved, "Basic victory rejected");
        Check(GearCount(result.items) == (victory == 3 ? 1 : 0), "Basic pity gear sequence");
        Check(encounters.GetBasicGearMisses() == (victory == 3 ? 0 : victory), "Basic pity counter");
        Check(!encounters.ResolveVictory(room, player, progression, map, *enemy, emptySeed).resolved,
            "Duplicate loot claim");
    }
    auto living = CreateCatalogueEnemy("Devotee Acolyte", 1);
    Check(!encounters.ResolveVictory(room, player, progression, map, *living, emptySeed).resolved,
        "Living enemy rewarded");
    std::cout << "Current-source Basic pity and reward ownership passed.\n";
}

void CheckRooms() {
    MapManager map;
    const auto& definitions = MapManager::GetFloor1Definitions();
    Check(definitions.size() == 5 && map.GetRooms().size() == 5, "Floor I room count");
    const int levels[] = {0, 8, 15, 22, 30};
    const int xp[] = {0, 200, 400, 800, 1600};
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const auto& definition = definitions[index];
        Check(map.GetRooms()[index].GetSpawnCondition() != EnemySpawnCondition::None,
            "Room ordinary spawn disabled");
        if (!index) continue;
        Check(definition.firstClearBossLevel == levels[index] &&
            definition.firstClearBossXP == xp[index] &&
            definition.firstBossSpawnChance == 50 && definition.farmBossStartingChance == 8,
            "Fixed boss configuration");
        const auto boss = CreateFirstClearBoss(definition);
        Check(boss && boss->getLevel() == levels[index] &&
            boss->getXPReward() == xp[index] && boss->IsFirstClearBoss(), "Fixed boss runtime");
    }
    Area& storage = map.GetRooms().at(4);
    storage.SetCleared(true);
    Player player;
    ProgressionManager progression;
    EncounterManager encounters;
    int ordinary = 0, farmBosses = 0;
    for (unsigned seed = 1; seed <= 2000; ++seed) {
        auto result = encounters.ResolveInvestigation(storage, player, progression, seed);
        if (!result.spawnedEnemy) continue;
        if (result.spawnedEnemy->IsFarmBoss()) ++farmBosses;
        else ++ordinary;
    }
    Check(ordinary > 400 && farmBosses > 20, "Storage farming encounters missing");

    // Resolve a fixed first-clear boss once, then reject a duplicate claim.
    MapManager firstClearMap;
    Area& entrance = firstClearMap.GetRooms().at(1);
    Area& archive = firstClearMap.GetRooms().at(2);
    Player firstClearPlayer;
    ProgressionManager firstClearProgression;
    EncounterManager firstClearEncounters;
    auto guardian = CreateFirstClearBoss(definitions[1]);
    guardian->takeDamage(1000000);
    const auto first = firstClearEncounters.ResolveVictory(entrance, firstClearPlayer,
        firstClearProgression, firstClearMap, *guardian, 77);
    Check(first.resolved && first.firstClear && entrance.HasBossBeenDefeated() && archive.IsUnlocked(),
        "First-clear boss/unlock failed");
    const auto xpAfterClear = firstClearProgression.GetXP();
    Check(!firstClearEncounters.ResolveVictory(entrance, firstClearPlayer,
        firstClearProgression, firstClearMap, *guardian, 77).resolved &&
        firstClearProgression.GetXP() == xpAfterClear, "Duplicate first-clear reward");
    MapManager farmMap;
    Area& farmEntrance = farmMap.GetRooms().at(1);
    Player farmPlayer;
    ProgressionManager farmProgression;
    EncounterManager farmEncounters;
    const auto* bossDefinition = FindEnemyDefinition(definitions[1].bossId);
    Check(bossDefinition != nullptr, "Farm boss fixture missing");
    Enemy farmBoss(*bossDefinition, 8, false, true);
    farmBoss.takeDamage(1000000);
    const auto farmResult = farmEncounters.ResolveVictory(farmEntrance, farmPlayer,
        farmProgression, farmMap, farmBoss, 77);
    Check(farmResult.resolved && !farmResult.firstClear && !farmEntrance.HasBossBeenDefeated() &&
        !farmMap.GetRooms().at(2).IsUnlocked(), "Farm boss replayed first-clear unlock");
    MapManager defeatMap;
    Area& defeatEntrance = defeatMap.GetRooms().at(1);
    Player defeatedPlayer;
    defeatedPlayer.takeDamage(1000000);
    ProgressionManager defeatProgression;
    EncounterManager defeatEncounters;
    auto defeatedBoss = CreateFirstClearBoss(definitions[1]);
    defeatedBoss->takeDamage(1000000);
    Check(!defeatEncounters.ResolveVictory(defeatEntrance, defeatedPlayer,
        defeatProgression, defeatMap, *defeatedBoss, 77).resolved &&
        !defeatEntrance.HasBossBeenDefeated(), "Defeat cleared boss");
    MapManager fleeMap;
    auto fledBoss = CreateFirstClearBoss(definitions[1]);
    fledBoss->ForfeitRewards();
    fledBoss->takeDamage(1000000);
    Player fleePlayer;
    ProgressionManager fleeProgression;
    EncounterManager fleeEncounters;
    Check(!fleeEncounters.ResolveVictory(fleeMap.GetRooms().at(1), fleePlayer,
        fleeProgression, fleeMap, *fledBoss, 77).resolved &&
        !fleeMap.GetRooms().at(1).HasBossBeenDefeated(), "Fled boss counted as clear");
    std::cout << "Current-source room/boss profiles and Storage farming passed.\n";
}

void CheckStatusesAndConsumables() {
    const auto consumables = BuildConsumableCatalog();
    Check(consumables.size() == 12, "Consumable catalogue size");
    for (const auto& item : consumables)
        Check(FindConsumableEffect(item.effect) != nullptr, "Unmapped consumable effect");
    const std::vector<StatusEffect> bleed{{StatusType::Bleed, 2, 0, 100}};
    const std::vector<StatusEffect> burn{{StatusType::Burn, 2, 0, 100}};
    const std::vector<StatusEffect> poison{{StatusType::Poison, 2, 0, 100}};
    Check(StatusRules::DotDamage(bleed.front(), 1000, bleed) == 30, "Bleed baseline");
    Check(StatusRules::DotDamage(burn.front(), 1000, burn) == 60, "Burn baseline");
    Check(StatusRules::DotDamage(poison.front(), 1000, poison) == 50, "Poison baseline");
    const std::vector<StatusEffect> marked{{StatusType::Bleed, 2, 0, 100, 0, 0, "Adelaide", 1},
        {StatusType::VerdigrisMark, 3, 20, 0, 0, 0, "Adelaide", 1}};
    Check(StatusRules::DotDamage(marked.front(), 1000, marked) == 36, "Same-source Mark bonus");
    const auto give = [&](Player& player, const char* name) {
        const auto found = std::find_if(consumables.begin(), consumables.end(), [=](const ItemDefinition& item) {
            return item.name == name;
        });
        Check(found != consumables.end(), "Missing consumable fixture");
        auto item = std::make_unique<Item>(*found);
        const auto id = item->getInstanceId();
        player.AddItem(std::move(item));
        return id;
    };
    Player player;
    MapManager map;
    Area& room = map.GetRooms().front();
    const auto healing = give(player, "cellar_rations");
    Check(!player.UseConsumable(healing, nullptr, nullptr, &room) &&
        player.GetInventory().GetQuantity(healing) == 1, "Full-health item consumed");
    player.takeDamage(40);
    Check(player.UseConsumable(healing, nullptr, nullptr, &room) &&
        player.GetInventory().GetQuantity(healing) == 0, "Healing quantity not consumed once");
    const auto accuracy = give(player, "mechanical_charge");
    Check(!player.UseConsumable(accuracy, nullptr, nullptr, &room) &&
        player.GetInventory().GetQuantity(accuracy) == 1, "Combat-only item used in room");
    Player fallen;
    fallen.takeDamage(1000000);
    fallen.heal(1000000);
    Check(!fallen.isAlive() && fallen.getHP() == 0, "Healing revived a dead player");
    auto livingEnemy = CreateCatalogueEnemy("Devotee Acolyte", 1);
    Check(livingEnemy != nullptr, "Dead actor fixture missing");
    fallen.RestoreHealth(0);
    CombatManager deadCombat(77);
    Check(!deadCombat.resolve(CombatAction::Attack, fallen, *livingEnemy) &&
        livingEnemy->getHP() == livingEnemy->getMaxHP(), "Dead player acted");
    std::cout << "Current-source consumable mapping and DOT math passed.\n";
}

void CheckMixedLegendarySets() {
    Player player;
    const auto add = [&](const char* name, const char* set, EquipSlot slot) -> Item& {
        ItemDefinition definition;
        definition.name = name;
        definition.category = ItemCategory::Equippable;
        definition.rarity = Rarity::Legendary;
        definition.slot = slot;
        definition.setName = set;
        auto item = std::make_unique<Item>(definition);
        Item& reference = *item;
        player.AddItem(std::move(item));
        return reference;
    };
    Item& head = add("Mixed head", "Shimmer Ascendant", EquipSlot::Head);
    Item& torso = add("Mixed torso", "Shimmer Ascendant", EquipSlot::Torso);
    Item& boots = add("Mixed boots", "Shimmer Ascendant", EquipSlot::Boots);
    Item& hand = add("Mixed hand", "Choir of Verdigris", EquipSlot::Hand);
    Item& accessory = add("Mixed accessory", "Choir of Verdigris", EquipSlot::Accessory);
    for (Item* item : {&head, &torso, &boots, &hand}) Check(player.Equip(*item), "Mixed equip failed");
    const Stats preview = player.PreviewEquipment(accessory);
    Check(player.Equip(accessory), "Final mixed equip failed");
    const Stats actual = player.GetEffectiveStats();
    Check(preview.hp == actual.hp && preview.atk == actual.atk &&
        preview.healingReceived == actual.healingReceived && preview.dotPotency == actual.dotPotency,
        "Mixed preview differs from equipped stats");
    Check(actual.hp == 135 && actual.atk == 26 && actual.healingReceived == 25 &&
        actual.dotPotency == 25 && player.GetShimmerDamageMultiplierPercent() == 135 &&
        player.GetActiveSets().size() == 2, "Legendary 3+2 bonuses incorrect");
    Check(player.ToggleEquip(accessory), "Mixed unequip failed");
    Check(player.getMaxHP() == 100 && player.getAttack() == 26 &&
        player.GetActiveSets().size() == 1, "Legendary unequip drift");
    Check(player.Equip(accessory) && player.getMaxHP() == 135,
        "Legendary re-equip drift");
    std::cout << "Current-source Legendary 3+2 preview/recalculation passed.\n";
}
}

void RunCompatibilityChecks() {
    CheckLoot();
    CheckPity();
    CheckRooms();
    CheckStatusesAndConsumables();
    CheckMixedLegendarySets();
}
