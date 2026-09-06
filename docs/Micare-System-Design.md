# Micare System Design

## Purpose

Micare - Cathedral of Lies is a C++17 noir roguelike RPG with two presentation paths: a native Win32 desktop interface and a console mode. The game is organized around small manager and model classes, deterministic Floor 1 content, catalogue-driven enemies and items, and a combat event stream that can be displayed by either interface.

This document describes the current implementation. Historical progress notes may describe earlier prototypes; the source files linked below are the implementation authority.

## Project structure

- [`dev/include/`](../dev/include/) contains public class declarations and shared data types.
- [`dev/src/`](../dev/src/) contains class implementations and the program entry point.
- [`COS-PP1.vcxproj`](../COS-PP1.vcxproj) defines the Visual Studio C++17 build.
- [`README.md`](../README.md) contains the user-facing feature summary and build instructions.

## Class design

### Shared data types

[`GameplayTypes.h`](../dev/include/GameplayTypes.h) contains value-oriented data used across systems:

- `Stats` stores combat and percentage modifiers.
- Enumerations such as `Rarity`, `EquipSlot`, `EnemyFaction`, `EnemyTier`, `EnemyRole`, and `RoomType` keep gameplay categories type-safe.
- `RoomDefinition`, `LootContext`, `DropProfile`, `EnemySkillDefinition`, `EnemyDefinition`, and `ItemDefinition` describe catalogue content without owning runtime objects.
- `StatusEffect` describes an active effect and its duration, potency, and source values.

These structures are intentionally simple value types. They can be copied when a catalogue entry is used as a template, such as when [`GenerateLoot`](../dev/src/ContentCatalog.cpp) creates a new rolled item instance.

### Player

[`Player.h`](../dev/include/Player.h) and [`Player.cpp`](../dev/src/Player.cpp) define the player model. `Player` owns the investigator's mutable gameplay state:

- health, SP, barrier, base stats, and recalculated total stats;
- an [`Inventory`](../dev/include/Inventory.h);
- active status effects;
- equipped item references;
- the investigator name and progression-facing values.

The class exposes gameplay operations such as damage, healing, status processing, equipment, consumable use, and stat recalculation. UI code reads through getters rather than directly changing fields.

### Enemy hierarchy and catalogue

[`Enemy.h`](../dev/include/Enemy.h) defines the runtime enemy model. It stores health, combat stats, statuses, faction, tier, phase, skill definitions, and skill cooldowns. The virtual destructor and virtual `takeDamage` method preserve polymorphic use for specialised enemy classes such as [`Automata`](../dev/include/Automata.h), [`Nun`](../dev/include/Nun.h), and [`Boss`](../dev/include/Boss.h).

[`EnemyCatalog.h`](../dev/include/EnemyCatalog.h) and [`EnemyCatalog.cpp`](../dev/src/EnemyCatalog.cpp) provide data-driven enemy definitions and room-compatible creation. The catalogue supplies tier scaling, roles, skills, cooldowns, passives, and loot metadata while `Enemy` owns the live battle state.

### Area and map

[`Area.h`](../dev/include/Area.h) models one room. It stores room state, visible interactables, search flags, progression flags, and the active enemy objects. `Area` is non-copyable and movable because it owns `std::unique_ptr<Enemy>` values.

[`MapManager.h`](../dev/include/MapManager.h) and [`MapManager.cpp`](../dev/src/MapManager.cpp) own the Floor 1 room list. `GenerateFloor1` creates the deterministic room arrangement and `SelectRoom`/`GetRooms` expose it to the game and UI. Room definitions provide shared metadata such as enemy pools, boss IDs, spawn conditions, and next-room relationships.

The current tutorial route starts in the Fountain and keeps Entrance Hall locked until the Fountain battle is cleared. Room state is stored in `Area`; the map manager defines the deterministic layout and room metadata.

### CombatManager

[`CombatManager.h`](../dev/include/CombatManager.h) and [`CombatManager.cpp`](../dev/src/CombatManager.cpp) coordinate combat without owning the `Player` or `Enemy` objects. The manager tracks:

- the active enemy and, for grouped turns, the current enemy party;
- turn state and gauges;
- combat flags such as blocking, exposed weakness, and consumable use;
- shimmer cooldowns;
- a vector of structured `CombatEvent` records.

`resolve` handles the player's selected action. `enemyTurn` handles enemy responses, including grouped enemy turns. Combat actions add structured events for attacks, skills, damage, status effects, healing, deaths, and other outcomes. The UI consumes these events to produce the battle log.

### Inventory and items

[`Inventory.h`](../dev/include/Inventory.h) owns item instances using `std::unique_ptr<Item>`. It also stores clues, quantities, lock state, and acquisition ordering. Sorting returns non-owning `Item*` views into the owned item collection.

[`Item.h`](../dev/include/Item.h) and [`Item.cpp`](../dev/src/Item.cpp) represent runtime item instances. [`ContentCatalog.cpp`](../dev/src/ContentCatalog.cpp) creates template definitions, selects rarity, and rerolls each dropped instance through `RollLootInstance`. This allows two items with the same name to have different rolled stats while remaining based on the same catalogue family.

### Progression and encounters

[`ProgressionManager.h`](../dev/include/ProgressionManager.h) and [`ProgressionManager.cpp`](../dev/src/ProgressionManager.cpp) manage XP, levels, Shimmer points, and equipped skill budget. [`EncounterManager.h`](../dev/include/EncounterManager.h) and [`EncounterManager.cpp`](../dev/src/EncounterManager.cpp) provide the console exploration and encounter flow.

[`GameManager.h`](../dev/include/GameManager.h) and [`GameManager.cpp`](../dev/src/GameManager.cpp) connect the console-facing map, room, encounter, combat, and progression loop.

### Win32 UI

[`Win32UI.h`](../dev/include/Win32UI.h) and [`Win32UI.cpp`](../dev/src/Win32UI.cpp) implement the desktop presentation layer. `Win32UI` owns the player, map manager, combat manager, progression manager, active UI view, window controls, navigation history, and display-oriented selections.

The UI does not duplicate the combat rules. It sends actions to `CombatManager`, schedules grouped enemy turns, synchronizes combat events into the visible log, and redraws current model state. Deferred refresh flags protect Win32 command handling from recursive refreshes.

## Vector usage

The project uses `std::vector` for ordered, variable-length collections:

- `MapManager::rooms_` stores Floor 1 rooms in deterministic navigation order.
- `Area::enemies_` stores owned enemies in room order.
- `Area::interactables_` and `visibleInteractables_` store room interaction data.
- `Inventory::items_` stores owned item objects, while `quantities_`, `locked_`, and `acquisitionOrder_` store parallel item metadata.
- `Player::statuses_` and `Enemy::statuses_` store active effects.
- `Enemy::skills_` and `skillCooldowns_` keep skill definitions aligned with cooldown state.
- `CombatManager::events_` stores the ordered event history used by the battle log.
- `CombatManager::currentEnemyParty_` stores non-owning pointers for the current grouped enemy turn.
- `Win32UI` vectors store navigation history, visible indices, target selections, and display lists.

Vectors are appropriate here because the game needs stable ordering for room navigation, deterministic catalog results, combat logs, and turn processing. Most collections are small, so contiguous storage and straightforward iteration are preferred over more complex containers.

Vectors do not automatically express ownership. The element type and surrounding API communicate that distinction: `std::vector<std::unique_ptr<T>>` owns objects, while `std::vector<T*>` is a temporary or observing view.

## Pointer and memory decisions

### Owning pointers

`Area` owns live enemies through `std::vector<std::unique_ptr<Enemy>>`. `Inventory` owns live items through `std::vector<std::unique_ptr<Item>>`. This gives each subsystem a clear lifetime boundary and automatically destroys objects when the owning container or owner is destroyed.

`Player::AddItem` and `Inventory::AddItem` accept `std::unique_ptr`, which makes ownership transfer explicit. Copying an `Area` is disabled because copying its unique-owned enemy objects would be ambiguous; moving an `Area` is allowed.

`CombatManager` owns its default shimmer skill with `std::unique_ptr<ShimmerSkill>`. This is appropriate because the manager owns the skill object and the skill is polymorphic.

### Non-owning pointers and references

Combat and UI managers observe objects owned elsewhere:

- `CombatManager` receives `Player&`, `Enemy&`, and `std::vector<Enemy*>` for actions.
- `Win32UI::activeEnemy_` and `targetEnemies_` point into the current room's enemy collection.
- `CombatManager::activeEnemy_` and `currentEnemyParty_` point into the current encounter.
- Inventory sorting returns `Item*` pointers into the inventory's owned collection.
- Equipped item pointers in `Player` refer to items owned by `Inventory`.

These pointers are not deleted by the observing class. They are valid only while the owning room or inventory keeps the referenced object alive and while operations such as dead-enemy removal or item removal do not invalidate the view. The code clears combat selections and ownership state when leaving combat to avoid stale UI references.

### Value semantics

Definitions and metadata are copied by value because they are small descriptions rather than long-lived runtime entities. For example, loot generation copies an `ItemDefinition`, changes its rarity-scaled main stat and substats, and then constructs an `Item` from the result.

References and `const` references are used for read-only access to avoid unnecessary copies while preserving ownership boundaries.

## Overall system design

The game follows a layered, manager-based design:

1. **Definitions and catalogues** describe rooms, enemies, skills, items, and loot rules.
2. **Runtime models** (`Player`, `Enemy`, `Item`, `Area`, `Inventory`) store mutable game state.
3. **Managers** coordinate map generation, encounters, combat, progression, and content validation.
4. **Presentation layers** (`Win32UI` and `ConsoleUI`) read model state and send commands to managers.
5. **Application entry** in [`main.cpp`](../dev/src/main.cpp) selects validation, console, or Win32 mode.

### Startup and mode selection

[`main.cpp`](../dev/src/main.cpp) checks `MICARE_VALIDATE` first. Validation runs catalogue/content checks and returns a process status. Otherwise, `MICARE_CONSOLE_MODE` selects the console path; the default launches the Win32 interface.

### Exploration flow

`MapManager` creates deterministic Floor 1 rooms. The active UI or console manager selects an `Area`, displays its interactables, and performs inspection/search actions through the appropriate encounter or UI methods. Room flags record clues, items, NPC discovery, boss encounters, and clear state.

### Combat flow

A room supplies live enemy objects to the combat layer. `CombatManager` begins the encounter, tracks turns, resolves player actions, and processes enemy actions. For a multi-enemy encounter, the grouped enemy path receives observing pointers to all living enemies and allows each eligible enemy to act according to its speed. Skills can target valid combat participants, and enemy skill use is emitted as a structured `CombatEvent::Kind::Skill` event for the battle log.

### Progression and loot flow

Combat outcomes award XP and may create a `LootContext`. `GenerateLoot` selects rarity and item templates using the deterministic context/seed, then `RollLootInstance` creates per-drop stat variation. The resulting definition is converted to an owned runtime `Item` and added to the player's inventory.

### Validation and determinism

The content builders and validation functions in [`ContentCatalog.h`](../dev/include/ContentCatalog.h) support deterministic checks of catalogue structure and generated content. Deterministic room and enemy selection makes the Floor 1 MVP reproducible while leaving the managers extensible for later content.

## Build and reference sources

Build with Visual Studio using the C++17 configuration in [`COS-PP1.vcxproj`](../COS-PP1.vcxproj). The project intentionally uses the C++ standard library and Windows APIs without external libraries.

- [C++17 language support](https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance)
- [Microsoft Win32 API documentation](https://learn.microsoft.com/windows/win32/api/)
- [Microsoft GDI documentation](https://learn.microsoft.com/windows/win32/gdi/windows-gdi)
- [Visual Studio C++ projects and build systems](https://learn.microsoft.com/cpp/build/projects-and-build-systems-cpp)
- [C++ standard library reference](https://en.cppreference.com/w/cpp)
- [Microsoft smart pointers overview](https://learn.microsoft.com/cpp/cpp/smart-pointers-modern-cpp)
- [Microsoft `std::vector` reference](https://learn.microsoft.com/cpp/standard-library/vector-class)
- [Git documentation](https://git-scm.com/docs)
