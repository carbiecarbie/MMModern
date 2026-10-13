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
[M61 price evidence](../milestone-61-plan.md#original-program-price-evidence-and-confidence).

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
`C:/Users/carbo/.codex/visualizations/2026/10/13/01a135ee-f0f6-78c2-a1fa-49986cff774e/m63-calibration-audio/`.
The superseded silent captures and their measurements remain in the adjacent
`m63-calibration/` directory for provenance.

Death and NPC recordings require separate Part B analysis. Their shared clock
must support independent two-/three-tick waits and sampled sound-active intervals;
death fire has no proved fixed tick hold.

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
`C:/Users/carbo/.codex/visualizations/2026/10/13/01a135ee-f0f6-78c2-a1fa-49986cff774e/m63-typematic/`.

## Spell and item control flow

The following findings are **DOS static, high confidence**, confined to the
stated paths. They were independently checked against the instruction slices
and pinned source. Existing implementation tests are not substitutes for these
original-program checks.

| Finding | Original anchors | Calibration and boundary |
| --- | --- | --- |
| All 76 SP costs and all 76 gem costs agree with the pin | Signed-word SP table `[0x4f752,0x4f7ea)`; gem-byte table `[0x4f7ec,0x4f838)`; cost lookup `0x2f2de..0x2f2e9`, refund lookup `0x2f353..0x2f35f` | Comparison against `SPELL_COSTS` and `SPELL_GEM_COST` has zero differing entries. The two-byte gap is not a gem entry. This establishes the tables, not every Guild or cost-overflow rule. |
| Resurrection cancellation refunds its own cost | Global dispatch 58 at `0x33598` reaches `0x30bee`; target call supplies 58 at `0x30bf3`; cancel return `0x30bff..0x30c01` | First Aid supplies 26 and Suppress Poison supplies 68 to the same prompt/refund convention. DOS refunds 125 SP/20 gems; the pin supplies Raise Dead 56 and refunds 50 SP/10 gems. Resurrection's recovery effect remains a separate unverified question. |
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
  **Unknown:** whether the downstream DOS handler ultimately allows any travel
  in combat. This observation must not be rewritten as permission to teleport.

The pin's shared Incinerate handler therefore does not establish identical item
presentation. Its Town Portal control flow is also not a complete account of
the original item route. Full non-First-Aid cancellation and travel outcomes
remain open.

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
the pin tests at least 34. Quest presentation also has separate branches. These
are outside current ordinary reward content and Part D's production correction.
No overflow storage or extra random draw is introduced.

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
| Combat | Establish remaining enchanted multi-attack arithmetic and Eradicate equipment-frame behavior. |
| Damage and targeting | Confirm ranged continuation, elemental multi-attack arithmetic and targeting distributions for every party size. |
| Items | Trace all other item-prompt refunds; Recharge, Enchant, Item to Gold, cursed refusals and high-tier generation. |
| Treasure | Extend ordinary delivery evidence to special sword/quest branches before admitting them. |
| Magic | Establish Resurrection's actual recovery, Holy Bonus's combat contribution, control-spell duration, Poison Volley arithmetic and high-tier overflow. |
| Travel | Settle Town Portal item behavior after its combat notice; complete Beacon, Teleport, Etherealize and terrain-arrival paths. |
| Time | Verify remaining script-mode dawn behavior and uncalibrated condition-byte edges. Current MMModern wrapping/saturation is not literal equivalence with the pin's live integer counters; see the time chapter. |
| Broader coverage | Event opcode catalogue, all services, AI/movement, character creation, Darkside/World transitions, audio and frame-level UI timing still need dedicated chapters and calibration. |

These are evidence gaps, not instructions to broaden an active milestone.
Normal DOS play or calibrated static analysis should settle reachable disputes;
only the condition-counter fallback explicitly authorized in
[AGENTS.md](../../AGENTS.md) bypasses an otherwise unverifiable case.
