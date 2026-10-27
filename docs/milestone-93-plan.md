# Milestone 93 plan - Startup and the Darkside intro

**Tier B. Status: completed and accepted** by the maintainer on 2026-10-27
after the play-tests. Plan `e38d851`, Part 0 `debbe9a`, Part A `4184954`,
Part B `18bf556` and the closure commit. Roadmap
[step 20](roadmap.md#next-arc-finishing-the-games-systems), after
[M92](milestone-92-plan.md), on the [M91](milestone-91-plan.md) presenter.
Save v21, schema and content unchanged.

## Goal

Every player launch (`mmodern <game-dir>`) starts as the CD's
`WORLD/XEEN.EXE` does: the full Darkside story until it has been watched
through, afterwards NWC, JVC and the World-logo tail, then a fresh World
title. Other Options `D` forgets the seen state and plays the full story.

## Key decisions

- **The EXE is the authority.** Calibrated analysis of `WORLD/XEEN.EXE`
  set the sequencing, resources, random speaking frames, captions, input
  and the seen point (`0xd345`); a DOSBox session (short path, `D`, early
  Escape, World-logo keys, a click) confirmed every prediction
  ([cutscene rules](rules/cutscenes.md#startup-dispatch),
  [evidence](rules/evidence.md#m93-darkside-startup-runtime-checks)).
  Where the pin differs, [quirks](original-quirks.md#sv-darkside-startup-seen)
  and [ScummVM differences](scummvm-differences.md) record it.
- **Seen state in `settings.ini`** (maintainer, 2026-10-26): the key
  `darkside_intro_seen` sits beside `won`; the original `XEEN.CFG` is never
  written. The reader still accepts M92's `won=1` file. No release wrote
  `settings.ini` before M93, so no release note is needed.
- **One presenter.** The intro reuses the M91 presenter, clock and audio
  path; random frames use M92's `XeenPresentationRandom`, never Journey RNG.
  No new reader was needed (`.int` resources are ordinary sprites).
- **Playable by default.** A missing EXE or archive skips startup with a
  diagnostic; a failure during the intro returns to the title with a notice.

## Results

- **Part 0:** Darkside rules, resource report (45 sprites with 470 frames,
  36 VOCs, 15 RAWs, 3 PALs, 2 songs) and the DOSBox session. The installed
  configuration selects `bigtheme.m`; the short path stops the JVC song.
- **Part A:** `XeenCutscene::darkside` in `games/xeen/XeenDarksideIntro.cpp`
  plays the full and short paths with archive switches between INTRO.CC
  and DARK.CC, boundary holds that only a key shortens, and Escape at the
  story's polls. Stage times land within the DOSBox brackets.
- **Part B:** startup in `Application::run` chooses the path from the seen
  state, stores 1 at the seen point and leaves developer modes and title
  returns alone; `D` clears the state before the full story.
- Complete suite 339/339 (run `20261026-233304-b8f66e74`); M44 digests
  unchanged. Captions without a digital device were not recorded in DOSBox.

## Acceptance

The maintainer compared `D`, Escape, a click, the World-logo keys, `C`,
Ctrl-Q, the first launch, the relaunch and the `settings.ini` contents with
DOS and the recordings, and accepted the milestone.
