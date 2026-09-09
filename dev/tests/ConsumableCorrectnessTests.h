#pragma once

#include "Area.h"
#include "CombatManager.h"
#include "ConsumableEffects.h"
#include "ContentCatalog.h"
#include "Enemy.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace ConsumableValidation {

inline ItemDefinition Definition(const std::string& name) {
    for (const auto& definition : BuildConsumableCatalog()) if (definition.name == name) return definition;
    assert(false && "Missing catalogue consumable");
    return {};
}

inline unsigned long long Give(Player& player, const std::string& name, int quantity = 1) {
    auto item = std::make_unique<Item>(Definition(name));
    const auto id = item->getInstanceId();
    player.GetInventory().AddItem(std::move(item), quantity);
    return id;
}

inline EnemyDefinition TargetDefinition() {
    EnemyDefinition definition;
    definition.name = "Consumable validation target";
    definition.hpBase = 10000;
    definition.atkBase = 1;
    definition.speed = 40;
    return definition;
}

struct Battle {
    Player player;
    Enemy enemy{TargetDefinition(), 1};
    CombatManager combat;
    ProgressionManager progression;
    Battle() {
        combat.BeginEncounter(player, enemy, 1);
        combat.PrepareTurn(player, enemy);
        assert(combat.IsPlayerTurn());
    }
    void NextPlayerTurn() {
        for (int actions = 0; actions < 10 && !combat.IsPlayerTurn() && player.isAlive(); ++actions)
            combat.enemyTurn(player, enemy, &progression);
        assert(player.isAlive() && combat.IsPlayerTurn());
    }
};

inline void TestRestorationAndCleansing() {
    for (const auto& sample : {std::pair<const char*, int>{"cellar_rations", 20}, {"blessed_bandages", 35}, {"lumen_draught", 50}}) {
        Player player;
        const auto id = Give(player, sample.first, 2);
        assert(!player.UseConsumable(id)); // Full health, no status to cleanse.
        assert(player.GetInventory().GetQuantity(id) == 2);
        player.takeDamage(70);
        assert(player.UseConsumable(id));
        assert(player.getHP() == 30 + sample.second);
        assert(player.GetInventory().GetQuantity(id) == 1);
    }
    Player player;
    const auto salts = Give(player, "purification_salts");
    for (StatusType type : {StatusType::Bleed, StatusType::Burn, StatusType::Poison, StatusType::Slow,
        StatusType::Stun, StatusType::AttackDown, StatusType::DefenseDown, StatusType::AccuracyDown,
        StatusType::VerdigrisMark, StatusType::HealResistance}) assert(player.ApplyStatus({type, 3, 20, 10}));
    assert(player.ApplyStatus({StatusType::AttackUp, 3, 20}));
    assert(player.ApplyStatus({StatusType::DotReduction, 3, 20}));
    player.AddBarrier(30, 3);
    assert(player.UseConsumable(salts));
    for (const auto& status : player.GetStatuses()) assert(!IsNegativeConsumableStatus(status.type));
    assert(player.HasStatus(StatusType::AttackUp) && player.HasStatus(StatusType::DotReduction));
    assert(player.HasStatus(StatusType::Barrier) && player.GetBarrier() == 30);

    const auto water = Give(player, "sanctum_water");
    const auto bandages = Give(player, "blessed_bandages");
    assert(player.ApplyStatus({StatusType::Bleed, 3, 0, 10}));
    assert(player.ApplyStatus({StatusType::Burn, 3, 0, 10}));
    assert(player.ApplyStatus({StatusType::Poison, 3, 0, 10}));
    assert(player.UseConsumable(water));
    assert(!player.HasStatus(StatusType::Burn) && !player.HasStatus(StatusType::Poison));
    assert(player.HasStatus(StatusType::Bleed) && player.HasStatus(StatusType::AttackUp));
    assert(player.UseConsumable(bandages)); // Full-HP multi-effect use still cleanses Bleed.
    assert(!player.HasStatus(StatusType::Bleed) && player.getHP() == player.getMaxHP());

    player.ClearStatuses();
    player.takeDamage(50);
    assert(player.ApplyStatus({StatusType::HealResistance, 3, 100}));
    const auto rations = Give(player, "cellar_rations");
    assert(!player.UseConsumable(rations)); // Healing would produce zero actual recovery.
    assert(player.GetInventory().Contains(rations) && player.getHP() == 50);
}

inline void TestBuffsAndCooldowns() {
    for (const auto& sample : {std::pair<const char*, StatusType>{"choir_incense", StatusType::AccuracyUp}, {"verdigris_tincture", StatusType::DefenseUp}}) {
        Battle battle;
        const auto id = Give(battle.player, sample.first, 2);
        assert(!battle.player.UseConsumable(id)); // Temporary buffs need a battle context.
        assert(battle.combat.UseConsumable(id, battle.player, battle.enemy));
        assert(battle.player.HasStatus(sample.second));
        const auto& application = battle.player.GetStatusApplications().back();
        assert(application.applied && application.status.turns == 3);
        assert(application.status.potency == (sample.second == StatusType::AccuracyUp ? 10 : 15));
        assert(battle.player.GetInventory().GetQuantity(id) == 1);
        assert(!battle.combat.UseConsumable(id, battle.player, battle.enemy));
        assert(battle.player.GetInventory().GetQuantity(id) == 1);
    }
    Battle battle;
    const auto vial = Give(battle.player, "shimmer_vial", 3);
    assert(!battle.combat.UseConsumable(vial, battle.player, battle.enemy));
    assert(battle.combat.IsPlayerTurn() && battle.player.GetInventory().GetQuantity(vial) == 3);
    auto skill = ShimmerSkill::Create("Prism Step");
    assert(skill && battle.combat.resolve(CombatAction::Shimmer, battle.player, battle.enemy, skill.get()));
    battle.NextPlayerTurn();
    const int before = battle.combat.GetSkillCooldown(skill->getName());
    assert(before > 0 && battle.combat.HasActiveShimmerCooldowns());
    assert(battle.combat.UseConsumable(vial, battle.player, battle.enemy));
    assert(battle.combat.GetSkillCooldown(skill->getName()) == before - 1);
    assert(battle.player.GetInventory().GetQuantity(vial) == 2);
    battle.combat.ReduceShimmerCooldowns(100);
    assert(!battle.combat.HasActiveShimmerCooldowns());
    battle.NextPlayerTurn();
    assert(!battle.combat.UseConsumable(vial, battle.player, battle.enemy));
    assert(battle.combat.IsPlayerTurn() && battle.player.GetInventory().GetQuantity(vial) == 2);
}

inline void TestDamageAndStatusEvents() {
    Battle battle;
    const auto charge = Give(battle.player, "mechanical_charge");
    assert(battle.enemy.ApplyStatus({StatusType::Barrier, 3, 50}));
    const int before = battle.enemy.getHP();
    assert(battle.combat.UseConsumable(charge, battle.player, battle.enemy));
    assert(battle.enemy.getHP() == before);
    bool damage = false, broken = false;
    for (const auto& event : battle.combat.GetEvents()) {
        if (event.kind == CombatManager::CombatEvent::Kind::Damage && event.skill == "mechanical_charge") {
            assert(event.requestedDamage == 50 && event.barrierAbsorbed == 50);
            assert(event.hpDamage == 0 && event.remainingHP == before);
            damage = true;
        }
        broken |= event.kind == CombatManager::CombatEvent::Kind::BarrierBreak;
    }
    assert(damage && broken);

    Battle burner;
    Stats potency;
    potency.dotPotency = 35;
    burner.player.ModifyStats(potency);
    const auto powder = Give(burner.player, "ritual_powder");
    assert(burner.combat.UseConsumable(powder, burner.player, burner.enemy));
    const auto& applied = burner.enemy.GetStatusApplications().back();
    assert(applied.applied && applied.status.type == StatusType::Burn && applied.status.turns == 3);
    assert(applied.status.sourceAttack == burner.player.getAttack());
    assert(applied.status.dotPotency == 35 && applied.status.source == burner.player.getName());
    burner.enemy.ProcessStatuses();
    assert(!burner.enemy.GetStatusDamageResults().empty());
    assert(burner.enemy.GetStatusDamageResults().front().damage.hpDamage == burner.player.getAttack() * 60 / 100 * 135 / 100);

    Battle cleanse;
    const auto salts = Give(cleanse.player, "purification_salts");
    assert(cleanse.player.ApplyStatus({StatusType::Bleed, 3, 0, 10}));
    assert(cleanse.combat.UseConsumable(salts, cleanse.player, cleanse.enemy));
    bool cleansed = false;
    for (const auto& event : cleanse.combat.GetEvents())
        if (event.kind == CombatManager::CombatEvent::Kind::Cleanse) { assert(event.value == 1); cleansed = true; }
    assert(cleansed && !cleanse.player.HasStatus(StatusType::Bleed));
}

inline void TestOwnershipAndInvalidUses() {
    Player player;
    const auto locked = Give(player, "cellar_rations", 3);
    const auto usable = Give(player, "cellar_rations", 2);
    assert(player.GetInventory().SetLocked(locked, true));
    player.takeDamage(60);
    assert(!player.UseConsumable(locked));
    assert(player.getHP() == 40 && player.GetInventory().GetQuantity(locked) == 3);
    assert(player.UseConsumable("cellar_rations"));
    assert(player.getHP() == 60 && player.GetInventory().GetQuantity(usable) == 1);
    assert(player.GetInventory().GetQuantity(locked) == 3);
    assert(player.UseConsumable(usable));
    assert(!player.GetInventory().Contains(usable) && player.getHP() == 80);
    assert(!player.UseConsumable(usable));

    Battle battle;
    const auto charge = Give(battle.player, "mechanical_charge");
    assert(!battle.player.UseConsumable(charge, &battle.enemy));
    assert(!battle.player.UseConsumable(charge, nullptr, &battle.combat));
    Enemy unrelated(TargetDefinition(), 1);
    assert(!battle.player.UseConsumable(charge, &unrelated, &battle.combat));
    auto unknown = Definition("cellar_rations");
    unknown.name = "Invalid effect fixture";
    unknown.effect = "restore_not_a_number_percent_hp";
    auto item = std::make_unique<Item>(unknown);
    const auto unknownId = item->getInstanceId();
    battle.player.AddItem(std::move(item));
    const auto health = battle.player.getHP();
    const auto enemyHealth = battle.enemy.getHP();
    assert(!battle.combat.UseConsumable(unknownId, battle.player, battle.enemy));
    assert(battle.combat.IsPlayerTurn() && battle.player.getHP() == health && battle.enemy.getHP() == enemyHealth);
    assert(battle.player.GetInventory().Contains(unknownId));
    battle.enemy.takeDamage(battle.enemy.getHP());
    assert(!battle.combat.UseConsumable(charge, battle.player, battle.enemy));
    assert(battle.player.GetInventory().Contains(charge));
    const auto healing = Give(battle.player, "lumen_draught");
    battle.player.takeDamage(battle.player.getHP());
    assert(!battle.player.UseConsumable(healing));
    assert(!battle.player.isAlive() && battle.player.GetInventory().Contains(healing));
}

inline void TestExplorationContexts() {
    MapManager map;
    Area& archive = map.GetRooms()[2];
    Player player;
    const auto capsule = Give(player, "shimmer_capsule", 2);
    const auto oil = Give(player, "verdigris_oil", 2);
    assert(!player.UseConsumable(capsule));
    assert(!player.UseConsumable(oil));
    assert(!player.UseConsumable(capsule, nullptr, nullptr, &map.GetRooms()[0]));
    assert(!player.UseConsumable(oil, nullptr, nullptr, &map.GetRooms()[1]));
    assert(archive.CanBeIlluminated() && !archive.IsIlluminated() && archive.HasRustedLock());
    const auto visibleBefore = archive.GetVisibleInteractables().size();
    const auto searchesBefore = archive.GetSearchCount();
    const auto failuresBefore = archive.GetDiscoveryFailures();
    assert(player.UseConsumable(capsule, nullptr, nullptr, &archive));
    assert(archive.IsIlluminated());
    assert(!player.UseConsumable(capsule, nullptr, nullptr, &archive));
    assert(player.GetInventory().GetQuantity(capsule) == 1);
    assert(player.UseConsumable(oil, nullptr, nullptr, &archive));
    assert(!archive.HasRustedLock());
    assert(!player.UseConsumable(oil, nullptr, nullptr, &archive));
    assert(player.GetInventory().GetQuantity(oil) == 1);
    assert(archive.GetVisibleInteractables().size() == visibleBefore);
    assert(archive.GetSearchCount() == searchesBefore && archive.GetDiscoveryFailures() == failuresBefore);

    Area invented("Archive Room"); // Matching room name is not an authored lock.
    assert(!player.UseConsumable(oil, nullptr, nullptr, &invented));
    Battle battle;
    const auto combatCapsule = Give(battle.player, "shimmer_capsule");
    assert(!battle.player.UseConsumable(combatCapsule, &battle.enemy, &battle.combat, &archive));
    assert(battle.combat.IsPlayerTurn() && battle.player.GetInventory().Contains(combatCapsule));
}

} // namespace ConsumableValidation

inline void RunConsumableCorrectnessTests() {
    const auto catalogue = BuildConsumableCatalog();
    std::string error;
    assert(catalogue.size() == 12 && ValidateConsumableEffects(catalogue, &error));
    for (const auto& item : catalogue) assert(FindConsumableEffect(item.effect) != nullptr);
    auto invalid = catalogue.front();
    invalid.effect += "_unexpected_suffix";
    assert(!ValidateConsumableEffects({invalid}, &error) && !error.empty());
    ConsumableValidation::TestRestorationAndCleansing();
    ConsumableValidation::TestBuffsAndCooldowns();
    ConsumableValidation::TestDamageAndStatusEvents();
    ConsumableValidation::TestOwnershipAndInvalidUses();
    ConsumableValidation::TestExplorationContexts();
    std::cout << "Consumable tests passed: all 12 catalogue effects, exact-instance ownership, contexts, invalid-use atomicity and actual combat events.\n";
}
