# Micare - Cathedral of Lies

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

