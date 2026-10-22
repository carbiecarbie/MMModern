# Milestone 83 - Audio, part 2: digital sound, voices and CD speech

**Tier A. Status: completed and accepted by the maintainer (2026-10-22)**
after play-tests of A2, B and C; Part D changes nothing audible. Part 0
`9affd5f`, A1 `69816ac`, A2 `af987a0`, B `aab321f`, C `c1d0e83`,
documentation audit `bc2536a` and the closure commit. Roadmap
[step 12](roadmap.md#next-arc-finishing-the-games-systems), after
[M82](milestone-82-plan.md). Save v18, schema 9 and content 14 unchanged.

## Scope and results

VOC effects and voices, service/entrance speech, the dream and death laugh,
Event samples and GOG CD speech mix with the installed CD promus OPL3 path.
Text off hides the NPC body replaced by speech. Logical waits stay on the
presentation clock, independent of output, with no saved playback state.
Darkside, intros/endings (step 15), other devices and a device selector (step 16)
remain outside scope. Inn music awaits Sign In (step 14); fall/landing samples
await their presentation, and title lightning belongs to step 15.

| Part | Commit | Result |
| --- | --- | --- |
| 0 - evidence | `9affd5f` | Calibrated original driver, sites, CD addresses and lifecycle |
| A1 - decoder/mixer | `69816ac` | Fixed-layout VOC decoding, independent source mixing and logical timing |
| A2 - digital sites | `af987a0` | Existing owners publish the original samples and gates |
| B - CD speech/Text | `aab321f` | Cue decoding, refused starts, cross-file playback, Text-off readers and fallback |
| C - balance/output | `c1d0e83` | Fixed gains, removal/default/rate recovery and failure containment |
| Documentation audit | `bc2536a` | Audited documentation corrections retained |
| D - closure | This change | Removal-cleanup guard, package notices, census and documentation |

## Key decisions

These are the maintainer's decisions of 2026-10-22.

- `XeenAudio` remains the process owner. FM, VOC and CD are independent;
  FM fades never hold digital starts. Payloads own their data; decoding and
  file access stay outside callbacks, and shutdown joins before reclamation.
- INTRO `prosnd` reads PCM at +`0x20`, takes the low length word at +`0x1b`
  minus 10 modulo 65536, plus one sample, and fixes time constant 188
  (`1000000/68` Hz). Descriptor/block/header-rate parsing is deliberately
  absent. The unsplit count is an approved sub-millisecond DMA approximation;
  insufficient data is a hard error. [Evidence](rules/audio.md#installed-digital-driver-command-and-length-rules).
- Both GOG cue sheets retain disc identity, repeated track numbers and
  absolute MSCDEX addresses. Static libvorbisfile/libvorbis/libogg decode
  bounded OGG windows. Gunther uses xeen19.ogg 3.600000..21.813333 s; map 49
  crosses files. Refused starts, including map 30's track-2 cues, create no
  busy interval. Missing/undecodable audio retains the requested interval.
- Retained operands and surviving playback follow the original lifecycle;
  same-process handoffs rebase remaining deadlines, Load/New never replay
  saved audio, actual disc changes stop/clear CD, and fresh processes start idle.
- Resource-read CD stop/restart cycles, including audible restarts, are
  deliberately omitted everywhere. The nine mode-0 PlayCD/NPC pairs retain
  final dismissal stops. F8 replay is omitted by maintainer decision.
- Unavailable speech, including output loss, restores readable text without
  altering NPC polling or logical timing (approved delivery fallback).
- FM/VOC/CD gains are 0.5/0.01875/0.34375. Against the maintainer's DOSBox
  Staging captures, VOC/FM is -0.7 dB and CD/FM agrees within ±0.3 dB,
  within the approved ±1.5 dB tolerance. This is not DOS hardware evidence.
- Output failures silence all audio with one notice and keep gameplay running.
  Removal reopens the current default at its rate, retaining the FM song;
  VOC/CD stay silent until their next start. Default/rate changes preserve
  source cursors, accepting one small converter discontinuity. FM latency
  retains its 33 ms regression guard. Removal cleanup exceptions use the
  same permanent-silence fallback.

The small, inaudible SDL 2.32 resampler rate error remains unchanged since
M82 by maintainer decision (2026-10-22). The 512-frame refills remain;
a bounded converter is a possible later fix.
See the [known limitation](rules/audio.md#sdl-resampler-rate-limitation).

## Validation and acceptance

Independent plan and implementation reviews are complete. The maintainer
accepted M83 on 2026-10-22 after play-testing A2, B and C. Closure checks cover
injected dummy-driver failures, source conversion/latency, static dependency
notices/imports, an extracted package run without MSYS2 on PATH, regenerated
coverage and validation via [tools/AGENTS.md](../tools/AGENTS.md).
Closure validation passed: affected tests 3/3, fast suite 231/231 and the
complete build-rel suite 269/269 (225 seconds, unsandboxed lightweight runner).
The extracted package opened the title and exited normally with MSYS2 absent
from PATH; imports retain only SDL2, zlib and winpthreads as non-system DLLs.
Static Vorbis/Ogg licenses and package versions are included. No release is
published or tagged.

M44 regression expectations stay unchanged by closure: mainland
`c77d601069a5d49e0bf799b26e1842ae2eddfff98fa7d90d3a6de5374bc8be94`,
with services, temple and roundtrip retaining their previous digests. M83 B's
approved replacement met both conditions: the first divergence at Myra, map 23
(9,11) West, is explained by the calibrated Text-off face-before-hold order;
a Text-on run reproduces the former digest, with services, temple and roundtrip
unchanged. The generic save round-trip and exact native continuation remain gates.

Settled rules, approved approximations and remaining gaps live in
[audio](rules/audio.md), [quirks](original-quirks.md) and
[ScummVM differences](scummvm-differences.md), rather than in this closure summary.
