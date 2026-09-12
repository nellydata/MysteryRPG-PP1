# Milestone 2 - Micare: Cathedral of Lies

## changes this week

this week was mostly focused on getting the systems i already had working together correctly instead of continuing to pile more stuff on top of broken systems.

- converted the class version of the project into a normal C++ console application and reorganized the Visual Studio project inside the dev folder
- kept the older GUI version separate so it can still be worked on later without affecting the version for class
- expanded the combat system with clearer damage, healing, barrier, status, cooldown and encounter tracking
- improved the inventory and equipment systems so individual item instances, locking, equipping, discarding and stat calculations are handled more safely
- reworked equipment stat generation because some of the early values were way higher than intended
- added proper equipment rarity ranges, main stats, substats and validation
- improved the leveling system, XP rewards and Shimmer skill rewards
- expanded room progression, investigations, boss encounters and farming after rooms are cleared
- added and tested consumable effects for combat and exploration
- fixed the Storage Room so its normal enemy pool can actually be used during investigations
- added more testing for combat, equipment, progression and longer playthroughs

## code structure updates

a lot of this week was also cleanup and separating responsibilities between systems.

console formatting was moved into its own helper instead of putting all of the formatting directly into the UI code. consumable behavior was also separated into its own effect system instead of having all of the item behavior mixed into the Player class.

equipment stat calculation and validation were also reorganized so the item, player and loot systems are using the same rules instead of each part interpreting stats differently.

the game is still split between classes for things like the player, enemies, areas, inventory, items, progression, encounters and combat.

## usability improvements

the console is still not where i want it visually, but i focused on making it understandable and usable before making it look better.

some of the usability changes this week were:

- numbered menus for navigation and combat
- input validation for menu choices
- clearer messages when rooms are locked or actions are unavailable
- complete encounter logs so i can see what happened during combat
- defeat review before returning to the Fountain
- inventory sorting and pagination
- clearer equipment information
- equipment stat previews so i can see how my stats would change before equipping something
- cooldown information for skills
- clearer room and boss progression information

## testing and debugging

one of the bigger things i changed this week was how i test the project.

instead of only checking whether the program compiles, i started doing full playthroughs through the actual console application.

during testing i played from a fresh character through the full first floor, tested losing and fleeing from encounters, leveling, equipment, consumables, Shimmer skills, boss fights and farming.

i also ran a longer endurance test that reached level 100 through normal gameplay. that exposed issues that shorter tests did not show, including equipment math, set bonuses and one room not spawning normal enemies.

the main thing ive learned from this part is that "it compiles" and "it works" are very different things.

## next steps

going into the next milestone i want to continue cleaning up the remaining gameplay bugs and then start improving the console presentation.

once the underlying systems are stable i want to make the console easier to read with things like health bars, status indicators, cleaner combat information and better spacing.

the mystery/narrative side of the game is also still behind the combat and RPG systems because most of this milestone was focused on getting the core gameplay loop working correctly.