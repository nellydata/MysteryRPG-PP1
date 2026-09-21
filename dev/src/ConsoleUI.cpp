#include "ConsoleUI.h"
#include "Area.h"
#include "CombatManager.h"
#include "ConsoleFormat.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

namespace {
bool fastPresentation = false;
}

void ConsoleUI::SetFast(bool fast) { fastPresentation = fast; }
bool ConsoleUI::IsFast() { return fastPresentation; }

void ConsoleUI::PrintPlayerPanel(const Player& player, const ProgressionManager& progression, bool developerMode) const {
    std::cout << ConsoleFormat::PlayerPanel(ConsoleFormat::RainbowName(player.getName()), progression.GetLevel(),
        player.getHP(), player.getMaxHP(), player.GetBarrier(), player.getAttack(), player.getDefense(),
        player.getSpeed(), progression.GetXP(), progression.GetXPToNextLevel(), developerMode);
}

void ConsoleUI::RenderDashboard(const Player& player, const ProgressionManager& progression,
                                const std::vector<Area>& rooms, const std::string& context,
                                bool developerMode) const {
    std::cout << '\n' << ConsoleFormat::Title("MICARE: CATHEDRAL OF LIES");
    PrintPlayerPanel(player, progression, developerMode);
    std::cout << ConsoleFormat::Note(context) << "\n\n" << ConsoleFormat::Section("DESTINATIONS");
    for (std::size_t index = 0; index < rooms.size(); ++index) {
        std::string note;
        if (!rooms[index].IsUnlocked()) note = index == 0 ? "progress further" : "defeat the previous guardian";
        else if (rooms[index].HasBossBeenDefeated()) note = "CLEARED - farming available";
        std::cout << ConsoleFormat::MenuOption(static_cast<int>(index + 1), rooms[index].getName(), note,
            rooms[index].IsUnlocked());
    }
    std::cout << '\n' << ConsoleFormat::Section("PANELS")
              << ConsoleFormat::MenuOption(6, "Inventory")
              << ConsoleFormat::MenuOption(7, "Shimmer skills")
              << ConsoleFormat::MenuOption(8, "Floor map")
              << ConsoleFormat::MenuOption(9, "Last encounter log");
    if (developerMode) std::cout << ConsoleFormat::MenuOption(10, "Room drop simulator", "DEV");
    std::cout << ConsoleFormat::MenuOption(0, "Quit");
}

void ConsoleUI::RenderRoom(const Area& area, const Player& player, const ProgressionManager& progression) const {
    // Keep the stable ROOM marker consumed by console automation while the
    // divider supplies the shared visual language.
    std::cout << "\n=== ROOM: " << area.getName() << " ===\n" << ConsoleFormat::Divider('=') << '\n';
    PrintPlayerPanel(player, progression);
    static const std::map<std::string, std::string> descriptions = {
        {"Fountain", "Cold water gathers beneath a silent cathedral arch."},
        {"Entrance Hall", "Broken saints watch the nave from dust and shadow."},
        {"Archive Room", "Shelves climb into darkness around a rusted cabinet."},
        {"Chapel", "Old prayers cling to the altar and empty benches."},
        {"Storage Room", "Crates and reliquaries crowd the cathedral's final chamber."}
    };
    const auto description = descriptions.find(area.getName());
    if (description != descriptions.end()) std::cout << '\n' << description->second << '\n';
    std::cout << "Discoveries  " << area.GetVisibleInteractables().size() << " / " << area.GetInteractables().size() << '\n';
    for (const auto& object : area.GetVisibleInteractables()) std::cout << "  - " << object << '\n';
    if (area.IsIlluminated()) {
        std::cout << "The room is illuminated by your lumen capsule. Objects in view:\n";
        for (const auto& object : area.GetInteractables()) std::cout << "  " << object << '\n';
    }
    if (area.IsRustedLockUnlocked()) std::cout << ConsoleFormat::Success("The cabinet's rusted lock is open.") << '\n';
    if (area.HasBossBeenDefeated()) std::cout << "Guardian     CLEARED - repeat encounters available\n";
    else if (area.HasAllClues() && area.AreAllRequiredInteractablesRevealed() && !area.IsHealingRoom()) {
        for (const auto& room : MapManager::GetFloor1Definitions()) if (room.id == area.getName())
            std::cout << "Guardian     READY - " << room.bossId << " (Lv. " << room.firstClearBossLevel
                      << ", suggested " << room.intendedProgressionLevel << ")\n";
    }
    else if (!area.IsHealingRoom()) std::cout << "Guardian     Unknown - complete room discoveries\n";
    std::cout << '\n' << ConsoleFormat::Section("ACTIONS")
              << ConsoleFormat::MenuOption(1, "Investigate")
              << ConsoleFormat::MenuOption(2, "Search room")
              << ConsoleFormat::MenuOption(3, "Inventory")
              << ConsoleFormat::MenuOption(4, "Floor map")
              << ConsoleFormat::MenuOption(5, "Shimmer skills")
              << ConsoleFormat::MenuOption(6, "Last encounter log");
    if (area.IsHealingRoom()) std::cout << ConsoleFormat::MenuOption(7, "Heal at the Fountain",
        area.IsHealingUnlocked() ? "READY" : "defeat the Fountain guardian", area.IsHealingUnlocked());
    std::cout << ConsoleFormat::MenuOption(0, "Leave room");
}

void ConsoleUI::RenderCombat(const Player& player, const ProgressionManager& progression,
                             const Enemy& enemy, const CombatManager& combat) const {
    std::cout << '\n' << ConsoleFormat::Title("COMBAT") << ConsoleFormat::Section("PLAYER")
              << ConsoleFormat::RainbowName(player.getName()) << "  Lv. " << progression.GetLevel() << '\n'
              << "HP       " << ConsoleFormat::HealthBar(player.getHP(), player.getMaxHP()) << '\n';
    if (player.GetBarrier() > 0) std::cout << "Barrier  " << ConsoleFormat::ProgressBar(player.GetBarrier(), player.getMaxHP(), 12, false) << '\n';
    std::cout << "Status   " << ConsoleFormat::StatusSummary(player.GetStatuses()) << '\n'
              << "Gauge    " << ConsoleFormat::ProgressBar(combat.GetPlayerGauge(), 1000, 12) << "\n\n"
              << ConsoleFormat::Section("ENEMY") << enemy.getName() << "  Lv. " << enemy.getLevel()
              << "  [" << EnemyTierName(enemy.getTier()) << "]\n"
              << "HP       " << ConsoleFormat::HealthBar(enemy.getHP(), enemy.getMaxHP()) << '\n';
    if (enemy.GetBarrier() > 0) std::cout << "Barrier  " << ConsoleFormat::ProgressBar(enemy.GetBarrier(), enemy.getMaxHP(), 12, false) << '\n';
    std::cout << "Status   " << ConsoleFormat::StatusSummary(enemy.GetStatuses()) << '\n'
              << "Gauge    " << ConsoleFormat::ProgressBar(combat.GetEnemyGauge(), 1000, 12) << '\n';
    if (enemy.getTier() == EnemyTier::Boss) std::cout << "Phase    " << enemy.GetPhase() << '\n';
    std::cout << "\nActing   " << (combat.IsPlayerTurn() ? ConsoleFormat::RainbowName(player.getName()) : enemy.getName()) << "\n"
              << ConsoleFormat::Section("SHIMMERS");
    const auto equipped = progression.GetEquippedShimmerSkills();
    if (equipped.empty()) {
        std::cout << "  None equipped\n";
    } else {
        for (std::size_t index = 0; index < equipped.size(); ++index) {
            const int cooldown = combat.GetSkillCooldown(equipped[index]);
            std::cout << "  " << equipped[index] << "  [" << ConsoleFormat::CooldownText(cooldown) << "]\n";
        }
    }
    if (combat.IsPlayerTurn())
        std::cout << '\n' << ConsoleFormat::Section("ACTIONS")
                  << ConsoleFormat::MenuOption(1, "Attack") << ConsoleFormat::MenuOption(2, "Consumable")
                  << ConsoleFormat::MenuOption(3, "Shimmer") << ConsoleFormat::MenuOption(4, "Flee")
                  << ConsoleFormat::MenuOption(5, "Full encounter log")
                  << ConsoleFormat::MenuOption(6, "Toggle fast presentation");
}

void ConsoleUI::PresentEvents(const CombatManager& combat) const {
    for (const auto& event : combat.GetEvents()) {
        if (event.sequenceId <= lastEventSequence_) continue;
        const char* label = "EVENT";
        switch (event.kind) {
        case CombatManager::CombatEvent::Kind::Critical: label = "CRIT"; break;
        case CombatManager::CombatEvent::Kind::Heal: label = "HEAL"; break;
        case CombatManager::CombatEvent::Kind::Shield: label = "WARD"; break;
        case CombatManager::CombatEvent::Kind::BarrierBreak: label = "BREAK"; break;
        case CombatManager::CombatEvent::Kind::DotTick: label = "DOT"; break;
        case CombatManager::CombatEvent::Kind::Buff: label = "BUFF"; break;
        case CombatManager::CombatEvent::Kind::Debuff: label = "DEBUFF"; break;
        case CombatManager::CombatEvent::Kind::Resistance: label = "RESIST"; break;
        case CombatManager::CombatEvent::Kind::Cleanse: label = "CLEANSE"; break;
        case CombatManager::CombatEvent::Kind::PhaseTransition: label = "PHASE"; break;
        case CombatManager::CombatEvent::Kind::Death: label = "DEATH"; break;
        case CombatManager::CombatEvent::Kind::Shimmer: label = "SHIMMER"; break;
        case CombatManager::CombatEvent::Kind::Reward: label = "REWARD"; break;
        default: break;
        }
        std::cout << '[' << label << "] " << ConsoleFormat::RainbowAdelaideIn(event.message) << '\n' << std::flush;
        lastEventSequence_ = event.sequenceId;
        // Console presentation alone waits; the resolver never uses wall-clock time.
        if (!fastPresentation && (event.kind == CombatManager::CombatEvent::Kind::Attack ||
            event.kind == CombatManager::CombatEvent::Kind::Skill || event.kind == CombatManager::CombatEvent::Kind::Shimmer))
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void ConsoleUI::ShowBattleHistory(const CombatManager& combat) const {
    std::cout << "\n=== FULL ENCOUNTER LOG ===\n";
    if (combat.GetEvents().empty()) std::cout << "No encounter recorded.\n";
    for (const auto& event : combat.GetEvents()) std::cout << '[' << event.sequenceId << "] " << ConsoleFormat::RainbowAdelaideIn(event.message) << '\n';
    std::cout << "=== END ENCOUNTER LOG ===\n";
}

void ConsoleUI::ShowDefeat(const Player& player, const CombatManager& combat) const {
    PresentEvents(combat);
    std::cout << "\n=== DEFEAT: " << ConsoleFormat::RainbowName(player.getName()) << " ===\n";
    const auto& events = combat.GetEvents();
    for (auto event = events.rbegin(); event != events.rend(); ++event) {
        if (event->target == player.getName() && event->remainingHP == 0 && event->hpDamage > 0) {
            std::cout << "Final cause: " << event->actor << " / " << event->skill << ". " <<
                ConsoleFormat::RainbowAdelaideIn(event->message) << '\n';
            break;
        }
    }
    for (std::size_t index = events.size() > 8 ? events.size() - 8 : 0; index < events.size(); ++index)
        std::cout << '[' << events[index].sequenceId << "] " <<
            ConsoleFormat::RainbowAdelaideIn(events[index].message) << '\n';
    while (std::cin) {
        const int choice = Prompt("Defeat review: 1 full log, 0 acknowledge and return to the Fountain.", 0, 1);
        if (choice == 0) break;
        ShowBattleHistory(combat);
    }
}

void ConsoleUI::ManageInventory(Player& player, CombatManager* combat, Area* area) const {
    enum class View { Equipment = 1, Consumables, KeyItems, Clues };
    constexpr std::size_t pageSize = 8;
    const auto categoryFor = [](View view) {
        return view == View::Equipment ? ItemCategory::Equippable : view == View::Consumables ?
            ItemCategory::Consumable : view == View::KeyItems ? ItemCategory::Crux : ItemCategory::Clue;
    };
    const auto viewName = [](View view) {
        return view == View::Equipment ? "EQUIPMENT" : view == View::Consumables ?
            "CONSUMABLES" : view == View::KeyItems ? "KEY ITEMS" : "CLUES";
    };
    const auto countCategory = [&](ItemCategory category) {
        std::size_t count = 0;
        for (const auto& entry : player.GetInventory().GetItems())
            if (entry.item && entry->getCategory() == category) count += static_cast<std::size_t>(entry.quantity);
        return count;
    };

    while (std::cin) {
        std::cout << '\n' << ConsoleFormat::Title("INVENTORY")
                  << "Equipment    " << countCategory(ItemCategory::Equippable) << '\n'
                  << "Consumables  " << countCategory(ItemCategory::Consumable) << '\n'
                  << "Key items    " << countCategory(ItemCategory::Crux) << '\n'
                  << "Clues        " << countCategory(ItemCategory::Clue) << "\n\n"
                  << ConsoleFormat::MenuOption(1, "Equipment")
                  << ConsoleFormat::MenuOption(2, "Consumables")
                  << ConsoleFormat::MenuOption(3, "Key items")
                  << ConsoleFormat::MenuOption(4, "Clues")
                  << ConsoleFormat::MenuOption(0, "Back");
        const int selectedView = Prompt("Choose an inventory category.", 0, 4);
        if (selectedView == 0) return;
        const View view = static_cast<View>(selectedView);
        std::size_t page = 0;
        InventorySort sort = view == View::Equipment ? InventorySort::RarityDescending : InventorySort::RecentlyObtained;
        int equipmentFilter = 0; // 0 all, 1..5 slots, 6 equipped only
        int rarityFilter = -1;
        std::string setFilter;

        while (std::cin) {
            auto sorted = player.GetInventory().GetSortedItems(sort, ItemStat::ATK, player.GetEffectiveStats());
            std::vector<Item*> items;
            for (Item* item : sorted) {
                if (!item || item->getCategory() != categoryFor(view)) continue;
                if (view == View::Equipment) {
                    if (equipmentFilter >= 1 && equipmentFilter <= 5 &&
                        static_cast<int>(item->getSlot()) != equipmentFilter - 1) continue;
                    if (equipmentFilter == 6 && player.GetEquipped(item->getSlot()) != item) continue;
                    if (rarityFilter >= 0 && static_cast<int>(item->getRarity()) != rarityFilter) continue;
                    if (!setFilter.empty() && item->getSetName() != setFilter) continue;
                }
                items.push_back(item);
            }
            page = ConsoleFormat::ClampPage(page, items.size(), pageSize);
            const std::size_t pages = (std::max)(std::size_t{1}, (items.size() + pageSize - 1) / pageSize);
            const std::size_t first = page * pageSize;
            const std::size_t shown = first < items.size() ? (std::min)(pageSize, items.size() - first) : 0;
            std::cout << '\n' << ConsoleFormat::PageHeader(viewName(view), page + 1, pages);
            if (view == View::Equipment) {
                const char* filters[] = {"All", "Head", "Torso", "Boots", "Hand", "Accessory", "Equipped only"};
                std::cout << "Filter: " << filters[equipmentFilter];
                if (rarityFilter >= 0) std::cout << " | " << ConsoleFormat::RarityName(static_cast<Rarity>(rarityFilter));
                if (!setFilter.empty()) std::cout << " | Set: " << setFilter;
                std::cout << '\n';
            }
            if (shown == 0) std::cout << ConsoleFormat::Note("No items match this view.") << '\n';
            for (std::size_t index = 0; index < shown; ++index) {
                const Item& item = *items[first + index];
                std::cout << '[' << index + 1 << "] " << item.getName();
                if (item.getCategory() == ItemCategory::Equippable) {
                    std::cout << "\n    " << ConsoleFormat::RarityName(item.getRarity()) << " | "
                              << ConsoleFormat::SlotName(item.getSlot());
                    if (!item.getSetName().empty()) std::cout << " | " << item.getSetName();
                    std::cout << "\n    " << item.getMainStat() << " +"
                              << (item.getMainStatPercent() ? item.getMainStatPercent() : item.getMainStatValue())
                              << (item.getMainStatPercent() ? "%" : "");
                    if (player.GetEquipped(item.getSlot()) == &item) std::cout << "  [EQUIPPED]";
                } else {
                    std::cout << ' ' << ConsoleFormat::RarityLabel(item.getRarity()) << " x"
                              << player.GetInventory().GetQuantity(item.getInstanceId());
                }
                if (player.GetInventory().IsLocked(item.getInstanceId())) std::cout << " [LOCKED]";
                std::cout << '\n';
                if (!item.isEquippable()) std::cout << "    " << item.getDescription() << '\n';
            }
            std::cout << "\n[9] Next  [10] Previous  [11] Sort";
            if (view == View::Equipment) std::cout << "  [12] Filter";
            std::cout << "  [0] Categories\n";
            const int choice = Prompt("Select an item or navigation action.", 0, view == View::Equipment ? 12 : 11);
            if (choice == 0) break;
            if (choice == 9) { if (page + 1 < pages) ++page; else ShowMessage(ConsoleFormat::Note("Already on the final page.")); continue; }
            if (choice == 10) { if (page > 0) --page; else ShowMessage(ConsoleFormat::Note("Already on the first page.")); continue; }
            if (choice == 11) {
                sort = sort == InventorySort::RarityDescending ? InventorySort::AlphabeticalAscending :
                    sort == InventorySort::AlphabeticalAscending ? InventorySort::RecentlyObtained : InventorySort::RarityDescending;
                page = 0; continue;
            }
            if (choice == 12 && view == View::Equipment) {
                std::cout << "\n[1] All  [2] Head  [3] Torso  [4] Boots  [5] Hand  [6] Accessory\n"
                          << "[7] Equipped only  [8] Rarity  [9] Set  [0] Cancel\n";
                const int filter = Prompt("Choose equipment filter.", 0, 9);
                if (filter >= 1 && filter <= 7) { equipmentFilter = filter == 1 ? 0 : filter - 1; rarityFilter = -1; setFilter.clear(); }
                else if (filter == 8) {
                    const int rarity = Prompt("Rarity: 1 Common, 2 Uncommon, 3 Rare, 4 Epic, 5 Legendary, 6 Artifact, 0 cancel.", 0, 6);
                    if (rarity) { equipmentFilter = 0; rarityFilter = rarity - 1; setFilter.clear(); }
                } else if (filter == 9) {
                    std::vector<std::string> sets;
                    for (Item* item : sorted) if (item && item->isEquippable() && !item->getSetName().empty() &&
                        std::find(sets.begin(), sets.end(), item->getSetName()) == sets.end()) sets.push_back(item->getSetName());
                    for (std::size_t i = 0; i < sets.size(); ++i) std::cout << '[' << i + 1 << "] " << sets[i] << '\n';
                    const int set = Prompt("Choose set, 0 cancel.", 0, static_cast<int>(sets.size()));
                    if (set) { equipmentFilter = 0; rarityFilter = -1; setFilter = sets[set - 1]; }
                }
                page = 0; continue;
            }
            if (choice > static_cast<int>(shown)) { ShowMessage(ConsoleFormat::Warning("That item is not on this page.")); continue; }

            Item& item = *items[first + choice - 1];
            while (std::cin && player.GetInventory().Contains(item.getInstanceId())) {
                const bool equipped = item.isEquippable() && player.GetEquipped(item.getSlot()) == &item;
                std::cout << '\n' << ConsoleFormat::Title(item.getName())
                          << ConsoleFormat::RarityLabel(item.getRarity()) << '\n';
                if (item.isEquippable()) {
                    std::cout << "Slot: " << ConsoleFormat::SlotName(item.getSlot()) << '\n';
                    if (!item.getSetName().empty()) std::cout << "Set: " << item.getSetName() << '\n';
                    std::cout << "State: " << (equipped ? "EQUIPPED" : "UNEQUIPPED") << "\n\n"
                              << ConsoleFormat::Section("STATS");
                    for (const auto& line : item.GetStatLines()) std::cout << line << '\n';
                    if (!item.getPassive().empty()) std::cout << '\n' << ConsoleFormat::Section(
                        item.getRarity() == Rarity::Artifact ? "UNIQUE" : "PASSIVE") << item.getPassive() << '\n';
                    const Stats before = player.GetEffectiveStats();
                    const Stats after = equipped ? player.PreviewUnequip(item.getSlot()) : player.PreviewEquipment(item);
                    std::cout << '\n' << ConsoleFormat::Section(equipped ? "UNEQUIP CHANGES" : "EQUIP CHANGES")
                              << ConsoleFormat::StatChanges(before, after);
                    if (!item.getSetName().empty()) {
                        const Item* replaced = player.GetEquipped(item.getSlot());
                        const int current = player.GetSetPieceCount(item.getSetName());
                        int projected = current + (equipped ? -1 : 1);
                        if (!equipped && replaced && replaced->getSetName() == item.getSetName()) --projected;
                        const bool artifactSet = item.getRarity() == Rarity::Artifact;
                        std::cout << '\n' << ConsoleFormat::Section("SET BONUS")
                                  << ConsoleFormat::SetProgress(item.getSetName(), current,
                                      artifactSet ? 5 : 3, artifactSet ? 3 : 2, artifactSet ? 5 : 3,
                                      item.getSetPassive())
                                  << "After action: " << current << " -> " << projected << " pieces\n";
                        for (const int threshold : {artifactSet ? 3 : 2, artifactSet ? 5 : 3}) {
                            if (current < threshold && projected >= threshold)
                                std::cout << threshold << "-piece bonus ACTIVATES\n";
                            else if (current >= threshold && projected < threshold)
                                std::cout << threshold << "-piece bonus DEACTIVATES\n";
                        }
                    }
                    if (!equipped && player.GetEquipped(item.getSlot())) {
                        const Item* replaced = player.GetEquipped(item.getSlot());
                        std::cout << "Replacing: " << replaced->getName() << '\n';
                        if (!replaced->getSetName().empty() && replaced->getSetName() != item.getSetName()) {
                            const int oldSetCount = player.GetSetPieceCount(replaced->getSetName());
                            const bool oldArtifactSet = replaced->getRarity() == Rarity::Artifact;
                            std::cout << ConsoleFormat::SetProgress(replaced->getSetName(), oldSetCount,
                                oldArtifactSet ? 5 : 3, oldArtifactSet ? 3 : 2, oldArtifactSet ? 5 : 3,
                                replaced->getSetPassive()) << "After action: " << oldSetCount << " -> "
                                << oldSetCount - 1 << " pieces\n";
                            for (const int threshold : {oldArtifactSet ? 3 : 2, oldArtifactSet ? 5 : 3})
                                if (oldSetCount >= threshold && oldSetCount - 1 < threshold)
                                    std::cout << threshold << "-piece bonus DEACTIVATES\n";
                        }
                    }
                } else {
                    std::cout << item.getDescription() << "\nQuantity: "
                              << player.GetInventory().GetQuantity(item.getInstanceId()) << '\n';
                    if (item.getCategory() == ItemCategory::Consumable)
                        std::cout << ConsoleFormat::Note(combat ? "Combat context available; target rules still apply."
                            : area ? "Exploration context available; combat-only effects remain unavailable."
                            : "No combat or room context; some uses may be unavailable.") << '\n';
                }
                std::cout << "\n[1] " << (item.isEquippable() ? (equipped ? "Unequip" : "Equip") :
                    item.getCategory() == ItemCategory::Consumable ? "Use" : "Read")
                          << "  [2] " << (player.GetInventory().IsLocked(item.getInstanceId()) ? "Unlock" : "Lock");
                if (item.getCategory() != ItemCategory::Clue && item.getCategory() != ItemCategory::Crux) std::cout << "  [3] Discard";
                std::cout << "  [0] Back\n";
                const bool storyItem = item.getCategory() == ItemCategory::Clue || item.getCategory() == ItemCategory::Crux;
                const int action = Prompt("Choose item action.", 0, storyItem ? 2 : 3);
                if (action == 0) break;
                const std::string name = item.getName();
                const auto id = item.getInstanceId();
                if (action == 1 && item.isEquippable()) {
                    const int confirm = Prompt(std::string(equipped ? "Unequip " : "Equip ") + name + "? 1 confirm, 0 cancel.", 0, 1);
                    if (confirm) { ShowMessage(player.ToggleEquip(item) ? ConsoleFormat::Success("Equipment updated: " + name) : ConsoleFormat::Error("Cannot equip this item.")); break; }
                } else if (action == 1 && item.getCategory() == ItemCategory::Consumable) {
                    const bool used = player.UseConsumable(id, nullptr, combat, area);
                    ShowMessage(used ? ConsoleFormat::Success("Used " + name) : ConsoleFormat::Warning(
                        "Cannot use this item here; its target or context requirement is not met."));
                    if (used) break;
                } else if (action == 1 && storyItem) {
                    ShowMessage(item.getDescription());
                } else if (action == 2) player.GetInventory().SetLocked(id, !player.GetInventory().IsLocked(id));
                else if (action == 3 && item.getCategory() != ItemCategory::Clue && item.getCategory() != ItemCategory::Crux) {
                    if (player.GetInventory().DiscardItem(id)) { ShowMessage(ConsoleFormat::Success("Discarded " + name)); break; }
                    ShowMessage(ConsoleFormat::Warning("Equipped or locked items cannot be discarded."));
                }
            }
        }
    }
}

void ConsoleUI::ManageShimmerSkills(ProgressionManager& progression) const {
    PresentLevelUpRewards(progression);
    while (std::cin) {
        const auto unlocked = progression.GetUnlockedShimmerSkills();
        const auto equipped = progression.GetEquippedShimmerSkills();
        std::cout << '\n' << ConsoleFormat::Title("SHIMMER SKILLS")
                  << "Free points  " << progression.GetShimmerPoints() << " / 12\n\n";
        for (std::size_t index = 0; index < unlocked.size(); ++index) {
            const auto skill = ShimmerSkill::Create(unlocked[index]);
            if (!skill) continue;
            std::cout << '[' << index + 1 << "] " << skill->getName() << "  Cost " << skill->getCost()
                      << "  CD " << skill->getCooldown();
            if (std::find(equipped.begin(), equipped.end(), unlocked[index]) != equipped.end()) std::cout << " [EQUIPPED]";
            std::cout << "\n   " << skill->getDescription() << '\n';
        }
        const int choice = Prompt("Shimmer loadout: select skill to equip/unequip, 0 return.", 0, static_cast<int>(unlocked.size()));
        if (choice == 0) return;
        const auto& name = unlocked[choice - 1];
        const auto skill = ShimmerSkill::Create(name);
        if (!skill) continue;
        const bool worn = std::find(equipped.begin(), equipped.end(), name) != equipped.end();
        const bool changed = worn ? progression.UnequipShimmerSkill(name) : progression.EquipShimmerSkill(name);
        ShowMessage(changed ? "Shimmer loadout updated." : "Not enough free Shimmer points.");
    }
}

void ConsoleUI::ShowMap(const std::vector<Area>& rooms) const {
    std::cout << '\n' << ConsoleFormat::Title("FLOOR I")
              << "Fountain -> Entrance Hall -> Archive Room -> Chapel -> Storage Room\n\n";
    for (std::size_t index = 0; index < rooms.size(); ++index) {
        std::string note = rooms[index].HasBossBeenDefeated() ? "CLEARED" : rooms[index].IsUnlocked() ? "OPEN" : "defeat the previous guardian";
        std::cout << ConsoleFormat::MenuOption(static_cast<int>(index + 1), rooms[index].getName(), note,
            rooms[index].IsUnlocked());
    }
}

void ConsoleUI::ShowMessage(const std::string& message) const {
    std::cout << ConsoleFormat::RainbowAdelaideIn(message) << '\n';
}

void ConsoleUI::PresentLevelUpRewards(ProgressionManager& progression) const {
    for (const auto& message : progression.ConsumeLevelUpMessages()) ShowMessage(message);
    while (std::cin && progression.HasPendingShimmerChoice()) if (!ChoosePendingShimmerSkill(progression)) break;
    for (const auto& message : progression.ConsumeLevelUpMessages()) ShowMessage(message);
}

bool ConsoleUI::ChoosePendingShimmerSkill(ProgressionManager& progression) const {
    if (!progression.HasPendingShimmerChoice()) return true;
    const auto choices = progression.GetPendingShimmerChoices();
    if (choices.empty()) return progression.ResolvePendingShimmerChoice();
    std::cout << "\nLevel-up Shimmer reward:\n";
    for (std::size_t index = 0; index < choices.size(); ++index) std::cout << index + 1 << ". " << choices[index] << '\n';
    const int choice = Prompt("Choose a level-up skill, or 0 to decide later.", 0, static_cast<int>(choices.size()));
    return choice != 0 && progression.SelectPendingShimmerSkill(choices[choice - 1]);
}

int ConsoleUI::Prompt(const std::string& prompt, int minimum, int maximum) const {
    for (;;) {
        std::cout << '\n' << prompt << "\n> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) return 0;
        std::istringstream input(line);
        int choice;
        char extra;
        if (input >> choice && !(input >> extra) && choice >= minimum && choice <= maximum) return choice;
        std::cout << ConsoleFormat::Warning("Enter a whole number from " + std::to_string(minimum) +
            " to " + std::to_string(maximum) + ".") << '\n';
    }
}
