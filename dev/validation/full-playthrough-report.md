# Micare real-console endurance QA

The completed baseline proves that a fresh console character can clear Floor I and reach Level 100 without injected state. Targeted repairs address Storage farming, acceleration, Legendary sets, and combat inventory pagination. Artifact mechanics remain incomplete; this report does not certify the entire earlier system-overhaul brief. Final repaired console validation is in progress.

## Observed baseline

Source checkpoint: `74540af`. Executable: `dev/bin/x64/Debug/COS-PP1.exe --fast`, SHA-256 `F92599DDAD5C8200EA9A12F08C5776A7ACB1795495006603030BD95BEA6B40F8`.

All progression used the real executable's stdin/stdout. The driver submits numbered menu choices; it never edits XP, level, items, room flags, skills or player memory. There is no save/profile system. Each named run starts in a new process. Full console output and submitted input are preserved in segmented transcripts.

The first complete `QA_Adelaide` run cleared all four required guardians at player levels 8/15/22/30, reached level 100, continued for 500 investigations, and exited normally. It recorded 2,117 investigations, 791 encounters, 489 victories, 301 flees and one deliberate defeat. All 31 owned Shimmers were cast. This is live-play evidence, distinct from the previous unit/integration tests.

## Failures recorded before engine changes

1. **Storage Room ordinary/farming encounters are disabled.** Its first-clear guardian can appear and was defeated normally, but ordinary encounters and farming bosses cannot spawn there. The first run includes 280 Storage investigations, including a post-clear farming sample. `MapManager::GenerateFloor1` assigns `EnemySpawnCondition::None`; `ResolveInvestigation` consequently assigns zero weight to enemy events despite a populated room enemy/farming catalogue. The proposed minimal correction is to enable that existing pool while retaining the approved weighted table and separate boss rules.
2. **Legendary 2/3-piece bonuses do not activate.** The real inventory was used to equip collected two- and three-piece configurations, with before/after snapshots in `endurance-state.json`. Choir's two pieces produce 4,352 MaxHP instead of applying the approved additional 35% final-HP bonus; Shimmer Ascendant's two-piece ATK is 2,333 without its approved additional 30%; Automaton Overlord's two pieces leave SPD at 100 instead of 120. `Player::CalculateEquipmentStats` and `GetActiveSets` still require five pieces. This is the previously identified, incomplete set overhaul, not a new balance adjustment.
3. **Acceleration descriptions disagree with runtime.** In the level-15 Overlord encounter, Gradus Coruscus raises ATK 361 to 451, DEF 177 to 221, and SPD 111 to 139 (25%, rounded), while its description promises 40%. The same generic handler serves Mutatio Horologii. Source: `DefinedShimmerSkill::apply`, `stat_boost`. A complete metadata/action-rule repair belongs to the pending ability audit.
4. **Endgame consumable selection is impractical.** New consumable instances are deliberately kept separate by Inventory::AddItem, and the combat menu prints all of them without pagination. The longer soak produced a menu with thousands of one-unit entries. This is a presentation/usability problem; repeated entries are independently earned items, not evidence of duplicated victory rewards.
5. **The QA reader timed out on a large menu.** The extended run stopped at 8,050 investigations while reading the consumable menu. Replaying all 44,554 recorded input lines plus four normal cancel/flee/exit choices succeeded with exit 0 and the same level-100 HP, XP and stats. The timeout was in the driver's buffered StreamReader, not a demonstrated game hang. The driver now reads available bytes with a persistent UTF-8 decoder; a new run is testing that correction.

The approved Artifact thresholds and unique effects remain incomplete. No visual redesign or balance change is part of this QA session.

## Evidence and coverage definitions

The 53 baseline captures and observer files are preserved in [endurance-evidence.zip](endurance-evidence.zip). Every archive entry was decompressed and checked against its original SHA-256; [the manifest](endurance-evidence-manifest.md) records lengths and hashes. Plaintext originals remain locally available. The archive contains data, not source scripts.

[The coverage matrix](full-playthrough-coverage.md) lists all 31 Shimmers, 26 registered enemies and their observed skills, all 12 consumables, 24 status types, and 15 Artifacts. [Artifact findings](artifact-live-findings.md) distinguish the approved effects from obsolete UI descriptions. Event IDs refer to the de-duplicated `endurance-final-events.txt` in the archive; opening history does not count as another action.

LIVE PLAY VERIFIED means a transition/effect was checked in the real console. LIVE CAST/USE records an action without certifying every advertised mechanic. AUTOMATED VERIFIED refers to separate deterministic/statistical fixtures. Unreached conditions remain unverified; owning an item is not proof of its passive.

## Fresh player and required guardians

Blank input selected Adelaide. The initial dashboard displayed Level 1, HP 108/108, ATK 20, DEF 10, SPD 100, XP 0/100. Initial inventory was `linen_veil`, equipped `plain_rosary`, one `cellar_rations`, one `purification_salts`, `storage_key`, and `first_nuns_diary`. All six were inspected. Pulsus Luminis was initially owned/equipped; Fountain healing and later rooms were locked.

Live checks exercised malformed/range input, empty history, map/room locks, cancellation, item details, lock/unlock, locked/equipped discard rejection, one expendable discard, equipment replacement/unequip, three inventory sort orders, pagination, duplicate instances, reward deferral, cooldown rejection, and the 12-point Shimmer budget. MaxHP changes stayed valid. Fountain victory unlocked recovery and Entrance Hall and awarded its Common gear once. A deliberate first-boss flee and defeat preserved first-clear eligibility; defeat history was reviewed before acknowledgment and Fountain recovery.

| First clear | Player level | Enemy level / HP / DEF / SPD | Player HP / ATK / DEF / SPD | Actions | Incoming HP damage | Fixed XP |
| --- | ---: | --- | --- | ---: | ---: | ---: |
| Matron of the Catacombs | 8 | 8 / 524 / 37 / 94 | 371 / 158 / 92 / 105 | 1 | 0 | 200 |
| Overlord-Prime | 15 | 15 / 1310 / 110 / 115 | 536 / 361 / 177 / 111 | 2 | 0 | 400 |
| Matron of Rot | 22 | 22 / 2310 / 152 / 106 | 800 / 593 / 277 / 120 | 1 | 0 | 800 |
| The Reliquary Engine | 30 | 30 / 3757 / 273 / 108 | 1056 / 805 / 378 / 120 | 2 | 0 | 1600 |

Storage's post-victory snapshot is Level 31 because its XP leveled the player after the Level-30 fight. Only four first-clear events appear in QA_Final: 1257, 2175, 2765, 3832. Farming bosses never repeat them.

## Longest completed run: QA_Final

The corrected reader completed 87,288 keyboard inputs and exited normally in 563.6 seconds. This is automation runtime, not human playtime. There were 2,930 Level-100 battles and 232 Level-100 farming-boss victories.

| Metric | Result |
| --- | ---: |
| Investigations | 11,635 |
| Encounters / victories | 3,592 / 3,290 |
| Deliberate flees / defeats | 301 / 1 |
| Ordinary / boss encounters, including retries | 3,031 / 561 |
| Farming-boss encounters | 255 |
| Player actions / Fountain recoveries | 3,350 / 19 |
| Gear instances inspected | 7,617 |
| Distinct Shimmers cast / successful consumable definitions | 31 / 11 |
| Final level / XP | 100 / 380,271 of 1,262,643 |
| Final displayed HP / ATK / DEF / SPD | 2,575 of 4,269 / 3,580 / 1,811 / 142 |

Investigations by room: Fountain 1, Entrance Hall 3,881, Archive 3,766, Chapel 3,707, Storage 280. Baseline Storage was avoided after its disabled pool was confirmed. Encounter tiers: Basic 167, Rare 490, Epic 1,199, Elite 1,175, Boss 561.

The observation map contains Common 177, Uncommon 1,124, Rare 1,889, Epic 4,192, Legendary 220, Artifact 29. These are 7,631 observed item records, including 14 consumable/story records and starting items, not 7,631 enemy gear drops. The gear-only distribution is derived separately below. The final soak acquired 29 Artifact instances across 11 definitions, exceeding the earlier eight-definition sample.

| Final slot | Selected item | Instance | Rarity |
| --- | --- | ---: | --- |
| Head | locket_crown_of_the_first_light | 11681 | Artifact |
| Torso | first_light_sanctum_attire | 10855 | Artifact |
| Boots | verdigris_saint_step | 13193 | Artifact |
| Hand | choirs_eternal_tome | 10220 | Artifact |
| Accessory | automaton_strike_charm | 7967 | Epic |

This was the strongest build chosen by the driver's visible-stat scoring, not a proof of globally optimal gear. The lower final current HP followed intentional equipment tests: changing MaxHP does not freely refill health. Acquired item details reproduce all four displayed maximum/core stats. Derived secondary totals are ACC 45, RES 17, Crit Rate 20%, Crit Damage 210%, Barrier Potency 11, and zero EVA/healing/DOT/lifesteal bonuses. These are derived from item details and the base template; the dashboard does not display them all.

## Completed targeted repairs

**Storage:** `None` became `Low`, enabling the existing enemy pool. The new regression first failed with exit 1 against the observed configuration. After repair, 10,000 seeded investigations produced 2,968 enemy events (231 farming bosses), 4,199 item events, 2,017 XP events, 394 beneficial events, and 422 hazards. It also asserts unchanged 50% first-clear and conditional 8% farming-boss chances, Level 30, and fixed XP 1,600. No content was added.

**Acceleration:** both skills now use explicit metadata for +40% ATK/DEF/SPD during the next two normal actions and a single immediate 800/1,000 gauge advance. Description, duration, status log, and gauge log agree. The 40% effect is authoritative; **800 gauge is the reported interpretation of the existing unused 800 field**, because no numeric action advance was supplied. Clarification was requested; that number is not described as independently approved. Tests verify both skills, exact stats/lifetime, the 1,080 versus 960 next-turn gauge comparison, identical core buffs without gauge acceleration, cooldowns, invalid/dead casts, and no RNG consumption on rejection. Existing major buffs retain their prior values with precise descriptions. The console uses the shared manager; the older compatibility group-round API is outside this single-opponent gauge repair.

**Legendary sets:** exact identities activate at two and three pieces. Core multipliers apply after ordinary equipment aggregation. Removed obsolete five-piece opening barriers, perfected accuracy, flat ATK/ACC, and Ascendant healing.

| Set | Two pieces | Three pieces, cumulative |
| --- | --- | --- |
| Choir of Verdigris | +35% final MaxHP; +25pp Healing Received | +30% final DEF; +30pp Barrier Potency |
| Shimmer Ascendant | +30% final ATK | +35% direct Shimmer damage; +25pp DOT Potency |
| Automaton Overlord | +20% final SPD | +20pp Crit Rate; +50pp Crit Damage |

Five pieces retain these thresholds without reapplying them. Regressions cover 1/2/3/5 pieces, exact identities, duplicate instances, same-slot replacement, mixed sets, equip/unequip, 100 replacements, rounding/order, previews, actual heal/barrier/DEF/direct-Shimmer/DOT/crit effects, caps, and integer extremes.

**Combat consumables:** twelve exact instances per page, next/previous/cancel, no action spent navigating or rejecting an invalid/locked choice. A 25-instance regression traverses three pages and removes only the selected last-page item. Independently earned consumables remain independent instances.

**QA transport:** available-byte pipe reads replace the buffered character read. The extended replay and completed later soak establish that the prior timeout was **NOT a Micare game failure**. All four repository PowerShell scripts are replaced with C++ tooling. Standalone assertion failures now go to stderr instead of opening CRT dialogs.

## Remaining correctness and coverage limits

- Artifact uniques and approved Artifact 3/5-piece rules remain incomplete. Natural tests reached three-piece Verdigris Matron and three/five-piece Oracle Automaton; only two distinct Shimmer Reliquary slots were acquired. Existing five-piece compatibility behavior is not the approved overhaul. [The Artifact report](artifact-live-findings.md) records four unambiguous live unique failures, unmet triggers, a Tome discrepancy, and set failures. Four definitions were not acquired; none is falsely labeled AUTOMATED VERIFIED.
- All 31 Shimmers were cast live. The earlier full audit still requires remaining catalogue-specific skill/enemy reconciliation, including DefenseDown potency independent of damage power, and complete Artifact charge/echo/turn rules.
- Eleven consumables succeeded live. Sanctum water lacked a matching successful status context; no-effect rejection was exercised. All twelve effects have separate automated tests.
- Several enemy skills and status contexts were not reached naturally, listed explicitly in the coverage matrix. Registration is not proof of runtime behavior.
- De-duplicated events contain 3,372 parsed damage resolutions with no HP-damage + barrier + mitigation sum exceeding requested damage. This arithmetic check alone does not prove every formula. No negative displayed stats, level above 100, duplicate first-clear reward, or dead-actor action was found in the observed run. Controlled tests cover overflow, reward ownership, DOT death, status lifetime and invalid-action atomicity.

**Balance, separate from bugs:** prepared first-clear bosses lasted only 1–2 actions with strong earned Shimmers, faster than the earlier 8–14-action aspiration. No damage cap, XP shortcut, boss-chance adjustment, or nerf was introduced to force that target.

**Presentation, separate from bugs:** the dashboard omits several secondary stats; some DOT messages name the actor without the status; overkill is represented in structured results without a dedicated console label; Artifact descriptions expose legacy effect identifiers. No visual redesign was started.

## Final validation in progress

EquipmentV2Tests passes 9,664 equipment rolls and ownership/preview/display/sorting checks. InvestigationHarness passes all four new regressions, existing combat/status/action/progression/ownership tests, all twelve consumables, all-tier loot statistics (100,000 victories per scenario), Basic pity, rarity anchors, and exploration/balance validation. Repaired `COS-PP1.exe --validate` passes. The C++ header checker passes 46 isolated fixtures, and C++ fresh smoke exits normally.

The canonical executable is held by an existing user game (PID 34264), causing LNK1104. A separate Debug-x64 output at `dev/bin/QA-current` builds successfully and is being used for repaired live Level-100/set validation without terminating that session. This is a file lock, not a compiler error. Final commands/results and commit status will be recorded after validation completes.
