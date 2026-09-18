#include "CombatManager.h"
#include "ContentCatalog.h"
#include "Enemy.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {
void Check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Enemy MakeTarget() {
    return Enemy(EnemyDefinition{"Shimmer test target", EnemyFaction::Devotee,
        EnemyTier::Basic, 1000000, 1, 10, 50}, 100);
}

const StatusEffect* FindStatus(const std::vector<StatusEffect>& statuses, StatusType type) {
    const auto found = std::find_if(statuses.begin(), statuses.end(), [=](const StatusEffect& status) {
        return status.type == type;
    });
    return found == statuses.end() ? nullptr : &*found;
}

void CheckCatalogue() {
    const auto names = BuildLatinShimmerSkillNames();
    Check(names.size() == 31, "Shimmer catalogue size changed");
    Check(std::set<std::string>(names.begin(), names.end()).size() == names.size(), "Duplicate Shimmer name");
    const std::set<std::string> supported = {"barrier", "heal_hot", "cleanse_resist", "defensive_buff",
        "damage_ignore", "damage_burn", "damage_stun", "damage_tridot", "damage_lifesteal",
        "damage_defdown", "stat_boost"};
    for (const auto& name : names) {
        auto skill = ShimmerSkill::Create(name);
        Check(skill != nullptr, "Missing skill: " + name);
        Check(skill->getName() == name && !skill->getDescription().empty(), "Incomplete skill: " + name);
        Check(skill->getCost() >= 1 && skill->getCooldown() >= 1, "Invalid cost/cooldown: " + name);
        Check(supported.count(skill->getEffect()) == 1, "Unimplemented effect: " + name);
        Check(skill->getTargetType() == (skill->getEffect().find("damage") == 0 ?
            ShimmerSkill::TargetType::SingleEnemy : ShimmerSkill::TargetType::Self), "Wrong target: " + name);
        Player player;
        auto enemy = MakeTarget();
        const int beforeHP = enemy.getHP();
        skill->apply(player, enemy);
        Check(enemy.getHP() < beforeHP || player.GetBarrier() > 0 || !player.GetStatuses().empty(),
            "Skill did nothing: " + name);
    }
    for (const auto& entry : {std::pair<const char*, const char*>{"mirror", "Claustrum Lucis"},
                              {"Matron of the Catacombs", "Curatio Refractionis"},
                              {"Catacombs", "Praesidium Lucis"}, {"corpse", "Siphon Lucis"},
                              {"statue", "Corruptio Lucis"}, {"relic", "Purgatio Lucis"},
                              {"vortex", "Vortex Lucis"}, {"Oracle Core", "Lorica Lucis"}}) {
        ProgressionManager progression;
        Check(progression.UnlockShimmerFromTrigger(entry.first) && progression.HasShimmerSkill(entry.second),
            std::string("Broken Shimmer unlock: ") + entry.first);
    }
}

void CheckAuthoredEffects() {
    for (const auto& entry : {std::pair<const char*, int>{"Corruptio Lucis", 220},
                              {"Nota Luminosa", 90}, {"Revelatio Occulta", 90}}) {
        auto skill = ShimmerSkill::Create(entry.first);
        Check(skill->getEffectMetadata().statusPotency == entry.second, "DEF-down metadata");
        Player player;
        auto enemy = MakeTarget();
        skill->apply(player, enemy);
        const auto* status = FindStatus(enemy.GetStatuses(), StatusType::DefenseDown);
        Check(status && status->potency == entry.second && status->turns == skill->getDuration(),
            std::string("DEF-down runtime: ") + entry.first);
    }
    auto barrier = ShimmerSkill::Create("Praesidium Lucis");
    Check(barrier->getEffect() == "barrier" && barrier->getTargetType() == ShimmerSkill::TargetType::Self,
        "Emergency barrier metadata");
    Player player;
    auto enemy = MakeTarget();
    barrier->apply(player, enemy);
    const auto* status = FindStatus(player.GetStatuses(), StatusType::Barrier);
    Check(status && status->turns == 1 && player.GetBarrier() == player.getMaxHP() * 35,
        "Emergency barrier runtime");
    Check(enemy.getHP() == enemy.getMaxHP(), "Emergency barrier damaged enemy");
    auto aegis = ShimmerSkill::Create("Aegis Lucis");
    Check(aegis->getEffectMetadata().statusPotency == 30 &&
        aegis->getDescription().find("30% damage reduction") != std::string::npos,
        "Aegis runtime/description mismatch");
}

void CheckAccelerationAndCooldown() {
    for (const auto& name : {"Gradus Coruscus", "Mutatio Horologii"}) {
        auto skill = ShimmerSkill::Create(name);
        const auto& buff = skill->getCombatBuff();
        Check(buff.attackPercent == 40 && buff.defensePercent == 40 && buff.speedPercent == 40 &&
            buff.nextActionGauge == 800 && skill->getDuration() == 2, "Acceleration metadata");
        Player player;
        auto enemy = MakeTarget();
        CombatManager combat(12345);
        combat.BeginEncounter(player, enemy, 1);
        combat.PrepareTurn(player, enemy);
        Check(combat.IsPlayerTurn(), "Player did not start acceleration test");
        Check(combat.resolve(CombatAction::Shimmer, player, enemy, skill.get()), "Acceleration cast failed");
        for (const auto type : {StatusType::AttackUp, StatusType::DefenseUp, StatusType::SpeedUp}) {
            const auto* status = FindStatus(player.GetStatuses(), type);
            Check(status && status->potency == 40 && status->turns == 2, "Acceleration status mismatch");
        }
        const auto acceleration = std::find_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.message.find("800/1000 next-action gauge progress") != std::string::npos;
            });
        Check(acceleration != combat.GetEvents().end(), "Acceleration event missing");
        Check(combat.GetSkillCooldown(name) > 0, "Cooldown not consumed");
        const auto advanceToPlayer = [&]() {
            for (int turn = 0; turn < 10 && !combat.IsPlayerTurn(); ++turn) combat.enemyTurn(player, enemy);
            Check(combat.IsPlayerTurn(), "Player turn never returned");
        };
        advanceToPlayer();
        const int cooldown = combat.GetSkillCooldown(name);
        Check(!combat.resolve(CombatAction::Shimmer, player, enemy, skill.get()), "Cooldown bypassed");
        Check(combat.GetSkillCooldown(name) == cooldown, "Rejected cast changed cooldown");
        auto secondSkill = ShimmerSkill::Create("Praesidium Lucis");
        Check(combat.resolve(CombatAction::Shimmer, player, enemy, secondSkill.get()), "Second cast failed");
        for (const auto type : {StatusType::AttackUp, StatusType::DefenseUp, StatusType::SpeedUp})
            Check(FindStatus(player.GetStatuses(), type)->turns == 2, "Support cast consumed normal-action buff");
        advanceToPlayer();
        Check(combat.resolve(CombatAction::Attack, player, enemy), "First normal attack failed");
        for (const auto type : {StatusType::AttackUp, StatusType::DefenseUp, StatusType::SpeedUp})
            Check(FindStatus(player.GetStatuses(), type)->turns == 1, "First normal action duration mismatch");
        advanceToPlayer();
        Check(combat.resolve(CombatAction::Attack, player, enemy), "Second normal attack failed");
        for (const auto type : {StatusType::AttackUp, StatusType::DefenseUp, StatusType::SpeedUp})
            Check(!FindStatus(player.GetStatuses(), type), "Buff survived two normal actions");
        Check(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.message.find("next-action gauge progress") != std::string::npos;
            }) == 1, "Gauge acceleration recurred");
    }
}

void CheckResistedStatus() {
    bool resisted = false;
    for (unsigned seed = 1; seed <= 50 && !resisted; ++seed) {
        Player player;
        auto enemy = MakeTarget();
        enemy.SetStatusSeed(seed);
        auto skill = ShimmerSkill::Create("Nota Luminosa");
        skill->apply(player, enemy);
        const auto& applications = enemy.GetStatusApplications();
        Check(!applications.empty() && applications.back().applied, "Shimmer status application missing");
        enemy.ClearStatuses();
        enemy.ApplyStatus({StatusType::DefenseDown, 3, skill->getEffectMetadata().statusPotency}, -100);
        resisted = enemy.GetStatusApplications().back().resisted &&
            !FindStatus(enemy.GetStatuses(), StatusType::DefenseDown);
    }
    Check(resisted, "No resisted Shimmer-authored status observed");
}
}

void RunCompatibilityChecks();
void RunArtifactRegression();

int main() {
    try {
        CheckCatalogue();
        CheckAuthoredEffects();
        CheckAccelerationAndCooldown();
        CheckResistedStatus();
        RunCompatibilityChecks();
        RunArtifactRegression();
        std::cout << "Shimmer regression tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Shimmer regression failed: " << error.what() << '\n';
        return 1;
    }
}
