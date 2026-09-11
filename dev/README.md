# Micare course console application

Open **COS-PP1.slnx in this folder** with Visual Studio, select **Debug / x64**, and use **COS-PP1** as the startup project. The game uses C++17, `int main`, and standard console input/output. No Python, GUI framework, or Win32 UI is required.

All solution files, game sources, headers, standalone test projects, and build outputs are contained in `dev`. The existing graphical version is preserved in Git on `archive/gui-before-course-conversion` at checkpoint `fa9bc0b`; it is absent from this course build.

```text
dev/
  COS-PP1.slnx
  COS-PP1.vcxproj
  COS-PP1.vcxproj.filters
  InvestigationHarness.vcxproj
  EquipmentV2Tests.vcxproj
  ConsoleValidation.vcxproj
  include/                         Shared model/manager and console headers
  src/                             Standard C++ game and console implementation
  tests/                           Existing tests plus console regressions
  validation/                      Captured test and actual console smoke results
  bin/x64/Debug/                   Generated executables (ignored)
  obj/<project>/x64/Debug/          Separate generated intermediates (ignored)
```

The relocated local `COS-PP1.vcxproj.user` is ignored by Git. Shared working-directory settings live in the project files.

## Build and validate

From this folder in a Visual Studio Developer Command Prompt:

```text
msbuild COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64
.\bin\x64\Debug\EquipmentV2Tests.exe
.\bin\x64\Debug\InvestigationHarness.exe
.\bin\x64\Debug\COS-PP1.exe --validate
.\bin\x64\Debug\ConsoleValidation.exe --headers
.\bin\x64\Debug\ConsoleValidation.exe --smoke
.\bin\x64\Debug\COS-PP1.exe
```

All four projects build together. The two gameplay test projects compile their own copy of the shared gameplay sources. ConsoleValidation is a separate C++17 executable that drives the real game's keyboard input and captures its output; it does not link gameplay managers or inject player state. Its process and pipe transport uses Windows system APIs without a graphical interface. Each project has its own intermediate directory, and no source/include reference depends on the parent directory. The repository's VS Code task directly runs the installed Visual Studio 18 Community MSBuild executable; update that task's executable path if a different Visual Studio edition or location is used.

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

## Current validation checkpoint (2026-09-09)

- Visual Studio MSBuild Debug x64: game and both standalone targets built successfully.
- EquipmentV2Tests, InvestigationHarness, and content validation: exit code 0.
- Combat regressions cover damage/mitigation/barrier/overkill conservation, dead-entity healing rejection, safe large multipliers, source-specific DOT/Mark, duration ownership, seeded replay, invalid actions, Stun, cooldowns, bounded enemy bonus actions, and group-round ownership. Header checks passed 46/46.
- The harness checks 700,000 seeded ordinary enemy selections, 40,000 investigations with actual inventory/XP mutation, 900,000 loot selections, Basic gear pity, first-clear and farm boss rules, early fights, and 32 fresh progression simulations. Equipment tests validate 9,664 rolls, percentage/fixed aggregation, and unchanged state during previews. All 12 consumable definitions have runtime regression coverage.
- The repeatable keyboard smoke test starts a fresh character, locks/unlocks and attempts a locked consumable, equips gear, uses a consumable in combat, purifies the Fountain, checks the guaranteed Common drop and its actual stat comparison, protects equipped items from discard, heals, changes the Shimmer loadout, enters Entrance Hall, and exits.
- The tested executable has PE subsystem 3 (console), no USER32/GDI/Python dependencies, and had a zero GUI window handle during the running smoke test. Only the terminal was used.

Evidence and phase boundaries: [gameplay audit](validation/gameplay-audit.md), [automated results](validation/phase-3-tests.txt), [equipment tests](validation/phase-3-equipment.txt), and [actual console transcript](validation/phase-3-console.txt). Capture a new smoke transcript with `bin/x64/Debug/ConsoleValidation.exe --smoke capture-name`; existing evidence remains unchanged.

All maintained automation is C++. `ConsoleValidation --endurance capture-name 100 250 --sets` starts a fresh character, earns every level through the real console, performs 250 more investigations after reaching Level 100, checks Storage farming, and exercises available equipment sets using only items actually earned. Every submitted key and observed item detail is captured in `validation`. `--replay input.txt capture-name` replays a keyboard-input text file; a segmented historical capture prefix is also accepted. These captures are observations, not save files or injected player state. The former PowerShell validation scripts have been replaced by this target.

There are no known course-compliance blockers in the tested Debug x64 configuration. Progress is retained during the running session; the existing game has no disk-save system. This is a functional audit checkpoint, not certification of every gameplay system. The full ability catalogue, approved set/Artifact triggers and their action exceptions, discovery consistency, final balance, and the full console progression smoke remain for later phases. Earlier conversion-era balance results predate the percentage equipment repair.
