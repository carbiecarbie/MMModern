# Milestone 98 plan - Cartographer marks every arrival

**Tier A. Status: completed and accepted** by the maintainer on 2026-10-30
after the play-test. Plan `44d1127`, Part 0 `ac1310a`, Part A `55c9944` and
the closure commit. Found by the M97 play-test; follows roadmap
[step 2](roadmap.md#next-arc-release-quality-and-the-darkside-start). Save
v22, Journey schema 9 and content 14 unchanged.

## Goal

As in DOS, whenever the scene is drawn with a pending step, a party with a
Cartographer marks its own cell on the automap and the Map (M) screen.
Before M98, MMModern marked a cell only when certain steps were published,
so the cell stayed blank after New Game, Load, leaving the Inn, a same-map
Event teleport, the stops of an Event Continue chain and an Event prompt
before a teleport, and probably after travel cast from an item.

## Evidence

- **DOS static, high confidence:** the scene's lighting routine
  `0x12012..0x12093` runs only while stepped (`DS:3067`) is set; it drains
  light on a drain cell, checks skill 4 (Cartographer) through `0x215b9`,
  sets the cell's bit in the map's stepped bitmap, then derives darkness.
  Map load sets stepped (`0xf58b`), the redraw clears it (`0x1184f`), and
  every combat exit and town service exit sets it again. Part 0 traced every
  writer, the Inn order and each Event wait:
  [stepped bitmap writers](rules/evidence.md#stepped-bitmap-writers),
  [Event waits and pending steps](rules/evidence.md#event-waits-and-pending-steps)
  and the quirk [pending Cartographer step](original-quirks.md#dos-pending-cartographer-step).
- **DOS play** (M97): a Cartographer party loaded on a blank cell and
  re-saved without moving; only that cell's stepped bit changed
  ([DOS saves](rules/dos-saves.md)).

## Key decisions

1. **One rule, at the scene check.** With a pending step the check drains,
   marks the party's cell when an active member has Cartographer, then
   derives darkness, which never suppresses the mark. Every guarded site
   publishes the exploration even when the light word is unchanged; step
   sites no longer write exploration themselves.
2. **Stepped stands on its own.** Every site that set stepped keeps setting
   it; every combat exit sets it after its redraw and delivery, and Smith and
   Training departures set it, as town service exits do in DOS. The Reaper,
   Golem, Dwarf and Sphinx entrances keep both DOS checks.
3. **Event waits follow DOS.** A wait that draws the scene consumes the step
   on the Event's detached candidate, published when the Event commits and
   discarded if it is refused. A wait behind window 11 (an NPC with Text on),
   location art, a service lobby, an animation-only wait or early-key text
   input keeps the step. A scene-drawing prompt before a teleport marks both
   cells; a silent teleport marks only the destination. The Event guards
   accept exactly the consumed step, the exploration and the drained light
   word. Scene draws in the Part 0 table that had no check gained one at the
   DOS point, without moving any RNG draw.
4. **NPC mode 2 art lasts the Event run**, as `DS:8d56` does in DOS, so a
   later prompt animates over it and keeps the step
   ([quirk](original-quirks.md#sv-npc-mode2-art-lifetime); the pin differs).
5. **Load and New Game arrive like a map load.** Restore stays exact before
   the first frame; `XeenGameplay` then sets the pending step once, for title
   Load, panel Load, New Game and Darkside startup, but not inside the Inn,
   whose exit reloads the map. This replaced the earlier restore-boundary
   policy in [magic](rules/magic.md#darkness-and-light-timing) and
   [time and conditions](rules/time-and-conditions.md#stepped-and-lighting).
6. **Save unchanged.** The explored overlay was already saved and its
   meaning did not change.

## Results

- **Part 0** (`ac1310a`): the DOS evidence above, recorded before code.
- **Part A** (`55c9944`): the rule, the guarded sites, the combat and service
  exits, the Load and New Game arrival, the Event waits and the NPC mode 2
  art, with unit and original-data tests for each path, refusals per site
  family and restore exactness before the first frame.
- **Reviews:** the independent plan review's findings were applied. The
  independent implementation review found that a drain could escape through
  a partial Event publication; a transient projection of the publishable
  light fixed it, and the second pass found nothing new.
- The four M44 digests stayed identical (zero differences in traces,
  checkpoints, fields and maps); none was replaced.
- The complete suite passed 370/370 on the final state (run
  `20261030-033924-b44b411d`).

## Acceptance

The maintainer played three items and accepted the milestone: New Game marks
Vertigo's start cell, Load marks the blank Area C1 cell `(14,11)` with a
Cartographer, and leaving the Inn after adding Seymour marks the door cell.
