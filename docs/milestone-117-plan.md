# Milestone 117 - Optional CRT picture filters

**Tier B. Status: completed and accepted by the maintainer (2026-11-14).**
The two CRT profiles replace the eleven classic scalers from
[M116](milestone-116-plan.md). The filter setting belongs to the host window.
Saves continue to use v26 with the same schema and content.

## Player controls

- Off is the default. F11 cycles through Off, CRT VGA and CRT Phosphor.
  The Windows launcher and the Linux setup screen offer the same choices.
  The player entry remembers the selected profile in `window.ini`. A saved
  name from the old scaler list selects Off.
- CRT VGA (A) keeps small letters and portraits crisp with subtle scanlines.
  CRT Phosphor (B) makes the texture more visible across the sky and other
  flat colours. Both support the default 4:3 and square-pixel shapes.

## Implementation

The two profiles share an adaptation of
[zfast_crt](https://github.com/libretro/glsl-shaders/blob/a35069864b4bf30f8c8e740d57206543fb7e745e/crt/shaders/zfast_crt.glsl).
The maintainer chose this shader over a custom one because it could deliver
the desired visual difference with a single rendering pass. The shader
ships with the game.

`SdlCrtFilter` draws the original frame into the final picture rectangle.
`SdlWindow` draws the pointer and F11 label afterwards. Frame captures
retain the 320x200 game pixels. If desktop OpenGL or shader setup fails,
the game explains the fallback and displays the sharp picture.
[Approved deviation](original-quirks.md#dev-picture-filter).

## Verification

- The complete Windows and Linux suites each passed all 423 tests.
  OpenGL readback at 1280x800 checked both profiles and picture shapes.
  Game-frame inspection covered sky, portraits and dialogue.
- The native Linux AppImage loaded the prepared save. On 2026-11-14 the
  maintainer compared A and B with Off on the Steam Deck at 1280x800.
  Walking and combat felt equally fluid with all three settings.
