# Milestone 23 - Static indoor objects and an original readable gravestone

**Proposed specification for architecture/specification review.** This document
does not authorize implementation or record milestone acceptance. The stable
capabilities remain those in [project status](project-status.md).

## 1. Goal and acceptance boundary

Render supported static ordinary Clouds objects in the existing indoor scene,
using original directional appearances, indoor placement, wall predicates and
ordered rasterization. Demonstrate the result by reading the original gravestone
at **Clouds map 29 (Nightshadow), `(4,6)`, facing West**, identity
`{Clouds,29,14}`. Its original clue and acknowledgment already execute through
MMModern's event system; the missing capability is its indoor visual presence.

This is a rendering foundation with a real interaction acceptance case. It is
not Nightshadow gameplay certification, a normally traversable route, a dungeon
system, or implementation of the nighttime/combat behavior mentioned by the clue.
The twelve original ordinary-object view positions form the rendering contract;
support must not be hard-coded to one resource, map, coordinate or facing.

**Recommendation: one implementation stage.** There is no missing gameplay
prerequisite to place in a second stage. Command construction and production
composition are parts of one testable rendering increment. Existing interaction,
presentation and persistence owners supply the acceptance path without another
subsystem. Unit work may precede integration within the stage; this is not a
separate authorization boundary called 23A/23B.

## 2. Verified baseline and evidence authority

Planning began from the maintainer-specified baseline, verified on 2026-09-09:

| Check | Result before investigation |
| --- | --- |
| Branch | `main` |
| HEAD | `7f8a7d5396ffe46dc5ce41467ede9464d17a6b3e` |
| `origin/main` | `7f8a7d5396ffe46dc5ce41467ede9464d17a6b3e` |
| Direct remote `refs/heads/main` | `7f8a7d5396ffe46dc5ce41467ede9464d17a6b3e` |
| Working tree / staged state | Clean / empty |

Current source and tests at that SHA, the external original installation, and
the pinned reference establish the facts below. Read [AGENTS.md](../AGENTS.md),
[roadmap](roadmap.md), [M22](milestone-22-plan.md), and
[dependencies](dependencies.md) together with this plan. Historical M15/M16/M20
plans supply identity, rendering and restore context; their old current-state
statements do not override the accepted M22 code.

The inspected external ScummVM checkout was clean at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Relevant pinned locations:

- [interface_scene.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface_scene.cpp):
  `IndoorDrawList` (line 186), `drawScene` (412), `drawIndoorsScene` (587),
  `setMazeBits` (836), `setIndoorsObjects` (2674), `setIndoorsWallPics` (2898),
  and `drawIndoors` (3568).
- [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp#L388):
  `INDOOR_OBJECT_X`, `MAP_OBJECT_Y`, `SCREEN_POSITIONING_X/Y`,
  `DIRECTION_ANIM_POSITIONS`.
- [map.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp):
  `MonsterObjectData::synchronize`, `AnimationEntry::synchronize`, resource
  loading and `Map::getCell` (1284).
- [sprites.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/shared/xeen/sprites.cpp#L236)
  and [window.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/window.cpp#L265):
  scale masks, internal cell offsets, clipping and ordered `drawList`.
- [scripts.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/scripts.cpp):
  event lookup/dispatch, `cmdDisplayBottom` (1244), Action 44 (1727),
  `cmdRemove` (828), and `cmdMakeNothingHere` (1334).

Original resources were read externally from the legally obtained World of Xeen
installation. The inspected archive signatures identify the evidence, not a new
hard-coded installation requirement:

| Archive | Bytes | SHA-256 |
| --- | ---: | --- |
| `XEEN.CC` | 13435646 | `f8a00fa2c75799c131ed62057c7c3b61e8afbc292db7d6d53264e7a41ec46636` |
| `DARK.CC` | 11217676 | `7cbaffab761e54c3f31a994ce25a6795f92dbc696181a5c74cc318bba688bf0e` |

Investigation used existing map/event diagnostics and temporary external probe
programs linked to the existing M22 libraries configured with the pinned
dependency. The probes called the real loaders, resolver, checked sprite drawer,
indoor geometry builder and event system. Fixed single-target raster experiments
used reference-derived anchors/orders; they were not a new indoor object builder
or a production implementation. Native-size probe images established resource
appearance and target contribution. They do not constitute maintainer physical
SDL acceptance or a running-original-engine comparison.

## 3. Encounter certification and alternatives

### Selected original encounter

| Field | Established fact |
| --- | --- |
| Map | Clouds 29, Nightshadow; `maze0029.dat`, indoor, `wallKind=0` (town), `flags2=0` |
| Object source | Initial Clouds `maze0029.mob`, ordinary object list |
| Original record / identity | Zero-based record 14 / `{Clouds,29,14}` |
| Position / object direction | `(4,6)` / West (3) |
| Table slot / resolved resource | 3 / 14; do not use slot 3 as the metadata index |
| Visual source | `XEEN.CC/014.obj`, 4546 bytes, two directory frames |
| Metadata source | `DARK.CC/clouds.dat`, 1452 bytes, entry 14 |
| Relative initial frames / limits / flips | `[0,1,0,1]` / `[0,1,0,1]` / `[0,0,0,1]` |
| Appearance | Static gravestone with RIP lettering, established by decoded-image inspection; an ordinary object even though placed against a wall |
| Primary camera | Map 29 `(4,6)` West; query 2, order 149, anchor `(-5,2)`, scale 0, scene and bottom clipping |
| Primary cell | Raw wall word `0x8088`: N=8, E=0, S=8, W=8; attributes `0x00`, no automatic trigger |
| Text | `XEEN.CC/aaze0029.txt`, zero-based entry 6, including its original leading line feed (`0x0a`) |
| Behavior | Display the clue “Only at night can you put up a fight.” in the bottom window, then acknowledge |
| State effects | None: no world removal, item, flag, party, camera or time mutation |
| Prerequisites | No quest condition, active-character choice, door operation, combat, animation, or wall-item support |

Camera N/E/S/W resolves frame/flip to **1/false, 0/false, 1/true,
0/false**. These are static directional appearances, not a two-frame animation.
The chosen West view uses the first relative metadata entry. Static classification
is the existing `initial + 1 >= limit` rule, including a zero limit.
Both selected directory frames passed the existing checked drawer; all four
direction/flip combinations were exercised without changing resource bytes.

The complete original event at this cell is:

| Original EVT record | Byte offset | Direction | Line | Opcode and operands |
| ---: | ---: | --- | ---: | --- |
| 65 | 583 | All (4) | 0 | `0x29 DisplayBottom`, text index 6 |
| 66 | 590 | All (4) | 1 | `0x09 If2`, Action 44, value 1, target line 2 |

No line 2 exists at the cell. The existing adjacent-acknowledgment natural
completion rule handles this exactly; it must not become an invalid-jump error.
The probe executed the unmodified script in all four facings: bottom-window
presentation, acknowledgment, then successful completion with two instructions
and selected object 14. It also confirmed no event at `(5,6)` West and `(4,8)`
South. The original accepts every facing: West is the useful front-view acceptance
orientation, **not an invented script-facing restriction**.

### Serious alternatives and why they were not selected

| Candidate | Data/visual/trigger evidence | Boundary and decision |
| --- | --- | --- |
| Map 33 barrel `(2,8)`, object 22 | Ordinary resource 28, slot 5, direction N, static frame 0 in every direction with E/W flip. Cell `0x8008`, attributes `0x08`; All-facing records 203-206: WhoWill `(2,15)`, AfterEvent `(0,0)`, Display0x01 text 5 (empty barrel), If2 Action 44 to adjacent line. North would be query 2/order 149 with a back wall. | Close to the geometry control, but the full interaction includes unsupported `0x18 AfterEvent`, not merely supported WhoWill/text. Script mutation requires its own semantics and persistence assessment. Do not certify only its pre-mutation prefix. |
| Castle Basenji map 68 bed `(11,4)`, object 30 | Ordinary resource 19, slot 3, N; static frame 0 and E/W flip. Cell `0xc808`, attributes `0x0a`; All-facing records 56-59: WhoWill `(0,1)`, Display0x01 text 0 (nothing here), If2 `(44,1,3)`, `0x2e MakeNothingHere`. North primary is order 149 against wall type 12. The existing castle transfer actually ends at map 68 `(15,2)` West. | Requires a currently unsupported opcode. Reference MakeNothingHere disables physical-cell events and exits without removing the bed; substituting Remove would be wrong. Existing disabled-event ownership could support a later narrow extension, but it is unnecessary for M23. Proximity alone does not justify it. |
| Shangri-La map 49 `(6,5)`, object 17 | Ordinary resource 2, slot 4, N, static directional initials/limits `[0,1,2,1]`, flips `[0,0,0,1]`; open cell, attributes `0x0b`. Record 43 at offset 385 is All-facing NPC `(5,6,37,1,1)`, an informational mode-1 conversation. Primary N would use query 2/order 149. | Credible existing-NPC alternative with no evident new durable effect. Its open location is a weaker original wall test than the gravestone's alcove. No need to add NPC acceptance complexity or certify travel to this checkpoint. |

Map 33's other inspected beds, crates and chest expose GiveMulti, MoveObj,
AfterEvent or damage; its wall item at `(7,12)` uses resource 0 from the separate
wall-item table and PlayEventVoc/ConfirmWord/mirror teleport. It cannot validate
ordinary-object rendering. Vertigo map 28 inspection also exposed object moves,
services and rewards; it did not offer a better certified bounded target.

**Wall-item decision:** exclude wall items. `parseMob` already separates them,
including their compacted resource-table lookup. The reference draws `.pic`
resources with exact facing equality and separate placements/frames through
`setIndoorsWallPics`. A gravestone is not reclassified as a wall item because a
wall is behind it. Supporting wall items would add a distinct contract without
helping the chosen encounter; no wall-item sub-stage is justified.

## 4. Current architecture and the missing increment

Paths here are relative to the repository root. They name actual owners at the
verified baseline, not required new files or invented APIs.

| Concern | Current owner and implemented behavior | M23 implication |
| --- | --- | --- |
| DAT/MOB decoding | `src/formats/xeen/XeenMapFormat.cpp`; `XeenMap`, `XeenMapEntity`, `XeenObjectFile` | Indoor walls, object coordinates, original indices, direction, table slot and resource ID already exist. No new game-data format is needed. |
| Resource adapters | `XeenMapLoader::loadGeometryMap/loadObjects`; `XeenAssetSource` | Keep geometry and immutable MOB loading separate. Production must use `world.objectFile`, not legacy `map.entities` copies. |
| Effective world state | `XeenWorld`, `XeenSessionWorldState`, `XeenRecordIdentity` | Original side/map/record identity and disabled overlays already work independently of map/object cache lifetimes. |
| Indoor geometry | `XeenIndoorScene::sampleWalls/build`, `XeenIndoorSceneTables.h` | 44 directional queries, wall-bit translation, wall-resource/frame decisions and original numeric draw order exist. No indoor object commands currently exist. |
| Scene composition | `CloudsMapComposer::compose` | Indoor branch currently draws geometry via `drawSprite`; outdoor branch resolves objects and uses checked object drawing. Extend the indoor branch at this owner. |
| Visual metadata | `XeenCloudsVisualMetadata`, `XeenObjectVisualResolver` | Already supplies stable identity, resource name, directional frame, flip, static/animated/unsupported status and diagnostics. Reuse unchanged static semantics. |
| Rasterization | `XeenAssetSource::drawObjectVisual`, `ScummVmXeenBridge::drawObjectSprite`, `validateXeenObjectSprite` | Checked normal 320x200 drawing, directory/cell validation, cached resources, two cells, scaling and clipping are reusable. No second decoder. |
| Outdoor reference pattern | `XeenOutdoorScene`, typed `XeenOutdoorDrawCommand` | Reuse the value-command/checked-draw pattern and identity policy. Its coordinates, orders and terrain layering are not indoor rules. |
| Interaction | `XeenNavigationFlow::processInteraction`, `XeenEventSystem::runManualEvent`, `XeenEventScript::findInstructionIndex` | Space dispatches at the current physical cell/facing, without the automatic-event gate; indoor special walls retain their existing guard. Lookup preserves first-record precedence. |
| Selection / mutation | `XeenWorld::selectObject/applyRemove`, `XeenEventInterpreter` | Selection is original eligible-record order at the physical cell, independent of visibility. Remove disables selected object and physical-cell events using existing identities. |
| Continuation / presentation | `XeenEventFlow`, `XeenEventPresenter`, `XeenTextRenderer` | Flow owns pending execution and recomposition; Presenter owns layered text/pages and acknowledgment. No indoor event or UI duplicate. |
| Application / SDL | `Application::renderMap`, `XeenGameplayServices`, `playGameplay`, `SdlWindow` | Bind loaders/composer, construct gameplay owners, convert input and call Flow. SDL displays frames; it does not select objects or interpret scripts. |
| Save/resume | `XeenSaveState`, `XeenSaveSnapshot`, `XeenSaveFormat`, `XeenSaveFile`, `playGameplay` | Existing v2 writer/v1-v2 reader and restore-before-first-frame already include disabled object/event identities. Rendering values are reconstructed. |

Missing work is an indoor object command payload, indoor placements and visibility
predicates sharing geometry's sampled wall bits, and composition of both kinds in
one ordered stream. No metadata field, event opcode or persistent state category
is missing for this encounter.

## 5. Architectural decisions

1. Extend `XeenIndoorScene` and its existing command stream. Use a typed
   geometry/object payload (the outdoor variant is a useful pattern), carrying
   `XeenObjectVisual` for objects and retaining geometry resource/frame/options.
   Preserve command source map/cell, original order and object query index for
   semantic tests. Do not make an object look like unchecked generic geometry
   merely by putting its sprite filename in the current string field.
2. Share the existing 44 samples and `kMazeBitIndex` translation within the
   indoor build. Extract a small private helper if needed; do not create a second
   wall visibility map, raycaster, depth-buffer owner or general entity framework.
3. Preserve a geometry-only build path for existing callers, preferably an
   optional resolver/diagnostics extension like the outdoor builder. With no
   resolver, existing geometry commands remain exactly reproducible. Production
   supplies the existing Clouds resolver and draws supported object commands
   with `drawObjectVisual`.
4. M23 supports **static directional ordinary appearances only**. Resolve indoor
   objects with the ordinary phase omitted. Animated results remain explicit
   `UnsupportedAnimation`, not frozen at zero. The composer's supplied outdoor
   phase must not accidentally enable indoor animation; indoor compositions
   continue to report `containsOrdinaryAnimation=false`.
5. All commands and visibility information are derived values for a composition.
   No retained per-object position, frame, enabled flag, click target or clock
   becomes a gameplay owner. Cache reconstruction consults `XeenWorld` again.
6. Keep the accepted 16x16 indoor domain. Only current-map ordinary records whose
   source cell is within that domain are eligible for M23 drawing. Preserve raw
   out-of-domain records without wrapping, clamping, deleting or loading neighbors.
   This is a deliberate bounded-support decision: reference `Map::getCell` can
   follow neighbors; current `XeenWorld::sampleCell` does not do so indoors.
   Rendering objects beyond the supported geometry plane would imply unsupported
   occlusion. The selected tests need no neighbor semantics.

Likely production changes are confined to `XeenIndoorScene.h/.cpp`, its table
header, and `CloudsMapComposer.h/.cpp`. Tests and their CMake registration may
change during the later implementation task. Event, Flow, save and SDL changes
are not presumed necessary; a discovered concrete mismatch must be reviewed
before widening this boundary.

## 6. Projection and occlusion contract

### Position, anchor and scale

Use the existing `SCREEN_POSITIONING_X/Y` subset in
`XeenIndoorSceneTables.h`. For each row below and camera direction `d`, match an
eligible object's raw cell to:

```text
object.x = camera.x + kScreenPositioningX[d][query]
object.y = camera.y + kScreenPositioningY[d][query]
```

Y increases north. In the table, F is forward distance and L/R is lateral
distance relative to the camera. A same-cell object is F=0; it is not an object
in the next cell. The reference's names such as “Position 1” do not change that.

| Query | Relative cell | Draw order | Normal anchor X,Y | Resource 113 anchor X,Y | Scale index |
| ---: | --- | ---: | --- | --- | ---: |
| 2 | 0F | 149 | -5,2 | -35,-65 | 0 |
| 7 | 1F | 125 | -7,25 | -35,-6 | 7 |
| 5 | 1F1L | 126 | -112,25 | -142,-6 | 7 |
| 9 | 1F1R | 127 | 98,25 | 68,-6 | 7 |
| 14 | 2F | 97 | -8,50 | -35,36 | 12 |
| 12 | 2F1L | 98 | -65,50 | -95,36 | 12 |
| 16 | 2F1R | 99 | 49,50 | 19,36 | 12 |
| 27 | 3F | 55 | -9,58 | -35,54 | 14 |
| 25 | 3F1L | 56 | -34,58 | -62,54 | 14 |
| 29 | 3F1R | 57 | 16,58 | -14,54 | 14 |
| 23 | 3F2L | 58 | -58,58 | -98,54 | 14 |
| 31 | 3F2R | 59 | 40,58 | 16,54 | 14 |

The exceptional anchor row is keyed by resolved **Clouds resource 113**, not MOB
slot or record index. Reference Darkside resource 47 is outside this scope.
There is no fourth-forward-distance ordinary-object row. Match table values
directly; outdoor anchors differ, especially lateral positions at depth two/three.

Anchors are drawer inputs, not sprite top-left pixels. The reused rasterizer
applies each cell's internal X/Y offsets, horizontal centering adjustment under
scaling, compressed transparent spans and flip. Scale indices are the existing
16-bit sampling masks: 0=`0xffff`, 7=`0xaeaa`, 12=`0x8888`, 14=`0x8080`.
Do not replace this with continuous perspective or ordinary image resizing.
Reference window 3 has inner origin `(0,0)`, so no extra viewport translation
is added to these anchors.

Object direction chooses only the relative metadata appearance:
`r=(cameraDirection+4-object.direction)%4`. Nonzero metadata flip means horizontal
flip; camera rotation does not independently flip placement. Do not filter
ordinary objects by object-direction equality as the wall-item path does.

### Wall predicates before emission

Let `W[n]` be exactly the boolean produced by the current `kMazeBitIndex`
translation over the 44 sampled wall faces. Missing samples contribute no bits,
as in the existing geometry build. It is incorrect to treat every nonzero front
wall nibble as the same opaque blocker: different values set different bits.
For example query 2 with nibble 8 sets W27; nibble 1 sets W127.

The following expressions mean **blocked**. A row is admitted only when its
expression is false (and its source cell is inside the supported map domain).
`|` and `&` here mean boolean OR and AND, not indexes or wall masks.

| Query | Blocked expression |
| ---: | --- |
| 2 | false |
| 7 | W27 |
| 5 | (W27 & W25) \| (W27 & W28) \| (W23 & W25) \| (W23 & W28) |
| 9 | (W27 & W26) \| (W27 & W29) \| (W24 & W26) \| (W24 & W29) |
| 14 | W22 \| W27 |
| 12 | W27 \| (W22 & W23) \| (W22 & W20) \| (W23 & W17) \| (W20 & W17) |
| 16 | W27 \| (W22 & W24) \| (W22 & W21) \| (W24 & W19) \| (W21 & W19) |
| 27 | W27 \| W22 \| W15 |
| 25 | W27 \| W22 \| (W15 & W17) \| (W15 & W12) \| (W12 & W7) \| (W17 & W7) |
| 29 | W27 \| (W15 & W19) \| (W15 & W14) \| (W14 & W9) \| (W19 & W9) |
| 23 | W27 \| (W22 & W20) \| (W22 & W23) \| (W20 & W17) \| (W23 & W17) \| W12 \| W8 |
| 31 | W27 \| (W22 & W21) \| (W22 & W24) \| (W21 & W19) \| (W24 & W19) \| W14 \| W10 |

Preserve the pinned reference's query-29 asymmetry: it does not have the
standalone W22 test present at query 25. Do not “correct” it by assuming mirrored
logic. Geometry's existing predicates and known accepted corrections also remain
unchanged; this task is not a general reference-bug cleanup.

### Ordered drawing supplies the remaining occlusion

An admitted command is not a guarantee of surviving pixels. Merge it into the
geometry stream in ascending original order. At three forward cells, objects
55-59 follow depth-four walls 29-45 but precede nearer walls 71 onward. Objects
97-99 follow walls 87-91 and precede walls 107 onward. Objects 125-127 follow
walls 119-121 and precede walls 133 onward. The same-cell object 149 follows
near walls 143-147: the gravestone legitimately appears in front of its back wall.

Opaque spans of later wall sprites overwrite object pixels; transparent spans
preserve them. This supplies partial and any remaining complete coverage. Do not
draw all objects after all walls, use a center-cell line-of-sight shortcut, or
remove a whole object merely because some of its projected rectangle intersects
a wall. Background reconstruction precedes every composition; prior object pixels
must not survive a turn, removal or failed/retried rebuild as ghost images.

All object commands set scene clipping. The normal drawer clips to the half-open
rectangle `[8,223) x [8,141)`; query 2 also sets the existing bottom clipping at
140. Other rows do not set bottom clipping. No enlargement is used. Preserve
the composer's later border, gem/indicator, party and interface overlays.

### Identity, precedence and failure

At each admitted position choose the first eligible original ordinary record,
using the current active/resource eligibility and `world.isObjectDisabled` before
visual resolution. Skip base `-128` disabled records, unresolved resource entries,
out-of-domain records and session-disabled identities. Retain record order and
side/map identity; never deduplicate by sprite, coordinates or visible index.
Disabling the first overlapping record allows the next eligible record to win.

Once an eligible record owns a position, an invalid direction, unsupported
animation or unavailable metadata must not promote a later record. Retain its
identity-bearing resolver diagnostic and emit no drawable object. Missing MOB
means no objects. Malformed present MOB/metadata and selected missing/malformed
sprite resources retain existing checked failure behavior; do not substitute a
frame, resource or plausible-looking placeholder. Sprite safety must run for the
selected frame on cache hits as well as misses. Composition exceptions must not
publish a successful partially drawn frame or mutate gameplay owners.

Do not derive selected gameplay object from the command stream. A renderer can
omit an object while its original cell script remains eligible; that separation
is already an accepted ownership contract.

## 7. Interaction and presentation contract

The required path is:

1. Original DAT and MOB load through `XeenMapLoader` into `XeenWorld`'s immutable
   caches. Record 14 retains its identity; the disabled overlay determines its
   effective eligibility.
2. **M23 addition:** the indoor builder resolves the static visual, emits the
   correct admitted object command, and `CloudsMapComposer` draws it among walls
   through the existing checked sprite path.
3. Space becomes `InteractionAction`; existing Flow/navigation dispatch at the
   committed physical camera `(29,4,6,West)`. Wall 8 does not enter the unsupported
   manual special-wall branch. The automatic bit is not required.
4. Existing `XeenWorld::selectObject` selects record 14 at the current cell.
   `XeenEventScript` locates All-facing line 0, then the interpreter requests
   `BottomWindowMessage` with the original text. Presenter renders it with the
   original font and responds `Presented` when its pages have been presented.
5. Line 1 requests Action-44 acknowledgment. Flow retains one continuation and
   blocks gameplay until Space/Enter acknowledges. The adjacent target line 2 is
   absent, so execution completes naturally after two instructions.
6. The gravestone remains. The existing bottom-message layer can remain after
   acknowledgment until the next gameplay action clears/recomposes it; do not
   change general message lifetime to make this encounter resemble NPC dismissal.
   Repetition displays the same clue, with no accumulated state effect.

No new command, click-picking, nearest-visible-object interaction, selected-object
facing requirement, text literal, event interpreter branch or presentation mode
is needed. No new permission, prerequisite flag or “read” bit should be invented.
Escape retains the ordinary existing exit behavior at this encounter; it is not
the NPC-specific acknowledgment control.

The original target has no wrong-facing negative case. Acceptance instead checks
all four facings positively, adjacent cells negatively, the distinct clue at
the sibling gravestone `(4,10)`, and the existing first-match/side identity tests.
A controlled **object-only** disable must hide record 14 but must not disable
its independent original event: it then executes without a selected object.
A controlled existing `applyRemove` additionally disables the physical-cell
events. These are explicitly synthetic mutation controls, not this gravestone's
original behavior.

## 8. State ownership and persistence policy

**No new persistent category and no save-version change are required.** The
gravestone interaction changes no durable state. It must not write a read marker,
disable the gravestone, grant an item, advance time, or change a flag.

`XeenWorld` remains authoritative for disabled object/event identities; camera and
game flags retain the existing committed/working publication policy; party and
inventory remain owned by `XeenPartyState`. Commands, wall bits, resolved frames,
sprite caches, diagnostics, text pages and visible-slot choices are transient.

The writer continues to emit v2, with v1/v2 reading unchanged. Existing saves
that contain indoor disabled identities must reconstruct the new visuals from
those identities; cache recreation must not resurrect a removed object. There
is no new on-disk field or compatibility migration. Save preflight may now check
newly drawn indoor resources; a genuinely malformed selected sprite can therefore
fail visual preflight through the existing error path, not through a format bump
or silent fresh-session fallback.

A required automated no-mutation check compares encoded snapshots at the same
camera before and after the original interaction. A synthetic existing-removal
restore check proves suppression after owner/cache reconstruction. A separate
new collection/reward restart scenario is unnecessary. Optional native F9/resume
at the gravestone uses existing semantics: no retained message or automatic
script replay, visible gravestone, and a fresh Space can read it again.

## 9. Single-stage implementation and acceptance work

**Objective:** deliver static ordinary indoor object commands and composition,
then validate the complete selected original interaction through existing owners.

**Contract established:** the twelve placements, exact wall predicates, ordered
coverage, static appearance and identity/lifetime rules in sections 5-8. The
authoritative inputs are immutable original data and current world/camera state;
all rendering products are derived. The outdoor builder and runtime phase policy
retain their current behavior.

Suggested implementation order within this one stage:

1. Add typed indoor object commands and table-driven placement/predicates sharing
   the existing wall samples. Preserve geometry-only construction and tests.
2. Wire the production indoor composer to resolve/draw static object commands in
   the merged stream; retain explicit unsupported diagnostics and checked drawing.
3. Add semantic, raster, original-data and Flow/Application acceptance coverage;
   verify removal reconstruction and outdoor/indoor transitions.
4. Build, run the complete CTest suite and required original-data checks, obtain
   independent review and maintainer physical SDL acceptance, then follow the
   repository's documentation closure process. None of those future actions is
   authorized by this planning document alone.

The stage is done only when the full definition of done below is satisfied.
Do not create a later stage whose only responsibility is to make the already
implemented renderer reachable or repair its presentation/state ownership.

## 10. Automated test strategy

### Pure placement, identity and visibility

Extend `XeenIndoorSceneTests.cpp` or add a focused indoor-object test target with
synthetic resource readers and maps. Ordinary CTest must remain data-independent.

- Exercise all twelve rows in all four camera directions, exact query/source
  coordinates, order, anchor, scale and clipping flags. Cover the resource-113
  exceptional anchors and non-113 controls; MOB slot and record index must not
  select that row.
- Test each predicate's independent blocking terms and conjunctions. A single
  conjunct must not block a two-term condition. Exercise enough combinations to
  detect omissions and added conditions, including the query-29 W22 asymmetry.
  Derive W through synthetic raw wall samples for integration with geometry;
  a pure predicate helper may additionally exhaust its relevant boolean inputs.
- Cover front-wall nibbles 0, 1, 2, 5 and 8, plus side walls, so visibility cannot
  collapse to `wall != 0`. Check that an intervening solid wall suppresses the
  target command while a wall behind it does not.
- Same resource in separate records stays separate. Overlapping eligible records
  preserve first precedence; an unsupported first visual is not replaced by a
  later supported one. Base/session disable excludes it before resolution and
  permits the next eligible record. Unresolved entries, malformed direction,
  missing MOB/metadata, unsupported side and out-of-domain coordinates retain
  explicit bounded behavior.
- Invalid cameras and outdoor maps retain rejection. Indoor map-edge views load
  no neighbor; raw records outside 0..15 neither wrap into view nor get deleted.
- Repeated builds and separate/combined map/object cache discards yield equal
  semantic commands and preserve disabled identities. Rendering does not change
  gameplay selection, flags, party values or immutable records.
- Static results are invariant under several supplied composer outdoor phases;
  indoor animated entries remain unsupported and animation presence remains false.

### Composition and checked raster coverage

Use synthetic sprites with distinct object/wall colors and transparent spans for
strong semantic pixel assertions, rather than relying on whole-frame hashes.

- Assert object contribution at each depth and on both sides; order must place
  the object in front of its back wall and behind nearer walls. Compare against
  a deliberately wrong all-objects-last replay to make the ordering oracle
  discriminating.
- Test partial coverage with both surviving and overwritten target pixels,
  fully blocked command absence, and an admitted command fully covered during
  rasterization. Distinguish clipping from wall suppression.
- Exercise scene edges, negative anchors, query-2 bottom clipping, flips and
  two-cell sprites using existing checked drawing. Reuse the sprite safety
  fixtures for missing resources, malformed directories/streams and selected
  invalid frames, including warmed caches; do not write another decoder test suite.
- Geometry-only commands remain equal to the accepted baseline. Keep map 33
  `(4,8)` North's nine geometry commands and wall-query vector as a regression
  oracle; a production frame may acquire legitimate object pixels, so compare
  filtered geometry and target-attributed changes rather than an old full image.
- Recompose cleanly after turns, disabled objects, reconstruction and errors.
  Verify border/party/interface pixels outside the scene remain valid and no
  indoor ordinary timing or additional periodic composition starts.

### Original-data and interaction integration

Add a dedicated external-data indoor-object integration target, or a clearly
separated mode in the existing integration infrastructure. The likely new target
name `mmodern_indoor_object_smoke` is a proposal, not an existing executable.

It must load the unmodified encounter through production providers and assert
the map/record/metadata/event facts in section 3. Do not inject replacement EVT,
MOB, text or animation metadata to make the primary case pass. Use the actual
production indoor command stream for attribution.

Required original target views:

| Camera (map 29) | Target placement and oracle |
| --- | --- |
| `(4,6)` West | Query 2, order 149; frame 0, no flip; back wall at order 145 and side walls 144/146 precede the gravestone. Visible original lettering/object silhouette. |
| `(5,6)` West | Query 7, order 125, `(-7,25)`, scale 7; visible but Space does not dispatch the target from this adjacent cell. |
| `(6,6)` West | Query 14, order 97, `(-8,50)`, scale 12; visible. |
| `(7,6)` West | Query 27, order 55, `(-9,58)`, scale 14; visible. |
| `(6,7)` West | Query 12, order 98, `(-65,50)`, scale 12; admitted despite W22 because its conjunctive blockers are false. Later center wall order 120 supplies partial coverage. |
| `(4,8)` South | Target would be query 14 but query 2 has nibble 8, setting W27: no target command and no target contribution. Original manual lookup here has no event. |

Fixed reference-derived raster probes during planning established nonzero target
contribution at all four centered distances. At `(6,7)` West, the probe had
surviving target pixels and additional coverage compared with drawing that same
target last. Those measurements establish that this original fixture can expose
the ordering error; their exact pixel counts are not acceptance goldens.

For each view compare the complete production composition to a replay with only
the command for `{Clouds,29,14}` omitted. Hold every other command and all UI
layers constant. The partial case must additionally compare the actual interleaved
order with a test-only final draw of the same target. Assert nonzero survivor
and covered sets, and appropriate scene-local bounds. A changed whole frame,
another gravestone, or a wall alone does not prove the selected target is drawn.

The direct event test must assert records 65/66, text identity/bytes, presentation
kind, selected record, All-facing behavior, pending acknowledgment, two-instruction
completion and repeatability without mutation. Compare sibling `(4,10)`'s text 7
to ensure coordinate lookup does not always return text 6. Negative adjacent-cell
tests must not invoke this encounter. Test object-only disable independently from
event disable; do not add visibility as a dispatch requirement.

Flow/Application integration must use the production composer with original
providers. Assert the gravestone is visible before Space and underneath the
bottom message, gameplay input is blocked while acknowledgment is pending,
reconstruction preserves the pending request/generation/pages, acknowledgment
cannot replay the event, and the next gameplay action clears retained text and
recomposes the right scene. Ordinary idle must not advance an indoor object phase.

### Regression and persistence scope

Run the relevant indoor scene, object visual/sprite, world/session identity,
manual event, presentation/Flow, visual Remove and save tests during iteration.
Exercise a controlled existing Remove on a synthetic indoor record, including
pending presentation, shared-resource siblings and combined cache reconstruction.
Restore its existing saved disabled identities into new owners and verify that
the command stays absent. The real gravestone's script is not a Remove fixture.

At milestone closure, build and run the full CTest suite. Run the external
map-33 geometry and Castle Basenji transfer controls, plus static outdoor objects,
Myra ordinary animation/presentation, Phirna/WhoWill removal and save/resume
regressions affected by shared composition or command changes. Do not alter save
versions or update old whole-scene goldens merely to conceal a geometry regression.

## 11. Original-data and manual acceptance

Set `$buildDirectory` to the chosen MMModern build configured with the pinned
dependency, then use the existing public CLI in the standalone SDL application:

```powershell
& "$buildDirectory/mmodern.exe" --inspect-events 'F:/Games/gog/Might and Magic 4-5' 29 4 6 west
& "$buildDirectory/mmodern.exe" --render-map 'F:/Games/gog/Might and Magic 4-5' 29 4 6 west
```

The first command is already available for reproduction before implementation.
The second currently displays indoor geometry; the following visible-object
requirements apply to the later M23 implementation:

1. Observe the static RIP gravestone in front of the alcove's back wall, with
   coherent scene/border clipping. It must remain still without input.
2. Press Space. Observe the original clue in the bottom message window while
   the gravestone remains in the scene. Attempt movement while acknowledgment
   is pending: the camera must not move.
3. Press Enter or Space to acknowledge. No reward, disappearance, flag effect or
   teleport occurs. The bottom message may remain until the next gameplay action,
   as specified above. Read again and observe the same bounded response.
4. From `(4,6)` West, backward movement approaches the listed centered distance
   views `(5,6)`, `(6,6)`, `(7,6)`; observe coherent decreasing size. This is only
   a local observation, not route certification. Direct CLI starts at these
   positions are also valid acceptance controls.
5. Launch `(6,7)` West and `(4,8)` South separately using the same command format.
   Observe partial wall coverage in the former and absence behind the wall in
   the latter. Use the target-isolated automated evidence to distinguish the
   small distant object from unrelated pixels; manual inspection complements it.
6. Recheck the map-33 geometry and existing outdoor-to-Basenji transfer controls.
   No doors, combat or normal journey to Nightshadow must be completed for this
   acceptance.

Maintainer-performed physical SDL acceptance and independent implementation review
are required at closure and must be attributed separately from automated SDL,
probe-image and source-reference evidence. No commercial resource or generated
original-data image is a repository fixture.

## 12. Explicit non-goals and replanning triggers

Excluded: indoor animation (including simply forwarding the outdoor phase),
scripted appearance changes, wall items, monsters/combat, complete dungeon/town
content, general indoor traversal or connected-map rendering, lighting gameplay,
doors/locks/grates/traps, night/day effects, a general animation system/world clock,
inventory/equipment use, Darkside gameplay, new mutation opcodes and save formats.
Base static rendering of another indoor object does not certify its script.

Reopen the scope for review if:

- The primary record, resource, event sequence or original wall evidence does
  not reproduce against compatible unmodified data.
- Accepted geometry cannot provide the specified predicates/order without a
  material correction; report the exact geometry discrepancy rather than hiding
  it in an object-only visibility system.
- The implementation proposes object-coordinate mutation, wall-item identities,
  read flags, new opcodes, indoor clocks or connected-map geometry to complete
  this encounter. Those are not established prerequisites here.
- A proposed optimization changes original-record precedence, removes commands
  based on approximate sprite rectangles, or ties gameplay selection to rendering.
- Independent review finds a conflict in the reference-derived table or its
  deliberate 16x16 boundary. Resolve that contract before changing production.

There is no unresolved encounter prerequisite at the planning boundary. General
lighting, traversal and other scripts remain unverified/excluded, not hidden
acceptance assumptions. The evidence supports the roadmap's direction without
changing its scope or promoting another future milestone.

## 13. Definition of done

- The twelve static ordinary-object placements, exact wall predicates, checked
  raster behavior and authoritative identity exclusions are implemented and
  covered by discriminating semantic/pixel tests.
- Existing indoor geometry remains unchanged, outdoor composition/animation
  remains correct, and no parallel state, event, sprite or presentation system
  has been introduced.
- The unmodified original gravestone renders and its existing two-instruction
  interaction completes through production Application/Flow/Presenter/SDL with
  the specified acceptance observations.
- No new persistent state exists; current save compatibility and reconstruction
  of disabled identities pass, with no original-data or production-text copying.
- Build, full CTest, required original-data regressions, independent review and
  maintainer physical acceptance pass. Failures are resolved before completion.
- Only then perform the milestone documentation closure required by AGENTS.md,
  keeping stable status, history, roadmap and this plan in their natural roles.
  Commit/push/tag actions require their applicable separate authorization.
