# Milestone 108 plan - Darkside audit invariants as tests

**Tier B. Status: completed and accepted by the maintainer (2026-11-02)**,
without a play-test (tests only). Commit `cdc48e8`.
Step 2 of [After the Darkside audit](roadmap.md#after-the-darkside-audit).
Tests only; save v25 unchanged.

## Goal

Keep the invariants that the Darkside fidelity audit (2026-11-02) checked by
hand as original-data CTest cases, so that later changes cannot silently
break them. A test that exposes a production defect stops the milestone with
a reproduction; M108 fixes nothing in production code.

## Already covered (inventory)

- Falls with Dead, Stoned and Eradicated members at 128 (5,5), 98 (8,8) and
  Clouds 25, and the Space defeat at 124 (2,4): `xeen_fall_defeat_original`.
- Dials beyond the old dispatch cap: `xeen_dial_original`.
- CD disc choice for Darkside Events: `xeen_darkside_cd_disc_original`.
- Single Pyramid crossings keep both banks, Portal, Beacon, mixed falls and
  quest chains: `xeen_darkside_original`, `xeen_mixed_travel_original`,
  `xeen_darkside_quests_original`.
- Darkside Create gives level 5: `xeen_darkside_services_original`.
- Both wins write one forced save that reloads and re-saves byte-identically:
  `xeen_ending` (`newWin`).

## Tasks

- [x] Bounded repeats (I1): 31 (1,10) N stops after 3 uses, 39 (4,3) E after
  2, 74 (10,15) W after 3. Explicit exceptions: a purchase (4 (15,12) N),
  the unguarded skill grants 32 (27,3), 41 (8,6)/(8,8) and 42 (3,6), including
  the byte wrap at 255 ([sv-skill-byte-wrap](original-quirks.md#sv-skill-byte-wrap)),
  and the six-stage statue 32 (30,4).
- [x] Interleaved chain: 31 (1,10) N used 3 times with a save/load and a
  departure and return between uses equals 3 uses in a row.
- [x] Exact restore (I3): 74 (6,3) N, 47 (12,0) and 116 (10,6) N re-encode
  byte for byte after a save and load, and a second use after the restore
  equals a second use on the live game.
- [x] Journey crossing: a Clouds Event, the Clouds Pyramid to Darkside,
  Ellinger, the Darkside Pyramid back, the Clouds repeat and the fourth head,
  with an exact save/load round trip after each step.
- [x] Post-win loads: `newWin` now also takes a step from the reloaded
  Alamar and World forced saves.
- [x] Continuation key: Ellinger 56 (4,8) W, with Up pressed during the last
  NPC wait the party ends one step forward; with Space it ends with a Wait
  ([sv-event-key-continuation](original-quirks.md#sv-event-key-continuation)).
- [x] Keys during a fall: 128 (5,5) with a Dead member and keys every tick
  lands at Clouds 8 (11,15) with the plain fall's result.
- [x] One mutation per new invariant, reverted.

## Key decisions

- The new cases are modes of `mmodern_darkside_tests` (real composer,
  helpers in that file): `xeen_darkside_repeats_original` (I1 and the chain),
  `xeen_darkside_restores_original`, `xeen_darkside_crossing_original` and
  `xeen_darkside_keys_original`. Monsters on the Event's map are parked, as
  the existing Darkside tests do, so no fight decides an outcome.
- Departures and returns use the World owner's map load, as the audit and
  the Clouds invariant tests do.
- 74 (6,3) N only alters walls (four AlterMap rows); the gold the audit saw
  there came from a fight with the map's monsters, so the test checks the
  wall writes.
- The continuation key is compared with the plain Event followed by the same
  key as an ordinary command. The Space continuation is the ordinary Wait
  (one minute here), not a fixed ten minutes.
- The interleaved chain leaves out explored bits: only the return marks the
  Cartographer's arrival cell (M98).

## Results

- Added suite time about 30 s of test time in the complete run at 12 jobs
  (repeats 16.9 s, keys 6.8 s, crossing 3.0 s, restores 2.1 s; `xeen_ending`
  gains one step per side).
- Mutations, each failing its test: disabled Events running again; a skill
  byte saturating at 255; saves dropping disabled Events (chain); loads
  ignoring disabled Events (restore); world flags 128+ aliasing the Darkside
  bank (crossing); a completed game refusing movement (post-win step); Event
  dialogs dropping the acknowledged key (continuation); a Wait during a fall
  advancing the clock (fall keys).
- No production defect found.

## Acceptance

- The new tests pass on original data, each fails under its mutation, and the
  complete CTest suite passes on `build-rel`.
- No play-test: the milestone adds tests only and changes no behavior, so
  nothing remains for the maintainer to see or hear.
