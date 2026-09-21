# Milestone 3 changelog

## Summary

Milestone 3 focused on completing and verifying Micare's existing gameplay systems, improving code organization, and making the C++17 console application easier to use. This milestone did not convert the project to a GUI or add external runtime dependencies.

## Features completed

- Completed the authored Shimmer catalogue/runtime consistency pass, including explicit effect metadata, emergency barriers, cooldown behavior, resisted statuses, and the two-action acceleration skills.
- Completed all 15 Artifact unique effects and the Verdigris Matron, Shimmer Reliquary, and Oracle Automaton set thresholds.
- Added Adelaide developer mode for level-100 content testing while keeping ordinary characters isolated at level 1.
- Added a room loot simulator that calls the production loot generators without awarding XP, clearing bosses, advancing pity, or mutating inventory during statistical samples.
- Added category-first inventory navigation, equipment filters and sorting, stable pagination, consumable/key/clue views, equipment confirmation, real stat previews, and set activation/deactivation presentation.
- Added a consistent exploration dashboard, combat HUD, compact status labels, Shimmer picker, and categorized combat-event presentation.

## System design updates

- `ConsoleFormat` owns pure presentation builders such as titles, menus, bars, rarity/status labels, cooldown text, stat changes, set progress, and page clamping.
- `ConsoleUI` composes those helpers into dashboard, room, combat, inventory, item-detail, Shimmer, and map views without embedding ANSI codes in game data.
- `DeveloperMode` owns developer activation, content initialization, and production-backed loot sampling. Runtime screens receive explicit developer state instead of repeatedly inferring privileges from display text.
- Equipment comparison continues to call `Player::PreviewEquipment` and `Player::PreviewUnequip`, keeping the live and preview calculation paths identical.
- `ConsoleValidation` is again a maintained solution project. Its smoke test launches the real executable and follows current category-first keyboard navigation.

## Refactoring and quality improvements

- Removed duplicated console formatting and standardized numeric prompts, Back behavior, EOF handling, invalid-input feedback, lock reasons, and message styles.
- Separated direct Shimmer multipliers from authored status potency so unrelated mechanics no longer derive values from each other.
- Preserved one authoritative source for Artifact and set descriptions in the content catalogue.
- Added targeted regressions for equipment preview/actual parity, mixed set thresholds, stat drift, developer content isolation, unique instance IDs, Artifact/Shimmer interactions, pagination bounds, plain underlying data, and real console navigation.
- Restored the current-source console process harness instead of relying on a historical binary with obsolete keystrokes.
- Removed repository-specific `.vscode` launch/task files; the supported build remains the Visual Studio/MSBuild solution under `dev`.

## Verification

The maintained Debug x64 solution contains the game plus Artifact, Shimmer/compatibility, and console validation projects. Milestone validation includes:

- clean solution build;
- `ArtifactRegression`;
- `ShimmerRegression` with compatibility checks;
- `COS-PP1.exe --validate`;
- isolated header compilation;
- an end-to-end real-console smoke covering startup, inventory categories and detail navigation, Shimmers, Fountain investigation/combat, room unlock, and clean exit;
- `git diff --check`.

## Known limitations

- Progress lasts for the running session; there is no disk-save system.
- ANSI color depends on terminal support, while underlying names and events remain plain.
