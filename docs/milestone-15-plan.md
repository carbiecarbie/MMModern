# Milestone 15 - Mutable session-world state and Remove

**Status: complete; 15A, 15B, and 15C complete.**

**Next milestone: not defined or approved by this plan.**

Milestone 15 is the completed stable milestone. 15A and 15B have passed
their required implementation validation. 15C has also passed its lifecycle
validation, completing M15. This document preserves the reviewed specification
and records stage implementation evidence separately below.

## Objective

Establish the minimum authoritative mutable world state with session lifetime
needed to reproduce the original Xeen `Remove` operation. Mutations must survive
map changes and reconstruction of disposable map and script caches. A new
session must recover the original resource state.

The primary validation case is the Phirna Root encounter on Clouds map 23 at
`(8,2)`. It is a mutation checkpoint, not the complete harvesting sequence:
granting the quest item and visibly rendering the plant's disappearance remain
outside M15.

The approved dependency order is **15A -> 15B -> 15C**. Persistence is a design
property established in 15A and must work with mutations in 15B. Stage 15C
validates and closes integration gaps; it must not introduce another owner or
persistence architecture.

## Project decisions and scope boundaries

- Clouds is the first functional and real-data validation target. World of Xeen,
  including Darkside, remains the long-term goal.
- Identity foundations distinguish sides without implementing Darkside gameplay.
- Behavioral fidelity is required; internal representations need not copy
  ScummVM's implementation.
- Original commercial resources remain untouched, recoverable base data.
- Mutations belong to the running session, not a cache or interpreter copy.
- A future versioned MMModern save format is outside this milestone.
- Preserve existing camera/flag commit and rollback guarantees. Do not extend
  them automatically to world mutations.
- Reuse existing abstractions and choose the smallest sufficient integration.
- Do not reorganize unrelated party or UI systems merely to centralize state.
- One authoritative state means a clear owner for each category and coherent
  consumers, not a monolithic object. Immutable snapshots and existing
  transactional working copies remain valid.
- Visible entity rendering and quest-item work are reserved for later milestones,
  provisionally M16 and M17. This plan does not approve or specify those milestones.

Explicit exclusions: quest-item possession/grants, inventory, equipment,
gold/gems/food or other party-resource expansion, character-selection UI, visible
objects or wall-item rendering, monsters/AI/combat, NPC systems, shops/services,
doors/locks/traps gameplay, time/calendar/rest, RNG systems, disk save/load,
original-save compatibility, Darkside gameplay, complete quests, other mutation
opcode families, generic ECS/entity frameworks, general mutation/property/history
systems, and generic transaction/rollback frameworks.

Do not broaden implementation into ScummVM or game-data investigation, rendering
investigation, or future milestone planning. Targeted verification of a concrete
current-stage discrepancy is permitted; original data must never be committed.

## Current implementation baseline

These findings were recorded during review of the proposal. They describe the
pre-M15 implementation, not capabilities delivered by this plan.

| Area | Current implementation | M15 implication |
|---|---|---|
| World | `XeenWorld` caches maps by numeric ID and returns constant references. | Separate mutation lifetime from loaded-map lifetime. |
| Runtime loading | `Application` uses `XeenMapLoader::loadGeometryMap`, without `.mob` data. | Add minimal per-map object loading when needed. |
| Objects | `XeenMapEntity` exposes disabled/active state; `parseMob` preserves record order. | Reuse the structures and parser. |
| Event base | `XeenEventRecord` retains offsets, length fields, and operands. | Keep it as the base event representation. |
| Older event representation | `XeenMap::instructions` lacks metadata retained by `XeenEventRecord`. | Do not make it a second authoritative mutable event store. |
| Script cache | `XeenEventSystem` stores and returns scripts by value. | Changing only its cache cannot update existing copies. |
| Suspended execution | `XeenEventExecutionState` owns a `currentScript` copy. | Consult effective state during execution. |
| Location | `logicalAddress` and `workingCamera` are distinct. | Keep script address separate from physical mutation position. |
| Decoder | `None` uses `decodeEmpty`; `Remove` is unsupported. | Add Remove and narrowly support retained None operands. |

Relevant repository sources:

- `src/games/xeen/XeenWorld.{h,cpp}`
- `src/games/xeen/XeenMap.h` and `XeenMapLoader.{h,cpp}`
- `src/formats/xeen/XeenMapFormat.{h,cpp}` and `XeenEventFormat.{h,cpp}`
- `src/games/xeen/XeenEventScript.{h,cpp}` and `XeenEventSystem.{h,cpp}`
- `src/games/xeen/XeenEventDecoder.{h,cpp}` and `XeenEventInterpreter.{h,cpp}`
- `src/games/xeen/XeenNavigation.h` and `src/app/Application.cpp`

The recorded automated baseline is 32/32 passing. It is not a fresh validation
result from formalizing this plan.

## Evidence provenance and original behavior

### Pinned reference

Use the ScummVM revision documented in [dependencies.md](dependencies.md):

`6814ee9ba54582f5b5adcffab49efbbd8f589edd`

During proposal review, the local `scummvm-known-good-candidate` tree was
confirmed at this revision with a clean status. The existing local `build`
CMake cache pointed to `scummvm-master`. Before implementation validation,
select/configure a build that demonstrably uses the pinned dependency, following
the existing dependency documentation. No dependency configuration was changed
to approve this plan. These local observations are review evidence, not fixed
directory requirements.

Reference paths below are relative to the pinned ScummVM source tree:

- `engines/mm/xeen/scripts.cpp`: `cmdRemove`, `cmdMakeNothingHere`, `cmdExit`,
  `doOpcode`, `cmdDoNothing`, `cmdCallEvent`, and `cmdTeleport`.
- `engines/mm/xeen/scripts.h`: `SCRIPT_ABORT = -1`.
- `engines/mm/xeen/interface_scene.cpp`: `setIndoorsObjects` and
  `setOutdoorsObjects`.
- `devtools/create_mm/create_xeen/constants.cpp`: `SCREEN_POSITIONING_X/Y`.

### Remove effects and execution correction

`Remove` is opcode `0x0E` and has no object-number operand. It acts on the
selected interaction object, when one exists. The reference moves that object
out of bounds without deleting or renumbering it. MMModern may use a clearer
disabled-state representation while preserving effective behavior.

`Remove` calls `cmdMakeNothingHere`, which changes only the opcode of every event
at the party's physical cell to `None` (`0x00`). There is no direction, line, or
branch filter. With no selected object, the event mutation still occurs.

**After applying its mutations, Remove returns execution to line 0 of the
current logical address. It does not simply advance to the next line.**

The reviewed reference control flow is:

1. `cmdRemove` disables the selected object when present.
2. `cmdMakeNothingHere` changes the physical-cell event opcodes and calls
   `cmdExit`.
3. `cmdExit` sets the line to `SCRIPT_ABORT`, or `-1`.
4. `cmdRemove` returns `true`.
5. For a living party, the dispatcher increments the line from `-1` to `0`.

In the Phirna checkpoint, the logical address is the modified cell. Execution
then traverses None records at lines 0-10 and naturally completes when line 11
is absent. A called logical address can differ from the physical cell: mutate
the physical cell, restart line 0 at the current logical address, and do not
arbitrarily clear the call stack. Protect this distinction with synthetic tests.
This milestone does not introduce new death/combat behavior.

### Stable records and None

Object identity is side + map + original zero-based object-record index, not
sprite ID, resource-table index, coordinates, or active/visible-object order.
Event identity is side + map + original zero-based event-record index.

Disabling records preserves count, order, indices, coordinates, directions,
line numbers, operand bytes, length fields, original serialized layout, and
useful diagnostic offsets. Only the effective event opcode changes.

None records remain present in script lookup and retain first-match ordering,
including duplicate keys. They ignore retained operands. Do not erase operands,
shorten instructions, remove records, or allow a later duplicate to replace a
disabled first match. `cmdDoNothing` in the reference does not consume operands.

Remove itself does not change geometry, walls, cell flags, seen/stepped state,
game flags, automatic-event bits, text resources, or unrelated objects. This
restriction does not describe unrelated instructions elsewhere in a script.

### Clouds map 23 checkpoint

During proposal review, the existing MMModern event inspector confirmed:

- resource `maze0023.evt`, with 170 records;
- 11 records at `(8,2)`, original indices 125-135 inclusive;
- offsets `1056, 1063, 1072, 1078, 1087, 1094, 1103, 1113, 1119, 1125, 1132`;
- lines 0-10, all with direction `All`;
- Remove at record 132, line 7, offset 1113, with no operands;
- TakeOrGive immediately before the checkpoint at line 6.

**Original object record 13 / resource 111 is a supplied prior-investigation
finding. Confirmation against the map's .mob data was required during 15B.**
The production checkpoint has now confirmed it; see 15B evidence below.
The proposal review did not re-inspect that .mob file, and this documentation
task does not claim to have verified it.

## Approved architecture

Keep `XeenWorld` alive for the session and make it the owner of world mutations,
separate from its disposable caches. There is no demonstrated need for a new
class aggregating party, camera, flags, and UI.

| Data | Owner and lifetime |
|---|---|
| Original commercial resources | Read-only resource source. |
| Loaded base geometry and objects | Disposable `XeenWorld` caches. |
| Loaded base scripts and texts | Disposable `XeenEventSystem` caches. |
| Disabled object/event identities | Session state owned by `XeenWorld`. |
| Committed camera and flags | Existing runtime owners. |
| Working camera/flags, logical address, selection | In-progress execution state. |

The only persistent world changes required here are sets of disabled object
identities and event identities whose effective opcode is None, grouped by side
and map. Do not introduce arbitrary properties, mutation operations, or history.

Script copies may remain immutable base snapshots. Before decoding an
instruction, resolve its original identity and apply the session's effective
state. An old `currentScript` copy must never restore a disabled opcode.

Effective object queries similarly combine base records with session
disabling. All behavioral consumers must use these queries. Do not create an
independently mutable `XeenMap::instructions` alongside the event script store.

## 15A - Session ownership and side-aware map identity

**Status: complete; validated on 2026-09-07.**

Implemented using `XeenMapIdentity` (side + number) throughout the existing
runtime identity paths. Numeric Clouds entry points remain supported, but the
identity has no implicit conversion back to a number, preventing providers from
silently losing side context. Original binary fields remain numeric.

`XeenWorld` became non-copyable in 15A and gained an initially empty
`XeenSessionWorldState` member separate from its maps; 15B now populates it. `discardMapCache`, `discardScriptCache`, and
`discardTextCache` invalidate only disposable data. Suspended scripts remain
owned base values. No mutation fields or operations were implemented in 15A.

Validation: Debug build, 16/16 focused tests, 33/33 final CTest, all nine existing
real-data smokes, all eight documented SDL runtime scenarios, and visual review
of the five 320x200 M14 presentation frames passed. The build used the clean
pinned ScummVM source at `6814ee9ba54582f5b5adcffab49efbbd8f589edd` and its
existing UCRT64 artifacts. See [project-status.md](project-status.md) for exact
local configuration and results. The remaining subsections retain the approved
15A requirements; completing 15A does not authorize 15B.

### Objective and placement

Establish side/map identity and explicit session/cache lifetimes while
preserving M14 behavior. Remove needs these identities and an owner before
mutations are implemented.

### Required changes

1. Introduce a map identity composed of side and map ID.
2. Propagate it through interfaces locating maps, scripts, and texts.
3. Add side context to the camera's current location without creating a second
   authoritative physical position.
4. Qualify logical addresses, calls, and diagnostics with the necessary map
   context.
5. Establish `XeenWorld` ownership of session changes separately from caches.
6. Define explicit cache discard without destroying the session owner.

Original neighbor IDs and existing teleport operands remain numeric in resource
data and are interpreted in the current side. Do not add cross-side transfers.
Real resource adapters remain Clouds-only and explicitly reject unsupported
Darkside requests rather than silently loading Clouds resources. Synthetic
providers may serve both sides. Do not change binary formats to embed side
information belonging to loading context.

Entering another map retains the same `XeenWorld`. Discarding map/script caches
retains mutation ownership. A new session creates a new owner and discards
pending executions from the old session; read-only resource access may be reused.
No new-game UI is required.

Cache-returned map/cell references are invalid after discard. Suspended execution
must retain identities and values, not pointers into discarded cache entries.

### Scope and tests

Include identity, ownership, lifetime, and adaptation of current consumers.
Exclude Remove execution, premature object loading, general party migration,
disk persistence, and other mutation families. Do not add an artificial mutation
solely to test storage; actual mutation persistence is exercised in 15B.

Adapt world, event, navigation, and presentation tests and verify:

- identical numeric map IDs on two sides produce distinct entries;
- navigation and teleport preserve side;
- diagonal sampling and indoor behavior remain unchanged;
- cache discard forces loading again;
- incompatible returned map/script identities are rejected;
- camera/flag commit and rollback boundaries remain unchanged;
- suspension/resume preserves calls and presentations.

Relevant components include `XeenWorld`, `XeenNavigation`, map/event/text
providers, event context, navigation and scene consumers, and `Application`.

### Completion, risks, and documentation

Build, run relevant tests, and run the full CTest suite (practical at the current
project size). Repeat affected real navigation/event smoke tests. Completion
requires M14 functioning with the new identity, discardable caches, and an
unambiguous session mutation owner.

Risks: leaving a cache/query keyed only by numeric ID, retaining invalid cache
references, or expanding this stage into a general runtime reorganization.

When implementation starts, record M15/15A as in progress in project-status.
After successful validation, record the architecture and actual results. Update
dependency notes only if the documented procedure genuinely changes.

## 15B - Stable objects/events and Remove execution

**Status: complete; implemented and validated on 2026-09-07.**

### Implemented contract and evidence

- `XeenObjectIdentity` and `XeenEventIdentity` use full `XeenMapIdentity` plus
  original zero-based record index. `XeenSessionWorldState` contains only
  disabled-object/event sets, owned by `XeenWorld` independently of caches.
- World APIs provide effective queries, `selectObject`, `disableObject`,
  `disableEventsAtCell`, and validated `applyRemove`. `effectiveEvent` changes
  only a returned copy's opcode; immutable base records remain recoverable.
- Separate lazy object-file caching uses `XeenMapLoader::loadObjects`, existing
  initial-resource access, and `parseMob`. Missing, valid empty, and malformed
  MOB are distinct. Geometry composition does not eagerly load neighboring
  objects; `discardMapCache` discards base object files without session effects.
  Real resource loading rejects Darkside.
- Static selection uses the first eligible original record at the physical
  cell. Suspension/calls retain its value; transfer invalidates it and resolves
  effective destination state when execution continues. No renderer is involved.
- `findInstructionIndex` preserves original first-match lookup. Every dispatch
  consults effective state, including pre-mutation `currentScript` copies.
  None ignores retained operands; Remove requires zero operands. Other decoder
  and binary-parser validations remain strict.
- Remove validates physical map/cell and optional original object identity
  before mutation, disables the optional object and all physical-cell events,
  then **sets logical line to 0**, retaining logical X/Y and the call stack.
  No selection and already-disabled valid selection succeed; invalid index or
  side/map errors do not apply new mutations. World effects survive subsequent
  camera/flag rollback; effective None consumes the existing instruction budget.
- `begin` has an optional initial line for the integration checkpoint. Runtime
  callers still start at 0; no gameplay option bypasses the preceding quest grant.

Validation: full Debug build using the clean pinned ScummVM checkout
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`; MSYS2 UCRT64 GCC 16.2.0 and MSYS
Makefiles in `build/15b`, source `../scummvm-known-good-candidate`, dependency
artifacts `../build-scummvm-6814ee9b-ucrt64`. Focused tests passed 7/7 and final
CTest passed **34/34**, including new `xeen_remove` coverage. Synthetic tests
exercise the required edge cases, copied scripts, a focused cache rebuild,
suspension/calls/transfers, the 1024-instruction boundary, and world mutations
surviving a later failure while camera/flags roll back.

`mmodern_remove_smoke` passed with production loading, selection, and execution:
MOB has **20 objects**, including active original object **13**, resource **111**
at **(8,2)**. Production selection returns 13 without an injected identity.
All 170 EVT records and the 11 cell records match the approved indices,
offsets, lines, and directions below. Starting at record 132 / line 7 /
offset 1113 dispatched **12 instructions: Remove + 11 effective None**, proving
restart at line 0. Only object 13 and the 11 cell event identities were disabled;
base entities/tables/events, geometry, flags, camera, and unrelated effective
state remained unchanged. Line 6 TakeOrGive remained intact and unexecuted.

Affected real-data event-script, event-text, interpreter, event-system,
manual-event, and navigation-flow smokes passed. SDL dummy/software modes
`event`, `manual`, `manual-no`, and `manual-yes` passed with Escape, including
presentation blocking/resumption. Air / Corner, Snake Oil, Castle question,
No, and Yes destination frames were inspected at 320x200. Logs/frames are
ignored outputs under `build/15b`. No material discrepancy was found.

To reproduce the real checkpoint after configuring the documented build:

```sh
cmake --build build/15b --parallel 4 --target mmodern_remove_smoke
build/15b/mmodern_remove_smoke.exe "<installed-game-directory>"
```

This is 15B validation only. The complete 15C persistence/new-session matrix
and full M14 runtime sequence have not been undertaken. The following sections
retain the approved 15B requirements.

### Objective and object loading

Implement the complete minimal Remove contract using 15A's session state.
Object loading belongs here because there is now a concrete consumer.

Load per-map .mob resources through existing initial-resource infrastructure
and `XeenMapFormat::parseMob`. Preserve record counts/order/indices, table index,
resource, direction, and other base fields. Distinguish missing resources,
valid empty resources, and parse failures. Load objects when interaction or
object queries need them; geometry composition need not load neighbor objects.

The existing parser also reads monsters and wall items. Reusing it does not
authorize gameplay for those categories.

### Minimal selection without rendering

The reviewed reference selects from the first scene-object slot. Indoors and
outdoors use `SCREEN_POSITIONING_X/Y[direction][2]`, whose reviewed offsets are
zero in all four directions.

For the static objects covered by M15, the production resolver must:

- search the current physical cell, not automatically the cell ahead;
- preserve original order and select the first effectively active object with
  a valid resource;
- return side + map + original object-record index;
- avoid sprite IDs as identity and remain independent of drawing.

This is a bounded derivation from the reviewed selection code, not a claim to
cover animated/frame-dependent selection or every scene-rendering detail.
Animation and other rendering-specific behavior remain outside M15.

Selection is an optional value in execution context. Presentation suspension
preserves it; CallEvent preserves selection and physical position. A physical
transfer invalidates the old location's selection and resolves selection using
the effective destination state. Never retain cache-element pointers.

| Context | Required behavior |
|---|---|
| Valid selection | Disable the object and physical-cell events. |
| No selection | Disable physical-cell events only. |
| Already disabled object with valid identity | Keep it disabled and apply the event effect normally. |
| Nonexistent index or incompatible map identity | Report a context error before this Remove applies changes. |
| Reconstructed caches | Resolve the same retained identity, without changing the selected object. |

No-selection behavior is reference evidence. Invalid-identity diagnostics are
MMModern integration policy, not a reproduction of invalid reference accesses.

### Execution, decoding, and effective state

For valid Remove execution:

1. Resolve records and validate necessary context.
2. Disable the selected object, if present.
3. Disable every event at the physical cell represented by `workingCamera`,
   using that position's side/map, not a CallEvent logical target.
4. Preserve all event fields other than effective opcode.
5. Return to line 0 of the current logical address.
6. Consult session-effective state on the next lookup/decode.

Do not clear the call stack arbitrarily. Count dispatched None instructions as
normal instructions. Preserve the instruction limit for synthetic cases where
Remove at a different logical cell repeatedly restarts execution.

Specialize opcode `0x00` alone to ignore retained operands. The format parser
still validates structure, lengths, and bounds; Exit, Return, and unrelated
opcodes retain strict operand checks. Decode Remove with zero operands in the
supported subset. Keep disabled records in normal first-match lookup.

World mutations become session state immediately. Camera and flags keep M14
working copies and commit/rollback. A later failure may roll back those values
without reactivating objects or events already changed. Document and test this
integration policy without attributing equivalent transactions to ScummVM.

Validate context and load required data before mutation to avoid predictable
mid-operation failures; this does not require general transactions.

### Required synthetic tests

- Remove decoding, including excess operands.
- None with retained operands; unchanged validation of unrelated opcodes.
- First active object in original order, neighbors, disabled objects, and
  repeated sprite IDs.
- Mutation with/without selection, invalid identity, and already disabled object.
- Every record at the cell, with multiple lines/directions and duplicate keys.
- Retention of operands, lengths, offsets, order, count, and unrelated data.
- Restart at line 0 after Remove.
- Physical cell distinct from a CallEvent logical address.
- Effective changes visible despite a copied `currentScript`.
- Selection surviving suspension/resume and correct transfer invalidation.
- Later failure rolling back camera/flags but preserving world mutations.
- Subsequent interaction observing disabled records.

### Required real-data checkpoint

Confirm in map 23 .mob data that original object 13 has resource 111 and is
eligible for selection at `(8,2)`. Require the production resolver to return 13.
A mismatch is a discrepancy to investigate, not permission to hard-code it.

Start the test at original line 7, record 132, offset 1113, with correct physical
context and the production executor. Do not delete, replace, or artificially
execute the preceding TakeOrGive, and do not bypass it in normal gameplay.

The smallest test support is an explicit initial address at the interpreter
boundary while normal execution still starts at line 0. No public gameplay
option is required. Supplying identity 13 alone proves mutation targeting, not
selection; the real integration checkpoint must separately prove production
selection.

Compare the relevant full state before/after and prove that only the intended
object state and 11 effective event opcodes changed.

### Completion, risks, and documentation

Build, run relevant tests, full CTest, and the real checkpoint. Completion
requires synthetic and real Remove execution with immediate, session-owned
effects. Quest-item grants, rendering, and other opcodes remain excluded.

Main components: object loading/effective queries, `XeenEventScript`, decoder,
interpreter, event system, and suspended context. Main risks: wrong selection,
logical rather than physical mutation, stale copied scripts, or incorrect
line advancement.

Update status and the implemented contract after validation. Any README
capability description must distinguish the mutation checkpoint from complete
Phirna harvesting.

## 15C - Same-session persistence and integrated validation

**Status: complete; validated on 2026-09-07.**

### Implemented evidence

15C required no production architecture change. The new synthetic lifecycle
test and expanded production checkpoint use the existing owner and discard APIs.
They prove actual provider reloads rather than repeated reads from retained
caches.

- One `XeenWorld` owner retains map 23 object/event mutations while loading a
  different Clouds map and returning.
- Separate map/object, script, and text discards empty their caches and increment
  focused provider counters on reload. Combined discard rebuilds all four kinds
  of base data while retaining the same session owner and mutation sets.
- A presentation suspended inside a logical call survives map/object/script/text
  cache discard. Its copied script, logical address, physical camera, selected
  identity, call stack, flags and mutation visibility remain valid on resume;
  subsequent interaction reloads discarded script/text resources.
- Another Clouds map stays unchanged. Colliding numeric object/event identities
  on synthetic Darkside can be mutated independently in both directions and
  remain isolated after cache reconstruction.
- A genuinely new world/event graph owns empty mutation sets and no pending
  execution. Production selection and all original map 23 opcodes are restored
  from immutable resources.
- The extended real checkpoint performs normal line-0 interaction after Remove,
  leave/return, separate and combined cache reconstruction, real text reload,
  and new-session restoration. Provider totals are maps=6, objects=5, scripts=5,
  texts=3. Object 13 remains disabled only in the original session; exactly the
  11 intended events remain effective None there.

Validation used `build/15c`, Debug, MSYS Makefiles and UCRT64 GCC 16.2.0 with
the pinned source SHA `6814ee9ba54582f5b5adcffab49efbbd8f589edd` and existing
UCRT64 dependency artifacts. Focused tests passed 10/10; complete CTest passed
35/35. All ten production-data smokes passed. All eight documented SDL
dummy/software scenarios passed. The five M14 frames for Air / Corner, Snake
Oil, Castle question, No, and Yes destination were regenerated and visually
inspected at 320x200. No production defect or real-data discrepancy was found.

The sections below retain the approved 15C acceptance requirements.

### Objective and acceptance sequence

Prove that 15B survives actual loading and presentation lifecycles. Fix
integration gaps without replacing the ownership/persistence design.

1. Create a fresh session and load map 23 base state.
2. Confirm the reference object and event records.
3. Resolve object 13 through production selection.
4. Execute the original Remove checkpoint.
5. Compare base and effective state: only the object's disabling and 11 event
   opcodes may differ.
6. Confirm restart at line 0 and safe None traversal.
7. Begin normal interaction again at `(8,2)`; the original branch must not replay.
8. Leave/load another map and return to map 23.
9. Discard/rebuild map caches and repeat the checks.
10. Discard/rebuild script caches and repeat the checks.
11. Discard/rebuild both and verify again.
12. In a synthetic suspended-presentation case, reconstruct caches and resume
    without invalid references or restored opcodes.
13. Verify another map and another synthetic side with colliding IDs are intact.
14. Create a genuinely new session and verify original object/opcode restoration.

Use load counters to prove cache reconstruction happened. Re-reading the same
cached object is not sufficient. Cache tests retain the session owner; new-session
tests replace it rather than merely clearing caches.

### M14 regression and manual/runtime validation

Repeat presentation, manual interaction, navigation, calls/returns, and transfer
tests. Follow existing documented smoke procedures and validate:

- Air / Corner;
- Snake Oil;
- Castle Basenji No and Yes;
- blocked navigation during presentation;
- resumed navigation and recomposition after teleport.

Visible plant disappearance is not an M15 acceptance criterion.

### Completion, risks, and documentation

Require a passing build, complete CTest suite, relevant real-data smokes, map 23
checkpoint, actual cache reconstruction, fresh-session restoration, and required
manual/runtime checks. Documentation must describe the implemented scope. An
unexecuted real-data test is not a pass.

Existing synthetic fixtures and installation-directory smoke targets are the
starting infrastructure. Add only checkpoint entry and observable cache discard
support as needed. Do not copy commercial resources into repository fixtures.

Risks: a persistence test that never discards caches, or one that reconstructs
the session owner when it intends only to rebuild a cache. Components are world,
event, and presentation tests plus a focused real mutation smoke target.

At completion update project-status, this plan's stage status, and the README
if public capability descriptions change. Do not claim full harvesting,
save/load, or Darkside support.

## Cross-stage invariants

- Every logical state category has one clear owner.
- Original resource state stays intact and recoverable.
- Cache eviction cannot erase gameplay changes.
- Original object/event indices do not change when disabled.
- Sprite IDs, positions, and visible ordering do not replace record identities.
- Behavioral queries use effective state.
- Script copies are base snapshots, not another mutable authority.
- Side/map identity isolates mutations.
- Physical position and logical script address remain distinct.
- None stays in lookup and cannot expose a later duplicate instead.
- Existing camera/flag guarantees remain; world mutations gain no general rollback.
- A new session discards pending execution from the previous session.
- No expansion into entity rendering, quest-item grants, or later milestones.

## Requirement-to-evidence mapping

The reviewed evidence is recorded here without claiming a new dependency or
commercial-data investigation during documentation approval.

| Requirement | Provenance |
|---|---|
| M14 completed baseline | Current project documentation. |
| Separate caches from mutations | Current architecture plus owner policy for session lifetime. |
| Remove uses selection, with no object operand | Pinned reference; real EVT checkpoint also had no operands. |
| No selection still disables events | Pinned reference, cmdRemove. |
| All events at physical party cell | Pinned reference, cmdMakeNothingHere. |
| Return to logical line 0 | Reviewed combined cmdRemove/cmdExit/dispatcher flow. |
| None ignores retained operands | Pinned reference, cmdDoNothing. |
| Event indices 125-135 and stated offsets | Commercial data inspected with the existing inspector during proposal review. |
| Object 13 / resource 111 | Supplied earlier investigation; confirmed by production .mob loading and selection in 15B. |
| Static selection at current cell | Derivation from reviewed reference selection routines and positioning tables. |
| Invalid-identity error | Approved MMModern integration policy. |
| workingCamera as execution's physical position | Adaptation to current transactional architecture. |
| No general world rollback | Owner policy; not a claim of equivalent ScummVM transactions. |
| Side isolation, new-session restoration, no disk saves | Owner decisions. |

## Expected repository impact

| Stage | Likely areas |
|---|---|
| 15A | Navigation/location, XeenWorld, map/event/text contexts and providers, map consumers, runtime assembly, corresponding regressions. |
| 15B | XeenMapLoader, effective object queries, XeenEventScript, decoder, interpreter, event system, suspended state, focused synthetic and real tests. |
| 15C | World/event/presentation tests, real mutation smoke, cache reconstruction checks and runtime/manual regressions. |
| Implementation stages | CMake test registration when needed and corresponding documentation updates. |

A small shared map-identity header and a dedicated Remove integration test may
be justified. New generic systems or a session layer above all subsystems are
not required by the reviewed architecture.

## Completion and next action

Milestone 15 is complete. The production map 23 lifecycle confirmed object 13,
resource 111, all specified EVT metadata, logical line 0 restart, same-session
persistence across every approved cache lifecycle, and new-session restoration
without a specification discrepancy. The complete M14 regression sequence also
passed. Animated-object and other scene-specific selection remain outside the
approved coverage.

No Milestone 16 scope is defined or approved here. A later planning task must
establish the next milestone before implementation begins.

Follow [AGENTS.md](../AGENTS.md) for implementation validation and documentation
updates. Do not start subsequent stages without explicit authorization. Do not
mark a stage or milestone complete before its required validations pass; do not
push or create milestone tags without explicit approval.
