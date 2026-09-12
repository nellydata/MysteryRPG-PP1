# Micare course console application

Open **COS-PP1.slnx in this folder** with Visual Studio, select **Debug / x64**, and use **COS-PP1** as the startup project. The game uses C++17, `int main`, and standard console input/output. No Python, GUI framework, or Win32 UI is required.

All solution files, game sources, headers, and build outputs are contained in `dev`. The existing graphical version is preserved in Git on `archive/gui-before-course-conversion` at checkpoint `fa9bc0b`; it is absent from this course build.

```text
dev/
  COS-PP1.slnx
  COS-PP1.vcxproj
  COS-PP1.vcxproj.filters
  include/                        Shared model/manager and console headers
  src/                            Standard C++ game and console implementation
  bin/x64/Debug/                  Generated executable (ignored)
  obj/COS-PP1/x64/Debug/          Generated intermediates (ignored)
```

The relocated local `COS-PP1.vcxproj.user` is ignored by Git. Shared working-directory settings live in the project file.

## Build and run
From this folder in a Visual Studio Developer Command Prompt:

```text
msbuild COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64
.\bin\x64\Debug\COS-PP1.exe --validate
.\bin\x64\Debug\COS-PP1.exe
```

`--validate` runs the content-safety checks (`ValidateMicareContent`) over the enemy, item, and room catalogues and exits 0 when every entry is well formed. `--fast` disables the 300 ms presentation pause. `--help` prints the usage line. The repository's VS Code task directly runs the installed Visual Studio 18 Community MSBuild executable; update that task's executable path if a different Visual Studio edition or location is used.

For VS Code, open the repository root and install the recommended Microsoft **C/C++** extension (`ms-vscode.cpptools`). It supplies the `cppvsdbg` debugger used by **Debug COS-PP1 (x64)**. If VS Code reports that `cppvsdbg` is unsupported, install/enable that extension and run **Developer: Reload Window**. To play directly in the terminal, use **Tasks: Run Task > Run COS-PP1 (console)**; this builds the solution and starts the game without a debugger.

## Keyboard controls

Enter a character name, or press Enter for Adelaide. Every numbered choice requires Enter. Invalid input is rejected; end-of-input exits cleanly.

- Dashboard: rooms 1-5, inventory 6, Shimmer loadout 7, map 8, last encounter log 9, quit 0.
- Room: investigate 1 or 2, inventory 3, map 4, skills 5, last encounter log 6, leave 0. The purified Fountain offers healing with 7.
- Combat: attack 1, consumable 2, equipped Shimmer 3, flee 4, full log 5, toggle presentation speed 6. The actual gauge determines who acts first.
- Inventory: choose an item to inspect its main/substats, fixed bonuses, and effective equipment comparison. Equip/unequip, use, lock/unlock, and discard apply to the selected instance. Discard removes its whole stack; using a consumable removes one unit. Equipped, locked, and story items are protected from discard.
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

There are no known course-compliance blockers in the tested Debug x64 configuration. Progress is retained during the running session; the game has no disk-save system.
