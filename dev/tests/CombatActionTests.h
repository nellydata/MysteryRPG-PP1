#pragma once
#include "CombatManager.h"
#include "ConsoleUI.h"
#include "StatusResolutionTests.h"
#include "ShimmerSkill.h"
#include <sstream>

namespace CombatActionValidation {
struct Battle {
    Player player;
    Enemy enemy{StatusResolutionValidation::Definition(), 1};
    CombatManager combat;
    explicit Battle(unsigned seed = 711) : combat(seed) {
        combat.BeginEncounter(player, enemy, 1);
        combat.PrepareTurn(player, enemy);
        assert(combat.IsPlayerTurn());
    }
    void NextPlayer() {
        for (int count = 0; count < 30 && !combat.IsPlayerTurn() && player.isAlive() && enemy.isAlive(); ++count)
            combat.enemyTurn(player, enemy);
        assert(player.isAlive() && enemy.isAlive() && combat.IsPlayerTurn());
    }
};
inline void InvalidActionsAndReplay() {
    Battle clean, noisy;
    Stats crit; crit.critRate = 45;
    clean.player.ModifyStats(crit); noisy.player.ModifyStats(crit);
    for (int turn = 0; turn < 8; ++turn) {
        const auto draws = noisy.combat.GetRandomDrawCount();
        const int health = noisy.player.getHP(), targetHealth = noisy.enemy.getHP();
        assert(!noisy.combat.resolve(CombatAction::Shimmer, noisy.player, noisy.enemy));
        assert(!noisy.combat.resolve(CombatAction::Item, noisy.player, noisy.enemy));
        assert(!noisy.combat.resolve(static_cast<CombatAction>(999), noisy.player, noisy.enemy));
        assert(!noisy.combat.UseConsumable(999999ULL, noisy.player, noisy.enemy));
        noisy.combat.RecordInfo("Opened encounter history");
        noisy.combat.PrepareTurn(noisy.player, noisy.enemy);
        assert(noisy.combat.GetRandomDrawCount() == draws);
        assert(noisy.player.getHP() == health && noisy.enemy.getHP() == targetHealth);
        assert(clean.combat.resolve(CombatAction::Attack, clean.player, clean.enemy));
        assert(noisy.combat.resolve(CombatAction::Attack, noisy.player, noisy.enemy));
        assert(clean.enemy.getHP() == noisy.enemy.getHP());
        clean.NextPlayer(); noisy.NextPlayer();
        assert(clean.player.getHP() == noisy.player.getHP());
        assert(clean.combat.GetRandomDrawCount() == noisy.combat.GetRandomDrawCount());
    }
    Player foreign;
    Enemy foreignEnemy(StatusResolutionValidation::Definition(), 1);
    const auto draws = clean.combat.GetRandomDrawCount();
    assert(!clean.combat.resolve(CombatAction::Attack, foreign, clean.enemy));
    assert(!clean.combat.resolve(CombatAction::Attack, clean.player, foreignEnemy));
    clean.combat.enemyTurn(foreign, clean.enemy);
    assert(clean.combat.GetRandomDrawCount() == draws);

    int critical = 0;
    for (unsigned seed = 0; seed < 1000; ++seed) {
        Battle battle(seed);
        battle.player.ModifyStats(crit);
        battle.combat.resolve(CombatAction::Attack, battle.player, battle.enemy);
        for (const auto& event : battle.combat.GetEvents())
            if (event.kind == CombatManager::CombatEvent::Kind::Critical) ++critical;
    }
    assert(critical > 430 && critical < 570);
}
inline void StatusActionsAndCooldowns() {
    Player interrupted;
    Enemy interrupter(StatusResolutionValidation::Definition(), 1);
    CombatManager interruption(51);
    interrupted.ApplyStatus({StatusType::Stun, 1, 0});
    interruption.BeginEncounter(interrupted, interrupter, 1);
    interruption.PrepareTurn(interrupted, interrupter);
    assert(!interruption.IsPlayerTurn() && !interrupted.HasStatus(StatusType::Stun));
    for (int step = 0; step < 10 && !interruption.IsPlayerTurn(); ++step)
        interruption.enemyTurn(interrupted, interrupter);
    assert(interruption.IsPlayerTurn() && interrupted.isAlive());
    int playerSkips = 0;
    for (const auto& event : interruption.GetEvents())
        playerSkips += event.message.find("loses the turn to Stun") != std::string::npos;
    assert(playerSkips == 1);
    Battle stunned;
    stunned.enemy.ApplyStatus({StatusType::Stun, 1, 0});
    stunned.combat.resolve(CombatAction::Attack, stunned.player, stunned.enemy);
    const int health = stunned.player.getHP();
    stunned.NextPlayer();
    assert(stunned.player.getHP() == health && !stunned.enemy.HasStatus(StatusType::Stun));
    int skipped = 0;
    for (const auto& event : stunned.combat.GetEvents())
        skipped += event.message.find("loses its turn to Stun") != std::string::npos;
    assert(skipped == 1);
    Battle cooldown;
    auto skill = ShimmerSkill::Create("Prism Step");
    assert(cooldown.combat.resolve(CombatAction::Shimmer, cooldown.player, cooldown.enemy, skill.get()));
    cooldown.NextPlayer();
    assert(cooldown.combat.GetSkillCooldown(skill->getName()) == skill->getCooldown() - 1);
    const auto draws = cooldown.combat.GetRandomDrawCount();
    const int before = cooldown.combat.GetSkillCooldown(skill->getName());
    assert(!cooldown.combat.resolve(CombatAction::Shimmer, cooldown.player, cooldown.enemy, skill.get()));
    assert(cooldown.combat.GetSkillCooldown(skill->getName()) == before &&
        cooldown.combat.GetRandomDrawCount() == draws && cooldown.combat.IsPlayerTurn());

    Battle dot;
    dot.enemy.takeDamage(dot.enemy.getHP() - 30);
    dot.enemy.ApplyStatus({StatusType::Burn, 1, 0, 100, 0, 0, dot.player.getName(), dot.player.GetCombatantId()});
    auto support = ShimmerSkill::Create("Prism Step");
    dot.combat.resolve(CombatAction::Shimmer, dot.player, dot.enemy, support.get());
    dot.combat.enemyTurn(dot.player, dot.enemy);
    assert(!dot.enemy.isAlive() && dot.player.getHP() == 100);
    for (const auto& event : dot.combat.GetEvents())
        assert(!(event.actor == dot.enemy.getName() && (event.kind == CombatManager::CombatEvent::Kind::Attack ||
            event.kind == CombatManager::CombatEvent::Kind::Skill)));

    auto bonusDefinition = StatusResolutionValidation::Definition();
    bonusDefinition.name = "Bonus action fixture";
    bonusDefinition.speed = 500;
    bonusDefinition.skills = {{"Clockwork acceleration", "extra_turn", 100, 1, 0, 0}};
    Enemy bonusEnemy(bonusDefinition, 1);
    Player bonusTarget;
    CombatManager bonus(21);
    bonus.BeginEncounter(bonusTarget, bonusEnemy, 1);
    bonus.PrepareTurn(bonusTarget, bonusEnemy);
    assert(!bonus.IsPlayerTurn());
    bonus.enemyTurn(bonusTarget, bonusEnemy);
    assert(!bonus.IsPlayerTurn() && bonusEnemy.GetSkillCooldown(0) == 1);
    bonus.enemyTurn(bonusTarget, bonusEnemy);
    assert(bonus.IsPlayerTurn() && bonusEnemy.GetSkillCooldown(0) == 1);
    int bonuses = 0;
    for (const auto& event : bonus.GetEvents()) bonuses += event.kind == CombatManager::CombatEvent::Kind::ExtraTurn;
    assert(bonuses == 1); // The bonus cannot tick cooldowns and recursively grant itself.
}
inline void GroupRoundOwnership() {
    Player player("Party player", 1000), foreign;
    auto definition = StatusResolutionValidation::Definition();
    definition.name = "Fast party member"; definition.speed = 200;
    Enemy first(definition, 1);
    definition.name = "Slow party member"; definition.speed = 20;
    Enemy second(definition, 1);
    CombatManager combat(100);
    const std::vector<Enemy*> party{&second, &first, &first, nullptr};
    first.ApplyStatus({StatusType::Stun, 1, 0});
    player.ApplyStatus({StatusType::AttackUp, 1, 40});
    combat.PrepareGroupTurn(player, party);
    const auto initialEvents = combat.GetEvents().size(), initialDraws = combat.GetRandomDrawCount();
    combat.enemyTurn(player, second); // The faster member owns this action.
    assert(!combat.StepEnemyPartyTurn(foreign, party));
    assert(combat.GetEvents().size() == initialEvents && combat.GetRandomDrawCount() == initialDraws);
    assert(combat.StepEnemyPartyTurn(player, party)); // First enemy's Stun skip.
    assert(!first.HasStatus(StatusType::Stun) && player.getHP() == 1000);
    const auto afterFirst = combat.GetEvents().size(), drawsAfterFirst = combat.GetRandomDrawCount();
    combat.PrepareGroupTurn(player, party); // No restart/reseed while a round is open.
    assert(combat.GetEvents().size() == afterFirst && combat.GetRandomDrawCount() == drawsAfterFirst);
    assert(!combat.StepEnemyPartyTurn(player, party) && combat.IsPlayerTurn());
    const int beforeChoice = player.getHP();
    combat.PrepareGroupTurn(player, party);
    assert(!combat.StepEnemyPartyTurn(player, party));
    assert(combat.IsPlayerTurn() && player.getHP() == beforeChoice && player.HasStatus(StatusType::AttackUp));
    auto support = ShimmerSkill::Create("Prism Step");
    assert(combat.resolve(CombatAction::Shimmer, player, second, support.get()));
    assert(!combat.IsPlayerTurn() && !player.HasStatus(StatusType::AttackUp));
    assert(combat.StepEnemyPartyTurn(player, party));
    assert(!combat.StepEnemyPartyTurn(player, party) && combat.IsPlayerTurn());
    assert(combat.GetSkillCooldown(support->getName()) == support->getCooldown() - 1);
    int firstTurns = 0, secondTurns = 0;
    for (const auto& event : combat.GetEvents()) if (event.kind == CombatManager::CombatEvent::Kind::Turn) {
        firstTurns += event.actor == first.getName();
        secondTurns += event.actor == second.getName();
    }
    assert(firstTurns == 2 && secondTurns == 2); // Duplicate input pointers do not add actions.

    Player interrupted("Interrupted party player", 1000);
    Enemy survivor(definition, 1), doomed(definition, 1);
    doomed.takeDamage(doomed.getHP() - 10);
    doomed.ApplyStatus({StatusType::Burn, 1, 0, 100});
    interrupted.ApplyStatus({StatusType::Stun, 1, 0});
    CombatManager secondCombat(99);
    const std::vector<Enemy*> secondParty{&doomed, &survivor};
    secondCombat.PrepareGroupTurn(interrupted, secondParty);
    assert(secondCombat.StepEnemyPartyTurn(interrupted, secondParty));
    assert(!doomed.isAlive());
    assert(!secondCombat.StepEnemyPartyTurn(interrupted, secondParty));
    assert(!secondCombat.IsPlayerTurn() && !interrupted.HasStatus(StatusType::Stun));
    assert(!secondCombat.StepEnemyPartyTurn(interrupted, secondParty));
    assert(secondCombat.IsPlayerTurn()); // A skipped player action starts the next round exactly once.
}
}
inline void RunCombatActionTests() {
    CombatActionValidation::InvalidActionsAndReplay();
    CombatActionValidation::StatusActionsAndCooldowns();
    CombatActionValidation::GroupRoundOwnership();
    std::cout << "Combat actions passed: seeded replay/variation, invalid selection immutability, actor ownership, Stun skips, cooldown rejection, bounded bonus actions, DOT death and group rounds.\n";
}
