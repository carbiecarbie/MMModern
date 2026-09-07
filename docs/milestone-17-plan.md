# Milestone 17 - Party quest items and normal Phirna harvesting

**Status: planning specification; 17A and 17B are not implemented.**

**The milestone direction and architectural decisions are approved for planning.
Implementation requires a separate request for the relevant stage.**

Milestone 16 remains the stable implemented milestone. This document specifies
the next bounded increment; creating it does not start implementation or claim
new gameplay capabilities. The dependency order is **17A -> 17B**.

## Goal

Complete the normal Clouds Phirna interaction through the existing gameplay
entry point:

Space -> question -> Yes -> evaluate possession -> successful digging message
-> acknowledgment -> grant one Phirna Root -> Remove -> immediate disappearance.

Choosing No must finish without mutation. Already owning a root must select
the original refusal branch, finish after acknowledgment, grant nothing and
leave the plant in place.

The architectural goal is counted, party-owned Clouds quest-item state and the
minimum script behavior needed to connect M14 interaction, M15 mutations and
M16 visible objects. This is not a general inventory or quest-system milestone.
The existing original-data Remove-boundary test remains a focused regression;
the new harvesting acceptance must start at line 0.

## Verified baseline

Planning verification on 2026-09-07 read `AGENTS.md`,
[project-status.md](project-status.md), [the M16 plan](milestone-16-plan.md),
the relevant [M15 ownership/mutation policy](milestone-15-plan.md), current
implementation and tests, and the pinned reference described below.

The pre-plan repository was clean and had no M17 plan. Project status records
M16 complete, no implementation target approved, and 40/40 passing tests. The
preceding investigation reran those 40 tests using existing `build/16c`
binaries with outputs outside the repository. Those results are a baseline,
not a fresh M17 build or implementation validation. This documentation task
does not claim new build, CTest, SDL or visual acceptance results.

Current capabilities relevant to M17:

- manual Space dispatch begins at line 0 on the physical cell/current facing;
  it does not require the automatic-event bit;
- event decoding is separate from execution, with explicit unsupported and
  malformed-instruction diagnostics;
- conditions support actions 9, 20 and restricted Action 44; calls, returns,
  supported teleports and resumable text presentation already exist;
- `TakeOrGive` already executes the restricted mode-20 game-flag set/clear
  combinations; it is not an entirely unimplemented opcode;
- `XeenPartyState` contains roster/membership information but no quest-item
  counters; its resource loader reads the roster and party header;
- camera and game flags use working values, committed on successful completion;
- `XeenWorld` owns immediate disabled-object/event overlays, independently of
  disposable geometry, object, script and text caches;
- static outdoor rendering consumes effective object state;
- `XeenEventFlow` is shared by Application and integration tests; it refreshes
  before reporting results, including suspension/error, and before pending input;
- the visual detector compares committed camera and disabled-object count;
  presenter rebasing retains semantic layers, page and response state.

Current implementation and regression evidence, relative to the repository:

| Responsibility | Implementation | Existing tests to retain/extend |
|---|---|---|
| Party membership and loading | `src/games/xeen/XeenParty.*`, `XeenPartyLoader.*`; `src/formats/xeen/XeenCharacterFormat.*` | `tests/XeenCharacterFormatTests.cpp`, `PartyIntegrationTest.cpp` |
| Operand layout and omitted pairs | `src/games/xeen/XeenEventDecoder.*` | `tests/XeenEventDecoderTests.cpp`, `testTakeOrGive` |
| Conditions and flag operations | `src/games/xeen/XeenEventInterpreter.*` | `tests/XeenEventInterpreterTests.cpp`, flag operations, flow visibility, rollback and missing-target tests |
| Pending response/continuation | Same interpreter and `src/games/xeen/XeenEventPresenter.*` | `tests/XeenEventPresentationTests.cpp`, `testAction44`; `XeenEventUiTests.cpp` |
| Runtime event entry and commit | `src/games/xeen/XeenEventSystem.*`; `src/app/XeenNavigationFlow.*`, `XeenEventFlow.*`, `Application.cpp` | `tests/XeenEventSystemTests.cpp`, `XeenManualEventTests.cpp`, `XeenNavigationFlowTests.cpp` |
| Immediate Remove effects | `src/games/xeen/XeenWorld.*` and interpreter Remove dispatch | `tests/XeenRemoveTests.cpp`, `worldEffectsSurviveTransactionFailure` |
| Owner/cache separation | World/event cache discard and session overlays | `tests/XeenSessionPersistenceTests.cpp`, `sessionLifecycle`; `XeenSessionIdentityTests.cpp` |
| Visual Remove through runtime | `src/app/XeenEventFlow.*`, presenter rebase, `CloudsMapComposer` | `tests/XeenVisualRemoveTests.cpp`, `RemoveIntegrationTest.cpp` |

### Blocking points and corrections

The real Phirna Yes path currently fails first at line 3, condition action 21,
item 99, with `UnsupportedConditionAction`. It has dispatched three instructions
(lines 0, 1 and 3). The unsupported quest-item grant is at line 6, reached only
after adding the missing possession semantics and acknowledging the digging
message.

The M16 plan's statement that normal Space interaction reaches unimplemented
`TakeOrGive` is therefore imprecise. Its approved line-7 Remove checkpoint and
M16 completion remain valid. This document records the precise frontier;
historical validation is not evidence of complete harvesting.

The phrase "the possession condition succeeds" must mean evaluation is
supported, not that its equality comparison is true: **absent root means the
line-3 comparison is false**, falling through to harvesting. Possession makes
it true and branches to refusal. There is no post-grant possession check in
this event; grant visibility to later checks must also be tested synthetically.

The refusal branch has another current incompatibility: Action 44 at line 10
targets absent line 11. MMModern's explicit-target policy reports
`InvalidJumpTarget`; the pinned reference finishes there. M17 changes only the
narrow structural case specified below.

## Evidence and reference behavior

### Pinned dependency

Use the full revision from [dependencies.md](dependencies.md):

`6814ee9ba54582f5b5adcffab49efbbd8f589edd`

The local checkout `../scummvm-known-good-candidate` was verified at that HEAD
with empty status using command-local safe-directory and `core.autocrlf=false`
options. No dependency or global Git configuration was changed. The existing
`build/16c/CMakeCache.txt` names that source and
`../build-scummvm-6814ee9b-ucrt64` artifacts. Older example directory names in
AGENTS.md are not a substitute for this pinned dependency configuration.

Targeted reference locations, relative to that checkout:

| Reference | Required evidence |
|---|---|
| `engines/mm/xeen/scripts.cpp`, `Scripts::cmdTakeOrGive` | Three mode/value pairs, value widths, ordinary opcode dispatch into `Party::giveTake`. |
| Same file, `Scripts::cmdIf` and `Scripts::ifProc`, action 21 | Item-ID possession result passed to the opcode's existing comparison. |
| Same file, Action 44 and `Scripts::checkEvents` dispatch loop | Acknowledgment returns comparison value 1; missing matching event ends execution. |
| `engines/mm/xeen/party.cpp`, `Party::giveTake`, give case 21 | Non-Swords quest IDs start at 82; grant increments `_questItems[giveVal - 82]`. |
| Same file, `Party::synchronize` | Quest counters are serialized as individual bytes after the flag blocks. |
| Same file, `Party::giveTreasureToCharacter` | Treasure presentation is separate; quest items are not copied into ordinary character inventory. |
| `engines/mm/xeen/dialogs/dialogs_quests.cpp`, quest-items display | Indices below 35 are the Clouds prefix; index 17 has counted display semantics. |
| `devtools/create_mm/create_xeen/en_constants.h`, `QUEST_ITEM_NAMES` | Index 17 is Phirna Root. |
| `engines/mm/shared/xeen/file.cpp`, `File::syncBitFlags` | Packed flag-block widths used to locate the quest prefix. |

This milestone reproduces the supported script/state outcome. It does not
port ScummVM's complete command, character-selection loop, treasure staging,
presentation, or Party serializer.

### Original Phirna records

The installed World of Xeen resources were verified read-only through existing
MMModern archive/loading infrastructure. Reproducible event inspection:

```powershell
build/16c/mmodern.exe --inspect-events "F:\Games\gog\Might and Magic 4-5" 23 8 2 north
```

The resource is `maze0023.evt`, with 170 original records. Phirna uses records
125-135, lines 0-10, direction All, at `(8,2)`. The cell is not an automatic
trigger. Operand numbers below are decimal; opcodes are hexadecimal.

| Record | Offset | Line | Opcode / operands | Meaning |
|---:|---:|---:|---|---|
| 125 | 1056 | 0 | `0x01 Display0x01`, text 30 | Question about digging up the plant. |
| 126 | 1063 | 1 | `0x09 If2`, action 44, value 0, target 3 | Yes branches to the possession check. |
| 127 | 1072 | 2 | `0x12 Exit` | No completes unchanged. |
| 128 | 1078 | 3 | `0x09 If2`, action 21, value 99, target 9 | Owning a root selects refusal. |
| 129 | 1087 | 4 | `0x01 Display0x01`, text 31 | Successful digging message. |
| 130 | 1094 | 5 | `0x09 If2`, action 44, value 1, target 6 | Acknowledge before granting. |
| 131 | 1103 | 6 | `0x0C TakeOrGive`, `(0,0), (21,99)` | Grant one root; omitted third pair is neutral. |
| 132 | 1113 | 7 | `0x0E Remove`, no operands | Remove selected object and physical-cell events. |
| 133 | 1119 | 8 | `0x12 Exit` | Base instruction retained after effective removal. |
| 134 | 1125 | 9 | `0x29 DisplayBottom`, text 32 | Refusal because another root is owned. |
| 135 | 1132 | 10 | `0x09 If2`, action 44, value 1, target 11 | Acknowledge refusal; line 11 is absent. |

Text indices refer to `aaze0023.txt` in the outer Clouds archive. Do not embed
commercial text resources as synthetic fixtures. Synthetic strings can express
the same presentation requirements.

The real grant payload is `00 00 15 63`. The existing decoder correctly produces
neutral first pair, give mode 21/value 99, and default neutral third pair.
Mode 21's value is the item ID, not a quantity. Mode 0/value 0 takes nothing.

The selected MOB identity is Clouds/map 23/original object record 13, at `(8,2)`,
table index 8, resource 111, original direction North, sprite `111.0bj`.
Existing `RemoveIntegrationTest.cpp` asserts this relationship and the event
record offsets. Static metadata and rendering remain M16 responsibilities.

After Remove, execution restarts at logical line 0 and dispatches the eleven
effective `None` records. It does not normally fall through to the original
line-8 Exit. The successful line-0 path therefore dispatches 18 instructions:
lines 0, 1, 3, 4, 5, 6, 7, then eleven `None`. Later interaction dispatches
eleven `None` and cannot replay the grant.

### Initial party resource

`XeenGameFlagsFormat.h` already derives the Clouds flags offset as 659.
The pinned Party serialization then stores:

| Block | Size in bytes |
|---|---:|
| Clouds game flags | 32 |
| Darkside game flags | 32 |
| World flags | 16 |
| Quest flags, 60 packed bits | 8 |

The quest-item block therefore begins at **747 = 659 + 32 + 32 + 16 + 8**.
M17 reads only its first 35 bytes, `[747,782)`. Phirna index 17 is byte 764.
Skipping intervening fields is not implementation of their gameplay semantics.

Targeted inspection found an initial `maze.pty` of 812 bytes, with the supported
prefix zero, including Phirna. This is sufficient for M17. Requiring all 85
reference counters would incorrectly require at least 832 bytes. Do not infer
a full modern ScummVM save layout from the initial resource, zero-fill missing
required Clouds bytes, or parse the remaining tail as part of this milestone.

### Myra: reuse evidence only

At map 23 `(9,11)` West, `maze0023.evt` offset 182/line 0 is
`If2 (21,99,7)`. Possession selects line 7, an unsupported `NPC` instruction.
Later instructions take item 99, use mode 104 quest flags, and execute five
`GiveEnchanted` operations. This confirms that Phirna ownership has an actual
consumer beyond the plant's refusal check.

An optional boundary smoke may verify that owning the root reaches the real
line-7 NPC diagnostic without consuming or rewarding anything. Report this as
an expected unsupported boundary, not successful exchange completion. Myra's
dialogue, consumption, quest flags and rewards are explicitly excluded.

## Architectural decisions

### Ownership, representation and scope

Add the authoritative quest-item value to **`XeenPartyState`**. A small dedicated
value type owned as a member is acceptable; putting it in `XeenSessionWorldState`
or creating a monolithic session abstraction is not. `XeenParty` can retain its
existing membership responsibility. Roster entries and equipment are untouched.

Represent the supported domain as a fixed array of **35 nonnegative counters**,
default-initialized to zero, with bounded access. Use `std::uint32_t` counters
and checked increment: incrementing `UINT32_MAX` is an explicit execution error
with no change to that counter. Never wrap, silently saturate, or reduce the
model to booleans. Serialized input bytes widen without loss. In-memory counts
may exceed 255; no disk serialization is defined here.

A shared checked conversion accepts script item IDs **82 through 116 inclusive**
and returns `itemId - 82`, indices 0 through 34. Validate before subtraction or
indexing. Item ID 99 is index 17; sprite ID 111 is unrelated to this mapping.
IDs below 82 and above 116 are unsupported for the new operations, including
valid ordinary items or later-game quest items in the original engine.

The new script operations require Clouds logical and physical execution context
and a nonempty active party. Unsupported side/context and empty party must be
diagnosed before mutation; reuse the existing error categories where suitable.
This explicit supported-context boundary does not alter older operations'
synthetic side support or define behavior for ScummVM's invalid empty-party
character access. Do not introduce a selected-character requirement or UI.

State travels with the caller's party across map changes. Cache discard does not
reload it. A fresh session loads a new party state and constructs a new world,
event system and presentation flow, without reusing suspended execution.

### Initialization

Extend the existing `XeenPartyLoader::loadInitialCloudsParty` /
`loadFromResources` path to initialize the supported prefix from `maze.pty`.
Keep byte-layout work in the format/loading boundary, not in the interpreter.
Require the entire supported prefix before returning a loaded party state;
truncated input is an error, not an empty quest-item collection.

`XeenCharacterFormat::parsePartyHeader` remains a header parser with its existing
contract. Tests that use header-only bytes with the full Party loader will need
synthetic resource fixtures enlarged to include the quest prefix. Do not weaken
production truncation validation to retain those abbreviated fixtures. Retain
their existing roster, duplicate-member, diagnostic and portrait assertions.

### Condition action 21

Within the supported context/domain, compute the unsigned comparison value as:

| Counter | Actual comparison value |
|---|---|
| Zero | `UINT32_MAX` (`0xffffffff`) |
| Nonzero | Requested script item ID |

Use the existing comparison operation against the requested item ID. This
supports action 21 within the already decoded `0x08` (>=), `0x09` (==), and
`0x0A` (<=) conditions without introducing a condition framework. In particular:

| Ownership | >= | == | <= |
|---|---|---|---|
| Absent | true | false | false |
| Possessed | true | true | true |

The counter itself and a boolean 0/1 are not the reference comparison value.
Do not scan character inventories. Unsupported IDs must produce a condition
diagnostic containing the source, action and value, rather than look absent.
Reading ownership has no side effects and does not require mutable party access.

### Quest-item grant

17B adds exactly this family to ordinary opcode `0x0C`:

- first pair `(mode 0, value 0)`;
- second pair `(mode 21, value in 82..116)`;
- third pair `(mode 0, value 0)`, explicit or omitted.

Each executed supported instruction adds **one** to the selected party counter,
once for the party, independent of its member count. Multiple grant instructions
may increment the same counter; a grant is not globally idempotent. Phirna's
script and effective Remove state prevent duplicate harvesting.

Validate the combination, ID, execution context, counter capacity and sequential
continuation bounds before applying this new mutation. A grant at line 255
cannot advance and must fail without adding an item. This prevalidation applies
to the new operation; preserve existing mode-20 flag behavior. An instruction
limit reached before dispatch must likewise not grant.

Reject take-mode-21 requests, nonneutral first/third pairs, ordinary item IDs,
unsupported grant modes and other TakeOrGive opcode variants. Preserve existing
supported mode-20 set/clear operations. The decoder continues to preserve valid
operands and reject malformed layout; semantic subset checks belong to execution.

### Immediate mutation and execution access

A successfully dispatched grant updates the caller's `XeenPartyState`
immediately. It is visible to subsequent conditions, calls and resumed execution.
It is not a pending delta or part of camera/flag commit state.

| Sequence | Required result after later error |
|---|---|
| Grant, then unsupported instruction | Item remains owned. |
| Grant, then failing Remove validation | Item remains; failing Remove does not apply its mutation. |
| Grant, Remove, then error | Item and removal remain; camera/game flags retain existing rollback. |
| Invalid grant itself | No item added; earlier successful mutations remain. |

Thus a grant followed by failure before Remove may leave a visible plant and
an owned root. A later ordinary interaction can refuse harvesting. This is the
approved immediate-mutation policy, not a reason to add compensation, retries,
replay journals or generalized transactions. Camera/flag transactions are an
MMModern policy; do not attribute them to ScummVM.

In 17B, pass mutable party access only along paths that can execute events:
interpreter begin/resume/execute, event-system entry/resume, navigation-flow
wrappers that dispatch events, and `XeenEventFlow`'s reference to the caller's
party. Application's interactive party instance must permit these writes.
Keep renderer/composer, character rules, diagnostics and observational helpers
const where practical. No `const_cast`, hidden mutable counters, whole-party
rollback copies, or shared ownership redesign is required.

Suspended execution holds its existing continuation values; the owning party
must outlive it. Do not copy authoritative counters into a suspended state or
overwrite the party with a stale pre-suspension snapshot. Continue after an
already executed grant when presentation resumes. The existing protocol assumes
the caller advances the current continuation; M17 does not add protection for
deliberately replaying an obsolete copied continuation from outside that flow.

### Narrow terminal acknowledgment rule

The compatibility exception applies only when all these conditions hold:

1. Execution is resuming a pending conditional Action 44 with value 1 and the
   acknowledgment response requirement.
2. The supplied response is valid and the existing comparison selects its target.
3. The target is the immediate numeric successor of the acknowledgment's
   logical line, without byte wraparound, in the same logical map/cell/direction.
4. Normal first-match lookup finds no matching successor instruction.

Then complete execution normally, using existing natural-completion semantics
and working camera/flags. Absence adds no dispatched instruction. If the
successor exists, execute it normally: an effective `None`, malformed record,
unsupported opcode, or instruction-limit error is not an absent instruction.

This rule does not apply to Yes/No (Action 44 value 0), another condition action,
a nonadjacent target or a missing call target. Existing ordinary fallthrough
completion and line-overflow behavior remain unchanged. At line 254, target 255
may qualify; at line 255, target 0 is not a successor and keeps ordinary branch
semantics. Never wrap 255 to zero for this exception.

Lookup uses logical execution coordinates, not the rendering or physical camera.
If this terminal case occurs inside a call, normal missing-line completion ends
the execution, as the existing natural-completion policy does; it must not invent
a Return or execute a caller continuation. Valid successor instructions can still
execute an explicit Return. Cache discard during acknowledgment changes none of
these rules.

Do not special-case Phirna, map 23, text IDs, record indices or offsets. Do not
convert ScummVM's broader missing-line behavior into a blanket MMModern rule.

### Existing Remove and visual flow

Reuse `XeenWorld::applyRemove`, effective-event lookup, stable selection and
`XeenEventFlow` unchanged in their semantics. Remove uses the working physical
cell, disables selected object plus all physical-cell events, and restarts at
logical line zero while preserving the call stack. Disabled records keep base
metadata/operands and original precedence, with effective opcode `None`.

No quest-item UI is added, so a grant alone requires no visual invalidation
signal. Plant disappearance continues to be detected through disabled-object
count. Composition uses committed camera and effective world state, including
on suspension/error. Preserve presenter rebasing and existing manual-error versus
automatic-error propagation. Do not broaden the recomposition architecture.

## Milestone 17A - Quest-item state, possession condition and refusal completion

**Status: not implemented.**

### Objective and responsibilities

Introduce the bounded party-owned value and real resource initialization,
implement condition action 21 for that value, and support the terminal
acknowledgment rule. Establish correct decision behavior without granting items.

### Likely files/components

- `src/games/xeen/XeenParty.h/.cpp`: owned counter value/query boundary; a small
  quest-item-specific value file is acceptable if useful.
- `src/games/xeen/XeenPartyLoader.h/.cpp` and a narrow format component under
  `src/formats/xeen/`: prefix parsing and initialization.
- `src/games/xeen/XeenEventInterpreter.h/.cpp`: possession evaluation,
  diagnostics and acknowledgment continuation policy.
- Existing decoder, interpreter, presentation and character/party-format tests;
  focused quest-item tests and real-data integration support as necessary.
- `CMakeLists.txt` only to register required tests/smokes; stage documentation.

No decoder redesign, world-state change or mutable runtime party propagation is
needed merely to query counters in 17A. Controlled test initialization is not
an implemented grant operation.

### Synthetic tests

1. Default zero value; all 35 indices; item IDs 82, 99 and 116 map to 0, 17 and
   34; reject 81, 117 and invalid wide/signed inputs at applicable boundaries.
2. Prefix parsing at exactly 782 bytes, nonzero counts including 255, unrelated
   leading/trailing bytes, missing/truncated prefix, and no input mutation.
   Preserve roster/member behavior while enlarging full-loader fixtures.
3. Action 21 absent/possessed with >=, == and <=; counts 1 and greater; unrelated
   counters untouched; unsupported ID/context and empty-party diagnostics.
4. Conditions inside calls and after map changes use the same party value;
   synthetic side rejection for the new operations does not regress older ones.
5. Acknowledgment to missing adjacent successor completes; existing adjacent
   successor runs; existing effective None is not mistaken for absence.
6. Missing nonadjacent acknowledgment target, missing Yes/No target, missing
   ordinary conditional target and missing call target retain errors.
7. Invalid response, line 254/255 boundaries, different lookup direction,
   logical/physical location separation, call-stack behavior, instruction count
   and budget, cache discard during suspension, and malformed/unsupported present
   successor all retain the specified semantics.
8. M14 Action-44/flag rollback regressions and M15/M16 Remove, presentation and
   session lifecycle tests continue to pass.

### Real-data checkpoint

Use production resource providers and shared `XeenEventFlow`, starting at line 0:

- initial party loads zero Phirna ownership and the complete supported prefix;
- No completes through lines 0, 1, 2 with no state mutation;
- Yes with no root evaluates line 3 as false, presents the digging message,
  acknowledges line 5, then reports the still-unsupported grant at line 6;
  item count stays zero and the plant remains;
- a controlled party initialized with one root follows lines 0, 1, 3, 9, 10
  and completes at absent line 11, without grant or Remove.

The expected unsupported grant in 17A is an asserted stage boundary, not a
failing test or completed harvest. With that boundary, the successful-message
text precedes the diagnostic because that is the original script order.
Do not alter the text, skip acknowledgment, or execute the grant in this stage.

### Exit criteria and exclusions

Fresh pinned-dependency build, focused tests, full CTest and the three original
decision paths pass their specified expectations. Inspect question/refusal
presentation and verify input recovery. Record stage scope and validation in
this plan and project status; leave 17B unimplemented until separately requested.

Exclude grant execution, consumption, treasure, inventory, quest UI, Myra
completion, Remove redesign, and all milestone-wide non-goals below.

## Milestone 17B - Quest-item grant and complete Phirna harvesting

**Status: not implemented; depends on completed 17A.**

### Objective and responsibilities

Execute the supported grant once per instruction, provide mutable party access
through the existing runtime execution path, and complete normal harvesting
through existing Remove and visual composition. Prove persistence and immediate
mutation behavior across suspension, later errors and cache reconstruction.

### Likely files/components

- `XeenPartyState`'s quest-item value: checked increment operation.
- `src/games/xeen/XeenEventInterpreter.h/.cpp`: grant validation/execution and
  mutable access for begin/resume/execute.
- `src/games/xeen/XeenEventSystem.h/.cpp`,
  `src/app/XeenNavigationFlow.h/.cpp`, `src/app/XeenEventFlow.h/.cpp`, and
  interactive `src/app/Application.cpp`: necessary reference/caller changes.
- Decoder/interpreter/presentation/event-system tests and shared visual/Remove
  lifecycle support; retain the focused original Remove smoke and add/extend
  a normal-harvesting smoke through the same runtime flow.
- CMake registration as needed; this plan, project status and README for actual
  completion/capability reporting. Dependency documentation only if necessary.

World/session overlays, renderer algorithms and visual invalidation need no new
behavior. Mechanical fixture/call-site updates must preserve their old assertions.

### Synthetic tests

1. Exact real grant payload and equivalent explicit third neutral pair decode
   correctly. Partial pairs/trailing garbage remain malformed; nonneutral pairs,
   take requests, unsupported IDs/modes/variants remain unsupported.
2. Grant from 0 to 1, repeated instructions to 2, boundary IDs and unrelated
   counters. Party size 1 versus 6 still grants once. Test increment above 255,
   maximum-minus-one to maximum, and maximum overflow with unchanged state.
3. Validate invalid combination/context/ID, empty party, line-255 continuation
   and pre-dispatch instruction-limit failure before mutation. Retain diagnostics
   and instruction accounting, plus earlier successful grants when a later one
   fails validation.
4. A later action-21 condition sees the grant, including calls/returns and
   presentation suspension. No authoritative counter snapshot is restored.
5. Grant before a multi-page presentation, repeated pending input, acknowledgment,
   Yes/No and subsequent continuation executes only once. Test cache discard
   while suspended and preserve the current page and gameplay blocking.
6. Grant then error, grant then failing Remove, and grant then successful Remove
   then error satisfy the mutation table. Include camera/flag working changes
   before the fault and verify their existing rollback independently.
7. Shared-flow manual and synthetic automatic paths, immediate and resumed
   results: retained diagnostics, automatic exception propagation, committed
   rendering camera and immediate visible Remove.
8. Same-session map/cache reconstruction preserves counters and removal; a new
   party/world/event/presenter graph restores initial state. Verify two independent
   party states do not share counters and same-sprite objects remain distinct.

### Real-data checkpoint

Ordinary Space/line-0 execution with the original records must now finish the
Yes/no-root path: root count 0 -> 1 and selected object removal, without movement
or a manual composer call to trigger disappearance. No and refusal still finish
unchanged. Retain the original line-7 Remove checkpoint as an independent test.

### Exit criteria and exclusions

All milestone acceptance cases below pass with a fresh build and full CTest,
required original-data smokes, SDL/runtime validation and native-frame inspection.
Update this plan, project status and README to the actual implemented scope.
Only then mark 17B and M17 complete. Do not start another milestone automatically.

Exclude taking/consumption, general item behavior, treasure staging/presentation,
Myra rewards, and all milestone-wide non-goals. No third stage is planned.

## Acceptance strategy and requirement-to-test mapping

Use deterministic synthetic scripts and byte buffers for automated semantics.
Use external original resources for integration evidence. Prefer explicit state,
command, pixel and diagnostic assertions over framebuffer hashes alone.

| ID | Required acceptance | Evidence |
|---|---|---|
| A01 | Fresh session starts with root count zero. | Initial prefix parser/loader assertion and real party load. |
| A02 | Phirna is visible and selected as original object 13/resource 111. | Production object selection, outdoor draw command and native frame. |
| A03 | Space -> No completes unchanged. | Shared flow, three dispatched instructions, unchanged counters/world/camera/flags. |
| A04 | Space -> Yes follows original question, condition, success message and acknowledgment. | Original line-0 dispatch and source-address/presentation assertions; no skipped lines. |
| A05 | Grant increments ownership exactly once. | Count 0 before acknowledgment, 1 after completion; synthetic multiple-member/repeated-instruction tests. |
| A06 | Plant disappears immediately. | Frame returned by the shared flow before movement; missing plant draw command and equality to effective composition after accounting for text layers. |
| A07 | Removed-cell interaction cannot grant again. | Count stays 1; eleven effective None dispatches; no success/refusal replay. |
| A08 | Reconstruction preserves both ownership and removal. | Leave/return, independent map/object, script, text and sprite cache discard, then combined discard; provider counters prove actual reloads. |
| A09 | Fresh session restores initial state. | New party/world/events/presentation graph: zero root, visible plant, base effective opcodes restored; no old continuation. |
| A10 | Already-owned fixture refuses correctly. | Original lines 0,1,3,9,10; terminal completion; same count; plant and events remain enabled. |
| A11 | Unrelated state is unchanged. | All other quest counters, roster/member fields, HP/SP/equipment data, flags, camera and unrelated object/event identities compared. |
| A12 | Later errors do not undo completed grants. | Synthetic grant/error and grant/Remove/error sequences; independent camera/flag rollback assertions. |
| A13 | Presentation cannot replay a completed grant. | Suspensions before/after grant, multi-page input, resume and cache reconstruction through production flow. |

For A06, the old focused scene-only Remove smoke records 541 changed scene
pixels. Do not demand that raw count from a new frame containing retained success
text. Compare equivalent presentation states or separately assert effective scene
and overlays. A direct reference composition is an oracle, not the operation
that updates the tested runtime frame.

For A10, prepare owned state through controlled fixture initialization, preferably
synthetic party-resource bytes. Original EVT/MOB/text resources remain unchanged.
This fixture is permitted in tests only; no runtime grant key, public cheat,
hardcoded initial root or alternate event entry is part of acceptance.

The focused suite must retain existing decoder, interpreter, presentation,
event-system/manual/navigation, party-format/rules, Remove, session persistence,
visual Remove and outdoor/indoor rendering regressions. Run the smallest relevant
set while iterating, and the complete CTest suite at each stage's exit. Expected
unsupported boundaries belong in passing assertion tests, not ignored failures.

## Manual / SDL validation

Use the existing SDL event loop and response mappings; do not add nested loops.
The scripted smoke may establish the camera at map 23 `(8,2)` North, but the
interaction itself must use ordinary Space/line-0 flow.

- Inspect the original question and Yes/No choice at native 320x200 resolution.
- Choose No; verify no removal/grant and resumed navigation.
- Choose Yes without a root; inspect the successful digging message, verify
  ownership stays zero while its acknowledgment is pending, then acknowledge
  using the existing Space/Enter mapping.
- In 17B, verify immediate disappearance with no movement and count exactly one;
  retained text must sit over the effective scene without resurrecting the plant.
- In an already-owned controlled fixture, inspect the refusal message, acknowledge
  it, and verify normal completion, visible plant and unchanged count.
- Exercise navigation attempts while choice/acknowledgment is pending, repeat-key
  suppression, and restored input after completion or the expected 17A diagnostic.
- Move away/return and cross the map/cache lifecycle boundaries; inspect continued
  absence after harvesting and visibility in a genuinely new session.
- Repeat Air / Corner, Snake Oil and Castle Basenji No/Yes presentation controls,
  along with the focused M15/M16 Remove lifecycle validation.
- Verify clean Escape/SDL quit paths. SDL dummy/software plus native-frame
  inspection is valid automated/visual evidence but must not be described as
  physical-display hardware validation.

Keep logs and generated frames in local ignored validation directories. Never
commit original resource bytes, extracted assets or commercial-data images.

## Risks and invariants

| Risk | Required constraint |
|---|---|
| Terminal acknowledgment exception becomes broad error suppression | Qualify the structural continuation before allowing missing-next-line completion; test nonadjacent jumps, Yes/No, calls and present-but-invalid successors. |
| Mutable party access spreads into rendering or snapshots | Change only event-capable paths; retain const observers and one caller-owned party value. |
| Quest counters grow into generic inventory | Fixed Clouds prefix and one grant family; no category slots, equipment, item database or treasure queue. |
| Resumption grants twice | Preserve the post-grant continuation and use the same runtime/test flow; test multiple presentation suspensions and input repetitions. |
| Bounds, subtraction or overflow corrupt state | Shared checked ID conversion, complete prefix validation and checked uint32 increment before mutation. |
| Byte parsing becomes a save-format commitment | Read-only initial Clouds prefix; no serializer or round-trip compatibility claim. |
| Later errors lose grants or imply all-or-nothing execution | Immediate party effects explicitly persist; camera/flags alone retain their existing transaction; no compensation framework. |
| Cache reconstruction resets party state | Reload only disposable resources; prove actual reload and stable party ownership independently. |
| Smoke passes through an artificial shortcut | Original records, normal line-0 dispatch, shared flow; controlled ownership only as test fixture setup. |

Preserve these M14-M16 invariants:

- decoding/execution separation and strict malformed-input diagnostics;
- stable side/map/original-record identities, first-match ordering, selection
  independent of drawing, and retained effective None records;
- physical working camera versus logical script address separation;
- existing instruction limits, call-stack rules and response validation except
  the explicitly specified terminal acknowledgment compatibility case;
- immediate world mutations and transactional camera/game flags;
- `XeenEventFlow` shared by Application and tests, with refresh before result
  delivery and pending input, committed camera rendering and presenter rebasing;
- manual errors retain usability; automatic errors retain existing propagation,
  without a promise of an additional SDL frame after a fatal exception;
- party ownership independent of map/script/text/sprite caches;
- no broad ECS, mutation history, revision-counter or invalidation framework;
- deterministic synthetic tests and external, read-only original game access.

If implementation exposes a concrete conflict with these requirements, identify
it against the specific specification, code and test before expanding scope.
File organization, diagnostic wording and test target names remain implementation
choices. Domain, ownership, mutation policy, continuation behavior and acceptance
outcomes are requirements, not open architectural questions.

## Explicit non-goals

- general character inventory, weapons/armor/accessory management or equipment UI;
- generic item ownership abstractions or generic TakeOrGive completeness;
- quest-item taking/consumption and TakeOrGive variants beyond the approved grant;
- treasure staging, distribution, acquisition animation/sound or reward UI;
- Myra exchange completion, GiveEnchanted and enchanted-item rewards;
- NPC dialogue/systems, shops, services or economy;
- combat, monsters, AI, doors, locks and traps;
- outdoor animation, indoor objects and alternate quest-dependent appearances;
- quest flags beyond existing support, journal or generic quest framework;
- disk save/load, original-save compatibility or full ScummVM Party serialization;
- Darkside, Swords, cross-side gameplay or their quest domains;
- generic ECS, invalidation, transaction, rollback or replay frameworks;
- public smoke-test shortcuts, altered event records or commercial asset copies.

## Definition of Milestone 17 complete

Both stages are implemented and reviewed, with successful pinned-dependency
builds, all focused tests and full CTest passing, required original-data smokes,
SDL validation and native-frame inspection complete, and documentation updated
to the actual behavior. The normal original Phirna interaction must satisfy
A01-A13 without bypassing any required instruction.

Only complete harvesting and the bounded quest-item semantics may be claimed.
Myra and generic inventory remain unsupported. No milestone tag, commit, push
or branch operation is authorized by this planning document; follow explicit
subsequent user instructions and the repository Git workflow.

## Next implementation task

When implementation is separately requested, implement **17A only**. Do not
implement the grant or begin 17B while completing 17A. After 17A's specified
validation, report its expected line-6 boundary and wait for the next stage request.
