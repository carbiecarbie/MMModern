# Milestone 82 plan - Audio, part 1: FM music and effects

**Tier A. Status: completed and accepted by the maintainer (2026-10-22)**
after the maintainer's listening play-tests. Plan `61112d9` (amendments
`d5bba59`, `6792341`, `a812944`), Part A `946ac81`, Part B `bcabfd6`, Part C1
`3c4d7d0`, Part C2 `088cfb0`, Part C3 `57e161f`, fixes `6d71e82`, `82eee2b`,
`861d607` and the closure commit. Roadmap
[step 12](roadmap.md#next-arc-finishing-the-games-systems), first of its two
milestones; M83 adds digital sound, speech and CD tracks.

## Scope

The game plays the original FM music and FM sound effects as the installed CD
configuration does: the Pro Audio Spectrum driver `promus` on an OPL3 chip in
stereo. Music follows maps, locations and the CD title (the Inn's song waits
for the Inn); 127 of the 133 original effect calls play, in movement, combat,
spells, items, Events, services and the panel. The control panel's Efx,
Music, Speech and Text buttons work and travel with the party in save v18.
VOC samples, voices, CD speech and CD tracks are M83; the other original sound
devices and an opt-in live switch are roadmap step 16. Darkside map music and
the intros stayed out. The rules, with DOS offsets, are in
[audio](rules/audio.md).

## Key decisions

- **DOS evidence first.** Part 0 settled the driver, options and song rules by
  calibrated static analysis of `WORLD/XEEN.DAT` and the INTRO.CC `promus`
  resource, checked against six DOSBox DRO captures by the maintainer.
- **The installed driver** (maintainer, 2026-10-21): `XEEN.CFG` music id 3,
  `promus`, which enables OPL3 and drives both banks with opposite stereo
  outputs, unlike the pin's OPL2 AdLib driver. `admus` and `blastmus` move to
  step 16 ([quirk](original-quirks.md#sv-cd-promus-fm-banks)). A missing
  `XEEN.CFG` refuses startup, as in DOS; MMModern never writes it.
- **MMModern's own driver on the Nuked core, no `OSystem`.** An initialized
  ScummVM `OSystem` needs backend code (`savefile.cpp` pulls the cloud
  manager), so `g_system` stays null: `PromusDriver` reproduces `promus`
  (72.80 Hz tick, fade 207 on a byte carry, the effect opcode 6 defect) and
  writes registers to the plain functions of the pinned `nuked.cpp`, filtered
  with `objcopy`. Writes use the pin's two-sample spacing
  (`OPL3_WriteRegBuffered`): unbuffered writes hid a key-off before a key-on,
  which silenced the Efx-on effect 20 ([quirk](original-quirks.md#promus-write-spacing)).
- **Output follows the device, the game follows its clock** (Part C3, after
  the maintainer heard late effects): a timeline driver on the presentation
  clock alone answers the game (fades, pending loads, CD-speech waits), and a
  render driver in `XeenFmSource` ticks from the device's own samples, fed by
  a queue with a fade barrier before song loads, as the DOS PIT runs `promus`
  apart from the game. Gameplay, time, RNG and saves never depend on the
  device. The device opens at its own mix rate: letting SDL accept 49716 Hz
  made WASAPI resample internally and drop blocks (crackle).
- **Options are party state** (save v18; schema 9 and content 14 unchanged):
  New Clouds starts Efx, Music and Speech on and Text off; Load replaces them
  without replaying a toggle. Speech off forces Text on and skips CD speech;
  Efx off gates effects and VOC waits and shows the entrance captions, timed
  from the maintainer's Dwarf video. The four M44 digests were replaced once,
  in Part B, with the maintainer's approval.
- **Approved limits**: Text off keeps text visible until M83 plays speech
  (temporary restriction); the PAS mixer gain is unmodelled (DOSBox does not
  emulate it; FM/PCM balance returns in M83); the door sound plays without
  its one-frame open-door redraw (Complete Clouds).
- **Energy Blast** (found in C2): DOS draws U(2,6) times the caster's level,
  not the pin's fixed 2L; fixed as a separate prerequisite. Moon Ray's
  batched healing draws remain a documented gap
  ([quirk](original-quirks.md#sv-moon-ray-preparation)).

## Parts

| Part | Result |
| --- | --- |
| 0 - evidence | Driver hardware and semantics, options, song rules, captions and the effect-site inventory from static analysis; six DRO captures. |
| A - FM music | Nuked core, SDL output, the audio owner, `PromusDriver`; map, location and title songs with fades and same-name continuation (`946ac81`). |
| B - options and save | The four panel buttons, save v18, Speech/Efx waits, entrance captions (`bcabfd6`). |
| C1 - effects outside spells | All 133 sites reconciled; 40 wired (`3c4d7d0`), the door cue later (`861d607`). |
| C2 - spell effects | 86 spell sites and the Energy Blast fix (`088cfb0`). |
| C3 - device-paced output | Timeline and render drivers, queue and fade barrier (`57e161f`). |
| Fixes | Spaced chip writes for effect 20 (`6d71e82`), device mix rate (`82eee2b`), door cue (`861d607`). |
| D - closure | Milestone review; a missing song stays reported; stale audio docs removed. |

## Results

- Six DRO comparisons, lockstep render tests at three callback sizes, a
  stall and latency test (no inserted silence, effects within ~30 ms), the
  effect-site table and original-data witnesses with Efx on and off, save
  round-trips, a device-rate test and the `g_system` guard.
- Every part had an independent review with its fixes applied; Part D's
  milestone review found one defect (a missing song could be swallowed by an
  effect) and stale docs, both fixed. Complete suite green on `build-rel`.
- The maintainer listened against DOSBox to the music, the panel buttons,
  effects, spells, captions and doors; the recordings measured ~70 dropouts
  a minute before the C3 and device-rate fixes and none after.
