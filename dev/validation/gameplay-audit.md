# Gameplay correctness audit

This is an incremental functional audit, not a declaration that every system is complete. Visual changes are deferred until the functional checkpoints pass.

## Phase 0: repository and baseline

- Starting branch: `main`, clean working tree, commit `31debb0`. Safe checkpoint: `checkpoint/console-before-gameplay-audit-20260908` at that commit. GUI archive remains `archive/gui-before-course-conversion` at `fa9bc0b`.
- Commit `e96df04` contains the completed console conversion. Subsequent commits deleted the test sources and validation captures while retaining both standalone test projects.
- The requested clean Debug x64 baseline failed with C1083 for `tests/EquipmentV2Tests.cpp` and `tests/CombatCorrectnessTests.cpp`. Linking the game also failed with LNK1104 while the user's running game held the executable open. The user closed that process before the retry.
- Restored the four missing test sources/headers from `e96df04`, without resetting Git or overwriting existing files.
- All 17 production source files are listed in the game project. No graphical startup or UI source is part of the assessed target.
- `ItemEntry::operator->` is defined in `Inventory.h`; the reported arrow-member accesses are valid and are not a compiler defect.
- Compiling each production header independently exposed a missing `GameplayTypes.h` include in `CombatMath.h`; added the direct dependency. Construction/destruction checks then exposed incomplete owned `Item`/`Enemy` types in `Inventory.h`, `EncounterManager.h`, and `EnemyCatalog.h`; these now include the types they own or return.

## Confirmed findings awaiting their functional phase

- Equipment core main stats are flat; a seeded Common book supplies 81 flat ATK. Core percentages and authored fixed bonuses are not consistently preserved or aggregated.
- Inventory quantity removal and name-wide locking are inconsistent. Consumables can apply an effect before a failed removal, and several catalogue effects are unsupported.
- Combat rolls depend on mutable HP/turn hashes; failed Shimmer choices can alter later outcomes. Status resistance, healing of dead entities, and several ability effects need correction.
- Existing five-piece set/Artifact effects do not implement the approved overhaul. These must not be called complete based on the older test suite.
- Investigation discovery identity, NPC state, and Storage Room spawn configuration need reconciliation.

Results for each completed phase will be added below with exact validation and remaining work.

### Phase 0 results

- Header parsing and public ownership construction: 38/38 passed with `tests/CheckHeaders.ps1` (19 headers, two isolated translation units each).
- Restored the declared but undefined five-argument ShimmerSkill constructor as a delegating constructor consistent with the existing compatibility constructor. A derived fixture in the actual harness checks construction and links this overload.
- Debug x64 game and both standalone projects built. EquipmentV2Tests, InvestigationHarness, and `COS-PP1.exe --validate` exited 0 after restoration. The actual console accepted a fresh character name, displayed the dashboard, and quit normally using keyboard input.
- Earlier baseline statistics are recorded in `phase-0-tests.txt`; they describe the pre-equipment-correction economy and must not be used as final balance evidence.
- Remaining functional findings above proceed to later phases; the old suite passing does not prove those mechanics.

## Phase 1: authoritative rewards and progression ownership

Verified defects: XP was delivered through two implementations, loot was marked claimed before generation/insertion, and reward state did not remember a fled/failed encounter. Loadout methods accepted caller-supplied costs, including an incorrect refund, and unknown skill IDs could be unlocked. Level-up processing could restore a dead player. Extreme XP input could overflow its 64-bit accumulator.

Changes: `CombatManager::DeliverDefeatXP` is shared by combat and `EncounterManager::ResolveVictory`. XP claims follow XP delivery; loot claims and room transitions follow successful item insertion. Enemy retains reward forfeiture after defeat/flee across resets, while a newly created retry remains eligible. Removed the unused private interrogation reward path after checking its callers. Progression resolves costs from skill definitions, rejects unknown IDs and forged costs, preserves the 12-point budget, prevents level-up revival, and saturates excessive XP safely. The normal 1-100 XP curve is unchanged.

The existing farm regression is updated to expect the newly visible, single XP delivery message and still reject first-clear/unlock messages. This is a presentation contract correction; reward values and boss profiles are unchanged.

The user supplied the missing loot table and Basic gear pity during this phase. The authoritative quantities, gates, anchors, and proposed intermediate rarity interpolation are recorded in [approved-loot-rules.md](approved-loot-rules.md); implementation follows in the economy phase.

### Phase 1 results

- [Debug x64 build](phase-1-build.txt): game and both standalone projects passed.
- EquipmentV2Tests, content validation, and [InvestigationHarness](phase-1-tests.txt): exit 0. New checks cover eight Attack/Shimmer/DOT/actual-catalogue-consumable kill paths with and without initial combat XP delivery; duplicate XP/loot/log rejection; unresolved/dead/fled enemies; reset/re-entry; fresh fixed-boss retry; and level-100 behavior.
- Progression checks cover unknown IDs, ownership, forged charges/refunds, duplicates, the 12-point budget, one independently owned item per slot per level, queued/deferred/external-unlock/empty-pool rewards, dead-player level-up rejection, and extreme 64-bit XP inputs.
- The [real console executable](phase-1-console.txt) accepted scripted keyboard input for fresh initialization, four ordinary attacks, Fountain purification, a single XP reward and loot delivery, recovery, Shimmer equip/unequip, and normal exit.
- Remaining work: equipment, consumables, status/action/RNG mechanics, complete ability/set/Artifact behavior, discovery/loot reconciliation, later balance sampling, and the full end-to-end progression run. These phases are not certified by the results above.
