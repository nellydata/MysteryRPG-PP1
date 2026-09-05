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

## Reference Links
- [Markdown syntax](https://guides.github.com/features/mastering-markdown/)
- [Git Commands cheat sheet](https://education.github.com/git-cheat-sheet-education.pdf)
- [Git Handbook](https://guides.github.com/introduction/git-handbook/)
- [Git documentation](https://git-scm.com/docs)
- [About Git on GitHub Docs](https://docs.github.com/en/get-started/using-git/about-git)
- [Ignoring files on GitHub](https://help.github.com/articles/ignoring-files/)
- [Microsoft Learn .gitignore guide](https://learn.microsoft.com/visualstudio/version-control/gitignore?view=vs-2022)
- [Microsoft PowerShell documentation](https://learn.microsoft.com/powershell/)
