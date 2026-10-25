# Milestone 91 plan - Cutscene presenter and the Clouds intro

**Tier B. Status: planned, not started.** Roadmap
[step 17](roadmap.md#next-arc-finishing-the-games-systems), after
[M89](milestone-89-plan.md); it may run while [M90](milestone-90-plan.md)
waits for its whole-quest play-through. Baseline `b85f34a`. Save v20,
schema 9 and content 14 unchanged; no settings file is written (the won
state belongs to step 18).

## Goal

Other Options plays the Clouds intro as the CD's `WORLD/XEEN.EXE` does,
with its screens, music, CD speech, text and skip keys, then returns to the
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
  program: none of its routines has been calibrated yet. The only earlier
  look (M82, `docs/rules/audio.md`, "DOS FM effect call sites") found no
  calls into the DAT's Efx wrapper.
- **Pin.** `engines/mm/xeen/worldofxeen/clouds_cutscenes.cpp`
  (`showCloudsIntro` = `showCloudsTitle` + `showCloudsIntroInner`, about
  350 lines; the ending about 600) and `darkside_cutscenes.cpp` reimplement
  the scenes on 50 Hz frames with ScummVM subtitles and a `seen_intro`
  auto-start. They guide Part 0; where XEEN.EXE says otherwise, the EXE
  wins.
- **MMModern today.** The CD World title (`world0/1/2`, 13-frame loop) is
  `XeenTitleFlow`; Other Options lists the two intros (keys `j`/`k`) and
  answers "not supported yet". MMModern links only ScummVM's CC archive and
  sprite code, so the presenter is ours. Existing pieces to reuse:
  `XeenAudio` (songs, `startDigital`/`digitalBusy`, CD cues), the sprite
  bridge, the title's retrace-paced palette fade, the 22-step scroll
  (`drawXeenScroll`, paced in `XeenInnFlow`; DAT `0xafd5..0xb01c`),
  `PresentationClock` BIOS ticks, and `XeenDosText`'s way of reading text
  and tables from the original program at run time with structural checks.
- **Title lightning.** DAT `0xbf90..0xbf99` tests `[DS:0x0095]`; nonzero
  jumps to `0xc056` past the 30-frame sequence (`elect.voc` start `0xbfcf`,
  stop `0xc029`). The current CD title never runs it, and the flag's writer
  is not traced (`docs/rules/audio.md`, "Title lightning outside M83").
  `XeenDigitalSitesTests` keeps it as the one explicit gap.

## Part 0 - XEEN.EXE calibration and DOS evidence

Read-only analysis of `WORLD/XEEN.EXE` (REA/Ghidra or Capstone; work copies
under `D:/MModern-re/m91/`, never in the repository). Calibrate first: find
the routines the EXE shares with XEEN.DAT (CC lookup and decryption,
sprite draw, palette fade, promus song start, VOC start/busy, keyboard
poll) by matching them against the already-settled DAT routines, then
anchor the intro on the code that references `mm4theme.m` and `crodo1.voc`.
Findings go to a new `docs/rules/cutscenes.md` (listed in
`docs/rules/README.md`) with offsets and evidence labels; every settled
oddity goes to `docs/original-quirks.md`.

- [ ] **Startup and hand-off.** How the EXE starts XEEN.DAT and reads the
  command back; whether any intro plays at program start or on first launch
  (the pin's `seen_intro`); what the DAT does between choosing an intro and
  exiting (music stop, fade, screen); what returns afterwards (a fresh DAT
  start: title fade-in, title song, menu shown or hidden).
- [ ] **Timer.** The EXE's tick source and rate (BIOS 18.2 Hz, a
  reprogrammed PIT, retrace) and how its waits count.
- [ ] **Resources.** Which archive each name comes from (`INTRO.CC`,
  `XEEN.CC`, `DARK.CC`, loose files first?), their formats (`.raw`, `.pal`,
  `.vga`, `.voc`, `.m`), and whether the intro's speech is VOC from an
  archive or CD audio tracks. Every Clouds intro resource decodes with the
  current readers, or the plan names the missing reader.
- [ ] **Clouds intro choreography.** Every screen in order: palette and
  background loads, sprite frames and positions, fades and their steps,
  waits, the story text on the scroll and where that text lives, each song,
  effect and voice and whether the next frame waits for it. Compare with
  the pin's `showCloudsTitle`/`showCloudsIntroInner` and its frame tables
  (`_INTRO_FRAMES_*`); list each difference.
- [ ] **Settings and input.** Whether the EXE honours `XEEN.CFG`'s music,
  speech and effects choices; whether any caption appears when speech is
  off (the pin's subtitles are its own); which keys and clicks skip, and
  whether a skip ends the whole intro or one screen.
- [ ] **Title lightning.** Who writes `[DS:0x0095]` and whether the CD
  World title can ever take the 30-frame path; if it can, its frames,
  timing and the `elect.voc` start and stop.
- [ ] **Darkside intro cost.** Using the same primitives, estimate what the
  `DARKINTRO` scene needs beyond the presenter (new step kinds, resources,
  CD disc 2 speech). The maintainer decides at the end of Part 0 whether it
  joins this milestone; by default it stays "not supported yet".
- [ ] **Ending primitives.** List, without implementing, any step kind the
  Clouds ending (step 18) needs that the intro does not, so the presenter's
  interface does not need a rewrite then.
- [ ] **DOSBox checks** for the maintainer (one session, video, measured
  with ffmpeg change timestamps and 10 fps contact sheets): (a) Other
  Options → Clouds intro to the end with default settings; (b) a key at
  three points (first logo, a story screen, Crodo speaking); (c) Escape and
  a mouse click once each; (d) if Part 0 shows the EXE reads `XEEN.CFG`,
  one run with speech off; (e) what the title looks like on return;
  (f) program start from the launcher, to confirm whether an intro plays.
  Part 0 sends the list before it ends; the maintainer's observations are
  recorded in `docs/rules/evidence.md`.

**Stop and report** when: the EXE's timer cannot be expressed on
`PresentationClock` (a timing change is Tier A); an intro plays at program
start or first launch (scope); speech comes from a source the audio owner
cannot play; a choreography value can be settled neither from the EXE nor
by the DOSBox checks (list them for the maintainer; do not fall back to the
pin silently); or the Darkside estimate is ready.

## Part A - Cutscene presenter

- [ ] **Core** (`src/games/xeen/XeenCutscene.h/.cpp`, no SDL): a cutscene is
  built once as an ordered list of segments. A segment draws a frame
  (background, palette, sprite frames at positions, scroll and text), then
  holds until its end condition: a tick count, the digital sample idle, the
  CD cue idle, the song finished, or a fade step done. Loops in the EXE
  that animate while a voice plays become segments that cycle frames until
  the voice ends. Skip semantics follow Part 0. Step kinds cover the intro
  and the ending primitives Part 0 listed.
- [ ] **Flow** (`src/app/XeenCutsceneFlow.h/.cpp`): a new owner, because
  XEEN.EXE is a separate original program whose screens belong neither to
  the title nor to gameplay, and step 18 reuses it. It paces segments on
  `PresentationClock` at Part 0's rate, publishes frames like
  `XeenTitleFlow`, sends songs, effects, samples and CD cues through the
  existing `XeenAudio` (no second audio path), and reports when it is done.
- [ ] **EXE data.** Tables and text the intro needs are read at run time
  from the installed `WORLD/XEEN.EXE`, as `XeenDosText` reads XEEN.DAT, and
  refused when the bytes are not the analysed build (Part 0's SHA-256 or a
  structural check); code immediates are written in code with offset
  comments. The scroll is reused if Part 0 finds the EXE's copy identical
  to the DAT's; otherwise the EXE's version is a step kind of its own.
  Nothing from the EXE is copied into the repository. A missing or
  different EXE leaves the intro entries with a clear notice and the rest
  of the game unaffected.
- [ ] **Routing.** `Application` hands the title to the presenter and, when
  it finishes or is skipped, rebuilds the title as Part 0's return state
  says. Escape never quits (as since v0.7.1). The game RNG, sessions and
  saves are untouched.

## Part B - Clouds intro and title lightning

- [ ] The Clouds intro (`XEENINTRO`) as Part 0 describes it, from Other
  Options `j`; `k` (Darkside) keeps its notice unless the maintainer added
  it after Part 0.
- [ ] Title lightning: reproduce it in `XeenTitleFlow` if Part 0 finds the
  CD title can reach it; otherwise record it as unreachable on the CD
  (`docs/rules/audio.md` row and `XeenDigitalSitesTests`'s gap status).
- [ ] Captions only where DOS shows them; the pin's subtitles and
  `seen_intro` auto-start, if DOS lacks them, become `**ScummVM:**` quirk
  entries and `docs/scummvm-differences.md` items.
- [ ] `docs/controls.md`: the intro's skip keys.

## Tests

- [ ] Unit (`tests/XeenCutsceneTests.cpp`): segment ordering, tick holds
  against a fake clock, holds on sample/CD/song idle, fade pacing, skip
  semantics, the presenter done signal.
- [ ] Original data: every Clouds intro resource decodes; the whole intro
  runs on a fake clock to its end with Part 0's tick total (voice holds
  counted with the real sample lengths) and the audio requests in Part 0's
  order (a counting audio sink, as `XeenInnTests` does); a skip at three
  points returns to the title; a wrong EXE hash gives the notice.
- [ ] `XeenTitleFlowTests`: Other Options `j` starts the intro, the return
  state matches Part 0; the lightning, if reproduced.
- [ ] Complete CTest suite green on `build-rel`; M44 digests unchanged.

## Acceptance

- Other Options plays the Clouds intro and returns to the title as the
  DOSBox videos show; the skip keys behave as in DOS.
- The presenter is one owner whose interface already covers step 18's
  primitives; the title lightning is reproduced or closed with evidence.
- `docs/rules/cutscenes.md` records the calibrated EXE evidence; quirks and
  ScummVM differences are recorded.
- Complete suite green; the maintainer compares MMModern's intro with the
  DOSBox video and accepts it.
