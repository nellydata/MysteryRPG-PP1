# Inventory and equipment console UX pass

Date: 2026-09-20. Branch: `main`. Starting HEAD: `ddd703d`. This pass extends the uncommitted shared console UX work and makes no gameplay or balance changes. No commit was made. The pre-existing `images/devmodeworking.png` remains untouched.

## Architecture and navigation

- Replaced the flat mixed-item wall with an overview showing counts for Equipment, Consumables, Key items, and Clues.
- Each category has its own browser and eight-item pages. Page clamping is centralized and safely returns to the preceding valid page after removal or filtering.
- Numeric navigation keeps zero as Back. Invalid, negative, alphabetic, blank, and out-of-range input continues through the shared retrying prompt rather than acting or exiting.
- Equipment defaults to rarity-descending order so Artifact and Legendary gear remain visible. Sorting cycles through rarity, name, and recent acquisition.
- Equipment filters cover all items, each of the five authored slots (Head, Torso, Boots, Hand, Accessory), equipped-only, each rarity, and every set present in the inventory.
- Next on the last page and Previous on the first page report the boundary without changing state.

## Equipment and detail presentation

- Equipment rows show name, centralized rarity name, slot, set, concise main stat, equipped state, and lock state.
- Detail screens show rarity, slot, set, equipped state, non-zero authored stat lines, passive/Artifact unique description, actual projected stat changes, set state, replaced item, and confirmation.
- Comparisons call `Player::PreviewEquipment` and `Player::PreviewUnequip`; the UI does not duplicate equipment calculations. Only effective fields that change are printed, including HP, ATK, DEF, SPD, Accuracy, Resistance, Crit Rate/Damage, Evasion, healing, barrier, DOT, lifesteal, and debuff reduction.
- Incoming and displaced set counts are shown. Legendary thresholds are 2/3 and Artifact thresholds are 3/5; current ACTIVE/LOCKED state and post-action ACTIVATES/DEACTIVATES transitions are explicit. Descriptions come from `Item::getSetPassive()`.
- Artifact unique text comes from `Item::getPassive()`, which is copied from the authoritative catalogue. No ANSI is stored in names or descriptions.

## Consumables and story items

- Consumables have a separate list with rarity, quantity, short authored effect, lock state, and a contextual-use note. Failed contextual use explains that no item was consumed.
- Crux items and clues are separate, readable categories and never presented as equipment. Their existing authored descriptions are preserved; no narrative text was invented.

## Live UX validation

- Normal Clara: overview showed two starter equipment items and two consumables. The starter `linen_veil` detail displayed actual projected changes, required confirmation, equipped successfully, and returned to the same one-page equipment view. Consumables displayed quantity/effect and safely backed through detail, category, inventory, and dashboard layers.
- Adelaide: overview showed 151 equipment, 12 consumables, six key items, and 12 clues. Equipment occupied 19 stable eight-item pages. Artifact rarity filtering produced two pages; Verdigris Matron set filtering produced exactly its five slot-distinct uniques.
- Normal menu equip actions moved Verdigris Matron from 0 through 5 pieces. The third piece preview included +35pp DOT Potency from the real set calculation; the UI showed 3-piece activation. The fifth showed 5-piece activation. Unequipping returned 5 to 4 and showed 5-piece deactivation. Existing automated repeated-swap checks found no stat drift.
- Individual screens remained below a conventional console height: overview is compact, list pages show at most eight two/three-line entries, and detail length grows only for mechanically rich set/Artifact items. No clipping, unreadable colors, ambiguous zero stats, or broken page indexes were observed.

## Automated validation

- Clean Debug x64 current-source rebuild of `COS-PP1`, `ArtifactRegression`, and `ShimmerRegression` to `dev/bin/x64/InventoryUXValidation`: passed.
- `ArtifactRegression`: passed, including page bounds, effective-stat difference formatting, set threshold semantics, catalogue-authoritative Artifact text, plain underlying names, preview/actual threshold checks, and repeated equip/unequip drift checks.
- `ShimmerRegression` including current compatibility and Artifact checks: passed.
- `COS-PP1.exe --validate`: passed.
- Maintained current-source `ConsoleValidation --headers`: 49/49 passed.
- Maintained current-source `ConsoleValidation --smoke`: passed. The prior failure was caused by the legitimately obsolete flat-list sequence: its first inventory input `4` became the Clues category after the UX change. Git history confirmed that commit `35a0961` deleted the harness source/project while leaving an old binary behind. The self-contained `ConsoleValidation` project and its process/endurance support were restored from the immediately preceding revision, wired into `COS-PP1.slnx`, and its smoke sequence/assertions were updated for current navigation. No player-name compatibility branch was added.
- The updated smoke verifies new-character startup; inventory overview; Equipment category; linen-veil detail; nested Back navigation; Consumables category and cellar-rations detail; return to the dashboard; Shimmer menu; Fountain investigation; real Pulsus Luminis combat; Fountain purification; Entrance Hall unlock and navigation; and clean session exit.
- The standard Debug executable remains held by the user-owned debugger process, so the current-source build uses the isolated output above.

## Known limitations

- Set descriptions are authoritative catalogue paragraphs. They are not split into separately wrapped per-threshold prose because the content model currently stores one combined string.
- Clue content remains the existing short catalogue text; narrative expansion is future content work.

## Files changed by the combined uncommitted UX work

- `.gitignore`
- `dev/COS-PP1.slnx`
- `dev/ConsoleValidation.vcxproj` (restored and maintained)
- `dev/include/ConsoleFormat.h`
- `dev/include/ConsoleUI.h`
- `dev/src/ConsoleFormat.cpp`
- `dev/src/ConsoleUI.cpp`
- `dev/src/EncounterManager.cpp`
- `dev/src/GameManager.cpp`
- `dev/tests/ArtifactRegression.cpp`
- `dev/tests/ConsoleValidation.cpp` (restored and updated)
- `dev/tests/ConsoleProcess.h` (restored)
- `dev/tests/ConsoleEndurance.h` (restored)
- `dev/validation/console-ux-report.md`
- `dev/validation/inventory-ux-report.md`
