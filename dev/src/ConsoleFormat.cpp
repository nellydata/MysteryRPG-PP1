#include "ConsoleFormat.h"
#include <algorithm>
#include <map>
#include <sstream>

namespace ConsoleFormat
{

    std::string RainbowName(const std::string& name)
    {
        if (name != "Adelaide") return name;
        static const char* colors[] = {"\x1b[91m", "\x1b[93m", "\x1b[92m", "\x1b[96m",
                                       "\x1b[94m", "\x1b[95m", "\x1b[91m", "\x1b[93m"};
        std::string result;
        for (std::size_t index = 0; index < name.size(); ++index) {
            result += colors[index];
            result += name[index];
        }
        return result + "\x1b[0m";
    }

    std::string RainbowAdelaideIn(const std::string& text)
    {
        const std::string needle = "Adelaide";
        std::string result;
        std::size_t cursor = 0;
        while (true) {
            const auto found = text.find(needle, cursor);
            if (found == std::string::npos) return result + text.substr(cursor);
            result += text.substr(cursor, found - cursor);
            result += RainbowName(needle);
            cursor = found + needle.size();
        }
    }

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

    std::string HealthBar(int current, int maximum, std::size_t width)
    {
        const int safeMaximum = (std::max)(0, maximum);
        const int safeCurrent = (std::clamp)(current, 0, safeMaximum);
        const std::size_t filled = safeMaximum > 0
            ? static_cast<std::size_t>((static_cast<long long>(safeCurrent) * width) / safeMaximum)
            : 0;
        return "[" + std::string(filled, '#') + std::string(width - filled, '-') + "] " +
            std::to_string(safeCurrent) + "/" + std::to_string(safeMaximum);
    }

    std::string StatusSummary(const std::vector<StatusEffect>& statuses)
    {
        static const char* names[] = {
            "Stun", "Bleed", "Burn", "Poison", "Heal over time", "Slow",
            "Verdigris Mark", "Damage reduction", "Barrier", "ATK up", "ATK down",
            "DEF up", "DEF down", "SPD up", "Accuracy up", "Accuracy down",
            "Crit up", "Crit damage up", "Guaranteed crit", "DOT up",
            "Healing potency up", "DOT reduction", "Heal resistance", "Accuracy floor"
        };
        struct StatusGroup {
            int count = 0;
            std::vector<int> durations;
        };
        std::map<StatusType, StatusGroup> groups;
        for (const auto& status : statuses) {
            if (status.type == StatusType::Barrier || status.turns <= 0) continue;
            auto& group = groups[status.type];
            ++group.count;
            group.durations.push_back(status.turns);
        }
        if (groups.empty()) return "None";

        std::ostringstream out;
        bool first = true;
        for (const auto& entry : groups) {
            if (!first) out << ", ";
            first = false;
            const int index = static_cast<int>(entry.first);
            out << (index >= 0 && index < static_cast<int>(std::size(names)) ? names[index] : "Unknown status");
            if (entry.second.count > 1) out << " x" << entry.second.count;
            out << " (";
            for (std::size_t duration = 0; duration < entry.second.durations.size(); ++duration) {
                if (duration) out << '/';
                out << entry.second.durations[duration];
            }
            out << (entry.second.durations.size() == 1 && entry.second.durations.front() == 1
                ? " turn)" : " turns)");
        }
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
