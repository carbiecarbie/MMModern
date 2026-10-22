# Milestone 83 plan - Audio, part 2: digital sound, voices and CD speech

**Tier A. Status: planned (not started)**
Roadmap [step 12](roadmap.md#next-arc-finishing-the-games-systems), second of
its two milestones, after [M82](milestone-82-plan.md). Planned on `6015668`,
committed in `87a77eb`, amended after the independent plan review
(2026-10-22).

## Goal and scope

The game plays the CD edition's digital sound as the installed configuration
does: VOC samples (shop and guild voices, entrance guardians, the death laugh,
the dream, Event voices) through the digital driver, and CD speech from the
disc's audio tracks, read from the GOG installation's OGG files. Text off
hides the text that speech replaces, ending M82's temporary restriction. FM
music, VOC samples and CD audio are mixed at a balance checked by ear against
DOSBox. An audio output failure and a removed output device get a decided
behavior. Out of scope: Darkside, the intros and the ending (step 15), the
other sound devices and a live device switch (step 16). Planning does not
authorize implementation. Independent implementation reviews, the complete
suite and a maintainer play-test are required.

## Current state

- `XeenAudio` (program state) owns `PromusDriver` and `XeenFmSource`
  ([audio rules](rules/audio.md)). The `XeenFmSource` queue drops requests
  without an FM driver, and its fade barrier holds every later operation.
  The device callback pulls 49716 Hz FM frames through one `SDL_AudioStream`.
  `SdlAudioDevice` opens the default output at its own rate; a callback error
  is stored and `check()` rethrows it, ending the session; an open failure
  leaves the game silent, and so does a removed device until restart.
- Known service, entrance, dream and death sites keep silent VOC timing
  (`xeenVocDurationNanoseconds`, about 40 named samples in `XeenSmithFlow`,
  `XeenTrainingFlow`, `XeenServiceAnimation`, `XeenEntranceScene` and
  `XeenEventFlow`); the complete DOS inventory is unverified. Event opcode 40
  PlayEventVoc (130 rows on 35 Clouds maps) shows a passive notice without a
  sample wait. Opcode 60 PlayCD (207 rows on 25 maps) shows a notice and
  retains its operands; the next NPC dialog starts the interval
  `(end-start+25)/75` s ([approved](original-quirks.md#dev-cd-speech-delay)),
  and NPC face animation while busy draws from the Journey generator.
  `XeenAudio::rebaseClock` clears its copy of the speech deadline at
  title/session handoffs.
- The four panel options are party state in save v18. Speech off skips CD
  waits; Efx off removes VOC-busy waits; Text off is stored but text stays
  visible ([restriction](original-quirks.md#sv-m82-visible-text)). Save
  snapshots exclude interpreter and presentation state.
- `GAME1.INS`/`GAME2.INS` list the data track and audio tracks 2-31 (disc 1)
  and 2-30 (disc 2) as `MUSIC/xeenNN.ogg`. `XeenInstallationDetector`
  accepts track 21's `TRACK 21AUDIO` line but keeps only the data track. A
  plain CD `GAME` copy has no audio tracks, and test configuration accepts
  one. MSYS2 UCRT64 provides static `libvorbisfile`, `libvorbis` and
  `libogg`; `tools/package-release.ps1` has a fixed notice list.

## Original rules and evidence

M82 Part 0 (`D:/MModern-re/audio-m82/dos-findings.md`) settled: `XEEN.CFG`
word 0 = 10 selects the digital driver `prosnd` (Pro Audio Spectrum, DMA 1,
IRQ 7, no PIT use), loaded from INTRO.CC; the VOC wrapper `0xdea9..0xdecd`
gates playback and busy queries on Efx; CD speech goes through MSCDEX
(packet `0x3d9b4..0x3d9e8`) and is skipped with Speech off; the pending song
load waits for CD idle. Gunther (Vertigo `(14,5)` West, track 19, encoded
370..2161) requests 25 sectors before the start: `xeen19.ogg` plays
3.600000..21.813333 s, 1366/75 s
([evidence](rules/evidence.md#death-and-cd-speech-captures-2026-10-13)).
DOS overrides the pin (`6814ee9b`, `engines/mm/shared/xeen/sound*.cpp`,
`cmdPlayEventVoc`, `cmdPlayCD`) where they differ.

| Rule | Status |
| --- | --- |
| VOC playback | **Unknown:** what `prosnd` does with a new sample while one plays (cut, ignore, queue); which block types it plays; the output rate from the time constant. |
| Lifecycle | **Unknown:** logical start, replacement, stop and busy queries for VOC and CD during combat, over FM effects, VOC with CD, across Efx and Speech toggles (a cue continuing after Speech off is medium confidence), Load, New, location exit, Escape, title entry and side changes. |
| VOC sites | **Unknown:** every DOS sample call with name, gate and owner, and the PlayEventVoc index-to-name table. |
| CD addressing | **Unknown:** track-relative or absolute MSCDEX addresses and the MSF bias; the disc Clouds cues use. DOSBox Staging builds cumulative disc positions from separate cue files (`AddTrack`); calibrate against the maintainer's installed DOSBox. |
| Text off | **Unknown:** every reader of Text-off (`DS:f384`) and which text it hides (NPC dialogs, Event messages, service greetings), including the all-zero cue and skipped speech. |
| Balance | **Known:** promus sets PAS mixer registers (`0x23c..0x265`), which DOSBox does not emulate; the maintainer's DOSBox (`sb16`, `sbmixer=true`) is the listening reference (roadmap, 2026-10-22). |

The established NPC page, mode-2, dismissal and all-zero-cue rules and the
approved CD interval without guessed latency stay as they are.

## Design decisions

- **One owner, independent paths.** `XeenAudio` owns the mixer and every
  playback resource; no new owner. One transport queue may carry requests,
  but the FM fade barrier holds only dependent FM operations, and VOC and CD
  never depend on FM being enabled. Requests own their payloads and never
  borrow Flow objects. The callback mixes at the device rate, with one
  resampler per source (FM 49716 Hz, VOC per time constant, CD 44100 Hz).
  File reads and decoding stay out of the callback; Part A specifies the
  buffering, callback shutdown and recovery.
- **Logical state on the presentation clock.** Retained cue operands (Event
  state), logical busy deadlines and render cursors (audio thread) are kept
  apart, each with the existing owner Part 0's lifecycle evidence points to.
  Gameplay waits use header and cue durations plus Part 0's transitions,
  never the device: traces, RNG and saves are identical with and without
  one. Playback that survives a title/session handoff rebases its deadline
  by its remaining duration; stopped playback is cancelled explicitly.
  Opcode 60 keeps retaining operands; invocation starts playback.
- **Own VOC decoder.** `XeenVocDuration.h` grows into a decoder producing
  unsigned 8-bit mono PCM by the same block rules (it accepts the damaged
  `sphinx13.voc` descriptor ScummVM rejects). Playback follows `prosnd`.
- **CD tracks from the GOG cue sheets.** The installation reader maps both
  discs' audio tracks to their OGG files (track numbers repeat across
  discs). MMSSFF conversion, the 25-sector preroll, disc and track choice,
  file-relative `INDEX` values and absolute MSCDEX addresses are documented
  separately. Static `libvorbisfile`/`libvorbis`/`libogg` through explicit
  CMake linkage decode them; this is a dependency-boundary choice (the pin's
  Vorbis decoder would not need `g_system` either), and the `g_system`-null
  guard stays. Speech energy alone never justifies a per-track correction;
  any correction needs evidence and the maintainer's approval, and the OGG
  files are never changed.
- **Text off as in DOS.** Each reader Part 0 finds hides text as DOS does.
  Whenever speech cannot be delivered (no tracks, a missing or undecodable
  OGG, output lost during a cue), the cue keeps its timing and the text is
  shown (decision 3).
- **Balance as fixed gains** for FM, VOC and CD, measured from maintainer
  DOSBox recordings and confirmed by ear. Before Part C, the plan records
  the capture settings, comparison windows, measurement method and the
  maintainer's numeric tolerance (decision 4).
- **Output failures** (decisions 1 and 2) distinguish a callback or
  conversion failure, a backend disconnection, physical removal, a default
  endpoint change (SDL 2.32's WASAPI backend may follow the default by
  itself) and a failed reopen. Device events are handled on the main
  thread; reopening waits for callback shutdown and keeps playback state.
- **Save v18 unchanged** (schema 9, content 14) while the saved layout and
  meaning stay the same; playing audio is process state. M44 digests are
  regression gates: any difference (Text-off presentation, waits, RNG) is
  investigated, never replaced automatically.

## Parts and acceptance

Each part takes one fresh session; amend this plan if the scope changes.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | Read-only static analysis of `WORLD/XEEN.DAT` and `prosnd`, calibrated on a known sample wait. Settle every **Unknown**; inventory VOC sites counted by owner; calibrate the DOSBox cue mapping; measure every Clouds cue against its OGG window. If the inventory is large, split Part A into A1 (decoder, mixer) and A2 (sites) before implementation. Ask the maintainer for short DOSBox recordings (FM alone, a shop voice over music, NPC speech over music). Record rules in `docs/rules/audio.md` and quirks. |
| A - digital samples | Mixer, resamplers, VOC decoder, `prosnd` semantics and lifecycle, every inventoried site gated by Efx, PlayEventVoc playing its sample. Acceptance: site table matches DOS; decoded PCM matches independently specified synthetic samples, lengths and rates, plus `sphinx13.voc`; simultaneous sources, replacement and stop, the FM barrier not holding VOC, several callback sizes; traces identical without a device. |
| B - CD speech and Text off | Cue mapping, OGG decoding, PlayCD and the NPC and service speech paths, Speech and Text off, the text fallback, handoffs. Acceptance: Gunther decodes `xeen19.ogg` 3.600000..21.813333 s; Text off hides exactly Part 0's readers and shows text when speech cannot play; the restriction is removed and the deviation recorded with an anchor, a `**ScummVM:**` tag and a differences entry; Load and fresh-process restore after passive speech replay nothing. |
| C - balance and output failures | Fixed gains; the failure contract; the rate-change crackle if the same mechanism fixes it. Acceptance: levels within the recorded tolerance; an injected failure on the dummy driver leaves gameplay unchanged; a Windows WASAPI play-test of removal, default change, text fallback and rate change. |
| D - closure | Dependencies, package-release notices and package versions, executable imports checked and the extracted release run without MSYS2 on `PATH`; quirks and differences; census regeneration; scheduled independent implementation review; complete suite; maintainer listening play-test against DOSBox. Update status, roadmap and history and condense this plan only after acceptance. |

**Stop rules** (stop and ask the maintainer): an Unknown stays unsettled;
lifecycle evidence invalidates existing logical waits; the VOC inventory
exceeds two parts; cue addresses need disc data the OGG files do not keep
(track offsets, pregaps); a gameplay wait would need the device; playing
audio would need saved state; Text off hides text with no speech replacing
it in a case Part 0 cannot explain.

## Tests and validation

- Unit tests without a device: synthetic VOC decoding (block types, damaged
  descriptor, continuation blocks); cue sheets (both discs, repeated track
  numbers, the track 21 line, bounds, preroll, EOF, missing and undecodable
  files); address mapping; resamplers, gains and clipping; queue barrier
  independence; lifecycle transitions; the Text-off reader table and the
  fallback.
- Original-audio tests require a configured GOG cue and audio source with
  `MUSIC/`: a missing configured directory fails configuration, missing or
  malformed required tracks fail validation, never a skip. Plain `GAME`
  runtime support stays, with its silent text fallback tested separately.
- Original-data tests: the VOC site table against Part 0's inventory;
  witnesses recording requested samples and cues through a sink with Efx and
  Speech on and off; Gunther's window; one route timed with and without a
  device; title/session handoffs with different clock epochs and pending
  song loads.
- Failure handling: injection on the SDL dummy driver; actual removal and
  default changes in the Windows play-test.
- Saves: the generic round-trip; same-process Load and fresh-process restore
  at eligible boundaries after passive speech; M44 digests as gates.
- Each production part: affected targets and tests, the fast suite, then the
  complete `build-rel` suite through [tools/AGENTS.md](../tools/AGENTS.md).
  This planning change checks links and `git diff --check` only.

## Decisions

1. **Output failure** (maintainer, 2026-10-22): a failure during play
   silences all audio with one notice and the game continues, instead of
   ending the session.
2. **Removed device** (maintainer, 2026-10-22): reopen the new default output
   at its own rate and continue the current song, instead of staying silent
   until restart.
3. **Text without speech** (maintainer, 2026-10-22; approved deviation): DOS
   always has the disc. Whenever a cue cannot be heard, the text is shown, so
   the player never has neither text nor speech.
4. **Balance** (roadmap, 2026-10-22): by ear against DOSBox. The numeric
   tolerance is the maintainer's, set before Part C.
5. **Technical risks:** a shifted OGG rip (a trimmed or kept pregap; Vorbis
   decoding removes the encoder's start padding), which Part 0's cue sweep
   checks; audio-thread races; static Vorbis in the release and its notice;
   audible latency, which never delays gameplay.
