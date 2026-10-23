# Milestone 85 plan - Event dispatch, part 2: the post-combat calls

**Tier A. Status: planned (not started).**
Roadmap [step 13](roadmap.md#next-arc-finishing-the-games-systems), second of
its two milestones, after [M84](milestone-84-plan.md). Baseline: `1ddb5d5`,
equal to `origin/main`.

## Goal and scope

As in DOS, an Event call that finds a monster in attack slot 0 stops and is
remembered, and combat ends with the original's two conditional calls: the
door-text call when a label was up as the fight began, and the skipped call
at the party's position, after Run included. A label returns after the
fight; an Event skipped while a monster held slot 0 acts after the fight;
Run lands as DOS does, and its "not supported yet" refusal at landings with
an Event goes away. Space while monsters are engaged follows Part 0. Every
Event call then clears WhoWill, and
[`dev-whowill-event-passes`](original-quirks.md#dev-whowill-event-passes) is
retired. Faithful oddities stay without an option: window 38 ("Nothing
here") can appear just before combat on an object-bearing cell, and labels
and traps return after fights.

Out of scope: Darkside, M84's per-input pass rules, Event-internal move
rules and combat rules. An abort between Event lines is in scope as one
generic line-head test. Planning does not authorize implementation;
independent plan and implementation reviews, the complete suite and a
maintainer play-test are required.

## Current state

Journey play at `1ddb5d5`:
- `XeenEncounterFlow::_regionalAutomatic` is the owed-call mark, written by
  the pass (`XeenEventFlow.cpp:647-660`), approach arrival
  (`XeenActorApproach.cpp:574-576`, `XeenJourneyFlow.cpp:900-901`), barrier
  Bash (`XeenBarrierFlow.cpp:258-260`), item-use arrival
  (`XeenJourneyFlow.cpp:480-517, 602-603`) and casting relocation
  (`XeenCastingFlow.cpp:484-489, 527-528`); about a dozen gates read it.
  `XeenJourneyCapture::inputPassPending` keeps an owed pass out of saves
  (`XeenSaveState.cpp:30, 320`).
- With monsters engaged, `prepareJourneyTransition` attaches combat first
  (`XeenEventFlow.cpp:691-695`); the mark runs only when Exploring without
  combat (`:697-699`, `beginJourneyEvent`). After a win it runs once at the
  same address; Run drops it (`originAutomaticSuperseded`,
  `XeenCombat.cpp:1734`, `XeenJourneyFlow.cpp:1066-1070`). No skipped-call
  or door-text value exists; labels are retained passive presentations.
- `xeenRunDestination` refuses a flagged landing with a record for the
  party's facing (`XeenCombat.cpp:815-826`).
- In combat, Space gives no interaction (`SdlWindow.cpp:168`) and a 3D click
  or interaction becomes Attack (`XeenMainScreenInput.h:68`,
  `XeenGameplay.cpp:496`). Outside combat, Space is dropped while
  `journeyQuiet()` is false (`XeenEventFlow.cpp:3047`).
- `XeenEncounterFlow::clearWhoWill` is unused. Non-Journey play has no
  combat.

## Original rules and evidence

Static analysis of the CD `WORLD/XEEN.DAT`, high confidence for the paths
(`D:/MModern-re/event-dispatch/findings.md` section 2; M81 and M84 records in
[evidence](rules/evidence.md#event-dispatch) and
[rules](rules/world-events.md#event-dispatch)).

- **Skip.** The Event routine `0x26774` clears WhoWill (`0x26797`) and
  stages, then tests attack slot 0 before each line, the first included
  (`0x26abd`, the between-line abort). If occupied it sets the skipped flag
  `[9756]` (`0x26ac4`) and jumps to cleanup `0x28d50`; the tail still
  applies. A call that scans clears the flag (`0x26acc`); there are no other
  writers.
- **Caller 3, `0x11802..0x1184a`.** After a combat started by the idle draw
  (slots 0-2, mode 1 or 5, `[97f6]`/`[97f9]` zero, `[96c4]` set), Events
  run at the party's current position when the flag is set, without testing
  the cell's Event flag: at the fight's cell after a win, at the landing
  after Run.
- **Caller 4, `0x2e62d..0x2e634`, `0x2ec18..0x2ec37`.** Combat entry saves
  and clears door text up `[3066]` (set only by the label opcodes,
  `0x26f7a`). At combat end, if it was up, the party cell's Events run when
  it is flagged. Caller 4 runs first; when its call scans it clears the
  skipped flag, so caller 3 follows only when caller 4 did not scan.
- The pin makes both calls but, per the review, never clears its skipped
  flag (`scripts.cpp:123, 177`); following DOS is a ScummVM difference.

## Maintainer decisions

- Two milestones for step 13 (2026-10-21); this is the second.
- Static program evidence first; DOSBox only where Part 0 cannot settle a rule.

## Part 0 - DOS evidence (read-only)

Static and calibrated, with offsets; work copies outside the repository.
Results go to the Event dispatch sections of `docs/rules/world-events.md` and
`docs/rules/evidence.md`, with quirk rows where they apply. No code.

1. **Combat entries and exits.** Every site that enters combat (idle draw,
   Event-started fights, Arena, any draw inside a handler) and every exit
   (win, full-party Run, partial Run then win, defeat, Load and Quit from the
   combat panel, spell or item relocation; Mr Wizard, refused in MMModern
   combat, recorded only): whether callers 4 and 3 are reached, at which map
   and position, in which order, whether an ordinary pass runs before the
   next input, and where combat-end presentations and treasure delivery fall.
2. **The skip.** Where `0x26abd` sits in the line loop; what a skipped call
   performs (WhoWill, treasure stash and restore, the tail); when an input's
   monster step fills slot 0; whether slot 1 or 2 can be held with slot 0
   empty outside combat; which Clouds opcodes (Spawn, TeleportAndContinue,
   the restart, map load) can fill slot 0 between lines, with a reachable
   witness.
3. **Engaged input.** Whether exploration input is read while a slot is held
   and combat has not started; what Space and a 3D click do then; what
   happens to keys typed between the monster step and combat.
4. **Door text.** Which label opcodes set `[3066]`; its clears (move, turn,
   Wait, `0x1df53`, `0x1e005`, others); whether it governs drawing the label
   and whether combat exit restores it; a caller-4 witness whose labelled
   facing can have slot 0 filled, Vertigo `(13,4)` only if so.
5. **Lifetime.** Both flags against the save ranges, Load, New Game, map
   load, the return to the title, and fights that do not reach caller 3.

## Design decisions

- **Existing owners.** `XeenEncounterFlow` owns combat entry and retirement,
  `XeenEventFlow` the calls and their candidate. Two named values carry the
  DOS state, the skipped flag and door text up, written only at their DOS
  sites and published with the Event candidate under the restore guard, so
  a stale or retried candidate leaves them unchanged. They live in
  `XeenProgramState` if Part 0 shows they outlive Load or New Game,
  otherwise in the encounter owner; neither is saved.
- **One obligation.** `_regionalAutomatic` stays the single owed call, now
  with a kind (pass, caller 4, caller 3) and its address. Every writer goes
  through the skip. `inputPassPending` stays set from the originating input
  until the last caller completes, so no capture holds an unmade caller.
- **Skip.** Slot 0 occupied means a monster in MMModern combat contact
  position 0. A pass or manual call admitted in the Engaged phase runs
  before attachment: entry, tail (window 38 included) and flag, then combat
  attaches. The line-head test in `XeenEventInterpreter` aborts a scan the
  same way between lines. This replaces carrying the mark through combat,
  the address match after a win and `originAutomaticSuperseded`.
- **Combat end.** At retirement: caller 4 when door text was up at entry
  and the cell is flagged, then caller 3 when the flag is still set, at the
  current position without a flag test. Each is an ordinary call through
  the existing candidate, tickets and boundaries and clears WhoWill. A flag
  left by a skip without a reaching fight survives until the next scanning
  call, so a later fight can make caller 3. A caller's Event can engage,
  move or kill the party like any call; re-engagement skips again and waits
  for the next fight, with no recursion.
- **Run.** `xeenRunDestination` no longer refuses a landing for its Event;
  other refusals stay. The quirk row is rewritten.
- **Door text** is set and cleared at the Part 0 sites and saved and cleared
  at combat entry. Label drawing follows the flag if Part 0 shows that the
  flag governs it.
- **Engaged input** follows Part 0 item 3: Space or a 3D click accepted while
  slot 0 is held makes a skipped manual call, Space's Wait and a skipped
  pass, then combat; a key DOS reads after combat waits; one DOS combat
  consumes goes to combat.
- **WhoWill.** Every call clears it; `dev-whowill-event-passes` and
  `clearWhoWill` go, with the comment at `XeenMonsterTreasure.h:35`.
- **Treasure.** M81's order and the Event-treasure duplicate stay; a caller
  precedes the delivery that follows it in DOS.
- **Save v18, schema 9 and content 14.**
- **Stop rules.** Part 0 answers within items 1-5 are implemented without
  asking. Stop only for: a saved flag; a new owner; a change to an M84 pass
  rule; a between-line abort needing more than one generic line-head test; a
  presentation difference other than label drawing; a case M63's approved
  movement buffering cannot express; a caller that cannot be represented
  without dropping or duplicating a call.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS evidence, rules chapters | Maintainer reads the findings |
| A | Skip, both callers, Run, door text, engaged input, WhoWill, tests, presets | Independent implementation review, suite, play-test |
| B | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap | Maintainer acceptance |

## Risks

- **Combat retirement** under Journey tickets, restore guards and the
  Victory and Disengaged phases.
- **Tests and digests.** Tests expecting the Run refusal, the superseded
  origin or the deferred arrival change only where a Part 0 rule explains
  it. A changed M44 digest needs its first divergence explained against a
  rebuilt baseline, preferably by a counterfactual run, and the
  maintainer's approval; it never waives exact restore.

## Tests and acceptance

- Unit: a skipped pass, a skipped manual call and a between-line abort
  (entry, tail, flag, no further lines); a scanning call clears the flag;
  caller 3 after a win (also on an unflagged cell after a skipped manual
  call) and after Run (scanning the landing, flagged or not); caller 4 only
  with door text up at entry and a flagged cell, clearing the flag so caller
  3 does not follow, and both when caller 4 is itself skipped; a stale flag
  across a quiet boundary making caller 3 after a later fight; the exits
  Part 0 excludes, combat Load included; door-text writers; Run admitted at
  a landing with an Event; engaged Space; a save refused before a caller; a
  rolled-back candidate leaving both values; WhoWill cleared and exactly one
  ordinary pass for the next input.
- Original data, driven until monsters engage, with no actor placed where
  DOS movement cannot go: the Part 0 caller-4 witness after a win and after
  Run; Run to the Vertigo landing `(18,4)` facing East, the former refusal,
  whose original monster stands at `(17,4)`; a trap skipped as a monster
  engages, hurting once after the fight, with live monsters able to reach
  it.
- Saves: the generic exact-restore test unchanged; neither value serialized.

Acceptance: Part 0 rules recorded; independent plan review applied; Part A
implemented and independently reviewed; the complete `build-rel` suite
passes as [tools/AGENTS.md](../tools/AGENTS.md) requires; M44 digests
unchanged or replaced with approval; coverage regenerated if support
changes. The maintainer play-tests the label after a win and a Run, the
Vertigo Run landing facing East and the skipped trap, from new
`m85-playtest` presets. Closure also corrects `docs/rules/items.md`'s
WhoWill paragraph, stale since M84.
