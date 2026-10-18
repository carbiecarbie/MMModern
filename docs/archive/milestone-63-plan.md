# Milestone 63 - Original presentation clock and movement pace

**Tier A, in two parts. Status: completed and accepted.** Plan `ddc5959`;
Part A `4f20b52`; Part B `5c40ffe`. The maintainer accepted the play-tests of
both parts (2026-10-13/14).

## Goal and final scope

Reproduce the original presentation timing and roughly nine movement steps per
second ([roadmap](../roadmap.md) step 9), with the approved no-phantom-steps
deviation. Authority: calibrated analysis of the original program `WORLD/XEEN.DAT`
(findings under `D:/MModern-re/ui-presentation-20261013/` and
`D:/MModern-re/movement-fluidity-20261013/`, outside the repository), the
maintainer's DOSBox CD recordings (title, dream, party death, Gunther speaking,
movement at 20000 and 80000 cycles) and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Part A - clock, movement and input.** One shared clock in original BIOS
  ticks (18.2065 Hz) replaces the scattered fixed waits; fades follow a
  separate 70 Hz retrace timeline. Scenes advance once per original two-tick
  draw, never per host refresh, without catch-up after stalls. The next step is
  accepted while only the actor countdown remains, and pending actor work runs
  as the original chargeStep does. The keyboard button hold is gone; mouse
  buttons wait for release. Every distinct tap is kept; a held key repeats
  after the PC keyboard's 500 ms delay at 10.9/s, capped by the draw pace, and
  release discards queued repeats.
- **Part B - sequences on the clock.** NPC speech holds for the exact CD cue
  duration and animates the face with the original CD-busy random draws. Shops
  use their speaking frames and VOC-length sample gates (Tavern goodbye);
  merchant stock is generated at the original departure and member-day calls.
  Event and monster treasure is shown item by item, with currency credited
  after the final key. Well sparkles and damage holds run per recipient; Shoot
  keeps simultaneous lanes, splats and the pre/post-HP draws before a lethal
  removal; monster attacks show two pose draws. The dream has its fades and
  sample holds without an Escape skip; the title has its initial fade and
  four-tick phases; party death plays fire, laugh, scroll and fade on every
  route and returns to the World of Xeen title instead of DEFEAT.

## Decisions

1. **Approved queue deviation (option b)** with the typematic delay added after
   the maintainer's first Part A play-test; recorded in
   [original quirks](../original-quirks.md).
2. **CD speech duration** is `(end - start + 25) / 75` s from the original play
   call; the drive/capture start latency could not be isolated from the
   recording and no guessed delay is added (maintainer, 2026-10-13).
3. **Shared RNG.** Original NPC and shop speaking draws use the world/Journey
   generator at eligible animation calls, never a cosmetic stream.
4. **Save v12** (schema 9, content 14) retained; clock, holds and visual
   continuations are transient.
5. **M44.** Part A kept all digests (presentation-phase trace differences
   approved). Part B replaced the Mainland digest after the maintainer-approved
   audit: Myra's five original speaking draws shift later combat rolls (five HP
   values, four conditions); Services and Temple kept their digests with
   trace-only differences.

## Results

- Movement on `build-rel`: 9.10 steps/s and 9.11 turns/s (DOS about 9.10),
  second step about 0.52-0.58 s after the press, no repeat after release.
- Rules, input and sequence tests for every row, exactly-once publication under
  failure and stale frames, original-data death and timing tests.
- Full CTest 190/190 (Part A) and 192/192 (Part B) on `build-rel`; Debug
  closure check passed; independent Tier A plan and implementation reviews
  passed. A Temple regression found during Part B (Heal debit before the
  departure reserve check) was fixed before the final runs.

## Deferred

Audio playback; an opt-in faster-movement option (maintainer decision);
guard/sprite-validation performance work.
