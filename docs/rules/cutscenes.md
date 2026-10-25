# Cutscenes

These rules describe the installed English World of Xeen CD programs.
For playable coverage, see [project status](../project-status.md).
Offsets are file offsets unless explicitly marked DS; they identify this build.

## Evidence and calibration

**DOS static, high within the stated routines; checked 2026-10-25.**
`WORLD/XEEN.EXE`: 137,737 bytes, SHA-256
`c5ade865b4d1f63201ce3a421df5182cf1d2923f3343deb82a5a06be22eea235`.
`WORLD/XEEN.DAT`: 353,511 bytes, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
Capstone 5.0.7 decoded x86 in 16-bit mode. EXE mapping uses its `0x2e30`-byte
MZ header and data-segment file base `0x1d880`. Calibration compares arguments,
instructions and control flow with the settled DAT routines after relocation.

| Primitive | EXE | DAT counterpart | Matched contract |
| --- | --- | --- | --- |
| CC index | `0x1d15f..0x1d16f` | `0x45e1b..0x45e2b` | Two-bit rotate and evolving additive key |
| Resource lookup | `0x1d1f4..0x1d338` | `0x45eb0..0x460b6` | Loose file first, name hash, eight-byte index entry |
| Payload decode | `0x1d3ec..0x1d410` | `0x460be..0x460dc` | Resource payload XOR |
| Sprite entry | `0x14fd6..0x15035` | `0x40352..0x403b1` | Resource/frame, position, scale, flags and window arguments |
| Palette fade | `0x14a69..0x14adb` | `0x3fe0f..0x3fe81` | In/out, 128-level scale, retrace before write |
| Retrace | `0x18c5a..0x18c6f` | `0x43fca..0x43fdf` | VGA status-port wait |
| BIOS counters | `0x18d8f..0x18e06` | `0x440cd..0x44144` | Three reset/read pairs using INT 1Ah |
| Song command | `0x1d48b` | `0x46171` | promus entry 6; buffer start and integer commands |
| VOC start/status | `0x11b42..0x11b5f`, `0x1d49f` | `0xdea9..0xdecd`, `0x46185` | prosnd entry 3; start, busy and stop |
| Keyboard | `0x1a0f8..0x1a217` | `0x45436..0x45555` | Key-state scan, repeat and key/modifier result |
| Scroll | `0x6f22..0x7223` | `0xad07..0xb02b` | Same hand/marble geometry and terminal image; different input gate |

The sprite comparison establishes the entry contract, not an exhaustive decoder
comparison. Clouds starts at `0x7ca6`: `mm4theme.m` is referenced at `0x7d16`
(name `0x1f677`); the voice loop uses table `0x1f540`, beginning with
`crodo1.voc` (name `0x1f5bd`), through `0x8503..0x852b` and start `0x8597`.
Full sequence, extracted tables, scripts, aligned dumps and pin comparison:
`D:/MModern-re/m91/findings.md`. Original-derived material stays outside the repo.

## Startup dispatch

**DOS static, high.** Successful startup reads inherited `XEENCOMMAND`
(`0x7a45..0x7a49`). Missing values become `(null)` in the formatter
(`0x3fa3..0x3fb2`); the comparator and table (`0x7a63..0x7a75`, `0x7c70`)
route that default to Darkside (`0x7a99` → `0xa9bd`). `XEENINTRO` routes to
Clouds; `DARKINTRO` also takes the Darkside path.

Entering Darkside on every launch does **not** mean playing the full story each
time. The EXE reads `XEEN.CFG` (`0x11dcb..0x11e1a`); its word at offset `0x1a`
selects full story when zero and NWC/JVC followed by the World-logo tail when
nonzero (`0xad3d..0xad64`). Completion sets it to one and writes the configuration
(`0xd345..0xd374`). Escape before this store leaves it unchanged. DAT Other
Options `D` clears it and writes the configuration before handing off
(`0xc571..0xc59e`). Thus DOS has persistent seen-intro behavior; the pin's
`seen_intro` is not grounds for claiming that DOS always plays the full story.

Startup and the Darkside intro are scheduled after the Clouds ending
([roadmap step 20](../roadmap.md#next-arc-finishing-the-games-systems);
maintainer, 2026-10-25).
This rule does not authorize writing the original configuration from MMModern.

## Hand-off and title return

**DOS static, high.** Other Options `C` sets `XEENCOMMAND=XEENINTRO`
(`0xc5a6..0xc5b1`, environment helper `0xc5e7`); `D` sets `DARKINTRO`
(`0xc565..0xc56e`). The environment carries the command between programs.
DAT's title exit (`0xc66b..0xc75d`) requests song fade 207, restores button
context, redraws title frame 7, clears input, calls the scroll helper, stops and
frees the song, then preloads `twnnitea.m`. The Darkside scroll gate makes that
scroll call return immediately in World title context. Main fades out by four,
waits for digital idle and shuts down the drivers/video before the runtime call
to `xeen.exe` (`0xb83e..0xb8f9`).

EXE completion installs `XEENCOMMAND=MAINGAME` (`0x7b9c..0x7ba5`) and invokes
`XEEN.DAT` (`0x7c39..0x7c4e`). The fresh DAT loads DARK.CC's `newbrigh.m`
(`0xb676..0xb68e`), starts it through the Music-gated wrapper (`0xbcfa..0xbd03`),
resets title/menu state (`0xbcf3..0xbcf8`) and fades the World title in by four
(`0xbe9a..0xbea9`). Menu input is awaited at `0xbee7..0xc0e8`; Other Options
is not resumed. DOSBox play shows about half a second of black between the
closed parchment and the title fade, with no text-mode flash
([M91 runtime checks](evidence.md#m91-clouds-intro-runtime-checks)); sound
cutoff latency is not measured.

## Timing and resource boundaries

**DOS static, high.** Waits subtract captured integer BIOS ticks, nominally
18.2065 Hz, preserving reset phase. Fades and frame publication wait for VGA
retrace; full/partial flips reach `0x19040`/`0x19322` → `0x18c5a`. Fade steps
128, 8, 4 and 2 write two, 17, 33 and 65 palette levels respectively.
**Implementation inference, high:** these independent counters fit the existing
`PresentationClock` BIOS and 70 Hz retrace conversions; no new timer is required.

The loader tries a loose file (`0x1d216..0x1d21b`), then the selected archive.
Opening another archive (`0x1d0e2`) replaces that resource slot; it does not
create an INTRO/XEEN/DARK fallback chain. Clouds switches to XEEN.CC at
`0x7cce..0x7cd9`; all subsequent scene, scroll, song and caption resources use
it. Startup support includes INTRO.CC's `pyramid1.raw` and `ido2.voc`, and
DARK.CC's opaque `46k.buf` (`0x7a0a`, `0x7a29`, `0x7cbe`); `ido2.voc` is an
allocation overwritten before playback, not a Clouds voice cue.

Clouds uses indexed RAW backgrounds, PAL palettes, Xeen sprites, fixed-format
Xeen VOC samples, promus songs and a NUL-delimited caption record. Music device
four/five selects `bigtheme.m`/`cantheme.m`; other choices select `mm4theme.m`
(`0x7ce9..0x7d23`). All Clouds speech uses VOC buffers, not CD track requests.

**Source-equivalent audit, medium; not compiled-reader execution.** The current
reader algorithms accepted 35 scene/scroll sprite resources (325 frames), 14
scene VOCs plus scratch `ido2.voc`, three scene RAWs, three palettes and the song
variants without errors. Startup RAW size and opaque buffer extent also pass.
No missing Clouds reader was identified. External reports under
`D:/MModern-re/m91/resources/` record archive offsets, sizes and decoder limits.
The compiled `XeenCutsceneTests` audit decodes all 35 sprites (325 frames),
scene/startup VOCs, RAWs and palettes and runs the complete song/scene with real
sample lengths on a fake clock. It also checks input, load/reset boundaries,
pan stride and the synthetic CFG caption path. **Unknown:** hardware-rendered
and audible equivalence; these still need the maintainer comparison.

## Clouds sequencing rules

**DOS static, high.** The full parameter sequence is external; these rules define
what the presenter must preserve, rather than transcribing the program.

| Stage | Rule | EXE evidence |
| --- | --- | --- |
| NWC logo | 80 frames, two elapsed BIOS ticks each; FM cues and fire/growl samples overlap animation; final 30-tick hold has no inner key poll | `0x7d2c..0x7e96` |
| JVC | Start selected song after logo; fade out eight/in four; 60-tick minimum includes subsequent resource loading | `0x7eb4..0x7fbb` |
| Stars and lake descent | Two vertical page merges; first measured elapsed value, clamped to at least one, sets the pixel stride; lake descent includes offset 200 at stride one | `0x7fc1..0x8219` |
| Lake and tower | Lake flight runs for 100 elapsed ticks, not 100 frames; each of 16 tower scales may redraw until a tick elapses; wizard shows frames 0–38, two ticks reset after each flip | `0x821e..0x840f` |
| Scroll/tableau | Same DAT hand/marble geometry; opening fade two and one-tick step holds; labels come from EXE text, not narration | `0x8410..0x84fe`, helper `0x6f22..0x7223` |
| Narration | Fourteen VOC slots; preloaded consecutive Crodo samples; animation holds on digital busy or caption completion; no uniform per-frame BIOS hold | `0x8503..0x88c8` |
| Exit | Natural completion restores mixer level 95; Escape bypasses it; both stop FM, request song fade 50, close scroll and clear keys before common EXE shutdown | `0x88c2..0x8933` |

EXE labels are at `0x1f759`; narration comes from XEEN.CC's `special.bin`,
first NUL record (390 bytes, copied with an 800-byte bound at `0x84b1..0x84df`).
The voice-pointer table is `0x1f540`; mouth frames `0x1f448`, elapsed thresholds
`0x1f258`, row lookup `0x1f578`, tower coordinate tables `0x1f586..0x1f5b5`.
The mouth tables have **31 entries per row**. At `0x85a7..0x88a9`, thresholds
are relative to voice start and waits stop on digital idle. Group animation
increments its Xeen counter in slots nine and thirteen before modulo
(`0x85c0..0x86d0`). Do not regroup the original tables as the pin's 32-entry rows.

There is no universal tick total: loading, pan stride, retrace-limited redraws
and voice completion affect duration. **Unknown until DOSBox:** realized stride
and redraw counts at the maintainer's chosen cycle setting. The formula is known;
a fixed stride or a pin timing is not an evidentiary substitute. See the
[reference differences](../original-quirks.md#sv-clouds-intro-presentation).

## Settings and input

Implementation: `games/xeen/XeenCutscene` describes ordered compositions and
segment holds; `app/XeenCutsceneFlow` publishes them using the existing clock
conversions and audio owner. `formats/xeen/XeenCutsceneData` reads and validates
the installed EXE and the hardware word in CFG. `app/XeenTitle.cpp` owns the
title exit and fresh-title return. No Journey clock, RNG or save fields are used.

The EXE's mixer commands 75 and 95 have no counterpart exposed by MMModern's
audio owner/output queue. They are not issued: natural completion and Escape
share cleanup in MMModern, while the EXE restores 95 only on natural completion.
The existing fixed audio gains remain in effect. Song fade 50 and digital stop
still go through `XeenAudio`. MMModern uses `mm4theme.m` with its CD promus
driver; device-four/five song drivers belong to roadmap step 19.

**DOS static, high.** `XEEN.CFG` configures hardware modules (digital word at
`+0`, music at `+4`), not DAT's saved party Music/Speech/Efx/Text switches.
EXE selects modules at `0x784c..0x79d3`; its audio wrappers use driver presence
(`0x11b24..0x11c66`) and do not apply those party gates.

Only hardware digital device zero/one enables the caption strip
(`0x1191b..0x119bf`). Every two elapsed BIOS ticks it advances through the first
`special.bin` record, showing up to 40 characters over `box.vga`; wrapping ends
the narration pass. The helper's formatting string is read from EXE `0x20f60`
(DS:36e0, passed at `0x11971`): its controls select the strip's color,
centering and y position, rather than positioning text at the box's left edge.
Labels remain visible with speech. Turning DAT Speech off,
or muting host output, is not this condition.

Escape ends Clouds at explicit scene polls. Ordinary keys are ignored except
that any key finishes the current scroll and draws its terminal image; the
caller then continues. Mouse clicks are not a scene/scroll skip in this EXE.
Ctrl-Q is a separate global quit (`0x11ab9..0x11af8`), not title return.
Resource reads and unpolled holds can delay a response. DOSBox play confirms
Space, Escape and the click during speech and Escape during the parchment
opening ([M91 runtime checks](evidence.md#m91-clouds-intro-runtime-checks)).

## Title lightning

**DOS static, high.** DAT's `DS:0095` begins zero. Startup checks for readable
`xeen.cc` through `0xb3c0..0xb3c8` → `0xf1ab`; success stores one at `0xb3d5`.
The name is at `0x51a36`, through the far pointer at `DS:2988`. No reset occurs
on the reached startup/title path. The guard at `0xbf90..0xbf99` therefore skips
to `0xc056` in installed CD World, bypassing the 30 frames and `elect.voc`
start/stop at `0xbfcf`/`0xc029`. Missing XEEN.CC is a different resource
configuration, not a normal World-title state. The physical
[audio inventory](audio.md#voc-inventory-boundaries) and `XeenDigitalSitesTests`
retain this row explicitly as unreachable on installed CD World.

## Darkside and ending boundaries

**DOS static, high for reached primitives.** Darkside
`0xa9bd..0xd608` uses the calibrated sprite, palette, BIOS/retrace, VOC and
caption operations. It adds horizontal page transitions, more caption records,
visual randomness (range helper `0x1cbdc`, time-mixed source `0x1cc27`) and
explicit INTRO/DARK switches (`0xbb48`, `0xcf38`, `0xd08f` and their returns).
Its narration is VOC playback, including `pharoh11.voc` at `0xcd6d`; no CD disc-2
speech backend is required by this EXE scene. **Unknown:** exhaustive Darkside
asset decoding and per-frame choreography; item 7 is an estimate, not that audit.

NWC/story polls accept Escape to common cleanup (`0xab4d..0xab57`, `0xd5b5`).
World-logo frame loops poll without aborting; any key only shortens the two
boundary holds (`0xd444..0xd458`, `0xd579..0xd58d`). Mouse does not skip;
Ctrl-Q remains global. Completion/Escape returns to the fresh DAT title above.

The [M91 estimate](../milestone-91-plan.md#part-0---xeenexe-calibration-and-dos-evidence)
includes startup persistence and visual randomness; neither is an approved
addition to the Clouds-only scope.

**Ending kinds only:** compressed full-screen frame playback (`0x89ce..0x8b3b`),
horizontal page merge/rotation and layered motion (`0x91ae` onward), dynamic
monster/attack resources (`0x93a0` onward), presentation-random score reveal and
terminal acknowledgement holds. Detailed score/input behavior is **Reference
only / Unknown in DOS**; this list does not certify the ending or its readers.
