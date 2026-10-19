# Evidence and unresolved questions

This chapter records what supports the [rules reference](README.md), how to
revisit the original-program findings, and where the evidence stops. It is a
research reference, not milestone acceptance or authorization to change code.

## Exact baseline

| Artifact | Identity |
| --- | --- |
| MMModern | `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`, branch `m61-part-b`; inspected 2026-10-12 |
| Pinned ScummVM | `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; separate checkout verified at that revision |
| Original | Installed English World of Xeen CD `WORLD/XEEN.DAT`, 353,511 bytes |
| Original SHA-256 | `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27` |
| Original local path | `F:\Games\gog\Might and Magic 4-5\WORLD\XEEN.DAT` |
| Analysis | Read-only original input, REA with Ghidra 12.1.4; bounded 16-bit instruction checks using Capstone |

The MZ header occupies `0x56c0` bytes; the load module has 331,303 bytes.
For an unrelocated module segment and offset, the file position is
`0x56c0 + segment * 16 + offset`. The REA import uses load segment `0x1000`,
so its linear address for a module byte is `file offset + 0xa940`.
The original data segment is `0x481d`, whose **file base is `0x4d890`**.
Do not confuse its module-relative base with a file offset.

All hexadecimal locations below are **original file offsets** unless explicitly
described as character fields or data-segment offsets. Ranges written `[a,b)`
include `a` and exclude `b`. These locations identify this exact executable;
other releases need their own hash, relocation and calibration.

## Method and limits

The analysis first established the load image and address mapping, then followed
dispatch entries, callers, arguments, conditional branches and writes. Known
behavior supplied calibration: First Aid's 6 HP, its global ID 26 and 1 SP cost;
previously established Suppress Poison refunds; retained item-use observations;
and the separate item-category layouts. A plausible function name alone was
never treated as proof.

Ghidra's inferred item-dispatch function body includes unrelated, noncontiguous
regions. Its aggregate decompilation is unsuitable evidence for that dispatcher.
The conclusions below instead use the bounded original bytes and 16-bit
control flow, checked against the verified image. Branches must be followed:
linear decoding past a return/jump can falsely attribute a neighboring handler
to an item power. Static agreement establishes the stated code path, not that
every possible surrounding game state has been observed in DOSBox.

The cost comparison covers every entry, rather than a random sample. The item
mapping comparison covers all 73 dispatch entries, but semantic destinations
do not prove all downstream effects, refunds, sound, animation or combat
admission identical. The chapters preserve these boundaries explicitly.

Original bytes, disassembly, decompiler output and analysis projects remain
outside the repository. Local evidence is retained under
`D:\MModern-re\rules-20261012\`: `rea-evidence.json`, `query.py`,
`spell-cost-comparison.txt`, `item-mapping.txt`, `monster-loot.txt` and bounded
instruction slices. `query.py` checks the exact original hash before reading.
These local paths describe provenance, not dependencies required to build
MMModern. Reproducing the original checks requires the owner's game installation.

Earlier evidence is retained under `D:\MModern-re\20261011-rea-ghidra\` and
the external `D:\MModern-re\ghidra-xeen-dat\WorldXeenDat.gpr` project.
Its authoritative summaries and calibration are in
[original quirks](../original-quirks.md) and
[Smith price rules](items.md#smith-prices).

## Presentation clock captures (2026-10-13)

**DOS play:** updated maintainer recordings `Abertura.mkv` and `Sonho.mkv`
in `D:/Projetos/MMODERN videos/`, normal CD play in DOSBox Staging with
`cpu_cycles 80000`, using the earlier fluidity recordings' settings. Both
contain 1920x1080 H.264 video at 60 fps (1 ms container timebase) and stereo
48 kHz AAC with game audio. Capture precision is approximately 16.7 ms, not
1 ms. The complete emulator/driver configuration was not separately logged.

FFmpeg 7.1 decoded all 2,170 title and 1,152 Rest frames with `-copyts`,
`showinfo` and `-fps_mode passthrough`. Analysis crops the game image and
retains exact frame PTS, RGB changes and checked frames. The title gem supplies
phase-change evidence; fades are measured within the game image alone.

| Observation | Video PTS / measurement | Interpretation |
| --- | --- | --- |
| Title idle | 29.383..35.983 s, 30 intervals; mean 220.000 ms, population SD 10.773 ms, range 200..234 ms | Four standard BIOS ticks predict 219.702 ms. Inverting the mean gives 18.1818 Hz, compatible with 18.2065 Hz at capture precision; this is not a direct PIT measurement. |
| Title initial fade | First brightening 28.917 s; full brightness about 29.383 s | Approximately 466 ms; compatible with step-4 VGA retrace fading near 70 Hz. |
| Dream fades | Gameplay darkening 6.167..6.633 s; dream brightening 6.717..7.167 s; dream darkening about 11.500..11.933 s; gameplay brightening 12.017..12.467 s | Approximately 466/450/433/450 ms. Visible transition spans are not exact helper entry/return times. A 60 fps capture cannot resolve all 33 writes at 70 Hz. |
| Dream samples | Matched `dreams2` onset about 7.690 s, end 9.355 s; `laff1` onset about 9.356 s, end 11.323 s (audio PTS) | Sequential, approximately 1.665 + 1.967 s; consistent with 24,478 and 28,923 samples at the header rate `1000000/68` Hz. |
| Dream Escape | Maintainer reports Escape did not interrupt it | Confirms no local skip at `0x1e605..0x1e72a`; no input overlay timestamps the press or settles deferred input. |

Audio alignment cross-correlates each original PCM sample against the decoded
recording, resampling in memory. Normalized peak correlations are approximately
0.468 and 0.399 despite background music and capture processing. The best of
nearby tested rates is 14705 Hz; the difference from the header's 14705.882 Hz
is below capture precision. No original bytes or extracted audio enter the repo.

The visible full-bright dream ends around 11.500 s after becoming fully bright
at 7.167 s: approximately 4.333 s, compatible with sequential samples plus two
phase-dependent 7-tick waits (aggregate pre-overhead range 4.290..4.400 s).
Recorded audio appears about 0.15..0.21 s late relative to video if both 7-tick
holds use the standard BIOS rate. A common A/V displacement reconciles both
boundaries; the recording cannot separate driver/mixer latency from capture
synchronization. Do not add that displacement to the emulated silent sample
holds. No contradiction with the static clock/sequence rules was found.

Static anchors: BIOS reset/read `0x440cd..0x44144`, four-tick title poll
`0x0bee7..0x0bf8d`, fade levels/publication `0x3fe0f..0x3ff02` and VGA retrace
wait `0x43fca..0x43fdf`. Use standard 18.2065 Hz and a separate 70 Hz retrace
conversion. At 70 Hz, 33 retrace slots take 471.429 ms; first-to-last of 33
writes takes 457.143 ms. Static evidence supplies the exact write count.

SHA-256: updated title
`b529ec6d1d1c9bdd8b4ebbbac0ae8aeee82ae5b4387a46f4dee0d9e71cba4333`;
updated Rest `7301e5172ff9cdd296ce48916b40dd604aa98a99f16c32c62bf6061f95e1db39`.
Scripts, decoded frames/logs, CSV measurements, `clock-measurements.json` and
`audio-match.json` are retained outside the repository under
`D:/MModern-re/codex-m63-evidence/m63-calibration-audio/`.
The superseded silent captures and their measurements remain in the adjacent
`m63-calibration/` directory for provenance.

Part B re-extracted both recordings with exact frame PTS and reproduced these
measurements. The thirteen title phases span successive cycles of about
2.867/2.850 s. The first idle phase follows the acquired final fade level;
only subsequent phase changes wait four ticks.

## Death and CD speech captures (2026-10-13)

**DOS play + DOS static:** `Morte.mkv` and `NPC falando.mkv` use the same
1920x1080, 60 fps H.264 / stereo 48 kHz AAC capture settings. The initially
paused death recording shows 50,000 cycles, but its window at 3.000 s shows
80,000; the maintainer confirms 80,000 during play. The NPC window also shows
80,000. FFmpeg decoded all 1,221 death and 1,443 NPC frames outside the repo.

| Observation | Exact video PTS / measurement | Established boundary |
| --- | --- | --- |
| Death fire | First visible fire 11.300 s; bare final scene 11.550 s | About 250 ms, consistent with eighteen retrace publications near 70 Hz; capture cannot resolve every phase. No one-tick-per-frame hold. |
| Death laugh | Original sample template matches audio at 11.772792..14.028556 s | 33,173 samples at `1000000/68` Hz: 2.255764 s. Output displacement is not an extra emulated tail. |
| Death return | First closing hands 13.950 s; closure about 15.217 s, fade through about 15.617 s; title usable around 15.800 s | Return is the World of Xeen idle title. It is not the load selector. |
| Gunther CD cue | Vertigo `(14,5,West)`, Event line 5, script offset `0x61` (CD/XEEN offset `0xfd97`), track 19, encoded 370..2161 | Carrier `xeen19.ogg`, requested range 3.600000..21.813333 s; `(end-start+25)/75 = 1366/75 = 18.213333 s`. |
| Gunther face boundary | Complete dialog 1.817 s; first observed nonzero face 2.300 s; last nonzero 20.117 s; permanently idle from 20.267 s | Four disjoint carrier windows identify track/cue. Requested audio output ends about 20.231833 s, between the last busy and first idle face. Speech energy ending earlier does not establish CD idle. |

Gunther's waveform alignment puts requested output start about 2.018500 s,
approximately 201.5 ms after the complete dialog. Driver, mixer, capture and
publication latency cannot be isolated from this recording. **Maintainer
decision, 2026-10-13:** model exactly the DOS requested duration from the
original playback call; add no guessed startup delay and require no instrumented
capture. The call is before the first three-tick poll (`0x25ac7..0x25aee`,
`0x25b8f..0x25bad`), not after a preliminary three-tick delay.

Death scroll passes roll-up=1/fade=0 at `0x0b38c..0x0b391`, publishes hands
22..1 with one-tick waits (`0x0aec3..0x0afd2`), and polls input to close early
(`0x0afbd..0x0afc4`). Its title caller then fades out with step 4
(`0x0bdd6..0x0bde4`). Fire/laugh and dream have no local Escape skip.

Capture SHA-256: death
`3b5daa94599954112419c6d6c376346e8c176275bb4b430576b4c7ef23090ee5`;
NPC `3d3a2e8e671bffd9b73b82b3921385cee8c34be870fc5f72233098bb73ada7fd`.
Frames, exact PTS, audio correlations and scripts remain under
`D:/MModern-re/codex-m63-evidence/m63-part-b/`.

## Navigation typematic evidence (2026-10-13)

**DOS play, capture-limited:** the maintainer's `20.000.mkv` and `80.000.mkv`
each contain one visible continuous forward departure from Temple to the gate.
Fresh FFmpeg 7.1 extraction uses `-copyts`, `showinfo` and passthrough timestamps,
cropping the existing verified game rectangles, outside the repository.

| Recording | First successful step (altar disappears, y28 to y27) | Second step (y26) | Visible gap |
| --- | --- | --- | --- |
| 20,000 cycles | Frame 173, PTS 2.883 s | Frame 201, PTS 3.350 s | 467 ms |
| 80,000 cycles | Frame 229, PTS 3.817 s | Frame 254, PTS 4.233 s | 416 ms |

The 60 fps captures resolve onsets to about one frame (16.7 ms). They have no
key-state overlay or physical press timestamp. These are first-to-second
**visible-step** intervals, not press-to-repeat delays: the first action's
draw/poll latency can shorten that observed gap. Earlier turns are separately
initiated, not evidence of a sustained turn hold. No additional forward hold
start is identifiable. Sustained views average 127.32/114.00 ms in the existing
investigation, excluding initial pause and door transition; drawing caps both
candidate typematic rates and cannot identify the producer's rate.

**Primary device/emulator evidence:** IBM's
[101/102-key keyboard manual, October 1990, printed p.25](https://www.ardent-tool.com/docs/pdf/ibm_hitrc11.pdf#page=29)
specifies default 500 ms and 10.9 characters/s, each with 20% tolerance. Parameter
`2Bh` selects that delay/rate; the ideal period is `(8+3)*2/240 = 11/120 s`
(91.6667 ms, 10.9091 Hz). The printed rounded formula gives 91.74 ms.
[DOSBox Staging v0.83.0 keyboard source](https://github.com/dosbox-staging/dosbox-staging/blob/v0.83.0/src/hardware/input/keyboard.cpp#L321)
instead initializes 500 ms / 33 ms in `set_defaults`; its parameter `2Bh` maps
to 500/92 ms. These are different defaults. The installed executable reports
0.83.0.0. Its explicit launcher/config has no MODE/typematic command; this does
not prove every original game/driver initialization leaves the device unchanged.
Retained DAT IRQ/FIFO analysis proves no repeat synthesis in the inspected
producer/consumer path, but leaves device programming unresolved.

**Maintainer-authorized fallback:** since the clips cannot isolate physical
press timing/rate, M63 Part A uses exactly 500 ms and the ideal 11/120 s nominal
IBM period, independent of host OS repeats, still capped by original draw
readiness. This fixes the maintainer-observed overly sensitive immediate repeat.
It is not a claim that Staging's default or the original driver's rate was
measured as 10.9 Hz. Both possible producer rates exceed the nominal 9.1033/s
draw ceiling. Release and distinct-tap obligations retain the approved option b.

SHA-256: `20.000.mkv`
`d6bd301c0315a66ce0516e97255db7e64c18cc55de7e83d89f4d22cd7d7be042`;
`80.000.mkv` `2ec8c87d7ee5fd6e4341410666ff1255baabfa5d26dc535ef2e3dc1ac467def2`.
Exact PTS maps, extraction logs, checked contact sheets and
`video-initial-gaps.json` are retained under
`D:/MModern-re/codex-m63-evidence/m63-typematic/`.

## Spell and item control flow

The following findings are **DOS static, high confidence**, confined to the
stated paths. They were independently checked against the instruction slices
and pinned source. Existing implementation tests are not substitutes for these
original-program checks.

| Finding | Original anchors | Calibration and boundary |
| --- | --- | --- |
| All 76 SP costs and all 76 gem costs agree with the pin | Signed-word SP table `[0x4f752,0x4f7ea)`; gem-byte table `[0x4f7ec,0x4f838)`; cost lookup `0x2f2de..0x2f2e9`, refund lookup `0x2f353..0x2f35f` | Comparison against `SPELL_COSTS` and `SPELL_GEM_COST` has zero differing entries. The two-byte gap is not a gem entry. This establishes the tables, not every Guild or cost-overflow rule. |
| Resurrection cancellation refunds its own cost | Global dispatch 58 at `0x33598` reaches `0x30bee`; target call supplies 58 at `0x30bf3`; cancel return `0x30bff..0x30c01` | First Aid supplies 26 and Suppress Poison supplies 68 to the same prompt/refund convention. DOS refunds 125 SP/20 gems; the pin supplies Raise Dead 56 and refunds 50 SP/10 gems. The later [healing check](#party-healing-spells) settles the recovery effect. |
| Nature's Cure heals 25 HP | Dispatch 49 at `0x33550` reaches `0x2fb9e`; target ID at `0x2fba3`; terminal refusals `0x2fbe0..0x2fbef`; heal argument 25 at `0x2fbf9`, helper `0x2ee3a` | First Aid passes 6 at `0x2f450` to the same helper. No separate condition cure exists in the traced handler; positive-HP Unconscious clearing belongs to healing. |
| Normal Item Use spends before dispatch, without learned-cast resource debit | Entry `0x313b7`, admission `0x313ce`; decrement `0x3145c`; caster pointer assignment `0x3146d..0x31470`; dispatch call `0x3149e`; exhausted removal `0x314a4..0x314c6`; monster response `0x314ce` | Retained DOS play establishes no time advance, failed Jump spending and last-charge removal. The item caller has no SP/gem debit. Combat restrictions and individual effect failures remain separate checks. |
| Escaping a First Aid item's target prompt grants the user 1 SP | Item dispatcher `0x39e4b`; power-4 branch `0x39e79` reaches First Aid `0x2f3f5`; ID 26 at `0x2f3fa`; prompt `0x2f0ba`; Escape branch `0x2f146`; refund `0x2f34b`, SP addition `0x2f386` | The caller set this user as caster and did not debit SP. Refund adds to caster field `0x158`, with no maximum-SP clamp or item-origin test; party gem addition at `0x2f38f..0x2f393` is zero for this spell. Charge and normal post-effect settlement remain. This does not establish other powers' refund IDs. |

**Implemented (M61 Part C):** First Aid item Escape grants the user 1 SP
without a maximum-SP clamp, retaining the spent charge. No other item power
gains this refund. Both this path and Misc staging were rechecked read-only
against the persistent Ghidra project and the same original SHA-256; bounded
instructions and query are retained in `D:\MModern-re\m61c-recheck\`.

### Item-power dispatch and variants

The 73-entry table `[0x3a114,0x3a1a6)` is indexed by `power - 1` from
dispatcher `0x39e4b`; its code-segment file base is `0x39ca0`.
The [power lookup](magic.md#item-power-lookup) agrees with the semantic targets
of all entries. Four routes illustrate why that does not mean byte-for-byte
equivalence with a learned spell:

- Power 21 reaches Wizard Eye at `0x2f8df`, inside the learned route.
- Power 33 reaches the special Frostbite handler at `0x2fc77`.
- Power 64 reaches item Incinerate `0x30b58`; learned Incinerate reaches
  `0x30f0d`. Both prepare 250 Fire damage against a single target, but the
  item supplies sound selector 13/projectile selector 2 and the learned route
  supplies 22/1. Their visible and audible presentation requires resource/play
  calibration; selector identity alone does not name an animation or sound.
- Power 55's combat branch at `0x3a058..0x3a075` calls the prohibited-use notice
  and then reaches Town Portal's handler. The pin returns after the notice.
  The [M76 check](#m76-spells) follows the downstream map guard, dialog and
  load: travel remains reachable in combat if the map permits it. This is
  static control flow, not a DOSBox observation of that anomalous route.

The pin's shared Incinerate handler therefore does not establish identical item
presentation. Its Town Portal control flow is also not a complete account of
the original item route. M75 and [M76](#m76-spells) separately trace further
cancellation and travel outcomes; untraced powers remain open.

## Party healing spells

**DOS static, high confidence (2026-10-17):** MMModern baseline `493f6f6`,
the exact original hash above, and pin `6814ee9b`. M74's hash-checking
Capstone query was reused through WSL after REA rejected the Windows path.
Fresh calibration reproduced First Aid's prompt ID 26 at `0x2f3fa` and
argument 6 at `0x2f450`, calling healing helper `0x2ee3a` at `0x2f459`.
Notes and bounded instruction slices remain outside the repository in
`D:/MModern-re/m75-healing-spells/`; `findings.md` answers the ten questions.
Existing Nature's Cure amount, Suppress Poison correction and Resurrection
refund evidence above is retained, rather than claimed as a new discovery.

| Spell ID | Dispatch -> handler | Member prompt ID location | SP / gems |
| --- | --- | --- | --- |
| 6 Create Food | `0x332fc -> 0x30642` | None | 20 / 5 |
| 7 Cure Disease | `0x33304 -> 0x302a1` | 7 at `0x302a2` | 10 / 0 |
| 8 Cure Paralysis | `0x3330c -> 0x3058b` | 8 at `0x3058c` | 12 / 0 |
| 9 Cure Poison | `0x33314 -> 0x2ffdf` | 9 at `0x2ffe0` | 8 / 0 |
| 16 Divine Intervention | `0x3334c -> 0x30d4f` | None | 200 / 20 |
| 49 Nature's Cure | `0x33550 -> 0x2fb9e` | 49 at `0x2fba3` (retained) | 6 / 0 |
| 52 Power Cure | `0x33568 -> 0x2fb10` | 52 at `0x2fb15` | 2L / 3 |
| 56 Raise Dead | `0x33588 -> 0x308fd` | 56 at `0x30902` | 50 / 10 |
| 58 Resurrection | `0x33598 -> 0x30bee` | 58 at `0x30bf3` (retained) | 125 / 20 |
| 64 Stone to Flesh | `0x335c8 -> 0x30825` | 64 at `0x30826` | 35 / 5 |
| 67 Suppress Disease | `0x3361f -> 0x2f838` | 67 at `0x2f83d` | 5 / 0 |
| 68 Suppress Poison | `0x33627 -> 0x2f62f` | 68 at `0x2f634` (retained handler) | 4 / 0 |

All 76 SP/gem entries again compare equal to the pin. These twelve dispatch
entries call their handlers directly, without a combat prohibition; charging
precedes dispatch (`0x3327b`, `0x332bb..0x332c7`). Escape passes the prompt's
ID to refund (`0x2f146..0x2f159`); SP/gem writes are `0x2f386..0x2f393`.

The shared helper refuses Dead/Stoned/Eradicated (`0x2ee61..0x2ee6e`) and
clears Unconscious only at positive HP (`0x2eeaf..0x2eeb7`). Cure Poison,
Disease, Paralysis and Stone to Flesh have no terminal refusal of their own:
helper calls `0x30013`, `0x302d5`, `0x305bf`, `0x30859` precede clears
`0x30030`, `0x302f2`, `0x305dc`, `0x30876`. Power Cure refuses terminal
targets at `0x2fb52..0x2fb61`, then makes one inclusive 2..12 draw
(`0x2fb6b..0x2fb74`, helper `0x45b9a`) multiplied by caster level
(`0x2fb78..0x2fb8d`, level helper `0x21fc9`).

Suppress Disease tests zero at `0x2f86c`, sets 1..3 to 1 at `0x2f87c`,
subtracts 3 at `0x2f88c`, then calls healing at `0x2f8a4` even for zero.
The pin also subtracts 3; its difference is skipping the helper at zero.
The follow-up also traces the complete Suppress Poison handler: zero branches
from `0x2f663..0x2f669` directly to `0x2f68a`; counts 1..3 are set to 1
at `0x2f673`, and counts 4+ subtract 3 at `0x2f683`. All selected-target
paths join at `0x2f68a` and call healing with zero at `0x2f692..0x2f69b`.
Cancellation exits before these paths. Unlike Suppress Disease, the pin's
Suppress Poison also calls healing at zero: there is no extra zero-path
discrepancy. The previously established prompt/refund and decrement differences
remain the only Suppress Poison differences in this check.
Raise Dead requires Dead (`0x30934..0x30943`), clears Dead/Unconscious and
sets HP 0 (`0x3094a..0x30954`) before healing 1 (`0x30963..0x3096c`).
Other terminal flags remain and can leave HP 0. No age write occurs.
Resurrection requires Eradicated (`0x30c25..0x30c34`), calls healing 0 while
it is still set (`0x30c3e..0x30c47`), then clears only Eradicated
(`0x30c50`): no HP refill or Unconscious clear. Both decrement the Endurance
byte before checking minimum 1 (`0x30975..0x30986`, `0x30c56..0x30c6b`):
input 0 wraps to 255, unlike the pin's integer decrement. Resurrection's
temporary age increases by 5, capped at 250 (`0x30c73..0x30c91`).

Divine Intervention ages the caster by 5 capped at 250 (`0x30d54..0x30d77`),
loops the active array at DS:`b41e`, stride `0x162`, count DS:`ed8c`
(`0x30d7f..0x30dde`), clears Cursed through Stoned while preserving
Eradicated (`0x30d91..0x30db7`), and assigns maximum HP only if not
Eradicated (`0x30dbc..0x30dd1`). It does not write SP. Create Food adds
that same unfiltered active count to the food word at DS:`eff6`
(`0x30642..0x30647`): modulo 65536, no saturation. The pin uses a wider
in-memory food field, although its save serializes 16 bits.

Item powers 26/27/37/41/44/51/56/57/66/73 call the same handlers at
`0x39f41`/`0x39f49`/`0x39fb1`/`0x39fd1`/`0x39ff5`/`0x3a038`/
`0x3a082`/`0x3a08a`/`0x3a0db`/`0x3a10c`. Their branches end at the common
return; no neighboring handler is attributed by fall-through. The item
caller debits a charge (`0x3145c..0x31462`), sets the user as caster
(`0x3146d..0x31470`), and dispatches (`0x3149e`) without SP/gem debit.
Thus the eight targeted powers refund their own spell costs on Escape;
Create Food/Divine Intervention have no member prompt. This extends the
First Aid cancellation evidence only to these traced powers.

**Combat turn order, DOS static, high confidence:** follow-up calibration
reproduced Run's 1..100 draw/strict threshold (`0x2e1c0..0x2e1d4`), combat
member removal/compaction and speed rebuild (`0x2e1dc..0x2e1eb`). Healing
handlers map a selected slot through DS:`9720`, bounded by DS:`971f`, to an
active record; they do not reinsert members or reset action flags/cursors.
`0x2dd6d..0x2dd9f` marks DS:`9707[current actor]`, increments cursor
DS:`97fe`, reads order DS:`9713` and stores actor DS:`97ff`. A disabled
slot takes the skip loop back through that same flag write. The condition
table is **`0x2de5e`**, correcting the earlier `0x2de7e` transcription:
CS offset `0x212e` plus file segment base `0x2bd30`. It skips conditions
8 and 11..15; 9/10 continue. Re-enabled before its slot, a member can act
this round; after a skipped slot, it waits for next round's flag reset.

The last disabled slot does not leave a revival opportunity before reset:
the completion predicate (`0x2cfe3..0x2d06d`) ignores unflagged terminal/
Paralyzed/Unconscious members (11..15), whereas Asleep is marked by continuing
the skip loop. The caller checks completion at `0x2ea10`, clears action and
Block flags, calls `0x2e3e5`, resets both cursors and selects again
(`0x2ea1a..0x2ea47`). The all-disabled inner path resets at
`0x2ddb6..0x2ddcf`; defeat/exit remains possible. Rebuilding speed order
alone (for example Run) is not the same as the next round's flag reset.
These rules match the pin's `Interface::nextChar`/`Combat::allHaveGone`.
They differ from current MMModern `XeenCombat.cpp::selectNext`, which starts
over and leaves skipped members unmarked; M75 Part A brings it in line
with DOS (maintainer decision). This concerns Awaken as
well as Part A's Cure Paralysis/Stone to Flesh and Part B's revival spells.

**HP-only admission, DOS static, high confidence:** zero/negative HP alone
does not deny a turn when no disabling condition remains. The party-defeat
gate `0x24f29..0x24f8f` reads only condition IDs (table `0x24f91`), not HP;
No Condition returns not defeated. Actor focus `0x1f8b6..0x1f92c` only
draws the portrait selection. Combat input `0x2e808..0x2e824` dispatches
keys through tables `0x2ecad`/`0x2ecd7`: Attack to `0x2e8c3`, Block to
`0x2e951`, Run to `0x2e920`, without an HP admission gate. Block's complete
effect `0x2e205..0x2e210` and Run's `0x2e1c0..0x2e204` do not test HP;
Attack enters `0x2c8a2`. The separate learned Cast menu condition gate is
`0x3324e -> 0x1e3bc`, followed by SP/gem checks, not a general combat menu.
The pin agrees on these admission paths. Thus Resurrection can leave HP
nonpositive without itself barring action, provided other disabling conditions
are absent and the turn-order rule permits it. This is an admission result,
not an exhaustive claim about every downstream attack/spell effect. No DOSBox
protocol remains required to settle the requested scheduling/admission questions.

## M76 spells

**DOS static, high confidence (2026-10-18)** where explicitly stated below.
MMModern baseline: clean `main`, `1bd6d5fcd084121aa92b6f047be7df31a47ca9b9`.
The installed CD `WORLD/XEEN.DAT` is 353,511 bytes with the exact SHA-256
in [Exact baseline](#exact-baseline). Pin verified locally:
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

Capstone 5.0.7, 16-bit x86, read the installed image through WSL. Work,
instruction slices and the detailed question-by-question ledger remain in
`D:/MModern-re/m76-part0/`, especially `findings.md`, `analyze.py`,
`handler-disassembly.txt`, `item-spell-actions.asm`, `maps-and-tables.py`
and `map-data.txt`. Preliminary mechanical inventories were checked and
corrected by the main analyst; `findings.md` is the final interpretation.
No original-program execution, DOSBox play, build or tests were part of this check.

The subsequent C1 and C2 implementation checks live in
`tests/XeenTravelSpellRulesTests.cpp` and `tests/XeenTravelSpellOriginalTests.cpp`.
They exercise the documented handler/Rest rules, per-character Beacon state,
map-load lifetimes and exact save v15 restore with synthetic cases and
original resources; they do not upgrade the DOS evidence labels or settle
the unknown post-item automatic-Event timing.

**Calibration passed:** item dispatch `0x39e4b` subtracts 1, bounds at 72
and indexes CS:`0474`, file `[0x3a114,0x3a1a6)`, segment base `0x39ca0`.
Power 1 reaches Light `0x2f39a`, power 4 the already established First Aid
`0x2f3f5`; power 16 uses Jump's combat notice branch. The three retained
Jump/arrival caller anchors `0x0d411`, `0x0d5ba`, `0x1e7df` again resolve
to `070d:0194`, file `0x0c924` (near calls use the caller's segment base).
Mapping remains file `=0x56c0+seg*16+off`; Ghidra linear `=file+0xa940`.
No Ghidra address was mistaken for a file offset.

### Handlers, costs and admission

All rows have **high confidence** for entry, cost and indicated writes.
SP/gem tables and shared debit/refund semantics retain their prior evidence;
the new trace establishes which M76 paths actually reach those helpers.
`L` is current level including temporary level; byte writes keep `L mod 256`.
The table's combat column is learned admission. Items share the effect
handler except their separately traced combat branches below.

| ID, spell (item power) | Effect file entry | SP/gems | Learned combat; guard/value/dialog |
| --- | --- | --- | --- |
| 3 Bless (22) | `0x2f8b5` | 2L/1 | Yes; no guard/prompt; Bless byte L |
| 30 Heroism (30) | `0x2fc4d` | 2L/3 | Yes; no guard/prompt; Heroism byte L |
| 31 Holy Bonus (25) | `0x2faf4` | 2L/1 | Yes; no guard/prompt; Holy Bonus byte L |
| 53 Power Shield (29) | `0x2ffc3` | 2L/2 | Yes; no guard/prompt; shield byte L |
| 55 Protection from Elements (14) | `0x2f6ac` | L/1 | Yes; Select Element; selected protection min(2L+5,200) |
| 12 Day of Protection (49) | `0x30e24` | 75/10 | Yes; no guard/prompt; all specified fields receive level byte |
| 13 Day of Sorcery (50) | `0x30e5a` | 40/10 | Yes; no guard/prompt; flags/counts receive level byte; automap zero |
| 42 Light (1) | `0x2f39a` | 1/0 | Yes; no guard/prompt; light increment, conditional stepped mark |
| 41 Levitate (20) | `0x2f8d1` | 5/0 | Yes; no guard/prompt; count 1 |
| 74 Walk on Water (32) | `0x2fc69` | 7/0 | Yes; no guard/prompt; flag 1 |
| 4 Clairvoyance (18) | `0x30e16` | 5/2 | Yes; no guard/prompt; flag 1 |
| 15 Detect Monster (34) | `0x30081` | 6/0 | Yes; no target guard; detection screen |
| 34 Identify Monster (23) | `0x2f8f2` | 5/0 | Yes; needs an occupied attack slot; identification screen |
| 19 Enchant Item (63) | `0x30b03` | 30/20 | No; member prompt 19, then Items mode 6 |
| 57 Recharge Item (53) | `0x30885` | 15/10 | Yes; member prompt 57, then Items mode 4 |
| 39 Item to Gold (-) | `0x30f53` | 20/10 | Yes; member prompt 39, then Items mode 11 |
| 21 Etherealize (58) | `0x30996` | 30/10 | No; flag 0x40 or full-word target 0x8888 refuses |
| 70 Time Distortion (39) | `0x30301` | 8/0 | Yes; flag 0x400 refuses; run X/Y |
| 66 Super Shelter (48) | `0x305eb` | 15/5 | No; flag 0x200 refuses; mode-12 Rest |
| 69 Teleport (42) | `0x30348` | 10/0 | No; flag 0x1000 refuses; distance dialog/lookup |
| 44 Lloyd's Beacon (28) | `0x2fc9a` | 6/2 | No; flag 0x800 refuses; Set/Return dialog |
| 71 Town Portal (55) | `0x30654` | 30/5 | No; flag 0x100 refuses; town dialog/load |

The six learned combat blocks are `0x33364`, `0x333b3`, `0x334e9`,
`0x335d8`, `0x3362f`, `0x3367e`: prior mode 2 takes the full refund
`0x2f34b` and skips the handler. Charging precedes dispatch
(`0x3327b`, `0x332bb..0x332c7`). A handler's Spell Failed has no automatic
refund. Select Element Escape (`0x2f05f..0x2f072`) refunds ID 55. The three
member prompts use their own IDs at `0x30b08`, `0x3088a`, `0x30f58`;
later item-selection cancellation has no refund call. Beacon, Teleport,
Town Portal and the shelter warning also have no refund call on cancel.

Items refuse powers 16/21/28/42/48/58 in combat: entries `0x39ed9`,
`0x39f0d`, `0x39f51`, `0x39fd9`, `0x3a015`, `0x3a092`, joining the
notice/return path. Town Portal 55 at `0x3a058` instead rejoins its handler
at `0x3a075`. All other M76 powers are admitted, including Enchant 63.
The shared item caller's retained charge debit and no-SP/gem-debit rule
therefore also applies to these new shared prompt/refund paths.

### Party effect bytes

**DOS static, high confidence (2026-10-18); same as the pin's
`Party::synchronize`.** The nine bytes keep this order in saves (v15).
Calibration: Wizard Eye writes DS:`eda0` and clears DS:`ed9f` at
`0x2f8df..0x2f8e4`; Event 69 tests DS:`ed9e` at `0x21a36`.
Offsets below are file offsets in the installed CD `WORLD/XEEN.DAT`.

| Index | DS address | Meaning | DOS anchors |
| --- | --- | --- | --- |
| 0 | `ed9e` | Levitate | Handler `0x2f8d1`; cloud fall `0x0c998`; border `0x1b9ee`; Day of Sorcery `0x30e73` |
| 1 | `ed9f` | Automap on | Wizard Eye clear `0x2f8e4`; Day of Sorcery clear `0x30e7b` |
| 2 | `eda0` | Wizard Eye | Handler `0x2f8df`; Day of Sorcery `0x30e70` |
| 3 | `eda1` | Clairvoyance | Handler `0x30e16`; Day of Sorcery `0x30e6d`; Temple Donation `0x1af07`; statues `0x1bb0f` |
| 4 | `eda2` | Walk on Water | Handler `0x2fc69`; Day of Protection `0x30e34` |
| 5 | `eda3` | Bless | Handler `0x2f8cd`; Day of Protection `0x30e3d`; Temple `0x1af26`; border `0x1bcdb` |
| 6 | `eda4` | Power Shield | Handler `0x2ffdb`; Day of Sorcery `0x30e6a`; Temple `0x1af23`; border `0x1bd10` |
| 7 | `eda5` | Holy Bonus | Handler `0x2fb0c`; Day of Protection `0x30e3a`; Temple `0x1af20`; border `0x1bd48` |
| 8 | `eda6` | Heroism | Handler `0x2fc65`; Day of Protection `0x30e37`; Temple `0x1af1d`; border `0x1bd81` |

### Consumer and dialog locators

The [magic chapter](magic.md#party-effects-and-movement) gives the settled
rules in player/mechanic terms. This table is the address index, including
confidence and comparison boundaries for the requested questions.

| Question | DOS file anchors; verdict and comparison |
| --- | --- |
| 1 | Handler table above; debit `0x3327b`, refunds `0x2f34b`. High: failed guards spend; only explicit early prompt/admission refunds. Same as pin, except Day assignments and Enchant details below. |
| 2 | Learned blocks above; items `0x39e4b..0x3a112`; Portal `0x30654..0x30819`. High: learned lists match; item 55 differs from pin by continuing to dialog/load after notice. |
| 3 | `0x21fc9..0x21fed`, buff calls `0x2f8c5`, `0x2fc5d`, `0x2fb04`, `0x2ffd3`, `0x2f6b5`, `0x30e2c`, `0x30e62`. High: current level, same as pin; DOS byte storage truncates. |
| 4 | `0x3bccf..0x3bd25`, caller `0x2dbcf..0x2dbdc`. High: Bless is added with temporary AC unless base-only requested; no condition predicate. Same as pin. |
| 5 | `0x2e0c8..0x2e0d2`; accuracy callers `0x2c917`, `0x2cb7c`. High: melee and Shoot/Poison Volley weapon accuracy, not a general spell accuracy bonus. Same as pin. |
| 6 | Retained `0x2d3a9..0x2d3f6`, generic `0x23ee5..0x23f25`. High: shield after damage saves, before secondary save; zero clamp. Same placement as pin; special death suffix bypasses final shielded amount in both. |
| 7 | Retained `0x2d2e4..0x2d374`; generic `0x23e27..0x23eb5`. High: flat party subtraction, then personal resistance save loop; monster initial Physical save precedes subtraction. Existing pin differences remain as recorded for M68/M72. |
| 8 | `0x2e0d6..0x2e0db`, `0x2e15d..0x2e165`, `0x2cb89..0x2cba1`. High: Holy Bonus adds weapon damage before difficulty scaling; prepared spell damage overrides it. Differs from pin, which omits the addition. |
| 9 | `0x2f6ac..0x2f70a`, dialog `0x2eed7..0x2f0b9`, key table `0x2f0a6`. High: capped formula, F/E/C/A, Acid means Poison, Escape refunds. Same as pin. |
| 10 | `0x1e72d`, `0x1e754..0x1e7b8`, `0x1e828..0x1e8ba`. High: reset before food/recovery; later four clears redundant. Same as pin. |
| 11 | `0x305eb..0x3061e`, `0x1e50d..0x1e5f0`, suffix `0x1e72b..0x1e826`. High: one 480-minute shelter call, no ordinary interruption opportunities; warning/cancel, dream/reset/food/arrival order match. Differs from pin for ordinary cell restriction, `0x11fe7..0x11fec`. |
| 12 | `0x2f39a..0x2f3b2`, `0x11fef..0x11ff4`, `0x12012..0x12093`; CD DAT attributes +0x200. High: increment/stepped, drain before darkness; none on open maps. Same as pin within the DOS word range; differs at Light overflow, wrapping modulo 65536. |
| 13 | `0x2f8d1`, border `0x1b9ec..0x1ba1a`; retained cloud `0x0c998..0x0c9af` and unconditional Event fall `0x0cc93..0x0ccb4`. High: presence, no step decrement, cloud protection only; reset ends it. Same as pin except Day spell producer. |
| 14 | `0x0ceeb..0x0cf26`, `0x0cf52..0x0cf8d`; retained arrival/reset. High: shallow water accepts Swimming or flag; deep requires flag; no arrival immunity. Same as pin. |
| 15 | HED prompt reads `0x24b5a..0x24bbb`, `0x25a30..0x25aa0`; frames `0x1baaf..0x1bb6a`. High for hint-byte selection/numeric frames; differs from pin's right hint-0 cycle. Emotional good/bad frame naming remains Reference only, medium confidence. |
| 16 | `0x1b9c6..0x1bdb8`; resource names `0x553ea`, `0x55401`, `0x5540a`. High: per-border-call cycles/positions in magic chapter; same as pin for requested indicators. No fixed wall-clock frequency inferred. |
| 17 | `0x30081..0x3023b`. High: 7x7 position/count screen, counts capped 3, any key/mouse; empty still paid. Same as pin. |
| 18 | `0x2f8f8..0x2faf3`. High: attack-slot guard, not combat-mode guard; up to three current monsters and five fields; no-target failure paid. Same as pin. |
| 19 | `0x30b03..0x30b55`, `0x31c76..0x31dd7`; generator `0x25ca8`. High: caster-level draw before refusal, weapon/armor material/state copy, early prompt refund only. Differs from pin in level owner, ID-34 equality, empty-result handling and retained generator draw differences. |
| 20 | `0x31bae..0x31c73`. High: eligible Misc, U(1,6), no break/destroy branch; broken cap comparison fills to 63 (cursed recharges normally). Differs from pin on broken records. |
| 21 | `0x31ddd..0x31fad`, calculator `0x3c67d`, mode-11 arm `0x3cb1d`. High: full value, no extra confirmation, no equipped/broken/cursed refusal. Differs from pin's ID threshold and retained attribute-price formula. |
| 22 | Etherealize `0x30996..0x30a0f`, Distortion `0x30301..0x30324`, Teleport `0x30348..0x3055e`, Beacon `0x2fef7..0x2ff61`, Portal `0x30779..0x307b8`; callers `0x0d5ba`, `0x314ce`, `0x2e8a5..0x2e8c0`, `0x2ea99..0x2ead1`. High for relocation/load/facing and immediate caller paths; same as pin except Portal item combat. Complete anomalous combat presentation and exact post-item automatic-Event timing remain Unknown, medium confidence; no DOS evidence label assigned to those details. |
| 23 | `0x3099b..0x30a0a`, lookup `0x11d40..0x11f03`. High: forward whole-word non-0x8888, restriction 0x40, no traversed-wall test. Same as pin. |
| 24 | `0x30301..0x30324`, run helper `0x1691f..0x1692b`. High: primary DAT run X/Y; same-map relocation both in and out of combat. Same as pin. |
| 25 | `0x303a5..0x3055e`, lookup `0x11d40..0x11f03`. High: one-digit 1..9, zero/empty cancel without refund, destination comparisons and resident-neighbor lookup; no traversed-wall test/load. Same as pin. |
| 26 | `0x2fc9f..0x2fd0b`, key table `0x2ffb3`, Return/Set `0x2feaa..0x2ffaf`. High: character map/X/Y/side, Clouds default 28 (18,4), same-map/side Return skips load. Same as pin. |
| 27 | `0x30659..0x30819`, town table `0x54054..0x54059`, run helper `0x1691f`. High: restriction 0x100, dialog 1..5, maps 28..32 and their original run positions. Same as pin apart from item-combat entry. |

The map check used existing `build-rel/mmodern.exe --inspect-map` for maps
23 and 28, then reconstructed the same read-only initial container as
`ScummVmXeenBridge::initial` to inspect all 256 attribute bytes per component.
The source was CD `GAME1.GOG`, MODE1/2352, ISO `GAME/XEEN.CC`, extent 11620,
13,438,429 bytes, SHA-256
`8c0ac4c0650080cc85e96af3e9d5cd9968dad16bfd2ed0785d77d21893cc155c`.
No resources were extracted into the repository. Internal reconstructed
container offsets for DAT 23/28/109/110/111 are
`0x2ec05/0x2fd71/0x3e6e5/0x3ea61/0x3eddd`; attributes occupy each DAT's
`[0x200,0x300)`. No drain cells occur. Ordinary Rest cell bit 0x40 does
occur on map 23; the external ledger lists every coordinate. Town run X/Y
are DAT offsets `0x32f/0x338`, not inferred town-entry coordinates.

The dawn mode/latch control flow retains its **Reference only** label;
the already confirmed temporary-reset field selection is cited, not
reopened. Likewise the existing monster damage, terrain arrival, dream
presentation, item generation and Smith-price findings retain their own
calibration and limits. See [M76 quirks](../original-quirks.md#m76-spell-findings)
for newly settled pin differences. None of this changes milestone scope,
production behavior or the save format by itself.

## Monster treasure staging

**DOS static, high confidence:** the original retains generated Misc monster
treasure in its own category. The pinned `Combat::attack2` Misc case instead
copies the temporary character's accessory record into staged accessories.
**Implemented (M61 Part C):** MMModern stages Misc in its own ten-slot array
and delivers it with the existing shared rules, preserving the generation draws
and their order. Pending Misc is retained in save envelope v12 (schema 9/content
14). Part D implements the separately traced delivery rules below.

The original attack/death routine starts at `0x2bdf8`. Its generation-result
dispatch is at `0x2c3f1..0x2c401`. The table at `0x2c592` uses code-segment
file base `0x2bd30` and selects the following branches and data destinations:

| Generated category | Branch file offset | Treasure base, relative to DS |
| --- | --- | --- |
| 0: weapon | `0x2c402` | `0x99e8` |
| 1: armor | `0x2c457` | `0x99c0` |
| 2: accessory | `0x2c4a6` | `0x9998` |
| 3: Misc | `0x2c4f5` | `0x9970` |

The Misc branch scans ten four-byte slots, testing the container field for
emptiness. It reads generated character fields `0x113`, `0x112` and `0x114`
at file `0x2c509`, `0x2c51a` and `0x2c52b`, then writes power, container and
state to DS-relative `0x9971`, `0x9970` and `0x9972` at file `0x2c513`,
`0x2c524` and `0x2c535`. These are distinct from accessory staging.

The generator independently identifies this layout: category 3 is selected at
`0x25e38`; it writes the Misc container at `0x25f68`, power at `0x26370` and
state at `0x26378`, then returns the category at `0x2637d`.
This caller/generator agreement, together with the item-use layout, calibrates
the meaning of the fields and excludes an accessory/Misc labeling mistake.

The conclusion is deliberately about **generation and staging**. It does not
prove full-pack delivery, recipient eligibility, overflow handling or every
monster's resource-defined drop chance in DOS. Those require separate traces; delivery is now covered below.

## Treasure delivery

**DOS static, high confidence; implemented (M61 Part D).** The 2026-10-12
playable-rules report and `evidence-index.json` are retained under
`D:\MModern-re\playable-rules-20261012\`. Its complete delivery record is
`ev_40c5c299c56c33aa600c91e27b2dce7650858004fffc25e6852ad8a9b02d6aba`;
sort is `ev_2730d9bb0d263ec2078ba8af76a26631ece50c58366b4365588ecc509de4024b`.
Part D rechecked the same SHA-256 read-only in the persistent Ghidra project;
query and instruction/table results remain in `D:\MModern-re\m61d-recheck\`.
Calibration: First Aid passes 6 at `0x2f450`, and known inventory record strides
and category arrays agree. Actual switch words were checked independently of
Ghidra's inferred switch annotations. These are static findings, not DOSBox play.

- Capacity helper `0x2493c..0x249be` checks all four category tails across active
  members. The delivery loop checks before every slot at `0x2431b`.
- Full packs clear four 40-byte item arrays at `0x2433e..0x2437b`. The bounded
  fill helper does not touch staged currency. Gold/gems are credited at
  `0x2486a..0x24884` and only then cleared at `0x24888..0x24896`.
- Preferred index validity is checked at `0x243d5..0x243f2`. Valid preferences
  use capacity/condition tests, then an eligible scan. Invalid/absent preferences
  branch directly to `0x24502..0x24565`, the condition-independent capacity scan;
  this is also the final fallback when eligible candidates fail.
- Exhausting this category's capacity selects member zero at `0x24567`.
  Ordinary item writes change three bytes, retaining the destination frame:
  weapons `0x245aa..0x245cc`, armor `0x2462b..0x2464d`, accessories
  `0x246a0..0x246c2`, Misc `0x24714..0x24736`. Sort at `0x38ae8` leaves a
  fully occupied nine-slot category unchanged. The replacement mark therefore
  survives. This finding does not establish arbitrary Misc hole/move behavior.

Special weapon ID exactly 34 has a separate full-pack exception (`0x24327`);
the pin tests at least 34. That remains outside ordinary reward content and
Part D's correction. No overflow storage or extra random draw is introduced.

**M63 Part B, DOS static:** item transfer precedes the blank one-tick hold
(`0x247a4..0x247ab`), sorting/found text precedes the ten-tick hold
(`0x247ed..0x247f4`), and a separate final input precedes currency
(`0x24830..0x24884`). Quest counts are already granted before their common
treasure display. The raw-ID name lookup at `0x245f8..0x24610` uses the shared
far-pointer table base `0x4fc14`; IDs 82..116 occupy `[0x4fd5c,0x4fde8)`.
All thirty-five names' pointers, terminators and segment `0x481d` were checked
read-only against the same identified DAT; the star branch is `0x2476e`.
Derived bounds/identity are retained in the external Part B
`quest-name-metadata.json`; original names remain loaded from the installation.

## Other playable-rules confirmations

The same external report/index retains calibrated, independently checked reads
for the following **DOS static, high-confidence** findings. Only delivery changes
production behavior in Part D; the other findings update the rules reference.

| Finding | Original file offsets and boundary |
| --- | --- |
| Natural 20 damages before the ordinary comparison and fresh AC read | `0x2db45`, `0x2db55`, `0x2db65`, `0x2dbd7`; injury `0x1f8a7` and armor bad-state gate `0x21275..0x21281` confirm armor breakage affects the later check. |
| Broken weapons retain base dice and equipped effectiveness | Bad-state material skip `0x2e057..0x2e05d`, base dice `0x2e0e9..0x2e134`, frame-only effectiveness `0x2c950`, selector `0x2c96f`, multiply `0x2caed..0x2cb00`. Break Weapon itself clears frame at `0x2d52c..0x2d532`; that ability is not admitted. |
| Hatred 1 bypasses preference | Compare/branch `0x2d84b..0x2d852` reaches random dispatch `0x2d9a3`; this does not establish every party-size distribution. |
| Run uses strict less-than | Inclusive draw `0x2e1c0..0x2e1c4`, unsigned threshold `0x2e1cc..0x2e1d0`, comparison/failure `0x2e1d2..0x2e1d4`. Threshold 100 accepts outcomes 1..99; no measured DOS RNG uniformity claim. |
| Monster recovery differs from the pin | Caller `0x20ca9..0x20cf1`, helper `0x2e19b`, comparison `0x2e1b2..0x2e1b4`, clear `0x20ce1`: DOS clears at draw `<= ID`, the pin at `>= ID`. Not reachable with currently admitted monster states. |
| Long time calls apply economy once | Calendar `0x263b3..0x26491`, stock `0x26401`, interest `0x26408`, latch `0x2640b..0x26411`, interest arithmetic `0x26492..0x264c4`. Confirmation is limited to word-sized arguments, including current 1,440/2,880-minute calls; a synthetic 144,000-minute call is not representable in DOS. |

## Monster damage and secondary attacks

**2026-10-15, M68 Part A; DOS static, high confidence.** Same executable hash
as above. Bounded Capstone 16-bit reads of file offsets, calibrated against the
already confirmed Break Weapon site `0x2d52c..0x2d532`, the natural-20 caller
`0x2db55 -> 0x2d22d` and the character record layout (conditions at character
offset `0x143`, current HP `0x156`, current SP `0x158`, temporary age `0x26`,
item categories at `0xa7`, `0xcc`, `0xf0`, `0x114` with 4-byte slots).

| Finding | Original file offsets and boundary |
| --- | --- |
| `doCharDamage` entry, Sleep clear, dice loop | `0x2d22d`; `mov byte [0x14b], 0` at `0x2d236`; `strikes` from monster offset `0x1a`, each `random(1, dmgPerStrike)` at `0x2d26c..0x2d27d`. |
| Initial save uses the Physical selector | `0x2d2e4` pushes 0, calls `0x2d0c2`, halves on success at `0x2d2f7`; the type switch at `0x2d2f9..0x2d35c` subtracts party protection for types 2-5 and selects the frame/sound. |
| Element save loop | `0x2d35e..0x2d374`: push type, call `0x2d0c2`, `je 0x2d376`, `sar di,1`, `or di,di`, `jne 0x2d35e`. Drawn once even at zero damage. |
| Power Shield and clamp | `0x2d3a9..0x2d3b5`; zero damage jumps to the subtraction at `0x2d6b5`. |
| Secondary save and dispatch | special byte test `0x2d3e3`, Physical save `0x2d3ed..0x2d3fe`, `special - 5` bound `0x11` at `0x2d433`, jump table `0x2d6d9` (entries for specials 5..22). |
| Handlers | Poison `0x2d442`, Sleep `0x2d462`, Disease `0x2d4a5`, Insane `0x2d486`, Confuse `0x2d4c4`, Curse `0x2d4e4`, Break Weapon `0x2d508`, Weaken `0x2d541`, Drain SP `0x2d561`, Paralyze `0x2d56e`, Aging `0x2d592`, Unconscious `0x2d5a2`, Eradicate `0x2d5c6` (helper `0x2bd6a`), Stone `0x2d611`, Death `0x2d641`, Curse Item `0x2d671`; specials 6 and 11 map to `0x2d6b5`. |
| Saving throw helper | `0x2d0c2`: selector 0 adds level and `statBonus(Luck)` and draws `1..v + 20` (`0x2d0fe`); selectors 1-6 add the resistance pair and `itemScan` bonus and draw `1..r + 40` (`0x2d202`); success when the draw is at most the value (`0x2d212..0x2d21d`). |

## REA record locators

The exported evidence bundle contains complete records. The stable IDs below
locate the relevant image, bounded reads and routine inspections; they are not
replacements for the control-flow interpretation above.

| Record | Evidence ID |
| --- | --- |
| Verified load image | `ev_ef399a290c4b7c75504981f121b45825eb0386c1b063506e371c1e9a54f5d8ff` |
| Resurrection instructions | `ev_be37f34557e6137b01871640d0436763d6b5b7534c3832938210451d6252c7a4` |
| Nature's Cure instructions | `ev_aae374dd20efa81db12498f505e7590bf2c03a1a88e5e4216bbc880508002567` |
| Refund instructions | `ev_197465b8f6dafd646483c20094ac5dbcddca29a7b04a89d890f1a1ef158bbc51` |
| Item-use caller bytes | `ev_f31b8ab9b94c617efcb72ddd337262a8b561f02371bf50cfaa90a689a5083645` |
| Item dispatch and map bytes | `ev_cdca8f91f7fa96ee3478931b5f8f1cf43ea6a2bfb85cb3cbd70a44d81514a682` |
| Target-dialog bytes | `ev_1230c42cf3898996c5c6df763e2dc44f14ea902cc88d79018ec25f89d2fc2f79` |
| Healing-helper bytes | `ev_2d8022d539f88b97d38fb1ede3ed1462128f5a0c95ea192d41f0f6ff9ac7a63e` |
| Complete cost-table bytes | `ev_4e599ced8b32116289df9e5171b5833a20d3fd222e759b8dd2cdfb19b272ccc5` |
| Treasure-category branch bytes | `ev_05335aaa0db1cc046e56d244169faf096e294362721b458eb532fda188afe7ad` |

## Highest-value remaining questions

| Area | Question to settle before extending a rule |
| --- | --- |
| Combat | Establish remaining enchanted multi-attack arithmetic. Eradicate equipment-frame behavior is settled above. |
| Damage and targeting | Confirm ranged continuation, elemental multi-attack arithmetic and targeting distributions for every party size. |
| Items | Trace the remaining unexamined item-prompt refunds and high-tier generation cases; M76 settles Recharge, Enchant and Item to Gold above. |
| Treasure | Extend delivery evidence to the special sword and other editions' quest branches; the Clouds quest display/count boundary is established above. |
| Magic | Extend control-spell duration and high-tier overflow checks. Holy Bonus is settled by [M76](#m76-spells); Poison Volley and Resurrection retain their earlier evidence. |
| Travel | Calibrate complete DOSBox presentation of Town Portal's anomalous combat item route and exact post-item automatic-Event timing. M76 settles handler relocation/load/admission; terrain keeps its earlier evidence. |
| Time | Verify remaining script-mode dawn behavior and uncalibrated condition-byte edges. Current MMModern wrapping/saturation is not literal equivalence with the pin's live integer counters; see the time chapter. |
| Broader coverage | Event opcode catalogue, all services, AI/movement, character creation, Darkside/World transitions, audio and frame-level UI timing still need dedicated chapters and calibration. |

These are evidence gaps, not instructions to broaden an active milestone.
Normal DOS play or calibrated static analysis should settle reachable disputes;
only the condition-counter fallback explicitly authorized in
[AGENTS.md](../../AGENTS.md) bypasses an otherwise unverifiable case.

### NPC CD invocation and window lifetime

**DOS static, 2026-10-13.** The invocation-local playback guard is initialized
at `0x256ec`, tested/set at `0x25ae8..0x25af3`; page and input retries retain it.
Mode 2 plays at `0x25aa8` and bypasses stop, while ordinary dismissal stops at
`0x25c94`. The all-zero request returns without changing playback at
`0x3da98..0x3daa7`. WhoWill retains window 11 (`0x251a8..0x251bf`); Event
cleanup closes all windows (`0x28e92`, `0x0dd80`) without stopping CD.

The original cue operands are audio globals, retained across normal play/stop
and same-disc Event boundaries; disc initialization clears them (`0x3d7fa`,
`0x3d800`, `0x3d806`). The installed maps 28 and 23 contain 29 NPC records,
each with an immediately preceding same-context opcode 60, including all three
mode-2 offers. This inventory establishes the normal supported paths, not every
possible branch target or future map. Audio intervals and visual continuations
are transient; it does not establish a new durable save field. The read-only
follow-up and record metadata remain in the external capture work directory.
