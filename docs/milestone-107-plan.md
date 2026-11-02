# Milestone 107 plan - World ending presentation

**Tier B. Status: completed and accepted by the maintainer (2026-11-02)**
after the play-test. Plan `73019b3`, Part 0 `8cbcef3`, Part A `bfc0ee8`.
Step 5 of the [Darkside arc](roadmap.md#darkside-arc), after
[M106](milestone-106-plan.md), on the cutscene presenter of
[M91](milestone-91-plan.md) and [M106](milestone-106-plan.md). Save v25,
Journey schema 9 and content 14, and the `settings.ini` won store
unchanged.

## Goal

The original World of Xeen ending plays from `WORLD/XEEN.EXE` wherever DOS
plays it: after the win at the altar of the Clouds of the Ancients (map 129),
after the `genesis` mirror word on either side, and from the title's W row
with the fixed score 1234567890, then returns to a fresh World title.

## Key decisions

- **Settled by M106, not reopened:** the save and CFG write before the
  hand-off, no write afterwards, and state argument 0 on every DOS path, so
  the Goober and Super Goober branches stay unimplemented
  ([state rule](rules/cutscenes.md#world-ending-state-argument)).
- **DOS check first.** Part 0 settled the sequence from calibrated analysis
  of EXE `0xfbc9..0x11864`; the maintainer's uninterrupted W replay capture
  then confirmed it frame for frame
  ([rules](rules/cutscenes.md#world-ending-presentation),
  [evidence](rules/evidence.md#m107-part-0),
  [capture](rules/evidence.md#m107-capture)). The pin was a guide only; six
  differences are recorded, the largest being three CD animations it omits
  ([quirks](original-quirks.md#sv-m107-cd-animations)).
- **No new owner.** `XeenCutscene::worldEnding` runs on the existing
  presenter; the Darkside score pages became a shared Pharaoh page helper.
  The presenter gained the `0x4000` top clip, a song-fade wait, the EXE
  writer's persistent alignment and trailing-space deduction, and skips
  program sprite components the EXE drawer never publishes, each for every
  EXE scene that uses them.

## Results

- The ending plays with INTRO.CC resources, the two songs, the effects, the
  EXE's own captions (no voices, whatever the Text setting or sound device),
  unskippable animations, Ctrl-Q at the EXE's keyboard checks, one opening
  and one score Pharaoh page advanced by keyboard only, and no write.
- All 2,681 presented frames equal captured frames in six-bit VGA RGB, and
  every stage lands within -0.01..+0.12 s of the capture. Effect onsets could
  not be measured under the music; they follow the EXE frame conditions.
- The text rule changes one frame elsewhere: the first frame of the Darkside
  score page 1 without a digital device, which no capture shows; the World
  capture confirms the same frame sequence.
- Tests: `xeen_world_ending` (order, frame counts, waits, capture frames and
  brackets, audio cues, ignored input, device independence) and the real
  map 129 run through the ending, a fresh title and Load. The census reports
  opcode 58 as supported. M44 digests unchanged; complete suite 402/402.
- **Play-test** (2026-11-02, level-200 presets and DOSBox comparison saves):
  the map 129 win through the whole ending, `genesis` with Ctrl-Q, and the W
  replay all passed.
