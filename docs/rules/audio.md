# Audio

This chapter describes original audio behavior and its evidence. It does not
claim that audio is implemented. See the [milestone 82 plan](../milestone-82-plan.md)
for scope and the [evidence labels](README.md#how-to-interpret-evidence).

**Maintainer decision, 2026-10-21:** M82 reproduces CD `promus` (music device
3) in OPL3 stereo on the pinned Nuked emulator. `admus`, `blastmus` and
other devices belong to roadmap step 16. Their byte identities below are
retained evidence, not a requirement to implement or compare those drivers
audibly in M82. This decision resolves the first hardware stop.

## CD FM driver hardware

**DOS static, high confidence (2026-10-21):** the CD `INTRO.CC` resource
`promus` uses two FM register banks and explicitly enables OPL3 mode. Its
complete behavior cannot be reproduced by one OPL2 chip. Whether the same
driver also works correctly on physical dual-OPL2 hardware is **Unknown**;
this check did not observe hardware or DOSBox playback.

The installed `WORLD/XEEN.CFG` selects music device 3 at word offset `0x04`
and port `0x220` at word offset `0x06`. In `WORLD/XEEN.DAT`, the device
dispatch at `0xb57d..0xb59d`, with table at `0xb9cc`, selects `promus` for
device 3. Its resource name is at file offset `0x51f32` (DS:`0x46a2`).
Startup selects `intro.cc` at `0xb4d2..0xb4dd`, with its name at `0x51f21`
(DS:`0x4691`). **DOS static, high:** `0xd6be` passes this to `0x45d9e`,
which opens resource-archive slot 2. The early `promus` load at
`0xb4f3..0xb503` uses `0xe035 -> 0x3ff37 -> 0x45eb0`; device 3 reuses that
buffer at `0xb58e`. The resolver first tries a loose file
(`0x45ed2..0x45ed7`), then compares the first four characters against
`MAZE` (`0x45f4c..0x45f93`, string at `0x45b7e`). Only MAZE names use
current/save archive slot 0; `promus` uses resource slot 2, still INTRO.CC.
There is no intervening XEEN.CC switch. This settles the installed copy;
a loose `promus` would override it.

The following offsets are relative to the decoded **INTRO.CC promus resource**,
not to XEEN.DAT or the raw CD image:

| Offsets | Observed program operation |
| --- | --- |
| `0x226..0x238` | Initialization stores the supplied music I/O base. |
| `0x4d0..0x4e7` | Writes the register number to base and its value to base + 1. |
| `0x4e8..0x502` | Writes the second bank through base + 2 and base + 3. |
| `0x2a1..0x2a6`, `0x2cb..0x2d0` | Writes second-bank register 5 first with 0, then with 1: OPL3 NEW mode off, then on. |
| `0x2d1..0x33c` | Sets channel registers C0..C8 to `0x20` in the first bank and `0x10` in the second: opposite stereo outputs. |
| `0x525..0x54a` | Separate frequency/key-on write paths address A0/B0 in each bank. |

The pin's `audio/softsynth/opl/nuked.cpp::OPL3_WriteReg` corroborates register
`0x105` as NEW mode, and `OPL3_ChannelWriteC0` interprets the stereo bits.
Pinned `SoundDriverAdlib` instead constructs `OPL::Config::create()` with
the default `kOpl2` type (`audio/fmopl.h`). That driver is not evidence that
the DOS `promus` hardware path needs only a single OPL2 bank.
See the [tagged comparison](../original-quirks.md#sv-cd-promus-fm-banks).

## CD resource identities and calibration

**DOS resource read, high confidence:** both CD archives contain all three
FM drivers, and each corresponding INTRO/XEEN pair differs. Different bytes
alone do not establish an audible difference or settle their command semantics.
Offsets below are within the ISO files' CC archives, before payload
XOR decoding; lengths are decoded resource lengths.

| Resource | INTRO.CC offset / length | XEEN.CC offset / length |
| --- | --- | --- |
| `admus` | `0x6e7ecb` / 6300 | `0x521ad2` / 6161 |
| `blastmus` | `0x6e9a90` / 6202 | `0x566293` / 6162 |
| `promus` | `0x6f041a` / 9381 | `0xad1b5a` / 9137 |

Source: `GAME1.GOG`, 2352-byte sectors with 2048-byte ISO payload at +16;
ISO `/GAME/INTRO.CC` at LBA 5553, length 12393360, and `/GAME/XEEN.CC` at
LBA 11620, length 13438429. Directory entries and CC members were reread;
all six payloads matched the existing external work copies. INTRO `promus`
SHA-256 is `16bc0ca1533690d59009dd5cb567a76577310e33e65c719c0bb3dc4a62e2b69b`;
XEEN `promus` is `400f50cbbcbce1896aa72e0af4e3fcd5ff873b2128e00cb269bc7f345b530bc2`.

Calibration rechecked the known Reaper two-BIOS-tick wait at DAT
`0x2ac94..0x2ac9c` and its INT 1Ah elapsed-time helper at
`0x440cd..0x440f4`. DAT identity remains the SHA-256 in
[the evidence baseline](evidence.md#exact-baseline). ScummVM checkout HEAD
is `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; the inspected driver and OPL
files have no local changes. The initial research baseline is `61112d9`;
the amended-plan baseline is `d5bba594106dd0946c90cbd0b1c8629d16deaf71`.
Reproducible checks, full resource hashes and disassembly remain outside
the repository in `D:/MModern-re/audio-m82/part0-boundary-check.py`,
`part0-driver-identity.json` and `intro-promus-part0.dis`.

## Promus commands and voices

**DOS static, high:** offsets in this section are relative to decoded INTRO
`promus`. Header entries at +0, +3, +6, +9 and +12 lead respectively to init
`0x226`, shutdown `0x344`, song command `0x3ca`, effect start `0x382`, and
interrupt handler `0x693`. Values below are decimal unless prefixed `0x`.

| Song command | Operation (`0x3ca..0x459`) |
| --- | --- |
| 0 | Stop music, cancel fade, key off voices 0..6 in both banks. Does not stop FX voices 7/8. |
| 1 | Restart the loaded song at offset 0; clear music countdown, return stack and fade speed. |
| 2..255 | Set fade speed to this byte and remaining fade steps to 63, even if music is not playing. Preserve the fade phase byte. |
| `0xffe0` | Return fade speed; zero means no fade remains, not necessarily no music. |
| `0x100` | Set the hardware mixer level from the next argument; write mixer register `0x26` through base+4/+5. This is separate from per-voice OPL attenuation. |
| Other values above 255 | Treat the value as the new song segment and restart at offset 0. |

For command `0x100`, let h be `((argument & 0xf0) + 0x60) mod 256`;
the mixer byte is `h + (h >> 4)` (`0x40f..0x438`). Init's nonzero mixer
flag (installed XEEN.CFG word +`0x14` = 1) writes mixer registers
`0x2e/0x22/0x26` to 255 and register `0x04` to 153 (`0x23c..0x265`).

There are nine logical two-operator voices, each backed by one channel in
**each** OPL bank: music uses 0..6, effects use 7/8. Notes write both banks;
each bank has its own patch and carrier attenuation. Init sets NEW=1,
four-operator mask=0 (`0x33d..0x342`) and rhythm off. Every first-bank voice
gets C0 routing bit `0x20`; every second-bank voice gets `0x10`. Patch
loads OR these bits into their respective C0 values (`0x5e7`, `0x678`),
rather than clearing any opposite routing bit supplied by a patch. Thus
the routing rule is per bank for all voices, not a per-instrument pan table.
The pin's Nuked maps these to outputs B and A respectively.
Music note/patch opcodes explicitly ignore voices above 6; level and slide
commands have their own guards, so the allocation is not a blanket filter
on every music opcode. A structural survey of the 20 extracted songs found
no stream overruns; ignored voice-7/8 note operands were excluded when
checking the actually played notes against the frequency table.

Effect start accepts IDs **0..180 inclusive**, via the little-endian offset
table at `0xadc` (`0x39e..0x3aa`). These 181 entries reference 68 distinct
stream starts. IDs 7..10 are ignored while an effect is active; any other
admitted ID replaces it. Starting resets the FX countdown and both slide
flags, keys off FX channels in both banks and sets their carrier levels to
63 (`0x382..0x3c9`, `0x489..0x4cf`). An out-of-range ID is ignored.
The table is part of the runtime driver resource; it must not be embedded.

The compatibility word at resource +`0x0f` is initially zero. Several FX
operations and the slide loop skip voice 7 if it is 2. This is not an
OPL3/OPL2 selector. Its use by other device combinations is outside M82.
The extra header entry +17 leads to `0x8fe`: it loads one first-bank patch
at `0x24a5 + 11 * index` for `voice & 15`. That address starts beyond this
resource's end. It is not the ordinary inline instrument table. The game's
FM trampolines expose only entries 0/3/6/9; the installed `prosnd` stores
the music segment at its +`0x0d` but never reads it in its decoded code
(`0x3b..0x36b`). No use of this auxiliary entry was found on that installed
path. An external caller/data contract for other devices is not established.

## Song and effect bytecode

**DOS static, high:** high nibble selects the command; low nibble is `v`
(voice or slot as appropriate). The independent dispatch tables are at
`0x1e6` (music) and `0x206` (FX). Operand lengths exclude the command byte.
This is a behavioral description, not copied original program code.

| Nibble | Music | FX | Handler offsets |
| --- | --- | --- | --- |
| 0 | Call song-relative little-endian 16-bit offset; 16-frame return stack | No-op, no operand | `0x92b`, `0x6f8` |
| 1 | Yield with countdown v, or next byte if v=0 | Same, separate countdown | `0x94e`, `0x996` |
| 2 | Define instrument slot v; consume 26 bytes | Define slot v; consume 22 bytes | `0x960`, `0x9a8` |
| 3 | No-op | Selector and attenuation, two bytes | `0x6f8`, `0x812` |
| 4, 5 | Skip two bytes | No-op | `0x6f6`, `0x6f8` |
| 6 | Skip one byte | Set note from one byte; faulty frequency-cache indexing, below | `0x6f7`, `0x714` |
| 7 | No-op | Key off cached note | `0x6f8`, `0x730` |
| 8 | Consume one ignored byte and key off music voice v<7 | Set note from one byte | `0x755`, `0x776` |
| 9 | Note-off/on from one note byte plus one ignored byte, v<7 | Note-off/on from one byte | `0x795`, `0x7d1` |
| A | Selector and attenuation, two bytes | No-op | `0x812`, `0x6f8` |
| B | Skip through and including terminator `0xf7` | No-op | `0x6ec`, `0x6f8` |
| C | Load instrument indexed by following byte, v<7 | Load instrument indexed by following byte | `0x8ab`, `0x8d1` |
| D | Disable voice slide | Same | `0x89d` |
| E | Enable slide; rate byte then signed **big-endian** 16-bit delta | Same | `0x863` |
| F | `0xff` returns, or loops to song offset 0 with empty stack; other Fx ends stream | `0xff` loops to initial effect offset; other Fx ends stream | `0x971`, `0x9b9` |

Music definitions reserve 26 bytes but the two bank loaders consume only
the first 22: two consecutive 11-byte patches (`0x54b..0x67d`). Each patch
contains five operator bytes for operator 1, five for operator 2, then C0
feedback/connection/routing. Operator fields address 20, 40, 60, 80 and E0
register groups. FX definitions have exactly those 22 bytes. The four
remaining music bytes are skipped. Definitions store pointers into the
stream, not copied patch structures (`0x12c` music slots, `0x14c` FX slots).
At the music call-stack limit, the driver falls back to command dispatch
without consuming the two target bytes (`0x92f..0x94b`); it has no checked
malformed-stream recovery. The resource survey did not encounter that case.

Attenuation commands accept selector 3 for bank 0 and selector 4 for bank 1;
other selectors are consumed without writes. During a fade all such commands
are consumed without changing levels. Carrier KSL bits are preserved.
Patch loading during a fade substitutes the remaining-step value into the
first bank's carrier TL; the second bank has no corresponding substitution
(`0x59c..0x5bc`, `0x640..0x654`). Patch loading does not refresh the separate
level-command caches used by the fade. Preserve that asymmetry.

Notes use a 16-bit frequency table at `0x1b6`, indexed by the low five note
bits, adding the high three bits shifted left five (`0x67e..0x692`).
FX opcode 6 writes the cached word at `0x10d + voice`, whereas the other
note paths use `0x10d + 2*voice` (`0x722` versus `0x784`). The hardware
write still addresses the requested voice. This original indexing defect
is exercised by FX 9 and its aliases 60..65; it is not merely malformed-data
behavior. The external resource survey decoded every admitted FX stream to
its first terminator without overruns; all voice commands used 7 or 8.
Music opcode 8 masks its emitted word with `0x1fff` (`0x762..0x771`), while
FX opcode 7 clears only key-on with `0xdfff` (`0x73b..0x750`). The distinction
matters after the faulty FX cache write introduces high bits into a music
voice's cached high byte. Neither operation should be replaced with the
other's mask.

Slides are postprocessed only for voices 8 and 7 (`0xa19..0xabd`). A byte
phase, initially 255 when enabled, adds the rate; carry applies the signed
delta to the cached ten-bit frequency. Positive crossing at 734 halves the
frequency and increases octave; negative crossing at 388 doubles it and
decreases octave. Octave wraps in its three bits, key-on is retained, and
the result is written to both banks. Crossing corrections occur only on
the corresponding threshold, with the original zero-frequency guards.

## Tick scheduling and fade length

**DOS static, high:** init `0x266..0x290` installs INT 08h and programs PIT
channel 0, mode 3, divisor `0x4006` = 16390. With nominal PIT 1193182 Hz,
the driver runs at approximately **72.79939 Hz**, conventionally 72.80 Hz.
The ISR adds 16390 to a 16-bit BIOS-chain accumulator and invokes the old
handler on carry (`0xac5..0xadb`), preserving the average BIOS rate.
Shutdown restores its vector and PIT divisor 0 (`0x344..0x381`).

Each tick first services music if its countdown is zero or decrements to
zero. Only otherwise does it decrement/test the FX countdown
(`0x6a4..0x6e9`). Consequently a music command tick does **not** decrement
FX time. The chosen stream runs until a countdown/yield or terminal
command. Yield reaches fade/slide postprocessing on that same tick; an
ordinary tick with neither stream due also reaches it. A terminal F0..FE
command exits that ISR without postprocessing (`0x98e`, `0x9c4`).

Fade state is speed at `0x192`, byte phase at `0x193`, and signed remaining
steps at `0x194`. On every eligible postprocessing tick the phase adds
speed modulo 256. Carry decrements remaining steps; a negative result stops
music, cancels fade and keys off music channels. Earlier carries increase
each bank's cached carrier attenuation toward 63 (`0x9cc..0xa19`).
The write uses the previous cached byte: `0x9f4/0xa04` load AH before the
cache increment at `0x9fd/0xa0d`, and `0x503/0x514` emit that retained AH.
The cache reaching 63 suppresses further level writes; completion still keys
off the music. This preserves the original one-carry lag, rather than applying
the incremented cache value immediately (DOS static, high, same calibrated
INTRO promus resource).

For initial phase p and speed s, completion requires
`ceil((64 * 256 - p) / s)` **eligible** ticks. Command 207 therefore takes:

| Starting phase | Eligible ticks | Approximate duration at the DOS rate |
| --- | --- | --- |
| 0..30 | 80 | 1.09891 s |
| 31..237 | 79 | 1.08517 s |
| 238..255 | 78 | 1.07144 s |

The fresh driver's phase is zero. Fade requests, stop and restart do not
reset phase. Add any terminal-command ticks excluded above; CD-speech and
input polling can delay the subsequent load further. A fixed 80-tick
wall-clock sleep is therefore not the DOS rule. The scheduler, phase and
stream state are sufficient to compute it without consulting real output.

**I/O audit, high for the decoded driver:** no OPL status or OPL timer reads
occur. The four input instructions are reads of PIT port `0x40`, at
`0x4d8`, `0x4e3`, `0x4f3`, `0x4fe`. Their values are discarded: fixed loops
perform three reads between address/data and twenty after data as delays.
Initialization and command `0x100` also write card mixer ports base+4/+5;
the interrupt tail writes PIC EOI. None branches on OPL status.

Pinned Nuked supports NEW, both banks, two-operator channels and stereo;
its `OPL::writeReg` uses `OPL3_WriteRegBuffered`. It remains the selected
emulator for those concrete capabilities. **Not established by static
analysis:** equivalence of audible write spacing and the card mixer path.
DRO/PCM evidence is still needed; no unsupported OPL dependency or audible
equivalence has been demonstrated.

## Comparison with the pinned driver

**Reference comparison** against `engines/mm/shared/xeen/sound_driver.cpp`,
`sound_driver_adlib.cpp/.h` and `audio/chip.cpp` at the pinned revision:

| Question | Verdict against DOS promus |
| --- | --- |
| Shared format | Nibble dispatch, countdown/yield, song-relative call/return, note encoding and 26-byte music definitions are related. That does not make the FX formats interchangeable. |
| Patches and levels | Pin loads one 11-byte patch per voice; FX definitions are 11 bytes. DOS loads two patches, FX definitions are 22 bytes. Pin's level selector is 5; DOS uses 3/4 for separate banks. |
| Fade accumulator suspicion | Confirmed difference: DOS uses byte carry; pin adds into an `int` and tests its sign. Pin also requires music-playing before starting a fade; DOS does not. |
| FX subroutine-base suspicion | Not an applicable promus repair: DOS FX opcode 0 is a no-op, not a call. FX FF loops to the effect start. The pin's FX call relative to the music base must not be imported. |
| FX countdown suspicion | **Matches DOS**: music-due ticks skip the FX countdown. Do not "fix" it. |
| Frequency cache/slide | DOS FX opcode 6 has the byte-indexed word-write defect above. Pin indexes by channel. Pin's slide accumulator is also an integer/sign test and its octave adjustment is outside the threshold branch. |
| Key off | Music opcode 8 clears key-on in the write without updating its cached frequency; pin writes the cached frequency unchanged. DOS FX opcode 7 writes the high frequency byte to B0; pin passes the whole word. |
| Rate | Pin passes 72.8 to integer `Chip::start`, resulting in 72 Hz. DOS divisor gives approximately 72.80 Hz. |
| Resource choice | DOS loads INTRO promus; pin's AdLib path loads `blastmus` from its active archive. |

These are implementation requirements for the approved driver, not claims
that MMModern already implements the differences. Master was not rechecked.

## Party options and New or Load

**DOS static/resource, high:** party base is DS:`ed8c`; save size is 1530.
Save writes it at DAT `0xd89e..0xd8a6`; load copies `maze.pty` into it at
`0xf05d..0xf069`. Options are not XEEN.CFG fields.

| Option | Party offset | New Clouds value | Meaning |
| --- | --- | --- | --- |
| Efx | `0x0e` | 1 | On |
| Music | `0x0f` | 1 | On |
| Text-off | `0x5f8` | 1 | Text off |
| Speech | `0x5f9` | 1 | Speech on |

The 812-byte Clouds block really has Efx/Music at +14/+15. It has no trailing
Text/Speech fields. New first opens the Darkside current template and calls
the full loader (`0x20b7c..0x20b95`, `0xf013`, archive switch `0xe89e`).
It then overlays the Clouds `maze.pty` (`0x20baa..0x20be1`) without clearing
the tail. The two values 1/1 from the 1530-byte Darkside template survive.
Both installed template blocks have Efx/Music 1/1. The New chooser is
called with argument 0 from the title at `0xc12b`; this is not an assumption
based on startup's provisional Efx/Music assignments.

Load copies the selected save archive (`0x1f193`, `0x1f1b3 -> 0xee4c`) and
returns to the main game setup (`0xb819..0xb839`, `0x39460 -> 0xf013`),
replacing the four party bytes. Song names, buffer, playback cursor and fade
state are program/driver state, not bytes in the party block. New/Load do
not restore a saved song position or replay a panel toggle. Map selection
then compares its requested song with the process's current name.

**Load with Music off sends no stop just because the loaded byte is off.**
The ordinary song-command wrapper at `0xde65..0xde89` gates *all* its
commands, including 0, on Music; the panel's explicit Off path bypasses
that gate. The main-loop Load return also bypasses the stop at `0xb8b5`.
A same-name map request therefore preserves the running driver's state,
even when the loaded option is off; a different name still requests a fade,
then loads without starting the replacement. New through the title has
the title exit's Music-gated fade/stop requests and buffer replacement
described below.

Efx toggle (`0x1f061..0x1f073`) requests effect 20 when becoming on; its
off branch's request 0 is suppressed by the Efx wrapper. It sends no stop.
Speech off forces Text on at panel redraw (`0x1ee00..0x1ee32`); Text off
with Speech on is allowed. M82's approved temporary visible-text restriction
is an implementation decision, not the DOS default.

## Song names and asynchronous changes

**DOS static, high:** Clouds outdoors always requests `outdoors.m`, with no
day/night change (`0x10276..0x102ac`). Indoor wall kind indexes the six-byte
table at DS:`187a`, then the name pointer table at file `0x4f04e`:

| Wall kind | Name-table index | Song |
| --- | --- | --- |
| 0 | 1 | `town.m` |
| 1 | 2 | `cavern.m` |
| 2 | 3 | `dungeon.m` |
| 3 | 4 | `castle.m` |
| 4 | 3 | `dungeon.m` |
| 5 | 5 | `outday1.m` (first entry of the adjacent Darkside table) |

There is no bounds fallback for wall kind 5 (`0x10435..0x10483`); its
pointer is at file `0x4f062`. The calibrated installed XEEN.CUR map census
found 128 matching map records of 892 bytes, with wall kind at +826:
kinds 0/1/2/3/4 have 70/20/15/16/7 maps respectively, and no kind 5.
CC hash collisions were excluded by checking each record's map ID at +768.
`outday1.m` is absent from CD XEEN.CC and present in DARK.CC. It is not a
missing sixth Clouds composition to invent.

The seven Clouds location table entries at `0x4f110..0x4f128` are
`bank.m`, `smith.m`, `guild.m`, `tavern.m`, `temple.m`, `grounds.m`,
`endgame.m`. Location entry requests with fade 223 at `0x1d451`; exit
copies the previous name and requests it with 223 at `0x1d750..0x1d778`.
The Clouds Inn/sign-in path selects `inn.m` with fade 128 at
`0x394d4..0x394f4` when its song-entry flag is clear. These names and the
five Clouds map names are all present in CD XEEN.CC.

**CD DAT title/menu:** startup selects DARK.CC and loads `newbrigh.m`
(`0xb676..0xb68e`); menu entry starts that buffer (`0xbcfa..0xbd03`).
Title exit requests fade 207 (`0xc673`), later stop 0 (`0xc6fa`), frees
the buffer and preloads `twnnitea.m` (`0xc702..0xc75d`). Neither title
operation sets the map's current-name string. This establishes the CD DAT
menu path, not every EXE intro. M82 includes this title song and the
preload (maintainer, 2026-10-21); Darkside map music and the intros stay out.
Both title commands use the Music-gated wrapper; they are suppressed if
that party option is off.

A missing resource is fatal in DOS: the resolver tries loose files, then
the selected archive; an absent index entry reaches `0x45c8a..0x45cfd`,
restores the display/cleans up and exits DOS with code 2. The missing-song
case has no silent fallback (`0x45f93..0x45fa8`).

For map music the same case-insensitive name continues regardless of Music
on/off or whether the driver is currently playing. A changed name is copied
to DS:`9766`, with the old name moved to DS:`9757`, **before** the buffer
changes (`0x3790f..0x37955`). The request sends the fade command directly,
even with Music off, and marks a pending load. Input polling checks fade
status and attempts the load when zero (`0xe0a3..0xe0c2`). It still polls
mouse and keyboard while fading: input is not blocked.

The pending loader (`0xe590..0xe5ee`) waits for CD speech to become idle,
loads the requested name into DS:`f42a`, and starts it only with Music on.
Music-off at `0x1f09e` immediately sends command 0 but leaves the pending
request; the next eligible poll can load it silently. Music-on at
`0x1f087..0x1f096` clears pending and restarts the *loaded buffer*:

- If the deferred load completed while off, On restarts the new song.
- If the fade/CD wait still holds the request, On cancels it and restarts
  the old buffer. The requested-name string is already new, so a later
  same-name map comparison need not repair that mismatch.

## Efx, VOC waits and entrance captions

**DOS static, high:** the VOC wrapper `0xdea9..0xdecd` gates both playback
and busy-status queries on Efx. With Efx off it returns zero, removing the
VOC-busy part of a wait without sending a stop to an already running sample.
Other fixed delays and caption-cursor conditions remain. The FM wrapper
`0xdece..0xdee6` likewise suppresses new effects without stopping the old one.

The existing [entrance evidence](evidence.md#part-0b-text-settings-and-cadence-2026-10-20)
establishes the separate caption gate: Efx off or digital-driver ID below 2
allows captions; Text/Speech do not select them. The renderer advances one
character only after more than one BIOS tick, resets its time mark, and
wraps modulo record length (`0xdca3..0xdd52`, timer `0x4411d..0x44144`).
There is no catch-up or universal duration measured from speech onset;
the mark was made before zoom. Clouds record lengths excluding NUL are
Dwarf 107, Reaper 76/78, Golem 72/73, Sphinx 73/50. A complete cursor cycle
needs that many qualifying advances; subsequent advances are separated by
at least two BIOS ticks and sampled by the scene loop.

Reaper and Golem draw captions in the first speech loop, not their second
sample loop (`0x2ac87`, `0x2a14c`, continuations `0x2aca2..0x2acb8` and
`0x2a167..0x2a17d`). Dwarf and Sphinx exit later sample phases in text mode
after one cursor cycle (`0x2b67b..0x2b6aa`, `0x2992d..0x29940`). These are
logical wait lengths. **Unknown:** the exact frame on which previously
drawn caption-box pixels disappear; cessation of renderer calls does not
prove erasure. The earlier evidence expressly leaves that observation open.

## Missing configuration and remaining evidence

**DOS static, high:** missing `XEEN.CFG` prints configuration instructions
and exits (EXE `0x11daf..0x11e1a`; DAT `0x3d320..0x3d33b`). MMModern
likewise refuses to start with a clear message (maintainer, 2026-10-21) and
never writes the file.

**DOSBox DRO captures (2026-10-21):** the maintainer recorded six promus
captures in classic DOSBox 0.74-2 (`oplmode=opl3`) in
`D:/MModern-re/audio-m82/captures/`: startup to the title menu, Clouds Inn,
town to outdoors with the fade, effect 20 alone, wall bump alone, and effect
20 over music; durations are approximate. A first check agrees with the
static reading: the startup stream sets second-bank register 5 to 1 before
any note, and channel registers C0..C8 carry routing `0x2_` in the first bank
and `0x1_` in the second. DOSBox labels the header hardware type 1 (dual
OPL2); the register stream, not that label, is the evidence. DRO delays need
alignment to the PIT grid, not one write per tick. The per-tick comparison
with MMModern's driver is M82 Part A's acceptance.

Two boundaries move to the parts that consume them: the per-function
effect-site/owner reconciliation below opens Part C, and the caption pixel
lifetime is settled in Part B from an Effects-off Dwarf or Sphinx video.

## DOS FM effect call sites

**DOS static, bounded inventory:** 133 decoded direct far calls target the
Efx wrapper at DAT `0xdece`. Its call at `0xdedb` is the only direct call
found to FM effect trampoline `0x4617b`. No push-CS/near-call bypass was
found. An equivalent bounded scan of XEEN.EXE found no references to these
DAT wrapper/trampoline addresses; that is not a full analysis of its own
intro drivers or indirect dispatch. EXE SHA-256:
`c5ade865b4d1f63201ce3a421df5182cf1d2923f3343deb82a5a06be22eea235`.

All effect IDs below are decimal. Expressions denote runtime IDs. The Efx
Off panel branch's ID 0 is gated out. Owner labels are existing coordinator
associations, not new APIs or implemented calls. Medium labels retain action
or upstream-route uncertainty; they must be refined before wiring effects.
The spell cluster's common coordinator does not imply every call originates
in combat. No site is excluded as an intro merely because of its address.

| DAT file offset | Effect id / expression | Existing MMModern owner | Operation | Confidence / limitation |
| --- | --- | --- | --- | --- |
| `0x0b2f1` | 33 | `XeenEventFlow (death continuation)` | Clouds death laugh frames 1 and 11 | High: DAT branch; pin WorldOfXeenEngine death sequence |
| `0x0b308` | 34 | `XeenEventFlow (death continuation)` | Clouds death laugh frame 15 | High: DAT branch; pin WorldOfXeenEngine death sequence |
| `0x0c903` | 10 | `XeenJourneyFlow (party interaction)` | gameplay character/item interaction | Medium |
| `0x0cf48` | 21 | `XeenEventFlow / XeenBarrierFlow` | barrier check/refusal | High |
| `0x0cfa9` | 47 | `XeenEventFlow / XeenBarrierFlow` | barrier state interaction | High |
| `0x0d442` | 7 or 8 = 7 + step-toggle bit | `XeenEncounterFlow (journey stepping presentation)` | gameplay movement | High |
| `0x0d5c1` | 21 | `XeenEncounterFlow (journey action/pulse)` | gameplay interaction; exact action unresolved | Medium |
| `0x11692` | 11 | `XeenEncounterFlow (falling/travel presentation)` | gameplay falling | High |
| `0x11716` | 31 | `XeenEncounterFlow (falling/travel presentation)` | gameplay falling sequence | High |
| `0x1b1a1` | 28 | `XeenEventFlow (Tavern service)` | Drink increments Drunk after random result below 26 | High: condition +0x14a and pin Tavern Drink |
| `0x1c4b5` | 45 | `XeenEventFlow (location/service presentation)` | gameplay service | High |
| `0x1c4d3` | 45 | `XeenEventFlow (location/service presentation)` | gameplay service | High |
| `0x1d918` | 49 | `XeenEncounterFlow (party/actor action resolution)` | gameplay; exact action unresolved | Medium |
| `0x1d922` | 21 | `XeenEncounterFlow (party/actor action resolution)` | gameplay; exact action unresolved | Medium |
| `0x1db60` | 46 | `XeenEventFlow / XeenBarrierFlow` | gameplay barrier/door interaction | Medium |
| `0x1e8d2` | 21 | `XeenEventFlow (shared service/resource notice)` | Insufficient or absent party/bank resource | High: DAT 0x1e8bb..0x1e940, message pointers DS:597a/599b |
| `0x1f073` | 20 when Efx becomes on; 0 when off | `XeenTitleFlow` | control-panel Efx toggle | High (calibrated) |
| `0x1f249` | 51 | `XeenTitleFlow dispatch -> XeenEventFlow / XeenEncounterFlow::mrWizard` | Mr Wizard confirmation; outside-combat map transition | High — settled original-quirks DOS finding |
| `0x1f881` | 38 | `XeenEncounterFlow (character damage/condition presentation)` | gameplay character condition | High |
| `0x21107` | 20 | `XeenEncounterFlow (party reward publication)` | gameplay treasure/item award | High |
| `0x236c6` | 10 (stack local initialized to 10) | `XeenEventFlow / XeenBarrierFlow` | gameplay lock success | High |
| `0x236d8` | 21 | `XeenEventFlow / XeenBarrierFlow` | gameplay lock failure | High |
| `0x23eb8` | type-selected: 22,23,24,26,25,27,29,38 | `XeenEncounterFlow` | gameplay typed combat damage | High |
| `0x240fe` | 51 | `XeenEventFlow (dialog/text response)` | gameplay response UI; exact action unresolved | Medium |
| `0x2428e` | 54 | `XeenEncounterFlow (party treasure award)` | gameplay reward | High |
| `0x2456b` | 20 | `XeenEncounterFlow (party distribution/reward)` | gameplay reward | Medium |
| `0x26ff3` | event parameter byte (record + 6) | `XeenEventFlow (event interpreter)` | gameplay Event PlayFX opcode | High semantic; value data-driven |
| `0x270a8` | 51 | `XeenEventFlow (event interpreter)` | gameplay event/dialog presentation | Medium |
| `0x270ea` | 44 | `XeenEventFlow (event/location presentation)` | gameplay service/event feedback | Medium |
| `0x28f1c` | 21 | `XeenEventFlow (event interpreter)` | gameplay refusal/feedback | Medium |
| `0x2c105` | 60 + weapon-power table value at current weapon index | `XeenEncounterFlow` | gameplay combat missile/power effect | High semantic; table-backed |
| `0x2c17c` | 6 | `XeenEncounterFlow (combat ranged action)` | gameplay combat projectile/action | Medium |
| `0x2c23c` | 4 | `XeenEncounterFlow (combat ranged action)` | gameplay combat projectile/action | Medium |
| `0x2d379` | type-selected: 22,23,24,25,26,27 | `XeenEncounterFlow` | gameplay typed combat damage | High |
| `0x2d5f0` | 37 | `XeenEncounterFlow` | gameplay monster attack/condition | Medium |
| `0x2d62e` | 37 | `XeenEncounterFlow` | gameplay monster attack/condition | Medium |
| `0x2d65e` | 38 | `XeenEncounterFlow` | gameplay monster attack/condition | Medium |
| `0x2d6af` | 37 | `XeenEncounterFlow` | gameplay monster attack/condition | Medium |
| `0x2dc0b` | 6 | `XeenEncounterFlow (combat action)` | gameplay combat | Medium |
| `0x2e1fe` | 51 | `XeenEncounterFlow (combat turn/action transition)` | gameplay combat presentation | Medium |
| `0x2e7e6` | monster-record byte at +0x32 | `XeenEncounterFlow (monster action in combat)` | gameplay monster FX | High semantic; data-driven |
| `0x2ec42` | 51 | `XeenEncounterFlow::mrWizard` | Mr Wizard combat exit tail and map transition | High — settled original-quirks DOS finding |
| `0x2f1d3` | 21 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f25c` | 21 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f3ac` | 39 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f3ee` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f44a` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f476` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f496` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f53a` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f575` | 14 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f5b7` | 16 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f5da` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f5fd` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f620` | 17 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f68c` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f6fb` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f72e` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f825` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f895` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f8b7` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f8d8` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2f8eb` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fa69` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2faf6` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fb65` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fbf3` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fc3e` | 14 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fc4f` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fc70` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fc8b` | 15 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fee7` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2ff65` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2ffc5` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x2fff0` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30072` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x301ee` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30250` | 17 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30292` | 15 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x302b2` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30316` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30339` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30557` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3057c` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3059c` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3060c` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30633` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3064d` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3076e` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30836` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x308ee` | 15 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x3095d` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30a01` | 51 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30a43` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30a6f` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30a93` | 16 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30aa7` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30af4` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30b6c` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30baa` | Storm FX lookup indexed by `rand(2..5)-2` = {13,14,15,17} | `XeenEncounterFlow` (combat spell effect)` | gameplay Storm spell typed damage | High semantic; matches ScummVM `spells.cpp` `STORM_FX_LIST` |
| `0x30bdf` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30c38` | 30 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30cb4` | 14 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30cd7` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30cfa` | 16 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30d1d` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30d40` | 13 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30de2` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30e07` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30e1d` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30e53` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30e82` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30e9d` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30edb` | 16 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30efe` | 18 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30f21` | 22 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30f44` | 17 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x30fd7` | 49 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31009` | 16 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31bfd` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31c44` | 21 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31d3d` | 19 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31dc3` | 21 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31e1e` | 21 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31e4c` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31ea9` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31f07` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x31f61` | 20 | `XeenEncounterFlow (shared spell/effect continuation)` | gameplay spell/combat effect handler | Medium; exact entry route may be exploration, combat, or item spell |
| `0x32d5b` | 31 | `XeenEventFlow / XeenBarrierFlow` | gameplay bash/barrier action | High |
| `0x34d71` | 21 | `XeenEventFlow / XeenEncounterFlow::buySpell` | Guild spell purchase resource-subtraction failure | High: DAT 0x34cd3..0x34cea, pin SpellsDialog purchase branch |
| `0x368c3` | 21 | `XeenEventFlow (event/dialog input)` | gameplay event refusal | Medium |
| `0x36a07` | Attack type 0..6 maps to 49,18,13,14,15,17,16 | `XeenEncounterFlow (monster ranged presentation)` | 36-entry shooting queue; MON +0x1d indexes table at file 0x50017 | High: DAT 0x3694f..0x36a0c; pin Combat::monstersAttack |
| `0x39bda` | 21 | `XeenEncounterFlow (party/member transition)` | gameplay party action refusal | Medium |

The pin inventory contains about 145 gameplay calls plus cutscenes, rather
than 133 corresponding source lines. Its 73 spell-file calls correspond
broadly to DOS's 86-call spell/effect band at `0x2f000..0x31fff`; the pin's
33 combat calls also cover shared typed-damage helpers outside that band.
Identified correspondences include step toggle, falls, barrier refusal,
Event PlayFX, typed damage, weapon-power effects, monster-record effects,
monster shooting, Efx-on, Mr Wizard, Tavern Drink, Guild purchase and death.

**Unknown:** a complete per-function reconciliation of every pin gameplay
site and every medium-confidence DOS owner association. The count difference
is not proof of missing behavior or permission to omit sites. Full decoded
contexts, the checked ID table and grouped source crosswalk remain outside
the repository under `D:/MModern-re/audio-m82/fx-inventory-pass2/`.
