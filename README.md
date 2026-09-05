# Micare - Cathedral of Lies

Micare is a console-based noir roguelike RPG written in C++17. Adelaide explores a cathedral overtaken by the Shimmer, discovers clues and items, encounters NPCs, and fights enemies while progressing through Floor 1.

## Project Structure

- `dev/include/` - C++ header files
- `dev/src/` - C++ source files and program entry point
- `docs/` - project and course documentation
- `COS-PP1.vcxproj` - Visual Studio project

## Current Features

- Deterministic Floor 1 map with Entrance Hall, Archive Room, Chapel, and Storage Room
- Interactable rooms with neutral environmental descriptions
- Deterministic enemy spawning
- Combat with Attack, Block, Shimmer, Interrogate, Expose Weakness, and Flee
- Health, attack, defense, XP, and level progression
- Item and clue inventory listing
- NPC placeholders and input validation
- Typed HP, DEF, SPD, ATK, ACC, RES, critical rate, and critical damage stats
- Equipment slots, rarity metadata, artifact effects, consumables, clues, and crux items
- Bleed, burn, poison, stun, healing-over-time, barrier, and mark status processing
- Percentage-based defense reduction, deterministic critical hits, and turn cooldowns
- Fifteen concrete shimmer skills with Latin display names and specification cooldowns
- Structured console UI with a persistent player panel, exploration/combat views, inventory overlay, map, and shimmer bar
- ASCII-safe rendering for Windows consoles, with optional ANSI color styling
- Hidden developer panel enabled with `MICARE_DEVELOPER_MODE=1`
- Native Windows desktop UI with clickable room, investigation, inventory, map, shimmer, and combat controls

## Building and Running

Open `COS-PP1.vcxproj` in Visual Studio and build Debug x64. The project uses C++17 and no external libraries.

## Milestone Stand-Up

### Overview

This milestone focused on building the initial Micare architecture and a playable console prototype. I organized the project into separate header and source folders, created the core classes, and connected map, encounter, combat, progression, player, and enemy systems.

### Challenges

The main challenges were keeping the project structure consistent with Visual Studio and separating room exploration from combat. I also diagnosed a Windows Application Control policy that blocked the unsigned debug executable even though the project compiled successfully.

### Accomplishments

I built a deterministic Floor 1 map, added four interactable rooms, created basic enemy types, implemented combat actions, added XP and level progression, and created readable item and clue inventory output. The project builds with zero errors and zero warnings.

### Next Steps

The core catalog and gameplay systems are now data-driven through `ContentCatalog`, while the room loop remains deterministic and console-friendly. Further work can expand NPC dialogue, procedural room generation, save/load support, and narrative branches without changing the core stat or item APIs.

## UI Controls

The main dashboard keeps Adelaide's live stats, statuses, and equipped items visible while navigating:

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
- Combat buttons perform Attack, Block, Shimmer, and Flee actions while refreshing enemy/player state.

For debugging or environments where a window is not available, set `MICARE_CONSOLE_MODE=1` before launching. The repository currently contains no local image asset files, so the desktop UI uses native drawing and the supplied visual references for its dark, brass, verdigris, and shimmer composition.
