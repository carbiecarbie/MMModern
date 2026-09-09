# Milestone 22 - Bounded ordinary outdoor object animation

**Milestone 22 is complete.** Stage 22A delivered pure explicit-phase rendering;
22B integrated live ordinary outdoor animation through Application/Flow/SDL.
This closed plan records the final contracts and [acceptance](#final-acceptance).
Current capabilities are summarized in [project status](project-status.md).

## Baseline and post-M21 horizon review

[M15](milestone-15-plan.md) owns removal identity, [M16](milestone-16-plan.md)
owns static outdoor placement/rebasing, [M20](milestone-20-plan.md) owns the
save boundary, and [M21](milestone-21-plan.md) owns the accepted item/reward
extension. M22 extends these contracts without changing their gameplay owners.

The post-M21 horizon review selected M22 before M23 for these reasons:

- The renderer already supplied coordinates, stable identity, metadata, flip,
  sprite safety, terrain order and clipping; ordinary animation needed no new
  world state.
- Ordinary animated records were verified in both map 1 and map 23. The Myra
  location supplies a visible stationary cycle and an existing supported NPC
  interaction, without a new route, item feature or scripted animation opcode.
- M21's reward continuation and M20's save ownership could be preserved.
  Visual phase fits outside their durable state categories.
- The existing idle callback and presenter rebase supported bounded visual
  timing without a second loop or prerequisite refactoring milestone.
- M23 needs indoor-specific object projection and wall occlusion, and still
  lacks a certified original encounter. Its static candidates do not technically
  depend on M22, but moving it ahead would open more unverified rendering/content
  boundaries than this already evidenced outdoor extension.
- General inventory use, combat, movement capabilities and playable-route
  certification remain broader independent concerns. None is required to observe
  or interact at the selected local checkpoint. No new persistent category or
  save compatibility debt blocks this work.

The review retained **M22 -> M23**, with M23 provisional. M22 completion makes
M23 the next planning candidate; [roadmap](roadmap.md) owns its scope and review
cadence. The reference game counter is 20 Hz, but ordinary idle redraws use two
counter ticks; that finding established the bounded 100 ms contract below.

## Pinned reference and exact semantics

The authoritative revision from [dependencies.md](dependencies.md) is
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

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

Validation used an external World of Xeen installation.
Metadata comes from `DARK.CC/clouds.dat`
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

Before 22A, the resolver and production scene diagnostic returned
`UnsupportedAnimation` for this exact identity. In 22A, omitted phase retains
that result, while an explicit phase resolves and renders `SupportedAnimated`.
22B supplies the explicit phase through live gameplay.

Native 320x200 inspection shows the tent's small flag moving.
Frames 0 and 2 look identical at this view: the cyclic appearance is A/B/A,
with a three-step period, not three distinct images. Tests must assert frame
selection and nontrivial motion over a complete cycle, not inequality on every
adjacent transition. All transition differences lie inside the existing scene.

The checkpoint uses the existing public entry point:

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

Flow's `Compose` callback accepts an explicit `std::uint64_t` phase and returns
`Composition { IndexedFrame frame; bool containsOrdinaryAnimation; }`. `refresh`
compares the committed camera and disabled-object count and supports forced
reconstruction without treating it as a timing cause. Composition
loads the metadata resolver, builds an effective outdoor command stream, draws
it, then draws border/UI. Scene building consults `world.isObjectDisabled` before
resolution and preserves the first applicable original record at each placement.
`drawObjectVisual` accepts `SupportedStatic` and `SupportedAnimated` since 22A;
the bridge validates the selected frame even on cached sprites. Rendering never
owns or advances phase, and omitted phase retains the unsupported-animation control.

`updatePresentation()` services ordinary timing and base invalidation before
independently updating a pending NPC, returning at most one combined frame. Presenter
`rebase()` rebuilds semantic layers/pages against the supplied base; its NPC path
uses the existing displayed portrait frame, page and timing without starting a
new page. It does not respond or change Flow's execution generation. These
boundaries preserve presentation while the underlying scene advances.

### State and plumbing

Flow is the smallest owner for **one transient outdoor phase and its deadline**:
it already coordinates camera changes, base recomposition and presentation, and
exists for one gameplay session. Its state is private to `XeenEventFlow`, with
no independently registered system or per-map/per-object store.
Phase advances for the current outdoor scene even when an object is offscreen.
Static-only views must not require repeated full compositions merely to count
phase; entering a visible placement later uses the continuing shared phase.

Delivered interface contract:

- Resolver, outdoor builder and composer accept an explicit optional ordinary
  phase. Omission keeps the existing static-only mode; supplying zero means an
  explicitly reconstructed ordinary scene, not absence of animation support.
- Resolver exposes `SupportedAnimated` as well as `SupportedStatic`, plus the
  selected frame and flip. Cycle length is a local derived value, not a public
  visual field. `UnsupportedAnimation` remains meaningful
  for callers that have supplied no ordinary phase. Neither resolver nor builder
  advances anything. Repeated inputs produce identical outputs.
- `CloudsMapComposer::compose` retains its frame return and diagnostics, with
  a phase input and an optional output reporting whether the emitted command
  stream contains an ordinary animated object. Derive it from actual effective
  commands, never an additional world scan or speculative global metadata flag.
  Fully terrain-occluded commands may conservatively count as animated.
- Flow's composition callback takes the explicit phase and returns a small
  composition value containing the frame and that animation-presence boolean.
  `XeenGameplayServices` passes the phase through to the production binding.
  Keep the boolean as derived recomposition information, not visibility authority.
- Use a monotonic millisecond clock function, defaulting to `steady_clock`,
  injectable in Flow/Application test services. The existing presenter `Clock`
  callable shape is reused. Flow stores one normalized callable before the
  presenter and passes `[this] { return _clock(); }` to it, so even a stateful
  functor has one clock source. `XeenGameplayServices::clock` is appended to the
  service aggregate and passed through the existing constructor parameter.
  Portrait state, randomness and 150 ms deadlines stay in the presenter.

In 22B, preflight composes with an independent explicit zero phase and ignores the
animation-presence result. It must not consume or reset the live Flow clock/phase.
M22 changes no fields in `XeenWorld`, party, save snapshot or save codec, and does
not increment a save version. Cache counters never determine animation resets.

The production service returns the frame plus the composer's emitted-command
presence result; it forwards phase unchanged. Save preflight validates the
returned frame at zero using its temporary world, ignores presence, and does
not invoke live Flow or its clock. Fresh startup has no redundant preflight.
This validates the selected frame at zero, not every future cell stream; a
malformed later frame still fails when selected, including from a warmed cache.

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

Flow keeps phase, deadline, last recognized committed map/facing and last
successful composition presence in one private transient value. Its scene key
is distinct from the successfully rendered camera: a failed render cannot
rearm the same committed transition on every retry. Phase/deadline decisions
survive composition failure. Presence is updated only after composition returns.

`XeenNavigationFlowResult::cameraAfterMovement` is copied immediately after
`movement.apply`, before automatic execution. A one-shot reset hint recognizes
movement A -> B even if a completing event immediately publishes B -> A.
Subsequent completed destinations are recognized through the timing scene key.
Suspended working cameras and logical script addresses never become that key.

An action is an explicit scene-step cause, not permission for every internal
`drive`/`refresh`/report/presenter call to advance again. In particular, a blocked
navigation action clears a retained label and requests only one step despite
forced recomposition. Calls rejected because Flow is dispatching or presentation
is blocking gameplay create no action step. Synthetic direct-result acceptance
and forced reconstruction preserve phase unless they publish a changed map/facing.
The first `drive` refresh consumes the single action/reset cause after adopting
any suspension. Interaction executes before this forced action refresh; it does
not compose a preliminary action frame. Later drive iterations, continuations
and reporting refreshes carry no additional action cause. The initial pending
failure guard still prevents redispatching the input that triggered cleanup.

After an action/reset, the immediately following SDL idle callback must not double
advance at the old deadline. Non-navigation input does not clear pending layers.
The reference does not promise a fixed phase offset between input-triggered and
idle redraws; this policy preserves its continuation versus reset distinction
while making MMModern's logical update count deterministic.

### Animated base and presentation coexistence

The existing `updatePresentation` path resolves effective-world/camera changes
and any due ordinary step before composing, under the dispatch/reentrancy guard.
A reset supersedes that step; causes coalesce into one base composition at the
final phase followed by `presenter.rebase(base)`. NPC timing updates independently;
the latest combined frame is returned once if either path changed it. This adds
no SDL timer thread, nested loop, world tick, interpreter resume or action injection.

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

**Completed scope:** represent and render an explicitly supplied ordinary phase
through the existing outdoor command pipeline, with no runtime advancement in 22A.

**Production areas:** `XeenObjectVisual.{h,cpp}`, `XeenOutdoorScene.{h,cpp}`,
`CloudsMapComposer.{h,cpp}`, and `formats/xeen/XeenAssetSource.cpp`'s supported
status gate. Metadata layout and bridge/sprite decoding were preserved. Existing
tests/smokes were extended without new CMake registrations. Flow/SDL clock
integration followed in 22B.

**Contract:** explicit optional phase, supported-animated classification and
effective-command animation information use the formula above. Omitted-phase
production callers rejected animation during 22A; 22B enabled live phase input.
Identity, first-record precedence, the 12 outdoor placements, terrain ordering,
scale, clipping, flip, diagnostics and static behavior are preserved. 22A
introduced no scripted state or runtime animation timer.

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
state. Production gameplay and preflight omitted it during 22A. The asset draw
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
Exact observed pixel-change counts are not mandatory goldens.
Controlled session removal is independent of Myra's original quest script.

**Regression coverage:** `XeenObjectVisualTests.cpp`,
`XeenOutdoorObjectTests.cpp`, `XeenOutdoorComposerTests.cpp` and
`XeenObjectSpriteTests.cpp`, plus `ObjectVisualIntegrationTest.cpp` and
`OutdoorObjectIntegrationTest.cpp` for explicit-phase Myra checks using the actual
unaltered metadata.

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

22A completed explicit-phase semantics, original-data frame/visibility validation
and independent review before the separately authorized 22B runtime integration.

## Stage 22B - Live scene animation with preserved presentation

**Completed scope:** stationary ordinary animation through the existing
Application/Flow/SDL path, using the explicit-phase rendering delivered in 22A.
Flow owns the transient timing; gameplay services forward phase and clock, and
navigation exposes its committed camera before automatic execution. Existing
presenter rebasing and SDL idle callbacks required no production redesign.

The reset/step/deadline tables above define live behavior. Save state/wire format,
world mutation ownership, navigation rules and event opcodes are unchanged.
Indoor animation, scripted object sequences, monsters/combat, Darkside expansion,
a general clock/scheduler, per-object clocks and historical per-map phase stores
remain excluded.

### Runtime regression contract

`XeenOutdoorAnimationTests.cpp` covers deterministic Flow behavior and the
Application/service/composer path. Its `xeen_outdoor_animation_sdl` mode invokes
the genuine SDL idle callback with an injected clock; it never assigns Flow's
phase. Existing NPC, visual Remove, reward, navigation, save and original-data
tests retain their continuation and gameplay owners.

The regression boundary includes:

- Phase zero initially, no advance at 99 ms, one at 100 ms, no repeated same-time
  advance and one step after a large stall. Pure refresh/reconstruction adds none.
- One action step for same-map forward/backward movement, blocked movement and
  interaction. Turns/map entry reset instead; an immediate idle cannot replay
  the action's old deadline. Same-map relocation preserves phase, and committed
  A -> B -> A transitions are observed independently of final camera equality.
- Suspended working cameras never reset live timing. Pending/reentrant rejected
  input does not step; logical timing survives pending and no-pending composition
  failures, including malformed later frames selected from a warm sprite cache.
- Static-only outdoor views advance logically without periodic full composition;
  newly visible objects use that phase. Indoor views do not advance outdoor timing.
- Independent ordinary/NPC boundaries at 99/100/149/150/200/300 ms and after a
  stall, with one combined outward result and no extra NPC random consumption.
  A stateful injected clock is shared rather than copied between consumers.
- Noninitial pages, retained labels, response kind/generation, blocking, character
  selection, rewards and portrait state survive rebase. Dismissal reveals the
  current base, including animation entirely covered by a panel. Refresh cannot
  replay scripts, responses or reward delivery.
- Remove excludes the exact identity at current phase while shared-resource
  siblings survive. Separate and combined map/object/script/text/sprite rebuilds,
  pending presentation and failed continuations preserve authoritative removals.
  Due idle and effective mutation coalesce into one composition.

### Persistence and acceptance oracles

Animation timing is absent from `XeenSaveSnapshot` and the wire format: v2 writes
and v1/v2 reads remain unchanged. Captures of equal durable owners at different
ordinary phases encode identically. F9 preflight composes independent phase zero
without accessing live timing; I inspection is also neutral. The live-world test
observer is established by `observeGameplay`, never retained from temporary
preflight composition.

Fresh restored Flow starts at zero with its first due tick at +100 ms, retaining
saved disabled identities without initial script replay. Application save/resume
and actual CLI restart remain the persistence controls. A controlled disabled
animated record is a labeled fixture, not a claim that Myra has a Remove script;
original Phirna removal is the separate end-to-end control.

Myra's complete-cycle oracle uses original metadata and production commands at
phases 0/1/2/3, selecting frames 0/1/2/0. It requires target-attributed nontrivial
motion, not inequality at every adjacent phase or an exact pixel count. Dismissal,
abandonment and recovery compare against an independently composed clean base at
the current phase. Combined idle comparisons account for ordinary rebase before
checking NPC-only portrait changes; a returned frame alone does not prove an NPC
advance. Fresh/restored original Myra cycles preserve existing CLI shutdown controls.

Native physical acceptance is distinct from automated SDL and image evidence.
The maintainer observes stationary motion, intact dialogue/NPC presentation and
the current scene after dismissal in the native production window. Deterministic
tests establish precise timing and resets; these are not estimated by eye.

## Final acceptance

**22A, 22B and Milestone 22 are complete.** Validation used the pinned dependency
and external, unmodified original resources; commercial data is not a CTest fixture.

- Fresh Debug build and all 60 unique CTest tests passed, including the new direct
  and SDL animation tests. Timing, failure, ownership, reconstruction and save
  compatibility contracts passed automated validation.
- Required original Myra, Phirna, WhoWill, Remove, save/resume, object, navigation,
  manual-event and Graphics regressions passed. Myra's ordinary cycle and
  independent NPC/dialogue behavior were validated through production paths.
- Independent implementation review passed with no blocking findings or minor
  findings requiring correction.
- Maintainer native physical acceptance passed at Myra: stationary ordinary
motion, existing NPC portrait animation, coherent appearance across viewing distances,
  dialogue/current-scene continuity and functional controls were confirmed.

M22 delivers bounded ordinary outdoor animation without extending indoor or
scripted animation or persistent gameplay state. This is not full ScummVM
host-loop timing emulation or certification of normal travel/playable regions.
[Project status](project-status.md) owns the stable snapshot,
[history](project-history.md#m22---ordinary-outdoor-object-animation) records the
completed milestone, and [roadmap](roadmap.md) retains M23 as the next planning
candidate. Closure does not authorize M23 implementation.
