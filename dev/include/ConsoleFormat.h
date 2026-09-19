#pragma once

#include "GameplayTypes.h"
#include <string>
#include <vector>

// Pure, side-effect-free string builders for the console presentation layer.
// Keeping them out of ConsoleUI.cpp lets them be unit tested without any
// console or stdin/stdout involvement.
namespace ConsoleFormat {

const char* SlotName(EquipSlot slot);
const char* RarityName(Rarity rarity);
std::string RainbowName(const std::string& name);
std::string RainbowAdelaideIn(const std::string& text);

// One-line player summary used by every panel header.
std::string PlayerPanel(const std::string& name, int level, int hp, int maxHp,
                        int attack, int defense, int speed,
                        std::int64_t xp, std::int64_t xpToNext);

// Compact combat-state builders. Both are pure and safe to call repeatedly.
std::string HealthBar(int current, int maximum, std::size_t width = 12);
std::string StatusSummary(const std::vector<StatusEffect>& statuses);

// "After equip/unequip" comparison block for the inventory detail view.
std::string StatPreview(const Stats& before, const Stats& after);

// Pagination footer text, e.g. "=== INVENTORY page 2/5 ===".
std::string PageHeader(const std::string& title, std::size_t page, std::size_t pageCount);

}  // namespace ConsoleFormat
