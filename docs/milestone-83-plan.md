# Milestone 83 plan - Audio, part 2: digital sound, voices and CD speech

**Tier A. Status: planned (not started)**
Roadmap [step 12](roadmap.md#next-arc-finishing-the-games-systems), second of
its two milestones, after [M82](milestone-82-plan.md). Baseline: clean `main`,
`6015668`, equal to `origin/main`.

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
authorize implementation. Independent plan and implementation reviews, the
complete suite and a maintainer play-test are required.

## Current state

- `XeenAudio` (program state) owns `PromusDriver` and `XeenFmSource`; the
  render side follows the device, the timeline driver follows the
  presentation clock ([audio rules](rules/audio.md)). `SdlAudioDevice` opens
  the default output at its own rate; `check()` rethrows an output error,
  which ends the session; a removed device leaves the game silent.
- Every digital sound already keeps its original length silently:
  `xeenVocDurationNanoseconds` times about 40 named samples in
  `XeenSmithFlow`, `XeenTrainingFlow`, `XeenServiceAnimation`,
  `XeenEntranceScene` and `XeenEventFlow`; `xeenCdSpeechDurationNanoseconds`
  times CD cues as `(end-start+25)/75` s
  ([approved](original-quirks.md#dev-cd-speech-delay)). Event opcodes 40
  PlayEventVoc (130 rows on 35 Clouds maps) and 60 PlayCD (207 rows on 25
  maps) show "not supported yet" notices ([coverage](coverage.md)).
- The four panel options are party state in save v18. Speech off already
  skips CD waits and Efx off already removes VOC-busy waits. Text off is
  stored but text stays visible ([restriction](original-quirks.md#sv-m82-visible-text)).
- The GOG root's `GAME1.INS`/`GAME2.INS` cue sheets list the data track and
  audio tracks 2-31 (disc 1) and 2-30 (disc 2) as `MUSIC/xeenNN.ogg`; the
  `GAME1.INS` line for track 21 lacks a space before `AUDIO`. A plain CD
  `GAME` copy has no audio tracks. msys2 provides `libvorbisfile`.

## Original rules and evidence

M82 Part 0 (`D:/MModern-re/audio-m82/dos-findings.md`) already settled:
`XEEN.CFG` word 0 = 10 selects the digital driver `prosnd` (Pro Audio
Spectrum, DMA 1, IRQ 7, no PIT use), loaded from INTRO.CC; the VOC wrapper
`0xdea9..0xdecd` gates playback and busy queries on Efx; CD speech goes
through MSCDEX (packet `0x3d9b4..0x3d9e8`) and is skipped with Speech off;
toggling Speech issues no CD stop (medium: a running cue continues); the
pending song load waits for CD idle. DOS overrides the pin
(`6814ee9b`, `engines/mm/shared/xeen/sound*.cpp`, `cmdPlayEventVoc`,
`cmdPlayCD`) where they differ.

| Rule | Status |
| --- | --- |
| VOC playback | **Unknown:** what `prosnd` does with a new sample while one plays (cut, ignore, queue); which VOC block types it plays; the output rate from the time constant; whether anything stops a sample (location exit, Load, New, combat, Escape). |
| VOC sites | **Unknown:** the full DOS list of sample calls with names and owners, and the PlayEventVoc index-to-name table. MMModern's ~40 timed sites are not proven complete. |
| CD speech | **Unknown:** whether the play packet uses track-relative or absolute disc addresses, which disc Clouds cues use, whether a new cue or a Load stops the running one, and whether VOC and CD play together. Known: Gunther is track 19, cue 370..2161, 18.213333 s. |
| Text off | **Unknown:** every reader of Text-off (`DS:f384`) and which text it hides: NPC dialogs, Event messages, service greetings; whether text still shows when a cue is the all-zero request or speech is skipped. |
| Balance | **Known:** promus sets PAS mixer registers (`0x23c..0x265`); DOSBox does not emulate the PAS mixer, so original hardware balance is out of reach. The maintainer's DOSBox (`sb16`, `sbmixer=true`) is the listening reference (roadmap, 2026-10-22). |

## Design decisions

- **One mixer in the existing owner.** `XeenAudio` stays the only audio
  owner. Its render side mixes the FM source, one digital voice and one CD
  stream into the device buffer, each converted to the device rate. Game
  requests cross through the existing queue. No new owner.
- **Timing never depends on the device.** Gameplay keeps the current header
  and cue durations on the presentation clock. A sample or cue starts audibly
  when requested and ends on its own; a late or missing device never delays,
  shortens or reorders gameplay. Traces, RNG and saves are identical with
  and without a device.
- **Own VOC decoder.** `XeenVocDuration.h` grows into a decoder producing
  unsigned 8-bit mono PCM by the same block rules (it already accepts the
  damaged `sphinx13.voc` descriptor that ScummVM rejects). Playback follows
  `prosnd` as Part 0 settles it.
- **CD tracks from the GOG cue sheets.** The installation reader, which
  already parses `.INS` for the data track, also maps audio tracks to their
  OGG files. Lines naming a track number and `AUDIO` are read as GOG wrote
  them, including track 21's missing space. `libvorbisfile` (static, BSD)
  decodes them; `dependencies.md` records it. Cue addresses map to sample
  offsets as Part 0 settles; Gunther is the calibration witness. If Part 0
  finds a track shifted against the cues, MMModern corrects it with a
  per-track offset in code, recorded with its evidence; the OGG files are
  never changed. DOSBox plays the same files, so applying a correction is
  the maintainer's decision.
- **Missing tracks.** A plain `GAME` copy or a missing or undecodable OGG
  keeps the cue's timing silently and shows the existing notice once per
  cue. DOS cannot run without the CD, so this case has no original behavior.
  Text off then shows the text (decision 3 below).
- **Text off as in DOS.** Each reader Part 0 finds hides text as DOS does.
  Remove the temporary restriction from `original-quirks.md` and
  `scummvm-differences.md` in the same change.
- **Balance as fixed gains.** FM, VOC and CD get fixed relative gains,
  measured from maintainer DOSBox recordings (ffmpeg loudness of FM-only,
  voice-over-music and speech-over-music clips) and confirmed by ear. They
  are constants, not options, and are recorded as DOSBox-referenced evidence.
- **Save v18 unchanged** (schema 9, content 14). A playing sample or cue is
  process state, as in DOS. If Part 0 shows that something audible must be
  saved, stop.

## Parts and acceptance

Each part takes one fresh session; amend this plan if the scope changes.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | Read-only static analysis of `WORLD/XEEN.DAT` and `prosnd`, calibrated on a known sample wait. Settle every **Unknown** above; inventory every DOS VOC call with name, gate and MMModern owner; measure every Clouds cue against its OGG (speech starting after the cue start and ending before its end) to find any per-track shift. A first look at Gunther: speech starts 0.10 s after the cue start, ends 0.47 s before its end, and the next line starts 0.02 s after the extra 25 frames. Ask the maintainer for a few short DOSBox recordings for the balance (FM alone, a shop voice over music, NPC speech over music). Record rules in `docs/rules/audio.md` and quirks. |
| A - digital samples | Mixer, VOC decoder, `prosnd` semantics, every inventoried site audible and gated by Efx, PlayEventVoc playing its sample instead of the notice. Acceptance: site table matches DOS; a sample's PCM matches the decoded VOC; traces identical without a device; M44 digests unchanged. |
| B - CD speech and Text off | Cue-sheet track mapping, OGG decoding, PlayCD and the NPC/service speech paths playing their cue, Speech off and Text off by the DOS rules, the missing-track notice. Acceptance: Gunther plays from 370 to 2161 at the right offset; Text off hides exactly Part 0's readers; the restriction is gone from the docs. |
| C - balance and output failures | Fixed gains from the recordings; silence with a notice on output failure and reopening a removed device (decisions 1 and 2 below); the rate-change crackle if the same mechanism fixes it. Acceptance: measured relative levels within an agreed tolerance of DOSBox; an injected device error leaves gameplay and traces unchanged. |
| D - closure | Dependencies, quirks and differences documents; census regeneration; scheduled independent implementation review; complete suite; maintainer listening play-test against DOSBox. Update status, roadmap and history and condense this plan only after acceptance. |

**Stop rules** (stop and ask the maintainer): an Unknown stays unsettled;
the VOC inventory exceeds one part; cue addresses need disc data the OGG
files do not keep (track offsets, pregaps); `prosnd` behavior would need a
gameplay wait on the device; playing audio would need saved state; Text off
hides text with no speech replacing it in a case Part 0 cannot explain.

## Tests and validation

- Unit tests without a device: VOC decoding (block types, damaged
  descriptor, continuation blocks), cue-sheet parsing (both GOG files, the
  track 21 line, a missing file), cue address to sample offset, mixer gains
  and clipping, `prosnd` overlap rules, the Text-off reader table, and the
  missing-track notice.
- Original-data tests: the VOC site table against Part 0's inventory;
  witnesses recording requested samples and cues through a sink with Efx and
  Speech on and off; Gunther's cue; one route timed with and without a
  device.
- An injected output failure and a device removal on the SDL dummy driver.
- The generic save round-trip stays green; M44 digests must not change.
- Each production part: affected targets and tests, the fast suite, then the
  complete `build-rel` suite through [tools/AGENTS.md](../tools/AGENTS.md).
  This planning change checks links and `git diff --check` only.

## Decisions

1. **Output failure** (maintainer, 2026-10-22): a failure during play
   silences all audio with one notice and the game continues, instead of
   ending the session. Gameplay is unaffected, because timing never depends
   on the device.
2. **Removed device** (maintainer, 2026-10-22): when SDL reports the
   removal, reopen the new default output at its own rate and continue the
   current song, instead of staying silent until restart.
3. **Text off without tracks** (maintainer, 2026-10-22; approved deviation):
   DOS always has the disc. When a cue cannot play, the text is shown, so a
   plain `GAME` copy never leaves the player with neither text nor speech.
   Part B records it in `original-quirks.md` and `scummvm-differences.md`.
4. **Balance reference** (decided, roadmap 2026-10-22): by ear against
   DOSBox; Part 0 only collects the recordings.
5. **Technical risks:** GOG's OGG rips may be shifted against the original
   tracks (a trimmed or kept pregap; Vorbis decoding itself removes the
   encoder's start padding), which Part 0's cue sweep checks;
   audio-thread races (one queue, no shared game state); static Vorbis in
   the release zip and its license notice; audible latency, which never
   delays gameplay.
