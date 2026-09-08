# Micare - Cathedral of Lies

Micare is a standard C++17 console RPG for the course deliverable. Open [dev/COS-PP1.slnx](dev/COS-PP1.slnx) in Visual Studio and build **Debug x64**, with **COS-PP1** as the startup project.

The complete Visual Studio solution, both standalone test projects, all source/header files, and build outputs are inside `dev`. The application uses keyboard input and console output; it has no GUI or Python dependency.

- [Build instructions and keyboard controls](dev/README.md)
- [Course conversion report](dev/validation/course-conversion.md)
- [Actual console smoke transcript](dev/validation/console-smoke.txt)
- [Automated validation results](dev/validation/progression-validation.txt)
- [Shared gameplay architecture](docs/Micare-System-Design.md)

The gameplay loop includes Fountain recovery, room navigation, weighted investigations, items and equipment, ordinary combat, XP and level-up rewards, Shimmer loadouts, fixed first-clear bosses, room unlocks, and farming in cleared rooms. Combat events and defeat history remain reviewable in the console.

The pre-conversion gameplay and graphical implementation are preserved at Git checkpoint `fa9bc0b` on branch `archive/gui-before-course-conversion`. Graphical components are absent from the assessed build. Existing course/reference documents remain under `docs` and illustrations under `images`; neither is a runtime dependency.
