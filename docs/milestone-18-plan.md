# Milestone 18 - WhoWill and Bone Whistle collection

**Status: approved/planned. Implementation has not started.**

**18A is the next implementation target, not implemented or complete.
18B is planned, not implemented or complete. Milestone 17 remains the latest
stable milestone.**

This document records the approved post-M17 investigation and specification.
Approval covers the plan; it does not authorize implementation during this
documentation task. Acceptance cases below are requirements, not passed results.
No build, CTest or M18 acceptance run was performed to create this document.

## 1. Objective and scope

Add opcode `0x20 WhoWill` through the existing resumable event architecture,
with a temporary script-selected character consumed by already-supported
condition Action 9. Complete the original local Bone Whistle interaction on
Clouds map 20 at `(5,14)`, facing North:

WhoWill -> valid character choice -> original DisplayBottom -> original
acknowledgment -> grant exactly one quest item 100 -> original Remove -> completion.

Cancellation must stop that event before its later presentation, grant or Remove.
After successful collection, repeat interaction must not grant another item.

This checkpoint establishes local collection only. It does not establish
Orothin's quest completion, combat correctness, monster support or correctness
of normal traversal to this location.

Throughout this specification, **Verified** describes inspected code/data;
**MMModern decision** describes the approved implementation design, including
defensive validation where the reference has no safe behavior.

## 2. Verified baseline and authority

The investigation read `AGENTS.md`, [project status](project-status.md),
[dependencies](dependencies.md), [M17](milestone-17-plan.md) and the
[README](../README.md), then inspected the relevant implementation/tests.

- Investigation branch: `main`, initially clean.
- Investigation HEAD: `8d62700274f250448489bd90b99fc5827e15dce1`.
- Completed M17 commit: `b59f39e975173444e3324787209d4ad20b5a0b70`.
- The intervening change was only the M17 stabilization documentation correction.
- M17 stages 17A/17B and acceptance cases A01-A13 were approved as complete.
- **42/42 is the recorded M17 CTest baseline, not a new investigation result.**
- Recorded M17 SDL evidence used dummy/software rendering and native-frame
  inspection, not physical-display hardware validation.

Reference authority is ScummVM commit
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, not current master. Its local HEAD
and clean content status were verified. The current CMakeCache uses:

- source: `D:/Projetos/MModern/scummvm-known-good-candidate`;
- dependency build: `D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`.

`AGENTS.md` mentions older local directory names (`../scummvm-master` and
`../build-xeen-probe-sdl`). Those names are not a revision authority: the explicit
pin in dependencies/M17 and the actual local configuration govern this work.
No dependency relocation, ScummVM change or broad reference reinvestigation is
part of M18.

Original resources were inspected read-only at
`F:/Games/gog/Might and Magic 4-5`, using existing diagnostics and production
loaders observed through GDB. No full Clouds opcode census was repeated.

## 3. Verified WhoWill semantics

All ScummVM paths/symbols in this section refer to the pinned revision above.

### Operands and text

**Verified:** `engines/mm/xeen/scripts.cpp`, `Scripts::cmdWhoWill`, reads two
bytes and calls `WhoWill::show(vm, message, action, true)`:

| Byte | Meaning |
|---|---|
| 0: `message` | Verb index in `Res.WHO_ACTIONS[32]` |
| 1: `action` | Index in the map's event text, because `type` is true |

The verb table domain is `0..31`. The event-text operand has a byte-sized index;
whether it exists depends on that map's text resource. `WHO_WILL_ACTIONS[4]`
belongs to the alternative dialog callers and is not the text source for opcode
0x20. Neither operand is a selected-character index.

`devtools/create_mm/create_xeen/en_constants.h`, `WHO_WILL` and `WHO_ACTIONS`,
provide the prompt template and verbs. Verb 0 is `search`; the complete table is:

| Index | Verb | Index | Verb |
|---|---|---|---|
| 0 | search | 16 | destroy |
| 1 | open | 17 | pull |
| 2 | drink | 18 | descend |
| 3 | mine | 19 | toss a coin |
| 4 | touch | 20 | pray |
| 5 | read | 21 | join |
| 6 | learn | 22 | act |
| 7 | take | 23 | play |
| 8 | bang | 24 | push |
| 9 | steal | 25 | rub |
| 10 | bribe | 26 | pick |
| 11 | pay | 27 | eat |
| 12 | sit | 28 | sign |
| 13 | try | 29 | close |
| 14 | turn | 30 | look |
| 15 | bathe | 31 | try |

**MMModern decision:** the decoder accepts exactly two parameter bytes.
Zero, one or extra bytes produce `MalformedInstruction` with source metadata.
Do not copy `EventParameters::Iterator`'s permissive out-of-range reads from the
reference or weaken existing decoder validation.

For 2-6 members, validate the verb index and resolve event text through the
existing text provider for the executing map. Preserve distinct missing-resource,
map-mismatch and invalid-index errors; an existing empty string is not a missing
entry. For the one-member fast path, preserve the reference's early return before
text lookup: unused text operands are not dereferenced. Structural two-byte
validation still applies. No generic resource/localization subsystem is required;
the small pinned prompt/verb constants fit the existing presenter.

### Party size, choice and cancellation

**Verified:** `engines/mm/xeen/dialogs/dialogs_whowill.cpp`, `WhoWill::execute`:

| Situation | Reference behavior |
|---|---|
| Empty party | `size() <= 1` returns 1 despite there being no such member |
| Exactly one member | Returns 1 automatically, before UI/text/eligibility |
| 2-6 members | Presents choice and waits |
| F1-F6, within active size | Uses corresponding active member |
| F-key above active size | Ignores it and keeps waiting |
| Ineligible member | Displays refusal, then keeps waiting |
| All members ineligible, size >= 2 | Keeps waiting; Escape remains available |
| Escape in choice dialog | Returns 0 |

The returned selection is 1-based. `Scripts::cmdWhoWill` assigns it to
`_charIndex`; zero invokes `cmdExit`, which sets `SCRIPT_ABORT`. Cancellation
terminates the entire current event, including the caller if inside CallEvent;
it is not an implicit Return. It does not undo earlier instructions. The
reference still performs its ordinary post-event cleanup/processing; M18 does
not add treasure distribution, death handling or other surrounding systems.

**MMModern decision:** WhoWill with an empty party returns `EmptyParty`, without
presentation or invalid roster access. This is explicit defensive behavior, not
a claim that the reference has a safe empty-party selection. Do not globally
reject otherwise supported events merely because the party is empty.

### Eligibility

**Verified:** `engines/mm/xeen/character.cpp`, `Character::noActions`, checks the
single result of `worstCondition()`, which searches nonzero serialized conditions
from highest index downward. It refuses only these worst conditions:

| Condition | Serialized index |
|---|---:|
| Asleep | 8 |
| Paralyzed | 11 |
| Unconscious | 12 |
| Dead | 13 |
| Stoned | 14 |
| Eradicated | 15 |

Testing whether any of those six flags is set would be incorrect. For example,
Asleep plus Confused resolves to Confused and is not refused by `noActions()`.
There is no extra HP, SP, class, name or portrait eligibility test in this method.
The one-member fast path bypasses this check even for an incapacitated member.

The reference refusal uses `Res.IN_NO_CONDITION` and `ErrorScroll::show` in
`engines/mm/xeen/dialogs/dialogs_message.cpp`, followed by a retry of WhoWill.

**MMModern decision:** add a small pure character helper using existing
`XeenCharacter::worstCondition()` and the existing 16 serialized condition bytes.
Keep presentation side effects out of the helper. No loader/format changes or
general status-effect framework are needed. Missing portraits or empty names
must not silently become new eligibility rules.

### Lifetime and consumers

**Verified:** in `engines/mm/xeen/scripts.cpp`:

- `Scripts::checkEvents` begins script character context with `_charIndex = 1`.
- A successful WhoWill changes the context used by subsequent instructions.
- `cmdCallEvent` saves address/line, and `cmdReturn` restores them; neither saves
  or restores character selection. A choice made inside the call survives Return.
- A new independent event begins with the first member again.
- `cmdTeleport` for TeleportAndContinue sets `SCRIPT_RESET`; the resulting script
  restart also resets `_charIndex` to 1.
- `cmdIf` passes `_charIndex - 1` to `ifProc` in the ordinary selected-member
  case. Action 9 reads that member's current SP.
- Other mechanisms use special values such as 0/8 with different semantics.
  WhoWill does not justify implementing generic SetChar semantics.

The separate `_whoWill` field is reset at event entry and is also used by
`engines/mm/xeen/party.cpp` for treasure-distribution preference. It is not the
same context as `_charIndex`; reproducing that unsupported inventory consumer
is not required for M18's quest-item counter grant.

## 4. Current MMModern architecture and gap

| Existing subsystem | Verified baseline | Required addition |
|---|---|---|
| XeenEventDecoder | Strict typed operations; no WhoWill operation | Two-byte WhoWill operation |
| XeenEventInterpreter | Value-owned resumable execution | Selected member and WhoWill continuation |
| XeenEventSystem | Dispatch and camera/flag commit | Carry extended response without policy changes |
| XeenEventPresenter | Presented, acknowledgment, Yes/No, pagination | Typed character selection presentation |
| XeenEventFlow | Owns pending execution; blocks gameplay | Selection consumption and stale-response protection |
| SDL input | Navigation, Space/Enter, Y/N; Escape exits | F1-F6 and contextual Escape |
| XeenCharacter | Loaded conditions and worstCondition | Small pure eligibility helper |
| XeenParty | Active index mapped to roster ID | Reuse mapping, no persistent selection |

Concrete sources are `src/games/xeen/XeenEventDecoder.{h,cpp}`,
`XeenEventInterpreter.{h,cpp}`, `XeenEventSystem.{h,cpp}`,
`XeenEventPresenter.{h,cpp}`, `src/app/XeenEventFlow.{h,cpp}` and
`src/platform/sdl/SdlWindow.cpp`.

`XeenEventInterpreter::run` currently implements Action 9 using
`party.member(roster, 0).currentSp`. It must use the execution context instead.
`XeenParty::member` in `src/games/xeen/XeenParty.cpp` already resolves an active
index through `activeRosterIds`. Original initial roster IDs were verified as
`0,18,14,11,1,6`, demonstrating why these indices cannot be interchanged.
`src/formats/xeen/XeenCharacterFormat.cpp` already loads all condition bytes.

## 5. Execution-state ownership and mutation policy

**MMModern decision:** keep an optional 0-based selected active-member index in
`XeenEventExecutionState`, not in party state or solely in presentation state.

| Candidate owner | Decision |
|---|---|
| XeenPartyState | Reject: makes temporary script context appear persistent |
| XeenEventExecutionState | Use: travels with suspension and calls |
| Presenter alone | Reject: Action 9 and non-SDL execution need the context |

The indexing boundary is singular: F1 maps to active index 0, F6 to index 5.
Resolve roster IDs only through the party abstraction. Cancellation is a distinct
response type, never index 0, -1 or another numeric sentinel.

- `begin`: initialize to index 0 if a member exists, otherwise no member.
- Valid WhoWill: replace the current index.
- Display/pagination/acknowledgment suspension and resume: preserve it.
- Call/Return: preserve the current context, including changes in a called event.
- New event or successful TeleportAndContinue script restart: reset to first member.
- Event completion/error: context ends with execution; it cannot leak to a later event.

The selected member accompanies existing logical address, physical working
camera, working flags, call stack, pending presentation, instruction count and
selected world object. Character and world-object selection are independent.
Do not store character selection in call frames or change the existing object
identity/reset rules. Teleport's character reset must not reset the instruction
budget or alter existing call/teleport restrictions.

Preserve M14-M17 policies in `XeenEventSystem` and `XeenWorld::applyRemove`:

- working camera/game flags commit on completion, not suspension or error;
- party grants and world Remove are immediate and survive later errors;
- canceling WhoWill completes like Exit, without executing following instructions;
- cancellation does not introduce a rollback of earlier mutations;
- budgets, line overflow, target validation and prevalidation remain enforced.

Thus "no side effects on cancellation/error" means no effects of the rejected
choice or subsequent instructions, not erasing earlier approved mutations.
Bone Whistle starts with WhoWill, so cancellation there leaves item, object,
events, camera and flags unchanged.

## 6. Presentation and input design

**MMModern decision:** extend the current typed protocol with a character-selection
presentation kind and response requirement, and a payload carrying either a
selected active index or explicit cancellation. Suggested names are
`CharacterSelection`, `SelectedCharacter{partyIndex}` and
`CharacterSelectionCancelled`; names are not an ABI requirement.
Do not encode choices through Yes/No or acknowledgment values.

The request carries resolved prompt text and minimal member metadata: active
index, roster identity, name and eligibility. It owns values, not cache pointers
or a second authoritative party. Execution revalidates the response against the
live party before using it.

| Input/response | Required handling |
|---|---|
| Valid choice | Consume pendency, update context, continue exactly once |
| F-key outside active range | Ignore in UI; remain pending |
| Ineligible member | Show refusal with name; remain pending for retry |
| Cancellation | Remove transient presentation; complete event as Exit |
| Wrong response type | InvalidPresentationResponse; no later instruction |
| Invalid index submitted directly to API | Response error, no invalid roster access |
| Party identity/range changed while pending | Revalidate; never silently retarget choice |
| Stale or repeated response | Reject at active continuation owner; no second resume |

`XeenEventFlow` must consume its pending continuation before calling resume and
associate responses with a presentation generation. Rebase preserves that
generation and produces no response. Completion, cancellation or replacement
invalidates the old generation. The interpreter separately checks response type
and payload. Existing execution states are copyable values: do not claim global
replay detection if a caller deliberately executes a copied historical state.
Exactly-once consumption must be demonstrated at the production boundary that
owns the active execution, not merely by matching payloads in isolated snapshots.

### Minimum player-facing UI

**Verified:** the reference uses window 36 at `(225,74)-(320,154)`, with title,
question and F1-Fn. For the original six-member Bone Whistle checkpoint:

```text
Bones

Who will
search?

F1 - F6
```

`src/games/xeen/CloudsUiComposer.cpp` already renders the party portraits in active
order. The reference dialog has no separate member-name list. Reuse existing
portraits/order, show F1-Fn and use the loaded member name in refusal feedback.
No new portrait resources or general party menu are required.

**MMModern presentation adaptation:** show refusal within the existing WhoWill
presentation flow/panel, retaining the pending choice. Do not reproduce the
reference ErrorScroll's nested wait loop. Ensure readable feedback and Escape
cancellation; this is a deliberate presentation adaptation, not a claim of
pixel-identical modal-error behavior. Verify clipping/wrapping with the existing
font, including the complete verb table, without a broad UI redesign.

### SDL routing

- Add typed select-member and cancel-interaction actions to PlayerAction.
- Map F1-F6 to indices 0-5; Enter/Space/Y/N do not choose a member.
- While WhoWill is pending, navigation and ordinary interaction cannot dispatch
  gameplay. Preserve blocking through rejection feedback and resume afterward.
- Escape cancels pending WhoWill; outside it, Escape retains application exit.
  Use an explicit cancellation capability from the flow, not blocksGameplay
  alone, because acknowledgments and Yes/No also block gameplay.
- Ignore selection key repeats. Repeated Escape from the same held press after
  canceling must not exit the application; a fresh Escape outside WhoWill exits.
- The selecting F-key must not acknowledge a following display or confirmation.
- SDL_QUIT retains its current behavior. No nested loop, general mouse handling,
  remapping or second UI execution system is added.

## 7. Action 9 integration

**MMModern decision:** change only the existing current-SP condition consumer to
read the selected active member. Preserve supported comparison variants, numeric
conversion and empty-party validation. Before WhoWill it still reads the first
member, matching reference event initialization.

Tests must distinguish members' SP and nonsequential roster IDs, and exercise
the selected member through presentation and Call/Return. After a destination
script restart, Action 9 must again read the first member.

No other new consumer is indispensable. Supported flags and quest-item operations
remain party/world operations; do not loop them over members. Item 100 grants
exactly one unit regardless of selected character. Defer other character-specific
conditions, SetChar/SetVar, TakeOrGive modes, damage and treasure distribution.

## 8. Stages

### 18A - Integrated WhoWill and character context

**Status: approved/planned; next implementation target; not implemented/complete.**

- Objective: deliver selection from decoder through SDL, with semantic use by Action 9.
- Added behavior: operand/cardinality validation, eligibility, context lifetime,
  cancellation, typed responses, F1-F6 and contextual Escape.
- Likely subsystems: decoder, interpreter, character, presenter, EventFlow,
  PlayerAction, SDL and Application wiring; EventSystem response transport.
- Exclusions: other character consumers, inventory, combat and new script operations.
- Focused tests: synthetic decoding, cardinality/eligibility, protocol, Action 9,
  calls/restarts, pagination/rebase and SDL input cases from A01-A25/A30-A32.
- Real checkpoint: open the original map-20 WhoWill and verify prompt, choice
  and cancellation through the integrated flow.
- Completion: the whole capability works publicly, focused tests and relevant
  regressions pass, and no runtime path recognizes WhoWill without a presenter
  able to complete it. Decoder-only or headless-only support is not stage completion.
- Still uncertified afterward: full original Bone Whistle acceptance and final
  milestone stabilization. All explicitly excluded systems remain unsupported.

### 18B - Original-data acceptance and stabilization

**Status: planned; not implemented/complete.**

- Objective: establish original collection, cancellation, repeat and fresh-session
  behavior while preserving M17.
- Added production behavior expected: none; fixes stay within the 18A contract.
- Likely subsystems: original-data smoke, regressions, test target registration
  and validation documentation.
- Exclusions: normal route certification, quest completion, NPCs and opcode expansion.
- Focused tests: full original chain, Remove identities, counts, cache/scene
  reconstruction, SDL and Phirna regressions.
- Real checkpoint: exactly one item 100 granted and original Remove applied;
  cancel leaves state intact; repeat cannot grant again.
- Completion: build, complete CTest, A01-A36, original-data smoke, visual inspection
  and Phirna regression pass with evidence recorded. Distinguish dummy/software
  SDL from any physical-display validation actually performed.
- Still unsupported afterward: every capability listed in section 11.

## 9. Acceptance matrix

All rows are required and pending. Synthetic cases cover verified semantics or
the explicit defensive MMModern decisions above, not invented original scripts.

| ID | Case | Required result |
|---|---|---|
| A01 | Decoder opcode 20, parameters 00 03 | Correct two operands and preserved source metadata |
| A02 | 0, 1 or more than 2 parameters | MalformedInstruction; no subsequent execution |
| A03 | Verb outside 0..31 when dialog is needed | Operand error before presentation |
| A04 | Missing text, wrong map, invalid index | Existing specific errors; no selection/grant |
| A05 | Empty party | EmptyParty; no member-0 access |
| A06 | One healthy member | Automatic index 0; no WhoWill suspension |
| A07 | One incapacitated member | Same automatic selection; no dialog eligibility filter |
| A08 | Parties of 2-6 members | Pending selection with corresponding valid range |
| A09 | First and last valid members | Indices 0 and N-1; correct roster resolution |
| A10 | F-key above N | Remains pending; selection unchanged |
| A11 | Each of six blocking worst conditions | Refusal, feedback and retry |
| A12 | Combined conditions | worstCondition governs, including Asleep plus Confused |
| A13 | All ineligible, N >= 2 | No automatic selection; Escape remains available |
| A14 | Cancel WhoWill | Terminates event without executing its next line |
| A15 | Cancel inside CallEvent | Terminates caller too; no implicit Return |
| A16 | Selection then display/pages/ack | Context retained; instructions run once |
| A17 | Selection before CallEvent | Available in callee and after Return |
| A18 | New selection in callee | Remains current after Return |
| A19 | New independent dispatch | Context resets to first member |
| A20 | Supported TeleportAndContinue | First member at destination; transaction/limits preserved |
| A21 | Action 9 before/after WhoWill | First member before; selected member after, for supported comparisons |
| A22 | Wrong response type or invalid payload | Protocol error; no subsequent instruction |
| A23 | Stale/repeated response in active flow | Cannot resume another presentation or repeat effects |
| A24 | Rebase/cache discard during selection | Request/context preserved; no automatic response |
| A25 | Cancel/error after prior effects | M17 policy retained; earlier grants/Remove not undone |
| A26 | Bone Whistle success | Original display, acknowledgment, +1 item 100 and correct Remove |
| A27 | Bone Whistle cancellation | Count unchanged, object/events present, no later display |
| A28 | Repeat after successful collection | No new grant; removal remains authoritative |
| A29 | Fresh session | Initial resources restored; no inherited selection/removal |
| A30 | F-keys and Escape with repeat | One action per press; repeated cancel Escape cannot exit |
| A31 | Navigation during/after choice | Blocked while pending; resumes on completion/cancellation |
| A32 | Escape outside WhoWill and SDL_QUIT | Existing exit behavior preserved |
| A33 | Phirna No, harvest, already-owned | Approved M17 outcomes preserved |
| A34 | Quest items | Counts, ranges and overflow preserved; no per-member multiplication |
| A35 | Remove | Identity, session persistence and event effects preserved |
| A36 | General regressions | Pagination, ack, Yes/No, calls, teleports, camera/flags and limits preserved |

Retain existing quest-grant prevalidation and immediate-error tests, Remove and
session-identity tests, presentation/navigation tests and SDL repeat suppression.
Do not weaken error validation or assertions to accommodate the new response type.

## 10. Exact original Bone Whistle checkpoint

**Verified original data:** `maze0020.evt` has 16 records. At `(5,14)`, filtered
for North, these five records apply. All have direction `All(4)`; the controlled
checkpoint faces North and is not an automatic trigger.

| Line | File offset | Opcode | Parameters | Meaning |
|---:|---:|---|---|---|
| 0 | 7 | 0x20 WhoWill | 00 03 | Verb search, event text 3 |
| 1 | 15 | 0x29 DisplayBottom | 00 | Event text 0 |
| 2 | 22 | 0x09 If2 | 2C 01 03 | Action 44, value 1: existing acknowledgment |
| 3 | 31 | 0x0C TakeOrGive | 00 00 15 64 | Existing bounded quest-item grant, ID 100 |
| 4 | 41 | 0x0E Remove | empty | Existing Remove |

`aaze0020.txt` index 3 is `Bones`; index 0 describes finding a bone whistle.
Do not hard-code the narrative in production or place extracted commercial data
in the repository. Load the original resources through existing providers.

The selected object was verified through the production loader/selector/resolver:

- identity: `{Clouds, map 20, recordIndex 1}`;
- position: `(5,14)`; object direction: `0/North`;
- table index: 1; resource ID: 26;
- sprite: `026.obj`; frame: 0; status: `SupportedStatic`.

Keep `XeenWorld::applyRemove` authoritative: it disables the selected object by
identity and the original event records at the physical cell. Do not add a
Bone Whistle-specific duplicate-grant guard or use sprite ID as object identity.

### Required validation procedure

1. Create a fresh graph of original loaders, party, world, event system and
   presentation flow at diagnostic camera `(20,5,14,North)`. Record initial item
   100 count `q0`, selected object and effective events.
2. Interact from line 0. Expect WhoWill; count/removal state remains unchanged.
3. Choose a valid member. Expect original DisplayBottom and then acknowledgment.
4. Acknowledge. Require count `q0 + 1`, correct disabled object and disabled
   events at that cell, with normal event completion.
5. Interact again. Require no repeat chain presentation and count still `q0 + 1`.
6. Reconstruct scene/caches and leave/return using controlled harness positioning.
   Ownership and removal remain authoritative in the session.
7. In a separate fresh session, cancel WhoWill. Require count `q0`, original
   object/events present and no presentation from subsequent lines.
8. Interact after cancellation. WhoWill must appear again.
9. In another fresh session, verify initial state restoration.

Run the future harness directly and through SDL input, inspect native frames and
record any physical-window validation separately. Use unmodified original data;
diagnostic positioning is not normal-route or combat certification. This chain
does not exercise Action 9, so the separate selected-SP tests remain mandatory.

## 11. Explicitly out of scope

Unless a separately identified contradiction makes one strictly necessary for
WhoWill correctness, the following are excluded; do not silently broaden M18:

- NPC opcode; Myra/Phirna delivery; Orothin quest completion;
- quest-item consumption; general inventory and treasure distribution;
- GiveEnchanted, GiveMulti, XP or gold rewards;
- generic TakeOrGive expansion;
- generic SetChar/SetVar and other character-specific condition/operation consumers;
- damage, combat, monster behavior and normal route certification to Bone Whistle;
- sound/event voices;
- world flags beyond existing supported behavior;
- animated exterior objects;
- save/load;
- Darkside-specific gameplay;
- general mouse UI, remappable controls, general menu framework or broad UI redesign;
- broad architectural refactoring.

No later milestone is planned by this document. Any concrete scope conflict must
be identified before implementation expands beyond the approved specification.

## 12. Risks and remaining validation questions

Core semantics and checkpoint identities were resolved by the investigation.
Remaining implementation/validation concerns are:

- EmptyParty is a defensive MMModern decision, not reference behavior.
- Eligibility must preserve worst-condition ordering and the one-member exception.
- Character context, object identity and the separate reference treasure preference
  must not be conflated.
- TeleportAndContinue resets character context while preserving MMModern's
  transaction and accumulated instruction budget.
- Exactly-once response behavior must be tested at the owner of live continuations.
- In-panel rejection feedback deliberately replaces the reference nested error
  wait; verify readability, retries and contextual Escape.
- Check wrapping/clipping with the original font and full verb table, not only Bones.
- The investigation verified data and static-object resolution, not a future
  WhoWill UI. All implementation acceptance remains pending.

None of these presently requires an excluded subsystem.

## 13. Expected implementation files and tests

Expected production changes, limited to the approved behavior:

- `src/games/xeen/XeenEventDecoder.{h,cpp}`;
- `src/games/xeen/XeenEventInterpreter.{h,cpp}`;
- `src/games/xeen/XeenEventSystem.{h,cpp}` as needed for response transport;
- `src/games/xeen/XeenCharacter.{h,cpp}` for the pure helper;
- `src/games/xeen/XeenEventPresenter.{h,cpp}`;
- `src/app/XeenEventFlow.{h,cpp}`;
- `src/core/PlayerAction.h`, `src/platform/sdl/SdlWindow.{h,cpp}` and
  `src/app/Application.cpp` for input and wiring.

No change is expected to serialized formats, party/character loaders, quest-item
storage or Remove implementation. Reuse existing portraits and text rendering.

Extend relevant `tests/XeenEventDecoderTests.cpp`,
`tests/XeenEventPresentationTests.cpp`, `tests/XeenEventUiTests.cpp`,
`tests/XeenCharacterRulesTests.cpp` and `tests/SdlInputTests.cpp`. Add focused
WhoWill coverage and an original-data Bone Whistle smoke following
`tests/PhirnaIntegrationTest.cpp`. Register necessary targets in CMakeLists only
during implementation; keep commercial-data smokes separate from ordinary
synthetic tests. Rerun interpreter/system/navigation, quest-item/grant,
Remove/session-identity/persistence and Phirna coverage. Mechanical signature
updates must preserve assertions.

Future authorized implementation must update this plan's evidence, project status
and public controls/capabilities when they actually change. Do not mark stages
complete in advance or replace historical M17 evidence with an unexecuted claim.

## 14. Recommended implementation order

1. Establish operand, cardinality, worst-condition and indexing tests.
2. Add ephemeral context and typed responses while preserving existing cases.
3. Integrate WhoWill, Action 9, calls and resets with validation before later effects.
4. Complete presenter, single-consumption ownership, F1-F6 and contextual Escape
   before declaring 18A complete.
5. Validate original success, cancellation and repeat paths in 18B.
6. Validate reconstruction, fresh sessions, SDL and Phirna regressions.
7. Build and run complete CTest for milestone completion; record actual evidence.

The current task only records this specification. No implementation, build,
test execution, commit, push, tag or branch change accompanies this approval.
