# Milestone 98 plan - Cartographer marks every arrival

**Tier A. Status: planned.** Found by the M97 play-test; the maintainer chose
a Tier A milestone that includes Load on 2026-10-30. Save v22; Journey
schema 9 and content 14 unchanged.

## Goal

As in DOS, whenever the scene is redrawn with a pending step, a party with a
Cartographer marks its own cell on the automap and the Map (M) screen. Today
MMModern marks a cell only when certain steps are published, so the cell
stays blank after New Game, Load, leaving the Inn, an Event teleport within
the same map, the stops of an Event Continue chain, an Event prompt before a
teleport, and probably travel cast from an item. After M98 one rule marks
every one of these exactly as DOS does.

## Evidence

- **DOS static, high confidence** (Capstone, 2026-10-30, rechecked by the
  plan review): the scene's lighting routine `0x12012..0x12093` runs only
  while stepped (`DS:3067`) is set. It looks up the cell, drains light on a
  drain cell, calls the party skill check `0x215b9` with skill 4
  (Cartographer), and when that passes sets bit `(y&15)*16+(x&15)` of the
  current map record (`[0xf494]*0x37c+0xc128`) through `1b98:3f73`; then it
  derives darkness. Scene setup calls it (`0x12190`, `0x1a81a`); the redraw
  `0x9cc:0x1843` clears stepped afterwards (`0x1184f`). Map load sets it
  (`0xf58b`); startup loads the map (`0x0d08c`) before its redraw
  (`0x0d0da`). The Event-wait draw (`0x24bf1`) calls the same redraw, so a
  prompt consumes a pending step. Every combat exit shares the tail
  `0x2eb6b..0x2eb8f`: redraw, treasure delivery, then stepped set again for
  the next redraw. The other stepped sources are in
  [travel lighting checks](rules/evidence.md#travel-lighting-checks).
- **DOS play** (M97, 2026-10-29): a Cartographer party loaded at Clouds map 9
  `(14,11)` on a blank cell and re-saved without moving; only that cell's
  stepped bit changed ([DOS saves](rules/dos-saves.md)).
- **Reference:** the pin agrees (`Map::load` and the combat exit set
  `_stepped`, `cmdDoTownEvent` sets it after every town service,
  `Party::handleLight` marks the cell, `draw3d` clears it).

## Today's code

- `XeenWorld::explorationAfterStep` (`XeenWorld.cpp`) is the only writer,
  called from ten flow files when a step, rest, service departure or combat
  exit is published. Several sites set stepped only inside that write
  (`XeenActorApproach.cpp:703`, `XeenEventFlow.cpp:2043/2081`,
  `XeenJourneyFlow.cpp:209-218`); combat exit sets stepped only after a
  disengaged exit, Run or Mr Wizard (`XeenJourneyFlow.cpp:1139-1144`); Smith
  and Training departures set none.
- Stepped is `XeenScenePresentation::markLightingStep`. The DOS lighting
  routine's counterpart is `XeenWorld::lightAfterScenePreparation`, called
  at exactly seven scene checks in `XeenEventFlow.cpp` (four, one of them
  the Event-wait draw at `:1541-1556`, which never publishes),
  `XeenJourneyFlow.cpp` (publishes only when the light word changes),
  `XeenCombat.cpp` and `XeenEventInterpreter.cpp` (Continue).
- Restore is exact and `XeenWorld::swapPreparedState` clears stepped; New
  Game sets none. This was an MMModern policy
  ([restore boundary](rules/evidence.md#restore-boundary)), not DOS evidence.
  Load and New Game already draw the arrival frame before the M84 entry pass
  (`XeenGameplay.cpp:326,656,687`).

## Key decisions

1. **One rule, at the scene check.** With a pending step the check drains,
   then marks the party's cell when an active member has Cartographer
   (`xeenPartyHasSkill`, dead members counting as today), then derives
   darkness, which never suppresses the mark. Every one of the seven sites
   publishes the exploration through its guard, independently of whether the
   light word changed; the ordinary refresh, which has no guard and runs only
   with an encounter context, records through `XeenWorld::recordExploration`
   regardless of that context.
2. **Stepped stands on its own.** Every site that sets stepped today keeps
   setting it unconditionally, not only when it has exploration to publish.
   Every combat exit sets stepped after its exit redraw and delivery, as the
   shared DOS tail does; every town service exit sets it, as the pin's
   `cmdDoTownEvent` does (Part 0 confirms the DOS site). `evidence.md`'s Run
   row is corrected: the combat-exit mark belongs to every exit.
3. **No second writer.** With decision 2 in place, the step-site calls to
   `explorationAfterStep` go; the occupied-cell helper becomes the check's
   implementation. If a path that DOS redraws with stepped set has no scene
   check in MMModern, or MMModern runs an Event or a save between a step and
   its check where DOS does not, stop and report: that is a separate
   presentation-order gap.
4. **An Event-wait draw consumes the step only when DOS prepares the
   scene.** DOS `draw3d` (`0x10bc3`) returns at once (`0x10bd0..0x10bd7` to
   `0x1187d`) while window 11 is open (flag `DS:9877`, set by the opener
   `0xdddb`; an NPC with Text on opens it at `0x257bb..0x257c3`): no scene
   preparation, no lighting check, and stepped stays pending. The pin's
   `draw3d` returns the same way while `SCENE_WINDOW` is open. Every Event
   wait follows the Part 0 table in
   [Event waits and pending steps](rules/evidence.md#event-waits-and-pending-steps):
   a **scene draw** performs the check on the Event's detached candidate and
   clears the candidate's pending step, and the result is published when
   the Event commits and discarded if it is refused. A wait that **keeps**
   the step (NPC with Text on, location art, service lobby, animation-only
   waits, early-key text input) adds no check, and the next real scene
   preparation marks the cell the party is on then. A scene-drawing prompt
   followed by TeleportAndExit therefore marks both cells, while a silent
   teleport, or one behind an NPC with Text on, marks only the destination.
   This reverses the
   rule at `XeenEventFlow.cpp:1550-1551` that an Event-wait draw never
   alters the retained Event party: the candidate guard, `work.guard` and
   the retained-continuation check must accept the consumed step and the
   exploration, and nothing else. A reconstruct draw or a second Event-wait
   draw changes nothing.
5. **Event Continue.** Each Continue redraw that checks light also marks its
   intermediate cell on the detached candidate, as `0x27121` does.
6. **Load and New Game arrive like a map load.** Restore stays exact: every
   saved byte comes back unchanged and nothing is replayed. `XeenGameplay`
   then sets the pending step once, before the first frame, for title Load,
   panel Load, New Game and Darkside startup, and skips it inside the Inn
   (leaving the Inn already reloads the map and sets it). That frame drains a
   drain cell and marks the cell. This replaces the restore-boundary policy
   and the matching wording in
   [magic](rules/magic.md#darkness-and-light-timing) and
   [time and conditions](rules/time-and-conditions.md#stepped-and-lighting).
7. **Save unchanged.** The explored overlay is already saved and its meaning
   does not change: v22, schema 9 and content 14 stay. M44 digests may change
   where a witness party has a Cartographer (the initial party's Sorcerer
   Seymour does); each change gets a counterfactual audit (disable the new
   marks, reproduce the old digest) and the maintainer's approval before the
   complete suite.
8. **M97 exporter unchanged.** `dos-saves.md` replaces "the DOS routine that
   marks the cell on load was not traced" with the routine above.

## Parts

- **Part 0 (read-only, before code):** confirm statically (a) that panel
  Load and New Game reach the same map load and redraw as startup; (b) that
  `1b98:3f73` has no other caller writing the stepped bitmap in Clouds play
  (the map-50 fill is Darkside only); (c) the DOS order of the step onto an
  Inn door, the Sign In and its save, so the Sign In save either holds the
  door-cell mark or not as DOS does; (d) which Event-wait draws reach scene
  preparation and consume stepped (`0x24bf1`) and which keep it (window 11
  open, animation-only waits), for every Event presentation; (e) the DOS site
  that sets stepped after a town service exit. Record results in
  `rules/evidence.md`. Stop if any differs from decisions 2, 4 or 6.
- **Part A:** the rule, the seven sites, unconditional stepped marks, combat
  and service exits, Load/New Game arrival, removal of the step-site writes,
  tests, docs, presets.
- **Part B:** closure.

## Tests

- Unit: the check marks only with a pending step, an active Cartographer and
  a physical cell; dark maps still mark; a second check is read-only; a
  refused candidate publishes nothing, for each site family; the ordinary
  refresh marks without an encounter context; the journey check publishes
  exploration when the light word is unchanged; a synthetic drain cell
  drains once on the first frame after Load.
- Restore exactness is asserted before the first frame. The assertions in
  `XeenTraversalLightingTests` (around lines 54 and 60) and any round-trip
  test that encodes after a frame are replaced by the arrival rule, not
  weakened.
- Original data: New Game marks Vertigo's start cell; Load on a blank cell
  with a Cartographer marks it on the first frame, without one it stays
  blank; leaving the Inn with a newly added Cartographer marks the door
  cell; the Sign In save matches Part 0 (c); a scene-preparing prompt
  followed by a teleport marks both cells, while a silent teleport and an
  NPC with Text on before a teleport mark only the destination; the same NPC
  with Text off marks both; a Yes/No prompt over location art keeps the
  step; a same-map
  teleport and a Continue chain mark their cells (the implementer picks
  Clouds sites from the Event census); learned and item Town Portal and
  Lloyd's Beacon mark the destination; every combat exit and a Smith and a
  Training departure leave stepped set for the next redraw; walking, map
  edges, falls and Rest behave as before.
- The generic save/load round-trip test stays green.

## Risks

- Seven guarded sites, including combat and the detached Event candidate: a
  missed guard could publish exploration from a refused run; the per-family
  refusal tests cover this.
- Decision 4 loosens the Event-wait draw's no-mutation guards; widening
  them beyond the step and the exploration would let a draw alter Event
  state. A test checks that a reconstruct draw and a second Event-wait draw
  change nothing.
- Moving the write from the step to the redraw changes when tiles appear in
  memory; tests that read tiles between the two must read them after the
  frame, and decision 3's stop rule guards real order differences.
- Digest replacements need the audit in decision 7.

## Acceptance

- Part 0 findings recorded; the complete suite passes on `build-rel`;
  approved digests only; `rules/world-events.md` records the silent-teleport
  rule from decision 4.
- Independent implementation review (mandatory, Tier A).
- Maintainer play-test, one short session: New Game and open the Map (start
  cell explored); load the preset with a blank cell; leave the Inn after
  adding a Cartographer. Frames are checked by the agent first; the
  play-test confirms the main result only.
