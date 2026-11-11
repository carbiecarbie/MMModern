# Milestone 116 plan - Picture filters

**Tier B. Status: completed and accepted by the maintainer (2026-11-11)**,
after a play-test on Windows and the Linux setup screen under WSLg. Plan and
Parts 0 and A `728e881`, Part B `2b8667f` and the closure commit. Second of
the opt-in extras of [later goal 6](roadmap.md#later-goals), after M115.
Save v26, Journey schema and content unchanged: the filter is a host window
setting, never game state. With the filter off, the default, nothing
changes.

## Scope

A player can soften the enlarged 320x200 picture with one of the classic
pixel-art filters of ScummVM and DOSBox, chosen in the launcher or the setup
screen, or switched with one key while playing. The default stays the sharp
4:3 picture. Out of scope: bilinear smoothing, scanline filters (TV2x,
DotMatrix), CRT shaders and OpenGL, changing the default, a controller
button for the switch, filters for the launcher's own window and Android
itself.

## Evidence

- The original has no picture filter: it draws 320x200 in VGA mode 13h, so
  any filter is an approved deviation by nature, like
  [square pixels](original-quirks.md#dev-square-picture); the entry is
  [dev-picture-filter](original-quirks.md#dev-picture-filter).
- The pinned ScummVM offers these filters as its scalers, already built into
  the linked library on both systems; they run on the CPU, so they need no
  OpenGL and carry over to Android.
- F11 and F12 produce no code anywhere in the original
  ([controls](controls.md)), so the game never sees either. F12 is Steam's
  default screenshot key, which leaves F11.

## Decisions

Maintainer, 2026-11-10:

1. **The classic ScummVM filters only.** A CRT shader would need the picture
   drawn through OpenGL on all three systems and is a milestone of its own if
   it is ever wanted.
2. **Three ways to switch:** a list in the Windows launcher, a row in the
   Linux setup screen, and a key while playing that the game does not use.
3. **Both picture shapes:** the default `aspect=4:3` and `aspect=square`.

Taken with the plan and accepted at the play-test:

1. **The list.** Every scaler of the pin except Normal, TV2x and DotMatrix:
   `advmame2x`, `advmame3x`, `advmame4x`, `hq2x`, `hq3x`, `edge2x`, `edge3x`,
   `sai2x`, `supersai2x`, `supereagle2x` and `pm2x`, plus `off`. Edge stays
   although it is the heaviest.
2. **The key is F11.** It selects the next filter, `off` included, wraps
   around, works wherever the window shows a picture, makes no gameplay pass
   and never reaches the game, like Alt+Enter. No controller button sends it.
3. **The setting.** `filter=<name>` in `window.ini`, `off` by default;
   unknown values and older files mean `off`. The player entry saves the
   choice F11 makes; developer entries read it but do not save it.

## Design as built

- `SdlWindow` presents the picture and gains the filter stage; `filter` is
  one more field of `WindowPlacement`. No new owner. The scaler classes of
  the linked `libgraphics.a` are created directly, without ScummVM's plugin
  manager.
- The 320x200 frame is filtered into a texture two, three or four times as
  large, which then takes the frame's place in the existing shape path
  (whole-number enlargement, then M99's sharp fitting). With `aspect=square`
  and a filter on, the picture is drawn through that same path. With the
  filter off, both shapes draw as before.
- A frame is filtered only when the frame or the filter changed, on the
  loop's thread, so a slow filter only delays drawing.
- Geometry stays on the 320x200 frame, so clicks, the pointer and the
  controller pointer are unchanged. The original cursor is drawn after the
  filter. Frame digests and captures stay before the filter, so no existing
  test expectation changed.
- The Windows launcher lists the filters, writes only the `filter` line and
  re-reads it after the game closes. The Linux setup screen has a row with
  the same values; its main page now pages with More (eight buttons fit),
  with Open saves folder and Open log folder on the second page.

## Results

- All eleven scalers link from the already linked `libgraphics.a`: no
  ScummVM reconfiguration, no other artifact, and none dropped. Output is
  identical bit for bit on Windows and on WSL Linux.
- Edge 2x and 3x cost 25-41 ms per new frame on the development PC, against
  at most about 11 ms for the others. They are kept because presentation
  deadlines come from the game's clock, so a slow filter only delays
  drawing, never game time.
- Tests: settings round trip, unknown and missing values; software-renderer
  readback with a fixed digest per filter and today's output with the filter
  off, in both shapes; SDL input fixtures at real window coordinates; loop
  tests for F11 (changes and saves, wraps, no pass, no handler, types
  nothing in a text entry). The complete suite passes on Windows and WSL
  Linux, 422 tests each, and GitHub Actions is green on both systems. The
  Linux suite found the ninth setup button off the frame, fixed with the
  paging.
- No independent review (Tier B); the orchestrating session reviewed both
  parts.
- Play-test (2026-11-11), passed without fixes: on Windows every filter in
  the 4:3 window through F11, fullscreen, `aspect=square` with a filter, the
  launcher list and the remembered choice, and Edge 3x while walking and
  fighting; and the Linux setup screen row under WSLg. The Steam Deck item,
  the heaviest filters in Game Mode, was not played: the maintainer left it
  pending and accepted the milestone without it.
