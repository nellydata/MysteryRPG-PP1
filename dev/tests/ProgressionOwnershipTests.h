#pragma once
#include "ContentCatalog.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <iostream>
#include <set>

inline void TestProgressionOwnership() {
    ProgressionManager loadout;
    assert(!loadout.UnlockShimmerSkill("unknown skill"));
    assert(!loadout.EquipShimmerSkill("Pulsus Luminis"));
    assert(loadout.GetShimmerPoints() == 12);
    assert(loadout.UnlockShimmerSkill("Refractio Ultima"));
    assert(!loadout.EquipShimmerSkill("Refractio Ultima", 1));
    assert(!loadout.EquipShimmerSkill("Refractio Ultima", -1));
    const int realCost = ShimmerSkill::Create("Refractio Ultima")->getCost();
    assert(loadout.EquipShimmerSkill("Refractio Ultima"));
    assert(!loadout.EquipShimmerSkill("Refractio Ultima"));
    assert(loadout.GetShimmerPoints() == 12 - realCost);
    assert(!loadout.UnequipShimmerSkill("Refractio Ultima", 100));
    assert(loadout.GetShimmerPoints() == 12 - realCost);
    assert(loadout.UnequipShimmerSkill("Refractio Ultima"));
    assert(!loadout.UnequipShimmerSkill("Refractio Ultima"));
    assert(loadout.GetShimmerPoints() == 12);
    int used = 0;
    for (const auto& name : BuildLatinShimmerSkillNames()) {
        loadout.UnlockShimmerSkill(name);
        const int cost = ShimmerSkill::Create(name)->getCost();
        const bool fits = used + cost <= 12;
        assert(loadout.EquipShimmerSkill(name) == fits);
        if (fits) used += cost;
        assert(loadout.GetShimmerPoints() == 12 - used);
    }
    assert(used <= 12 && used > 0);

    Player player;
    ProgressionManager rewards;
    rewards.GrantXP(1000);
    assert(rewards.CheckLevelUp(player));
    assert(player.GetInventorySize() == static_cast<std::size_t>(5 * (rewards.GetLevel() - 1)));
    std::set<unsigned long long> instances;
    int slots[5]{};
    for (const auto& entry : player.GetInventory().GetItems()) {
        assert(entry->isEquippable() && entry->getSlot() != EquipSlot::None);
        assert(instances.insert(entry->getInstanceId()).second);
        ++slots[static_cast<int>(entry->getSlot())];
    }
    for (int count : slots) assert(count == rewards.GetLevel() - 1);
    const auto firstChoices = rewards.GetPendingShimmerChoices();
    assert(firstChoices.size() == 3);
    assert(std::set<std::string>(firstChoices.begin(), firstChoices.end()).size() == 3);
    // Deferral leaves the reward unchanged; a later external unlock refreshes it.
    assert(rewards.GetPendingShimmerChoices() == firstChoices);
    assert(rewards.UnlockShimmerSkill(firstChoices.front()));
    int resolved = 0;
    while (rewards.HasPendingShimmerChoice() && resolved < 100) {
        const auto& choices = rewards.GetPendingShimmerChoices();
        assert(choices.size() <= 3);
        for (const auto& name : choices) {
            assert(!rewards.HasShimmerSkill(name));
            assert(ShimmerSkill::Create(name));
        }
        if (choices.empty()) assert(rewards.ResolvePendingShimmerChoice());
        else assert(rewards.SelectPendingShimmerSkill(choices.front()));
        ++resolved;
    }
    assert(!rewards.HasPendingShimmerChoice() && resolved == rewards.GetLevel() - 1);
    for (const auto& name : BuildLatinShimmerSkillNames()) rewards.UnlockShimmerSkill(name);
    rewards.GrantXP(rewards.GetXPToNextLevel());
    assert(rewards.CheckLevelUp(player));
    assert(rewards.HasPendingShimmerChoice() && rewards.GetPendingShimmerChoices().empty());
    assert(rewards.ResolvePendingShimmerChoice());
    assert(!rewards.HasPendingShimmerChoice());

    Player fallen;
    ProgressionManager pendingXP;
    pendingXP.GrantXP(100);
    fallen.takeDamage(fallen.getHP());
    assert(!pendingXP.CheckLevelUp(fallen));
    assert(!fallen.isAlive() && pendingXP.GetLevel() == 1 && pendingXP.GetXP() == 100);
    fallen.RestoreHealth(fallen.getMaxHP());
    assert(pendingXP.CheckLevelUp(fallen) && pendingXP.GetLevel() == 2);
    ProgressionManager largeXP;
    const auto limit = (std::numeric_limits<std::int64_t>::max)();
    largeXP.GrantXP(limit - 1);
    largeXP.GrantXP(100);
    assert(largeXP.GetXP() == limit);
    assert(XPRequirementForLevel((std::numeric_limits<int>::max)()) == XPRequirementForLevel(100));
    largeXP.SetMaxLevel(fallen);
    largeXP.GrantXP(limit);
    assert(largeXP.GetLevel() == 100 && largeXP.GetXP() == 0);
    assert(!largeXP.CheckLevelUp(fallen));
    std::cout << "Progression ownership regressions passed.\n";
}
