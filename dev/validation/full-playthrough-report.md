# Micare compatibility, Artifact, and Adelaide developer-mode pass

Date: 2026-09-18. Branch: `main`. Current HEAD at the start of this continuation: `053668a` (`more stat calculation fixesw`). The Artifact/Shimmer implementation was already committed externally between turns; this pass did not make a commit. The accidental one-space change at the start of `dev/COS-PP1.slnx` was removed without changing its project entries.

## Live verified

- Earlier legitimate endurance run: level 100, 1,892 investigations, 808 encounters, 323 ordinary post-clear Storage encounters, 28 farm bosses, and three distinct Artifacts earned. Those three Artifacts were **not equipped** in that run, so earning them is not live verification of their unique effects. There is no save system with which to carry that character into this session.
- Earlier real-console smoke and the current `ConsoleValidation --smoke` exercised ordinary fresh-game Fountain combat, reward/unlock, equipment, consumables, Shimmer loadout, healing, navigation, and exit 0. The current short direct `--fast` run defeated the Fountain guardian, unlocked Entrance Hall, investigated, and exited normally.
- A current direct developer-mode `--fast` run with a blank name displayed rainbow-colored Adelaide, level 100, all five rooms open, and a room drop simulator. A fresh 10,000-roll Storage boss sample reported 35,085 gear and 10,000 consumables: Rare 4.99%, Epic 64.88%, Legendary 25.04%, Artifact 5.09% of gear. An earlier 1,000-roll sample reported 4.91% Artifact gear. A separate current direct run entered Entrance Hall, spawned and defeated the uncleared Matron guardian, and received the normal first-clear path. A typed `Adelaide` also activated the mode. A fresh `Clara` run remained level 1 with Entrance Hall locked and no simulator option.

## Automated verified

- Clean Debug x64 **rebuild to alternate output directory** succeeded for `COS-PP1`, `ArtifactRegression`, and `ShimmerRegression`. The normal `dev/bin/x64/Debug/COS-PP1.exe` could not be replaced because VS Code's debugger was running that executable (PID 9596). The running debug session was not interrupted. This means the standard-path rebuild is not yet verified, but the alternate-path executables were freshly compiled and linked from current source.
- Rebuilt `ArtifactRegression.exe`, rebuilt `ShimmerRegression.exe` (which also compiles/runs `CompatibilityChecks.cpp` and the Artifact checks), and rebuilt `COS-PP1.exe --validate` all passed.
- `ConsoleValidation --headers` passed 49/49 isolated header fixtures; `ConsoleValidation --smoke --exe <alternate current executable>` passed. `ConsoleValidation.exe` is an older driver, but its smoke launched the newly built executable. `git diff --check` exited 0; only CRLF conversion notices were printed.
- Compatibility regression used 5,000 seeds per enemy tier and 5,000 level-100 boss encounters. Its level-100 boss sample had 17,465 gear pieces: Rare 4.94131%, Epic 64.8955%, Legendary 25.2677%, Artifact 4.89551%. It also checked Basic gear scaling at levels 1/15/30, non-boss and investigation rarity caps, Basic pity, duplicate reward claims, fixed room/boss profiles, farm-boss and defeat non-clear paths, Storage farming, consumable mapping/use restrictions, DOT/Mark math, dead-player action/healing guards, and Legendary 3+2 equipment recalculation.
- Developer-mode regression checks both name triggers, normal-name non-trigger, per-letter ANSI coloring, level/health and the level-100 XP cap, all rooms and Archive cabinet access without falsely clearing bosses, all equipment/consumables/clues/Crux/Shimmers in owned/unlocked state, all six equipment rarities and exactly 15 Artifact uniques, unique item instance IDs, idempotent setup, equip/unequip operation for every catalogue item, a 5,000-roll pure boss sample, investigation Epic cap, single-roll availability, and Fountain boss-source rejection. Batch sampling does not alter inventory or combat/pity state.

## Source audit and Artifact matrix

Before commit `053668a`, all 15 Artifact unique IDs were authored but had no runtime dispatch. The obsolete five-piece set text and unrelated stat bonuses did not match the approved three/five tiers. The committed implementation routes effects through actual equipped item names plus matching unique-effect IDs, and combat/player/Enemy lifecycle hooks. Every item below is **automated only**, not live-equipped.

| Unique / set tier | Runtime hook and positive check | Negative/limit evidence and remaining gap |
| --- | --- | --- |
| Verdigris Halo | DOT detonation in Shimmer resolution; longest ready **offensive** cooldown reduced. Five-piece detonation regression checks reduction event and distinguishes the one scheduled turn-start tick from the one Halo decrement. | Ordinary DOT tick has no Halo hook; cooldown helper ignores zero. Explicit two-detonations-in-one-turn test remains. |
| Verdigris Saint Vestment | Final incoming damage stage applies 90%; landed enemy direct hit applies 2-turn Bleed. Damage and live-combat reaction regressions pass. | DOT/environmental paths do not call retaliation hook. Per-attacker repeated-hit and dead-attacker tests remain. |
| Verdigris Saint Step | Completed normal Attack counter grants immediate sixth-action bonus; regression checks a seventh bonus Attack does not recurse or count. | Cross-encounter reset is source-audited, not isolated by a dedicated test. |
| Verdigris Saint Relic | Successful direct Attack applies three-turn same-source Mark and +20% DOT damage; regression checks both and Mark expiration. | Miss/secondary non-trigger fixtures remain. |
| Verdigris Saint Reliquary | Actual Adelaide DOT HP damage heals +10pp and overheal becomes capped barrier; regression observes DOT overheal shield event. | No explicit blocked-DOT or max-barrier test for this unique alone yet. |
| Choir's Eternal Mourning Veil | First successfully applied Stun is removed and grants two-turn barrier; second valid Stun lands. | Regression proves a resisted Stun does not consume protection; separate barrier-expiry test remains. |
| Choir's Eternal Ritual Gown | Bleed immunity; negative-DOT resistance/cleanse response heals and shields. Once-per-turn, next-turn refresh, positive DOT cleanse, and non-DOT cleanse non-trigger regressions pass. | Multiple negative DOTs in one cleanse are source-audited but not separately exercised. |
| Choir's Eternal Boots | Slow immunity and refreshed two-normal-action +25% SpeedUp. Regression checks repeated immunity does not duplicate speed and expires after two completed normal actions. | Separate cleanse-origin refresh test remains. |
| Choir's Eternal Tome | First landed direct Attack is guaranteed critical with +50pp Crit Damage; first and later attack regressions pass. A default-zero optional enemy evasion metadata field supplies a high-evasion fixture that proves a miss does not consume the critical. | Secondary/echo non-consumption remains source-audited. |
| Choir's Eternal Reliquary | `Player::heal` converts 40% effective overheal to barrier, capped at MaxHP; cap and ordinary-healing non-overheal regressions pass. | Barrier-Potency combination is source-audited. |
| Locket Crown of First Light | First direct critical heals and reduces longest cooldown; combat event/cooldown regression passes. Crown plus Saint Step bonus checks once-per-turn state. | No DOT/secondary false-trigger fixture. |
| First Light Sanctum Attire | Enemy direct-damage hook adds final DEF stack; combat trigger and +50% cap regressions pass. | Multi-hit single-action fixture not yet built. |
| First Light Sandals | Normal-turn start adds +3% final SPD; +30% cap and ordinary SpeedUp combination regressions pass. | Bonus action is verified to skip normal-turn start in source but lacks a dedicated sandals event test. |
| First Light Crucible | Successful original Shimmer adds +5% final ATK; failed cooldown cast does not. +50% cap and AttackUp combination regressions pass. | Echo non-trigger is source-audited. |
| First Light Reliquary | Third original offensive cast arms next direct cast echo; regression checks no third-cast echo, one next-cast echo, and 40% of Ascendant-amplified actual HP damage. | Heal-only/zero-HP/barrier-only and repeated cycle fixtures remain. |
| Verdigris Matron 3 | Shared equipment stat calculation adds +35pp DOT potency; player-source actual DOT HP damage heals 10%; both regressions pass. | Blocked DOT non-healing not separately tested. |
| Verdigris Matron 5 | Direct Attack applies Mark; third original offensive Shimmer against marked target detonates and removes remaining DOT. Regression confirms DOT removal, healing, and Halo cooldown event. | Unmarked target, exact remaining-tick sum, and recursion-negative fixtures remain. |
| Shimmer Reliquary 3 | +40pp Barrier Potency, +25pp Healing Potency; barrier break heals and cleanses **one** negative effect. Stat and barrier-break regressions pass. | Partial barrier damage and second break in one turn are source-audited only. |
| Shimmer Reliquary 5 | First successful ready Shimmer starts no cooldown; invalid cast does not consume privilege. Barrier gains are amplified 50%; regression passes. | A second Shimmer in the *same* normal turn is not reachable with the current five-slot loadout/action economy and has no dedicated fixture. |
| Oracle Automaton 3 | Every third completed normal Attack grants 200/1000 action gauge; regression checks event. | Bonus action exclusion follows shared normal-action counter; exact gauge boundary fixture remains. |
| Oracle Automaton 5 | +25% final player SPD in shared equipment calculation; separate -25% final enemy SPD in combat; regression checks suppression. | Enemy speed suppression is refreshed on combat preparation/action, so immediate `Enemy::getSpeed()` after an out-of-combat equipment change is not explicitly proven. Choir Boots plus five-piece Oracle is slot-incompatible. |

The normal-action term above follows the existing combat distinction: direct `Attack` advances normal-action counters and two-normal-action buffs; Shimmer/consumable actions do not. The committed implementation preserved that distinction.

## Bugs found and fixes implemented

- Blank name only changed the string to Adelaide; there was no developer setup. Added a dedicated developer initializer for level 100, all room/discovery/healing/cabinet access, all catalogue items, Crux unlocks, and all Shimmer unlocks. Boss-clear flags remain false so fights remain available.
- No no-combat room drop-rate tool existed. Added an opt-in dashboard simulator with room, source (investigation or Basic/Rare/Epic/Elite/Boss), single kept roll, and non-mutating batch statistics. It calls the existing loot generators, not a second loot model; batch seeds come from a seeded PRNG rather than correlated adjacent seeds. It does not grant XP, change pity, claim boss rewards, or clear rooms.
- Adelaide was displayed as plain text. Added per-letter ANSI rainbow rendering in panels, combat, messages, and event presentation while keeping the internal identity plain. Normal names are unchanged.
- The application project listed source files explicitly and initially failed to link the new developer helper. `DeveloperMode.cpp` was added to `COS-PP1.vcxproj`; the alternate clean rebuild then passed.
- Prior Artifact fixes, now already in HEAD, replaced dead unique IDs/passives and set text, added equipment-aware triggers and set thresholds, status/heal/barrier/DOT/direct-hit/cooldown/action hooks, detonation/echo, and equipment-preview/counter reset behavior. No duplicate Artifact system was found.
- Added optional enemy evasion metadata with a default of zero, preserving every authored enemy's balance while enabling a real missed-attack fixture for Tome non-consumption.

## Known limitations / not yet verified

- The requested Artifact edge matrix still has explicitly listed interaction fixtures that are source-audited rather than independently exercised. Artifact behavior is already committed at `053668a`; this pass did not silently claim those gaps as new runtime verification.
- `EquipmentV2Tests.exe` and `ProgressionTests.exe` old binaries passed when run, but their source/project is not in the current solution; they are **not current-source evidence**. `InvestigationHarness.exe` is absent. Compatibility checks now cover key overlapping cases in the rebuilt Shimmer project, but do not substitute for all historical tests.
- The standard output executable remains locked by the user's running VS Code debug adapter, so default-path Debug x64 rebuild and a default-path current smoke are pending. No process was killed. The alternate `DebugValidation` output passed clean rebuild and live smoke.
- No current run naturally acquired and equipped all Artifacts. Artifact behavior is AUTOMATED VERIFIED only where tests exist; the previous endurance acquisition does not change that. No full L1→L100 endurance was repeated after the Artifact changes.
- ANSI rainbow rendering depends on VT-capable terminal presentation; Windows console VT mode is enabled when a console handle is available. Redirected output intentionally contains ANSI sequences.
- Developer batch rolls are raw-generator samples at the current player level; they intentionally exclude combat, Basic pity, and first-clear special rewards. Single rolls are added to inventory; batch rolls are not.

## Commands and results

- `git status --short`, `git diff --stat`, `git diff`, `git log -5 --oneline`: inspected. HEAD had changed to `053668a`; only the leading-space `.slnx` edit was dirty before this continuation.
- `MSBuild dev/COS-PP1.slnx /t:Build /p:Configuration=Debug /p:Platform=x64`: first attempt compiled but link failed `LNK1168` because VS Code had `COS-PP1.exe` open. This is an output-file lock, not a source compile failure.
- `MSBuild dev/COS-PP1.slnx /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:OutDir=<repo>/dev/bin/x64/DebugValidation/ /m /v:minimal`: passed; all three current projects rebuilt.
- `DebugValidation/ArtifactRegression.exe`: passed. `DebugValidation/ShimmerRegression.exe`: passed, including compatibility. `DebugValidation/COS-PP1.exe --validate`: passed.
- `ConsoleValidation.exe --headers --exe DebugValidation/COS-PP1.exe`: 49/49 passed. `ConsoleValidation.exe --smoke --exe DebugValidation/COS-PP1.exe`: passed.
- Direct `DebugValidation/COS-PP1.exe --fast` input scripts: fresh normal-name isolation, blank and typed Adelaide activation, fresh 10,000-roll Storage simulator sample, kept roll, developer Entrance guardian victory/first-clear, and ordinary fresh Fountain smoke passed.
- `git diff --check`: exit 0, no whitespace errors; CRLF notices only.

## Files changed in this continuation

`.gitignore` (report exception); `dev/COS-PP1.vcxproj`; `dev/include/Area.h`, `ConsoleFormat.h`, `ConsoleUI.h`, `GameplayTypes.h`, new `DeveloperMode.h`; `dev/src/Area.cpp`, `ConsoleFormat.cpp`, `ConsoleUI.cpp`, `Enemy.cpp`, `GameManager.cpp`, new `DeveloperMode.cpp`; `dev/tests/ArtifactRegression.cpp`, `CompatibilityChecks.cpp`; this report. `dev/COS-PP1.slnx` is clean after removing its accidental leading space.

No commit was made in this continuation. The developer-mode implementation passes current-source build/regression and live-console validation, but the tree is not yet fully ready for the requested checkpoint because the normal output path remains blocked by the user-owned debugger process. That rebuild must be repeated after the lock is released; the documented Artifact edge fixtures also remain follow-up verification rather than claims made by this pass.
