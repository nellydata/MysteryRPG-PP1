# Approved loot rules

User-supplied during the gameplay audit. These replace the simplified quantities in the baseline implementation.

| Tier | Guaranteed gear | Independent extra gear roll | Consumable |
| --- | ---: | --- | --- |
| Basic | 0 | 85% at player level 1, linearly increasing to 100% at 30; 100% thereafter | Independent 30% |
| Rare | 1 | 20% for one additional item | Independent 40% |
| Epic | 1 | 50% for one additional item | Independent 50% |
| Elite | 2 | 35% for one additional item | Independent 65% |
| Boss | 3 | 50% for one additional item | One guaranteed |

Each awarded gear item receives its own seeded instance roll. Basic pity guarantees gear after the previous two eligible Basic victories yielded none. A Basic gear award resets that counter. Other enemy tiers, investigations, level-ups, death, and fleeing neither advance nor reset it.

The mandatory first Fountain guardian guarantees one Common equipment item and retains its purification/unlock and XP behavior. Fixed room bosses use the normal Boss quantity table; first-clear XP, room unlocks, and authored special rewards are separate and occur once. A first clear does not guarantee four ordinary gear items. Farming never repeats first-clear rewards.

Rarity gates: Common 1, Uncommon 5, Rare 10, Epic 20, Legendary 40 (bosses only), Artifact 70 (bosses only). Non-boss loot and ordinary investigations are capped at Epic.

Approved level-100 Boss distribution: Rare 5%, Epic 65%, Legendary 25%, Artifact 5%. Artifact anchors: level 70 = 1%, level 80 = 2%, level 100 = 5%.

Reported interpolation, applied in Phase 2: interpolate Artifact chance linearly within 70-80 and 80-100. Preserve the existing cumulative 30% Legendary-or-Artifact bucket, so increasing Artifact share replaces Legendary share; retain the existing 65% Epic / 5% Rare shares once all four tiers are eligible. Thus level 70 is 1% Artifact / 29% Legendary / 65% Epic / 5% Rare, and level 80 is 2% / 28% / 65% / 5%. This is the explicitly reported proposal for intermediate weights, not an assertion that the user supplied every intermediate weight.

Encounter selection is independent of loot: preserve 50% eligible first-clear boss spawning and the separate 8% farming-boss draw.
