# Milestone 82 plan - Audio, part 1: FM music and effects

**Tier A. Status: planned (not started)**
Roadmap [step 12](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones. Baseline: clean `main`, `f2bee4b`, equal to `origin/main`,
plus the uncommitted reorder that made audio step 12 and Event dispatch
step 13 (`roadmap.md`, and the step reference in `milestone-81-plan.md` and
`project-status.md`).

## Goal and scope

The game plays its original FM music and FM sound effects, as a DOS player
with an AdLib-compatible card hears them, and the control panel's Efx, Music,
Speech and Text buttons work and are saved as in DOS. Digital sound (VOC
samples, voices), CD speech and CD tracks are the second milestone (M83).
Other devices (MT-32, PC speaker, the other original drivers) and switching
device while playing are roadmap step 16; M82 only keeps the design ready for
them (maintainer, 2026-10-21). Darkside music and the intros stay out.
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

Static analysis of `WORLD/XEEN.DAT`/`XEEN.EXE`, calibrated on known behaviors
(Reaper wait, config read, panel table, RNG; 2026-10-21,
`D:/MModern-re/audio-m82/dos-findings.md`); ScummVM inventory in
`D:/MModern-re/audio-m82/scummvm-inventory.md`. Pin
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, `engines/mm/shared/xeen/sound*.cpp`
and `engines/mm/xeen/map.cpp`. DOS overrides the pin where they differ.

| Rule | Evidence and decision |
| --- | --- |
| Device | **DOS, high:** `XEEN.CFG` word +0x04 selects the music driver (1 admus, 2 blastmus, 3 promus, 4 rolmus, 5 canmus, 6 specmus, 7 ibmmus, 8 wavemus, 9 tapmus, other ids nullmus); FM effect data belongs to the driver. The GOG install selects 3. The driver is loaded from INTRO.CC at startup (**medium-high**; the pin reads `blastmus` from the current archive: tagged difference, Part 0 confirms on the CD copies). MMModern only reads `XEEN.CFG`, never writes it (DOS rewrites it at the intro and ending). |
| Timer | **DOS, high:** every music driver sets PIT channel 0 to 72.80 Hz and chains the BIOS tick, which still averages 18.2065 Hz. The clock stays unchanged; the driver ticks at 72.80 Hz from the sample count. The pin runs 72 Hz: tagged difference. |
| Toggles | **DOS, high:** Efx, Music, Text and Speech live in the saved party block, not in `XEEN.CFG` (1530-byte block: +0x0e, +0x0f, +0x5f8 with 0 = on, +0x5f9). Efx on plays effect 20; Efx off stops nothing but gates FM effects and VOC samples. Music off silences at once (driver command 0); Music on restarts the loaded song from its start. Speech off forces Text on at redraw; Text off with Speech on is allowed; Speech off skips CD speech. Labels "on"/"off" are original text. **Medium-high:** Load replaces all four. **Unknown:** the New-game Text/Speech values, since the 812-byte Clouds block has no such bytes; Part 0 settles them and confirms Efx/Music offsets there. |
| Music choice | **DOS, high:** Clouds outdoors plays `outdoors.m`; indoors the wall-kind index table selects a name from the table in `XEEN.DAT`. No day/night. The same name (case-insensitive) continues, even with Music off. A new name fades the old song (command 207), waits for the fade and for CD speech, then loads it and plays it only when Music is on. Tag the Clouds-relevant differences from the pin (continuation rule, waiting for CD speech). **Unknown:** wall-kind index 5, a name missing from the archive, and what Music on plays after a name change while Music was off. |
| Pin's driver | **Unknown:** `SoundDriverAdlib` matches the FM drivers only in outline; three suspected defects (fade accumulator sign, FX subroutine base, FX countdown on music ticks), the 72 Hz rate and the driver archive. |

## Design decisions

- **MMModern's own FM driver over ScummVM's mixer and OPL emulator.** The
  pin's driver hard-codes the 72 Hz rate, the emulator choice through ConfMan
  and the suspected defects in its private logic; correcting them would need
  the first source patch to the pin. MMModern therefore implements the DOS FM
  driver's semantics itself, with the pin's driver as reference, and links
  only ScummVM's `audio/` mixer, streams, rate converter and OPL emulators,
  compiled from source like `cc_archive.cpp` and created with an explicit
  emulator id. `dependencies.md` lists the new sources and their licenses.
- **`OSystem` for the whole process.** ScummVM mutexes and the string pool
  need `g_system` with `backendInitialized()`; a minimal subclass (recursive
  mutexes, stubs elsewhere) is installed at startup, before any audio object,
  and lives until process exit, in tests too, so tests run the production
  configuration.
- **Owners.** The platform layer opens the SDL audio device and calls the
  mixer. A program-lifetime audio owner next to `XeenProgramState` (the
  loaded driver is running-program state in DOS) holds the device, the
  driver, the current song name and its buffer for the driver's lifetime; it
  is the one place songs and effects are requested. The map and location
  owners choose songs (map load, services, Inn), the title flow chooses the
  title song, and the toggles are party state in the snapshot. Requests cross
  to the audio thread through one queue.
- **Timing never depends on the device.** Where DOS waits (the song fade, and
  its wait for CD speech), the wait is computed from the configured driver's
  72.80 Hz tick count and the existing simulated CD-speech timing on the
  presentation clock, never by waiting on real output. Without an output
  device (tests, the SDL dummy driver) the game times, draws and saves
  exactly as with one; audio never feeds RNG, Events or saves.
- **Device choice ready, not exposed.** The owner creates and destroys the
  driver on request, keyed by the original device id, so step 16 can add
  devices and an opt-in live switch without new ownership. Ids outside 1-9
  are faithful silence (nullmus); a real device not yet supported plays
  nothing and reports "not supported yet" once.
- **Save v17 -> v18; schema 9 and content 14 unchanged.** The snapshot gains
  the four toggles. Restore applies them without replaying anything; the
  current song follows Part 0's Load rule. v17 and older are rejected.

## Parts and acceptance

Each part takes one fresh session; amend this plan if the scope changes.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | Read-only for code and original data. Compare admus, blastmus and promus (commands, instrument and effect tables, OPL2/OPL3 use, fade length) with each other and the pin's driver; settle the three defects and the driver archive on the CD copies. Inventory every DOS FM effect site with its id and MMModern owner. Settle every **Unknown** above, plus: title and location songs; whether input blocks during a fade; New/Load and the current song (Load with Music off sends no stop); a pending fade when Music goes off; whether Efx off removes VOC waits and how long entrance captions stay; a missing `XEEN.CFG`. Obtain DOSBox raw OPL (DRO) captures of a few songs and effects, with the maintainer if needed, and recommend an OPL emulator. Record rules in `docs/rules/`, quirks and tagged differences. |
| A - audio foundation | ScummVM audio sources, `OSystem`, SDL output, the audio owner and the FM driver at 72.80 Hz; songs for maps, locations, title and Inn by the DOS rules, with the fade. Acceptance: songs play and change as in DOS; same-name continuation; register streams match the DRO captures; identical timing and traces with and without a device. |
| B - toggles and save | The four panel buttons with original labels and rules; save v18; New and Load values; Speech off skips CD speech waits; Efx off as Part 0 settles for VOC waits and entrance captions. Text off is stored, shown, saved and coupled, but text stays visible until M83 plays CD speech (temporary restriction approved by the maintainer, recorded in `original-quirks.md`), so the DOS defaults never leave a player without text or speech. |
| C - FM effects | Every DOS FM effect site from Part 0's inventory, gated by Efx, in movement, combat, spells, items, dialogs and services; C1/C2 if large. Acceptance: each site's effect id matches DOS; none plays with Efx off. |
| D - closure | Dependencies, quirks and differences documents; M44 digest audit; scheduled independent implementation review; complete suite; maintainer play-test against DOSBox. Update status, roadmap and history and condense this plan only after acceptance. |

**Stop rules** (stop and ask the maintainer): the FM drivers differ audibly
(choice of default); promus needs OPL3 or two OPL2 chips; an Unknown stays
unsettled; the audio link needs ScummVM `backends/` or `gui/` code beyond
headers; a non-null `g_system` changes any existing output or test; the effect
inventory exceeds two parts; the fade wait cannot be modelled from the clock.

## Tests and validation

- Unit tests without a device: music selection for every Clouds map from
  original data (outdoor, each wall kind, continuation, Music off), toggle
  rules (Speech/Text coupling, Efx on effect 20), device ids and nullmus.
- Driver fidelity: per-tick OPL register streams for a few songs and effects
  compared with the DOSBox DRO captures (DOS evidence for the rate and
  defects). PCM hashes only guard regressions and may differ across platforms.
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
2. **Default FM driver:** if Part 0 finds admus, blastmus and promus audibly
   different, the maintainer chooses which one M82 reproduces.
3. **Missing `XEEN.CFG`:** DOS refuses to start; MMModern starts today. Part 0
   reports the options and the maintainer decides before Part A.
4. **Text off stopgap (approved 2026-10-21):** text stays visible until M83
   as a temporary restriction, recorded in `original-quirks.md` by Part B.
5. **Technical risks:** audio-thread races (one queue, no shared game state);
   ScummVM's transitive audio dependencies (QuickTime/WAV decoders, zlib) and
   GPL notices; latency between a request and the heard sound, which never
   delays gameplay; effect coverage drifting from DOS without the inventory.
