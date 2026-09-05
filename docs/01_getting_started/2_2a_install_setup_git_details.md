## Git Settings

### Details on Customization

To get the most out of Git, take some time to customize your git config settings. These user preference settings (configuration or "config" files) are stored in multiple locations:

**Local:** Settings only available to the current project †  
**Global:** Settings apply for any repository for the current user  
**System:** Settings apply to all user accounts

For detailed info on customization, visit [git-scm.com](https://git-scm.com/book/en/v2/Customizing-Git-Git-Configuration).

**Note** † The "current project" is the repository you are currently pointing to in Terminal: [pwd](https://www.git-tower.com/learn/git/ebook/en/command-line/appendix/command-line-101)

<br>

## How to Customize Settings

**List Configs...**  
These commands will pull up and list in Terminal your current git config settings...

```
git config --list         // List All settings - Global, Local and System
git config --global -l    // Global
git config --local -l     // Local
git config --system -l    // System
```

**Config Locations**  
CD via Terminal to be taken directly to your git config files...

```
~/.gitconfig   // Global
.git/config    // Local
/usr/local/etc  // System
```

**Edit Config**  
Command to quickly edit git config settings...

```
git config --global --edit   // Global
git config --edit            // Local
git config --system --edit   // System
```

**Manual way to Update Specific Config Settings**

```
git config --global user.name "Name Here"
git config --global user.email

// Change global to system or leave out to set for the current repo
```

**Check Settings**

```
Examples...

git config user.eMail
git config user.name
git remote show origin
git config --get remote.origin.url    // show just the URL
```

**Useful Git Config Settings (Global)**  
Global git settings are a conveninent way to set preferences for all repositories. You can always override a global setting by defining a local git configuration (see above).

To edit the Global config setting, use this command: `git config --global --edit`

<br>

## Issue & Solution

### Working on the same file using Windows and Mac

After much research, it turns out the **autocrlf** and **safecrlf** settings listed above are useful for a very specific working situation: when edits are being made to the same file on both a Mac and Windows machine. If you aren't experiencing CRLF errors then you probably don't need to worry about adding the autocrlf and safecrlf settings.

This article provides helpful background on the difference between Mac and Windows when it comes to editing text, and how that sometimes leads to a headache when using version control: [Mind the End of Your Line](https://adaptivepatchwork.com/2012/03/01/mind-the-end-of-your-line/)
