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
`D:/Projetos/MModern/MModern-re/m91/findings.md`. Original-derived material stays outside the repo.

## Startup dispatch

**DOS static, high.** Successful startup reads inherited `XEENCOMMAND`
(`0x7a45..0x7a49`). Missing values become `(null)` in the formatter
(`0x3fa3..0x3fb2`); the comparator and table (`0x7a63..0x7a75`, `0x7c70`)
route that default to Darkside (`0x7a99` → `0xa9bd`). `XEENINTRO` routes to
Clouds; `DARKINTRO` also takes the Darkside path.

Entering Darkside on every launch does **not** mean playing the full story each
time. The EXE reads `XEEN.CFG` (`0x11dcb..0x11e1a`); its word at offset `0x1a`
selects full story when zero and NWC/JVC followed by the World-logo tail when
nonzero (`0xad3d..0xad64`). Both paths set it to one and write the configuration
**before** the World-logo tail (`0xd345..0xd374`). Escape before this store
leaves it unchanged. DAT Other Options `D` clears it and writes the
configuration before handing off (`0xc571..0xc59e`). Thus DOS has persistent
seen-intro behavior; the pin's `seen_intro` is not grounds for claiming that
DOS always plays the full story.

MMModern plays startup and the Darkside intro since
[M93](../milestone-93-plan.md) and keeps the seen state in `settings.ini`
(maintainer, 2026-10-26); it never writes the original configuration.

Both paths draw the same NWC and JVC sequences. At JVC, seen nonzero stops the
song; zero starts it (`0xac5a..0xac75`). The 60-tick hold starts after fade-in
and includes pyramid/voice preloads even on the short path (`0xac7e..0xad23`).
The jump at `0xad64` reaches the seen store directly. The write has no local
result check; persistence after a disk error is not guaranteed.

Before the first frame, video setup enters mode 13 and writes a black palette
when the inherited mode differs (`0x73bb`, `0x14610..0x1464d`); already-mode-13
entry skips that reset. Other Options `D` has already faded the title. Drivers
and song storage are initialized, but no scene song or VOC has started
(`0x784c..0x79f1`). The hardware mixer follows the existing
[initialization rule](audio.md#card-mixer). Device eight can show its hardware
initialization screen (`0x11b9f..0x11c46`); its latency is not a scene hold.

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
`D:/Projetos/MModern/MModern-re/m91/resources/` record archive offsets, sizes and decoder limits.
The compiled `XeenCutsceneTests` audit decodes all 35 sprites (325 frames),
scene/startup VOCs, RAWs and palettes and runs the complete song/scene with real
sample lengths on a fake clock. It also checks input, load/reset boundaries,
pan stride and the synthetic CFG caption path. **Unknown:** bit-exact rendered
and audible equivalence with DOS.

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

The intro and the ending issue the EXE's mixer commands through `XeenAudio`,
which applies them to the Sound Blaster mixer ([card mixer](audio.md#card-mixer)):
the intro sends 75 after the opening scroll and 95 only on natural completion
(`XeenCutscene::completion`; Escape enters the shared cleanup without it), the
ending 75 and 95. Each EXE start and the fresh DAT title rewrite the init
levels. Song fade 50 and digital stop still go through `XeenAudio`. MMModern uses `mm4theme.m` with its CD promus
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
Ctrl-Q quits from any XEEN.EXE presentation (`0x11ab9..0x11af8`), not title
return. This EXE poll does not enable process quit in DAT gameplay or its title,
including the title-exit fade and digital wait (`0xb83e..0xb8ae`). The shared
EXE fade (`0x14a69..0x14adb`) neither polls nor clears keys: a key pressed
during a Clouds or Darkside fade waits for the next scene poll. The Clouds scene
clears keys only at entry (`0x7cc9`) and exit (`0x892b`).
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

**DOS static, high; calibrated 2026-10-26.** Darkside `0xa9bd..0xd608` uses the
M91 primitives and M92 range helper. The complete rules, resource report,
extracted tables, pin comparison and supplied-video timestamps are external:
`D:/Projetos/MModern/MModern-re/m93/findings.md`. This calibration describes
original behavior. `XeenCutscene::darkside` (`games/xeen/XeenDarksideIntro.cpp`)
plays both paths; `Application::run` in `app/XeenTitle.cpp` chooses the path at
startup and stores the seen state (see [startup dispatch](#startup-dispatch)).

Darkside disables the scroll at `0xa9c3`. Completion/Escape joins `0xd5b5`:
FM zero and buffer cleanup, then the scroll call at `0xd5f0` immediately returns
through `0x6f28..0x6f35`. There is no parchment or Clouds song-fade-50 call.
Common shutdown publishes, frees pages, stops song/VOC and releases drivers
(`0x7b27..0x7b8e`). Video shutdown argument one retains mode 13
(`0x7b96`, `0x14893..0x1489c`). The fresh DAT title above follows; Other Options
does not resume. There is no final World-logo fade-out in the scene routine.

**Implementation inference, medium:** every reached hold fits the existing
`PresentationClock` BIOS/retrace conversions. Counter one usually resets before
composition, counter two retains room-entry time, and counter three paces the
strip. Resource loading and voice idle remain part of elapsed time. Horizontal
page merge is `0x193bb` (320 columns); vertical merge is `0x19429` (200 rows).
Fade step 24 uses seven retrace-paced writes; steps four/eight use 33/17.

## Darkside sequencing rules

**DOS static, high.** All counts below are BIOS elapsed holds, not the pin's
50 Hz waits. Sprite scale zero is full size. Positions, paired zoom coordinates,
voice/cue tables and all reset sites are retained in the external findings.

| Stage | Rule | EXE evidence |
| --- | --- | --- |
| NWC | `dark.pal`, two RAW pages; initial merge/publish/fade four and FM one. First 55 iterations use the first three sprite handles, changing at 34/44; dragon voices at 17/35 replace that iteration's frame increment. Next 42 draw `nwc4` frames 0..41; FM 40/0/3/3/2 at 3/11/27/30/33 and dragon3 at 31. Two ticks per composition; final ten-tick hold shares the last frame's reset. | `0xa9bd..0xac10` |
| JVC | Mixer 95; load before fade-out eight, publish, conditional song start/stop, fade-in four; 60-tick hold includes the following resource loads. Same graphics on both seen paths. | `0xac11..0xad64` |
| Pyramid approach | Vertical page offset 0..200, title base plus animation advanced every fourth draw. Latch first elapsed-minus-one, minimum one, before fade-in four as pixel stride; clamp the endpoint. One-tick minimum per draw, then bare endpoint/save/free pages and a newly reset 30-tick hold. | `0xad67..0xaeac`, stride `0xae13..0xae27`, hold reset `0xae91` |
| Pyramid zoom | Restore background at every scale 14..0, two ticks each; mixer 48 after publishing scale two. Then 19 pyramid frames at (132,62), four ticks each, with caption updates inside the holds. Start pharoh1a, then 1b once on idle during the animation. The final voice/caption tail cannot start a late 1b. Fade four. | `0xaead..0xb055` |
| Pharaoh room | Reset counter two before loading; horizontal offset 320 downward by twice the approach stride, one-tick minimum. Dragon positions wrap at ten and frame lookup at 36; every third iteration resets the subcounter and advances both. FM seven/eight at old position five/zero. Chain pharoh2a..e on idle during the pan. The endpoint tail draws frame zero and waits only for the current voice/caption; it does not advance that chain. No-digital hardware also requires 220 ticks since entry. Stop VOC/fade four. | `0xb056..0xb3c3`, tail `0xb322..0xb383` |
| Crystal-ball composition | Retain `dark.pal`, restore `ball.raw` each draw. A 32-phase composite layers ball, right claws, dragon and left idle/rubbing claws. Speaker/fizzle at (9,57); claw six at (149,184), dragon x=139; animated claw/dragon layers use flag `0x800`. Reset before composing, publish/poll once, then wait to three ticks. Initial rub runs 32 phases and fades in after the first publication. | `0xb3c4..0xcd65` |
| Queen | Incoming fizzle 0..10, queen zero behind indices >3; FM 42, pharoh3/FM zero at six. On voice **and** caption idle, chain queen1/2/hiss; random speaking frames 0..5, hiss frames 7..16 then zero. Clear the strip for the later queen clips; pharoh4 follows with queen zero. | `0xb4ed..0xbb40` |
| Dark Lord | dark3, then pharoh5a/5b/laff1 on idle; random frames 0..3 only in the first speaking state. Reverse fizzle 10..0 selects the reply caption and can start/restart laff1 once on idle. Random Dark Lord remains behind indices >3; FM 42 then zero. | `0xbb42..0xbfb1` |
| Alamar/goon | Rub 32; select caption before incoming fizzle, FM 42, pharoh6w at six. Start alamar1/caption/FM zero on both idle; goon cycles 0..16 while speaking. Reverse fizzle selects the next caption, starts pharoh7t once on idle, draws goon zero behind indices >4; FM 42 then zero. | `0xbfb4..0xc5ee` |
| Ellinger/orb decision | Rub 32; caption before fizzle, pharoh8 at five. On idle start ellinger/caption/FM zero, then pharoh9a with the strip cleared. Only Ellinger uses random wizard frames. Reverse fizzle draws wizard zero behind indices >4 without caption updates. Then combined record 19 accompanies pharoh9b; wait for voice and strip, FM zero/fade four. | `0xc5f0..0xcd65` |
| Release creature | `fly.pal/raw`, frames 0..34, two ticks, pharoh11; update captions each frame, with no final caption-completion hold. FM 43 at 16/28, zero afterwards; fade four, mixer 95. | `0xcd6d..0xce90` |
| Bird/comet | `bird.raw`, 81 frames selected by index/8 and index%8 from bird01..11, two ticks. Comet at 1/21, FM 43 at 1/16/39, screech at 56, disint at 61, laff1 at 73; FM zero afterwards. | `0xce94..0xd04e` |
| Fall | `drop.raw`, 39 frames selected by index/12 and index%12 from drop1..4, two ticks. Branch at nine, vine at 22, thud after the loop; fade-out 24. | `0xd07d..0xd1ab` |
| Orb/title | `end.raw`, fade four, song command 16 (fade speed). Nineteen end1..3 frames selected by index/8 and index%8, four ticks. Save, reset before loading title2, hold 20 ticks. Restore for each paired title scale 11..0, two ticks, save then seen store. | `0xd1af..0xd374`, command `0xd213`, reset `0xd292` |
| World logo | Fade four, `firemain.pal/raw`, publish/fade four. Reset before loading wfire1..7; first ten-tick boundary hold. Reveal 28 frames at (0,45), index/5 and index%5, two ticks; restart rumble when idle before index 17, explosio at 17. Save final composition, then twice draw wfire7 frames 0..20 over it, two ticks; thud at 0/11 and FM 60 at three each cycle. Last ten-tick boundary hold shares the final frame reset. | `0xd37f..0xd58e`, resets `0xd409`, `0xd503` |

The supplied seen-zero video shows JVC around 12.4..14.1 s, room by 35.6 s,
ball at 45 s, release at 106.8 s, bird at 111.6 s and fall at 120.2 s.
The World tail is still visible at 141 s; black at 141.5 s precedes the fresh
title fade at 142 s. These are capture observations, not fixed scene deadlines.

## Darkside resources and data

**DOS static, high for selection; compiled-reader audit for decoding.**
INTRO.CC supplies the scenes except temporary DARK.CC loads: laff1 at
`0xbb48..0xbb69`; screech/disint/laff1 at `0xcf38..0xcf7f`; branch at
`0xd08f..0xd0af`. Startup also loads DARK's opaque `46k.buf`
(`0x7a1e..0x7a3d`). Each range ends by restoring INTRO; there is no archive
fallback. All narration is VOC, including pharoh11, with no CD speech request.

The external report accepts 103 distinct archive/name dependencies: 45 sprites
(470 frames), 36 VOCs, 15 RAWs, three PALs, two songs, one caption resource and
one scratch buffer. `XeenCutsceneTests` exercises the compiled sprite, VOC, RAW,
PAL and song readers; this does not establish audible or rendered equivalence.
`.int` resources have the ordinary Xeen frame/component headers
and scanline RLE; no new decoder is needed. `fizzle.int` uses the already-admitted
transparent terminal row overshoot. No compressed-frame decoder call is reached.
`wizmth.int` is loaded but not drawn; pharoh2d/e are generic voice handles copied
to alternating buffers (`0xb27b`, `0xb298`), not sprites. Startup `ido2.voc` is
overwritten before playback (`0xb484..0xb499`).

Darkside music devices five/nine choose `cantheme.m`; all others choose
`bigtheme.m` (`0x7932..0x79e9`, jump table `0x7c80`, names `0x1ef78/0x1ef83`).
This differs from Clouds' device-four/five/other selection. Mixer arguments are
95/48/95 (argument pushes `0xac2b`, `0xaf13`, `0xce83`; each far call follows
two bytes later), not percentage volume. Escape adds no
restoration; fresh DAT initialization restores its mixer state.

The four range-helper calls are queen `0xb8d1` (0..5), Dark Lord `0xbc86` and
`0xbe46` (0..3), wizard `0xca27` (0..3). They select sprite frames only, using
the M92 private time-mixed source `0x1cc27`, low-15-bit inclusive modulo through
`0x1cbdc`. They never consume Journey RNG. The pin cycles by host time instead.

A runtime EXE reader must verify identity and bounds, retain signed coordinates,
and check the following shapes without copying original tables into the repo:

- Three 12-word title tables (`0x1fb60/0x1fb78/0x1fb90`), three 15-word pyramid
  tables (`0x1fba8/0x1fbc6/0x1fbe4`).
- Two ten-word room position tables (`0x1fc02/0x1fc16`), 36 frame words
  (`0x1fc2a`, values 0..9). The following 34 words are not consumed.
- Eight 32-word ball tables (`0x1fcb6..0x1feb5`); frame values 0..3, signed
  offscreen coordinates preserved. All 17 consumed tables match the pin.
- Bounded NUL resource names, exact formatted-name patterns, cue selectors
  (`0xd60a`, `0xd626`, `0xd63e`), and the existing strip format (`0x20f60`).
  Validate selected frames against decoded resources and caption seeds against
  nonempty NUL-terminated records within the 1000-byte copy bound.

## Darkside captions and input

**DOS static, high.** The same every-NUL record loader and hardware predicate
as M91/M92 apply (`0x1189e..0x119bf`): digital device zero/one enables the strip;
party Speech/Text and host mute do not. One character advances after more than
one BIOS tick on counter three, then resets it; each record selection also
resets that counter (for example `0xaf82`, `0xc12f`, `0xc777`, `0xcde6`). A 40-character window uses
`box.vga` at (36,189) and the EXE format controls. Three-tick ball composites
consult it once per frame; pyramid voice holds consult it inside their waits.

| Record / initial index | Selection | EXE evidence |
| --- | --- | --- |
| 0 / 8; 1 / 25 | Pyramid, room | `0xaf52..0xaf82`, `0xb16f..0xb19e` |
| 9 / 1; 10 / 1; 11 / 1 | Queen introduction, queen1, Pharaoh reply; later queen clips clear the strip | `0xb5df..0xb60e`, `0xb7c3..0xb7f2`, `0xb9db..0xba0a` |
| 12 / 1; 13 / 1 | Dark Lord; reply only at reverse fizzle | `0xbb84..0xbbb3`, `0xbd12..0xbd41` |
| 14 / 1; 15 / 1; 16 / 1 | Before goon fizzle, Alamar, reverse fizzle | `0xc105..0xc134`, `0xc2df..0xc30e`, `0xc461..0xc490` |
| 17 / 1; 18 / 1; 19 / 1 | Before wizard fizzle, Ellinger, combined orb/destiny text with pharoh9b; pharoh9a clears the strip | `0xc74d..0xc77c`, `0xc922..0xc951`, `0xcbc4..0xcbf3` |
| 6 / 0 | Release creature | `0xcdb6..0xcde6` |

There is no record-20 selection. Reverse wizard fizzle publishes without updating
captions (publication `0xcb9a`). Extracted text stays in the external findings.
The Alamar and Ellinger incoming fizzles select their captions before the loop,
but call the strip helper only after frame six (`0xc252..0xc268`,
`0xc892..0xc8a8`); earlier frames use a plain flip.

NWC/JVC/story polls accept Escape to cleanup (`0xab4d..0xab57`, `0xad26`,
`0xd31f`, `0xd5b5`); ordinary keys do not skip. Fixed inner holds and voice tails
can delay response; `0xb01d..0xb03f` and `0xb322..0xb383` do not directly poll.
World frame polls (`0xd4d6`, `0xd55b`) ignore Escape too. Any key only shortens
its two boundary holds (`0xd444..0xd458`, `0xd579..0xd58d`). Mouse does not skip.
Ctrl-Q still exits the EXE through `0x11aa2..0x11afc`, not to the fresh title;
it cannot undo the already-written seen flag during the World tail.

**DOSBox corroboration:** the maintainer's M93 session confirms the short
repeat launch with a silent JVC, `D`'s forced story, the full story due again
after an early Escape, a click that does not skip, and a key that shortens
only the first World boundary hold while Escape leaves the World frames
running ([runtime checks](evidence.md#m93-darkside-startup-runtime-checks)).
On a fake clock, M93's stage times land within these recordings' brackets;
`XeenCutsceneTests` checks the World-logo tail. **Unknown:** the realized DOS
strides and redraw counts per scene, pre-first-frame and sound-cutoff latency,
and no-digital caption appearance. See the
[reference differences](../original-quirks.md#sv-darkside-startup-seen).

## Clouds ending entry and save

**DOS static, high; save corroborated by the maintainer's slot-06 capture,
2026-10-25.** The complete member comparison is in
`D:/Projetos/MModern/MModern-re/m92/archive-diff.json`; provenance is in
[M92 evidence](evidence.md#m92-clouds-ending-evidence).

Opcode 30 enters the DAT tail at `0x28b98`. In order, it sets the saved
completion word (DS `efe8`), copies an eight-character fixed marker and
its terminator into CFG, sets game flag 75, and moves the party to map 28
`(18,4)` without changing facing. The Sixth Mirror is used facing West,
which explains the West facing in the resulting save. It keeps the Scepter.
It then writes the 30-byte CFG, performs forced Save to the current slot
(`0x28c0b..0x28c12`), requests song fade 207, turns Efx and Music on, computes
the score, and sets `XEENCOMMAND=ENDGAME,<ten-digit score>,0`
(`0x28c13..0x28d23`). The sound switches written in that save are consequently
the switches from before the forced-on stores. The save flushes the live
map before loading resources for map 28; see [party persistence](party.md#clouds-ending-save).

The CFG marker is a fixed string, not an encoded score. Startup compares its
first eight bytes (`0xb61e..0xb634`), independently of saved party completion.
Its NUL at byte 18 also clears the low byte of DAT's word-copy/dword-copy
optimization option (`0xb451`, setter `0x3f866`, consumers including
`0x3f8a2..0x3f8c3`). DAT's comparison does not need the terminator. The EXE
loads CFG for hardware selection but its ending does not test this marker
or write CFG; the write at `0xd345` belongs to the separate Darkside path.
For the ending, MMModern's approved settings store needs only the won boolean.

Clouds mirror result 40 (`showtime`) jumps to `0x28bd1`
(`0x2801b..0x2802a`), skipping the victory, marker, flag and movement stores.
It still writes CFG and performs the ordinary forced Save, computes the real
score, plays the same ending and returns to a fresh title. The party's saved
position and completion remain as they were. Result 62 (`bloopers`) calls the
empty far-return routine `0x26773`, then exits the Event at `0x28d50`
(`0x2804e..0x28061`); it plays nothing and does not execute the next Event line.

Other Options offers `V` only when the marker matched and XEEN.CC is present
(`0xc1f7..0xc206`, `0xc320..0xc34b`, `0xc5c0..0xc5d3`). Before a win the row
and hotkey are absent, rather than drawn disabled. In installed World its
row follows `D` and `C`, at y=103. It hands off with fixed score `1234567890`;
the EXE displays those ten digits. This title-only path writes no party,
counter or won state. EXE completion returns through the fresh DAT title
described above, with a new program start mark.

## Clouds ending score

**DOS static, high; calibrated against the recorded `0071001722`.** Every
active member contributes, including terminal-condition members. The helper
at `0x1e336..0x1e3bb` reconstructs total earned XP from raw saved XP, class
and permanent level, as the character sheet does
([level thresholds](combat.md#level-thresholds)). For normal positive levels,
with `L = level - 1`:

| L | Added to raw XP |
| --- | --- |
| 0 | 0 |
| 1..11 | class constant shifted left by L-1 |
| 12 or more | 1,024,000 × (L-11) + class constant shifted left by 10 |

Class constants, in class-ID order 0..9, are 1500, 2000, 2000, 1500, 2000,
1000, 1500, 1500, 1500, 2000 (`0x501b0`, DS `2920`). The score loop sums the
32-bit results, divides by active count, divides by 10,000, then multiplies
by 100,000, truncating at each division (`0x28c38..0x28c98`). It adds hours
× 100 + minutes from the real play-time seconds (`0x28c9b..0x28cea`), omitting
seconds. Arithmetic retains DOS widths: the XP sum/product wrap at 32 bits;
the time quotient is narrowed to 16 bits, the hour product and minute
addition wrap at 16 bits and are sign-extended before adding to the score.
The output is an unsigned decimal field of minimum width ten (`%010lu`).

The six slot-05 pre-fight total-XP values are 6,855,710; 6,851,510; 6,457,562;
8,133,740; 6,874,910; 6,869,910. Sum 42,043,342 plus the 600,000 kill reward
gives 42,643,342. Dividing by six and then 10,000 gives 710; the XP score is
71,000,000. Saved play time 62,545 seconds is 17:22:25, contributing 1722;
the result is `0071001722`. The score reads live records after forced Save.
The fixture needs class and level as well as raw XP; raw XP alone cannot
reproduce this calculation. The new capture distributes 150,000 XP to four
eligible members, with the same aggregate reward.

## Clouds ending presentation

**DOS static, high for the stated operations; DOSBox timestamps refer to the
maintainer's uninterrupted ending capture.** Full resource extents, decoded
payload hashes, selector order, tables and aligned disassembly remain outside
the repository in `D:/Projetos/MModern/MModern-re/m92/`. Every ending scene uses XEEN.CC after
the initial INTRO.CC scratch allocation. Scene speech uses VOCs, not CD tracks.

| Stage | DOS operations and timing | Capture time |
| --- | --- | --- |
| Castle, pyramid and vortex | `0x89ce..0x905b`: 41 `prec` compressed frames, six `cast`/`casb` pairs and three `darklrd` sprites, 20 cycling `vort` frames; initial 15-tick hold and three-tick animation holds; `xeenlaff`, `dark1..3`, `darklaff`, original FM cues and mixer changes | Castle first visible between 51.100 and 51.200 s; pyramid 59.341 s; destruction transitions 62.965..66.261 s; vortex ends around 82 s |
| Burlock and throne | `0x905b..0x94cf`: start `endgame.m`, show `later.raw` for a 100-tick minimum including setup, horizontal throne-page reveal with measured stride, layered people/Crodo and scaled king; `king1..3` with one-tick movement and three-tick mouth holds | Later around 82..89 s; throne around 89..94.969 s; king close-up 94.969..about 103 s |
| Mirror and parade | `0x94d0..0x9d16`: mirror-room pan, nine scale steps, measured-stride king/Crodo entrance; 73 monster selectors, each with eight idle frames, three attack frames, 15-tick attack hold and seven-tick power hold; sky rotates through its 320 columns | Room around 103..107.938 s; zoom/parade 107.938..240.860 s |
| Scroll and score | `0x9d19..0xa694`: shared scroll, 20 heading frames held one tick, 30 random-digit frames and ten settling frames held two ticks, 38 downward steps, then two key acknowledgements | Scroll 240.860..243.157 s; score reveal and both prompts 243.157..255.927 s |
| Last king and return | `0xa695..0xa82a`: `king4.voc`, random mouth frames 1..6 at three ticks, caption completion, closing scroll, common EXE shutdown | King after 255.927 s; closing 261.963..263.076 s; title fades in after the intervening black |

Detected cuts are frame measurements; approximate boundaries above are bracketed
by the timestamped one-frame-per-second sheet. Loading, palette retraces,
measured pan stride and speech completion contribute to elapsed time. Prompt
waits depend on the player's acknowledgement. Do not replace these with one
fixed whole-ending duration or tune random digits to match the recording.
Direct frame inspection corrects the earlier scene-detector label: 49.553 s
shows the DAT's closing scroll over the Sixth Mirror, not the castle. Timing
tests align the first castle frame to the 51.100..51.200 s bracket. Later is
bracketed by 82 s (vortex fading) and 83 s (Later visible); throne by 89 s
(black) and 89.5 s (room visible). EXE `0x9094` fades the vortex with step 1,
not the step 4 used for the Later fades. No artificial transition delays are
needed to match the C1 boundaries.

The only new resource kind is the compressed full-screen frame. EXE
`0x18c70` calls decoder `0x1cd42`; its 314-leaf adaptive tree, 4096-byte history,
initial fill word and big-endian expanded count match the pinned reader's
contract. All 61 reached frames declare 64,000 output bytes. A
**corrected Python audit and compiled-reader test** reconstruct all 61 outputs:
37 accept complete word reads, while 24 end with one final byte. The initial
Python audit omitted the LZ match-source increment and its hashes are superseded
(60 of 61 changed). **DOS static, high:** the decoder reads the match source at
`0x1d01e`, increments it at `0x1d03e` and loops at `0x1d03f`; this agrees with
the pinned reader. The corrected hashes are checked by `XeenEndingPresentationTests`.
Tracking bit provenance proves that the absent low byte of that prefetched
word is never consumed. `XeenCompressedFrame` bounds the final read to its
one available byte and rejects any attempt to consume absent bits.
The repeated audit with that absent byte set to 0 and 255 produces identical
64,000-byte outputs and matches all 24 corrected provenance-audit hashes.
These are static and decoder checks, not execution of the DOS decoder or
rendered equivalence with DOSBox. The audit and before/after `prec0002` PNGs
remain in `D:/Projetos/MModern/MModern-re/m92/exe-inventory/`.
Other resources use the existing RAW, PAL, sprite, VOC and song families.
The manifest resolves 146 parade MON/ATT resources, 69 unique attack VOCs,
six power VOCs, the scene voices and shared scroll assets, with no missing
member. No compiled-reader or rendered-equivalence claim follows from that
inventory.

The score uses original sprite digits, with frames 20..29 drawn at x=64+19n.
The rightmost digit settles first; the ten-entry branch table at `0xa82c`
selects fall-through stores from `0x9fc7..0xa02b`. Random digits use range
20..29 (`0x9ded`, `0x9f9b`), power sounds use 0..5 (`0x9bb1`), and king mouth
selection uses 1..6 (`0x93fe`, `0xa75a`). All call range helper `0x1cbdc`,
which takes the low 15 bits of time-mixed source `0x1cc27`, reduces modulo
the inclusive range and adds its lower bound. The source samples DOS time
with hundredths and mixes three private words with carry/rotate operations.
The host sample retains civil hour, minute and second directly, independently
of the DAT's DOS-seconds/ctime conversion. This preserves the spring-forward
hour and times after the signed 2038 display limit; fixed dev/test clocks
retain a deterministic civil interpretation of their injected seconds.
It is independent of Journey randomness. The parsed final-score digits come
from the command's ten-character field, not from that random source.

**DOSBox corroboration:** the maintainer's final Part 0 checks confirm
`showtime`, `bloopers`, title `V`, animation input, keyboard-only score
acknowledgements and the scroll's key behavior
([runtime results](evidence.md#m92-clouds-ending-evidence)).

**Input differs from the pinned ending.** Animation polls only update the
key result; they do not branch on Escape or ordinary keys. Mouse clicks do
not acknowledge the two text pages: `0xa4e5..0xa4ee` and
`0xa670..0xa679` wait for a nonzero keyboard result, including Escape.
Each page clears keys at entry. Scrolls retain the shared any-key terminal
image behavior; the caller then continues. Ctrl-Q quits throughout the EXE
presentation, without changing DAT title/gameplay input. The pin's `WAIT`/`Subtitles::wait` instead lets key or mouse input
abort stages. Caption indexing is also DOS-specific: loader
`0x1189e..0x11906` builds a word-offset table from every NUL. Alamar uses
record 8 (`0x8e4a..0x8e69`), while both the king's first speech and final
`king4` speech use record 10 (`0x9371..0x938d`, `0xa6d6..0xa6f5`). Record 9
is the congratulatory speech; DOS skips it even for `king1..3`, showing
the final Darkside instruction instead when captions are enabled. Keep
that original mismatch. The same no-digital-device condition and two-tick
strip threshold as M91 apply. The ending calls the helper once per mouth
iteration (`0x8f22`, `0x942a`), then waits three ticks; it does not poll the
strip inside that wait. Thus the threshold is two ticks, but uninterrupted
ending mouths advance the strip once per three-tick iteration. The helper
publishes the composed frame before that wait. Speech-off in the DAT is not
the hardware condition.

The parade's flying offsets come from signed EXE bytes at DS `009c`/`00a4`,
with the modulo-eight counter advanced on every redraw. Idle/attack frame
holds reset the BIOS counter once per frame; the 15-tick attack and seven-tick
power holds keep redrawing and rotating the sky until elapsed. The power hold
draws attack frame **3** (`0x9c84`), after the three attack frames 0..2.
The pin instead keeps attack frame 2. Room zooms and the 38 score downward
steps are retrace-paced; the score pages keep the sky rotating while waiting
for a fresh key. Their text is read from EXE DS `21db` and `2263` at run time.
**DOS static, high; capture 253/255 s.** Both use window 28, set to
`(63,60,191,100)` by `0xa353..0xa363` / `0x1d51f`. Its eight-pixel margin
(CS `0222`, file `0x12962`) is applied by `0x19540..0x19583`: text starts
at `(71,68)`, with a 175-pixel wrap width and inner bottom 152. Normal lines
advance ten pixels (`0x18aa6..0x18ab9`). The resource controls select body
color 23 (palette entry 128) and prompt color 33 (entry 189); both pages
center the prompt at relative y 70, hence screen y 138 (`0x189ec..0x189ff`).
When the EXE halves a centered run's width it rounds upward
(`0x18810..0x18818`); the 71-pixel prompt starts at x 124.

The complete-program timing test answers page one at capture-aligned 252.0 s
and page two at 255.7 s. These are fixed test inputs, not claims about the
recorded player's exact key-down timestamps. The final numeric field is
formatted to ten characters from the outcome; title replay supplies 1234567890.

## Darkside and World win entry and save

**DOS static, high within the cited paths (M106 Part 0, 2026-11-01).**
Same installed CD hashes and M92 calibration; [evidence](evidence.md#m106-part-0).
The maintainer's DOSBox capture (2026-11-01) confirms the saved state below
([capture](evidence.md#m106-dosbox-capture)). These are original rules, not
claims that the current engine implements these endings.

Original map 51 (`maze0051.evt`) reaches opcode 57 at `(2,13)` North,
record `@378`, line 16. The approach at `(2,5)` clears levitation (SetVar 84),
changes its line 0 to DoNothing, then TeleportAndContinue chains up `(2,6)`
through `(2,13)` (`@126..236`). At the destination, SetChar 0 selects everyone;
If2 item ID 160 tests quest counter 78, **Soul Box with Corak inside**
(`@245..252`). Without it, the text/voice "You Fools!" path gives Dead (13)
to everyone and exits (`@261..316`). With it, lines 1 and 2 become DoNothing,
every member gets 5,000,000 raw XP and award 75, and the Soul Box counter is
taken before opcode 57 (`@322..378`). No battle or Spawn appears in this run;
the original MOB has one monster parked at `(100,100)`. The confrontation is
the ending presentation. This corrects the plan's former "defeating Alamar"
wording; a combat reward must not be invented here.

Map 129 (`mazex129.evt`) uses `(13,15)`, any facing. Its initial selector is
member 1. Lines 0..11 require item 90 (quest 8, Xeen's Scepter of Temporal
Distortion), item 135 (quest 53, Cube of Power), and awards 61, 62, 63, 64
on that member (`@39..146`). Missing any requirement jumps to line 13's
CD speech/NPC explanation. Passing reaches opcode 58 at `@147`, line 12;
no reward, item consumption, movement or flag write precedes it on that path.
These are the direct admission tests, not an assertion about all earlier
quests needed to acquire the objects. The XP, item and award semantics reuse
[settled Event rules](world-events.md#script-values-conditions-writes-and-random-draws).

Dispatch `0x26c50..0x26c66`, table `0x29320` (indexed by opcode minus one),
selects these tails:

| Entry | Stores, in instruction order |
| --- | --- |
| 57, `0x28b6f..0x28b8f` | Quest counter 53 = 1 (`DS:f0ac`, PTY 800); CFG byte 22 = 1 (`DS:8d4e`); Darkside completion word = 1 (`DS:efea`, PTY 606..607); map 29, x 25, y 21, facing North. Then select `ENDGAME2`. No game/quest flag store in the tail. |
| 58, `0x28b5d..0x28b6d` | CFG byte 23 = 1 (`DS:8d4f`); World completion word = 1 (`DS:efec`, PTY 608..609); select `WORLDEND`. No move, quest-item or flag store. |

Both join Clouds at `0x28bd8`. In order: open/write/close **30 bytes of CFG**
(`0x28bde..0x28c0a`), forced Save(1) to the current slot (`0x28c0b..0x28c12`),
song command 207, Efx and Music = 1 (`0x28c13..0x28c21`), then the exact
[M92 score loop](#clouds-ending-score) (`0x28c24..0x28cea`). Save preserves the
**live party and roster**, including the Event's XP, award and consumed item;
its audio switches are the values before forced-on. Map flushing uses the old
loaded map, so Darkside does not load Castleview before saving. See
[party completion](party.md#darkside-and-world-completion) and M92's established
Save(1) merge, rather than reconstructing a pre-Event party.

Strings at `0x53b70`, `0x53b79` and format `0x53c28` produce
`XEENCOMMAND=WORLDEND,<ten-digit score>,0` or
`XEENCOMMAND=ENDGAME2,<ten-digit score>,0`. At `0x28d26..0x28d2b` the
interpreter marks the ending exit and sets line 256; the original script does
not execute its ordinary next line. Normal hand-off and a fresh World title
follow the [shared lifecycle](#hand-off-and-title-return).

The completion-word address sweep found **no gameplay decision readers** for
Darkside or World, unlike Clouds' Castle Xeen/Beacon readers. They persist
through bulk party copies; they do not enable title rows. The only DAT tests
of the Darkside/World won bytes are the three Other Options tests per byte:
height, row construction and hotkey admission (`0xc209`, `0xc213`, `0xc2f9`,
`0xc34f`, `0xc5b3`, `0xc5d5`). No further map or Beacon change is justified by
these words. Search coverage and indirect-access limits are in the evidence.

## Ending mirror results

**DOS static and installed-resource inspection, high.** The 36-record
`darkmirr.txt` and 62-record `xeenmirr.txt` in CD DARK.CC contain all the words
below. Search bounds and the approved exclusion of stale out-of-resource rows
remain as [already settled](world-events.md#travel-and-typed-input).
Darkside starts at record 0; Clouds starts at 0 or 38 depending on map number,
so its ending records remain reachable on either side of that threshold.
There is no completion or won-marker prerequisite for these words.

| Side / result | Word | DOS dispatch and effects |
| --- | --- | --- |
| Darkside 33; Clouds 60 | `showdown` | `0x27fe4..0x27ff1`, `0x2802d..0x2803c` -> `0x28b8f`. Select Darkside ending; **skip** quest 53, won byte, completion and move. |
| Darkside 34; Clouds 61 | `genesis` | `0x27ff4..0x28001`, `0x28064..0x28073` -> `0x28b66`. Select World ending; **skip** won byte and completion. |
| Darkside 35 (World installation); Clouds 40 | `showtime` | `0x28004..0x2802a` -> `0x28bd1`. Select Clouds ending; skip its victory stores. |
| Darkside 36; Clouds 62 | `bloopers` | `0x2803f..0x28061` -> empty far return `0x26773`, then Event exit `0x28d50`. No ending, CFG write or forced Save. |

Every actual ending word still writes CFG once, forces Save once, turns sound
on and computes the real score through the shared tail. It preserves the
previous completion and camera; even a Darkside `showdown` from Clouds does
not move the party to Castleview. The score page's generic Castleview message
does not change that fact. These words terminate the Event via the ending
exit; `bloopers` exits the Event without hand-off or its next script line.
The pin's `doDarkSideEnding` and `doCloudsEnding` instead set completion on
mirror entry; see [quirks](../original-quirks.md#sv-m106-mirror-completion).

## World ending state argument

**DOS static, high.** `0x28c33` clears the state byte before the common score
loop; `0x28cfa..0x28d12` formats that unchanged byte. Both wins and every
ending mirror send **0**, irrespective of Goober or Super Goober awards or
member order. There is no member-precedence rule in these DOS paths.
The pin's first qualifying member, with Super Goober checked before Goober,
is not followed ([difference](../original-quirks.md#sv-m106-world-state)).

Title replay omits the third field altogether. EXE startup zeros
`DS:417e..5791` (`0x2e30..0x2eee`); the command copy ends at `DS:4a8f`, leaving
`DS:4a90` zero. World dispatch `0x7ad5..0x7b22` reads it and selects the
zero/default state. Explicit ASCII 1/2 have separate branches, but no scoped
DAT win, mirror or title replay supplies them. The World presentation starts
at EXE `0xfbc9`, main return `0x11467`, with final helper `0x11468..0x11864`
and shared text helpers `0xfadf..0xfbc8`: roughly 7.3 KiB of main/final-helper
code, plus shared primitives/resources. Step 5 should retain that scope;
award-selected variants are not needed for these entry contracts.

## Other Options ending rows

**DOS static, high, installed World CD with XEEN.CC present.** At x=93 the fixed rows are D at
y=53 and C at y=78. Present ending rows are packed in the order E, V, W,
each 25 pixels below the preceding row (`0xc2bf..0xc37d`); absent rows have
no hotkey. Rendering uses the matching even sprite frames 0/2/4/6/8
(`0xc4c3..0xc500`). Height is 100 plus 25 for each win (`0xc1eb..0xc238`).

The DOS E row does not depend on XEEN.CC: height `0xc209`, construction
`0xc2f9` and hotkey `0xc5b3` test only the Darkside won byte `DS:8d4e`.
C, V and W also require the Clouds context `DS:0095`. Without XEEN.CC,
C is absent and E, when won, is at y=78. MMModern currently gates every
ending row on the Clouds context and still draws C without it; that
missing-XEEN.CC title case remains a gap.

| Clouds / Darkside / World won | Ending rows (hotkey, top y) |
| --- | --- |
| 0 / 0 / 0 | none |
| 0 / 0 / 1 | W 103 |
| 0 / 1 / 0 | E 103 |
| 0 / 1 / 1 | E 103, W 128 |
| 1 / 0 / 0 | V 103 |
| 1 / 0 / 1 | V 103, W 128 |
| 1 / 1 / 0 | E 103, V 128 |
| 1 / 1 / 1 | E 103, V 128, W 153 |

Hotkey paths `0xc5b3..0xc5ef` use literal command strings at `0x521fe`,
`0x5221e`, `0x5223d`: respectively `ENDGAME2`, `ENDGAME`, `WORLDEND`, all
with fixed score **1234567890**, and all without a third field. They write
no CFG, completion, party or roster. The pin agrees on row order, eligibility
and score. The title replay never invokes the gameplay win tail.

## Darkside ending presentation

**DOS static, high for the stated sequence and primitives; the maintainer's
uninterrupted capture ([evidence](evidence.md#m106-dosbox-capture)) is the
audiovisual and wall-time reference for Part B.** EXE
`0x7abb..0x7ad3` parses the score and calls `0xd64e`; the ending returns at
`0xfade`. The pin's four functions are a useful division of the single DOS
routine, not separate DOS entry points. Its main span is about 9.1 KiB.
The external M106 findings retain the ordered resource references, complete
bounded instruction trace, tick thresholds and caption-selection sites.

| Approximate pin part / EXE range | DOS sequence and resource families |
| --- | --- |
| 1, `0xd64e..0xdffe` | Load `special.bin` and `box.vga`; `dngon3.m` from DARK.CC; `scene1.raw`/`endgame.pal`; throne `scene2-b.raw` + `face.end`; `come2`, doors in `mainback.raw` (`sc03a..c.end`); `scene4.raw`/`scene4-1.raw` with `disk.end`; `sc050001.raw`, `diska..e.end`, `sc06a/b.end`; Corak materializes against `sc070001.raw` with `sc07a..h.end`. Door, whoosh, thud, click, explosion, alien and wind effects are preloaded/reused around their frame triggers. |
| 2, `0xdfff..0xe73c` | `corak2`, `yes1` (`sc08.end`), `nowre1` (`sc090001.raw`, `sc09.end`), `nordo2` (`sc10.end`); switch to `dngon2.m`, blank room and `sc11.end`, `scene12.raw`, `ready2`/`sc13.end`, `fight2`/`sc14.end`. |
| 3, `0xe73d..0xf176` | `mainback.raw`/`sc15.end` fight; `fail1`, split scene4 background/`sc16.end`; sc17 pair, sc18 quartet, sc19, `sf17.m`, blank/sc20, mainback/sc21, split scene4/sc22, sc23 pair. `disint`, explosion, alien, thud, gas compression, cast, laugh and `padspell` cues follow fixed frame conditions. |
| 4, `0xf177..0xfa62` | Ricochet `sc24a/b.end`; `admit2` over sc25, `ido2` over sc26 quartet, `what3` over sc27; `dungeon2.m`, mainback with 11 sc28 sprites, then `sc290001.raw` and six sc29 sprites. Castle: frames 0..53, then four repeats of 49..53, all three BIOS ticks. |
| Score/return, `0xfa63..0xfade` | Mixer 95, `outday3.m`, score format at `0xfa87..0xfaa0`, then two Pharaoh pages through `0x6c91`; release caption buffer, closing-scroll helper, shared shutdown and fresh DAT title. |

Timing uses the calibrated BIOS counters, with reset before composition and
publication/retrace inside the measured interval. Initial scene holds 30 ticks
(`0xd714`); throne rise y=74 down to 22 by two uses **one** tick (`0xd84e`),
not the pin's two. `come2` waits at least 27 ticks and caption completion
(`0xd869..0xd891`); the full room holds 30. Most following fixed frames use
2, 3 or 4 ticks; disk movement has one-tick steps and a ten-tick tail, and
sc05 holds 20 (`0xdb09`, `0xdb52`, `0xdbc8`). Corak's 45 arrival frames use
two ticks, enabling the prepared caption after frame 40. `ido2` repeats
frames 22..31 after reaching 31 until voice/caption completion at two ticks
(`0xf500..0xf57a`). Exact waits and sound-gated tails, not a single constant
movie duration, govern playback. Do not transfer the pin's timing unchanged.

Caption records 21..31 come from the same every-NUL `special.bin` loader and
hardware predicate as M93 (digital device 0/1 enables captions). Record 21 is
selected before the throne-rise loop (`0xd7c0..0xd7ef`); record 22 is prepared
at `0xdf26..0xdf55` but enabled only after arrival frame 40 (`0xdfc1..0xdfc6`).
Records 23..31 are selected at `0xe068`, `0xe17d`, `0xe25c`, `0xe533`,
`0xe695`, `0xe90e`, `0xf33c`, `0xf476`, `0xf5c0`. The 40-character strip
retains the calibrated counter-three cadence; voice tails can keep it active.

The animation's 39 keyboard polls ignore the returned key: Escape, ordinary
keys and clicks do not skip it. Ctrl-Q follows the existing EXE quit path
`0x11aa2..0x11afc`; long unpolled inner waits can delay it. The two score
pages use `claw.int`, `dragon1.int`, `3room.raw`, `dark.pal` and the existing
32-position Pharaoh animation (`0x6ccf..0x6ef5`). They show the supplied score
as `%010lu`, the original mailing-address text, then the Castleview/load-game
message. Every page frame waits **three** ticks (`0x6eff..0x6f07`); a keyboard
result, including Escape, advances the page (`0x6f09..0x6f19`). Mouse does not.
The pin uses a one-tick wait and accepts mouse. No score-digit random reveal
occurs here. The closing-scroll key behavior remains the calibrated EXE helper.

Four callsites use the EXE's time-mixed random helper `0x1cbdc`: `0xe0cd`
0..2 (only while `yes1` is busy, otherwise frame zero), `0xe1d8` 0..8,
`0xe2ad` 0..1 and `0xe5a7` 0..3. They animate mouths, independently of Journey
RNG. The pin's `getSpeakingFrame` cycles with host time instead. Full resource
and cue-site detail remains outside the repository; the M106 capture supplies
the audible, wall-time and frame references for Part B's comparison.

## Ending write boundary

**DOS static, high for traced file paths; confirmed by the maintainer's
DOSBox before/after captures ([evidence](evidence.md#m106-dosbox-capture)).** Win opcodes and the three actual ending mirror words perform one
CFG rewrite and one forced Save **before** EXE hand-off. Save merges live
characters, flushes the loaded map, then snapshots NAM/CHR/PTY and copies the
working archive pair to the selected numbered slot (M92 evidence). A replay
from Other Options performs neither operation. `bloopers` performs neither.
These counts are logical save/CFG operations, not the number of buffered DOS
write syscalls or map-member writes.

Neither Darkside's score return nor the World ending's traced calls reach a
slot save or CFG writer. EXE's known 30-byte CFG writer is in the **intro**
(`0xd345..0xd374`), not the ending. M106's whole-image direct-call/write-wrapper
inventory and the residual indirect-call coverage limit are recorded in
[evidence](evidence.md#m106-part-0). No second gameplay save is justified by
the EXE score page; the pin's `showDarkSideScore` late save is not copied.
Successful completion returns with `XEENCOMMAND=MAINGAME`, so the fresh title
does not run the intro's seen-state write. Ctrl-Q during the movie leaves the
already-saved DAT result intact.
