# Milestone 21 - Myra's return exchange and bounded item rewards

**Milestone 21 is complete.** Stages 21A-21D are accepted. This closed record
preserves the final specification, architectural decisions and acceptance boundary.
See [project status](project-status.md) for the current technical snapshot,
[history](project-history.md) for completed milestones and [roadmap](roadmap.md)
for future direction. Completion does not authorize another milestone.

## Goal and final scope

Complete the original bounded Myra request -> Phirna collection -> Myra return
exchange: consume one Root, clear the request, deliver actual character-held
rewards, inspect live ownership and preserve the result through production F9
saving and a separate CLI restart.

| Stage | Accepted responsibility |
| --- | --- |
| 21A | Authoritative complete item records, original loading, tail capacity/explicit compaction, save v2 and narrow v1 compatibility |
| 21B | Bounded reward delivery/finalization, warning/receipt, live inspection and save guards |
| 21C | Checked quest-item consumption, quest-flag clearing and deterministic GiveEnchanted through that lifecycle |
| 21D | Genuine original exchange, production disk/restart acceptance and physical-window acceptance |

Non-goals: item use, new equipment effects, equip/unequip, shops/trading, random
loot, gold/gems, combat, a quest journal, Darkside/Swords support, a full item
name/effect database, suspended-execution saves, a general migration framework,
normal-route certification and later milestones. Original resources remain
external and read-only.

The accepted design extends existing character, party, interpreter, Flow and
save abstractions. It adds no parallel inventory/treasure owner, service graph,
exchange transaction, capability toggle, Myra-specific script shortcut or gameplay RNG.

## Original-data and reference contracts

Reference findings use the pinned ScummVM revision in
[dependencies.md](dependencies.md): `engines/mm/xeen/scripts.cpp`
(`cmdTakeOrGive`, `cmdGiveEnchanted`, `checkEvents`), `party.cpp`
(`giveTreasure`, `arePacksFull`, `giveTreasureToCharacter`, `giveTake`),
`item.h/.cpp` and `character.cpp`. These findings explain the bounded choices;
accepted MMModern defensive adaptations are distinguished below.

The validated original Clouds roster contains 30 records of 354 bytes
(10,620 bytes total), 35 occupied item records and no miscellaneous items.
Its active order is `[0,18,14,11,1,6]`; active characters are eligible and every
category tail is empty. These are installation-specific prerequisites, not
assumptions for arbitrary loaded sessions. Original-party acceptance compares
all four bytes of all 1,080 item slots, including inactive owners.

### Myra script and exchange

Myra is on Clouds map 23 `(9,11)` West; Phirna is on map 23 `(8,2)` North.
Myra's event file has 170 records; the relevant original records are 21..35,
offsets 182..315. Operand bytes below are hexadecimal; other quantities are decimal.

| Lines | Original operation / operands | Accepted meaning |
| --- | --- | --- |
| 0 | If2 `15 63 07` | Any nonzero party Root count (item 99, counter index 17) branches to 7. |
| 1,4,5,6 | If1 `09 00 04`; NPC request; TakeOrGive `00 00 68 02`; Exit | With no Root and nonempty party, unsigned current SP >= 0 selects the request. Final NPC acknowledgment sets Q2. Five dispatched instructions; lines 2/3 are not a normal low-SP alternative. |
| 7 | NPC `01 03 11 01 08` | Original mode-1 return text; no consumption on opening/intermediate pages. |
| 8 | TakeOrGive `15 63 00 00` | Consume exactly one party Root after final return acknowledgment. |
| 9 | TakeOrGive `68 02 00 00` | Clear quest flag 2, independently of its prior value. |
| 10..14 | Five GiveEnchanted `46 25 00 01` | Produce five independent miscellaneous records `{10,37,1,0}`. |
| 15 absent | Natural script end | Finalize rewards without requiring an explicit Exit; nine dispatched instructions in the return branch. |

The original script checks Root possession, not Q2. It contains no permanent
completion marker, Myra Remove or event disable. With zero Roots, either initial
Q2 value ends at Q2=true after request acknowledgment. With N>=1, return ends at
N-1/Q2=false and produces five rewards subject to delivery. More Roots permit
fresh returns; after exhaustion the request resumes. Capacity/eligibility loss
does not refund consumption or change this revisit behavior.

Phirna's original Yes/acknowledgment grants one Root and removes object
`{Clouds,23,13}` plus events `{Clouds,23,125..135}`. No/already-owned refusal
grants nothing and leaves the plant present; completed collection cannot repeat.
Removal is independent of later Root consumption. The primary exchange obtains
its Root through this script, never by fixture injection.

### Deterministic item meaning

For the reference non-Swords miscellaneous branch, code 70 minus 60 gives
material 10 and the next byte gives special ID 37. Only those first two bytes
are consumed on this branch: suffix `00 01` is not state, frame, quantity or
charges. Material 10/11 deterministically sets one charge; other miscellaneous
materials use randomness and are outside scope.

The clean record is `{material=10, id=37, state=1, frame=0}`: low six state bits
hold the counter, bit 6 is cursed and bit 7 broken. The reference full description
is potion of antidotes; no antidote effect is implemented. Reference names and
treasure strings come from generated `CONSTANTS_7` tables in separate `mm.dat`
engine data, not Myra event text. Numeric MMModern feedback avoids hard-coding
original names or adding that data adapter/dependency.

## 21A item ownership and storage

`XeenCharacter` owns weapons, armor, accessories and miscellaneous arrays, each
with nine `XeenItem` records of four uint8 fields in material/ID/state/frame order.
The complete records replace the old modifier-only arrays; no synchronized copy,
occupancy mask, unique-item handle or modifier cache is introduced. Existing
rules retain their material/state/frame behavior, with no new ID-nonzero gate
or miscellaneous effects. Inspection identity is roster/category/slot at that
moment; compaction can change slot positions.

Original loading reads category offsets 166/202/238/274 without changing the
source CHR format. Preserve every byte and slot, including inactive owners,
holes, unknown values and metadata on ID-zero empty slots. All uint8 values are
valid opaque storage; unknown bytes must not index effect/name tables. Supported
production is narrower than storage.

`xeenItemHasTailCapacity` checks only slot 8's ID. Earlier holes do not provide
capacity when the tail is occupied. `xeenCompactItems` explicitly compacts one
fixed-size category in stable occupied order and clears remaining empty records,
including metadata. Loading, capture and restoration never compact.

Active membership remains an ordered vector of up to six roster references;
duplicates are aliases, not copied or deduplicated inventories. Checked access
and current owner resolution preserve that relationship during insertion and
inspection. Other categories, inactive owners and occupied bystanders remain exact.

## Save v2 and legacy v1 compatibility

M21 writes **v2**, reads **v1/v2**, and introduces only one narrow legacy case.
The [M20 format](milestone-20-plan.md#4-concrete-format-mmmodern-clouds-save-v1)
owns the unchanged envelope/payload contract; M21 changes the character item block.

- Retain the 20-byte header: `MMMSAVE\0`, u16 version (now 2), Clouds side 0,
  reserved 0, u32 payload length and u32 payload CRC32, with little-endian encoding.
- Keep roster ID, bounded name, sex/race/class, six attribute i32s, level,
  temporary level/age i32s and five booleans in their existing order.
- Replace the 81-byte v1 equipment modifier block with **144 bytes**: weapons,
  armor, accessories, miscellaneous; slots 0..8; material/ID/state/frame per slot.
  No counts, padding, pointers or handles appear within this block.
- Keep current HP/SP i16s, 16 condition bytes and birth year u16 afterward.
  Resource fingerprints and membership precede the roster; 35 u32 counters,
  30 quest flags, 256 game flags and sorted independent removal arrays follow it.
- Each character grows by 63 bytes (fixed portion excluding name: 149 -> 212);
  the same state grows by 1,890 bytes overall. The independent minimal v1/v2
  golden fixtures are 4,957/6,847 bytes.

Retain the 4 MiB file bound, fixed roster count, membership bounds/duplicates,
canonical booleans, name bounds, checked reads/arithmetic, exact EOF, CRC,
ordered identity validation, archive compatibility and active-rule/composition
preflight. Item bytes, holes and ID-zero metadata round-trip exactly; valid byte
changes are not structural corruption. No raw CHR/PTY or commercial payload is saved.

`XeenSaveItemState` is a transient snapshot presence discriminator, not a wire
field or live owner. Structural validation accepts unresolved v1 for decode,
read, restore and existing-target validation; v2 encoding rejects unresolved
legacy input. Unknown versions fail before payload interpretation.

Restoration checks resource signatures and loads initial metadata into an
unpublished candidate. Before whole-character replacement, resolve v1 by roster slot:

1. Preserve every saved modeled value, including changed material/state/frame
   modifier triples, HP/SP, membership and all independent state categories.
2. Supply only absent equipment IDs and the complete absent miscellaneous array
   from the matching original initial character.
3. For v2, saved arrays win in full, including explicit empties; do not merge defaults.
4. Validate/preflight all owners before publication. Failure preserves destination
   owners/caches and exposes no gameplay window. Recapture yields complete v2.

This reconstructs fields v1 could not model; it does not infer hypothetical
external-tool inventory edits, heal characters or replay grants. New item IDs
and miscellaneous arrays are durable; queue, preference, phase, receipt,
diagnostics and UI remain transient.

Existing file publication is retained: encode before I/O, structurally validate
an existing target, create/flush/close a sibling temporary, then replace safely.
A valid v1 target is supported for v2 replacement without resolving its old
inventories or matching the old target's resources; the new capture receives
resource-aware preflight. Reading/startup leaves v1 disk bytes untouched; only
explicit eligible F9 upgrades the loaded path. Invalid/unknown targets remain
protected and handled failures preserve old bytes. Windows directory identity
and installation-containment protections remain those of M20.

## 21B bounded reward lifecycle

### Production, recipients and insertion

`XeenPendingRewards` is a fixed ten-entry miscellaneous queue in execution state,
not live inventory. Each execution starts empty with no preferred recipient;
Myra does not use WhoWill. The typed helper accepts opaque nonzero IDs, rejects/counts
ID-zero input and counts overflow without overwrite or early delivery. Counters
saturate; script production remains within the existing 1024-instruction budget.
This helper domain is broader than the supported GiveEnchanted opcode domain.

Delivery resolves authoritative owners before an allocation-free mutation segment.
For each item in production order, try the valid preferred active index, then
scan current active order using current eligibility and miscellaneous tail capacity.
Accepted multi-member WhoWill sets the separate reward preference; ordinary
condition selection, single-member automatic selection and cancellation do not.
The index is interpreted against current membership, not a cached character copy.
Aliases observe every insertion and cannot multiply available capacity.

Eligibility uses existing `canAct()` and the worst-condition predicate: Asleep,
Paralyzed, Unconscious, Dead, Stoned and Eradicated are excluded. HP/SP, class,
sex and lesser conditions add no gates. Worst-condition masking is preserved;
do not replace it with an any-disabled-bit rule. Inactive owners receive no rewards.

Insertion requires an empty tail, writes miscellaneous slot 8, then explicitly
compacts. Occupied fields/order remain intact. If the first owner has two slots,
it gets two rewards and the next eligible owner gets the remainder. With verified
original capacity all five Myra rewards reach roster 0, miscellaneous slots 0..4.

Global fullness means all four category tails of every active reference are full,
regardless of eligibility. Misc-only fullness or ineligible spare capacity does
not imply that warning. Unlike the reference's vacuous empty-party result, the
accepted helper returns no global warning and no recipient for empty membership;
production rejects an empty party before enqueueing. Loss reasons distinguish
empty party, no eligible member and eligible miscellaneous tails full. No item
is displaced and no quest mutation is refunded.

### Finalization and mutation boundaries

Explicit Exit, natural missing successor, terminating presentation, WhoWill
cancellation and TeleportAndExit funnel through one finalizer. Ordinary suspension
and Call/Return retain queued rewards; Return with a call frame resumes its caller.
Invalid targets/returns and execution errors do not finalize as successful exits.
Delivery/presentation adds no interpreter instructions.

| Phase | Contract |
| --- | --- |
| Running | Queue survives suspension and cache reload. Original quest mutations occur at their instructions, independently of delivery. |
| Optional Warning | If globally full, show a blocking pre-delivery warning. No insertion/discard until acknowledgment. |
| Synchronous delivery | Insert once into live owners, account for losses, clear production and establish a fixed typed receipt before formatting/callbacks. |
| Receipt | Show actual delivery/loss results; remain blocked even if all rewards were lost. Paging, redraw and rebasing cannot redeliver. |
| Final acknowledgment | Discard transient receipt, return Completed and allow EventSystem to publish working camera/game flags once. |

Unsupported/malformed execution errors and explicit abandonment discard undelivered
records with reason/count while preserving earlier immediate party, quest and
world mutations. After insertion, failures (including receipt drawing), abandonment
or shutdown remove only transient state and cannot requeue/refund. Working camera
and game flags retain their existing completion/error policy. Shutdown never saves.
This discard-on-error policy is an accepted defensive adaptation, not reference
abort-path treasure delivery or an exchange-wide transaction.

`XeenEventFlow` remains the noncopyable production continuation owner. It adopts
suspensions by nonthrowing move before refresh/reporting/layout; copied reports
are observations, not live continuations. Generation and response kind are checked
before consumption. Stale, duplicate or wrong-phase responses preserve the current
request; accepted responses consume their generation before resumption. Pending
replacement must explicitly abandon old work before executing new work. Cleanup
precedes error reporting even if a transient layer was never drawn. Manual
presentation errors remain recoverable; automatic errors remain fatal.

### Presentation, inspection and saving

Use the existing font/window/presenter path for paginated numeric reward receipts
and the separate capacity warning. Space/Enter/Escape advances one page or
acknowledges its final page; one input cannot acknowledge the following phase.
Movement/selection cannot dismiss rewards. Clear transient layers over the retained
underlay on completion/error/abandonment. Blocked navigation that clears a retained
label forces one recomposition even when camera/object state does not change.

Feedback contains actual owner IDs/resource-loaded names, category and raw item
fields, decoded counter/charges/cursed/broken bits and delivered/lost/overflow/
invalid counts. Name control bytes are displayed safely. English diagnostic
labels and one post-delivery paginated receipt are deliberate adaptations from
original timed treasure messages, names and audio; no nested loop is introduced.
Original Myra text/title/portrait still use resource providers.

I inspects live state only at idle: ordered active aliases, all 30 unique owners,
four categories, raw records, capacity/eligibility and Root/Q2. Setup prints the
same snapshot after valid composition and before initial automatic dispatch;
resume reports restored values before interaction. Inspection while blocked does
nothing. It creates no owner, writes no file and dispatches no event.

Application's active/dispatching/pending guards cover queued execution, warning,
delivery and receipt without an idle gap. F9 while blocked performs no capture,
I/O, advancement or queued save. Final acknowledgment or recoverable terminal
cleanup permits a new F9 under ordinary eligibility guards; surviving immediate
mutations are saved. Nonblocking labels are allowed but excluded from saves.
Fatal failure/shutdown is not saveable. The receipt and queue are never serialized.

## 21C checked exchange operations

Take-only mode 21 supports Clouds quest-item IDs 82..116, decrementing once per
party. Take-only mode 104 clears Clouds quest flags 0..29 idempotently; flag zero
is valid. Other pairs must be neutral in both mode and value, including omitted
complete pairs. Generic combinations, character-item removal and conditional
quest-flag Action 104 remain outside scope.

Before mutation, validate ID/index, nonempty membership, Clouds logical and
physical contexts, and a sequential line below 255. Logical/physical maps may
differ; there is no coordinate whitelist. Consumption has no `canAct`, HP/SP or
capacity gate. Zero uint32 counters produce `QuestItemUnderflow` without mutation
or subsequent execution, instead of the reference's blind decrement/wrap.
Successful consumption/clear remains immediate across subsequent errors.

GiveEnchanted opcode `0x2c` accepts codes **70/71**, special IDs **1..73**, and
**2..4 bytes**. Length validation precedes operand support; 0/1 bytes are truncated
and more than four are surplus. The typed operation owns zero-to-two suffix
bytes for diagnostics but ignores their values semantically. This envelope is
an explicit bounded adaptation of the permissive reference iterator.

After context/membership/line and dispatch-budget guards, zero-initialize one
record, set material=code-60, ID=special ID and state=1, leaving frame zero, then
enqueue once. Eligibility is a delivery concern. Even a full queue must validate
new operands; later valid productions count as ignored overflow. No RNG, quantity
or state/frame interpretation comes from the suffix. Sequential natural completion
after Myra line 14 reaches the existing finalizer.

## 21D production restart and acceptance boundary

`tests/SaveResumeIntegrationTest.cpp` extends the existing
`mmodern_save_resume_smoke` with the `myra-exchange` checkpoint and a physical mode.
It reuses Application input/save/startup, original providers and shared typed
comparators. No new executable, serializer, commercial-data CTest registration
or production correction was needed for this stage.

### Genuine producer and independent oracle

1. Start a fresh original session; independently check active order, original
   items, live eligibility and tail capacity. Complete Myra's original request
   (five instructions, Q2=true), then Phirna Space/Yes/acknowledgment (18
   instructions, one Root plus removal), then return to Myra. Only camera
   positioning is harness setup; no quest/reward/item/removal injection is allowed.
2. Opening return dialogue changes no Root/Q2/items. Final NPC acknowledgment
   consumes one Root, clears Q2 and reaches the nine-instruction reward receipt.
   Verify five exact delivered entries, no losses/overflow/invalid/discard and an
   empty production queue. Receipt paging remains blocking without duplicate delivery.
3. F9 during pending return or any receipt page must refuse without changing
   durable state, pixels/palette, page, generation, report counts or portrait
   timing at the handler boundary. Subsequent idle animation is not an F9 mutation.
4. After final receipt acknowledgment, an idle observation proves no deferred save.
   A **new production Application F9** must save successfully. Read the actual
   disk bytes independently and compare the complete typed expected v2 snapshot.
   End the producer process before starting any consumer.

Expected characters start from independently loaded defaults with only five
literal `{10,37,1,0}` records assigned to roster 0 miscellaneous slots 0..4.
The oracle does not call delivery/compaction/finalization, capture mutated owners,
or decode the actual save as its expectation. It includes original resource
signature; Clouds map 23 `(9,11)` West; unchanged membership/all modeled fields
and all item bytes; Root index 17=0; Q2=false; unchanged unrelated counters/flags;
only Phirna's object/event removal identities. Every other category/owner and
original geometry/entity/effective-event record remains unchanged.

### Restart, reconstruction and revisit

A distinct consumer uses `Application::playGameplay(..., resume=true)` on the
producer file. Restored owners precede first composition/show; there is no retained
NPC/receipt/selection/timing or initial automatic dispatch. Inspect all five
items/owners, Root=0/Q2=false, removal and camera before another gameplay event.
An additional actual `mmodern --load-game` process checks the same file through
production CLI, setup diagnostics and a process-owned Windows SDL window.

The exchange consumer performs **one combined cache eviction** before its first
Myra revisit: map/object, script, text and sprites, followed by `refresh(true)`.
A Castle question/No control exercises genuine script/text providers where
removal alone cannot require text. Loader/construction counters and typed/frame
comparisons prove real reconstruction with unchanged exchange state. The new
process reconstructs owners; refresh reconstructs caches/composition, not owners.
No second save/restart cycle is part of this bounded acceptance.

Only afterward, Myra's no-Root revisit completes a new request, sets Q2=true and
adds no reward/removal. Phirna remains removed. A separate fresh process restores
original defaults, objects and normal initial dispatch without reading the save.
Consumers, fresh controls and CLI leave producer bytes unchanged. Clean Myra
resumed/fresh frames may match despite distinct typed quest/item state; dialogue
appearance alone does not prove Q2 persistence.

The existing Phirna, Bone Whistle, Myra-request and cumulative checkpoints retain
their pre-exchange producer meaning and individual-plus-combined cache controls.
Synthetic/labeled fixtures separately cover multiple Roots, all key variants,
partial/full capacity, ineligibility, aliases, malformed operands and cleanup.
They do not claim additional Roots from the nonrepeatable original Phirna plant.

### Physical acceptance interface

```text
mmodern_save_resume_smoke --manual-myra-exchange <game-directory> <new-save-path.mmsave>
mmodern --load-game <game-directory> <same-save-path.mmsave>
```

Use an existing parent outside the installation and a new target. The physical
producer uses one continuous real `SdlWindow::showInteractive` loop with no key
injection, automatic acknowledgment/save or automatic closer. Camera positioning
occurs only after completed Myra-request and Phirna callbacks. The maintainer
performs original inputs, observes pending-return/receipt F9 refusal, completes
the receipt, presses a new F9 and fully exits before separate CLI resume.
Startup/I inspection precedes Myra revisit and confirms unchanged rewards afterward.

Early exit or a skipped required observation cannot pass the producer. The
physical gate is this focused sequence, not a manual capacity/all-key matrix,
third fresh process or normal-travel certification; automated fresh controls
provide contamination coverage.

**Superseded design:** a proposed GraphicsSmokeTest exchange mode was not used.
Its existing `resume` oracle expects Phirna with a Root, which is incompatible
with a completed exchange. The existing save/resume coordinator owns exchange
acceptance; Graphics `save-phirna/resume` and `save-idle/resume-idle` remain controls.

## Validation responsibilities

- Character/rule/party and save-format/state/file tests cover all item bytes,
  aliases/inactive owners, holes, compaction, independent v1/v2 wire fixtures,
  legacy resolution, failed publication and protected replacement.
- Decoder, quest, event and reward tests cover bounded operand/context domains,
  underflow, overflow, terminal paths, immediate effects, recipient policy and
  exactly-once delivery. Reward CTest registrations are `xeen_item_reward`,
  `xeen_reward_execution`, `xeen_reward_flow`, `xeen_reward_gameplay` and `xeen_reward_sdl`.
- Flow/presentation/Application/SDL/save tests cover generation rejection,
  cleanup on both sides of insertion, retained layers, F9/I guards and resumed owners.
- Original `mmodern_party_smoke`, `mmodern_myra_smoke`, `mmodern_phirna_smoke`,
  `mmodern_who_will_smoke`, `mmodern_save_resume_smoke` and Graphics controls
  establish original loading/exchange/restart behavior separately from ordinary CTest.

## Final acceptance

All M21 stages are accepted and the milestone is complete. The final independent
review reported no remaining findings and confirmed preservation of the accepted
storage, mutation, reward, save and continuous physical-harness architecture.

The recorded closure validation passed the pinned-dependency Debug build and
required smoke builds, **58/58 unique full CTest registrations**, original-party
comparisons, Myra/Phirna/WhoWill direct and SDL controls, both five-checkpoint
restart coordinators and Graphics save/resume controls. Separate Application
producer/consumer/fresh and actual Windows SDL CLI processes passed. Native
receipt and clean resumed/rebuilt images were inspected separately from automated
pixel/state checks. Original archives and the pinned dependency remained unchanged.

**Maintainer-performed physical acceptance:** the supplied successful sequence
observed original request/collection/return, pending F9 refusal, completed receipt,
explicit F9 save, full producer exit and a separate production CLI resume with a
clean scene and no replay. Live inspection showed five roster-0 miscellaneous
records in slots 0..4, each M=10 ID=37 S=1 F=0, with slot 5 empty. The subsequent
no-Root request left those rewards unchanged. This establishes the focused human
window boundary, not manual inspection of every durable field or travel certification.

**Independent review:** separately verified automated/native evidence and decoded
the successful physical save, confirming complete byte equality with automated
and independent-audit exchange saves. Typed assertions establish Root/Q2 and
all unrelated-state preservation. Physical observations are attributed to the
maintainer, not to the reviewer; neither automated Windows SDL nor image inspection
is represented as a human window test.

The final scope and persistence/reconstruction boundary above are closed. General
inventory/equipment use, broader rewards and uncertified routes remain excluded.
