# Original quirks

MMModern reproduces the original DOS game, including its oddities. This page
records behavior that looks like a bug but is kept on purpose, so that nobody
"fixes" it by accident, and lists the cases where the pinned ScummVM reference
itself differs from the original and is not followed. Entries that could later
become opt-in quality-of-life fixes (always off by default) are marked
**QoL candidate**. Add an entry when a milestone deliberately keeps or rejects
such a behavior, with its evidence.

## Original behavior reproduced on purpose

| Behavior | Evidence | Where | Notes |
| --- | --- | --- | --- |
| In the save/load chooser, the scroll arrows show their pressed frame but do nothing, and the bar between them does nothing; only ten slots exist. | Maintainer DOSBox CD play, 2026-10-09 (an eleventh slot pair is ignored). | M54 chooser | QoL candidate (scrolling or more slots). |
| After a member runs from combat, the Quick Fight Options dialog keeps the old party indexing: F1-F5 and the portraits select the members by their pre-Run positions, so the visible portrait and the selected member differ; F6 does nothing. | Maintainer DOSBox CD play, 2026-10-10; matches `dialogs_quick_fight.cpp:97-99` in the reference. | Planned in M55 | QoL candidate. Combat actions still use the real actor. |

## Unconfirmed in DOS, following the reference

Cases the original cannot be checked for in normal play; they follow the
pinned ScummVM reference under the AGENTS.md condition-counter rule.

| Behavior | Where | Notes |
| --- | --- | --- |
| A condition byte `0xFF` counts as the `-1` sentinel: it suppresses the Weak/Drunk change and the dawn Weak increment, and adds 1 in stat modifications. | M51, `XeenCombatRules.cpp`, `XeenCharacterRules.cpp` ("follows ScummVM; not confirmed in DOS") | Only reachable with extreme counters. |
| The eight-hour Poison and Disease checks only draw for members who do *not* have the condition; poisoned or diseased members neither worsen nor recover over time. | M51 daily time | Looks inverted in the reference; a DOSBox check of a poisoned member over several hours would settle it. |

## Reference behavior not reproduced

Places where the pinned ScummVM code differs from the DOS original or contains
a port defect; MMModern follows the original instead.

| Reference behavior | MMModern | Where |
| --- | --- | --- |
| `FMT_CHARGES` carries a redundant alignment letter that draws a stray glyph in item titles. | Uses the DOS bytes from `WORLD/XEEN.DAT`, without the glyph (DOSBox-confirmed). | M47, M53 |
| `ItemsDialog::setEquipmentIcons` sets an accessory frame through `_id = 8`. | Uses the item's own frame. | M47 |
| `CharacterInfo::execute` forces combat mode after every stat popup. | Restores the caller's mode. | M47 |
| Repair's `calcItemCost` passes `actionIndex` as the Merchant skill. | Keeps the M38 Armor Repair prices. | M47 |
| The control panel drops Save As and rearranges the buttons; saves use ScummVM's own chooser. | DOS panel and chooser from `WORLD/XEEN.DAT` and DOSBox evidence. | M54 |
| `_currentCantRest` tests a flag that the cell loader can never set, so only the map-wide flag refuses Rest. | Same effective behavior: only the primary map's flag refuses Rest. | M51 |
