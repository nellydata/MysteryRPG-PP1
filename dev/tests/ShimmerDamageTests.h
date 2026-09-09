#pragma once
#include "ContentCatalog.h"
#include "ShimmerSkill.h"
#include "StatusResolutionTests.h"
inline void RunShimmerDamageTests() {
    for (const auto& name : BuildLatinShimmerSkillNames()) {
        auto skill = ShimmerSkill::Create(name);
        assert(skill);
        Player dead;
        dead.takeDamage(100);
        Enemy untouched(StatusResolutionValidation::Definition(), 1);
        const int before = untouched.getHP();
        skill->apply(dead, untouched);
        assert(!dead.isAlive() && dead.GetBarrier() == 0 && dead.GetStatuses().empty());
        assert(untouched.getHP() == before && untouched.GetStatuses().empty());
        if (skill->getTargetType() == ShimmerSkill::TargetType::Self) continue;
        Player player;
        Stats boost; boost.atk = INT_MAX - 20;
        player.ModifyStats(boost);
        Enemy enemy(StatusResolutionValidation::Definition(), 1);
        skill->apply(player, enemy);
        StatusResolutionValidation::Conservation(enemy.GetLastDamageResult());
        assert(enemy.GetLastDamageResult().requestedDamage == INT_MAX && !enemy.isAlive());
    }
    auto siphon = ShimmerSkill::Create("Siphon Lux");
    Player player;
    player.takeDamage(50);
    Enemy shielded(StatusResolutionValidation::Definition(), 1);
    shielded.ApplyStatus({StatusType::Barrier, 3, 10000});
    siphon->apply(player, shielded);
    assert(player.getHP() == 50 && shielded.GetLastDamageResult().hpDamage == 0);
    Enemy nearlyDead(StatusResolutionValidation::Definition(), 1);
    nearlyDead.takeDamage(nearlyDead.getHP() - 8);
    siphon->apply(player, nearlyDead);
    assert(nearlyDead.GetLastDamageResult().hpDamage == 8 && player.getHP() == 52);
    Enemy burnTarget(StatusResolutionValidation::Definition(), 1);
    auto burn = ShimmerSkill::Create("Lumen Spiculum");
    burn->apply(player, burnTarget);
    assert(burnTarget.HasStatus(StatusType::Burn));
    assert(burnTarget.GetStatuses().back().sourceId == player.GetCombatantId());
    assert(burnTarget.GetStatuses().back().source == player.getName());
    std::cout << "Shimmer damage passed: all offensive catalogue paths at large ATK, dead casters, actual-damage Siphon and DOT source identity.\n";
}
