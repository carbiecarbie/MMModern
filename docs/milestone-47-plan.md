# Milestone 47 plan - Original dialogs

**Tier B. Status: completed and accepted.** Part A in `02214e1`; Part B in the
following M47 Part B commit.

## Goal

Replace the project's inventory and service menus with the original dialogs,
operable by mouse and keyboard, following the pinned ScummVM reference.

## Final scope

- **Part A - character sheet and inventory.** Dialog hit tables on the M45/M46
  input path, shared Y/N `Confirm` and `ErrorScroll`, window borders and text
  controls. Original `CharacterInfo` sheet (stat cells, popups, cursor blink,
  member switching) opened from F1-F6 or portraits; original `ItemsDialog` in
  character mode (Weapons/Armor/Accessories/Misc, Equip, Remove, transfer by
  row then F-key or portrait, antidote Use). Main-screen `I` is Info, as in
  the original. In combat the dialogs are view-only: equip/remove/transfer say
  "not supported yet"; Use and Exchange give the original refusals.
- **Part B - services.** Original location frame (town art visible, right-hand
  text panel, party strip), which also uncovers the Temple art. Smith uses
  `ItemsDialog` in Buy mode with Y/N confirmation; Fix uses the existing Armor
  Repair rules. Training `T` and Temple `H` act in one step; Temple keeps its
  Donation/Uncurse controls. The project quote/result phases were removed
  without changing prices, payment, stock, service days, interest or RNG.
- **Not supported yet** (notice, no state change): Discard, Quest, Quick
  Reference, Exchange, Awards, Info; Sell, Identify, buying Accessories/Misc,
  Fix of non-armor items, Temple Donation and Uncurse when they have a cost.
- All buttons show the original pressed frame for 100 ms, shared with the main
  screen.

## Decisions

1. **Text source.** Original strings are never checked in. A pinned build-time
   generator (`tools/GenerateXeenItemCatalog.ps1 -DialogText`) reads named
   fields of ScummVM's `CONSTANTS_7` Git blob, verified by revision, object
   identity, size and SHA-256; see [dependencies](dependencies.md).
2. **New character fields** (resistances, skills, awards, birth day, food) are
   read-only from the original resources; the save format and M44 digests are
   unchanged. **Rule:** the first milestone in which any of these fields can
   change must persist them in the save in that same milestone (food with
   Rest; skills and awards with the Guild or quests).
3. **Original key behavior** is followed (one-step Train/Heal, inventory
   through the sheet, `I` = Info).

## ScummVM deviations not reproduced

- `ItemsDialog::setEquipmentIcons` sets an accessory `_id = 8` where it means
  the frame; the item's own frame is used.
- `CharacterInfo::execute` forces `MODE_COMBAT` after every stat popup; the
  caller's mode is restored.
- Repair's `calcItemCost` call passes `actionIndex` as the Merchant skill;
  the M38 Armor Repair prices are kept.
- The pinned `FMT_CHARGES` has a redundant literal alignment letter that draws
  a stray title glyph; the maintainer's DOSBox comparison confirmed the
  original has none, so the generator omits it.

## Recorded future work

Equipping during combat (allowed in the original through `ItemsDialog` in
`CHAR_INFO` mode) and the combat Use button need combat mutation authority
(Tier A).

## Results

- Tests: `xeen_dialogs`, `xeen_dialogs_original`, `xeen_dialog_generation`,
  `xeen_service_dialogs` (fast) plus adapted input, flow, allocation and
  process tests. Part A full CTest 143/143; Part B full CTest 144/144 (single
  job, lightweight runner); M44 scenario digests unchanged throughout.
- Maintainer play-test with keyboard and mouse, compared with the original in
  DOSBox: sheet, items, transfer, antidote use, save/reload, combat refusals,
  Ironworks Buy, Temple Heal/resurrection and art passed. Armor Fix and
  Training were not reachable in a short session; they are covered by the
  `services` digest scenario (paid Repair and Training) and the Smith and
  Training process tests.
