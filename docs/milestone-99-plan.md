# Milestone 99 plan - 4:3 picture

**Tier B. Status: completed and accepted** by the maintainer on 2026-10-30
after the play-test. Plan `46ef112`, implementation `51508de` and the closure
commit. Follows roadmap
[step 3](roadmap.md#next-arc-release-quality-and-the-darkside-start). Save
v22, Journey schema 9 and content 14 unchanged: the picture shape is a host
window setting, never game state.

## Goal

The game shows its 320x200 picture with the shape the original's 4:3
monitors gave it, each pixel 1.2 times as tall as it is wide, as DOSBox does
with aspect correction. Before M99 MMModern showed square pixels (16:10).

## Key decisions (maintainer, 2026-10-30)

1. **4:3 is the default; square pixels are an opt-in.** `aspect=square` in
   `%LOCALAPPDATA%\MMModern\window.ini` keeps the former presentation in
   whole multiples of 320x200, through the unchanged SDL logical-size path.
   It is an [approved deviation](original-quirks.md#dev-square-picture); the
   pinned ScummVM also corrects the aspect ratio by default.
2. **Sharp scaling over the largest 4:3 area.** The frame and the original
   cursor are drawn into a render target enlarged by a whole number with
   nearest scaling, then copied with linear scaling to the largest centred
   4:3 rectangle of the renderer's output (1440x1080 on a 1920x1080 screen).
   Every pixel has one size; only its edges are softened. Plain
   nearest-neighbour stretching, with rows of uneven height, was rejected.

## Results

- `WindowPlacement` reads and writes `aspect=4:3|square`; unknown values
  keep 4:3 and older files keep working. Developer entries read the shape
  but, as before, do not save the window placement.
- Pure geometry in `SdlWindowPlacement`: the picture rectangle, the
  enlargement factor within the renderer's texture limit, and the mapping
  between window and frame coordinates in both directions, high DPI
  included. Clicks, the cursor and button feedback all use it; clicks
  outside the picture are ignored.
- The target follows output size changes and renderer resets. The first
  4:3 window is the largest exact 4:3 size within 90% of the usable display;
  the minimum is 320x240.
- Tests: geometry and settings unit tests, a software-renderer readback of
  the scaling and of the redraw after resets, and SDL input fixtures that
  click at real window coordinates, mostly in 4:3 with one case in square
  mode. Complete suite 371/371.
- Play-test (2026-10-30): windowed and resized, fullscreen through Alt+Enter
  on a 3840x2160 display, and `aspect=square`; all passed.
