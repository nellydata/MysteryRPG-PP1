# Version Control and Markdown

## Project Information

- **Course:** COS Project & Portfolio 1
- **Student:** Nelson
- **Project:** Micare - Cathedral of Lies
- **Language:** C++17
- **Final Platform:** Console application
- **Development Environment:** Visual Studio and GitHub

## Terminal Basics

During this course I became more comfortable navigating projects and working with source control through the terminal.

### Common Commands

- `clear` - Clear the terminal screen.
- `pwd` - Display the current working directory.
- `ls` - List files and folders.
- `ls -a` - Include hidden files.
- `ls -alh` - Display a detailed, human-readable directory listing.
- `cd folder` - Enter a folder.
- `cd ..` - Move up one directory.
- `cd ../..` - Move up two directories.
- `cd ~` - Return to the home directory.
- `cd ~/Desktop` - Move to the Desktop directory.

A folder path can also be dragged into a terminal after typing `cd `, which makes navigating to long project paths easier.

## Version Control

Version control records changes to a project over time and makes it possible to review, restore, and share different versions of the code.

### Types of Version Control

- **Local:** Version history exists on one computer.
- **Centralized:** A central server stores the main version of the project.
- **Distributed:** Every developer has a complete copy of the repository and its history.

Git is a distributed version control system.

## Git Commands Used

- `git clone URL` - Copy a remote repository locally.
- `git status` - View modified, staged, and untracked files.
- `git add file` - Stage a file for a commit.
- `git add .` - Stage all intended changes in the current repository.
- `git commit -m "message"` - Record staged changes with a message.
- `git log` - Review commit history.
- `git diff` - Review unstaged changes.
- `git diff --cached` - Review staged changes.
- `git pull` - Retrieve and integrate remote changes.
- `git push` - Upload local commits to the remote repository.
- `git branch` - View or manage branches.
- `git branch --show-current` - Display the current branch.

GitHub was used as the remote repository for Micare throughout the course.

## Git Workflow Used for Micare

My normal workflow became:

1. Work from the `dev` branch.
2. Use GitHub issues and milestones to plan development tasks.
3. Make and test changes locally.
4. Review changes with `git status` and `git diff`.
5. Stage completed work.
6. Create meaningful commits.
7. Push the changes to GitHub.
8. Merge completed milestone work into `main` through pull requests.

This workflow helped keep experimental development separate from the stable project history.

## .gitignore

The `.gitignore` file prevents generated and machine-specific files from being committed.

For Micare, files such as these are excluded:

- Visual Studio cache files
- `.vs/`
- object files
- compiled executables
- intermediate build files
- temporary validation output
- per-user IDE settings

These files can be recreated and are not required source files.

The repository keeps the actual source code, project configuration, documentation, screenshots, and relevant testing material.

## Micare Project Structure

The final project is organized primarily into:

- `dev/include/` - Header files and class declarations
- `dev/src/` - Main C++ implementation files
- `dev/tests/` - Development and regression testing source
- `dev/validation/` - Final validation documentation
- `docs/` - Milestone and design documentation
- `images/` - Project screenshots
- `README.md` - Project overview and milestone stand-ups

The final Visual Studio solution uses `COS-PP1` as the assessed application and launches Micare directly.

## Version Control Growth During the Project

Micare changed substantially throughout the course.

Early versions established the basic multi-file C++ project, map, rooms, enemies, combat, inventory, progression, and game loop.

Later milestones added and refined:

- enemy and Shimmer catalogues
- equipment and item statistics
- room-based investigations
- combat status effects
- Shimmer loadouts
- Artifact equipment and set bonuses
- category-based inventory navigation
- boss progression
- developer-mode testing
- improved console presentation
- error handling and navigation
- automated regression and endurance testing
- object, pointer, and reference refactoring
- final narrative and usability polish

Git history allowed these systems to be developed incrementally while preserving earlier versions of the project.

## Final Milestone Version Control

For Milestone 4, I used version control to manage the final cleanup and validation process instead of making one large last-minute change.

The final development work included:

- completing the Floor I progression
- improving investigation and narrative presentation
- refactoring room lookup and reference usage
- fixing C++17 portability issues
- updating automated testing tools
- validating the project under Visual Studio Debug x64
- cleaning generated and temporary files from the repository
- documenting the completed project
- preparing the final `dev` to `main` pull request

Using commits, issues, milestones, and branches made it easier to track which changes belonged to each part of the project and reduced the risk of losing working versions of the game.

## Biggest Version Control Takeaway

The most important thing I learned was that Git is more useful than simply backing up code.

Throughout Micare, version control became part of the development process itself. Branches allowed me to separate stable and active work, commits created checkpoints, issues helped organize tasks, and pull requests provided a final review point before changes reached `main`.

As the project grew across many `.h` and `.cpp` files, having that history became increasingly important.

## Reference Links

- [Markdown Syntax](https://guides.github.com/features/mastering-markdown/)
- [Git Commands Cheat Sheet](https://education.github.com/git-cheat-sheet-education.pdf)
- [Git Handbook](https://guides.github.com/introduction/git-handbook/)
- [Git Documentation](https://git-scm.com/docs)
- [About Git - GitHub Docs](https://docs.github.com/en/get-started/using-git/about-git)
- [Ignoring Files - GitHub Docs](https://docs.github.com/en/get-started/getting-started-with-git/ignoring-files)
- [Microsoft PowerShell Documentation](https://learn.microsoft.com/powershell/)
- [Visual Studio C++ Projects and Build Systems](https://learn.microsoft.com/cpp/build/projects-and-build-systems-cpp)
- [cppreference C++ Reference](https://en.cppreference.com/w/cpp)
