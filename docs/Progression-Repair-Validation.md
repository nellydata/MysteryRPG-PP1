# Progression repair validation

This report records the September 7, 2026 repair. The automated baseline was reproduced before rebuilding the modified code. Native desktop GUI automation is unavailable in this session; the interactive smoke test below remains manual and is not reported as passed.

## Reproduced baseline

The original Debug x64 solution built successfully and both existing standalone executables returned exit code 0. Those passing regressions did not detect the reported progression failures.

A temporary executable linked against the baseline InvestigationHarness object files ran real `CombatManager` encounters using basic Attack. It reproduced the GUI starter equipment: `plain_rosary` equipped, `linen_veil` available but unequipped, full health, and `SetLevelStats(level)`. No Shimmer skills, consumables, synthetic attack boosts, or assumed victories were used.

| Level | Player HP / ATK / DEF | Enemy | Enemy HP / ATK / DEF | XP | Player actions | Enemy actions | HP damage received | Largest damage in an action | Result |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 105 / 20 / 10 | Acolyte | 280 / 8 / 14 | 8 | 20 | 18 | 105 | 8 | Defeat |
| 1 | 105 / 20 / 10 | Penitent | 420 / 31 / 21 | 11 | 3 | 3 | 105 | 73 | Defeat |
| 1 | 105 / 20 / 10 | Flagellant | 455 / 30 / 25 | 11 | 3 | 3 | 105 | 53 | Defeat |
| 1 | 105 / 20 / 10 | Matron | 1,750 / 70 / 63 | 32 | 0 | 1 | 105 | 105 | Opening defeat |
| 5 | 210 / 80 / 50 | Acolyte | 534 / 15 / 26 | 12 | 9 | 7 | 70 | 14 | Victory |
| 5 | 210 / 80 / 50 | Penitent | 800 / 58 / 39 | 16 | 3 | 3 | 210 | 132 | Defeat |
| 5 | 210 / 80 / 50 | Flagellant | 867 / 55 / 45 | 16 | 4 | 4 | 210 | 94 | Defeat |
| 5 | 210 / 80 / 50 | Matron | 3,539 / 132 / 119 | 46 | 0 | 1 | 210 | 210 | Opening defeat |
| 10 | 341 / 155 / 100 | Acolyte | 914 / 25 / 42 | 18 | 8 | 7 | 115 | 23 | Victory |
| 10 | 341 / 155 / 100 | Penitent | 1,372 / 95 / 63 | 26 | 3 | 3 | 341 | 209 | Defeat |
| 10 | 341 / 155 / 100 | Flagellant | 1,486 / 89 / 74 | 26 | 4 | 4 | 341 | 145 | Defeat |
| 10 | 341 / 155 / 100 | Matron | 6,531 / 224 / 201 | 73 | 0 | 1 | 341 | 341 | Opening defeat |

The rosary adds 5% HP, explaining the difference from naked player HP of 100, 200, and 325. Enemy defensive skills prolong battles beyond a simple HP/ATK estimate. The Matron has speed 121 versus player speed 100 and previously resolved a lethal opening before a player action.

## Automated coverage

`dev/tests/ProgressionRepairTests.h` extends the existing InvestigationHarness entry point without replacing its correctness checks.

- 100,000 seeded ordinary catalogue selections at each of Levels 1, 5, 10, 20, 40, 70, and 100; observed tier frequencies; enemy minimum levels; no ordinary bosses; room membership; faction restrictions; empty-tier normalization; interpolation; selection trace fields.
- 20,000 independent fresh-room and 20,000 exhausted repeat-room investigations; event frequencies and category split; actual inventory quantity mutation; exact XP mutation; replayed deterministic seeds for all six weighted outcomes; fifth eligible search discovery pity; continued item and XP discoveries after finite rewards are exhausted.
- Real starter-equipment fights at Levels 1, 5, and 10 against Acolyte, Unit-01, Penitent, and Flagellant. Basic encounters must finish in 2–4 player actions, all selected encounters must be winnable, and Level-1 Rare actions must stay below half starter HP.
- Fixed first-clear boss stats through the investigation resolver at player Levels 1, 8, and 20; 10,000 eligible boss spawn draws; living-enemy and dead-player reward rejection; retry availability; first-clear unlock/rewards exactly once; farm flags and differing farm stats at Levels 1 and 20.
- Thirty-two fresh progression runs starting with the Fountain guardian and continuing through Entrance Hall. The simulation resolves actual combat, grants actual loot/XP, equips upgrades already present in inventory, counts defeats and Fountain recoveries, and executes Flee against first-clear bosses. Reaching Level 10 requires no boss kill. After reaching Level 10 it attempts the fixed boss with the earned equipment.

Simulation policy: basic Attack only, no Shimmer and no consumable use; item upgrades chosen by a simple HP/ATK/DEF/speed score; visit the already-purified Fountain when HP is below 60%; deaths recover at the Fountain and are counted. Ordinary battle totals include the one Fountain guardian. Investigation totals exclude the initial Fountain activation. Boss withdrawal investigations remain in the investigation count. Exploration XP event counts include one-time discoveries. This is an automated gameplay simulation, not measured human play time or a GUI test.

## Commands

Run from the repository root in PowerShell:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' COS-PP1.slnx /m /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal
& '.\x64\Debug\InvestigationHarness.exe'
& '.\x64\Debug\EquipmentV2Tests.exe'
```

The existing `.vscode/tasks.json` build configuration is preserved.

## Manual Win32 smoke checklist — pending

1. Launch `x64\Debug\COS-PP1.exe`. Start a fresh Adelaide session. Verify Level 1, 105 maximum HP with the starting rosary, and the Fountain destination. Do not enable developer/max-level equipment.
2. Enter the Fountain and investigate. Watch encounter start, gauge/turn owner, player attack, enemy skill, damage and remaining HP appear in order. Defeat the guardian. Confirm the Fountain becomes restorative and Entrance Hall unlocks. Heal at the Fountain.
3. Enter Entrance Hall and investigate repeatedly. Record the first 20 outcomes. Confirm several actual item discoveries and exploration XP awards; compare inventory quantity and XP immediately before/after each. Verify the displayed item name and XP amount match the mutation.
4. Complete at least three ordinary Basic fights. Count player actions and health lost. Expect roughly 2–4 actions with starting equipment. When Flagellant appears, inspect the Flagellation damage and Defense Down message; it should remain dangerous without routinely taking half starting HP.
5. Reach Level 2 through ordinary fights and investigation. Verify the level-up display, all five equipment rewards, and Shimmer choice. Equip an earned upgrade through inventory, return to Entrance Hall, and continue. Record investigations and fights required.
6. Finish both required discoveries and clues. Verify eligibility messaging identifies the first boss as a fixed readiness check. Continue investigating and verify repeatable item/XP rewards remain available. An eligible boss should appear on about half investigation attempts; a short run need not match that percentage exactly.
7. Encounter Matron while below the readiness threshold. Verify its displayed fixed level/profile. Inspect the readable opening and use Flee, or deliberately lose. Confirm the next room stays locked and first-clear eligibility remains after fleeing/defeat.
8. On defeat, confirm the view stays on the defeat panel until acknowledgement. Verify the final cause, actual HP damage, defeat event and recent events are visible. Open the full encounter log and confirm it includes encounter start through completion, without duplicated events or a missing prefix after more than 40 events. Then acknowledge returning to the Fountain; verify intended recovery and retained progression.
9. Compare the same encounter presentation with the normal and fast/instant options. Check that button responsiveness remains normal and actions appear sequentially in normal mode. Verify fast mode changes presentation timing only. In a multi-enemy encounter, confirm separate enemies' actions repaint in order.
10. Continue earning levels and equipment, then defeat Matron around its intended readiness threshold. Confirm first-clear rewards, first-clear state and Archive Room unlock happen once. Revisit Entrance Hall, find a farming Matron, and verify its level scales to the player and it awards farming rewards without repeating the unlock/unique rewards.
11. Verify Fountain healing remains accessible after a failed boss attempt, retry Matron if uncleared, and confirm a later failed encounter never removes an existing room unlock. Check console entry behavior separately if exercising that executable route.

Interactive smoke status: **not run**. It requires a person operating the native Win32 interface; no native desktop automation tool is exposed in this session.
