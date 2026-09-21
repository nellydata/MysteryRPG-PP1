# Shimmer catalogue/runtime audit (Milestone 3)

This table covers all 31 names returned by `BuildLatinShimmerSkillNames()` after the consistency pass. `Self` and `Enemy` are actual target types. Amounts are percentages of max HP or ATK; status percentages are authored status potencies, not values inferred from direct damage. A dash means the effect has no such component. All rows now agree with their player-facing descriptions and produce a runtime effect. Status application to enemies remains subject to accuracy/resistance.

| Skill | Cost/CD | Target | Effect ID | Direct amount | Status / potency | Turns | Gauge | Description = runtime |
|---|---:|---|---|---|---|---:|---:|---|
| Pulsus Luminis | 2/5 | Enemy | damage_ignore | 900% ATK, ignores DEF | - | - | - | Yes |
| Velum Radiāns | 3/6 | Self | barrier | 1500% HP barrier | Barrier | 3 | - | Yes |
| Gradus Coruscus | 2/4 | Self | stat_boost | - | ATK/DEF/SPD +40% | 2 normal actions | +800 once | Yes |
| Echo Lucernae | 3/6 | Enemy | damage_stun | 1300% ATK | Stun attempt | 2 | - | Yes |
| Fulgur Aurorae | 4/8 | Enemy | damage_ignore | 2000% ATK, ignores DEF | - | - | - | Yes |
| Flos Aeruginis | 3/6 | Enemy | damage_burn | 1500% ATK | Burn, DOT potency +50 | 3 | - | Yes |
| Resonantia Chori | 5/10 | Self | stat_boost | - | ATK/DEF/SPD +25%, crit rate +15pp, crit damage +25pp, damage reduction +15pp | 3 | - | Yes |
| Requiem Mechanicum | 3/6 | Self | barrier | 1500% HP barrier | Barrier | 3 | - | Yes |
| Scutum Coruscum | 2/4 | Self | defensive_buff | 1000% HP barrier | Damage reduction 20% | 3 | - | Yes |
| Nota Luminosa | 2/5 | Enemy | damage_defdown | 900% ATK | DEF down 90% | 3 | - | Yes |
| Revelatio Occulta | 2/5 | Enemy | damage_defdown | 900% ATK | DEF down 90% | 3 | - | Yes |
| Catena Aeruginis | 3/6 | Enemy | damage_stun | 1300% ATK | Stun attempt | 2 | - | Yes |
| Onus Coruscum | 5/10 | Self | stat_boost | - | ATK/DEF/SPD +25%, crit rate +15pp, crit damage +25pp, damage reduction +15pp | 3 | - | Yes |
| Lux Sanctorum | 5/8 | Self | heal_hot | 3000% HP healing | HoT 600% HP/tick | 3 | - | Yes |
| Mutatio Horologii | 2/4 | Self | stat_boost | - | ATK/DEF/SPD +40% | 2 normal actions | +800 once | Yes |
| Claustrum Lucis | 3/6 | Self | barrier | 1500% HP barrier | Barrier | 3 | - | Yes |
| Radius Misericordiae | 1/3 | Self | heal_hot | 500% HP healing | HoT 100% HP/tick | 2 | - | Yes |
| Gradus Prismatis | 1/4 | Self | defensive_buff | 500% HP barrier | Damage reduction 20% | 2 | - | Yes |
| Acus Fulgoris | 1/3 | Enemy | damage_ignore | 500% ATK, ignores DEF | - | - | - | Yes |
| Curatio Refractionis | 5/8 | Self | heal_hot | 3000% HP healing | HoT 600% HP/tick | 3 | - | Yes |
| Praesidium Lucis | 5/8 | Self | barrier | 3500% HP barrier | Barrier | 1 | - | Yes |
| Corruptio Lucis | 4/7 | Enemy | damage_defdown | 2200% ATK | DEF down 220% | 4 | - | Yes |
| Purgatio Lucis | 3/5 | Self | cleanse_resist | Cleanse | Damage reduction 30% | 3 | - | Yes |
| Lorica Lucis | 2/4 | Self | defensive_buff | 1000% HP barrier | Damage reduction 20% | 3 | - | Yes |
| Refractio Ultima | 5/10 | Enemy | damage_ignore | 3200% ATK, ignores DEF | - | - | - | Yes |
| Lumen Spiculum | 3/6 | Enemy | damage_burn | 1500% ATK | Burn, DOT potency +50 | 3 | - | Yes |
| Vortex Lucis | 4/8 | Enemy | damage_tridot | 2100% ATK | Burn/Bleed/Poison, DOT potency +40 | 3 | - | Yes |
| Siphon Lucis | 4/6 | Enemy | damage_lifesteal | 1800% ATK; heal 25% of actual HP damage | - | - | - | Yes |
| Aegis Lucis | 5/12 | Self | defensive_buff | 3000% HP barrier | Damage reduction 30% | 3 | - | Yes, text corrected from 20% |
| Cataclysmus Refractionis | 5/12 | Enemy | damage_ignore | 3600% ATK, ignores DEF | - | - | - | Yes |
| Ascensio Lucis | 5/10 | Self | stat_boost | - | ATK/DEF/SPD +25%, crit rate +15pp, crit damage +25pp, damage reduction +15pp | 3 | - | Yes |

## Corrections and retained limitations

- `Corruptio Lucis`, `Nota Luminosa`, and `Revelatio Occulta` now author DEF-down potency independently of direct damage (220%, 90%, 90%). Runtime values are unchanged.
- Barrier, damage-reduction, damage-over-time, and heal-over-time status magnitudes are likewise explicit metadata. The 3000% barrier's 30% reduction was retained and its description corrected.
- The former `revive` ID was only a one-turn barrier; it is now `barrier`. Unused `all_damage` and `all_burn` target IDs were removed. No resurrection or multi-target damage was invented.
- English and Greek catalogue names were replaced with Latin names; no save migration is necessary because this console build has no disk-save system. Effect identifiers remain internal English strings.
- Existing trigger-based unlock mappings were updated to the Latin names and are regression-tested.
- Gradus Coruscus and Mutatio Horologii now retain their 40% ATK/DEF/SPD buffs through other Shimmer casts and consume the two-turn duration only on normal attacks, matching their description. Their single-cast +800 gauge gain remains unchanged.
- Combat's enemy resistance currently defaults to zero, so normal player Shimmer debuffs effectively cannot be resisted at the default accuracy. The regression suite forces low accuracy at the shared status-application layer to cover the resisted branch without changing combat rules.
