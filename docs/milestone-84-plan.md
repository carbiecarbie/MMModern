# Milestone 84 plan - Event dispatch, part 1: a pass on every input

**Tier A. Status: completed and accepted by the maintainer (2026-10-23)**
after the play-test of Part A. Plan `d13ced5` (independent review applied,
amendments `10260af`, `2bbd262`), Part 0 `ee9fe24`, A1 `7a6ed85`, A2
`9e6fdd5` and the closure commit. Roadmap
[step 13](roadmap.md#next-arc-finishing-the-games-systems), first of its two
milestones, after [M83](milestone-83-plan.md). Save v18, schema 9 and
content 14 unchanged.

## Scope and results

As in DOS, every input the exploration loop processes ends with one
main-loop pass, and a pass over a cell with the Event flag runs that cell's
Events, even when no record matches. Standing on a trap, any key hurts
again; door labels return after Wait; guarded NPCs return while their guard
passes; Event sounds repeat. Space and a 3D-view click run a flagged cell's
Events twice. Load and New Game begin with a pass, and WhoWill clears on
every pass. The keyboard and mouse follow the CD original's tables.

Left for the second milestone: the two conditional post-combat calls (an
Event skipped while a monster held attack slot 0, and the door-text run at
combat end), Space while monsters are engaged, the Run refusal on landings
with an automatic Event, and the rest of
[`dev-whowill-event-passes`](original-quirks.md#dev-whowill-event-passes).

| Part | Commit | Result |
| --- | --- | --- |
| 0 - DOS evidence | `ee9fe24` | Input table, handler returns, Load/New Game/Save paths, key continuation, flush, typematic and tail rules; Clouds census corrected to 300 flagged cells |
| A1 - dispatch core | `7a6ed85` | One pass per outer input in the existing owners; record-less calls; continuation with the key an Event leaves; save, Load and New Game passes; non-Journey play |
| A2 - input layer | `9e6fdd5` | DOS key translation, generic unmatched-key action, right-click as left-click everywhere, DOS mouse bounds, queued keys and non-navigation typematic, play-test presets |
| B - closure | This change | Trap preset moved to map 55, documentation |

## Key decisions

These are the maintainer's decisions of 2026-10-21 and 2026-10-23.

- **Two milestones** for step 13; this one is the per-input pass.
- **Load and New Game begin with one pass**, as in DOS. Restore still
  reconstructs the saved state without executing Events, rewards, time or
  RNG; the Load pass is subsequent gameplay. A panel Save returns to the
  panel, whose close makes the visit's one pass; F9 ends with its own pass.
  The saving game's following pass therefore matches a loaded game's entry
  pass, which a continuation test compares.
- **Either mouse button makes the same click on every screen.** The CD
  build's callback queues coordinates only and the right-click Escape branch
  at `0xe0dc` is unreachable; the maintainer confirmed it in DOSBox.
- **Exploration keys follow the DOS table**: W waits, the period is an
  unmatched key, keypad 4/6 turn, Ctrl+Down has no turn-around, Ctrl
  translates the plain key, and F10-F12 and Delete give no code. F8 keeps
  the approved omission of speech replay.
- **M44 digests replaced with approval.** Mainland `76651b1a`, services
  `a971e48d`, temple `b2cea761`, roundtrip `8f0b712c`. Each first
  divergence is Space's Wait skipped because an Event replaced or cleared
  the key (NPC `0x25ac7` with the Enter acknowledgement at `0x25ba6`,
  WhoWill `0x25299`/`0x252c5`, map load `0xf590`). With only that rule
  disabled, mainland and roundtrip reproduce their old digests; services and
  temple differ only in presentation order and the override's forced
  post-service Wait, which DOS does not perform.
- The repeats are faithful and get no option.

The rules, input table, mouse bounds, flush, typematic and tail are in
[world Events](rules/world-events.md#event-dispatch); offsets and
calibration are in [evidence](rules/evidence.md#event-dispatch).

## Validation and acceptance

The independent plan review was applied before Part 0. A1 and A2 each had an
independent implementation review in a fresh context, which found and closed
five and seven defects. The complete `build-rel` suite passed 272/272 on A1
combined with the GiveMulti fix `cfa9978`, and 276/276 on A2, run as
[tools/AGENTS.md](../tools/AGENTS.md) requires. The A2 M44 audit found zero
differences from the approved digests, with exact reload and re-save. The
maintainer play-tested the map 55 trap on Wait, Info, Escape and unmatched
keys, the Vertigo label after Wait, Space, Save and Load, the mouse buttons
and the new keys, then normal play, and accepted the milestone on
2026-10-23.
