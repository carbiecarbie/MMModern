# Audio

This chapter describes original audio behavior and its evidence. M82/M83
implement it in `PromusDriver`, `XeenAudio`, `XeenFmSource`, `XeenVoc` and
`XeenCdAudio`, with SDL output/conversion behind `platform/sdl`, except where
an entry says otherwise. See the [milestone 83 plan](../milestone-83-plan.md)
for scope and the [evidence labels](README.md#how-to-interpret-evidence).

**Maintainer decision, 2026-10-21:** M82 reproduces CD `promus` (music device
3) in OPL3 stereo on the pinned Nuked emulator. `admus`, `blastmus` and
other devices belong to roadmap step 19. Their byte identities below are
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
the repository in `D:/Projetos/MModern/MModern-re/audio-m82/part0-boundary-check.py`,
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
| `0x100` | Set the card mixer's FM level from the next argument: Sound Blaster mixer register `0x26` through base+4/+5. This is separate from per-voice OPL attenuation. |
| Other values above 255 | Treat the value as the new song segment and restart at offset 0. |

For command `0x100`, let h be `((argument & 0xf0) + 0x60) mod 256`;
the mixer byte is `h + (h >> 4)` (`0x40f..0x438`). Init's nonzero mixer
flag (installed XEEN.CFG word +`0x14` = 1) writes mixer registers
`0x2e/0x22/0x26` to 255 and register `0x04` to 153 (`0x23c..0x265`). These
are Sound Blaster Pro mixer registers; [the card mixer](#card-mixer) gives the
ports, every site and the levels MMModern applies.

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
Initialization and command `0x100` also write the Sound Blaster mixer ports
base+4/+5; the interrupt tail writes PIC EOI. None branches on OPL status.

Pinned Nuked supports NEW, both banks, two-operator channels and stereo;
its `OPL::writeReg` uses `OPL3_WriteRegBuffered`. It remains the selected
emulator for those concrete capabilities. The card mixer is a separate
stage after the chip ([card mixer](#card-mixer)).

**Write spacing, inferred from the I/O count, high; interval reference
only.** The delay loops above (DOS static, high) make every register write
take about 23 bus reads, roughly one 20.1 µs native sample or more, so a
tick's writes reach the chip one after another. The DOSBox capture
`promus-fx20-alone` records 36 writes within one millisecond of its second
burst, an upper bound of about 1.4 native samples on the average interval;
its first millisecond holds 176 writes, so DOSBox I/O timing is not hardware
timing. The exact interval depends on the machine's bus. Nuked enters release only when
a generated sample sees key-off, so applying a tick's key-off and key-on
together keeps a held voice from re-attacking: effect 20 re-strikes voices
7/8 that way on its third tick and was almost silent (about 1/40000 of the
capture's rendered energy). MMModern uses the pin's two-sample buffered
spacing; the rendered energy of effect 20 then matches the capture within a
factor of two (`xeen_promus`). No other audible equivalence is claimed.

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

MMModern's `PromusDriver` follows DOS in each row. Master was not rechecked.

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
with Speech on is allowed. M83 removes M82's temporary visible-text
restriction and applies the NPC reader and delivery-fallback rules below.

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
`0x394d4..0x394f4` when its song-entry flag is clear. `XeenAudio::requestInn`
implements that request; since M87 the Inn's Sign In screen issues it
(roadmap step 14). These names and the
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
**DOS static, high:** the same loose-first lookup applies to VOC resources
(`0x45ed2..0x45ed7`); gameplay scene waits now use the same selected VOC as
playback, so an overriding loose file supplies both durations.

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
allows captions; Text/Speech do not select them. MMModern reads the selected
CFG's raw digital id, including when that id uses a null-driver stopgap.
The renderer advances one
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
logical wait lengths.

**DOSBox video, Dwarf only (2026-10-21):** maintainer captures
`D:/Projetos/MModern/MModern-re/audio-m82/captures/dwarf-efx-off.avi` and
`dwarf-efx-on.avi`, classic DOSBox 0.74-2 ZMBV, decoded with the maintainer's
pure-Python `D:/Projetos/MModern/MModern-re/tools/zmbv.py`. At 14,268 microseconds per frame,
zero-based frame 136 first shows the box after the rise, in the first speech
loop. Frame 137 briefly shows the actor redraw without it; frames 138..960
show the strip, empty after the cursor wraps at the end. Frame 961 redraws
the final Dwarf body/mouth without the box, before the later "Please Wait"
overlay and map reconstruction. The Efx-on control shows no caption.
Thus the Dwarf box lasts through its caption speech loop and is erased by
its final redraw, not by a universal timeout or the map load. Other guardians'
box erasure is **not video-confirmed**; retain their original sprite/restore
compositions. Decoded frames and analysis remain outside the repository.

## Installed digital driver: command and length rules

**DOS static, high confidence (2026-10-22):** freshly extracted INTRO.CC
`prosnd` is 876 bytes at archive offset `0x6f28bf`, SHA-256
`1b517eee5ad4e1f64de9cdd6f27e60946d879e6c274517f7338f12c0ba1ea4c6`.
Calibration reproduced the Efx/driver-presence gate at DAT
`0xdea9..0xdecd`. Its trampoline `0x46185..0x4618e` enters driver +3.
Offsets in the following table are **decoded prosnd resource offsets**.
These are static program rules, not a new observation of audio output.

| Operation | Rule | Offsets |
| --- | --- | --- |
| Command 0 | Stops an active sample and clears busy. | `0x11f..0x130`, `0x183..0x1a1` |
| Command 1 | Returns the driver's busy word, 0 or 1; it is not a remaining-sample count. | `0x132..0x13a` |
| Other command | Treats the value as the sample segment; stops an active sample before replacing it, then sets busy. No queue or ignore-while-busy branch. | `0x13c..0x177` |
| Input layout | Reads only the word at sample +`0x1b`, subtracts 10 modulo 65536, and sets the initial DMA address to segment base +`0x20`. | `0x153..0x168`, `0x1a2..0x1b6` |
| Output rate | Writes DSP command `0x40` and literal time constant `0xbc` (188), independently of the sample header. Nominal period is 68 microseconds, or `1000000/68` Hz. | `0x227..0x254` |
| Completion | The final accepted sample IRQ clears busy; a non-final IRQ programs the remainder across the DMA page boundary. | `0x260..0x2d9` |

The driver does **not** parse VOC block types, descriptor, data offset,
codec, continuation blocks, silence blocks or terminator. Its fixed layout
matches ordinary type-1 PCM at offset 26, but that does not make it a
general type-1/2/3 decoder. A new sample with a different header rate still
receives time constant 188. Invalid layouts are not a supported format to
emulate by unchecked reads.

The decoded driver `0x3b..0x36b` makes no mixer-level write. Its IRQ writes
the mixer register selector `0x82` and then **reads** base+5
(`0x26b..0x27b`); this is status handling, not a volume setting. Register
`0x82` is the SB16 interrupt status, read through the same Sound Blaster
mixer ports that the music drivers write ([card mixer](#card-mixer)).

The Sphinx call passes the loaded segment directly (`0x29775..0x29785`,
`0x2989d..0x298b0`). DAT's resource loader reads the CC member into the
segment and XOR-decodes its bytes (`0x45fab..0x45ffb`, `0x460b8..0x460dc`);
there is no VOC-header conversion on that path. The damaged descriptor
therefore does not affect this driver.

**Timing conflict, high confidence for the programmed counts:** the first
block's low length word minus 10 is the count written to DMA and DSP
(`0x21a..0x220`, `0x336..0x34c`). It is not the header PCM payload length.
`XeenVoc.h` uses the low word minus 10 modulo 65536, plus one sample, for
both owned unsigned 8-bit mono PCM and `xeenVocDurationNanoseconds`.
**Maintainer decision, 2026-10-22:** a resource shorter than offset `0x20`
plus its programmed count-plus-one transfer is a hard error. No audited
Clouds sample hits this case; a corrupt installation fails clearly rather
than padding, truncating or inventing PCM.

| Clouds resource | Header PCM bytes | Programmed unsplit count | Former header duration |
| --- | --- | --- | --- |
| `sphinx13.voc` | 39243 | 39235 | 2.668524 s |
| `sphinx14.voc` | 18751 | 18743 | 1.275068 s |
| `xeenlaff.voc` | 33173 | 33165 | 2.255764 s |

**Inference, medium:** with count-plus-one DSP/DMA semantics, an unsplit
transfer is seven samples (476 microseconds) shorter than the header
interval. The page-split path uses separate count arithmetic
(`0x1b9..0x1d3`, `0x2a5..0x2cb`); do not apply a universal seven-sample
correction as a claim about every DOS allocation. **Maintainer decision,
2026-10-22:** use the unsplit count-plus-one model for playback and timing;
the allocation-dependent sub-millisecond difference is an
[approved approximation](../original-quirks.md#dev-prosnd-dma-timing), with
no further split investigation. Existing captures do not establish
sub-millisecond agreement. MMModern's logical duration is exactly
`(uint16(lengthWord - 10) + 1) * 68000` ns. Sphinx 13/14 therefore use
2.668048/1.274592 seconds, respectively. All timing callers use this rule;
M44 digests remain approval gates.

`XeenAudio` owns the digital logical deadline on the presentation clock and
mixes one owned PCM voice with FM at the device rate. Its digital command
slot retains only the latest start or stop, separately from the FM queue and
fade barrier. The VOC resampler uses exact rational zero-order hold at
`1000000/68` Hz; the FM resampler uses 49716 Hz. Decoding and reads precede
publication. Commands are taken before every mix block of at most 128
output frames; stop/replacement discards the old voice without a converted
tail. Closing
the SDL device joins its callback before the owner's PCM, queues and
resamplers are destroyed. Playback cursors never answer logical busy.
Digital commands use their exact action timestamp even when it lags the
latest presentation advance: logical time is `elapsed - (last - now)`,
checked against underflow. Only future commands advance the clock
(maintainer, 2026-10-22). The FM conversion stream is prepared through the
SDL platform interface before the device callback starts; creation failure
keeps output silent. Live rate changes recreate the small-block converter and
retain the rational VOC cursor, as described below.
See the [driver quirk](../original-quirks.md#sv-prosnd-fixed-layout).

**DOS resource audit, high (2026-10-22):** the samples in the initial
36-name Clouds VOC census have a single type-1 block at offset
26, codec 0, time constant 188, a length fitting the low word, and no
additional blocks. This includes the dream's INTRO `dreams2.voc` and DARK
`laff1.voc`, and the damaged Sphinx descriptor. No exception to the fixed
layout was found. Counts of distinct names by referring owner are Entrance
17, EventFlow 5, ServiceAnimation 6, SmithFlow 6 and TrainingFlow 3; these
sum to 37 because `ahh.voc` is shared. These are resource-name counts, not
physical DOS calls or runtime executions. Combat and other DOS samples
absent from current loaders are outside this completed layout audit.

Reproduction scripts, extracted resources, disassembly and full hashes are
outside the repository in `D:/Projetos/MModern/MModern-re/audio-m83/`; `findings.md` records
the final evidence, decisions and remaining measurement limits. No original bytes or code are
included here.

## Card mixer

**DOS static, high, 2026-10-26.** The music-init port argument is the CFG
music port (`0x220` installed): DAT `0xdf25` passes CFG +6 and the flag at CFG
+`0x14`, EXE `0x11c0c` does the same from its CFG copy at DS `0x427e`. Ports
base+4/+5 are the Sound Blaster Pro mixer index and data ports, and
`0x2e/0x22/0x26/0x04` are its line, master, FM and voice (DAC) registers; they
are not PAS registers. This paragraph describes **promus**: the
[other drivers](#other-original-sound-devices) have different flag/port
consumers, and `canmus` has no such mixer block. Other full music drivers
also recognize command `0x100`, with different meanings. Each program runs
driver init once: EXE `0x79f1` → `0x11b8b`, DAT `0xb646` → `0xdef9`. Promus
shutdown writes no mixer register. Every EXE start (intro, ending) and every
fresh DAT (startup, the title after an intro or the ending) therefore
rewrites the init levels when the flag is set, and writes nothing when it is
zero.

Command `0x100` wrappers (EXE `0x11b24`, DAT `0xde8b`) test only that the
music driver is loaded, not the party Music or Efx switch. Site offsets below
are the argument pushes; each far call to the wrapper follows two bytes later:

| Program | Sites and arguments | Moment |
| --- | --- | --- |
| EXE, Clouds intro | `0x8481` 75; `0x88c2` 95 | After the opening scroll; natural completion only (Escape enters cleanup at `0x88ca`) |
| EXE, Clouds ending | `0x8e42` 75; `0x8f91` 95 | Before Alamar's first voice; as he vanishes |
| EXE, Darkside intro | `0xac2b` 95; `0xaf13` 48; `0xce83` 95 | As JVC begins, before the fade-out to it; after the pyramid zoom publishes scale two; after the release-creature fade, before the bird (M93) |
| EXE, other Darkside/World | `0xe318` 95, `0xe51f` 48, `0xec94` 95, `0xfa63` 95 | Presentations after the Darkside intro (`0xa9bd..0xd608`); not yet implemented |
| DAT, Sphinx `0x2968b` | `0x29838` 48; `0x29984` 95 | After the zoom; after the speech, before the retreat |
| DAT, Golem `0x29ab6` | `0x29e92` 48 (Clouds side byte only); `0x2a060` 48; `0x2a453` 95 | Before the slide and ogre growl; before the speech; after it, before the retreat |
| DAT, Reaper `0x2a66a` | `0x2ab98` 48; `0x2afd2` 95 | After the slide; at the end, after any retreat |
| DAT, Dwarf `0x2b052` | `0x2b4fd` 48; `0x2b702` 95 | After the rise; at the end |

Nothing writes CD register `0x28`, and neither program sends MSCDEX audio
channel control: its IOCTL output blocks are one byte with codes 0, 2 or 5
(DAT `0x3d39c`, `0x3d712`, `0x3d90f`, `0x3d8da`, `0x3d94b`; EXE `0x11e75`,
`0x122e6`, `0x122fd`, `0x1247b`, `0x124cf`). CD speech stays at the reset
level.

**DOSBox-referenced mapping.** The listening reference applies these writes
on its SB16 ([soundblaster.cpp](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/hardware/audio/soundblaster.cpp#L2400)):
each nibble n becomes (n << 1) | 1 (high nibble left); `calc_vol`
attenuates 2 dB per step below 31, one dB less beyond twenty steps; master
multiplies DAC, OPL and CD audio; line-in `0x2e` is stored but not applied.

| Byte | Origin | Level |
| --- | --- | --- |
| 255 | Init master and FM; reset | 0 dB |
| 153 | Init DAC | −24 dB |
| `0xaa` | Command 75 | FM −20 dB |
| `0xbb` | Command 95 | FM −16 dB |
| `0x99` | Command 48 | FM −24 dB |

No command argument restores 255 (arguments `0x90..0x9f` would), so after the
first entrance scene the music stays 16 dB lower until the next program
start. MMModern keeps this ([quirk](../original-quirks.md#sv-promus-pas-mixer)).
Real SB Pro hardware has a coarser curve; MMModern follows the DOSBox SB16
reference.

`XeenAudio` holds the registers (`XeenCardMixer`): a fresh process starts at
the reset levels and applies the installed CFG flag; `startProgram` repeats
the init for each EXE start and fresh DAT title; promus command `0x100` sets
FM. Gains reach the mix at the next block, independent of saves, RNG and
timing. The Clouds intro and ending, the Darkside intro and the four entrance
scenes issue the implemented sites above.

**DOSBox evidence, 2026-10-26:** device-free renders aligned with the
maintainer's captures (`D:/Projetos/MModern/MModern-re/audio-mixer-20261026/`):

| Comparison | DOSBox | MMModern | Without mixer |
| --- | ---: | ---: | ---: |
| King's voices over `endgame.m` at 95, minus the music before them | 9.1 dB | 9.2 dB | 4.2 dB |
| Crodo's narration at 75, minus the music before the tableau | −8.0 dB | −7.8 dB | +0.6 dB |

In `sphinx.avi` the music between the Sphinx's voices is 8.8 dB below the
music before and after the scene (48 against 95 predicts 8 dB), and those two
levels agree. Outside these differences MMModern's output stays about
3.4 dB below DOSBox's, the FM 0.5 against 1.5 × master 0.5 noted below.

## DOSBox-referenced balance and output recovery

**DOSBox-referenced measurement, 2026-10-22; not DOS hardware:** the
maintainer's DOSBox Staging 0.83.0 recordings and `xeen-staging.conf` are in
`D:/Projetos/MModern/MModern-re/audio-m83/`. The config contains no sblaster/mixer overrides.
The version tag resolves to `7b40053b7ac580843d0461eba8c36a47a990e66c`.
Its defaults are:

| Setting | Versioned source and default |
| --- | --- |
| Sound Blaster | [soundblaster.cpp](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/hardware/audio/soundblaster.cpp#L3763): `sb16`, `sbmixer=true`. `ctmixer_reset` sets master, FM, DAC and CD to 31/31, unity under `calc_vol`; programs can subsequently change them. |
| OPL | [opl.cpp](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/hardware/audio/opl.cpp#L843): `oplmode=auto` selects OPL3 for SB16; Nuked is the only emulator; channel scalar 1.5 (+3.52 dB). |
| Channel and CD volume | [mixer.h](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/audio/mixer.h#L349): user, application and base scalars start at 1. CD adds no OPL-style boost; [MSCDEX channel control](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/dos/cdrom_image.cpp#L845) uses volume/255. |
| Master and processing | [mixer.cpp](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/audio/mixer.cpp#L162): master 0.5 (−6.02 dB); compressor and denoiser on; 20 Hz master high-pass. Reverb, chorus and crossfeed off. Windows defaults: 48000 Hz, 1024-frame blocks, 25 ms prebuffer, negotiation off (`init_mixer_config_settings`). |

This SB16 backend applies the music drivers' mixer writes
([card mixer](#card-mixer)): during ordinary play the running game's DAC is at
−24 dB, not at its reset value. These defaults identify the listening
reference; the measured source pairs below confirm the resulting balance.

All three AVIs carry lossless PCM s16le, 48000 Hz stereo with ZMBV video:
`ferreriromusiconefxoffspeechoff.avi` (13.869 s),
`ferreiro musiconefxon.avi` (7.563 s), and `gunther.avi` (26.438 s).
The method and ±1.5 dB per-pair tolerance were recorded in M83 decision 4
before measurement. ffmpeg/ffprobe from `C:/msys64/ucrt64/bin` decoded them.
Device-free `XeenAudio::render` produced isolated smith.m, whaddayo.voc,
town.m and Gunther's 3.600000..21.813333 s cue at 48000 Hz, both before and
after the fixed gains. Measurement scripts (`measure.py`, `local-fit.py`,
`music-fit.py`, `final-measure.py`, supporting alignment scripts and
`render.cpp`) and their PCM/array evidence are retained outside the
repository in `D:/Projetos/MModern/MModern-re/audio-m83/`.

Local waveform correlation estimates voice amplitude against the isolated
renderer; aligned music-only power estimates the FM amplitude. The greeting
begins at **0.9377 s**. The 2..3 s interval principally identifies the smith
song starting, and is not a greeting-level comparison window. Thirty-three
correlated voice segments give a median amplitude near 0.0299 relative to
unity VOC. Music-only alignment puts DOSBox FM at approximately 0.73..0.78
of unity FM. Gunther begins near 1.102 s; 113 voice segments, compensating
capture drift, give CD amplitude approximately 0.51, with the underlying
town FM near 0.74. The results below are **source gain ratios**, independent
of a selected passage's music/voice energy. Ratio differences use
`20 log10(MMModern ratio / DOSBox ratio)`; common output gain cancels.

| Source gain ratio | DOSBox | MMModern | Difference |
| --- | ---: | ---: | ---: |
| VOC/FM | 0.0407 | 0.0375 | −0.7 dB |
| CD/FM | approximately 0.69 | 0.6875 | within ±0.3 dB |

Both meet the maintainer's ±1.5 dB tolerance. **Maintainer decision,
2026-10-22:** the capture is the authority for VOC level. Its approximately
28 dB attenuation below the SB16 reset-level prediction was the init DAC
level (−24 dB) plus DOSBox's 1.5 OPL scalar (3.5 dB): the card-mixer model
predicts VOC/FM 0.042 and CD/FM 0.667 (2026-10-26).

Every source has the common base g = 1/3: FM **1.5·g = 0.5** (DOSBox's OPL
scalar), VOC and CD **g**. These bases are fixed, not options or per-track
corrections; the card mixer then scales each source as DOSBox does. At init
levels VOC is 0.0210 and CD 0.333: VOC/FM 0.042 (+0.3 dB against the
capture) and CD/FM 0.667 (−0.3 dB). M83's earlier fixed 0.01875 and 0.34375
gave −0.7 dB and within 0.3 dB. FM/CD base gain is applied to native PCM **before**
SDL resampling and its 16-bit output clamp, preserving inter-sample peaks
that would otherwise clip before attenuation. The VOC base and every mixer
gain apply once at the final sum, rounded to nearest. At init levels the
gains total 0.854, giving **1.37 dB nominal headroom** for coherent
full-scale inputs; interpolation overshoot can consume that margin, and the
final sum retains an S16 safety clamp. With the mixer flag off the DAC stays
at 0 dB and the sum can reach that clamp. The simultaneous-source and
synthetic inter-sample-peak tests cover both paths. The maintainer's M83
Part C listening and Windows device play-test passed on 2026-10-22.

**Output contract (maintainer decisions 1/2):** gameplay deadlines, RNG,
timing and save state never consult the device. `check()` reports an output
failure once and keeps the session running; it no longer rethrows a callback
error. The existing speech-notice path restores hidden NPC text on output
loss without changing the NPC polling schedule.

| Case | Behavior |
| --- | --- |
| Callback/FM renderer or conversion error | Zero the entire failed callback, latch silence, close/join on the main thread, report one notice. No further FM requests accumulate in an unconsumed queue. |
| Backend STOPPED without a removal event | Allow 250 ms for SDL's removal notification, then close/join and report one notice. |
| SDL_AUDIODEVICEREMOVED for the opened output | Main-thread close/join, discard already queued notices for the old ID, reopen the current default at its own rate. Capture and unrelated-device events are ignored. Continue the same FM driver/chip/song. VOC and CD stay silent until their next explicit start; logical deadlines and retained cue operands remain. An interrupted CD cue restores text. |
| Windows default endpoint or mix-rate change | SDL WASAPI may follow it internally. Probe SDL's current default name/rate every 250 ms and reopen when either changes, retaining source state and VOC phase. Recreating FM/CD conversion discards its small queued tail; one small discontinuity is accepted. SDL cannot identify two endpoints with identical names/rates; its WASAPI default-generation tracking handles that case. |
| Failed reopen or conversion preparation | Close the attempted output, remain silent, report one notice and continue gameplay. |

SDL does not identify whether a removal notification came from unplugging
hardware or a backend failure: [SDL_audio.c](https://github.com/libsdl-org/SDL/blob/release-2.32.10/src/audio/SDL_audio.c)
uses the same event for disconnection. Recovery therefore follows the event,
not an invented physical cause. [WASAPI recovery](https://github.com/libsdl-org/SDL/blob/release-2.32.10/src/audio/wasapi/SDL_wasapi.c#L135)
and [fresh default endpoint queries](https://github.com/libsdl-org/SDL/blob/release-2.32.10/src/core/windows/SDL_immdevice.c#L489)
stay behind `platform/sdl`; there is no new platform owner or saved state.

**Maintainer decision, 2026-10-22:** preserve M82's FM latency. Both FM and CD
conversion refill in **512 native frames**, using preallocated buffers and
SDL workspace prepared before callbacks. Reconfiguration recreates conversion
at the new output rate and discards the small buffered tail; no history/skip
replay or whole rate-ratio periods remain. One small discontinuity is accepted
instead of adding steady-state effect latency. The current FM driver/chip/song
and rational VOC phase survive; gameplay never waits for the device. FM
requests are transferred with producer-allocated queue nodes, and the bounded
Promus call stack is reserved before callbacks. No source buffers are changed
until the old callback has joined. A 48 kHz real-converter witness sweeps
512-frame callback phases: effect 20 becomes audible within 32.92 ms
(33 ms regression guard, including its initial attack).

### SDL resampler rate limitation

**Known limitation retained by maintainer decision, 2026-10-22:** SDL 2.32's
resampler makes FM about 0.066% fast at 48 kHz (0.036% at 44.1 kHz) and CD
about 0.050% fast at 48 kHz. This is unchanged since M82 and inaudible.
The 512-frame refills remain; a bounded converter is a possible later fix.
Logical deadlines, gameplay timing, RNG and saves remain independent of output.

## CD disc addressing and boundary failures

`XeenInstallationDetector` retains separate disc identities from both INS
files. `XeenCdAudio` measures cumulative separate-file extents, maps the cue
to 44100 Hz sample windows and decodes only those windows with static
libvorbisfile. `XeenAudio` publishes immutable PCM to its independently
resampled CD mixer slot. File access and decoding never run in the callback.
A worker decodes the requested cue window (at most seven minutes from the
16-bit MMSSFF operands). NPC starts sample logical timing before
queuing work; generations discard results after stop or replacement. The
worker retains PCM owners until callback references retire, so replacement
and EOF never free large CD buffers in the callback. Absolute starts
can select a later track. At EOF, Staging only advances when at least two
seconds remain; otherwise the valid prefix plays to EOF while the logical
interval retains its approved duration. A started request reaching disc
lead-out likewise keeps its playable prefix (`PlayNextAudioTrack`, lines
1007..1014 of the pinned Staging source below); this differs from an initial
start in lead-out, which is refused. INDEX 01 positions contribute to
cumulative track starts; they do not seek ahead within a separate file.
Unexpected audio-track metadata disables that disc's audio with a per-cue
notice and text fallback; malformed data-track metadata still rejects the
installation. A final rounded partial sector decodes to silence, and initial
refusal follows the absolute starting track type, not the operand's track
number (maintainer edge-case decision, 2026-10-22).
Missing media preserves the logical interval and enables the text fallback;
the two data-track refusals below have no interval.

**DOS static, high (2026-10-22):** DAT `0x3d827` obtains the track's
absolute MSF address. `0x3da0d..0x3da73` converts MSF to sectors;
`0x3db3d` subtracts 175 from the start and `0x3db9a` subtracts 150
from the end. Packet `0x3d9b4..0x3d9e8` uses mode 0, absolute sectors,
not track-relative addresses. For track start LBA T and decoded cue times
S and E in sectors, the request is `[T+S-25, T+E)`. The 150-sector MSF
bias cancels; the extra 25 sectors are the game's preroll. Ordinary Clouds
selects disc 1 (`0xf383`, `0x3d63f..0x3d806`). Track numbers alone do not
identify a file across both discs.

**Installed-emulator source, high:** DOSBox Staging reports 0.83.0
(`7b400`), matching release commit `7b40053b7ac580843d0461eba8c36a47a990e66c`.
Its [AddTrack](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/dos/cdrom_image.cpp#L1571)
builds cumulative starts for separate files from the preceding file's
length, rounded up to a sector, and the cue's index/pregap. Track-info
MSF adds 150; playback subtracts the selected track's cumulative start
and seeks within its file. The installed `GAME1.INS` has separate files,
INDEX 01 zero and no pregap directives. Thus Gunther's encoded 370..2161
becomes S=295, E=1636 and plays `xeen19.ogg` at 3.600000..21.813333 s.
This independently reproduces the retained DOS capture calibration.

**Program/resource/emulator inference, high; no new playback capture:**
two map 30 rows, `(10,20)` and `(9,22)`, line 0, request track 2 with
encoded 24..416. Their start is T-1, inside data track 1. DOSBox's
`GetTrack` selects that data track; `PlayAudioTrack` rejects its data
attribute. Its [MSCDEX status path](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/dos/dos_mscdex.cpp#L866)
reports `0x8000`, with no busy bit. DOS retries only `0x810c`
(`0x3dbcb..0x3dbf6`), rounding the encoded start to the next second once.
This emulator error does not take that retry. **Maintainer decision,
2026-10-22:** MMModern plays nothing and starts no CD-busy interval or speech
wait on this refusal; this replaces the former unconditional 317/75-second
interval. Show its text under the approved delivery-failure fallback.
Do not clamp, shift or guess another start. This is high-confidence static
inference for the installed emulator, not a newly captured DOS play-test;
see the [failed-start rule](../original-quirks.md#sv-cd-failed-start).

Map 49 `(8,17)`, line 0, track 3, encoded 20..474, starts five sectors
inside the preceding audio track. **Program/resource/emulator inference,
high:** the request has 379 sectors (5.053333 seconds). It begins at
`xeen02.ogg` 153.453333 seconds, reaches that file's 153.520000-second
end, then plays `xeen03.ogg` from zero through 374/75 seconds. DOSBox's
[`CDAudioCallback` and `PlayNextAudioTrack`](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/dos/cdrom_image.cpp#L1007)
advance to the next audio file at EOF when at least two seconds of the
request remain, preserving the total/played-frame accounting. This cue
has 4.986667 seconds remaining, so it takes that path. Do not clamp its
start to track 3. The nominal interval is confirmed; codec seek precision
and callback latency are not new measured timing corrections.

**Resource census and signal measurement, high for counts and bounds:**
the initial Clouds CUR reconstructed from XEEN.CC has 207 PlayCD rows on
25 maps, using tracks 2..29. Primary Event IDs below 100 avoid the hash
aliases produced by treating extension map IDs as separate Event files.
All 207 nominal ends fit their OGG files; three nominal starts are negative
as described above. Decoded mono 8 kHz measurements use 100 ms edge RMS;
tracks 9, 17 and 18 have edges above -35 dBFS. These are suspicions for
listening, not evidence of a shifted rip or a correction table. Speech
intelligibility and full utterance containment remain **Unknown** for
every track; energy alone cannot certify them. Per-row operands, source
offsets, durations and measurements are in the external `cues/` tables.

## Digital and CD lifecycle evidence

`XeenAudio` retains cue operands and logical CD playback in process state.
NPC invocations start them; F8 replay is omitted by maintainer decision
(2026-10-22). Ordinary panel entry and Escape leave playback alone.
Logical deadlines use accumulated presentation time, retaining their remaining
duration across title/session epochs. NPC, YesNo and WhoWill dismissal stops
playback, as does confirmed Quit; an actual disc change also clears operands.
None of these values belongs to save v18 (schema 9, content 14).

**DOS static, high:** Efx off masks sample starts, stops and queries at
`0xdea9..0xdecd`; it does not issue a driver stop. Speech toggling at
`0x1f054` changes its option only; panel exit `0x1f31b -> 0xf383` returns
when Speech is off. Already playing CD audio therefore continues. CD busy
`0x3d5fd..0x3d608` tests MSCDEX bit `0x200` without a Speech gate. FM,
VOC and CD use separate drivers/status; the digital driver does not consult
FM or CD busy. A combat sample replaces another VOC through the same
wrapper (`0x2c111`, `0x2c172`, `0x2d802`); an FM effect alone does not.

Resource access can interrupt CD audio: `0x3d23d..0x3d279` stops a busy
cue when the active archive is not available locally, setting a pending
flag. Resource-load returns at `0xdfaf`, `0xdfea`, `0xe02b`, `0xe072`
invoke the retained cue again; this is a restart, not a saved audio cursor.
Archive-presence checks are at `0x3d55a..0x3d595`; the installed WORLD
folder has CUR files but no local XEEN.CC/DARK.CC. **Maintainer decision,
2026-10-22:** preserve existing cue intervals across resource reads as an
[approved approximation](../original-quirks.md#dev-cd-resource-read-timing).
**Final maintainer decision, 2026-10-22:** resource-read stop/restart cycles
are never modelled, including audible restarts. The entire class is closed;
additional reads are listed externally without another decision.

**Ordinary Clouds witness, DOS static/resource, high (2026-10-22):**
map 12 `(11,12)`, facing 2, line 1 retains track 16, encoded 5410..10123;
line 2 opens portrait 15 in NPC mode 0. Initial CUR offsets are
`0x9f5c/0x9f67` (Event offsets `0x17/0x22`). With Speech on and Text off,
the cue starts at `0x25aee`. Input is accepted while busy
(`0x25b6d..0x25b8d`); an early Escape or N reaches the mode-0 exit
(`0x25bda..0x25bf8`) without first stopping CD. That exit clears the
portrait handles (`0x25c04..0x25c16`) and, with Text off, reloads
`face15.fac` and `frame.fac` at `0x25c40/0x25c51`. The name format and
frame name are at DAT `0x53af1/0x53afe`.
The resource members exist in XEEN.CC at `0x76685f` (1132 bytes) and
`0x7f2a50` (1102 bytes), respectively; these are archive offsets.

Both loads call `0xdff1`: its pre-load hook `0xdff5 -> 0x3d23d`
stops still-busy CD and its post-load check `0xe024..0xe02b` restarts
the retained cue. The first load alone establishes the exception; whether
the second sees a renewed busy bit depends on playback starting promptly.
The dialog's explicit CD stop is later, at `0x25c94`. Thus a normal
input path can request CD resources while speech is busy, even though
it is about to dismiss that speech. This is a reachable code/data path,
not a captured audible restart or a measured I/O delay. Nine Clouds
PlayCD/NPC-mode-0 adjacent pairs are retained in the external
`mode0-cd-witnesses.json`; this does not claim a complete read inventory.
**Maintainer decision, 2026-10-22:** MMModern omits the transient restart
for this witness and the nine mode-0 pairs, and keeps the final explicit
dismissal stop. Dismissing the dialog stops its cue. Other read sites are
listed externally under the same final approximation.

The all-zero retained cue is a no-op before replacement
(`0x3da98..0x3daa7`); valid replacement stops the prior cue at `0x3dba7`.
Normal NPC dismissal stops CD at `0x25c94`; disc changes stop it at
`0x3d65f/0x3d664`. Service departure is not one universal digital stop:
the tavern explicitly replaces the sample with goodbye and polls busy
(`0x1d680..0x1d6c5`). Title lightning starts at `0xbfcf`, stops at
`0xc029`; dream samples run sequential busy loops (`0x1e66d..0x1e6bb`).
The following matrix separates explicit transitions from resource-read
side effects. **DOS static, high** for the listed calls; absence of a stop
is a **medium-confidence bounded control-flow finding**, not a playback
capture.

| Path | VOC | CD |
| --- | --- | --- |
| Ordinary control-panel entry and Escape | No sample stop in the handler | No final stop on unchanged-options Escape (`0x1f2e8..0x1f331`); approved approximation preserves the interval |
| Load from the panel | Does not issue a sample stop or take the title's drain; driver busy continues until completion/replacement, subject to the newly loaded Efx query gate | `0x1f14d..0x1f193`, `0xd2fa..0xd305`, `0xb819..0xb829`, `0x39460 -> 0xf013`: no explicit stop or cue-operand reset. Preserve active process playback unless the resulting disc selection changes |
| Cancel Load or save-slot selection | No sample stop in `0x2042d..0x20c72` | No explicit stop or operand reset; resource cycles omitted |
| New or Load from the title | Title lightning has its own start/stop; New `0xc113..0xc13a` and Load `0xc162..0xc1b8` join at `0xc66b`. Neither initializes the sample driver again | No automatic cue start from save/template state. Retained process operands survive unless a disc change clears them; New template load `0x20b95..0x20be5` does not clear them |
| Return from gameplay to title | Polls VOC before freeing its buffer (`0xb933..0xb947`); with Efx on it drains, with Efx off the query is masked and no physical stop occurs | No explicit stop/reset in `0xb90f..0xb98b -> 0xb65c`; preserve the active interval across the clock handoff under the resource-read approximation |
| Tavern departure | Stops/replaces with goodbye, then polls busy (`0x1d680..0x1d6c5`) | No independent location-exit stop |
| Other Clouds service exits / ordinary map exit | Shared exit `0x1d669..0x1d7d3` has no blanket stop; the other voiced departure branch is Darkside-only. Map reload likewise does not stop the sample driver | No location-wide cancellation. Map-load tail `0x10772 -> 0xf383` checks whether the selected disc changes |
| NPC Escape/N dismissal | Does not itself issue a VOC stop | Explicit stop `0x25c94`; mode-0 portrait reload exception is approved above |
| Yes/no and Who Will helper exits | No universal VOC cancellation inferred | Explicit stops `0x24c66` and `0x252ec` |
| Numeric Choose123, ordinary messages, Event cleanup | No global Escape-triggered stop; explicit action samples still follow their own sites | No blanket cancellation: numeric return `0x24ee9`, Event cleanup `0x291d8`; a nested yes/no or NPC helper still has its explicit stop |
| Main exploration Escape | Not a global audio action | Main key table `0xd63c` has no Escape action; generic input normalization does not stop CD |
| Title animation Escape / menu return | Title lightning's explicit stop remains at `0xc029`; input is not a generic sample cancel | No generic CD stop in `0xbcce..0xc766`; a process exit is distinct |
| Confirmed Quit / termination | Driver shutdown, without the title drain | Panel confirmation calls `0x21ecd` at `0x1f20d`; shutdown `0x21ecd..0x21efb` reaches runtime termination rather than returning through `0xb90f` |
| Archive side selection / actual disc change | Neither stops the digital driver | Archive selector `0xe89e..0xe937` has no stop. With Speech on, `0xf383..0xf40e` checks the required disc; an actual change stops twice at `0x3d65f/0x3d664` and clears operands at `0x3d7fa..0x3d806`. Speech off skips that map/panel disc check |
| Fresh process | Driver initialization starts idle | Startup stops CD (`0x3d45a`); operands begin zero. Saves contain options, not audio cursors or retained cue operands |

The CD stop/start/busy call inventory and the digital wrapper inventory
bound these conclusions. The driver shutdown stubs are reached from
termination, not ordinary Load/New or archive selection. Resource helpers
`0x46080/0x460dd` close/free archive data, not the audio drivers.
The auxiliary F10 handler explicitly stops CD at `0x1be38` and resumes it
with MSCDEX command `0x88` at `0x1bf3d -> 0x3d984`; it has no VOC stop.
This is distinct from Escape and from a resource-read restart.

**Logical-state consequence:** retained operands `DS:84e0/84e2/84e4`
are process state, separate from saved party options at `DS:ed8c`.
Opcode 60 writes them (`0x28b49..0x28b57`); disc change clears them.
Same-process Load/New therefore does not restore an audio cursor or
automatically start speech. A later NPC can use the retained operands.
`XeenAudio` preserves surviving intervals by remaining duration across
presentation-clock epochs and keep explicit stops separate. Current
Clouds NPC paths already stop on dismissal; F8 replay is omitted.
The approved driver-count and failed-start changes remain the only
established corrections to current wait lengths. Exact waveform effects
of DOS reusing a sample buffer during a masked-busy Load are unmeasured;
the matrix describes driver commands/busy lifetime, not freed-memory audio.

**Ordinary continuing-restart witness, DOS static/resource, high:** F8
replays the retained CD operands at `0xd4d0 -> 0x3da87`. Keyboard scan
`0x42` maps to internal key `0xd0` in the table at `0x456e8`; the main
dispatch table is at `0xd63c`. CD stop `0x3da74..0x3da86` leaves those
operands intact, so the replay is available after NPC dismissal. After
Gunther, replay with F8, let speech advance, then open the panel with Tab
while busy. Panel entry loads `cpanel.icn` at `0x1eec0` (name at
`0x5336e`, XEEN.CC member `0x63066c`, 279 bytes). This invokes the
same pre-load stop `0xdff5 -> 0x3d23d` and post-load replay `0xe02b`.
The panel accepts input without a CD-idle wait and its unchanged-options
Escape return has no final CD stop (`0x1f2e8..0x1f331`). The restarted
cue therefore continues while the panel is open and after Escape. The
control flow is established; no audible restart or I/O delay was captured.
**Maintainer decision, 2026-10-22:** F8 speech replay is omitted. The
preceding DOS witness remains evidence for the resource-read behavior;
MMModern has no F8 binding or action and does not reproduce that route.
Opcode 60 retention and NPC-started playback remain implemented. See the
[approved omission](../original-quirks.md#dev-cd-f8-omitted).

**DOS static, high for the table:** Event opcode 40 selects six names at
DAT `0x508cc..0x508e3`: 0 `ahh.voc`, 1 `whereto.voc`, 2 `gulp.voc`,
3 `null.voc`, 4 `scream.voc`, 5 `laff1.voc`. Its handler stops the old
sample, loads and starts the new one (`0x28724..0x2875a`), without a busy
wait. The Clouds census has 130 rows on 35 maps, using indices 0..4.

The title drain does not service input: `0xb933..0xb941` only polls busy,
then frees the sample at `0xb943..0xb947`. MMModern rejects dispatch as
soon as the gameplay outcome is finished, including Escape and F9,
while its logical busy interval drains. Panel Load writes return code 2
at `0xd2ff`, returns through `0xd5e8..0xd61b`, and branches from
`0xb836..0xb839` directly to `0xb819`; it never reaches `0xb90f`.
Confirmed panel Quit takes the termination branch above and likewise
does not reach the title drain. The separate main-loop return-code-1
cleanup (`0xb831..0xb834 -> 0xb83e`) has its own poll at
`0xb892..0xb8a0`; it is not the confirmed panel Quit path. No title wait
is added to the supported Load or Quit outcomes.

### VOC inventory boundaries

**DOS static/resource, high for call operands; medium for owner mapping:**
87 references to the digital wrapper include 35 physical start calls:
32 Clouds/shared and three explicitly Darkside-only. A physical call is
counted once even when a table, loop or branch chooses several samples.
The external `dos-voc-starts.md` lists every start, name/template and gate.
All pass the Efx/driver-presence gate; side-specific alternatives are
identified separately.

| Existing owner or bounded correspondence | Clouds/shared calls |
| --- | ---: |
| `XeenEventFlow` death/dream | 4 |
| Title audio path (`XeenAudio` owns current title music) | 1 |
| `XeenEventFlow::beginFall`/`advanceFall` fall and landing | 2 |
| `XeenTrainingFlow` success | 1 |
| `XeenSmithFlow` service actions | 9 |
| Shared training-refusal/guild-rejection call | 1 |
| `XeenServiceAnimation` greeting via Event flow | 1 |
| Event interpreter opcode 40 | 1 |
| `XeenEntranceScene` | 9 |
| Player-damage candidates / monster attack path | 3 |

The implementation's literal resource census is 38 distinct names, plus
dynamic weapon-hit and monster-voice names below. Entrance, Event flow,
service greetings/actions, training refusals and shared damage sites use
their existing owners. This corrects the approximate 40-name estimate;
resource names and physical DOS calls are different counts.

Dynamic player-hit names at `0x2c0c5..0x2c111` are `pow0.voc` through
`pow5.voc`, selected by the 35-byte weapon table at `0x4d660`.
`miss.voc` loads at `0xb7b1` and plays at `0x2c172` in the nonpositive
player-damage branch, corresponding to `XeenPhysicalPlayerCandidate` and
`XeenSpellDamageCandidate`; it is not the enemy miss effect. Monster
attack voices format the record's string at +`0x33` as `%s.voc`
(`0x2d7c8..0x2d802`). The current Clouds bridge reads
`DARK.CC/xeen.mon`: 90 records, 70 case-sensitive strings / 69 normalized
voice names, all present in XEEN.CC. This is an attack voice, not a death
voice. Exact lists and member offsets are external.

The expanded audit finds header time constants 120, 181, 172 and 173 in
`blob.voc`, `boom1.voc`, `roc.voc` and `snakeman.voc`, respectively, all
codec 0. These differ from the driver-selected 188, already covered by
the adopted fixed-rate rule. It finds no extra blocks,
non-type-1 first block or length-word overflow in the identified monster
and weapon set. The initial 36 literal samples retain their clean
layout audit above. Broad string-scan candidates are not a reachable
Clouds census and are not included in these claims. Fall/landing samples
belong to the fall presentation documented below (M86).

**Publication mapping, DOS static/current source, high:**

| Sample | DOS ordering and existing MMModern publication point |
| --- | --- |
| `pow0..5.voc` | Positive-damage gate precedes `0x2c082`; stop old VOC `0x2c0b2`, select/load sample, FM `60+selector` at `0x2c105`, VOC at `0x2c111`, then HP comparison/stores `0x2c129..0x2c15b`. The existing weapon-impact callback in `XeenCombat::serviceConsequences` publishes physical `PreHp` immediately before HP publication or the nonsequenced positive result; `XeenCombat::serviceCast` publishes `Impact` before HP/status publication. `XeenEncounterFlow::fmWeaponEffect` in `XeenJourneyFlow.cpp` reaches `XeenAudio::weaponEffect`; that request includes the sample, preserving its FM/VOC order and independent Efx gate. No new wait or RNG draw. |
| `miss.voc` | VOC `0x2c172` precedes FM 6 `0x2c17c`. The exact candidates are `XeenPhysicalPlayerCandidate::service` (`XeenCombatRules.cpp`), condition `!damage && (!shoot || hit)`, and `XeenSpellDamageCandidate::service`, condition `attack2Entered && damage<=0`. Publish once with the existing guarded effect request, before result publication. A missed missile or a spell rejection that never enters attack2 bypasses both cues; do not attach it to every displayed miss or every FM 6. |
| Monster record `+0x33` voice | Pose frame 8 is set at `0x2d778`, scene calls at `0x2d793/0x2d79b`, old VOC stops at `0x2d7a5`, resource loads at `0x2d7f4`, and playback starts at `0x2d802`, before the target/attack loop. Current `XeenCombat::serviceConsequences` publishes `MonsterPose1` before enemy-candidate service. The sample belongs to `XeenEncounterFlow::observeCombat`'s once-per-generation first-pose branch, beside the existing record `+0x32` FM request and frame-8 presentation, before subsequent target-selection/RNG in `XeenEnemyAttackCandidate`. Do not replay at Pose2, per target, or on redraw. |

**Fall/landing presentation, DOS static, high (M86, 2026-10-23):** FM 11
(`0x11692`) precedes `scream.voc` (`0x116a0`). After the fifteenth scrolling
image, `0x116ec` stops the sample with prosnd command 0; `unnh.voc`
(`0x1170c`) and FM 31 (`0x11716`) precede the four impact images. Each image
waits one VGA retrace; damage and its existing FM cues follow the sequence. `XeenEventFlow::beginFall`
and `advanceFall` use the existing `XeenAudio` commands, through the terrain
transition shared by walking, Rest, casting and manual step consequences.
Effects off gates the samples and effects without skipping images or changing
terrain, time or RNG. See [terrain arrival](time-and-conditions.md#terrain-arrival).

**Title lightning outside M83 (settled by M91 Part 0, 2026-10-25):** DAT
`0xbf90..0xbf99` tests byte `[DS:0x0095]`; a nonzero value jumps to
`0xc056`, bypassing the 30-frame lightning sequence, start `0xbfcf`
and stop `0xc029`.
Readable XEEN.CC sets that byte to one at `0xb3d5`, and no reset occurs on
the reached startup/title path. The 30-frame path is therefore unreachable
on installed CD World. The CD title retains its 13-frame `world0/1/2` loop;
never attach `elect.voc` to it. See [title lightning](cutscenes.md#title-lightning)
for the calibrated writer and guard evidence.
Its resource exists in DARK.CC (constant 188, codec 0). The physical
inventory count above includes this excluded site, not 32 implemented starts.

### Digital start sites implemented by A2

M83 Part 0 inventories physical DOS calls, not every alternative name. All starts
use XeenAudio's Efx gate; Efx off also masks stop and busy without silencing
an existing sample. PCM is loaded and decoded on the main thread. Existing
flow waits retain the adopted driver-count duration; Tavern departure and
gameplay teardown to title poll the program-owned logical voice. No wait
consults the device and no playback state enters saves.

| DOS start | Clouds/shared names | Gate | Owner | Coverage |
| --- | --- | --- | --- | --- |
| `0xb112` | fire.voc | Efx | XeenEventFlow | implemented |
| `0xb294` | xeenlaff.voc | Efx | XeenEventFlow | implemented |
| `0xbfcf` | elect.voc | Efx | Title lightning | unreachable: installed CD World |
| `0x116a0` | scream.voc | Efx | `XeenEventFlow::beginFall` | implemented |
| `0x1170c` | unnh.voc | Efx | `XeenEventFlow::advanceFall` | implemented |
| `0x1abf7` | trainin2.voc | Efx | XeenTrainingFlow | implemented |
| `0x1aedc` | coina.voc | Efx | XeenSmithFlow | implemented |
| `0x1af54` | ahh.voc | Efx | XeenSmithFlow / Event bridge | implemented |
| `0x1b0c9` | gulp.voc | Efx | XeenSmithFlow | implemented |
| `0x1b31e` | thankyou.voc | Efx | XeenSmithFlow | implemented |
| `0x1b469` | thankyou.voc | Efx | XeenSmithFlow | implemented |
| `0x1b8d4` | trainin0.voc, trainin1.voc, guild11.voc | Efx | XeenTrainingFlow / XeenSmithFlow | implemented |
| `0x1c7cb` | coina.voc | Efx | XeenSmithFlow | implemented |
| `0x1d5c7` | banker.voc, whaddayo.voc, guild10.voc, hello.voc, maywe2.voc, training.voc | Efx | XeenServiceAnimation via XeenEventFlow | implemented |
| `0x1d6b1` | goodbye.voc | Efx | XeenSmithFlow | implemented |
| `0x1e66d` | dreams2.voc | Efx | XeenEventFlow / XeenRestFlow | implemented |
| `0x1e6af` | laff1.voc | Efx | XeenEventFlow / XeenRestFlow | implemented |
| `0x2875a` | ahh.voc, whereto.voc, gulp.voc, null.voc, scream.voc, laff1.voc | Efx | Event interpreter / XeenEventFlow | implemented |
| `0x298ab` | sphinx10.voc, sphinx11.voc, sphinx12.voc, sphinx13.voc, sphinx14.voc | Efx | XeenEntranceScene | implemented |
| `0x29e9f` | ogre.voc | Efx | XeenEntranceScene | implemented |
| `0x29f97` | ogre.voc | Efx | XeenEntranceScene | implemented |
| `0x2a06d` | golem15.voc, golem13.voc | Efx | XeenEntranceScene | implemented |
| `0x2a250` | ogre.voc | Efx | XeenEntranceScene | implemented |
| `0x2a2a2` | golem16.voc, golem14.voc | Efx | XeenEntranceScene | implemented |
| `0x2abfd` | reaper12.voc, reaper14.voc | Efx | XeenEntranceScene | implemented |
| `0x2ad63` | reaper13.voc, reaper15.voc | Efx | XeenEntranceScene | implemented |
| `0x2b5f9` | dwarf10.voc, dwarf11.voc, dwarf12.voc | Efx | XeenEntranceScene | implemented |
| `0x2c111` | pow0.voc, pow1.voc, pow2.voc, pow3.voc, pow4.voc, pow5.voc | Efx | XeenCombat impact / XeenAudio | implemented |
| `0x2c172` | miss.voc | Efx | Player damage candidates / XeenAudio | implemented |
| `0x2d802` | monster record +0x33 | Efx | XeenEncounterFlow::observeCombat | implemented |
| `0x34881` | guild11.voc | Efx | XeenSmithFlow | implemented |
| `0x34d32` | guild12.voc | Efx | XeenSmithFlow | implemented |

Three further physical starts are Darkside-only (`0x1c500` spit1.voc,
`0x1d6fe` come1.voc, `0x319f2` choice2.voc). The 35-call physical census
therefore comprises 31 implemented starts and four excluded starts: the
unreachable title lightning and the three Darkside-only starts.

Opcode 40 reads the six far pointers at DAT `0x508cc`, publishes once and
continues immediately. Its Clouds members use XEEN; the final `laff1.voc`
member uses DARK, as in the M83 Part 0 resource inventory. Opcode 60 publishes
retained CD operands without a notice. Every original Clouds opcode 40 row
(130 rows in 35 maps) has an index in 0..4. An out-of-table index (6 or
more) retains the automatic "Voice audio not supported yet" notice and
continues; Efx off skips that notice along with the start.

Combat pow stops the old voice at the existing positive-damage impact callback,
requests FM 60+selector then VOC before HP publication. Player miss guards
request VOC before FM 6; unrelated enemy misses do not request miss.voc.
Monster voices publish only on guarded MonsterPose1, before target-selection
RNG, once per attack, never per target, second pose or redraw. Missing
or corrupt impact and miss samples raise the same resource error as
other starts; the FM output-failure suppression does not cover VOC loads.
Entrance guardians use the caption gate above. Death starts fire at
entry to the retained death state, independently of drawing or retries.

## Text-off byte: readers and presentation

`XeenEventFlow` selects the title-only NPC presentation for an admitted
cue. A known media failure shows the body immediately; a worker decode
failure restores it on the next presentation update, with one notice.
The presenter uses the original title-only format and compact window 40.

**DOS static calibrated against the maintainer Gunther screenshot, high
confidence (2026-10-22):** `0x257a5..0x257c3` closes window 11 and opens
window 40 when Text is off; `0x2585b..0x25861` writes its title. The original
window table gives x=226 (`0x3dc4e`), y=0 (`0x3dcae`), width=93
(`0x3dd0e`), height=100 (`0x3dd6e`) and margin=8 (`0x3de2e`).
The bounds reader `0x4487e..0x448c1` derives inner bounds
[234,311) x [8,92); the border writer at `0x44b60..0x44b73` places its
rightmost eight-pixel tile at x=311. Thus the outer rectangle is
[226,319) x [0,100), preserving the scene, party bar and remaining buttons.
The format at `0x53b20` clears only that inner rectangle, chooses normal
font, sets relative y=56 and centers the title: Gunther's resource newline produces two
lines at y=64 and 74; `0x2582b..0x2584a` resets the resource tab to zero. The title writer retains the reduced-font retry
(`0x25874..0x25880`). Frame position (248,12) comes from
`0x258e3..0x2590f`; face position (255,18) from
`0x256ef..0x25716`, drawn at `0x2591d..0x2592d`. These match the
small upper-right parchment in `gunther-textoff-dos.webp`; the former
full-screen bounds were borrowed incorrectly from the pin's window-40 table.
Acknowledgment still accepts any key or mouse click. Mode-0 YesNo buttons
and hit rectangles remain at x=235/260, y=101 (`0x25969..0x259ac`);
The mode-0 redraw at `0x259bb..0x25a31` restores the compact window
over the button-panel backing; the delivery fallback retains body text
and y=75.

The Text-off loop polls at one tick (`0x25b77`); the visible-text loop uses three ticks
(`0x25b94`). Missing speech restores body text only: Text off still uses
the one-tick ordinary loop, and fallback YesNo buttons stay at y=75
(maintainer Q1, 2026-10-22). Event
messages and service greetings do not use this NPC body gate.

**DOS static, high confidence; recalibrated 2026-10-22:** the Text-off
branch at `0x25af8..0x25aff` enters `0x25b02`, which resets the BIOS timer
through `0x440cd`. It draws the face, polls input at `0x25b6d`, and tests
elapsed time from `0x440dc` against **1** at `0x25b77`. Input can end the
hold early; otherwise the loop repeats from the reset at `0x25b02`.
Text on instead enters `0x25b8f..0x25bad`: wait for **3** ticks at
`0x25b94`, draw through `0x1c038`, reset, then poll input. These are
different loops, not two interpretations of the same interval. The earlier
three-tick NPC entry describes the Text-on loop only.

Both use the elapsed helper calibrated against the known Reaper two-tick
wait at `0x2ac94..0x2ac9c`: `0x440cd..0x440db` records INT 1Ah's CX:DX
tick counter and `0x440dc..0x440f4` subtracts that recorded counter.
The Text-off CD-busy test (`0x25b0f..0x25b29`) selects one RNG draw in
0..3 while busy, or face zero while idle; it does not change the one-tick
cadence. Text-on face activity uses the busy gate at `0x1c417..0x1c45c`
within its three-tick loop. The ordinary Text-off loop chooses its first
face before its first hold. Passive NPC mode 2 returns at `0x25ac4`,
before entering either ordinary NPC loop. A retained face in the YesNo
helper uses the helper's three-tick hold (`0x24bd7..0x24c16`, constant
at `0x24bec`), including with Text off; hiding its body does not transfer
the ordinary NPC loop into that helper. The instruction bytes in external
`audio-m83/npc-text-poll.dis` and `audio-m82/part0-calibration.dis` were
rechecked against the installed `WORLD/XEEN.DAT`, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
This is calibrated program evidence, not a measured startup latency or an
offset correction based on speech energy.

**DOS static, high for direct scalar accesses:** the scan of `DS:f384`
finds 29 accesses, including the option toggle and three plain writes.
This is a classification of those direct accesses, not a claim about every
possible alias in a bulk party-state copy.

| Offsets | Classification |
| --- | --- |
| `0x1e45e`, `0x1e48a`, `0x1e49e` | Redraw the retained NPC window/face when an NPC is active; no new message suppression |
| `0x1ee2d`, `0x1ee35`, `0x1f04c` | Panel forces Text on when Speech is off, displays the option, and toggles it |
| `0x24a1b`, `0x24a66`, `0x24a84`, `0x24a98` | Yes/no helper positions controls and restores the existing NPC presentation; does not discard its question |
| `0x24cb9`, `0x24ccd`, `0x24cfb`, `0x24d0f` | Numeric Choose123 helper positions controls and restores the existing NPC presentation |
| `0x256ef`, `0x25703`, `0x257a5`, `0x258e3`, `0x258f7`, `0x25969`, `0x259b4`, `0x259d5`, `0x259e9`, `0x25c19` | NPC face/window/control placement and restoration; Text-off presentation uses window 40 |
| `0x257c9` | Omits NPC body text: format at `0x53b08` has title and body; Text-off format at `0x53b20` has title only |
| `0x25af8` | Selects the Text-off face loop, which polls CD busy at `0x25b0f` |
| `0x26a7d`, `0x26a83`, `0x291d8` | Save, temporarily clear and restore Text off for the Darkside map-29 special case; outside Clouds |

NPC body omission does not test Speech, successful start or CD busy.
Consequently an all-zero cue with idle playback, or the two refused map-30
starts, can leave only the NPC title visible. This is explained by the
independent option branch; DOS does not restore the body on playback
failure. Speech off normally forces Text on through the panel. An unusual
raw option combination still follows the unconditional body-selection
branch. Ordinary Event messages and service greetings have no Text-off
body gate among these readers; their helpers can restore an existing NPC
window without suppressing the new text. MMModern applies the maintainer's
decision-3 text fallback whenever speech cannot be delivered; this is an
approved deviation, not evidence that DOS checks delivery.

## Other original sound devices

**Evidence boundary, 2026-10-27:** M94 Part 0 established the following
rules. Part A1 selects both drivers in DAT and EXE, implements null drivers
and blastsnd/prosnd digital transfers, and applies the reference mixers
below. Music 1/2/4/6 is selected but silent with a notice until its later
part; deferred devices use the approved null-driver restriction. Calibrated original
program evidence is **DOS static, high**; captures and the matching emulator
source are explicitly **DOSBox reference evidence**, not measurements of
physical cards. Detailed offsets, hashes, captures and analysis are retained
under `D:/Projetos/MModern/MModern-re/m94/` in `findings-0a.md`,
`findings-0b.md`, `findings-0c.md` and `analysis-0c/`. The ScummVM pin remains
the revision in [dependencies](../dependencies.md).

### Configuration, resources and program boundaries

Both programs read 29 bytes of `XEEN.CFG`: digital id +0, digital port +2,
music id +4, music port +6, IRQ +8, mixer word +`0x14`, DMA +`0x18`.
The installer writes 29 bytes too; no-question fields can retain old values.
DAT selection is `0xb506..0xb604`, tables `0xb9cc/0xb9de`; EXE selection is
`0x78a3..0x79e9`, tables `0x7c80/0x7c92`. Both load selected drivers from
INTRO.CC, with the ordinary loose-file override, initialize music then
digital once per program start, and shut down in reverse order. They do not
fall back to a similarly named driver in another archive.

| Music id / driver | Installer caption; established protocol |
| --- | --- |
| 1 `admus` | Adlib; one OPL2-compatible bank |
| 2 `blastmus` | Sound Blaster; one OPL2-compatible bank |
| 3 `promus` | Sound Blaster Pro; the installed two-bank NEW-mode OPL3 path above |
| 4 `rolmus` | Roland LAPC-1 / MT-32; MPU interface and Roland model-16 DT1, LA patches |
| 5 `canmus` | Roland Canvas / GS; GS-capable MIDI, including model-42 and model-16 messages; **not CMS** |
| 6 `specmus` | Pro-Audio Spectrum; selected INTRO bytes are identical to `blastmus`, one bank |
| 7 `ibmmus` | PC Speaker; PIT/speaker effects, no song interpreter |
| 8 `wavemus` | Wave Blaster; MIDI plus EXE-only E-mu product-04 codebook upload |
| 9 `tapmus` | Roland TAP-10; MIDI with model-42/56 traffic; exact target remains unknown |
| other `nullmus` | Silence, including effects; still installs the normal logical timer |

All 18 requested music/digital names exist in INTRO; XEEN contains 15,
and DARK none. Inactive XEEN copies can differ materially: in particular
XEEN `specmus` is two-bank and is not the selected INTRO alias. Never
substitute them. The installer never writes digital ids 3/4. Their DAT
selection is `nullsnd`, but EXE requests absent `rolsnd`/`cansnd`; digital
9 likewise requests absent INTRO `covsnd` in EXE instead of DAT `covxsnd`.
The [approved deviation](../original-quirks.md#dev-xeen-unavailable-digital)
uses notice/title return for EXE 3/4/9; DAT 3/4 selects nullsnd and DAT 9
uses the deferred stopgap.

Music init receives music port, mixer flag, an extra segment and digital
port; digital init receives digital port, music segment, IRQ and DMA.
`promus/nullmus` use **musicPort+4/+5** for flag-gated SB mixer writes.
`admus/blastmus/specmus/rolmus/ibmmus/wavemus/tapmus` instead use
**digitalPort+4/+5**; `canmus` neither reads the flag nor runs that block.
Ordinary init writes 2e/22/26=ff, 04=99; wave/tap use 04=cc plus 41/42=40.
These transactions affect a compatible mixer only at the addressed port.
Mono OPL command `0x100` caches a patch-volume argument, MIDI profiles send
controller traffic, and only promus changes the card FM level with it.

DAT Other Options **D** clears seen +`0x1a` and writes 30 bytes
(`0xc571..0xc59e`); **C does not write CFG**. The ending writes 30 bytes
at `0x28bde..0x28c0a`; EXE's Darkside seen-store does so at
`0xd34b..0xd374`. Byte +`0x1d` is adjacent memory, not an audio field read
next time. These handoffs do not change audio ids/ports or pass audio cursors.
MMModern never writes the original file; its remembered presentation flags
remain in settings. Ordinary New/Load inside DAT does not reinitialize audio.

### Interpreters, fades and digital transfers

Mono OPL and promus share flow operations, not all voice/patch semantics.
All use music voices 0..6 and FX 7/8. Mono music records occupy 26 bytes
but emit one 11-byte patch; mono FX records consume 11 bytes, promus 22.
Use the selected resource's dispatch, effect and patch tables. Admus has
additional slide/postprocess cutoff logic. Mono fade starts only for active
music, uses remaining=63 and unsigned phase carries, and keys off music on
completion. Retain the installed promus rules above unchanged.

Rolmus needs its own MIDI opcode profile: pitch, pan, program, velocity,
CC11 expression and checksummed SysEx differ from OPL operations. Its fade
visits voices 8 and 0..6, uses remaining=127, and decrements a cache when its
old value is at least 40: **40 is emitted as 39**. An eligible carry with all
old caches below 40 stops early; otherwise at most 128 carries.
Phase survives requests; terminal stream ticks can skip postprocessing.
The load-busy query is fade **speed**, not sounding notes. Full OPL/MIDI
effect ids are **0..180 inclusive**; active FX protect ids 7..10. Rolmus
ids 5,9,41..44,48,59,66..70 are timed ten-tick no-note replacements, not
permission to play promus effects. Evidence: INTRO rolmus
`0x36d..0x38f`, `0x985..0xa07`, effect table `0x10eb`; mono fade
admus `0x861..0x8b5`, blast/spec `0x856..0x8a0`.

**DOSBox MIDI:** the outdoor-change fade emits final expression 39 at
116002 ms, all-notes-off at 116029..116030, then the next song's controller
initialization at 116088..116098. The 27-ms interval is compatible with two
logical ticks and the static eligible-tick/terminal-skip rules; it is not
a fixed fade duration or evidence that a cache clamps to 40.

Let L be the unsigned word at sample +`0x1b`; PCM starts at +`0x20`.

| Digital driver / ids | Original transport and logical busy |
| --- | --- |
| `nullsnd` / 0, defaults, DAT 3/4 | Every command returns zero; no samples or timer changes |
| `blastsnd` / 6, including installer PAS Alt | Stop=0, busy query=1, other requests replace. Count register `uint16(L-14)`, fixed DMA 1, CFG DMA ignored; BC time constant. Count+1 samples at 68 us each is **medium-confidence inference**, as for M83. DOS has IRQ/page continuation and latched DSP-failure busy clearing (`0x101..0x29c`); MMModern omits page splitting under the approved approximation below. |
| `prosnd` / 7,10 | Existing installed model: `uint16(L-10)+1` samples at 68 us, CFG DMA consumed; existing allocation approximation unchanged |
| `ibmsnd` / 1 | Only caller flag 1 starts; foreground blocks for L-32 IRQs, PIT0 divisor 81, speaker PWM |
| `adsnd` / 2 | OPL TL modulation, L-32 or ceil((L-32)/2) completion IRQs, PIT0 divisor 81/162; last read is not written; CPU calibration selects branch |
| `source` / 5 | Disney Sound Source, eight strided writes per batch, advance 17, `floor((L-32)/17)+1` batches plus cleanup IRQ; PIT0 divisor 1364; busy is not FIFO drain |
| `specsnd` / 8 | PAS resident-query IRQ/DMA, on-card divisor 81, `uint16(L-10)` count; physical clock/oracle still open |
| `covxsnd` / 9, DAT | Covox DMA protocol, `uint16(L-10)`, raw busy count, stop only when busy==1; selected installer address versus actual control base remains open |

**Maintainer decision, 2026-10-27:** extend the
[unsplit transfer approximation](../original-quirks.md#dev-prosnd-dma-timing)
to blastsnd: `(uint16(L-14)+1) * 68 us`, no DMA page split. Part A1 uses
fixed DMA 1 for blastsnd and the CFG DMA for prosnd. A latched blastsnd DSP
failure clears busy on its next admitted query; host output failure does not
change that logical state. Prosnd's existing `(uint16(L-10)+1) * 68 us`
model stays unchanged. These are playback and
logical-busy durations, separate from waveform equivalence; the capture
publication bound cannot establish allocation-specific continuation.

**Approved temporary restriction, 2026-10-27:** deferred music 5/7/8/9 and
digital 1/2/5/8/9 behave exactly as nullmus/nullsnd, except EXE digital 9's
notice/title return. They are silent, have no driver busy/fade/load waits of
their own, retain the fixed BIOS rate and issue one standard "not supported
yet" notice. Deferred music runs **nullmus's own** flag/port-gated mixer
transaction, never the deferred driver's; deferred digital has no mixer
transaction of its own. Common CD speech and its waits
remain independent. This replaces the original deferred fades, blocking,
clock and mixer behavior; see the [dated restriction and DOS evidence](../original-quirks.md#dev-m94-deferred-null).

Efx gates sample starts, stops and queries. Entrance captions test Efx off
or digital id<2, not whether speech is actually audible. Speech controls
CD cue starts independently of the digital driver; nullsnd does not remove
CD narration or its waits. Text controls NPC body/face presentation separately.

### Clocks and the bounded MT-32 reference

All approved music profiles, silent nullmus included, install PIT divisor
**16390** and add it to a 16-bit BIOS-chain accumulator. IRQ rate is
1193182/16390 Hz; BIOS average remains 1193182/65536 Hz. Approved DMA
drivers do not take over PIT0. No approved driver changes the BIOS tick rate.
The original deferred drivers must not inherit that assertion; M94's
approved null-driver stopgap above deliberately replaces these behaviors:

| Deferred path | Settled static clock consequence; physical oracle still separate |
| --- | --- |
| ibmmus idle / type 2,3 | Idle divisor 16390; type 2,3 divisor 1193 with BIOS accumulator chaining |
| ibmmus type 0,1 | Variable divisors; active IRQs skip BIOS chaining, freezing BIOS time |
| ibmsnd | Divisor 81, forwarding increment 328: ordinary-music BIOS average 18.438034 Hz |
| adsnd fast / slow | Divisor 81/increment 324 or 162/increment 649: BIOS 18.213180 / 18.241287 Hz |
| source | Divisor 1364/increment 5453: BIOS 18.203165 Hz; idle handler remains until music shutdown |

Those averages assume ordinary music; ibmmus paired with the timer PCM
drivers can freeze BIOS time instead. Speaker initial PWM/gate behavior,
ADSND calibration reference, PAS clock, DSS drain and Covox working address
remain deferred questions, not M94 implementation work.

**DOS static, high:** rolmus init uploads 13 DT1 packets, lengths
33,267,18,256,18,256,18,256,18,256,18,256,18: **1688 bytes**, plus nine CC7
messages/27 bytes. Addresses are system `10 00 00`, six timbres at
`08 00/02/04/06/08/0a 00` and six patch mappings at `05 00..28`.
There is no factory-reset SysEx or fixed post-upload sleep. MPU reset FF
is an interface command. Command/data polling is at `0x57a..0x5f9`;
song opcode B adds **176 port-61 reads per address/data byte**. Counter
loops use FFFF retries; unexpected non-FE responses restart the ACK loop,
so a universal broken-hardware timeout is not established.

**DOSBox reference, maintainer-authorized:** Staging 0.83.0 commit
`7b40053b7ac580843d0461eba8c36a47a990e66c`, intelligent MPU, 80000 cycles,
empty midiconfig, raw MIDI output on. Its MPU reset schedules a 14-ms
completion event; command pending and response queue determine ready/ACK,
not synth/audio completion (`src/hardware/audio/mpu401.cpp`). The healthy
D0..D7/DF path supplies FE; no external input or output-device poll is needed.
`src/hardware/port.cpp:64..87` charges reads/writes nominally 1/0.75 us,
quantized here to **0.975/0.725 us**. A bounded virtual transport can charge
those operations, retain the original polling counters and pending-command
deadline, and account for opcode B's additional reads on its **renderer
transport timeline**. These are DOSBox reference timestamps, not a physical
MPU measurement or a CPU emulator.

**Maintainer timing approximation, 2026-10-27:** the charge is render-only.
It does **not** advance, rebase or add elapsed time to the game's presentation
clock. The logical upload barrier is **ordering-only**: enqueue all init
messages before later notes; the renderer preserves that order and its byte
deadlines. No game wait tests the last F7's render timestamp, the virtual
reset deadline or audio completion. Logical ticks, fades, VOC/CD deadlines
and missing-ROM behavior still use the caller-supplied presentation time.
DOS actually busy-waits on MPU polling while the BIOS clock continues;
omitting this game-visible cost is an approved approximation, not DOS static
equivalence. The captured upload span is about 3 ms, below a 13.736-ms driver
tick, but does not bound the uncaptured total init/MPU-reset interval.
The reference's 14-ms reset event, if represented, belongs only to renderer
transport timestamps; it is not an additional game delay.

This matches the existing OPL boundary: DOS helper I/O delays space chip
writes, and `XeenOpl3::write` uses buffered writes, but neither mutates
`XeenAudio::advanceClock`'s supplied time. There is no asymmetric MIDI-only
addition to game-visible time. Installed 3/10 does not use rolmus transport
and retains its timing, output, gains and traces **byte for byte**. See the
[known approximation](../original-quirks.md#dev-audio-io-cost-clock).
Unexpected traffic, a renderer profile that fails the endpoint tolerance,
or any independently required game-visible wait without a bound remains a
stop; this policy does not invent a finite hardware-failure timeout.

Two captured initializations each contain the same 13 packets with correct
checksums, at MIDI times 0..3 ms and 25301..25304 ms. A healthy I/O-only
schedule predicts packet ends through 2.98705 ms after the first CC; all
13 endpoints fit their integer-ms bins with first-event phase
0.52030..0.87025 ms, in both groups. This bounds observed serialization
at the capture resolution; it does not expose init entry/return or prove
the omitted instruction cost is zero. Part B must retain this as a bounded
reference profile and stop if fuller bookkeeping cannot meet the same bins.
The separate physical-wire occupancy, 1688*10/31250 = 0.540160 s, is **not**
a DOS init wait. Staging/mt32emu immediate delivery does not emulate that
wire. Do not add either that delay or the pin's eight-bit MIDI delay again.
The synth survives logical EXE/DAT handoffs; repeat the driver's shutdown
and initialization traffic without inserting a fresh synth/factory reset.

Use explicit **MT-32 1.07** (`mt32_107`) and the verified player's PCM ROM
for comparison; protocol model 16 alone does not distinguish old/new/CM ROMs.
Staging auto would choose the available CM-32L 1.02. ROM choice and current
capture path are reference settings; MMModern's ROM location remains the
maintainer's decision before Part B. The
[MT-32 comparison](../original-quirks.md#sv-rolmus-protocol) explains why
the pin's SoundDriverMT32 is not the DOS driver.

### Reference combinations

**DOS static, high:** chip protocol follows the selected music driver,
independently of digital id: 1/2/6 use one OPL2-compatible bank, 3 NEW-mode
OPL3, 4 MT-32 MIDI without OPL music/FX, nullmus no chip output but an active
timer. Mixer writes retain the port/flag consumers above; those writes
neither identify nor create a physical card.

**Maintainer reference-topology decision, 2026-10-27, not DOS evidence:**
the following matrix defines every M94 music/digital combination. Digital N
means 0; deferred 1/2/5/8/9; DAT 3/4; and every out-of-range id. Each cell
specifies the compatible reference mixer, not a physical-card deduction
from `XEEN.CFG`. Digital transfer/notice/caption rules remain separate.

| Music / renderer mode | Digital 6 | Digital 7 | Digital 10 | Digital N |
| --- | --- | --- | --- | --- |
| 1/2/6: OPL2 | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer |
| 3: NEW-mode OPL3 | SB16 mixer | SB16 mixer | Existing installed SB16 mixer | SB16 mixer |
| 4: MT-32, no OPL music/FX | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer |
| nullmus: no chip output | SB2, no Pro mixer | SB Pro mixer | SB16 mixer | No SB mixer |

Promus 3 keeps the installed SB16 mixer/CD calibration for **all** digital
ids, an explicit exception to digital 6/7/10's default SB2/SB Pro/SB16 rule.
Nullmus covers music 0 and out-of-range music. Deferred ids use the
[approved null-driver temporary restriction](../original-quirks.md#dev-m94-deferred-null),
including nullmus's mixer transaction, no deferred-driver busy/fade/load
waits and fixed BIOS rate. EXE digital 3/4/9 returns to title with its notice
instead of initializing EXE audio; the table still applies to DAT 3/4/9.
The specmus/SB Pro 2 take is a topology control, not its digital 6 default.

For non-promus rows the digital reference mixer, when present, is at the
CFG digital port; nullmus still writes its CFG **music** port, so its init
affects that mixer only when the addresses match. Promus uses its music-port
SB16 reference. Keep the actual mixer flag and write addresses, including
the absence of init writes when the flag is zero; do not fabricate a common
init for all drivers. Rolmus's synth channel never receives SB FM gain.

**DOSBox reference, not physical-card evidence:** Staging 0.83.0, commit
`7b40053b7ac580843d0461eba8c36a47a990e66c`,
[soundblaster.cpp](https://github.com/dosbox-staging/dosbox-staging/blob/7b40053b7ac580843d0461eba8c36a47a990e66c/src/hardware/audio/soundblaster.cpp#L2400),
lines 2400..2483, 2552..2565, 2652..2669 and 3652..3656:

- SB1/SB2 skip installation of mixer index/data ports base+4/+5. The
  original init writes have no effect: master, FM, DAC and CD card gains
  stay unity. Do not run the unreachable SB2 register handlers as a mixer.
- SB Pro reset sets master/FM/DAC/CD amounts to 31 (unity). Registers
  22/26/04 write master/FM/DAC; each channel nibble n maps to `(n<<1)|3`.
  Register 2e stores line-in without applying it to these output channels.
  For amount a, let c=31-a: c=0 is unity; otherwise attenuation dB is c-1
  for c<16, c for c=16, c+1 for c>16, with another +2 at c=24; c>27 mutes.
  Linear gain is `10^(-dB/20)`. Both Pro models use this mapping.
- The ordinary original init 2e/22/26=ff, 04=99 therefore leaves SB Pro
  master/FM/CD at 1.0 and DAC at **-11 dB**, approximately **0.281838**.
  If the flag is zero or the addressed port misses the reference mixer,
  reset gains stay unity. Original wave/tap's 04=cc would give Pro DAC
  -3 dB (approximately 0.707946); M94's deferred stopgap instead runs the
  ordinary nullmus transaction, including its music-port addressing.
- SB16 keeps its existing mapping/calibration: ordinary init master/FM/CD
  unity, DAC -24 dB. `ctmixer_update_volumes` applies master times source
  gain to DAC, OPL and CD; MT-32 is a separate channel. A1 tests
  each SB2/SB Pro/SB16 reference, covering reset, ignored ports,
  flag/address gating, init gains and register updates.

The original game writes master=ff and no CD register (**DOS static**, audit
above); these reset/init transactions leave CD's card multiplier at 1.0.
**Maintainer decision, 2026-10-27:** with no compatible mixer, use multiplier
**1.0** as well: no card attenuation, not full-scale source playback.
Existing per-source bases remain **g=1/3 for VOC/CD and 1.5*g=0.5 for FM**;
output processing/clamping stays unchanged. Speech/CD selection and waits
remain independent of digital id. Installed 3/10's existing code path,
calibration, sample output and logical timing remain byte-for-byte unchanged.

### Capture oracle, topology and limits

The four takes contain 6006/12689/4277 retained one-bank DRO writes for
admus/blastmus/specmus and 5489 MIDI events for rolmus. Align each compared
window (song start, stable title, outdoor key-on) from its matching stream;
the measured stable title and outdoor key-on windows fit the original IRQ
grid within **1 ms** inside each window. Foreground command writes compare
by order only. One origin per program is intended, **not demonstrated** by
these captures.
Blast/spec share a contiguous 418-write title subsequence.

For OPL, require exact ordered register/value writes and logical tick indices
in the recording sink. Against DRO compare its projection: mapped changed
writes, synthetic first-note cache separated, timestamps within 1 ms after
matching-window alignment. DRO does not record init order, duplicates or sub-ms bus
spacing. Mono helpers (`admus 0x407..0x41e`, blast/spec `0x405..0x41c`)
retain their I/O delays; do not collapse key-off/on into one chip sample.
Their three address/data delay reads plus twenty tail reads and two writes
give a nominal 23.875-us virtual-I/O charge in this reference, before
instruction overhead/slice clamping. Stable title bins retain 10/12/12
writes in one millisecond: average retained spacing is below
111.2/91.0/91.0 us respectively. These are aggregate bounds, not a measured
minimum or maximum for every write. Sub-ms spacing needs the static helper
and a render check, not a fabricated per-write capture tolerance.
Mono OPL profiles reuse `XeenOpl3`'s **two-native-sample buffered writes**,
the same approximation as [promus write spacing](../original-quirks.md#promus-write-spacing).
Installed promus's spacing and existing effect-energy acceptance remain unchanged; new-family waveform
equivalence needs its own Part A comparison, not a universal PCM hash.

For MT-32 require exact bytes, checksums and message order, logical
fade carries/early stops, transport deadlines and init/load barriers.
SMF timestamps complete messages in integer milliseconds, first message
zero; allow **1 ms** after origin/phase alignment for captured endpoints.
Use the matching window's phase.
It omits MPU status/ACK, byte-level wire spacing and init-entry time.
Waveform/listening compares the same ROM pair separately from transport.
No new waveform-equivalence tolerance is certified by these data.

For PCM require exact transfer count and logical busy deadline in unit
tests, separately from output samples. Crodo1/2 give N=58555/55808 and
3.981740/3.794944 s for blastsnd. Matched next-slot onsets leave 1.760 and
13.056 ms beyond those nominal deadlines, compatible with one 70.086-Hz
publication plus 1-ms timestamps and 0.5-ms matching resolution: **15.8 ms**
black-box tolerance. That coarse observation cannot distinguish four samples
(272 us) from prosnd or validate allocation-specific DMA splits.
New DRO/SMF captures stay outside the repository under MModern-re and are
passed to tests by path, as `PromusDriverTests` does; never commit captures.

The [reference-combination matrix](#reference-combinations) owns card and
CD-gain selection; captures do not establish unrecorded physical wiring.

Gunther's CD cue is captured with admus and blastmus, including digital 0;
it is absent from specmus/rolmus. Thus simultaneous Roland/CD balance and
SB Pro CD gain are not newly listening-calibrated. The source topology is
established; those waveform checks remain acceptance work before claiming
an audible match. Captured actions are not an exhaustive effects census.

## Missing configuration and remaining evidence

**DOS static, high:** missing `XEEN.CFG` prints configuration instructions
and exits (EXE `0x11daf..0x11e1a`; DAT `0x3d320..0x3d33b`). MMModern
likewise refuses to start with a clear message (maintainer, 2026-10-21) and
never writes the file.

**DOSBox DRO captures (2026-10-21):** the maintainer recorded six promus
captures in classic DOSBox 0.74-2 (`oplmode=opl3`) in
`D:/Projetos/MModern/MModern-re/audio-m82/captures/`: startup to the title menu, the Tavern
song (capture `promus-inn`), town to outdoors with the fade, effect 20
alone, wall bump alone, and effect 20 over music; durations are approximate. A first check agrees with the
static reading: the startup stream sets second-bank register 5 to 1 before
any note, and channel registers C0..C8 carry routing `0x2_` in the first bank
and `0x1_` in the second. DOSBox labels the header hardware type 1 (dual
OPL2); the register stream, not that label, is the evidence. DRO delays need
alignment to the PIT grid, not one write per tick. `xeen_promus` compares
these captures' register writes with MMModern's driver, tick by tick. The
effect-site/owner reconciliation follows below; the Dwarf Effects-off video
[above](#efx-voc-waits-and-entrance-captions) settles the Dwarf caption lifetime.

## DOS FM effect call sites

**DOS static, bounded inventory:** 133 decoded direct far calls target the
Efx wrapper at DAT `0xdece`. Its call at `0xdedb` is the only direct call
found to FM effect trampoline `0x4617b`. No push-CS/near-call bypass was
found. An equivalent bounded scan of XEEN.EXE found no references to these
DAT wrapper/trampoline addresses; that is not a full analysis of its own
intro drivers or indirect dispatch. EXE SHA-256:
`c5ade865b4d1f63201ce3a421df5182cf1d2923f3343deb82a5a06be22eea235`.

All effect IDs below are decimal. Expressions denote runtime IDs. The Efx
Off panel branch's ID 0 is gated out. The function column identifies the
existing action owner, including shared helpers reached by spells, Events or
ordinary actions. C1 has 46 non-spell or shared gameplay sites; C2 has 86
casting/spell-handler sites. One site is not reachable yet: the Darkside smith
animation. M86 wires the two terrain fall cues and the full opening-door
redraw; M87 wires the Inn's full-party refusal and M88 character creation's
unrecognized-key refusal. No scanned call was discarded as a non-effect. Confidence refers
to the stated original action and argument paths, not implementation coverage.

**DOS static, high (C1 reconciliation, 2026-10-21):** incoming branches matter:
`0xcf6a/0xcf6c` feed effect 46 to call `0xcf48`, bypassing the push of 21 at
`0xcf46`. The maintainer's DOSBox `promus-wall-bump` capture also matches
46 ([capture configuration](../dependencies.md)). Likewise,
`0x270e4/0x270e6` feed 45 to `0x270ea`, and the secondary-attack call at
`0x2d6af` merges several different pushes. The wrapper at `0xdece..0xdee6`
was calibrated against the saved Efx byte `DS:ed9a` (party +0x0e); the original
DAT hash remains the evidence baseline. Monster misses at `0x2dc0b` are separate
from the player attack2 zero-total cue at `0x2c17c`. Missed player missiles
(`0x2cb86 -> 0x2ccc4`) bypass attack2; melee still submits its zero total.
The projectile loop checks the next wall (`0x1db3e..0x1db84`) before testing
whether any arrows remain. Single/Group spells stop before that check after
dispatching a target; All spells continue. Thus a spent last arrow can still
request 46 at the immediately following wall, but cannot reach a farther wall.
 Named spell handlers were traced from
the original 76-entry dispatcher table at `0x3382b`, with the M74-M77 handler
evidence; item-target helpers were decoded separately. External semantic notes
and bounded instruction evidence stay under `D:/Projetos/MModern/MModern-re/audio-m82/c1/` and
`c1-spell-inventory-86.tsv`; no original code or bytes are kept here.

Spell requests use the live casting, item and travel owners; pure speculative
`prepareEffect`, `applyPartyEffect`, `information` and `travel` calls do not emit
audio. Launch cues follow the retained scalar/type preparation and precede the
projectile. The item-only Frostbite/Incinerate selectors remain distinct.
Awaken and information cues wait for the existing frame handoff. Member cues
precede healing presentation; Power Cure retains its one cue across RNG yields.
Moon Ray uses the existing portrait sequence for each member's 30, then shared
20 and sparkle. **Existing preparation gap:** MMModern batches Moon Ray's
healing draws and publication before those portraits; DOS interleaves a draw
and healing presentation per member. C2 keeps that preparation unchanged
(maintainer, 2026-10-22); this is not approval of the gap.


| DAT file offset | Effect id / expression | Existing MMModern function | DOS action | Class | Confidence / evidence |
| --- | --- | --- | --- | --- | --- |
| `0x0b2f1` | 33 | `XeenEventFlow::advanceDeath` | Clouds death laugh frames 1 and 11 | C1 | High: DAT frame branches 0xb2e5..0xb2f1; pinned death sequence |
| `0x0b308` | 34 | `XeenEventFlow::advanceDeath` | Clouds death laugh frame 15 | C1 | High: DAT 0xb301..0xb308 |
| `0x0c903` | 10 | `XeenEventFlow::serviceBarrier` | successful grate opening, after both wall faces change and before redraw | C1 | High: DAT 0xc794..0xc91a; grate evidence in evidence.md |
| `0x0cf48` | 21 from 0xcf46; 46 from 0xcf6a via 0xcf6c | `XeenMovement::apply; XeenEncounterFlow::advanceJourney` | outdoor admission refusal (21); indoor wall or swamp refusal without Swimming/Walk on Water (46) | C1 | High: DAT 0xcf46..0xcf91; DOSBox promus-wall-bump matches effect 46 |
| `0x0cfa9` | 47 | `XeenMovement::doorAhead; XeenEncounterFlow::advanceJourney` | admitted indoor forward move through a front door wall (types 4 and 13, the pin's `_wo[107]`), before the step suffix; the step waits for the opening-door redraw, held at least two BIOS ticks (M86) | C1 | High: DAT 0xcf93..0xcfb7; pinned checkMoveDirection |
| `0x0d442` | 7 + toggled stepping bit (7 or 8) | `XeenEncounterFlow::advanceJourney; XeenEventFlow::serviceBarrier` | movement/turn step-time suffix, including passable indoor B, excluding Space and W; toggle before request | C1 | High: DAT 0xd42b..0xd448; pinned stepTime |
| `0x0d5c1` | 21 | `XeenEncounterFlow::beginShoot` | Shoot refused because no active member can shoot | C1 | High: DAT 0xd558..0xd5c7; pinned perform(S) |
| `0x11692` | 11 | `XeenEventFlow::beginFall` | falling animation starts after view setup | C1 | High: DAT 0x11686..0x11697; retrace-paced fall |
| `0x11716` | 31 | `XeenEventFlow::advanceFall` | after sample stop/unnh, before four impact images | C1 | High: DAT 0x11706..0x1171b; retrace-paced landing |
| `0x1b1a1` | 28 | `XeenEncounterFlow::simpleService` | Drink result below 26 increments Drunk; redraw then burp | C1 | High: DAT 0x1b18f..0x1b1a6; pinned Tavern |
| `0x1c4b5` | 45 | `XeenEventFlow::pulseServiceArt` | Clouds smith background frames 3 and 9 | C1 | High: DAT 0x1c46e..0x1c4bb; pinned BaseLocation::drawAnim |
| `0x1c4d3` | 45 | `none; Darkside services` | Darkside smith background frames 4 and 13 | Not reachable yet: roadmap long-term 3 (Darkside) | High: DAT 0x1c4bd..0x1c4d8 |
| `0x1d918` | 49 | `XeenEncounterFlow::beginShoot` | arrow projectile start after at least one shooting lane is selected | C1 | High: DAT 0x1d8c0..0x1d91e; pinned rangedAttack(POW_ARROW) |
| `0x1d922` | 21 | `XeenEncounterFlow::beginShoot` | arrow projectile refused when no shooting lane can be selected | C1 | High: DAT 0x1d912..0x1d927; pinned rangedAttack(POW_ARROW) |
| `0x1db60` | 46 | `XeenEncounterFlow::serviceShoot` | projectile hits an outdoor blocking wall or indoor no-pass wall; shared with spell projectiles | C1 | High: DAT 0x1db3e..0x1db84; M74 projectile evidence |
| `0x1e8d2` | 21 | `XeenEventFlow::presentPendingEvent` | insufficient or absent carried/banked resource notice | C1 | High: DAT 0x1e8bb..0x1e940; DS 0x597a/0x599b |
| `0x1f073` | 20 on enabling Efx; off branch 0 is gated out | `XeenGameplay control-panel publishSound` | Efx toggle feedback | C1 | High: DAT 0x1f061..0x1f073; wrapper Efx gate |
| `0x1f249` | 51 | `XeenEncounterFlow::mrWizard` | Mr Wizard confirmed outside combat, before map transition | C1 | High: settled Mr Wizard entry in original-quirks.md |
| `0x1f881` | 38 | `xeenApplyPhysicalInjury` | HP subtraction leaves a member unconscious, before armor-break handling | C1 | High: DAT 0x1f812..0x1f887; pinned subtractHitPoints |
| `0x21107` | 20 | `XeenEventFlow::preparePortraitSequence` | eligible member portrait sparkle starts; shared by spells, Events and services | C1 | High: DAT 0x210bc..0x21169; existing portrait evidence |
| `0x236c6` | 10 | `XeenEventInterpreter::execute` | GiveMulti chest lock succeeds, before redraw | C1 | High: DAT local 0x235e7 and call 0x236c3..0x236ce |
| `0x236d8` | 21 | `XeenEventInterpreter::execute` | GiveMulti chest lock fails before failure notice | C1 | High: DAT 0x236d6..0x23726 |
| `0x23eb8` | Physical 29; Magical 27; Fire 22; Electrical 23; Cold 24; Poison 26; Energy 25; fallback 38 | `XeenTypedDamageCandidate::service` | typed party damage overlay after resistance saves, before injury | C1 | High: DAT 0x23e09..0x23edf |
| `0x240fe` | 51 | `XeenEventInterpreter::execute` | valid mirror-name input matched against original mirror list | C1 | High: DAT 0x23fa5..0x24127, 0x240d5..0x24103; pin StringInput differs |
| `0x2428e` | 54 | `xeenBeginTreasureDelivery` | currency delivery begins when gold or gems are nonzero, before item delivery | C1 | High: DAT 0x2426a..0x24294; pinned giveTreasure |
| `0x2456b` | 20 | `xeenDeliverRewardItem` | each delivered treasure item, before copy/message | C1 | High: DAT 0x24567..0x2464d; pinned giveTreasureToCharacter |
| `0x26ff3` | Event record parameter byte +6 | `XeenEventInterpreter::execute` | PlayFX Event opcode | C1 | High: DAT 0x26fe9..0x26ff8; pinned cmdPlayFX |
| `0x270a8` | 51 | `XeenEventInterpreter::execute` | mirror teleport only: after destination resolution, before map-change test | C1 | High: DAT mirror branch 0x27000..0x270b9; pinned cmdTeleport |
| `0x270ea` | 45 for selected object sprite 47; 44 for sprite 48 | `XeenEventInterpreter::execute` | teleport changes map through stair object, before load | C1 | High: DAT 0x270be..0x270f8; both incoming pushes 0x270e4/0x270e8 |
| `0x28f1c` | 21 | `XeenEventSystem::runManualEvent` | interaction object has no matching facing script and current cell is not an Event | C1 | High: DAT 0x28efd..0x28f21; pinned checkEvents refusal |
| `0x2c105` | 60 + byte at DAT 0x4d660[attack weapon ID] | `XeenEncounterFlow::serviceShoot; XeenCombat::servicePresented; XeenCombat::serviceCast` | positive monster damage after pre-HP redraw and before HP subtraction; ordinary spells retain the previous weapon selector | C1 | High: DAT 0x2c0f6..0x2c112; pinned attack2 |
| `0x2c17c` | 6 | `XeenPhysicalPlayerCandidate::service; XeenSpellDamageCandidate::service` | attack2 damage is nonpositive, including boss refusal; magic-resistance rejection bypasses this call | C1 | High: DAT attack2 zero-damage branches into 0x2c16c..0x2c181 |
| `0x2c23c` | 4 | `XeenEncounterFlow::serviceShoot; XeenCombat::servicePresented` | monster death, after boss-specific quest writes and before XP/treasure generation | C1 | High: DAT 0x2c1b0..0x2c247; pin omits cue |
| `0x2d379` | Physical 29; Magical 27; Fire 22; Electrical 23; Cold 24; Poison 26; Energy 25 | `XeenEnemyAttackCandidate::service` | monster damage overlay after saves, before Power Shield and secondary attack | C1 | High: DAT 0x2d2ab..0x2d3a1; pinned doCharDamage |
| `0x2d5f0` | 37 | `XeenEnemyAttackCandidate::service` | Eradicate secondary effect after breaking inventory, before zeroing positive HP | C1 | High: DAT 0x2d5c6..0x2d60e |
| `0x2d62e` | 37 | `XeenEnemyAttackCandidate::service` | Stone secondary effect before zeroing positive HP | C1 | High: DAT 0x2d611..0x2d63f; pin uses 38 |
| `0x2d65e` | 38 | `XeenEnemyAttackCandidate::service` | Death secondary effect before zeroing positive HP | C1 | High: DAT 0x2d641..0x2d66f |
| `0x2d6af` | 26 Poison/Disease; 28 Insane/Confuse; 36 Sleep/Weaken; 37 CurseItem/DrainSP/Curse/Paralyze/Unconscious/BreakWeapon/Aging | `XeenEnemyAttackCandidate::service` | shared tail of successful secondary-attack branches | C1 | High: DAT switch 0x2d43d, pushes 0x2d45d/0x2d4df/0x2d55c/0x2d6ad |
| `0x2dc0b` | 6 | `XeenEnemyAttackCandidate::service` | monster physical attack misses on a natural 1 or failed AC check, including the second check after a critical first hit | C1 | High: DAT 0x2d9ea..0x2dc11; monster +0x1d physical gate; hit calls party-damage helper 0x2d22d; pin doMonsterTurn |
| `0x2e1fe` | 51 | `XeenRunCandidate::service` | successful combat Run removes current participant | C1 | High: DAT 0x2e1c0..0x2e204; pinned run |
| `0x2e7e6` | monster-record byte +0x32 | `XeenEncounterFlow::observeCombat` | monster melee attack starts before frame 8 redraw | C1 | High: DAT 0x2e7df..0x2e7eb; pinned doCombat |
| `0x2ec42` | 51 | `XeenEncounterFlow::mrWizardAfterCombat` | Mr Wizard combat-exit tail after caller 4, before destination load | C1 | High: settled Mr Wizard entry in original-quirks.md |
| `0x2f1d3` | 21 | `XeenCombat::respondCast/settleCast; XeenEncounterFlow::respondCombatCast; XeenEventFlow::beginTravelDialog` | casting refused while engaged, including Quick Fight and the continuing Town Portal item notice | C2 | High: DAT 0x2f1b6..0x2f23e; message DS 0x6887; Portal item branch 0x3a058..0x3a075 |
| `0x2f25c` | 21 | `XeenEventFlow::handleCasting; XeenCombat::respondCast; XeenEncounterFlow::respondCombatCast` | casting refused for insufficient SP or gems, including Quick Fight | C2 | High: DAT 0x2f23f..0x2f2c3; message DS 0x68aa |
| `0x2f3ac` | 39 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Light | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f3ee` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Awaken, after the party redraw | C2 | High: DAT 0x2f3b3..0x2f3f4; party redraw at 0x2f3e6 precedes cue; emitted by framePresented |
| `0x2f44a` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | First Aid | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f476` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Flying Fist | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f496` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Revitalize | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f53a` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Cure Wounds | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f575` | 14 | `XeenSpellDamagePreparation::requestFmEffect` | Sparks | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f5b7` | 16 | `XeenSpellDamagePreparation::requestFmEffect` | Energy Blast, after one U(2,6) draw multiplied by caster level, before projectile dispatch | C2 | High: DAT 0x2f584..0x2f5c5; RNG call 0x2f588, level call 0x2f599, multiplication 0x2f5a4; pinned ScummVM instead uses fixed 2L without this draw; MMModern follows DOS preparation |
| `0x2f5da` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Sleep | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f5fd` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Pain | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f620` | 17 | `XeenSpellDamagePreparation::requestFmEffect` | Toxic Cloud | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f68c` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Suppress Poison | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f6fb` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Protection from Elements, including selector cancellation | C2 | High: DAT 0x2f6cd..0x2f709; selector dispatch 0x2f6d4..0x2f6de rejoins cue tail before party redraw |
| `0x2f72e` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Turn Undead | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f825` | 51 | `XeenEncounterFlow::publishAwaken/settleItemUse` | Jump | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f895` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Suppress Disease | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f8b7` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Bless | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f8d8` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Levitate | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2f8eb` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Wizard Eye | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fa69` | 20 | `XeenEncounterFlow::publishAwaken/beginItemUse; XeenCombat::settleCast; XeenEventFlow::framePresented` | Identify Monster | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2faf6` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Holy Bonus | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fb65` | 30 | `XeenLearnedSpellRules::preparePowerCure; XeenEncounterFlow::publishCastingTarget; XeenCombat::settleCast` | Power Cure, after target/terminal admission and before U(2,12) | C2 | High: DAT 0x2fb10..0x2fb9d; guard 0x2fb52..0x2fb61; cue 0x2fb65 precedes RNG 0x2fb6f |
| `0x2fbf3` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Nature's Cure | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fc3e` | 14 | `XeenSpellDamagePreparation::requestFmEffect` | Lightning Bolt | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fc4f` | 30 | `XeenEncounterFlow::publishAwaken/settleItemUse; XeenCombat::settleCast` | Heroism | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fc70` | 20 | `XeenEncounterFlow::publishAwaken/settleItemUse; XeenCombat::settleCast` | Walk on Water | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fc8b` | 15 | `XeenSpellDamagePreparation::requestFmEffect` | Item power 33 Frostbite | C2 | High: M74 item-power range; local handler branch; not learned-spell dispatch |
| `0x2fee7` | 51 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse` | Lloyd's Beacon Return, after admission and before relocation | C2 | High: DAT 0x2feaa..0x2ff61; admitted branches reach the push of 51 at 0x2fee5; castle-completion refusal at 0x2fede..0x2fee2 bypasses the call |
| `0x2ff65` | 20 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse` | Lloyd's Beacon | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2ffc5` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Power Shield | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x2fff0` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Cure Poison | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30072` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Fireball | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x301ee` | 20 | `XeenEncounterFlow::publishAwaken/beginItemUse; XeenCombat::settleCast; XeenEventFlow::framePresented` | Detect Monster | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30250` | 17 | `XeenSpellDamagePreparation::requestFmEffect` | Acid Spray | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30292` | 15 | `XeenSpellDamagePreparation::requestFmEffect` | Cold Ray | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x302b2` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Cure Disease | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30316` | 51 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse; XeenCombat::settleCast` | Time Distortion | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30339` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Deadly Swarm | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30557` | 51 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse` | Teleport | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3057c` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Finger of Death | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3059c` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Cure Paralysis | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3060c` | 30 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse` | Super Shelter | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30633` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Fiery Flail | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3064d` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Create Food | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3076e` | 51 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse; XeenCombat::settleCast` | Town Portal | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30836` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Stone to Flesh | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x308ee` | 15 | `XeenSpellDamagePreparation::requestFmEffect` | Fantastic Freeze | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x3095d` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Raise Dead | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30a01` | 51 | `XeenEncounterFlow::confirmCasting/beginItemUse/publishAwaken/settleItemUse` | Etherealize | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30a43` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Dancing Sword | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30a6f` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Prismatic Light | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30a93` | 16 | `XeenSpellDamagePreparation::requestFmEffect` | Moon Ray | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30aa7` | 30 | `XeenEncounterFlow::serviceShoot; XeenCombat::settleCast; XeenEventFlow::prepareStandalonePortrait` | Moon Ray, effect 30 per party member before that member's healing sparkle | C2 | High: DAT 0x30aa1..0x30adf; each iteration cues 30, draws U(1,30), calls healing; healing 0x2ee3a..0x2eed6 reaches sparkle 0x21087..0x21185 |
| `0x30af4` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Mass Distortion | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30b6c` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Item power 64 Incinerate | C2 | High: M74 item-power range; local handler branch; not learned-spell dispatch |
| `0x30baa` | stormFx[rand(2..5)-2] = {13,14,15,17} | `XeenSpellDamagePreparation::requestFmEffect` | Elemental Storm | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30bdf` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Holy Word | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30c38` | 30 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Resurrection | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30cb4` | 14 | `XeenSpellDamagePreparation::requestFmEffect` | Mega Volts | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30cd7` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Inferno | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30cfa` | 16 | `XeenSpellDamagePreparation::requestFmEffect` | Sun Ray | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30d1d` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Implosion | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30d40` | 13 | `XeenSpellDamagePreparation::requestFmEffect` | Star Burst | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30de2` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Divine Intervention | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30e07` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Beast Master | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30e1d` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Clairvoyance | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30e53` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Day of Protection | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30e82` | 20 | `XeenEncounterFlow::publishAwaken/publishCastingTarget/settleItemUse; XeenCombat::settleCast` | Day of Sorcery | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30e9d` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Dragon Sleep | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30edb` | 16 | `XeenSpellDamagePreparation::requestFmEffect` | Golem Stopper | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30efe` | 18 | `XeenSpellDamagePreparation::requestFmEffect` | Hypnotize | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30f21` | 22 | `XeenSpellDamagePreparation::requestFmEffect` | Incinerate | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30f44` | 17 | `XeenSpellDamagePreparation::requestFmEffect` | Insect Spray | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x30fd7` | 49 | `XeenSpellDamagePreparation::requestFmEffect` | Poison Volley | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x31009` | 16 | `XeenSpellDamagePreparation::requestFmEffect` | Shrapmetal | C2 | High: Learned dispatcher 0x3382b maps ID to handler; M74/M76 entry labels; local call predecessor/branch |
| `0x31bfd` | 20 | `XeenItemSpellPreparation::service` | Recharge Item success only: after eligibility and charge draw, before charge mutation | C2 | High: DAT eligibility branches 0x31b76..0x31bea precede RNG 0x31bf0 and cue 0x31bfd; charge mutation 0x31c0d..0x31c3a |
| `0x31c44` | 21 | `XeenItemSpellPreparation::service` | Recharge Item refusal only; no effect 20 or charge draw on this path | C2 | High: DAT eligibility failures reach 0x31c42 directly, bypassing 0x31bec..0x31c3f |
| `0x31d3d` | 19 | `XeenItemSpellPreparation::service` | Enchant Item | C2 | High: M76 item handler ranges; local code/data flow and category branch |
| `0x31dc3` | 21 | `XeenItemSpellPreparation::service` | Enchant Item | C2 | High: M76 item handler ranges; local code/data flow and category branch |
| `0x31e1e` | 21 | `XeenItemSpellPreparation::service` | Item to Gold | C2 | High: M76 item handler ranges; local code/data flow and category branch; frame weapon offset 0xa7 |
| `0x31e4c` | 20 | `XeenItemSpellPreparation::service` | Item to Gold | C2 | High: M76 item handler ranges; local code/data flow and category branch; weapon offset 0xa7 |
| `0x31ea9` | 20 | `XeenItemSpellPreparation::service` | Item to Gold | C2 | High: M76 item handler ranges; local code/data flow and category branch; armor offset 0xcb |
| `0x31f07` | 20 | `XeenItemSpellPreparation::service` | Item to Gold | C2 | High: M76 item handler ranges; local code/data flow and category branch; accessory offset 0xef |
| `0x31f61` | 20 | `XeenItemSpellPreparation::service` | Item to Gold | C2 | High: M76 item handler ranges; local code/data flow and category branch; miscellaneous offset 0x112 |
| `0x32d5b` | 31 | `XeenBarrierCandidate::XeenBarrierCandidate` | blocked-wall indoor Bash after the first charge; passable B takes the step suffix; outdoor Bash is silent | C1 | High: DAT 0xd516..0xd547; passable B jumps at 0xd534 to 0xd3aa; blocked path reaches 0x32d4f..0x32d60 |
| `0x34d71` | 21 | `XeenEncounterFlow::buySpell` | Guild Buy spell cannot subtract required resource | C1 | High: DAT 0x34cd3..0x34cea; pin SpellsDialog purchase |
| `0x368c3` | 21 | `XeenEventFlow::handleCreate` | unrecognized key in character creation after failing 26-key dispatch lookup | C1 | High: DAT 0x362a5..0x362bb; create.raw at DS 0x75c2 |
| `0x36a07` | attack type 0..6 maps to 49,18,13,14,15,17,16 | `XeenEncounterFlow::observeRanged` | each original shooting-queue entry requests type sound before projectile animation | C1 | High: DAT 0x3694f..0x36a0c; MON +0x1d and table 0x50017 |
| `0x39bda` | 21 | `XeenEventFlow::handleInn` | recruiting refused because party already contains six members | C1 | High: DAT 0x39b76..0x39beb, party-full message DS 0x7cd1 |

The pin inventory contains about 145 gameplay calls plus cutscenes, rather
than 133 corresponding source lines. Its 73 spell-file calls correspond
broadly to DOS's 86-call spell/effect band at `0x2f000..0x31fff`; the pin's
33 combat calls also cover shared typed-damage helpers outside that band.
Identified correspondences include step toggle, falls, barrier refusal,
Event PlayFX, typed damage, weapon-power effects, monster-record effects,
monster shooting, Efx-on, Mr Wizard, Tavern Drink, Guild purchase and death.

The physical wrapper count does not equal the pin's source-line count:
several original branches share one call, while helpers have multiple callers.
C1/C2 classify action semantics, not an address band or the active mode. In
particular, common injury, sparkle and projectile helpers remain C1 because
ordinary non-spell actions reach them too; C2 must reuse those helpers.

## CD and Text presentation gaps

These are existing presentation gaps, excluded from M83 under the
maintainer's 2026-10-22 rule; they do not gate implemented Clouds dialogs.

| Presentation | Evidence and current boundary |
| --- | --- |
| Auxiliary F10 | Explicit stop at `0x1be38`, resume command 0x88 at `0x1bf3d -> 0x3d984`. MMModern has no corresponding action or presentation; it does not synthesize this pause/resume sequence. F8 replay is also omitted by maintainer decision. |
| Numeric Choose123 | Text readers `0x24cb9`, `0x24ccd`, `0x24cfb`, `0x24d0f` position controls and restore the retained NPC. The numeric-choice Event opcode 48 is outside `XeenEventDecoder`'s supported subset; its helper is not implemented. Arena number entry is a different presentation. |
| Darkside map-29 exception | `0x26a7d`, `0x26a83`, `0x291d8` temporarily clear/restore Text off. Darkside is outside this milestone. |

The implemented YesNo helper retains Text-off control placement; ordinary
NPC modes 0/1 and passive mode 2 use their distinct polling paths. Ordinary
Event messages and service greetings have no NPC body gate. Intros and
endings remain outside this milestone; resource-read stop/restart
cycles are deliberately omitted everywhere, not left as implementation gaps.

**Map 49 play-test fixture limitation (maintainer, 2026-10-22):** the
(8,17) cue is not reachable with the play-test fixture. No further route
investigation is required; the original-data automated cross-file test
remains the witness for its track-2 tail followed by track 3.
