#pragma once

#include "CombatManager.h"
#include "EquipmentStats.h"
#include "Enemy.h"
#include "Player.h"
#include "ShimmerSkill.h"
#include <cassert>
#include <climits>
#include <iostream>
#include <memory>

namespace LegendarySetValidation {
inline Item& AddPiece(Player& player, const std::string& setName, EquipSlot slot,
                     Stats stats = {}, const std::string& name = "Duplicate set-piece name") {
    ItemDefinition definition;
    definition.name = name;
    definition.category = ItemCategory::Equippable;
    definition.rarity = Rarity::Legendary;
    definition.slot = slot;
    definition.setName = setName;
    definition.stats = stats;
    auto item = std::make_unique<Item>(definition);
    Item& result = *item;
    player.AddItem(std::move(item));
    return result;
}

inline void EqualStats(const Stats& expected, const Stats& actual) {
    for (auto field : EquipmentStats::Fields()) assert(expected.*field == actual.*field);
}

inline EnemyDefinition Target() {
    EnemyDefinition definition;
    definition.name = "Legendary combat target";
    definition.hpBase = 100000;
    definition.atkBase = 1;
    definition.defBase = 1;
    definition.speed = 1;
    return definition;
}

inline void Choir() {
    Player player;
    Stats ordinary;
    ordinary.hp = 20; ordinary.hpPercent = 25;
    ordinary.def = 3; ordinary.defPercent = 25;
    Item& head = AddPiece(player, "Choir of Verdigris", EquipSlot::Head, ordinary);
    Item& torso = AddPiece(player, "Choir of Verdigris", EquipSlot::Torso);
    Item& boots = AddPiece(player, "Choir of Verdigris", EquipSlot::Boots);
    assert(player.Equip(head));
    assert(player.getMaxHP() == 150 && player.getDefense() == 16);
    assert(!player.HasActiveSet("Choir of Verdigris"));
    assert(player.GetEffectiveStats().healingReceived == 0 && player.getBarrierPotency() == 0);

    const Stats twoPiecePreview = player.PreviewEquipment(torso);
    assert(twoPiecePreview.hp == 203 && twoPiecePreview.def == 16);
    assert(twoPiecePreview.healingReceived == 25 && twoPiecePreview.barrierPotency == 0);
    assert(player.Equip(torso));
    EqualStats(twoPiecePreview, player.GetEffectiveStats());
    assert(player.HasActiveSet("Choir of Verdigris") && player.GetSetPieceCount("Choir of Verdigris") == 2);
    assert(player.GetActiveSets().size() == 1);

    const Stats threePiecePreview = player.PreviewEquipment(boots);
    assert(threePiecePreview.hp == 203 && threePiecePreview.def == 21);
    assert(threePiecePreview.healingReceived == 25 && threePiecePreview.barrierPotency == 30);
    assert(player.Equip(boots));
    EqualStats(threePiecePreview, player.GetEffectiveStats());
    player.RestoreHealth(1);
    player.heal(40);
    assert(player.getHP() == 51 && player.GetLastHealResult().hpRestored == 50);
    player.AddBarrier(80);
    assert(player.GetBarrier() == 104);
    player.ClearStatuses();
    const auto damage = player.ApplyDirectDamage(1000);
    assert(damage.mitigatedDamage == 21); // Actual combat uses final DEF 21.
    player.RestoreHealth(player.getMaxHP());

    const Stats removalPreview = player.PreviewUnequip(EquipSlot::Boots);
    assert(player.ToggleEquip(boots));
    EqualStats(removalPreview, player.GetEffectiveStats());
    EqualStats(twoPiecePreview, player.GetEffectiveStats());
    assert(player.ToggleEquip(torso));
    assert(player.getMaxHP() == 150 && player.getDefense() == 16);
    assert(!player.HasActiveSet("Choir of Verdigris"));
    assert(player.GetEffectiveStats().healingReceived == 0);

    // Four and five pieces retain the approved thresholds and do not grant
    // the removed five-piece opening barrier or stack the bonuses again.
    assert(player.Equip(torso) && player.Equip(boots));
    assert(player.Equip(AddPiece(player, "Choir of Verdigris", EquipSlot::Hand)));
    assert(player.Equip(AddPiece(player, "Choir of Verdigris", EquipSlot::Accessory)));
    EqualStats(threePiecePreview, player.GetEffectiveStats());
    player.StartBattleEffects();
    assert(player.GetBarrier() == 0 && player.GetSetPieceCount("Choir of Verdigris") == 5);
}

inline void Ascendant() {
    Player player;
    Stats ordinary;
    ordinary.atk = 10; ordinary.atkPercent = 25;
    Item& head = AddPiece(player, "Shimmer Ascendant", EquipSlot::Head, ordinary);
    Item& torso = AddPiece(player, "Shimmer Ascendant", EquipSlot::Torso);
    Item& boots = AddPiece(player, "Shimmer Ascendant", EquipSlot::Boots);
    assert(player.Equip(head));
    assert(player.getAttack() == 38 && player.GetShimmerDamageMultiplierPercent() == 100);
    assert(player.Equip(torso));
    assert(player.getAttack() == 49 && player.GetShimmerDamageMultiplierPercent() == 100);
    assert(player.getDotPotency() == 0 && player.getAccuracy() == 0);
    auto strike = ShimmerSkill::Create("Pulsus Luminis");
    Enemy twoPieceTarget(Target(), 1);
    strike->apply(player, twoPieceTarget);
    assert(twoPieceTarget.GetLastDamageResult().requestedDamage == 441);

    const Stats preview = player.PreviewEquipment(boots);
    assert(player.Equip(boots));
    EqualStats(preview, player.GetEffectiveStats());
    assert(player.getAttack() == 49 && player.getDotPotency() == 25);
    assert(player.GetShimmerDamageMultiplierPercent() == 135);
    Enemy threePieceTarget(Target(), 1);
    strike->apply(player, threePieceTarget);
    assert(threePieceTarget.GetLastDamageResult().requestedDamage == 595);

    // The direct bonus is applied once. DOT receives only its separate 25pp
    // potency and snapshots the same equipped attack as other status sources.
    auto burn = ShimmerSkill::Create("Flos Aeruginis");
    Enemy burnTarget(Target(), 1);
    burn->apply(player, burnTarget);
    assert(burnTarget.GetLastDamageResult().requestedDamage == 992);
    assert(burnTarget.GetStatuses().size() == 1);
    assert(burnTarget.GetStatuses()[0].sourceAttack == 49);
    assert(burnTarget.GetStatuses()[0].dotPotency == 75);
    const int beforeTick = burnTarget.getHP();
    burnTarget.ProcessStatuses();
    assert(beforeTick - burnTarget.getHP() == 50);

    // The console uses CombatManager::resolve: an offensive Shimmer must not
    // inherit the obsolete five-piece heal at either threshold or at five.
    for (int pieces : {3, 5}) {
        if (pieces == 5) {
            assert(player.Equip(AddPiece(player, "Shimmer Ascendant", EquipSlot::Hand)));
            assert(player.Equip(AddPiece(player, "Shimmer Ascendant", EquipSlot::Accessory)));
        }
        player.RestoreHealth(40);
        Enemy enemy(Target(), 1);
        CombatManager combat(71);
        combat.PrepareTurn(player, enemy);
        assert(combat.IsPlayerTurn());
        assert(combat.resolve(CombatAction::Shimmer, player, enemy, strike.get()));
        assert(player.getHP() == 40 && enemy.GetLastDamageResult().requestedDamage == 595);
        assert(player.getAttack() == 49 && player.getAccuracy() == 0);
    }
    assert(player.ToggleEquip(*const_cast<Item*>(player.GetEquipped(EquipSlot::Hand))));
    assert(player.ToggleEquip(*const_cast<Item*>(player.GetEquipped(EquipSlot::Accessory))));
    assert(player.ToggleEquip(boots));
    assert(player.getDotPotency() == 0 && player.GetShimmerDamageMultiplierPercent() == 100);
    assert(player.ToggleEquip(torso));
    assert(player.getAttack() == 38 && !player.HasActiveSet("Shimmer Ascendant"));
}

inline void Overlord() {
    Player player;
    Stats ordinary;
    ordinary.spd = 10; ordinary.spdPercent = 25;
    ordinary.critRate = 10; ordinary.critDamage = 30;
    Item& head = AddPiece(player, "Automaton Overlord", EquipSlot::Head, ordinary);
    Item& torso = AddPiece(player, "Automaton Overlord", EquipSlot::Torso);
    Item& boots = AddPiece(player, "Automaton Overlord", EquipSlot::Boots);
    assert(player.Equip(head));
    assert(player.getSpeed() == 138 && player.getCritRate() == 15 && player.getCritDamage() == 180);
    assert(player.Equip(torso));
    assert(player.getSpeed() == 166 && player.getCritRate() == 15 && player.getCritDamage() == 180);
    const Stats preview = player.PreviewEquipment(boots);
    assert(player.Equip(boots));
    EqualStats(preview, player.GetEffectiveStats());
    assert(player.getSpeed() == 166 && player.getDisplayedCritRate() == 35 && player.getCritDamage() == 230);
    assert(player.getAccuracy() == 0); // No legacy perfect-accuracy bonus.

    player.ApplyStatus({StatusType::GuaranteedCrit, 2, 0});
    Enemy enemy(Target(), 1);
    CombatManager combat(39);
    combat.PrepareTurn(player, enemy);
    assert(combat.IsPlayerTurn());
    assert(combat.resolve(CombatAction::Attack, player, enemy));
    assert(enemy.GetLastDamageResult().requestedDamage == 46); // 20 ATK * 230%.
    player.ClearStatuses();
    const Stats removalPreview = player.PreviewUnequip(EquipSlot::Boots);
    assert(player.ToggleEquip(boots));
    EqualStats(removalPreview, player.GetEffectiveStats());
    assert(player.getSpeed() == 166 && player.getCritRate() == 15 && player.getCritDamage() == 180);
    assert(player.ToggleEquip(torso));
    assert(player.getSpeed() == 138 && !player.HasActiveSet("Automaton Overlord"));
}

inline void IdentityAndMixtures() {
    Player player;
    Item& first = AddPiece(player, "Choir of Verdigris", EquipSlot::Head);
    Item& sameSlot = AddPiece(player, "Choir of Verdigris", EquipSlot::Head);
    assert(first.getName() == sameSlot.getName() && first.getInstanceId() != sameSlot.getInstanceId());
    assert(player.Equip(first) && player.Equip(sameSlot));
    assert(player.GetSetPieceCount("Choir of Verdigris") == 1 && player.getMaxHP() == 100);
    assert(player.GetEquipped(EquipSlot::Head)->getInstanceId() == sameSlot.getInstanceId());
    Item& nearName = AddPiece(player, "Choir of Verdigris imitation", EquipSlot::Torso);
    assert(player.Equip(nearName));
    assert(player.GetSetPieceCount("Choir of Verdigris") == 1 && player.GetSetPieceCount("") == 0);
    Item& exact = AddPiece(player, "Choir of Verdigris", EquipSlot::Torso);
    assert(exact.getName() == first.getName() && exact.getInstanceId() != first.getInstanceId());
    assert(player.Equip(exact));
    assert(player.getMaxHP() == 135 && player.GetActiveSets().size() == 1);

    assert(player.Equip(AddPiece(player, "Automaton Overlord", EquipSlot::Boots)));
    assert(player.Equip(AddPiece(player, "Automaton Overlord", EquipSlot::Hand)));
    Item& ascendant = AddPiece(player, "Shimmer Ascendant", EquipSlot::Accessory);
    assert(player.Equip(ascendant));
    assert(player.GetActiveSets().size() == 2 && !player.HasActiveSet("Shimmer Ascendant"));
    assert(player.getMaxHP() == 135 && player.getSpeed() == 120 && player.getAttack() == 20);
    assert(player.getDefense() == 10 && player.getBarrierPotency() == 0 && player.getCritRate() == 5);
    Item& thirdChoir = AddPiece(player, "Choir of Verdigris", EquipSlot::Accessory);
    const Stats combined = player.PreviewEquipment(thirdChoir);
    assert(player.Equip(thirdChoir));
    EqualStats(combined, player.GetEffectiveStats());
    assert(player.getMaxHP() == 135 && player.getDefense() == 13 && player.getSpeed() == 120);
    assert(player.getBarrierPotency() == 30 && player.getCritRate() == 5);

    // Repeated exact-instance replacement cannot stack or leave stale bonuses.
    for (int iteration = 0; iteration < 100; ++iteration) {
        assert(player.Equip(ascendant));
        assert(player.getDefense() == 10 && player.getBarrierPotency() == 0);
        assert(player.Equip(thirdChoir));
        EqualStats(combined, player.GetEffectiveStats());
    }
}

inline void CapsAndOverflow() {
    Player choir;
    Stats excessive;
    excessive.hp = INT_MAX; excessive.def = INT_MAX;
    excessive.healingReceived = 140; excessive.barrierPotency = 190;
    assert(choir.Equip(AddPiece(choir, "Choir of Verdigris", EquipSlot::Head, excessive)));
    assert(choir.Equip(AddPiece(choir, "Choir of Verdigris", EquipSlot::Torso)));
    assert(choir.Equip(AddPiece(choir, "Choir of Verdigris", EquipSlot::Boots)));
    assert(choir.getMaxHP() == INT_MAX && choir.getDefense() == INT_MAX);
    assert(choir.GetEffectiveStats().healingReceived == 150 && choir.getBarrierPotency() == 200);
    choir.RestoreHealth(1); choir.heal(40);
    assert(choir.getHP() == 101);
    choir.AddBarrier(80);
    assert(choir.GetBarrier() == 240);

    Player ascendant;
    excessive = {}; excessive.atk = INT_MAX; excessive.dotPotency = 190;
    assert(ascendant.Equip(AddPiece(ascendant, "Shimmer Ascendant", EquipSlot::Head, excessive)));
    assert(ascendant.Equip(AddPiece(ascendant, "Shimmer Ascendant", EquipSlot::Torso)));
    assert(ascendant.Equip(AddPiece(ascendant, "Shimmer Ascendant", EquipSlot::Boots)));
    assert(ascendant.getAttack() == INT_MAX && ascendant.getDotPotency() == 200);

    Player overlord;
    excessive = {}; excessive.spd = INT_MAX; excessive.critRate = 90; excessive.critDamage = INT_MAX;
    assert(overlord.Equip(AddPiece(overlord, "Automaton Overlord", EquipSlot::Head, excessive)));
    assert(overlord.Equip(AddPiece(overlord, "Automaton Overlord", EquipSlot::Torso)));
    assert(overlord.Equip(AddPiece(overlord, "Automaton Overlord", EquipSlot::Boots)));
    assert(overlord.getSpeed() == INT_MAX && overlord.getDisplayedCritRate() == 100);
    assert(overlord.getCritDamage() == INT_MAX);
}
}

inline void RunLegendarySetTests() {
    LegendarySetValidation::Choir();
    LegendarySetValidation::Ascendant();
    LegendarySetValidation::Overlord();
    LegendarySetValidation::IdentityAndMixtures();
    LegendarySetValidation::CapsAndOverflow();
    std::cout << "Legendary sets passed: exact 2/3-piece thresholds, final-stat ordering, previews, independent instances, mixed sets, repeated replacement, combat effects and caps.\n";
}
