# Milestone 85 plan - Event dispatch, part 2: the post-combat calls

**Tier A. Status: completed and accepted by the maintainer (2026-10-23)**
after the play-test of Part A. Plan `1d2b92d` (independent review applied,
amendment `70e152c`), Part 0 `15ea744`, Part A `e3066f0` and the closure
commit. Roadmap
[step 13](roadmap.md#next-arc-finishing-the-games-systems), second of its
two milestones, after [M84](milestone-84-plan.md). Save v18, schema 9 and
content 14 unchanged.

## Scope and results

As in DOS, an Event call that finds a monster in attack slot 0 before any of
its lines stops there and is remembered, and a fight ends with the
original's two conditional calls: the door-text call when a label was up as
the fight began, then the skipped call at the party's current position,
after Run included. A door label comes back after the fight, a trap or NPC
skipped as a monster arrived acts once the fight is over, and Run lands
wherever the map's run position is, without the old "not supported yet"
refusal at landings with an Event. Every Event call clears WhoWill, which
retired `dev-whowill-event-passes`. Mr Wizard in combat stays refused.

| Part | Commit | Result |
| --- | --- | --- |
| 0 - DOS evidence | `15ea744` | Line-head skip and its flag, both callers, every combat entry and exit, engaged input, door-text writers and drawing, both lifetimes, the Vertigo witness from original monster movement |
| A - implementation | `e3066f0` | Skipped flag in `XeenProgramState`, door text in the encounter owner, one owed call with a kind, line-head test in `XeenEventInterpreter`, callers at combat retirement, Run admission, presets `m85-playtest-label`, `-run` and `-trap` |
| B - closure | This change | Status, history, roadmap |

## Key decisions

- **Rules follow static DOS evidence** (Part 0, high confidence): the skip
  is a test before every line, not only at entry; caller 3 follows any
  combat a draw admits, while a fight entered by exploration Shoot has none;
  a scanning caller 4 clears the skipped flag and so suppresses caller 3;
  defeat still reaches caller 4 and Quit reaches neither. Neither flag is
  saved; the skipped flag survives Load, New Game and the return to the
  title, while gameplay entry clears door text. The pin never clears its
  skipped flag on a scan, a recorded ScummVM difference.
- **Approved deviation `dev-skipped-window-first`** (maintainer,
  2026-10-23). In DOS a skipped call's "Nothing here" window draws the
  screen before its acknowledgement, which can start the fight and its
  callers inside the unfinished call. MMModern acknowledges the window and
  finishes the call first, then the fight starts.
- **Treasure keeps its gate.** Combat end calls delivery before caller 4,
  but in mode 2, so the delivery routine's own monster-in-view test still
  defers it (`0x2eb78`, `0x2414e..0x241e4`). A first implementation that
  delivered unconditionally changed the roundtrip digest and was rejected;
  with the gate kept, all four M44 digests are unchanged.

The rules, tables and witness are in
[world Events](rules/world-events.md#event-dispatch), offsets and
calibration in [evidence](rules/evidence.md#event-dispatch), and the
deviation in [original quirks](original-quirks.md#dev-skipped-window-first).

## Validation and acceptance

The independent plan review (approve with changes, 14 findings) was applied
before Part 0; Part 0 stopped once, on the window ordering, and the
maintainer approved the deviation. Part A had an independent implementation
review in a fresh context. The complete `build-rel` suite passed 279/279 as
[tools/AGENTS.md](../tools/AGENTS.md) requires (two stale defeat expectations
fixed in tests and rerun), and the M44 audit found zero differences with
exact reload and re-save. Coverage counts did not change. The maintainer
play-tested the Vertigo label after a win and a Run, the East-facing Run to
`(18,4)` and the map 55 trap, and accepted the milestone on 2026-10-23.
