# Milestone 62 - Faithful screens and animation

**Tier B, in three parts. Status: completed and accepted.** Plan `d332406`;
Part A `5cc0079`; Part B `41237e1`; Part C `27f344a`. The maintainer accepted
the play-tests (2026-10-13). The approved no-phantom-steps deviation and the
M63 roadmap merge (`22e99af`) landed during this milestone.

## Goal and final scope

Replace the project-made stopgaps a player meets in Vertigo and map 23 with
the original presentation ([roadmap](roadmap.md) step 8). Authority: original
program analysis of `WORLD/XEEN.DAT` (REA findings under
`D:/MModern-re/ui-presentation-20261013/`, outside the repository), the CD
resources, maintainer DOSBox evidence and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Part A - everyday screens.** The developer overlay and log-style combat,
  Shoot and treasure narration are gone from normal play. Event text, Yes/No
  prompts and NPC conversations use the original windows, sprites, templates
  and page continuation; WhoWill shows the full template, also at doors and
  grates, and keeps the DOS incapacitation refusal; the combat monster panel
  uses window 2 with the DOS health colors (white, yellow at a quarter or more
  of maximum HP, red below). Escape and right click go to the active dialog
  instead of ending the session; "Spell Points" wording.
- **Part B - treasure.** Event and monster treasure use window 10 "The Party
  Found" with per-item recipient lines, the quest-item star and the final
  "Hit a key"; the full-pack warning is its own modal window; currency is
  credited after the final acknowledgment. Rest screens were confirmed
  original. Party-death frames are composed and tested for M63.
- **Part C - animation.** The six shopkeepers play their DOS base cycles; their
  speaking frames are composed but inactive. Scenery animations include their
  end frame. The title's first phase and the credits pages follow DOS. The
  Smith and Temple art no longer leaves one column of the world visible.

## Decisions

1. **Tier B with an explicit M63 boundary:** no timing, scheduling, RNG,
   publication or save changes. Left for M63: treasure item holds and per-item
   publication (including quest grants in the treasure sequence), shop speaking
   gates, the exact shop and title cadence, dream timing and the party-death
   playback that replaces the DEFEAT screen.
2. **Combat row hitboxes** were confirmed from the maintainer's DOSBox capture
   and the original button tables; they were already correct. Health colors are
   also confirmed by the maintainer's long play.
3. Save v12 and all M44 digests unchanged; the mainland trace gained six
   identical presentation observations from the treasure pages (approved).

## Results

- Tests for DAT text and control streams, windows, sprites and colors, page
  continuation, mouse/keyboard/right-click/modifier handling, treasure pages,
  shop cycles and overlay composition, inclusive end frames, title and credits.
- Full CTest 188-189/189 on `build-rel` after each part; Debug closure check
  passed; maintainer play-tests of all three parts accepted.

## Deferred

Everything listed for M63 above; audio.
