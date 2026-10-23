# Milestone 84 plan - Event dispatch, part 1: a pass on every input

**Tier A. Status: planned (not started).**
Roadmap [step 13](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones, after [M83](milestone-83-plan.md). Baseline: clean `main`,
`d7f278e`, equal to `origin/main`.

## Goal and scope

As in DOS, every input the exploration loop processes ends with one
main-loop pass, and a pass over a cell with the Event flag runs that cell's
Events for the party's position and facing. Standing on a trap, any key
hurts again; door and sign labels come back after Wait; guarded NPCs return
while their guard passes; Event sounds and speech repeat. Space and a
3D-view click run a flagged cell's Events twice. Load and New Game begin with
a pass. WhoWill clears on every pass over a flagged cell.

Out of scope, for step 13's second milestone: the two conditional
post-combat calls (an Event skipped while a monster held attack slot 0, and
the door-text run at combat end), Space while monsters are engaged, the Run
refusal on landings with an automatic Event, and the rest of
[`dev-whowill-event-passes`](original-quirks.md#dev-whowill-event-passes).
Darkside stays out. The repeats are faithful and get no option. Planning
does not authorize implementation; independent plan and implementation
reviews, the complete suite and a maintainer play-test are required.

## Current state

Journey play:
- Moves, turns, strafes, blocked moves, Jump landings, barrier Bash and
  travel by spell or item mark the reached cell's automatic Event when it is
  flagged and a record matches the facing (`XeenActorApproach::
  regionalTransition` `automaticEvent`, casting and item
  `automaticArrival`). `XeenEncounterFlow::_regionalAutomatic` holds the
  mark; `XeenEventFlow::prepareJourneyTransition` starts the detached Event
  candidate (`beginDetachedEvent`, `XeenEventSystem::runAutomaticEvent`).
  Both `beginJourneyEvent` and `beginDetachedEvent` refuse a cell without a
  matching record.
- Wait, Rest, Cast without travel, Shoot, non-travel item Use, exploration
  dialogs, Escape and unmapped keys run no Event; SDL delivers only mapped
  keys, synthesizes repeats only for navigation and turns every right-click
  into Escape (`SdlWindow.cpp`, `XeenMainScreenInput.h`).
- Space and a 3D-view click both become `InteractionAction`: one manual
  Event, then the manual step (Wait) for both.
- WhoWill clears on arrival at a new position or facing over a flagged cell
  (`_whoWillPass`), on Space and in every Event run; `clearWhoWill` needs a
  quiet boundary, which a pending mark prevents.
- With monsters engaged, combat attaches first; a pending arrival Event runs
  once after a win and Run clears it.
- Saves: panel Save, F9 and New Game's initial publication
  (`XeenGameplay.cpp`, `XeenSaveState::captureInitialized`). F9 and the
  panel bypass `XeenEventFlow::handle`. Ordinary `canSave` refuses a pending
  mark through `journeyQuiet`; initialization capture has its own admission.
  Load and New Game run no Event.
- Only the manual path has a tail: sound 21 for a wrong-facing record with
  an object in front, on unflagged cells (`XeenEventSystem::runManualEvent`).

Non-Journey play: `XeenNavigationFlow` runs the automatic Event itself after
a successful move or turn, and Wait returns early in `XeenEventFlow::handle`.

## Original rules and evidence

Static analysis of the CD `WORLD/XEEN.DAT`, 2026-10-21, high confidence,
calibrated on the M81 callers (`D:/MModern-re/event-dispatch/findings.md`);
the pin's `scripts.cpp:134-300` agrees on the Event routine. The maintainer
confirms from DOS play that traps hurt again on every key.

- The main loop (`0xd18c..0xd5e5`) looks up the party cell (`0x11f04`); with
  flag `0x10` it calls the Event routine `0x26774` (`0xd1a5`), then delivery
  (`0xd1b7`), the idle draw where combat starts (`0x10bc3`) and the input
  wait. Idle frames return after the Event check: no re-run without input.
- A pass is any processed input: a key with a nonzero code (`0x45436`,
  `0xe14e`), a button click (`0xe102`) or a 3D-view click (`0xe136`); not a
  click on empty panel area. Either mouse button is the same click: the CD
  build's callback (`0x43832`) queues coordinates only, its reader
  (`0x4384f`) returns 0 or 1, and the right-click Escape branch at `0xe0dc`
  is never reached (M84 Part 0, 2026-10-23). Keys that
  match nothing in the 26-key table (`cs:0xeac`) also return (`0xd2e9`,
  `0xd5c7`, `0xd5e5`).
- A call is a full Event run even with no matching record: WhoWill cleared
  (`0x26797`), the stash, the scan from line 0; only the script's guards
  stop a repeat. The tail (`0x28e97..0x28f1c`) may show "Nothing here"
  (window 38) and suppresses sound 21 on flagged cells (`0x28f13`).
- Space or a 3D-view click runs the Events at `0xd2be`; only Space then
  waits (`0xd468`: time, monsters, door text cleared at `0xd472`), and the
  pass runs them again. A run starting while attack slot 0 is occupied is
  skipped and flagged (`0x26ac4`) for the second milestone.
- Clouds has 376 flagged cells: 187 with records (108 without a leading If
  guard) and 189 outdoor cells without. Witnesses: the damage trap at map 26
  (8,3), Vertigo's door labels at (13,4) West and (10,8) North, the guarded
  NPC at map 7 (6,3).

## Maintainer decisions

- Two milestones for step 13; this one is the per-input pass (2026-10-21).
- **Load and New Game begin with one pass, as in DOS** (2026-10-23).
  Restore still reconstructs the saved state without executing Events,
  rewards, time or RNG; the Load pass is subsequent gameplay. A panel Save
  or Save As returns to the panel (Part 0: `0x1f1e9`, `0x1ef36`), so the
  saving game's next pass comes when the panel closes, once per visit
  however many saves it made; F9, a key DOS does not match, ends with its
  own pass. Either way the saving game's following pass matches a loaded
  game's entry pass at the same cell. The generic exact-restore
  test stays unchanged; a continuation test compares the two passes.
- **Right-click acts as left-click on every screen, as in DOS**
  (2026-10-23). Part 0 found it statically; the maintainer confirmed in
  DOSBox that the right button always acts as the left. This replaces
  MMModern's right-click as Escape and corrects the earlier evidence for it
  (quirk rows on WhoWill, combat rows and death/credits pacing,
  `docs/controls.md`).

## Part 0 - DOS evidence (read-only)

Static and calibrated, with offsets; work copies stay outside the
repository. Results go to a new "Event dispatch" section of
`docs/rules/world-events.md` and to `docs/rules/evidence.md`, with quirk
rows where they apply. No code.

1. **Inputs.** The keys and combinations the key routine reports with a
   nonzero code (modifiers alone, Alt and Ctrl, keypad, F7-F12, Insert,
   Delete, Page Up/Down); the mouse regions that return a code; typematic
   repeat of non-navigation keys; the key flush after an executed script.
   Deliver a table that classifies every MMModern exploration input, F9,
   Alt+Enter and window close included, as a pass or not.
2. **Handlers.** Confirm that every exploration handler returns to `0xd18c`:
   character sheets and their item Use, Info, Quick Reference, Map, Quests,
   the control panel (Save, Load, Quit, options, Mr Wizard), Cast, Rest,
   Bash, Shoot, the automap. Name any that leave elsewhere.
3. **Load and New Game.** Trace title Load, panel Load and New Game to the
   Event check; place the pass against the map load, monster rebuild and
   arrival view.
4. **Space and 3D click.** Confirm manual run, Wait (Space only), pass;
   grate interception; and what the second run does when Wait engages a
   monster.
5. **Tail.** The exact window 38 and sound 21 conditions for a call without
   a matching record and for Space, and the Clouds cells where they show.

## Design decisions

- **One pass rule in the existing owners.** `XeenEventFlow`, owner of
  exploration input and the Event candidates, takes one pass when an
  exploration input completes; `XeenEncounterFlow` keeps the pending mark in
  Journey play; the detached Event candidate runs it. The arrival-only
  marking becomes this rule rather than a second trigger. No new owner.
- **One completion per outer input.** An outer exploration input owns one
  pass obligation across its dialogs, item Use, services, animation and
  actor settlement. Nested responses and Event-generated presentations add
  none. Travel and Bash arrival marks are that input's pass, not a second
  run. Cancelled and refused actions still complete with a pass.
- **A pass**, on a flagged cell, calls the Events: it clears WhoWill and
  stages as the original's entry does, runs the lines that match position
  and facing (or All), and applies the Part 0 tail. On an unflagged cell it
  does nothing. It follows the input's own effect (the move, Wait's time and
  monsters, the dialog closing) and precedes ordinary delivery.
- **Calls without a matching record.** The existing Event admission and
  candidate extend to represent a flagged call with no matching entry, under
  the same publication guards, tickets, boundaries and save exclusion
  through retirement. A missing record is normal completion, not
  unsupported content.
- **Every processed input is a pass** (class-wide default): moves, turns,
  strafes, blocked moves, Wait, Rest, Cast, Bash, Shoot, item Use, each
  exploration dialog when it closes, Escape, a click with either button on
  a button, portrait or the 3D view, unmapped keys and F9 (unmapped in
  DOS). Not passes: modifiers
  alone, empty panel clicks, input consumed by an open dialog or Event
  presentation, and MMModern host keys (Alt+Enter, window resize and close).
  Part 0's table overrides this default; a difference the rule cannot
  express stops the part.
- **Space and the 3D-view click** keep their origin through suspension and
  publication: both run the manual Event; Space then waits and a 3D click
  does not; then the pass samples the resulting position, facing and flag,
  also after manual travel.
- **Engaged monsters, until the second milestone.** This exception
  overrides the pre-combat order: a pass whose input engages monsters, new
  ones included, keeps today's deferred arrival (combat first, the Event once
  after a win, cleared by Run). Engaged Space and the Run refusal stay. No
  DOS post-combat caller is added.
- **Input layer.** SDL delivers one generic exploration action for unmapped
  keys, carrying no rules, as `DialogKeyAction` does. The right button
  produces exactly the left button's action on every screen (exploration,
  combat, dialogs, title and credits); right-click as Escape is removed in
  one input-layer change, with the quirk rows and `docs/controls.md`
  corrected in the same change. Held keys and keys
  typed during an Event follow Part 0; M63 repeats only navigation today. A
  conflict with M63's approved buffering stops for a maintainer decision.
- **WhoWill.** The per-pass clear replaces `_whoWillPass`;
  `dev-whowill-event-passes` narrows to the two post-combat calls.
- **Non-Journey play.** `XeenNavigationFlow`'s own automatic dispatch moves
  into the common pass rule, and Wait's early exit no longer skips the
  pass. A suspended completion stays in existing `XeenEventFlow` state; no
  Encounter owner is built for it.
- **Save v18, schema 9 and content 14, provisionally.** Panel Save and Save
  As are nested in the panel's outer input and arm nothing; the panel's
  close completes that input's one pass. F9 and New Game's initial
  publication capture before arming their own following pass, then complete
  it through `XeenEventFlow`. A successful Load discards the old session's continuation
  and starts exactly one entry pass. Ordinary and initialization capture
  must be shown unable to serialize an armed pass; otherwise, or for any new
  saved field, stop for an amendment.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS evidence, rules chapters | Maintainer reads the findings |
| A | Pass rule, inputs, Space, saves, Load/New Game, WhoWill, tail, tests | Implementation review, suite, play-test |
| B | Closure: census, quirks, differences, status, history, roadmap | Maintainer acceptance |

## Risks

- **Journey admission.** A pass keeps its obligation through ordinary
  asynchronous readiness (tickets, boundaries, restore guards); stop only if
  it cannot be represented without dropping or duplicating a pass.
- **Tests and digests.** Many tests assume one Event per arrival; change an
  expectation only where a Part 0 rule explains it. A changed M44 digest
  needs its first divergence explained against a rebuilt baseline and the
  maintainer's approval. M44 assertions name their comparison phase (state
  before input, reload and re-save); digest approval never waives exact
  restore or continuation equality.
- **Harsher play.** About 40 unguarded damage or reward cells repeat on
  every key. This is faithful; held-key behavior follows Part 0.
- **Event-internal moves** (teleports, stairs) keep their M70 behavior; the
  pass adds runs only at input boundaries.

## Tests and acceptance

- Unit: the pass table for every exploration action; one pass per outer
  input, including cancellation, refused actions and presentation retries;
  Wait, closing Info, Escape and an unmapped key once; Space and 3D click
  twice, with Space's time and monster step only; the automatic pass never
  runs Events on an unflagged cell (manual interaction still can); calls
  with no record, a wrong-facing record and at `(0,0)` North; WhoWill per
  pass; the tail; a win and a Run after an engaging pass.
- Input: right-click equals left-click on a button, a portrait, the 3D view
  (manual run plus pass), a combat row and a dialog; SDL tests for mapped
  and unmapped repeats, taps buffered across a
  non-modal executed script, modal completion, record-less calls, and kept
  key releases and host actions.
- Saves: a panel visit with no save, one save and two saves each followed
  by exactly one pass at close; F9 (successful, failed, unavailable) and New
  Game publication each followed by exactly one pass; the generic
  exact-restore test unchanged; a continuation test comparing Save,
  acknowledge, close panel and its pass with the loaded branch's entry pass
  under identical responses (state, RNG and Event counts).
- Non-Journey: movement, blocked movement, Wait, dialog closing and manual
  interaction.
- Original data: map 26 (8,3) damage on Wait and after closing Info; the
  Vertigo (13,4) West label back after Wait; map 7 (6,3) re-prompting only
  while its guard passes; Load on a flagged cell running its Events once.

Acceptance: Part 0 rules recorded; independent plan review applied; Part A
implemented and independently reviewed; the complete `build-rel` suite
passes as [tools/AGENTS.md](../tools/AGENTS.md) requires; M44 digests
unchanged or replaced with approval; coverage regenerated if support
changes. The maintainer play-tests the map 26 trap on Wait, Info and Escape,
the Vertigo label after Wait, Space on a flagged cell and a Load there, from
an `m84-playtest` preset if no existing one reaches them. Closure updates
quirks, ScummVM differences, status, history and roadmap.
