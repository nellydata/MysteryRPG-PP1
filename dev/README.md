# Micare course console application

Open **COS-PP1.slnx in this folder** with Visual Studio 2026 (version 18), with the **Desktop development with C++** workload installed. Select **Debug / x64** and press **Ctrl+F5** to build and play or **F5** to debug. **COS-PP1** is the only project in the solution and is the startup application, so Run launches Micare itself. The game uses C++17, `int main`, and standard console input/output. No Python, GUI framework, or Win32 UI is required.

All solution files, game sources, headers, and build outputs are contained in `dev`. The existing graphical version is preserved in Git on `archive/gui-before-course-conversion` at checkpoint `fa9bc0b`; it is absent from this course build.

```text
dev/
  COS-PP1.slnx                    Solution: COS-PP1 is the only project
  COS-PP1.vcxproj                 Assessed console game project
  COS-PP1.vcxproj.filters
  include/                        Shared model/manager and console headers
  src/                            Standard C++ game and console implementation
  tests/                          Retained regression/QA sources (not in the solution)
  validation/                     Retained validation reports and evidence
  bin/x64/Debug/                  Generated executable (ignored)
  obj/COS-PP1/x64/Debug/          Generated intermediates (ignored)
```

The relocated local `COS-PP1.vcxproj.user` is ignored by Git. Shared working-directory settings live in the project file.

## Build and run
The submission is opened and run entirely from Visual Studio: open `COS-PP1.slnx`, select **Debug / x64**, and press **F5** or **Ctrl+F5**. `COS-PP1` is the only project in the solution and is the startup application, so Run launches Micare itself.

From this folder in a Visual Studio Developer Command Prompt:

```text
msbuild COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64
.\bin\x64\Debug\COS-PP1.exe --validate
.\bin\x64\Debug\COS-PP1.exe
```

`--validate` runs the content-safety checks (`ValidateMicareContent`) over the enemy, item, and room catalogues and exits 0 when every entry is well formed. `--fast` disables the 300 ms presentation pause. `--help` prints the usage line.

The repository intentionally has no editor-specific launch folder and no command launcher script. Use Visual Studio with `COS-PP1` as the startup project, or the MSBuild command above. Do not try to execute an individual `.cpp`, `.slnx`, README, or `COMMIT_EDITMSG` file as the game.

## Keyboard controls

Enter a character name, or press Enter for Adelaide. Every numbered choice requires Enter. Invalid input is rejected; end-of-input exits cleanly.

- Dashboard: rooms 1-5, inventory 6, Shimmer loadout 7, map 8, last encounter log 9, quit 0.
- Room: investigate 1 or 2, inventory 3, map 4, skills 5, last encounter log 6, leave 0. The purified Fountain offers healing with 7.
- Combat: attack 1, consumable 2, equipped Shimmer 3, flee 4, full log 5, toggle presentation speed 6. The actual gauge determines who acts first.
- Inventory: first choose Equipment, Consumables, Key Items, or Clues. Equipment pages support sorting and slot/rarity/set filters. Item detail shows authored effects and actual projected stat/set changes before confirmation. Equipped, locked, and story items remain protected from discard.
- Skills: choose each earned level-up reward, then equip/unequip unlocked skills within the 12-point loadout. A deferred reward remains available from the skills menu.
- Defeat: review recent events and the final damage source, optionally open the complete encounter, then acknowledge with 0 to recover at the Fountain. Room progress survives death and fleeing.

Combat events are printed once as actions resolve, with stable sequence IDs and an explicit full-log review. Output is never cleared between actions or room transitions. Normal presentation pauses 300 ms at action messages; `--fast` or combat option 6 removes that delay without changing combat calculations. The last encounter remains reviewable until another encounter starts.

## Code layout
| Area | Files |
| --- | --- |
| Entry point | `src/main.cpp`, `src/GameManager.cpp` |
| Console presentation | `src/ConsoleUI.cpp`, `include/ConsoleFormat.h`, `src/ConsoleFormat.cpp` |
| Combat | `src/CombatManager.cpp`, `include/CombatMath.h`, `include/StatusRules.h` |
| Exploration | `src/EncounterManager.cpp`, `src/MapManager.cpp`, `src/Area.cpp` |
| Progression | `src/ProgressionManager.cpp`, `src/ShimmerSkill.cpp` |
| Content catalogues | `src/EnemyCatalog.cpp`, `src/ContentCatalog.cpp`, `include/Catalog.h` |
| Player model | `src/Player.cpp`, `src/Inventory.cpp`, `src/Item.cpp` |

`ConsoleFormat` holds the pure string builders used by the console panels, so presentation text can be reasoned about without touching stdin/stdout. `Catalog` holds the shared name-lookup and validation helpers used by both the enemy and content catalogues.

There are no known course-compliance blockers in the tested Debug x64 configuration. Progress is retained during the running session; the game has no disk-save system. Development-only content is reached through the isolated Adelaide developer path in `DeveloperMode.cpp`, which does not alter the ordinary progression flow.

## Validation projects

The solution builds only the assessed game, so opening `COS-PP1.slnx` and pressing Run always starts Micare. The maintained regression and console-QA harnesses are kept as source for project history in `tests/`, but they are no longer referenced by the solution.

They can still be built from a Developer Command Prompt when a regression pass is wanted. Each harness compiles the game sources directly, so no solution entry is required:

```text
msbuild tests\ArtifactRegression.vcxproj /m /p:Configuration=Debug /p:Platform=x64
msbuild tests\ShimmerRegression.vcxproj /m /p:Configuration=Debug /p:Platform=x64
msbuild ConsoleValidation.vcxproj /m /p:Configuration=Debug /p:Platform=x64

.\bin\x64\Debug\ArtifactRegression.exe
.\bin\x64\Debug\ShimmerRegression.exe
.\bin\x64\Debug\ConsoleValidation.exe --headers --exe .\bin\x64\Debug\COS-PP1.exe
.\bin\x64\Debug\ConsoleValidation.exe --smoke --exe .\bin\x64\Debug\COS-PP1.exe
```

`ConsoleValidation --smoke` drives the real category-first inventory, Shimmer, Fountain combat, unlock, and navigation flow through keyboard input. Capture files it writes land in the ignored `validation/` output folder and are not submission content.

The recorded evidence for the final pass is kept in `validation/`, including `final-submission-report.md`, `full-playthrough-report.md`, `console-ux-report.md`, `inventory-ux-report.md`, and `milestone4-code-quality-report.md`.
