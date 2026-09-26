# Milestone 4

## Overview

For Milestone 4, I focused on finishing and polishing Micare so the project feels complete and stable. Most of the work this week was improving the existing gameplay loop, cleaning up code structure, fixing remaining usability issues, and completing final testing before submission.

## Final Features Added

- Finalized the full Floor I progression from the Fountain through the Entrance Hall, Archive Room, Chapel, and Storage Room.
- Completed the investigation and clue flow for each room.
- Added room-specific narrative fragments and progression messages.
- Finalized first-clear boss encounters and repeatable post-clear room encounters.
- Completed the equipment, Artifact, Shimmer, consumable, and inventory systems already developed during previous milestones.
- Preserved Adelaide developer mode for testing advanced equipment, Shimmers, Artifacts, and late-game content.
- Added clearer end-of-floor messaging while leaving room for future areas and story content.

## Refactoring Improvements

- Improved room lookup logic so the game can access room definitions without creating unnecessary temporary objects.
- Added focused const and mutable room lookup functions using pointers and references where appropriate.
- Cleaned up combat math types for better C++17 compiler compatibility.
- Refactored Shimmer construction to remove problematic default-argument behavior while keeping the same gameplay effects.
- Updated the automated console testing tools so they match the current inventory, HUD, Shimmer, and combat menu formats.
- Separated testing utilities from the main game project so the final Visual Studio solution launches Micare directly.

## Bug Fixes and Usability Updates

- Fixed older testing logic that still expected the previous flat inventory menu.
- Updated inventory navigation for the current category-first layout and pagination system.
- Fixed parsing for the multiline player status panel and current Shimmer menus.
- Improved boss encounter text so it reads more naturally to the player.
- Replaced several placeholder investigation messages with more polished narrative text.
- Improved the final Floor I completion message.
- Verified invalid menu inputs, locked-room navigation, page limits, and clean exits.
- Removed unnecessary build files, temporary validation output, IDE cache files, and other files that were not needed for the final submission.

## Final Testing

The final project was tested in both Windows and a separate C++17 environment.

The final Windows checks included:

- Clean Visual Studio Debug x64 build
- Artifact regression tests
- Shimmer and compatibility regression tests
- Content validation
- 49/49 isolated header checks under MSVC C++17
- Console smoke testing
- PE console-subsystem verification

A full endurance test also confirmed that a normal character could progress through all five rooms, reach level 100, continue farming after completing Floor I, and use all 31 Shimmer abilities.

At the end of Milestone 4, Micare is a complete and playable console RPG build with its main gameplay systems, progression, testing, and documentation finished for submission.
