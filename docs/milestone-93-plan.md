# Milestone 93 plan - Startup and the Darkside intro

**Tier B. Status: Parts 0, A and B done; awaiting closure.** Roadmap
[step 20](roadmap.md#next-arc-finishing-the-games-systems), after
[M92](milestone-92-plan.md), on the [M91](milestone-91-plan.md) presenter.
Baseline `87fe933`. Save v21, schema and content unchanged. The settings
file gains the Darkside intro's seen state (maintainer, 2026-10-26).

## Goal

Every player launch (`mmodern <game-dir>`) starts as the CD's
`WORLD/XEEN.EXE` does: the full Darkside story until it has been watched
through, afterwards NWC, JVC and the World-logo tail, then a fresh World
title. Other Options `D` forgets the seen state and plays the full story.
Screens, music, VOC narration, captions, random visual variants and keys
follow the EXE.

## What is known

- **Routing and seen state** ([startup dispatch](rules/cutscenes.md#startup-dispatch)):
  no `XEENCOMMAND` enters the Darkside routine `0xa9bd..0xd608`. The word at
  `XEEN.CFG+0x1a` chooses full story (0) or NWC/JVC then the World-logo tail
  (nonzero, `0xad3d..0xad64`). Both paths store 1 and write CFG at
  `0xd345..0xd374`, before the World logo. Escape before that store leaves
  it unchanged. DAT `D` clears it and writes CFG before handing off
  (`0xc571..0xc59e`). Completion and Escape return to the fresh title of
  [hand-off and title return](rules/cutscenes.md#hand-off-and-title-return).
- **Content** ([Darkside boundaries](rules/cutscenes.md#darkside-and-ending-boundaries),
  estimate in `D:/Projetos/MModern/MModern-re/m91/findings.md` item 7):
  NWC dragon and JVC; pyramid approach and Pharaoh room; crystal-ball
  vignettes; flying creature, comet and fall; World-logo tail. Mostly
  INTRO.CC, with temporary DARK.CC switches (`0xbb48`, `0xcf38`, `0xd08f`)
  and their restores. About 80 resource names or templates, including
  `.int` resources that no current reader has been checked against. VOC
  narration only; no CD track request.
- **New beyond Clouds:** horizontal page operations (M92 added
  `HorizontalMerge`), more caption records, archive switching, and random
  visual variants through range helper `0x1cbdc` on time-mixed source
  `0x1cc27`, which M92's `XeenPresentationRandom` already reproduces.
- **Input:** Escape at NWC and story polls goes to common cleanup
  (`0xab4d..0xab57`, `0xd5b5`). World-logo loops ignore keys; any key only
  shortens the two boundary holds (`0xd444..0xd458`, `0xd579..0xd58d`). The
  mouse never skips; Ctrl-Q quits from the EXE presentation.
- **DOSBox:** `D:/Projetos/MMODERN videos/a.intro original + intro clouds
  menu.avi` already holds a seen-0 launch with the full story (0 to about
  135 s) and the World title return.
- **Pin:** `showStartup` and `darkside_cutscenes.cpp` keep `seen_intro` in
  ScummVM's configuration and set it after the World logo completes. Part 0
  compares each difference; the EXE wins.
- **MMModern today:** `Application::run` opens the World title directly;
  `D` gives a notice. `platform/XeenWonStore` writes `settings.ini` with
  the exact content `won=1`, and its reader rejects anything else.

## Part 0 - Darkside calibration and DOS evidence

Read-only analysis of `WORLD/XEEN.EXE` with the M91 calibration (work
copies in `D:/Projetos/MModern/MModern-re/m93/`, nothing original in the
repository). Findings extend [cutscene rules](rules/cutscenes.md) with
offsets and evidence labels; settled oddities go to
[original quirks](original-quirks.md).

- [x] **Sequencing rules** for each scene family: backgrounds, palettes,
  sprite frames and positions, scales, vertical and horizontal page
  operations, fades and steps, BIOS holds and their reset points, redraws
  limited by retrace, voice-idle loops, caption records, songs, FM cues,
  VOCs and mixer commands. Record them as rules, as M91 did, not as a
  transcription of the program.
- [x] **Paths:** the short path's NWC/JVC against the full path's; the jump
  to `0xd345`; the World-logo tail; what is on screen and playing before the
  first frame at process start, and after Escape or completion.
- [x] **Resources:** the archive behind every reached name across the
  switches; every resource decodes with the current readers (sprites, RAW,
  PAL, VOC, songs, compressed frames), or the missing reader is named with
  its format evidence. Song selection by music device as in Clouds
  (`0x7ce9..0x7d23`); device-specific variants stay in step 19.
- [x] **Random visual variants:** every call site, its range and what it
  selects. Fidelity means the same helper and source, not matching one
  recording.
- [x] **Captions:** records used, the no-digital-device predicate and strip
  pacing, compared with M91 and M92.
- [x] **EXE data:** tables, coordinates and text the scenes need, read at
  run time with structural checks, as `XeenCutsceneData` does today.
- [x] **DOSBox checks** for the maintainer: one session on a disposable copy
  of the installation, recorded and measured with ffmpeg timestamps and
  timestamped contact sheets. (a) Relaunch after the recorded full story:
  short path to the title. (b) Title, Other Options `D`: full story forced.
  (c) `D` again and Escape during the story, before the World logo; relaunch
  to confirm the full story is still due. (d) Space, then Escape, during the
  World-logo frames; one left click during the story. (e) Optional, if the
  installer offers it: no digital device, to see the captions. The existing
  full-story video provides the timing of the scenes. Part 0 sends the list
  before it ends; observations go to [evidence](rules/evidence.md).

**Stop and report** if the EXE's timing cannot be expressed on
`PresentationClock` (Tier A); a resource needs a reader larger than one new
decoder; speech needs a source `XeenAudio` cannot play; DOSBox contradicts
the seen-state routing above; or a choreography value is settled neither by
the EXE nor by the checks (list them; never fall back to the pin silently).

**Part 0 result (2026-10-26):** no stop condition. The rules are in
[cutscene rules](rules/cutscenes.md#darkside-sequencing-rules) and the
maintainer's session in
[evidence](rules/evidence.md#m93-darkside-startup-runtime-checks). No new
reader is needed (`.int` resources are ordinary sprites). The installed
configuration selects `bigtheme.m`. The short path stops the JVC song.
Stage timing is still to be compared with the recordings; captions without
a digital device were not recorded.

## Part A - Darkside intro on the presenter

- [x] Extend `XeenCutsceneData` with the Darkside tables and resource names,
  refused with a clear message when the installed EXE is not the analysed
  build.
- [x] Archive switching inside one cutscene: a program archive for INTRO.CC
  with the temporary DARK.CC switches Part 0 finds, next to
  `CloudsProgram`/`CloudsEnding`.
- [x] `XeenCutscene::darkside(data, captions, clock, path)` in
  `games/xeen/XeenDarksideIntro.cpp` (as `XeenCloudsEnding.cpp`), for the
  full and the short path. New composition or hold kinds only where Part 0
  needs them; random variants use `XeenPresentationRandom`; no Journey RNG.
- [x] The seen store at the `0xd345` point: the cutscene reports when it
  passes it, so Escape before it leaves the state unchanged.

**Part A result (2026-10-26):** the full and short paths run on the M91
presenter with no second clock or audio path; the cutscene reports the
seen point and stores nothing. Until Part B, Other Options `D` plays the
full path and returns to a fresh title; a failure returns to the title
with a notice. The maintainer's play-test of `D`, Escape, a click, the
World-logo keys, `C` and Ctrl-Q matched DOS. Complete suite 339/339
(run `20261026-201308-b3d15592`).

## Part B - Startup, settings and Other Options D

- [x] **Settings file.** `platform/XeenWonStore` becomes the settings store:
  one `key=value` per line, `won` and `darkside_intro_seen`, each 0 or 1.
  The reader still accepts M92's `won=1` file; a write replaces the whole
  file atomically and keeps the other key. A missing file means not won and
  not seen. A failed write is silent, and the full story plays again at the
  next launch. Builds before M93 read a file with both keys as not won;
  recorded in the release notes at closure. The original `XEEN.CFG` is never
  written.
- [x] `XeenProgramState` carries the seen state beside `cloudsWon`.
- [x] **Startup.** `Application::run` plays the Darkside path before the
  first title, then the fresh-title return as after the Clouds intro.
  Developer entry modes and the title's other returns (from gameplay, intro
  or ending) do not. Without the analysed EXE or its archives, startup goes
  straight to the title with a diagnostic on standard error.
- [x] **Other Options D** clears the seen state, writes it, leaves the title
  as `C` does and plays the full story; without the EXE it keeps its notice.
- [x] Captions, mixer commands, song fade and Ctrl-Q as in the Clouds intro
  unless Part 0 says otherwise; the pointer stays hidden.
- [x] Docs: `docs/controls.md` (startup keys, `D`); quirks and
  [ScummVM differences](scummvm-differences.md) for every difference from
  the pin, including where the seen state is set; the screen checklist row
  in `tools/XeenCoverageScreens.h`.

**Part B result (2026-10-26):** `settings.ini` holds `won` and
`darkside_intro_seen`; the M92 `won=1` file still reads as won. A player
launch plays the full story while the state is 0 and the short path
afterwards, storing 1 at the seen point; developer modes and title
returns skip it. `D` clears the state in the file before the full story.
The maintainer's play-test of the first launch, relaunch, `D` with an
early Escape, Ctrl-Q in the World logo and the file contents matched the
plan. Complete suite 339/339 (run `20261026-233304-b8f66e74`).

## Tests

- [x] Settings store: the M92 file, both keys, keeping the other key on
  write, a damaged file, a failed write.
- [x] Startup routing: seen 0 plays the full path and stores 1; seen 1 plays
  the short path; Escape before the store keeps 0; Escape after it keeps 1;
  `D` clears and plays the full path; developer modes skip startup.
- [x] Original data (`xeen_cutscene`): every reached Darkside resource
  decodes; both paths run to the end on a fake clock with real sample
  lengths, and the stage times fall within the DOSBox brackets; audio
  requests in Part 0's order; random sites draw from an injected clock;
  input at the polls Part 0 lists.
- [x] Existing title and application tests set the seen state they rely on.
- [x] Complete CTest suite green on `build-rel`; M44 digests unchanged.

## Acceptance

- A first launch plays the full Darkside story and a later launch the short
  path, as the DOSBox recordings show; `D` and Escape behave as in DOS; the
  seen state survives restarts in `settings.ini`.
- The intro runs on the M91 presenter with no second clock or audio path.
- Cutscene rules, evidence, quirks and ScummVM differences are recorded.
- Complete suite green; the maintainer compares MMModern with the DOSBox
  recordings and accepts it.
