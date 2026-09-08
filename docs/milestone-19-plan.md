# Milestone 19 - NPC dialogue and Myra's quest request

**Status: approved specification; 19A complete on 2026-09-08; 19B pending.**

The specification and historical planning evidence below are retained. Section
17 records the separately authorized 19A implementation and acceptance evidence.
Only its stage-specific criteria have passed; M19 is not stable or complete.
Roadmap approval and 19A completion do not authorize beginning 19B or a successor.

Throughout this document, **Verified** identifies inspected repository/reference
code or original resource evidence. **Decision** identifies the proposed
MMModern contract, including defensive checks and presentation adaptations.

## 1. Goal and approved boundary

Execute the original Clouds Myra request from ordinary manual interaction at
map 23 `(9,11)`, facing West, without a Phirna Root:

Space -> original possession/SP conditions -> original NPC request -> final
acknowledgment -> set Clouds quest flag 2 -> original Exit.

Preserve resumable execution and the existing event/presentation path. Repeated
requests must follow the original script. With a root, present the original
return dialogue, then stop at unsupported consumption without consuming the root,
clearing quest state, granting rewards or implying that the exchange succeeded.

The [approved roadmap](roadmap.md) remains unchanged. Investigation found no
contradiction requiring replanning. NPC portrait timing belongs to this bounded
presentation, independently of M22 world-object animation. M20 may later serialize
the resulting authoritative party state; this plan designs no disk format.

## 2. Verified baseline and evidence

Planning began with these repository facts:

- Branch: `main`.
- HEAD: `3dff574a466a692db2efc3b7ec2952e64c7912c2`.
- `git status --short`: empty; no preexisting changes.
- [Project status](project-status.md): stable M18; 18A complete and independently
  approved, 18B complete on 2026-09-08, without a claimed independent 18B review.
- Recorded validation: successful build, **44/44 full CTest**, **31/31 focused
  regressions**, direct and SDL dummy/software Bone Whistle/Phirna smokes, inspected
  native frames, no physical-window validation. These are recorded M18 results,
  not new planning-task runs.
- [Roadmap](roadmap.md): approved on 2026-09-08; M19 is the immediate successor.
  Current decoder has no NPC operation, party state has no quest flags, and
  interpreter mode 104 is unsupported. M19 has not been implemented.

Read `AGENTS.md`, project status, roadmap, [dependencies](dependencies.md), and
the completed [M17](milestone-17-plan.md) and [M18](milestone-18-plan.md) plans.
Their older baseline/status statements remain historical; current repository
code and the current project-status heading govern this specification.

The relevant current architecture, without repeating the full status inventory:

| Responsibility | Current implementation and relevant tests |
|---|---|
| Strict operands and diagnostics | `XeenEventDecoder.{h,cpp}`; `XeenEventDecoderTests.cpp` |
| Value-owned suspension, calls, selected character, conditions and effects | `XeenEventInterpreter.{h,cpp}`; interpreter, presentation, quest-item, quest-grant and WhoWill tests |
| Script/text caches; camera/game-flag completion commits | `XeenEventSystem.{h,cpp}`; system/manual-event tests and WhoWill `integratedTextErrors` |
| Owned roster, active membership, counted quest items and initial loading | `XeenParty.*`, `XeenPartyLoader.*`, `XeenQuestItemFormat.*`; character-format, quest-item and party integration tests |
| Text, retained layers, transient UI, paging and rebasing | `XeenEventPresenter.*`, `XeenTextRenderer.*`; event UI and visual Remove tests |
| Live continuation owner and generation checking | `src/app/XeenEventFlow.*`; WhoWill `productionFlow`, `directResponses`, `passiveSelectionInputs`; quest-grant `presentationNoReplay` |
| Input and application wiring | `src/platform/sdl/SdlWindow.*`, `src/app/Application.cpp`; SDL input/WhoWill SDL tests |
| Read-only archives and cached sprite ownership | `XeenAssetSource.*`, `ScummVmXeenBridge.*`, `XeenObjectSpriteSafety.*`; sprite, visual, composer and session tests |
| Original collection controls | `PhirnaIntegrationTest.cpp`, `WhoWillIntegrationTest.cpp`, shared `XeenPartySnapshotTestSupport.h` |

Paths above are under `src/games/xeen`, `src/formats/xeen`, `src/compat/scummvm`
or `tests`, as appropriate. No new parallel dialogue system is needed.

### Reference provenance and bounded investigation

The dependency pin is **`6814ee9ba54582f5b5adcffab49efbbd8f589edd`**.
Verified local HEAD and empty content status at
`D:/Projetos/MModern/scummvm-known-good-candidate`, using command-local
`safe.directory` and `core.autocrlf=false`, without changing Git configuration.
`build/18a/CMakeCache.txt` uses that source and
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. M18B reused this build;
`build/18b` contains validation outputs, not a separate configured build.

Targeted reference evidence, relative to that pinned source:

| Location | Evidence used |
|---|---|
| `engines/mm/xeen/scripts.cpp`, `doOpcode`, `cmdNPC`, `cmdIf`, `ifProc`, `cmdTakeOrGive` | Title origin, five operands, response/fallthrough, SP and item conditions, mode widths and dispatch |
| `engines/mm/xeen/locations.cpp`, `LocationMessage::execute`, `BaseLocation::drawAnim`; `locations.h` | NPC resources, layout, pages, acknowledgment and temporal portrait behavior |
| `engines/mm/xeen/events.{h,cpp}` | 50 ms game-counter ticks; NPC waits three ticks between animation steps |
| `engines/mm/xeen/window.{h,cpp}`, `font.cpp` | Window 11, inner bounds, coordinate controls and normal-font line spacing |
| `engines/mm/xeen/party.cpp`, `synchronize`, `giveTake` | Separate packed quest flags; side-local mode-104 set/clear |
| `engines/mm/shared/xeen/file.cpp`, `syncBitFlags` | Least-significant-bit-first packed layout |
| `engines/mm/xeen/dialogs/dialogs_quests.cpp`, current-quests case | Quest flags identify active requests; journal indexing is not script indexing |
| `engines/mm/shared/xeen/cc_archive.cpp`, `sprites.cpp` | Targeted archive lookup and FAC frame/cell directory interpretation |

Original resources were accessed read-only at
`F:/Games/gog/Might and Magic 4-5`. The existing production diagnostic was run:

```powershell
build/18a/mmodern.exe --inspect-events 'F:\Games\gog\Might and Magic 4-5' 23 9 11 west
```

A transient, read-only in-memory archive inspection, following the existing
bridge/SyntheticXeenArchive layout and pinned archive reader, checked only the
Myra text entries, `face17.fac`, `frame.fac`, initial party flag bytes and the
same event records. It reproduced the production diagnostic's offsets/operands.
No extracted resource, helper source, commercial asset or modified game file was
written. No broad opcode/resource census or route investigation was performed.

## 3. Original Myra records and present frontier

**Verified:** `maze0023.evt` has 170 records. Myra occupies original records
21-35, all at `(9,11)`, direction West (`3`). The cell is not an automatic
trigger. The resource is in the reconstructed initial Clouds archive; text is
in the outer Clouds `xeen.cc` resource `aaze0023.txt`.

Operands below are hexadecimal bytes; explanations use decimal values.

| Record | Offset | Line | Opcode / parameter bytes | Meaning |
|---:|---:|---:|---|---|
| 21 | 182 | 0 | `09 If2`: `15 63 07` | Action 21, item 99, equality -> line 7 if root owned |
| 22 | 191 | 1 | `08 If1`: `09 00 04` | Action 9, current SP >= 0 -> line 4 |
| 23 | 200 | 2 | `05 NPC`: `01 02 11 01 03` | Title 1, body 2, portrait 17, mode 1, target 3 |
| 24 | 211 | 3 | `12 Exit` | Exit after alternate no-root text |
| 25 | 217 | 4 | `05 NPC`: `01 00 11 01 05` | Title 1, request body 0, portrait 17, mode 1, target 5 |
| 26 | 228 | 5 | `0C TakeOrGive`: `00 00 68 02` | Take nothing; give mode 104, value 2: set quest flag 2 |
| 27 | 238 | 6 | `12 Exit` | Request completes |
| 28 | 244 | 7 | `05 NPC`: `01 03 11 01 08` | Title 1, return body 3, portrait 17, mode 1, target 8 |
| 29 | 255 | 8 | `0C TakeOrGive`: `15 63 00 00` | Take mode 21, item 99; no give: consumption boundary |
| 30 | 265 | 9 | `0C TakeOrGive`: `68 02 00 00` | Clear quest flag 2; beyond M19 |
| 31-35 | 275,285,295,305,315 | 10-14 | `2C GiveEnchanted`: `46 25 00 01` each | Five original reward instructions; beyond M19 |

The omitted third TakeOrGive pair is neutral, as already decoded in M17.
There is no quest-flag condition, WhoWill, Action-44 acknowledgment, Remove,
Call or explicit line-15 record in this Myra sequence.

### Reachability and instruction accounting

**Verified from current code and pinned semantics:** Action 9 converts current
SP to an unsigned comparison value. Thus `>= 0` is true for every represented
SP value, including zero and negative serialized SP after conversion. Independent
dispatch initializes character context to the first active member. With a
nonempty party and no root, normal line-0 execution always uses **0,1,4,5,6**.
Do not invent a second no-root outcome based on class, quest flag or zero SP.
Lines 2/3 exist in the original resource but are not reached by this line-0 path.
They may be covered as a diagnostic NPC layout variant, not a normal revisit.

Today the no-root path stops at **line 4, offset 217, record 25, opcode 0x05**,
`UnsupportedOpcode`, after **two dispatched instructions**. Decode failure is
before the interpreter increments the count. With a root, today it stops at
**line 7, offset 244, record 28**, after one dispatched instruction.
These frontier counts are derived from inspected production control flow;
planning did not run a new Myra execution smoke.

After NPC support, the no-root stage boundary becomes line 5 / offset 228,
`UnsupportedOperationMode`, four dispatched instructions. Complete M19 request
execution counts five instructions. Root-owned execution after NPC support
reaches line 8 / offset 255, `UnsupportedOperationMode`, three dispatched
instructions. That remains the required M19 frontier. Paging/timer updates do
not add script instructions or rerun the NPC opcode.

## 4. NPC decoding and execution decisions

### Bounded typed operation

Add a typed NPC operation to the existing decoded variant, containing exactly
five bytes: `titleTextIndex`, `bodyTextIndex`, `portraitId`, `confirmationMode`,
`targetLine`. Zero through four bytes, or extra bytes, are malformed. Retain
source map/resource/record/offset/coordinates/direction/opcode in diagnostics.
Neither portrait 17 nor title index 1 is an NPC-service identifier.

Structural decoding preserves all byte values. Execution supports **mode 1
only**, in Clouds logical and physical context. Reject mode 0 (Yes/No), mode 2
(the reference's immediate-return presentation form), and every other value
with `UnsupportedOperand` identifying the mode value and source, before any
presentation or subsequent effect. Do not silently map nonzero modes to 1.
Do not impose a map, coordinate, portrait-ID or dialogue-text whitelist.

### Title, body and branch semantics

The reference's `doOpcode` derives `_message` from the first parameter before
`cmdNPC` consumes that byte. It is the **current NPC record's title**, not a
previous display's text. The second parameter selects the body from the same
executing map's text table. Resolve both once through the existing text provider;
requests own both raw strings, indices, portrait metadata and source by value.
Never retain pointers into script/text caches or use the physical camera's map
to resolve a called event's text.

Preserve specific missing-resource, map-mismatch and invalid-index diagnostics.
Existing empty strings are valid. **Defensive decision:** invalid title indices
are errors, matching MMModern's strict text contract; the reference substitutes
an empty title for an out-of-range first parameter. Do not change existing
display/WhoWill text error behavior to accommodate NPCs.

For mode 1, `LocationMessage` returns false after acknowledgment. Therefore
`cmdNPC` **falls through**; it does not take `targetLine`. Myra's target bytes
happen to equal the following line, but this coincidence must not drive the
implementation. Preserve the unused byte, and do not validate the existence of
an unused branch target. Synthetic targets different from, or absent relative
to, the successor must prove fallthrough. Ordinary sequential overflow and
natural missing-line completion apply; the M17 Action-44 exception is unrelated.

Use a distinct NPC presentation kind with acknowledgment response, title/body
metadata and mode 1. Reuse the existing `Acknowledged` signal and Advance
continuation: validate sequential line bounds before suspension, retain the
next logical line, and resume there only after the final page is acknowledged.
There is no need for a generic NPC-response union or service registry in M19.

NPC presentation itself does not select a party member or require action
eligibility. Do not apply WhoWill's `canAct()` checks. Original line-0 Myra already
requires a nonempty party through Action 21; the new party mutation also checks
that supported context. Standalone synthetic NPC presentation need not reject
an otherwise empty party merely to draw text.

## 5. NPC visual presentation and asset ownership

### Verified resource and layout requirements

The original text entries have these roles; full commercial prose is not a
fixture or production constant:

| Entry | Verified content characteristics |
|---:|---|
| 0 | Request explanation and offer; 259 bytes, 49 literal spaces; no formatting controls |
| 1 | Two-line Myra/herbalist title; 22 bytes, newline and a `TAB` + `125` horizontal-position control |
| 2 | Alternate no-root response; 69 bytes, 13 spaces |
| 3 | Return/ingredients response; 58 bytes, 10 spaces |

Title entry 1 contains one literal space. Body strings have no leading title or
portrait data. The title's positioning control must not appear as visible digits.

`LocationMessage` loads `face%02d.fac` from the operand (decimal 17 therefore
`face17.fac`) and `frame.fac`, both from the current Clouds outer archive. It
does not use party portraits, `.mob` sprites, world-object metadata or a separate
NPC title database. `face17.fac` is 878 bytes, **four frames 0..3**. The frames
share a 32-by-31 base cell at relative `(0,1)` and have optional additional cells;
all layers must render. `frame.fac` is 1102 bytes, one 46-by-44 frame.

Use original `fnt` and the already loaded Clouds palette (`mm4.pal`). The reference
uses main window 11, outer rectangle `(8,8)-(224,140)`, inner rectangle
`(16,16)-(216,132)`. Its English wrapper clears the text area, centers the title
at inner X + 125, Y + 14, then body at inner Y + 54. Thus:

- frame 0 of `frame.fac`: anchor `(16,16)`;
- selected portrait frame: anchor `(23,22)`;
- first title line: Y=30, horizontal center anchor X=141;
- title's explicit second-line tab: the same X=141 anchor;
- body: starts Y=70, centered over the inner text width, with normal 10-pixel
  line spacing, wrapping and pagination inside the body region.

Use the existing MMModern window-border/text rasterization conventions; this
milestone is not pixel-identical reproduction of every reference window symbol
or border. Correct original portrait/frame, relative placement, readable complete
text and response/timing behavior are required.

### Integration, rendering and failure

Extend `XeenEventPresenter`, used by `XeenEventFlow`; keep interpreter and
EventSystem free of SDL and decoded sprite ownership. Inject a small NPC sprite
composition callback into the presenter/flow from Application. It draws the
requested original frame/portrait onto a supplied indexed frame, using the
existing `XeenAssetSource`/bridge cache. This is a drawing dependency, not an
NPC service layer. Synthetic tests supply controlled assets through this seam.

The bridge currently draws onto its own composer surface and returns snapshots.
Add only the narrow ability needed to draw cached, validated normal sprites onto
a supplied `IndexedFrame` (or equivalent isolated surface adapter). Do not copy
the archive/sprite decoder into a second system, contaminate the scene composer's
surface with transient NPC pixels, or make the presenter own a second archive.
Resource names are derived from the portrait operand; asset owners outlive the
flow. Presenter layers retain semantic identifiers, not cached sprite pointers.

Preflight `frame.fac` frame 0 and all four portrait frames before accepting the
NPC visual request. Reuse the existing normal-sprite structural preflight and
pinned sprite drawing, with a narrow generalization of its object-specific API
if needed. Require available frames 0..3 and valid referenced cell streams;
extra frames need not be used. Missing/truncated/invalid assets fail explicitly,
not as a blank portrait or silent static fallback. No reduction, enlargement,
world occlusion, palette animation or alternate sprite family is required.

The current text renderer consumes but ignores absolute X/Y controls. Merely
passing the reference wrapper to it would misplace title and body. Lay out the
two semantic regions separately, and add a bounded, opt-in positioned-title
layout for newline and `TAB nnn` using the existing glyph metrics. Tab offsets
are relative to the NPC inner rectangle, not arbitrary literal 125 matching.
Default title anchor is X=141; an explicit tab updates the anchor for its line.
Preserve existing handling/diagnostics for supported font/alignment/color controls.
Malformed positioning controls produce diagnostics; do not alter M14/M18 layouts
globally. Bound long titles to the heading region and diagnose clipping; original
Myra's full title must fit. Body pagination must never silently discard text.

Expose minimal per-page consumed-source metadata from body layout if needed
for animation duration and continuation. Preserve original whitespace in those
spans. This is a small text-layout extension, not a new text interpreter. The
exact native wrapping/page count is a **pending visual acceptance measurement**;
it is not assumed to be one page in tests or input logic.

Draw the panel/text and the original portrait/frame without overlap. Keep the
portrait and title on every page. Rebase/redraw from clean underlay plus retained
semantic layers, so an optional sprite cell from a previous frame cannot leave
stale pixels. NPC is transient: remove its entire panel, heading and portrait
on final acknowledgment, replacement, abandonment or error. Preserve unrelated
retained M14 layers. Do not leave an animated portrait as a passive message.

## 6. Bounded portrait timing

**Verified:** the reference does animate this dialog. Static frame 0 alone is
insufficient for faithful M19 visual acceptance. This animation is neither a
world-object capability nor an indefinitely cycling portrait.

`LocationMessage` starts `_drawFrameIndex=0`; the base constructor initializes
both counters to zero. Each displayed page sets the speech counter to twice
the number of literal spaces in the formatted title plus consumed body span.
While waiting, it calls `drawAnim(false)` after three 50-ms game ticks, nominally
**150 ms**. The call first draws the current frame, then toggles a phase bit,
chooses a random frame in 0..3, and decrements the speech counter on every second
step (or whenever the counter is already zero). Once the counter expires, the
next selected frame is forced to 0 and remains 0. Acknowledgment need not wait
for this duration. Page transitions reset the speech counter for their text;
they retain the current frame/phase, as the reference does.

**Decision:** reproduce this bounded four-frame/counter behavior with transient
presenter-owned state: displayed/next frame as needed, phase, remaining speech
counter and next deadline. Use a local random source with injectable deterministic
values for tests; exact equality to a particular ScummVM RNG history is not an
acceptance requirement. Count literal source spaces, not normalized word tokens;
include the repeated title and the body's consumed span for that page. Use
checked/wide arithmetic for counters derived from text size.

Add an optional monotonic-time frame-update callback to the existing SDL loop,
wired to a narrow `XeenEventFlow` presentation-update method. Poll it without
keyboard input; tests must prove this path. No nested event loop, background
thread, fake navigation action or interpreter timer is needed. No tick changes
party/world state, selected character, instruction count or pending generation.

Keep input responsive. Match the reference's bounded polling behavior: at most
one due animation step per callback; schedule the next step from observed time
rather than replaying an unbounded backlog after a stall. Once resting frame 0
has been displayed, stop producing changed frames until a new page/request.
No-input ticks while no NPC is active do no composition work. SDL_QUIT must
prevent any subsequent callback/resume in that loop iteration.

Rebasing preserves page, current/next frame, phase, speech counter and generation;
it must not reseed, advance time or restart speech. Sprite-cache discard reloads
resources on reconstruction without changing temporal state. Independent dispatch
starts a new portrait session at frame 0/phase 0. No timing data belongs in
party/world persistence or any M20 handoff.

## 7. Response, suspension and cancellation lifecycle

**Verified:** mode 1 accepts a dismissal and returns false. Unlike WhoWill,
Escape does **not** invoke Exit/abort. On an intermediate page, dismissal
advances the page; on the final page it falls through to the next instruction.
Mode-0 Yes branching is not used by Myra.

**MMModern input adaptation:** use the existing Space/Enter acknowledgment
controls and contextual Escape. All three advance a nonfinal NPC page, and
acknowledge the final page. Y/N do not mean acceptance/refusal; F1-F6 do not
select a member; navigation stays blocked. The reference accepts broader
key/button dismissal, but M19 retains MMModern's explicit keyboard controls;
general any-key/mouse dialog input is not required.

Consequently **Escape on the final request page sets quest flag 2** by continuing
the original script. Do not implement a fictional "decline Myra's request"
branch. Label this behavior dismissal/acknowledgment, not WhoWill cancellation.
The existing `CancelInteractionAction` may be contextually translated to
`Acknowledged` for NPC mode 1; `CharacterSelectionCancelled` remains invalid for
an NPC continuation.

Extend the SDL Escape-routing capability to include pending NPC mode 1. Keep
character-selection detection distinct: `XeenEventFlow::handle` currently uses
`canCancelInteraction()` also to guard F-key input. Broadening Escape handling
must not accidentally make F-keys choose/acknowledge an NPC. A narrowly named
Escape capability or explicit response-kind check is sufficient.

- Intermediate pages stay entirely within the presenter: no event resume and
  no mutation. Even one-page NPC text always waits for acknowledgment.
- Final input uses `XeenEventFlow::respond(generation, Acknowledged)`. Consume
  pending state before resume, finalize transient UI idempotently, and issue at
  most one response. The same input must not acknowledge a following NPC/display.
- Repeated SDL keydowns remain ignored, including repeated Escape after dismissal;
  a fresh Escape outside contextual UI retains application exit.
- Direct responses and keyboard finalization share the same visual cleanup.
  Direct acknowledgment is the trusted semantic response for the whole NPC;
  keyboard paging cannot emit it before the final page.
- Wrong response kinds produce `InvalidPresentationResponse`, no later instruction
  and no NPC-specific state mutation. Obsolete/completed/replaced generation
  tokens are rejected by the live flow without resuming anything.
- Calls, instruction budgets, working camera/game flags, active character,
  selected world object and logical lookup direction survive NPC suspension.
  No snapshots of authoritative quest flags/counters are stored in the continuation.
- A new presentation receives a new generation. Rebase and animation do not.
  This preserves M18's live-owner guarantee, not global replay prevention for a
  deliberately copied historical interpreter value or cross-owner token reuse.

Closing the application (`SDL_QUIT`), destroying/replacing a flow, or abandoning
a session is different from Escape acknowledgment. Discard the pending request
without resuming it. No subsequent flag mutation occurs. Previously completed
immediate effects remain in any still-live owner. A newly created flow starts
with no pending presentation and must not receive old response tokens. Replacing
a party owner requires ending its old flow/continuations first, not rebinding a
suspended event to an unrelated party.

## 8. Quest-state ownership, loading and mutation

### Separate domain and initial data

**Verified reference:** `_gameFlags[2][256]`, `_worldFlags[128]`,
`_questFlags[60]` and `_questItems[85]` are separate fields. Mode 104 sets/clears
quest flags at `side * 30 + value` for non-Swords games. Clouds uses side 0,
script values **0..29**. The give path asserts this 30-value domain. Myra's value
2 is zero-based flag 2, not item 2, item 99/index 17, game flag 2 or journal row 2.
The journal looks up flags with its own `idx + 1` convention; do not apply that
offset to script state.

The existing initial party layout is:

| Block | Offset / byte size |
|---|---|
| Clouds game flags | 659 / 32 |
| Darkside game flags | 691 / 32 |
| World flags | 723 / 16 |
| Quest flags, 60 packed bits | **739 / 8** |
| Quest-item counters | 747 onward; M17 reads first 35 bytes |

Bits are LSB-first: absolute flag `i` is `(bytes[739 + i/8] >> (i%8)) & 1`.
Clouds flags occupy bits 0..29, not an independently byte-aligned four-byte bank:
bits 30/31 in the fourth byte already belong to Darkside. Myra is byte **739,
mask 0x04**. High bits of the final serialized byte are padding, not extra flags.
The inspected initial `maze.pty` is 812 bytes; its eight quest-flag bytes are zero,
and Phirna counter 17 is zero. These values are loaded, not hard-coded defaults
for a real session.

### Authoritative owner

**Decision:** add a dedicated **30-boolean Clouds quest-flag value** to
`XeenPartyState`, alongside `questItems`. A small bounded value type, for example
`XeenCloudsQuestFlags`, owns its storage and checked const queries/set operation.
Default synthetic values are all false. Reject negative, wide or out-of-domain
indices before narrowing/indexing; no aliasing/modulo mapping. Different value
instances are independent. No quest journal or general quest service is needed.

Do not extend `XeenGameFlags` or copy its transaction policy by analogy. That
type's 256-bit domain/loader/working copy already has a different contract.
Do not put quest flags in world overlays, EventSystem, presenter or caches.
Quest-item counters remain counted possession, not request state.

Add a narrow format parser under `src/formats/xeen` and invoke it from the existing
`XeenPartyLoader::loadFromResources` / `loadInitialCloudsParty`. Require the complete
eight-byte packed field through offset 747 before publishing its parsed result;
extract only Clouds bits 0..29. Ignore Darkside/padding bits without requiring
them to be zero. Full party loading still requires the M17 prefix through byte
781; it does not become a full ScummVM save parser. Preserve header-only parsing
and existing roster/duplicate-member/character diagnostics. No truncated field
may silently initialize as an empty request set.

This defines loading from the already supported original initial-party resource
and the resource-byte loader seam. Reading arbitrary external saved games or
writing any save to disk is outside M19.

### Required script mutation, and deliberately deferred operations

Add exactly this mode family to ordinary opcode `0x0C`, alongside existing
mode-20 set/clear and mode-21 grant:

`(0,0), (104, value 0..29), (0,0)` with explicit or omitted neutral third pair.

It sets the selected Clouds quest flag to **true once for the party**, regardless
of party size or temporary selected character. Setting an already-set flag is
idempotent. It is not a counter increment or a toggle.

Validate the whole combination, index, Clouds logical/physical context, nonempty
active party and sequential line bounds before setting anything. Invalid mode
combinations are `UnsupportedOperationMode`; invalid quest indices identify the
quest domain (reuse `InvalidFlagIndex` with an unambiguous message); side mismatch
is `UnsupportedExecutionContext`; empty party is `EmptyParty`; line 255 is
`LineOverflow`. The existing pre-dispatch instruction limit remains authoritative.
No partial take/give, earlier-pair application or loop over party members.

**Read/check decision:** const state queries are required for loading, ownership
and acceptance. No Myra request/revisit record executes condition Action 104;
do **not** implement that action solely to test a write. Reference action 104
returns the requested index when set, `UINT32_MAX` when clear, through existing
comparisons; record this fact without adding a new consumer in M19.

**Clear decision:** reference take mode 104 assigns false; Myra uses it only at
line 9, after unsupported root consumption. M19 does **not** execute quest-flag
clear or mixed set/clear instructions. An initially clear value comes from the
loader/default value, not a gameplay clear workaround. Reserve the clear operation
for the separately authorized exchange; do not generalize TakeOrGive now.

### Immediate mutation and lifetime

Follow M17 party-grant/M15 Remove policy: a successfully validated quest-flag set
updates the live party immediately and survives subsequent errors, abandonment
or WhoWill cancellation. Camera and game flags retain their existing working-copy
commit on completion and rollback on execution error. This is MMModern's policy;
the pinned reference is not the source of a global transaction model.

The flag is visible to direct party queries during later suspensions/calls and
to subsequent dispatches in the same session. Resume never restores an old party
copy. Map/script/text/sprite cache discard, scene reconstruction and controlled
leave/return cannot reload or reset flags. A new EventSystem and a new EventFlow
using the **same party owner** retain its flag; they inherit no old continuation.
An explicitly copied party value carries copied flags but is independent. A
genuinely new session loads a new party from original bytes and constructs new
world/events/flow owners; the verified initial request flag is false again.

No new visual mutation counter is needed for a flag with no M19 world appearance
or journal UI. No journals, deferred deltas, undo compensation, retry/replay
framework or disk persistence are introduced.

## 9. Error and reconstruction contract

| Situation | Required outcome |
|---|---|
| Suspended at request, no final response | Flag retains its prior value; no line 5 execution |
| Final Space/Enter/Escape | Remove transient NPC; resume line 5 once, set flag, execute Exit |
| Escape on an intermediate page | Next page only; pending generation preserved, no flag write |
| SDL_QUIT or flow destruction before final acknowledgment | No resume/write; discard transient UI and continuation |
| NPC decode/text/context/asset failure before the set | No new quest mutation; earlier immediate effects remain |
| Invalid mode-104 set | No write by that instruction; earlier successful effects remain |
| Set then unsupported instruction / bad target / failing Remove | Flag stays true; camera/game flags follow existing error rollback; failing Remove adds no effect |
| Set then later successful Remove then error | Flag and Remove survive; camera/game flags roll back |
| Set then WhoWill cancellation | Flag survives; existing whole-event completion and camera/game-flag commit apply |
| Request dispatched again | Execute original conditions/NPC/set again; no flag-based suppression |
| Cache reconstruction during NPC | Retain strings, page, frame/phase/counter, execution context and generation; rebuild assets safely |
| New EventSystem/Flow, same party/world | Flags/counters/removals remain; no pending response or animation |
| New party/session | Use newly loaded flags; old pending execution is discarded, never resumed against it |

Asset/rendering failures require a narrow flow-level failure path: discard the
affected pending presentation/generation, remove its transient layer, and report
an explicit presentation/resource execution error with NPC source and instruction
count (a narrow `PresentationFailed` error kind is appropriate). Do not translate
failure into acknowledgment or silently continue to the
set. Catch failures during initial NPC composition, tick and rebase as well as
input finalization. Preserve manual nonfatal reporting and the existing automatic
error propagation; do not promise a post-exception SDL frame for fatal automatic
errors. A later independent manual interaction may retry from line 0.

If rebuilding the base scene itself fails, preserve existing fatal composition
behavior; it is not evidence of an NPC acknowledgment. Presentation failure
cleanup must not erase earlier party/world effects or commit working game flags.

## 10. In scope and non-goals

In scope: strict five-byte NPC decoding; Clouds mode-1 acknowledgment; original
title/body/portrait/frame loading; bounded four-frame temporal presentation;
multi-page blocking/dismissal; continuation/generation/rebase integration;
party-owned Clouds quest flags and initial loading; only the required mode-104
set; original Myra request/repeat/owned-boundary acceptance and touched regressions.

Out of scope:

- Phirna Root consumption, completed return/exchange, quest-flag clear execution;
- miscellaneous reward storage/distribution, GiveEnchanted, treasure UI, general
  inventory or equipment use;
- generic TakeOrGive, condition-104 execution without a request-path consumer,
  other NPC modes/services, shops, training, inns or a quest journal;
- disk save/load, original-save compatibility or M20 implementation;
- outdoor object animation/M22, general game time or world animation;
- combat, monster AI, new movement, normal travel certification to Myra;
- Darkside/Swords gameplay, speech/audio, localization expansion, generic mouse
  UI or broad UI/architecture replacement.

Production must never special-case Myra by coordinate, NPC/portrait number,
text, event offset or known quest flag. Original script operands determine behavior.
Test checkpoint coordinates and controlled in-memory party fixtures are permitted.

## 11. Proposed stages

Two stages follow the actual dependencies. Neither a decoder-only intermediate
stage nor an additional stabilization-only stage is necessary.

### 19A - Integrated bounded NPC acknowledgment presentation

Deliver one usable capability from decoder/interpreter through EventSystem,
presenter, EventFlow, Application and SDL, including original assets, title layout,
paging, Escape dismissal, temporal portrait behavior, failure cleanup and rebase.
No quest-flag storage/loading/mutation is added in this stage.

Expected files: existing event decoder/interpreter/presenter/flow and SDL/Application;
the narrow asset-source/bridge drawing seam and text-layout metadata as needed.
Reuse sprite preflight. Extend decoder/presentation/UI/WhoWill/SDL tests and add
focused NPC tests plus a production-loading Myra smoke, separately registered
from data-free CTest fixtures.

Independent stage checkpoint: original no-root lines 0,1,4 present the NPC request;
final acknowledgment/Escape reaches the **still unsupported line 5 mode 104**,
with no party/world mutation and a usable scene afterward. Root-owned line 7
presents its original body, then stops at line 8 consumption with root unchanged.
This is an asserted stage frontier, not successful request completion.

Exit: build, focused tests, full CTest, direct/SDL original NPC smoke, native
visual/temporal inspection and affected Phirna/WhoWill controls pass. Cover
M19-A01 through A15 and the stage-appropriate portions of A21, A24-A28 below;
modeled quest-flag assertions belong to 19B. Record actual evidence
without claiming M19 complete. No public path may accept NPC while lacking a
presenter/response path able to finish it. Stop before 19B pending authorization.

### 19B - Quest request state and complete original acceptance

Add the 30-flag party value/parser/loading and only the mode-104 set. Extend
synthetic owner/loading/mutation tests, shared snapshot checks and the 19A Myra
smoke to prove the full request and revisit matrix through production execution.
Use the original EVT/text/FAC data, ordinary line-0 dispatch and the shared flow.
No additional dialogue implementation should be needed except fixes within 19A's
contract. Preserve the root-owned boundary at line 8.

Expected files: party state/loader, narrow quest-flag format component, interpreter,
diagnostic names only if needed, tests/CMake and completion documentation. Add
quest flags to original-data snapshots explicitly; the existing shared helper
currently excludes quest counters and does not know about new flags.

Exit: all acceptance IDs, new build, full CTest, original direct/SDL Myra matrix,
Phirna/Bone Whistle regression smokes and required visual evidence pass. Update
plan evidence, project status and public README only to actual implemented scope.
Only then may M19 be marked complete; no M20 implementation follows automatically.

## 12. Stable acceptance criteria

Keep these IDs stable in implementation/review reports. Synthetic cases may
share fixtures, but assertions must distinguish the policies under test.

| ID | Requirement and expected evidence |
|---|---|
| M19-A01 | Strict NPC five-byte decode with exact fields/source; every truncation and trailing bytes malformed |
| M19-A02 | Mode 1 supported; 0,2,other modes explicitly rejected before presentation/effects; non-Clouds context rejected |
| M19-A03 | Independently resolved title/body, empty valid strings, missing resource/map mismatch/invalid index diagnosed through EventSystem |
| M19-A04 | Mode-1 final response falls through, ignoring a distinct/absent target; line overflow, natural completion and budgets preserved |
| M19-A05 | Original FAC multi-cell frames and frame border use validated production asset loading; missing/corrupt/too-few-frame assets fail with source and no later effect |
| M19-A06 | Native title/body/portrait placement, full original title controls, readable request/return and no overlap/clipping of original content |
| M19-A07 | Long synthetic body paginates; repeated title/portrait; no execution on page advance; final page still requires its own acknowledgment |
| M19-A08 | Frame 0 start, injectable 0..3 choices, 150-ms cadence, alternating decrement, bounded rest at 0; page reset retains phase/frame |
| M19-A09 | Production SDL idle callback updates portrait without input; no animation work when inactive; stalls bounded; animation never changes instruction/state/generation |
| M19-A10 | Space/Enter/Escape page/final semantics; navigation/Y/N/F-keys do not dispatch/select; final input never consumes a following presentation |
| M19-A11 | Live generation consumption once; stale/repeated/replaced responses rejected; direct and keyboard finalization remove identical transient layers |
| M19-A12 | Calls/Return, selected character, world selection, logical/physical distinction and working camera/flags preserved through NPC suspension |
| M19-A13 | Pending cache discard/rebase retains raw strings/page/frame/phase/counter/generation; real provider/sprite reload counts increase |
| M19-A14 | Wrong response and presentation failure clear NPC pendency without subsequent effect; prior effects preserved and independent retry works |
| M19-A15 | SDL repeats, contextual/fresh Escape and SDL_QUIT; closing a pending flow causes no implicit acknowledgment; navigation recovers |
| M19-A16 | Thirty independent default-false flags; checked index domain including 0,2,29 and rejection of negative/30/wide indices |
| M19-A17 | LSB packed loading at 739, bit 29/30 isolation, nonzero fixtures, padding/unrelated bytes ignored, truncation rejected; existing full/header loaders preserved |
| M19-A18 | Only neutral/104-set/neutral accepted; explicit/omitted third pair; idempotent set, all supported indices, one/six-member parties and no unrelated writes |
| M19-A19 | Invalid combination/index/side/empty party/line255/pre-dispatch limit writes nothing; take104, take21 and mixed variants remain unsupported |
| M19-A20 | Successful set immediately visible and survives later error/Remove failure/WhoWill cancellation; game-flag/camera transaction semantics independently asserted |
| M19-A21 | Original root-owned lines 0,7,8 stop at exact consumption diagnostic; root, quest flags, characters, rewards and world unchanged for either initial request-flag value |
| M19-A22 | Original fresh no-root request completes lines 0,1,4,5,6 in five instructions; only quest flag 2 changes false->true after final acknowledgment |
| M19-A23 | Original no-root revisit repeats request/set, true stays true; final Escape also records request; no invented refusal/completion branch |
| M19-A24 | Original pending request has no effect until final acknowledgment; abandonment differs from Escape; independent dispatch starts a fresh NPC presentation |
| M19-A25 | Same-owner reconstruction/new EventSystem/Flow/controlled leave-return retains flags; new session reloads false with no inherited continuation/animation |
| M19-A26 | Phirna No/harvest/owned, counters, Remove and reconstruction remain unchanged; original Bone Whistle WhoWill/ack/grant/Remove/cancel/repeat remain unchanged |
| M19-A27 | M18 audit regressions, M14 pagination/ack/YesNo/labels, calls/teleports/limits, M15-M17 mutation/identity and scene rebasing remain passing |
| M19-A28 | Full CTest/build plus required original-data and visual/temporal evidence recorded; no commercial data changes, no unsupported exchange/travel claims |

## 13. Original-data checkpoint matrix

All mandatory Myra cases use unmodified original records, manual Space at
Clouds map 23 `(9,11)` West, and a nonempty party. `Q2` is Clouds quest flag 2;
`R` is the counted possession of item 99. Controlled states are created in the
test harness's in-memory party initialization, never by patching original data
or adding a production cheat/shortcut.

| Initial state / action | Original line path | Required final M19 outcome |
|---|---|---|
| R=0, Q2=false; final Enter/Space | 0,1,4,5,6 | Complete; Q2=true, R=0; five instructions |
| R=0, Q2=true; final Enter/Space | Same | Complete; Q2 remains true; same request text, five instructions |
| R=0, either Q2; Escape on nonfinal page | Suspended at NPC from line 4 | Advance page only; Q2 unchanged |
| R=0, Q2=false; final Escape | 0,1,4,5,6 | Complete and Q2=true; Escape is dismissal, not decline |
| R=0, Q2=true; final Escape | Same | Complete; Q2=true |
| R=0, either Q2; SDL_QUIT/abandon before final response | 0,1,4 then discard | Q2 unchanged; no line 5; new flow has no pending request |
| R>=1, Q2=false or true; acknowledge return text (including Escape) | 0,7,8 | UnsupportedOperationMode at offset 255; three instructions; R and Q2 unchanged |
| R>=1, either Q2; abandon return text | 0,7 then discard | No line 8 execution; all state unchanged |
| Repeat after request completion | 0,1,4,5,6 | Same request again; idempotent set; no disabled events/objects |
| Fresh session after a completed request | Original initial state, then request path | Initial Q2=false and R=0 restored by loading |

Run root-owned cases with count 1 and a larger controlled count. A count's
magnitude does not change the possession branch. Cover zero and negative current
SP synthetically to prevent the unreachable lines 2/3 from becoming an invented
normal branch. Starting explicitly at line 2 may validate that original text
variant but is not part of the principal gameplay acceptance.

No-root, Q2=false after an eventual completed exchange would again follow the
same request path; this is a control-flow deduction, **not M19 exchange acceptance**.
Do not set a completion bit, suppress dialogue or remove Myra after a request.

For all original cases compare camera, 256 game flags, all modeled character and
membership fields, every quest counter/quest flag, objects and effective event
records. Except for successful request setting Q2, those values must be unchanged.
The original return text can precede the unsupported diagnostic because the
script orders it that way; do not alter text, claim rewards or skip its NPC.

## 14. Test and validation strategy

### Automated correctness

Use synthetic byte buffers/scripts/FAC resources with no commercial strings or
asset bytes. Extend the existing strict decoder, presentation, UI, WhoWill,
party loading and SDL suites; add focused NPC and quest-flag tests as warranted.
Use deterministic time/random injection to assert temporal sequences, counters,
changed-pixel bounds, stable text and absence of trailing portrait cells. Test
initial and mid-animation asset failures, no-input SDL updates, multi-page final
acknowledgment, direct response cleanup and source-rich errors.

Quest-state tests must compare game flags and quest-item counters independently,
not only check Q2. Test exact parser field-end size, one byte short, every bit
boundary, nonzero adjacent Darkside bits and unchanged source buffers. Verify
actual owner reconstruction rather than only clearing a cache map. Keep mutable
party access along existing event execution paths; rendering remains observational.

Run the smallest affected test groups during iteration. Build and run the full
CTest suite at each stage exit; do not close a stage with failing tests. Use the
pinned dependency configuration, with no dependency update or production ScummVM
engine instantiation. Expected unsupported boundaries are passing assertions,
not skipped tests or accepted failing executions.

### Original-data behavioral acceptance

The future Myra smoke must load original party, map, EVT, text, font and FAC
resources through production providers and use production EventSystem/EventFlow.
It starts at line 0; diagnostic camera positioning is allowed. Assert original
record identities/operands, ordered presentations, counts, exact unsupported
source and all state changes. Do not prove the final checkpoint solely by calling
the flag setter, presenting an invented request, or resuming from line 5.

Exercise actual map/object, script, text and sprite cache reconstruction while
the NPC is pending and after completion, plus new event/flow owners retaining
the same party. Use load counters to distinguish rereads from cached draws.
Use a genuinely fresh party/world/events/flow graph for initial-state restoration.
Original files remain read-only and external; compare relevant resource bytes
before/after runs without placing copies in the repository.

### Runtime and visual acceptance

| Evidence | Required? | Reason |
|---|---|---|
| Direct production Myra smoke | Yes | Separately proves original control flow and authoritative state without SDL timing |
| SDL dummy/software smoke | Yes | New idle-update callback and contextual Escape need end-to-end SDL routing, repeat and quit checks |
| Captured native 320x200 frames/time sequence | Yes | Title controls, portrait multi-cell layering, pages and restoration are not certified by state assertions |
| Manual visual inspection | Yes | Inspect request/return, title, all four forced portrait states, representative timed sequence, rest, rebase and dismissed scene for readability and artifacts |
| Physical-window validation | Not mandatory | No new scaling/backend/window feature; real SDL idle scheduling plus deterministic timing and native-frame inspection cover this bounded change. Record separately if performed; do not claim it from dummy runs |

Capture a short time-indexed sequence showing idle animation and a deterministic
accelerated/fake-clock sequence reaching rest; tests must also verify the real
monotonic SDL callback without input. A single static portrait screenshot does
not close temporal acceptance. The future page count and observed layout must be
recorded from actual production captures, not asserted from this draft.

Rerun original Phirna and Bone Whistle smokes directly and with SDL. Other touched
presentation controls should include Air/Corner, Snake Oil and Castle Basenji
No/Yes, preserving labels, font size and transfer rebasing. Synthetic tests cover
the more extensive call/selection/error combinations; do not repeat unrelated
resource archaeology or mechanically rerun every historical visual scenario.

This certifies **local original behavior, automated semantics and the bounded
NPC visual presentation**. It does not certify normal travel, combat, rewards,
disk persistence or Darkside.

## 15. Completion criteria and risks

M19 is complete only after separately authorized stages implement this contract,
all M19 acceptance IDs have evidence, builds/full CTest pass, required original
and runtime/visual validation passes, and status/README accurately report the
capability and remaining consumption boundary. No milestone tag or push follows
without explicit authorization. No M20-M23 implementation is implied.

The behavior/domain questions are resolved by targeted evidence. Remaining risks
are implementation/validation tasks, not grounds for changing the roadmap:

- Myra's title uses real positioning controls that the current renderer ignores;
  verify the bounded title path rather than trusting generic main-window output.
- Exact production wrapping/page count and visual fidelity are unverified until
  19A rendering exists. Full request text must remain readable across whatever
  pages the specified metric layout produces.
- Portrait optional cells require a clean redraw, and all animation frames must
  be preflighted before a late tick can encounter corrupt data.
- The SDL callback must work with no keypresses and remain distinct from gameplay
  updates. Rest, page changes, rebase and generation lifetime need explicit tests.
- Escape must not acquire WhoWill's abort semantics; nor may broadening Escape
  routing accidentally broaden F-key behavior.
- The fourth packed flag byte straddles Clouds/Darkside domains. Test bit 29 vs
  30 and never confuse quest flags with game flags or item counters.
- Presentation errors and new owners must invalidate pending work without erasing
  prior immediate effects or applying an unacknowledged quest set.

The original planning task claimed no production NPC frame, new Myra acceptance,
build or CTest run. Its validation was final-diff review and `git diff --check`
only, leaving project status, roadmap and README unchanged. Subsequent 19A
evidence is recorded separately in section 17.

## 16. Exact recommended first implementation prompt

> Implement **19A only: integrated bounded Clouds NPC mode-1 acknowledgment
> presentation**, following this plan. Read the current implementation first.
> Add strict five-byte NPC decoding, current-record title/body resolution,
> mode-1 fallthrough and resumable acknowledgment through the existing
> interpreter -> EventSystem -> presenter -> EventFlow -> SDL/Application path.
> Include original FAC/frame loading through the existing asset/cache boundary,
> bounded positioned title/body paging, four-frame speech/rest timing through an
> idle SDL callback, Space/Enter/Escape dismissal, generation-safe continuation,
> rebase/cache reconstruction and explicit presentation-failure cleanup. Preserve
> WhoWill cancellation and existing mutation policies. Add focused synthetic
> coverage and a production-loading original Myra smoke that expects the no-root
> path to stop at line 5/offset 228's unsupported mode-104 set after acknowledgment,
> and the root-owned path to stop at line 8/offset 255's unsupported consumption
> without mutation. Build, run relevant tests/full CTest, direct/SDL controls and
> native visual/temporal validation; document only the stage actually completed.
> **Do not add quest-flag storage/loading/mutation, begin 19B, consume a root,
> implement rewards/save-load/world animation, or expand NPC modes/services.**
> Do not commit, push, tag or change branches without a separate instruction.

The first-stage prompt above has now been separately authorized and implemented.
It remains the historical scope boundary, not an instruction to restart 19A.

## 17. 19A implementation and validation evidence

Completed 2026-09-08, after the user's explicit **19A only** instruction. Starting
repository baseline was clean `main` at
`d93752978f94281f089a0b6e8fa2fbf2a2aff0d7`. The branch and HEAD are unchanged.
M18 remains the latest stable milestone. The approved roadmap and dependency
configuration are unchanged; 19B was not started.

### Implementation actually delivered

- `XeenEventNpc` holds exactly the five decoded bytes. The interpreter accepts
  only Clouds mode 1, resolves the current logical record's title and body by
  value, retains the unused target and suspends with the existing Advance /
  Acknowledged continuation. No EventSystem parallel execution path was added.
- `NpcAcknowledgment` is a transient presenter layer. NPC-only title layout
  handles newline and relative `TAB nnn` positioning in the bounded heading.
  The existing renderer handles font/color/alignment controls, including their
  state across title lines; its existing clear-text control discards prior
  heading glyphs without resetting styles. Body wrapping exposes raw source
  ends for per-page speech counters. Every page repeats the title and portrait,
  and the last page still needs its own response.
- Application injects `XeenAssetSource::drawNpc`. The bridge derives `faceNN.fac`
  from the operand, preflights all four frames plus `frame.fac` frame 0, and uses
  the pinned multi-cell sprite renderer on an isolated supplied-frame surface.
  Existing archive/sprite caches own resources; the scene surface is unaffected.
- Presenter-owned displayed/next frame, phase, wide counter and deadline provide
  one due step per callback at nominal 150 ms. Production time is monotonic;
  tests inject time/random choices. Rest performs no additional portrait draw.
  Rebase does not advance/reseed/reset timing. No world clock, script instruction,
  gameplay tick or M22 infrastructure was needed.
- `XeenEventFlow::handlesEscape` is separate from WhoWill selection detection.
  Space/Enter/Escape page or acknowledge NPCs, while Y/N, F-keys and navigation
  cannot resume them. The existing generation owner consumes one response before
  resume. Direct final acknowledgment and keyboard finalization share cleanup.
- A narrow optional SDL idle callback advances presentation without keyboard
  events. Quit stops processing before subsequent input/idle work; Application
  explicitly abandons remaining presentation state after the loop. Destruction,
  replacement and abandonment never acknowledge an NPC.
- `PresentationFailed` reports NPC source and instruction context, consumes the
  failed pending generation and removes only the transient NPC layer. Initial
  draw, page, animation and rebase failures do not execute the next instruction.
  Prior immediate party/world effects remain; transactional working camera/game
  flags keep the established completion/error policy. Base scene composition
  failures retain the existing outer fatal-error boundary.

No production branch identifies Myra, map 23, portrait 17, her strings or event
offsets. Fixed window anchors are the specified general NPC layout, not a Myra
exception. No quest-flag field/parser/load/write, Action 104, clear, TakeOrGive
expansion, Root consumption, reward, persistence or additional NPC mode exists.

### Acceptance coverage

`tests/XeenNpcTests.cpp` supplies only synthetic scripts, strings, fonts and FAC
archives. CTest adds `xeen_npc` and `xeen_npc_sdl`; the latter uses SDL dummy /
software. Existing tests were retained. `tests/MyraIntegrationTest.cpp` adds
`mmodern_myra_smoke`, excluded from the default build and from data-free CTest.
It takes an external game directory and an ignored output directory, optionally
`sdl`. It uses original production party/map/EVT/text/font/FAC loading and the
same EventSystem/EventFlow/presenter used by gameplay.

| Acceptance IDs | 19A evidence |
|---|---|
| A01-A04 | Exact five-byte fields/provenance; truncations/trailing bytes; rejected modes 0/2/3/255 and Darkside; title/body errors, absent/mismatched text, empty valid strings; absent target ignored; natural completion, overflow and budget checks |
| A05-A07 | Synthetic cached FAC two-cell frames; missing portrait/border, corrupt border/directory/late frame and insufficient frames fail before pixels change; isolated scene surface; title anchors/control diagnostics/style carry/clipping; multi-page glyph-count and raw-span coverage; original native placement/page inspection |
| A08-A09 | Injected draw/select/decrement order, 149/150-ms boundary, bounded stall, rest and optional-cell cleanup; page phase/frame preservation; real SDL idle callback without keys; original deterministic four-frame sequences and changed pixels confined to the portrait |
| A10-A11 | Space/Enter/Escape intermediate/final behavior, Y/N/F-key/navigation rejection, following NPC protected; direct/wrong/stale/repeated/replaced responses and identical transient cleanup |
| A12-A15 | WhoWill-selected member through Call/Return; logical transfer vs committed camera; selected-object continuation; working game flags vs prior grants; automatic/manual routing, draw/page/tick/rebase failure, same-owner retry, retained sign, generation/rebase, SDL repeat/quit and flow destruction |
| A21 (19A portion) | Counts 1 and 3 reach original NPC line 7 then exact unsupported consumption line 8/255, three instructions, unchanged state; no quest-flag-value matrix claimed |
| A24-A25 (19A portions) | Original pending state, final Escape, repeat dispatch, abandonment, actual cache reload and fresh EventSystem/Flow; controlled leave/return and new original party/world/event/flow graphs without inherited timing/continuation. Quest-flag retention/loading belongs to 19B |
| A26-A28 (19A portions) | Original Phirna/Bone Whistle direct+SDL controls; affected text/transfer controls; all 44 prior CTest cases remain passing; build/full suite and native temporal evidence below |

A16-A20 and A22-A23 remain pending. No claim is made that A21/A25's future quest
flags are modeled, loaded, mutated or validated. Existing party snapshots and
all 35 counters, 256 game flags, camera, geometry, objects and effective event
records are compared; original `maze.pty`, `maze.chr`, `maze0023.evt` and
`maze0023.mob` bytes are also compared in memory before/after the smoke.

### Actual original-data frontiers

All rows start by ordinary line-0 interaction at Clouds map 23 `(9,11)` West.
Root counts are controlled only through the test's in-memory party initialization.

| Case | Verified 19A result |
|---|---|
| No Root, initial request | `0 -> 1 -> 4`, two-page original NPC request; final Enter or Escape reaches line 5/offset 228, `UnsupportedOperationMode`, four instructions |
| No Root, repeat after that diagnostic | Same request/frontier again, new generation and frame 0/phase 0; no quest state invented or prior presentation inherited |
| Root count 1 or 3 | `0 -> 7`, one-page original return; final acknowledgment reaches line 8/offset 255, `UnsupportedOperationMode`, three instructions; original Root count retained |
| Pending request/return abandoned | No following instruction or terminal response; transient layer removed; independent dispatch works |
| Same-party new EventSystem/Flow, leave/return | Fresh presentation only, no inherited continuation/animation; all authoritative state unchanged |
| New party/world/events/flow graph | Original initial Root count zero reloads before any controlled setup; no session state leaks |

These expected unsupported results pass acceptance. They do not bypass an
instruction or treat a failed smoke as success. No Root, character/member,
game-flag, camera, object or event mutation occurs on either branch. Quest flags
are entirely absent from the 19A model; no bytes are written back to original data.
Direct provider totals per controlled case are maps=3, objects=3, scripts=3,
texts=3. Pending map/object/sprite reconstruction really reloads; the owned
continuation strings remain intact. Discarded script/text caches reload on the
next independent dispatch, then new event owners reload them again. SDL totals
are maps=2, objects=2, scripts=1, texts=1, consistent with rebasing owned pending
metadata without rereading its strings.

### Build and execution record

Debug build: `build/19a`, MSYS2 UCRT64, dependency source
`D:/Projetos/MModern/scummvm-known-good-candidate`, dependency build
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. The pinned source remains
clean at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. No dependency update/patch.

- `cmake --build build/19a --parallel 4`: passed.
- Focused CTest regex
  `xeen_(npc|who_will|event_|manual_event|navigation_flow|quest_|remove|session_|visual_remove)`:
  **21/21 passed**, final run 3.26 s.
- `ctest --test-dir build/19a --output-on-failure`: **46/46 passed**, final run
  4.72 s, including the existing `sdl_input` and all M18 audit regressions.
- Explicit builds of `mmodern_myra_smoke`, `mmodern_phirna_smoke`,
  `mmodern_who_will_smoke`, `mmodern_manual_event_smoke` and
  `mmodern_graphics_smoke`: passed.
- `mmodern_myra_smoke <game-directory> build/19a/myra-direct` and the same
  command with `build/19a/myra-sdl sdl`: passed all three counts (0/1/3).
- Phirna direct/SDL: No = 3 instructions/no grant/plant present; Yes = 18
  instructions/one Root/plant removed; owned = 5 instructions/no new grant/plant
  present. Repeat, reconstruction and fresh-session checks passed.
- Bone Whistle direct/SDL: ten-instruction harvest, item 100 increment and
  object 1/effective event records 1-5 removed; retained success text, repeat,
  cancellation/retry and fresh-session checks passed.
- Manual original smoke: Air/Corner, reduced Snake Oil and Castle question,
  No and Yes transfer passed. Application SDL graphics smoke modes `manual`,
  `manual-no`, `manual-yes` and `event`, with Escape shutdown, passed.

External original game directory: `F:/Games/gog/Might and Magic 4-5`, read-only
throughout. SDL runs use `SDL_VIDEODRIVER=dummy` and
`SDL_RENDER_DRIVER=software`. Captures remain under ignored `build/19a`, not
source fixtures or committed commercial assets.

### Native visual and temporal acceptance

Manually inspected native **320x200** captures in `build/19a/myra-direct` and
`build/19a/myra-sdl`: both request pages, return, all four portrait states for
request/return, deterministic time sequence, rest, pending reconstruction and
dismissed scene. The title is complete, its control digits are not visible,
the frame/face do not overlap it, the body stays within its region, the second
page preserves the remainder, and dismissal restores the scene/HUD cleanly.
Optional facial cells leave no stale pixels. Automated temporal checks assert
that changed pixels stay within the portrait region while state/generation stay
unchanged; pixel equality separately verifies rebase and final scene restoration.

Measured layout: request **2 pages**, initial speech counter **72**, second-page
counter **30**; return **1 page**, counter **22**. With injected random sequence
0,1,2,3, initial frame 0 and 150-ms steps, the first request page reaches displayed
rest at **21,750 ms** and return at **6,750 ms**. These are accelerated injected
times, not required user wait times; acknowledgment remains immediately available.
Captured sequence includes 0,150,300,450,600,750,900,1050 ms and rest. Real SDL
captures also show a changed portrait after approximately 0.46-0.48 s without
keyboard input (draw-before-select plus the injected initial random choice 0).

Air/Corner, Snake Oil and Castle question/No/Yes captures in
`build/19a/manual-controls` were inspected as regression controls. Native PPM
captures were losslessly converted to BMP for inspection. No physical-window,
normal-travel, combat, Root exchange, reward, disk persistence or Darkside
acceptance is claimed.

### Stage closure and remaining boundary

No scope or architecture deviation from the approved 19A plan was needed. The
local SDL idle callback was sufficient; no broader scheduling architecture was
introduced. No unresolved 19A defect is known. The complete diff was reviewed and
`git diff --check` passed. Historical milestone evidence is retained.

Changed files: `CMakeLists.txt`; `README.md`; this plan and `project-status.md`;
`Application.cpp`; `XeenEventFlow.{h,cpp}`; `ScummVmXeenBridge.{h,cpp}`;
`XeenAssetSource.{h,cpp}`; `XeenEventDecoder.{h,cpp}`;
`XeenEventInterpreter.{h,cpp}`; `XeenEventPresenter.{h,cpp}`;
`XeenTextRenderer.{h,cpp}`; `SdlWindow.{h,cpp}`; new `XeenNpcTests.cpp` and
`MyraIntegrationTest.cpp`. No party/quest-state production file was changed.

**Stop at 19A.** The next separately authorized scope is 19B's authoritative
Clouds quest-flag storage/loading and bounded mode-104 set, completing the no-root
request through line 6 while preserving line 8 consumption as unsupported.
The flag implementation, its tests and its full acceptance matrix remain pending.
No commit, push, tag, branch change or history rewrite was performed.
