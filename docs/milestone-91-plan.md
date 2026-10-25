# Milestone 91 plan - Cutscene presenter and the Clouds intro

**Tier B. Status: planned, not started.** Roadmap
[step 17](roadmap.md#next-arc-finishing-the-games-systems), after
[M89](milestone-89-plan.md); it may run while [M90](milestone-90-plan.md)
waits for its whole-quest play-through. Baseline `b85f34a`. Save v20,
schema 9 and content 14 unchanged; no settings file is written (the won
state belongs to step 18).

## Goal

Other Options plays the Clouds intro as the CD's `WORLD/XEEN.EXE` does,
with its screens, music, speech, text and skip keys, then returns to the
title as DOS does. The intro runs on one cutscene presenter that step 18's
Clouds ending, and later the Darkside intro and endings, reuse unchanged.
The title lightning (`elect.voc`, DAT `0xbfcf`) is either reproduced or
closed as unreachable on the CD title.

## What is known

- **Hand-off.** XEEN.DAT never draws a cutscene. Other Options stores
  `XEENCOMMAND=XEENINTRO` (or `DARKINTRO`), the ending stores
  `ENDGAME,<score>,<state>` (DAT strings `0x521c9..0x5223d`), and the DAT
  exits to `WORLD/XEEN.EXE`, which plays the scene and starts the DAT again
  with `XEENCOMMAND=MAINGAME` (completion audit §4.3,
  `D:/MModern-re/clouds-completion/findings.md`).
- **XEEN.EXE** (137,737 bytes, SHA-256 `c5ade865…a235`, not packed: its
  resource names are plain strings) holds every Clouds intro and ending
  resource name: `logobak.raw`, `logo.vga`, `fire.voc`, `meangro&.voc`,
  `jvc.raw`, `mm4theme.m`, `stars.vga` ... `box.vga`, `crodo1.voc` ...
  `crodo6.voc`, `king1-3.voc`, `mm4e.pal` and the rest. It is a separate
  program: its shared primitives and startup dispatch are now
  [calibrated](rules/cutscenes.md#evidence-and-calibration), while complete
  Clouds sequencing is recorded in that chapter. The earlier look
  (M82, `docs/rules/audio.md`, "DOS FM effect call sites") found no
  calls into the DAT's Efx wrapper.
- **Pin.** `engines/mm/xeen/worldofxeen/clouds_cutscenes.cpp`
  (`showCloudsIntro` = `showCloudsTitle` + `showCloudsIntroInner`, about
  350 lines; the ending about 600) and `darkside_cutscenes.cpp` reimplement
  the scenes on 50 Hz frames with ScummVM subtitles and a `seen_intro`
  auto-start. They guide Part 0; where XEEN.EXE says otherwise, the EXE
  wins.
- **MMModern today.** The CD World title (`world0/1/2`, 13-frame loop) is
  `XeenTitleFlow`; Other Options lists the two intros (keys `D` Darkside
  and `C` Clouds, read from DAT `0xc2d1`/`0xc2ea`) and answers "not
  supported yet". MMModern links only ScummVM's CC archive and
  sprite code, so the presenter is ours. Existing pieces to reuse:
  `XeenAudio` (songs, `startDigital`/`digitalBusy`, CD cues), the sprite
  bridge, the title's retrace-paced palette fade, the 22-step scroll
  (`drawXeenScroll`, paced in `XeenInnFlow`; DAT `0xafd5..0xb01c`),
  `PresentationClock` BIOS ticks, and `XeenDosText`'s way of reading text
  and tables from the original program at run time with structural checks.
- **Title lightning.** DAT `0xbf90..0xbf99` tests `[DS:0x0095]`; nonzero
  jumps to `0xc056` past the 30-frame sequence (`elect.voc` start `0xbfcf`,
  stop `0xc029`). MMModern's current CD title never runs it. DAT `0xb3d5`
  sets the flag when `xeen.cc` can be opened, making lightning unreachable
  in installed CD World ([evidence](rules/cutscenes.md#title-lightning)).
  Part B still needs to close `XeenDigitalSitesTests`'s explicit gap.

## Part 0 - XEEN.EXE calibration and DOS evidence

**Startup and Darkside intro: decided after Part 0.** A launcher start
without `XEENCOMMAND` enters Darkside before the DAT (`0x7a45..0x7a99`,
entry `0xa9bd`). `XEEN.CFG +0x1a` selects full story or a shorter title/logo
path; completion sets it and Other Options `D` clears it. DOS therefore has
persistent seen-intro behavior. See
[startup dispatch](rules/cutscenes.md#startup-dispatch). The maintainer
decided after the item 7 estimate (2026-10-25) that the startup path and the
Darkside intro stay outside M91 and become their own milestone after the
Clouds ending ([roadmap step 20](roadmap.md#next-arc-finishing-the-games-systems)).
Until then MMModern starts at the World title and Other Options `D` keeps
its notice.

Read-only analysis of `WORLD/XEEN.EXE` (REA/Ghidra or Capstone; work copies
under `D:/MModern-re/m91/`, never in the repository). Calibrate first: find
the routines the EXE shares with XEEN.DAT (CC lookup and decryption,
sprite draw, palette fade, promus song start, VOC start/busy, keyboard
poll) by matching them against the already-settled DAT routines, then
anchor the intro on the code that references `mm4theme.m` and `crodo1.voc`.
Findings go to a new `docs/rules/cutscenes.md` (listed in
`docs/rules/README.md`) with offsets and evidence labels; every settled
oddity goes to `docs/original-quirks.md`.

- [x] **Startup and hand-off.** Environment commands travel both ways;
  DAT cleanup and fresh title return are traced. The return starts
  `newbrigh.m`, fades in by four and waits for input before showing the menu.
- [x] **Timer.** BIOS 18.2065 Hz elapsed counters and separate retrace waits
  fit `PresentationClock`. Resource loads, measured pan stride and voice idle
  prevent a universal fixed tick total.
- [x] **Resources.** Loose files precede the selected archive; Clouds uses
  XEEN.CC after INTRO/DARK startup allocations. VOC speech, no CD tracks.
  A source-equivalent audit found no missing reader; compiled-reader and
  rendered/audio validation remain implementation tests, not Part 0 results.
- [x] **Clouds intro choreography.** Complete sequence, coordinates, frame
  tables and pin differences are in `D:/MModern-re/m91/findings.md`.
  [Repository rules](rules/cutscenes.md#clouds-sequencing-rules) retain
  semantics and offsets. DOS mouth/wait tables use 31 entries per row.
- [x] **Settings and input.** EXE honours hardware choices in `XEEN.CFG`,
  not DAT party Music/Speech/Efx/Text gates. No-digital-device configurations
  show the `special.bin` caption strip. Escape ends the scene at polls;
  any key finishes only a scroll; mouse does not skip; Ctrl-Q quits globally.
- [x] **Title lightning.** Readable XEEN.CC sets DAT `DS:0095`, bypassing
  the 30-frame path on the installed CD World title.
- [x] **Darkside intro cost (item 7).** Full scene plus first/repeat/skip
  startup is estimated at 8–13 developer days after the Clouds presenter,
  plus maintainer play-test. Same VOC source; the earlier disc-2 speech
  assumption is corrected. Horizontal page composition, visual randomness,
  more caption records and persistent seen state add work. The estimate is
  ready; the maintainer moved Darkside/startup to roadmap step 20.
- [x] **Ending primitives.** Compressed full-screen frames, horizontal
  merge/rotation, dynamic monster/attack animation, score reveal and terminal
  acknowledgement holds; exact ending choreography remains outside Part 0.
- [x] **DOSBox checks** (maintainer, 2026-10-25), recorded in
  [M91 runtime checks](rules/evidence.md#m91-clouds-intro-runtime-checks):
  (a) the whole Clouds intro from Other Options `C` and the return to the
  title; (b) Space at the logo and during Crodo's speech changes nothing;
  (c) Escape during speech ends the intro through the closing parchment,
  Escape during the parchment opening only completes it, a click does
  nothing; (e) half a second of black, then the title fades in without its
  menu. (d), the caption strip without a digital device, was skipped:
  MMModern always has digital sound. (f) belongs to the startup/Darkside
  decision; one launch with the seen word at zero showed the full Darkside
  story. The realized pan stride is not measured; Part A takes it from the
  recordings if the formula needs a cycle-dependent value.

**Stop and report** when: the EXE's timer cannot be expressed on
`PresentationClock` (a timing change is Tier A); speech comes from a source the audio owner
cannot play; a choreography value can be settled neither from the EXE nor
by the DOSBox checks (list them for the maintainer; do not fall back to the
pin silently); or the Darkside estimate is ready.

## Part A - Cutscene presenter

- [ ] **Core** (`src/games/xeen/XeenCutscene.h/.cpp`, no SDL): a cutscene is
  built once as an ordered list of segments. A segment draws a frame
  (background, palette, sprite frames at positions, scroll and text), then
  holds until its end condition: elapsed BIOS ticks preserving reset phase,
  digital sample idle, an input rule, or retrace-paced fade steps. Loops in the EXE
  that animate while a voice plays become segments that cycle frames until
  the voice ends. Skip semantics follow Part 0. Implement only the kinds the
  Clouds intro needs. Part 0's ending kinds fit as later composition operations,
  frame generators and end conditions without reworking the segment model;
  compressed frames, horizontal merge/rotation, dynamic monster/attack animation,
  score reveal and acknowledgement holds are not implemented in M91.
- [ ] **Flow** (`src/app/XeenCutsceneFlow.h/.cpp`): a new owner, because
  XEEN.EXE is a separate original program whose screens belong neither to
  the title nor to gameplay, and step 18 reuses it. It paces segments on
  `PresentationClock` at Part 0's rate, publishes frames like
  `XeenTitleFlow`, sends songs, FM effects and VOC samples through the
  existing `XeenAudio` (no second audio path), and reports when it is done.
- [ ] **EXE data.** Tables and text the intro needs are read at run time
  from the installed `WORLD/XEEN.EXE`, as `XeenDosText` reads XEEN.DAT, and
  refused when the bytes are not the analysed build (Part 0's SHA-256 or a
  structural check); code immediates are written in code with offset
  comments. Reuse the DAT scroll geometry with the EXE's key-only skip policy,
  opening fade and terminal-image behavior; its input policy is not identical
  to the DAT's.
  Nothing from the EXE is copied into the repository. A missing or
  different EXE leaves the intro entries with a clear notice and the rest
  of the game unaffected.
- [ ] **Routing.** `Application` hands the title to the presenter and, when
  it finishes or is skipped, rebuilds the title as Part 0's return state
  says. Escape never quits (as since v0.7.1). The game RNG, sessions and
  saves are untouched.

## Part B - Clouds intro and title lightning

- [ ] The Clouds intro (`XEENINTRO`) as Part 0 describes it, from Other
  Options `C`; `D` (Darkside) keeps its notice unless the maintainer added
  it after Part 0.
- [ ] Title lightning: reproduce it in `XeenTitleFlow` if Part 0 finds the
  CD title can reach it; otherwise record it as unreachable on the CD
  (`docs/rules/audio.md` row and `XeenDigitalSitesTests`'s gap status).
- [ ] Captions follow the hardware-device predicate and original caption
  record. Preserve the [documented pin differences](original-quirks.md#sv-clouds-intro-presentation).
  DOS does have persistent seen-intro behavior; startup belongs to roadmap
  step 20.
- [ ] `docs/controls.md`: the intro's skip keys.

## Tests

- [ ] Unit (`tests/XeenCutsceneTests.cpp`): segment ordering, tick holds
  against a fake clock, digital-idle loops, fade pacing, skip
  semantics, the presenter done signal.
- [ ] Original data: every Clouds intro resource decodes; the whole intro
  runs on a fake clock with declared resource readiness, frame-publication
  times and real sample lengths; verify reset points, measured pan stride,
  holds and the audio request order (a counting sink, as `XeenInnTests` does); a skip at three
  points returns to the title; a wrong EXE hash gives the notice.
- [ ] `XeenTitleFlowTests`: Other Options `C` starts the intro, the return
  state matches Part 0; the lightning, if reproduced.
- [ ] Complete CTest suite green on `build-rel`; M44 digests unchanged.

## Acceptance

- Other Options plays the Clouds intro and returns to the title as the
  DOSBox videos show; the skip keys behave as in DOS.
- The presenter is one owner whose segment model admits step 18's added
  kinds without reworking it; title lightning is reproduced or closed with evidence.
- `docs/rules/cutscenes.md` records the calibrated EXE evidence; quirks and
  ScummVM differences are recorded.
- Complete suite green; the maintainer compares MMModern's intro with the
  DOSBox video and accepts it.
