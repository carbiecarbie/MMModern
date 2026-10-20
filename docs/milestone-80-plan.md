# Milestone 80 plan - Entrance scenes and the Arena

**Tier B; the Arena save question can raise one part to Tier A (see Part
B). Status: planned.** Step 10 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M79](milestone-79-plan.md). Baseline `93398ce`.

## Goal

The Clouds entrance scenes and the Arena work as in the original, reached
from the original `DoTownEvent` records through the existing terminal
Event-to-Service handoff. Each scene checks its quest item, plays its
original animation and, with the key, loads the map it guards. The
reference's Clouds branch (`locations.cpp`, `getNewLocation`) gives these
sites; Part 0 confirms them in DOS:

| Scene (action) | Site map: quest item -> destination |
| --- | --- |
| Reaper (8) | 12: item 3 -> 55 `(3,8)` E; 15: item 2 -> 59 `(11,8)` W; 24: item 1 -> 51 `(7,12)` S; 7: item 46, 13: item 45 -> Darkside |
| Golem (9) | 8: item 6 -> 81 `(1,17)` E; 12: item 5 -> 80 `(29,16)` W; 20: item 7 -> 79 `(5,16)` E; 19: item 50 -> Darkside |
| Dwarf Mine (10) | no item: 14 -> 37 `(1,4)` E; 18 -> 35 `(1,12)` E at x=9, else 36 `(7,1)` N; 23 -> 33 `(7,1)` N at x=5, else 34 `(7,30)` S |
| Sphinx (11) | 5: item 4 -> 82 `(7,5)` N; 2: item 51 -> Darkside |
| Arena (6) | 85 The Warzone: Battle Master dialog, level and count prompts, generated monsters, Warzone award; exits to 28 or 6 `(12,4)` W |

Without the key a scene plays its refusal and reloads the current map with
the party where it was, rebuilding its monsters and setting stepped, as DOS
does; no scene charges service time. A site whose key leads to Darkside
always plays the keyless refusal. Those destinations are Darkside-archive maps (113 Darkstone Tower, 117
Dragon Tower, 121 Dungeon of Death, 125 Southern Sphinx), and their keys
(quest items 45, 46, 50, 51) are granted only by Darkside Events, so Clouds
play always reaches the keyless refusal. Clouds keys 1-7 are granted by
Clouds Events.
The Pyramid (action 12) and the Darkside-only action 13 keep their refusal.
Audio stays out (step 12): speech keeps its original duration silently, with
the existing speech notice, as the dream and NPC speech do.

**Save:** the save stays v16 (schema 9, content 14). The scenes add no
durable value. DOS never saves Clouds map 85 and empties its monster table
on every load, so a generated fight lives only while the party is there;
saving is refused there (flag `0x8000`). MMModern keeps no per-map state
for map 85: capture never writes it, and restore accepts and drops a map-85
entry from an older v16 save, which no play can observe because the map
resets on load. **M44:** digests are expected unchanged; any change stops
the work for the maintainer.

## Maintainer decisions (2026-10-20)

- Map 85's per-map state is dropped as above; the save stays v16 without
  an independent review, and the Arena stays Tier B.
- Darkside keys count as never held: the party's saved quest counters
  cover the Clouds items 0-34 only, and Clouds Events never grant items
  45, 46, 50 or 51, so those four sites always play the keyless refusal.
  The keyed outcome (the World CD loads the Darkside map) waits for the
  Darkside arc; no preset or save change carries those keys (2026-10-20,
  after Part A stopped on the save rule).
- Scenes show no captions, as the installed DOS default (Effects on) does;
  the Effects-off captions wait for step 12 (audio) and its Efx button.
- Loops without an explicit wait (Sphinx zoom, speech and retreat; the
  Dwarf zooms; the Reaper and Golem retreats) run at the DOSBox-observed
  two retraces per iteration (about 35 Hz); explicit BIOS-tick waits keep
  their DOS values.
- No "Please Wait..." box on map loads
  ([approved deviation](original-quirks.md#dev-no-please-wait)).

## Tasks

- [x] **Part 0 - DOS check** ([evidence](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)).
      Site table confirmed; keyless refusal reloads the map and sets
      stepped; scene frames draw from the gameplay random state, with
      loop counts tied to speech; the Arena draws five species first and
      formats 21st/22nd/23rd (both unlike the reference); map 85 is never
      saved and resets on load.
- [x] **Part 0b - scene text and cadence**, with the maintainer's DOSBox
      captures of the Dwarf Mine and the Sphinx refusal. The installed
      default shows no captions; Speech and Text do not gate them; loops
      without an explicit wait wait one retrace, observed as two; the
      Reaper retreats on refusal and the Sphinx refusal draws indices 0..8
      (both unlike the reference).
- [ ] **Part A - entrance scenes.** One scene presenter driven by a
      per-scene data table (resources, frame tables, speech, destination
      rows from Part 0), not four copies; quest-item check and destination
      load through the existing map-load and travel owners; random draws
      and timing per Part 0; Darkside rows always resolve to the keyless
      refusal.
- [ ] **Part B - Arena.** Battle Master dialogs, numeric prompts, the DOS
      generation order, award and ordinal, and exits on the existing
      dialog, Event text, actor and award owners; map 85 empties its
      monster table on load and keeps no per-map state, as above. If this
      needs a new save layout or meaning, stop for the maintainer.
- [ ] Developer test save `m80-playtest`: the Clouds quest items 1-7, one
      member without them reachable by Exchange, and Town Portal or Lloyd's
      Beacon to reach each site; README entry.
- [ ] Regenerate the census; update its calibration if needed.

## Acceptance

- Unit tests: the site table (with and without key, Dwarf positions,
  Darkside sites), the Arena generation from a fixed random state, award
  counting, ordinals and the maxed exit, and a v16 save carrying map-85
  state restoring without it.
- Original-data tests: one keyed scene loading its destination, one
  keyless refusal reloading the map with the party in place, one Dwarf
  Mine branch, an Arena fight from prompt to award, and leaving and
  re-entering map 85 finding it empty.
- Actions 12 and 13 still refuse without changing state, time or random
  draws; each Darkside row resolves to the keyless refusal.
- The census lists every Clouds action 6 and 8-11 site as supported; the
  keyed Darkside outcome is classified as outside the Clouds quest items. One complete suite per `tools/AGENTS.md`;
  M44 digests unchanged.
- Maintainer play-test with `m80-playtest`: each scene with and without its
  key, both Dwarf branches, and an Arena fight won, declined and repeated,
  by mouse and keyboard.
