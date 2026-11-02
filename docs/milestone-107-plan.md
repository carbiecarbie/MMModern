# Milestone 107 plan - World ending presentation

**Tier B. Status: planned.** Step 5 of the [Darkside arc](roadmap.md#darkside-arc),
after [M106](milestone-106-plan.md), which records the World win (opcode 58),
the `genesis` mirror word and the title's W row, and today shows "World
ending: not supported yet" in their place. Save v25, Journey schema 9 and
content 14, and the `settings.ini` won store stay unchanged.

## Goal

The original World of Xeen ending plays from `WORLD/XEEN.EXE` wherever DOS
plays it: after the win at the altar of the Clouds of the Ancients (map 129),
after the `genesis` mirror word on either side, and from the title's W row
with the fixed score 1234567890. It shows the party's score as DOS does and
returns to a fresh World title. The notice disappears from all three paths.

## Settled by M106 (do not reopen)

- Entry, save and write boundary: one CFG write and one forced save before
  the hand-off; the presentation writes nothing
  ([win rules](rules/cutscenes.md#darkside-and-world-win-entry-and-save),
  [write boundary](rules/cutscenes.md#ending-write-boundary)).
- State argument: every DOS path sends **0**, so only the default
  presentation is reachable; the Goober and Super Goober branches (ASCII 1/2)
  are not implemented and need no test
  ([state rule](rules/cutscenes.md#world-ending-state-argument),
  [quirk](original-quirks.md#sv-m106-world-state)).
- Scope in the EXE: main `0xfbc9..0x11467`, final helper `0x11468..0x11864`,
  shared text helpers `0xfadf..0xfbc8`, plus the shared score pages
  `0x6c91..0x6f21` that M106 already implements.

## Part 0 - DOS check

- [ ] Calibrated static analysis of the scope above, as M106 Part 0 did for
      the Darkside movie: ordered resource and archive selection (the pin
      switches to a third archive), palettes, songs, effects and any voices,
      caption records and their selection, every wait and its tick count,
      sound-gated tails, random sites, input polls (skip keys, Ctrl-Q points),
      sprite drawers and clipping, the score/congratulations pages and the
      return to the title. The pin's `worldEnding1..3` is a guide only.
- [ ] Prepare a DOSBox seed with the World won byte (CFG byte 23) and a
      double-click launcher, so the maintainer can record one uninterrupted
      W title replay with the M106 capture settings. Record the capture's
      measured anchors in [evidence](rules/evidence.md).
- [ ] Write the settled sequence into a "World ending presentation" section
      of [cutscenes](rules/cutscenes.md); every place where DOS and the pin
      differ gets a quirk entry and a `scummvm-differences.md` item.
- [ ] Stop and ask the maintainer if DOS is ambiguous, contradicts the
      capture, or needs a new owner or saved state.

## Part A - Presentation

- [ ] Add `XeenCutscene::worldEnding` (and a `World` resource table in
      `XeenCutsceneData` read from the installed EXE) on the M91/M106
      presenter: segments, holds, captions, audio, the presentation random
      source and the mode-8 sprite adapter. No new owner.
- [ ] Reuse the Darkside ending's score pages and their keyboard-only advance
      for the World pages, with the differences Part 0 finds.
- [ ] Replace the World notice in `XeenTitle` (win, `genesis`, W replay); keep
      the existing unavailable/resource-error notice and title return.
- [ ] Input as Part 0 settles it; animations follow the EXE's polls, Ctrl-Q
      quits at the original keyboard checks.

## Acceptance

- [ ] Original-data runs: map 129 win and `genesis` reach the presentation
      with the party's score and a fresh title, writing nothing after the
      hand-off; the W replay shows 1234567890 and writes nothing.
- [ ] Fake-clock tests of every stage's timing inside the capture brackets;
      key frames and both score pages match the capture pixel for pixel
      (six-bit VGA RGB, as M106).
- [ ] Keys and clicks during animations, Ctrl-Q, Text on and off.
- [ ] M44 digests unchanged; complete CTest suite passes.
- [ ] Play-test (one short session, presets and DOSBox comparison saves as
      [playtests](playtests.md) describes): only what the capture and tests
      cannot settle, normally the map 129 win through the whole ending and
      the W replay, watched and heard against DOSBox.
- [ ] Closure updates project status, history and roadmap, and condenses
      this plan.
