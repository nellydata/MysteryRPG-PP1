#include "ConsoleUI.h"
#include "Area.h"
#include "CombatManager.h"
#include "Enemy.h"
#include "GameplayTypes.h"
#include "Inventory.h"
#include "Item.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "ShimmerSkill.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
const char* Reset() { return "\x1b[0m"; }
const char* Brass() { return "\x1b[38;5;179m"; }
const char* Verdigris() { return "\x1b[38;5;115m"; }
const char* Shimmer() { return "\x1b[38;5;159m"; }
const char* Muted() { return "\x1b[38;5;245m"; }

std::string RarityName(Rarity rarity) {
	switch (rarity) {
	case Rarity::Common: return "Common";
	case Rarity::Uncommon: return "Uncommon";
	case Rarity::Rare: return "Rare";
	case Rarity::Epic: return "Epic";
	case Rarity::Legendary: return "Legendary";
	case Rarity::Artifact: return "Artifact";
	}
	return "Unknown";
}

std::string SlotName(EquipSlot slot) {
	switch (slot) {
	case EquipSlot::Head: return "Head";
	case EquipSlot::Torso: return "Torso";
	case EquipSlot::Boots: return "Boots";
	case EquipSlot::Hand: return "Hand";
	case EquipSlot::Accessory: return "Accessory";
	case EquipSlot::None: return "None";
	}
	return "None";
}

bool HasEnvironmentValue(const char* name, const char* expected = nullptr) {
	char* value = nullptr;
	std::size_t length = 0;
	if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return false;
	const bool matches = expected == nullptr || std::string(value) == expected;
	std::free(value);
	return matches;
}
}

ConsoleUI::ConsoleUI()
	: colorEnabled_(!HasEnvironmentValue("MICARE_NO_COLOR")) {}

void ConsoleUI::Clear() const {
	std::cout << "\x1b[2J\x1b[H";
}

void ConsoleUI::PrintTitle(const std::string& title) const {
	if (colorEnabled_) std::cout << Brass();
	std::cout << "+----------------------------------------------------------------------+\n"
			  << "| " << title << "\n"
			  << "+----------------------------------------------------------------------+\n";
	if (colorEnabled_) std::cout << Reset();
}

void ConsoleUI::PrintRule() const {
	if (colorEnabled_) std::cout << Muted();
	std::cout << "------------------------------------------------------------------------\n";
	if (colorEnabled_) std::cout << Reset();
}

void ConsoleUI::RenderDashboard(const Player& player, const ProgressionManager& progression,
								const std::vector<Area>& rooms,
								const std::string& context) const {
	Clear();
	PrintTitle("MICARE  //  THE CATHEDRAL OF SHIMMER");
	PrintPlayerPanel(player, progression);
	PrintRule();
	if (colorEnabled_) std::cout << Shimmer();
	std::cout << "[ " << context << " ]\n";
	if (colorEnabled_) std::cout << Reset();
	std::cout << "Rooms on Floor I:\n";
	for (std::size_t index = 0; index < rooms.size(); ++index) {
		std::cout << "  " << index + 1 << ". " << rooms[index].getName();
		if (rooms[index].hasEnemies()) std::cout << "  [HOSTILE]";
		std::cout << "\n";
	}
	std::cout << "  5. Open inventory\n"
			  << "  6. Shimmer skill bar\n"
			  << "  7. View floor map\n";
	if (IsDeveloperMode()) std::cout << "  8. Developer mode\n";
	std::cout << "  0. Leave the cathedral\n";
}

void ConsoleUI::PrintPlayerPanel(const Player& player, const ProgressionManager& progression) const {
	if (colorEnabled_) std::cout << Verdigris();
	std::cout << "+- ADELAIDE // PLAYER PANEL -------------------------------------------+\n"
			  << "|        .-.       " << player.getName() << "  |  Level " << progression.GetLevel() << "\n"
			  << "|       (o o)      HP  " << player.getHP() << "/" << player.getMaxHP()
			  << "  DEF " << player.getDefense() << "  SPD " << player.getSpeed() << "\n"
				<< "|        |=|       ATK " << player.getAttack() << "  ACC " << player.getDisplayedAccuracy()
			  << "%  RES " << player.getResistance() << "%\n"
				<< "|       /| |\\      CRIT " << player.getCritRate() << "% / "
			  << player.getCritDamage() << "%  XP " << progression.GetXP() << "\n"
				<< "+----------------------------------------------------------------------+\n";
	if (colorEnabled_) std::cout << Reset();
	PrintStatusEffects(player);
	PrintEquipment(player);
}

void ConsoleUI::PrintStatusEffects(const Player& player) const {
	std::cout << "Statuses: ";
	if (player.GetStatuses().empty()) {
		std::cout << "none\n";
		return;
	}
	for (const auto& status : player.GetStatuses()) {
		std::cout << StatusName(static_cast<int>(status.type)) << " (" << status.turns << ")  ";
	}
	std::cout << "\n";
}

void ConsoleUI::PrintEquipment(const Player& player) const {
	std::cout << "Equipment: ";
	for (EquipSlot slot : {EquipSlot::Head, EquipSlot::Torso, EquipSlot::Boots, EquipSlot::Hand, EquipSlot::Accessory}) {
		const Item* item = player.GetEquipped(slot);
		std::cout << SlotName(slot) << "=" << (item == nullptr ? "--" : item->getName()) << "  ";
	}
	std::cout << "\n";
}

void ConsoleUI::RenderRoom(const Area& area, const Player& player,
						   const ProgressionManager& progression) const {
	Clear();
	PrintTitle("EXPLORATION  //  " + area.getName());
	PrintPlayerPanel(player, progression);
	PrintRule();
	std::cout << "+- ROOM VIEW -----------------------------------------------------------+\n"
			  << "| The cathedral holds its breath. Objects below may reveal clues.       |\n"
			  << "+----------------------------------------------------------------------+\n"
			  << "Objects and points of interest:\n";
	for (std::size_t index = 0; index < area.GetInteractables().size(); ++index) {
		std::cout << "  " << index + 1 << ". " << area.GetInteractables()[index] << "\n";
	}
	std::cout << "\nActions:\n  1. Inspect environment\n  2. Search room\n"
			  << "  3. Open inventory\n  4. View map\n  0. Leave room\n";
}

void ConsoleUI::RenderCombat(const Player& player, const ProgressionManager& progression,
							 const Enemy& enemy, const CombatManager& combat) const {
	Clear();
	PrintTitle("COMBAT  //  HOSTILE PRESENCE DETECTED");
	PrintPlayerPanel(player, progression);
	PrintRule();
	if (colorEnabled_) std::cout << Brass();
	std::cout << "ENEMY: " << enemy.getName() << "\n";
	if (colorEnabled_) std::cout << Reset();
	std::cout << "  HP " << enemy.getHP() << "/" << enemy.getMaxHP()
			  << "  ATK " << enemy.getAtk() << "  DEF " << enemy.getDefense()
			  << "  SPD " << enemy.getSpeed() << "\n"
			  << "Turn order: Adelaide (SPD " << player.getSpeed() << ")  >  "
			  << enemy.getName() << " (SPD " << enemy.getSpeed() << ")\n"
			  << "\nActions:\n"
			  << "  1. Attack       2. Block        3. Shimmer\n"
			  << "  4. Interrogate  5. Expose Weakness  6. Flee\n";
	if (const ShimmerSkill* skill = combat.GetDefaultSkill()) {
		std::cout << "\nSkill ready state: " << skill->getName() << " ["
				  << (skill->IsReady() ? "READY" : "COOLDOWN " + std::to_string(skill->getRemainingCooldown()))
				  << "]\n";
	}
}

void ConsoleUI::ShowInventory(const Player& player) const {
	Clear();
	PrintTitle("INVENTORY  //  EQUIPMENT AND EVIDENCE");
	std::cout << "Items:\n";
	if (player.GetInventory().GetItems().empty()) std::cout << "  -- empty --\n";
	for (const auto& item : player.GetInventory().GetItems()) {
		if (colorEnabled_) std::cout << (item->getRarity() == Rarity::Artifact ? Shimmer() : "");
		std::cout << "  " << item->getName() << "  [" << RarityName(item->getRarity()) << "]";
		if (item->isEquippable()) std::cout << "  slot=" << SlotName(item->getSlot());
		if (!item->getUniqueEffect().empty()) std::cout << "  * " << item->getUniqueEffect();
		std::cout << "\n";
		if (colorEnabled_) std::cout << Reset();
	}
	std::cout << "\nClues and crux items:\n";
	for (const auto& clue : player.GetInventory().GetClues()) std::cout << "  * " << clue.description << "\n";
	std::cout << "\nPress 0 to return.\n";
}

void ConsoleUI::ShowMap(const std::vector<Area>& rooms) const {
	Clear();
	PrintTitle("FLOOR I MAP  //  CATHEDRAL LOWER LEVELS");
	auto roomName = [&](std::size_t index, const std::string& fallback) -> std::string {
		return index < rooms.size() ? rooms[index].getName() : fallback;
	};
	std::cout << "                    [" << roomName(1, "CHAPEL") << "]\n"
			  << "                           |\n"
			  << "[" << roomName(0, "STORAGE") << "] ---- [" << roomName(2, "ENTRANCE HALL")
			  << "] ---- [" << roomName(3, "ARCHIVE") << "]\n\n"
			  << "Room status:\n";
	for (const auto& room : rooms) {
		std::cout << "  " << (room.hasEnemies() ? "[!]" : "[ ]") << " " << room.getName();
		if (!room.GetInteractables().empty()) std::cout << "  [inspect]";
		std::cout << "\n";
	}
	std::cout << "\nPress 0 to return.\n";
}

void ConsoleUI::ShowShimmerSkills(const CombatManager& combat) const {
	Clear();
	PrintTitle("SHIMMER SKILL BAR  //  LATIN RESONANCE");
	std::cout << "  Pulsus Luminis       CD 3   Reveal and strike\n"
			  << "  Velum Radiāns        CD 4   Negate the next attack\n"
			  << "  Gradus Coruscus      CD 2   Gain an immediate turn\n"
			  << "  Fulgur Aurorae       CD 5   Massive light attack\n"
			  << "  Flos Aeruginis       CD 4   Burn and damage over time\n"
			  << "  Lux Sanctorum        CD 4   Heal and cleanse\n";
	if (const ShimmerSkill* skill = combat.GetDefaultSkill()) {
		std::cout << "\nEquipped: " << skill->getName() << "  //  "
				  << (skill->IsReady() ? "READY" : "COOLDOWN") << "\n";
	}
	std::cout << "\nPress 0 to return.\n";
}

void ConsoleUI::ShowDeveloperPanel() const {
	Clear();
	PrintTitle("DEVELOPER MODE  //  HIDDEN PANEL");
	std::cout << "  1. Spawn item\n  2. Spawn enemy\n  3. Teleport\n"
			  << "  4. Reveal map\n  5. Add shimmer skill\n  6. Toggle invincibility\n"
			  << "  7. Print debug log\n  0. Return\n";
}

void ConsoleUI::ShowMessage(const std::string& message) const {
	std::cout << "\n  " << message << "\n";
}

int ConsoleUI::Prompt(const std::string& prompt, int minimum, int maximum) const {
	std::cout << "\n" << prompt << "\n> ";
	int choice = 0;
	if (!(std::cin >> choice)) {
		if (std::cin.eof()) return 0;
		std::cin.clear();
		std::cin.ignore(10000, '\n');
		return minimum - 1;
	}
	if (choice < minimum || choice > maximum) return minimum - 1;
	return choice;
}

bool ConsoleUI::IsDeveloperMode() const {
	return HasEnvironmentValue("MICARE_DEVELOPER_MODE", "1");
}

std::string ConsoleUI::StatusName(int statusType) const {
	switch (static_cast<StatusType>(statusType)) {
	case StatusType::Stun: return "Stun";
	case StatusType::Bleed: return "Bleed";
	case StatusType::Burn: return "Burn";
	case StatusType::Poison: return "Poison";
	case StatusType::HealOverTime: return "Healing";
	case StatusType::Slow: return "Slow";
	case StatusType::VerdigrisMark: return "Verdigris Mark";
	case StatusType::DamageReduction: return "Damage Reduction";
	case StatusType::Barrier: return "Barrier";
	}
	return "Unknown";
}
