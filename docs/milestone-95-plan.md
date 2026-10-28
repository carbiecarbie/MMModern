# Milestone 95 plan - Clouds audit follow-up

**Tier B. Status: planned, not started.** Roadmap
[step 1](roadmap.md#next-arc-release-quality-and-the-darkside-start).
Baseline `d631ad8` (release 0.10.1). Save v21, schema and content unchanged.

## Goal

While an Event waits for the player, the small map and the border show what
the Event has changed so far, as in the original. The Clouds audit's
invariants become permanent original-data tests, so that a defect of the
kinds it looked for fails a test instead of reaching a player.

## What is known

- **The audit.** It ran on 2026-10-28 in four sessions over Clouds'
  original data: 8,034 Event runs, about 7,500 exact save round trips,
  frames checked in 987 runs, party variants and interrupted Events. Results,
  evidence and reproductions are in
  `D:/Projetos/MModern/MModern-re/clouds-audit-20261028/` (`findings.md`,
  `details/`). It found one defect, the hidden-object save failure fixed in
  `3020bbe` (release 0.10.1), and one open question, Q01. Everything else
  matched the original or is a recorded exception.
- **Q01.** The skull on Witch Tower Level 4 (map 54 `(10,6)`, facing East)
  asks for a password: Rosebud opens a passage, and a later wrong word
  closes it. The audit opened the passage, walked through it, returned and
  gave a wrong word. During "Wrong. Begone!" the small map still showed the
  passage open, although the Event had already closed it (30 pixels in
  `[224,317) x [9,75)`, seen in real frames). Its fixture placed the party
  at the skull; everything after that was normal play. This is the only
  Clouds Event cell the audit found where the difference can show.
- **DOS** redraws the 3D view, the border and the small map from its one
  live state during a wait, so they already show the Event's changes.
  Calibrated static analysis, high confidence: wait redraw
  `0x24bf1..0x24bf8`; draw3d calls the small map at `0x1161d -> 0x13b27`
  (indoor walls `0x13d6b..0x1438c`) and the border assembly at
  `0x117b1 -> 0x1b9c6` (Danger Sense `0x1ba67..0x1baab`, Spot Secret Doors
  `0x1ba1e..0x1ba63`, Direction Sense `0x1bdb8..0x1bdfd`).
- **MMModern** draws Event waits in `XeenEventFlow::composeJourneyScene`
  (since `22a9d2b`). When the Event has moved the party, the whole frame
  comes from the pending candidate. When it waits in place, the frame is
  composed from the live world and only the 3D viewport (`[8,224) x [8,140)`)
  is copied from the candidate's full composition, so the small map, the
  lower part of Direction Sense and any border element outside the viewport
  stay as before the Event. The small map draws a tile only once it has
  been stepped on or while Wizard Eye is active (`XeenMapView.cpp`), which
  is why unexplored tiles hide the difference.
- **Gated rewards.** Of the 729 runs whose reward sat behind a gate, 604
  are covered by equivalence: their instructions ran elsewhere with the
  same kinds of operands. One kind never ran outside a gated path:
  TakeOrGive giving condition values 2, 9 and 10, on 7 cells of maps 68
  and 70.
- **R11.** For If action 21 with Misc values 60-81, DOS (`0x21891`) and
  MMModern (`XeenEventValues.cpp`) test the Misc item's material; pinned
  `scripts.cpp::ifProc` tests its ID. No Clouds Event uses these values,
  and the difference is not recorded yet.

## Part A - The small map and border during Event waits

- [ ] While an Event waits in place, the small map and every border element
  that the DOS border assembly (`0x1b9c6`) draws come from the pending
  candidate, as the 3D view already does. Check that list against the DOS
  routine and cite offsets for any element beyond the three indicators
  above.
- [ ] Keep unchanged: publication and rollback (drawing never publishes the
  candidate, and the restore guards keep their checks), scheduling, input,
  the portraits and buttons, the moved-party branch, dialogs and overlays.
- [ ] If a correct result needs a change of ownership, publication or
  scheduling, stop and report: that would make the milestone Tier A. If the
  portraits or another layer also differ from DOS during a wait, report it
  with evidence instead of widening the scope.
- [ ] Extend the Event-wait drawing rule in
  [world Event rules](rules/world-events.md) and its
  [evidence](rules/evidence.md) with the offsets above.
- [ ] Tests in `XeenEventDispatchOriginalTests.cpp`, beside the trap scenes.
  At the wait, with the small map on, compare the presented frame's
  small-map rectangle with the candidate's composition, then check that the
  Event completes and publishes as before. A fixture may place the party at
  the skull. Cases at the skull:
  - **Explored:** Rosebud opens the passage through the Event, `(9,5)` is
    explored, and a wrong word closes it again.
  - **Wizard Eye:** Wizard Eye is active and Rosebud opens the passage,
    checked at its confirmation wait.
  - **Fresh control:** no exploration and no Wizard Eye.

  The map 33 and map 55 trap scenes keep passing.
- [ ] Add a developer preset at the skull for the play-test, with a party
  that can win the Witch Tower's fights and cast Wizard Eye, documented in
  [development](development.md) like the other presets.

## Part B - Audit invariants as tests

`findings.md`'s section "Invariants worth keeping as CTest" gives the cases,
exceptions and rough runtimes. Extend an existing test where it already pins
a case (the map 33 rubble in `XeenEventDispatchOriginalTests.cpp`, the map
81 switches in `XeenOpeningOriginalTests.cpp`) instead of duplicating it. New
cases go in one new original-data test file; a helper used by one file stays
in that file. Setup code may be adapted from the audit's `harness.patch`;
the harness itself is not added. Keep each invariant's recorded exceptions,
and never widen a test into a sweep over every cell.

- [ ] **I1 Second use:** a completed one-shot reward never pays twice. Map 1
  `(0,15)` and map 3 `(8,14)` answering Yes; map 33 `(2,30)` after a dig
  that succeeds on the party's own random draws.
- [ ] **I2 Completion:** Events finish or reach a supported presentation
  without stalling. Map 41 `(26,11)` barrier selection; map 33 `(9,25)`
  rubble.
- [ ] **I3 Exact restore:** save bytes, owners and both RNG states survive a
  restore and a second Event. The three map 81 switches, the map 3 camp and
  the map 33 dig.
- [ ] **I5 Leaving and returning:** durable cell, object and Event changes
  survive leaving a map and coming back; a surviving bound actor reloads
  with full HP, asleep and Physical. The map 33 dig, a map 81 switch and
  one bound actor.
- [ ] **I6 Small parties and full packs:** a one-member party, and a party
  whose first member is disabled, choose the member their owners require
  (map 33 `(9,25)`); with every pack full, staged items are discarded but
  currency is still credited (map 12 `(1,11)`).
- [ ] **B1/B2 Interruptions and input:** an actor's interruption resumes the
  Event after combat (map 28 `(13,4)`); a key or click during the map 33
  `(3,15)` hold waits for its end; Spawn comes before combat (map 33
  `(9,25)`); input during the Vertigo starting shot waits until the flight
  ends.
- [ ] **B5 Misc material (R11):** a unit case where If action 21 tests a
  generated Misc item's material, not its ID. In the same change, record
  R11 under "Reference behavior not reproduced" in
  [original quirks](original-quirks.md) with an anchor and **ScummVM:**
  differs, as [the Sphinx entry](original-quirks.md#sv-sphinx-might) does,
  and add its item to [ScummVM differences](scummvm-differences.md).
- [ ] **Gated-only operand:** a unit case for TakeOrGive giving conditions
  2, 9 and 10, unless a test already covers it.
- [ ] Each test names the audit finding it pins. Label a test `fast` only if
  it takes a few seconds.

## Out of scope

The audit's recorded limits stay limits: the five combat boundaries without
a post-combat wait, the gated paths it left uncovered, full certification of
overlays, lighting, audio and device timing, temporary walls and Warzone
lifetimes after leaving, and an exhaustive opcode comparison with the pin.
No save-format change, no new owner and no Darkside work.

## Acceptance

- [ ] The explored and Wizard Eye cases fail on the baseline and pass after
  Part A; the control passes on both. M44 digests are unchanged, or the
  report names each changed frame and why.
- [ ] Part B's tests pass, and the complete CTest suite passes on
  `build-rel` ([test runner](../tools/AGENTS.md)).
- [ ] One short maintainer play-test after Part B, as a simple numbered
  guide with `.cmd` launchers: at the skull with Wizard Eye active, the
  small map shows the passage open during Rosebud's confirmation; after
  walking through and giving a wrong word, it shows the passage closed
  during "Wrong. Begone!"; one ordinary Event wait elsewhere looks as
  before. The DOS rule rests on calibrated static evidence, so there is no
  DOSBox item.
- [ ] Each part is its own commit.
