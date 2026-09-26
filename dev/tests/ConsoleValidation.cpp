#include "ConsoleEndurance.h"
#include <cstdlib>
#include <iomanip>

using namespace ConsoleQA;

namespace {
fs::path DevelopmentRoot() {
    auto candidate = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        if (fs::exists(candidate / "COS-PP1.slnx")) return candidate;
        if (fs::exists(candidate / "dev" / "COS-PP1.slnx")) return candidate / "dev";
        if (!candidate.has_parent_path()) break;
        candidate = candidate.parent_path();
    }
    throw std::runtime_error("Run ConsoleValidation from the repository or dev folder");
}

std::string Trim(std::string text) {
    const auto end = text.find_last_not_of("\r\n \t");
    return end == std::string::npos ? "" : text.substr(0, end + 1);
}

void Smoke(const fs::path& dev, const fs::path& executable, const std::string& capture) {
    RequireConsole(executable);
    Process game(executable, {L"--fast"}, dev);
    std::ofstream transcript(dev / "validation" / (capture + "-console.txt"), std::ios::binary);
    std::string output = game.Read(true, &transcript);
    const std::vector<std::string> keys{
        "ConsoleInventorySmoke",
        // Inventory overview -> Equipment -> linen detail -> Back twice.
        "6", "1", "1", "0", "0",
        // Consumables -> cellar rations detail -> Back to dashboard.
        "2", "2", "0", "0", "0",
        // Shimmer loadout view -> dashboard.
        "7", "0",
        // Fountain investigation, real combat, Pulsus Luminis, leave room.
        "1", "1", "3", "1", "0",
        // Visit the newly unlocked Entrance Hall, then leave and quit.
        "2", "0", "0"
    };
    for (const auto& key : keys) {
        transcript << key << '\n'; game.Send(key); output += game.Read(true, &transcript);
    }
    game.CloseInput(); output += game.Read(false, &transcript);
    if (!game.WaitForExit() || game.ExitCode()) throw std::runtime_error("Console smoke exited unsuccessfully");
    const std::vector<std::string> expected{
        "ConsoleInventorySmoke  Lv. 1", "INVENTORY", "Equipment    2", "Consumables  2",
        "=== EQUIPMENT page 1/1 ===", "linen_veil", "EQUIP CHANGES", "[0] Categories",
        "=== CONSUMABLES page 1/1 ===", "cellar_rations [Common] x1", "Quantity: 1",
        "SHIMMER SKILLS", "Pulsus Luminis", "=== ROOM: Fountain ===",
        "A weak guardian rises from the sealed Fountain", "COMBAT", "Choose combat Shimmer",
        "uses Shimmer skill Pulsus Luminis", "The Fountain is purified.",
        "Entrance Hall is now unlocked.", "=== ROOM: Entrance Hall ===", "Session complete."
    };
    for (const auto& needle : expected) if (!Contains(output, needle)) throw std::runtime_error("Console smoke missed: " + needle);
    for (const auto& needle : {"The Fountain is purified."}) {
        const auto first = output.find(needle);
        if (first == std::string::npos || output.find(needle, first + 1) != std::string::npos)
            throw std::runtime_error("Duplicate or missing smoke effect: " + std::string(needle));
    }
    std::cout << "Console smoke passed: fresh character, category-first Equipment/detail/Back and Consumables navigation, Shimmer view, Fountain investigation/combat, unlock, room navigation; exit 0; "
#ifdef _WIN32
        "PE subsystem 3.\n";
#else
        "executable console process.\n";
#endif
}

void Replay(const fs::path& dev, const fs::path& executable, const fs::path& source, const std::string& capture) {
    RequireConsole(executable);
    std::vector<std::string> keys;
    if (fs::is_regular_file(source)) {
        std::ifstream input(source); std::string line;
        while (std::getline(input, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); keys.push_back(line); }
    } else {
        std::vector<fs::path> segments;
        for (const auto& entry : fs::directory_iterator(dev / "validation")) {
            const auto name = entry.path().filename().string();
            if (name.rfind(source.string() + "-console-", 0) == 0 && entry.path().extension() == ".txt")
                segments.push_back(entry.path());
        }
        std::sort(segments.begin(), segments.end());
        for (size_t i = 0; i < segments.size(); ++i) {
            std::ifstream input(segments[i]); std::string line; size_t number = 0;
            while (std::getline(input, line)) {
                ++number; if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.rfind("> ", 0) == 0) keys.push_back(line.substr(2));
                else if (i && number == 2 && std::regex_match(line, std::regex("[0-9]+"))) keys.push_back(line);
            }
        }
        const auto state = ReadFile(dev / "validation" / (source.string() + "-state.json"));
        std::smatch count;
        if (!std::regex_search(state, count, std::regex(R"("Inputs"\s*:\s*(\d+))")) || keys.size() != std::stoull(count[1]))
            throw std::runtime_error("Transcript reconstruction does not match recorded input count");
    }
    if (keys.empty()) throw std::runtime_error("Replay has no keyboard input");
    Process game(executable, {L"--fast"}, dev);
    std::ofstream output(dev / "validation" / (capture + "-console.txt"), std::ios::binary);
    auto screen = game.Read(true, &output);
    size_t submitted = 0;
    for (const auto& key : keys) {
        if (game.Exited()) throw std::runtime_error("Game exited before all replay keys were submitted");
        output << key << '\n'; game.Send(key); screen = game.Read(true, &output); ++submitted;
    }
    game.CloseInput(); const auto tail = game.Read(false, &output);
    if (!game.WaitForExit() || game.ExitCode() || (!Contains(screen, "Session complete.") && !Contains(tail, "Session complete.")))
        throw std::runtime_error("Replay did not reach normal console exit");
    std::cout << "Replay passed: " << submitted << " keyboard lines; exit 0. This confirms process completion; compare the captured gameplay state separately.\n";
}

std::string Xml(std::string value) {
    std::string escaped;
    for (char c : value) {
        if (c == '&') escaped += "&amp;";
        else if (c == '"') escaped += "&quot;";
        else if (c == '<') escaped += "&lt;";
        else escaped += c;
    }
    return escaped;
}

fs::path FindMSBuild(const fs::path& dev) {
#ifdef _WIN32
    wchar_t programFiles[32768]{};
    GetEnvironmentVariableW(L"ProgramFiles(x86)", programFiles, 32768);
    auto vswhere = fs::path(programFiles) / "Microsoft Visual Studio/Installer/vswhere.exe";
    if (!fs::exists(vswhere)) {
        GetEnvironmentVariableW(L"ProgramFiles", programFiles, 32768);
        vswhere = fs::path(programFiles) / "Microsoft Visual Studio/Installer/vswhere.exe";
    }
    unsigned code = 0;
    auto path = Trim(CaptureProcess(vswhere, {L"-latest",L"-products",L"*",L"-requires",
        L"Microsoft.VisualStudio.Component.VC.Tools.x86.x64",L"-find",L"MSBuild\\Current\\Bin\\MSBuild.exe"}, dev, code));
    if (code || !fs::exists(fs::u8path(path))) throw std::runtime_error("Visual Studio C++ MSBuild not found");
    return fs::u8path(path);
#else
    (void)dev;
    return "/usr/bin/g++";
#endif
}

void Headers(const fs::path& dev) {
    const auto output = dev / "obj" / "HeaderChecks";
    fs::create_directories(output);
    const std::map<std::string, std::string> construction{
        {"Area.h","Area area;"},{"Clue.h","Clue clue;"},{"CombatManager.h","CombatManager combat;"},
        {"CombatMath.h","auto roll = CombatMath::SubstatRoll(ItemStat::ATK, Rarity::Common, 1u);"},
        {"CombatResolution.h","DamageResolution damage;"},{"ConsoleUI.h","ConsoleUI ui;"},
        {"ContentCatalog.h","auto items = BuildEquippableCatalog();"},
        {"ConsumableEffects.h","auto effect = FindConsumableEffect(\"restore_20_percent_hp\");"},
        {"EncounterManager.h","EncounterManager encounters; InvestigationResult investigation; VictoryResult victory;"},
        {"Enemy.h","EnemyDefinition definition; Enemy enemy(definition, 1);"},
        {"EnemyCatalog.h","auto enemy = CreateRandomCatalogueEnemy(1u, 1);"},
        {"EquipmentStats.h","auto stats = EquipmentStats::Finalize(Stats{});"},{"GameManager.h","GameManager game;"},
        {"GameplayTypes.h","Stats stats; ItemDefinition item; EnemyDefinition enemy; RoomDefinition room;"},
        {"InteractionManager.h","InteractionManager interactions;"},{"Inventory.h","Inventory inventory; ItemEntry entry;"},
        {"Item.h","Item item;"},{"MapManager.h","MapManager map;"},{"Player.h","Player player;"},
        {"ProgressionManager.h","ProgressionManager progression;"},{"ShimmerSkill.h","auto skill = ShimmerSkill::Create(\"Pulsus Luminis\");"},
        {"RandomService.h","RandomStream random(1); auto roll = random.RollPercent(50);"},
        {"StatusRules.h","auto id = StatusRules::NextIdentity();"}
    };
    std::vector<fs::path> headers;
    for (const auto& entry : fs::directory_iterator(dev / "include"))
        if (entry.path().extension() == ".h") headers.push_back(entry.path());
    std::sort(headers.begin(), headers.end());
    std::ostringstream project;
    project << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<Project DefaultTargets=\"ClCompile\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
        "<ItemGroup Label=\"ProjectConfigurations\"><ProjectConfiguration Include=\"Debug|x64\"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>\n"
        "<PropertyGroup Label=\"Globals\"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>\n"
        "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n"
        "<PropertyGroup Label=\"Configuration\"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v145</PlatformToolset><UseDebugLibraries>true</UseDebugLibraries></PropertyGroup>\n"
        "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n"
        "<PropertyGroup><IntDir>$(ProjectDir)intermediate\\</IntDir><OutDir>$(ProjectDir)output\\</OutDir></PropertyGroup>\n"
        "<ItemDefinitionGroup><ClCompile><LanguageStandard>stdcpp17</LanguageStandard><ConformanceMode>true</ConformanceMode><ExceptionHandling>Sync</ExceptionHandling><AdditionalOptions>/Zs %(AdditionalOptions)</AdditionalOptions><AdditionalIncludeDirectories>"
        << Xml((dev / "include").string()) << "</AdditionalIncludeDirectories></ClCompile></ItemDefinitionGroup>\n<ItemGroup>\n";
    int total = 0;
    for (const auto& header : headers) {
        std::vector<std::pair<std::string, std::string>> checks{{"include",""}};
        const auto contract = construction.find(header.filename().string());
        if (contract != construction.end()) checks.emplace_back("construction", contract->second);
        for (const auto& check : checks) {
            const auto fixture = header.stem().string() + "." + check.first + ".cpp";
            WriteFile(output / fixture, "#include \"" + header.filename().string() + "\"\nint main() { " + check.second + " return 0; }\n");
            project << "<ClCompile Include=\"" << fixture << "\" />\n"; ++total;
        }
    }
    project << "</ItemGroup><Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" /></Project>\n";
    WriteFile(output / "HeaderChecks.vcxproj", project.str());
#ifdef _WIN32
    unsigned code = 0;
    const auto result = CaptureProcess(FindMSBuild(dev), {(output / "HeaderChecks.vcxproj").wstring(),
        L"/t:ClCompile",L"/p:Configuration=Debug",L"/p:Platform=x64",L"/v:minimal",L"/nologo"}, dev, code);
    WriteFile(output / "compiler-output.txt", result);
    if (code) { std::cerr << result; throw std::runtime_error("Isolated header compilation failed"); }
    const auto summary = "Header checks: " + std::to_string(total) + "/" + std::to_string(total) +
        " isolated include/construction fixtures passed under MSVC C++17 /permissive- /Zs.\n";
#else
    std::ostringstream compilerOutput;
    int passed = 0;
    for (const auto& fixture : fs::directory_iterator(output)) {
        if (fixture.path().extension() != ".cpp") continue;
        unsigned code = 0;
        const auto result = CaptureProcess(FindMSBuild(dev), {L"-std=c++17", L"-pedantic-errors", L"-fsyntax-only",
            L"-I" + (dev / "include").wstring(), fixture.path().wstring()}, dev, code);
        compilerOutput << "== " << fixture.path().filename().string() << " ==\n" << result;
        if (code) { WriteFile(output / "compiler-output.txt", compilerOutput.str()); throw std::runtime_error("Isolated header compilation failed"); }
        ++passed;
    }
    WriteFile(output / "compiler-output.txt", compilerOutput.str());
    const auto summary = "Header checks: " + std::to_string(passed) + "/" + std::to_string(total) +
        " isolated include/construction fixtures passed under GCC C++17 -pedantic-errors -fsyntax-only.\n";
#endif
    WriteFile(output / "summary.txt", summary); std::cout << summary;
}
}

int main(int argc, char** argv) {
    try {
        const auto dev = DevelopmentRoot();
        std::vector<std::string> args(argv + 1, argv + argc);
        auto executable = dev / "bin/x64/Debug/COS-PP1.exe";
        for (size_t i = 0; i < args.size();) {
            if (args[i] == "--exe") {
                if (i + 1 == args.size()) throw std::runtime_error("--exe requires an executable path");
                executable = fs::absolute(fs::u8path(args[i + 1]));
                args.erase(args.begin() + i, args.begin() + i + 2);
            } else ++i;
        }
        std::string mode = !args.empty() ? args[0] : "--help";
        if (mode == "--headers") Headers(dev);
        else if (mode == "--smoke") Smoke(dev, executable, args.size() > 1 ? args[1] : "cpp-smoke");
        else if (mode == "--replay") {
            if (args.size() < 2) throw std::runtime_error("--replay needs an input text file or segmented capture prefix");
            Replay(dev, executable, args[1], args.size() > 2 ? args[2] : "cpp-replay");
        } else if (mode == "--endurance") {
            const int target = args.size() > 2 ? std::stoi(args[2]) : 100;
            const int extra = args.size() > 3 ? std::stoi(args[3]) : 250;
            if (target < 1 || target > 100 || extra < 0) throw std::runtime_error("Invalid endurance bounds");
            Endurance run(dev, executable, args.size() > 1 ? args[1] : "cpp-endurance");
            run.Run(target, extra, args.size() > 4 && args[4] == "--sets");
        } else {
            std::cout << "C++ console QA; run from dev or repository root:\n"
                "  ConsoleValidation --smoke [capture-prefix]\n"
                "  ConsoleValidation --headers\n"
                "  ConsoleValidation --replay <input.txt|segmented-capture-prefix> [output-prefix]\n"
                "  ConsoleValidation --endurance [capture-prefix] [target-level=100] [extra-investigations=250] [--sets]\n"
                "Endurance starts a fresh character and drives only ordinary keyboard menus.\n";
            std::cout << "All gameplay modes accept --exe <path> to test a separately built real executable.\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Console validation failed: " << error.what() << '\n'; return 1;
    }
}
