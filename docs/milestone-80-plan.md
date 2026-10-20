# Milestone 80 plan - Entrance scenes and the Arena

**Tier B. Status: completed and accepted by the maintainer (2026-10-20)**
after maintainer play-tests of every part. Plan `a7b9bc3`, DOS checks
`3d6b6ee` (Part 0) and `b09dec7` (Part 0b, with the maintainer's DOSBox
captures), plan amendment `4126973`, entrance scenes `81bc644` (Part A),
Arena `041bc22` (Part B). Step 10 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M79](milestone-79-plan.md).

## Scope

The Clouds entrance scenes (`DoTownEvent` 8-11: Reaper, Golem, Dwarf Mine,
Sphinx) at every Clouds site, with their quest-item checks and the maps
they guard, and the Warzone Arena (action 6, map 85). The Pyramid (12),
action 13, audio and the Effects-off scene captions stayed out.

| Scene (action) | Site map: quest item -> destination |
| --- | --- |
| Reaper (8) | 12: item 3 -> 55 `(3,8)` E; 15: item 2 -> 59 `(11,8)` W; 24: item 1 -> 51 `(7,12)` S; 7: item 46, 13: item 45 -> Darkside |
| Golem (9) | 8: item 6 -> 81 `(1,17)` E; 12: item 5 -> 80 `(29,16)` W; 20: item 7 -> 79 `(5,16)` E; 19: item 50 -> Darkside |
| Dwarf Mine (10) | no item: 14 -> 37 `(1,4)` E; 18 -> 35 `(1,12)` E at x=9, else 36 `(7,1)` N; 23 -> 33 `(7,1)` N at x=5, else 34 `(7,30)` S |
| Sphinx (11) | 5: item 4 -> 82 `(7,5)` N; 2: item 51 -> Darkside |
| Arena (6) | 85 The Warzone: Battle Master; exits to 28, 6 `(12,4)` W or map 85's run position |

## Key decisions

- **DOS checks first** ([evidence](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)):
  the site table; a keyless refusal reloads the current map (rebuilding
  its monsters) and charges no service time; scene frames draw from the
  gameplay random state. The DOS Arena chooses five species before placing
  monsters and formats 21st/22nd/23rd, the Reaper retreats on refusal and
  the Sphinx refusal draws indices 0..8, all unlike the reference.
- **Darkside keys are never held** in Clouds: the party saves only the
  Clouds quest counters 0-34 and only Darkside Events grant items 45, 46,
  50 and 51, so those four sites always play the keyless refusal. The
  keyed outcome waits for the Darkside arc.
- **Presentation from the DOSBox captures:** no captions (the installed
  default, Effects on); loops without an explicit wait run at two
  retraces per iteration (about 35 Hz); speech keeps each sample's
  duration in silence. `sphinx13.voc` has a damaged descriptor that DOS
  ignores, so the VOC duration reader checks only the structure.
- **No "Please Wait..." box** on map loads
  ([approved deviation](original-quirks.md#dev-no-please-wait)).
- **Battle Master dialogs** follow the reference's modes, which the
  maintainer confirmed in DOS: the offer shows the original Yes/No
  buttons; every other message waits for acknowledgement.
- **Save v16 unchanged** (schema 9, content 14). Map 85 was already never
  saved and its restore rejected; every Warzone load now empties its
  monster table, so leaving discards a generated fight.

## Results

- Census: every Clouds action 6 and 8-11 site supported; the keyed
  Darkside outcomes are classified as outside the Clouds quest items.
- M44 digests, traces and save bytes unchanged in every part. Complete
  suite 254/254 after Part A and 258/258 after Part B.
- Two stop rules fired cheaply and were resolved by the maintainer: the
  Darkside-key preset (save rule) and the Warzone preset camera (the
  presets start at the map 6 entrance instead).
- The maintainer play-tested every scene with and without its key, both
  Dwarf branches and the Arena (won, declined, maximum) with the
  `m80-playtest`, `m80-arena` and `m80-arena-maxed` presets; the play-test
  added the offer's Yes/No buttons.
