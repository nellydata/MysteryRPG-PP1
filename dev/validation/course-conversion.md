# Course compliance conversion: 2026-09-08

The assessed deliverable is `dev/COS-PP1.slnx`, a Visual Studio C++17 console solution. Checkpoint `fa9bc0b` captures the existing gameplay, GUI, pending test changes, and harness output before restructuring. Branch `archive/gui-before-course-conversion` retains that checkpoint for future GUI development.

## Files and paths

Moved from the repository root into `dev`: `COS-PP1.slnx`, `COS-PP1.vcxproj`, `COS-PP1.vcxproj.filters`, `InvestigationHarness.vcxproj`, `EquipmentV2Tests.vcxproj`, and the local ignored `COS-PP1.vcxproj.user`.

Solution project paths remain sibling-relative. Project/filter paths changed from `dev/src`, `dev/include`, and `dev/tests` to `src`, `include`, and `tests`. Include search paths now use `$(ProjectDir)include`. All executables go to `$(ProjectDir)bin\$(Platform)\$(Configuration)\`; intermediates go to `$(ProjectDir)obj\$(ProjectName)\$(Platform)\$(Configuration)\`. Debugger working directories use `$(ProjectDir)`. Parent-directory documentation references were removed from the project, making `dev` self-contained.

All game configurations use the Console linker subsystem. `main.cpp` now defines standard `int main`; the Windows graphical entry point, console-allocation shim, `Win32UI.cpp`, `Win32UI.h`, and their project/filter entries were removed. No Python source, Python runtime dependency, GUI framework, or graphical startup target remains in the assessed application. `Win32` platform names and the Visual Studio `Win32Proj` project keyword describe native C++ project metadata, not a GUI dependency.

The root VS Code build task now builds `dev/COS-PP1.slnx`; its debugger launches `dev/bin/x64/Debug/COS-PP1.exe` with `dev` as the working directory. Machine-specific `.user`, generated `bin`/`obj`, and IDE caches are ignored.

Stale generated executables and compiler intermediates at the repository root and in `dev/tests` were removed after verifying they were untracked build artifacts. The game filter no longer lists the standalone harness source; both added regression headers are visible in the harness project.

## Gameplay and correctness

ConsoleUI presents state and numbered controls. GameManager owns character setup/navigation/recovery. EncounterManager uses the shared investigation and victory resolvers; CombatManager remains authoritative for actions, gauges, damage, events, and XP claims. Player/Inventory retain equipment, consumable, instance protection, and stat rules. ProgressionManager owns level-up rewards and Shimmer loadouts.

The console now provides real inventory and loadout actions rather than read-only placeholder menus. It displays rewards without clearing them, presents each combat event once using its sequence ID, preserves a full encounter for review, and requires defeat acknowledgment before Fountain recovery. EOF and malformed keyboard input are handled without loops or invalid indexing. Temporary statuses and enemy ownership are reset after encounters, while cleared rooms and first-clear state remain available.

Validation exposed missing room values left at defaults: first-clear boss level/readiness/XP are now explicitly Entrance 8/8/200, Archive 15/15/400, Chapel 22/22/800, Storage 30/30/1600. Eligible first-clear spawning remains 50%; farm boss selection remains a separate 8% draw within eligible cleared-room enemy encounters.

Queued Shimmer rewards now copy the selected name before removing its owning queue and remove already-unlocked choices from pending rewards. This repairs stale references and stalled multi-level reward selection. Combat logging also preserves battle-start effects, records actual Shimmer damage/status results, targets DOT skills correctly, retains the combined barrier/accuracy skill, and records guaranteed critical skills correctly. A dead player cannot receive victory XP.

Lethal exploration hazards start a fresh event history using the actual damage result from Player. They retain the requested and applied HP damage, final cause, death, and exactly one completion event even after a prior completed fight.

## Executed validation

From `dev`:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo
.\bin\x64\Debug\EquipmentV2Tests.exe
.\bin\x64\Debug\InvestigationHarness.exe
.\bin\x64\Debug\COS-PP1.exe --validate
.\bin\x64\Debug\COS-PP1.exe --fast
```

The solution built and all validation executables exited 0, including the final hazard regression. An initial sandboxed MSBuild attempt hit FileTracker access denial; the authorized build outside the sandbox succeeded without source compilation errors. The final [Debug x64 build output](build-debug-x64.txt) and [full numeric results](progression-validation.txt) are captured here.

| Level | Basic observed | Rare observed | Epic observed | Elite observed |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 90.033% | 9.967% | 0% | 0% |
| 5 | 80.048% | 18.005% | 1.947% | 0% |
| 10 | 65.020% | 28.018% | 6.962% | 0% |
| 20 | 44.747% | 35.157% | 17.074% | 3.022% |
| 40 | 25.095% | 34.887% | 30.000% | 10.018% |
| 70 | 10.106% | 25.053% | 39.885% | 24.956% |
| 100 | 4.899% | 15.013% | 39.998% | 40.090% |

Each row contains 100,000 seeded selections. Fresh investigation observations were 25.420% enemy, 35.135% item, 17.185% XP, 12.285% discovery, 5.985% beneficial, 3.990% hazard. Repeat observations were 30.190%, 41.990%, 19.735%, 0%, 3.955%, and 4.130%. Observed consumable shares were 69.987% and 79.960%. Every successful item/XP result was checked against inventory/XP mutation.

| Target level | Mean investigations | Mean ordinary battles | Mean item events | Mean exploration XP events |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 21.469 | 4.469 | 5.281 | 2.750 |
| 5 | 137.531 | 22.031 | 29.125 | 14.906 |
| 10 | 318.750 | 50.656 | 66.406 | 33.469 |

These are 32 simulations that deliberately flee first-clear bosses and use ordinary Attack, earned equipment, and Fountain recovery. Battles include the Fountain guardian; exploration XP event counts exclude the two finite clue events. At level 10 the fixed boss was defeated in 32/32 simulated readiness checks. Simulations are separate from the live keyboard smoke test.

The original baseline level-1 Acolyte had 280 HP and killed the 105-HP starter after 20 player actions. The repaired Acolyte has 54 HP, takes four starter actions, deals 16 total HP damage, and grants 10 XP. The repaired level-1 Flagellant has 88 HP, takes five starter actions, deals 48 total HP damage (largest action 13), and grants 14 XP. These values use starter gear and ordinary Attack, not later acquired upgrades.

## Actual console smoke

The real executable ran inside a terminal and accepted keyboard input throughout. [console-smoke.txt](console-smoke.txt) records the session, including terminal wrapping.

1. Initialized Adelaide at level 1, 105 HP, 20 ATK, 10 DEF.
2. Fought the Fountain guardian, used cellar rations to recover 8 HP, received 10 XP and loot, unlocked Entrance Hall, and used Fountain healing.
3. Investigated Entrance Hall, received real consumables/equipment and visible 12/11 XP events, equipped an earned +81 ATK book and the starting veil, and won ordinary encounters.
4. Completed discoveries and encountered the same 524-HP level-8 Matron at player levels 1 and 2.
5. Lost the first attempt. Burn requested 18 damage with only 8 HP remaining; the log showed 8 actual HP damage, defeat, and completion. The defeat review remained open. Full review retained events 57-100, including the encounter start. Acknowledgment returned Adelaide to the Fountain at 110/110 HP with 64 XP and Archive still locked.
6. Retried/fled without first-clear rewards, reached level 2 through investigation/ordinary fights, selected Gleam Needle, equipped it, toggled normal presentation, and used it in combat.
7. Defeated the fixed Matron with earned equipment and Shimmer, gained exactly 200 first-clear XP, selected the next level-up reward, and entered the newly unlocked Archive Room. Returning to Entrance Hall preserved its cleared state and the complete reward log.
8. Quit normally; the process exited 0. The executable's PE subsystem was 3 (console), its running MainWindowHandle was 0, and DLL dependencies were the C++ debug runtime plus KERNEL32, with no USER32, GDI, or Python DLL.

A second [keyboard check of the final executable](final-console-check.txt), after the hazard fix, initialized a fresh character, defeated the Fountain guardian with four ordinary attacks, earned 10 XP and loot, unlocked Entrance Hall, healed at the Fountain, and quit with exit code 0. Console subsystem, zero graphical window handle, and DLL dependencies were rechecked on this final build.

No known course-compliance issue remains for Debug x64. Disk saving was not part of the existing game. Existing item and Shimmer strengths allowed the live boss clear at player level 2; the stated readiness level is advisory and further balance changes were kept outside this console conversion. Release/x86 configurations were not runtime-tested.
