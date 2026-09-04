# Micare

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

The next priority is adding the first working Shimmer skill, improving NPC and clue interactions, and expanding the room loop while keeping the systems small and deterministic.