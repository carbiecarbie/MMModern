# Milestone 105 plan - The last mixed-map Event refusals

**Tier B. Status: completed and accepted by the maintainer (2026-11-01)**
after the play-test. Plan, DOS check (Part 0), implementation and closure in
one commit. Step 3a of the [Darkside arc](roadmap.md#darkside-arc), after
M104. Save v24 unchanged (Journey schema 9, content 14).

## Scope

The three original Event records M104's runtime-refusal sweep left on the
mixed World of Xeen maps, implemented once for every place the original uses
the mechanic:

| Map | EVT | Instruction |
| --- | --- | --- |
| Darkside 116, Darkstone Tower | @361 | TakeOrGive (12), world flag 128 (mode 103) |
| Darkside 121, Dungeon of Death | @175, @182 | DisplayLarge (50), lines 5 and 189 |

The endings (opcodes 57 and 58) stay for the next milestone.

## Key decisions

- **DOS check first** (Part 0, calibrated static analysis of
  `WORLD/XEEN.DAT`; [evidence](rules/evidence.md#last-mixed-map-events-m105-part-0)).
- **World flags beyond 127.** Mode 103 tests, takes and gives one bit array
  with no side or bound, so flags 128-187 are the quest-flag bits; Darkstone
  Tower's fourth head sets Clouds quest flag 0, the bit Auto Note 128 already
  read. Flags 188-255 have no original use and refuse
  ([quirk](original-quirks.md#sv-world-flag-quest-alias); the pin keeps an
  unsaved extra flag).
- **DisplayLarge** shows line N of the map's `aaze2NNN.txt` through
  DisplayMain's window-11 path, written as is with a bare clear between pages.
  Over an open window 11 it continues at the previous text's cursor, so the
  Dungeon of Death's book (lines 5 and 189) reads as one text
  ([quirk](original-quirks.md#dos-display-large-continues)). A line beyond the
  file and a DisplayLarge after another window's text have no original use and
  refuse.
- **No new owner and no saved state:** the existing Event value, text loader,
  interpreter and presenter owners carry both rules.

## Results

- The census lists no partial or unsupported Event on maps 113-128; Darkside
  passes 21193 of 21195 Event records, the two left being the endings.
- Original-data tests run the four heads and the book with live input. The
  book's 35 pages match an independent DOS-writer oracle pixel for pixel, with
  Text on and off; reloads are exact.
- M44 digests unchanged. Complete suite 394/394.
- The maintainer play-test passed every item: the book and the fourth head
  and its Auto Note, each compared with DOSBox from the presets `m105-book`
  and `m105-head` and exported DOS saves.
