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
std::string RarityLabel(Rarity rarity);
std::string RainbowName(const std::string& name);
std::string RainbowAdelaideIn(const std::string& text);
std::string Divider(char fill = '-', std::size_t width = 56);
std::string Title(const std::string& title);
std::string Section(const std::string& title);
std::string MenuOption(int number, const std::string& label,
                       const std::string& note = "", bool enabled = true);
std::string Note(const std::string& message);
std::string Success(const std::string& message);
std::string Warning(const std::string& message);
std::string Error(const std::string& message);
std::string CooldownText(int turns);
std::string ProgressBar(int current, int maximum, std::size_t width = 12,
                        bool showMaximum = true);

// One-line player summary used by every panel header.
std::string PlayerPanel(const std::string& name, int level, int hp, int maxHp,
                        int barrier, int attack, int defense, int speed,
                        std::int64_t xp, std::int64_t xpToNext,
                        bool developerMode = false);

// Compact combat-state builders. Both are pure and safe to call repeatedly.
std::string HealthBar(int current, int maximum, std::size_t width = 12);
std::string StatusSummary(const std::vector<StatusEffect>& statuses);

// "After equip/unequip" comparison block for the inventory detail view.
std::string StatPreview(const Stats& before, const Stats& after);
std::string StatChanges(const Stats& before, const Stats& after);
std::string SetProgress(const std::string& name, int equipped, int maximum,
                        int firstThreshold, int secondThreshold,
                        const std::string& authoredDescription);
std::size_t ClampPage(std::size_t requestedPage, std::size_t itemCount, std::size_t pageSize);

// Pagination footer text, e.g. "=== INVENTORY page 2/5 ===".
std::string PageHeader(const std::string& title, std::size_t page, std::size_t pageCount);

}  // namespace ConsoleFormat
