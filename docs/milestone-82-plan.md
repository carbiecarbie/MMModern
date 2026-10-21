# Milestone 82 plan - Audio, part 1: FM music and effects

**Tier A. Status: Part 0 completed; Parts A-D not started**
Roadmap [step 12](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones. Plan `61112d9`, on `main` after M81 (`f2bee4b`); amended
`d5bba59` for the promus driver after Part 0's OPL3 stop, `6792341` at
Part 0's closure, and for the direct Nuked core after Part A's first attempt.

## Goal and scope

The game plays its original FM music and FM sound effects as the installed
CD configuration does: the Pro Audio Spectrum driver `promus` on an OPL3
chip in stereo (maintainer, 2026-10-21). The control panel's Efx, Music,
Speech and Text buttons work and are saved as in DOS. Digital sound (VOC
samples, voices), CD speech and CD tracks are the second milestone (M83).
Other devices, the AdLib (`admus`) and Sound Blaster (`blastmus`) FM drivers
included, and switching device while playing are roadmap step 16; M82 only
keeps the design ready for them. Darkside map music and the intros stay out;
the CD title song is in.
Planning does not authorize implementation. Independent plan and
implementation reviews, the complete suite and a maintainer play-test are
required.

## Current state

- No audio output. `SdlWindow` initializes video and events only; nothing
  links ScummVM's `audio/` code, and `dependencies.md` needs no source patch.
- The clock already keeps original durations: [PresentationClock](../src/core/PresentationClock.h)
  counts 18.2065 Hz BIOS ticks; CD speech and entrance-scene samples wait
  silently for their original length (`XeenCdSpeechTiming.h`, `XeenVocDuration.h`).
- [XeenTitleFlow](../src/app/XeenTitleFlow.cpp) answers Efx, Music, Speech and
  Text (panel buttons 0, 1, 6, 7) with "not supported yet".
- Save v17 (`kJourneyVersion`, schema 9, content 14) holds no sound state. New
  games read the 812-byte Clouds `maze.pty` ([Application.cpp](../src/app/Application.cpp));
  `WORLD/XEEN.DAT` is already read at run time as the CD UI module.
  `XeenProgramState` is the running-program owner (M79, M81).

## Original rules and evidence

Part 0 (completed 2026-10-21) settled the rules from calibrated static
analysis of `WORLD/XEEN.DAT`, `XEEN.EXE` and the INTRO.CC `promus` resource,
plus six DOSBox DRO captures; [audio rules](rules/audio.md) hold them with
offsets and evidence labels, and the tagged entries in
[original quirks](original-quirks.md#sv-cd-promus-fm-banks) the differences
from the pin (`6814ee9ba54582f5b5adcffab49efbbd8f589edd`). In short:

| Rule | Settled (DOS, high unless noted) |
| --- | --- |
| Device | `XEEN.CFG` word +0x04 selects the driver (ids outside 1-9: nullmus); the GOG install selects 3, `promus`, loaded from INTRO.CC (a loose file overrides it). It enables OPL3, drives both banks with opposite stereo outputs and owns the FM effect data. A missing `XEEN.CFG` exits; MMModern refuses to start with a clear message (maintainer, 2026-10-21) and never writes it. |
| Driver | [Commands, voices, patches, bytecode](rules/audio.md#promus-commands-and-voices): music on voices 0-6, effects on 7-8, two 11-byte patches per voice, 22-byte effect definitions, effect ids 0-180 over 68 streams. Ticks at about 72.80 Hz; music-due ticks skip the effect countdown; fade 207 takes 78-80 ticks on a byte carry. No OPL status or timer reads. The original effect opcode 6 cache-indexing defect is kept. The pin's fade sign test and effect call opcode are not imported. |
| Options | Efx, Music, Text and Speech are saved party bytes (+0x0e, +0x0f, +0x5f8 with 0 = on, +0x5f9); New Clouds starts Efx on, Music on, Text off, Speech on; Load replaces them without replaying a toggle. Efx on plays effect 20; Efx off gates new FM effects, VOC samples and VOC-busy waits without stopping sound. Music off silences voices 0-6 and cancels a fade but keeps a pending load; Music on restarts the loaded buffer and cancels a pending load. Speech off forces Text on and skips CD speech. |
| Songs | [Names and changes](rules/audio.md#song-names-and-asynchronous-changes): Clouds outdoors `outdoors.m`, indoors by wall kind (kind 5, unused in Clouds, reads `outday1.m`), locations and the Inn (`inn.m`) from the `XEEN.DAT` tables. The same name continues even with Music off; a new name fades, waits for the fade and CD speech while input stays active, then loads. A missing song exits with a resource error. Song state is process state. The CD title plays DARK.CC `newbrigh.m` and preloads `twnnitea.m` on exit, in M82's scope (maintainer, 2026-10-21). |
| Captions | Entrance captions advance one character per qualifying two-tick interval; Efx off removes VOC-busy waits but keeps fixed scene waits. When the box pixels disappear is settled in Part B. |

## Design decisions

- **MMModern's own `promus` driver on the Nuked OPL3 core.** The pin's
  driver is OPL2-only, hard-codes 72 Hz and chooses the emulator through
  ConfMan; it cannot be patched into `promus`. MMModern implements the
  `promus` semantics Part 0 establishes (both banks, stereo routing, voices,
  fade, effects), reading its instrument and effect tables from the installed
  INTRO.CC resource at run time, never embedding them. It drives the plain
  functions of the pinned `audio/softsynth/opl/nuked.h` core:
  `OPL3_Reset(&chip, 49716)`, unbuffered `OPL3_WriteReg` and `OPL3_Generate`
  at the native 49716 Hz, whose two outputs carry both banks under promus's
  routing. Not `OPL3_WriteRegBuffered` (two-sample delay; the pin's choice),
  not `OPL3_GenerateStream`/`GenerateResampled` (a second resampling), not
  `OPL3_Generate4Ch` (inline in `nuked.cpp`), and `OPL_ENABLE_STEREOEXT`
  stays undefined so the core has no lazy static.
- **No `OSystem`, no ScummVM mixer (amended after Part A's first attempt,
  2026-10-21).** An initialized `OSystem` needs four backend managers, and the
  save manager's vtable lives in `backends/saves/savefile.cpp`, which with the
  pin's `USE_CLOUD` pulls the cloud manager. Instead `g_system` stays null and
  no ScummVM class is instantiated: not the mixer, not `EmulatedChip`, not
  Nuked's `OPL` class (its `init` reads the mixer through `g_system`).
  `nuked.cpp` is compiled from source and, preferably, filtered with
  `objcopy` like the existing `sprites.cpp` step (removing the `OPL` class
  members, its type info and `Audio` references), so it links without other
  ScummVM audio objects. Fallback, verified to link with `g_system` null:
  `audio/fmopl.cpp`, `audio/chip.cpp`, `audio/timestamp.cpp` and
  `audio/softsynth/opl/{nuked,dosbox,dbopl,mame}.cpp` with the libcommon
  already linked, none of them instantiated. No pin patch.
  `dependencies.md` lists the sources, the filter and the licenses.
- **Mixing.** MMModern mixes its own sources: FM in M82; VOC samples and CD
  tracks join in M83. Each source feeds its own `SDL_AudioStream` to the
  device rate, and the owner mixes at the device rate. Part A states whether
  promus's PAS mixer writes (initialization and command `0x100`) change the
  audible FM level; if they do, the mix applies them, otherwise they are
  recorded as unmodelled in `original-quirks.md`.
- **Owners.** The platform layer opens the SDL audio device and pulls mixed
  samples from the audio owner. A program-lifetime audio owner next to
  `XeenProgramState` (the loaded driver is running-program state in DOS)
  holds the selected original device id, the driver, the current song name
  and its buffer for the driver's lifetime; it is the one place songs and
  effects are requested. The map and location owners choose songs (map load,
  services, Inn), the title flow chooses the title song, and the toggles are
  party state in the snapshot. Requests cross to the audio thread through one
  queue.
- **Timing never depends on the device.** The driver ticks from the FM sample
  count with an exact integer accumulator (49716 x 16390 / 1193182, about
  682.918 samples per tick), independent of callback size and wall time; a
  tick's register writes apply at its sample boundary and are logged per tick
  for the DRO comparison. Where DOS waits (the song fade, and its wait for CD
  speech), the wait is computed from the driver's tick count and the existing
  simulated CD-speech timing on the presentation clock, never by waiting on
  real output. Without an output device (tests, the SDL dummy driver) the
  game times, draws and saves exactly as with one; audio never feeds RNG,
  Events or saves.
- **Device choice ready, not exposed.** The owner creates and destroys the
  driver on request, keyed by the original device id, so step 16 can add
  devices and an opt-in live switch without new ownership. Ids outside 1-9
  are faithful silence (nullmus); a real device not yet supported, admus and
  blastmus included, plays nothing and reports "not supported yet" once.
- **Save v17 -> v18; schema 9 and content 14 unchanged.** The snapshot gains
  the four toggles. Restore applies them without replaying anything; the
  current song follows Part 0's Load rule. v17 and older are rejected.

## Parts and acceptance

Each part takes one fresh session; amend this plan if the scope changes.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | **Completed 2026-10-21**, read-only: the rules above, the [audio chapter](rules/audio.md), tagged quirks and six DRO captures in `D:/MModern-re/audio-m82/captures/`. Moved on: the per-function effect-site reconciliation opens Part C; the caption pixel lifetime goes to Part B; the per-tick DRO comparison is Part A's acceptance. |
| A - audio foundation | The Nuked core and its link closure, SDL output, MMModern's mixing, the audio owner and the `promus` driver on Nuked OPL3 at 72.80 Hz; songs for maps, locations, title (with the preload) and Inn by the DOS rules, with the fade; a missing `XEEN.CFG` refuses startup with a clear message. Acceptance: songs play and change as in DOS; same-name continuation; register streams match the DRO captures; identical timing and traces with and without a device. |
| B - toggles and save | The four panel buttons with original labels and rules; save v18; New and Load values; Speech off skips CD speech waits; Efx off removes VOC-busy waits; entrance captions with Efx off, their box lifetime settled from a maintainer video of the Dwarf or Sphinx scene with Effects off. Text off is stored, shown, saved and coupled, but text stays visible until M83 plays CD speech (temporary restriction approved by the maintainer, recorded in `original-quirks.md`), so the DOS defaults never leave a player without text or speech. |
| C - FM effects | First reconcile Part 0's 133 direct DOS sites function by function with MMModern owners (several are provisional). Then every site, gated by Efx, in movement, combat, spells, items, dialogs and services; C1/C2 if large. Acceptance: each site's effect id matches DOS; none plays with Efx off. |
| D - closure | Dependencies, quirks and differences documents; M44 digest audit; scheduled independent implementation review; complete suite; maintainer play-test against DOSBox. Update status, roadmap and history and condense this plan only after acceptance. |

**Stop rules** (stop and ask the maintainer): `promus` relies on OPL behavior
the pinned Nuked emulator cannot reproduce (status or timer reads, write
timing); the DRO captures contradict the static reading; a moved boundary
(effect reconciliation, caption lifetime) cannot be settled; the audio link
needs more than the `audio/` sources named above and the libcommon already
linked, or any ScummVM object must be instantiated or `g_system` set; the effect
inventory exceeds two parts; the fade wait cannot be modelled from the clock.

## Tests and validation

- Unit tests without a device: music selection for every Clouds map from
  original data (outdoor, each wall kind, continuation, Music off), toggle
  rules (Speech/Text coupling, Efx on effect 20), device ids and nullmus.
- Driver fidelity: per-tick OPL3 register streams, both banks, for a few
  songs and effects compared with the DOSBox `promus` DRO captures (DOS
  evidence for the rate, stereo routing and defects), aligned to the PIT
  grid. PCM hashes only guard regressions and may differ across platforms.
- A guard test asserts `g_system` is still null after the audio owner and
  driver start.
- Effect sites: a table test mapping each Part 0 site to its id, plus a few
  original-data gameplay witnesses recording requested effects through a
  recording sink. A timing test runs one route with and without a device.
- Extend the generic `XeenSaveStateTests` round-trip for the toggles and
  format rejection for v17.
- M44 digests hash raw saves and **must change** with v18; audit them with
  `tools/m44-digest-audit.ps1`, explain every delta and obtain the
  maintainer's approval before replacing hashes.
- Each production part: affected targets and tests, the fast suite, then the
  complete `build-rel` suite through [tools/AGENTS.md](../tools/AGENTS.md),
  with `SDL_AUDIODRIVER=dummy`. This planning change checks links and
  `git diff --check` only.

## Decisions and risks

1. **Device scope (decided 2026-10-21):** FM only; the other drivers and an
   opt-in live switch are step 16. Live switching would be an approved
   deviation (DOS loads one driver at startup).
2. **Driver (decided 2026-10-21, after Part 0's OPL3 stop):** M82 reproduces
   `promus` in OPL3 stereo on the Nuked emulator, the installed CD
   configuration; `admus` and `blastmus` move to step 16.
3. **Missing `XEEN.CFG` (decided 2026-10-21):** MMModern refuses to start
   with a clear message, as DOS does; it never writes the file.
4. **Title song (decided 2026-10-21):** the CD title's DARK.CC `newbrigh.m`
   and its `twnnitea.m` preload are in scope; Darkside map music is not.
5. **Text off stopgap (approved 2026-10-21):** text stays visible until M83
   as a temporary restriction, recorded in `original-quirks.md` by Part B.
6. **Technical risks:** audio-thread races (one queue, no shared game state);
   the link closure of `nuked.cpp` (objects compiled but never instantiated) and
   GPL notices; latency between a request and the heard sound, which never
   delays gameplay; effect coverage drifting from DOS without the inventory.
