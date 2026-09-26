#pragma once
#include "ConsoleProcess.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

namespace ConsoleQA {
inline bool Contains(const std::string& text, const std::string& needle) { return text.find(needle) != std::string::npos; }
inline std::vector<std::smatch> Matches(const std::string& text, const std::regex& pattern) {
    return {std::sregex_iterator(text.begin(), text.end(), pattern), std::sregex_iterator()};
}

// Every decision below comes from printed menus. No game objects or test-only
// state are linked into this executable. The transcript contains every key.
class Endurance {
    struct Entry { int index = 0; std::string id, name, rarity, slot, detail; bool equipped = false; };
    struct Skill { int index = 0, cost = 0, power = 0; std::string name, description; bool equipped = false; };
    fs::path dev_, prefix_;
    Process child_;
    std::ofstream capture_, observations_;
    std::string screen_, room_ = "Fountain", enemyTier_, enemyName_;
    std::map<std::string, Entry> items_;
    std::map<std::string, std::string> gear_;
    std::map<std::string, Skill> skills_;
    std::set<std::string> used_, cleared_;
    std::set<long long> events_;
    int level_ = 1, hp_ = 108, maxHp_ = 108, atk_ = 20, def_ = 10, spd_ = 100, unlocked_ = 1;
    long long inputs_ = 0, investigations_ = 0, battles_ = 0, victories_ = 0, deaths_ = 0, flees_ = 0;
    long long at100_ = -1, storageEnemies_ = 0, storageBosses_ = 0;
    long long xp_ = 0, nextXp_ = 100;
    bool inventoryDirty_ = true, skillsDirty_ = true, farming_ = false;
    std::chrono::steady_clock::time_point started_ = std::chrono::steady_clock::now();
    static int RoomNumber(const std::string& room) {
        const std::vector<std::string> names{"Fountain", "Entrance Hall", "Archive Room", "Chapel", "Storage Room"};
        auto it = std::find(names.begin(), names.end(), room);
        return static_cast<int>(it - names.begin()) + 1;
    }
    void Read() {
        screen_ = child_.Read(true, &capture_);
        static const std::regex panel(R"( \| Level (-?\d+) \| HP (-?\d+)/(-?\d+) \| ATK (-?\d+) DEF (-?\d+) SPD (-?\d+) \| XP (-?\d+)/(-?\d+))");
        for (const auto& match : Matches(screen_, panel)) {
            level_ = std::stoi(match[1]); hp_ = std::stoi(match[2]); maxHp_ = std::stoi(match[3]);
            atk_ = std::stoi(match[4]); def_ = std::stoi(match[5]); spd_ = std::stoi(match[6]);
            xp_ = std::stoll(match[7]); nextXp_ = std::stoll(match[8]);
            if (level_ < 1 || level_ > 100 || hp_ < 0 || maxHp_ <= 0 || hp_ > maxHp_ || atk_ < 0 || def_ < 0 || spd_ <= 0 || xp_ < 0 || nextXp_ <= 0 || xp_ >= nextXp_)
                throw std::runtime_error("Invalid visible player stats or XP");
        }
        std::smatch match;
        if (std::regex_search(screen_, match, std::regex(R"(  Lv\. (-?\d+)(?:  \[DEV\])?\r?\nHP       \[[^\]]*\] (-?\d+) / (-?\d+))"))) {
            level_ = std::stoi(match[1]); hp_ = std::stoi(match[2]); maxHp_ = std::stoi(match[3]);
        }
        if (std::regex_search(screen_, match, std::regex(R"(ATK (-?\d+)   DEF (-?\d+)   SPD (-?\d+))"))) {
            atk_ = std::stoi(match[1]); def_ = std::stoi(match[2]); spd_ = std::stoi(match[3]);
        }
        if (std::regex_search(screen_, match, std::regex(R"(XP       (-?\d+) / (-?\d+))"))) {
            xp_ = std::stoll(match[1]); nextXp_ = std::stoll(match[2]);
        } else if (Contains(screen_, "XP       MAX LEVEL")) { xp_ = 0; nextXp_ = 1; }
        if (std::regex_search(screen_, match, std::regex(R"(=== ROOM: (.+?) ===)"))) room_ = match[1];
        for (const auto& unlock : Matches(screen_, std::regex(R"((Entrance Hall|Archive Room|Chapel|Storage Room) is now unlocked\.)")))
            unlocked_ = (std::max)(unlocked_, RoomNumber(unlock[1]));
        inventoryDirty_ |= Contains(screen_, "Found:") || Contains(screen_, "Five equipment rewards were added");
        skillsDirty_ |= Contains(screen_, "Shimmer skill unlocked") || Contains(screen_, "Level up!");
        if (std::regex_search(screen_, match, std::regex(R"(Encounter begins: (.+) \(level (\d+), (\w+)\)\.)"))) {
            ++battles_; enemyName_ = match[1]; enemyTier_ = match[3]; farming_ = cleared_.count(room_) != 0;
            if (room_ == "Storage Room" && farming_) { if (enemyTier_ == "Boss") ++storageBosses_; else ++storageEnemies_; }
        }
        if (Contains(screen_, "First clear: ")) cleared_.insert(room_);
        if (Contains(screen_, "Encounter complete: victory.")) ++victories_;
        if (Contains(screen_, "Encounter complete: defeat.")) ++deaths_;
        if (Contains(screen_, "Encounter complete: fled.")) ++flees_;
        for (const auto& cast : Matches(screen_, std::regex(R"(uses Shimmer skill (.+) on )"))) used_.insert(cast[1]);
        static const std::regex eventPattern(R"((?:^|\n)\[(\d+)\] ([^\r\n]+))");
        for (const auto& event : Matches(screen_, eventPattern)) {
            if (!events_.insert(std::stoll(event[1])).second) continue;
            const std::string message = event[2];
            if (std::regex_search(message, match, std::regex(R"(^Encounter begins: (.+) \(level (\d+), (\w+)\)\.)"))) {
                ++battles_; enemyName_ = match[1]; enemyTier_ = match[3]; farming_ = cleared_.count(room_) != 0;
                if (room_ == "Storage Room" && farming_) { if (enemyTier_ == "Boss") ++storageBosses_; else ++storageEnemies_; }
            }
            if (std::regex_search(message, match, std::regex(R"(uses Shimmer skill (.+) on )"))) used_.insert(match[1]);
            if (message.rfind("First clear: ", 0) == 0) cleared_.insert(room_);
            if (message == "Encounter complete: victory.") ++victories_;
            if (message == "Encounter complete: defeat.") ++deaths_;
            if (message == "Encounter complete: fled.") ++flees_;
        }
        if (level_ >= 100 && at100_ < 0) at100_ = investigations_;
    }
    void Key(const std::string& key) { ++inputs_; capture_ << key << '\n'; child_.Send(key); Read(); }
    void Key(int key) { Key(std::to_string(key)); }
    void Snapshot(const std::string& label) {
        std::ostringstream line;
        line << "LIVE " << label << ": level=" << level_ << " HP=" << hp_ << '/' << maxHp_
            << " XP=" << xp_ << '/' << nextXp_
            << " ATK=" << atk_ << " DEF=" << def_ << " SPD=" << spd_ << " room=" << room_
            << " investigations=" << investigations_ << " battles=" << battles_ << " victories=" << victories_
            << " deaths=" << deaths_ << " flees=" << flees_ << " inputs=" << inputs_
            << " elapsed=" << std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count() << "s\n";
        std::cout << line.str() << std::flush; observations_ << line.str() << "  Gear:";
        for (const auto& slot : gear_) observations_ << ' ' << slot.first << '=' << slot.second;
        observations_ << '\n' << std::flush;
    }
    void Rewards() {
        while (Contains(screen_, "Choose a level-up skill")) {
            const auto options = Matches(screen_, std::regex(R"((?:^|\n)(\d+)\. ([^\r\n]+))"));
            if (options.empty()) throw std::runtime_error("Reward choices absent");
            auto pick = options.front();
            for (const auto& option : options) {
                const auto name = option[2].str();
                if (name == "Refraction Cataclysm" || name == "Refractio Ultima" || name == "Ray of Mercy" || name == "Prism Step") { pick = option; break; }
            }
            Key(pick[1]);
        }
    }
    std::vector<Entry> Entries() const {
        std::vector<Entry> result;
        static const std::regex pattern(R"((?:^|\n)\[(\d+)\] ([^\r\n]+)\r?\n    (Common|Uncommon|Rare|Epic|Legendary|Artifact) \| ([^|\r\n]+)([^\r\n]*)\r?\n    ([^\r\n]+))");
        for (const auto& match : Matches(screen_, pattern)) {
            Entry entry; entry.index = std::stoi(match[1]); entry.name = match[2]; entry.rarity = match[3];
            entry.slot = match[4];
            while (!entry.slot.empty() && entry.slot.back() == ' ') entry.slot.pop_back();
            const auto tail = match[5].str() + match[6].str(); std::smatch identity;
            entry.id = "name:" + entry.name;
            if (std::regex_search(tail, identity, std::regex(R"(instance=(\d+))"))) {
                entry.id = identity[1]; std::istringstream words(tail); words >> entry.slot;
            }
            entry.equipped = Contains(tail, "[EQUIPPED]"); result.push_back(entry);
        }
        return result;
    }
    bool NextPage() const {
        std::smatch page;
        return std::regex_search(screen_, page, std::regex(R"(=== EQUIPMENT page (\d+)/(\d+) ===)")) && std::stoi(page[1]) < std::stoi(page[2]);
    }
    static double Score(const std::vector<double>& stats) {
        return std::log((std::max)(1.0, stats[0])) + 3 * std::log((std::max)(1.0, stats[1]))
            + .7 * std::log(stats[2] + 100) + 1.5 * std::log((std::max)(1.0, stats[3]))
            + stats[4] * .001 + stats[5] * .002 + stats[6] * .003;
    }
    void Inventory() {
        Key(Contains(screen_, "Destination or panel:") ? 6 : 3);
        if (!Contains(screen_, "Choose an inventory category.")) throw std::runtime_error("Expected inventory category menu");
        Key(1);
        for (;;) {
            if (!Contains(screen_, "Select an item or navigation action.")) throw std::runtime_error("Expected equipment menu");
            bool foundNew = false;
            for (auto entry : Entries()) {
                if (items_.count(entry.id)) continue;
                foundNew = true; Key(entry.index); entry.detail = screen_;
                observations_ << "ITEM " << entry.id << ' ' << entry.name << " [" << entry.rarity << "] " << entry.slot << " at level " << level_ << '\n' << screen_ << '\n';
                items_[entry.id] = entry;
                const auto stats = Matches(screen_, std::regex(R"((?:HP|ATK|DEF|SPD|ACC|RES|Crit) (\d+)%? -> (\d+)%?)"));
                int action = 0;
                if (entry.equipped) gear_[entry.slot] = entry.id;
                if (!entry.slot.empty() && stats.size() == 7 && !entry.equipped) {
                    std::vector<double> before, after;
                    for (const auto& value : stats) { before.push_back(std::stod(value[1])); after.push_back(std::stod(value[2])); }
                    if (Score(after) > Score(before) + .00001) { action = 1; gear_[entry.slot] = entry.id; }
                }
                if (action) { Key(1); Key(1); }
                else Key(0);
            }
            if (!foundNew || !NextPage()) break;
            Key(9);
        }
        Key(0); Key(0); inventoryDirty_ = false;
    }
    std::vector<Skill> SkillRows() {
        std::vector<Skill> result;
        for (const auto& match : Matches(screen_, std::regex(R"((?:^|\n)\[(\d+)\] (.+?)  Cost (\d+)  CD (\d+)([^\r\n]*)\r?\n   ([^\r\n]+))"))) {
            Skill skill; skill.index = std::stoi(match[1]); skill.name = match[2]; skill.cost = std::stoi(match[3]);
            skill.equipped = Contains(match[5], "[EQUIPPED]"); skill.description = match[6];
            std::smatch power;
            if (std::regex_search(skill.description, power, std::regex(R"(Deal (\d+)%)"))) skill.power = std::stoi(power[1]);
            result.push_back(skill); skills_[skill.name] = skill;
        }
        return result;
    }
    void Configure(const std::vector<std::string>& explicitSkills = {}) {
        Key(Contains(screen_, "Destination or panel:") ? 7 : 5); Rewards();
        if (!Contains(screen_, "Shimmer loadout:")) throw std::runtime_error("Expected skill loadout");
        auto rows = SkillRows(); std::vector<std::string> desired = explicitSkills; int remaining = 12;
        auto add = [&](const Skill& skill) {
            if (skill.cost <= remaining && std::find(desired.begin(), desired.end(), skill.name) == desired.end()) {
                desired.push_back(skill.name); remaining -= skill.cost; return true;
            }
            return false;
        };
        if (desired.empty()) {
            std::stable_sort(rows.begin(), rows.end(), [](const Skill& a, const Skill& b) { return a.cost < b.cost; });
            for (const auto& row : rows) if (!used_.count(row.name)) { add(row); break; }
            std::stable_sort(rows.begin(), rows.end(), [](const Skill& a, const Skill& b) { return a.power > b.power; });
            for (const auto& row : rows) if (row.power && add(row)) break;
            for (const auto& row : rows) if ((Contains(row.description, "Restore") || Contains(row.description, "barrier")) && add(row)) break;
            for (const auto& row : rows) if (row.power) add(row);
        }
        for (const auto& row : rows) if (row.equipped && std::find(desired.begin(), desired.end(), row.name) == desired.end()) Key(row.index);
        for (const auto& row : rows) if (!row.equipped && std::find(desired.begin(), desired.end(), row.name) != desired.end()) Key(row.index);
        Key(0); skillsDirty_ = false;
    }
    void Combat(const std::string& preferred = "") {
        const std::map<std::string, int> readiness{{"Entrance Hall",8},{"Archive Room",15},{"Chapel",22},{"Storage Room",30}};
        const auto ready = readiness.find(room_);
        if (preferred.empty() && enemyTier_ == "Boss" && !farming_ && ready != readiness.end() && level_ < ready->second) { Key(4); return; }
        Key(3);
        if (!Contains(screen_, "Choose combat Shimmer")) { Key(1); return; }
        auto options = Matches(screen_, std::regex(R"((?:^|\n)\[(\d+)\] (.+?)  Cost \d+  \[(READY|CD \d+)\]\r?(?:\n|$))"));
        std::string pick; int bestPower = -1;
        for (const auto& option : options) {
            if (option[3] != "READY") continue;
            const auto name = option[2].str();
            if (!preferred.empty() && name == preferred) { pick = option[1]; break; }
            if (preferred.empty() && !used_.count(name)) { pick = option[1]; break; }
            const auto& skill = skills_[name];
            if (hp_ < maxHp_ * .45 && Contains(skill.description, "Restore")) { pick = option[1]; break; }
            if (skill.power > bestPower && skill.power > 0) { pick = option[1]; bestPower = skill.power; }
        }
        if (pick.empty()) { Key(0); Key(1); }
        else { Key(pick); skillsDirty_ = true; }
    }
    void Investigate() { ++investigations_; Key(1 + static_cast<int>(investigations_ % 2)); }
    void Defeat() { Snapshot("defeat-review"); Key(1); Key(0); }
    void Recover(int destination) {
        Key(0); Key(1); Key(7); Key(0); Key(destination);
    }
    void SetGear(const std::string& id, bool equip) {
        Key(3);
        if (!Contains(screen_, "Choose an inventory category.")) throw std::runtime_error("Expected inventory category menu");
        Key(1);
        for (;;) {
            const auto entries = Entries();
            const auto target = std::find_if(entries.begin(), entries.end(), [&](const Entry& e) { return e.id == id; });
            if (target != entries.end()) {
                Key(target->index);
                if (target->equipped != equip) { Key(1); Key(1); }
                else Key(0);
                if (equip) gear_[target->slot] = id; else gear_.erase(target->slot);
                Key(0); Key(0); return;
            }
            if (!NextPage()) throw std::runtime_error("Earned item instance missing: " + id);
            Key(9);
        }
    }
    void ClearGear() { const auto prior = gear_; for (const auto& entry : prior) SetGear(entry.second, false); }
    void Exercise(const std::string& label) {
        Recover(2);
        Configure({"Prism Step", "Ray of Mercy", "Flos Aeruginis", "Lux Vortex", "Pulsus Luminis"});
        Snapshot(label + "-live-combat-start");
        for (int i = 0; i < 100 && Contains(screen_, "Room action:"); ++i) Investigate();
        const std::vector<std::string> cycle{"Prism Step","Ray of Mercy","Attack","Flos Aeruginis","Lux Vortex","Pulsus Luminis"};
        for (int i = 0; i < 100 && Contains(screen_, "Combat action:"); ++i) {
            if (cycle[i % cycle.size()] == "Attack") Key(1);
            else Combat(cycle[i % cycle.size()]);
        }
        if (Contains(screen_, "Combat action:")) { Key(5); Key(4); }
        if (Contains(screen_, "Defeat review:")) Defeat();
        Rewards();
        if (Contains(screen_, "Room action:")) Key(6);
        Snapshot(label + "-live-combat-end");
    }
    void Sets() {
        const auto original = gear_;
        for (const auto& set : std::vector<std::string>{"Choir of Verdigris","Shimmer Ascendant","Automaton Overlord","Verdigris Matron","Shimmer Reliquary","Oracle Automaton"}) {
            std::map<std::string, std::string> bySlot;
            for (const auto& item : items_) if (!item.second.slot.empty() && Contains(item.second.detail, set + ":")) bySlot[item.second.slot] = item.first;
            const bool artifact = set == "Verdigris Matron" || set == "Shimmer Reliquary" || set == "Oracle Automaton";
            const int first = artifact ? 3 : 2, last = artifact ? 5 : 3;
            observations_ << "SET " << set << ": " << bySlot.size() << " legitimately earned distinct slots\n";
            if (static_cast<int>(bySlot.size()) < first) { observations_ << "NOT REACHED " << set << " threshold\n"; continue; }
            ClearGear(); Snapshot(set + "-bare-baseline"); int pieces = 0;
            for (const auto& item : bySlot) {
                SetGear(item.second, true); ++pieces;
                if (pieces == first || pieces == last) Snapshot(set + '-' + std::to_string(pieces) + "-pieces");
                if (pieces >= last) break;
            }
            for (const auto& item : original) if (!gear_.count(item.first)) SetGear(item.second, true);
            Exercise(set);
        }
        ClearGear(); for (const auto& item : original) SetGear(item.second, true);
        Configure(); Snapshot("restored-optimized-build");
    }
public:
    Endurance(const fs::path& dev, const fs::path& executable, const std::string& capture)
        : dev_(dev), prefix_(dev / "validation" / capture), child_(executable, {L"--fast"}, dev),
          capture_(prefix_.string() + "-console.txt", std::ios::binary), observations_(prefix_.string() + "-observations.txt") {
        if (!capture_ || !observations_) throw std::runtime_error("Could not create QA captures");
        RequireConsole(executable); capture_ << "REAL console output; submitted keyboard input follows each > prompt.\n";
        Read();
    }
    void Run(int target, int extra, bool sets, int minutes = 20) {
        Key("QA_CPP"); Snapshot("fresh-character");
        Inventory(); Configure(); Key(1); Investigate();
        for (;;) {
            if (std::chrono::steady_clock::now() - started_ > std::chrono::minutes(minutes)) throw std::runtime_error("QA time budget reached before target; not a game failure");
            Rewards();
            if (Contains(screen_, "Defeat review:")) { Defeat(); continue; }
            if (Contains(screen_, "Combat action:")) { Combat(); continue; }
            if (Contains(screen_, "Destination or panel:")) { Key(cleared_.count("Storage Room") ? 5 : unlocked_); continue; }
            if (!Contains(screen_, "Room action:")) throw std::runtime_error("Unrecognized prompt: " + screen_.substr(screen_.size() > 800 ? screen_.size() - 800 : 0));
            if (inventoryDirty_) { Inventory(); continue; }
            if (skillsDirty_) { Configure(); continue; }
            if (level_ >= target && (target < 100 || (at100_ >= 0 && investigations_ - at100_ >= extra))) break;
            if (room_ == "Fountain" && unlocked_ > 1) { Key(7); Key(0); Key(unlocked_); continue; }
            const int destination = cleared_.count("Storage Room") ? 5 : unlocked_;
            if (RoomNumber(room_) != destination) { Snapshot("next-room"); Key(0); Key(destination); continue; }
            if (hp_ < maxHp_ * .70 && unlocked_ > 1) { Recover(destination); continue; }
            if (investigations_ % 100 == 0) Snapshot("progress");
            Investigate();
        }
        Snapshot("target-reached");
        if (sets && level_ >= 40) Sets();
        observations_ << "Skills owned " << skills_.size() << "; skills cast " << used_.size() << '\n';
        for (const auto& skill : skills_) observations_ << (used_.count(skill.first) ? "LIVE CAST " : "NOT CAST ") << skill.first << '\n';
        observations_ << "Storage post-clear ordinary encounters " << storageEnemies_ << "; farming bosses " << storageBosses_ << '\n';
        Snapshot("final-live-state"); Key(6); Key(0); Key(0); child_.CloseInput();
        capture_ << child_.Read(false);
        if (!child_.WaitForExit() || child_.ExitCode() != 0 || !Contains(screen_, "Session complete.")) throw std::runtime_error("Game did not exit normally");
        if (target == 100 && extra >= 100 && (!storageEnemies_ || !storageBosses_)) throw std::runtime_error("Storage farming coverage incomplete");
        std::cout << "Endurance passed: real console exit 0; "
#ifdef _WIN32
            "PE subsystem 3; "
#else
            "executable console process; "
#endif
            << used_.size() << " distinct skills cast.\n";
    }
};
}
