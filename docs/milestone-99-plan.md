# Milestone 99 plan - 4:3 picture

**Tier B. Status: planned (not started).** Follows roadmap
[step 3](roadmap.md#next-arc-release-quality-and-the-darkside-start).
Baseline: `ad6e8a2`, equal to `origin/main`. Save v22, Journey schema 9 and
content 14 unchanged: the picture shape is a host window setting, never game
state.

## Goal

The game shows its 320x200 picture with the shape the original's 4:3
monitors gave it: each pixel 1.2 times as tall as it is wide, the whole
picture 4:3, as DOSBox shows it with aspect correction. This is the default.
Today MMModern shows square pixels (16:10), which flattens every portrait,
font and scene.

## Key decisions (maintainer, 2026-10-30)

1. **4:3 is the default; square pixels stay as an opt-in.** One line in
   `%LOCALAPPDATA%\MMModern\window.ini`, `aspect=square`, brings back
   today's presentation exactly (integer multiples of 320x200 with black
   borders). This is an approved deviation from the original and is recorded
   as one.
2. **Sharp scaling that fills the largest 4:3 area.** The picture fills the
   largest 4:3 rectangle that fits the window or screen (1440x1080 on a
   1920x1080 screen), centred, black borders elsewhere. It is drawn by
   enlarging the frame by a whole number with hard pixels, then reducing to
   the final rectangle with linear filtering ("sharp bilinear", as DOSBox
   and ScummVM offer): every pixel has the same size, only its edges are
   softened. Plain nearest-neighbour stretching, with rows of uneven height,
   was rejected.

## Tasks

- [ ] `WindowPlacement` gains the aspect setting. `aspect=4:3` (default) or
      `aspect=square`; a missing, unknown or malformed value keeps 4:3; the
      file says how to choose it in its comment and keeps fullscreen and the
      remembered size as today. Old `window.ini` files keep working.
- [ ] Pure, unit-tested geometry in `SdlWindowPlacement` (or beside it) for
      both modes: the picture rectangle in drawable pixels for a given output
      size, the whole-number enlargement factor, and the mapping from window
      coordinates to frame coordinates (`floor`, outside the picture = no
      click, as today). Use the renderer's output size so high-DPI displays
      stay sharp.
- [ ] `SdlWindow` presents 4:3 through an intermediate render target: the
      frame and the original cursor are drawn into it with nearest scaling,
      then copied to the picture rectangle with linear scaling. The cursor is
      part of the picture, so it gets the same shape. Square mode keeps the
      present path (logical size and integer scale) unchanged.
- [ ] The target follows the output: it is recreated when the drawable
      size changes (resize, maximize, Alt+Enter, DPI change) and the whole
      picture is redrawn after `SDL_RENDER_TARGETS_RESET` and
      `SDL_RENDER_DEVICE_RESET`. Its size stays within the renderer's
      maximum texture size. The software renderer fallback works too.
- [ ] Every mouse use goes through the same mapping in 4:3 mode: button
      events, the cursor position and mouse feedback. Clicks keep landing on
      the same original buttons at every window size, in both modes.
- [ ] Window sizes in 4:3 mode: the first window is the largest exact 4:3
      size within 90% of the usable display area; the minimum window size
      is 320x240. A remembered size is kept as today and letterboxed.
- [ ] The title, intros, ending and every gameplay screen use it, since they
      share `SdlWindow`.
- [ ] Documentation in the same change: README "Display and sound";
      `docs/original-quirks.md` entry for the square-pixel option as an
      approved deviation (anchor, maintainer date, DOS 4:3 basis) with its
      `**ScummVM:**` tag stating the pinned reference's default for aspect
      ratio correction as read from its source, and the matching
      `docs/scummvm-differences.md` item.

## Tests and evidence

- Unit tests: `window.ini` parsing and writing of the aspect key (default,
  both values, unknown value, old files), picture rectangle and enlargement
  factor for common outputs (1920x1080, 2560x1440, 3840x2160, 1366x768,
  a narrow and a tall window, the minimum size) in both modes, and the
  window-to-frame mapping at the picture's corners and edges.
- A presentation test with SDL's offscreen or dummy video driver, if SDL
  allows it here: render a known frame in 4:3 mode, read the output back
  and check the picture rectangle, the black borders and that pixel rows
  have one height. If the driver cannot render, say so and rely on the
  agent's own screenshots below.
- The agent captures the real window itself (title, gameplay with text and
  a portrait, cursor over a button) at 1920x1080 fullscreen and at two
  window sizes, in both modes, and inspects them before the play-test.

## Acceptance

- 4:3 by default, square by `aspect=square`, both remembered as above.
- Sharp, uniform pixels filling the largest 4:3 area; clicks land on the
  right buttons at every size; Alt+Enter, resize and maximize keep working.
- Complete CTest suite passes on `build-rel`.
- Maintainer play-test, one short session after the implementation: the look
  on the maintainer's real monitor (sharpness and shape, which screenshots
  cannot judge for them), mouse accuracy in windowed and fullscreen, and
  switching back with `aspect=square`.
