# Micare - Cathedral of Lies

## Version 0.4.0

This version records the finalized rarity-driven itemization system. Boots roll flat SPD as their main stat: Common +5, Uncommon +8, Rare +12, Epic +16, Legendary +20, and Artifact +25. Non-boot main stats use rarity values: ATK 100/150/200/300/400/500, HP 1000/1500/2000/3000/4000/5000, and DEF 75/100/150/200/250/300 from Common through Artifact. Equipment main stats and per-drop rolls are applied to runtime player stats.

Equippable substats are percentage-based and use slot-specific pools. Head items use Crit Rate, Crit Damage, ATK, DEF, and HP; torso items use HP, DEF, Resistance, and ATK; hand items use ATK, Crit Rate, Crit Damage, and Accuracy; boots use SPD, Evasion, Accuracy, ATK, and DEF; accessories use ATK, DEF, HP, Resistance, DOT Potency, Healing Potency, and Barrier Potency. Common and Uncommon items have no substats, Rare has one, Epic has two, Legendary has three, and Artifact has four. Substat rolls are Rare 5–7%, Epic 7–10%, Legendary 10–12%, and Artifact 12–15%. Upgrade-roll persistence is not yet part of the current item model.

Micare is a C++17 noir roguelike RPG with both a native Win32 desktop UI and a console mode. The player chooses an investigator name, explores a cathedral overtaken by the Shimmer, discovers clues and items, encounters NPCs, and fights enemies while progressing through Floor 1.

## Project Structure

- `dev/include/` - C++ header files
- `dev/src/` - C++ source files and program entry point
- `docs/` - project and course documentation
- `COS-PP1.vcxproj` - Visual Studio project

## Architecture Documentation

See [`docs/Micare-System-Design.md`](docs/Micare-System-Design.md) for an explanation of:

- Class responsibilities and manager relationships
- `std::vector` usage and deterministic collection ordering
- Ownership, smart pointers, references, and non-owning pointer decisions
- Exploration, combat, progression, loot, UI, and validation flow
- Direct links to the relevant implementation files in `dev/include/` and `dev/src/`

## Current Features

- Deterministic Floor 1 map with Entrance Hall, Archive Room, Chapel, and Storage Room
- Interactable rooms with neutral environmental descriptions
- Deterministic enemy spawning
- Combat with Attack, Item, Shimmer, and Flee actions
- Health, attack, defense, XP, and level progression
- Item and clue inventory listing
- NPC placeholders and input validation
- Typed HP, DEF, SPD, ATK, ACC, RES, critical rate, and critical damage stats
- Equipment slots, rarity metadata, artifact effects, consumables, clues, and crux items
- Bleed, burn, poison, stun, healing-over-time, barrier, and mark status processing
- Percentage-based defense reduction, deterministic critical hits, and turn cooldowns
- Fifteen concrete shimmer skills with Latin display names and specification cooldowns
- Rarity-driven itemization with flat boot SPD mains, fixed non-boot mains, slot-specific percentage substats, and rarity-scaled substat counts/ranges
- Metadata-driven Shimmer Skills menu with descriptions, effects, equip/unequip loadouts, and combat selection
- Structured console UI with a persistent player panel, exploration/combat views, inventory overlay, map, and shimmer bar
- ASCII-safe rendering for Windows consoles, with optional ANSI color styling
- Hidden developer panel enabled with `MICARE_DEVELOPER_MODE=1`
- Native Windows desktop UI with clickable room, investigation, inventory, map, shimmer, and combat controls
- Player-selected name displayed in the character panel, combat messages, turn order, status messages, and event log
- Inventory paging with aligned item buttons, item details, rarity colors, sorting, locking, discarding, equipping, and consumable use
- Battle result reporting with defeat messages, XP rewards, and item drops
- Defensive state validation for room selection, stale enemies, and repeated command handling
- Deferred Win32 control refresh to prevent command-handler re-entry and UI freezes
- Completion-mode level cap and guarded XP progression to prevent infinite level-up loops
- Data-driven Devotee, Automata, and Verdigris enemy catalogue with Basic, Rare, Epic, Elite, and Boss tiers
- Tier-specific enemy scaling, fixed archetype speed, catalogue skills, cooldowns, passives, and drop metadata
- Data-driven enemy spawning, faction skill behavior, elite Shimmer Surge escalation, and boss phase transitions
- Structured combat events for attacks, skills, damage, DOT effects, healing, debuffs, deaths, and phase changes
- Runtime combat support for healing received, barrier potency, damage reduction, lifesteal, resistance, evasion, slow, debuff reduction, enemy criticals, and stun handling
- Room-definition-driven boss spawning and next-room progression based on required interactables
- Boss loot weighted toward Epic equipment with rare Legendary/Artifact outcomes and occasional consumable drops
- Separate owned equipment instances, including same-name drops with different rolled stats
- Temporary Shimmer buffs and an explicit Floor I completion state
- Shimmer unlock triggers and a 10-point Shimmer loadout tracker

## Building and Running

Open `COS-PP1.vcxproj` in Visual Studio and build Debug x64. The project uses C++17 and no external libraries.

## Reference Sources

- [C++17 language support](https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance)
- [Microsoft Win32 API documentation](https://learn.microsoft.com/windows/win32/api/)
- [Microsoft GDI drawing documentation](https://learn.microsoft.com/windows/win32/gdi/windows-gdi)
- [Visual Studio C++ project documentation](https://learn.microsoft.com/cpp/build/projects-and-build-systems-cpp)
- [C++ standard library documentation](https://en.cppreference.com/w/cpp)
- [Microsoft smart pointers overview](https://learn.microsoft.com/cpp/cpp/smart-pointers-modern-cpp)
- [Microsoft `std::vector` reference](https://learn.microsoft.com/cpp/standard-library/vector-class)
- [Git documentation](https://git-scm.com/docs)

The detailed cleanup implementation is recorded in [`docs/Documentation Progress`](docs/Documentation%20Progress) and [`docs/Micare-System-Design.md`](docs/Micare-System-Design.md), with source links to the relevant C++ headers and implementations.

## Milestone Stand-Up

### Overview

This milestone focused on building the Micare architecture, a playable console prototype, and a native Win32 desktop experience. I organized the project into separate header and source folders, created the core classes, and connected map, encounter, combat, progression, player, enemy, inventory, and UI systems.

### Challenges

The main challenges were keeping the project structure consistent with Visual Studio, separating room exploration from combat, and keeping native Win32 controls aligned across different window sizes. I also diagnosed UI freezes caused by synchronous refresh paths and an infinite level-up loop when the XP threshold was zero.

### Accomplishments

I built a deterministic Floor 1 map, added four interactable rooms, created basic enemy types, implemented combat actions, added XP and level progression, created readable item and clue inventory output, and added a clickable desktop UI. The project builds successfully with the Visual Studio project configuration.

### Progress Update - 9/5

#### Itemization and Item Information

- Added explicit item metadata for flat main-stat values and percentage main-stat values.
- Added explicit rolled substat metadata so generated item stats can be applied and displayed consistently.
- Normalized rarity-based item generation and substat scaling.
- Kept boot SPD as a flat main stat while preserving the intended flat DEF and ATK main-stat rules.
- Corrected HP main-stat handling so Linen Veil provides percentage maximum HP instead of an excessive flat HP bonus.
- Updated the item information panel to display only the item name, description, main stat, all substats, and available effects or passive information.
- Added inventory filtering by equippable type: Head, Torso, Boots, Hands, and Accessory.

#### Combat and Target Selection

- Reset Shimmer skill cooldowns when a new encounter begins so cooldowns do not carry over from a previous battle.
- Added explicit target confirmation for attacks and single-target Shimmer skills.
- Added target cancellation before an action is committed.
- Added selected-target labels in combat so the player can see which enemy is currently selected.
- Cleared pending target selection when combat ends.

#### Health and Defeat Recovery

- Ensured new games initialize the player at full HP.
- Prevented non-combat environmental damage from leaving the player stranded at 0 HP without a recovery path.
- Preserved fountain recovery behavior: the player can respawn into the same battle at 10% HP or leave the battle and return to the main floor at full HP.
- Cleared combat selection state when leaving combat to prevent dead or stale battle UI states.

#### Validation

- The updated workspace build completed successfully.
- Targeted modified files reported no compilation errors.

### Next Steps

The core catalog and gameplay systems are now data-driven through `ContentCatalog`, while the room loop remains deterministic and console-friendly. Further work can expand NPC dialogue, procedural room generation, save/load support, and narrative branches without changing the core stat or item APIs.

## UI Controls

The main dashboard keeps the chosen investigator's live stats, statuses, and equipped items visible while navigating:

- `1`-`4` - enter a Floor I room
- `5` - open inventory
- `6` - view Latin shimmer skills and cooldown state
- `7` - view the Floor I map
- `0` - leave the cathedral or return from a panel

Inside a room, the UI provides environment inspection, room search, inventory, and map actions. During combat, the player panel remains visible above enemy stats, turn order, and combat actions.

Set `MICARE_NO_COLOR=1` for plain output. Set `MICARE_DEVELOPER_MODE=1` to reveal the hidden developer-mode panel.

## Desktop UI

The normal build launches **Micare - Cathedral of Lies** with a name prompt before the native Windows UI opens. The entered investigator name is shown throughout the character panel, combat turn order, and gameplay messages. Its layout uses the reference-inspired hierarchy of a persistent character/equipment panel, a central gameplay viewport, an event log, and action buttons:

- Room buttons enter Floor I locations.
- Investigation buttons inspect individual objects and immediately update the clue log.
- Search, inventory item, map, and shimmer buttons operate without numeric console input.
- Combat buttons perform Attack, Item, Shimmer, and Flee actions while refreshing enemy/player state.

For debugging or environments where a window is not available, set `MICARE_CONSOLE_MODE=1` before launching. The repository currently contains no local image asset files, so the desktop UI uses native drawing and the supplied visual references for its dark, brass, verdigris, and shimmer composition.

0.5.0
