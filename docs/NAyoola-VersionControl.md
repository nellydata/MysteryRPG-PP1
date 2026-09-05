# Version Control and Markdown

## Project Information
- **Course:** COS Project & Portfolio 1
- **Student:** Nelson
- **Project:** Micare
- **Date:** September 6, 2026

## Terminal

### Essential Commands
- `clear` - Clear the terminal screen.
- `pwd` - Print the current working directory.
- `ls` - List files and folders.
- `ls -a` - List files and folders, including hidden files.
- `ls -alh` - List files and folders in human-readable form.
- `cd folder` - Change to a folder.
- `cd /` - Change to the filesystem root.
- `cd ~` - Change to the home directory.
- `cd ..` - Move up one folder level.
- `cd ../..` - Move up two folder levels.
- `cd ~/Desktop` - Change to the Desktop folder.

### Folder Drop
Typing `cd ` and dragging a folder into the terminal pasted its full path. Pressing Enter changed to that directory without typing the path manually.

## Version Control and Git

### Three Types of Version Control
- **Local:** Stores versions on one machine.
- **Centralized:** Stores the main repository on one server.
- **Distributed:** Gives each user a complete repository copy. Git is distributed.

### Essential Git Commands
- `git clone URL` - Clone a remote repository.
- `git config --global user.name "Name"` - Set the global Git username.
- `git config --global user.email "email@example.com"` - Set the global Git email.
- `git status` - Show the working tree and staging area.
- `git add file` - Stage a file for the next commit.
- `git commit -m "message"` - Create a commit.
- `git log` - Show commit history.
- `git help` - Show Git help topics.

### Connecting to GitHub with HTTPS
I clone the repository HTTPS URL, enter the repository with `cd`, then use `git add`, `git commit`, `git pull`, and `git push`. GitHub authentication is required when requested.

### .gitignore
A `.gitignore` file keeps generated files, IDE settings, operating-system metadata, and build output out of the repository. `.DS_Store`, `.vs/`, and `x64/` should be ignored because they are not source files and can be recreated.

## Micare Milestone Notes
The milestone established a multi-file C++17 console project under `dev/include` and `dev/src`. It includes a deterministic Floor 1 map, room interaction, enemy encounters, combat actions, progression, item storage, clue storage, and a main game loop.

## Change Tracking
- **2026-09-06 - `3d8ab69` - enemy and shimmer catalogue introduced:** Added the data-driven enemy catalogue, tier scaling, enemy skill metadata, elite and boss definitions, catalogue-based spawning, faction skill behavior, boss phase events, tier-aware drops, and Shimmer unlock/loadout metadata.
- **2026-09-06 - Documentation update:** Added catalogue feature bullets to `README.md`, added implementation progress bullets to `docs/Documentation Progress`, and added relevant C++, Win32, Visual Studio, and Git reference links.
- **2026-09-07 - Investigation, encounter, and recovery update:** Added previous-page navigation, investigation inventory access, area/level encounter scaling, fresh battle resets, named status events, immediate death logging, and zero-health recovery locks.
- **2026-09-08 - Version `0.4.0` - Shimmer loadout update:** Added metadata-driven skill descriptions/effects, skill equip and unequip controls, equipped-skill presentation in the player panel, and selectable equipped skills in combat.
- **2026-09-08 - Version `0.4.1` - Gameplay correction update:** Fixed starter skill availability and Skills presentation, bounded enemy scaling, corrected Verdigris Mark ownership and battle cleanup, added turn-gated target groups, weighted level-aware encounter selection, exact investigation no-result feedback, and active set-status presentation.
- **2026-09-08 - Version `0.4.2` - Skills and item-stat balance update:** Split the Skills screen into inventory and information panels, added skill sorting/filtering, limited target buttons to multi-enemy battles, applied percentage-based level-scaled equipment main stats with flat SPD substats, and made investigation consumable rewards reachable and room-aware.
- **Validation:** Debug x64 build completed successfully with zero errors; generated build output remains excluded by `.gitignore`.

## Reference Links
- [Markdown syntax](https://guides.github.com/features/mastering-markdown/)
- [Git Commands cheat sheet](https://education.github.com/git-cheat-sheet-education.pdf)
- [Git Handbook](https://guides.github.com/introduction/git-handbook/)
- [Git documentation](https://git-scm.com/docs)
- [About Git on GitHub Docs](https://docs.github.com/en/get-started/using-git/about-git)
- [Ignoring files on GitHub](https://help.github.com/articles/ignoring-files/)
- [Microsoft Learn .gitignore guide](https://learn.microsoft.com/visualstudio/version-control/gitignore?view=vs-2022)
- [Microsoft PowerShell documentation](https://learn.microsoft.com/powershell/)
- [Microsoft C++ language conformance](https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance)
- [Microsoft Win32 API documentation](https://learn.microsoft.com/windows/win32/api/)
- [Microsoft Windows GDI documentation](https://learn.microsoft.com/windows/win32/gdi/windows-gdi)
- [Visual Studio C++ projects and build systems](https://learn.microsoft.com/cpp/build/projects-and-build-systems-cpp)
- [cppreference C++ language and library reference](https://en.cppreference.com/w/cpp)
