# Micare - Cathedral of Lies

Micare is a C++17 console RPG developed as my final course project. The game focuses on exploration, investigation, turn-based combat, equipment, Shimmer abilities, Artifacts, boss encounters, and progression through the first floor of the cathedral.

To build the project, open `dev/COS-PP1.slnx` in Visual Studio, select **Debug x64**, and run **COS-PP1** as the startup project.

The assessed version of the project is a standard console application with no GUI or Python dependency.

## Project Documentation

- [Build instructions and keyboard controls](dev/README.md)
- [Gameplay architecture](docs/Micare-System-Design.md)
- [Milestone 2 changelog](docs/NAyoola_Milestone2.md)
- [Milestone 3 changelog](docs/Milestone3.md)
- [Milestone 4 changelog](docs/Milestone4.md)

## Gameplay

The gameplay loop includes:

- room exploration and investigation
- turn-based combat
- XP and level progression
- equipment and inventory management
- Shimmer abilities
- Artifact equipment and set bonuses
- consumable items
- boss encounters
- room unlocking and progression
- repeatable encounters and farming after room completion

Floor I currently progresses through:

Fountain → Entrance Hall → Archive Room → Chapel → Storage Room

The pre-conversion graphical version of the project is preserved at Git checkpoint `fa9bc0b` on branch `archive/gui-before-course-conversion`. The final assessed build is console-only.

---

## Milestone 2 Stand-Up

**Overview.** This week I continued the console conversion of Micare. I refactored the combat turn engine out of one oversized function into named helpers, split console presentation into the `ConsoleFormat` module, added shared catalogue lookup functionality, and improved enum validation.

**Challenges.** One major issue was a linking failure after `ConsoleUI` began depending on the new `ConsoleFormat` translation unit. Some testing projects were not linking the new source file correctly. I fixed the project dependencies and continued simplifying the code structure while keeping combat behavior unchanged.

**Accomplishments.** I improved at separating game logic from presentation and learned how useful smaller, focused modules are for testing and maintenance. I also became more comfortable working with multiple `.h` and `.cpp` files instead of keeping too much logic in one place.

**Next Steps.** My next goal was to expand the game systems, improve console usability, and continue refining the overall gameplay loop.

---

## Milestone 3 Stand-Up

**Overview.** This week I completed a major gameplay and presentation pass. I finished the Artifact system, expanded Shimmer functionality, added Adelaide developer mode, improved loot testing, and redesigned the inventory and equipment menus.

**Improvements.** Inventory now separates equipment, consumables, key items, and clues. Equipment browsing supports pagination, sorting, filters, stat comparisons, Artifact descriptions, and set bonus information. I also improved the combat HUD, player information panels, and general console formatting.

**Challenges.** The redesigned menus caused older automated tests to stop matching the current interface. Instead of changing the game to satisfy outdated tests, I updated the testing tools to work with the new UI.

**Accomplishments.** I became much more comfortable separating presentation from game state, testing generated content, and maintaining larger connected systems without breaking existing gameplay.

**Next Steps.** My goal for the final milestone was to stabilize the entire project, complete the remaining progression and narrative work, and perform a full final testing pass.

---

## Milestone 4 Final Stand-Up

### ⚙️ Overview

This week I completed the final version of Micare for the course. I finished the full five-room Floor I progression, completed the investigation and narrative presentation, polished boss and room progression, and finalized the major gameplay systems that were developed throughout the project.

### 🛠️ Project Wrap-Up

Before submission, I focused on polishing and stability instead of adding another major system. I improved room lookup and object/reference usage, cleaned up several C++17 compatibility issues, updated the inventory and console testing tools to match the current interface, and improved several player-facing investigation and boss messages.

I also cleaned the Visual Studio solution so `COS-PP1` is the main project shown and launched during normal use while keeping the testing source available in the repository for development history and validation.

The final project passed a clean Visual Studio Debug x64 build along with Artifact, Shimmer, compatibility, content, header, and console validation tests.

### 🌵 Challenges

One of the biggest challenges was keeping older parts of the project synchronized as the game evolved. Some automated tests still expected older versions of the inventory, player panel, and Shimmer menus even though the game itself had already changed.

I solved this by updating the testing tools instead of adding special behavior to the game. I also spent a lot of time tracking down compiler, project configuration, and navigation issues while making sure fixes did not change the intended gameplay balance.

### 📚 Biggest Takeaway

The most valuable thing I learned during this course was how important organization becomes as a project gets larger.

At the beginning, I mostly thought about whether an individual feature worked. By the end of the course, I was thinking much more about class responsibilities, references and pointers, data flow, project structure, version control, regression testing, and whether a change in one system could affect another part of the game.

I also became much more comfortable working with a larger C++ project split across many source and header files.

### 🔮 Future Development

If I continue Micare after this course, I would expand the cathedral beyond Floor I with new rooms, enemies, bosses, investigations, and narrative content.

I would also like to add a proper save/load system, expand the Shimmer and Artifact systems, deepen character and enemy progression, and eventually continue developing the larger version of the game beyond the limitations of the course project.
