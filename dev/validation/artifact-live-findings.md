# Artifact findings from the completed natural console run

These are baseline observations from `QA_Final`, executable built at checkpoint `74540af`, recorded on 2026-09-09. They are not claims that the later Legendary/acceleration repairs completed the separate Artifact system. The character earned **29 Artifact instances across 11 identities**, all at Level 100. Every obtained identity was equipped for at least one real encounter; not every duplicate instance was separately exercised.

The authoritative effects below come from the user's full gameplay audit brief, Phase 5, supplied in attachment `04cdf145-804e-4ac7-92a8-b336c1c67d87/pasted-text.txt`. Several item descriptions still expose older effect strings. Those strings are evidence of a description mismatch, not a replacement specification. In particular, the approved Saint Step threshold is **six normal actions**, and the approved Halo trigger is **DOT detonation/consumption**, not every Shimmer cast.

Evidence is preserved in [endurance-evidence.zip](endurance-evidence.zip): `endurance-final-state.json` and `endurance-final-console-001.txt` through `018.txt`. The derived `endurance-final-events.txt` lists unique event IDs. Ordinary encounters do not retain their full event arrays in the JSON, so this review joins each JSON battle's `FirstEvent`/`LastEvent` to the complete event file. Reopening history does not count as another action.

**LIVE FAIL** means an observed, equipped trigger produced a demonstrably incorrect result. **NOT TRIGGERED** means the relevant condition was not established; it is not a pass. **SOURCE FAIL** means the missing implementation was confirmed by code inspection, independently of live coverage. None of the missing Artifact mechanics is labeled AUTOMATED VERIFIED merely because the executable or unrelated tests passed.

## Obtained identities and trigger coverage

| Artifact | Earned instance IDs | Equipped encounters | Approved effect and baseline result |
| --- | --- | ---: | --- |
| Verdigris Halo of Saint Adelaide | 7348, 9399, 11344 | 86 | DOT detonation/consumption reduces the longest offensive cooldown by one, once per normal turn. **NOT TRIGGERED.** No player DOT detonation/consumption occurred. The natural character lacked the Matron torso needed for five pieces, and the approved detonation hook is absent. Ordinary cooldown decrement after casts does not test this effect. |
| Verdigris Saint Step | 5243, 13193 | 191 | Every six normal actions grants an immediate bonus action. **NOT TRIGGERED within an encounter:** maximum four player actions while equipped in one encounter. There were 194 actions across these encounters; aggregating unrelated battles would not isolate a battle action counter. The displayed old threshold of twelve is also incorrect. |
| Verdigris Saint Relic | 7031 | 819 | Direct attacks apply a three-target-turn Mark; marked targets receive +20% DOT damage from Adelaide. **LIVE FAIL for Mark application:** successful Attack at events 44502–44504 left the target alive, with no Mark before the next action. The missing Mark prevented isolation of its downstream DOT amplification. |
| Verdigris Saint Reliquary | 11378, 13073 | 1 | +10 percentage points DOT healing; DOT overheal becomes barrier, capped at 100% MaxHP. **NOT TRIGGERED:** equipped in events 44476–44512, but the player's final Flos direct hit killed the enemy before any outgoing DOT tick. Incoming Bleed and Ray of Mercy are not this trigger. |
| Choir's Eternal Ritual Gown | 6909, 13157 | 12 | Bleed immunity; cleansing/resisting a negative DOT heals 8% MaxHP and grants 15% barrier once per normal turn. **NOT TRIGGERED:** all twelve equipped encounters ended in one offensive Shimmer, before an enemy hit or negative DOT attempt. |
| Choir's Eternal Tome | 10220, 11047, 11684 | 798 | The first successful direct attack each battle is guaranteed critical with +50 percentage points CritDamage; a miss does not consume it. **Direct-Shimmer discrepancy observed; basic-Attack trigger not isolated.** All 798 encounters ended with one Shimmer and none included the Attack command. Event 34236 shows an ordinary `3262 * 36 = 117432` Refraction Cataclysm request with no critical bonus. A first successful basic Attack is still required to remove any ambiguity about the currently separate Attack/Shimmer critical paths. |
| Locket Crown of the First Light | 11681, 13022 | 427 | The first direct critical per normal turn heals 8% MaxHP and reduces the longest cooldown by one. **NOT TRIGGERED:** the three Attack commands at 44390, 44429, and 44532 did not critically hit. Most other encounters ended with a Shimmer. Being injured while wearing the crown alone is insufficient evidence. |
| First Light Sanctum Attire | 5360, 6560, 6949, 10855, 12481, 13129 | 2020 | Direct enemy damage grants +5% final DEF once per enemy action, stacking to ten/+50%. **LIVE FAIL:** instance 10855 was worn when an ordinary enemy Attack dealt 752 HP damage at event 44337; the following combat panel still showed DEF 1943. |
| First Light Sandals | 5836 | 1869 | Each normal turn start grants +3% final SPD for the battle, stacking to ten/+30%. **LIVE FAIL:** four normal turn starts in events 44515, 44525, 44530, and 44540 leave displayed SPD at 146 throughout. |
| First Light Crucible | 5489, 8431, 13120 | 376 | Each successful original Shimmer grants +5% final ATK, stacking to ten/+50%; echoes do not count. **LIVE FAIL:** with instance 8431 equipped, successful Prism Step and Ray of Mercy casts at 44518/44527 leave ATK 2930. Subsequent Attack requests exactly 2930, and Flos requests exactly `2930 * 15 = 43950`. |
| First Light Reliquary | 2584, 6558, 7121, 7698 | 1 | Every third original offensive Shimmer empowers the next direct offensive Shimmer with a 40% actual-direct-damage echo, without a new action/cooldown/recursive cast. **NOT TRIGGERED:** its sole encounter had two self-targeted Shimmers and only one offensive Shimmer. Three total Shimmers do not satisfy three offensive casts. |

No complete unique effect earns LIVE PASS from this baseline. Four have unambiguous live failures, the Tome has a direct-Shimmer discrepancy requiring a basic-Attack isolation, and the remaining six have unmet triggers. Their ordinary rolled/fixed equipment stats did apply; successful equipment selection does not prove a unique passive.

## Numerical live evidence

**Saint Relic — successful attack without Mark.** JSON battle 3591 identifies Hand instance 7031, Head 9399, Boots 13193, Accessory 11378, and Torso 10855. In transcript segment 016, events 44502–44504 record Attack, a successful hit, and 1762 actual HP damage, leaving Devotee Penitent at 8075 HP. Event 44505 starts the next player action without Mark application. Event 44507 casts Flos, whose direct damage kills the remaining target. The target's death explains why this encounter cannot additionally prove DOT amplification or the Reliquary's DOT recovery.

**Sanctum Attire — damage without DEF gain.** JSON battle 3588 identifies Torso instance 10855. Transcript segment 014, lines 36841–36849, shows DEF 1943 before the enemy's ordinary Attack and DEF 1943 after 752 actual HP damage at event 44337. No Defense Down is active at this point: purification salts removed the earlier debuff at 44325–44326. The approved first +5% final DEF stack would raise 1943 to approximately 2040, so this comparison does not depend on interpreting a resisted hit or barrier-only damage. The later ordinary damage at 44353 also produces no Artifact DEF stack.

**Sandals — normal turns without SPD growth.** JSON battle 3592 identifies Boots instance 5836. Segment 017 shows SPD 146 before and after Prism Step, Ray of Mercy, Attack, and the next normal action. The normal action-start gauges are 1022, 1044, 1066, and 1088. The screen after event 44530 (line 58152) and after 44540 (line 63947) still displays SPD 146. No Slow was applied in this encounter. The separately missing Oracle five-piece speed aura cannot explain absence of the Sandals' own incremental growth.

**Crucible — successful casts without ATK growth.** The same battle equips Hand instance 8431. Prism Step at 44518 and Ray of Mercy at 44527 successfully apply their effects. The combat panel after the second cast still shows ATK 2930. At 44534 the basic Attack requests 2930 damage; at 44545 Flos requests 43950. Neither includes the approved ATK stacks from the two earlier original casts. Enemy mitigation and barrier absorption occur afterward and cannot hide a missing increase in requested damage.

## Artifact set trials

These snapshot labels are exact JSON keys; input counts and transcript segments identify the corresponding console decisions. Equipment was earned and selected through the inventory UI.

| Trial | Input / segment | Equipped Artifact instances | Observed stats and coverage |
| --- | --- | --- | --- |
| `Verdigris Matron-bare-baseline` | 78008 / 016 | None | MaxHP 2575, ATK 1505, DEF 1000, SPD 100. |
| `Verdigris Matron-3-pieces` | 78896 / 016 | Head 9399, Hand 7031, Boots 13193 | MaxHP 3932, ATK 2518, DEF 1220, SPD 142. The live battle additionally equips Matron Accessory 11378, for four pieces, with an unrelated torso. It produces no outgoing DOT tick, so the approved three-piece +35 DOT Potency/10% actual-DOT healing is not isolated live. SOURCE FAIL: the runtime retains the old five-piece recovery model. |
| `Oracle Automaton-bare-baseline` | 80589 / 017 | None | MaxHP 2575, ATK 1505, DEF 1000, SPD 100. |
| `Oracle Automaton-3-pieces` | 82542 / 017 | Boots 5836, Hand 8431, Accessory 2584 | MaxHP 3622, ATK 2488, DEF 1280, SPD 146. The third completed normal action in the later five-piece battle does not award the approved +20% gauge progress; see the trace below. |
| `Oracle Automaton-5-pieces` | 83360 / 017 | Above plus Head 11681, Torso 5360 | MaxHP 5302, ATK 2930, DEF 1933, SPD 146. **LIVE FAIL:** approved persistent +25% final player SPD is absent. Matron of the Catacombs remains SPD 94 in combat, so the approved -25% final enemy SPD aura is absent too. |
| Shimmer Reliquary | No three/five-piece snapshot | Only Gown and Tome identities acquired | At most two distinct slots were available. Neither set threshold was reached naturally. SOURCE FAIL for the approved overhaul; NOT LIVE VERIFIED. |

The Oracle third-action trace is independently consistent with no gauge reward. At the start of the third normal action, event 44530 gives player gauge 1066. Attack completes, then the enemy acts at gauge 1068. The next player start is 1088 at event 44540. With observed SPD 146/94, ordinary threshold subtraction and accumulation produce that exact sequence. Adding the approved 200 gauge progress after the third completed normal action would change the next player start to 1142 under the same observed speeds. There is no bonus-action or echo event that could legitimately consume this progress. This comparison isolates the missing three-piece hook; the five-piece speed aura is a separate failure.

The Matron five-piece threshold could not be reached without the missing Vestment. No five-piece Matron detonation or Halo cooldown reduction is claimed as live tested.

## Four identities not acquired

| Artifact | Approved trigger still requiring separate coverage | Classification |
| --- | --- | --- |
| Verdigris Saint Vestment | 10% final damage reduction; direct enemy hits apply two-turn Bleed once per attacking enemy per normal turn | NOT LIVE ACQUIRED / NOT LIVE VERIFIED |
| Choir's Eternal Mourning Veil | Prevent the first would-land Stun each battle; grant 40% MaxHP barrier for two turns | NOT LIVE ACQUIRED / NOT LIVE VERIFIED |
| Choir's Eternal Boots | Slow immunity; cleansing/resisting a negative effect grants +25% final SPD for two normal turns, refreshing | NOT LIVE ACQUIRED / NOT LIVE VERIFIED |
| Choir's Eternal Reliquary | Convert 40% effective overheal into barrier up to 100% MaxHP | NOT LIVE ACQUIRED / NOT LIVE VERIFIED |

The baseline source has no consumer of `Item::getUniqueEffect()` in Player, CombatManager, ShimmerSkill, or Enemy. Its hardcoded Artifact set branches implement the earlier five-piece rules, not the approved Phase 5 behavior. This is a SOURCE FAIL for the approved Artifact implementation and does not turn an unmet natural trigger into a live failure. No separate deterministic Artifact verification is asserted in this document.

## Remaining deliberate trigger work

These are bounded follow-up scenarios for the C++ console driver using legitimately earned gear. They identify missing evidence; they are not claims that these scenarios have already run.

1. **Step:** keep one encounter alive for at least six completed normal actions, using recovery or low-damage actions, and inspect the immediate bonus action and subsequent counter. Do not sum one-action victories across battles as the sole proof.
2. **Gown:** equip an earned gown against a Bleed-capable enemy such as Devotee Penitent. Allow Blood Offering to attempt Bleed. Also cleanse or resist a negative DOT while injured, then inspect the 8% recovery, 15% barrier, and once-per-normal-turn limit.
3. **Tome:** enter a new encounter with an earned tome and choose basic Attack as the first offensive action. Capture a successful hit, its critical classification, and requested damage including the extra 50 percentage points CritDamage. A miss must leave the first-hit benefit pending.
4. **Crown:** while injured and with an offensive cooldown active, obtain a direct critical using legitimately equipped CritRate gear. Capture HP and cooldown immediately before/after. Ordinary noncritical hits do not test the crown.
5. **Verdigris Reliquary:** lower offensive equipment enough that a DOT-applying Shimmer leaves its target alive. Observe actual outgoing DOT HP damage while injured, then repeat at full HP to test overheal-to-barrier and the MaxHP cap. Record incoming DOT separately.
6. **First Light Reliquary:** keep a target alive through three original offensive Shimmers and a fourth direct offensive Shimmer, with normal cooldown recovery between casts. Self-targeted Shimmers cannot advance this trigger. Verify the echo uses actual HP damage and creates no extra cast/action/cooldown.
7. **Halo:** the required DOT-consumption trigger cannot be exercised naturally in this baseline: the approved detonation implementation is missing and the character lacks the fifth Matron slot. Mark this dependency explicitly. Do not inject the missing Vestment or mislabel a generic cast/cooldown tick as a Halo test.

The four unambiguous live failures already establish that those unique effects need repair; more RNG farming is not needed to rediscover them. Complete new live scenarios only where they add trigger coverage, and record their results separately from this preserved baseline.
