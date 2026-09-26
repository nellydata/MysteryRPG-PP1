# Micare - Cathedral of Lies

Micare is a standard C++17 console RPG for the course deliverable. Open [dev/COS-PP1.slnx](dev/COS-PP1.slnx) in Visual Studio and build **Debug x64**, with **COS-PP1** as the startup project.

To build and run from a terminal, use `dev\Run-Micare.cmd` from the repository root. From a terminal currently inside `dev\src`, use `..\Run-Micare.cmd`. The project intentionally has no editor-specific launch folder; build the solution or use the launcher instead of trying to execute an individual `.cpp` file.

The complete Visual Studio solution, all source/header files, and build outputs are inside `dev`. The application uses keyboard input and console output; it has no GUI or Python dependency.

- [Build instructions and keyboard controls](dev/README.md)
- [Shared gameplay architecture](docs/Micare-System-Design.md)
- [Milestone 2 changelog](docs/NAyoola_Milestone2.md)
- [Milestone 3 changelog](docs/Milestone3.md)
- [Milestone 4 final-delivery changelog](docs/Milestone4.md)

The gameplay loop includes Fountain recovery, room navigation, weighted investigations, items and equipment, ordinary combat, XP and level-up rewards, Shimmer loadouts, fixed first-clear bosses, room unlocks, and farming in cleared rooms. Combat events and defeat history remain reviewable in the console.

The pre-conversion gameplay and graphical implementation are preserved at Git checkpoint `fa9bc0b` on branch `archive/gui-before-course-conversion`. Graphical components are absent from the assessed build. Existing course/reference documents remain under `docs` and illustrations under `images`; neither is a runtime dependency.

## Milestone 2 stand-up
**Overview.** This week I continued the console conversion of Micare. I refactored the combat turn engine out of one oversized function into named helpers, split the console presentation into a pure formatting module (`ConsoleFormat`), added a shared catalogue-lookup helper (`Catalog`) used by both the enemy and item catalogues, and tightened enum validation at the catalogue boundaries in `GameplayTypes`.

**Challenges.** The main obstacle was a linking failure introduced when `ConsoleUI` began depending on the new `ConsoleFormat` translation unit — the test/harness projects that compiled `ConsoleUI.cpp` no longer linked. I resolved it by making the dependency explicit, then simplified the repository by removing the generated capture folder and the standalone test projects so the solution builds as a single clean deliverable. Keeping combat behaviour identical while extracting helpers was the other challenge; I verified each change with MSVC C++17 `/permissive-` compile checks and a full clean rebuild.

**Accomplishments.** I levelled up on separating pure logic from I/O — `ConsoleFormat` has no console dependency, so panel text is now testable in isolation. I also learned to validate enum values at the deserialization boundary instead of scattering magic number casts, and to consolidate duplicated catalogue code into one header. The codebase is noticeably cleaner as a result.

**Next Steps.** Before Week 3 I plan to add disk-based save/load so progress persists across sessions, expand the ability catalogue, and finish the usability pass on combat prompts and error messages.

## Milestone 3 stand-up

**Overview.** This week I completed the gameplay compatibility pass and redesigned the console presentation without changing the approved balance. The game now has verified Shimmer metadata/runtime agreement, all 15 Artifact uniques and their three set families, an isolated Adelaide developer mode, production-backed room drop simulation, and category-based inventory/equipment navigation. I refactored and adjusted the structue and handling of multiple cpps.

**Improvements.** I separated presentation helpers into `ConsoleFormat`, consolidated player panels, combat HUDs, status/rarity labels, menu formatting, progress bars, and equipment comparisons. Inventory now separates equipment, consumables, key items, and clues; equipment browsing supports pagination, sorting, slot/rarity/set filters, authoritative Artifact descriptions, real projected stat comparisons, and clear set activation changes. The maintained console smoke harness was restored to the solution and updated to drive the current menus.

**Challenges.** A running debugger can lock the normal Debug executable and prevent the linker from replacing it. Validation therefore supports a separate output directory when needed, without terminating a user-owned process. The inventory redesign also made the old flat-menu smoke sequence obsolete; I corrected the harness itself instead of adding special-case behavior to the game.

**Accomplishments.** I improved my ability to separate game state from presentation, validate statistically generated content, preserve plain testable event data while adding console styling, and test full keyboard-driven workflows through the real executable. Current Artifact, Shimmer, compatibility, header, content, and console smoke regressions pass together.

**Next Steps.** Before Week 4, I want to stabilize the current build and make sure that everything including the new cpps are working together before finalizing the gameplay loop and flow and other milestone 4 adjustments.

## Milestone 4 final stand-up

**Overview.** This week I completed Micare's final-delivery pass. I finished the five-room progression and mystery-fragment presentation, kept first-clear and repeat-farming behavior distinct, completed the final object/reference audit, and brought the maintained console automation back in sync with the current category-first menus and multiline HUD.

**Improvements.** Room-definition lookup is now static, next-room access uses focused const/mutable lookup helpers, and the remaining C++17 portability problems in combat math and Shimmer construction are resolved without changing combat values. Console QA now runs through both Windows and POSIX process transports, parses the current inventory, player panel, Shimmer loadout, and combat picker, and performs ordinary-character progression, post-clear farming, Adelaide, and invalid-input coverage through the real executable.

**Challenges.** The largest issue was validation drift: the short smoke test reflected the current UI, but ConsoleEndurance still expected the older flat inventory and one-line player panel. Fixing the harness exposed the difference between a test executable merely launching and it actually observing the current game state. The Linux pass supplied full endurance evidence, while the final native Windows pass separately certified the Visual Studio Debug x64 build, MSVC header checks, console smoke, and PE console subsystem.

**Accomplishments.** The current source passes Artifact, Shimmer, compatibility, content, isolated-header, smoke, replay, and endurance validation in a clean GCC C++17 build. A fresh ordinary character reached level 100, cleared Fountain through Storage, completed 250 bounded post-cap investigations, observed 323 ordinary Storage encounters and 28 farming bosses, and cast all 31 Shimmers. Adelaide coverage verified level-100 isolation, advanced equipment and Artifact views, the complete Shimmer catalogue, and a 1,000-roll Storage boss sample. No combat balance values or major systems were changed.

**Next Steps.** Capture the 3–5 minute showcase and any required submission screenshots, review the prepared `dev` → `main` pull request, and merge only after final approval.
