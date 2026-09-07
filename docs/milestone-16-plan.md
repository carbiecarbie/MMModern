# Milestone 16 - Static outdoor map objects and visual Remove

**Status: approved; implementation has not started.**

**Next implementation target: 16A - Visual resolution and resource safety.**

Milestone 15 remains the completed stable milestone. This document approves the
scope and implementation order for Milestone 16; it does not record any M16 code
as implemented.

## Recommendation and objective

Milestone 16 implements the smallest coherent visible-object increment: static
appearance-base map objects in supported outdoor Clouds scenes, including the
visual result of the existing `Remove` mutation.

The approved scope is deliberately narrower than general entity rendering. It
does not implement every `.mob` category, indoor objects, temporal animation,
monsters, NPC gameplay, wall items, or an entity framework.

At milestone completion, supported static objects must use original visual
resources and appear at the correct camera-relative positions with the correct
directional frame, mirroring, scale, clipping, palette, and terrain ordering.
Rendering must consume the session-effective object state owned by `XeenWorld`.
The real Phirna object must be visible before `Remove`, disappear immediately
afterward, remain absent for the rest of that session across cache lifecycles,
and return in a genuinely new session.

## Current baseline

Milestone 15 established the prerequisites that M16 must preserve:

- `.mob` parsing retains object tables, original record order, coordinates,
  direction, table index, and resolved resource ID;
- `XeenMapLoader::loadObjects` loads per-map object data independently of
  geometry;
- object identity is side + map + original zero-based record index;
- `XeenWorld` owns session-effective disabled-object state independently of
  disposable map, object, script, and text caches;
- `XeenWorld::isObjectDisabled` combines original disabled state and the
  session overlay;
- interaction selection is already implemented independently of rendering;
- `Remove` can disable Clouds map 23 object 13 and events 125-135 without
  modifying the original commercial data;
- cache reconstruction preserves session mutation, while a new session restores
  the base state.

The existing rendering path already provides:

- a 320x200 indexed `IndexedFrame` with the Clouds palette;
- `XeenAssetSource` and the ScummVM compatibility bridge for archive access,
  sprite caching, decoding, scaling, horizontal flipping, and clipping;
- outdoor and indoor scene builders that emit value-based draw commands;
- stable ordering of outdoor terrain commands;
- `CloudsMapComposer` composition of scene, border, party, and interface;
- the M14 presentation layer, which draws event text over a composed frame.

M16 must reuse these facilities. It must not introduce another sprite decoder,
another authoritative object-state system, or a general ECS.

The approved planning baseline is the existing **35/35 passing** automated test
suite. This is not an M16 implementation result and remains unchanged by
approving this documentation.

## Evidence and provenance

### Pinned ScummVM reference

The authoritative reference revision is the commit recorded in
[dependencies.md](dependencies.md):

`6814ee9ba54582f5b5adcffab49efbbd8f589edd`

Relevant reference locations are relative to that checkout:

- `engines/mm/xeen/map.cpp`
  - `MonsterObjectData::synchronize`: resolves each object record's table index
    to an object sprite ID while retaining original record order;
  - `AnimationEntry::synchronize`: reads four initial frames, four horizontal
    flip flags, and four frame limits, for 12 bytes per visual entry;
  - map loading around the object-sprite loop: resolves sprite IDs below 100 as
    `%03d.obj` and IDs of 100 or greater as `%03d.0bj`;
  - map initialization selects the Clouds animation metadata appropriate to the
    installed game layout.
- `engines/mm/xeen/interface_scene.cpp`
  - `InterfaceScene::drawScene`: resolves direction-relative frame and flip and
    advances animated object frames;
  - `InterfaceScene::setOutdoorsObjects`: assigns outdoor objects to the 12
    supported view positions and to specific entries in the ordered draw list;
  - `InterfaceScene::setIndoorsObjects`: demonstrates that indoor object
    visibility has separate wall-occlusion rules and is outside this milestone;
  - `InterfaceScene::setIndoorsWallPics`: demonstrates that wall items use a
    separate direction-sensitive `.pic` path and are outside this milestone.
- `devtools/create_mm/create_xeen/constants.cpp`
  - `SCREEN_POSITIONING_X/Y`, `OUTDOOR_OBJECT_X`, `INDOOR_OBJECT_X`,
    `MAP_OBJECT_Y`, and `DIRECTION_ANIM_POSITIONS` provide the reviewed
    projection and direction tables.
- `engines/mm/shared/xeen/sprites.cpp`
  - `SpriteResource::load` and `SpriteResource::draw` provide the decoder and
    rasterizer already reused by MMModern, including two sprite cells per frame,
    compressed scan lines, internal offsets, scale, flip, and clipping.
- `engines/mm/xeen/scripts.cpp`
  - `Scripts::cmdRemove` moves the selected object outside the visible space and
    then removes the events at the physical cell. MMModern represents the same
    effective visibility through M15's disabled-identity state.

MMModern must reproduce the visible behavior and data relationships. It need
not copy ScummVM's mutable frame storage, pointer graph, draw-list classes, or
out-of-bounds mutation representation.

### Validated World of Xeen data layout

The supported M16 real-data validation target is the installed World of Xeen
layout used by the project. Targeted investigation established:

- Clouds object sprites are in `XEEN.CC`;
- Clouds visual metadata is in `DARK.CC` as `clouds.dat`;
- `clouds.dat` is 1,452 bytes and contains 121 entries of 12 bytes;
- the pinned reference uses equivalent Clouds animation metadata for frame and
  mirroring resolution.

Reading `DARK.CC/clouds.dat` as visual metadata for Clouds is a resource-origin
detail. It does not change the gameplay identity from Clouds to Darkside and
does not constitute Darkside gameplay support.

For M16, World of Xeen data containing this validated metadata source is the
supported real-data target. If a Clouds-only installation does not expose a
validated source for the metadata, existing engine capabilities must remain
functional and the new object-rendering capability must report its
unavailability explicitly. M16 must not extract or reconstruct the table from a
commercial Clouds executable, and it must not claim general standalone Clouds
object-rendering support unless implementation evidence later establishes it.

Original commercial data must remain external and read-only. No extracted
resource, sprite, screenshot, or framebuffer may be committed as a fixture.

## Approved visual subset

M16 supports appearance-base object records whose visual metadata requires no
temporal frame advancement for the selected direction. Static support is
determined from the visual metadata and scene behavior, not from sprite frame
count alone. A resource may contain multiple directional frames or multiple
sprite cells and still belong to the static subset.

Objects that require a temporal animation cycle are visually unsupported by
M16. They must not be frozen arbitrarily at frame 0. A later overlapping record
must not be promoted merely because the first applicable record is visually
unsupported; original record precedence must remain stable.

A decorative object that resembles a creature or person remains an
appearance-base object for this renderer. Its appearance does not imply monster,
NPC, combat, or interaction support. Base static appearance also does not imply
quest-dependent alternate visual forms.

## Visual resource resolution

For each supported object, the minimum resolver must produce values sufficient
for a draw command:

- stable `XeenObjectIdentity`;
- sprite resource name derived from the resolved resource ID;
- frame selected from the 12-byte entry and the relative direction;
- horizontal-flip flag selected from the same entry;
- an explicit supported-static or unsupported-animated result;
- a diagnostic result for unavailable or invalid metadata/resources.

The filename boundary is:

- resources 0-99: three decimal digits plus `.obj`;
- resources 100 and above: three decimal digits plus `.0bj`.

Direction-relative selection follows the four-by-four
`DIRECTION_ANIM_POSITIONS` relationship from the pinned reference. Resource
identity, frame number, coordinates, and visible ordering never replace the
stable object identity.

The existing `SpriteResource` stream decoder and rasterizer remain the drawing
implementation. M16 adds only the validation needed for supported inputs to
fail safely and diagnostically. This includes the resource index, frame bounds,
cell offsets, headers, and compressed data bounds needed by the M16 path. It is
not approval for a second decoder or a universal sprite-validation framework.

Transparency follows the sprite format's skipped regions and draw behavior.
It must not be reduced to an unverified rule that palette index zero is always
transparent. Frames with two cells draw both cells in their original order.

## Real-data checkpoints

### Primary Phirna checkpoint

Clouds map 23 contains the primary end-to-end acceptance object:

- object identity: Clouds, map 23, original record index 13;
- physical cell: `(8,2)`;
- table index: 8;
- resource ID: 111;
- original direction: North;
- resource name: `111.0bj`;
- resource size observed during planning: 748 bytes;
- one frame and one populated sprite cell;
- visual metadata initial frames: `0, 0, 0, 0`;
- visual metadata flip flags: false, true, false, true;
- visual metadata frame limits: `0, 0, 0, 0`.

The populated sprite-cell header was observed as x offset 0, width 250, y
offset 115, height 26. Consequently, a draw-list anchor is not the first opaque
pixel. At the current-cell anchor `(−5,2)`, the unscaled sprite cell begins at
Y=117 before clipping.

This object is static enough for the first production rendering checkpoint and
must disappear simply because rendering consults its effective disabled state.

### Air / Corner checkpoint

Clouds map 1 provides the alternate naming and composite-sprite checkpoint:

- original object index 4;
- cell `(1,14)`;
- resource ID 54;
- original direction West;
- resource `054.obj`;
- multiple directional frames;
- two populated sprite cells per frame.

This is also the location of an existing M14 sign-text validation, making it a
useful regression point for composition between scene objects and presentation.

### Directional control checkpoint

Clouds map 23 provides a direction-relative frame checkpoint:

- original object index 11;
- cell `(12,2)`;
- resource ID 117;
- original direction East;
- resource `117.0bj`;
- four frames and two sprite cells per frame;
- no temporal animation required for the intended validation.

For camera directions North, East, South, and West, the reviewed relative-frame
results are 1, 0, 3, and 2 respectively, without horizontal flipping.

These three objects are sufficient targeted real-data coverage. M16 must not
expand the investigation or tests into a catalog of all game objects.

## Outdoor positioning and draw-order contract

Outdoor object projection uses 12 visible positions from the current cell
through depth three. Camera-relative cell offsets rotate for North, East, South,
and West using the existing screen-positioning relationship.

| Depth | Lateral | Sample index | Draw order | Normal anchor X,Y | Scale |
|---:|---:|---:|---:|---|---:|
| 0 | 0 | 2 | 111 | `-5, 2` | 0 |
| 1 | -1 | 5 | 88 | `-112, 25` | 7 |
| 1 | 0 | 7 | 87 | `-7, 25` | 7 |
| 1 | +1 | 9 | 89 | `98, 25` | 7 |
| 2 | -1 | 12 | 67 | `-77, 50` | 12 |
| 2 | 0 | 14 | 66 | `-8, 50` | 12 |
| 2 | +1 | 16 | 68 | `61, 50` | 12 |
| 3 | -2 | 23 | 40 | `-74, 58` | 14 |
| 3 | -1 | 25 | 38 | `-43, 58` | 14 |
| 3 | 0 | 27 | 37 | `-9, 58` | 14 |
| 3 | +1 | 29 | 39 | `25, 58` | 14 |
| 3 | +2 | 31 | 41 | `56, 58` | 14 |

Negative lateral values are left of the camera. Every position is scene-clipped.
The current-cell position additionally uses lower clipping at the scene bottom.
Sprite-internal offsets and the existing scale masks determine final pixels;
the listed anchors must not be treated as opaque sprite bounds.

For Clouds resource 113, the approved reference-derived contract selects the
second row of `OUTDOOR_OBJECT_X` and `MAP_OBJECT_Y`. This small special case is
part of correct outdoor positioning. It does not approve other
resource-specific gameplay or appearance substitutions.

The draw-order values place objects among terrain entries. Objects must be
inserted into the existing ordered scene command stream and stable-sorted with
terrain. Drawing all objects as a final overlay is incorrect.

Only object records belonging to the current map are considered. Terrain
sampling may cross an outdoor map boundary, but that must not cause neighboring
maps' objects to be loaded or rendered. Original signed coordinates outside the
16x16 grid remain data and are compared normally; only records disabled in the
base data or by session state are excluded as disabled.

## Architecture decisions and invariants

The following decisions apply to every M16 stage:

1. Reuse existing sprite decoding, rasterization, indexed color, palette, scale,
   flip, and clipping behavior.
2. Add only the minimum visual-metadata representation and resolver.
3. Define the supported static subset from metadata and scene behavior.
4. Do not freeze unsupported temporal animation at an arbitrary frame.
5. Integrate object commands into existing outdoor scene ordering.
6. Query the M15 effective object state on every scene reconstruction.
7. Keep `XeenWorld` as the only owner of session object visibility.
8. Store stable identities and values rather than pointers into disposable
   caches when data must survive cache reconstruction.
9. Keep interaction selection separate from visual rendering.
10. Rendering never mutates object gameplay state.
11. Original resources and parsed base records remain immutable.
12. Cache reconstruction cannot reactivate a disabled object.
13. A genuinely new session restores base visibility.
14. Darkside identities remain distinct even when Clouds metadata physically
    resides in `DARK.CC`.
15. Recompose the scene when visible world state changes without camera motion.

For a given outdoor position, original object record order determines the first
applicable record. Visual support must not become a new selection authority or
alter the M15 interaction selection result.

## 16A - Visual resolution and resource safety

**Status: approved; not started.**

### Objective

Resolve a supported static object's visual description and draw its isolated
sprite correctly using the existing MMModern/ScummVM infrastructure.

### Required implementation

- provide explicit read-only access to the validated Clouds visual metadata;
- read `DARK.CC/clouds.dat` in the supported World of Xeen layout without
  changing the object's Clouds identity;
- parse its 12-byte entries with exact length and index validation;
- implement the `.obj`/`.0bj` filename convention;
- resolve frame and horizontal flip from object direction and camera direction;
- distinguish the supported static subset from temporal animation;
- validate the sprite structure only as far as required for safe, diagnostic
  M16 use;
- expose a small value-based visual description suitable for later scene
  commands;
- draw supported resources in isolation with the existing rasterizer.

### Existing abstractions to reuse

- `GameInstallation` and the compatibility runtime's registered archives;
- `XeenAssetSource` for read-only resource access and drawing;
- the existing ScummVM `CCArchive`, `SpriteResource`, `XSurface`, cache, and draw
  flags;
- `XeenSpriteDrawOptions` and `IndexedFrame`.

Do not implement scene projection, object visibility ownership, or another
sprite decoder in this stage.

### Tests

Use synthetic metadata and sprite fixtures to cover:

- exactly valid, empty, truncated, trailing, and malformed metadata;
- resource index bounds and direction bounds;
- naming on both sides of the 99/100 boundary, including 54, 111, and 117;
- valid and invalid frame bounds;
- direction-relative frames and horizontal flips;
- supported-static classification and explicit unsupported-animation results;
- one-cell and two-cell sprites;
- transparent skipped regions;
- scale, horizontal flip, scene clipping, and lower clipping;
- missing, empty, truncated, and malformed sprite resources with useful
  diagnostics.

The malformed-input work must remain targeted to the M16 load/draw path rather
than grow into a universal sprite-validation framework.

### Real-data checkpoints and manual validation

- resolve and draw `111.0bj` frame 0, with the Phirna flip behavior in all four
  directions;
- resolve representative frames of `054.obj` and confirm both cells contribute;
- resolve `117.0bj` as frames 1/0/3/2 for North/East/South/West;
- inspect the isolated real sprites in the existing Clouds palette.

Real-data validation outputs remain local and ignored.

### Completion criteria

- the value-level resolver and targeted safety rules are deterministic and
  covered synthetically;
- all three real checkpoints resolve and draw successfully from the supported
  installation;
- unavailable metadata leaves existing non-object-rendering capabilities
  usable and reports why M16 object drawing is unavailable;
- focused tests and the complete suite pass;
- no outdoor scene integration has been claimed.

### Risks

The reused decoder assumes structurally valid input more readily than an
untrusted-file parser should. The stage must establish safe bounds for its
inputs without reimplementing the codec. Installation variants may not expose
the validated metadata source.

## 16B - Static objects in the outdoor scene

**Status: approved; blocked on completion of 16A.**

### Objective

Render supported static map objects in the existing outdoor scene with correct
projection, direction, scale, clipping, terrain ordering, and effective state.

### Required implementation

- extend the outdoor scene command construction with the approved 12 positions;
- rotate camera-relative object-cell samples for all four camera directions;
- consume current-map object records in original order;
- carry stable object identity and resolved visual values in commands;
- omit base-disabled and session-effectively disabled objects;
- preserve the first applicable object at an occupied view position;
- keep an unsupported first record from promoting a later overlapping record;
- select the normal or resource-113 placement row as specified;
- set scale, mirroring, scene clipping, and current-cell lower clipping;
- insert object commands at their reviewed draw-order positions before the
  existing stable sort;
- draw through `CloudsMapComposer` and the existing asset pipeline.

### Existing abstractions to reuse

- `XeenWorld::objectFile` and `XeenWorld::isObjectDisabled`;
- `XeenObjectIdentity` and immutable `XeenMapEntity` values;
- `XeenOutdoorScene`, its rotation/sampling approach, draw-command model, and
  stable ordering;
- `CloudsMapComposer`, `XeenAssetSource`, and `XeenSpriteDrawOptions`.

Rendering must not call interaction selection as its visibility authority and
must not mutate selection or session state.

### Tests

Synthetic tests must cover:

- every supported depth/lateral position and all four camera directions;
- the approved anchors, order values, scales, and clipping flags;
- resource 113's alternate positioning row;
- direction-relative frame and flip values passed into commands;
- terrain/object ordering and deterministic final pixels;
- original record order and overlapping records;
- base-disabled, session-disabled, outside-view, outside-grid, missing-resource,
  and unsupported-animation records;
- two stable identities sharing the same sprite;
- Clouds/Darkside synthetic identity isolation where relevant;
- no neighboring-map object loads while terrain crosses a map boundary;
- cache reconstruction returning new base values while preserving effective
  session visibility.

Use command assertions and targeted framebuffer pixel checks or hashes. Hashes
must not be the only proof of layout or ordering.

### Real-data checkpoints and runtime validation

- render Phirna at multiple depths and at least one mirrored direction;
- render Air / Corner to exercise `.obj`, directional frames, and two cells;
- render object 11 on map 23 from all four camera directions;
- inspect terrain occlusion, transparent regions, scene clipping, border
  preservation, and the M14 Air / Corner presentation composition.

### Completion criteria

- the supported objects appear in the correct outdoor positions and command
  order;
- effective disabled state suppresses their contribution on the next scene
  reconstruction;
- current-map-only loading, record precedence, shared resources, and cache
  behavior pass automated tests;
- focused, full-suite, real-data, and applicable visual checks pass;
- no immediate runtime `Remove` result is claimed until 16C.

### Risks

The principal fidelity risk is placing objects as a final layer instead of
interleaving them with terrain. Large transparent sprite bounds and internal
offsets can also make apparently reasonable anchor adjustments incorrect.

## 16C - Visual Remove and runtime lifecycle

**Status: approved; blocked on completion of 16B.**

### Objective

Complete the player-visible vertical slice: execute the existing M15 `Remove`
and make the Phirna object disappear immediately and persistently according to
the existing session lifetime.

### Runtime recomposition requirement

The current runtime generally reconstructs the scene after navigation and after
event-driven camera changes. A world mutation such as `Remove` can change visible
state while `cameraChanged == false`. Requiring camera motion would leave the
old framebuffer on screen even though `XeenWorld` already reports the object as
disabled.

16C must use the smallest mechanism consistent with the current runtime to
recompose or invalidate the scene after relevant world-visible mutations. It
must specifically prevent:

- an old framebuffer continuing to show a removed object;
- presentation suspension or resume restoring an obsolete background;
- a mutation followed by a later execution failure rolling back camera/flags
  while incorrectly restoring world visuals.

The existing M14 presentation protocol and M15 policies remain authoritative:
camera and flags may follow their established transactional behavior, while
world mutations remain immediate session effects. A generalized invalidation
framework is not approved unless implementation evidence shows it is necessary.

### Required implementation

- recompose after a relevant effective-world change even without camera motion;
- cover event begin, automatic/manual progression, presentation suspension,
  presentation resume, completion, and error paths that can follow mutation;
- ensure presentation underlays reflect the effective world state;
- preserve M14 input blocking, responses, pagination, and scene-label behavior;
- extend the existing M15 real-data checkpoint to observe the framebuffer and
  full visual lifecycle.

### Existing abstractions to reuse

- M15 `Remove`, selection, disabled identities, and cache-discard behavior;
- `CloudsMapComposer` as the scene reconstruction boundary;
- `XeenEventSystem`, resumable execution state, and `XeenEventPresenter`;
- existing Remove/persistence smokes and SDL/manual-event validation paths.

### Tests

Cover synthetically and with the targeted real data:

- world mutation with an unchanged camera causes immediate recomposition;
- only the targeted stable object identity becomes visually absent;
- another visible object and two objects sharing a sprite remain independent;
- leave/return and actual map/object/resource cache reconstruction preserve
  absence while counters prove reconstruction occurred;
- presentation suspension and resume cannot restore an obsolete underlay;
- mutation followed by later event failure retains world mutation and correct
  visuals while camera/flags follow their existing rollback contract;
- a genuinely new world/event graph restores original visibility;
- M14 presentation and M15 Remove/persistence behavior remain unchanged.

### Real-data acceptance sequence

Using the established Clouds map 23 checkpoint and production selection:

1. Create a fresh session and confirm object 13 is visibly present.
2. Execute the original M15 `Remove` boundary for the selected object.
3. Confirm the Phirna sprite is absent immediately, without moving or turning.
4. Confirm unrelated supported visible objects are unaffected; supplement this
   with a deterministic synthetic multi-object scene where necessary.
5. Leave the map, return, and confirm absence.
6. Discard and reconstruct each applicable cache, with counters proving reload,
   and confirm absence.
7. Exercise presentation suspension/resume and the later-failure policy.
8. Create a genuinely new session and confirm Phirna is visible again.

The real sequence starts at the existing `Remove` boundary used by M15. It does
not bypass the unsupported preceding quest flow in ordinary gameplay and does
not implement `TakeOrGive`.

### Runtime/manual validation

- inspect before/after native 320x200 frames for Phirna;
- exercise immediate disappearance under SDL without intervening movement;
- repeat leave/return, cache reconstruction, and new-session cases;
- repeat the M14 Air / Corner, Snake Oil, and Castle Basenji No/Yes presentation
  checks and the M15 Remove lifecycle regressions.

### Completion criteria

- the eight-step acceptance sequence passes;
- automated tests prove recomposition and identity isolation;
- required real-data smokes and SDL checks pass;
- generated real-data frames receive manual visual inspection and remain
  uncommitted;
- the complete CTest suite passes;
- documentation accurately records implemented behavior and remaining limits.

### Risks

The largest integration risk is an old base frame retained by the presentation
controller or completion path. A test that manually calls the composer after
`Remove` does not prove the runtime invalidation path and is insufficient by
itself.

## Milestone-wide testing and validation

M16 implementation must include focused automated coverage for:

- visual-metadata parsing and the `.obj`/`.0bj` boundary;
- missing, empty, malformed, and out-of-range resources;
- safe frame and sprite-cell bounds;
- direction-relative frame selection and flip behavior;
- one-cell and multi-cell sprite drawing;
- transparent regions, scale, scene clipping, and lower clipping;
- all 12 outdoor positions and four directions;
- terrain/object draw ordering and original object record order;
- stable identity and distinct objects sharing one sprite;
- base and session-effective disabled state;
- no unintended neighboring-map object loads;
- cache reconstruction, session isolation, and new-session restoration;
- immediate post-`Remove` recomposition;
- presentation suspension/resume and later execution failure;
- M14 presentation regressions and M15 Remove/persistence regressions.

Synthetic fixtures should use deterministic command and pixel assertions.
Framebuffer hashes are useful for regression detection but cannot be the only
proof of layout, clipping, or ordering. Real-data frames supplement automated
checks through manual inspection; commercial resources and screenshots must
not be committed.

Milestone completion requires:

- a successful build with the documented pinned dependency;
- all focused M16 tests passing;
- the complete CTest suite passing;
- required production-data smokes passing;
- SDL/runtime validation passing;
- appropriate native-resolution visual inspection;
- M14 and M15 regressions passing;
- project status, this plan, dependencies if changed, and README updated to the
  actual implemented scope.

## Explicit exclusions

The following remain outside Milestone 16:

- `TakeOrGive` quest-item semantics;
- complete Phirna harvesting and quest-item ownership;
- inventory, equipment, and character UI;
- a general quest system;
- animated object cycles and a general animation framework;
- indoor object rendering;
- monsters, monster AI, and combat;
- NPC gameplay, shops, and services;
- wall-item rendering or gameplay;
- projectiles, effects, and particles;
- doors, locks, and traps;
- disk save/load and original-save compatibility;
- Darkside gameplay;
- World of Xeen cross-side gameplay transitions;
- generic ECS or entity-component architecture;
- arbitrary new world-state, mutation, history, or invalidation systems;
- quest-dependent alternate visual forms.

The provisional future quest-item work mentioned by M15 remains unapproved and
is not planned by this document.

## Expected repository impact

The exact edits remain an implementation decision, but expected areas are:

| Stage | Probable areas |
|---|---|
| 16A | `GameInstallation`/archive access as needed, `XeenAssetSource`, ScummVM bridge, a small Xeen visual-metadata format/resolver, focused tests and a real-resource smoke |
| 16B | `XeenOutdoorScene` tables/commands/build logic, `CloudsMapComposer`, scene/world tests, and real outdoor rendering validation |
| 16C | `Application` runtime composition flow, presentation/event integration tests, Remove persistence smoke, SDL validation, and documentation |
| All stages | CMake registration only for implementation tests or smokes added by that stage; no dependency-source copies |

No general engine reorganization, complete ScummVM integration, or M15 redesign
is expected.

## Complexity assessment

These are relative implementation-complexity estimates, not schedules or time
estimates:

| Work | Relative difficulty |
|---|---:|
| M15A comparative baseline | 6/10 |
| M15B comparative baseline | 7/10 |
| M15C comparative baseline | 4/10 |
| 16A | 6/10 |
| 16B | 6/10 |
| 16C | 5/10 |
| M16 overall | 7/10 |

Resource naming, metadata parsing, and table transcription are mostly
mechanical. The most difficult part is likely to be reliable pixel fidelity
across sprite structure, scale, clipping, and terrain ordering while making
malformed M16 inputs fail safely. Runtime recomposition is narrower but requires
care around presentation underlays and post-mutation failures.

## Open questions and approved constraints

The following decisions are approved by this plan:

- M16 is limited to static appearance-base outdoor Clouds objects;
- the installed World of Xeen layout is the supported real-data target;
- `DARK.CC/clouds.dat` is the validated Clouds metadata origin without implying
  Darkside gameplay;
- a Clouds-only installation without a validated metadata source retains
  existing capabilities and receives an explicit unavailability result for the
  new renderer;
- M16 does not extract metadata from a commercial executable.

Questions that may be resolved during implementation without changing scope:

- the exact class/file organization for the small metadata resolver;
- the narrow validation boundary around the reused sprite decoder;
- the smallest runtime recomposition signal or comparison needed by 16C;
- final deterministic pixel assertions and hashes after the renderer exists;
- exact diagnostic wording for unavailable metadata and unsupported animation.

No currently known question blocks starting 16A under these constraints.

## Definition of Milestone 16 complete

Milestone 16 is complete only when supported static appearance-base map objects
in supported outdoor Clouds scenes are rendered from original resources with
correct direction, scale, clipping, and ordering while consuming `XeenWorld`
effective object state.

The real Phirna acceptance must prove:

1. a fresh session renders Phirna;
2. the existing M15 `Remove` checkpoint executes;
3. Phirna disappears immediately without requiring movement;
4. unrelated supported objects remain unaffected;
5. leaving/returning and actual cache reconstruction preserve the visual
   absence;
6. a genuinely new session restores visibility.

The build, complete suite, focused M16 tests, real-data smokes, SDL/runtime
checks, visual inspection, M14 regressions, M15 regressions, and documentation
must all pass. Complete Phirna harvesting must not be claimed.

## First implementation task

The first task after explicit implementation begins is **16A - Visual resolution
and resource safety**: read the validated Clouds metadata, resolve Phirna's
resource name/frame/flip for all four directions, and establish synthetic tests
for the 12-byte metadata and filename rules. Complete all 16A safety and isolated
real-resource requirements before starting outdoor scene integration.

