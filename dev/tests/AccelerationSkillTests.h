#pragma once

#include "CombatManager.h"
#include "Enemy.h"
#include "Player.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace AccelerationSkillValidation {
inline EnemyDefinition TargetDefinition() {
    EnemyDefinition target;
    target.name = "Acceleration timing target";
    target.hpBase = 100000;
    target.defBase = 0;
    target.atkBase = 1;
    target.speed = 23; // The existing Devotee normalization produces exactly 80 SPD.
    return target;
}

inline int CountAcceleration(const CombatManager& combat) {
    return static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(), [](const auto& event) {
        return event.message.find("next-action gauge progress") != std::string::npos;
    }));
}

inline void NextPlayer(CombatManager& combat, Player& player, Enemy& enemy) {
    for (int step = 0; step < 30 && !combat.IsPlayerTurn() && player.isAlive() && enemy.isAlive(); ++step)
        combat.enemyTurn(player, enemy);
    assert(combat.IsPlayerTurn() && player.isAlive() && enemy.isAlive());
}

// Exercise the same catalogue skill's real status implementation while omitting
// only its gauge metadata. This isolates acceleration from the 40% SPD buff.
class CoreStatsOnly final : public ShimmerSkill {
public:
    explicit CoreStatsOnly(const std::string& name)
        : ShimmerSkill(name, 2, 4, "Timing comparison fixture", "stat_boost", 0, 2),
          source_(ShimmerSkill::Create(name)) {}
    void apply(Player& player, Enemy& enemy) override { source_->apply(player, enemy); }
private:
    std::unique_ptr<ShimmerSkill> source_;
};

inline void VerifySkill(const std::string& name) {
    auto skill = ShimmerSkill::Create(name);
    assert(skill && skill->getTargetType() == ShimmerSkill::TargetType::Self);
    assert(skill->getEquipCost() == 2 && skill->getCooldown() == 4 && skill->getDuration() == 2);
    assert(skill->getPowerPercent() == 0); // Gauge progress is not damage power.
    const auto& metadata = skill->getCombatBuff();
    assert(metadata.attackPercent == 40 && metadata.defensePercent == 40 && metadata.speedPercent == 40);
    assert(metadata.nextActionGauge == 800);
    assert(metadata.critRatePoints == 0 && metadata.critDamagePoints == 0 && metadata.damageReductionPoints == 0);
    assert(skill->getDescription().find("40% ATK, DEF and SPD") != std::string::npos);
    assert(skill->getDescription().find("2 normal actions") != std::string::npos);
    assert(skill->getDescription().find("800/1000") != std::string::npos);

    Player player;
    Enemy enemy(TargetDefinition(), 1);
    assert(enemy.getSpeed() == 80);
    CombatManager combat(513);
    combat.BeginEncounter(player, enemy, 1);
    combat.PrepareTurn(player, enemy);
    assert(combat.IsPlayerTurn());
    assert(combat.resolve(CombatAction::Shimmer, player, enemy, skill.get()));
    assert(player.getAttack() == 28 && player.getDefense() == 14 && player.getSpeed() == 140);
    assert(player.getCritRate() == 5 && player.getCritDamage() == 150);
    assert(player.GetStatuses().size() == 3);
    for (const auto& status : player.GetStatuses()) assert(status.potency == 40 && status.turns == 2);
    assert(combat.IsPlayerTurn()); // Acceleration reaches the next action before the enemy.
    assert(combat.GetSkillCooldown(name) == 3); // Its next normal action started once.
    assert(CountAcceleration(combat) == 1);
    int playerTurns = 0, enemyTurns = 0, detailedBuffs = 0;
    for (const auto& event : combat.GetEvents()) {
        if (event.kind == CombatManager::CombatEvent::Kind::Turn) {
            playerTurns += event.actor == player.getName();
            enemyTurns += event.actor == enemy.getName();
            if (playerTurns == 2 && event.actor == player.getName())
                assert(event.playerGauge == 1080 && event.enemyGauge == 960);
        }
        if (event.message.find("next-action gauge progress") != std::string::npos)
            assert(event.value == 800 && event.playerGauge == 1800 && event.enemyGauge == 800 && event.skill == name);
        detailedBuffs += event.skill == name && event.message.find("2 turns, potency 40") != std::string::npos;
    }
    assert(playerTurns == 2 && enemyTurns == 0 && detailedBuffs == 3);

    const auto randomBefore = combat.GetRandomDrawCount();
    const int playerGaugeBefore = combat.GetEvents().back().playerGauge;
    const int enemyGaugeBefore = combat.GetEvents().back().enemyGauge;
    assert(!combat.resolve(CombatAction::Shimmer, player, enemy, skill.get()));
    assert(!combat.resolve(CombatAction::Shimmer, player, enemy));
    assert(!combat.resolve(CombatAction::Item, player, enemy));
    combat.PrepareTurn(player, enemy);
    assert(combat.GetRandomDrawCount() == randomBefore && combat.GetSkillCooldown(name) == 3);
    assert(CountAcceleration(combat) == 1 && combat.IsPlayerTurn());
    assert(combat.GetEvents().back().playerGauge == playerGaugeBefore && combat.GetEvents().back().enemyGauge == enemyGaugeBefore);
    for (const auto& status : player.GetStatuses()) assert(status.turns == 2);

    assert(combat.resolve(CombatAction::Attack, player, enemy));
    NextPlayer(combat, player, enemy);
    assert(player.getAttack() == 28 && player.getDefense() == 14 && player.getSpeed() == 140);
    for (const auto& status : player.GetStatuses()) assert(status.turns == 1);
    assert(combat.resolve(CombatAction::Attack, player, enemy));
    NextPlayer(combat, player, enemy);
    assert(player.getAttack() == 20 && player.getDefense() == 10 && player.getSpeed() == 100);
    assert(player.GetStatuses().empty() && CountAcceleration(combat) == 1);

    Player comparison;
    Enemy comparisonEnemy(TargetDefinition(), 1);
    CombatManager withoutAcceleration(513);
    CoreStatsOnly coreOnly(name);
    withoutAcceleration.BeginEncounter(comparison, comparisonEnemy, 1);
    withoutAcceleration.PrepareTurn(comparison, comparisonEnemy);
    assert(withoutAcceleration.resolve(CombatAction::Shimmer, comparison, comparisonEnemy, &coreOnly));
    assert(comparison.getSpeed() == 140 && !withoutAcceleration.IsPlayerTurn());
    assert(CountAcceleration(withoutAcceleration) == 0); // Core SPD alone cannot win this next action.

    Player defeated;
    Enemy livingTarget(TargetDefinition(), 1);
    CombatManager invalid(52);
    defeated.takeDamage(defeated.getHP());
    assert(!invalid.resolve(CombatAction::Shimmer, defeated, livingTarget, skill.get()));
    assert(defeated.GetStatuses().empty() && CountAcceleration(invalid) == 0);
}

inline void MajorBuffMetadataPreserved() {
    for (const std::string name : {"Lux Ascension", "Onus Coruscum", "Resonantia Chori"}) {
        auto skill = ShimmerSkill::Create(name);
        Player player;
        Enemy enemy(TargetDefinition(), 1);
        skill->apply(player, enemy);
        assert(player.getAttack() == 25 && player.getDefense() == 13 && player.getSpeed() == 125);
        assert(player.getCritRate() == 20 && player.getCritDamage() == 175);
        assert(skill->getCombatBuff().nextActionGauge == 0);
        for (const auto& status : player.GetStatuses()) assert(status.turns == 3);
    }
}
}

inline void RunAccelerationSkillTests() {
    AccelerationSkillValidation::VerifySkill("Gradus Coruscus");
    AccelerationSkillValidation::VerifySkill("Mutatio Horologii");
    AccelerationSkillValidation::MajorBuffMetadataPreserved();
    std::cout << "Acceleration skills passed: explicit 40% core stats, 800/1000 next-action progress, two-action duration, real turn order, cooldowns, rejected casts and detailed events.\n";
}
