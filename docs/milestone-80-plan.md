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

Without the key a scene plays its refusal and returns the party where it
was. A site whose key leads to Darkside plays the refusal without the key
and, with the key, keeps a "not supported yet" refusal before any effect.
Those destinations are Darkside-archive maps (113 Darkstone Tower, 117
Dragon Tower, 121 Dungeon of Death, 125 Southern Sphinx), and their keys
(quest items 45, 46, 50, 51) are granted only by Darkside Events, so Clouds
play always reaches the keyless refusal. Clouds keys 1-7 are granted by
Clouds Events.
The Pyramid (action 12) and the Darkside-only action 13 keep their refusal.
Audio stays out (step 12): speech keeps its original duration silently, with
the existing speech notice, as the dream and NPC speech do.

**Save:** the scenes add no durable value; the save stays v16 (schema 9,
content 14). The Arena replaces map 85's monster table with generated
monsters (the reference clears the MOB records). Map 85 forbids saving
(flag `0x8000`), but its per-map state outlives the visit. Whether the
existing per-map monster table (MOB records followed by script-created
slots) holds that table without a new layout or meaning is Part B's first
question. **M44:** digests are expected unchanged; any change stops the work
for the maintainer.

## Tasks

- [ ] **Part 0 - DOS check (read-only, delegated, calibrated against a
      settled `DoTownEvent` service routine, cited offsets).** (1) Each
      scene's site/quest-item/destination table, the Dwarf position tests,
      and which Clouds sites lead to Darkside. (2) Without the key, and
      at a Darkside site with the key: whether DOS reloads the current map
      (rebuilding its monsters), steps or passes time. (3) Presentation:
      sprite resources, frame order and pacing, whether animation frames
      draw from the gameplay random state and how many draws a scene makes,
      and whether the CD shows subtitle text (source and timing) or only
      speech. (4) Arena: text sources, prompt limits, monster generation
      (species formula, positions, id, frame, wake state) and its random
      draw order, the award index and ordinal suffix, the victory, decline
      and maxed exits and whose run position applies, and whether the
      generated table survives leaving and re-entering map 85 (and whether
      the award can then repeat). Record results in `docs/rules/` and
      `docs/original-quirks.md`.
- [ ] **Part A - entrance scenes.** One scene presenter driven by a
      per-scene data table (resources, frame tables, speech, destination
      rows from Part 0), not four copies; quest-item check and destination
      load through the existing map-load and travel owners; random draws
      and timing per Part 0; Darkside-with-key refusal before any effect.
- [ ] **Part B - Arena.** First, without code, establish whether map 85's
      generated table fits the v16 per-map state with unchanged layout and
      meaning. If not, stop: the Arena becomes a Tier A part with its own
      design note, independent plan and implementation reviews, and a save
      version increment, decided by the maintainer. Otherwise implement the
      Battle Master dialogs, numeric prompts, generation, award and exits on
      the existing dialog, Event text, actor and award owners.
- [ ] Developer test save `m80-playtest`: the Clouds quest items 1-7, one
      member without them reachable by Exchange, and Town Portal or Lloyd's
      Beacon to reach each site; README entry.
- [ ] Regenerate the census; update its calibration if needed.

## Acceptance

- Unit tests: the site table (with and without key, Dwarf positions,
  Darkside sites), the Arena generation from a fixed random state, award
  counting and the maxed exit.
- Original-data tests: one keyed scene loading its destination, one
  keyless refusal leaving the party in place, one Dwarf Mine branch, and an
  Arena fight from prompt to award.
- Actions 12 and 13, and Darkside sites with the key, still refuse without
  changing state, time or random draws.
- The census lists every Clouds action 6 and 8-11 site as supported, except
  the Darkside-with-key outcome. One complete suite per `tools/AGENTS.md`;
  M44 digests unchanged.
- Maintainer play-test with `m80-playtest`: each scene with and without its
  key, both Dwarf branches, and an Arena fight won, declined and repeated,
  by mouse and keyboard.
