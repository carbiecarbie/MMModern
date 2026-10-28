# Milestone 94 plan - Other sound devices

**Tier A. Status: completed and accepted** by the maintainer on 2026-10-28
after the play-tests and the independent implementation review. Plan
`b4448b8`, amendment `dfd05c1`, Part A1 `ea39c26`, Part A2 `67d241f`,
Part B `6fc9a22`, review fixes `a12639e` and the closure commit. Roadmap
[step 19](roadmap.md#next-arc-finishing-the-games-systems). Save v21,
schema and content unchanged.

## Goal

Both original programs pick their music and digital drivers from the two
`XEEN.CFG` ids, as the installer wrote them. The installed CD pair 3/10
(Pro Audio Spectrum music, prosnd digital) keeps its output, timing, tests
and digests exactly.

## Scope

- **Implemented:** selection in DAT and EXE; `nullmus` and `nullsnd`;
  music 1 `admus` (AdLib), 2 `blastmus` (Sound Blaster), 3 `promus`
  (unchanged), 4 `rolmus` (MT-32), 6 `specmus`; digital 6 `blastsnd`,
  7/10 `prosnd`; the reference Sound Blaster mixers.
- **Deferred:** music 5/7/8/9 and digital 1/2/5/8/9 behave as the null
  drivers, silent with one notice
  ([approved restriction](original-quirks.md#dev-m94-deferred-null)). EXE
  digital 3/4/9 gives the notice and returns to the title instead of the DOS
  disk error ([approved deviation](original-quirks.md#dev-xeen-unavailable-digital)).
  Their open questions (GS synth and bank, Wave codebook, TAP model, speaker
  clocks, ADSND branch, PAS clock, DSS FIFO, Covox address) wait for a later
  milestone.
- **Excluded:** a device switch while playing (a later opt-in decision),
  host MIDI and physical-card timing.

## Key decisions

- **Original program evidence first.** Part 0 analysis of the DAT, EXE,
  installer and INTRO drivers, with DOSBox Staging captures per device,
  settled the tables, clocks, mixers and driver rules
  ([audio rules](rules/audio.md#other-original-sound-devices)).
- **One owner, native drivers.** `XeenAudio` keeps the presentation clock;
  mono OPL and rolmus are resource-driven profiles of MMModern's own driver
  on the pinned Nuked OPL3 (OPL2 mode) and mt32emu 2.7.0 cores. No original
  code runs and no ScummVM MIDI backend is used.
- **Render-only I/O cost.** MPU polling and OPL helper delays timestamp
  output only; they never add game-visible time
  ([timing policy](original-quirks.md#dev-audio-io-cost-clock)). Each
  program start initializes its driver once.
- **MT-32 ROMs** (maintainer, 2026-10-27): the player's content-identified
  MT-32 1.07 control and PCM ROMs in `%LOCALAPPDATA%\MMModern\roms`; any
  other set gives one notice and silence without changing timing.
- **Rolmus fade** (maintainer, 2026-10-28): DOS indexes the first fade
  cache with the interrupted foreground BX; MMModern fixes it at 8
  ([approved deviation](original-quirks.md#dev-rolmus-fade-bx)).
- **Kept oddities:** the admus pointer cutoff, the per-profile attenuation
  selector, the voice-7 guards, the FX note-off selection and specmus being
  exactly blastmus are reproduced and recorded in
  [original quirks](original-quirks.md#sv-admus-pointer-cutoff).

## Results

- **A1:** every CFG reader follows the selected file; the developer option
  `--xeen-cfg` selects another one read-only. DMA digital uses each driver's
  count under the [unsplit approximation](original-quirks.md#dev-prosnd-dma-timing).
- **A2:** AdLib, Sound Blaster and PAS music match the DOSBox register
  captures exactly in each compared window. An automated A/B level check
  found a wrong attenuation selector that the register oracle had missed.
- **B:** the MIDI oracle matches the capture from the first message; a
  test-only replay of the captured BX values and an inferred FX 26 byte
  decrement covers the BX-dependent steps. Replaying the captured DOS MIDI
  through the same synth matches MMModern's render. The renderer reserves
  synth queue headroom and never defers in ordinary play.
- **Levels:** every device sits about 3.5 dB below DOSBox Staging, the
  existing base mix (source gain 1/3 against Staging's about 0.5).
- Complete suite 351/351 (run `20261028-054429-1e9aaa00`); M44 digests
  unchanged throughout.

## Review and acceptance

The independent implementation review approved with fixes (one medium
hermeticity finding, six low). Its re-check approved the fix round, and
the one new low finding (a developer-entry driver init) was fixed. The maintainer play-tested A2 and B, compared
MT-32 with DOSBox, and accepted the milestone.
