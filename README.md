# Micare - Cathedral of Lies

Micare is a standard C++17 console RPG for the course deliverable. Open [dev/COS-PP1.slnx](dev/COS-PP1.slnx) in Visual Studio and build **Debug x64**, with **COS-PP1** as the startup project.

The complete Visual Studio solution, all source/header files, and build outputs are inside `dev`. The application uses keyboard input and console output; it has no GUI or Python dependency.

- [Build instructions and keyboard controls](dev/README.md)
- [Shared gameplay architecture](docs/Micare-System-Design.md)
- [Milestone 2 changelog](docs/NAyoola_Milestone2.md)

The gameplay loop includes Fountain recovery, room navigation, weighted investigations, items and equipment, ordinary combat, XP and level-up rewards, Shimmer loadouts, fixed first-clear bosses, room unlocks, and farming in cleared rooms. Combat events and defeat history remain reviewable in the console.

The pre-conversion gameplay and graphical implementation are preserved at Git checkpoint `fa9bc0b` on branch `archive/gui-before-course-conversion`. Graphical components are absent from the assessed build. Existing course/reference documents remain under `docs` and illustrations under `images`; neither is a runtime dependency.

## Milestone 2 stand-up
**Overview.** This week I continued the console conversion of Micare. I refactored the combat turn engine out of one oversized function into named helpers, split the console presentation into a pure formatting module (`ConsoleFormat`), added a shared catalogue-lookup helper (`Catalog`) used by both the enemy and item catalogues, and tightened enum validation at the catalogue boundaries in `GameplayTypes`.

**Challenges.** The main obstacle was a linking failure introduced when `ConsoleUI` began depending on the new `ConsoleFormat` translation unit — the test/harness projects that compiled `ConsoleUI.cpp` no longer linked. I resolved it by making the dependency explicit, then simplified the repository by removing the generated capture folder and the standalone test projects so the solution builds as a single clean deliverable. Keeping combat behaviour identical while extracting helpers was the other challenge; I verified each change with MSVC C++17 `/permissive-` compile checks and a full clean rebuild.

**Accomplishments.** I levelled up on separating pure logic from I/O — `ConsoleFormat` has no console dependency, so panel text is now testable in isolation. I also learned to validate enum values at the deserialization boundary instead of scattering magic number casts, and to consolidate duplicated catalogue code into one header. The codebase is noticeably cleaner as a result.

**Next Steps.** Before Week 3 I plan to add disk-based save/load so progress persists across sessions, expand the ability catalogue, and finish the usability pass on combat prompts and error messages.
