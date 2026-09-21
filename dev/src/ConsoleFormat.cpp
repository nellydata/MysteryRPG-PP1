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

    std::string RarityLabel(Rarity rarity) { return "[" + std::string(RarityName(rarity)) + "]"; }

    std::string Divider(char fill, std::size_t width) { return std::string(width, fill); }

    std::string Title(const std::string& title) {
        return Divider('=') + "\n  " + title + "\n" + Divider('=') + "\n";
    }

    std::string Section(const std::string& title) { return "-- " + title + " " + Divider('-', 51 - (std::min)(title.size(), std::size_t{49})) + "\n"; }

    std::string MenuOption(int number, const std::string& label, const std::string& note, bool enabled) {
        std::ostringstream out;
        out << '[' << number << "] " << label;
        if (!note.empty()) out << "  [" << (enabled ? note : "LOCKED - " + note) << ']';
        else if (!enabled) out << "  [LOCKED]";
        return out.str() + '\n';
    }

    std::string Note(const std::string& message) { return "[i] " + message; }
    std::string Success(const std::string& message) { return "[+] " + message; }
    std::string Warning(const std::string& message) { return "[!] " + message; }
    std::string Error(const std::string& message) { return "[x] " + message; }

    std::string CooldownText(int turns) {
        return turns <= 0 ? "READY" : "CD " + std::to_string(turns) + (turns == 1 ? " turn" : " turns");
    }

    std::string ProgressBar(int current, int maximum, std::size_t width, bool showMaximum) {
        const int safeMaximum = (std::max)(0, maximum);
        const int safeCurrent = (std::clamp)(current, 0, safeMaximum);
        const std::size_t filled = safeMaximum > 0
            ? static_cast<std::size_t>((static_cast<long long>(safeCurrent) * width) / safeMaximum) : 0;
        std::string result = "[" + std::string(filled, '#') + std::string(width - filled, '-') + "] " +
            std::to_string(safeCurrent);
        if (showMaximum) result += " / " + std::to_string(safeMaximum);
        return result;
    }

    std::string PlayerPanel(const std::string &name, int level, int hp, int maxHp,
                            int barrier, int attack, int defense, int speed,
                            std::int64_t xp, std::int64_t xpToNext, bool developerMode)
    {
        std::ostringstream out;
        out << name << "  Lv. " << level;
        if (developerMode) out << "  [DEV]";
        out << "\nHP       " << ProgressBar(hp, maxHp);
        if (barrier > 0) out << "\nBarrier  " << ProgressBar(barrier, maxHp, 12, false);
        out << "\nATK " << attack << "   DEF " << defense << "   SPD " << speed << "\nXP       ";
        if (level >= 100) out << "MAX LEVEL";
        else out << xp << " / " << xpToNext;
        out << '\n';
        return out.str();
    }

    std::string HealthBar(int current, int maximum, std::size_t width)
    {
        const int safeMaximum = (std::max)(0, maximum);
        const int safeCurrent = (std::clamp)(current, 0, safeMaximum);
        const std::size_t filled = safeMaximum > 0
            ? static_cast<std::size_t>((static_cast<long long>(safeCurrent) * width) / safeMaximum)
            : 0;
        return ProgressBar(safeCurrent, safeMaximum, width);
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
            out << ')';
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

    std::string StatChanges(const Stats& before, const Stats& after)
    {
        struct Field { const char* label; int Stats::*member; const char* unit; };
        static const Field fields[] = {
            {"HP", &Stats::hp, ""}, {"ATK", &Stats::atk, ""}, {"DEF", &Stats::def, ""},
            {"SPD", &Stats::spd, ""}, {"Accuracy", &Stats::acc, "pp"},
            {"Resistance", &Stats::res, "pp"}, {"Crit Rate", &Stats::critRate, "pp"},
            {"Crit Damage", &Stats::critDamage, "pp"}, {"Evasion", &Stats::evasion, "pp"},
            {"Healing Potency", &Stats::healingPotency, "pp"},
            {"Healing Received", &Stats::healingReceived, "pp"},
            {"Barrier Potency", &Stats::barrierPotency, "pp"},
            {"DOT Potency", &Stats::dotPotency, "pp"}, {"Lifesteal", &Stats::lifesteal, "pp"},
            {"Debuff Reduction", &Stats::debuffDurationReduction, "pp"}
        };
        std::ostringstream out;
        bool changed = false;
        for (const auto& field : fields) {
            const int oldValue = before.*field.member;
            const int newValue = after.*field.member;
            if (oldValue == newValue) continue;
            const int delta = newValue - oldValue;
            out << field.label << "  " << oldValue << " -> " << newValue << "  ("
                << (delta > 0 ? "+" : "") << delta << field.unit << ")\n";
            changed = true;
        }
        return changed ? out.str() : "No effective stat change.\n";
    }

    std::string SetProgress(const std::string& name, int equipped, int maximum,
                            int firstThreshold, int secondThreshold,
                            const std::string& authoredDescription)
    {
        std::ostringstream out;
        out << name << " [" << equipped << '/' << maximum << "]\n"
            << firstThreshold << "-piece " << (equipped >= firstThreshold ? "ACTIVE" : "LOCKED");
        if (secondThreshold > firstThreshold)
            out << " | " << secondThreshold << "-piece " << (equipped >= secondThreshold ? "ACTIVE" : "LOCKED");
        out << '\n' << authoredDescription << '\n';
        return out.str();
    }

    std::size_t ClampPage(std::size_t requestedPage, std::size_t itemCount, std::size_t pageSize)
    {
        if (itemCount == 0 || pageSize == 0) return 0;
        return (std::min)(requestedPage, (itemCount - 1) / pageSize);
    }

    std::string PageHeader(const std::string &title, std::size_t page, std::size_t pageCount)
    {
        std::ostringstream out;
        out << "=== " << title << " page " << page << '/' << pageCount << " ===\n";
        return out.str();
    }

} // namespace ConsoleFormat
