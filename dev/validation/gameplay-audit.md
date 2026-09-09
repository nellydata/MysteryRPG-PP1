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
- Combat HP/turn hash rolls, dead-entity healing, and baseline status/action timing are repaired in the Phase 3 core checkpoint below. Full ability metadata and approved Artifact action exceptions remain pending.
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

At the Phase 2 checkpoint, Phase 3 was next: shared damage/heal/barrier edge cases, source-aware statuses and stack policies, a real combat RNG service, action ownership and cooldown timing. Subsequent results follow below. Full skill/enemy coverage, approved sets/Artifacts, discovery/NPC/Storage consistency, remaining console workflows, final balance sampling and full console progression were still pending.

## Phase 3 core checkpoint: damage, statuses, RNG, and actions

This is a coherent combat-core checkpoint. Catalogue-specific action effects and the approved set/Artifact overhaul remain later work; the existing five-piece compatibility behavior is not certified as the approved design.

### Verified causes and changes

| Source | Before | Current behavior |
| --- | --- | --- |
| CombatResolution.h / CombatMath.h / StatusRules.h | Direct callers pre-mitigated damage, losing raw/mitigated information; large products could overflow; dead targets could be healed | Shared damage/heal records retain requested amounts, mitigation, absorption, actual HP change, overkill/overheal and remaining HP; safe intermediate arithmetic; ordinary healing and barrier application reject dead actors |
| Player.cpp / Enemy.cpp | Separate status implementations; removing absorbed barriers during iteration could invalidate status references; duration expiry could precede Stun's skipped action | Both models use the same application, damage, cleanse and expiration rules; processing uses snapshots and explicit BeginStatusTurn/EndStatusTurn boundaries |
| GameplayTypes.h / StatusRules.h / ShimmerSkill.cpp / ConsumableEffects.cpp | DOTs lacked stable source identity; Mark damaged/debuffed independently of its source; beneficial DOT reduction was treated as negative | Actor/application IDs, source ATK and potency snapshots; independent DOT layers; three-target-turn Mark amplifies only its source's DOT by 20%; no intrinsic Mark damage/debuff; cleanse preserves beneficial effects |
| CombatManager.cpp / RandomService.h | HP/turn arithmetic determined hit/crit outcomes; failed selections changed later rolls; resistance was inconsistent | Seedable mt19937 streams with explicit percent draws; independent per-actor resistance streams; invalid prepared actions and presentation/history access preserve RNG; beneficial statuses never roll resistance |
| CombatManager.cpp | Repeated preparation could repeat normal-turn triggers; one-turn Stun could expire before its skip; bonus actions could tick/recast their own extra-turn skill | One player action lifecycle, cooldown ticks at normal-turn start, end-of-action status expiry, one intended Stun skip, no DOT-dead action, and enemy bonus actions without normal-turn ticks or recursive bonus grants |
| CombatManager.cpp group API | Repeated preparation restarted the party round; another party member or foreign player could take the pending action; duplicate pointers could add turns | Idempotent preparation, exact actor ownership, unique living party members, one speed-ordered enemy round per player action, and correct cooldown/status boundaries across rounds |
| Player.cpp / Enemy.cpp stat helpers | Large growth/SP inputs could overflow; direct level/stat health increases could revive a dead player; lowering level could subtract HP twice | Saturating stat/SP operations, existing level 1-100 bounds, preserved living-player HP deficit across level changes, and no implicit revival; extreme enemy speed normalization stays within existing bounds |

`ApplyDirectDamage` captures raw damage before DEF and damage reduction. Applied damage obeys requested = mitigated + absorbed + actual HP damage + overkill. Lifesteal and Siphon Lux use actual HP damage. Barriers are independent layers consumed by earliest expiry, with persistent layers last; an expiring barrier remains available through its final owner's action. Enemy damage reduction now affects direct damage, and DOT reduction affects DOT damage. Defensive recovery is retained in the structured damage record and event sequence.

DOT baselines remain Bleed 30% source ATK, Burn 60% source ATK, Poison 5% target MaxHP. Snapshot potency applies before the source's Mark multiplier. DOTs bypass DEF and use DOT-specific reduction. Percentage damage/healing scales truncate at their documented step; DEF mitigation rounds once. Equipment aggregation retains Phase 2 rounding. Saturation at INT_MAX prevents unrepresentable integer state; no new universal boss damage cap or one-shot prevention was added.

Status application is resistible only for negative effects. Duration reduction applies after a successful negative application. A failed resistance roll is distinct from rejection of an invalid or weaker application in the event log. DOT/HoT/barrier layers are independent, Mark refreshes per source, Stun refreshes duration, and ordinary numerical buffs/debuffs retain the stronger potency. Statuses present at normal-turn start lose one duration at action completion; a status newly applied/refreshed during that action keeps its new duration for future owner turns. `ProcessStatuses` remains a complete standalone-cycle compatibility wrapper; CombatManager explicitly begins and ends cycles.

The combat RNG defaults to seed 0 for reproducible sessions and accepts an explicit constructor seed or SetSeed before an encounter. Encounter setup draws separate resistance seeds once per actor; uncertain hit, crit and resistance checks consume separate draws. Guaranteed/impossible percent outcomes consume none. Percent rejection sampling is explicitly defined rather than depending on a standard library's distribution implementation. Existing independently seeded loot and investigation generators remain unchanged. Replay tests interleave invalid choices, history messages, and repeated preparation with real actions and compare HP and draw counts.

Single-enemy gauge timing retains the 1000 threshold, positive effective-speed accumulation, retained overflow, and player priority at simultaneous readiness. A valid normal action or Stun skip spends one threshold; invalid choices spend none. Enemy bonus actions do not advance status/cooldown normal-turn clocks and cannot grant another bonus from the same extra-turn handler. The retained group compatibility API explicitly uses one speed-ordered enemy round followed by one player action; it is tested separately. The current console encounter path resolves one enemy at a time, so this checkpoint does not claim that a console group-target menu exists.

Project integration: the game project/filter lists RandomService.h and StatusRules.h; InvestigationHarness lists StatusResolutionTests.h, ShimmerDamageTests.h and CombatActionTests.h and runs them before its older regressions. CheckHeaders exercises both new headers in isolation and through construction. ConsoleEconomySmoke accepts a capture name so earlier transcripts remain preserved. All 18 production cpp files are compiled by the game. No source/solution relocation or GUI dependency was introduced; the solution and all three project files remain in dev, with the GUI archive at fa9bc0b.

### Phase 3 results

- [Clean Debug x64 rebuild](phase-3-build.txt): game, EquipmentV2Tests and InvestigationHarness passed from dev/COS-PP1.slnx using MSBuild /t:Rebuild.
- [InvestigationHarness](phase-3-tests.txt): exit 0. New checks cover damage conservation, overkill, overheal, dead-entity healing/barrier rejection, integer extremes, final-turn barriers, safe DOT iteration, Mark source attribution, stacking, cleanse, 1,000 seeded resistance trials, seeded attack replay and 1,000-seed crit variation, invalid actor/action immutability, both actors' Stun skips, cooldown rejection, DOT death, bounded bonus actions, and group-round ownership/repeated preparation.
- The same harness exercises all 31 defined Shimmers for dead-caster safety and all offensive catalogue paths at extreme ATK, plus actual-damage Siphon and DOT source snapshots. This establishes damage-path safety; it does not validate every catalogue description, multiplier, target, or promised secondary effect.
- Existing reward, progression, inventory, consumable, first-clear/farm and statistical regressions pass, including 700,000 ordinary enemy selections and 900,000 loot selections. Approved loot rates and first-clear protections remain intact; 50% eligible first-clear and 8% farming boss selection were not changed.
- The 32 fresh simulations all reached level 10 without first-clear XP, averaging 322.312 investigations, 51.062 ordinary battles and 0.688 defeats. All 32 won the subsequent level-10 readiness boss attempt. These phase measurements are provisional until the catalogue/set repairs and final balance scenarios pass.
- [EquipmentV2Tests](phase-3-equipment.txt): exit 0, including 9,664 equipment rolls. [Content validation](phase-3-content.txt): exit 0. [Header self-containment](phase-3-headers.txt): 46/46 passed. [Project source checks](phase-3-projects.txt): 18 production sources, 18 compile entries, no missing/stale entry.
- The [actual console test](phase-3-console-check.txt) passed against the freshly rebuilt executable. Its [keyboard transcript](phase-3-console.txt) covers a fresh character, item lock/use/unlock, equipment comparisons/equip, combat consumable, ordinary attacks, exactly one Fountain loot/XP/unlock, recovery, protected discard, Shimmer loadout changes, Entrance Hall entry and normal exit. PE subsystem is 3; GUI window handle is 0. [Direct imports](phase-3-dependencies.txt) contain only the C++ runtime and KERNEL32, with no GUI or Python library.

### Remaining work

Next is the full Phase 4 catalogue reconciliation: explicit effect metadata, acceleration's promised 40% stats/next action, DefenseDown potency independent of damage power, every enemy effect and legal target, unknown effects, and runtime loadout validation. Phase 5 must replace the old five-piece behavior with all approved Legendary/Artifact thresholds, 15 unique effects, additive capped stacks, charges, auras, detonation, echoes and their exact normal/bonus-turn rules. In particular, the existing Shimmer Reliquary cooldown bypass still needs the approved first-ready-cast-per-normal-turn rule; this checkpoint does not certify that exception.

Discovery/NPC/Storage consistency, remaining console/input workflows, complete fresh-console progression through level-up/Shimmer use, failed boss/defeat review/recovery and successful boss clear, the level-2 Matron regression, later-level balance scenarios, and final investigation samples remain. The actual console was exercised here, but the short smoke is not the full end-to-end progression run. Presentation design remains deferred until those functional checkpoints pass. There are no known course-compliance blockers in the tested Debug x64 build.
