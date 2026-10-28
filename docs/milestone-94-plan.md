# Milestone 94 plan - Other sound devices

**Tier A. Status: implementation authorized (2026-10-27); Parts A1 and A2 done, B not started; maintainer acceptance pending.**
Roadmap [step 19](roadmap.md#next-arc-finishing-the-games-systems).
Evidence/amendment baseline: clean `main`, `b4448b8`, equal to `origin/main`.
The maintainer approved this amendment after its independent plan review
(`dfd05c1`).

## Goal and approved scope

Both original programs select music/effects and digital drivers from the two
`XEEN.CFG` ids. The installed CD pair **3/10** preserves its current output,
logical timing, tests and digests exactly. Scope approved 2026-10-27:

- **Part A1/A2:** generic selection in DAT and EXE; `nullmus` and `nullsnd` with
  their logical behavior; OPL music 1 `admus`, 2 `blastmus`, 3 `promus`
  (unchanged), 6 `specmus`; DMA digital 6 `blastsnd`, 7/10 `prosnd`.
- **Part B:** music 4 `rolmus`, direct pinned mt32emu with the player's ROMs.
- **Deferred:** music 5 `canmus`, 7 `ibmmus`, 8 `wavemus`, 9 `tapmus`;
  digital 1 `ibmsnd`, 2 `adsnd`, 5 `source`, 8 `specsnd`, 9 `covxsnd`.
  DAT digital 9 uses the nullsnd stopgap; EXE 9 takes the exception below.
  Each deferred path behaves exactly as nullmus/nullsnd: silence, no driver
  busy/fade/load waits and fixed BIOS rate; deferred music uses nullmus's
  own mixer transaction, deferred digital has none of its own. Each gives
  one standard "not supported yet" notice. This is the dated
  [approved temporary restriction](original-quirks.md#dev-m94-deferred-null).
- **Digital 3/4:** not installer choices, outside M94. DAT follows `nullsnd`;
  EXE digital **3/4/9** gives the standard notice and returns to title instead
  of the DOS fatal disk error ([approved deviation](original-quirks.md#dev-xeen-unavailable-digital)).

No live device switch, host MIDI, extra Darkside content, new ownership layer
or save-format change. `kJourneyVersion`, schema and content stay unchanged.

## Current state and known original rules

[XeenAudio](../src/app/XeenAudio.cpp) currently implements installed promus
and prosnd, with M82's presentation-clock timeline and sample-driven render
split, owned PCM/CD sources and XeenCardMixer; `g_system` stays null. Other
music choices have notices. The digital id currently mainly controls caption
gates; it does not yet select a native digital profile.

[Audio rules](rules/audio.md#other-original-sound-devices) own the full tables,
evidence labels, clocks and capture limits. Part 0 evidence under
`D:/Projetos/MModern/MModern-re/m94/` is summarized here without original bytes:

| Rule | Settled evidence / consequence |
| --- | --- |
| Installer identities | Music 1 Adlib, 2 Sound Blaster, 3 SB Pro, 4 LAPC-1/MT-32, 5 Canvas/GS, 6 PAS, 7 speaker, 8 Wave Blaster, 9 TAP-10. Captions are not chip protocols: `canmus` is GS-capable MIDI, not CMS; selected INTRO `specmus` is byte-identical to `blastmus`. |
| Selection/resources | DAT and EXE load selected INTRO drivers, loose-file override first, no cross-archive fallback. All 18 names are in INTRO, 15 in XEEN, none in DARK. Inactive XEEN copies differ. Digital 6 is blastsnd; 7/10 prosnd; DAT 3/4 nullsnd versus absent EXE rolsnd/cansnd; DAT 9 covxsnd versus absent INTRO covsnd in EXE. |
| CFG/handoff | Read/installer write is 29 bytes. DAT Other Options D and ending, plus EXE's Darkside seen-store, write 30; Clouds C does not. Audio ids/ports do not change in these rewrites. Program starts initialize music then digital once; shutdown reverses them. Ordinary DAT New/Load does not reinitialize. MMModern never writes XEEN.CFG. |
| Ports/mixer | Music init receives musicPort, flag, extraSegment, digitalPort. Promus/nullmus mixer uses musicPort; other flag consumers digitalPort; canmus has no mixer block. Command 0x100 exists beyond promus, but only promus changes card FM gain. |
| Approved clocks | All approved music, including nullmus, use PIT divisor 16390, BIOS accumulator +16390/carry; IRQ 1193182/16390 Hz, BIOS average 1193182/65536 Hz. DMA 6/7/10 do not take over PIT0. **No approved driver changes BIOS tick rate.** |
| Deferred clocks | Ibmmus type 0/1 freezes BIOS; type 2/3 uses divisor 1193 with chaining. Ibmsnd 81/+328, adsnd 81/+324 or 162/+649, source 1364/+5453 change forwarded rates; special speaker pairings can freeze BIOS. These are later-milestone facts, not fixed-clock M94 paths. |
| Driver differences | Mono music patches emit 11 bytes; admus level commands use selector 2, blastmus/specmus selector 5. Promus has two banks/22-byte FX. Admus postprocess and mono active-only fade need profiles. Rolmus has different MIDI handlers, CC11/cache-driven fade with 128-carry bound, own timed FX omissions, inclusive effect bound 180 and its own tables. |
| Digital counts | With L at +1b, PCM at +20: blastsnd count is uint16(L-14), fixed DMA 1; prosnd uint16(L-10), CFG DMA. Count+1 duration at 68 us is medium confidence. The [approved unsplit approximation](original-quirks.md#dev-prosnd-dma-timing), extended to blastsnd on 2026-10-27, omits DMA page continuation; retain each driver's other busy/failure semantics. |

## Routes, bounds and timeline/render boundary

**Part A1, selection and digital.** Pre-approved split, 2026-10-27; A1 and A2
each run in their own session with the usual validation. Before any refactor,
record a device-free golden of installed **3/10 at b4448b8 behavior** over a
fixed clock script: promus register trace, mixed FM+VOC+CD PCM hash, mixer
gains and VOC/CD/fade deadlines. All Part A changes assert it unchanged.
A1 covers selection in both programs, null drivers, the deferred stopgap,
EXE 3/4/9 notice/title return, notices, reference mixers and DMA digital.
Shared owned PCM transport gets driver-specific count/DMA/busy semantics
for 6 versus 7/10, using the approved unsplit count-plus-one approximation.
Add the read-only developer option to select another CFG for play-tests and
document it in [development.md](development.md). It feeds **every CFG reader**:
XeenAudio, XeenCutsceneData's digital/music ids for caption gates, and the
missing-CFG refusal. The selected file must satisfy that refusal; do not
silently use the installed file. MMModern never writes `XEEN.CFG`.

**Part A2, mono OPL.** Extend the existing native driver with resource-driven
one/two-bank, patch/FX and postprocess profiles and the DRO oracle; keep
promus's path unchanged. Use the pinned Nuked boundary in OPL2 mode for
1/2/6, existing NEW-mode OPL3 for 3. Mono profiles reuse XeenOpl3's
**two-native-sample buffered writes**; their DOSBox-reference nominal helper
I/O cost is 23.875 us, not an exact reproduced DOS interval
([spacing quirk](original-quirks.md#promus-write-spacing)). Specmus aliases
only the authenticated INTRO blastmus profile. Do not execute original code
or import a DOS CPU/DMA machine. A new approximation or a family exceeding
its approved part remains a stop.

**Part B, one bounded MT-32 part.** A native rolmus MIDI/FX/transport profile
may share safe flow helpers, not the pin's SoundDriverMT32 interpreter. Use
pinned `audio/softsynth/mt32` (mt32emu 2.7.0) directly, including its ROM/queue/
render support; bound its `common/util.h`/ARRAYSIZE dependency. No OSystem,
ConfMan, host MIDI, backends or ScummVM engine coordinator. Document new
sources/licences in dependencies and the release package. ROM loading,
synth open and allocations stay outside the render callback.

XeenAudio remains the owner. The externally supplied presentation clock
answers logical ticks, busy, fades and pending loads. Register/chip/PCM/MIDI
render cursors use output samples. Immutable, bounded queues preserve event
order and timestamps; render-ahead cannot consume future events and
render-behind cannot extend game waits. MT-32 synth state survives EXE/DAT
handoffs: repeat driver traffic, not a new synth/factory reset. Missing output
or ROMs gives one notice and silence without changing logical timing.

For rolmus use the **DOSBox reference** healthy intelligent-MPU model,
explicitly not physical hardware timing: FF reset completion at 14 ms;
ready/ACK state from command pending/response queue; original FFFF polling
counters and 176 port 61 reads per opcode-B address/data byte. **Maintainer
timing approximation, 2026-10-27:** virtual I/O charges (0.975 us/read,
0.725 us/write at the reference 80000 cycles) timestamp MIDI for the renderer
only; they never advance, rebase or add elapsed game presentation time.
The logical init/upload barrier is **ordering-only**: enqueue the complete
nine CC7 messages and 13 uploads/1688 bytes before subsequent notes. The
renderer serializes them, including its reference MPU reset deadline; game
code never waits for their render deadlines or output completion. DOS really
busy-waits on MPU polling while BIOS time runs; omitting that game-visible
cost is an approved approximation, as for OPL helper delays. The observed
3-ms upload span is below one driver tick, not a bound on total DOS init.
Select mt32emu immediate
MIDI delay mode and schedule explicit timestamps, avoiding its eight-bit
transfer delay and any extra 0.540160-s physical-wire sleep. The 1-ms capture
bounds omit exact init entry/return and instruction costs. Stop if renderer
bookkeeping cannot meet them, or an independently required game-visible wait
remains unbounded. [Timing policy](original-quirks.md#dev-audio-io-cost-clock).
OPL helper costs likewise affect chip write spacing only. No I/O cost is
added to the logical tick/busy/fade/CD clocks; installed 3/10 stays unchanged
byte for byte and never enters the rolmus transport.

**Maintainer reference-topology decision, 2026-10-27, not DOS evidence:**
N below is digital 0, deferred 1/2/5/8/9, DAT 3/4 and out-of-range ids.

| Music | Digital 6 | Digital 7 | Digital 10 | Digital N | Synthesis mode (DOS static) |
| --- | --- | --- | --- | --- | --- |
| 1/2/6 | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer | OPL2, one bank |
| 3 | SB16 mixer | SB16 mixer | Installed SB16 mixer | SB16 mixer | NEW-mode OPL3 |
| 4 | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer | MT-32; no OPL music/FX |
| nullmus (0/out-of-range music) | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer | No chip output; logical timer retained |

Promus's SB16 reference overrides the digital-card rule for **every** digital
id, without changing that id's transfer/notice/caption semantics. EXE digital
3/4/9 takes the notice/title exception instead of starting EXE audio. Deferred
ids use the [null-driver temporary restriction](original-quirks.md#dev-m94-deferred-null),
including nullmus's mixer transaction and no deferred-driver waits or clock
changes. Use the null row for deferred music. Mixer writes keep
their actual CFG port/flag consumers. CD is independent of digital id. With
no mixer its card multiplier is 1.0; this removes only card attenuation,
retaining source base g=1/3 and FM 1.5*g. Present mixers follow their reference
registers: SB2 skips mixer ports (all card gains unity); SB Pro init gives
master/FM/CD unity and DAC -11 dB; installed SB16 keeps DAC -24 dB.
MT-32 bypasses SB FM gain. A1 adds a unit test per reference mixer, including
reset, flag/port gating and init gains. These gain mappings are **DOSBox
Staging 0.83.0 reference**, not physical-card measurements. See the
[combination rules and evidence](rules/audio.md#reference-combinations).

## Oracles and acceptance

Detailed measurements and distinctions are in [audio](rules/audio.md#capture-oracle-topology-and-limits).
All logical count, byte, register-order, fade and barrier assertions are exact;
output hashes guard regressions only, not physical sound equivalence.

| Part | Capture oracle / tolerance |
| --- | --- |
| A2 OPL | Exact ordered register/value writes per logical tick. Project through DRO's map/cache/duplicate filtering; align each compared window (song start, stable title, outdoor key-on) from its matching stream, with 1 ms tolerance within that window. Foreground command writes compare by order only. One origin per program is intended, not demonstrated by these captures. Static helper I/O delays and a render test cover sub-ms spacing/key-off visibility, not DRO. Preserve installed two-sample spacing/energy acceptance; calibrate new-family render/listening separately. |
| A1 DMA | Exact L-14 versus L-10 counts and logical busy without output, under the approved unsplit approximation (count+1 duration: medium confidence). Crodo1/2 predict 3.981740/3.794944 s for blastsnd; observed next-slot offsets fit a 15.8-ms video/audio publication bound. This does not distinguish four samples or allocation-specific splits. Waveform is separate from the busy oracle. |
| B MIDI | Exact nine CCs, 13 DT1 packets/1688 bytes, checksums, effects and fades. Two captures show uploads spanning 0..3 ms and 25301..25304 ms; all endpoint bins fit the virtual I/O schedule within 1 ms and unknown sub-ms origin. Validate init/fade/load barriers without output. Same-ROM listening separately; no new universal waveform tolerance certified. |

Capture gaps are explicit: no Gunther/CD segment in rolmus or specmus, no
mixer screenshots, and no exhaustive identified hit/spell matrix. Source
settles routing; fill necessary waveform/effect witnesses during family
acceptance before claiming audible equivalence. Do not request deferred
families' captures or turn those open questions into M94 implementation.

Installed-configuration checks stay unchanged: promus DRO/register streams,
fade/load ordering, VOC/CD deadlines, card mixer, intros/ending observations,
render-ahead/behind behavior, generic exact save round-trip and four M44
saved-byte digests. Test both id tables/programs, defaults/out-of-range ids,
loose override, caption/wait gates, notices, silent drivers and each distinct
logical transport without a device. Extend `XeenFmOutputTests`' render-ahead
and render-behind cases to the generalized boundary. Run the device-free
timing test **with and without output for every distinct timing
implementation**. A few original-data witnesses suffice;
no per-feature process-continuation scaffolding or route certification.
Pass new DRO/SMF captures to tests by external path, as PromusDriverTests
does; keep them under MModern-re, never commit them to the repository.

Each production part: affected targets/tests, fast suite, complete build-rel
CTest via [tools/AGENTS.md](../tools/AGENTS.md), SDL_AUDIODRIVER=dummy,
and maintainer play-test versus DOSBox. Closure includes the scheduled
independent implementation review and updates dependencies/licences, quirks,
differences, status, roadmap and history only after acceptance. This amendment
runs links/diff checks only.

## Decisions and stops

1. **Scope/reference cards:** fixed above by the maintainer; no Part C in M94.
2. **Live switch:** excluded; a later opt-in decision.
3. **MT-32 ROM location and accepted runtime sets:** still for the maintainer
   before Part B; recommended location
   `%LOCALAPPDATA%\MMModern\roms`. Comparison set: **MT-32 control 1.07 + MT-32
   PCM**, explicit `mt32_107`; auto would choose available CM-32L 1.02.
4. **Stops:** contradictory calibrated evidence; missing approved resource or
   needed capture; an unbounded/output-dependent wait or unexpected MPU
   traffic; save/RNG/Event/BIOS-rate changes; a new subsystem/CPU emulator,
   disallowed ScummVM dependency or family exceeding one part; any installed
   behavior/test/digest change; **an Unknown needed by the approved set stays
   unsettled**. Reference timing bounds are not hardware proof.
5. **Deferred questions:** GS synth/bank, Wave codebook target, TAP model 56,
   speaker clocks/PWM, ADSND branch, PAS clock, DSS FIFO and Covox address
   stay recorded for a later milestone. No substitute emulator,
   renamed resource or silent correction is authorized.

## Technical risks

Bound mt32emu work on the audio thread and retain the **33 ms FM latency
guard**; ROM/open/allocation work stays outside the render callback. Verify
Nuked's OPL2 versus OPL3 register and rendering behavior rather than treating
bank selection alone as sufficient. Keep the pre-approved A1/A2 bounds;
stop if either grows beyond its part or requires a new subsystem.
Track package size and include the new source licences and required notices.
