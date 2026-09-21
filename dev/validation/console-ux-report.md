# Console UX / presentation pass

Date: 2026-09-20. Branch: `main`. Starting HEAD: `ddd703d` (`dev mode fixes and roll stat checks`). No commit was made. The pre-existing untracked `images/devmodeworking.png` was not modified.

## Presentation changes

- Added shared pure formatting helpers for bordered titles, section dividers, numbered/locked options, informational/success/warning/error notices, progress bars, cooldown labels, and rarity labels.
- Replaced the one-line player summary with a reusable compact panel containing name, level, optional `[DEV]`, HP bar, optional barrier bar, core stats, and XP. Level 100 now says `MAX LEVEL` rather than displaying a misleading next-level threshold.
- Redesigned the destination dashboard with grouped destinations/panels and explicit lock reasons. Selecting a locked room repeats the reason.
- Redesigned room views around a stable room marker, short authored atmosphere line, discovery count, visible discoveries, guardian readiness/clear state, Fountain state, and grouped actions.
- Redesigned combat around stable PLAYER, ENEMY, SHIMMERS, and ACTIONS sections. Both combatants show HP, optional barrier, compact statuses, and action gauge. Bosses also show phase.
- Categorized presented combat events with compact prefixes including `CRIT`, `HEAL`, `WARD`, `BREAK`, `DOT`, `BUFF`, `DEBUFF`, `RESIST`, `CLEANSE`, `PHASE`, `DEATH`, `SHIMMER`, and `REWARD`. `CombatEvent` data and messages remain unchanged and plain.
- Improved the combat Shimmer picker to show equip cost, `READY`/cooldown state, and the authored short description. A cooling-down selection stays visible but is rejected without consuming the turn.
- Standardized bracketed numeric choices in dashboard, room, combat, map, inventory, and Shimmer views. Existing retry-on-invalid-input, zero back/cancel, and EOF behavior remains intact; invalid input now uses the shared warning style.
- Kept Adelaide ANSI coloring solely in the renderer. Player names, item names, statuses, and event data remain plain.

## Before / after examples

Before: `Clara | Level 1 | HP 108/108 | ATK 20 DEF 10 SPD 100 | XP 0/100`.

After:

```text
Clara  Lv. 1
HP       [############] 108 / 108
ATK 20   DEF 10   SPD 100
XP       0 / 100
```

Before: `2. Entrance Hall [LOCKED]`.

After: `[2] Entrance Hall  [LOCKED - defeat the previous guardian]`.

Before: `1. Pulsus Luminis cooldown 0`.

After:

```text
[1] Pulsus Luminis  Cost 2  [READY]
    Deal 900% ATK as a focused Shimmer strike.
```

## Automated validation

- Clean Debug x64 rebuild of all three current solution projects to `dev/bin/x64/UXValidation`: passed.
- `ArtifactRegression.exe`: passed, including new semantic presentation assertions.
- `ShimmerRegression.exe`: passed, including current compatibility and Artifact regressions.
- `COS-PP1.exe --validate`: passed.
- `ConsoleValidation --headers`: 49/49 passed.
- `ConsoleValidation --smoke`: passed after preserving its stable `=== ROOM: ... ===` marker.
- Standard `dev/bin/x64/Debug/COS-PP1.exe` remained held by PID 23196. The user-owned process was not terminated; MSBuild reported the denied clean operation while successfully compiling/linking the requested alternate Debug x64 output.

## Live smoke observations

- Normal `Clara`: level 1, no DEV marker, Fountain available, later rooms show actionable lock reasons, alphabetic input retries cleanly, and selecting a locked room does not exit or trap the player.
- Adelaide: level 100, `[DEV]`, `MAX LEVEL`, all destinations and developer simulator visible.
- Adelaide entered Entrance Hall, spawned the still-uncleared Matron, opened the redesigned combat HUD, selected Pulsus Luminis from the detailed picker, defeated the boss, and received the ordinary first-clear reward path.
- Dashboard height is roughly 25 lines. The tested one-Shimmer combat view remained comfortably within a conventional console; five equipped skills add only four lines. No clipped lines, unreadable color combinations, or status overload were observed. Incremental event rendering keeps older events from being reprinted on every action.

## Gameplay findings

No new gameplay defect was found. No combat, item, loot, progression, Artifact, Shimmer, boss, or room-rule balance was changed.

## Files changed

- `dev/include/ConsoleFormat.h`
- `dev/include/ConsoleUI.h`
- `dev/src/ConsoleFormat.cpp`
- `dev/src/ConsoleUI.cpp`
- `dev/src/EncounterManager.cpp`
- `dev/src/GameManager.cpp`
- `dev/tests/ArtifactRegression.cpp`
- `dev/validation/console-ux-report.md`

