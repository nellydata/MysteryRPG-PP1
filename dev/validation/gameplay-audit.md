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

- Resolved in Phase 2: equipment core main stats were flat; a seeded Common book supplied 81 flat ATK. Core percentages and authored fixed bonuses were not consistently preserved or aggregated.
- Resolved in Phase 2: inventory quantity removal and name-wide locking were inconsistent. Consumables could apply an effect before a failed removal, and several catalogue effects were unsupported.
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

## Phase 2: equipment, inventory, consumables, and approved enemy loot

Verified root causes and changes:

| Source | Before | Current behavior |
| --- | --- | --- |
| ContentCatalog.cpp / CombatMath.h | Core mains used flat combat-sized ranges; seed arithmetic correlated ordinary substats; rerolling erased fixed stats | Approved HP/ATK/DEF percentage and flat SPD ranges; independent seeded main, stat selection, and substat values; unique legal substats; original fixed bonuses and identity preserved |
| EquipmentStats.h / Item.cpp / Player.cpp | Separate aggregation paths lost or counted the wrong stat units | One equipment aggregation and preview path: add fixed bonuses, sum core percentages, round the core result once, then apply combat modifiers; secondary values use percentage points |
| Inventory.cpp / Player.cpp | Partial quantity removals and first-name locking could affect the wrong copy; external objects could be equipped | Validate full eligible quantity before removal; stable instance IDs, explicit per-instance locks, protected story/equipped items, and exact ownership verification |
| ConsumableEffects.cpp / CombatManager.cpp | Substring parsing and incomplete effects; mutations could precede a failed removal | Exact typed registry for all 12 effects, ownership/context/effect checks before exact-unit removal; legal cooldown reduction, cleansing, buffs, damage, Burn and exploration utilities; actual damage/healing/status events |
| ContentCatalog.cpp / EncounterManager.cpp | Simplified quantities, four guaranteed first-clear boss items, no Basic gear pity, 5% Artifact immediately at 70 | User-supplied tier quantities and independent consumable rolls, Basic gear guarantee after two eligible misses, mandatory first Fountain Common gear, preserved one-time rewards and reported rarity interpolation |
| ConsoleUI.cpp | Raw fixed Stats display omitted rolled values; name-based item actions could select another copy | Actual main/substats and fixed bonuses, true before/after equipment preview, selected-instance lock/use/discard, combat item IDs, and exploration context passed to shared managers |

Equipment interpretation is explicit in EquipmentStats.h. An ordinary Common book now grants 8-12% ATK, producing 22 ATK for the current 20-ATK starter template instead of a flat +81. Compatibility Stats fields remain fixed authored bonuses. Rerolls retain the authored Artifact flat bonuses; the later set/Artifact overhaul is still pending.

Exploration consumables use the existing areas: capsules illuminate Archive Room/Chapel for 30 seconds and show their authored object names without consuming discovery rewards; oil opens the Archive's authored rusted cabinet lock. Opening it does not silently reveal a required discovery or lose its XP. Stable clue identity, inspection semantics, and the broader discovery workflow remain in Phase 6.

The Basic pity counter is scoped to the EncounterManager game session and changes only after successful eligible Basic loot delivery. Duplicate claims, other enemy tiers, investigations, level-ups, dead-player victories, and fleeing cannot change it. Boss baseline gear remains three plus a separate 50% fourth roll; first-clear XP, unlocks, and trigger skills keep the Phase 1 ownership guards. Boss spawn selection and fixed profiles were not changed.

Project integration: added ConsumableEffects.cpp to all three projects, plus CombatManager.cpp to EquipmentV2Tests to satisfy the shared consumable dependency. The game/filter includes the new registry and stat headers; standalone projects list the new regression headers. All 18 production cpp files are compiled by the game. The solution, project references, output folders, startup target, and archived GUI branch retain their course-conversion layout. No files were relocated or removed in this phase.

### Phase 2 results

- [Clean Debug x64 build](phase-2-build.txt): game and both standalone projects passed from dev/COS-PP1.slnx.
- [EquipmentV2Tests](phase-2-equipment.txt): exit 0; 9,664 rolls spanning all 151 equipment templates, all rarities/slots, approved ranges, stable identity metadata, fixed bonuses, substat legality/uniqueness, seeded replay, preview immutability, actual runtime/display agreement, and sorting with flat versus percentage bonuses.
- [InvestigationHarness](phase-2-tests.txt): exit 0; existing regressions plus inventory ownership/removal tests, all 12 consumable handlers and invalid-use atomicity, deterministic Basic pity and Fountain guarantees, 900,000 seeded loot selections (100,000 per configuration), and rarity gate checks.
- Observed gear/extra percentages: Basic L1 85.128%, L15 92.218%, L30 100%; Rare 20.003%, Epic 50.042%, Elite 34.913%; Boss fourth-item rolls 49.562-50.186%. Independent gear/consumable joint-frequency checks passed.
- Observed boss Artifact share: L70 1.004%, L80 1.984%, L100 4.984%. L100 Rare/Epic/Legendary shares were 5.027%/65.112%/24.877%, within the statistical tolerances around the supplied anchors.
- The fresh progression simulator now chooses earned gear using Player's actual equipment preview rather than a raw fixed-Stats score. All 32 runs reached level 10 without first-clear XP, averaging 321.594 investigations, 50.844 ordinary battles and 0.562 defeats. All 32 then won their level-10 first-clear boss attempt. These are phase-specific measurements; later ability/status repairs require final balance recomputation.
- [Content validation](phase-2-content.txt), isolated [header checks](phase-2-headers.txt), and the [real console smoke](phase-2-console-check.txt) passed. The [keyboard transcript](phase-2-console.txt) covers fresh initialization, selected item lock/use/unlock, gear previews/equip, combat consumable consumption, ordinary attacks, single Fountain reward and unlock, protected discard, recovery, loadout changes, next-room entry and exit.
- The tested executable has console PE subsystem 3 and zero GUI window handle. [Direct executable imports](phase-2-dependencies.txt) contain the C++ runtime and KERNEL32; no GUI or Python dependency is linked.

### Remaining work at this checkpoint

Phase 3 is next: shared damage/heal/barrier edge cases, source-aware statuses and stack policies, a real combat RNG service, action ownership and cooldown timing. Full skill and enemy effect coverage (Phase 4), approved 2/3 and 3/5 sets and all 15 Artifact effects (Phase 5), discovery/NPC/Storage consistency (Phase 6), remaining console workflows (Phase 7), final large-sample investigations, level-2 boss regression and later-level balance, and full console progression (Phase 8) remain. This checkpoint does not certify those systems or the old five-piece set behavior. Presentation planning remains deferred until functional work passes.
