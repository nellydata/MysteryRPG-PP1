> Use this worksheet to plan the next phase of your project **before you begin coding**
> Be clear, specific, and intentional—this will guide your development this week.

---
## 📌 Project Overview

**Project Name:**
→ Micare: Cathedral of Lies  
  
**What does your program currently do? (1–3 sentences)**   
→ right now Micare is a console based RPG where the player can move through different rooms, investigate, fight enemies, collect equipment and consumables, level up and unlock new areas. the basic gameplay loop is there, but a lot of the systems still need testing because some of the math and interactions between combat, equipment and progression are not always doing what i expect.

---
## 🔍 Current Progress Check  
  
**What is working right now?**   
→ the game can start a new character, move between unlocked rooms, investigate, trigger enemy encounters, fight using attacks and skills, gain xp, level up, use equipment and consumables, and unlock new rooms after bosses. the inventory, enemy system, room system and most of the core combat loop are working enough for me to actually play through and test them.  
  
**What is NOT working or incomplete?**   
→ balancing is still rough and some equipment values are stronger than i intended. some status effects, item effects and set bonuses still need to be checked to make sure the description and the actual result match. the mystery and narrative parts are also still mostly placeholders because ive been more focused on getting the gameplay loop working first.  
  
**What feels confusing or messy in your code?**   
→ there are a lot more systems interacting now, so it can be hard to tell where a wrong value is coming from. combat, player stats, equipment, loot and progression all affect each other, so i want to keep cleaning up which class is responsible for what instead of having the same kind of logic spread across different files.

---
## 🚀 Feature Planning  
  
List the features you plan to add or improve this week.  
  
### Feature 1  
**Name:**   
→ combat and equipment correctness  
  
**What does this feature do?**   
→ go through damage, healing, status effects, equipment stats, item rarity and loot generation and make sure the numbers are being calculated consistently. i also want equipment to show the player what stats it is actually giving them.  
  
**Why is this feature important?**   
→ most of the game depends on these systems, so if the math is wrong then boss difficulty, leveling and loot are hard to judge. i want to fix that foundation before adding more content.  
  
---
### Feature 2  
**Name:**   
→ progression and room flow  
  
**What does this feature do?**   
→ improve how investigations, bosses, room unlocks, xp and repeat farming work together. the player should be able to progress through the floor normally, lose or flee without breaking anything, and return to cleared rooms to keep farming.  
  
**Why is this feature important?**   
→ this is the structure that holds the first floor together. i need to know a player can progress normally without getting stuck, skipping something important or getting rewards more than once.  
  
---
  
### Feature 3 (optional)  
**Name:**   
→ console usability improvements  
  
**What does this feature do?**   
→ make the console easier to understand with clearer menus, better combat logs, inventory sorting, better item information and more useful messages when an input or action is invalid.  
  
**Why is this feature important?**   
→ the console does not need to look good yet, but it does need to be readable enough to understand what is happening while playing and testing.  
  
---
## 🧩 System Design Updates  
  
**Will you need to create any new classes? If so, which ones?**   
→ i may create smaller helper classes or systems when one class starts doing too much, especially for console formatting or item effects. i want to avoid adding classes unless they actually make the code easier to manage.  
  
**Will you modify any existing classes? How?**   
→ yes. Player and Item need cleaner stat and equipment handling. CombatManager needs better tracking for damage, statuses and encounter history. EncounterManager and Area need progression and investigation fixes. ProgressionManager needs more testing around xp, level ups and skills. ConsoleUI also needs clearer output and navigation.  
  
**What data structures will you use (vectors, 2D vectors, etc.)?**   
→ i am using vectors for rooms, enemies, items, skills and combat events. inventory entries keep separate item instances so duplicate equipment can still have different stats. sets and maps can also be useful for things like equipped sets, protected item ids and unique unlocks.  
  
---
## 🔄 Program Flow  
  
**Describe how a user interacts with your program:**  
  
1. Program starts → the player enters a name or uses Adelaide as the default and sees the main floor menu.  
2. User chooses → the player selects a room or opens things like inventory, skills, the map or encounter log.  
3. Program responds → the game resolves the action, updates the game state and gives feedback through the console.  
4. Loop/next step → the player returns to the room or floor menu and keeps investigating, fighting and improving their character until the floor is cleared or they quit.  
  
---
## 🎯 Usability Improvements  
  
How will you make your program easier to use this week?  
  
- Clearer prompts:   
→ use numbered options consistently and make prompts explain what the player is choosing.  
  
- Better error handling:   
→ validate menu input so invalid numbers or other input do not crash the game or accidentally change the game state.  
  
- Improved menu/navigation:   
→ keep controls consistent between menus, add inventory sorting and pages when needed, keep encounter logs available and let the player back out of submenus safely.  
  
---
## ⚠️ Potential Challenges  
  
**What do you think will be the hardest part this week?**   
→ debugging the math between different systems. once equipment, status effects, skills, enemy scaling and xp are all interacting, one wrong value can look like a problem somewhere else.  
  
**What is your plan if you get stuck?**   
→ break the problem down and test one system at a time instead of changing a bunch of values at once. i can check the actual values being used, reproduce the problem in a smaller test and compare the result to what i expected before changing anything.  
  
---
  
## 📈 Level Up Goal  
  
**What skill are you focusing on improving this week?**   
→ debugging and testing  
  
**What will you do to improve it?**   
(e.g., tutorial, practice, debugging, office hours)   
→ im going to test the actual console program from a fresh character, use smaller tests for individual systems and compare expected values to what the game actually produces. i want to get better at finding the cause of a bug instead of changing numbers until the problem goes away.  
  
---
## 🗓️ Task Breakdown (GitHub Issues Planning)  
  
List the tasks you plan to create as GitHub Issues:  
  
- [ ] convert and organize the class version as a complete console build inside the required project structure  
- [ ] repair equipment stats, loot generation and inventory behavior  
- [ ] improve combat feedback, status handling and defeat/recovery behavior  
- [ ] test full room progression, bosses, leveling and repeat farming through actual playthroughs  
  
---
  
## 🔥 Final Check  
  
Before you start coding, ask yourself:  
  
- [x] Do I know what I’m building this week?   
- [x] Do I know where to start?   
- [x] Did I break my work into small tasks?   
  
If yes → start coding 🚀   
If no → refine your plan first   
  
---
## 😈 Final Thought  
  
> Plan it now… or debug it later.
