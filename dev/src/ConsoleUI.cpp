#include "ConsoleUI.h"
#include "Area.h"
#include "CombatManager.h"
#include "EnemyCatalog.h"
#include "Item.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

namespace {
bool fastPresentation = false;
const char* SlotName(EquipSlot slot) {
    static const char* names[] = {"Head", "Torso", "Boots", "Hand", "Accessory", "None"};
    return names[static_cast<int>(slot)];
}
const char* RarityName(Rarity rarity) {
    static const char* names[] = {"Common", "Uncommon", "Rare", "Epic", "Legendary", "Artifact"};
    return names[static_cast<int>(rarity)];
}
}

void ConsoleUI::SetFast(bool fast) { fastPresentation = fast; }
bool ConsoleUI::IsFast() { return fastPresentation; }

void ConsoleUI::PrintPlayerPanel(const Player& player, const ProgressionManager& progression) const {
    std::cout << player.getName() << " | Level " << progression.GetLevel()
              << " | HP " << player.getHP() << '/' << player.getMaxHP()
              << " | ATK " << player.getAttack() << " DEF " << player.getDefense()
              << " SPD " << player.getSpeed() << " | XP " << progression.GetXP()
              << '/' << progression.GetXPToNextLevel() << '\n';
}

void ConsoleUI::RenderDashboard(const Player& player, const ProgressionManager& progression,
                                const std::vector<Area>& rooms, const std::string& context) const {
    std::cout << "\n=== MICARE: CATHEDRAL OF LIES ===\n";
    PrintPlayerPanel(player, progression);
    std::cout << context << '\n';
    for (std::size_t index = 0; index < rooms.size(); ++index) {
        std::cout << index + 1 << ". " << rooms[index].getName();
        if (!rooms[index].IsUnlocked()) std::cout << " [LOCKED]";
        else if (rooms[index].HasBossBeenDefeated()) std::cout << " [CLEARED - farming available]";
        std::cout << '\n';
    }
    std::cout << "6. Inventory\n7. Shimmer skills\n8. Floor map\n9. Last encounter log\n0. Quit\n";
}

void ConsoleUI::RenderRoom(const Area& area, const Player& player, const ProgressionManager& progression) const {
    std::cout << "\n=== ROOM: " << area.getName() << " ===\n";
    PrintPlayerPanel(player, progression);
    for (const auto& object : area.GetVisibleInteractables()) std::cout << "  Discovered: " << object << '\n';
    if (area.HasBossBeenDefeated()) std::cout << "First clear complete. Repeat rewards and farming encounters are available.\n";
    else if (area.HasAllClues() && area.AreAllRequiredInteractablesRevealed() && !area.IsHealingRoom()) {
        for (const auto& room : MapManager::GetFloor1Definitions()) if (room.id == area.getName())
            std::cout << "Guardian eligible: " << room.bossId << " | fixed level " << room.firstClearBossLevel
                      << " | suggested readiness " << room.intendedProgressionLevel << ".\n";
    }
    std::cout << "1. Investigate\n2. Search room\n3. Inventory\n4. Floor map\n5. Shimmer skills\n6. Last encounter log\n";
    if (area.IsHealingRoom() && area.IsHealingUnlocked()) std::cout << "7. Heal at the Fountain\n";
    std::cout << "0. Leave room\n";
}

void ConsoleUI::RenderCombat(const Player& player, const ProgressionManager& progression,
                             const Enemy& enemy, const CombatManager& combat) const {
    PrintPlayerPanel(player, progression);
    std::cout << "Enemy: " << enemy.getName() << " | Level " << enemy.getLevel() << ' ' << EnemyTierName(enemy.getTier())
              << " | HP " << enemy.getHP() << '/' << enemy.getMaxHP() << " DEF " << enemy.getDefense()
              << " SPD " << enemy.getSpeed() << '\n';
    std::cout << "Acting: " << (combat.IsPlayerTurn() ? player.getName() : enemy.getName()) << '\n';
    if (combat.IsPlayerTurn())
        std::cout << "1. Attack\n2. Consumable\n3. Shimmer\n4. Flee\n5. Full encounter log\n6. Toggle fast presentation\n";
}

void ConsoleUI::PresentEvents(const CombatManager& combat) const {
    for (const auto& event : combat.GetEvents()) {
        if (event.sequenceId <= lastEventSequence_) continue;
        std::cout << '[' << event.sequenceId << "] " << event.message << '\n' << std::flush;
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
    for (const auto& event : combat.GetEvents()) std::cout << '[' << event.sequenceId << "] " << event.message << '\n';
    std::cout << "=== END ENCOUNTER LOG ===\n";
}

void ConsoleUI::ShowDefeat(const Player& player, const CombatManager& combat) const {
    PresentEvents(combat);
    std::cout << "\n=== DEFEAT: " << player.getName() << " ===\n";
    const auto& events = combat.GetEvents();
    for (auto event = events.rbegin(); event != events.rend(); ++event) {
        if (event->target == player.getName() && event->remainingHP == 0 && event->hpDamage > 0) {
            std::cout << "Final cause: " << event->actor << " / " << event->skill << ". " << event->message << '\n';
            break;
        }
    }
    for (std::size_t index = events.size() > 8 ? events.size() - 8 : 0; index < events.size(); ++index)
        std::cout << '[' << events[index].sequenceId << "] " << events[index].message << '\n';
    while (std::cin) {
        const int choice = Prompt("Defeat review: 1 full log, 0 acknowledge and return to the Fountain.", 0, 1);
        if (choice == 0) break;
        ShowBattleHistory(combat);
    }
}

void ConsoleUI::ManageInventory(Player& player) const {
    std::size_t page = 0;
    InventorySort sort = InventorySort::RecentlyObtained;
    while (std::cin) {
        auto items = player.GetInventory().GetSortedItems(sort);
        if (items.empty()) { ShowMessage("Inventory is empty."); return; }
        page = (std::min)(page, (items.size() - 1) / 12);
        const std::size_t first = page * 12;
        const std::size_t count = (std::min)(std::size_t{12}, items.size() - first);
        std::cout << "\n=== INVENTORY page " << page + 1 << '/' << (items.size() + 11) / 12 << " ===\n";
        for (std::size_t index = 0; index < count; ++index) {
            const Item& item = *items[first + index];
            std::cout << index + 1 << ". " << item.getName() << " [" << RarityName(item.getRarity()) << "]";
            if (item.isEquippable()) {
                std::cout << " " << SlotName(item.getSlot()) << " " << item.getMainStat() << "+"
                          << (item.getMainStatPercent() ? item.getMainStatPercent() : item.getMainStatValue())
                          << (item.getMainStatPercent() ? "%" : "") << " instance=" << item.getInstanceId();
                if (player.GetEquipped(item.getSlot()) == &item) std::cout << " [EQUIPPED]";
            } else std::cout << " x" << player.GetInventory().GetQuantity(item.getName());
            if (player.GetInventory().IsLocked(item.getName())) std::cout << " [LOCKED]";
            std::cout << '\n';
        }
        const int choice = Prompt("Inventory: select item; 13 next, 14 previous, 15 sort (newest/rarity/name), 0 return.", 0, 15);
        if (choice == 0) return;
        if (choice == 13) { if (first + count < items.size()) ++page; continue; }
        if (choice == 14) { if (page) --page; continue; }
        if (choice == 15) {
            sort = sort == InventorySort::RecentlyObtained ? InventorySort::RarityDescending :
                sort == InventorySort::RarityDescending ? InventorySort::AlphabeticalAscending : InventorySort::RecentlyObtained;
            page = 0; continue;
        }
        if (choice > static_cast<int>(count)) { ShowMessage("That item is not on this page."); continue; }
        Item& item = *items[first + choice - 1];
        ShowMessage(item.getName() + ": " + item.getDescription());
        const auto& stats = item.getStats();
        std::cout << "Stats: ATK+" << stats.atk << " HP+" << stats.hp << " DEF+" << stats.def << " SPD+" << stats.spd
                  << " ATK%+" << stats.atkPercent << " HP%+" << stats.hpPercent << " DEF%+" << stats.defPercent
                  << " CRIT+" << stats.critRate << " ACC+" << stats.acc << '\n';
        if (!item.getPassive().empty()) ShowMessage(item.getPassive());
        if (!item.getSetPassive().empty()) ShowMessage(item.getSetName() + ": " + item.getSetPassive());
        const int action = Prompt("Item action: 1 equip/unequip or use, 2 lock/unlock name, 3 discard instance, 0 cancel.", 0, 3);
        const std::string name = item.getName();
        if (action == 1) {
            if (item.isEquippable()) ShowMessage(player.ToggleEquip(item) ? "Equipment updated: " + name : "Cannot equip this item.");
            else if (item.getCategory() == ItemCategory::Consumable) ShowMessage(player.UseConsumable(name) ? "Used " + name : "This item needs a combat target.");
            else ShowMessage(item.getDescription());
        } else if (action == 2) {
            player.GetInventory().SetLocked(name, !player.GetInventory().IsLocked(name));
        } else if (action == 3) {
            ShowMessage(player.GetInventory().DiscardItem(item.getInstanceId()) ? "Discarded " + name : "Equipped or locked items cannot be discarded.");
        }
    }
}

void ConsoleUI::ManageShimmerSkills(ProgressionManager& progression) const {
    PresentLevelUpRewards(progression);
    while (std::cin) {
        const auto unlocked = progression.GetUnlockedShimmerSkills();
        const auto equipped = progression.GetEquippedShimmerSkills();
        std::cout << "\n=== SHIMMER SKILLS | free points " << progression.GetShimmerPoints() << "/12 ===\n";
        for (std::size_t index = 0; index < unlocked.size(); ++index) {
            const auto skill = ShimmerSkill::Create(unlocked[index]);
            if (!skill) continue;
            std::cout << index + 1 << ". " << skill->getName() << " cost " << skill->getCost() << " cooldown " << skill->getCooldown();
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
    std::cout << "\n=== FLOOR I MAP ===\nFountain -> Entrance Hall -> Archive Room -> Chapel -> Storage Room\n";
    for (const auto& room : rooms) std::cout << room.getName() << (room.IsUnlocked() ? " [OPEN]" : " [LOCKED]")
        << (room.HasBossBeenDefeated() ? " [CLEARED]" : "") << '\n';
}

void ConsoleUI::ShowMessage(const std::string& message) const { std::cout << message << '\n'; }

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
        std::cout << "Enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
}
