# Milestone 116 plan - Picture filters

**Tier B. Status: planned (2026-11-10).** Second of the opt-in extras of
[later goal 6](roadmap.md#later-goals), after M115. Baseline: `c83ff6d`
(release 0.19.0), equal to `origin/main`. Save v26, Journey schema and
content unchanged: the filter is a host window setting, never game state.
With the filter off, which is the default, nothing changes.

Tier note: the filters come from the `libgraphics.a` MMModern already
links. If Part 0 shows that they need another ScummVM artifact, another
source file compiled directly or a configuration change, that is ScummVM
integration (Tier A): stop and ask the maintainer before going on.

## Goal

A player who wants it can soften the enlarged 320x200 picture with one of
the classic pixel-art filters of ScummVM and DOSBox (AdvMame, HQ, SaI and
their relatives), chosen in the launcher or the setup screen, or switched
with one key while playing. The default stays today's picture: sharp pixels
in the 4:3 shape.

## Evidence

- The original has no picture filter: it draws 320x200 in VGA mode 13h, so
  any filter is an approved deviation by nature, like
  [square pixels](original-quirks.md#dev-square-picture).
- The pinned ScummVM offers these filters as its "scalers". They are already
  built into the linked library on both systems (`USE_SCALERS`,
  `USE_HQ_SCALERS` and `USE_EDGE_SCALERS` are set in the Windows build's
  `config.h` and in `.github/ci/scummvm-use-linux.txt`). They run on the
  CPU, so they need no OpenGL and carry over to Android.
- F11 and F12 produce no code anywhere in the original
  ([controls](controls.md); `SdlWindow.cpp`, the key translation's default
  case), so the game never sees either. F12 is Steam's default screenshot
  key, which leaves F11.

## Decisions (maintainer, 2026-11-10)

1. **The classic ScummVM filters only.** Out of scope: whole-picture
   smoothing (bilinear), scanline filters (TV2x, DotMatrix) and CRT shaders.
   A CRT shader would need the picture drawn through OpenGL on all three
   systems and is a milestone of its own if it is ever wanted.
2. **Three ways to switch:** a list in the Windows launcher, a row in the
   Linux setup screen, and a key while playing that the game does not use
   in any way.
3. **Both picture shapes.** The filters work with the default `aspect=4:3`
   and with `aspect=square`.

## Decisions taken with the plan

The maintainer delegated these on 2026-11-10 and reviews them at the
play-test.

1. **The list.** Every scaler of the pin except Normal, TV2x and DotMatrix,
   under its ScummVM name: `advmame2x`, `advmame3x`, `advmame4x`, `hq2x`,
   `hq3x`, `edge2x`, `edge3x`, `sai2x`, `supersai2x`, `supereagle2x` and
   `pm2x`, plus `off`. Part 0 may drop one that fails its checks; the plan
   is amended to say which and why.
2. **The key is F11.** It selects the next filter of the list, `off`
   included, and wraps around. It works wherever the window shows a picture
   (title, intros, play, text entry, the setup screen), makes no gameplay
   pass, and the game never receives it, like Alt+Enter. No controller
   button sends it; Steam Input can bind one to F11.
3. **The setting.** `filter=<name>` in `window.ini`, `off` by default;
   unknown values and older files mean `off`. The player entry saves the
   choice F11 makes; developer entries read the setting but do not save it,
   as with the rest of the window placement.

## Design

- **Owner.** `SdlWindow` presents the picture and gains the filter stage;
  the setting is one more field of `WindowPlacement`. No new owner.
- **Where the filter sits.** The game's 320x200 frame is filtered into a
  texture two, three or four times as large, and that texture takes the
  frame's place in the existing shape path: enlarged by a whole number
  without smoothing, then fitted to the picture rectangle with M99's sharp
  scaling. With `aspect=square` and a filter on, the picture keeps its
  whole-multiple rectangle but is drawn through that same path, so a 2x
  filter in a 3x window has no uneven pixels. With the filter off, both
  shapes draw exactly as today.
- **Geometry does not change.** The picture rectangle and the mappings
  between window and frame coordinates keep working on the 320x200 frame,
  so clicks, the pointer, the controller pointer and button feedback are
  untouched.
- **The original cursor is not filtered.** It is drawn after the filter, as
  today. In the pin most of these scalers do not filter the cursor either
  (`canDrawCursor` is false for all of them but AdvMame).
- **ScummVM use.** The scaler classes (`graphics/scaler/*.h`) are created
  directly; ScummVM's plugin manager is not started. The source frame gets
  the border the scalers read beyond (`extraPixels`).
- **Timing.** Filtering runs on the loop's thread after the frame is
  acquired. It must never move a presentation deadline: Part 0 measures
  each filter on the development PC and on the Steam Deck against the
  loop's fastest cadence. A frame is filtered again only when the frame or
  the filter changed.
- **Tests read the game's frame.** Frame digests and captures stay before
  the filter, so no existing expectation changes.

## Part 0 results

All eleven scalers link from the already linked `libgraphics.a` (no other
ScummVM artifact, no source compiled directly) and give identical output on
Windows and on WSL Linux; none was dropped. Edge 2x and 3x cost 25-41 ms per
new frame on the development PC, against at most about 11 ms for the others.
They are kept because presentation deadlines come from the game's clock, so a
slow filter only delays drawing, never game time; the Steam Deck check is
part of the play-test.

## Tasks

Part 0 - checks before production code:

- [x] A probe links each proposed scaler from the existing `libgraphics.a`
      and runs it on a fixed 32-bit frame, on Windows and on WSL Linux;
      record factors, the border each needs and whether the result is the
      same on both systems.
- [x] Measure the time per frame of each filter on the development PC and
      the Steam Deck; drop or report any that cannot hold the cadence.
- [x] Render one play frame and one text-heavy frame through every filter
      and inspect them (AGENTS.md, play-tests: evidence first).

Part A - the filter:

- [x] `filter` in `WindowPlacement`: parse, format, defaults.
- [x] The filter stage in `SdlWindow`, both shapes, renderer resets and
      output size changes included.
- [x] F11 in the loop beside Alt+Enter; saves the choice in the player
      entry.

Part B - choosing it and documents:

- [x] Windows launcher: a list that writes only the `filter` line and keeps
      the others, as the controller box does.
- [x] Linux setup screen: a row with the same values.
- [x] `controls.md`, `player-guide.md`, the approved deviation
      `dev-picture-filter` in `original-quirks.md` with its `**ScummVM:**`
      tag and its item in `scummvm-differences.md`; `dependencies.md` only
      if the linked ScummVM portions change.
- [ ] Play-test folder and guide (`docs/playtests.md`).

## Acceptance

- Tests:
  - settings round trip, unknown and missing values;
  - software-renderer readback: with the filter off the output is identical
    to today's in both shapes, and each filter gives one fixed digest on
    Windows and Linux;
  - SDL input fixtures click at real window coordinates with a filter on,
    in both shapes, and reach the same frame pixels as with it off;
  - loop tests: F11 changes and saves the filter, wraps around, makes no
    pass, reaches no handler and types nothing in a text entry.
- The complete suite passes on Windows and on WSL Linux, and GitHub Actions
  is green on both systems.
- One maintainer play-test after Part B, of what the tests cannot settle:
  how each filter looks at real sizes (window, fullscreen, both shapes),
  F11 and the launcher list on Windows, the setup screen row on Linux, and
  smooth play with the heaviest filter on the Steam Deck.

## Out of scope

Bilinear smoothing, scanlines, CRT shaders and OpenGL; changing the
default; a controller button for the switch; filters for the Windows
launcher's own window; Android itself.
