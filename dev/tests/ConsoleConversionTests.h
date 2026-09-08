#pragma once
#include "ConsoleUI.h"
#include "EncounterManager.h"
#include "Area.h"
#include <sstream>

inline void TestConsoleConversion() {
    {
        Player player;
        ProgressionManager progression;
        progression.GrantXP(1000);
        progression.CheckLevelUp(player);
        int choices = 0;
        while (progression.HasPendingShimmerChoice() && choices++ < 20) {
            const auto& pending = progression.GetPendingShimmerChoices();
            if (pending.empty()) assert(progression.ResolvePendingShimmerChoice());
            else assert(progression.SelectPendingShimmerSkill(pending.front()));
        }
        assert(choices > 1 && !progression.HasPendingShimmerChoice());
    }
    ConsoleUI::SetFast(true);
    ConsoleUI ui;
    CombatManager combat;
    EnemyDefinition definition;
    definition.name = "Opening attacker";
    definition.hpBase = 1000;
    definition.atkBase = 10000;
    definition.speed = 500;
    definition.skills = {{"Opening verdict", "def_down", 120, 3, 2, 20}};
    Enemy enemy(definition, 1);
    Player player;
    combat.BeginEncounter(player, enemy, 1);
    combat.PrepareTurn(player, enemy);
    assert(!combat.IsPlayerTurn());
    combat.enemyTurn(player, enemy);
    assert(!player.isAlive());
    combat.CompleteEncounter("defeat");
    bool damage = false, death = false;
    for (const auto& event : combat.GetEvents()) {
        if (event.kind == CombatManager::CombatEvent::Kind::Damage) {
            assert(event.actor == definition.name && event.target == player.getName());
            assert(event.skill == "Opening verdict" && event.hpDamage == 100 && event.remainingHP == 0);
            assert(event.requestedDamage >= event.hpDamage);
            damage = true;
        }
        death |= event.kind == CombatManager::CombatEvent::Kind::Death;
    }
    assert(damage && death);
    const auto snapshot = combat.GetEvents();
    combat.ResetEncounter();
    assert(combat.GetEvents().size() == snapshot.size());
    std::istringstream input("1\n0\n");
    std::ostringstream output;
    auto* oldIn = std::cin.rdbuf(input.rdbuf());
    auto* oldOut = std::cout.rdbuf(output.rdbuf());
    ui.ShowDefeat(player, combat);
    assert(!player.isAlive()); // Presentation must wait for acknowledgment, never respawn itself.
    assert(output.str().find("Final cause: Opening attacker / Opening verdict") != std::string::npos);
    assert(output.str().find("FULL ENCOUNTER LOG") != std::string::npos);
    std::cin.rdbuf(oldIn);
    std::cout.rdbuf(oldOut);
    std::cin.clear();

    for (int index = 0; index < 200; ++index) combat.RecordInfo("history marker " + std::to_string(index));
    const auto& events = combat.GetEvents();
    assert(events.size() == snapshot.size() + 200);
    for (std::size_t index = 1; index < events.size(); ++index) assert(events[index].sequenceId > events[index - 1].sequenceId);
    output.str(""); output.clear();
    oldOut = std::cout.rdbuf(output.rdbuf());
    ui.PresentEvents(combat);
    const auto once = output.str();
    ui.PresentEvents(combat);
    assert(output.str() == once && once.find("history marker 199") != std::string::npos);
    std::cout.rdbuf(oldOut);
    std::istringstream invalid("abc\n1x\n-1\n2\n");
    oldIn = std::cin.rdbuf(invalid.rdbuf());
    oldOut = std::cout.rdbuf(output.rdbuf());
    assert(ui.Prompt("Input regression", 0, 2) == 2);
    assert(ui.Prompt("EOF regression", 1, 2) == 0);
    std::cin.rdbuf(oldIn); std::cout.rdbuf(oldOut); std::cin.clear();

    {
        CombatManager hazardCombat;
        Player investigator;
        EnemyDefinition weakDefinition;
        weakDefinition.name = "Prior completed encounter";
        weakDefinition.hpBase = 1;
        weakDefinition.atkBase = 1;
        weakDefinition.speed = 1;
        Enemy weakEnemy(weakDefinition, 1);
        hazardCombat.BeginEncounter(investigator, weakEnemy, 1);
        hazardCombat.PrepareTurn(investigator, weakEnemy);
        assert(hazardCombat.IsPlayerTurn());
        assert(hazardCombat.resolve(CombatAction::Attack, investigator, weakEnemy));
        assert(!weakEnemy.isAlive() && investigator.isAlive());
        hazardCombat.CompleteEncounter("victory");
        const auto previousSequence = hazardCombat.GetEvents().back().sequenceId;
        hazardCombat.ResetEncounter();

        EncounterManager encounters;
        bool foundLethalHazard = false;
        for (unsigned seed = 0; seed < 1000 && !foundLethalHazard; ++seed) {
            Area room("Hazard regression room");
            room.SetSpawnCondition(EnemySpawnCondition::None);
            ProgressionManager progression;
            investigator.RestoreHealth(1);
            const auto investigation = encounters.ResolveInvestigation(room, investigator, progression, seed);
            if (investigation.type != InvestigationEventType::Hazard) continue;
            foundLethalHazard = true;
            assert(!investigator.isAlive() && investigation.hpDelta == -1);
            const auto actual = investigator.GetLastDamageResult();
            assert(actual.hpDamage == 1 && actual.requestedDamage > actual.hpDamage && actual.remainingHP == 0);
            hazardCombat.RecordEnvironmentalDefeat("Falling masonry", investigator);
            // Completion of the previous fight must not suppress this new defeat.
            hazardCombat.CompleteEncounter("defeat");
            int damageEvents = 0, deaths = 0, completions = 0;
            for (const auto& event : hazardCombat.GetEvents()) {
                assert(event.sequenceId > previousSequence);
                assert(event.message.find(weakDefinition.name) == std::string::npos);
                if (event.kind == CombatManager::CombatEvent::Kind::Environment) {
                    ++damageEvents;
                    assert(event.actor == "Falling masonry" && event.target == investigator.getName());
                    assert(event.requestedDamage == actual.requestedDamage);
                    assert(event.barrierAbsorbed == actual.barrierAbsorbed);
                    assert(event.hpDamage == actual.hpDamage && event.remainingHP == 0);
                }
                deaths += event.kind == CombatManager::CombatEvent::Kind::Death;
                completions += event.kind == CombatManager::CombatEvent::Kind::Completion;
            }
            assert(damageEvents == 1 && deaths == 1 && completions == 1);
            ConsoleUI hazardUI;
            std::istringstream acknowledgment("1\n0\n");
            std::ostringstream defeatOutput;
            oldIn = std::cin.rdbuf(acknowledgment.rdbuf());
            oldOut = std::cout.rdbuf(defeatOutput.rdbuf());
            hazardUI.ShowDefeat(investigator, hazardCombat);
            std::cin.rdbuf(oldIn); std::cout.rdbuf(oldOut); std::cin.clear();
            assert(!investigator.isAlive());
            assert(defeatOutput.str().find("Final cause: Falling masonry") != std::string::npos);
            assert(defeatOutput.str().find("1 actual HP damage") != std::string::npos);
            assert(defeatOutput.str().find("Encounter complete: defeat") != std::string::npos);
            assert(defeatOutput.str().find("FULL ENCOUNTER LOG") != std::string::npos);
        }
        assert(foundLethalHazard);
    }
    std::cout << "Console conversion regressions passed.\n";
}
