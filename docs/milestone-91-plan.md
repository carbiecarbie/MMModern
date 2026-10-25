# Milestone 91 plan - Cutscene presenter and the Clouds intro

**Tier B. Status: completed and accepted by the maintainer (2026-10-25)**
after the play-test. Plan `0543336`, Part 0 `5bdd161`, implementation
`fcc54d8` and the closure commit. Roadmap
[step 17](roadmap.md#next-arc-finishing-the-games-systems), after
[M89](milestone-89-plan.md), while [M90](milestone-90-plan.md) waits for its
whole-quest play-through. Save v20, schema 9 and content 14 unchanged; M44
digests unchanged.

## Scope and results

Other Options `C` plays the Clouds intro as the CD's `WORLD/XEEN.EXE` does,
then returns to a fresh World title as the original program does. In DOS
the DAT never draws a cutscene: it hands off to XEEN.EXE through
`XEENCOMMAND` and is started again afterwards. MMModern keeps both programs
in one process:

- **Presenter.** `XeenCutscene` (`games/xeen`, no SDL) is an ordered list
  of segments. Each composes a frame (RAW background, palette, sprite
  frames, vertical page merge, scroll, text) and holds until elapsed BIOS
  ticks, a retrace-paced fade, digital idle or an input rule ends it.
  `XeenCutsceneFlow` (`app`) is the new owner, because XEEN.EXE's screens
  belong neither to the title nor to gameplay. It paces segments on
  `PresentationClock`, publishes frames and sends songs, FM cues and VOC
  samples through `XeenAudio` only. The ending's kinds (compressed frames,
  horizontal merges, monster parade, score reveal, acknowledgement holds)
  are new composition, generator or termination kinds for step 18; none is
  implemented yet.
- **EXE data.** `XeenCutsceneData` reads the resource names, voice table,
  31-entry mouth and wait rows, tower coordinates and labels from the
  installed XEEN.EXE at run time, with size and structural checks. A
  missing or different EXE gives a notice in place of the intro.
- **Clouds intro.** NWC logo with its fire and growl, JVC, stars and the
  Clouds of Xeen title, the lake descent and flight, the tower, the wizard,
  the parchment and Crodo's fourteen narration slots, the closing parchment
  and the title return. The pan stride is measured, not fixed. Speech,
  music and effects play whatever the party's Music, Speech and Efx
  switches say, as in the EXE, and the switches are restored afterwards.
  The caption strip appears only when `XEEN.CFG` names no digital device.
- **Input.** Escape ends the intro at its scene polls through the closing
  parchment; a key while the parchment opens completes it and the intro
  goes on; Space and mouse clicks do nothing, and the pointer is hidden.
- **Title lightning** is unreachable on the CD title (`[DS:0x0095]` is set
  when `xeen.cc` opens), so the title is unchanged.

## Key decisions

- **Startup and the Darkside intro** (maintainer, 2026-10-25): every CD
  launch runs XEEN.EXE's Darkside path first (full story until watched,
  then a short logo sequence). Part 0 estimated 8-13 developer days, so it
  became [roadmap step 20](roadmap.md#next-arc-finishing-the-games-systems),
  after the Clouds ending. Until then MMModern opens at the World title and
  Other Options `D` keeps its notice.
- **EXE over the pin.** The original tables have 31-entry rows where the
  pin regroups them as 32; waits are BIOS holds, not 50 Hz frames; only
  Escape skips. Recorded in [original quirks](original-quirks.md#sv-clouds-intro-presentation)
  and [ScummVM differences](scummvm-differences.md).
- **Song.** MMModern plays the CD promus driver, so the intro uses
  `mm4theme.m`; the device-specific variants belong to step 19.

## Evidence

Calibrated static analysis of XEEN.EXE and XEEN.DAT in
[cutscene rules](rules/cutscenes.md), with the maintainer's DOSBox
recordings in [evidence](rules/evidence.md#m91-clouds-intro-runtime-checks).
The Other Options hotkeys are `D`, `C` and, after a win, `V`; the plan's
first draft named `j`/`k` from a synthetic test fixture.

## Results

- Complete suite 307/307 (run `20261025-052431-b0650419`); new
  `xeen_cutscene` original-data and unit cases, title and application tests.
- On a fake clock every intro stage lands within about half a second of
  the DOSBox recording (stars to hands 33.5 s against 33.4 s; Crodo 33.9 s
  against 34.4 s).
- Maintainer play-test (2026-10-25): the whole intro, its music and speech,
  Escape at both points, Space and clicks, `D`'s notice and the restored
  sound switches.
