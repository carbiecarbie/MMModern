# MMModern - Project Status

## Current development state

Current stable milestone: **Milestone 19**

The [Milestone 20 save/resume plan](milestone-20-plan.md) is **approved**.
**20A is independently approved** following commit `4d65e34` and the explicit
APPROVE MILESTONE 20A review. Its [historical evidence](milestone-20-plan.md#12-20a-implementation-and-validation-evidence)
remains intact: A01-A05 closed, A08-A10 partial at that boundary.

**20B is implemented and validated, awaiting independent re-review.** Both
independent stabilization reviews requested changes: first for directory aliases,
then for loss of the checked identity when stripping the extended-path prefix.
The corrected file boundary retains native extended paths through all save I/O.
The distinct `protected` / literal `protected.` junction regression, previous
alias cases and the 6/6, 33/33 and 53/53 selections pass. A06 implementation
evidence is ready for independent reconsideration; 20B is not approved.
Local Windows file
replacement, F9 idle saving and `--load-game` startup resume are implemented.
Debug build, **6/6 save tests**, **33/33 stage-focused tests** and **53/53 full
CTest** pass. The production new-session/resume choice closes A09; A06/A07/A11
and the production responsibilities of A08/A10 are demonstrated. Native/original
checkpoint and physical-window evidence remains for 20C. Bounded original-data
SDL dummy/software idle save/resume and four existing Application controls pass.
See [20B evidence](milestone-20-plan.md#13-20b-implementation-and-validation-evidence).
M19 remains the latest stable milestone; M20 is not complete and 20C has not started.

**M19B and Milestone 19 are complete on 2026-09-08.** The separately authorized
stage adds authoritative in-memory Clouds quest-request state and completes
the original Myra request. The user supplied an independent APPROVE review of
committed/pushed 19A; no independent 19B review is claimed.

M19 completion validation: successful Debug build and required smoke targets,
**23/23 focused tests**, **47/47 full CTest**, and the full **18-case Myra matrix
plus revisits in each of direct and SDL dummy/software modes**. Phirna and Bone
Whistle direct/SDL regressions and manual text/Application controls pass.
Native 320x200 dialogue/portrait captures were inspected; 35 direct captures
match the accepted 19A evidence byte for byte.
Detailed acceptance evidence is in the
[Milestone 19 plan](milestone-19-plan.md#18-19b-implementation-and-validation-evidence).

Separately, on 2026-09-08, the user's post-completion manual validation with
original game resources in a real Windows/SDL physical window passed for Myra,
Phirna Root and the observed Bone Whistle collection flow. This supplements the
prior automated, SDL dummy/software and native-frame evidence; it does not
re-prove the full test matrix. Non-blocking parchment, Yes/No styling and Bone
Whistle acknowledgment/Y-N communication notes are recorded in the
[post-completion validation](milestone-19-plan.md#19-post-completion-physical-window-manual-validation).

The party owns 30 checked Clouds quest flags, loaded LSB-first from the complete
eight-byte field at offset 739 in original party data. Bounded mode-104 set is
immediate and idempotent; it survives later errors, abandonment and reconstruction
with the same party owner. A fresh session loads its own original state.
Game flags, quest-item counters and their existing policies remain separate.

Myra at Clouds map 23 `(9,11)` West without a Root now executes
`0 -> 1 -> 4 -> 5 -> 6` in five instructions. Only quest flag 2 becomes true,
after final acknowledgment. Already-requested visits repeat the same dialogue;
final Escape also records the request. Intermediate paging, pre-acknowledgment
failure and abandonment do not write it. Root counts 1 and 3, with flag 2 false
or true, still stop at unsupported consumption at line 8/offset 255 in three
instructions, preserving every flag, item counter and unrelated state.
NPC rendering/timing code was unchanged. Quest clear/check Action 104, exchange,
rewards, disk persistence, ordinary travel and later milestones remain outside
this M19 acceptance. M20 had not started at that completion boundary.

## Milestone 19A - Historical implementation evidence

**Milestone 19A is complete on 2026-09-08; M19B is pending.** Milestone 19 is
not stable or complete. The separately authorized stage follows the approved
[Milestone 19 plan](milestone-19-plan.md#17-19a-implementation-and-validation-evidence).
No independent review of 19A is claimed.

Latest validation: successful Debug build, **21/21 focused tests**, **46/46 full
CTest**, direct and SDL dummy/software original Myra, Phirna and Bone Whistle
smokes. Native 320x200 request/return, four portrait frames, timed sequence,
rest/rebase/dismissal and affected text controls were inspected. No physical
window was tested.

Clouds NPC mode 1 now uses strict five-byte decoding, current-record title/body
lookup, sequential acknowledgment continuation, positioned titles and complete
body pagination. Original FAC portraits/frame use the existing asset cache and
multi-cell renderer. Bounded four-frame speech/rest timing advances through an
optional SDL idle callback. Space/Enter/Escape advances pages or acknowledges the
final page; Escape remains cancellation for WhoWill. Generation consumption,
cache rebasing, retained layers and explicit presentation-failure cleanup are
covered without a general animation scheduler.

At original Clouds map 23 `(9,11)` West, no Root executes `0 -> 1 -> 4`, shows
the two-page request and stops after acknowledgment at unsupported mode 104,
line 5/offset 228, four instructions. Root counts 1 and 3 execute `0 -> 7`,
show the one-page return and stop at unsupported consumption, line 8/offset 255,
three instructions. All modeled party, item-counter, camera, game-flag and world
state stays unchanged. Repeat, abandonment, real cache reload, new event/flow
owners and fresh original loading pass. Quest flags are not modeled or loaded;
19B's quest-flag storage/loading/set remains entirely pending. No consumption,
reward, save/load, world-animation or additional NPC mode/service was added.

## Milestone 18B - Historical stabilization evidence

**18A is complete and independently approved; 18B is complete on 2026-09-08.**
The approved [Milestone 18 plan](milestone-18-plan.md#16-18b-original-data-acceptance-and-stabilization)
records A01-A36 acceptance and stabilization. No independent review of 18B is claimed.
Milestones 14-17 remain complete; their detailed records below are historical.

M18B validation: **44/44 full CTest**, **31/31 focused regressions**, successful
build and Bone Whistle/Phirna smokes directly and through SDL dummy/software.
New native frames were inspected; no physical-window test was performed.

Original Bone Whistle interaction on Clouds map 20 `(5,14)` North now has full
local acceptance: WhoWill -> original display -> acknowledgment -> one item 100
-> Remove of object record 1. Count 0 becomes 1 in ten instructions, event records
1-5 become effective None, and the returned frame shows removal with success text.
Repeat runs five None and grants nothing. Actual cache reload, controlled leave/
return, fresh-session restoration, cancellation and retry passed. Fresh Escape,
SDL_QUIT, input repeats, navigation blocking/recovery and existing 18A audit
regressions remain covered. Phirna No/harvest/owned outcomes remain unchanged.

18B changes tests and documentation only; no production or dependency change was
needed. It does not certify normal travel to this checkpoint, combat, Orothin's
quest completion, inventory/consumption, save/load or Darkside gameplay. No known
pending issue remained within its approved scope. No later milestone was started
as part of M18B.

## Milestone 18A - WhoWill and temporary character context

Completed on 2026-09-07, in the existing decoder/interpreter/EventSystem/presenter/
EventFlow/SDL/Application path. WhoWill strictly decodes two bytes, resolves
map text, handles party cardinality and worst-condition eligibility, and owns
value metadata through suspension. Selection is a temporary 0-based active-party
index, retained through displays and Call/Return, reset at independent dispatch
and TeleportAndContinue, and consumed by existing Action 9 SP comparisons.

The production continuation owner consumes responses once, validates presentation
generations and preserves them on rebase. Live party identity, range and eligibility
are checked before acceptance. F1-F6 selects; contextual Escape cancels the entire
event, retaining existing camera/flag completion and immediate grant/Remove policies.

Original Clouds map 20 `(5,14)` North was exercised from line 0 through production
loaders and flow, directly and with SDL dummy/software. Valid selection reached
the original following display/acknowledgment; independent cancellation executed
no later presentation, grant or Remove. Original-font choice, all 32 verbs, refusal
layout, next display and cancellation frames were inspected at native resolution.
No physical-window validation was performed. Existing Phirna smoke passed unchanged
in direct and SDL modes. Commercial resources were read only; outputs are ignored
under `build/18a`.

At the historical 18A boundary, full collection/repeat/session certification was
not performed. 18B has now supplied that evidence; see the current status above.

## Milestone 13

Milestone 13 established the first functional Xeen event execution foundation.

Current implemented capabilities include:

- Loading original Xeen game resources
- Outdoor map rendering
- Indoor map rendering
- Player navigation
- Collision handling
- Loading real party data
- HP and SP state
- Xeen event decoding
- Xeen event script execution
- Automatic map events
- Event conditions
- Event calls and returns
- Teleport events
- Game flags
- Automated tests
- Manual runtime validation

At the completion of Milestone 13:

- Full automated test suite: **27/27 passing**
- Manual validation: **passing**
- First public Git repository created
- Milestone 13 is the initial public MMModern codebase

## Milestone 14 - Manual world interaction and text events

The main goal of Milestone 14 is to allow the player to manually
interact with the Xeen world.

The intended gameplay flow is:

Player faces an object or direction
-> presses Space
-> MMModern resolves the event using the current position and direction
-> the appropriate event script executes
-> text or other basic interaction feedback is displayed

Unlike automatic events, manual interaction should not require the
automatic-event bit (`0x10`).

### Planned investigation / stages

#### 14A - Xeen event text resources

**Status: complete.**

Investigate and implement loading of the Xeen `aazeXXXX.txt` resources.

Provide a reliable mapping between event text indices and the original
game strings.

Implemented behavior includes:

- read-only access to event text resources in the outer `xeen.cc` archive;
- NUL-delimited parsing with raw bytes and empty entries preserved;
- zero-based lookup;
- `aazeXXXX.txt` / `aazexXXX.txt` resource naming;
- distinct missing-resource, empty-resource, and invalid-index states;
- synthetic tests and validation against real map 1 data.

#### 14B - Manual interaction input

**Status: complete.**

Add manual interaction, initially through the Space key.

Resolve event execution using:

- current map
- current player position
- current facing direction

Manual interaction must use the appropriate Xeen event rules rather than
requiring the automatic-event flag.

Implemented behavior includes:

- Space dispatches an interaction action distinct from navigation;
- repeated keydown events are ignored;
- lookup starts at line 0 on the current cell and uses the current facing;
- original first-match ordering and the all-directions entry are preserved;
- manual lookup is independent of the automatic-event bit (`0x10`), while
  automatic events remain gated by it;
- no-event, completion, execution-error, and unsupported special-interaction
  outcomes are distinct;
- supported scripts retain the existing transactional camera/flag behavior;
- applicable Clouds grate/door handling is recognized before ordinary event
  lookup and reported as unsupported without changing the world.

Validation completed for 14B:

- full automated test suite: **30/30 passing**;
- synthetic SDL validation covers Space dispatch, repeat suppression, release
  and re-press, and continued navigation input;
- real-data validation at map 1 `(8,8)`, facing West, reaches the expected
  `Display0x01` instruction at offset 461 without changing camera or flags;
- headless runtime validation confirms that the unsupported text opcode is
  reported and the application continues accepting interaction and navigation.

Text opcode semantics are implemented by 14C; graphical presentation remains 14D.

#### 14C - Text/display event opcodes

**Status: complete.**

Investigate and implement the initial text-oriented Xeen event opcodes,
including relevant examples such as:

- `0x01 Display0x01`
- `0x02 DoorTextSml`
- `0x03 DoorTextLrg`
- `0x04 SignText`
- `0x29 DisplayBottom`
- `0x31 DisplayBottomTwoLines`
- `0x35 DisplayMain`

Implemented behavior includes:

- strict decoding for all seven planned display opcodes, including the
  two-byte `DisplayBottomTwoLines` layout;
- distinct centered-message, reduced/normal scene-label, sign-label,
  bottom-window, two-line bottom-window, and main-window semantics;
- zero-based event-text resolution against the currently executing map while
  preserving raw string bytes and valid empty strings;
- distinct missing-resource and invalid-index diagnostics;
- resumable execution state retaining the logical address, current script,
  working camera and flags, call stack, instruction count, transfer state, and
  pending presentation request;
- response validation for presentation completion, acknowledgment, and Yes/No;
- `DisplayBottomTwoLines` acknowledgment followed by immediate event
  termination without executing the next script line;
- restricted condition Action 44 support: value 0 requests Yes/No with original
  comparison values Yes=`0` and No=`2`; value 1 requests acknowledgment and
  compares as `1`;
- transactional camera and flag commit/rollback across any number of
  presentation suspensions.

Validation completed for 14C:

- full automated test suite: **31/31 passing**;
- focused synthetic coverage for all seven opcodes, malformed operands, text
  errors and empty text, semantic presentation kinds, repeated suspension,
  instruction limits, calls/returns, cross-map text, rollback, and Action 44;
- all existing interpreter, event-system, manual-event, navigation, and SDL
  input regressions remain passing;
- real Castle Basenji map 1 `(8,8)`, facing West, resolves text index 19 from
  `Display0x01` at offset 461, then requests the Action 44 Yes/No response;
- the real-data No path completes without teleport, while Yes follows the
  existing supported teleport implementation.

14C remains headless at the presentation boundary. It does not parse fonts,
interpret text formatting bytes, paginate by pixel metrics, or render windows.

The `NPC` opcode requires a later or separate stage if it introduces
dialogue, portraits, confirmation, branching, or other larger systems.

#### 14D - Gameplay UI integration and validation

**Status: complete.**

Implemented behavior includes:

- independent decoding of the original English `fnt` resource, including its
  normal and reduced 8x8 glyph banks and per-glyph advance tables;
- indexed-color glyph rasterization with clipping, metric wrapping, centered
  alignment, supported formatting controls, explicit diagnostics for
  unsupported controls, and page splitting for the supported windows;
- distinct scene-label placement for reduced door text, normal door text, and
  signs, plus the original main, bottom, two-line bottom, and centered-message
  window regions;
- presentation composition directly into the existing 320x200 `IndexedFrame`;
- a runtime presentation controller that uses the 14C response protocol and
  keeps SDL and drawing concerns out of the event interpreter;
- presentation handling in the existing SDL loop without nested event loops;
- blocked gameplay input while an acknowledgment or choice is pending, with
  navigation restored afterward;
- Space or Enter for acknowledgment and Y/N for the standalone Yes/No mapping;
- removal of scene labels on the next gameplay action and recomposition after
  event-driven camera changes.

Validation completed for 14D:

- full automated test suite: **32/32 passing**;
- synthetic coverage for deterministic font parsing, normal/reduced glyphs,
  glyph metrics, clipping, formatting diagnostics, metric wrapping, semantic
  composition, pagination, response mapping, and gameplay blocking/resumption;
- SDL input coverage confirms repeat suppression and the Space, Enter, Y, and
  N mappings while Escape retains application-exit behavior;
- the real map 1 `(1,14)` West SignText renders the two-line `Air` / `Corner`
  label over the scene;
- the real map 31 `(5,1)` West DoorTextSml renders `Snake Oil` with the reduced
  font over the doorway;
- Castle Basenji at map 1 `(8,8)` West visibly renders text index 19 and its
  Yes/No choice; No completes without teleport and Yes recomposes at the
  existing teleport destination;
- headless SDL runtime checks exercise blocked navigation, both Yes and No
  response paths, resumed navigation, and clean Escape shutdown;
- captured indexed frames for all selected real cases were inspected at native
  320x200 resolution, including the distinct reduced font and the post-teleport
  frame.

Milestones 14, 15 and 16 are complete.

## Out of scope for Milestone 14

Unless required by investigation, Milestone 14 is not intended to
implement complete:

- NPC interaction
- shops
- inventory
- combat
- Swimming / Walk on Water capabilities

Swimming / Walk on Water remains navigation/capability work and should
not redefine the primary goal of Milestone 14.

## Milestone 15 - Mutable session-world state and Remove

**Status: complete; 15A, 15B, and 15C complete.**

The [dedicated plan](milestone-15-plan.md) is the specification for this milestone.
It defines the minimum session-owned world mutations needed by the original
`Remove` operation, independently of disposable map and script caches.

Approved stages:

- **15A - Session ownership and side-aware map identity:** complete.
- **15B - Stable object/event identities and Remove execution:** complete.
- **15C - Same-session persistence and integration validation:** complete.

The real-data checkpoint passed on Clouds map 23 at `(8,2)`, starting at the
original `Remove` boundary. Production `.mob` loading and selection confirmed
original object record 13 / resource 111 during 15B. This does not include quest-item grants, visible
entity rendering, disk save/load, or Darkside gameplay.

Implemented and validated in 15A:

- `XeenMapIdentity` carries side + numeric map ID through camera, world/cell
  queries, scene command provenance, event/text providers and caches, logical
  addresses, call stacks, presentation requests, and diagnostics;
- numeric resource neighbor/teleport operands are resolved in the current side;
- `XeenWorld` owns a separate session-state boundary, deliberately empty at completion of
  15A, and cannot be copied; 15B now populates it with real mutation identities;
- explicit map, script, and text cache discard forces subsequent loading while
  preserving session ownership and suspended execution values;
- map/script/text identity mismatches are rejected, and real Clouds resource
  adapters reject Darkside requests; both sides are exercised only synthetically;
- existing physical/logical location separation and camera/flag transactions
  are preserved.

15A validation completed on 2026-09-07:

- Debug build in `build/15a`, MSYS Makefiles, MSYS2 UCRT64 GCC 16.2.0;
- source dependency `../scummvm-known-good-candidate`, verified clean at
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd`;
- dependency artifacts from `../build-scummvm-6814ee9b-ucrt64`;
- focused world/navigation/event/presentation suite: 16/16 passed;
- final full CTest suite: 33/33 passed, including the new session identity test;
- all nine existing real-data smoke executables passed: party, indoor map,
  event script, event text, game flags, interpreter, event system, manual event,
  and navigation flow;
- SDL dummy/software runtime passed for ui, map, indoor, event, manual,
  manual-no, and manual-yes with Escape, plus ui with SDL quit;
- generated 320x200 frames for Air / Corner, Snake Oil, Castle Basenji question,
  No, and Yes destination were visually inspected;
- validation logs and frames are local ignored outputs under `build/15a`.

### 15B implementation and validation

15B completed on 2026-09-07:

- Separate object/event identity types use full side + map + original record
  index. `XeenWorld` owns disabled-identity sets in `XeenSessionWorldState`,
  independently of disposable geometry/object/script caches.
- `XeenMapLoader::loadObjects` lazily reads `mazeXXXX.mob` through the existing
  initial-resource adapter and `parseMob`. Missing, valid empty, and malformed
  resources remain distinct; real adapters reject Darkside. Original tables,
  record order, disabled records, and all entity metadata remain intact.
- Production selection chooses the first active, resource-valid, session-enabled
  object at the current physical cell in original order. It is independent of
  rendering. Selection is a value across calls and presentation suspension;
  teleport clears it and continuing execution resolves the destination.
- `Remove` (`0x0E`) accepts zero operands. It validates context before disabling
  the selected object, if any, and every event at the physical `workingCamera`
  cell, regardless of direction or line. Invalid map/side/index is an error;
  absent or already-disabled valid selection is accepted.
- **Remove restarts at line 0 of the current logical address**, preserving
  logical X/Y and the call stack. Each dispatch consults session-effective
  state even through a previously copied script. Disabled records retain first
  match/order and all base metadata; only effective opcode becomes `None`.
  Only None ignores retained operands; unrelated decoder checks stay strict.
- World effects apply immediately and survive later camera/flag rollback.
  Focused reconstruction tests preserve selected identities and mutations;
  the comprehensive persistence/new-session matrix remains 15C.

Validation performed:

- Full Debug build in `build/15b`, MSYS Makefiles, MSYS2 UCRT64 GCC 16.2.0;
  clean ScummVM source at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, using
  `../scummvm-known-good-candidate` and `../build-scummvm-6814ee9b-ucrt64`.
- Focused decoder/world/interpreter/presentation/manual/session/Remove tests:
  7/7 passed. Final full CTest: **34/34 passed**. New `xeen_remove` tests cover
  identity isolation, loading, selection, invalid context, metadata retention,
  copied scripts, suspension, calls/transfers, instruction limits, and rollback
  policy. Decoder tests cover zero-operand Remove and retained None operands.
- `mmodern_remove_smoke` passed against the installed commercial data:
  map 23 MOB contains 20 objects; original index 13 is active, resource 111,
  at `(8,2)`, and is selected by the production resolver without injection.
  EVT contains 170 records; cell indices 125-135 retain the approved offsets,
  All direction, and lines 0-10. Remove is record 132 / line 7 / offset 1113
  with no operands. Execution dispatched **12 instructions: Remove followed
  by 11 effective None instructions from logical line 0**. Only one object
  identity and 11 event identities were disabled; base/unrelated state stayed
  unchanged. The preceding TakeOrGive was neither executed nor modified.
- Real-data event-script, event-text, interpreter, event-system, manual-event,
  and navigation-flow smoke tests passed.
- SDL dummy/software `event`, `manual`, `manual-no`, and `manual-yes` scenarios
  passed, including presentation input blocking/resumption and Escape shutdown.
  Generated Air / Corner, Snake Oil, Castle question, No, and Yes destination
  frames were visually inspected at 320x200.
- Logs and generated frames remain ignored local outputs under `build/15b`.

### 15C integration and validation

15C completed on 2026-09-07. No production defect or missing lifecycle hook was
found: the stage added focused lifecycle coverage and extended the real-data
checkpoint without changing the session architecture.

- A new `xeen_session_persistence` test proves same-owner leave/return,
  independent and combined map/object/script/text cache reconstruction,
  presentation suspension with logical/physical state, selected identity and
  call stack preserved, two-way Clouds/Darkside synthetic identity isolation,
  and original effective state in a genuinely new `XeenWorld` owner.
- Provider counters distinguish cache reuse from reconstruction. The synthetic
  test verifies each cache is empty after discard and that its provider is
  called again before accepting the persistence result.
- The production map 23 checkpoint now performs normal line-0 interaction after
  Remove, loads another real Clouds map and returns, rebuilds each cache
  separately and together, exercises real Castle Basenji text-cache reload, and
  creates a fresh production-equivalent world/event graph. The final observed
  provider totals were maps=6, objects=5, scripts=5, and texts=3.
- In the original session, object 13 remains unselectable and exactly event
  identities 125-135 remain effective None throughout every lifecycle. In the
  new session, production selection returns object 13 again and all 11 original
  opcodes, including Remove at record 132, are effective again. Commercial MOB
  and EVT bytes remain unchanged.

15C validation used a fresh Debug build in `build/15c`, MSYS Makefiles and
MSYS2 UCRT64 GCC 16.2.0. The configured ScummVM source and artifacts were
`../scummvm-known-good-candidate` and
`../build-scummvm-6814ee9b-ucrt64`; the source SHA was confirmed as
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` with no content differences. The
default Windows Git configuration reports line-ending normalization noise for
that external checkout; status is empty with `core.autocrlf=false`, and no
dependency file was modified by this work.

Validation results:

- focused lifecycle and affected subsystem tests: **10/10 passed**;
- final complete CTest suite: **35/35 passed**;
- all ten real-data smokes passed: Remove lifecycle, party, indoor map, event
  script, event text, flags, interpreter, event system, manual event, and
  navigation flow;
- SDL dummy/software runtime passed for `ui`, `map`, `indoor`, `event`,
  `manual`, `manual-no`, and `manual-yes` with Escape, plus `ui` with SDL quit;
- Air / Corner, Snake Oil, Castle Basenji question and No state, and the Yes
  teleport destination were regenerated and visually inspected at 320x200.

Milestone 15 is complete. This does not implement complete Phirna harvesting,
quest-item grants, visible plant removal, disk save/load, or Darkside gameplay.

## Milestone 16 - Static outdoor map objects and visual Remove

**Status: 16A, 16B and 16C complete. Milestone 16 is stable.**

The [dedicated plan](milestone-16-plan.md) is the approved specification.
Milestone 16 is limited to static appearance-base objects in supported outdoor
Clouds scenes. Its stages are:

- **16A - Visual resolution and resource safety:** complete;
- **16B - Static objects in the outdoor scene:** complete;
- **16C - Visual Remove and runtime lifecycle:** complete.

The validated real-data target is the installed World of Xeen layout: Clouds
object sprites are read from `XEEN.CC`, while Clouds visual metadata is read
from `DARK.CC/clouds.dat`. This physical resource origin does not change Clouds
gameplay identity and does not implement Darkside gameplay.

### 16A implementation and validation

Completed on 2026-09-07. The [16A implementation record](milestone-16-plan.md#16a-implementation-and-validation-record)
contains the exact contracts, safety bounds, checkpoint values, and evidence.

- `XeenCloudsVisualMetadata` parses exactly 1,452 bytes into 121 immutable
  entries, each with four initial-frame bytes, four raw flip bytes, and four
  limit bytes. Empty, truncated, trailing, and out-of-range data are diagnostic
  failures.
- `XeenAssetSource::readCloudsVisualMetadataFromDarkArchive` explicitly and
  lazily reads `DARK.CC/clouds.dat`; ordinary resource reads remain in XEEN.CC.
  Missing archive/member returns an unavailable result; empty or malformed
  present metadata is an error. Existing application construction and Clouds
  capabilities do not require this optional metadata.
- `XeenObjectVisualResolver` returns stable original object identity, resource
  name, frame, flip, status, and diagnostic, without visibility or scene state.
  Names use `.obj` for IDs 0..99 and `.0bj` for 100..254; FF/invalid IDs are
  rejected. Relative direction follows `(camera + 4 - object) % 4`. The exact
  static rule is `initialFrame + 1 >= frameLimit`; temporal advancement returns
  `UnsupportedAnimation` and cannot be drawn as a frozen substitute.
- Targeted M16 preflight checks frame directories, selected cells and headers,
  dimensions/offsets, compressed row sizes, skips, operands, line advances,
  and stream-copy bounds before the existing decoder/drawer consumes them.
  It does not decode pixels. The checked path uses native 320x200, bounded
  anchors, scale 0..15, and rejects enlargement; it is not a universal codec
  validator. Cached source bytes and the existing SpriteResource cache are
  reused, without session-state or metadata caches.
- All three real checkpoints passed in four camera directions: Phirna uses
  frame 0 with alternating flip; Air / Corner uses frames 1/2/1/0 (South
  mirrored); resource 117 uses frames 1/0/3/2 with no flip. Both cells were
  proven to contribute to composite resources through test-only instrumentation
  of the reused rasterizer, without changing original bytes.
- Twelve native isolated BMPs were generated under ignored
  `build/16a/isolated-objects`. Phirna North/East, Air / Corner West/North, and
  all four resource-117 views were visually inspected with the Clouds palette.
- Fresh Debug build in `build/16a`, MSYS Makefiles, UCRT64 GCC 16.2.0, using
  `../scummvm-known-good-candidate` at clean pinned revision
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd` (`core.autocrlf=false`) and
  artifacts from `../build-scummvm-6814ee9b-ucrt64`.
- Baseline **35/35**, new focused tests **2/2**, focused regression set
  **10/10**, and final full CTest **37/37** passed. All ten existing real-data
  smokes passed, including M14 manual events and M15 Remove lifecycle. All
  seven SDL modes passed with Escape, plus UI with SDL quit, using dummy/software.

All 16A completion criteria were satisfied. That stage only validated isolated
sprites; outdoor integration was subsequently implemented by 16B below.

### 16B implementation and validation

Completed on 2026-09-07. Supported static Clouds objects now participate in
outdoor gameplay scene composition. At the 16B boundary, immediate runtime
visual Remove remained unimplemented; 16C subsequently completed it below.

- `XeenOutdoorDrawCommand` contains a terrain/object variant. The object payload
  retains the authoritative 16A visual value, scale, and lower-clip flag. Common
  fields retain order, anchor, sample index, and raw source map/coordinates.
  Object resource/frame/flip values are not duplicated into terrain fields.
- `CloudsMapComposer` obtains the 16A resolver only for outdoor maps and supplies
  it to `XeenOutdoorScene`. The builder uses the existing sample rotation and
  the twelve approved placements, including the pinned resource-113 alternate
  X/Y row. Objects enter the existing collection before its stable order sort.
  One drawing loop dispatches terrain to `drawSprite` and objects to the
  checked `drawObjectVisual` path, then the original border/UI pass runs.
- Only the current map's MOB is loaded. Signed raw coordinates are compared
  directly; terrain can still cross into neighbors. The first applicable
  original record owns each slot. Base/session-disabled and invalid-resource
  records are ineligible. Unsupported animation or invalid/unavailable visuals
  do not promote overlapping later records. Optional diagnostic output retains
  skipped 16A results; present corrupt metadata or sprites retain exceptions.
- `XeenWorld::isObjectDisabled` is consulted with full stable identity on each
  reconstruction. Explicit recomposition removes a disabled object's command
  and pixels, including after proven map/MOB cache reload. No visibility cache,
  selection change, event change, or Application invalidation was added.
- Real Phirna current-cell composition contributes 541 pixels. Approved depth-1
  and depth-2 commands draw correctly but intervening `ltree.wal` commands hide
  the plant in the final image. The mirrored depth-3 view is partially occluded
  and contributes two final pixels. This was investigated and reported, not
  corrected by moving the approved anchors or changing the draw order.
- Air / Corner renders its two cells over the scene and its original SignText
  remains layered above it. Resource 117 renders frames 1/0/3/2 for N/E/S/W,
  without flip. Border preservation and explicit disabled reconstruction passed
  for all nine real checkpoint views.
- Native 320x200 images inspected: Phirna current/depth1/depth2/depth3 mirrored;
  Air / Corner base and SignText; resource 117 N/E/S/W; Snake Oil; Castle Basenji
  question/No/Yes destination. Depth-3 Phirna is the real partial terrain-occlusion
  checkpoint. Images/logs remain ignored under `build/16b`.
- Fresh Debug build `build/16b`, MSYS Makefiles, UCRT64 GCC 16.2.0; configured
  against `../scummvm-known-good-candidate` at unchanged pinned SHA
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd` and
  `../build-scummvm-6814ee9b-ucrt64`. Dependency status is empty with
  `core.autocrlf=false`.
- Baseline **37/37**, focused regressions **13/13**, final CTest **39/39** passed.
  New `xeen_outdoor_objects` and `xeen_outdoor_composer` cover the full rotation
  and placement matrix, resource 113, identity/precedence/state, current-map-only
  loading, missing/corrupt metadata, cache reconstruction, and direct overlapping
  pixel assertions in the production composer.
- The new outdoor smoke, existing 16A isolated smoke, all ten existing real-data
  smokes, and all eight SDL dummy/software scenarios passed. M14 presentations,
  M15 Remove/persistence, and navigation/collision regressions passed. Snake Oil
  and Castle Basenji's indoor Yes destination match freshly regenerated 16A
  baseline images byte-for-byte; indoor composition code remains unchanged.

All 16B completion criteria are satisfied, with the real Phirna occlusion finding
recorded explicitly. Animated objects, indoor objects, monsters, wall items,
quest-item granting, complete Phirna harvesting, save/load, and Darkside gameplay
remain unsupported. No 16C implementation or automatic post-Remove recomposition
was performed as part of 16B; that work is recorded below.

### 16C implementation and validation

Completed on 2026-09-07. Milestone 16 became the stable milestone at that point.

- `Application::renderMap` uses the new small `XeenEventFlow` coordinator for
  initial automatic events, navigation, interaction, presentation input and
  continuations. Tests use the same production result/composition boundary.
  Session ownership remains in the existing world/event graph.
- The coordinator compares the committed camera and the session's monotonic
  disabled-object count with its last composition. It refreshes before handling
  each result, including suspension/error, and before pending input. This
  detects visual Remove without motion, a second visibility authority, or
  continuous tick recomposition. Explicit reconstruction reuses the same flow.
- Presenter rebasing rebuilds retained text layers, underlays, current frame
  and pending pages over the effective scene. It preserves page, response and
  blocking state without restarting events. Confirmation dismissal removes
  only the choice layer; valid M14 labels survive completion.
- Suspension renders the committed camera, never the logical script address
  or uncommitted working camera. Errors retain the existing camera/flag rollback
  and immediate world effects. Manual diagnostics remain nonfatal; automatic
  errors still throw and terminate the application/SDL loop. The flow refreshes
  its result before error reporting, without claiming a post-exception SDL frame.
- `discardSpriteCache` clears existing cached bytes and decoded sprite owners
  safely while keeping providers and the session alive. Entry/load counters
  distinguish actual reads and sprite reconstruction from cached drawing.
  Existing map/object/script/text caches are reused; no metadata cache added.
- New synthetic runtime tests cover manual/automatic paths, trigger gating,
  no-motion Remove, same-sprite identity isolation, pagination, Yes/No and
  acknowledgment, suspension across cache discard, mutation while suspended,
  immediate continuations, later errors, camera/flag rollback, no-event and
  no-mutation labels, navigation blocking/resumption and fresh-session isolation.
  Two visible objects contribute 1,568 pixels before Remove and 368 afterward;
  the selected object's 1,200 pixels disappear while its neighbor remains.
- The extended original Remove smoke confirms map 23 `(8,2)` North, production
  selection 13 / resource 111, 170 EVT records and unchanged indices 125-135,
  Remove index 132 / line 7 / offset 1113 with zero operands. The interpreter
  executes Remove + 11 None. The shared runtime returns a frame with exactly
  541 changed scene pixels, full effective-reference equality and unchanged UI,
  without movement. Subsequent normal interaction executes 11 None. Commercial
  bytes, TakeOrGive, original record metadata and unrelated state remain intact.
- Same-session leave/return, independent and combined cache reconstruction,
  and real Castle Basenji suspension preserve absence. Provider totals:
  maps=8, objects=8, scripts=6, texts=4; successful sprite constructions=89.
  An isolated surviving-object draw after cache discard constructs exactly one
  sprite; its repeated draw constructs none. A genuinely new world/event/
  presenter graph restores selection, original opcodes and initial pixels.
- Existing `build/16b` baseline: **39/39 passed**. Fresh Debug `build/16c`,
  MSYS Makefiles/UCRT64 GCC 16.2.0, confirms source
  `D:/Projetos/MModern/scummvm-known-good-candidate` at unchanged clean
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd` (command-local
  `core.autocrlf=false`) and artifacts
  `D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64` in CMakeCache.
  Focused regressions **14/14 passed**; full new-build CTest **40/40 passed**.
- All twelve real-data smokes passed after explicit build: party, indoor map,
  event script/text, flags, interpreter, event system, manual events, navigation,
  Remove lifecycle, isolated object visuals and outdoor objects. SDL
  dummy/software passed seven existing modes with Escape, UI with SDL quit,
  and the new original Remove checkpoint using the same result flow.
- Native 320x200 frames were visually inspected for before/after Remove,
  return/reconstruction/new session, text over the modified scene, real control
  objects, synthetic shared-sprite objects and post-mutation errors, plus Air /
  Corner, Snake Oil, Castle Basenji question/No/Yes, outdoor occlusion/direction
  and isolated sprite regressions. Logs/images remain ignored in `build/16c`.
  SDL dummy/software plus frame inspection is not physical-display validation.

The full [16C validation record](milestone-16-plan.md#16c-implementation-and-validation-record)
documents paths, camera/error policy, cache evidence and visual checks.
All approved Milestone 16 criteria are satisfied. Complete Phirna harvesting,
TakeOrGive quest-item grants, animation, indoor objects, save/load and Darkside
gameplay remain excluded. Space does not bypass the unimplemented quest path.
No commit, push, tag or branch was created; no next milestone was started.

## Milestone 17A - Quest-item state, possession and refusal completion

**Status: complete on 2026-09-07.**

The following records the 17A-only implementation boundary, before 17B.

- Party-owned state holds exactly 35 unsigned 32-bit Clouds counters with checked
  script-ID mapping 82..116 and bounded queries. Initial loading requires bytes
  747..781 of `maze.pty`; truncated required prefixes fail explicitly.
- Condition action 21 compares the requested item ID when possessed, or
  `UINT32_MAX` when absent, using the existing comparisons. Unsupported IDs,
  non-Clouds logical/physical contexts and empty parties remain errors.
- A resumed Action-44 value-1 acknowledgment may complete at an absent immediate
  numeric successor, without wraparound. Present instructions and all other
  explicit transfers retain normal validation and instruction limits.
- Original line-0 Phirna No completes in 3 instructions; Yes without a root
  reaches the expected unsupported line-6 grant in 6; already-owned refusal
  completes in 5. Counts remain 0/0/1 and the plant remains present in all cases.
  No grants, normal-flow Remove, mutable party propagation or 17B behavior added.
- Fresh Debug build against pinned ScummVM `6814ee9b` passed. Focused tests
  passed 3/3 and full CTest passed 41/41. Original-data checkpoints, previous
  M14-M16 smokes, SDL input/lifecycle checks and native-frame visual inspection
  passed. SDL used dummy video/software rendering, not physical-display hardware.

See the [17A implementation and validation record](milestone-17-plan.md#17a-implementation-and-validation-record)
for the exact checks and local artifacts. Original assets remained external and
read-only. At the 17A handoff, changes were left uncommitted for review; no push,
tag or branch operation was performed.

## Milestone 17B - Quest-item grant and complete normal Phirna harvesting

**Status: complete on 2026-09-07; independent final review approved.**

- The caller-owned party supports checked one-at-a-time increments of the 35
  Clouds counters, including values above 255. Overflow leaves state unchanged.
- Ordinary TakeOrGive supports only neutral first/third pairs and give mode 21
  with ID 82..116, alongside the previous mode-20 flag operations. Invalid IDs,
  context, empty party, overflow and impossible continuation fail before grant.
- Mutable party access is confined to execution paths. Successful grants persist
  immediately through later errors; camera/game flags keep their previous commit
  policy. Suspensions retain continuations, not copies of authoritative counters.
- Original Phirna line-0 Yes completes in 18 instructions: one root, original
  Remove, then eleven effective None records. The returned shared-flow frame
  already shows removal and retained success text. Later interaction runs eleven
  None records and cannot grant again. No/refusal remain 3/5 instructions.
- Same-session cache reconstruction preserves ownership/removal; a new
  party/world/events/presenter graph restores zero roots and the visible plant.
- Fresh pinned-dependency Debug build and all smoke targets passed. Final focused
  CTest passed 4/4 and complete CTest passed 42/42. All A01-A13 acceptance cases,
  original-data and SDL checkpoints passed. Native 320x200 frames were inspected;
  SDL used dummy/software rendering, not physical-display hardware validation.

The [17B validation record](milestone-17-plan.md#17b-implementation-and-validation-record)
contains the acceptance mapping and local evidence. The 17A line-6 unsupported
boundary above is historical and is now replaced by the bounded grant.
No consumption, generic inventory/TakeOrGive, Myra completion, save/load or
future milestone work was added. Original commercial resources were not modified.

### Milestone 17 final stabilization

Independent final review returned **APPROVE MILESTONE 17**. No implementation,
test, architecture, scope or validation issues were found. Both stages are
complete, all A01-A13 acceptance cases passed, and the full CTest result remains
**42/42**. Milestone 17 was the latest stable milestone at this historical
stabilization boundary. The recorded SDL
validation remains dummy/software, not physical-display hardware validation.

Final stabilization changes documentation/status only and preserves the detailed
validation records. No commit, push, tag, branch change or history rewrite was
performed during stabilization itself. The completed Milestone 17 was
subsequently committed on main at
`b59f39e975173444e3324787209d4ad20b5a0b70`. M18 had not been approved or started
at the time of that stabilization; its later planning approval is recorded below.

## Milestone 18 - WhoWill and Bone Whistle collection

**Status: complete; M18 was the latest stable milestone at this historical
completion boundary, within its approved scope.**

- **18A:** implemented, validated and independently approved (APPROVE 18A).
  The implementation and audit history above remain unchanged.
- **18B:** original-data acceptance and stabilization completed on 2026-09-08.
  A01-A25/A30-A32 use the existing tests, rerun successfully; A26-A29 and
  A33-A36 are closed by the extended original smoke and regression runs.
- Extended `WhoWillIntegrationTest.cpp` verifies exact original instructions,
  party/world snapshots, immediate scene/text results, effective event overlays,
  real cache reconstruction and fresh owners. The existing Phirna snapshot helper
  is now shared by the two tests. No production or CMake change was required.
- New full CTest **44/44**, focused regressions **31/31**, both smoke targets and
  all four direct/SDL executions passed. Native framebuffer inspection passed;
  SDL used dummy/software, with no physical-window validation.
- See the [18B record](milestone-18-plan.md#16-18b-original-data-acceptance-and-stabilization)
  for exact commands, assertions, provider counts, images and limitations.

## Architecture notes

MMModern is a standalone application and is not a fork of the complete
ScummVM application.

The current implementation uses selected ScummVM Xeen components and
ScummVM libraries through the compatibility layer under:

`src/compat/scummvm/`

The local development setup currently expects ScummVM source and build
trees outside the MMModern repository.

See `docs/dependencies.md` for additional dependency information.

## Game data

Original Might and Magic / World of Xeen game data is not part of this
repository.

Development and runtime testing require legally obtained original game data.

## Updating this document

Update this file whenever:

- a milestone begins or ends;
- a major subsystem becomes functional;
- test status changes significantly;
- architecture or major dependencies change.

This document should describe the current repository state rather than
outdated plans or assumptions.
