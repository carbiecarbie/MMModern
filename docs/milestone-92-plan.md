# Milestone 92 plan - Clouds ending and score

**Tier A. Status: completed and accepted by the maintainer (2026-10-26)**
after the play-tests. Plan `1260911`, Part 0 `4c57a2e`, Part A `5394da2`,
Part B `bc85012`, Part C1 `3c192f0`, Part C2 `56961dd` and the closure
commit. Roadmap [step 18](roadmap.md#next-arc-finishing-the-games-systems),
after [M91](milestone-91-plan.md); [M90](milestone-90-plan.md) still waits
for its whole-quest play-through. Save v21 (schema 9, content 14); M44
digests replaced once, at Part A's format audit.

## Scope and results

Clouds of Xeen can be finished as in DOS:

- **The win.** Giving Lord Xeen's Scepter to the Sixth Mirror follows the
  DAT tail (`0x28b98..0x28d23`) in order:
  1. sets the saved completion;
  2. writes the won marker;
  3. sets flag 75;
  4. moves the party to Vertigo `(18,4)`, keeping its West facing;
  5. saves the live party and roster to the current slot, without loading
     map 28;
  6. turns Efx and Music on;
  7. computes the score;
  8. hands off to the ending.

  The save keeps experience, deaths, award 42 and equipment, as the repeat
  DOSBox capture of slot 06 shows.
- **Score.** Party total experience is raw XP plus a class/level allowance,
  using the DOS 1,024,000 multiplier. Dead members count. The truncated mean
  is divided by 10000 and multiplied by 100000, then hours × 100 plus
  minutes of real play time are added. The recorded `0071001722` is a unit
  test.
- **Play time and panel clocks.** Real play time is counted in seconds
  through an injectable wall clock with Borland's no-`TZ` conversion. The
  start mark is taken at program start, after every save and at a fresh
  title after an EXE hand-off, never at Load. The control panel shows the
  12-hour clock and the play time only after G, O, B, R, with DOS's 16-bit
  formatter artifacts; O or B out of order closes the panel.
- **Mirror words.** `showtime` saves and plays the ending with the real
  score, without the win or the move; `bloopers` does nothing; `showdown`
  and `genesis` stay "not supported yet".
- **After the win.** Castle Xeen is gone from map 27, and Lloyd's Beacon
  (spell or item) to maps 75-78 fails, both keyed on the saved completion.
- **The ending.** The `WORLD/XEEN.EXE` ending plays on the M91 presenter:
  the castle, pyramid and vortex in 61 compressed full-screen frames,
  Castle Burlock and the king, the mirror room and the 73-monster parade,
  the score reveal settling rightmost first, two keyboard-only text pages,
  the king's last speech, and a fresh World title. Animations ignore
  Escape, keys and clicks; Ctrl-Q quits only inside the EXE presentations.
  The EXE's own time-mixed random drives digits, mouths and power sounds.
- **Won store and V.** The won state lives in
  `%LOCALAPPDATA%\MMModern\settings.ini`. Other Options then shows `V`,
  which replays the ending with the fixed score `1234567890` and changes
  nothing. A missing or different XEEN.EXE gives a notice after the save.

## Key decisions

- **Won state outside the save** (2026-10-24): `settings.ini`, never
  `XEEN.CFG` (approved deviation:
  [`dev-clouds-won-store`](original-quirks.md#dev-clouds-won-store)). A
  failed write shows a notice, the ending continues and an earlier win is
  kept.
- **Save v21** for the play-time seconds and the completion flag. It also
  admits an omitted camera-map monster table for an unvisited camera map
  (2026-10-26; v21 was never released before this milestone).
- **A failed ending save** shows the error and retries the same snapshot,
  extending [`dev-inn-save-recovery`](original-quirks.md#dev-inn-save-recovery).
- **No stale roster.** The first DOSBox reading was wrong because it
  compared the XEEN-side `maze.chr`. The repeat capture shows DOS saves
  the live roster.
- **Part C in two phases** (C1, C2), each with its own review, suite and
  play-test.

## Evidence

Calibrated static analysis of `WORLD/XEEN.DAT` and `WORLD/XEEN.EXE`, the
maintainer's DOSBox ending capture, and before/after slot archives.
Details are in [cutscene rules](rules/cutscenes.md#clouds-ending-entry-and-save),
[party rules](rules/party.md#clouds-ending-save),
[time rules](rules/time-and-conditions.md#real-play-time-and-panel-clock) and
[evidence](rules/evidence.md#m92-clouds-ending-evidence). Part 0's
frame-hash audit script omitted the LZ source increment that EXE `0x1d03e`
performs; C1 corrected the 61 hashes against the EXE. The Beacon refusal
rests on static evidence only (no suitable Beacon in DOSBox).

## Results

- Complete suite 312/312 (run `20261026-052447-254c0e4c`); M44 digests
  `84a72d60`, `6a5b4c54`, `7f973e65` and `ad37fa95` (v21 audit).
- On a fake clock every ending stage lands within half a second of the
  DOSBox capture (first castle frame at 51.10-51.20 s, black before the
  title at 263.076 s). The score pages match the DOS frames pixel for
  pixel. The real Sixth Mirror preset runs through the ending in CTest.
- An independent review approved every part, after fix rounds for Parts A,
  B, C1 and C2.
- Maintainer play-tests (2026-10-26): the Lord Xeen fight, the mirror and
  the save, `showtime`, `bloopers`, the castle, the whole ending, the score
  pages and Other Options `V`.
- Follow-ups left for maintenance:
  - voices play quieter than in DOS throughout the game, including the
    EXE's mixer commands 75/95;
  - unrecognized control-panel keys close the DOS panel;
  - the sheet's experience may be 1,024,000 low at level 13 and above.
