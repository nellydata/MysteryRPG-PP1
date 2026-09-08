# Progression repair validation

This report preserves the September 7 progression baseline. The September 8 course deliverable is console-only and has passed an actual keyboard-driven smoke test. Current results and build commands are in [the conversion report](../dev/validation/course-conversion.md); the original Win32 implementation is preserved on the GUI archive branch.

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

Simulation policy: basic Attack only, no Shimmer and no consumable use; item upgrades chosen by a simple HP/ATK/DEF/speed score; visit the already-purified Fountain when HP is below 60%; deaths recover at the Fountain and are counted. Ordinary battle totals include the one Fountain guardian. Investigation totals exclude the initial Fountain activation. Boss withdrawal investigations remain in the investigation count. Exploration XP event counts exclude the separately counted one-time discoveries. This is an automated gameplay simulation, not measured human play time or a GUI test.

## Current verification

Open `dev/COS-PP1.slnx` and build Debug x64. Both standalone test targets and the game passed. See [dev/README.md](../dev/README.md) for commands and [the current numeric report](../dev/validation/progression-validation.txt) for all observed distributions and simulations. The earlier manual Win32 checklist is superseded by the completed [actual console smoke](../dev/validation/console-smoke.txt).
