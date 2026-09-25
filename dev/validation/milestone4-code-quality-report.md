# Milestone 4 Code-Quality Audit

Date: 2026-09-25  
Branch/starting HEAD: `dev` / `7654b1f`  
Commit created: no

## Issue found

`EncounterManager::ResolveInvestigation` constructed a temporary `MapManager` for every investigation only to look up immutable room metadata. Constructing the manager also generated all five mutable `Area` objects and their interactable containers. The lookup result itself was already read-only, so the temporary object graph expressed ownership that the caller neither needed nor used.

The boss unlock path also repeated a linear traversal of `MapManager::GetRooms()` and exposed the whole mutable room container to code that needed only one non-owning room pointer.

The ownership audit otherwise found the important runtime ownership boundaries sound: `Area` exclusively owns enemies with `std::unique_ptr<Enemy>`, `Inventory` exclusively owns items with `std::unique_ptr<Item>`, and combat/UI APIs generally use references or explicitly nullable non-owning pointers. `RollLootInstance(ItemDefinition, ...)` is intentionally pass-by-value because it produces a modified independent roll. Result objects intentionally return reward vectors by value so ownership can move to the caller.

## Exact design improvement

- Made immutable room-definition lookup static and retained its `const RoomDefinition*` result. The pointer refers to the function-local static definition catalogue, whose lifetime lasts until process exit.
- Added const and mutable `MapManager::FindRoom` overloads. They return nullable, non-owning pointers into the manager-owned room vector and prevent callers from requesting the entire mutable container for a single lookup.
- Replaced the investigation-time temporary `MapManager` with direct static definition lookup.
- Replaced duplicated next-room traversal with `FindRoom`.

This is a meaningful object/pointer improvement: it removes repeated construction and destruction of unrelated owned objects, makes read-only metadata access explicit, and narrows mutable access while preserving existing ownership and gameplay behavior.

## Progression and lore audit

- Verified the existing configured chain is Fountain -> Entrance Hall -> Archive Room -> Chapel -> Storage Room.
- Added targeted regression coverage that resolves the Fountain guardian and every first-clear room boss, checks the configured next room unlocks, and confirms the final Storage boss intentionally has no next-room unlock.
- Added one short, incomplete lore fragment to the Fountain purification and every first-clear boss milestone.
- Added room-specific minor fragments to existing clue-discovery events. These are observations rather than answers; no plot is concluded and the fragments require the player to connect recurring details.
- No combat values, spawn chances, rewards, ownership semantics, or existing system designs were rebalanced or redesigned.

## Files changed

- `dev/include/GameplayTypes.h`
- `dev/include/MapManager.h`
- `dev/src/MapManager.cpp`
- `dev/src/EncounterManager.cpp`
- `dev/tests/CompatibilityChecks.cpp`
- `dev/validation/milestone4-code-quality-report.md`
- `.gitignore` (allows the required report to be checkpointed while other generated validation captures remain ignored)

## Validation results

All validation used current source after the final changes.

- Clean Debug x64 solution build: passed; 0 warnings, 0 errors.
- `ArtifactRegression`: passed, exit 0.
- `ShimmerRegression` / `CompatibilityChecks`: passed, exit 0. This includes the new room lookup, complete boss-unlock chain, Fountain milestone, and milestone-lore assertions.
- `COS-PP1.exe --validate`: passed, exit 0 (`Micare content validation passed`).
- `ConsoleValidation --headers`: passed, exit 0; 49/49 isolated MSVC C++17 `/permissive-` fixtures passed.
- `ConsoleValidation --smoke`: passed, exit 0; fresh-character inventory, Shimmer, Fountain combat/purification, Entrance Hall unlock/navigation, and clean exit covered.
- `git diff --check`: passed.
- The normal `dev/bin/x64/Debug/COS-PP1.exe` built successfully; no locked executable or alternate output directory was encountered.

## Git status and checkpoint assessment

The tree contains only the seven intended modified/new source, test, report, and ignore-policy files listed above. No commit was made. With the complete requested validation suite passing and no whitespace errors, the tree is safe to checkpoint after review.
