#include "CombatManager.h"
#include "CombatMath.h"
#include "ConsoleFormat.h"
#include "ContentCatalog.h"
#include "DeveloperMode.h"
#include "Enemy.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("Artifact regression: " + message);
}

Item& Equip(Player& player, const std::string& name) {
    const auto catalogue = BuildEquippableCatalog();
    const auto found = std::find_if(catalogue.begin(), catalogue.end(), [&](const ItemDefinition& item) {
        return item.name == name;
    });
    Require(found != catalogue.end(), "missing item " + name);
    auto item = std::make_unique<Item>(*found);
    Item& reference = *item;
    player.AddItem(std::move(item));
    Require(player.Equip(reference), "equip " + name);
    return reference;
}

std::vector<Item*> EquipSet(Player& player, const std::string& set, int count) {
    std::vector<Item*> equipped;
    for (const auto& definition : BuildEquippableCatalog()) {
        if (definition.setName != set) continue;
        auto item = std::make_unique<Item>(definition);
        Item* raw = item.get();
        player.AddItem(std::move(item));
        Require(player.Equip(*raw), "equip set " + set);
        equipped.push_back(raw);
        if (static_cast<int>(equipped.size()) == count) break;
    }
    Require(static_cast<int>(equipped.size()) == count, "set pieces " + set);
    return equipped;
}

Enemy Target(int hp = 1000000, int attack = 1) {
    return Enemy(EnemyDefinition{"Artifact target", EnemyFaction::Devotee, EnemyTier::Basic,
        hp, attack, 0, 1, 1}, 1);
}

bool Has(const std::vector<StatusEffect>& statuses, StatusType type) {
    return std::any_of(statuses.begin(), statuses.end(), [&](const StatusEffect& status) { return status.type == type; });
}

bool Act(CombatManager& combat, Player& player, Enemy& enemy, CombatAction action,
         ShimmerSkill* skill = nullptr) {
    combat.PrepareTurn(player, enemy);
    for (int index = 0; index < 50 && !combat.IsPlayerTurn() && player.isAlive() && enemy.isAlive(); ++index)
        combat.enemyTurn(player, enemy);
    return combat.resolve(action, player, enemy, skill);
}

int EventCount(const CombatManager& combat, CombatManager::CombatEvent::Kind kind) {
    return static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
        [&](const CombatManager::CombatEvent& event) { return event.kind == kind; }));
}

void CheckCatalogueAndEquip() {
    int count = 0;
    for (const auto& definition : BuildEquippableCatalog()) if (definition.rarity == Rarity::Artifact) {
        ++count;
        Require(definition.uniqueEffect == definition.name && !definition.passive.empty() &&
            definition.passive != definition.uniqueEffect, "unique ID or authored passive");
        Require(definition.name.find('\x1b') == std::string::npos &&
            definition.passive.find('\x1b') == std::string::npos, "ANSI leaked into Artifact catalogue data");
        Player player;
        Require(!player.HasArtifact(definition.name), "unequipped unique active");
        auto item = std::make_unique<Item>(definition);
        Item& equipped = *item;
        Require(equipped.getPassive() == definition.passive && equipped.getSetPassive() == definition.setPassive,
            "Artifact presentation metadata diverged from catalogue");
        player.AddItem(std::move(item));
        Require(player.Equip(equipped) && player.HasArtifact(definition.name), "unique not active on equip");
        Require(player.ToggleEquip(equipped) && !player.HasArtifact(definition.name), "unique active after unequip");
    }
    Require(count == 15, "Artifact catalogue count");
}

void CheckPlayerHooks() {
    {
        Player player;
        Equip(player, "verdigris_saint_vestment");
        const int before = player.getHP();
        player.ApplyDirectDamage(20, true);
        Require(before - player.getHP() == 18, "Vestment final reduction");
    }
    {
        Player player;
        Equip(player, "choirs_eternal_mourning_veil");
        player.SetStatusSeed(1);
        Require(!player.ApplyStatus({StatusType::Stun, 2}, 100) &&
            !player.HasStatus(StatusType::Stun) && player.GetBarrier() >= 40,
            "Veil first Stun protection");
        Require(player.ApplyStatus({StatusType::Stun, 2}, 100), "Veil used twice");
        player.BeginArtifactBattle();
        player.ClearStatuses();
        Require(!player.ApplyStatus({StatusType::Stun, 2}, 100), "Veil failed next battle");
    }
    {
        bool foundResist = false;
        for (unsigned seed = 1; seed <= 100 && !foundResist; ++seed) {
            Player player;
            Equip(player, "choirs_eternal_mourning_veil");
            player.SetStatusSeed(seed);
            if (player.ApplyStatus({StatusType::Stun, 2}, -100) ||
                !player.GetStatusApplications().back().resisted) continue;
            Require(player.GetBarrier() == 0, "Veil consumed on resisted Stun");
            Require(!player.ApplyStatus({StatusType::Stun, 2}, 100) && player.GetBarrier() > 0,
                "Veil resistance consumed protection");
            foundResist = true;
        }
        Require(foundResist, "no resisted Veil fixture");
    }
    {
        Player player;
        Equip(player, "choirs_eternal_ritual_gown");
        player.takeDamage(30);
        const int before = player.getHP();
        Require(!player.ApplyStatus({StatusType::Bleed, 2}, 100) &&
            !player.HasStatus(StatusType::Bleed) && player.getHP() > before && player.GetBarrier() > 0,
            "Gown Bleed immunity/response");
        const int barrier = player.GetBarrier();
        player.ApplyStatus({StatusType::Bleed, 2}, 100);
        Require(player.GetBarrier() == barrier, "Gown exceeded once-per-turn response");
        player.BeginArtifactTurn();
        player.ApplyStatus({StatusType::Bleed, 2}, 100);
        Require(player.GetBarrier() > barrier, "Gown did not refresh next turn");
        player.BeginArtifactTurn();
        const int barrierBeforeNonDot = player.GetBarrier();
        const int hpBeforeNonDot = player.getHP();
        Require(player.ApplyStatus({StatusType::Stun, 2}, 100), "Gown non-DOT fixture");
        player.CleanseNegativeStatuses();
        Require(player.GetBarrier() == barrierBeforeNonDot && player.getHP() == hpBeforeNonDot,
            "Gown triggered on non-DOT cleanse");
        player.BeginArtifactTurn();
        Require(player.ApplyStatus({StatusType::Poison, 2}, 100), "Gown DOT cleanse fixture");
        const int beforeCleanse = player.GetBarrier();
        Require(player.CleanseNegativeStatuses() == 1 && player.GetBarrier() > beforeCleanse,
            "Gown did not trigger on negative DOT cleanse");
    }
    {
        Player player;
        Equip(player, "choirs_eternal_boots");
        const int base = player.getSpeed();
        Require(!player.ApplyStatus({StatusType::Slow, 2, 50}, 100) &&
            !player.HasStatus(StatusType::Slow) && player.getSpeed() > base,
            "Boots Slow immunity/SpeedUp");
        player.ApplyStatus({StatusType::Slow, 2, 50}, 100);
        Require(player.getSpeed() == CombatMath::ApplyPercent(base, 25), "Boots speed duplicated");
        std::vector<std::pair<StatusType, int>> damage;
        int healing = 0;
        player.BeginStatusTurn(damage, healing);
        player.EndStatusTurn(true);
        Require(player.getSpeed() == CombatMath::ApplyPercent(base, 25), "Boots expired after one action");
        player.BeginStatusTurn(damage, healing);
        player.EndStatusTurn(true);
        Require(player.getSpeed() == base, "Boots survived two normal actions");
    }
    {
        Player player;
        Equip(player, "choirs_eternal_reliquary");
        player.heal(1);
        Require(player.GetBarrier() == 0, "Choir ordinary healing made barrier");
        player.takeDamage(1);
        player.heal(1000);
        Require(player.GetBarrier() > 0 && player.GetBarrier() <= player.getMaxHP(),
            "Choir overheal conversion/cap");
        const int barrier = player.GetBarrier();
        player.heal(1000);
        Require(player.GetBarrier() == barrier, "Choir barrier exceeded cap");
    }
    {
        Player player;
        Equip(player, "first_light_sandals");
        const int base = player.getSpeed();
        for (int index = 0; index < 15; ++index) player.BeginArtifactTurn();
        Require(player.GetArtifactSpeedStacks() == 10 && player.getSpeed() == CombatMath::ApplyPercent(base, 30),
            "Sandals speed cap");
        player.ApplyStatus({StatusType::SpeedUp, 2, 40});
        Require(player.getSpeed() == CombatMath::ApplyPercent(base, 70), "Sandals plus SpeedUp");
    }
    {
        Player player;
        Equip(player, "first_light_crucible");
        const int base = player.getAttack();
        for (int index = 0; index < 15; ++index) player.AddArtifactAttackStack();
        Require(player.GetArtifactAttackStacks() == 10 && player.getAttack() == CombatMath::ApplyPercent(base, 50),
            "Crucible attack cap");
        player.ApplyStatus({StatusType::AttackUp, 2, 40});
        Require(player.getAttack() == CombatMath::ApplyPercent(base, 90), "Crucible plus AttackUp");
    }
    {
        Player player;
        Equip(player, "first_light_sanctum_attire");
        const int base = player.getDefense();
        for (int index = 0; index < 15; ++index) player.AddArtifactDefenseStack();
        Require(player.GetArtifactDefenseStacks() == 10 && player.getDefense() == CombatMath::ApplyPercent(base, 50),
            "Sanctum defense cap");
    }
}

void CheckSetStats() {
    for (const auto& set : {"Verdigris Matron", "Shimmer Reliquary", "Oracle Automaton"}) {
        Player player;
        auto pieces = EquipSet(player, set, 3);
        Require(player.GetSetPieceCount(set) == 3 && player.HasActiveSet(set), "3-piece threshold");
        if (std::string(set) == "Verdigris Matron") Require(player.getDotPotency() >= 35, "Verdigris DOT potency");
        if (std::string(set) == "Shimmer Reliquary") Require(player.getBarrierPotency() >= 40 &&
            player.getHealingPotency() >= 25, "Shimmer Reliquary 3-piece stats");
        const Stats preview = player.PreviewUnequip(pieces.back()->getSlot());
        Require(player.ToggleEquip(*pieces.back()) && !player.HasActiveSet(set), "3-piece unequip");
        Require(preview.dotPotency == player.getDotPotency() && preview.barrierPotency == player.getBarrierPotency() &&
            preview.spd == player.getSpeed(), "set preview/unequip drift");
    }
    {
        Player player;
        Equip(player, "verdigris_halo_of_saint_adelaide");
        Equip(player, "verdigris_saint_vestment");
        Equip(player, "verdigris_saint_step");
        int legendary = 0;
        for (const auto& definition : BuildEquippableCatalog()) {
            if (definition.setName != "Shimmer Ascendant" ||
                (definition.slot != EquipSlot::Hand && definition.slot != EquipSlot::Accessory)) continue;
            auto item = std::make_unique<Item>(definition);
            Item& reference = *item;
            player.AddItem(std::move(item));
            Require(player.Equip(reference), "Artifact 3 + Legendary 2 equip");
            ++legendary;
        }
        Require(legendary == 2 && player.GetSetPieceCount("Verdigris Matron") == 3 &&
            player.GetSetPieceCount("Shimmer Ascendant") == 2 &&
            player.getDotPotency() >= 35, "Artifact 3 + Legendary 2 mixed thresholds");
    }
    {
        Player player;
        Item& sandals = Equip(player, "first_light_sandals");
        player.BeginArtifactTurn();
        Require(player.GetArtifactSpeedStacks() == 1, "equipment counter fixture");
        const Stats preview = player.PreviewUnequip(EquipSlot::Boots);
        player.ToggleEquip(sandals);
        Require(preview.spd == player.getSpeed(), "Artifact preview retained unequipped stack");
        player.Equip(sandals);
        Require(player.GetArtifactSpeedStacks() == 0, "re-equip retained battle stack");
        const Stats baseline = player.GetEffectiveStats();
        for (int index = 0; index < 20; ++index) {
            Require(player.ToggleEquip(sandals) && player.Equip(sandals), "repeat Artifact switch");
            const Stats actual = player.GetEffectiveStats();
            Require(actual.hp == baseline.hp && actual.spd == baseline.spd &&
                actual.atk == baseline.atk && actual.def == baseline.def,
                "repeated Artifact switching drifted stats");
        }
    }
}

void CheckDirectCombatHooks() {
    {
        Player player("Adelaide", 1000);
        Equip(player, "verdigris_saint_vestment");
        auto enemy = Target(1000000, 100);
        CombatManager combat(4);
        Require(Act(combat, player, enemy, CombatAction::Attack), "Vestment setup attack");
        combat.enemyTurn(player, enemy);
        Require(Has(enemy.GetStatuses(), StatusType::Bleed), "Vestment retaliation Bleed");
    }
    {
        Player player("Adelaide", 1000);
        Equip(player, "first_light_sanctum_attire");
        auto enemy = Target(1000000, 100);
        CombatManager combat(4);
        Require(Act(combat, player, enemy, CombatAction::Attack), "Sanctum setup attack");
        combat.enemyTurn(player, enemy);
        Require(player.GetArtifactDefenseStacks() == 1, "Sanctum direct-hit stack");
    }
    {
        Player player;
        Equip(player, "locket_crown_of_the_first_light");
        Stats critical; critical.critRate = 100;
        player.ModifyStats(critical);
        player.takeDamage(30);
        auto enemy = Target();
        CombatManager combat(2);
        auto skill = ShimmerSkill::Create("Acus Fulgoris");
        Require(Act(combat, player, enemy, CombatAction::Shimmer, skill.get()), "Crown setup Shimmer");
        const int cooldown = combat.GetSkillCooldown(skill->getName());
        Require(Act(combat, player, enemy, CombatAction::Attack), "Crown critical attack");
        Require(std::any_of(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.kind == CombatManager::CombatEvent::Kind::Heal &&
                    event.message.find("First Light critical") != std::string::npos && event.value > 0;
            }) && combat.GetSkillCooldown(skill->getName()) == cooldown - 2,
            "Crown first critical heal/cooldown: HP " + std::to_string(player.getHP()) +
            "/" + std::to_string(player.getMaxHP()) + " cooldown " +
            std::to_string(combat.GetSkillCooldown(skill->getName())) + "/" + std::to_string(cooldown));
    }
    {
        Player player;
        Equip(player, "verdigris_saint_reliquary");
        player.RestoreHealth(player.getMaxHP());
        auto enemy = Target();
        Require(enemy.ApplyStatus({StatusType::Burn, 3, 0, player.getAttack(), 0, 0,
            player.getName(), player.GetCombatantId()}), "DOT healing fixture");
        CombatManager combat(14);
        Require(Act(combat, player, enemy, CombatAction::Attack), "DOT healing setup attack");
        const int healthBeforeTick = player.getHP();
        combat.enemyTurn(player, enemy);
        Require(std::any_of(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.kind == CombatManager::CombatEvent::Kind::Shield &&
                    event.message.find("DOT overhealing") != std::string::npos;
            }), "Saint Reliquary DOT overheal barrier: ticks " +
            std::to_string(EventCount(combat, CombatManager::CombatEvent::Kind::DotTick)) +
            " shields " + std::to_string(EventCount(combat, CombatManager::CombatEvent::Kind::Shield)) +
            " preHP " + std::to_string(healthBeforeTick) + "/" + std::to_string(player.getMaxHP()) +
            " dotHP " + std::to_string(enemy.GetStatusDamageResults().empty() ? -1 :
                enemy.GetStatusDamageResults().front().damage.hpDamage));
    }
    {
        Player player;
        auto pieces = EquipSet(player, "Verdigris Matron", 5);
        Require(player.HasArtifact("verdigris_halo_of_saint_adelaide"), "Verdigris Halo fixture");
        auto enemy = Target();
        CombatManager combat(15);
        Require(Act(combat, player, enemy, CombatAction::Attack) &&
            Has(enemy.GetStatuses(), StatusType::VerdigrisMark), "Verdigris five-piece attack Mark");
        Require(enemy.ApplyStatus({StatusType::Burn, 3, 0, player.getAttack(), player.getDotPotency(), 0,
            player.getName(), player.GetCombatantId()}), "Verdigris detonation DOT fixture");
        int cooldownBeforeDetonation = -1;
        for (const char* name : {"Acus Fulgoris", "Pulsus Luminis", "Nota Luminosa"}) {
            if (std::string(name) == "Nota Luminosa") {
                combat.PrepareTurn(player, enemy);
                for (int wait = 0; wait < 50 && !combat.IsPlayerTurn(); ++wait)
                    combat.enemyTurn(player, enemy);
                cooldownBeforeDetonation = combat.GetSkillCooldown("Pulsus Luminis");
            }
            auto skill = ShimmerSkill::Create(name);
            Require(Act(combat, player, enemy, CombatAction::Shimmer, skill.get()), "Verdigris marked Shimmer");
        }
        Require(!Has(enemy.GetStatuses(), StatusType::Burn), "Verdigris detonation left DOT ticks active");
        Require(std::any_of(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.kind == CombatManager::CombatEvent::Kind::Heal &&
                    event.message.find("DOT damage") != std::string::npos && event.value > 0;
            }), "Verdigris three-piece actual-HP DOT healing");
        Require(std::any_of(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.message.find("reduces ") != std::string::npos &&
                    event.message.find("cooldown") != std::string::npos;
            }), "Halo detonation cooldown reduction");
        // One scheduled turn-start tick occurs as resolve prepares the third
        // action; the other decrement is the single Halo detonation trigger.
        Require(cooldownBeforeDetonation > 1 &&
            combat.GetSkillCooldown("Pulsus Luminis") == cooldownBeforeDetonation - 2,
            "Halo did not reduce the longest offensive cooldown by exactly one: Pulsus " +
            std::to_string(combat.GetSkillCooldown("Pulsus Luminis")) + " Acus " +
            std::to_string(combat.GetSkillCooldown("Acus Fulgoris")) + " before " +
            std::to_string(cooldownBeforeDetonation));
    }
    {
        Player player("Adelaide", 1000);
        EquipSet(player, "Shimmer Reliquary", 3);
        auto enemy = Target(1000000, 100);
        CombatManager combat(5);
        Require(player.ApplyStatus({StatusType::Poison, 3, 0, 50, 0, 0, "Enemy", enemy.GetCombatantId()}),
            "barrier-break poison fixture");
        Require(player.ApplyStatus({StatusType::Burn, 3, 0, 50, 0, 0, "Enemy", enemy.GetCombatantId()}),
            "barrier-break burn fixture");
        player.ApplyStatus({StatusType::Barrier, 0, 1});
        Require(Act(combat, player, enemy, CombatAction::Attack), "barrier-break setup action");
        combat.enemyTurn(player, enemy);
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::BarrierBreak) >= 1 &&
            EventCount(combat, CombatManager::CombatEvent::Kind::Cleanse) >= 1,
            "Shimmer three-piece barrier-break response");
        Require(static_cast<int>(Has(player.GetStatuses(), StatusType::Poison)) +
            static_cast<int>(Has(player.GetStatuses(), StatusType::Burn)) == 1,
            "barrier break did not cleanse exactly one negative status");
    }
    {
        Player player;
        Equip(player, "choirs_eternal_tome");
        auto enemy = Target();
        CombatManager combat(11);
        Require(Act(combat, player, enemy, CombatAction::Attack), "Tome attack");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::Critical) == 1,
            "Tome did not guarantee first direct critical");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::Miss) == 0, "Tome test missed");
        Require(Act(combat, player, enemy, CombatAction::Attack), "Tome second attack");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::Critical) == 1,
            "Tome guaranteed more than first attack");
    }
    {
        bool observedMissThenCritical = false;
        for (unsigned seed = 1; seed <= 60 && !observedMissThenCritical; ++seed) {
            Player player;
            Equip(player, "choirs_eternal_tome");
            EnemyDefinition definition{"Evasive test target", EnemyFaction::Devotee,
                EnemyTier::Basic, 1000000, 1, 0, 1, 1};
            definition.evasion = 45;
            Enemy enemy(definition, 1);
            CombatManager combat(seed);
            if (!Act(combat, player, enemy, CombatAction::Attack) ||
                EventCount(combat, CombatManager::CombatEvent::Kind::Miss) == 0) continue;
            for (int attempt = 0; attempt < 12 && EventCount(combat, CombatManager::CombatEvent::Kind::Hit) +
                EventCount(combat, CombatManager::CombatEvent::Kind::Critical) == 0; ++attempt)
                Require(Act(combat, player, enemy, CombatAction::Attack), "Tome post-miss attack");
            observedMissThenCritical = EventCount(combat, CombatManager::CombatEvent::Kind::Critical) == 1;
        }
        Require(observedMissThenCritical, "Tome miss consumed guaranteed critical");
    }
    {
        Player player;
        Equip(player, "verdigris_saint_relic");
        auto enemy = Target();
        CombatManager combat(19);
        Require(Act(combat, player, enemy, CombatAction::Attack) &&
            Has(enemy.GetStatuses(), StatusType::VerdigrisMark), "Relic direct Mark");
        Require(enemy.ApplyStatus({StatusType::Burn, 3, 0, player.getAttack(), 0, 0,
            player.getName(), player.GetCombatantId()}), "Burn fixture");
        const auto& statuses = enemy.GetStatuses();
        const auto burn = std::find_if(statuses.begin(), statuses.end(), [](const StatusEffect& status) {
            return status.type == StatusType::Burn;
        });
        Require(burn != statuses.end() && StatusRules::DotDamage(*burn, enemy.getMaxHP(), statuses) ==
            CombatMath::Scale(CombatMath::Scale(player.getAttack(), 60), 120), "Relic same-source DOT mark");
        for (int turn = 0; turn < 3; ++turn) enemy.ProcessStatuses();
        Require(!Has(enemy.GetStatuses(), StatusType::VerdigrisMark), "Relic Mark did not expire");
    }
    {
        Player player;
        Equip(player, "verdigris_saint_step");
        Equip(player, "locket_crown_of_the_first_light");
        Stats critical; critical.critRate = 100;
        player.ModifyStats(critical);
        auto enemy = Target();
        CombatManager combat(7);
        for (int action = 0; action < 6; ++action)
            Require(Act(combat, player, enemy, CombatAction::Attack), "Saint Step normal action");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::ExtraTurn) == 1 &&
            combat.IsPlayerTurn(), "Saint Step sixth-action bonus");
        const int crownBeforeBonus = static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.message.find("First Light critical") != std::string::npos;
            }));
        Require(combat.resolve(CombatAction::Attack, player, enemy), "Saint Step bonus attack");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::ExtraTurn) == 1,
            "Saint Step bonus counted as sixth normal action");
        Require(static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) {
                return event.message.find("First Light critical") != std::string::npos;
            })) == crownBeforeBonus, "Bonus action reset once-per-turn Crown");
    }
    {
        Player player;
        Equip(player, "first_light_crucible");
        auto enemy = Target();
        CombatManager combat(8);
        auto skill = ShimmerSkill::Create("Acus Fulgoris");
        Require(Act(combat, player, enemy, CombatAction::Shimmer, skill.get()) &&
            player.GetArtifactAttackStacks() == 1, "Crucible original cast stack");
        Require(!Act(combat, player, enemy, CombatAction::Shimmer, skill.get()) &&
            player.GetArtifactAttackStacks() == 1, "Crucible failed cast stacked");
    }
    {
        Player player;
        Equip(player, "first_light_reliquary");
        EquipSet(player, "Shimmer Ascendant", 3);
        Require(player.GetShimmerDamageMultiplierPercent() == 135,
            "Reliquary plus Ascendant fixture");
        auto enemy = Target();
        CombatManager combat(9);
        const char* names[] = {"Acus Fulgoris", "Pulsus Luminis", "Nota Luminosa", "Fulgur Aurorae"};
        for (int index = 0; index < 3; ++index) {
            auto skill = ShimmerSkill::Create(names[index]);
            Require(Act(combat, player, enemy, CombatAction::Shimmer, skill.get()), "Reliquary arming cast");
        }
        const int beforeEcho = static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) { return event.skill.find("echo") != std::string::npos; }));
        Require(beforeEcho == 0, "Reliquary echoed third cast instead of next");
        auto fourth = ShimmerSkill::Create(names[3]);
        Require(Act(combat, player, enemy, CombatAction::Shimmer, fourth.get()), "Reliquary echo cast");
        const int afterEcho = static_cast<int>(std::count_if(combat.GetEvents().begin(), combat.GetEvents().end(),
            [](const CombatManager::CombatEvent& event) { return event.skill.find("echo") != std::string::npos; }));
        Require(afterEcho == 1, "Reliquary echo missing or recursive");
        int originalHP = -1, echoHP = -1;
        for (const auto& event : combat.GetEvents()) {
            if (event.kind != CombatManager::CombatEvent::Kind::Damage) continue;
            if (event.skill == names[3]) originalHP = event.hpDamage;
            if (event.skill == std::string(names[3]) + " echo") echoHP = event.hpDamage;
        }
        Require(originalHP > 0 && echoHP == CombatMath::Scale(originalHP, 40),
            "Reliquary echo was not 40% of Ascendant-amplified actual HP damage");
    }
    {
        Player player;
        EquipSet(player, "Shimmer Reliquary", 5);
        auto enemy = Target();
        CombatManager combat(10);
        auto skill = ShimmerSkill::Create("Acus Fulgoris");
        Require(!Act(combat, player, enemy, CombatAction::Shimmer),
            "Invalid Shimmer unexpectedly succeeded");
        Require(Act(combat, player, enemy, CombatAction::Shimmer, skill.get()) &&
            combat.GetSkillCooldown(skill->getName()) == 0, "Reliquary first Shimmer cooldown");
        auto barrierSkill = ShimmerSkill::Create("Claustrum Lucis");
        const int before = player.GetBarrier();
        barrierSkill->apply(player, enemy);
        Require(player.GetBarrier() > before && player.GetBarrier() >=
            CombatMath::Scale(CombatMath::Scale(player.getMaxHP(), 1500), 150),
            "Reliquary five-piece barrier amplification");
    }
    {
        Player player;
        EquipSet(player, "Oracle Automaton", 5);
        auto enemy = Target();
        CombatManager combat(12);
        const int originalEnemySpeed = enemy.getSpeed();
        combat.BeginEncounter(player, enemy, 1);
        combat.PrepareTurn(player, enemy);
        Require(enemy.getSpeed() == CombatMath::Scale(originalEnemySpeed, 75),
            "Oracle persistent enemy speed suppression");
        const int before = EventCount(combat, CombatManager::CombatEvent::Kind::Buff);
        for (int index = 0; index < 3; ++index)
            Require(Act(combat, player, enemy, CombatAction::Attack), "Oracle normal action");
        Require(EventCount(combat, CombatManager::CombatEvent::Kind::Buff) > before,
            "Oracle third-action gauge bonus");
    }
}

void CheckDeveloperMode() {
    Require(DeveloperMode::IsDeveloperName("") && DeveloperMode::IsDeveloperName("  ") &&
        DeveloperMode::IsDeveloperName("Adelaide") && DeveloperMode::IsDeveloperName("adelaide") &&
        !DeveloperMode::IsDeveloperName("Clara"), "developer name trigger");
    const auto rainbow = ConsoleFormat::RainbowName("Adelaide");
    Require(rainbow.find("\x1b[91mA") != std::string::npos &&
        rainbow.find("\x1b[93md") != std::string::npos &&
        rainbow.find("\x1b[0m") != std::string::npos &&
        ConsoleFormat::RainbowName("Clara") == "Clara", "Adelaide-only rainbow output");
    Player player;
    MapManager map;
    ProgressionManager progression;
    DeveloperMode::Initialize(player, progression, map);
    Require(progression.GetLevel() == 100 && player.getHP() == player.getMaxHP(),
        "developer level/health");
    progression.GrantXP(1000000);
    Require(!progression.CheckLevelUp(player) && progression.GetLevel() == 100 && progression.GetXP() == 0,
        "developer level exceeded cap or accepted XP");
    for (const auto& room : map.GetRooms())
        Require(room.IsUnlocked() && room.AreAllRequiredInteractablesRevealed() &&
            !room.HasBossBeenDefeated(), "developer room access without forced clear");
    Require(map.GetRooms().front().IsHealingUnlocked(), "developer Fountain access");
    Require(map.GetRooms().at(2).IsRustedLockUnlocked(), "developer Archive cabinet access");
    for (const auto& definition : BuildEquippableCatalog())
        Require(player.FindItem(definition.name) != nullptr, "developer equipment unlock " + definition.name);
    for (const auto& definition : BuildConsumableCatalog())
        Require(player.FindItem(definition.name) != nullptr, "developer consumable unlock " + definition.name);
    for (const auto& definition : BuildClueCatalog())
        Require(player.FindItem(definition.name) != nullptr, "developer clue unlock " + definition.name);
    for (const auto& definition : BuildCruxCatalog())
        Require(player.FindItem(definition.name) != nullptr && progression.HasCrux(definition.name),
            "developer Crux unlock " + definition.name);
    for (const auto& name : BuildLatinShimmerSkillNames())
        Require(progression.HasShimmerSkill(name), "developer Shimmer unlock " + name);
    std::set<unsigned long long> instanceIds;
    std::set<Rarity> equipmentRarities;
    int artifactCount = 0;
    for (const auto& entry : player.GetInventory().GetItems()) {
        Require(entry.item && instanceIds.insert(entry->getInstanceId()).second,
            "developer inventory duplicate instance ID");
        if (entry->getCategory() == ItemCategory::Equippable) {
            equipmentRarities.insert(entry->getRarity());
            artifactCount += entry->getRarity() == Rarity::Artifact;
        }
    }
    Require(equipmentRarities.size() == 6 && artifactCount == 15,
        "developer inventory missing an equipment rarity or Artifact unique");
    const auto initializedItems = player.GetInventory().GetItemCount();
    DeveloperMode::Initialize(player, progression, map);
    Require(player.GetInventory().GetItemCount() == initializedItems,
        "developer initialization duplicated catalogue items");
    for (const auto& definition : BuildEquippableCatalog()) {
        Item* item = player.FindItem(definition.name);
        Require(item && player.Equip(*item) && player.ToggleEquip(*item),
            "developer catalogue item cannot equip/unequip " + definition.name);
    }
    const auto beforeItems = player.GetInventory().GetItemCount();
    const auto& room = MapManager::GetFloor1Definitions().at(4);
    const auto sample = DeveloperMode::SampleRoom(room, 100, DeveloperDropSource::Boss, 5000, 13);
    Require(sample.rolls == 5000 && sample.gear > 16000 && sample.gear < 19000 &&
        sample.rarity[static_cast<int>(Rarity::Artifact)] > 500 &&
        player.GetInventory().GetItemCount() == beforeItems,
        "developer boss simulator distribution or side effect");
    const auto cache = DeveloperMode::SampleRoom(room, 100, DeveloperDropSource::Investigation, 100, 42);
    Require(cache.rolls == 100 && cache.gear + cache.consumables == 100 &&
        cache.rarity[static_cast<int>(Rarity::Legendary)] == 0 &&
        cache.rarity[static_cast<int>(Rarity::Artifact)] == 0,
        "developer investigation simulator rarity gate");
    Require(!DeveloperMode::RollRoom(room, 100, DeveloperDropSource::Boss, 13).empty(),
        "developer single boss roll");
    bool rejected = false;
    try { DeveloperMode::RollRoom(MapManager::GetFloor1Definitions().front(), 100,
        DeveloperDropSource::Boss, 1); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "Fountain boss simulator should be unavailable");
}

void CheckConsolePresentation() {
    const auto capped = ConsoleFormat::PlayerPanel("Clara", 100, 80, 100, 25, 12, 9, 7, 0, 100, false);
    Require(capped.find("Clara  Lv. 100") != std::string::npos &&
        capped.find("MAX LEVEL") != std::string::npos && capped.find("[DEV]") == std::string::npos,
        "level-cap player panel");
    const auto developer = ConsoleFormat::PlayerPanel("Adelaide", 100, 100, 100, 0, 12, 9, 7, 0, 100, true);
    Require(developer.find("[DEV]") != std::string::npos && developer.find("Barrier") == std::string::npos,
        "developer marker or collapsed zero barrier");
    const auto normal = ConsoleFormat::PlayerPanel("Clara", 4, 75, 100, 20, 12, 9, 7, 30, 100, false);
    Require(normal.find("HP       [#########---] 75 / 100") != std::string::npos &&
        normal.find("Barrier  [##----------] 20") != std::string::npos &&
        normal.find("XP       30 / 100") != std::string::npos, "player panel bars");
    Require(ConsoleFormat::CooldownText(0) == "READY" && ConsoleFormat::CooldownText(1) == "CD 1 turn" &&
        ConsoleFormat::CooldownText(3) == "CD 3 turns", "cooldown labels");
    Require(ConsoleFormat::MenuOption(5, "Chapel", "defeat the previous guardian", false).find(
        "[LOCKED - defeat the previous guardian]") != std::string::npos, "locked menu reason");
    Require(ConsoleFormat::RarityLabel(Rarity::Artifact) == "[Artifact]", "Artifact rarity label");
    Require(ConsoleFormat::ClampPage(99, 17, 8) == 2 && ConsoleFormat::ClampPage(4, 0, 8) == 0 &&
        ConsoleFormat::ClampPage(4, 10, 0) == 0, "inventory pagination bounds");
    Stats before;
    before.hp = 100; before.atk = 20; before.critRate = 5;
    Stats after = before;
    after.hp = 135; after.atk = 18; after.critRate = 15;
    const auto changes = ConsoleFormat::StatChanges(before, after);
    Require(changes.find("HP  100 -> 135  (+35)") != std::string::npos &&
        changes.find("ATK  20 -> 18  (-2)") != std::string::npos &&
        changes.find("Crit Rate  5 -> 15  (+10pp)") != std::string::npos &&
        changes.find("DEF") == std::string::npos, "effective stat comparison");
    const auto set = ConsoleFormat::SetProgress("Verdigris Matron", 3, 5, 3, 5, "Authored bonus.");
    Require(set.find("[3/5]") != std::string::npos && set.find("3-piece ACTIVE") != std::string::npos &&
        set.find("5-piece LOCKED") != std::string::npos && set.find("Authored bonus.") != std::string::npos,
        "mixed-threshold set presentation");
    const std::vector<StatusEffect> statuses{{StatusType::Burn, 2}, {StatusType::AttackUp, 1, 40}};
    const auto summary = ConsoleFormat::StatusSummary(statuses);
    Require(summary.find("Burn (2)") != std::string::npos && summary.find("ATK up (1)") != std::string::npos &&
        summary.find("turn") == std::string::npos, "compact status labels");
}
}

void RunArtifactRegression() {
    CheckCatalogueAndEquip();
    CheckPlayerHooks();
    CheckSetStats();
    CheckDirectCombatHooks();
    CheckDeveloperMode();
    CheckConsolePresentation();
    std::cout << "Artifact catalogue, player hooks, caps, and set-stat regression passed.\n";
}

#ifdef ARTIFACT_STANDALONE
int main() {
    try {
        RunArtifactRegression();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
#endif
