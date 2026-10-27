# Milestone 94 plan - Other sound devices

**Tier A. Status: planned (not started)**
Roadmap [step 19](roadmap.md#next-arc-finishing-the-games-systems), the last
step of the systems arc. Baseline: clean `main`, `a0189b0`, equal to
`origin/main`.

## Goal and scope

The game plays music, FM effects and digital sound on the original sound
devices that the maintainer approves after Part 0, chosen the way the
original chooses them: the music and digital device ids that the installer
writes to `WORLD/XEEN.CFG`. Each approved device sounds, and the game times
its waits, as a DOS player with that card hears and sees them. The installed
CD configuration (music 3 `promus`, digital 10 `prosnd`) keeps exactly its
current behavior.

Part 0 establishes which devices the CD data actually contains and what each
one needs; the maintainer then fixes the device scope and this plan is amended
before Part A. The existing intros and ending, the Darkside intro included,
follow the chosen device like the game. Out of scope: switching device while
playing (an opt-in switch would be an approved deviation, maintainer
2026-10-21; it is a later decision, not M94 work), new Darkside content, and
any change to the save format. Planning does not authorize implementation.
Independent plan and implementation reviews, the complete suite and a
maintainer play-test are required.

## Current state

- [XeenAudio](../src/app/XeenAudio.cpp) reads the music id at `XEEN.CFG` word
  +0x04 and the mixer flag at +0x14. Id 3 creates `PromusDriver` from INTRO.CC
  `promus`; ids 1-9 except 3 print "Music device N: not supported yet" and
  stay silent; other ids are silent as `nullmus`.
- The digital id at +0x00 is read only by the intros and the ending
  (`digitalDevice>=2` caption gate in [XeenTitle](../src/app/XeenTitle.cpp));
  VOC playback always follows `prosnd` rules
  ([audio rules](rules/audio.md#installed-digital-driver-command-and-length-rules)).
- `PromusDriver` writes the pinned Nuked OPL3 core through `XeenOpl3`. As M82
  Part C3 built it, a timeline driver on the presentation clock answers the
  game (fades, pending loads, CD-speech waits) and a render driver in
  `XeenFmSource` ticks from the device's own samples, fed by a queue with a
  fade barrier. `XeenCardMixer` models the Sound Blaster Pro mixer writes.
  `g_system` stays null.
- Gameplay, time, RNG and saves never consult the output device. Sound
  options are party state in the save; the device is process configuration.

## Known original rules

M82 Part 0 evidence (`D:/Projetos/MModern/MModern-re/audio-m82/dos-findings.md`,
`scummvm-inventory.md`) and the later [card mixer](rules/audio.md#card-mixer)
analysis already settle, by calibrated static analysis of `WORLD/XEEN.DAT`
and `XEEN.EXE`:

| Rule | Evidence |
| --- | --- |
| Music ids | **DOS, high:** 1 `admus`, 2 `blastmus`, 3 `promus`, 4 `rolmus`, 5 `canmus`, 6 `specmus`, 7 `ibmmus`, 8 `wavemus`, 9 `tapmus`, any other id `nullmus` (switch `0xb57d..0xb604`, table `0xb9cc`). |
| Digital ids | **DOS, high:** 1 `ibmsnd`, 2 `adsnd`, 3/4 `nullsnd`, 5 `source`, 6 `blastsnd`, 7 and 10 `prosnd`, 8 `specsnd`, 9 `covxsnd`, any other id `nullsnd` (switch `0xb506..0xb575`, table `0xb9de`). |
| Ports | **DOS, high:** +0x02 digital port (also passed to music init), +0x06 music port, +0x08 IRQ, +0x18 DMA; the floppy configuration uses music port 0x388. Which fields each driver actually consumes is unsettled. |
| Loading | **DOS, high:** drivers load from INTRO.CC at startup (a loose file overrides); effect data belong to the music driver. Each program (DAT, EXE) initializes its drivers once. |
| Configuration rewrite | **DOS, high:** the game program rewrites 30 bytes of `XEEN.CFG` before an intro (`0xc565..0xc5a3`) and at the endgame (`0x28bde..0x28c0a`). MMModern keeps that state in `settings.ini` and never writes `XEEN.CFG`. |
| Mixer | **DOS, high:** every INTRO music driver, `nullmus` included, writes the Sound Blaster Pro mixer at the music port + 4/+5 when CFG +0x14 is set; only `promus` implements command `0x100`. The effect on other cards is unsettled. |
| Timer | **DOS, high** for `promus`, `nullmus`, `admus`, `blastmus`, `rolmus`, `canmus`, `specmus`, `wavemus` and `tapmus`: 72.80 Hz, chaining the BIOS tick at its exact average rate. **Unresolved:** `ibmmus` (no accumulator found by the preliminary scan) and the digital drivers `adsnd`, `ibmsnd` and `source`, which may reprogram the PIT for speaker or sample output. |
| Resources | The card mixer analysis read every INTRO.CC music driver's init; the CD XEEN.CC copies of `admus`, `blastmus` and `promus` are identified. Names, offsets and hashes of the rest, and every digital driver except `prosnd`, are for Part 0 to verify on the CD. |

The hardware behind each name is **Unknown** until Part 0: the names suggest
AdLib, Sound Blaster, Pro Audio Spectrum, Roland MT-32, Creative Music System,
PC speaker, Covox and the Disney Sound Source, but `specmus`, `wavemus` and
`tapmus` have no settled identity. The pin's `SoundDriverMT32` reads `rolmus`
with hard-coded patch offsets; it is reference only.

## Design decisions

- **One device choice, from the original configuration.** `XeenAudio` builds
  the music and digital drivers from the two `XEEN.CFG` ids, for the game
  program and the intro/ending program alike, as DOS does at each program
  start. MMModern still never writes `XEEN.CFG`; observable effects of the DOS
  rewrite and of each program's init and shutdown are reproduced in memory.
  Players change devices with the original installer. A developer option may
  point at another configuration file for play-tests, documented in
  `development.md`.
- **Shared driver semantics, per-device chips.** M82 reimplemented `promus`
  natively. Part 0 compares every music driver's command interpreter, voice,
  patch and effect tables with `promus`. If they share the interpreter, one
  generalized driver reads each resource's tables at run time and writes to
  a per-device chip backend (OPL2, dual OPL2, OPL3, MT-32 MIDI, speaker and
  the rest); otherwise each family gets its own driver, and the part split
  follows the families. Executing the original driver code under a CPU
  emulator is not planned: it would be a new subsystem and debugging path.
  Part 0 reports if the drivers diverge so much that it becomes cheaper.
- **An integration route per family, chosen in the amendment.** OPL2 and dual
  OPL2 use the pinned Nuked core as M82 links it. MT-32 can use the pinned
  `audio/softsynth/mt32` library (mt32emu 2.7.0, LGPL-2.1-or-later) directly,
  without ScummVM's MIDI driver, `ConfMan` or `OSystem`. The pin's speaker and
  CMS wrappers cannot be linked as they are (`PCSpeaker` takes its mixer from
  `g_system`; CMS inherits `Audio::EmulatedChip`, whose callbacks use the
  ScummVM mixer), so the amendment chooses, per family, the core to extract or
  adapt or a small MMModern renderer of the same hardware rules, and bounds
  it. No standalone Tandy/SN76496 core was identified under `audio/`; AGI and
  SCI carry engine-integrated implementations, whose suitability is a Part 0
  question. `dependencies.md` lists each new source and licence. ROM loading,
  chip initialization and allocation stay outside the render callback.
- **Timing follows each device's DOS rules on the presentation clock.** Part 0
  delivers, per family, the logical clock, its BIOS-tick consequences, the
  busy predicates and blocking intervals (VOC busy time, captions, CD-speech
  waits, a blocking speaker sample, MIDI transmission and patch upload). The
  amendment then specifies, before Part A, how `XeenAudio` generalizes the
  M82 split: timeline state on the presentation clock, render state from the
  device's samples, their queues and completion barriers. No wait ever
  depends on real output. With the installed ids, timing and traces stay
  exactly as now.
- **Missing pieces stay playable.** An approved device whose emulator input is
  absent (MT-32 ROMs) plays nothing and reports one notice; timing keeps the
  device's DOS rules. A device outside the approved set keeps today's single
  "not supported yet" notice. Neither stops the game.
- **No save change.** The save format, `kJourneyVersion`, schema and content
  are unchanged. If Part 0 finds device-dependent party or save state, stop.

## Parts and acceptance

Each implementation part takes one fresh session. Part 0 may take several
bounded sessions (for example 0a archives, installer and program handoff;
0b driver families; 0c captures); it ends with one plan amendment that the
maintainer approves and an independent reviewer checks before Part A.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | Read-only for code and original data; work copies outside the repository. (1) Which of the 18 driver names exist in the CD's INTRO.CC, XEEN.CC and DARK.CC, with offsets, lengths and hashes. (2) The installer: `WORLD/INSTALL.EXE` is PKLITE-packed; unpack a work copy and list every device it offers, the ids, ports, IRQ and DMA it writes, and any device-specific question it asks. (3) Program handoff: compare DAT and EXE selection, initialization, shutdown and configuration handoff, the consequences of the 30-byte rewrite, and which configuration fields each driver consumes. (4) Per music driver: hardware and port I/O, command set compared with `promus`, voice, patch and effect tables, fade and stop rules, init (MT-32 patch and SysEx upload), the effects each omits, and its mixer writes on its own card. (5) Per digital driver: busy and length rules, PIT or blocking use, the caption and wait gates each id produces, simultaneous music and digital operation, and CD speech with Speech and Text when the digital driver is absent or not digital. (6) Per family: logical clock, BIOS-tick consequences, busy predicates and blocking intervals, as the timing decision requires. (7) The pin's cores and wrappers, the AGI/SCI SN76496 code and the pin's `SoundDriverMT32` against DOS. (8) DOSBox Staging captures in short numbered takes with the usual launcher and CFG backup scripts, with the capture settings and alignment rule recorded: DRO for the OPL drivers, raw MIDI with timestamps for MT-32, WAV for the speaker and DAC devices, one song and a few effects each. Deliver a device table with evidence labels, the oracle and tolerance per family, a recommended scope and part split, and the stop items below. Record settled rules in `docs/rules/audio.md`, quirks and tagged differences. |
| A - selection and OPL family | Generic selection from both ids in both programs; the developer configuration option; the generalized timeline and render boundary; the OPL drivers in scope (`admus`, `blastmus` and any other Part 0 places here) and the digital drivers that share `prosnd`'s rules. Acceptance: ordered, timestamped OPL writes match the DRO captures per tick; the installed-configuration checks below unchanged; each id selects its DOS drivers; unsupported ids keep one notice. |
| B - MT-32 | `rolmus` on mt32emu with the player's ROMs, patch upload and effects per Part 0; ROM location as the maintainer decides. Acceptance: MIDI and SysEx bytes and their timing match the capture; ROM-dependent sound by calibrated listening; missing ROMs give one notice and DOS timing. |
| C and later | The remaining families Part 0 places in scope (speaker, DACs, CMS and the rest), one part per family, each with the oracle the amendment sets: digital transfer counts and busy intervals, speaker/PIT transitions where observable, calibrated waveform or listening comparison with stated tolerances. |
| Closure | Dependencies, licences in the release package, quirks and differences; M44 digest audit (unchanged); scheduled independent implementation review; complete suite; maintainer play-test of each device against DOSBox. Update status, roadmap and history and condense this plan only after acceptance. |

**Stop rules** (stop and ask the maintainer): a driver in the approved set is
missing from the CD data; a device needs a host MIDI device, or an emulator
that neither the pin nor a bounded MMModern renderer can supply without a new
subsystem; a required wait lacks a defensible output-independent model; a
device changes gameplay timing beyond Part 0's settled rules, or any save, RNG
or Event state; the drivers need a CPU emulator to reproduce; the route the
amendment approves for a family needs ScummVM `backends/`, `gui/` or
`OSystem`; the installed ids change any existing output, test or digest; an
Unknown needed by the approved set stays unsettled; a family exceeds one part.

## Tests and validation

- Unit tests without a device: id-to-driver selection for both tables, both
  programs and out-of-range ids; loose-file override; one notice per
  unsupported or incomplete device; caption and wait gates per digital id.
- Installed configuration, unchanged: the existing `promus` DRO comparisons
  and driver register streams, fade and load ordering, VOC and CD logical
  deadlines, card mixer transitions, the intro and ending observations, and
  the four M44 digests (which cover saved bytes only) through the existing
  audit baseline. `XeenFmOutputTests`' render-ahead and render-behind cases
  extend to the generalized boundary.
- Per family: its Part 0 oracle against the DOSBox captures; PCM hashes only
  guard regressions.
- A few original-data witnesses per family through a recording sink, run with
  an injected configuration, and the device-free timing test with and
  without output for every distinct timing implementation.
- The generic save round-trip stays unchanged.
- Each production part: affected targets and tests, the fast suite, then the
  complete `build-rel` suite through [tools/AGENTS.md](../tools/AGENTS.md),
  with `SDL_AUDIODRIVER=dummy`. This planning change checks links and
  `git diff --check` only.

## Decisions and risks

1. **Device scope:** the maintainer decides after Part 0, from its device
   table and recommended split. Devices left out keep the notice and may form
   a later milestone.
2. **Live switch:** not in M94. If the maintainer wants it later, it is an
   opt-in, approved deviation that leaves the `XEEN.CFG` choice as default.
3. **MT-32 ROMs:** MMModern cannot ship them. Recommended location:
   `%LOCALAPPDATA%\MMModern\roms`, beside `settings.ini`, so release and
   development builds find the same files; the maintainer decides before
   Part B. MT-32 versus CM-32L ROMs follow what Part 0 finds `rolmus` uses.
4. **Hardware identities:** names alone do not settle `specmus`, `wavemus`
   and `tapmus`; Part 0 labels each identity by evidence, never by guess.
5. **Technical risks:** mt32emu cost on the audio thread (render in bounded
   blocks, keep the 33 ms FM latency guard); OPL2 versus OPL3 register
   differences in Nuked; speaker drivers that busy-wait or reprogram the PIT
   in DOS, whose timing must be modelled on the clock; Part 0 overflowing its
   sessions; package size and licence notices.
