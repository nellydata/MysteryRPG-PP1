#pragma once
#include "Enemy.h"
#include "Player.h"
#include <cassert>
#include <climits>
#include <iostream>

namespace StatusResolutionValidation {
inline EnemyDefinition Definition() {
    EnemyDefinition definition;
    definition.name = "Status target";
    definition.hpBase = 10000;
    definition.defBase = 1000;
    definition.atkBase = 10;
    definition.speed = 50;
    return definition;
}
inline void Conservation(const DamageResolution& result) {
    assert(result.requestedDamage == result.mitigatedDamage + result.barrierAbsorbed +
        result.hpDamage + result.overkillDamage);
    assert(result.hpDamage >= 0 && result.remainingHP >= 0 && result.mitigatedDamage >= 0);
}
inline void DamageAndHealing() {
    Player growth;
    growth.SetLevelStats(2);
    assert(growth.getMaxHP() == 125 && growth.getHP() == 125 && growth.getAttack() == 35 && growth.getDefense() == 20);
    growth.SetLevelStats(INT_MAX);
    assert(growth.getMaxHP() == 2575 && growth.getAttack() == 1505);
    growth.SetLevelStats(INT_MIN);
    assert(growth.getHP() == 100 && growth.getMaxHP() == 100);
    growth.increaseMaxHP(INT_MAX); growth.increaseAttack(INT_MAX); growth.increaseDefense(INT_MAX);
    growth.gainSP(INT_MAX);
    assert(growth.getHP() == INT_MAX && growth.getMaxHP() == INT_MAX &&
        growth.getAttack() == INT_MAX && growth.getDefense() == INT_MAX && growth.getSP() == 10);
    growth.takeDamage(INT_MAX);
    growth.SetLevelStats(10); growth.increaseMaxHP(10); growth.heal(INT_MAX);
    assert(!growth.isAlive());
    auto extremeDefinition = Definition();
    extremeDefinition.speed = INT_MAX;
    Enemy extreme(extremeDefinition, 1);
    assert(extreme.getSpeed() == CombatMath::Balance::MaximumEnemySpeed);
    Player player;
    player.AddBarrier(40, 2);
    player.ApplyStatus({StatusType::DamageReduction, 2, 25});
    const auto damage = player.ApplyDirectDamage(100);
    Conservation(damage);
    assert(damage.requestedDamage == 100 && damage.mitigatedDamage > 25 && damage.barrierAbsorbed == 40);
    assert(damage.hpDamage == 100 - damage.mitigatedDamage - 40 && damage.barrierBroken);
    player.heal(INT_MAX);
    assert(player.getHP() == 100 && player.GetLastHealResult().overheal > 0);
    player.ClearStatuses();
    const auto lethal = player.ApplyDirectDamage(INT_MAX, true);
    Conservation(lethal);
    assert(lethal.hpDamage == 100 && lethal.overkillDamage == INT_MAX - 100);
    player.heal(INT_MAX);
    player.AddBarrier(INT_MAX);
    assert(!player.isAlive() && player.GetBarrier() == 0 && player.GetLastHealResult().hpRestored == 0);
    player.RestoreHealth(100); // Explicit Fountain recovery remains available.
    player.AddBarrier(INT_MAX);
    player.AddBarrier(INT_MAX);
    assert(player.GetBarrier() == INT_MAX);
    player.takeDamage(INT_MAX);
    Conservation(player.GetLastDamageResult());
    assert(player.getHP() == 100 && player.GetBarrier() == 0);

    Enemy enemy(Definition(), 1);
    enemy.ApplyStatus({StatusType::DamageReduction, 3, 25});
    const int before = enemy.getHP();
    const auto result = enemy.ApplyDirectDamage(1000);
    Conservation(result);
    assert(result.mitigatedDamage > 250 && enemy.getHP() == before - result.hpDamage);
    enemy.ApplyStatus({StatusType::HealResistance, 3, 100});
    enemy.heal(INT_MAX);
    assert(enemy.getHP() == before - result.hpDamage && enemy.GetLastHealResult().effectiveHealing == 0);
    enemy.ClearStatuses();
    enemy.takeDamage(INT_MAX);
    Conservation(enemy.GetLastDamageResult());
    enemy.heal(INT_MAX);
    assert(!enemy.isAlive() && enemy.GetLastHealResult().hpRestored == 0);
    assert(!enemy.ApplyStatus({StatusType::Barrier, 2, INT_MAX}));
    assert(CombatMath::Mitigate(0, INT_MAX) == 0 && CombatMath::Scale(INT_MAX, INT_MAX) == INT_MAX);
}
template<class Actor> inline void TimingAndSources(Actor& actor) {
    const int startHP = actor.getHP();
    assert(actor.ApplyStatus({StatusType::Barrier, 1, 40}));
    assert(actor.ApplyStatus({StatusType::Burn, 1, 0, 100, 0, 0, "Source", 101}));
    actor.BeginStatusTurn();
    assert(actor.getHP() == startHP - 20);
    assert(actor.GetStatusDamageResults().size() == 1);
    const auto tick = actor.GetStatusDamageResults().front();
    Conservation(tick.damage);
    assert(tick.damage.barrierAbsorbed == 40 && tick.damage.hpDamage == 20 && tick.sourceId == 101);
    assert(actor.HasStatus(StatusType::Burn)); // Final tick still belongs to this action.
    actor.BeginStatusTurn(); // Presentation/repeated preparation must not tick twice.
    assert(actor.getHP() == startHP - 20);
    actor.EndStatusTurn();
    assert(!actor.HasStatus(StatusType::Burn) && !actor.HasStatus(StatusType::Barrier));
    actor.BeginStatusTurn();
    actor.ApplyStatus({StatusType::AttackUp, 2, 40});
    actor.EndStatusTurn(); // A new self buff survives the cast action.
    assert(actor.HasStatus(StatusType::AttackUp) && actor.GetStatuses().front().turns == 2);
    actor.BeginStatusTurn(); actor.EndStatusTurn();
    assert(actor.HasStatus(StatusType::AttackUp));
    actor.BeginStatusTurn(); actor.EndStatusTurn();
    assert(!actor.HasStatus(StatusType::AttackUp));
    actor.ClearStatuses();
    const int before = actor.getHP();
    assert(actor.ApplyStatus({StatusType::VerdigrisMark, 99, 999, 0, 0, 0, "A", 101}));
    assert(actor.GetStatuses().front().turns == 3 && actor.GetStatuses().front().potency == 20);
    actor.ApplyStatus({StatusType::Burn, 1, 0, 100, 0, 0, "A", 101});
    actor.ApplyStatus({StatusType::Burn, 1, 0, 100, 0, 0, "B", 202});
    actor.ProcessStatuses();
    assert(before - actor.getHP() == 72 + 60);
    assert(actor.GetStatusDamageResults().size() == 2);
    actor.ClearStatuses();
    actor.ApplyStatus({StatusType::DotReduction, 3, 50});
    actor.ApplyStatus({StatusType::AttackUp, 3, 50});
    actor.ApplyStatus({StatusType::HealResistance, 3, 30});
    actor.ApplyStatus({StatusType::Bleed, 3, 0, 10});
    assert(actor.CleanseNegativeStatuses() == 2);
    assert(actor.HasStatus(StatusType::DotReduction) && actor.HasStatus(StatusType::AttackUp));
}
inline void ResistanceAndIndependentStacks() {
    Player first("A", 1000), replay("B", 1000);
    Stats stats; stats.res = 50;
    first.ModifyStats(stats); replay.ModifyStats(stats);
    first.SetStatusSeed(317); replay.SetStatusSeed(317);
    int applied = 0;
    for (int draw = 0; draw < 1000; ++draw) {
        const StatusEffect dot{StatusType::Burn, 2, 0, 10};
        const bool left = first.ApplyStatus(dot, 0);
        assert(left == replay.ApplyStatus(dot, 0));
        applied += left;
        first.ClearStatuses(); replay.ClearStatuses();
    }
    assert(applied > 430 && applied < 570 && first.GetStatusRandomDrawCount() == replay.GetStatusRandomDrawCount());
    const auto beforeDraws = first.GetStatusRandomDrawCount();
    for (StatusType type : {StatusType::AttackUp, StatusType::DefenseUp, StatusType::SpeedUp,
        StatusType::AccuracyUp, StatusType::CritUp, StatusType::CritDamageUp, StatusType::GuaranteedCrit,
        StatusType::HealingPotencyUp, StatusType::DotUp, StatusType::DotReduction, StatusType::AccuracyFloor,
        StatusType::DamageReduction, StatusType::HealOverTime, StatusType::Barrier})
        assert(first.ApplyStatus({type, 2, 10}, -1000));
    assert(first.GetStatusRandomDrawCount() == beforeDraws);
    first.ClearStatuses();
    first.ApplyStatus({StatusType::Burn, 2, 0, 20});
    first.ApplyStatus({StatusType::Burn, 2, 0, 30});
    assert(first.GetStatuses().size() == 2);
    first.ApplyStatus({StatusType::AttackUp, 2, 50});
    assert(!first.ApplyStatus({StatusType::AttackUp, 3, 10}));
    assert(first.getAttack() == 30);
}
}
inline void RunStatusResolutionTests() {
    StatusResolutionValidation::DamageAndHealing();
    Player player("Status player", 1000);
    Enemy enemy(StatusResolutionValidation::Definition(), 1);
    StatusResolutionValidation::TimingAndSources(player);
    StatusResolutionValidation::TimingAndSources(enemy);
    StatusResolutionValidation::ResistanceAndIndependentStacks();
    std::cout << "Status resolution passed: conservation, overkill, no revival, safe extremes, final-turn barriers, source Mark, stacks, cleanse and seeded resistance.\n";
}
