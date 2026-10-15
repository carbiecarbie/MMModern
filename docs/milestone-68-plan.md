# Milestone 68 - Monster abilities

**Tier A, in two parts. Status: completed and accepted.** Plan `5460a53`;
Part A `ff38fb2`; Part B `d32f9ce`. Independent plan and implementation
reviews passed; the maintainer accepted both play-tests (2026-10-15).

## Goal and final scope

Step 2 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems):
every monster attack, secondary effect and movement kind the Clouds and
Darkside data use works as in the original. No map was admitted; the monster
status system (sleeping, paralyzed or controlled monsters) stays with the
spells step.

- **Part A - attacks and consequences.** Monster damage follows the DOS
  sequence (`WORLD/XEEN.DAT` `0x2d22d`): a nonphysical attack's initial save
  uses the Physical selector, and the element-save loop draws once on entry
  even at zero damage. Every damage type and special is supported: special 11
  draws the save and does nothing; Eradicate and Curse Item follow the DOS
  frames, leaving weapon ID 34 untouched; Break Weapon unequips; Aging wraps
  its byte. Ranged attacks of every element work in combat and exploration
  with the shared volley projectile. Combat, exploration and Rest publish every
  changed character field. A developer `--monster-fixture` entry places a
  chosen original species on F3 through the normal validation, with saving
  disabled.
- **Part B - movement.** The original `canMonsterMove` (`0x1fc91..0x1fda1`):
  outdoors, Water and Deep Water admit flyers and species 59, Space admits
  flyers, and other surfaces follow the Clouds or Darkside rule. Approach
  order, occupancy and contact stay shared. The fixture gains `--water-edge`
  to watch a Giant Bat cross water.

## Decisions

1. **DOS over the reference**, from the plan review and the implementation:
   the damage save sequence (also correcting the previous Poison rule), the
   special 11 no-op, Eradicate and Curse Item frames, and two movement
   differences the maintainer approved: middles {1,7,9,10,12} always refuse,
   and the wall threshold is strict. All are recorded in the combat rules and
   [original quirks](original-quirks.md).
2. **M44:** the Poison correction changed the services and temple digests,
   replaced with the maintainer's approval; mainland unchanged; Part B had
   zero differences.
3. **Species 59** was refused by Part A's fixture and census until Part B made
   its movement real, after the implementation review found the mismatch.

## Results

- Census: no monster gap left on either side except the empty `dark.mon 0`
  record.
- Complete `build-rel` suite 213/213 after each part.
- Two maintenance fixes from the Part B play-test were committed separately:
  Shoot with nobody able to shoot is silent (`ca29b29`), and the death final
  scene is built over the completed fire (`65f283d`).

## Deferred

The monster status system (spells step).
