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
  include/                         Shared model/manager and console headers
  src/                             Standard C++ game and console implementation
  tests/                           Existing tests plus console regressions
  validation/                      Captured test and actual console smoke results
  bin/x64/Debug/                   Generated executables (ignored)
  obj/<project>/x64/Debug/          Separate generated intermediates (ignored)
```

The relocated local `COS-PP1.vcxproj.user` is ignored by Git. Shared working-directory settings live in the project files.

## Build and validate

From this folder in a Visual Studio Developer PowerShell:

```powershell
msbuild COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64
.\bin\x64\Debug\EquipmentV2Tests.exe
.\bin\x64\Debug\InvestigationHarness.exe
.\bin\x64\Debug\COS-PP1.exe --validate
.\bin\x64\Debug\COS-PP1.exe
```

All three projects build together. Test projects compile their own copy of the shared gameplay sources. Each project has its own intermediate directory, and no source/include reference depends on the parent directory. The repository's VS Code task and debugger configuration point to this solution and executable.

For VS Code, open the repository root and install the recommended Microsoft **C/C++** extension (`ms-vscode.cpptools`). It supplies the `cppvsdbg` debugger used by **Debug COS-PP1 (x64)**. If VS Code reports that `cppvsdbg` is unsupported, install/enable that extension and run **Developer: Reload Window**. To play directly in the terminal, use **Tasks: Run Task > Run COS-PP1 (console)**; this builds the solution and starts the game without a debugger.

## Keyboard controls

Enter a character name, or press Enter for Adelaide. Every numbered choice requires Enter. Invalid input is rejected; end-of-input exits cleanly.

- Dashboard: rooms 1-5, inventory 6, Shimmer loadout 7, map 8, last encounter log 9, quit 0.
- Room: investigate 1 or 2, inventory 3, map 4, skills 5, last encounter log 6, leave 0. The purified Fountain offers healing with 7.
- Combat: attack 1, consumable 2, equipped Shimmer 3, flee 4, full log 5, toggle presentation speed 6. The actual gauge determines who acts first.
- Inventory: choose an item to inspect and equip/unequip, use, lock/unlock, or discard. Paging and sorting retain separate rolled equipment instances. Equipped and locked items are protected from discard.
- Skills: choose each earned level-up reward, then equip/unequip unlocked skills within the 12-point loadout. A deferred reward remains available from the skills menu.
- Defeat: review recent events and the final damage source, optionally open the complete encounter, then acknowledge with 0 to recover at the Fountain. Room progress survives death and fleeing.

Combat events are printed once as actions resolve, with stable sequence IDs and an explicit full-log review. Output is never cleared between actions or room transitions. Normal presentation pauses 300 ms at action messages; `--fast` or combat option 6 removes that delay without changing combat calculations. The last encounter remains reviewable until another encounter starts.

## Validation completed on 2026-09-08

- Visual Studio MSBuild Debug x64: game and both standalone targets built successfully.
- EquipmentV2Tests, InvestigationHarness, and content validation: exit code 0.
- The harness checks 700,000 seeded ordinary enemy selections, 40,000 investigations with actual inventory/XP mutation, first-clear and farm boss rules, early fights, and 32 fresh progression simulations. Added console regressions cover lethal opening history, acknowledgment, stable IDs beyond 200 events, duplicate-free presentation, invalid input/EOF, and multiple queued Shimmer rewards.
- A real keyboard-driven console session purified the Fountain, completed ordinary fights, discovered items and XP, equipped a weapon, reached level 2 without boss XP, selected/equipped/used Shimmer, lost to the level-8 Matron, reviewed the entire defeat, recovered, retried, earned its 200 first-clear XP, entered Archive Room, and revisited cleared Entrance Hall.
- The tested executable has PE subsystem 3 (console), no USER32/GDI/Python dependencies, and had a zero GUI window handle during the running smoke test. Only the terminal was used.

Evidence: [automated results](validation/progression-validation.txt), [actual console transcript](validation/console-smoke.txt), and [conversion report](validation/course-conversion.md).

There are no known course-compliance blockers in the tested Debug x64 configuration. Progress is retained during the running session; the existing game has no disk-save system. Existing strong equipment and Shimmer values can allow boss clears earlier than the suggested readiness level; the smoke session cleared Matron at level 2 using earned equipment and Gleam Needle. Those existing item/skill balance values were preserved during this conversion.
