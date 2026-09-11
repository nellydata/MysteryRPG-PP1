#pragma once

#include "ConsoleProcess.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

namespace ConsoleQA {
namespace ArtifactAudit {
inline bool Has(const std::string& text, const std::string& value) { return text.find(value) != std::string::npos; }
inline std::vector<std::smatch> Rows(const std::string& text, const std::regex& expression) {
    return {std::sregex_iterator(text.begin(), text.end(), expression), std::sregex_iterator()};
}
inline std::uint64_t Fingerprint(const std::string& bytes) {
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char byte : bytes) { value ^= byte; value *= 1099511628211ULL; }
    return value;
}
inline std::vector<std::string> Reconstruct(const fs::path& dev) {
    std::vector<fs::path> segments;
    for (const auto& entry : fs::directory_iterator(dev / "validation")) {
        const auto name = entry.path().filename().string();
        if (name.rfind("endurance-final-console-", 0) == 0 && entry.path().extension() == ".txt") segments.push_back(entry.path());
    }
    std::sort(segments.begin(), segments.end());
    std::vector<std::string> keys;
    for (std::size_t segment = 0; segment < segments.size(); ++segment) {
        std::ifstream file(segments[segment]); std::string line; int number = 0;
        while (std::getline(file, line)) {
            ++number; if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.rfind("> ", 0) == 0) keys.push_back(line.substr(2));
            else if (segment && number == 2 && std::regex_match(line, std::regex("[0-9]+"))) keys.push_back(line);
        }
    }
    const auto observer = ReadFile(dev / "validation/endurance-final-state.json");
    std::smatch count;
    if (!std::regex_search(observer, count, std::regex(R"("Inputs"\s*:\s*(\d+))")) || keys.size() != std::stoull(count[1]))
        throw std::runtime_error("Artifact audit: historical keyboard count mismatch");
    if (keys.size() < 2 || keys.back() != "0" || keys[keys.size() - 2] != "0")
        throw std::runtime_error("Artifact audit: expected room-leave and session-quit tail");
    keys.resize(keys.size() - 2); // Remain in the legitimately reached Entrance Hall.
    return keys;
}

class Driver {
    struct Item { std::string name, slot, identity; int index = 0; bool equipped = false; };
    fs::path dev_;
    std::ofstream replay_, capture_, observations_;
    Process game_;
    std::string screen_, enemy_, trial_, trialEvents_;
    std::map<std::string, Item> artifacts_;
    std::map<std::string, std::string> equipped_;
    std::set<long long> seenEvents_;
    int hp_ = 0, maxHp_ = 0, attack_ = 0, defense_ = 0, speed_ = 0, enemyMaxHp_ = 0;
    int actions_ = 0, investigations_ = 0;
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
    void Read() {
        screen_ = game_.Read(true, &capture_);
        static const std::regex panel(R"(QA_Final \| Level (\d+) \| HP (\d+)/(\d+) \| ATK (\d+) DEF (\d+) SPD (\d+) \| XP (\d+)/(\d+))");
        for (const auto& row : Rows(screen_, panel)) {
            if (row[1] != "100" || row[7] != "380271" || row[8] != "1262643")
                throw std::runtime_error("Artifact audit: earned Level-100 state changed unexpectedly");
            hp_ = std::stoi(row[2]); maxHp_ = std::stoi(row[3]); attack_ = std::stoi(row[4]); defense_ = std::stoi(row[5]); speed_ = std::stoi(row[6]);
        }
        std::smatch enemy;
        if (std::regex_search(screen_, enemy, std::regex(R"(Enemy: (.+?) \| Level \d+ \w+ \| HP \d+/(\d+))"))) {
            enemy_ = enemy[1]; enemyMaxHp_ = std::stoi(enemy[2]);
        }
        for (const auto& event : Rows(screen_, std::regex(R"((?:^|\n)\[(\d+)\] ([^\r\n]+))"))) {
            if (!seenEvents_.insert(std::stoll(event[1])).second) continue;
            trialEvents_ += event[0].str() + '\n';
            if (Has(event[2], "QA_Final uses Attack on ") || Has(event[2], "QA_Final uses Shimmer skill ")) ++actions_;
        }
    }
    void Key(const std::string& key) {
        if (std::chrono::steady_clock::now() - start_ > std::chrono::minutes(20))
            throw std::runtime_error("Artifact QA time budget reached, not a game failure");
        capture_ << key << '\n'; game_.Send(key); Read();
    }
    void Key(int key) { Key(std::to_string(key)); }
    void Note(const std::string& message) {
        observations_ << message << '\n' << std::flush;
        std::cout << "ARTIFACT " << message << '\n' << std::flush;
    }
    void Snapshot(const std::string& label) {
        Note(label + ": HP " + std::to_string(hp_) + "/" + std::to_string(maxHp_) + " ATK " + std::to_string(attack_) +
            " DEF " + std::to_string(defense_) + " SPD " + std::to_string(speed_) + " actions " + std::to_string(actions_));
    }
    std::vector<Item> InventoryRows() const {
        std::vector<Item> result;
        for (const auto& row : Rows(screen_, std::regex(R"((?:^|\n)(\d+)\. (.+?) \[(?:Common|Uncommon|Rare|Epic|Legendary|Artifact)\] (Head|Torso|Boots|Hand|Accessory) [^\r\n]*?instance=(\d+)([^\r\n]*))")))
            result.push_back({row[2], row[3], row[4], std::stoi(row[1]), Has(row[5], "[EQUIPPED]")});
        return result;
    }
    bool NextPage() const {
        std::smatch page;
        return std::regex_search(screen_, page, std::regex(R"(INVENTORY page (\d+)/(\d+))")) && std::stoi(page[1]) < std::stoi(page[2]);
    }
    void ScanEarnedInventory() {
        Key(3);
        for (;;) {
            for (const auto& item : InventoryRows()) {
                if (item.equipped) equipped_[item.slot] = item.identity;
                // IDs and names come from the real console, never from an item factory.
                if (!artifacts_.count(item.name)) artifacts_[item.name] = item;
            }
            if (!NextPage()) break;
            Key(13);
        }
        Key(0);
    }
    void SetInstance(const std::string& identity, bool equip) {
        if (!Has(screen_, "Room action:")) throw std::runtime_error("Artifact equipment requires room menu");
        Key(3);
        for (;;) {
            auto rows = InventoryRows();
            auto found = std::find_if(rows.begin(), rows.end(), [&](const Item& item) { return item.identity == identity; });
            if (found != rows.end()) {
                const auto item = *found;
                Key(item.index); observations_ << "INSPECT " << item.identity << ' ' << item.name << '\n' << screen_ << '\n';
                Key(item.equipped == equip ? 0 : 1);
                if (equip) equipped_[item.slot] = identity; else equipped_.erase(item.slot);
                Key(0); return;
            }
            if (!NextPage()) throw std::runtime_error("Legitimately earned instance missing from console inventory: " + identity);
            Key(13);
        }
    }
    void Equip(const std::string& name) {
        const auto item = artifacts_.find(name);
        if (item == artifacts_.end()) throw std::runtime_error("Requested Artifact was not actually earned: " + name);
        SetInstance(item->second.identity, true);
        Note("EQUIPPED " + name + " actual instance " + item->second.identity);
    }
    void Configure(const std::vector<std::string>& desired) {
        Key(5);
        struct SkillRow { int index; std::string name; bool equipped; };
        std::vector<SkillRow> skills;
        for (const auto& row : Rows(screen_, std::regex(R"((?:^|\n)(\d+)\. (.+?) cost \d+ cooldown \d+([^\r\n]*))")))
            skills.push_back({std::stoi(row[1]), row[2], Has(row[3], "[EQUIPPED]")});
        for (const auto& row : skills) if (row.equipped && std::find(desired.begin(), desired.end(), row.name) == desired.end()) Key(row.index);
        for (const auto& row : skills) if (!row.equipped && std::find(desired.begin(), desired.end(), row.name) != desired.end()) Key(row.index);
        if (Has(screen_, "Not enough free Shimmer points")) throw std::runtime_error("Artifact QA requested an invalid Shimmer loadout");
        Key(0);
    }
    bool Cast(const std::string& name) {
        if (!Has(screen_, "Combat action:")) return false;
        Key(3);
        std::string selection;
        for (const auto& row : Rows(screen_, std::regex(R"((?:^|\n)(\d+)\. (.+?) cooldown (\d+))")))
            if (row[2] == name && row[3] == "0") selection = row[1];
        if (selection.empty()) { Key(0); return false; }
        Key(selection); return true;
    }
    void Support() {
        for (const auto& name : {"Ray of Mercy", "Prism Step", "Scutum Coruscum", "Velum Radi\xc4\x81ns"})
            if (Cast(name)) return;
        if (Has(screen_, "Combat action:")) Key(1);
    }
    void RoomAfterTrial() {
        if (Has(screen_, "Combat action:")) { Key(5); Key(4); }
        if (Has(screen_, "Defeat review:")) { Key(1); Key(0); }
        if (Has(screen_, "Destination or panel:")) Key(2);
        if (!Has(screen_, "Room action:")) throw std::runtime_error("Artifact trial did not return to a room");
        Key(6);
    }
    void Fountain(int destination = 2) { Key(0); Key(1); Key(7); Key(0); Key(destination); }
    bool Encounter(const std::function<bool()>& eligible, int limit = 180) {
        for (int attempt = 0; attempt < limit; ++attempt) {
            if (Has(screen_, "Room action:")) { ++investigations_; Key(1); }
            if (Has(screen_, "Defeat review:")) { RoomAfterTrial(); Fountain(); }
            if (Has(screen_, "Combat action:")) {
                if (eligible()) return true;
                Key(4);
            }
        }
        Note("LIVE NOT REACHED: eligible enemy within " + std::to_string(limit) + " investigations"); return false;
    }
    void Begin(const std::string& label) {
        trial_ = label; trialEvents_.clear(); actions_ = 0;
        Note("BEGIN " + label); Snapshot("before");
    }
    void End(const std::string& classification) {
        Snapshot("after"); Note(classification);
        RoomAfterTrial(); observations_ << "DEDUPLICATED EVENTS " << trial_ << '\n' << trialEvents_ << '\n';
    }
public:
    Driver(const fs::path& dev, const fs::path& executable, const std::string& capture)
        : dev_(dev), replay_(dev / "validation" / (capture + "-replay-console.txt"), std::ios::binary),
          capture_(dev / "validation" / (capture + "-console.txt"), std::ios::binary),
          observations_(dev / "validation" / (capture + "-observations.txt")), game_(executable, {L"--fast"}, dev) {
        RequireConsole(executable);
        if (!replay_ || !capture_ || !observations_) throw std::runtime_error("Cannot write Artifact QA evidence");
        const auto keys = Reconstruct(dev);
        std::ostringstream fingerprint; fingerprint << std::hex << Fingerprint(ReadFile(executable));
        Note("Historical replay executable " + executable.string() + "; FNV1a64 " + fingerprint.str() + "; keyboard inputs " + std::to_string(keys.size()));
        screen_ = game_.Read(true, &replay_);
        for (std::size_t index = 0; index < keys.size(); ++index) {
            replay_ << keys[index] << '\n'; game_.Send(keys[index]); screen_ = game_.Read(true, &replay_);
            if (index % 5000 == 4999) Note("Historical keyboard replay " + std::to_string(index + 1) + "/" + std::to_string(keys.size()));
        }
        if (!Has(screen_, "=== ROOM: Entrance Hall ===") || !Has(screen_, "Level 100 | HP 2575/4269 | ATK 3580 DEF 1811 SPD 142 | XP 380271/1262643"))
            throw std::runtime_error("Historical replay diverged; refusing to label gear as the original earned character");
        capture_ << "Earned-character continuation after replaying all historical inputs except final leave/quit.\n" << screen_;
        Key(4); ScanEarnedInventory();
    }
    void Run() {
        const auto original = equipped_;
        const std::vector<std::string> support{"Prism Step", "Ray of Mercy", "Scutum Coruscum", "Velum Radi\xc4\x81ns", "Gleam Needle", "Lucerna Echo"};
        Configure(support);

        Equip("choirs_eternal_tome"); Fountain(); Begin("Choir's Eternal Tome: first successful basic attack");
        if (Encounter([] { return true; })) {
            for (int i = 0; i < 3 && Has(screen_, "Combat action:"); ++i) {
                Key(1);
                if (Has(trialEvents_, "QA_Final hits ") || Has(trialEvents_, "QA_Final critically hits ")) break;
            }
        }
        End(Has(trialEvents_, "QA_Final hits ") ? "LIVE FAIL: first successful Attack was not guaranteed critical." : "LIVE ATTEMPT: inspect first-hit critical damage; random critical alone cannot certify the unique.");

        Equip("verdigris_saint_step"); Fountain(); Begin("Verdigris Saint Step: six completed normal actions then next action");
        if (Encounter([] { return true; })) for (int i = 0; i < 9 && Has(screen_, "Combat action:"); ++i) Support();
        End(actions_ >= 7 ? "LIVE TRIGGER REACHED: six-action threshold crossed; inspect gauge/turn log. SOURCE FAIL: unique has no runtime consumer." : "LIVE NOT REACHED: six-action threshold interrupted.");

        Equip("verdigris_saint_relic"); Fountain(); Begin("Verdigris Saint Relic: successful direct Attack should apply Mark");
        if (Encounter([] { return true; })) for (int i = 0; i < 3 && Has(screen_, "Combat action:"); ++i) {
            Key(1); if (Has(trialEvents_, "QA_Final hits ") || Has(trialEvents_, "QA_Final critically hits ")) break;
        }
        End(Has(trialEvents_, "receives Verdigris") ? "LIVE TRIGGER OBSERVED: inspect Mark duration/source." : "LIVE FAIL: successful direct Attack produced no Verdigris Mark.");

        Equip("first_light_crucible"); Fountain(); Begin("First Light Crucible: original support Shimmer increases final ATK");
        if (Encounter([] { return true; })) { const int before = attack_; Cast("Prism Step"); Note("ATK before " + std::to_string(before) + "; after " + std::to_string(attack_)); }
        End("LIVE TRIGGER REACHED: original Shimmer cast; inspect ATK. SOURCE FAIL: unique has no runtime consumer.");

        Equip("first_light_sandals"); Fountain(); Begin("First Light Sandals: successive normal turn starts");
        if (Encounter([] { return true; })) for (int i = 0; i < 4 && Has(screen_, "Combat action:"); ++i) { Snapshot("normal-turn-start"); Support(); }
        End("LIVE TRIGGER REACHED: normal starts recorded; inspect unchanged SPD. SOURCE FAIL: unique has no runtime consumer.");

        Equip("first_light_sanctum_attire"); Fountain(); Begin("First Light Sanctum Attire: direct enemy damage should stack final DEF");
        if (Encounter([&] { return enemy_ == "Devotee Penitent"; })) for (int i = 0; i < 8 && Has(screen_, "Combat action:"); ++i) {
            Key(1); Snapshot("after-action-and-enemy-response"); if (hp_ < maxHp_) break;
        }
        End("LIVE ATTEMPT: direct enemy-hit and DEF snapshots captured; classify trigger from damage events. SOURCE FAIL: unique has no runtime consumer.");

        Equip("choirs_eternal_ritual_gown"); Fountain(); Begin("Choir's Eternal Ritual Gown: receive or resist hostile Bleed");
        if (Encounter([&] { return enemy_ == "Devotee Penitent" || enemy_ == "Choir Warden"; })) for (int i = 0; i < 12 && Has(screen_, "Combat action:"); ++i) {
            Support(); if (Has(trialEvents_, "QA_Final receives Bleed") || Has(trialEvents_, "QA_Final resists Bleed")) break;
        }
        End(Has(trialEvents_, "QA_Final receives Bleed") ? "LIVE FAIL: Bleed landed while gown equipped; immunity missing." : "LIVE ATTEMPT: inspect DOT resistance/cleanse healing and barrier; absence of a landed DOT alone does not certify immunity.");

        Equip("locket_crown_of_the_first_light"); Fountain(); Begin("Locket Crown of the First Light: injured direct critical");
        bool injuredCritical = false;
        for (int fight = 0; fight < 8 && !injuredCritical; ++fight) {
            if (!Encounter([] { return true; }, 40)) break;
            for (int i = 0; i < 14 && Has(screen_, "Combat action:"); ++i) {
                const int before = hp_; const auto firstEvent = trialEvents_.size(); Key(1);
                if (before < maxHp_ && Has(trialEvents_.substr(firstEvent), "QA_Final critically hits ")) { injuredCritical = true; break; }
            }
            if (!injuredCritical) { RoomAfterTrial(); Fountain(); }
        }
        End(injuredCritical ? "LIVE TRIGGER REACHED: direct critical while injured; inspect immediate 8% healing/cooldown event. SOURCE FAIL: unique has no runtime consumer." : "LIVE NOT REACHED: no injured direct critical within eight encounters.");

        Equip("verdigris_saint_reliquary"); Configure({"Prism Step", "Ray of Mercy", "Flos Aeruginis", "Lux Vortex", "Pulsus Luminis"}); Fountain();
        Begin("Verdigris Saint Reliquary: actual hostile DOT HP damage healing/overheal barrier");
        if (Encounter([&] { return enemyMaxHp_ >= 30000; })) {
            Key(2); std::string powder;
            for (const auto& row : Rows(screen_, std::regex(R"((?:^|\n)(\d+)\. ritual_powder x\d+)"))) { powder = row[1]; break; }
            if (!powder.empty()) Key(powder); else { Key(0); Note("LIVE NOT REACHED: no available ritual_powder"); }
            for (int i = 0; i < 4 && Has(screen_, "Combat action:"); ++i) { if (!Cast("Prism Step")) Key(1); }
        }
        End("LIVE ATTEMPT: ritual powder Burn and subsequent target ticks captured; inspect actual HP damage, healing and barrier. SOURCE FAIL: unique has no runtime consumer.");

        Equip("verdigris_halo_of_saint_adelaide");
        Begin("Verdigris Halo of Saint Adelaide: detonation prerequisite audit");
        Note("LIVE NOT REACHED: Verdigris Saint Vestment was not legitimately obtained, so five-piece Matron detonation cannot be assembled. Baseline detonation is also absent. No item injected; no extra RNG farming.");
        End("SOURCE FAIL: Halo cooldown trigger has no runtime consumer; successful detonation not live verified.");

        const auto beforeEcho = equipped_; for (const auto& slot : beforeEcho) SetInstance(slot.second, false);
        Equip("first_light_reliquary"); Configure(support); Fountain(); Begin("First Light Reliquary: third original offensive Shimmer then fourth direct Shimmer");
        int offensive = 0;
        if (Encounter([&] { return enemyMaxHp_ >= 35000; })) {
            for (int i = 0; i < 40 && Has(screen_, "Combat action:") && offensive < 4; ++i) {
                const std::string skill = offensive < 3 ? "Lucerna Echo" : "Gleam Needle";
                if (Cast(skill)) { ++offensive; Note("Offensive original cast " + std::to_string(offensive) + ": " + skill); }
                else Support();
            }
        }
        End(offensive == 4 ? "LIVE TRIGGER REACHED: third offensive cast followed by fourth direct Shimmer; inspect actual-damage echo. SOURCE FAIL: unique has no runtime consumer." : "LIVE NOT REACHED: fourth offensive cast interrupted before legal echo trigger.");

        const auto remaining = equipped_; for (const auto& slot : remaining) SetInstance(slot.second, false);
        for (const auto& slot : original) SetInstance(slot.second, true);
        Snapshot("restored original earned equipment"); Note("Additional bounded investigations " + std::to_string(investigations_));
        Key(0); Key(0); game_.CloseInput(); capture_ << game_.Read(false);
        if (!game_.WaitForExit() || game_.ExitCode() != 0) throw std::runtime_error("Artifact audit console did not exit normally");
        Note("Audit process PASS: normal console exit0. Per-effect PASS/FAIL/NOT REACHED findings are above; process completion does not certify missing uniques.");
    }
};
}

inline void RunArtifactConsoleAudit(const fs::path& dev, const fs::path& executable, const std::string& capture) {
    ArtifactAudit::Driver audit(dev, executable, capture); audit.Run();
}
}
