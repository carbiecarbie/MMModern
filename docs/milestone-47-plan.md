# Milestone 47 plan - Original dialogs

**Tier B (roadmap). Status: approved by the maintainer on 2026-10-03; implementation not yet authorized.** Two parts, A then B,
each accepted separately. No save-format change in either part.

## Goal

Replace the project's keyboard-only inventory, Smith, Training and Temple
menus with the original dialogs - original art, layout, text, buttons and
keys - operable by mouse and keyboard on the M45/M46 input path. Add the
character sheet that F1-F6 and the portraits open in the original. Actions the
original offers that MMModern has no rules for show "not supported yet"; no
behavior is invented. Sources (pinned ScummVM, `engines/mm/xeen/`):
`dialogs/dialogs.cpp` (`ButtonContainer`), `dialogs/dialogs_char_info.cpp`,
`dialogs/dialogs_items.cpp`, `dialogs/dialogs_query.cpp` (`Confirm`),
`dialogs/dialogs_message.cpp` (`ErrorScroll`), `locations.cpp`, `window.cpp`
(window rectangles), `party.cpp` (`Party::subtract`, `notEnough`).

## Shared approach

- A dialog is a hit table plus a drawing: each button rectangle yields the key
  the original gives it (`ButtonContainer::checkEvents`), the portrait row
  (`addPartyButtons`) yields F1-F6, `(8,8)-(224,140)` yields Space. Clicks and
  keys reach the same `PlayerAction`s; dialogs stay strict contexts (fresh
  presses, no queueing, stale-frame input dropped, left button only).
  `InputContext` gains the active dialog's hit table; `SdlWindow` stays free of
  game rules.
- Existing owners keep the rules: `XeenParty`/`XeenRoster`, equipment, transfer
  and purchase rules, `XeenEventFlow`/`XeenEncounterFlow` and the Journey
  departure, authority and save-preimage machinery. Dialogs replace the menu
  text/hit layer and the project's extra phases, not the rules.
- Original behavior over project behavior: Train, Heal and Equip act in one
  step; Buy and Discard ask the original Y/N `Confirm`; the project's quote and
  result screens, `I`/`T`/`E`/`U` inventory keys and Smith lobby keys go away.
  On the main screen `I` becomes Info ("not supported yet"), as in the original.
- Text templates (character sheet, item lists, location panels, popups) use the
  original strings and the Xeen control codes. No original text is checked in:
  a pinned build-time generator produces it (task A0); extend
  `XeenTextRenderer` only for codes the templates need.
- Sprites exist in `XEEN.CC`/`DARK.CC` (verified): `view.icn`, `items.icn`,
  `buy.icn`, `equip.icn`, `esc.icn`, `train.icn`, `confirm.icn`.

## Part A - Character sheet and inventory (est. 2,500-3,500 lines with tests)

- [ ] **A0 Dialog input foundation.** Dialog hit tables in `InputContext`,
      click-to-action mapping, optional pressed-button frame, shared small
      dialogs: `Confirm` (windows 21/22, Y/N, Esc = No) and `ErrorScroll`
      (window 6). Clicks outside a dialog's rectangles do nothing.
- [ ] **A0b Dialog text generator.** A build-time generator modeled on the
      English item catalog (`tools/GenerateXeenItemCatalog.ps1`,
      [dependencies](dependencies.md)): it requires the exact pinned checkout
      and tree entry, reads (preferably) `devtools/create_mm/files/xeen/CONSTANTS_7`,
      reusing the item catalog's existing reader, or otherwise
      `devtools/create_mm/create_xeen/en_constants.h`, as
      a verified blob through `git cat-file` (size and SHA-256 checked, no
      worktree or compiler input), extracts only the named templates the
      dialogs use (character sheet, item list and Buy/Sell text, `Confirm` and
      error messages, stat popups, Smith/Training/Temple panels and their
      per-line formats), validates exact names, counts and bounded sizes, and
      emits a deterministic private include in the build tree only. The include
      is not committed, installed or read at runtime from companion data; a
      missing or malformed template fails the build. Tests use it through the
      same generated include. `dependencies.md` is updated with the new input.
- [ ] **A1 Character data.** The model lacks what the sheet shows:
      Fire/Energy/Magic resistances and temporaries, the 18 skills, awards,
      birth day, and party food. Read them read-only from the original
      CHR/PTY on load (see *Decisions*); they are not saved and no code in
      M47 changes them. Add the derived rules (item/condition-adjusted stats
      and `statColor`, age, total resistance via item bonuses, skill and award
      counts, experience to next level, food days = food / members / 3),
      reusing `XeenCharacterRules`.
- [ ] **A2 Character sheet** (`CharacterInfo::execute`, `loadDrawStructs`,
      `addButtons`, `expandStat`): window 24, `view.icn` draw list, 20 stat
      cells with the blinking cursor (frames 48/49, 4-tick blink), arrows and
      Enter, click or number to open the stat popup (window 28, any key or
      click closes), F1-F6/portraits switch member, Esc exits. Opened from F1-F6
      and portraits in exploration and combat (combat party in combat). Replaces
      the M46 "Character sheet" notice. Item opens A3; Quick, Exch and Awards
      (cell 15) show "not supported yet"; Exch in combat shows the original
      "Exchanging in combat is not allowed!".
- [ ] **A3 Items dialog, character mode** (`ItemsDialog::execute`,
      `loadButtons`, `doItemOptions`, `ItemSelectionDialog`): windows 29/30,
      `items.icn` buttons, categories W/A/C/M, rows 1-9 by click or key,
      `equip.icn` glyphs, original list text and charges for Misc. Equip/Remove
      use `xeenSetEquipment`. Selecting an item then a portrait/F-key moves it
      to that member (original; no confirm; cursed-item and full-backpack
      messages); this replaces Transfer. Use (Misc) uses the existing charged
      antidote flow and the original blocked-state messages; any other item
      effect, Discard and Quest show "not supported yet". Combat Use button and
      `U` open the Misc list; effects follow existing support.
- [ ] Remove the project inventory view (`XeenInventoryView`), its keys and
      `UseConfirm`/`Confirm`/`ChooseDestination` modes once A3 covers them.

## Part B - Services (est. 1,200-1,800 lines with tests)

Depends on A0 and A3: the Smith is the Items dialog in Buy mode.

- [ ] **B0 Location frame** (`BaseLocation::show/drawBackground/drawWindow`):
      town art stays visible at `(8,8)`, text panel is window 10
      `(226,0)-(320,146)` with the original button rectangles, `esc.icn` Esc at
      `(261,108)`, party strip below with the selected member highlighted
      (F1-F6/portraits switch member). Static art frame 0. Fixes the Temple art
      being covered. Esc departs with the existing departure rules.
- [ ] **B1 Smith** (`BlacksmithLocation`, `ItemsDialog` Buy mode,
      `calcItemCost`): panel with Browse (B) and Esc; Browse opens the Buy
      dialog (`buy.icn`): categories, Buy/Sell/Identify/Fix, rows, original
      cost column, F-key/portrait chooses the recipient. Buy asks `Confirm`,
      then the existing purchase rules (carried gold, unequipped delivery,
      stock depletion; backpack-full and not-enough-gold messages). Fix is the
      existing Armor Repair (broken items only; `Confirm`; "not broken"
      message). Sell, Identify, and buying Accessories/Misc show "not supported
      yet" (stock stays visible).
- [ ] **B2 Training** (`TrainingLocation`, `train.icn`): original experience /
      eligible / learned-all text; T trains immediately with existing Training
      rules (cost level^2 x 10, Vertigo cap, temps reset, HP/SP refilled, one
      day per newly trained member); not-enough-gold message; ineligible T does
      nothing, as in the original.
- [ ] **B3 Temple** (`TempleLocation`): original `TEMPLE_TEXT` with Heal cost,
      Donation, Uncurse cost and gold; H heals at once with the existing Temple
      rules. D (Donation) and U (Uncurse, when a cost is shown) show "not
      supported yet".
- [ ] Remove the project Smith/Training/Temple panels and their phases
      (Quote, Result) once B1-B3 cover them; Journey departure, authority and
      save-preimage handling stay.

## Out of scope

Town art animation, voices and sound; Sell, Identify, Enchant, Recharge,
Discard, Quests, Quick Reference, Exchange, Awards, Info, Dismiss, Map and the
Control Panel dialogs; Donation, Uncurse, Guild, Tavern, Bank; item-spell
effects beyond the existing antidote; the casting menu (stays keyboard-only);
mouse cursor art; Rest and food consumption.

## Tests

- Hit tables: every rectangle edge, portrait row, wait bounds, outside =
  nothing, right button ignored, strict-context rules (no queue, stale frame).
- Layout: each template renders on one page without clipping; sheet values vs
  the original formulas for the prepared party (original-data test).
- Save: format unchanged; the generic round-trip stays byte-identical and the
  A1 fields come from the original resources after a restore, as before it.
- Generator: rejects a wrong checkout, blob, hash, missing or oversized
  template; output is deterministic.
- Dialog actions: equip/remove, move item, Use antidote, Buy, Fix, Train,
  Heal; unsupported actions show the notice and change nothing.
- A few original-data end-to-end plays through each dialog by mouse and key.
- `ctest -L fast` while iterating; the complete suite once at the end through
  the AGENTS.md test-runner rule. The M44 scenario digests must stay unchanged;
  scenario key scripts are updated to the original keys.

## Acceptance

- Maintainer opens the sheet from portraits and F1-F6, browses stats, equips,
  removes and moves items, uses the antidote, buys and repairs at the Ironworks,
  trains, and heals at the Temple - each with mouse and with keyboard - and
  compares against the original (the installation ships DOSBox).
- Complete CTest passes.

## Decisions

1. **Text source.** Original strings are never checked in; they come from the
   pinned build-time generator (A0b), like the item catalog.
2. **A1 data.** The new character fields are read read-only from the original
   resources on load; the save and the M44 digests do not change. **Rule:** the
   first milestone in which any of these fields can change must persist them
   in the save in that same milestone - food in M48 (Tier A); skills and awards
   with the Guild or quests.
3. **Original key behavior** is followed: `I` is Info, inventory is reached
   through the character sheet, Train and Heal are one step.

## ScummVM deviations not reproduced

- `ItemsDialog::setEquipmentIcons` sets an accessory `_id = 8` where it
  presumably means the frame (an apparent typo); we use the frame the item's
  id selects, so the Buy-list glyph is not corrupted by an id change.
- `CharacterInfo::execute` sets `_vm->_mode = MODE_COMBAT` after every stat
  popup regardless of the real mode; that is engine bookkeeping, and we restore
  the caller's mode.
- `calcItemCost` is called for Repair with `actionIndex` where the Merchant
  skill goes. Armor Repair prices were settled in M38; the dialog shows those
  prices, not the ScummVM call's result.
