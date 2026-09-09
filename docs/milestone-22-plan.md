# Milestone 22 - Bounded ordinary outdoor object animation

This specification separates pure explicit-phase rendering (22A) from live
runtime advancement (22B). Independent review and explicit authorization gate
each stage; the stable committed capabilities remain in [project status](project-status.md).

## Baseline and post-M21 horizon review

The planning baseline is `ff8e0d80f6413e118967ac040688c0f04f89bceb`,
`Reorganize project documentation workflow`. After fetching, local `main`, HEAD,
origin/main and direct remote main agreed and the working tree was clean.
The maintainer explicitly authorized this baseline after the initial gate found
it one commit beyond the originally expected
`bb524c0bb12f56e49950bd7ad689982844902a04` (M21 closure).

Current behavior is established by code/tests and [project status](project-status.md).
[M15](milestone-15-plan.md) owns removal identity, [M16](milestone-16-plan.md)
owns static outdoor placement/rebasing, [M20](milestone-20-plan.md) owns the
save boundary, and [M21](milestone-21-plan.md) owns the accepted item/reward
extension. Historical claims that M16 is the latest milestone do not override
the post-M21 implementation or stable status.

The broader post-M21 horizon review concludes that **M22 remains the smallest
useful next milestone**, for the following reasons:

- The renderer already knows outdoor object coordinates, stable identity,
  metadata, flip, sprite safety, terrain order and clipping. Its explicit
  animation rejection is a bounded missing capability, not missing world state.
- Ordinary animated records were verified in both map 1 and map 23. The Myra
  location supplies a visible stationary cycle and an existing supported NPC
  interaction, without a new route, item feature or scripted animation opcode.
- M21's reward continuation and M20's save ownership need preservation tests,
  not replacement. Visual phase fits outside their durable state categories.
- The existing idle callback and presenter rebase eliminate the need for a
  second loop. The early NPC-only return in `updatePresentation`, phase input
  plumbing and refresh-cause distinctions are work inside M22, not a prerequisite
  refactoring milestone. Re-reading a small metadata table and recomposing the
  native frame are bounded costs; introduce no cache/scheduler project without
  measured evidence of a problem.
- M23 needs indoor-specific object projection and wall occlusion, and still
  lacks a certified original encounter. Its static candidates do not technically
  depend on M22, but moving it ahead would open more unverified rendering/content
  boundaries than this already evidenced outdoor extension.
- General inventory use, combat, movement capabilities and playable-route
  certification remain broader independent concerns. None is required to observe
  or interact at the selected local checkpoint. No new persistent category or
  save compatibility debt blocks this work.

Retain **M22 -> M23** as the direction, with M23 still provisional. The timing
investigation corrects a premise of M22, not its ordering: the reference game
counter is 20 Hz, but ordinary idle scene redraws use two counter ticks.
Do not add speculative milestone entries. [Roadmap](roadmap.md) records the
horizon verdict; this plan owns its detailed evidence and scope.

## Pinned reference and exact semantics

The authoritative revision from [dependencies.md](dependencies.md) is
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The local external checkout was
verified at that SHA with empty status using command-local safe-directory and
autocrlf settings; no dependency or global Git configuration was changed.

Primary reference locations, all pinned to that revision:

- [interface_scene.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface_scene.cpp#L412):
  `drawScene`, ordinary increment/reset and selected-object special branch;
  `drawOutdoorsScene`/`drawIndoorsScene` clear the reset latch; `setOutdoorsObjects`
  consumes frames without independently advancing them.
- [interface.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp#L370):
  `perform`, navigation assignments, `doStepCode`, and `draw3d` at line 1332.
- [interface.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.h#L218):
  `draw3d(bool updateFlag, bool pauseFlag = true)`.
- [events.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/events.cpp#L68)
  and [events.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/events.h#L37):
  polling, frame counters, `ipause5`, `timeMark5` and elapsed-counter arithmetic.
- [map.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp#L379):
  `MonsterObjectData::synchronize`, `AnimationEntry::synchronize`, `Map::load`
  and `getNewMaze`; frame initialization and reconstruction.
- [saves.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/saves.cpp#L155):
  `loadGameState` clears/reloads the map.
- [scripts.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/scripts.cpp#L213):
  selected-object appearance replacement, `cmdTeleport` at line 479 and
  `_animCounter` setup in `cmdGiveMulti`.
- [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp#L415):
  the four-by-four `DIRECTION_ANIM_POSITIONS` table.

### Ordinary frame selection

Use the resolved sprite resource ID, not the MOB table slot, to index metadata.
For object direction `o` and camera direction `c` (N/E/S/W = 0/1/2/3), the
relative entry is `r = (c + 4 - o) % 4`. Each 12-byte entry contains four initial
frames, four flip flags, then four frame limits. Let `I`, `F` and `L` be those
values for `r`.

On a reset redraw, the reference assigns `frame = I` without incrementing. On
an ordinary subsequent `drawScene`, it increments the frame, then assigns `I`
if the incremented value is `>= L`. Only then does it draw. Thus **L is exclusive**,
not the last displayed frame or the sprite directory size. `F != 0` sets horizontal
flip for the whole direction-specific cycle, independently of its phase.

The deterministic MMModern contract after reconstruction/reset is:

```text
cycleLength = (I + 1 < L) ? (L - I) : 1
frame(I, L, phase) = I + (phase % cycleLength)
horizontalFlip = (F != 0)
```

Promote byte operands before arithmetic. This retains all previously supported
static cases, including `L=0`, `L=I`, `L=I+1`, and `I>L`; none becomes an invalid
range or cycles unrelated directory frames. Use an unsigned wide phase and
reduce modulo length before adding `I`. Invalid resource/direction/metadata and
sprite bounds retain diagnostic behavior. A sprite with eight directory frames
can have a three-frame cycle; never derive the cycle from directory count.

`drawScene` updates **all current-map object records**, including objects not
currently visible, with a shared reset condition and one increment per scene
draw. After initialization/reset, their ordinary sequences are functions of the
same step count with their own initial values and lengths. Independent mutable
per-object frame storage is unnecessary for this subset. Two records sharing a
resource remain distinct for selection/removal, but do not need separate clocks.
No phase is keyed by visible position or resource ID.

### Reference reset and reconstruction behavior

All `_isAnimReset` assignments in the pinned Xeen source were inspected:

| Trigger | Reference behavior |
| --- | --- |
| InterfaceScene construction | Latch starts false (`interface_scene.cpp:407`). This alone does not initialize a loaded object's displayed frame. |
| Object/MOB load | Each object's frame is set to 100 (`map.cpp:481`); frame is not serialized with MOB position/ID/direction. The next ordinary increment reaches 101 and wraps to the selected initial frame. The verified Clouds table's maximum limit is 23, so this initialization is effective for the supported data. |
| Left/right turn | Latch true (`interface.cpp:514,523`); next draw uses the new direction's initial frame. |
| Successful left/right strafe | Latch true (`476,504`). Strafing is reference context only; adding controls is outside M22. |
| Turn around | Latch true (`582`). No new MMModern turnaround control is required. |
| Forward/backward movement within a map | No reset assignment in either movement branch. The next scene draw continues the sequence. Blocked movement does not set the latch either. |
| Map crossing / cross-map teleport | `Map::load` reloads MOB records, giving the initialization above. `doStepCode` reaches `getNewMaze` on crossings; `cmdTeleport` loads only when the map changes. |
| Same-map teleport | No map load and no explicit reset assignment. With unchanged facing, the ordinary sequence continues. Supported MMModern teleports do not introduce mirror-facing semantics. |
| Ordinary scene redraw | Advances the ordinary sequence, not a reconstruction reset. Outdoor/indoor drawing clears the latch (`interface_scene.cpp:485,622`). |
| Save/load and restart | Saving MOB state omits visual frames; loading calls `map.clearMaze()` then `map.load()`. Fresh gameplay also loads the map. Loaded appearance is reconstructed, not resumed at a saved visual phase. |

MMModern must distinguish cache discard from an actual map entry. Its caches
are disposable acceleration, not ScummVM map-object lifetime. Rebuilding caches
in the same live scene preserves the explicit phase. Re-entering a map, including
a previously cached map, starts at phase zero. A new Flow after startup/resume
also starts at zero. An explicit redraw or `refresh(true)` is not a reset command.

### Timing: 20 Hz counter, approximately 10 Hz ordinary idle animation

`GAME_FRAME_TIME=50` ms and `SCREEN_UPDATE_TIME=10` ms. `pollEvents` uses a
single `if`, sets the prior frame time to the current host time and calls
`nextFrame()` once. A delayed poll does not replay the missed ticks.

However, **50 ms is not the ordinary stationary object-animation interval**:

1. `Interface::perform` marks the game counter and calls `draw3d(true)`.
2. The declaration defaults `pauseFlag` to true.
3. `draw3d` marks counter 5, calls `drawScene` once, and finally calls `ipause5(2)`.
4. `ipause5` polls until two game-counter ticks have elapsed. `perform` also polls
   for input; its one-tick minimum is already satisfied by that wait.

Consequently the ordinary idle loop advances an object once per approximately
100 ms scene redraw, or 10 Hz. Tick alignment, 10 ms polling, rendering time and
host stalls prevent an exact universal wall-time guarantee. Faster screen refresh
does not advance objects. Other callers can suppress the pause, and an enabled
`SCENE_WINDOW` makes `draw3d` return before drawing; this is not a universal
assertion that every game-counter tick runs scene animation.

**M22 timing contract:** a nonblocking **100 ms** ordinary visual deadline,
separate from the existing **150 ms** portrait deadline. First frame after reset
is `I`, first timed advance is due at `resetTime+100`. Before the deadline, no
timed advance. At/after it, advance once and set the next deadline to `now+100`.
Do not compute phase from wall-clock elapsed time, loop over missed deadlines,
or multiply steps after a stall. The reference does not perform missed object
draws after a delay; MMModern need not reproduce the exact alignment of two
underlying counters or block input for the reference's waiting period.

### Special scripted sequences remain excluded

After increment, the reference takes a different branch only for the selected
record when `_animCounter>0` and the sprite is Clouds 16 (Darkside 15), 58 or 73.
It forces frame 1 for sprite 58, or wraps values above 4 to 1 for the other two.
Script completion can then replace resource identity (for example chest 16 to
62, or 73 to 119). Those effects are not the metadata cycle described above.

Resource 9 does not satisfy that branch, even when selected at Myra. M22 supplies
ordinary appearance only; it adds no `_animCounter`, chest state, resource swap,
door sequence, quest appearance, monster, combat, spell effect, terrain animation,
calendar or general gameplay clock. Ordinary base rendering of an object must
not imply support for its otherwise unsupported scripted interaction.

## Certified original-data checkpoint

Read-only investigation used the external installation
`F:\Games\gog\Might and Magic 4-5`. Metadata comes from `DARK.CC/clouds.dat`
(1,452 bytes, 121 entries), MOB from the existing initial Clouds archive path,
and sprites from `XEEN.CC`. This is the validated World of Xeen layout, not a
new standalone-Clouds metadata adapter or Darkside gameplay claim.

**Primary checkpoint: the tent at Myra's existing location.**

| Field | Verified value |
| --- | --- |
| Map / MOB | Clouds 23 / `maze0023.mob` |
| Original zero-based record / effective identity | 1 / `{Clouds,23,1}` |
| Original cell / base state | `(9,11)` / active; outdoor cell raw attributes 0, so no automatic-event trigger |
| MOB sprite-table index / resolved resource | 1 / 9 |
| Sprite | `009.obj`, 12,450 bytes, eight directory frames (0..7) |
| Object direction | West (3) |
| Metadata initial frames, relative order 0..3 | `[0,4,4,4]` |
| Metadata exclusive limits | `[3,7,7,7]` |
| Metadata flip flags | `[0,0,1,0]` |
| Primary party position/facing | Map 23 `(9,11)` West |
| Primary relative entry / cycle | 0 / `0,1,2,0,...`, three steps (nominal 300 ms) |
| Existing placement | Sample 2, order 111, anchor `(-5,2)`, scale index 0, scene and bottom clipping |

Camera N/E/S/W selects relative entry 1/2/3/0 respectively: N is frames 4..6,
unflipped; E is 4..6 flipped; S is 4..6 unflipped; W is 0..2 unflipped. Frames
3 and 7 exist but are not included in these ordinary cycles. Every selected
cycle frame in all four directions passed existing sprite validation.

At the primary camera, the current resolver **and the production scene diagnostic**
return `UnsupportedAnimation` for this exact identity. A temporary investigation
probe supplied a copied, in-memory metadata entry with each candidate frame
frozen as static, enabling the existing placement/rasterizer to demonstrate its
pixels without changing production code or original resources. This is planning
evidence of representability and visibility, not evidence that runtime animation
has been implemented.

The full scene has 11,188 / 11,210 / 11,188 pixels attributable to that addition
for phases 0/1/2 in the probe. Transitions 0->1 and 1->2 change 334 pixels;
2->0 changes zero. Native 320x200 inspection shows the tent's small flag moving.
Frames 0 and 2 look identical at this view: the cyclic appearance is A/B/A,
with a three-step period, not three distinct images. Tests must assert frame
selection and nontrivial motion over a complete cycle, not inequality on every
adjacent transition. All transition differences lie inside the existing scene.

Reproduction after implementation uses the existing public entry point:

```text
mmodern --render-map <game-directory> 23 9 11 west
```

Wait without input to see the flag, then use Space for the existing original
Myra interaction. The verified cell does not automatically dispatch it on entry.
No new cheat, public animation switch, quest shortcut or normal-travel claim is
required. The existing no-Root request is sufficient for presentation acceptance;
full exchange semantics remain M21 regression coverage.

Additional metadata evidence within the allowed maps: map 1 record 0 at `(8,8)`,
West, resource 85 (`085.obj`) has relative initials `[0,12,8,4]`, limits
`[3,15,11,7]`, no flip and three-step cycles. Map 23 record 8, `(8,10)`, North,
resource 105 has a North cycle 0..6 (limit 7), while E/S/W are static frame 8
(limit 8; South flipped). These are metadata/frame-safety controls, not additional
certified interactions or travel routes. They are unnecessary for expanding M22
scope; the Myra checkpoint already satisfies it.

Preserve static controls from M16: Phirna `{Clouds,23,13}`, `111.0bj`; Air / Corner
`{Clouds,1,4}`, `054.obj`; and `{Clouds,23,11}`, `117.0bj`. The latter's camera
N/E/S/W frames remain `[1,0,3,2]`, unflipped. Static control *objects* remain
identical across phases; an entire scene containing a newly supported animated
neighbor can legitimately differ from an old baseline image.

## Smallest architecture and observable runtime policy

### Current ownership and call flow

`Application::renderMap` binds assets, loaders and `CloudsMapComposer` into
`XeenGameplayServices`. `playGameplay` creates world/party/camera/flags, performs
save restoration/preflight where applicable, then constructs `XeenEventFlow`.
The Flow's constructor composes; fresh sessions call `initial()` and resumes do
not replay it. SDL `showInteractive` calls the existing idle function, which
already calls `flow.updatePresentation()`.

Today Flow's `Compose` callback has no arguments. `refresh` compares the committed
camera and disabled-object count and supports forced reconstruction. Composition
loads the metadata resolver, builds an effective outdoor command stream, draws
it, then draws border/UI. Scene building consults `world.isObjectDisabled` before
resolution and preserves the first applicable original record at each placement.
`drawObjectVisual` currently accepts only `SupportedStatic`; the bridge validates
the selected frame even on cached sprites.

`updatePresentation()` currently exits unless an NPC is pending. Presenter
`rebase()` rebuilds semantic layers/pages against the supplied base; its NPC path
uses the existing displayed portrait frame, page and timing without starting a
new page. It does not respond or change Flow's execution generation. These are
the existing boundaries to extend.

### State and plumbing

Flow is the smallest owner for **one transient outdoor phase and its deadline**:
it already coordinates camera changes, base recomposition and presentation, and
exists for one gameplay session. Use a small private state/helper beside
`XeenEventFlow`; no independently registered system or per-map/per-object store.
Phase advances for the current outdoor scene even when an object is offscreen.
Static-only views must not require repeated full compositions merely to count
phase; entering a visible placement later uses the continuing shared phase.

Recommended interface shape (names may follow local style without changing the
contract):

- Resolver, outdoor builder and composer accept an explicit optional ordinary
  phase. Omission keeps the existing static-only mode; supplying zero means an
  explicitly reconstructed ordinary scene, not absence of animation support.
- Resolver exposes `SupportedAnimated` as well as `SupportedStatic`, plus the
  selected frame and flip. Cycle length is a local derived value, not a public
  visual field. `UnsupportedAnimation` remains meaningful
  for callers that have supplied no ordinary phase. Neither resolver nor builder
  advances anything. Repeated inputs produce identical outputs.
- Preserve `CloudsMapComposer::compose`'s frame return and existing diagnostics;
  add the phase input and an optional output reporting whether the emitted command
  stream contains an ordinary animated object. Derive it from actual effective
  commands, never an additional world scan or speculative global metadata flag.
  Fully terrain-occluded commands may conservatively count as animated.
- Flow's composition callback takes the explicit phase and returns a small
  composition value containing the frame and that animation-presence boolean.
  `XeenGameplayServices` passes the phase through to the production binding.
  Keep the boolean as derived recomposition information, not visibility authority.
- Use a monotonic millisecond clock function, defaulting to `steady_clock`,
  injectable in Flow/Application test services. The existing presenter `Clock`
  callable shape can be shared. Copy the injected source to both consumers;
  do not move it into the presenter before Flow retains it. Portrait state,
  randomness and 150 ms deadlines stay in the presenter.

In 22B, preflight composes with an independent explicit zero phase and ignores the
animation-presence result. It must not consume or reset the live Flow clock/phase.
M22 changes no fields in `XeenWorld`, party, save snapshot or save codec, and does
not increment a save version. Cache counters never determine animation resets.

### Explicit reset, advance and refresh rules

The following is the bounded MMModern event-loop adaptation of the reference.
It makes logical presentation updates explicit rather than copying incidental
ScummVM draw calls or blocking its SDL handler:

| Operation | Phase/deadline policy |
| --- | --- |
| First Flow frame / fresh startup / restored startup | Phase 0; deadline `now+100`; compose current effective scene before display. |
| Committed map or facing changes | Reset to 0 and rearm `now+100` before composing the destination. Reset wins over an advance in the same operation. Include return to a cached map and transition through an indoor map. |
| Ordinary unblocked gameplay navigation or interaction action | Request one ordinary scene step for the action's redraw and rearm `now+100`, unless the committed map/facing change resets it. Same-map forward/backward and blocked movement continue with the next phase; they do not reset. |
| Pending-presentation response input | No extra ordinary step merely for pagination/acknowledgment. Any committed destination change still applies the reset rule; idle drives ordinary animation during the presentation. |
| Same-map event relocation with unchanged facing | Preserve the current phase; recomposition itself adds no step. An enclosing ordinary gameplay action can already have requested its single step. |
| Due idle update, including active text/NPC | One phase advance and rearm `now+100`; recompose/rebase if effective commands include animation. Never process a backlog. |
| Remove / disabled-count change / `refresh(true)` / cache discard | Recompose at the current phase, without adding a step or resetting its deadline. Always consult effective identity state. |
| Repeated compose/resolve/rebase, redraw/expose, F9 or I | No phase advance or reset. Saving may take time; the next due idle still advances only once. |
| Indoor scene | No outdoor timed redraws. Next outdoor entry reconstructs at zero; indoor animation remains out of scope. |

An action is an explicit scene-step cause, not permission for every internal
`drive`/`refresh`/report/presenter call to advance again. In particular, a blocked
navigation action clears a retained label and requests only one step despite
forced recomposition. Calls rejected because Flow is dispatching or presentation
is blocking gameplay create no action step. Synthetic direct-result acceptance
and forced reconstruction preserve phase unless they publish a changed map/facing.

After an action/reset, the immediately following SDL idle callback must not double
advance at the old deadline. Non-navigation input does not clear pending layers.
The reference does not promise a fixed phase offset between input-triggered and
idle redraws; this policy preserves its continuation versus reset distinction
while making MMModern's logical update count deterministic.

### Animated base and presentation coexistence

Extend the existing `updatePresentation` path; do not add an SDL timer thread,
nested loop, world tick, interpreter resume or action injection. Preserve the
dispatch/reentrancy guard. Determine effective-world/camera changes and any due
ordinary step before composing. A reset supersedes that step; coalesce the causes
into one base composition at the final phase, then call `presenter.rebase(base)`.
Update NPC timing independently and return the latest combined frame once if
either path changed it.

Base refresh must not clear layers just because the phase changed. Preserve
text/page selection, retained labels, pending request and generation, blocking,
NPC speech/rest counters, displayed/next portrait frames, random source and
deadline. At coincident 100/150 ms deadlines both systems may update once, but
neither resets the other. When the NPC panel hides the tent, frame selection
must still advance; dismissing the panel reveals the current base, not its
pre-dialog snapshot. Time continues over retained text after execution completion.

Retain existing failure policy: a composition/rebase/NPC failure with pending
execution goes through Flow's cleanup/accounting path; manual failure remains
recoverable and automatic failure keeps existing propagation. Without pending
execution, propagate composition errors through Application/SDL, rather than
calling a helper that dereferences `_pending`. Never silently freeze a corrupt
animated resource. Tests must cover a later selected frame that is malformed or
outside the sprite directory; existing bridge checks remain active on cache hits.

## Stage 22A - Deterministic ordinary visual semantics

**Objective:** represent and render an explicitly supplied ordinary phase through
the existing outdoor command pipeline, with no runtime advancement.

**Production areas:** `XeenObjectVisual.{h,cpp}`, `XeenOutdoorScene.{h,cpp}`,
`CloudsMapComposer.{h,cpp}`, and `formats/xeen/XeenAssetSource.cpp`'s supported
status gate. Preserve `XeenCloudsVisualMetadata` layout and the bridge/sprite
decoder; touch their code only for a demonstrated safety gap. Extend existing
tests/smokes without new CMake registrations. No Flow/SDL clock is enabled in this stage.

**Contract and scope:** implement the formula, explicit optional phase,
supported-animated classification and effective-command animation information.
Keep omitted-phase production callers rejecting animation until 22B, so this
stage does not ship animated objects as arbitrarily frozen substitutes. Preserve
identity, first-record precedence, the 12 outdoor placements, terrain ordering,
scale, clipping, flip, diagnostics and static behavior. No scripted state or
animation timer is introduced.

The exact by-value interfaces retain existing argument order/defaults:

```cpp
XeenObjectVisual resolve(const XeenObjectFile &objects, std::size_t recordIndex,
    XeenDirection cameraDirection,
    std::optional<std::uint64_t> ordinaryPhase = std::nullopt) const;
std::vector<XeenOutdoorDrawCommand> build(XeenWorld &world,
    const XeenCamera &camera = kAreaA1Camera,
    const XeenObjectVisualResolver *resolver = nullptr,
    std::vector<XeenObjectVisual> *diagnostics = nullptr,
    std::optional<std::uint64_t> ordinaryPhase = std::nullopt) const;
IndexedFrame compose(XeenAssetSource &assets, XeenWorld &world,
    const XeenPartyState &partyState, const XeenCamera &camera,
    const XeenCharacterRulesContext &context,
    std::vector<XeenObjectVisual> *objectDiagnostics = nullptr,
    std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
    bool *containsOrdinaryAnimation = nullptr) const;
```

Promote initial and limit to `std::uint64_t` before computing the ordinary cycle.
A one-frame cycle always returns `SupportedStatic` at the initial frame. A longer
cycle without phase retains `UnsupportedAnimation`, its initial frame and diagnostic;
with phase, including zero, it returns `SupportedAnimated` at
`initial + (phase % cycleLength)`. This status establishes metadata resolution,
not sprite availability or safety. The existing checked draw gate accepts either
supported status and checks the complete directory plus the selected frame's cell
streams, including cache hits.
Invalid metadata, side and identity cases remain unsupported.

Phase passes unchanged from compose through build to resolve, with no retained
state. Production gameplay and preflight still omit it in 22A. The asset draw
signature and `drawOutdoorCommands` signature remain unchanged.

Composition resets a supplied presence output to false at entry, derives a local
value from actual emitted `SupportedAnimated` object commands, and publishes it
only after the complete composition and snapshot succeed. Exceptions leave false.
Omitted-phase, indoor, static-only and missing-metadata compositions report false;
removed, offscreen and precedence-suppressed records do not count. Commands later
covered by terrain still count. Clean background reconstruction precedes each draw.

Original-data acceptance requires native full compositions for phases 0/1/2/3
and an omitted-phase control. Attribute complete-cycle visible change to Myra
through the production command trace; a test-only trace may hold other commands
fixed and substitute Myra's actual resolved command. Require target-isolated wrap
and repeated same-global-phase equality after cache reconstruction, not arbitrary
adjacent-frame inequality or whole-scene wrap when other cycle lengths differ.
The historical 334/334/0 pixel counts are observations, not mandatory goldens.
Controlled session removal is independent of Myra's original quest script.

**Tests and real data:** extend `XeenObjectVisualTests.cpp`,
`XeenOutdoorObjectTests.cpp`, `XeenOutdoorComposerTests.cpp` and
`XeenObjectSpriteTests.cpp`. Extend `ObjectVisualIntegrationTest.cpp` and
`OutdoorObjectIntegrationTest.cpp` for explicit-phase Myra checks using the actual
unaltered metadata; the planning probe's frozen-metadata technique is not the
implementation acceptance path.

Required assertions:

- All initial/limit byte pairs retain the static classification rule; static
  pixels/frame/flip remain invariant under several phases, including large ones.
- Animated nonzero initials, exclusive limits, all 16 object/camera direction
  pairs, nonzero flip bytes, cycle wrap and large phase modulo work without byte
  overflow. Omitted phase still reports `UnsupportedAnimation` for animated data.
- Repeated calls and reordered pure resolutions cannot advance phase or mutate
  source metadata, object records, world state or draw ordering.
- Multiple cycle lengths use the same supplied phase; equal resources retain
  distinct identities. Offscreen-to-visible placement uses the supplied phase.
- An animated first record now draws; a genuinely invalid first record still
  blocks promotion of a later overlapping record. Disabled/base-disabled objects
  stay absent at every phase, after map-cache reconstruction and with shared IDs.
- Cached and uncached animated frames use existing safety validation, including
  malformed later frames, out-of-range limits, missing resources, unsupported
  side, unavailable metadata, two-cell drawing and flip/clipping behavior.
- Real `{Clouds,23,1}` matches the exact direction/cycle table; production explicit
  phase renders 0/1/2/0 with nontrivial cycle motion. Static M16 controls remain
  valid; compare their commands/pixels without demanding unchanged old full-scene
  hashes where newly supported neighbors are intentionally present.

**Validation:** build with the pinned dependency; run CTest selections
`xeen_object_visual`, `xeen_object_sprite`, `xeen_outdoor_objects`,
`xeen_outdoor_composer`, `xeen_outdoor_scene`, `xeen_visual_remove`, and relevant
world/session tests. Build/run `mmodern_object_visual_smoke` and
`mmodern_outdoor_object_smoke` with the external installation and ignored output
directory. Inspect the native cycle frames and static controls. Run the complete
CTest suite when practical; no physical idle acceptance can be claimed yet.

**Exit/gate to 22B:** reviewed explicit-phase semantics, real-data frame selection
and visibility, passing relevant tests and no runtime frozen substitution.
Resolve any data/safety discrepancy before runtime work. Independent review and
explicit authorization are required before starting 22B; accepting 22A does not
implicitly authorize it.

## Stage 22B - Live scene animation with preserved presentation

**Objective:** deliver stationary ordinary animation through the existing
Application/Flow/SDL path and prove its transient-state boundary.

**Production areas:** `app/XeenEventFlow.{h,cpp}`, a small adjacent transient
state/helper if useful, `app/XeenGameplayServices.h`, `app/XeenGameplay.cpp`,
`app/Application.cpp`, and the 22A composition boundary. `XeenEventPresenter`
changes are allowed only for a demonstrated rebase/integration defect. The SDL
loop already has the required idle hook and should need no scheduling redesign.
Do not change save wire/state, world mutations, navigation rules or event opcodes.

**Contract and scope:** enable explicit phase in production and preflight, enforce
the reset/step/deadline tables, independent portrait cadence, minimal recomposition,
effective removal and preserved semantic presentation. Integration, save-boundary
tests and physical visual acceptance belong here because they validate this same
runtime capability; a separate stage solely for tests or a restart ceremony would
not create an independent architectural boundary.

**Tests expected to change/add:** add `XeenOutdoorAnimationTests.cpp` (registered
as `xeen_outdoor_animation`) for fake-clock/controller/Flow behavior; extend
`XeenNpcTests.cpp`, `XeenVisualRemoveTests.cpp`, `XeenSaveFlowTests.cpp`,
`XeenSaveStateTests.cpp` and their existing fixture helpers. Add a focused SDL mode
registered as `xeen_outdoor_animation_sdl`, using the existing window/idle callback.
Extend `MyraIntegrationTest.cpp` for original tent plus NPC timing and presentation.
Add a focused production-service save/reconstruction case to the existing save
test support/coordinator only where required by the boundary below.

Callback-signature adaptations also affect existing quest-grant/flag, WhoWill,
reward, Phirna, Remove and save/resume fixtures. Keep their assertions and
continuation owners; do not replace them with a separate animation test loop.

Required deterministic and integration assertions:

- First frame phase zero; 99 ms causes no timed advance, 100 ms causes one,
  repeated calls at the same time cause none, and a large stall causes one
  advance with a new `now+100` deadline. Explicit rendering alone changes nothing.
- Same-map forward/backward and blocked navigation continue; turns and map entry
  reset; pending navigation is blocked; a following idle callback does not replay
  an action step. Test same-map relocation and committed versus working camera
  during suspended teleport/error as well as return to a cached map.
- Idle advances without key input or pending NPC. Static-only/indoor views avoid
  periodic composition; a newly visible object uses the continuing phase.
- NPC alone advances at its existing 150 ms boundaries; ordinary animation uses
  100 ms. Test 99/100/149/150/200/300 ms and a stall with both active, checking
  independent deadlines, random draws and no duplicate composition/publication.
- `rebase` retains a noninitial page, generation, response requirement, retained
  label, blocking and NPC timing. Dismissal exposes the latest animated base;
  an entirely covered scene is tested by state/frame checks after dismissal.
- Remove while idle or pending immediately excludes the selected identity at
  the current phase. Subsequent ticks, separate and combined map/object/script/
  text/sprite discard, and failed event continuations cannot resurrect it. A
  second identity sharing a resource survives. Cache counters prove real reloads.
- Reentrant/report callbacks and failures preserve existing execution/reward
  cleanup policy; add pending and no-pending animation-composition failure cases.
- Capturing unchanged durable owners at two phases yields equal encoded v2
  bytes. F9 preflight and I leave live phase/deadline/presentation untouched.
  Restoring into new owners/Flow yields phase zero, the same persistent removals
  and no initial script replay. Existing v1/v2 tests and save restrictions remain.
- SDL dummy/software test records at least a complete cycle from actual idle
  callbacks without movement, then exits deterministically. Assert phase/frame
  progression and original pixels, not an assumed exact number of callbacks or
  every-adjacent-frame inequality. This is automated evidence, not physical
  acceptance.
- Real Myra no-Root dialogue still has its original pages, portrait and request
  semantics while the underlying tent phase progresses. The original Phirna
  collection/removal and M21 request/return/reward smokes remain regression controls.

**Save/restart boundary:** no new persistent field exists, so no M20/M21-style
manual multi-process exchange ceremony is required. Automated Application
producer/consumer construction and encoded-byte equality prove phase exclusion;
use the existing actual CLI resume smoke to check that the restored effective
scene starts at zero and subsequently animates. A controlled disabled animated
record is a labeled persistence fixture, not a claim that Myra has a Remove script.
The existing original Phirna removal separately remains an end-to-end control.

**Physical acceptance required:** the maintainer uses a native production window
at `23 9 11 west`, observes several tent-flag cycles with no input, opens the
original Myra dialogue, waits, advances its pages, and observes intact text/NPC
presentation and a continuing scene after dismissal. Turn and turn back to check
directional appearance, then confirm responsive input and exit. Supplement with
native static-control images/observations. No automatic key injection or closer
may be described as this human observation. Timing precision and reset identity
are established by deterministic tests, not estimated by eye.

**Validation:** build all changed targets, run focused animation/NPC/visual
Remove/navigation/session/save/reward tests and the **complete CTest suite**.
Build/run the two object smokes, `mmodern_myra_smoke`, `mmodern_phirna_smoke`,
`mmodern_remove_smoke`, `mmodern_save_resume_smoke` and applicable existing Graphics
save/resume controls. Use existing command modes/arguments; any added targeted
mode must document its usage with the test. Original data is never a CTest fixture.
Investigate pixel-oracle changes rather than blindly updating full-frame hashes.

**Exit/closure:** all required build/tests, original-data checks and physical
observations pass, followed by independent review. Only then update stable status,
history, README's public capability description and roadmap per [AGENTS.md](../AGENTS.md),
and condense this plan to its durable final contract. Commit/push require their
own applicable authorization. Closure does not authorize M23 implementation.

## Specification limits and planning validation

There is no unresolved original-data, frame, reset, ownership or timing question
blocking implementation specification. Independent review must assess the
explicit logical action-step policy and the 100 ms derivation; neither is a
claim of full ScummVM dialog-loop or host scheduling emulation. If evidence
requires a different observable contract, amend the plan before implementation.

Planning used read-only source inspection and two temporary C++ probes linked
against existing pinned-dependency MMModern libraries: one enumerated map-1/map-23
animated metadata and validated every reachable sprite frame; one verified the
baseline rejection and rendered the primary cycle using in-memory frozen views.
The visual probe initially rejected identical adjacent cycle images; investigation
showed 2->0 is correctly identical and the corrected exact 334/334/0 assertion
passed. This discovery is reflected in the acceptance oracle above.

No production code, proprietary resource file, dependency or save was modified.
Temporary probes are not deliverables. Planning does not constitute a fresh
full build, CTest run, runtime implementation test or maintainer physical
acceptance. Documentation validation consists of factual/link/diff review and
`git diff --check`; project status remains the stable committed M21 snapshot.
