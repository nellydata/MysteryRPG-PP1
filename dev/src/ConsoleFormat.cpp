#include "ConsoleFormat.h"
#include <sstream>

namespace ConsoleFormat
{

    const char *SlotName(EquipSlot slot)
    {
        static const char *names[] = {"Head", "Torso", "Boots", "Hand", "Accessory", "None"};
        const int index = static_cast<int>(slot);
        return index >= 0 && index < 6 ? names[index] : "None";
    }

    const char *RarityName(Rarity rarity)
    {
        static const char *names[] = {"Common", "Uncommon", "Rare", "Epic", "Legendary", "Artifact"};
        const int index = static_cast<int>(rarity);
        return index >= 0 && index < 6 ? names[index] : "Common";
    }

    std::string PlayerPanel(const std::string &name, int level, int hp, int maxHp,
                            int attack, int defense, int speed,
                            std::int64_t xp, std::int64_t xpToNext)
    {
        std::ostringstream out;
        out << name << " | Level " << level
            << " | HP " << hp << '/' << maxHp
            << " | ATK " << attack << " DEF " << defense
            << " SPD " << speed << " | XP " << xp
            << '/' << xpToNext << '\n';
        return out.str();
    }

    std::string StatPreview(const Stats &before, const Stats &after)
    {
        std::ostringstream out;
        out << "After equip/unequip: HP " << before.hp << " -> " << after.hp
            << ", ATK " << before.atk << " -> " << after.atk
            << ", DEF " << before.def << " -> " << after.def
            << ", SPD " << before.spd << " -> " << after.spd
            << "\nACC " << before.acc << " -> " << after.acc
            << ", RES " << before.res << " -> " << after.res
            << ", Crit " << before.critRate << "% -> " << after.critRate << "%\n";
        return out.str();
    }

    std::string PageHeader(const std::string &title, std::size_t page, std::size_t pageCount)
    {
        std::ostringstream out;
        out << "=== " << title << " page " << page << '/' << pageCount << " ===\n";
        return out.str();
    }

} // namespace ConsoleFormat
