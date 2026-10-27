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
`D:\Projetos\MModern\MModern-re\rules-20261012\`: `rea-evidence.json`, `query.py`,
`spell-cost-comparison.txt`, `item-mapping.txt`, `monster-loot.txt` and bounded
instruction slices. `query.py` checks the exact original hash before reading.
These local paths describe provenance, not dependencies required to build
MMModern. Reproducing the original checks requires the owner's game installation.

Earlier evidence is retained under `D:\Projetos\MModern\MModern-re\20261011-rea-ghidra\` and
the external `D:\Projetos\MModern\MModern-re\ghidra-xeen-dat\WorldXeenDat.gpr` project.
Its authoritative summaries and calibration are in
[original quirks](../original-quirks.md) and
[Smith price rules](items.md#smith-prices).

## Outdoor movement skills

**DOS static, high confidence (2026-10-19).** MMModern baseline: clean
`main`/`origin/main`, `612a4ac42045e331bdf6df7d9b753f40271a1ba5`.
The pin's HEAD was verified as `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
REA opened the installed CD `WORLD/XEEN.DAT` and reported the SHA-256 in
[Exact baseline](#exact-baseline); an independent SHA-256 check agreed.
Hash-checked Capstone 16-bit reads of that installed file verified the
branches below. Tables were read as little-endian words, not instructions.
Earlier external notes in `D:/Projetos/MModern/MModern-re/skills-20261019/` supplied leads;
their conclusions alone were not treated as new evidence.

Calibration uses the settled two-holder rule: the complete shared helper
`0x215b9..0x21609` indexes table `0x2160a` with the supplied skill ID.
The table's segment file base is `0x21040`; entries 9 and 11 both resolve
to the two-holder comparison at `0x215e9`. Entries 5 and 14 resolve to
the whole-party comparison at `0x215f0`; entry 10 resolves to the first-holder
success at `0x215e4`. This distinguishes Pathfinder (11) from Navigator (10).

| Rule | Original file offsets and conclusion |
| --- | --- |
| Conditions | `0x215c6..0x215d5` reads each active member's skill byte (record stride `0x162`) and counts a nonzero value. The complete helper reads no HP or condition fields and calls no condition helper. Dead and unconscious active members count, including for Mountaineer and Pathfinder. |
| Outdoor wall dispatch | `0x0cec4..0x0cedf` selects the outdoor path and indexes the current wall word through table `0x0cfc1`, with segment file base `0x0c790`. Entries 1/7/9/10/12 resolve to `0x0cf29`; entries 3/6 resolve to `0x0cf38`; entry 5 resolves to `0x0cee4`. |
| Mountaineer | `0x0cf29..0x0cf35` supplies skill 9 to the calibrated helper. Wall 5 checks the side at `0x0cee4..0x0cee9`: Clouds continues to surface handling, Darkside to Mountaineer. Agrees with the pin. |
| Pathfinder | `0x0cf38..0x0cf44` supplies skill 11 to that same helper. Two holders permit outdoor wall types 3 and 6; failure reaches the shared refusal. This is a positive movement consumer, absent from pinned `Interface::canMove`. |
| Refusal | `0x0cf46..0x0cf50` requests FX 21 and returns false. Success returns true at `0x0cfbc..0x0cfbf`. The four callers at `0x0d317`, `0x0d355`, `0x0d3a0`, `0x0d3d8` branch on false directly to `0x0d5c7`, skipping position writes, chargeStep (`0x1d838`) and arrival (`0x0d411`). No action time or explicit action-driven monster opportunity is charged. Ordinary redraw scheduling still runs. |
| Navigator | `0x0c981..0x0c995` supplies skill 10 and skips the outdoor desert `addTime(170)` when present. This retained arrival rule calibrates the distinct skill ID; it is not the wall-3/6 consumer. |

The pin's `party.cpp::checkSkill` agrees on aggregation and condition handling.
Its `interface.cpp::canMove` omits wall cases 3 and 6, which pass through
the default path. MMModern implements the DOS consumer in `XeenMovement`;
`XeenSensesRulesTests` and `XeenSensesOriginalTests` cover holder counts and
both original map-23 crossings without widening area admission.
See [movement rules](time-and-conditions.md#outdoor-movement-skills) and
[the indexed difference](../original-quirks.md#sv-pathfinder-movement).
These are static control-flow findings, not DOSBox observations. Wall numbers
are raw outdoor wall types, not surface types or a newly inferred forest rule.
The [lighting and travel check](#darkness-and-lighting-continuations) and
[fall integration](#fall-and-outdoor-edge-integration) extend these bounded reads.

## Darkness and lighting continuations

**DOS static, high confidence (2026-10-19)** for the paths stated here.
Same executable/hash and pin as [the skill check](#outdoor-movement-skills).
Work scripts, instruction slices and derived inventories remain outside the
repository in `D:/Projetos/MModern/MModern-re/m78-part0/` (`query.py`, `maps.py`,
`map-inventory.txt`, `inventory-summary.txt`). Calibration reuses the settled
drain latch, Light producer and two-holder rule; these were not re-established
from an uncalibrated bit search. Near-call targets were checked from their
signed displacements and caller segment; embedded jump tables were read as data.

| Question | Original file offsets and conclusion |
| --- | --- |
| Dark flag | `0x12077..0x12093` selects obscurity 0 when the light word is zero and primary map flags2 (`DS:c0d8`, DAT +`0x30c`) has bit `0x4000`; otherwise it selects 4. The map record base is `DS:bdcc`. |
| Independent meaning check | Renderer `0x3fd4a..0x3fd7a` accepts that obscurity: 0 clears pixels to palette index 0; 4 does nothing. Start `0x0a08`, 108 words per row, 104-byte row gap, 132 rows establishes the rectangle `[8,224) x [8,140)`. Independent spell site `0x2f39e..0x2f3a5` treats obscurity 0 as black when deciding Light's stepped mark. These are independent semantic checks downstream of the flag test, not a claim that two direct reads of the flag mask were found. |
| Partial obscurity | The renderer supports levels 1/2/3 through translation tables (`0x3fd7b..0x3fe0e`). Ordinary map lighting writes only 0/4. The distinct UI fade `0x39ada..0x39b11` traverses 4..0. Partial shading is not a light-counter gradient on a dark map. Agrees with the pin. |
| Layers | Scene and monster composition precede the obscurity call at `0x11622..0x1162b`. Minimap drawing (`0x13b27`, called at `0x1161d`) uses the right-hand panel (base sprite at `(232,9)`, `0x13b60..0x13b76`), outside the rectangle. The Cartographer exploration write precedes the dark test (`0x1203c..0x12074`), so darkness does not suppress it. Full Map has its separate entry `0x130ed`; neither map path calls the obscurer. Border/indicators assemble afterward at `0x117b1`, so they remain visible. |
| Check and clear | Indoor scene setup calls lighting at `0x12190`; outdoor setup calls it at `0x1a81a`. Redraw calls those setup paths, then clears stepped (`DS:3067`) at `0x1184f`. With stepped zero, `0x12012` branches over lookup/decrement to `0x12077`, deriving obscurity only. A pending step is therefore consumed during scene preparation; an ordinary subsequent redraw cannot invent another step. |
| Turns and blocked moves | Turns join the arrival suffix at `0x0d38d..0x0d411`; `0x0c92b` sets stepped even without displacement. Wait reaches that suffix at `0x0d468..0x0d482`. False canMove branches bypass it. Combat turns separately set stepped at `0x2e896`. |

The two direct far calls to the obscurer found in the relocation-backed call
inventory are scene redraw `0x11626` and the UI fade `0x39aeb`; the indoor
lighting call is a near call, verified separately. The scene's fixed rectangle
also explains why darkness cannot hide the minimap or border. The pin's
`Party::handleLight`, scene setup and `Interface::obscureScene` agree on these
ordinary lighting paths. These findings do not certify every UI fade or a
particular DOSBox frame rate.

### Travel lighting checks

Checks below mean the shared lighting routine reached through scene preparation,
not an extra terrain-arrival call. More than one visible continuation can occur
inside a single outer action; do not collapse them to one final-destination check.

| Family | Stepped and scene-check continuation (high confidence) |
| --- | --- |
| Walk/turn/wait | Shared arrival sets stepped (`0x0c92b`); outdoor normalization may load (`0x0c9df..0x0c9f4`); the next scene preparation checks the destination. Terrain-damage presentation can draw earlier (`0x0ca04`). |
| Event teleport, including mirrors/stairs | `0x270b9` marks stepped even on same-map travel. Continue explicitly redraws at `0x27121` before restarting the destination Event chain. Exit returns to its outer caller. A later outer arrival can mark stepped again. Each Continue redraw can check an intermediate destination. |
| Learned travel spells | Successful Cast reaches arrival at `0x0d5b4..0x0d5ba`, marking stepped. Jump also marks it in its handler (`0x2f82b`); Portal always loads, Beacon Return loads across map/side changes, and map load marks stepped at `0x0f58b`. Etherealize/Teleport handlers themselves only relocate. Time Distortion explicitly redraws after relocation (`0x3030f..0x30323`) before its caller's suffix. |
| Item travel | Retains the established item/learned distinction: item response `0x314ce` has no terrain-arrival call. Jump/map-load marks still apply; a used item closing the character sheet sets stepped at `0x3a493`, with the outer sheet caller testing it at `0x0d4be`. Do not infer lighting solely from the absence of an item terrain call. [M84](#event-dispatch) later settled the exploration post-item automatic-Event timing: Use stays inside the sheet's input, and the one ordinary pass follows the sheet's close and optional monster step ([rule](world-events.md#event-dispatch)). |
| Surface/scripted fall | Source views are drawn at `0x0ca91`/`0x0ca9e`; destination load at `0x0ccc2` marks stepped. Scripted fall normalizes outdoor coordinates and explicitly redraws at `0x0cced` before its damage. Surface-fall damage presentation uses the shared arrival draw; see the fall integration below. |
| Run | Relocates to the current map's run position (`0x2eb1f`, helper `0x1691f`), without map loading. Combat exit redraws at `0x2eb7f`, then marks stepped at `0x2eb8f`; the following scene check uses the run destination. It does not invoke terrain arrival directly. |

### Restore boundary

**DOS static, high confidence:** map load sets stepped (`0x0f58b`), and startup
loads the current map (`0x0d08c`) before scene redraw (`0x0d0da`). Thus a blanket
claim that every DOS load/redraw bypasses the drain branch is unsupported:
redraw calls lighting, and the branch depends on pending stepped state.
The full native saved-game entry/continuation was not established here; do not
upgrade its no-drain behavior to a DOS-confirmed rule. Pinned native save loading
also calls `Map::load`, which sets stepped, before the later scene check.

**MMModern current-format policy, not native-DOS-save evidence:** exact snapshot
restore must retain the saved light word and derive darkness without replaying
an arrival or manufacturing a step. `XeenSaveState::restoreJourney` copies the
saved encounter context; `XeenWorld::swapPreparedState` clears the transient
lighting step before presentation. `XeenTraversalLightingTests` checks exact
restore and redraw on a synthetic drain cell. Any claim
that native DOS restore itself never reaches drain still needs clarification/
evidence before being used as a fidelity rule.

## Fall and outdoor edge integration

**DOS static, high confidence (2026-10-19)** for the normal valid-neighbor paths;
the [settled fall table](time-and-conditions.md#terrain-arrival) remains unchanged.
Surface fall uses that table and preserves or sets facing exactly as listed.
Explicit Event falls instead assign their script map/X/Y at `0x0cca5..0x0ccb4`,
with no additional facing assignment. Both reach the ordinary map loader at
`0x0ccc2`; there is no invented route-specific map translation.

Scripted falls with outdoor bit-4 coordinates call edge normalization after
loading and before destination redraw/damage (`0x0cccc..0x0cce1`). Surface
arrival also normalizes after its fall handling (`0x0c9df..0x0c9f4`). The shared
normalizer `0x0f454..0x0f541` adjusts Y first: negative adds 16 and selects South;
nonnegative subtracts 16 and selects North (`0x0f48c..0x0f4d4`). X then uses
the selected neighbor: negative adds 16/West, nonnegative subtracts 16/East
(`0x0f4d8..0x0f525`). It writes adjusted coordinates before loading the selected
nonzero map (`0x0f529..0x0f53d`). Facing is unchanged. This is a bit-4 test and
one adjustment per axis, not an arbitrary modulo for malformed coordinates.
Pinned `Map::getNewMaze` agrees. Missing-neighbor/malformed inputs are not
certified by this normal-path check.

## Clouds darkness and traversal resource inventory

**Original resource inspection (2026-10-19), high confidence for initial data.**
Read installed `GAME1.GOG` as MODE1/2352 ISO, `GAME/XEEN.CC` extent 11620,
13,438,429 bytes, SHA-256
`8c0ac4c0650080cc85e96af3e9d5cd9968dad16bfd2ed0785d77d21893cc155c`.
The same initial-container block sequence as the existing loader supplies the
DAT/EVT records. Each DAT was required to be 892 bytes with matching map ID;
flags2 is at +`0x30c`, cell attributes at `[0x200,0x300)`. No game bytes were
copied into the repository, and no generated census was changed.

There are **128 DAT maps**: 85 primary IDs and 43 indoor neighbor tiles.
**36 have the dark flag**: 33-45, 71, 79-84, 115-129 and 142. The first
20 are primary maps; the other 16 are physical indoor neighbor tiles.
**No initial DAT map has a drain cell: 0 maps, 0 cells**, over 32,768 cells.
This is not a claim about arbitrary runtime cell edits, saved mutations or
Darkside. There is no natural initial Clouds drain-cell play-test witness.

The engine census now reports dark primary maps, dark physical DAT tiles,
drain cells (including the zero-use Clouds row), and each outdoor skill wall
consumer. `XeenSensesOriginalTests` also checks all byte-ID Clouds Event files
for drain-bit operands in AlterCellFlag; none writes the drain bit. That
operation changes the indoor surface's low three bits (outdoors, a wall
nibble); other map-edit operations change walls, not attribute bit `0x20`.
Drain behavior therefore uses synthetic fixtures, not a claimed natural
Clouds Event witness.

- Mountaineer teacher: Rivercity, logical map 30 `(30,30)` North (physical
  tile 114 `(14,14)`). `maze0030.evt` record @228 takes 5,000 gold (mode 34),
  @241 grants skill 9 (mode 13) to the selected member. The teacher chain
  starts at @199; the grant is resource evidence, not a DOSBox observation.
- Mountain crossing: map 23 `(2,12)` North to `(2,13)`, raw wall 1.
- Pathfinder crossing: map 23 `(1,1)` East to `(2,1)`, raw wall 3.
  Both crossings start at wall 0 and retain the same ordinary surface on both
  cells (grass 2 and dirt 1 respectively); neither endpoint has an Event
  record. Monster positions remain ordinary gameplay state, not a safety claim.

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
| Dream samples | Matched `dreams2` onset about 7.690 s, end 9.355 s; `laff1` onset about 9.356 s, end 11.323 s (audio PTS) | Sequential, approximately 1.665 + 1.967 s. The adopted unsplit driver model ([approved approximation](../original-quirks.md#dev-prosnd-dma-timing)) gives 24,471 and 28,916 samples at fixed `1000000/68` Hz (1.664028 + 1.966288 s); the capture cannot resolve the sub-millisecond difference from header lengths. |
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
`D:/Projetos/MModern/MModern-re/codex-m63-evidence/m63-calibration-audio/`.
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
| Death laugh | Original sample template matches audio at 11.772792..14.028556 s | The former header estimate is 33,173 samples (2.255764 s); the adopted driver count is 33,166 samples at fixed `1000000/68` Hz (2.255288 s). Output displacement is not an extra emulated tail. |
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
`D:/Projetos/MModern/MModern-re/codex-m63-evidence/m63-part-b/`.

### NPC CD invocation and window lifetime

**DOS static, high confidence, 2026-10-13.** Bounded read of the baseline
`WORLD/XEEN.DAT`; the calibrated Ghidra export was cross-checked against an
independent i8086 decoding of the complete NPC function and its CD helpers.
The invocation-local playback guard is initialized
at `0x256ec`, tested/set at `0x25ae8..0x25af3`; page and input retries retain it.
Mode 2 plays at `0x25aa8` and bypasses stop, while ordinary dismissal stops at
`0x25c94`. The all-zero request returns without changing playback at
`0x3da98..0x3daa7`. WhoWill retains window 11 (`0x251a8..0x251bf`); Event
cleanup closes all windows (`0x28e92`, `0x0dd80`) without stopping CD.

The original cue operands are audio globals, retained across normal play/stop
and same-disc Event boundaries; disc initialization clears them (`0x3d7fa`,
`0x3d800`, `0x3d806`). The initial maps 28/23 inventory contained 29 NPC records. The complete
Clouds inventory also includes maps 30 and 49; all cue addresses and boundary
failures are recorded in [Audio](audio.md#cd-disc-addressing-and-boundary-failures).
This inventory establishes the supported paths, not every future map. Audio intervals and visual continuations
are transient; it does not establish a new durable save field. The read-only
follow-up and record metadata remain in
`D:/Projetos/MModern/MModern-re/codex-m63-evidence/m63-part-b/npc-evidence/`.

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
`D:/Projetos/MModern/MModern-re/codex-m63-evidence/m63-typematic/`.

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
instructions and query are retained in `D:\Projetos\MModern\MModern-re\m61c-recheck\`.

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
`D:/Projetos/MModern/MModern-re/m75-healing-spells/`; `findings.md` answers the ten questions.
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
M75 Part A brought MMModern's `XeenCombat.cpp::selectNext`, which used to
start over and leave skipped members unmarked, in line with DOS (maintainer
decision). This concerns Awaken as well as M75's Cure Paralysis/Stone to
Flesh and revival spells.

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
`D:/Projetos/MModern/MModern-re/m76-part0/`, especially `findings.md`, `analyze.py`,
`handler-disassembly.txt`, `item-spell-actions.asm`, `maps-and-tables.py`
and `map-data.txt`. Preliminary mechanical inventories were checked and
corrected by the main analyst; `findings.md` is the final interpretation.
No original-program execution, DOSBox play, build or tests were part of this check.

The subsequent C1 and C2 implementation checks live in
`tests/XeenTravelSpellRulesTests.cpp` and `tests/XeenTravelSpellOriginalTests.cpp`.
They exercise the documented handler/Rest rules, per-character Beacon state,
map-load lifetimes and exact save restore with synthetic cases and
original resources; they do not upgrade the DOS evidence labels or settle
the post-item automatic-Event timing, which [M84](#event-dispatch) later
settled for exploration.

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
`Party::synchronize`.** The nine bytes keep this order in the current save
format.
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
| 22 | Etherealize `0x30996..0x30a0f`, Distortion `0x30301..0x30324`, Teleport `0x30348..0x3055e`, Beacon `0x2fef7..0x2ff61`, Portal `0x30779..0x307b8`; callers `0x0d5ba`, `0x314ce`, `0x2e8a5..0x2e8c0`, `0x2ea99..0x2ead1`. High for relocation/load/facing and immediate caller paths; same as pin except Portal item combat. Complete anomalous combat presentation and exact post-item automatic-Event timing remain Unknown, medium confidence; no DOS evidence label assigned to those details. [M84](#event-dispatch) later settled the exploration timing ([rule](world-events.md#event-dispatch)); the combat item route remains open. |
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

## M77 combat items, Quests, Awards and skill indicators

**DOS static, high confidence (2026-10-18)** where stated below; this is a
read-only analysis, not DOSBox execution. MMModern baseline: `main`
`e63502b`. Installed CD `WORLD/XEEN.DAT`, 353,511 bytes, SHA-256 as in
[Exact baseline](#exact-baseline). Pin verified locally:
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Capstone 5.0.7, 16-bit x86,
through WSL; the instruction slices, tables and helper script remain in
`D:/Projetos/MModern/MModern-re/m77-part0/` (`x.py`, `items.asm`, `itemaction.asm`,
`quests.asm`, `quests-tail.asm`, `awards.asm`, `awards2.asm`,
`danger.asm`, `thinwall.asm`, `checkskill.asm`). REA's Windows Ghidra
provider refused the DOS MZ image, as in M76.

**Calibration passed.** The Items dialog entry `0x3a652` is the routine M76
already identified through its mode arguments 6/4/11 (`0x3a7ae..0x3a874`
selects the Enchant/Recharge/Gold button sets). The item-use caller
`0x313ce..0x314dd` is M76's established path and again dispatches through
`0x39e4b`. The border assembler `0x1b9c6..0x1be1b` and the skill check
`0x215b9` reproduce the already settled Levitate and buff indicators. Segment
bases: Items/sheet `0x39ca0`, item actions/spells `0x31010`, Quests/Awards
`0x33900`, scene `0x10380`, combat/party `0x1d7f0`; mapping
`file = 0x56c0 + seg*16 + off`. The key translation table at `0x456e8`
(scan code to key byte) supplied Escape `0x1b` and F1 `0xc9` as known
anchors; it maps Up and keypad 8 to `0xf2`, Down and keypad 2 to `0xf3`,
Home `0xdc`, End `0xdd`, Page Up `0xde`, Page Down `0xdf`.

### Combat items

| Question | Anchors; verdict |
| --- | --- |
| Use button | Combat key handler `0x2e8e3..0x2e91b`: saves the acting index (`0x97ff`), opens Items mode 7 for the combat-party member (`0x9720` table, roster stride `0x162`), re-highlights when the index is unchanged (`0x2e9ff`), otherwise restores it and advances (`0x2dcf3`); common end-of-action tests `0x2ea0f`, `0x2ea99`. Same as pin. |
| Dialog modes | `0x3a669..0x3a695`: modes 7 and 4 start on Misc; mode 7 sets the "combat Use" local and becomes mode 0. Key table `0x3b921` (keys) / `0x3b95d` (handlers): Esc, 1-9, A B C D E F G I M Q R S U W, F1-F6. Same as pin. |
| Equip/Remove/Use/Discard/Quest keys | `0x3b34d` (E: modes 0/6/11, not Misc), `0x3b37c` (R: modes 0/4), `0x3b3a9` (U: mode 0, Misc), `0x3b3c0` (D: mode 0), `0x3b401` (Q: mode 0, calls `0x3390f`). Action call `0x3b3cc..0x3b3dc` to `0x311ae`; result 1 returns a null member (`0x3b3f4`), 2 closes (`0x3b8aa`). No combat-specific restriction. Same as pin. |
| F1-F6 | `0x3b4b1..0x3b5c7`: refused in the combat Use dialog, modes 3/4/6/11 and on map 0; count from `0x971f` with `0x9720` when the mode byte is 2, else `0xed8c`; transfer `0x3b5ca..0x3b89e` tests the source cursed bit `0x40` (`0x80bb` text) and the destination's slot 8 (`0x80de`, `0x8107`, `0x812e` and the Misc text), copies the record, clears the source, zeroes the destination frame and compacts both (`0x38ae8`). Same as pin. |
| Item action routine | `0x311ae`: empty category returns (`0x311c8..0x3120c`); selection prompt window 13 (`0x3121d..0x3136a`); mode table `0x32006`, action table `0x31ffe`: Equip `0x37b83`, Remove `0x31119`, Use `0x313b7`, Discard `0x31018`. Same as pin except Discard's weapon test below. |
| Use path | Map 0 `0x313b7..0x313cb` (text DS `0x6b61`); condition table `0x31fee` refuses 8, 11-15 through `0x313ef` (DS `0x18ca` pointer), admits 9 and 10; sheet-in-combat flag `0x97c6` at `0x31411..0x3141e` (text DS `0x6b97`); power/charge/state tests `0x31420..0x3144c` (text DS `0x6bda`); charge `0x3145c`; caster `0x3146d`; windows 30, 29, 24 closed `0x31474..0x3148b`; dispatch `0x3149e`; exhausted item `0x314aa..0x314c6`; `0x314c9` shooting clear, `0x314ce` monster response, `0x314d3` turn cursor `0xff`, return 1. Flag `0x97c6` set at `0x3a468..0x3a476` with the sheet, cleared `0x3a59b`. Same as pin. |
| Discard | `0x31018..0x31110`: cursed `0x31034..0x31054` (DS `0x6a62`); weapon ID exactly 34 `0x31061..0x3106f` (DS `0x6a86`, fixed text); confirmation DS `0x6acf` through `0x1ea3a`; Yes clears and compacts, No returns 0. Differs from pin, which has no weapon test and returns true on No. |
| Remove | `0x31119..0x311ad`: cursed per category (DS `0x6af3`), else frame 0; Misc does nothing. Same as pin. |
| Town Portal item in combat | Dispatch branch `0x3a058..0x3a075` and handler `0x30654..0x30819` as in M76; the handler saves and restores the mode byte around its dialog (`0x30672..0x3067a`, `0x30762`), sets `0x96e1` at `0x30782`, loads the map (`0xf542`) and the run or town position (`0x307b3`, `0x307c9..`). The Items dialog exit reads `0x96e1` at `0x3b8d9` only to skip freeing its icon resource. Attack slots `0x98b4..0x98b6` are cleared and rebuilt by the scene setup (`0x14413` indoors, `0x16939` outdoors), not by the load; the combat loop ends when they are all `0xff` (`0x2ea99..0x2eaf0`, one minute charged at `0x2eaae`). **Medium confidence / Unknown:** whether a redraw rebuilds the slots before the next member's turn is shown depends on the idle input loop, which this check did not trace. |

### Quests and Awards

| Question | Anchors; verdict |
| --- | --- |
| Quests entry and resources | `0x3390f..0x339d3`: `quest.icn` (DS `0x6f95`), `qnotes.bin` (DS `0x6f9f`) loaded with file set 1 (`0x3398e..0x339aa`), mode byte saved `0x33968` and restored `0x34023`, button table DS `0x1e36` (file `0x4f6c6`: x 12/80/148/216/250/284, y 109, 24x20, keys I Q A `0xf2` `0xf3` Esc), header DS `0x6faa`. Callers `0x0d4d8` (main screen) and `0x3b40a` (Items). Same as pin. |
| Quest Items page | `0x33a4f..0x33c4a`: 85 counters at DS `0xf077`; title lines DS `0x6f57`/`0x6f75` at index < 35 / >= 35; multi-count set table `0x34056` = 17, 26, 79-84 with "%d %s%c" (DS `0x7003`, `s` when > 1); empty lines DS `0x6f52` (backspace, space, backspace, star); none: DS `0x700b`; format DS `0x703a`. Same as pin. |
| Current Quests page | `0x33c4d..0x33d89`: bit test helper `0x24f07` on DS `0xf06f`, flags 1-56, titles at < 29 / >= 29, note index flag-1; none: second line DS `0x70ba`; format DS `0x70d8`. Same as pin. |
| Auto Notes page | `0x33d8c..0x33f3c`: DS `0xf05f`, flags 1-128, titles at < 72 / >= 72, note index flag-1+56; none: second line DS `0x7103`. Same as the pin's World branch (offset 56). Flag 128 reads the bit following the array (quest flag 0); unreachable in Clouds. |
| Quests keys | Table `0x3402e` / handlers `0x34042`: Esc `0x33fdc`; I/Q/A select the page and reset the top row (`0x33f8c..0x33fa5`); Up `0x33fa7`, Down `0x33fb5`, Page Up `0x33fc5` (-3), Page Down `0x33fcb` (+3), Home `0x33fd1`, End `0x33fd3`, clamped to `0..count-1`. Differs from pin, which also accepts U and D. |
| Awards entry and resources | `0x36c41..0x36d0e`: `award.icn` (DS `0x7692`), `award.bin` (DS `0x769c`) with file set 1; button table DS `0x1e62` (file `0x4f6f2`: x 216/250/284, y 109, 24x20, keys `0xf2` `0xf3` Esc); header DS `0x76a6`; caller `0x384b0..0x384b6` passes the sheet's member. Same as pin for layout; no member selection in either. |
| Awards list | `0x36d83..0x36ed3` over awards 0-87 through `0x3ba44` (73 to 62, 81 to 63, else modulo 64; low nibble below 64, high nibble above); award 9 formats text 9 with the count byte `0x36db2..0x36e1e`; award 17 formats text 17 with the race name `0x36e21..0x36e90`; none: second line DS `0x76cf`; header DS `0x76db` with name and class, nine lines `0x36ee7..0x36fbb`. Same as pin. |
| Awards keys | `0x36fdd..0x37019`: Esc; `0xf2` up by one, floor 0; `0xf3` down by one while nine lines fit. Differs from pin, which uses U and D and does not react to the arrows. |

### Skill indicators

| Question | Anchors; verdict |
| --- | --- |
| Skill check | `0x215b9..0x21609`, table `0x2160a`: active members in roster order up to `0xed8c`; one member suffices for skills other than 5 and 14 (whole party) and 9 and 11 (two). Same as pin. |
| Danger Sense | Border `0x1ba67..0x1baab`: `0x974c` and skill 17, frames 40 + own counter `0x974f` at (107,9), counter modulo 12 every call. Source `0x1fdc1`: early returns `0x1fdc7..0x1fde4`, clear `0x1fe49`, set `0x1fefb` for a position match in the 7x7 window (`0x1fea9..0x2015c`) before the attacking/asleep tests. Same condition as pin; the pin's frame comes from the Spot Doors counter after its increment, one frame ahead. |
| Spot Secret Doors | Border `0x1ba1e..0x1ba63`: `0xf496` and skill 16, frames 28 + counter `0x974e` at (194,91). Source `0x12303..0x1231e` in the indoor scene setup `0x12094` (called from `0x1129c`): wall word `0xf48e` not `0x8888` and `0xf4c6` nonzero, the latter counted for facing wall values 5 and 8 of the party's cell (pin `interface_scene.cpp:923-925`, cell index 2 at offset (0,0) with shift 12). No outdoor writer. Same as pin. |
| Direction Sense | Border `0x1bdb8..0x1bdfd`: skill 6 and `0x8c52` zero; letter from the direction name table DS `0x17a2` by `0xed96`; format DS `0x51fa` (file `0x52a8a`) into window 0. `0x8c52` set `0x1e015`, `0x1e19f`; cleared `0x1e16c`, `0x1e26c`. Same as pin. |
| Counter resets | `0x1c402..0x1c410` and `0x1d4c4..0x1d4d2` zero the Levitate, Spot Doors and Danger counters together with the statue frames. The pin resets its Spot Doors counter alone in `locations.cpp`; no visible difference, since it draws both indicators from that counter. |

### Open questions from this check

1. **Town Portal item in combat, arrival state.** DOS continues into the town
   dialog, loads the town and finishes the Use as above; the combat loop then
   runs until its attack-slot test finds no attackers, which happens once the
   scene setup rebuilds the slots. Whether the player sees one more combat
   turn in town is not settled statically. **Maintainer decision
   (2026-10-19):** MMModern ends combat at the first attacker re-evaluation
   after the load, with the ordinary end-of-combat minute; no DOSBox check
   first. The static label stays medium confidence / Unknown.
2. **Spot Secret Doors outdoors** keeps its last indoor value in DOS and the
   pin (no outdoor writer). Static only; a DOSBox observation would settle it.

None of this changes milestone scope, production behavior or the save format
by itself.

## Monster treasure staging

**DOS static, high confidence:** the original retains generated Misc monster
treasure in its own category. The pinned `Combat::attack2` Misc case instead
copies the temporary character's accessory record into staged accessories.
**Implemented (M61 Part C):** MMModern stages Misc in its own ten-slot array
and delivers it with the existing shared rules, preserving the generation draws
and their order. Since M81, MMModern keeps pending Misc as running-program state, never
saved, as the [M81 DOS trace](#m81-monster-treasure) establishes. M61 Part D
implements the separately traced delivery rules below.

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

## M81 monster treasure

**2026-10-20; DOS-confirmed by calibrated static analysis, high confidence,
within the bounds below.** MMModern baseline `29f5b308568678f9f891476e9acec9ac3c72aab5`;
same installed DAT hash as [Exact baseline](#exact-baseline). No DOSBox execution.
REA identified the input/hash, but its new load-image query timed out; it supplies
no new successful image-verification record. A read-only, no-analysis query of the
existing Ghidra project independently matched the hash, 16-bit language, drop
field loads `0x2c36c/0x2c370`, delivery entry/return `0x24128/0x248f2`, currency
operands, Save map guard and play-entry reset. Ghidra has no defined instruction
at the four staging entries; bounded Capstone decoding and the actual dispatch
table, rather than inferred Ghidra function bodies, establish those loops.
The cited ScummVM files match pinned Git blobs after CRLF normalization;
the external checkout's unrelated dirty files are not used as authority.
Original instructions/projects stay in `D:\Projetos\MModern\MModern-re\m81-part0\`;
derived inventories and lookup tables stay in `build-rel/m81-part0/`.

**Resource calibration.** Installed `DARK.CC`: 11,217,676 bytes,
SHA-256 `7cbaffab761e54c3f31a994ce25a6795f92dbc696181a5c74cc318bba688bf0e`.
The decrypted CC index has 1,171 entries; resource `xeen.mon` (ID `0xc5d8`)
starts at archive offset 5,516,454 and has 5,400 decoded bytes, SHA-256
`fd04e2408a444cb9ff8b3f5f16badfc6733bc3d973e56c3275edfa787daef52b`.
All 90 sixty-byte records match the planning CSV's ID/name/gold/gems/drop.
Orc #6 is 10/0/1. Guaranteed-drop IDs are 43, 45, 50, 76, 80 (field 4),
82, 84, 85 (5), and 87 Darzog (6, 0 gold/50 gems). There are no mismatches.
The death routine's stride 60, gold word at `+0x2a`, gem byte at `+0x2c`
and drop byte at `+0x2d` calibrate the interpretation, not just the CSV.

| Rule | Result, file offsets and evidence boundary |
| --- | --- |
| Chance | Field zero branches past the random call (`0x2c373..0x2c377`). Every nonzero field, including 4-6, draws inclusive `R(1,100)` (`0x2c38b..0x2c394`), then accepts draw <= unsigned-byte threshold (`0x2c398..0x2c39a`; zero-extension makes the signed comparison harmless). Same drop byte supplies generation level (`0x2c3e2..0x2c3ec`). **DOS static, high confidence.** |
| Complete lookup boundary | Lookup base is file `0x4d650` (module `47f9:0000`), indexed by field minus one without a bound. Fields 1/2/3 read 10/20/50; 4/5/6 read 100; 7-17 read 0; 18/19/20/21 read 5/4/5/5 at `0x4d661..0x4d664`. The last four contradict the pin's zeros. These are exact memory-lookup observations, not evidence that generating at those levels is safe or reachable. No Clouds resource uses 7-21. |
| Capacity | Weapon/armor/accessory/Misc entry `0x2c402/0x2c457/0x2c4a6/0x2c4f5`; each starts at index 0, advances four-byte slots and stops at index 10 (`0x2c44e..0x2c452`, `0x2c49d..0x2c4a1`, `0x2c4ed..0x2c4f1`, `0x2c53c..0x2c540`). Equipment tests ID, Misc tests container. First empty slot receives three payload bytes; frame is untouched. All full paths exit without insertion or setting hasItems. Generation already completed at `0x2c3e7`; no retry/refund. **DOS static, high confidence.** |
| Currency | Gold word load `0x2c2be`, add/carry to DS:`9a16/9a18` at `0x2c2c2..0x2c2c6`; gem byte zero-extension and add/carry to `9a12/9a14` at `0x2c2fc..0x2c307`. Both are 32-bit accumulators, updated before chance, without currency draws or caps. Final acknowledgment adds them to 32-bit purse gold `f00a/f00c` and gems `f00e/f010` (`0x2486a..0x24884`), then clears them. **DOS static, high confidence for ordinary sums and widths.** |
| Overflow boundary | The instruction pairs discard carry beyond 32 bits; hardware arithmetic would wrap modulo 2^32. Ordinary-play reachability of staged or purse overflow is not established. The pin's treasure fields are signed `int`, purse fields unsigned `uint`, with no explicit overflow policy. |
| Arena | Map byte DS:`ed99` == 85 branches from `0x2c283..0x2c28a` to death cleanup `0x2c542`, after XP and before gold, gems and chance. It skips all ordinary treasure draws, not attack/XP or Arena generation. **DOS static, high confidence.** |
| Run | After Run/next-character, interactive-mode test `0x2e928..0x2e92f` admits full-party escape. `0x2e932..0x2e946` clears both currency pairs and hasItems only, then moves to the run position (`0x2e949`). Four item arrays survive, dormant until a later readiness/currency trigger; they occupy capacity meanwhile. **DOS static, high confidence; agrees with pin.** |

### Save and load boundary

**DOS static, high confidence.** Normal control-panel Save checks the combat
guard and displays the combat refusal (`0x1f1cb..0x1f1e6`); the called Save
routine checks maze restriction `0x8000` (`0x1ebcf..0x1ebf5`). Neither checks
pending currency, hasItems or item arrays. Thus deferred or dormant loot adds no
Save restriction where the ordinary guards permit it. Ready loot normally
delivers before the exploration input dispatch (`0xd1b7`); enemy-slot checks can
defer it (`0x24158` onward). This does not authorize saving in combat.

An open receipt is synchronous. Entry, timed holds, final input and return
remain within `0x24128..0x248f2`; the input poll `0xe0a3..0xe2cb` returns
keys/buttons to that caller and does not dispatch the control panel. Escape/Save
input cannot open Save during delivery. No transferred-prefix/receipt cursor is
saved; already transferred items are ordinary character inventory.

The Save routine preserves live party bytes across its roster preparation
(`0x1ec2f..0x1ec8f`), copies characters, flushes maze data (`0x1ed29`) and
calls the save snapshot (`0x1ed2e`). Snapshot `0xe951..0xe982` writes only
`maze.nam` from DS:`96c5` (31 bytes), `maze.chr` from the roster allocation
(10,620 bytes), and `maze.pty` from DS:`ed8c` (1,530 bytes). Archive writer
`0x45e36..0x45eae` obtains each member's size from its CC index, then writes
that source range. Load and slot inspection independently expose these sizes
(`0xf013..0xf069`, `0xf0e9..0xf13d`); the party-copy size is also explicit
at `0x1ec37`. None includes staging DS:`9970..9a19`. Ordinary purse and
character inventory are saved; staged currency, flag and forty items are not.

**Load does not reset everything.** Selected Load returns through
`0x1f18e`, `0xd2fa..0xd305` and the outer loop `0xb819..0xb839`.
Resource/roster reload `0x392f7..0x394c8` / `0xf013..0xf076` replaces the
saved ranges and characters; its clears (`0x39390`, `0x393b6`, `0x39c37`)
are other buffers. Play entry `0xcfe3..0xcff7` zeros pending currency and
hasItems, **not the four arrays**. Consequently an in-process Load retains
whatever items are currently staged, as dormant process state. It neither
recovers items present at the time of Save nor clears items staged afterward.
A fresh process starts those arrays empty: startup `0x576c..0x577c` zeroes
DS:`[8c4c,fd78)`, including every treasure field. It cannot recover them from a save.
The pin's `SavesManager::loadGameState -> Treasure::reset` instead clears all
four arrays. [Quirk](../original-quirks.md#sv-loot-load-process-state).

**New Game keeps them too (2026-10-20, DOS static, high confidence).** The
title menu (`0xbcce..0xc766`, key table `0xc77c`) starts a game through the
slot chooser `0x2042d`; its start branch `0x20a2b..0x20c43` loads the new
party resources (`0x20bb0..0x20bea`), clears character items (`0x373e0`,
DS:`eda8..f27c`) and writes the slot (`0xe938`), then play enters through the
outer loop (`0xb829`) and the same play entry `0xcfe3..0xcff7`, which zeros
currency and hasItems only. A sweep of every reference to the four array
bases and of every memset and repeated string-store fill finds no other writer on that
path; the nearest clears end at `0x9970` (`0x3054d`) or start after `0x9a19`
(`0x21d2f`). Calibrated against the startup, play-entry, Run and Load ranges
above. Pinned `SavesManager::newGame` does not reset treasure either.

### Event processing stash

**2026-10-20, DOS static, high confidence for the paths below.** MMModern
baseline `30c548ab8bc85637a8f45051b9ae8a00eb938b6d`; same DAT hash as
[Exact baseline](#exact-baseline); hash-checked Capstone reads under WSL, no
DOSBox execution. Bounded disassemblies stay in `D:\Projetos\MModern\MModern-re\m81-partA\`.
`0x26774` is the general Event-processing routine (the pin's
`Scripts::checkEvents`), not a map-change routine: it runs every Event at the
party position, and its event loop is skipped when attack slot DS:`98b4` is
occupied (`0x26abd`), like the pin.

- **Stash.** On entry, if pending gold, gems or hasItems is nonzero
  (`0x267b1..0x267cc`; the pin tests `gold & gems`), it copies both currency
  pairs, hasItems and all forty three-byte slots of the four arrays into a far
  stash (`0x267d1..0x26a1b`), then clears currency and hasItems only
  (`0x26a1e..0x26a30`). **The live arrays are not cleared.** Otherwise it
  zeroes the stashed currency and flag (`0x26a35..0x26a5d`).
- **Event items share the arrays.** GiveEnchanted inside the routine scans the
  same live arrays for the first empty slot (weapon `0x287f9..0x28855`, armor
  `0x28858..0x288bb`, accessory `0x288be..0x28922`, Misc `0x28925..`) and sets
  hasItems (`0x28844`); the weapon-34 grant at `0x2808a..0x280bb` does the same.
  Pending and dormant monster items therefore occupy Event capacity.
- **Delivery after the Events.** If live currency or hasItems is nonzero, the
  routine calls delivery `0x24128` (`0x28d70..0x28d8d`). Delivery transfers
  every occupied slot, monster items included, credits only the live (Event)
  currency, and finally clears all four arrays (`0x2489f..0x248d7`). Its gate
  (`0x24158..0x24259`) defers delivery while any of the 26 attack slots
  DS:`98b4..98cd` is occupied, but only outside mode 9: `0x2414e` compares the
  mode byte DS:`9a1c` with 9 and `0x24155` jumps past the slot tests to
  delivery. The Event loop writes mode 9 at `0x26bf9`, just before it runs an
  Event (the only write of 9 to that byte in the program), and restores the
  entry mode at `0x28e8f`, after this delivery; it saved it at `0x2679b`.
  Combat writes modes 2 and 1 (`0x2e63e`, `0x2eaf0`). So delivery after the
  Events is not deferred by monsters, as in the pin's
  `_mode != MODE_SCRIPT_IN_PROGRESS` test. Checked in the plan review of
  2026-10-20 against the hash-checked image.
- **Restore.** If the stash is nonzero, `0x28f22..0x291a6` copies currency,
  hasItems and all forty slots back, overwriting the live arrays.
- **Map change stays inside one invocation.** TeleportAndContinue (opcode 31)
  loads the map (`0x270f3`, `0x0f542`), sets the position and restarts the
  event scan at the arrival cell (`0x27117..0x27127`, back to `0x26a8e`);
  another opcode path (call `0x0ca60`) also restarts, at `0x28afc..0x28b08`. Arrival Events thus run
  under the entry stash; TeleportAndExit ends processing (`0x2711c`) by jumping
  to `0x28d50`, which falls through to the delivery test at `0x28d70`, so it
  still delivers and restores.

**Consequences.** Without Event treasure the restore reproduces the live state:
retention is unchanged. With stashed monster currency and an Event that gives
treasure, delivery hands over the Event treasure together with the monster
items, and the restore then re-stages those monster items with their currency,
so the items are delivered a second time and the monster currency once.
Monster currency is pending when an Event starts only if its own delivery was
deferred: a kill while another monster still holds an attack slot. Without
stashed currency, an Event reward delivers dormant monster items with it.

**Delivery mode and main-loop order (M81 Part B, 2026-10-20; DOS static, high
confidence for Clouds).** MMModern baseline `d210bf169f3e927e24e94710b2eced57029a2150`;
same DAT hash; hash-checked Capstone reads under WSL and a read-only sweep of the
CD `XEEN.CC` map scripts, no DOSBox execution. Work files stay in
`D:\Projetos\MModern\MModern-re\m81-partB\`.

- *No Clouds Event reaches delivery outside mode 9.* All 77 writes of the mode
  byte were swept and the direct-call closure of `0x26774` followed. Reachable
  writers either save and restore the mode on every exit (`0x249bf`,
  `0x24c71`, `0x24fef`, the town services `0x1cea5` with mode 17, and every
  writer nested inside them), are the delivery itself (`0x24128`, called only
  at `0x28d8d`), or start movement and combat only in modes 1, 2 or 5
  (draw routine `0x10bc3`: `0x10c01..0x10c2d`, `0x11817..0x1183e`). The one
  non-restoring writer an Event line reaches is giveExt's lock pick
  (`0x2358e`, mode 7 at `0x236a3`, called from GiveMulti at `0x27ea3`;
  success continues at `0x28d30` to the next line). Every Clouds GiveMulti
  line on a box (sprites 16, 58, 73: 65 lines on 76 boxes) is followed by a line
  that always matches, so `0x26bf9` writes 9 again before delivery. The
  between-line abort for attack slot 0 (`0x26abd`) cannot newly trigger there:
  the draw routine recomputes the slots (`0x14406`, `0x1692c`), but monsters
  move only in modes 1 and 2 and no box script moves the party. Ten indirect
  call sites (runtime and driver code) were not followed; Dark Side boxes
  (sprite 15) are outside Clouds.
- *Events run before ordinary delivery.* The exploration loop
  (`0xd18c..0xd5e5`) looks up the party cell (`0x11f04`, the only writer of the
  Event-cell flag DS:`f482`, at `0x11ffc`), runs the Events (`0xd1a5`) when the
  cell has any, leaves the loop if they return nonzero (`0xd5e8`), then calls
  ordinary delivery (`0xd1b7`) before the input wait (`0xd1c9..0xd1f2`, whose
  back edge `0xd1bc` follows delivery). Every iteration therefore runs Events
  first, as the pin's `gameLoop` does. With monster currency pending and no
  monster in an attack slot, an Event treasure is delivered with the monster
  items, the restore stages them again, and ordinary delivery hands them over
  once more in the same iteration.

<a id="preferred-recipient-after-the-events"></a>**Preferred recipient after the Events (M81 Part C, 2026-10-21; DOS static, high
confidence).** MMModern baseline `1447735cdfea42cc5636f943ece28535cf7da278`;
same DAT hash; hash-checked Capstone reads under WSL, no DOSBox execution. Work
files stay in `D:\Projetos\MModern\MModern-re\m81-partC\`. Calibration: delivery's preferred-index
test (`0x243d5..0x243f2`, [Treasure delivery](#treasure-delivery)) reads the
WhoWill byte `47c9:0008` (file `0x4d358`), decrements it and falls back to the
capacity scan when it is negative or not below the party size; the pin's
`Party::giveTreasure` reads `Scripts::_whoWill` the same way. The raw segment
value occurs four times in the program and no other paragraph aliases the byte,
so these are all its references:

- the delivery test above is the only reader; makeItem's Misc power table at
  `47c9:0000` (`0x2621d..0x26224`, indexed by level) is the fourth use;
- Event-routine entry `0x26774` writes 0 at `0x26797`, on every call (main loop
  `0xd1a5`, action dispatch `0xd2be`, `0x1184a`, `0x2ec37`);
- the WhoWill dialog `0x2507a` stores the 1-based selection at `0x252d2`; a
  one-member party returns 1 without writing (`0x25082..0x2508c`) and Cancel
  skips the store (`0x2528c`). Its callers are the WhoWill opcode (`0x285ae`),
  SetChar's ask (`0x27a1c`) and the grate routine `0xc794` (message 13, from the
  exploration action dispatch `0xd260..0xd2b1`), which ends with `f5fe=0`
  (`0xc911`) so the dispatcher skips the Event run.

Nothing writes the byte at Event exit (`0x28d50..0x291a6`), play entry, Run,
Load, New Game or a map change; it is initialized data (0) outside the startup
BSS clear and every save range. **DOS therefore does not reset the preferred
recipient after the Events:** ordinary delivery in the same iteration
(`0xd1b7`), and every later one until the next Event run, prefers the member
last chosen by a WhoWill dialog, inside an Event or at a grate, under the
existing validity, `canAct` and capacity tests. The pin agrees: only
`dialogs_whowill.cpp` sets `_whoWill`, and only the `Scripts` constructor and
`checkEvents` entry reset it.

*When the Event routine runs (review of the Part C implementation, 2026-10-21;
same DAT, same method).* Its entry clear precedes any record scan, and its four
callers run it whether or not a record matches the position and facing:

1. Main loop `0xd1a5`: every pass looks up the party cell (`0x11f04`; attribute
   bit `0x10` becomes `f482` at `0x11ffc`) and calls the routine when the bit is
   set, whatever the facing, before ordinary delivery `0xd1b7`. Idle input
   waiting loops back after this check, so a pass is one processed input. Pin
   `xeen.cpp:328`.
2. Space, or a click inside the 3D view (`0xd1f4..0xd23d`): the wall ahead
   selects through table `cs:0xf14` (code base `0xc790`); walls 1/6/9/13
   indoors go to the grate routine `0xc794`, all others call the routine at
   `0xd2be`. After the grate routine Events still run unless it zeroed the
   input word at `0xc911`; its early refusals (`0xc7a7`, `0xc7be`) do not.
   Pin `interface.cpp:393-431`.
3. After combat in modes 1/5, when an Event run was cut by attack slot 0
   (byte `9756`, the pin's `_eventSkipped`): `0x1184a`. Pin
   `interface.cpp:1381-1389`.
4. At combat end, when door text was up at combat entry (byte `3066`, saved at
   `0x2e631`): a cell lookup, then the routine if bit `0x10`, at `0x2ec37`. Pin
   `interface.cpp:1904-1907`.

## Treasure delivery

**DOS static, high confidence; implemented (M61 Part D).** The 2026-10-12
playable-rules report and `evidence-index.json` are retained under
`D:\Projetos\MModern\MModern-re\playable-rules-20261012\`. Its complete delivery record is
`ev_40c5c299c56c33aa600c91e27b2dce7650858004fffc25e6852ad8a9b02d6aba`;
sort is `ev_2730d9bb0d263ec2078ba8af76a26631ece50c58366b4365588ecc509de4024b`.
Part D rechecked the same SHA-256 read-only in the persistent Ghidra project;
query and instruction/table results remain in `D:\Projetos\MModern\MModern-re\m61d-recheck\`.
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

## Other town services (2026-10-20)

**DOS static, high confidence for the stated paths.** MMModern baseline
`2632944256be55a830d82a35024c227e4ba692b0`, clean `main`; `origin/main`
`093d5175aede27223e7b4097d27dbe97cc9f95a1`. The separate ScummVM checkout
was clean at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The original DAT
matches the size/hash in [Exact baseline](#exact-baseline). Analysis used
WSL Python 3 and Capstone 5.0.7; file offsets below use the mapping above.
No game execution, original-data mutation, code change or tests were involved.
Details, work copy and bounded disassemblies:
`D:/Projetos/MModern/MModern-re/m79-services-20261020/findings.md`.

Calibration preceded the new conclusions. Temple selection
`0x1c9f5..0x1cab3` reproduces the settled four surcharge rows. Its Clouds
keys/targets are at **`0x1ce87..0x1ce96`**, not the previously cited
`0x1d187`: the service code segment's file base is `0x1aa80`, and the
table is at `CS:2407`. Tip `0x1b49e..0x1b525` confirms maps 28/30/49,
bases 30/45/60 and the stored remainder after the byte increment. Rumors
`0x1b626..0x1b6ad` confirms bases 0/10/20 plus `day%10`. Donation
`0x1ae8f..0x1af73` confirms the modulo-ten visit counter and blessing latch;
exit `0x1d78e..0x1d79c` adds 1,440 and calls the settled time routine.
The original Event sites/facing/line are taken from the M79 plan, not rescanned.

| Question | Original file offsets | Result and pin comparison |
| --- | --- | --- |
| Training | Clouds selector `0x1c8a7..0x1c8cb`; cap checks `0x1c8e6..0x1c935`; quote `0x1c935..0x1c95d`; payment `0x1ab7c..0x1abb2` | 28 -> 10, 30 -> 15, others -> 20; both cost paths use permanent level squared times ten. No map-dependent XP/cost factor. Pin `locations.cpp::TrainingLocation` agrees. |
| Food | Selector `0x1b1ac..0x1b247`; text `0x1b248..0x1b28b`; Full/payment/assignment `0x1b2be..0x1b2e8`; action prologue `0x1ab24..0x1ab3a` | 28: 15/10/75; **30: 60/50/76**; 49: 120/100/77. Pin `TavernLocation::doOptions` uses 50/50/76 at 30. Full is unsigned `food >= target`. Untabled Clouds skips target/price/text-index assignments; no defined DOS text index is established. Pin's explicit 15/10/75 fallback remains **Reference only** for that unreachable path. |
| Guild | Row table `[0x4d6e0,0x4d744)`, class tables `[0x4f8ae,0x4f926)`; row builder `0x343de..0x344ce`; all-class scan `0x342ff..0x343ce`; membership `0x1aa80..0x1ab15`; Buy/Info callers `0x1b7f3..0x1b84c` | All 100 row entries and 120 class-table entries match the pin (byte padding interpreted as signed where appropriate). Rows 0..4 for 28..32, ascending global IDs at 49, class filtering, learned filtering only for Buy, same catalogue for Info; awards 0..4 and 5. Pin `dialogs_spells.cpp`, `character.cpp` and `locations.cpp` agree. |
| Ironworks | Map table `[0x4d6d0,0x4d6d8)`; two stock selectors `0x326c0..0x32701`, `0x32944..0x32985`; complete price routine `0x3c67d..0x3cc27` | First four table entries 28/30/73/49 select 0/1/2/3, else 0. Price routine uses item/action/Merchant, not the map. Pin `party.cpp::BlacksmithWares::getSlotIndex` agrees; existing DOS price corrections are unchanged. |
| Temple 74 | Map-preserving visit setup/quote calls `0x1cee0..0x1cef4`, `0x1d4ae..0x1d4c3`, `0x1d5d3..0x1d5f2`; map read/switch miss `0x1ca30..0x1ca4d`; startup `0x5762..0x577d`; current-state Save `0xe951..0xe982`, Load `0xf03b..0xf069`; initialization buffers `0xd7d9..0xd7f7`, `0xd899..0xd8ab` | Price routine sees 74. Five process globals survive the miss and ordinary New/Load/Save/map changes; startup alone zeroes them outside the table assignments. They are absent from original saves. Pin constructs/destroys the location per visit and zeroes its members, so 74 always uses zero surcharges and zero Donation. |

Temple globals are **DS-relative offsets**, not initialized file positions:
`f64e` Donation, `f650` Uncurse, `f652` Eradicate, `f654` Resurrect,
`f656` Heal. Startup sets DS to module segment `0x481d` and zeroes
`[DS:8c4c,DS:fd78)`; all five lie inside that BSS interval. Direct-reference
and address-taking sweeps across code, followed by instruction-boundary
checks, identify no other price-global writer. Normal New/Load/Save and map
paths use other buffers, not a whole-DS snapshot. Save writes `maze.pty`
from `DS:ed8c` through the archive entry's length (writer
`0x45e36..0x45eae`, DOS write at `0x45ea4`); Load reads its bounded file
into the same pointer (`0xe035..0xe07b`, `0x3ff37..0x3ff58`, read
`0x45f1c..0x45f43`). Party sizes `0x32c`/expanded `0x5fa` end at
`DS:f0b8`/`DS:f386`, both below `f64e`; roster/name/map data use separate
buffers. The existing Clouds/Darkside CUR directory entries corroborate
those two sizes. No copy of the price globals into those buffers was found.
These are static normal-path conclusions, not observations of DOSBox play
or guarantees for corrupted files/memory.

The price row is therefore process state, not save state: it starts at
zero when the program starts, survives New Game and Load, and is never
written to a save.

## Entrance scenes and the Arena (2026-10-20)

**DOS static, high confidence only for the paths explicitly settled below.**
MMModern began on clean `main` at
`a7b9bc365a52b305d431e400a52deb8cd95ff228` (also `origin/main`), parent
`93398ced8d324da91893b87fd6553fa603512247`. The separate ScummVM checkout was
clean at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
`WORLD/XEEN.DAT` was read directly: 353,511 bytes, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
WSL Python 3 / Capstone 5.0.7 decoded bounded 16-bit ranges. Mapping:
file `= 0x56c0 + segment*16 + offset`; Ghidra linear `= file + 0xa940`.
No game execution or other original-resource inspection was performed.
Scripts, disassemblies, table comparison and findings are external at
`D:/Projetos/MModern/MModern-re/m80-scenes-20261020/`. This is evidence, not implementation
or milestone acceptance.

**Calibration.** Event caller `0x27b1f..0x27b3b` passes the action to
`0x1cea5`. Dispatch `0x1d02a..0x1d038`, table `[0x1d7e2,0x1d7fe)`,
routes 0..5 to the Smith, Bank, Guild, Tavern, Temple and Training setup
blocks respectively. The option dispatch at `0x1ab24..0x1ab36`, table
`[0x1b9b8,0x1b9c6)`, and quote dispatch at `0x1c858..0x1c869`, table
`[0x1ce97,0x1cea5)`, preserve that ordering. Temple action 4 reaches
`0x1c9ce`, then the settled surcharge selector `0x1c9f5`: Clouds keys
28/30/49/73 and the four M79 rows agree (table `0x1ce87`).
Tavern action 3 reaches `0x1b057`; its T option reaches `0x1b349`,
including the settled Tip selector `0x1b49e`. Ordinary service exit still
adds 1,440 at `0x1d78e` and calls time at `0x1d798`.
Actions 6/8/9/10/11 instead call
`0x2b784`/`0x2a66a`/`0x29ab6`/`0x2b052`/`0x2968b`.
**Calibration passes; same as the settled reference/service evidence.**

### 1. Entrance sites

**High confidence; same as the reference's Clouds branch and the plan.**
Quest bytes are indexed from `DS:f077`. The last column identifies the
archive requested on successful entry, not the archive containing the scene.

| Action/site | Quest item | Destination, position, facing | Archive | File offsets |
| --- | --- | --- | --- | --- |
| Reaper 12 | 3 | 55 `(3,8)` E | Clouds | `0x2a7db..0x2a7f6` |
| Reaper 15 | 2 | 59 `(11,8)` W | Clouds | `0x2a7f9..0x2a805`, shared `0x2a6d5..0x2a6e7` |
| Reaper 24 | 1 | 51 `(7,12)` S | Clouds | `0x2a765..0x2a780` |
| Reaper 7 | 46 | 113 `(7,4)` N | Darkside | `0x2a7a5..0x2a7b6`, `0x2a7c9..0x2a7d8` |
| Reaper 13 | 45 | 117 `(7,4)` N | Darkside | `0x2a7b8..0x2a7d8` |
| Golem 8 | 6 | 81 `(1,17)` E | Clouds | `0x29c11..0x29c27`, `0x29b89` |
| Golem 12 | 5 | 80 `(29,16)` W | Clouds | `0x29c2a..0x29c45` |
| Golem 20 | 7 | 79 `(5,16)` E | Clouds | `0x29c6a..0x29c80`, `0x29b89` |
| Golem 19 | 50 | 121 `(18,0)` N | Darkside | `0x29bee..0x29c0e` |
| Dwarf 14 | none | 37 `(1,4)` E | Clouds | `0x2b0e9..0x2b0fb` |
| Dwarf 18, x=9 / other x | none | 35 `(1,12)` E / 36 `(7,1)` N | Clouds | `0x2b0ce..0x2b0e7`, `0x2b0a7..0x2b0b6` |
| Dwarf 23, x=5 / other x | none | 33 `(7,1)` N / 34 `(7,30)` S | Clouds | `0x2b09d..0x2b0cb` |
| Sphinx 5 | 4 | 82 `(7,5)` N | Clouds | `0x296ec..0x2970a` |
| Sphinx 2 | 51 | 125 `(7,6)` N | Darkside | `0x2972e..0x29749` |

Reaper Clouds switch keys/targets are `[0x2b02a,0x2b03e)`;
Golem's dense switch is `[0x2a63a,0x2a654)`. Dwarf dispatch is
`0x2b07d..0x2b0fb`; Sphinx dispatch is `0x296dd..0x29749`.
The four cross-side branches set `DS:96e2=1` before loading.

### 2. Refusal, map reload and time

**High confidence for the installed World CD normal return path.** A failed
key check preserves map/position/facing as the eventual load target
(`0x2a783..0x2a7a3`, `0x29c48..0x29c68`, `0x2970c..0x2972c`).
It does not skip loading: Reaper `0x2afe1`, Golem `0x2a5db`, Sphinx
`0x29a57` call the ordinary loader; Dwarf loads at `0x2b70f`.
The loader saves ordinary maze state (`0xf595 -> 0x265f4`), clears the live
monster buffer (`0xf5be..0xf5cc`) and reconstructs records from MOB data
(`0xfa07..0xfb84`, including frame/effect randomization). Thus reload
rebuilds runtime monsters; it does not imply respawning killed monsters.
This agrees with the reference's unconditional `setNewLocation()`.

Map load sets stepped at `0xf58b`. Final scene redraw calls are Reaper
`0x2b005`, Golem `0x2a615`, Dwarf `0x2b74b`, Sphinx `0x29a91`;
redraw consumes stepped at the already calibrated `0x1184f`. The Event
caller then sets it again at `0x27b32`. These are not state-neutral refusals.
Actions 8..11 return through `0x1d426..0x1d42b -> 0x1ceb8`, bypassing
the service time charge. No direct elapsed-game-time addition occurs in
these scene handlers; this does not erase the ordinary redraw/lighting
effects described above.

**Outside the reference edition:** the result with a Darkside key in a
Clouds-only installation. The installed program requests the Darkside
destination; the loader also branches on installation state
(`0xf616..0xf77b`), and archive selection has an installation-dependent
override (`0xe89e..0xe8b0`). MMModern's reference is the World of Xeen CD,
where both archives are installed, and Clouds Events never grant these keys
(quest items 45, 46, 50 and 51 come only from Darkside Events), so the
Clouds-only failure does not bound M80. MMModern's party saves only the
Clouds quest counters 0-34, so those keys are never held and the four sites
always play the keyless refusal; the keyed outcome waits for the Darkside
arc.

### 3. Presentation and random draws

**High confidence for the following constants and explicit control flow;
not a complete timing certification.** Clouds resource pointers name
`tower.vga`/`freap.vga`, `golmback.vga`/`golem.vga`, and
`dwarf1.vga`/`dwarf3.vga`/`dwarf2.vga` at
`0x53c56..0x53c7e`; Sphinx names `sphinx.vga` at `0x53dec`.
Scene loaders select by current side, before destination archive loading
(`0x2a808..0x2a843`, `0x29ae1..0x29b1c`, `0x2b1cc..0x2b226`,
`0x296b6..0x296c4`). Clouds therefore uses Clouds scene assets even at the
four cross-side sites. The Clouds branches have no night-dependent frame
selection; Reaper's night-frame selection belongs to the Darkside branch
(`0x2aa56..0x2aa60`). Resource choices agree with the reference.

All 16 named coordinate arrays in the reference match contiguous signed
words in the DAT, including both side rows: Reaper X1/Y1/X2/X3 at
`0x4d360/0x4d3dc/0x4d3a0/0x4d3c0`; Golem X1/Y1/X2 at
`0x4d55c/0x4d5bc/0x4d58c`; Dwarf X0/X1/X2/Y at
`0x4d4c0/0x4d4f4/0x4d4a0/0x4d528`; Dwarf2 X/Y at
`0x4d414/0x4d454`; Sphinx X1/Y1/X2 at
`0x4d5f0/0x4d630/0x4d610`.

| Scene | Confirmed order and direct scene RNG calls | Timing evidence and reference comparison |
| --- | --- | --- |
| Reaper | Zoom indices 13..0, slide x=200 downward by 16 while nonnegative, speech over base frame 0 with random frame 0..4; **keyless** retreat uses indices 0..13 (corrected by Part 0b). `0x2a92c..0x2aa84`, `0x2ab3a..0x2abaf`, `0x2ac05..0x2acb8`, `0x2ad6b..0x2ae0f`, `0x2ae48..0x2af92`. One random draw per iteration of each speech loop: `N1+N2`. | Zoom waits 1 BIOS tick (`0x2aa70..0x2aa78`); speech loops explicitly wait 2 (`0x2ac94..0x2ac9c`, `0x2adf5..0x2adfd`); inter-speech pause 7 (`0x2acfe..0x2ad06`). Reference speech uses `WAIT(1)`: different explicit pacing. |
| Golem | Zoom 11..0; 13 slide iterations from -200 by +16. Slide draws Y `R(0,9)-5`, then X `slideX+R(0,9)-5`, with frame 2 over frames 0/1. Speech draws Y `R(0,9)-3`, then X `R(0,5)-3`, also frame 2. Direct total `26+2*(N1+N2)`. `0x29d46..0x29fac`, `0x2a114..0x2a17d`, `0x2a34f..0x2a3a3`. | Explicit 1-tick waits at `0x29e72`, `0x29f9f`, `0x2a154`, `0x2a384`; formulas and visible sequence agree with the reference. |
| Dwarf | Zoom 12..0, second zoom 15..0, then frame `R(0,9)` over the second sprite's frame 0. Three speech phases when Effects/driver configuration permits, otherwise the phase loop exits early. Direct total is the sum of executed speech-loop iterations. `0x2b2e3..0x2b530`, `0x2b533..0x2b6aa`; RNG `0x2b634`. | Speech waits 2 ticks (`0x2b65a..0x2b662`), unlike reference `WAIT(1)`. Phase loop also checks elapsed 3 ticks (`0x2b68e..0x2b696`). |
| Sphinx | Zoom 8..0; frames 0/1 plus `R(2,10)` during speech; refusal retreat indices **0..8** (corrected by Part 0b). Three keyed or two refusal phases, subject to the same audio-configuration early exit. `0x297bb..0x29836`, `0x29840..0x29981`, `0x29993..0x29a20`. Direct total is the sum of executed speech-loop iterations. | No explicit BIOS-tick wait in these zoom/speech loop bodies. Part 0b finds one retrace wait per iteration; DOSBox shows two retraces per speech iteration, about 35 Hz (below). The reference has `WAIT(1)`. |

These are loop indices and draw formulas, not a fixed total for an entire
Event invocation. All direct calls use the gameplay range helper
`0x45b9a`; its underlying generator at `0x45be5` mixes DOS time with
shared state. Speech duration, configuration and text completion can alter
iteration counts. Reload can make additional monster-frame/effect draws
(`0xfab0`, `0xfb3d`). Do not budget one fixed gameplay RNG count per scene.

Speech names agree with the reference: Reaper keyed
`reaper12.voc`, `reaper13.voc`, refusal `reaper14.voc`, `reaper15.voc`
(`0x2a884..0x2a8e9`, `0x2ad1b..0x2ad68`); Golem keyed
`golem15.voc`, `golem16.voc`, refusal `golem13.voc`, `golem14.voc`,
plus the entry sound (`0x29c9d..0x29cea`, `0x29e92..0x29fae`);
Dwarf `dwarf10.voc`, `dwarf11.voc`, `dwarf12.voc`
(`0x2b232..0x2b244`, `0x2b5b3..0x2b5fe`); Sphinx keyed
`sphinx10.voc`, `sphinx11.voc`, `sphinx12.voc`, refusal
`sphinx13.voc`, `sphinx14.voc` (`0x2974b..0x29799`,
`0x29845..0x298ab`).

DOS has a conditional text renderer, not an unconditional speech-only
implementation. It loads `special.bin` (`0x53de0`, loader `0x20cf2`)
and uses its NUL-separated records: Dwarf 1, Reaper 2/3, Golem 4/5,
Sphinx 6/7 (success/refusal). The text-offset table starts at
`40d4:165c`; callers use `1660..166a`, with Dwarf's index at
`0x2b28a..0x2b2bf`. The shared renderer `0xdca3..0xdd52` suppresses
text when `DS:ed9a != 0 && DS:8d38 >= 2`; otherwise it advances a
modulo-length character cursor after more than one timer tick and draws
a text box. First speech loops test both audio and that cursor; later
Reaper/Golem loops test audio alone. Dwarf/Sphinx can skip later samples
when Effects are off or the driver selection is below 2; this is not the
control panel's separate Speech setting.
The reference's subtitle handling is not equivalent to these DOS branches.
Part 0b below settles the installed startup configuration, cursor clock
and record lengths. Literal lines were deliberately not extracted. A
[video](audio.md#efx-voc-waits-and-entrance-captions) settles when the
Dwarf's box disappears in Effects-off mode; the other guardians' box
erasure is not video-confirmed.

Scene screen capture uses `0xdd72 -> 0x43fe5`; restores use `0x4405f`
(e.g. Reaper `0x2afeb`, Golem `0x2a5e5`, Sphinx `0x29a61`,
Dwarf `0x2b509`), followed by map/party-view reconstruction. The reference
also saves/restores a background. Part 0b settles the waits in the
no-explicit-wait loops. Complete pixel equivalence remains **unresolved**.

#### Part 0b: text settings and cadence (2026-10-20)

**DOS static, high confidence except the explicitly bounded pixel/runtime
questions below.** Clean `main` and `origin/main` were
`3d6b6ee4f750d002b5a64a515ccadb016d3c8070`. The DAT size and SHA-256 above
were rechecked by the reused Capstone script; the reference checkout was
clean at the same pin. Calibration re-decoded `0x2ac94..0x2ac9c`:
the Reaper loop retries the elapsed-time helper until its result is at
least 2; `0x440dc..0x440f4` reads BIOS time and subtracts the timer mark.
This reproduces Part 0's two-tick wait. New external notes:
`D:/Projetos/MModern/MModern-re/m80-scenes-20261020/findings-0b.md`.
No execution of the game, tests, code edits or original-data writes.

**1. Settings and default text.** `DS:ed9a` is the Effects enable byte,
set to 1 at startup (`0xb63c`); the sample-play wrapper also tests it
(`0xdea9..0xdecc`). `DS:8d38` is a **word**, the installed effects-driver
selection, not a Text or Speech toggle. Initialization clears the 29-byte
configuration block (`0x3d2f3..0x3d300`), opens the configured
`xeen.cfg` name (pointer `0x50214`, string `0x51a2d`) and reads 29 bytes
into `DS:8d38` (`0x3d303..0x3d348`).
The installed `WORLD/XEEN.CFG` is 29 bytes, SHA-256
`da316ffed9d11ee80bff6fdd0928ac6e992b72ec475368832d902ced14f81384`;
its first word is **10**. The driver switch (`0xb506..0xb575`,
table `0xb9de`) maps 10 to the `prosnd` resource (`0x51f42`).
This establishes the installed startup values, not the settings of an
arbitrary loaded save or an unexamined retail installer.

The control-panel key dispatch `0x1f036..0x1f047`, table
`[0x1f332,0x1f360)`, distinguishes these controls:

| Control | DOS state change | Effect on entrance text gate |
| --- | --- | --- |
| Effects (E) | Toggle `DS:ed9a`, `0x1f061..0x1f078` | Yes: Effects off allows text and disables the scene's sample wrapper. |
| Music (M) | Toggle `DS:ed9b`, `0x1f07b..0x1f09e` | None. |
| Text (T) | Toggle `DS:f384`, `0x1f04c`; zero displays as on (`0x1ee35..0x1ee68`) | Not tested by this scene renderer. |
| Speech (P) | Toggle `DS:f385`, `0x1f054`; zero is off (`0x1ee00..0x1ee2b`) | Not tested by this scene renderer or its sample wrapper. Speech off also forces Text on (`0x1ee2b..0x1ee32`). |

No panel action changes the driver-selection word. Speech's exit path
refreshes speech/map setup (`0x1f315..0x1f32a`, `0xf383`), rather than
changing either gate operand. Therefore **with the installed startup
settings, none of the four entrance scenes draws `special.bin` text.
Turning Speech off alone does not enable these captions or disable their
VOC samples.** Effects off is the relevant switch. The pin's configurable
`Sound::_subtitles` / `Subtitles::show` path differs from this gate.

**2. Text timing.** In the default configuration there is no text box to
time. When the gate permits text, `0xdca3..0xdd52` draws `box.vga` at
`(36,189)`, writes the text, then updates the screen. The shared cursor
advances by **one character**, modulo the record's byte length, only when
the elapsed timer is greater than 1 (`0xdcba..0xdccf`). It resets that
timer at `0xdd1a`; the timer reads BIOS ticks (`0x4411d..0x44144`).
Thus the threshold is two 18.2065-Hz BIOS ticks, about 109.85 ms, sampled
by the scene loop, without catch-up increments. Scene setup marks time
before the zoom, so this is not a fresh two-tick delay from speech onset.

| Scene | Calls and lifetime when text is enabled |
| --- | --- |
| Reaper | First speech loop draws text at `0x2ac87` after its sprite draw and continues while audio is active or cursor is nonzero (`0x2aca2..0x2acb8`). The seven-tick pause and second sample follow; the second loop updates without calling the text renderer (`0x2ade8..0x2ae0f`). No new caption phase is started there. |
| Golem | First speech loop calls the renderer at `0x2a14c` and uses the same audio-or-cursor continuation (`0x2a167..0x2a17d`). The second speech loop updates without the renderer (`0x2a37c..0x2a3a3`). |
| Dwarf | Text renderer at `0x2b64d` is inside the speech loop. Audio-or-cursor continuation is `0x2b664..0x2b679`; after completion, the Effects/driver test exits the phase loop early in text mode (`0x2b67b..0x2b6aa`). With Effects off, one complete caption replaces the normal three sample phases. |
| Sphinx | Text renderer at `0x29909` is inside the speech loop. Audio-or-cursor continuation is `0x29916..0x2992b`; the same test skips later samples (`0x2992d..0x29940`). With Effects off, one complete caption replaces the normal two refusal or three success sample phases. |

**Settled for the Dwarf only:** its box lasts through the caption speech
loop and the final redraw erases it
([Audio](audio.md#efx-voc-waits-and-entrance-captions)). For Reaper, Golem
and Sphinx the frame on which already drawn box pixels disappear remains
**unresolved**: stopping renderer calls is not proof that another sprite
erases the whole box. Background restores/map reconstruction eventually
replace it.
The pin also differs in cursor behavior: `Subtitles::show` uses
`1 + lineLength/100` characters per step, so the 107-byte Clouds Dwarf
record advances two at a time there, versus one in DOS.

**3. Effective cadence.** Screen update with argument 0
(`0x462b6..0x46323`) calls `0x44145`; after copying the screen it calls
`0x43fca` at `0x4437e`. This polls VGA status port `0x3da`, first until
retrace ends, then until the next retrace begins (`0x43fca..0x43fdf`),
before flipping the display page. It is a retrace wait, not a BIOS wait.
The text renderer reaches this same update even when text is suppressed
(`0xdd48..0xdd50`).

All four scenes disable the mouse at entry (Reaper `0x2a675`, Golem
`0x29abc`, Dwarf `0x2b058`, Sphinx `0x29691`). That helper clears
`CS:16be` (`0x438f1..0x43913`). Input polling therefore takes the
mouse-disabled return (`0x43855..0x4385d -> 0x438e5..0x438ef`), not the
mouse-enabled redraw at `0x438d4/0x438d9`. The idle keyboard path reads
its queue and returns (`0x4549c..0x45505`); normal no-input return is
`0xe18f..0xe196 -> 0xe2c7`. There is no second retrace or BIOS wait in
that idle input path.

| No-explicit-BIOS-wait loop | File offsets | Static wait per iteration |
| --- | --- | --- |
| Sphinx zoom | `0x29826..0x29836` | One retrace per iteration. |
| Sphinx speech | `0x29904..0x2992b` | One retrace through the text renderer, including with text suppressed. |
| Sphinx refusal retreat | `0x299fb..0x29a0c` | One retrace per iteration. |
| Dwarf first / second zoom | `0x2b449..0x2b45b` / `0x2b4e8..0x2b4fa` | One retrace per iteration. |
| Reaper / Golem retreat | `0x2af94..0x2afa7` / `0x2a565..0x2a586` | One retrace per iteration. |

On the 70-Hz display clock this code path alone would give 14.286 ms per
iteration, provided drawing completes before the next retrace. Slow
execution can miss retraces; static analysis does not promise a measured
frame rate on every DOSBox cycle setting. The DOSBox capture below observes
two retraces per Sphinx speech iteration (about 35 Hz) and irregular zoom
and retreat intervals; M80 uses two retraces for all of these loops
([quirk](../original-quirks.md#sv-scene-speech-cadence)). These loops are retrace-paced,
not unbounded host-speed loops. The pin's corresponding `WAIT(1)` loops
use its game-frame clock instead. Explicit BIOS waits elsewhere remain
as recorded in Part 0; a retrace call does not remove them.

This trace also corrects two Part 0 summaries. Reaper's
`0x2ae50..0x2ae57` skips retreat when the key is present; refusal iterates
0..13 (`0x2afa1..0x2afa7`). The pin retreats on success.
Sphinx refusal iterates **0..8**, because `0x29a08..0x29a0c` compares the
incremented index with 9; the pin iterates 0..7. Both are high-confidence
DOS-versus-reference differences.

**4. Record metadata only.** Read-only index decoding of the installed
root `XEEN.CC`, using the pin's CC format, located `special.bin` ID
`0x4c23`: directory entry at archive offset `0x1bb2`, payload at
`0xb54145`, size 1,318 bytes. It contains 11 NUL-terminated records.
Zero-based lengths excluding terminators are
`390, 107, 76, 78, 72, 73, 73, 50, 159, 155, 74`.
Thus DOS indices 1 (Dwarf), 2/3 (Reaper), 4/5 (Golem), 6/7 (Sphinx)
are valid; the lengths corroborate Part 0's callers. The pin reads this
resource but splits record 0 into several intro subtitles, shifting its
later line numbers. No payload text or original bytes were written into
the repository or printed in the report.

**Remaining observation.** The default captures below show no scene
caption and measure the retrace pacing. A Speech-off capture at Dwarf map
14 `(9,9)` E or Sphinx map 5 `(12,8)` S should also show none; none is
recorded. Effects-off passes for Reaper, Golem and Sphinx (first/last box
frame and phase exit) remain open; the Dwarf's is recorded in
[Audio](audio.md#efx-voc-waits-and-entrance-captions). The settings
branches, cursor threshold, retrace calls and record metadata above are
settled statically.

**DOSBox capture (maintainer, 2026-10-20).** Two DOSBox ZMBV captures at
70.09 frames per second, default settings: the Dwarf Mine entry (map 14,
no key, to map 37) and the Sphinx refusal (map 5, no key). Measured by
decoding every frame (outside the repository):

- **No caption** appears in either scene, as the static default predicts.
- **Dwarf speech** changes frame every 7-8 captured frames, with multiples
  when the random frame repeats: two BIOS ticks (7.7 frames), confirming
  the explicit wait.
- **Sphinx speech** changes frame every 2 captured frames (4 when the
  random frame repeats): **two retraces per iteration, about 35 Hz**, at
  the maintainer's cycle setting. The zoom and retreat run at irregular
  1-3 frame intervals. This is the observed DOSBox rate for the one-retrace
  wait above.
- The original **"Please Wait..." box** appears while the scene's map
  load runs: over the dwarf before the destination view, and over the
  restored view during the Sphinx refusal's reload, for 16 and 14 captured
  frames (about 0.2 s, load-time dependent). The maintainer confirms DOS
  shows it on every map load, including outdoor region changes and
  stairs. The pinned reference loads this string but never shows it;
  MMModern omits it as an
  [approved deviation](../original-quirks.md#dev-no-please-wait).

**Sphinx sample descriptor (maintainer, 2026-10-20).** The refusal capture
has about 4.1-4.4 seconds of speech animation. `sphinx13.voc` has damaged
descriptor bytes 0..7, but its offset, single uncompressed PCM block and
terminator are intact: 39,243 audio bytes at 68 microseconds each, or
2.668524 seconds. Together with `sphinx14.voc` (1.275068 seconds), this
matches the capture; the second sample alone does not. DOS plays the first
programmed sample transfer without checking descriptor text. The pinned
`Audio::checkVOCHeader` instead rejects it. Maintainer decision: follow DOS;
the fixed-layout `XeenVoc.h` reader ignores descriptor text and block structure,
checks the +`0x20` PCM offset and count-plus-one bounds, and uses the fixed
68-microsecond period. The adopted durations are 2.668048 and 1.274592 s;
see [Audio](audio.md#installed-digital-driver-command-and-length-rules). [Quirk](../original-quirks.md#sv-sphinx13-descriptor).

These captures did not include Effects off; the later Dwarf Effects-off
capture is recorded in [Audio](audio.md#efx-voc-waits-and-entrance-captions).

### 4. Arena dialog, generation and exits

**High confidence; same as the reference except the species pool and
ordinal suffix below.** Map Event text indices 0..4 provide offer, decline,
fight start, victory format and unfinished-fight message:
`0x2b8cf..0x2b93b`, `0x2b9d5..0x2ba05`,
`0x2b7b3..0x2b80e`. Fixed title, maxed message and numeric prompts are
at `0x53f4a`, `0x53f6d`, `0x53fa2`, `0x53fc2`. Both numeric prompts
use window 11, two characters and argument 200; levels above 10 or counts
above 20 reprompt; zero exits (`0x2b95d..0x2b9d3`). Escape clears the
numeric input (`0x1e246..0x1e27d`), so cancels without generation.

DOS draws **five species slots first**, irrespective of the requested
monster count (`0x2ba4b..0x2bb81`). Each is
`R(1,7)+(level-1)*7`, then subtract 3 if above 67, replace 59 by 60,
replace 28 by 29. It loads each slot's normal/attack sprites. Only then,
in monster order, it draws X `R(3,11)`, Y `R(2,10)`, changes Y to 5
when X is 5 or 10 and Y is 4 or 8, draws slot ID `R(0,4)`, obtains
species and HP from that slot, draws frame `R(0,7)`, copies the species'
animation effect to effects 1/2, and draws effect 3 `R(0,7)` only for
nonzero animation effect (`0x2bb8c..0x2bd11`). It sets the attacking/awake
byte at record offset 4 to 1 (`0x2bd17`). There is no effect-3 assignment
on the zero-effect branch and no blanket record clear in generation;
map load initially clears the live buffer.
Direct generation draw count is `5 + 4*count + nonzeroEffectCount`.
The reference instead draws a species per monster, interleaved with its
other fields. This is a material difference, not just equivalent sampling.

Award byte `character+0x42` is index 9 of the packed award region
(`0x2b818..0x2b82e`, first-member read `0x2b830`; character stride
`0x162`). Every active member receives an increment on victory. The
99 check is a pre-generation guard for any active member
(`0x2b891..0x2b89d`), not a saturating increment. DOS formats a five-digit
count, tests the tens digit for 1, otherwise uses the units digit to select
the suffix (`0x2b830..0x2b87a`, suffix pointers `0x53c8e..0x53cb6`,
format `0x53f68`). Thus 11..19 use `th`, but 21/22/23 use `st/nd/rd`.
The reference uses `th` for all counts >=10.

Victory and maxed exit load 28 (`0x2b8b7..0x2b8bf`) and then use **map
28's** run position, facing East (`0x2bd2c..0x2bd36`,
run helper `0x1691f..0x1692b`). Decline loads 6 and explicitly places
`(12,4)` West, bypassing that common exit (`0x2b93e..0x2b95a`).
Zero/cancel, unfinished fight and successful generation use map 85's run
position, facing East. These exits agree with the reference.

### 5. Arena persistence and remaining boundary

**High confidence; same as the reference, correcting the plan's persistence
premise.** DOS skips saving the Clouds maze 85 entirely at
`0x26600..0x26610`. On loading it, `0xfb8a..0xfba9` clears the species
slots and sets monster count to zero, even after the normal MOB load.
Leaving and returning therefore discards the generated fight. Returning
after victory cannot repeat the award without a new generated fight.
The reference also excludes Clouds 85 in `Map::saveMaze/saveMap`.
Awards persist as ordinary character data; level/count are invocation
locals (`0x2b784`, `0x2b984..0x2b999`), not new saved Arena fields.

**Resource flag (settled by resource inspection, 2026-10-20):** map 85's
header carries flag `0x8000` (`mmodern --inspect-map <game-dir> 85` prints
`Flags: 0x8000`), the bit MMModern's control panel already treats as "saving
forbidden here". Do not conflate the loader's `0x8000` mask at
`0xf85b..0xf861` with the Save UI's restriction; a DOSBox save attempt in
map 85 would confirm the visible prohibition. No exhaustive claim about
every save entry is made beyond the settled maze exclusion and ordinary
award persistence.

## Other playable-rules confirmations

The 2026-10-12 playable-rules report and index under
`D:\Projetos\MModern\MModern-re\playable-rules-20261012\` retain calibrated, independently
checked reads for the following **DOS static, high-confidence** findings. Only
delivery changed production behavior, in M61 Part D; the other findings update
the rules reference.

| Finding | Original file offsets and boundary |
| --- | --- |
| Natural 20 damages before the ordinary comparison and fresh AC read | `0x2db45`, `0x2db55`, `0x2db65`, `0x2dbd7`; injury `0x1f8a7` and armor bad-state gate `0x21275..0x21281` confirm armor breakage affects the later check. |
| Broken weapons retain base dice and equipped effectiveness | Bad-state material skip `0x2e057..0x2e05d`, base dice `0x2e0e9..0x2e134`, frame-only effectiveness `0x2c950`, selector `0x2c96f`, multiply `0x2caed..0x2cb00`. Break Weapon itself clears frame at `0x2d52c..0x2d532`; implemented since M68 Part A. |
| Hatred 1 bypasses preference | Compare/branch `0x2d84b..0x2d852` reaches random dispatch `0x2d9a3`; this does not establish every party-size distribution. |
| Run uses strict less-than | Inclusive draw `0x2e1c0..0x2e1c4`, unsigned threshold `0x2e1cc..0x2e1d0`, comparison/failure `0x2e1d2..0x2e1d4`. Threshold 100 accepts outcomes 1..99; no measured DOS RNG uniformity claim. |
| Monster recovery differs from the pin | Caller `0x20ca9..0x20cf1`, helper `0x2e19b`, comparison `0x2e1b2..0x2e1b4`, clear `0x20ce1`: DOS clears at draw `<= ID`, the pin at `>= ID`. Implemented since M73 Part A ([quirk](../original-quirks.md#sv-monster-recovery)). |
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
| `doCharDamage` entry, Sleep clear, dice loop | `0x2d22d`; the Sleep byte at character offset `0x14b` is cleared at `0x2d236`; `strikes` from monster offset `0x1a`, each `random(1, dmgPerStrike)` at `0x2d26c..0x2d27d`. |
| Initial save uses the Physical selector | `0x2d2e4` pushes 0, calls `0x2d0c2`, halves on success at `0x2d2f7`; the type switch at `0x2d2f9..0x2d35c` subtracts party protection for types 2-5 and selects the frame/sound. |
| Element save loop | `0x2d35e..0x2d374`: calls `0x2d0c2` with the damage type, leaves the loop for `0x2d376` when the save fails, otherwise halves the damage and repeats from `0x2d35e` while it stays nonzero. Drawn once even at zero damage. |
| Power Shield and clamp | `0x2d3a9..0x2d3b5`; zero damage jumps to the subtraction at `0x2d6b5`. |
| Secondary save and dispatch | special byte test `0x2d3e3`, Physical save `0x2d3ed..0x2d3fe`, `special - 5` bound `0x11` at `0x2d433`, jump table `0x2d6d9` (entries for specials 5..22). |
| Handlers | Poison `0x2d442`, Sleep `0x2d462`, Disease `0x2d4a5`, Insane `0x2d486`, Confuse `0x2d4c4`, Curse `0x2d4e4`, Break Weapon `0x2d508`, Weaken `0x2d541`, Drain SP `0x2d561`, Paralyze `0x2d56e`, Aging `0x2d592`, Unconscious `0x2d5a2`, Eradicate `0x2d5c6` (helper `0x2bd6a`), Stone `0x2d611`, Death `0x2d641`, Curse Item `0x2d671`; specials 6 and 11 map to `0x2d6b5`. |
| Saving throw helper | `0x2d0c2`: selector 0 adds level and `statBonus(Luck)` and draws `1..v + 20` (`0x2d0fe`); selectors 1-6 add the resistance pair and `itemScan` bonus and draw `1..r + 40` (`0x2d202`); success when the draw is at most the value (`0x2d212..0x2d21d`). |

## Event dispatch

**2026-10-21 and 2026-10-23; DOS static, high confidence**, plus the
maintainer's 2026-10-23 DOSBox confirmation that right-click always acts as
left-click on buttons and in the 3D view. Read-only WSL Python/Capstone analysis
of the English World CD `WORLD/XEEN.DAT`, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
Addresses below are file offsets; Ghidra linear address adds `0xa940`, and
module-relative `segment:offset` resolves as `0x56c0 + segment*16 + offset`.
External traces and the full input/handler tables are in
`D:/Projetos/MModern/MModern-re/m84-part0/findings.md`; the earlier main-loop evidence is in
`D:/Projetos/MModern/MModern-re/event-dispatch/findings.md`. ScummVM `6814ee9b` was comparison
context, not the authority for these DOS conclusions.

The post-combat extension (M85 Part 0, 2026-10-23) is retained in
`D:/Projetos/MModern/MModern-re/m85-part0/findings.md`, with hash-asserting scripts and bounded
traces alongside it. New paths were calibrated against Event entry `0x26774`,
Wait's label clear `0xd472`, Run exit `0x2eb11`, the established M81 delivery
sites and M80 Arena generation. Conclusions below are **high-confidence DOS
static** unless explicitly marked otherwise; the new witnesses are static
movement proofs, not DOSBox observations. The window-first ordering is a
maintainer-approved deviation, not a DOS finding.

Calibration reused the established movement/door-label callers, M77's sheet
and Item Use debit, and M78's map rebuild. New input paths were checked against
the known arrow, Space, F1-F6 and icon actions. The Save helper was identified
by its original saving/completion messages, party block and map-state writer.
Tail selection was checked against both object builders and the renderer's
one-based-to-zero-based conversion. No original program bytes or disassembly
are stored in the repository.

| Finding | File offsets and calibration |
| --- | --- |
| One pass after each surviving outer input; idle frames do not repeat | `0xd18c` calls cell lookup `0x11f04`; flag `0x10` gates `0xd1a5 -> 0x26774`; delivery `0xd1b7`; idle draw `0xd1c3 -> 0x10bc3`. Unmatched key `0xd2e9`, common tail `0xd5c7`, loop-back `0xd5e5`. Calibrated by Wait's door-label clear and the earlier M81 callers. |
| Full entry without a matching record | WhoWill clear `0x26797`, stash setup and line-0 scan; `(x\|y\|facing)==0` exclusion `0x26b5f..0x26b69`. Matching position/line/facing `0x26b1f..0x26bf6`. |
| Physical keyboard translation | Keyboard IRQ installation `0x4557b..0x455a0`, producer `0x455bf..0x45691`, Set-1 table `0x456e8`, consumer `0x45436..0x45555`, wrapper `0xe149..0xe185`. Known arrows `f0..f3`, Space `20`, sheets `c9..ce` calibrate the table. F7/F9 are unmatched; F8 returns from CD replay `0x3dbfb`; F10 is intercepted `0xe17a` and cleared `0xe17f`; F11/F12/Delete have zero translations. Ctrl subtracts `0x40` from plain codes at least that large; Alt's high byte is stripped. |
| Coordinate-only mouse input | Both press masks registered `0x43758..0x43766`; callback `0x43832..0x4384e` queues coordinates only; reader `0x4384f..0x438e4` returns only 0/1. Escape branch `0xe0dc` requires greater than 1 and is unreachable. DOSBox confirmation above independently corroborates the result. |
| Mouse bounds | Main table DS `0x197c`, file `0x4f20c`; decoder `0xe32d..0xe396` uses exclusive upper bounds, as in established YesNo calibration. Portraits `0xe2cc..0xe313`. View override `0xe118..0xe136` includes x 8..224/y 8..135. Outer manual predicate `0xd1f4..0xd217` includes y through 140: a stationary top-of-panel-icon click can enter manual Events first. Feedback poll `0xe426` can update the coordinates used by that predicate. |
| All 26 table entries and nested Item Use | Keys at `0xd63c`, handlers `0xd670`; ordinary routes join `0xd5c7`. Sheet `0xd4b8 -> 0x3a1a6`; Items `0x3a483 -> 0x3a652`; Use `0x3b3dc -> 0x311ae -> 0x313b7`, known charge debit `0x3145c`; null-member completion `0x3b3f4`, sheet stepped `0x3a493`, sheet return `0x3a5c1`, optional monster step `0xd4c8`. Cast return `0x33829`, Rest `0x1e827`, Map `0x13aad`, Info `0x1f6ff`, Quick Reference `0x3c515`, Quests `0x3402c`. |
| Nonordinary exits | Successful panel Load returns 1, main sets result 2 at `0xd2fa` and leaves through `0xd5e8/0xd61b`; outer `0xb836 -> 0xb819` restarts. Quit `0x1f20d -> 0x21ecd` reaches runtime termination. Death/session result checked at `0xd5d5..0xd5e3`; manual terminal result leaves at `0xd2cd`. Mr Wizard returns 2 from the panel (`0x1f296`) and takes the ordinary pass, not this restart. |
| Save is nested in the panel | Save `0x1f1e6 -> 0x1ebc2`, return `0x1f1e9 -> 0x1ef3d`; Save As chooser `0x1f139`, return `0x1f143/0x1f14a -> 0x1ef36`. Calibration: save/completion text `0x532b1/0x532cf`, party block and map writer `0x265f4`. Panel close `0x1f2f8..0x1f331` completes one input regardless of save count. |
| Title Load, panel Load and New Game converge | Title Load `0xc162/0xc198`; Start `0xc113/0xc12d`, New initialization `0x20b74`, roster `0x20beb`, difficulty `0x20c14`, initial save `0x20c38`; title exit `0xc66b..0xc766`, outer call `0xb829 -> 0xcfdf`. Panel Load `0x1f17d/0x1f193` uses the restart above. Map load `0xd086/0xd08c -> 0xf542`; indoor rebuild `0xd0bd/0xd0c2`, outdoor `0xd0ce/0xd0d3`; initial view `0xd0da`; Event `0xd1a5` afterward. |
| Manual run, current-key continuation and final pass | Manual call `0xd2be`; current `f5fe` read `0xd2d0`; Space/W Wait `0xd468`, monster step `0xd46d`, label clear `0xd472`; final pass `0xd5e5`. The original input is not preserved across manual dialogs or clearing. Indoor wall types 1/6/9/13 call grate handler `0xc794` before manual; handled path clears key `0xc911`. |
| Wait can make the second scan skip | Attack slot 0 test `0x26abd`, skipped flag `0x26ac4`, jump to cleanup `0x28d50`; non-skipped clear `0x26acc`. Entry/WhoWill and the tail still occur. Post-combat callers `0x1184a` and `0x2ec37` remain separate evidence. |
| No generic post-script keyboard flush | Common completion `0x28d50..0x291f6` and its close/death/treasure/context callees were audited. Final `0x291eb -> 0xe6bf` restores button context, not input. Keyboard clear `0xe097 -> 0x45413` drops queued makes and clears current code; Event direct sites are text-page continuation `0x26e41` and teleport `0x27112`; map load `0xf590`, NPC `0x25ad9`, actual treasure presentation `0x2429e` also clear. `0xe07c` separately drains mouse clicks and zeros current `f5fe` at `0xe090`; NPC calls it at `0x25ac7` before waiting. NPC `0x25ad9` is conditional on remaining text (`0x25ad1..0x25ad7`), also before waiting; final acknowledgement is read at `0x25ba6`, with no later unconditional clear before return `0x25ca7`. This corrects the 2026-10-21 report's blanket flush claim; the pin does clear on `_scriptExecuted`. |
| Typematic applies to ordinary non-navigation makes too | Ten-key capacity set `0xb3b8..0xb3bf`; every nonzero make enters `0x45664..0x4568c`; ordinary releases do not purge. NumLock release calls clear at `0x455e5`. Exact hardware/emulator timing remains Unknown; the approved M63 navigation deviation is unchanged. |
| Tail window and sound | `0x28e97..0x28efb`: window 38 iff executed marker clear, selected object nonzero, other-facing marker clear. `0x28efd..0x28f1c`: FX21 iff executed marker clear, selected object nonzero, other-facing marker set, sampled Event flag `f482` zero (`0x28f13`). Neither tests Space. The window's acknowledgement does not generally flush keys. |
| Tail markers and selection | Entry clears locals at `0x2678c/0x2678f`; wrong-facing marker set `0x28d3d` only for current position/requested line. Shared executed-marker tail `0x28b2b`; CallEvent `0x28375` does not mark by itself; SetChar's cancelled selection can exit `0x27a2c` before marking. Selected object `98d9` is index+1: indoor `0x156ee..0x15836`, outdoor `0x170a4..0x171d1`, renderer subtracts one `0x10c69..0x10c6c`. Known nearest-object coordinate tables select the party's logical cell. |
| Skip is a line-head test, not just an entry guard | `0x26abd`, set `9756` at `0x26ac4`, admitted-scan clear `0x26acc`; line increment `0x28d37` and back-edge `0x28d3a`. Entry `0x26797` precedes the test; cleanup `0x28d50`, mode restoration `0x28e8f`, stash restoration `0x28f22` and return `0x291f6` follow it. A between-line skip retains the executed/other-facing markers already accumulated. |
| Two direct combat entries; caller 3 belongs to the draw | Relocated far-call inventory to combat `0x2e61f`: draw `0x1183e` and exploration Shoot `0xd5aa`. Draw gate `0x11802..0x1183c` tests slots 0-2, mode 1/5, `97f6`, `97f9`, `96c4`; caller-3 flag test/call `0x11843/0x1184a`. Shoot `0xd575..0xd5aa` has no `96c4` test or caller-3 suffix; resumes missile action `0xd5af`, action completion `0xd5b4`, common tail `0xd5c7`. |
| Caller 4 uses saved label state and the current cell | Combat saves `3066` at `0x2e62d..0x2e631`, clears it `0x2e634`; saved-value gate `0x2ec18`, current-coordinate lookup `0x2ec1e..0x2ec28`, Event-flag gate/call `0x2ec30..0x2ec37`; return `0x2ec8b`. There is no assignment restoring `3066`. A scanning caller 4 clears `9756` before the draw tests it. |
| Normal combat presentation, delivery and Run | Full Run `0x2e928..0x2e949` calls landing helper `0x1691f`; surviving contacts and casualty handling `0x2eaf5..0x2eb69` include known Run exit `0x2eb11`. No contacts after a partial Run/win bypass that relocation. Window close `0x2eb6b`, keyboard clear `0x2eb73`, mode 2 `0x2eb78`, draw `0x2eb7f`, M81 delivery `0x2eb85 -> 0x24128`, mode 1 `0x2eb8a`, UI restoration `0x2eb94..0x2ec13`, then caller 4. Delivery still applies its occupied-slot entry gate (`0x24158..0x241e4`); only mode 9 bypasses it (`0x2414e..0x24155`). Mode 2 defers staged treasure when monsters remain visible. Retained M81 `combat-return.dis` and `delivery-entry.dis`; maintainer correction, 2026-10-23. Ordinary pass delivery remains `0xd1b7`. |
| Return to the enclosing handler determines the next pass | Idle draw `0xd1c3` returns to input `0xd1c9`; surviving handlers join `0xd5c7/0xd5e5 -> 0xd18c`. Cast/Shoot pre-draws `0xd4fe/0xd56f` can admit combat. Panel/Info/Map/Dismiss suppress through `96c4`; sheets use mode 10, casting mode 3, Rest mode 5. Manual Event returns to current-key dispatch `0xd2d0`; terminal exclusions remain M84's. Event instructions set mode 9 at `0x26bf9`. |
| Defeat, Load and Quit are distinct | Early no-active-character branch `0x2e719..0x2e720 -> 0x2ebc5` bypasses the normal redraw/treasure prefix but still reaches caller 4. Later death check `0x2ead3..0x2eae4` joins ordinary combat cleanup. Main death presentation `0xd604` belongs to the terminal continuation. Combat panel Load refuses mode 2 at `0x1f14d..0x1f162`; Quit `0x1f20d -> 0x21ecd` terminates without callers. |
| Arena uses normal draw admission | Opcode 17 `0x27b29 -> 0x1cea5`; service call `0x1d385 -> 0x2b784`. M80 generation ends with East facing `0x2bd2c`, Run-coordinate helper `0x2bd31`, window close `0x2bd36`; resource/context return `0x1d38a..0x1d426 -> 0x1ceb8` and Event completion `0x28d50`. No direct combat call. Subsequent eligible draw has the ordinary callers; Arena awards remain a later service interaction, and map 85 retains M81's monster-loot exclusion. |
| Combat relocation returns through the combat loop | M76 learned movement guards reused. Item Beacon `0x39f51..0x39f69` and Teleport `0x39fd9..0x39fe5` refuse; item Portal `0x3a058..0x3a075` continues to `0x30654`. Town load `0x30779..0x30798`; item return `0x314c9..0x314d3`, sheet exit `0x3b3f4..0x3b3fe`, combat continuation `0x2e909..0x2e955`. Time Distortion `0x30301..0x3031e` uses Run helper and draws in combat mode. Relocation itself does not force exit; later contact tests govern it. **Unknown:** exact first presentation with stale pre-Portal slots; capture slots and the first post-item combat iteration in DOSBox. |
| Mr Wizard's combat-only ordering | Combat window 2 flag `986e`, calibrated against window helper `0xdddb`; panel gate `0x1f23e` skips immediate town load while that window is open; gems zeroed `0x1f28a..0x1f290` before result 2 at `0x1f296`. Combat handles it `0x2e989..0x2e996` by setting a local deferred-load flag and jumping to exit `0x2eaf0`. The exit's escape branch still applies: flag `fafe` (cleared at entry `0x2e6d9`, set by each successful Run `0x2e1f7`) with any contact slot `0x2eaf5..0x2eb0f` reaches defeat gate `0x24f29`, Run landing `0x2eb1f` and casualty loop `0x2eb24..0x2eb69`; no currency forfeiture (full Run only). Ordinary cleanup `0x2eb6b..0x2ec13`, caller 4 `0x2ec18..0x2ec37`. Deferred load `0x2ec3c..0x2ec7e` (gated by that flag) follows caller 4: FX 51, map 28/East or map 29/South according to game side, then Run coordinates; gems are not zeroed again. Draw-origin caller 3 follows at the new address if still flagged as skipped, but its line-head test `0x26abd` reads slot byte `98b4`, written only by the indoor/outdoor builders (`0x14413/0x14523`, `0x16939/0x169d2`); loader `0xf542` and the tail call no draw or builder, so it tests the last build before the load (cleanup draw `0x2eb7f`, the same-position delivery and UI draws, or caller 4's Event draws; independent review byte scan, 2026-10-23). MMModern keeps that slot 0 through frame preparation while post-combat calls are owed; an owed ordinary pass is marked and started before the next frame. Pin `interface.cpp:1700-1702` breaks to the same escape branch `:1870-1882`. Implemented 2026-10-23 (maintenance): `XeenCombat::mrWizard`, `XeenEncounterFlow::mrWizardAfterCombat`, post-combat block in `XeenEventFlow::prepareJourneyTransition`. |
| Actor movement and slot construction are different stages | Monster step `0x1fdc1`, movement helper `0x1fbf2`; indoor draw/build `0x11495 -> 0x14406`, outdoor `0x1106c -> 0x1692c`. Builders clear `98b4..98cd`, then fill 0/1/2 in order at `0x14523/0x14591/0x145fe` and `0x169d2/0x169e3/0x169f4`. Death removes the actor at `0x2c542` and redraws `0x2c576`, rather than making a stable hole in contact slots. |
| Between-line builders | Spawn `0x27a59` changes actor records only. TeleportAndContinue map load `0x270f3`, coordinate writes `0x27106..0x2710f`, clear `0x27112`, draw `0x27121`, restart `0x26a8e`. Fall `0x28adc..0x28afe -> 0xca60` includes draws `0xca91/0xca9e/0xcced`, map load `0xccc2`, then restart `0x28b08`. Map loader `0xf542` has no direct slot rebuild. Label draw `0x26f81` supplies the Clouds witness below. |
| Engaged input and ordinary combat key clearing | Reader `0xd1c9` has no slot predicate; admission belongs to draw. Missile gates are set at `0x3739a` (`97f6`) and `0x1d936` (`97f9`). Combat entry has no unconditional queue clear before reader `0x2e7ec`; normal exit clears at `0x2eb73 -> 0xe097`. **Unknown:** frame-exact reachability of each projectile/input interleaving; break at draw admission and the two readers, record gates/current key/FIFO, without changing state. |
| Window-38 order and approved deviation | DOS restores mode `0x28e8f`, opens/writes/updates window `0x28eb0..0x28ec8`, draws `0x28ed5`, acknowledges `0x28edb`, restores stash `0x28f22`. Pin `scripts.cpp:241..281` likewise restores mode, opens window, draws before polling acknowledgement, clears events/closes, then restores saved treasure. Maintainer decision 2026-10-23: MMModern finishes the skipped call/window first, automatic or manual, then starts combat. Nested-stash outcome not traced. |
| Door-text setters, rendering and temporary suppression | Opcode dispatch 2/3/4/39 to `0x26f48/0x26f52/0x26f5c/0x26f66`, common set `0x26f7a`, draw `0x26f81`; rendering predicate `0x117b6..0x117ca` also requires empty slot 0. Clears `0xc930`, `0xd023`, `0xd448`, Wait `0xd472`, combat `0x2e634`. Text/numeric input helper `0x1df36` saves at `0x1df3f`, clears `0x1df53`, **restores** `0x1e005`; near callers `0x1e05c/0x1e1d4`. Map-load message `0xf575 -> 0x1df10` is a different helper. Teleport buffer clear `0x3054d` does not clear `3066`. |
| Both flags are unsaved; lifetime differs at gameplay entry | M81/M84 save ranges reused: `maze.nam` DS `96c5`, length 31; `maze.pty` DS `ed8c`, length 1530; `maze.chr` allocated roster, length 10620. Neither flag overlaps. Direct, overlapping and bulk-write audits find only skip/scan writers for `9756`; startup BSS clear `0x5773..0x577c` covers it. `3066` starts zero in initialized data and has the writers above. Load/New Game converge on gameplay clear `0xd023`; map load, title return `0xb90f..0xb98b`, New Game party initialization and archive restoration do not reset `9756`. Title alone does not clear `3066`. |
| Vertigo caller-4 and between-line witness from original AI movement | Map 28 `(13,4)` West, EVT offset `0x1253`, opcode 2/line 0; DAT attribute `0x11`, wall `0x8084`. Original MOB actor 35 `(15,4)` can awaken in the East view, then step west via `(14,4)` to `(13,4)`. AI branch `0x20086`, tables DS `5788/581b/279e`, wall helper `0x1fc91..0x1fd8f`: both source wall words zero, tested nibble 0 below map threshold 7; destination occupancy below 3. Same-cell slot lookup accepts all facings. Wait step `0xd46d`, clear `0xd472`, pass `0xd1a5`, label draw `0x26f81`, next head `0x26abd` prove the abort and entry label. Resource and table checks retained in `resources.py`, `ai-proof.py`, `ai-witness.json`. **High static**, not a DOSBox play observation. |
| Pin never clears a previously skipped Event on scanning | Exact checkout `6814ee9ba54582f5b5adcffab49efbbd8f589edd`: `_eventSkipped=false` only in constructor `scripts.cpp:123`, true on skip `:177`, read by `interface.cpp:1388`. No counterpart to DOS `0x26acc`. This comparison does not upgrade any DOS conclusion. |

The Clouds census uses the CD's `GAME/XEEN.CC`, SHA-256
`8c0ac4c0650080cc85e96af3e9d5cd9968dad16bfd2ed0785d77d21893cc155c`,
read through M78's archive parser. The retained final script/result are
`D:/Projetos/MModern/MModern-re/m84-part0/clouds-inventory/final_domain_sweep.py` and
`final-domain-sweep.json`. Calibration: require the DAT's embedded ID to match
the requested ID; DOS resource naming `0xf8d0..0xf8e1` uses `mazexNNN` above 99;
resolve the known indoor neighbour lookup before testing the flag, while using
the owning map's EVT/MOB coordinates. Rivercity map 30 `(30,30)` resolves to
physical DAT 114 `(14,14)`, the established M78 witness. Outdoors use 16x16,
indoors 32x32; all 85 base maps have EVT/MOB resources. EVT length counts the
body after its length byte.

The result is 128 physical DATs and 300 flagged cells, each represented once in
the playable domains. There are 299 cells with coordinate records; map 25
`(27,3)` alone has none, and no object there can trigger the tail window.
There are 135 flagged cells with same-cell objects; 18 have only one eligible
line-0 facing, giving 54 wrong-facing cases where the window is excluded and
the flag suppresses sound. Their map/cell lists and the full input table are in
the external findings. First matching instructions are 4, 6, 7, 9, 15, 25, 39
or 43. All mark execution before an ordinary tail; the CallEvent entries first
call IfMapFlag(42) or SetChar(15, parameter 0), which also mark. No unchanged,
unengaged flagged-cell window witness remains. The old 376/187/189 counts
included hash aliases and compared continuation-tile coordinates with the wrong
EVT/MOB owner; they are superseded, not additional unexplained cells.

**Unknown:** exhaustive reachability after Event/object mutations or an
occupied-slot skip. The predicate is settled, but the resource census is not
an exploration of all saved states. The smallest skipped-call check is Wait
on an object-bearing flagged cell while watching attack slot 0, then observe
window 38 before combat resumes.

**Unknown:** exact non-navigation typematic timing under a particular DOSBox
configuration. The smallest check is a held unmapped key on the unguarded
Tower of High Magic damage trap, map 55 `(7,8)` (map 26's traps lie on clouds,
where the party falls without Levitate), with repeat settings and successive
damage times recorded. Unusual host keyboard layouts/keypad Equals also have
no measured DOS equivalence. The static US Set-1 table is explicit above;
MMModern's host-action exclusions are in [World Events](world-events.md#event-dispatch).

## M87 Inn and party size

**Scroll input and F-key follow-up, 2026-10-24:** calibrated, hash-checked
static slices `input-skip-c.dis`, `key-consumption-c.dis`, `mouse-query-c.dis`,
`input-tail-c.dis`, `inn-after-scroll-c.dis`, `item-fkeys-c.dis` and
`sheet-fkeys-c.dis` remain under `D:/Projetos/MModern/MModern-re/m87/`. The poll consumes the
interrupting key/click; the Inn polls anew rather than reusing it. `0xe6bf`
restores button context, consistent with the M84 input audit. The Items map-0
guard precedes both switching and transfer, while the sheet's separate F-key
handler permits active-member switching. Exact offsets and scroll final-image
and stall rules are in [party rules](party.md). No new DOSBox play is claimed.

**Scroll follow-up, 2026-10-24:** the same hash-checked image and calibrated
16-bit Capstone workflow establish 22 opening and 22 closing steps, one BIOS
elapsed-tick wait with reset before composition, and the Inn's single retrace
full-palette restoration (`fadeIn(0x81)`). Exact offsets and the remaining
partial-frame/overlap Unknowns are in [party rules](party.md). Retained slices
`scroll-b-full.dis`, `inn-open-b.dis`, `inn-close-b.dis`, `scroll-clock-b.dis`,
`palette-b.dis` and `flip-b.dis` stay outside the repository under
`D:/Projetos/MModern/MModern-re/m87/`. REA again rejected the Windows path before opening a
session; no REA analysis or new DOSBox play is claimed. Maintainer's 60 fps
recordings `DOSinn.mkv` and `MMMODERNinn.mkv` establish MMModern's former,
less fluid scroll cadence, fixed in M87 Part B; their distinct captured
pictures do not determine the static loop's step count.

Checked 2026-10-23 against clean HEAD = `origin/main` =
`f77720f8c46f6bdd68a0ff19b9313a55849c78d0`. Rules are in
[The Inn, membership and party size](party.md) and the
[combat targeting section](combat.md#monster-targeting-and-hit-resolution).
This is an evidence record, not an assertion that those rules are implemented.

**DOS static, high confidence:** read-only analysis of the CD
`WORLD/XEEN.DAT`, 353511 bytes, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
REA rejected the supplied Windows paths before opening a session, so the
analysis used WSL Python3 and Capstone 5.0.7 in 16-bit mode. File offset is
`0x56c0 + segment*16 + offset`; Ghidra linear address is file offset plus
`0xa940`. The image was hash-checked by the trace helper. No original files
were modified; no new DOSBox play is claimed.

Calibration passed against the settled [Tavern Food](time-and-conditions.md#tavern-food)
rule: map 28 reaches `0x1b1c4..0x1b1da`, multiplying active count `DS:ed8c`
by 15 and selecting price 10. The unsigned existing-food comparison is at
`0x1b2c2..0x1b2c9`; food is replaced after debit at `0x1b2e3..0x1b2e6`.
No condition filter intervenes. This establishes the count identity and
address interpretation against M79 before following Inn consumers.

**Resource inspection, high confidence:** the existing developer commands
`--inspect-party`, `--inspect-map` and `--inspect-events` read the installed
`GAME1.GOG`; maps 28–32, 49 and 73–74 were inspected. The party command
prints active characters only, so the complete roster/portrait census reused
the existing external ISO/CC decoder. Initial `maze.chr` is 10620 bytes at
inner-save offset `0x989`, SHA-256
`c1bb681d2a9c5b3b29b2b75ad314328debfd6f512d942d699c4891a5f6da227a`.
Six named slots and their metadata are listed in [the roster rule](party.md#roster-and-list).
Tavern action is 3: map 28 Event offset 16, map 30 offset 7 and map 49
offset 45. No Tavern placement was found in the other enumerated maps.
This is a census of those resources, not a failed executable search used as
proof of absence.

External findings, scripts and instruction slices are retained under
`D:/Projetos/MModern/MModern-re/m87/`, chiefly `findings.md`, `roster.json`,
`resource-summary.txt`, `tavern-sites.json` and the consumer slices.
Existing `event-dispatch/`, `m79-services-20261020/`, `m85-part0/`,
`conditions-20261022/` and `skills-20261019/` evidence supplied settled rules;
the retained M76/M81 ranged and treasure boundaries were also reused.
Preliminary slices beginning mid-instruction are not evidence for their
leading bytes. The trace helper's derived call-target annotations apply to
direct far calls only. The target-draw verdict below supersedes a
preliminary one-draw interpretation. Preliminary action-1
exit lookups were Smith lookups, not Tavern evidence.

| Question | Verdict, file offsets and evidence |
| --- | --- |
| Roster/list | First 24 slots, named and matching town/side, ascending IDs: `0x39576..0x395d5`; In Party and face mapping `0x38e9c..0x39090`; page step/limits `0x398bb..0x398d4`. High, DOS static; portrait availability is Resource inspection. |
| Sign In admission/lodging | Direct S handler `0x1b6b0`; active CHR side/map writes `0x1b754..0x1b784`; no terminal-condition refusal. High, DOS static. |
| Exits | Vertigo/Rivercity table `0x506ca/0x506ce`; Shangri-La override `(11,12)` West `0x1b723..0x1b734`. High, DOS static + Resource inspection of callers. |
| Time | AddTime(1440), then map 0, `0x1b786..0x1b794`; calendar/interest and mode-17 dawn exception `0x263b3..0x26491`. No food, Rest or condition ticks. High, DOS static with retained time evidence. |
| Forced Save order | Farewell/wait `0x1d688..0x1d6cf`, prior song `0x1d763..0x1d773`, forced Save `0x1d7a4`, Event completion `0x27b1f..0x27b3b`, closing scroll `0xd5de..0xd60f`, Inn song/opening `0x394d4..0x394f4`, `0x39738..0x39745`. High for call order, DOS static; audiovisual overlap Unknown. |
| Saved state | Live-party preservation/roster copy `0x1ec37..0x1ed26`, map flush `0x1ed29`, snapshot `0x1ed2e`. Map 0, prior map, door/facing, post-day time, pre-Inn membership and full roster. High, DOS static. |
| Save destination | `0xe938` writes roster/party to DARK.CUR; `0xec85` uses selected slot `DS:95a6` for numbered WOX copy; `0xe9b8` copies XEEN.CUR in normal World play. New Game clears its special-copy flag at `0x20c3d`. High, DOS static. |
| Ordinary versus forced Save | Argument check `0x1ebc8..0x1ec2a`, final acknowledgement `0x1ed46..0x1ed98`; ordinary panel combat gate `0x1f1cb..0x1f1e5`. Shared copying/snapshot/disk reset, forced path skips admission/prompts. High, DOS static. |
| Error paths | Open failure to `0x21ecd`; archive carry-error termination `0x45c8a..0x45cfd`; copy-loop write results unchecked at `0xedb1..0xee09`. High for branches, DOS static; physical partial-write outcomes Unknown. |
| Load/Exit | Roster/active reconstruction and map-0 Inn dispatch `0x39460..0x394bc`; Exit restores prior map, copies and snapshots `0x39b1b..0x39b2b`. High, DOS static. |
| Other Inn saves | Add/Remove alone do not snapshot; confirmed Delete `0x39a2e..0x39a32` and Create return `0x39a96..0x39a9a` do. High, DOS static; M88 owns their editing rules. |
| Return | Unconditional map load and actor rebuild `0xd086..0xd0d3`, ordinary flagged-cell Event pass `0xd18c/0xd1a5`. High, DOS static with retained M84/M85 dispatch evidence. |
| Input/refusals | Key table `0x39c59`; Add/FX21 `0x39b33..0x39beb`; Remove `0x39a3a..0x39a73`; empty Exit `0x39ac1..0x39ad0`. High, DOS static. Full mouse table in party chapter. |
| Dismiss | Exploration dispatch `0xd45b..0xd460`, absent from combat table `0x2ecad`; window/refusal `0x20e81..0x20f6d`; compaction/tail `0x20f70..0x21004`; Save's deferred Vertigo lodging `0x1ec97..0x1ed1e`. High, DOS static. |
| Dismiss drawing | Poll loop `0x20eb8..0x20ec4` calls `0x10bc3(0)`; counter advancement `0x10bda..0x10be7`; the routine's first argument controls the optional post-draw update at `0x117d2`, separately from scene mode `DS:9a1c`. Retained `m86/door.dis` plus `m87/dismiss.dis`; high, DOS static. |
| Poison Volley visuals | Six flags enabled `0x1d899..0x1d8bb`; hit loop bounded by active count `0x2cb46..0x2ccdb`. Settled 2026-10-27 (DOS static, high, calibrated on handler `0x30fc3` against the pin's `Spells::poisonVolley`): the sprite placement loop `0x1d9f5..0x1dac3` stops at the active count `DS:ed8c`, a hit clears its lane (`0x2cbab..0x2ccb6`), and exploration Cast, combat Cast and item power 46 (`0x39e4b`, `0x3a005`) share the routine. Evidence `D:/Projetos/MModern/MModern-re/k1-k7-20261027/findings.md`. |
| Consumer bounds | Per-category verdicts and original call sites in [consumer table](party.md#shared-state-and-consumer-bounds). High for stated checks, DOS static including retained evidence; no whole-program absence claim. |
| Target draw count | Combat count switch `0x2d9a3..0x2d9b2`, six table entries at `0x2dc70`, conditional second draw `0x2d9c4..0x2d9e8`; eligibility fallback is later. High, DOS static. |
| Shared state | Full roster and 1530-byte party snapshot `0xe938..0xe97d`; membership copies do not split currency/food. Pending treasure is excluded, with gameplay-entry clearing `0xcfe3..0xcff7`; reuse [M81](#save-and-load-boundary). High, DOS static, including retained evidence. |

**CHR lodging layout, maintainer adopted 2026-10-23:** byte 339 is an
unknown byte; the lodging map is the word at 340–341. Save's `0x1ecae`
store addresses CHR lane base `0xb41e` plus `0x154`. The retained resource
audit reads the same word and finds 28 for the six named initial records.
The pin's unknown word at 339 and map byte at 341 overlap that DOS field.
`DS:ed8c` is the active count (first PTY byte), `DS:ed8d` the retained count
(second byte). Save clears departed IDs at `0x1ece7`, normalizes the retained
count at `0x1ed23..0x1ed26`, then snapshots at `0x1ed2e`; no active-tail
array is serialized. See the [layout difference](../original-quirks.md#sv-chr-lodging-layout).

The significant inactive-character witness is six-to-five Dismiss followed
by Save: the departed full CHR at lane 5 is read and lodged at Vertigo. This
depends on retained character/ID state and is bounded by the six physical
lanes; the saved canonical result is the full roster. It is not an assertion
that DOS serializes a separate inactive-tail array. The portrait compactor
also reads handle slots 6/7, but those belong to a larger resource cache,
not the CHR buffer (`0x389ff..0x38a44`, loader `0x1f700`).

**Reference comparison:** the pinned tree at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` differs in town filtering,
Shangri-La exit correction, Sign In/Exit saves, same-map return reload, and
the one/six-participant initial target draws. The pin's Dismiss erases the
active entry rather than retaining DOS's departed tail for the Save-time
lodging pass. Its Elder-weapon refusal is Swords-only and is not a Clouds
difference. Master was not rechecked. These are source comparisons, not
claims of current MMModern implementation.

### M87 runtime checks

**Unknown: audiovisual overlap and frame timing.** Capture normal DOSBox
video and audio from Vertigo's S input through the first interactive Inn
frame, then Exit. Mark farewell end, prior song, disk activity, closing
scroll, Inn song, palette/opening scroll and first accepted input. Static
call order above remains settled; the capture resolves overlap and duration.

**Unknown: physical partial-write outcome.** Use disposable game/save copies.
Make the selected numbered target unopenable, Sign In, and record messages,
termination and working/slot hashes. Separately exhaust a disposable writable
volume during archive copying and compare both CUR/WOX pairs afterward.
This distinguishes checked failures from unchecked partial writes; do not
infer rollback from the successful-path trace. Arbitrarily corrupted slot
state outside normal gameplay was not certified.

Useful runtime corroboration, rather than unresolved static verdicts: select
a disposable slot, record time/membership, Sign In and hash both CUR/WOX
pairs. Restart and Load: it should open the Inn without another day. Remove
and Add, Exit, then Load to check saved membership/door position. Repeat in
Shangri-La at `(11,12)` West. For targeting, break on the bounded RNG at
file `0x45b9a` translated to its loaded address: six able combatants with no
preferred target should call `R(1,6)`, then `R(0,4)` unless the first result
is 6, separately from any eligibility fallback.

## M88 Create and Delete

Checked 2026-10-24 against clean `main`, HEAD = `origin/main` =
`e8621070e52933af70d2bfd38c6fe28ae5ade00e`. The comparison checkout is
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The rules and complete
text locators are in [Create and Delete](party.md#create-and-delete).
This establishes original behavior, not implemented coverage or play-test
acceptance.

**DOS static, high confidence:** read-only analysis of
`F:/Games/gog/Might and Magic 4-5/WORLD/XEEN.DAT`, 353511 bytes, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
The file size and hash were checked before analysis; each disassembly helper
also asserts the hash. WSL Python3 with Capstone 5 in 16-bit mode reused
M87's tooling, without retrying the Windows-path REA route that M87 had
already found unsupported. No REA session or new DOSBox observation is
claimed. Mapping remains `file = 0x56c0 + segment*16 + offset`, and
`Ghidra linear = file + 0xa940`.

Calibration first reproduced the settled Inn Add rule in
`0x39b33..0x39beb`: missing/already-active rows return, `DS:ed8c` is compared
with six at `0x39b76`, admitted IDs append with a 354-byte record copy, and
the full-party path calls FX21 before its known text at `0x55561`.
This checks address mapping, active-count identity, roster stride and the
shared input/sound conventions beside the Create/Delete dispatch. Existing
M87 save, Load, Dismiss and Inn presentation evidence and M84 keyboard,
mouse, button-feedback and ErrorScroll evidence were reused, not recertified
through new play.

| Question | New or retained proof; confidence |
| --- | --- |
| 1. Entry and return | C/local-list capacity `0x39a76..0x39abe`; Create return `0x368d3..0x368e6`; unconditional known snapshot `0x39a96..0x39a9a`; Inn return redraw/fade `0x39460..0x39785`. DOS static, high. |
| 2. Free slots | Name-only scan 0–23 `0x35e40..0x35e90`, race/sex `0x35e95..0x35ecb`, slot limits `0x3637a..0x363d1`, face lookup `0x361b5..0x361dd`. DOS static, high; portrait availability reuses M87 resource inspection. |
| 3. Rolls and class | Rolls `0x359f2..0x35a93`, one-call bounded helper `0x45b9a..0x45bb4`, requirements `0x358d2..0x359dc`, first eligible selection/colors `0x35a94..0x35cb4`. DOS static, high. |
| 4. Screen and input | Key tables `0x368e7/0x351af`; parallel mouse arrays `0x4f5ba/0x4f656`; dice `0x351d0..0x355db`; initial/detail redraw `0x35f19..0x36263` and `0x363fd..0x36744`. DOS static, high for composition and waits; exact visible overlap Unknown. |
| 5. Record | Name `0x1e00f..0x1e179`; clear/init `0x3564a..0x358cb`; class/race skill and spell tables `0x4d770..0x4d7d1`; capacity calls `0x20182/0x3bb30`. DOS static, high. All CHR fields accounted for against the existing character/supplement and save fields, including separate learned-spell and award payloads. |
| 6. Delete | Selection `0x39109..0x39290`; protected weapon check `0x325f5..0x3261a`; confirmation `0x1ea3a..0x1ebc1`; clear, town-only list rebuild, membership removal and snapshot `0x39985..0x39a37`. DOS static, high; page index is preserved. |
| 7. Empty party/order | Count-bounded copy `0x392be..0x392f6`, unconditional snapshot payload writes `0xe961..0xe982`, Load `0xf013..0xf076`, Inn entry/Exit guards `0x39460..0x394bc` and `0x39ac1..0x39ad0`; earlier Dismiss settlement `0x1ec97..0x1ed2e`. DOS static, high, reusing M87. |

The RNG audit follows the ordinary Create, exchange, name, Delete and confirm
paths and their shared UI helpers. Its relocation-backed direct-call census
finds the sole roll call at `0x35a2b`; dice arithmetic/polling and name mode
255 account for the suspected timing-dependent paths. The latter suppresses
the shared key helper's gameplay draw. The dice calls `0x5efb` are far-pointer
arithmetic, not random draws. No extra save site, skipped normal Create-return
snapshot, or generator call outside attribute rolls was identified in these
paths. This bounded audit does not prove absence across unrelated program
code. The known clock-mixed DOS generator itself remains the plan's deferred
difference.

**Resource inspection, high confidence:** the retained read-only ISO/CC
decoder inspected installed `GAME1.GOG` / `XEEN.CC`. All empty `maze.chr`
records (24 of 30) contain zero in every field; its 10620-byte resource at
inner archive offset `0x989` retains M87's SHA-256
`c1bb681d2a9c5b3b29b2b75ad314328debfd6f512d942d699c4891a5f6da227a`.
The following offsets are within `XEEN.CC`, **not XEEN.DAT**:

| Resource | Archive offset | Length | Inspected layout |
| --- | --- | --- | --- |
| `create.raw` | `0x63abbf` | 64000 | 320×200 background pixels |
| `create.icn` | `0x6343ca` | 26613 | 66 frames; arrow clear frame 61 is 7×108, arrow frame 63 is 6×5 |
| `create2.icn` | `0x64a5bf` | 6036 | 16 frames; Escape frame 0 is 24×20 |
| `dice.vga` | `0x70bf1e` | 14417 | Eight frames; seven varying die sizes matching DOS tables, plus 152×83 background |

Scripts, full hashes, table derivations and instruction slices remain outside
the repository in `D:/Projetos/MModern/MModern-re/m88/`. Main locators are `trace.py`,
`probe.py`, `followup.py`, `tables.py`, `audit.py`, `capacity-compare.py`,
`calibration-add.dis`, `inn-dispatch.dis`, `create-main.dis`,
`create-helpers.dis`, `roll.dis`, `roll-details.dis`, `rng.dis`,
`exchange.dis`, `input-name.dis`, `name-key.dis`, `confirm.dis`,
`delete-selection.dis`, `protected-items.dis`, `party-copy.dis`,
`load-party.dis`, `list-draw.dis`, `memset-core.dis`, `hp.dis`, `sp.dis`,
`stat-bonus.dis`, `creation-tables.txt`, `text-and-audit.txt`,
`draw-and-capacity-tables.txt`, `capacity-comparison.txt`, and the
`resources/` inventory and sprite-metadata scripts/results. Long linear
slices can include embedded tables after a routine's return; only verified
entry-point slices and reachable instructions support the conclusions.

### M88 runtime checks

**DOSBox observation (maintainer, 2026-10-24),** with a 60 fps recording
(`Chegagem 1.mkv`) and four screenshots kept outside the repository in
`D:/Projetos/MMODERN INN/`. They corroborate the static rules above:

1. With six members, C opens Create. The Inn fades out in about 0.42 s,
   Create fades in over about 0.43 s; Escape fades Create out in about
   0.40 s and the Inn in over about 0.47 s, without the opening scroll. The
   Inn song plays on through both transitions without a gap or restart
   (audio level measured over the recording).
2. The dice roll while idle and stop while the exchange window or the name
   prompt is open; after either closes they continue where they were,
   without catch-up frames. The exchange window covers the dice area at
   the top left.
3. Unavailable classes are gray, available ones white and the committed
   class blue. The arrow starts on the first available class; Left and
   Right move only the arrow, and only Enter commits. R clears the
   committed class. A Gnome slot shows "Secret Doors" and a Druid
   "Direction".
4. With 24 characters listed in Vertigo, C shows "Your Roster is full!"
   without a sound. The globally full roster with fewer than 24 listed
   locally was not observed; that case rests on the static rule.
5. Deleting the only face on the last page leaves the page blank until Up.
   Confirming Delete makes no sound.
6. Loading the save written by a Create return with nobody in the party,
   or by deleting the last active member through its listed face, reopens
   the Inn with an empty party. Neither Load adds a day beyond Sign In's,
   and no removed or deleted character returns. The Load menu shows that
   slot's level as 0 ("W 0"), and the empty party bar keeps the yellow
   selection frame on its first slot.
   In the empty Inn, Exit refuses with its message until a member is added.

**Inn song after Load.** Loading a save made inside the Inn from the
title's Load menu opens the Inn in silence, and the map's song returns after
Exit. Loading a save outside the tavern from the title and then signing in
plays the Inn song, and so does an in-game panel Load into an Inn save.
**DOS static, high confidence (2026-10-24),** checked against these
observations. A byte search finds three direct writers of `DS:8d28`:
title-menu entry (`0xbcee`, set), the Inn's song start (`0x394f4`, set) and
the Inn routine's tail (`0x39c47`, clear). The one reader it finds,
`0x394cb`, skips `inn.m` while the flag is set. Every gameplay entry and every return code 2 of the main loop
calls the Inn routine `0x392f7` first (`0xb819..0xb839`, call `0xb824`).
Outside the Inn, that routine leaves through its tail at once
(`0x394b7..0x394c8`, `0x39c13..0x39c47`) and clears the flag; inside it, the
song starts only when the flag is clear. Only a title Load straight into the
Inn therefore arrives with the flag still set.

## M91 Clouds intro runtime checks

**DOSBox observation (maintainer, 2026-10-25),** five recordings of the CD
World edition kept outside the repository in `D:/Projetos/MMODERN videos/`
(analysis in `D:/Projetos/MModern/MModern-re/m91/dosbox/`). They corroborate the
[cutscene rules](cutscenes.md):

1. Other Options shows View Darkside Intro (`D`), View Clouds Intro (`C`)
   and, in a game already won, View Clouds End (`V`). Any other key, such
   as `j`, closes Other Options and returns to the menu.
2. A launch with the configuration's seen word at zero plays the full
   Darkside story (about two minutes) before the World title.
3. `C` plays the Clouds intro in about 90 s: the NWC and JVC logos, the
   stars and the Clouds of Xeen title, the lake and the tower, the tower
   room, then the hands open the parchment (about 0.5 s) on the tableau of
   King Burlock, Crodo, Lord Xeen and Xeen's Pet, where Crodo speaks for
   about 34 s. The hands close it (about 1.1 s), about half a second of
   black follows, and the World title fades in without its menu.
4. Space at the first logo and during Crodo's speech changes nothing: every
   scene cut of that run matches the uninterrupted run within one BIOS tick
   (0.06 s).
5. Escape during Crodo's speech ends it at once; the hands close the
   parchment as at the natural end and the title returns.
6. Escape while the hands hold the parchment opens it at once to the full
   tableau, and the intro continues with Crodo's speech.
7. The mouse pointer is hidden while Crodo speaks, and a left click does
   nothing.

No caption strip appears with the installed digital device, and no text-mode
flash is visible at the hand-off.

## M92 Clouds ending evidence

**DOS static, high within the cited routines; DOSBox capture and complete
archive-member comparison, 2026-10-25.** Baseline `1260911`; the DAT/EXE
identities and calibrated sprite, palette, timing, keyboard, archive and
audio contracts remain those in [cutscenes](cutscenes.md#evidence-and-calibration).
Capstone 5.0.7 x86-16; DAT MZ header `0x56c0`, data base `0x4d890`; EXE
header `0x2e30`, data base `0x1d880`. Work scripts, disassembly, hashes,
tables and resource extents stay in `D:/Projetos/MModern/MModern-re/m92/`, outside this repo.

The ending's `0x28c0d` call and the settled Inn's `0x1d7a4` both invoke
forced Save `0x1ebc2(1)`. Compiler structure-copy direction is verified at
`0x5d31`; active-record merge `0x1ecee..0x1ed11` and archive member update
`0xe96d` -> `0x45e36` explain the live roster saved in the repeat capture.
The full 745-member comparison uses the existing save codec with an exact
decode/encode round trip for each input archive. Both after numbered slots
are byte-identical to the corresponding CUR. Per-member payload hashes are
in `archive-diff.json`; map payload differences are in `map-diff.json`.

| Capture observation | Result |
| --- | --- |
| XEEN, all 301 members compared | Only `maze0078.dat` stepped bits and `maze0078.mob` change; Lord Xeen's record moves from `(5,9)` to `(-128,-128)` |
| DARK, all 444 members compared | Only `maze.chr` and `maze.pty` change |
| Active CHR | Four recipients gain 150,000 XP, all six gain award 42, terminal conditions and fight changes remain; the two excluded recipients are Ella and Seymour |
| Party | Map 78 `(5,8)` North becomes map 28 `(18,4)` West; Scepter retained; completion set; quest flag 23 cleared, game flags 60 and 75 set; seconds 62482 -> 62597 |
| Events and map 28 | No member changes on either side |

The maintainer reports Ella (third member) and Seymour (sixth) defeated.
Their stored conditions include Eradicated; Ella also has Unconscious,
Seymour Dead. Zippo retains Unconscious but qualifies for XP. The earlier
claim that the ending lost XP, award 42 and deaths compared the inactive
XEEN roster and is withdrawn: DOS saves the live roster, and no pre-fight
roster is retained.

The six known pre-fight class/level/XP records and the calibrated total-XP
helper reproduce `0071001722` in the older ending recording. This confirms
the normal-case score formula, not a particular split among that older
run's eligible recipients. Its post-ending archive pair no longer exists.
The repeat slot-06 pair is the archive evidence; do not conflate the two runs.

The video now lives at `D:/Projetos/MMODERN videos/old/ENDING.avi`:
266.756826 seconds, 320x200 ZMBV, 70.086 fps, PCM 48 kHz stereo. Analysis
artifacts `ending-1fps.png` and `ending-scenes.json` record timestamped
frames and detected cuts. The image was inspected, including the fixed
score and both key prompts. Speech, fades, loading and measured strides
prevent a universal fixed duration; see the [stage table](cutscenes.md#clouds-ending-presentation).

The external EXE manifest resolves all reached compressed frame families
and all 73 parade selectors, using the installed archive indexes and
validated EXE string pointers. The Part 0 source-equivalent Python decoder
checked the compressed-frame output extent and bit provenance, but omitted
the LZ match-source increment; its frame hashes are superseded by the
corrected audit and the compiled-reader test `XeenEndingPresentationTests`
recorded in [cutscenes](cutscenes.md#clouds-ending-presentation). Neither is
a rendered or audible comparison with DOSBox.

**Maintainer DOSBox checks, reported and accepted 2026-10-25:**

1. `showtime` plays the complete ending and returns to the title; Load
   restores the group at the same position.
2. `bloopers` plays nothing and gameplay continues.
3. `V` follows `D` and `C`, plays the ending with score `1234567890`,
   and returns to the title.
4. Space, Escape and a click do not skip the animations. A click does not
   acknowledge a score page; a key, including Escape, does. A key during
   a scroll only shortens that scroll.
5. Panel clocks are initially hidden. G, O, B, R reveals a 12-hour civil
   clock and play time; both advance. Reopening the panel hides them again.
6. Castle Xeen is absent on map 27. **Beacon limitation:** no Beacon was
   marked in the castle, so its refusal was not exercised. That gate
   retains **DOS static, high** evidence at `0x2feb3..0x2fee2`; it is not
   labeled DOSBox-confirmed.

These reports corroborate the stated observations, not clock rollback/DST,
the long-time formatter, caption-only hardware mode, I/O failure, or every
save field. Those retain their explicit static or source-equivalent labels.
The maintainer accepted Part 0 with the Beacon limitation. Detailed evidence
and the completed checklist are in
`D:/Projetos/MModern/MModern-re/m92/findings.md`.

## M93 Darkside startup runtime checks

**DOSBox observation (maintainer, 2026-10-26),** four DOSBox Staging captures
(ZMBV, 70.086 fps, cycles 80000) of the CD World edition launched through the
usual `Xeen - DOSBox Staging.bat`, kept outside the repository in
`D:/Projetos/MModern/MModern-re/m93/dosbox-sessao/` with timestamped contact
sheets and frame-change lists. The configuration started with the seen word
at one and was restored afterwards. They corroborate the
[cutscene rules](cutscenes.md#startup-dispatch):

1. **Repeat launch (A).** NWC, JVC, the World-logo tail and the title, with
   no pyramid or story; the title arrives at 24.6 s. JVC is silent (about
   -91 dB against -44..-51 dB on the full path), so the short path stops
   the song. Take B begins with the same short launch.
2. **Other Options `D` (B).** The full story plays again and the fresh title
   arrives 142.5 s after the key, as in the M91 seen-zero recording.
3. **Escape (C1, C2).** After `D`, Escape at the crystal ball returns to the
   fresh title with the menu hidden. The next
   launch (C2) plays the full story again, so the early Escape left the
   seen word at zero.
4. **Click.** A left click during the ball scenes skips nothing: the story
   stages of C2 stay within 0.16 s of B.
5. **World-logo keys (C2).** Space and Escape during the tail did not end
   it. Per-frame hashes place the first reveal at the first frame changed after
   the last fade-in step: A `15.842 / 16.399 / 24.573 s`, B
   `165.557 / 166.113 / 174.284 s`, and C2 `132.871 / 132.914 / 141.041 s`
   (fade end / reveal / title). The first hold is 0.557 s in A, 0.556 s in B
   and about 0.04 s in C2; reveal to title takes 8.174, 8.171 and 8.127 s.
   The earlier scene-threshold measurement missed the faint first reveal
   frame. No World frame was skipped and the last hold kept its length.

Key-down times were not logged; item 5 places the first key in the first
boundary hold from the frame timing alone. The no-digital caption take was
not recorded.

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
| Combat | Establish remaining enchanted multi-attack arithmetic. Eradicate equipment-frame behavior is settled in [Combat](combat.md#secondary-effects). |
| Damage and targeting | Confirm ranged continuation and elemental multi-attack arithmetic. [M87](#m87-inn-and-party-size) settles the count-dependent initial target draws; it does not measure the generator's statistical distribution. |
| Items | Trace the remaining unexamined item-prompt refunds and high-tier generation cases; M76 settles Recharge, Enchant and Item to Gold above. |
| Treasure | Extend delivery evidence to the special sword and other editions' quest branches; the Clouds quest display/count boundary is established above. |
| Magic | Extend control-spell duration and high-tier overflow checks. Holy Bonus is settled by [M76](#m76-spells); Poison Volley and Resurrection retain their earlier evidence. |
| Travel | Calibrate the combat state and first post-item combat iteration after Town Portal's combat item route ([M77 open question](#open-questions-from-this-check), [M85 Unknown](#event-dispatch)). M76 settles handler relocation/load/admission; M77 settles the Use path and the slot test; [M84](#event-dispatch) settles exploration post-item automatic-Event timing ([rule](world-events.md#event-dispatch)); terrain keeps its earlier evidence. |
| Time | Verify remaining script-mode dawn behavior and uncalibrated condition-byte edges. Current MMModern wrapping/saturation is not literal equivalence with the pin's live integer counters; see the time chapter. |
| Broader coverage | A complete Event opcode catalogue beyond [World Events](world-events.md), all services, AI/movement, creation outside the Clouds Inn (whose rules are in [Create and Delete](party.md#create-and-delete)), Darkside/World transitions and frame-level UI timing still need dedicated chapters and calibration; [Audio](audio.md) lists its own remaining gaps. |

These are evidence gaps, not instructions to broaden an active milestone.
Normal DOS play or calibrated static analysis should settle reachable disputes;
only the condition-counter fallback explicitly authorized in
[AGENTS.md](../../AGENTS.md) bypasses an otherwise unverifiable case.
