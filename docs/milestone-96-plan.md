# Milestone 96 plan - Opened chests stay open

**Tier A. Status: completed and accepted** by the maintainer on 2026-10-29
after the play-test. Plan `ae1373a`, Part A `8b4e9f4` and the closure commit.
Found by roadmap
[step 2](roadmap.md#next-arc-release-quality-and-the-darkside-start), the DOS
save investigation, which paused for it. Save v22; Journey schema 9 and
content 14 unchanged.

## Goal

As in DOS, a locked chest whose lock is picked becomes an open chest for
good: when the Event run ends, after its treasure, the chest takes its map's
open chest picture, is drawn open indoors and outdoors, and stays open
through MoveObj, Remove, leaving the map, Save and Load. While the lock is
being picked, the lid follows the DOS animation. 37 Clouds chests on 18 maps
can be picked open; 12 trap chests never are. Before M96 a picked chest
looked open only until the party left the map, and saves could not hold the
change.

## Scope

- **Excluded:** the DOS save converter and the rest of the investigation;
  GiveMulti mode 67, which no script uses; Darkside play; ExchObj, still not
  supported; map 12's object 0 animating from sprite 85 in DOS, and the pin
  skipping Ali Baba's chest on Darkside, noted by the plan review for later.
- **Evidence:** calibrated DOS static analysis of the end-of-run rule
  (`0x28d92..0x28e87`), its counter (`0x9755`), the map writer and loader,
  the drawing and the animation update (`0x10c32..0x10dc7`), confirmed by a
  real DOS save (slot 01, map 9's chest at (15,11) went from index 3 to 4)
  and by the pinned ScummVM. Notes and scripts are in
  `D:/Projetos/MModern/MModern-re/m96/`. The
  [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws)
  hold the rule, its frames and its offsets.

## Key decisions

- **Rule in the run's tail.** The Event interpreter applies it once per run,
  after the treasure delivery and before "Nothing here", on the detached
  candidate; a refused or abandoned run publishes nothing. The
  TeleportAndContinue restart zeroes the counter, as in DOS.
- **Selection.** DOS rebuilds the selection at every draw3d: the
  lowest-index object on the party's cell. MMModern selects once per run by
  the same rule, so while a picked lock is pending, a run that reaches any
  other teleport, a map change, a Town service, a Remove of the selection or
  any MoveObj is refused as not supported yet. No Clouds pick run reaches
  one: a census of all 41 runs that can pick a lock found none. MMModern
  still skips objects without a picture when it selects
  ([current discrepancy](original-quirks.md#sv-pictureless-selection)),
  which no lock reaches.
- **The index belongs to the record.** An object override gains the map's
  picture-table index; MoveObj and Remove keep it, and the picture, frames
  and flip are drawn from it, as in DOS. The giveExt frame follows the DOS
  update: the lid cycles 1..4 while the lock is pending, then the open
  sprite's start frame. Every sprite that can hold such a frame is static in
  each view.
- **Save v22.** Each object entry adds its index (six bytes). Restore accepts
  only the record's own index or its chest's open slot, and refuses anything
  else without partial publication; v21 saves are refused. An audit-only
  build writing v21 reproduced the four M44 digests exactly; the maintainer
  approved the v22 digests on 2026-10-29.

## Results

- **Part A** (`8b4e9f4`): the rule, refusals, object state, drawing, frames,
  save v22, the `m96-chest` preset, corrected chest quirks and rules, and
  original-data tests on maps 9, 44 and 70 plus a real-input route.
- **Independent implementation review:** the first pass asked for rework
  (the selection could change on a later draw, and the rendered lid frames
  were identical reconstructions). Fix round 1 settled the DOS selection,
  widened the MoveObj refusal, fixed the lid renders and proved the static
  sprites. The completion pass approved with changes: this plan's refusal
  text, the picture-less selection entry and a test for an index on an empty
  slot, all applied.
- The complete suite passed 363/363 (run `20261029-023643-b54926fe`);
  later changes touched only tests and docs.

## Acceptance

The maintainer play-tested the main result and accepted the milestone: pick
the lock on map 9 and watch the lid, leave and return, save and load; the
chest stays open. The frames rest on DOS code, so there was no DOSBox item.
