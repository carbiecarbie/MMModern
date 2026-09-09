# Milestone 21 - Myra's return exchange and bounded item rewards

**Status: 21D accepted; Milestone 21 complete and the current stable completed
milestone.** Section 18 records Gabriel's successful required physical acceptance
and Astra's independent **APPROVE 21D FOR FINAL DOCUMENTATION/CLOSURE**, with
no P0/P1/P2/P3 findings. 21A/21B/21C are committed. Gabriel supplied Astra's
**APPROVE 21C FOR COMMIT**, with no P0/P1/P2/P3 findings, before `f80ed51`.
Section 16 preserves 21C implementation-time evidence; section 17 records the
bounded 21D work and supersedes earlier restart/physical harness proposals.
The final human-evidence and independent-approval gates are satisfied. Earlier
implementation-time and pre-review statements below are historical and are
superseded by section 18's closure; they do not authorize later milestones.

The previous current-status statement, "21B implemented, independent review
pending", is historical and superseded by section 15. Sections 13 and 14 retain
the original pending-review evidence and the P2 correction history.

Historical planning authorization was limited to 21A: character storage, initial
loading, tail-capacity/explicit compaction, save v2, narrow v1 restoration and
existing-target compatibility in sections 4 and 7. Section 12 preserves that
implementation evidence and its then-pending review status.

Recommendation: retain one milestone with four small, separately authorized
stages. The necessary prerequisite is bounded character item storage plus its
save extension, not a general inventory/equipment/treasure engine. No substantial
separate roadmap milestone is justified by the inspected path. This conclusion
depends on approving the bounded presentation and error policies below. The
roadmap is unchanged; implementation must not start automatically after review.

In this document **Verified** means inspected current code, pinned reference or
the focused original-data checks recorded below. **Decision** means a recommended
MMModern contract awaiting approval except for the separately authorized 21A and
bounded 21B policies and the explicitly authorized 21C scope in section 16. Future acceptance is not a claim of tests
already passing. Paths are relative to this repository unless identified as
reference paths.

## 1. Baseline, authority and scope

Verified baseline at `D:/Projetos/MModern/mmodern`:

- Branch `main`; HEAD `3625380b71a75e386cd0ecac588c58abf1c9f6a8`.
- Recent history: `3625380` reorganizes current status/history, immediately after
  `a449bb7` closes M20C/M20; then `3ca9582` M20B and `4d65e34` M20A.
- Initial `git status --short` was empty. There was no pre-existing work to move
  or modify. No branch/history/dependency operation was performed.
- M20 is the current stable completed milestone. Its recorded Debug build,
  53/53 CTest, original-resource restart acceptance and user-supplied physical
  checks remain historical evidence, not validation of M21.
- `build/20a/CMakeCache.txt` points to ScummVM source
  `D:/Projetos/MModern/scummvm-known-good-candidate` and library build
  `D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`.
- That source checkout's actual HEAD is the documented pin
  **`6814ee9ba54582f5b5adcffab49efbbd8f589edd`**; status is empty when checked
  with command-local `safe.directory` and `core.autocrlf=false`.

Authority read first: `AGENTS.md`, `docs/project-status.md`, `docs/roadmap.md`,
`docs/dependencies.md`; relevant M19 behavioral evidence and M20 ownership,
wire format, restoration and acceptance contracts. M19's old unsupported return
frontier describes the implemented baseline, not a prohibition on an approved
M21. M20's heading still says its reviewed candidate was pending commit; actual
history and current status establish that it was committed. The roadmap's
statement that M21 has not begun predates this draft and does not authorize
implementation. No material code/approved-contract conflict was found.

Goal: collect the Root through original Phirna, execute Myra's original return
script, consume one Root, clear the request, deliver actual bounded items to
roster characters, inspect their fields/owners, and preserve the result through
production F9 and a separate `--load-game` process. Preserve original requests,
capacity losses and revisits. Checkpoint positioning is not route certification.

Non-goals: item use, new equipment effects, equip/unequip, shops/trading,
generated loot, combat, gold/gems, quest journal, Darkside/Swords support, a full
item database, suspended-execution saves, a general migration framework, and
later milestones. Preserve commercial resources read-only.

## 2. Evidence map and current implementation

The following are the inspected implementation seams, not proposed parallel
owners. Headers and corresponding implementation files were followed as needed.

| Responsibility | Current code / tests and consequence |
| --- | --- |
| Character records/rules | `src/games/xeen/XeenCharacter.{h,cpp}`, `XeenCharacterRules.cpp`; `src/formats/xeen/XeenCharacterFormat.cpp`; `tests/XeenCharacterFormatTests.cpp`, `XeenCharacterRulesTests.cpp`, `PartyIntegrationTest.cpp`. Three arrays contain nine `{material,state,frame}` modifier records; IDs and all miscellaneous records are absent. Rules must continue reading the same three fields with the same semantics. |
| Ownership/loading | `XeenParty.{h,cpp}`, `XeenPartyLoader.cpp`: 30 roster values; active membership is an ordered vector of up to six roster references, including duplicates. Loading diagnoses duplicates without copying or deduplicating characters. Quest counters and flags are separate party values. |
| Event semantics | `XeenEventDecoder.{h,cpp}`, `XeenEventInterpreter.{h,cpp}`, decoder/interpreter, presentation, quest-item/grant/flag tests. TakeOrGive parses up to three pairs with omitted complete pairs neutral. Only bounded grants, quest-flag set and game-flag set/clear currently execute. `0x2c` is unsupported. |
| Completion and presentation | `XeenEventSystem.cpp`, `src/app/XeenEventFlow.{h,cpp}`, `XeenEventPresenter.{h,cpp}`. System publishes camera/game flags only on Completed; party effects are immediate. Flow owns pending value continuations and consumes a generation before resume. `blocksGameplay()` currently tests `_pending`; there is no reward finalization phase. Presenter supports NPC paging/acknowledgment but needs a transient reward receipt kind. |
| Persistence | `XeenSaveSnapshot.h`, `src/formats/xeen/XeenSaveFormat.{h,cpp}`, `XeenSaveState.cpp`: v1 only. Capture copies all roster characters. Restore loads initial metadata, then **replaces whole characters** with saved values before unpublished preflight/publication. Defaults added only to the initial loader would consequently disappear on resume. |
| Production boundary | `src/app/XeenGameplay.cpp`, `Application.cpp`, `XeenGameplayServices.h`; `src/platform/XeenSaveFile.cpp`. F9 is intercepted before Flow; active/dispatch/pending guards precede capture, candidate restoration and write. Resume uses the same Application path, restored owners before first frame, zero initial event dispatch. Existing-target write calls `read(path)` before creating a temporary file. |
| Original acceptance | `tests/MyraIntegrationTest.cpp`, `PhirnaIntegrationTest.cpp`, `SaveResumeIntegrationTest.cpp`, `XeenCheckpointTestSupport.h`, `XeenChildProcessTestSupport.h`. Current Myra root cases deliberately assert the line-8 unsupported boundary. Restart tests compare characters to initial defaults; exchange coverage must replace that oracle with actual expected item changes. |
| Comparison/negative tests | `XeenPartySnapshotTestSupport.h::checkSameCharacter/partySnapshot`, `XeenSaveTestSupport.h::sameSnapshot/sample`; save format/state/flow/file/SDL/CLI tests. Extend both character comparisons, not just the new exchange assertions. `XeenSaveFormatTests.cpp::golden()` is already an independent 4,957-byte v1 fixture with a bitwise CRC oracle. Preserve it unchanged as the old-format oracle. |

Reference paths below are within the pinned ScummVM source above. Function
locations are one-based at that revision:

| Reference location | Verified purpose |
| --- | --- |
| `engines/mm/xeen/scripts.cpp:38`, parameter iterator; `:134`, `Scripts::checkEvents` | Missing operand reads return zero; dispatch initialization, natural termination and final treasure delivery. |
| `scripts.cpp:618`, `cmdTakeOrGive`; `:891`, `cmdExit`; `:1142`, `cmdWhoWill`; `:1279`, `cmdGiveEnchanted`; `:1650`, `ifProc` Action 21 | Operand widths, take/grant dispatch, recipient designation, production and possession branch. M19 records NPC/Action-9 behavior. |
| `engines/mm/xeen/party.cpp:71`, `Treasure` construction/clear; `:699`, `giveTreasure`; `:816`, `arePacksFull`; `:836`, `giveTreasureToCharacter`; `:883`, `giveTake`; `:1142/:1476`, take/give 104 | Temporary slots, capacity, recipients, insertion/feedback/ack, immediate quest changes. `party.h:60` gives ten treasure slots/category; quest counters are `int`, not a protected decrement API. |
| `engines/mm/xeen/item.h:34` and `XeenItem::empty`; `item.cpp:30/:59`, state/item serialization; `:227/:282`, inventory sort/full; `:90`, `getItemName`; `:761`, misc description | Nine slots/category, ID-zero emptiness, four-byte records, compaction rather than value sorting, short reward name versus full description. |
| `engines/mm/xeen/character.cpp:168`, `synchronize`; `:251/:267`, worst condition/disabled-or-dead; `dialogs/dialogs_whowill.cpp:37`, `WhoWill::execute` | Item ordering/eligibility and the separate successful-selection reward preference; single-member WhoWill returns before assigning that preference. |
| `engines/mm/xeen/resources.cpp:62`, `loadData`; `:280` item names; `:382` reward strings; `files.cpp:72` engine-data setup | `CONSTANTS_7` English tables in separate `mm.dat` engine data; not Myra event-text entries. |
| `devtools/create_mm/create_xeen/en_constants.h:999/:1008` | MISC_NAMES and SPECIAL_NAMES provenance; material 10 is potion, special ID 37 is antidotes. Generated constants, not an existing MMModern resource adapter. |
| `engines/mm/shared/xeen/cc_archive.cpp`, index/name lookup; `engines/mm/xeen/files.cpp`, `SaveArchive::reset` | Read-only original initial-container reconstruction already used by MMModern's bridge. |

### Focused original-resource observations made in this planning task

M19's recorded Myra resource evidence was used first. Running the existing
`build/20a/mmodern.exe --inspect-events <game> 23 9 11 west` confirmed the same
170-record map, original records 21..35, offsets 182..315, operands below and
absence of line 15. No broad event census was performed.

A transient Python command read `xeen.cc` into memory and followed the existing
bridge/pinned archive rules: initial blocks `2a0c,2a1c,2a2c,2a3c,284c,2a5c`,
then only `maze.chr` and `maze.pty`. It checked record bounds and reported item
occupancy, not extracted assets. The 10,620-byte roster has **35 occupied item
records** across all 30 characters; no holes and no nonzero metadata on ID-zero
slots in this installation. Active order and occupied counts are:

| Roster ID | Weapons | Armor | Accessories | Miscellaneous |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 1 | 4 | 1 | 0 |
| 18 | 1 | 4 | 1 | 0 |
| 14 | 2 | 3 | 1 | 0 |
| 11 | 2 | 3 | 2 | 0 |
| 1 | 1 | 2 | 2 | 0 |
| 6 | 1 | 2 | 2 | 0 |

Every active category's final slot is empty. This establishes initial capacity,
not a rule that all future sessions have empty misc packs. Current original-party
tests record these active characters as Good; M21 acceptance must recheck live
eligibility/capacity immediately before the exchange.

Focused outer-Clouds lookups did not find `CONSTANTS_7`, `CONSTANTS_0`,
`MISC_NAMES` or `SPECIAL_NAMES`; the pinned source establishes the separate
engine-data provenance. This is not a claim to have surveyed every executable
or archive for embedded text. No original text/name database is copied here.
The attempted existing `build/20a/mmodern_party_smoke.exe` run could not start
because that optional binary is absent; its source/target were inspected instead.
No build or suite was run for this documentation task.

## 3. Complete bounded original behavior

### 3.1 Branches and quest mutations

Bytes in this subsection are hexadecimal; numeric meanings elsewhere are
decimal. M19 section 3 and the repeated diagnostic establish this sequence:

| Lines | Operation / operands | Consequence |
| --- | --- | --- |
| 0 | If2 `15 63 07` | Party quest-item possession for item 99, not count equality with 99; any nonzero Root count goes to 7. |
| 1,4,5,6 | If1 `09 00 04`; NPC request; TakeOrGive `00 00 68 02`; Exit | With a nonempty party and no Root, unsigned SP >= 0 selects line 4; final NPC acknowledgment then sets Clouds Q2. Five instructions. Lines 2/3 are not an ordinary line-0 alternate based on low SP. |
| 7 | NPC `01 03 11 01 08` | Original return text, mode 1, final acknowledgment, then fallthrough. No Root consumption on opening or intermediate pages. |
| 8 | TakeOrGive `15 63 00 00` | Take-side mode 21, value 99, neutral give and omitted neutral third pair. Decrement party quest counter index 17 by **one**, not all Roots and not a character-held Root. |
| 9 | TakeOrGive `68 02 00 00` | Take-side mode 104 clears request flag 2, independently of possession and prior flag value. |
| 10..14 | Five GiveEnchanted `46 25 00 01` | Produce five independent items, once each. No loop, stack grant or quantity operand. |
| 15 absent | No record | Natural end of script; reference `checkEvents` still calls `giveTreasure`. No explicit Exit is required. Nine dispatched instructions for the return branch. |

Reference mode 21 uses global party counters for IDs >=82 (non-Swords); lower
IDs search the selected character's category, clear the first match and compact,
returning failure when absent. Character item removal is outside this draft.
For quest items it blindly decrements an `int`; zero can become -1 when entering
the take line directly. It does not reject an absent Root. The normal line-0
possession branch avoids this. Serialization later stores a byte; do not imitate
negative/wrapped state in MMModern's uint32 counter.

Reference mode 104 uses `files._ccNum * 30 + value` (Swords has a different
offset). Clear has no equivalent of the give-side range assertion. Mode 104
explicitly bypasses `howMuch()` when the value is zero: flag zero is legitimate.
Neither operation is gated by character consciousness. `giveTake` still accesses
the selected active character, default first member for this script. Reference
combined take/give equal values can invoke randomness and zero-valued take modes
can prompt for an amount; those combinations are outside the bounded domain.

**Decision:** support take-only mode 21 for Clouds IDs 82..116 and take-only
mode 104 for Clouds indices 0..29. Require neutral other pairs, nonempty active
membership, Clouds logical **and** physical working context, and a valid
sequential successor number before mutation, following current grant/set checks.
Logical/physical maps may differ; no Myra coordinate or exact tuple whitelist.
Root underflow returns a specific bounded execution error without changing the
counter or executing following lines. Invalid ID/flag/context/line similarly
fails before this instruction's mutation. These are defensive decisions, not
claims that the reference validates them. Add decrement/clear to the existing
party value APIs; apply once per party, including duplicate membership.

No permanent completion flag, Q2 test, object Remove or event disable appears
in Myra's script. With 0 Roots, either initial Q2 state ends in Q2=true after the
request. With N>=1, a completed return ends with N-1 and Q2=false, whether Q2 was
initially true or false. With another Root, revisit runs the return again and can
produce another five items. Once none remain, revisit requests again and sets
Q2=true. Capacity loss does not refund Roots or suppress this revisit behavior.

### 3.2 Reward fields, bytes and temporary capacity

For non-Swords, item codes 60..81 select miscellaneous and subtract 60 into
`material`. `0x46`=70 therefore selects **material 10**. The next byte
`0x25`=37 is the misc special/effect ID. On this branch the reference consumes
**only the first two bytes**. The stored `00 01` suffix is unused: it is neither
item state, frame, quantity nor charges. Material 10 or 11 sets counter=1;
other misc materials draw a random number 3..10. The selected path is deterministic.

`XeenItem` construction/treasure clearing zeroes material, ID, state and frame.
The misc opcode sets material/ID/counter but does not reset other bits itself.
In the clean no-combat reward buffer, each resulting record in serialized order
is `{material=10, id=37, state=1, frame=0}`. State's low six bits are counter,
bit 6 cursed, bit 7 broken. Thus charge=1, uncursed, unbroken, frame=0. The
reference full description is potion of antidotes; the short treasure message
only names the misc material. This draft implements no antidote use/effect.

There are ten temporary slots per category. `cmdGiveEnchanted` finds the first
ID-zero slot. If all ten are occupied it warns and ignores the new item, then
continues; no overwrite or early delivery. On overflow that branch does not
even consume the second operand. Five Myra instructions fit an initially empty
buffer. Delivery clears the buffer later, not after each opcode.

**Decision:** a typed bounded GiveEnchanted operation supports item codes **70
and 71**, special IDs **1..73**, yielding material 10/11 and one charge. This
reuses the complete deterministic potion/scroll branch without restricting it
to ID 37 or adding random loot. Reject ID zero (empty marker), IDs >=74 and other
categories/materials as unsupported before enqueueing. No new gameplay RNG or
saved RNG state; existing NPC visual randomness stays separate.

Before enqueueing require nonempty membership, Clouds logical and physical
working context and no sequential line overflow, as for the new take forms.
No per-character action-eligibility gate applies to production: eligibility is
checked at delivery, so a wholly ineligible party can still consume the Root
and lose the produced rewards. Each new record is explicitly zero-initialized
before its fields are assigned; do not depend on stale queue-slot contents.

Decoder policy: require 2..4 bytes for this supported form; own the first two
and the zero-to-two unused suffix bytes for diagnostics. Accept arbitrary suffix
values without interpreting them. Reject 0/1 bytes as truncated and >4 as
surplus. Two or three bytes are valid because the branch requires only two.
This explicitly bounded envelope differs from the permissive reference iterator
and is a documented exception to the decoder header's exact-consumption comment.
Decode sizes before semantic support; validate even when the queue is full.
Tests must prove that changing the unused suffix cannot alter rewards. Existing
TakeOrGive widths, omitted complete pairs, truncated-pair and surplus rejection
remain unchanged. Unsupported combinations/modes remain explicit errors.

Use a ten-entry misc pending buffer (or bounded vector), preserving production
order. Count/log each overflow as an ignored reward while continuing, within
the existing 1024-instruction budget. Never broaden to four treasure queues,
gold/gems or combat merely to copy the reference `Treasure` class.

### 3.3 Delivery, capacity, eligibility and feedback

Reference `checkEvents` sets `_whoWill=0` and `_charIndex=1` independently. The
first selects no preferred reward recipient; the second selects the first
character for ordinary script conditions. A prior successful multi-member
WhoWill selection can designate a recipient; the single-member shortcut returns
without changing `_whoWill`. Delivery tries that eligible character, otherwise scans active order
for a recipient with room. The last loop's comment claims consciousness is
ignored, but **its executable predicate still tests `!isDisabledOrDead()`**.

That predicate uses the **worst** condition: Asleep, Paralyzed, Unconscious,
Dead, Stoned or Eradicated are excluded. HP/SP, class, sex and lesser conditions
are not additional tests. MMModern `canAct()` already implements this exact
worst-condition predicate. For example, a later-numbered Confused condition
can mask Asleep under the reference's worst-condition rule; do not replace it
with an any-disabled-bit test.

Inventory `isFull()` tests ID in **slot 8**. `arePacksFull()` tests slot 8 in
all four categories of every active member, regardless of eligibility. Empty
membership is vacuously full in that routine, although this draft rejects reward
production with no party. A party whose misc categories are full but whose
weapon/armor/accessory categories are not all full does **not** get the global
backpacks-full warning. Ineligible characters' spare slots can likewise make
global-full false while no reward can be delivered.

For each deliverable item, reference copies it into target category slot 8 and
then calls `sort()`: scan ascending slots, clear ID-zero records, move the next
nonzero-ID record into each hole. This is stable compaction, not sorting by item
ID/material/value. With a compact pack it appends in production order. If slot 8
is occupied but earlier holes exist, reference still treats the category as full.
No existing item is displaced for these misc rewards. The exceptional forced
weapon-slot clearing for a Xeen Slayer Sword is irrelevant to this domain.

With a suitable initial party all five go to the **first active roster ID 0**,
misc slots 0..4. They are five records, not five owners. If that character has
two available slots, it gets two and the next eligible member gets the rest.
When no eligible misc recipient remains, remaining rewards are discarded at
final cleanup. Global-full can clear all remaining treasure early. Both paths
lose undeliverable items without refunding or rolling back quest mutations.

The reference opens treasure feedback, warns/awaits input if globally full,
inserts each deliverable item **before** its owner/item feedback and pauses,
then requires a final key/mouse acknowledgment and clears temporary treasure.
Even all-undeliverable treasure reaches acknowledgment. Frame redraws, shooting
flags, combat target resets, mode values, chest animation bookkeeping, saved
gold/gem treasure, audio and fixed display pauses are transient or unrelated to
this no-combat slice. Do not import those subsystems.

## 4. Character model and mutation API

**Decision:** generalize the existing three character arrays into a single
typed record definition `{u8 material, u8 id, u8 state, u8 frame}`, and add a
fourth nine-slot miscellaneous array. Keep `weapons`, `armor`, `accessories`
under the same character; explicitly replace the modifier-only type and adjust
its callers/fixture initializers. There must not be an inventory copy alongside
the old arrays. Existing rules still use material/state/frame as before and do
not start applying miscellaneous effects or a new ID-nonzero equipment rule.
This is a deliberate compatible model extension, not a reinterpretation of the
old three bytes as an item ID or a separate modifier cache.

The extra 27 equipment ID bytes per character provide exact all-category
occupancy and preservation without a separate occupancy mask. Storing complete
four-byte records is smaller conceptually than keeping modifier arrays, item
IDs, category occupancy flags and synchronization rules as parallel systems.
No unique item handles are needed: identity for inspection is roster/category/
slot at that instant; compaction is allowed to change slots.

Loading reads four-byte records at character offsets 166 (weapons), 202 (armor),
238 (accessories), **274 (misc)**, nine each. Preserve every field and serialized
slot, including inactive roster characters. Do not compact, normalize, clear
ID-zero metadata, or assume initial packs empty at load/capture/restore time.
ID==0 means empty; other bytes still round-trip even in an empty record. All
byte values are valid opaque storage; unknown values must not index name/effect
tables. Item execution support is narrower than storage support.

**21A helper allocation:** only a read-only category tail-capacity query and
explicit stable category compaction. Slot 8's ID determines capacity regardless
of earlier holes. Compaction preserves occupied fields/order and clears the
remaining empty records, including their metadata, only when explicitly called.
The fixed nine-slot category type checks the boundary; neither helper accepts
unchecked category, roster or slot indices. Loading and persistence never call
compaction.

**Originally deferred to 21B; now explicitly authorized:** party-global tail-capacity scans,
insertion, recipient selection/eligibility, preferred WhoWill recipients,
pending treasure, delivery results and their lifecycle.
Use checked roster/category/slot access, `canAct()`, active-party order and
current authoritative records at insertion time. On a full target return a
defined no-insertion result with no changes. Insertion clears empty misc metadata
only as reference compaction does; pre-existing **occupied** item records retain
all fields and relative order. Other categories and bystanders remain exact.

Repeated membership references the same roster inventory. Repeated appearances
in the recipient scan do not multiply capacity or copy items: after an insertion
all aliases observe the changed slots. Tests must include reordered and repeated
IDs and a preferred recipient that aliases another active slot. Preserve the
existing membership vector; do not reject or deduplicate it for this feature.

## 5. Execution lifecycle and save-safe completion

**Decision:** extend the existing value-owned interpreter execution state with
the bounded pending misc rewards, optional preferred recipient and finalization
phase. It is transient execution state, not live party inventory. A preferred
recipient is absent on begin; an accepted multi-member WhoWill response sets it
separately from the default condition character. Single-member automatic
selection leaves it unchanged, as does cancellation. Reuse its active-party index semantics and
validate it against current membership when delivering. Myra uses no WhoWill.

All successful terminal paths must funnel through one bounded finalization
operation: explicit Exit, natural missing successor, termination continuation,
WhoWill cancellation and TeleportAndExit. Return with a call frame resumes the
caller, not delivery. Invalid explicit jumps/calls are still errors. Keep
instruction counts unchanged by delivery/presentation; Myra completes nine.

| Phase / transition | Required ownership and observable behavior |
| --- | --- |
| Begin / NPC suspended | Empty reward buffer. No consumption until final return-dialog acknowledgment; rebase/page/timer ticks do not execute opcodes. |
| Line 8 | Validate then immediately decrement one Root. No exchange transaction/refund. |
| Line 9 | Validate then immediately clear Q2, idempotently. Earlier Root consumption survives a later failure. |
| Lines 10..14 | Initialize/enqueue one bounded item each. No character insertion yet. Queue survives ordinary script suspension/Call/Return/cache reload within this execution. |
| Successful script terminal, queue present | Do not return externally stable Completed. Enter finalization. If globally full, suspend on a transient capacity warning **before** insertion/discard, then continue after acknowledgment. |
| Delivery | Scan/insertion/compaction runs synchronously once. Deliver eligible items in production order, discard undeliverable entries with explicit result reasons, clear the pending production buffer and retain a value receipt. Each successful insertion is an immediate roster mutation. |
| Receipt pending | Present actual owner/field results and loss count; await final acknowledgment even if zero items delivered. Pending state marks delivery already done. Paging/rebase/duplicate input must never deliver again. |
| Final receipt acknowledgment | Remove transient receipt, return Completed and let EventSystem commit working camera/game flags once. Clear execution state/generation. Now stable F9 is allowed. |

Original feedback uses synchronous timed messages; MMModern instead displays
one paginated receipt after synchronous delivery. This preserves insertion
before acknowledgment and the global-full pre-delivery warning while avoiding
nested event loops. Do not claim exact reference timing, audio or presentation.

### Error, abandonment and exactly-once policy

An MMModern unsupported/malformed execution error is not a reference script
termination. **Decision requiring approval:** discard the not-yet-delivered
queue on such an error or explicit abandonment, report how many pending items
were discarded and why, and retain earlier immediate Root/flag/world/party
mutations. Do not automatically finalize rewards on error, or silently treat
error as successful natural completion. This is a bounded defensive adaptation;
the reference `checkEvents` may distribute treasure after its abort paths.
No exchange-wide rollback or compensating Root grant is introduced.

Abandonment before return acknowledgment changes nothing; after consumption
but before delivery retains consumption/clear and discards queued work explicitly.
Abandonment/error after insertion (including receipt drawing failure) retains
all inserted items; discard only transient UI/receipt, never requeue/refund.
Working camera/game flags remain governed by the existing error/abandon policy.
A pending global-full warning abandoned before delivery also discards the queue.
Shutdown does not implicitly save; an earlier disk save is unchanged.

Extend Flow's existing cleanup/error handling beyond its current NPC-only catch
paths. `acceptManual/acceptAutomatic` must reject replacement of pending work
unless it first executes this explicit abandonment policy; `_pending.reset()`
alone is insufficient once it owns rewards. Reports must distinguish discarded
pending rewards from completed delivery and cannot imply success after error.

Flow remains the production continuation owner. Validate generation and response
kind before consuming pending work; obsolete, duplicate and wrong-phase input
returns false without changing the current pending generation/queue. For an
accepted response, consume its generation before resume.
Do not expose copied pending states as a user replay API. Interpreter unit tests
may copy values, but production guarantees are established at the Flow boundary,
as with current immediate grants. Rebase reconstructs receipt pages from owned
values and never invokes insertion. The receipt is not durable ownership.

F9 stays at Application's current synchronous boundary. `blocksGameplay()` must
cover execution, queued rewards, full-pack warning, delivery finalization and
receipt acknowledgment; there must be no visible idle gap between script end
and reward presentation. Insertion within a callback is also protected by
`dispatching`. Pending F9 performs no capture/I/O, advancement or queued save.
After explicit terminal error/abandonment cleanup, a new F9 may save the surviving
immediate effects; report the discard before exposing that idle state. Save
never serializes pending rewards, selected recipient, receipt or generation.

## 6. Presentation and useful inspection

**Decision:** use the existing presenter/font/window path for a transient reward
receipt plus the separate global-full warning. Use Space/Enter/Escape as explicit
acknowledgment/page advance, with final Escape acknowledging rather than exiting;
ordinary Escape exits only once no interaction is pending. Movement and selection
inputs do not dismiss the receipt. F9 retains its separate refusal path. Preserve
unrelated retained labels and remove the entire transient reward layer on finish,
error or abandonment. No new blocking loop or item animation scheduler.

The receipt lists successful recipients from live roster names and item numeric
fields, plus delivered/discarded/overflow counts and concrete loss reasons.
Owner names are resource-loaded character names, preserved by saves. Original
Myra NPC title/body/portrait still come through the current text/assets providers.
Use original `fnt` for rendering and existing callbacks/composition seams.

**Presentation adaptation requiring approval:** M21 uses English MMModern
labels and numeric item descriptions (category, material, special ID, charges,
cursed/broken, frame). It does not hard-code “potion of antidotes”, original
treasure prose, or generated ScummVM constants. Verified reference names above
are evidence, not proposed production strings. Native item names would require
a separate bounded resource adapter/provenance decision; they are not necessary
to prove actual ownership in this diagnostic slice. A full original-name UI is
not silently included as a prerequisite.

Add one bounded read-only **I-key inventory diagnostic** to the existing
PlayerAction/SDL/Application path. At idle it prints active order and unique
roster owners, all four categories/slots, raw fields and decoded state bits,
capacity/eligibility and Root/Q2 values to the console. Include the all-roster
records so inactive ownership is inspectable; aliases are labeled as references,
not duplicated holdings. Print an English summary in the existing window title.
During any pending interaction, refuse without advancing or changing state.
Do not dispatch events, grant items, trigger initial events or write files.

Emit the same bounded diagnostic automatically after successful new-session/
resume setup before the first gameplay input, and make it available after receipt
completion. Thus a user can inspect the production resumed result **before**
interacting with Myra again. Do not substitute `--inspect-party` (currently loads
initial state) or a test observer for inspection of live/resumed owners. A full
inventory screen and a separate inspection CLI are not needed. Tests may use a
formatting helper; no second inventory owner or persisted diagnostic text.

## 7. Durability, exact v2 wire and v1 policy

Durable new state: the 27 previously absent equipment IDs and all 36 misc
record bytes per roster character. Existing modifier fields remain durable.
Pending production/delivery phase, selected recipient, diagnostics, UI and cache
state remain transient. Snapshot remains a short-lived transfer object.

**Decision:** write binary **v2**, read **v1 and v2** with one explicit legacy
case. Do not append fields while retaining version 1, silently default to empty
packs, or introduce a general migration framework.

### v2 layout

Retain the M20 20-byte header (`MMMSAVE\0`, u16 version now 2, Clouds side 0,
reserved 0, u32 payload length, u32 CRC32) and all payload fields/limits/order
except the character item block. For each of 30 records in roster-index order:

1. Keep v1's rosterId, bounded name length/name, sex/race/class, six attribute
   i32s, level/temporary-level/temporary-age i32s and five booleans unchanged.
2. Replace the v1 81-byte modifier block with **144 bytes**: categories weapons,
   armor, accessories, misc; slots 0..8 in each; each slot exactly
   `u8 material, u8 id, u8 state, u8 frame`. No counts, pointers, padding or handles.
3. Keep current HP/SP i16s, 16 condition bytes and birthYear u16 unchanged.

After all characters retain 35 u32 quest counters, 30 boolean quest flags,
256 boolean game flags, sorted independent object/event identity arrays exactly
as v1. Resource fingerprints and active membership remain before the roster.
Each character grows by 63 bytes; every file grows by **1,890 bytes** for the
same old state/name/membership/removal payload. Minimal v2 counterpart to the
existing 4,957-byte v1 fixture is **6,847 bytes**. Roster/category/slot order is
deterministic; encode must not compact inventory.

Keep the 4 MiB file bound, fixed roster size, membership bounds/duplicates,
boolean checks, name bounds, checked reads/count arithmetic, exact EOF, CRC,
ordered identities, resource compatibility and active-rule/composition preflight.
All four stored item fields span 0..255; the state byte is an exact bit encoding.
ID-zero metadata and slot holes are valid preservable state, not corrupted data
to normalize. Unknown IDs/materials cannot index tables; numeric diagnostics
make this safe. There is no unverified equipment-effect semantic validation.
The mutation API separately restricts generated misc records to the supported
domain. Distinguish structurally invalid new item blocks from altered but valid
item values; a checksum-correct change to a legal byte is not a rejection case.

### Narrow legacy restoration

The decoder records a transient format-origin/presence discriminator (v1 absent
item additions versus v2 explicit records) in the transfer snapshot. This is
not another gameplay owner or an extra payload field. v1 reading uses its exact
old three-byte modifier layout; never read it with the v2 character parser.
Unknown versions fail before payload interpretation. Encoding is v2-only and
must reject an unresolved legacy snapshot; production captures resolved owners.

In `restoreBeforeGameplay`, after signature verification, load initial party
through the current provider into the unpublished candidate. Preserve its new
item defaults long enough to resolve v1 **by roster slot**:

- Restore every v1 modeled character value exactly, including all saved modifier
  material/state/frame bytes, even if unlike the original defaults.
- Supply only missing weapon/armor/accessory **IDs** from that slot's original
  initial character; supply the whole absent misc array from that initial slot.
- Keep saved membership/order and all other saved categories. No quest/item
  inference, no healing, no grant replay, no active-party-only overlay.
- v2's explicitly empty or changed arrays win in full; do **not** merge initial
  items into v2. Zero is a value, not a missing-field marker.
- Validate/preflight the completed candidate and publish only once all owners
  succeed. Recapture produces ordinary fully populated v2; the legacy marker
  never survives into live gameplay.

This reconstructs fields M20 could not mutate or save, from the required matching
installation, while preserving every field M20 did model. It intentionally does
not reconstruct hypothetical inventory modifications made by external tools to
a v1 save. Absent/default handling must occur before whole-character replacement
loses those initial additions. Failed v1 preparation must leave sentinel party,
camera, flags and world unchanged and must not expose a gameplay window.

### Existing-target replacement

`XeenSaveFile::write` encodes first, then calls `read` on an existing target,
then creates/flushes/closes/replaces a sibling temporary file. With the explicit
v1 decoder retained, a structurally valid v1 destination is a **supported valid
MMModern target** and may be replaced by v2 through the same protocol. Existing
target validation does not need to materialize old inventories or match the
target's resources; this remains the existing structural destination check.
The new capture undergoes full resource-aware preflight before write as today.

An actual v1 resume uses its load path for F9, resolving legacy defaults in
memory and replacing that file with v2 only on an explicit eligible save. Read
alone never migrates a disk file. Unknown versions and invalid v1/v2 files remain
protected from overwrite. Failure preserves old bytes and is not reported as
success. Reuse M20 file/path identity protections and failure tests; no new
filesystem abstraction. Do not serialize raw CHR/PTY or any commercial blob.

## 8. Tests by responsibility

Tests below are future requirements. Existing targets were verified in
`CMakeLists.txt`; new target names are proposals, not currently runnable tests.

| Owner | Required new coverage / existing coverage to reuse |
| --- | --- |
| Model/loading | Extend `xeen_character_formats`, `xeen_character_rules`, `xeen_party_visual_state` and explicit `mmodern_party_smoke`. All 30 four-category records/offsets; zero/nonzero metadata; initial occupied records; tails, holes and stable compaction; no changes to rule outputs. Proposed `xeen_item_rewards` / `mmodern_item_reward_tests` tests nine-slot capacity, ordered distribution, preferred fallback, every relevant worst-condition and mixed-condition case, HP independence, repeated references and unchanged bystanders. |
| Decoder/interpreter | Extend `xeen_event_decoder`, `xeen_event_interpreter`, `xeen_quest_items`, `xeen_quest_grants`, `xeen_quest_flags`. Supported first/last IDs/flags/specials/materials; required bytes vs ignored suffix; partial pairs, unsupported modes/categories, neutral third pair, Clouds logical/physical mismatch, empty party, line overflow. Root 0/1/3/UINT32_MAX and flag false/true; missing-root direct take rejects without underflow; earlier mutations survive later errors. Ten pending rewards plus overflow; nine Myra instructions and no phantom Exit. |
| Lifecycle/presentation | Proposed `xeen_reward_flow` / `mmodern_reward_flow_tests`, with existing `xeen_event_presentation`, `xeen_event_ui`, `xeen_event_system`, `xeen_manual_event`, `xeen_npc`, `xeen_who_will` as regressions. Every successful terminal path finalizes; Call/Return does not finalize early; ordinary suspension preserves queue; global-full warning before discard; misc-full-only no global warning; all-ineligible outcome. Insertion before receipt ack, paging/rebase no replay, duplicate/stale/wrong-phase response rejection, pre/post-delivery error/abandon cleanup, no pending replacement loss. Synthetic data supplies variants; do not duplicate this whole matrix in original-data tests. |
| Save format/state | Extend `xeen_save_format`, `xeen_save_state`. Actual wire round-trip of new fields in all slots/rosters, inactive/repeated members, legal byte boundaries/empty metadata/holes; independent v2 layout oracle. Preserve old `golden()` v1 fixture independently of encoder. Verify legacy origin, initial IDs/misc defaults but saved modifier overrides, explicit-empty v2 behavior, recapture-to-v2 and unresolved legacy encode rejection. Truncate/remove/insert item-block bytes with repaired envelope/CRC to exercise parsing, and legal altered item bytes that must round-trip. Late resource/preflight failure publishes nothing, including new fields. |
| File/Application/input | Extend `xeen_save_file`, `xeen_save_flow`, `xeen_save_sdl`, `xeen_save_cli`, `sdl_input`. Valid independent v1 target -> v2 replacement, failed replacement keeps exact v1 bytes, invalid/unknown target still refused. V1 production resume/F9 upgrades correctly. F9 refused through queued/full-warning/receipt phases with zero I/O or generation change; inspection does not dispatch/mutate, including first resumed output. Keep existing M20 path/alias/fault matrix rather than rebuilding it in exchange tests. |
| Original/Application restart | Extend `mmodern_myra_smoke`, `mmodern_save_resume_smoke`, `mmodern_graphics_smoke`; Phirna/WhoWill controls. Original acquisition/exchange/first resumed inspection below; actual file bytes and separate PIDs, plus real CLI and native/physical presentation. Extend shared `checkSameCharacter`, `partySnapshot`, `sameSnapshot` so all old regressions now observe new fields. |

**21C/21D only; keep unchanged in 21B:** replace Myra's successful root-path assertions for `UnsupportedOperationMode`
at line 8 (including the cumulative M20 restart test). Retain NPC failure and
abandonment before acknowledgment, unrelated unsupported opcode/mode cases,
no-root/SP/Q2 variants, final Escape, source diagnostics and cache regressions.
Do not globally remove expected errors or compare all characters to initial
defaults after exchange. Tests for unknown version 2 must move to another
unsupported version; do not weaken version rejection. Synthetic modifier
initializers must explicitly supply the new ID field without shifting old values.

## 9. Final original-resource and physical acceptance

This is the earlier acceptance proposal. Section 17 records the subsequently
authorized bounded implementation: one combined exchange cache pass, no second
save/restart cycle, automated fresh control, and a focused physical checklist.

Use `F:/Games/gog/Might and Magic 4-5` read-only. A checkpoint harness may move
the camera through existing `observeGameplay`/test seams. It may not directly
set the Root, Q2, reward queue, inventory or removals for the primary acceptance.
Use a new producer directory/save path for each run.

1. In a new production gameplay session inspect initial roster/items. Complete
   Myra's original request first to establish Q2=true without a direct flag set,
   so the later clear proves a true-to-false change. At Phirna
   map 23 `(8,2)` North, original Space/Yes/ack grants one Root and performs its
   established removal. Compare all initial item records to the loaded source.
2. Position at Myra map 23 `(9,11)` West using disclosed camera-only setup. Check
   live eligibility/tail capacity and snapshot all records. Space shows original
   return text. F9 while pending refuses. Complete all original NPC pages.
3. Observe exactly one Root consumed, Q2 cleared, five initialized queued records
   and natural termination at absent line 15. Delivery must reach a receipt,
   not the obsolete unsupported frontier or an idle gap. With unchanged verified
   initial capacity, roster 0 gets exactly five `{10,37,1,0}` records in misc
   slots 0..4. Existing occupied records and every bystander/category are intact;
   no Myra removal, game-flag change or permanent completion marker is invented.
4. Inspect the receipt and owners; F9 while receipt pending still refuses. Finish
   its final acknowledgment, use the live I diagnostic, then save through the
   **actual Application F9 handler**. Decode actual saved bytes for typed expected
   state. End the producer process, not merely its Flow.
5. Launch a separate consumer through production `--load-game <game> <save>`.
   Before any gameplay event, inspect the startup diagnostic/I output and typed
   observers: all five items, fields, owners, initial inventory, Root=0, Q2=false,
   Phirna removal and saved camera survive. No dialogue/receipt/queue is pending;
   zero initial dispatch, no replay, no duplicate grant.
6. Reconstruct owners in that new process and exercise existing genuine cache
   reload seams. Item values/ownership and inspection stay equal. Reuse the M20
   cache matrix and counters; add the new fields to its assertions rather than
   copying the whole matrix. Re-save/restart must still have exactly five items.
7. Only **after first resumed inspection**, revisit Myra separately: with no Root
   the original request runs and acknowledgment sets Q2=true, without additional
   items. This is legitimate script behavior, not lost persistence. Phirna stays
   removed. A separate fresh-session control has its original items and no reward.

Separate synthetic/explicitly labeled fixture cases cover multiple Roots and
capacity: 3 Roots consumes one per return regardless of Q2, and each return
produces five subject to current capacity. Verify repeated exchanges until no
Roots remain, then request/Q2=true. For partial capacity assert exact recipient
and slot order plus losses; with no eligible recipient zero delivered and all
five discarded, Root still consumed and Q2 cleared. Distinguish all-misc-full
from globally full four-category packs; only the latter receives the extra
pre-delivery full-pack warning. Do not claim fixtures obtained multiple Roots
through original Phirna, which cannot grant again after removal.

Automated direct and SDL dummy/software runs must record commands, separate
PIDs, exit codes, phase assertions and diagnostic output. Inspect native receipt
and clean resumed frame images separately. Image generation/equality assertions
are not visual inspection, and SDL dummy is not a physical-window result.

**Required user-performed physical checks:** original Phirna question/harvest,
Myra return text, visible receipt with owners/fields, correct pagination and
Space/Enter/Escape handling, pending F9 refusal and final F9 success, full process
exit, separate CLI resume and useful console inspection before another event,
clean first frame, original no-root revisit and fresh control. Use the bounded
checkpoint harness for the combined producer when route navigation is not
certified. A physical harness mode must keep real SDL input/window handling and
production save/startup; only location selection may be automated. Its command
and camera-only behavior must be documented during 21D, not replaced with a
pre-granted fixture. Do not claim normal travel between the checkpoints.

## 10. Stages, activation dependencies and commands

Each stage requires explicit authorization. Stage completion does not authorize
the next stage. Authorization originally covered only 21A, which was implemented,
independently reviewed, accepted and committed. 21B was subsequently explicitly
authorized, independently accepted and committed as `be98bb6`. Independent 21B review
accepted the reward architecture but found the P2 retained-label correction
recorded below. Final approval was then pending; that historical status is now
superseded by section 15's **APPROVE 21B FOR COMMIT** verdict after independent
re-review of the correction. The user's subsequent assignment explicitly authorizes
21C only; section 16 records that stage's implementation-time boundary. The later
explicit 21D assignment and implemented boundary are recorded in section 17.
Historical implementation evidence is preserved separately below.

### 21A - Authoritative item records and persistence

Objective: preserve initial and saved complete records before any new exchange
can mutate the party. Prerequisite: approval of sections 4 and 7. Likely files:
character/party/format/rules files, SaveSnapshot/Format/State, comparison helpers,
character/save tests and PartyIntegrationTest. The existing file protocol should
need only compatibility tests, not a rewrite.

In scope: four typed arrays, narrow capacity/compaction helpers, exact initial
loading, v2 codec and v1 resolution/replacement policy. Out of scope: event
consumption/clear activation, GiveEnchanted execution, receipt and item use.
The helper allocation above excludes all insertion and party-level delivery
operations from 21A. Structural validation must accept unresolved v1 snapshots
for decode/read/restore and old-target replacement; v2 encoding alone rejects
unresolved input. The transient discriminator is not a wire or gameplay field.
Observable acceptance: original items preserved, old modeled stats unchanged,
independent v1 resolves correctly, explicit-empty v2 remains empty and new fields
survive production restoration. Build plus focused model/save tests; original
party smoke required. No physical-window gate for this storage stage.

### 21B - Bounded delivery, receipt and live inspection

Objective: a reusable exactly-once finalization path backed by persistent roster
items. Prerequisite: 21A accepted. Likely files: Interpreter state/completion,
EventSystem/Flow, Presenter, PlayerAction/SdlWindow, XeenGameplay/Application and
bounded reward/flow/save-input tests. Add small helpers beside existing Xeen
model/event code only as needed; no treasure engine or parallel service graph.

In scope: ten pending misc records, recipient logic, full-pack warning,
insertion/receipt lifecycle, error/abandon behavior, I/startup diagnostics and
F9 guards. Exercise pending rewards via synthetic test inputs to the bounded
helper/state seam. **Keep production take-only 21/104 and opcode 0x2c unsupported.**
No debug reward/grant key. Out of scope: original exchange activation and final
restart claim. Observable acceptance: exact synthetic delivery/capacity, visible
transient receipt, no repeated insertion, inspection sees actual owners after
resume, no save-safe gap. Build and flow/presentation/save tests; inspect synthetic
native frames. Final original/physical gate remains 21D.

### 21C - Activate the supported exchange semantics together

Objective: execute original Myra through working rewards. Prerequisites: 21A and
21B accepted; all mutation/lifecycle adaptations approved. Likely files:
Decoder/Interpreter, party quest APIs, quest/decoder tests, MyraIntegrationTest,
affected SaveResumeIntegrationTest expectations and documentation.

In scope: bounded decrement/clear, deterministic GiveEnchanted decode/execute,
overflow behavior, successful-terminal finalization and original zero/one/multiple
Root/Q2 matrix. **Activate consumption, clearing and reward opcode support in
the same accepted change**, only after delivery, receipt and persistence exist.
Before this stage the M19 pre-consumption boundary stays intact. Do not add a
future reward-path capability toggle or a Myra-script pre-scan transaction.
Out of scope: other TakeOrGive combinations/random materials/equipment actions.
Observable acceptance: original return yields nine instructions, five actual
items under verified capacity, original requests/revisits and defined errors.
Build, focused event/model/reward tests and direct/SDL Myra plus Phirna/WhoWill
smokes required. This does not alone close restart or physical acceptance.

### 21D - Production restart and acceptance closure

Objective: close section 9 through production Application, not a snapshot-only
test. Prerequisite: 21C accepted. Likely files: SaveResumeIntegrationTest,
GraphicsSmokeTest, checkpoint/process/comparison helpers, necessary narrowly
scoped corrections, project status/README and this plan's acceptance evidence.
In scope: primary genuine collection/exchange, v2 F9/disk/CLI restart inspection,
separate revisit, regression stabilization and documented physical harness mode.
Out of scope: travel certification, extra inventory features and M22.
Observable acceptance: every section-9 result, full CTest, native image review
and separately recorded user physical checks pass. Missing physical evidence
keeps the milestone incomplete. Prepare a concise commit message at closure;
do not commit/push/tag without the appropriate explicit authorization.

### Commands for future implementation validation

Use the existing pinned dependency configuration. These commands are a future
checklist, **not commands claimed to have passed during planning**. Example MSYS2
UCRT64 shell from the repository root (PowerShell users may translate paths):

```sh
GAME='F:/Games/gog/Might and Magic 4-5'
B='build/21a'
SC_SRC='D:/Projetos/MModern/scummvm-known-good-candidate'
SC_BUILD='D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64'
cmake -S . -B "$B" -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Debug \
  -DSCUMMVM_SOURCE_DIR="$SC_SRC" -DSCUMMVM_BUILD_DIR="$SC_BUILD"
cmake --build "$B" --parallel 4
# 21A; keep using this build in subsequent stages.
ctest --test-dir "$B" --output-on-failure -R 'xeen_(character_|party_visual_state|save_)'
cmake --build "$B" --parallel 4 --target mmodern_party_smoke
"$B/mmodern_party_smoke.exe" "$GAME"
# 21B/C: item_rewards and reward_flow are proposed new test registrations.
ctest --test-dir "$B" --output-on-failure -R 'xeen_(item_rewards|reward_flow|event_|manual_event|quest_|npc|who_will|save_)|sdl_input'
cmake --build "$B" --parallel 4 --target mmodern_myra_smoke \
  mmodern_phirna_smoke mmodern_who_will_smoke mmodern_save_resume_smoke \
  mmodern_graphics_smoke mmodern_manual_event_smoke mmodern_navigation_flow_smoke
"$B/mmodern_myra_smoke.exe" "$GAME" build/21c/myra-direct
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_myra_smoke.exe" "$GAME" build/21c/myra-sdl sdl
"$B/mmodern_phirna_smoke.exe" "$GAME" build/21c/phirna-direct
"$B/mmodern_who_will_smoke.exe" "$GAME" build/21c/whistle-direct
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_phirna_smoke.exe" "$GAME" build/21c/phirna-sdl sdl
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_who_will_smoke.exe" "$GAME" build/21c/whistle-sdl sdl
# 21D extends this existing coordinator with the exchange checkpoint.
"$B/mmodern_save_resume_smoke.exe" "$GAME" build/21d/restart-direct
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_save_resume_smoke.exe" "$GAME" build/21d/restart-sdl sdl
"$B/mmodern_manual_event_smoke.exe" "$GAME" build/21d/manual
"$B/mmodern_navigation_flow_smoke.exe" "$GAME"
ctest --test-dir "$B" --output-on-failure
git diff --check
```

**Superseded proposal (not implemented):** extend `mmodern_graphics_smoke` with a named `save-myra-exchange` mode
that uses original script inputs and camera-only checkpoints, then run that
mode with a new save path and the existing `resume` mode against its file.
Section 17 instead uses `mmodern_save_resume_smoke --manual-myra-exchange`.
Graphics `resume` is a Phirna/Root-owned control and cannot validate this exchange.
Retain existing Application `save-phirna`, `save-idle`, `resume`, `resume-idle`
controls. Run the real CLI `mmodern --load-game "$GAME" <exchange.mmsave>` in a
separate process as well. Place outputs outside the installation under ignored
build directories. Reuse the same acceptance logic for a future documented
physical mode; do not run dummy settings for human physical checks.

## 11. Approval decisions and planning closure

There is no unresolved critical reference semantic for the bounded numeric
inspection slice described here. Implementation can be specified without a
separate inventory prerequisite **if** the reviewer approves these decisions:

1. Complete raw character records across four categories, while only deterministic
   misc reward production/delivery gains behavior; preserve existing rules.
2. Ten pending rewards, tail-based capacity, current worst-condition eligibility,
   active-order distribution and reference capacity loss without refunds.
3. Explicit error/abandon queue discard before delivery; immediate effects survive;
   insertion precedes receipt acknowledgment and saving remains blocked until
   successful acknowledgment or explicit terminal cleanup.
4. Numeric read-only live/startup inspection and adapted paginated feedback instead
   of original item names/timed treasure messages; no new engine-data dependency.
5. Version 2 writes, narrow v1 reads/default resolution and valid-v1 replacement.
6. Four staged activations, original Application restart and physical gates.

If native original item names or exact timed treasure feedback are made mandatory,
reopen the bounded display-resource decision before calling that expanded scope
implementation-ready. If a new requirement demands equipment effects, trading,
random treasure or suspended reward saves, reassess whether it merits a separate
prerequisite; do not silently add it to these stages. None is demonstrated as
necessary for the present objective. The exact future physical harness command
is an implementation deliverable, not an unresolved reward semantic.

Historical planning closure (before the separate 21A authorization below):
planning changed only this new draft and the minimal project-status pointer.
Read-only baseline/source/test inspection, the original event diagnostic and
focused in-memory inventory/provenance checks were performed. The optional party
smoke executable was absent; no build/CTest, exchange execution, saved-result
acceptance, native image inspection or physical-window check was performed here.
Final document review and whitespace/scope checks are recorded in the task report.
No implementation, test/helper/config change, commercial-data write, commit,
push, tag, branch switch or later milestone work is part of this task.

## 12. 21A implementation and validation evidence

**2026-09-08: 21A implemented and locally validated; independent implementation
review pending.** This is limited implementation authorization and evidence,
not approval of all M21 decisions or full milestone acceptance. 21B/21C/21D
remain unauthorized and unstarted. M20 remains the stable accepted milestone.

### Baseline and implementation

- Initial branch `main`, HEAD `b5b1e5b80213ec5cdc60431e00e58e2f9f50c00a`, clean
  working tree. Recent history matched `b5b1e5b`, `3625380`, `a449bb7`; no material
  baseline discrepancy. No branch, commit, push, tag, history or dependency change.
- The ScummVM source path below was checked at
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd` with clean status. The existing UCRT64
  GCC 16.2.0 / CMake 4.4.2 / MSYS Makefiles configuration was reused unchanged.
- `XeenItem` replaces the modifier-only record with four uint8 fields in
  material/ID/state/frame order. `XeenCharacter` owns weapons, armor, accessories
  and miscellaneous categories of nine records. The original loader reads
  offsets 166/202/238/274 without changing the 354-byte CHR source format.
  All roster slots retain their values; repeated membership refers to the same
  owner. Existing equipment rules only received the type adaptation.
- `xeenItemHasTailCapacity` reads only the final ID. `xeenCompactItems` is an
  explicitly invoked stable operation on one fixed-size category; it clears
  leftover empty records. Neither helper is called during loading or persistence.
  There is no insertion, party scan, recipient, pending reward or delivery API.
- The codec writes v2 only and explicitly parses v1 or v2. Item blocks are
  81/144 bytes; fixed character sizes excluding name contents are 149/212;
  minimal golden fixtures are 4,957/6,847 bytes. The 20-byte envelope, all other
  fields/order/limits, CRC32, exact EOF and structural validations are retained.
- Snapshot-level `XeenSaveItemState` distinguishes complete records from v1
  missing fields. It is neither a wire field nor live state. Shared `validate`
  accepts unresolved v1; only v2 encoding rejects it. Capture and v2 decode
  produce complete snapshots without resource loading in the codec.
- Restoration verifies signatures, loads the initial candidate, overlays saved
  values and supplies only missing v1 equipment IDs/miscellaneous by roster slot
  before whole-character replacement. All resource/rule/composition checks still
  precede publication. Complete v2 empties receive no merge. Failed late legacy
  preflight preserves live items, membership, camera, flags, world and cache.
- The file layer and Application production code required no changes. Existing
  encode-before-I/O, target validation, temporary ownership, flush/close,
  replacement, alias protection and cleanup remain in force. Valid v1 targets
  need only structural validation for replacement. Startup/read leaves their
  exact disk bytes unchanged; only an explicit eligible save upgrades to v2.

### Evidence at owning layers

- Character tests compare all four bytes/categories/slots at original offsets,
  exercise inactive owners, IDs/metadata at zero and 255 and the whole byte range,
  aliases, tail fullness despite earlier holes, and stable compaction with
  distinctive occupied records and empty metadata. Other categories/characters
  remain exact. Existing rule outputs and visual-state tests still pass, with
  explicit evidence that IDs/miscellaneous introduce no modifier effects.
- Typed character comparisons and textual party snapshots include IDs and misc;
  snapshot comparison also checks transient presence. Existing unusual values,
  inactive invalid class/race values and historical assertions are preserved.
- The original independent `golden()` v1 fixture is unchanged. A separately
  constructed 5,050-byte nonzero v1 fixture adds names, reordered/duplicate
  membership, distinctive triples and HP/SP, plus quest/game flags, using an
  independent bitwise checksum repair. Tests decode these actual bytes before
  restoration or file replacement. A separate asymmetric v2 oracle covers all
  roster positions, four categories, holes, ID-zero metadata, nonempty names and
  distinctive fields after the item block. Tests cover truncation with repaired
  envelopes, inserted/removed bytes, legal item-byte changes and legacy encoding
  refusal, alongside the existing envelope/domain matrices. Unsupported-version
  fixtures now use version 3; a v1 payload labeled v2 separately fails schema
  validation.
- `xeen_save_state` checks v1 modifier preservation against different nonzero
  resource defaults, matching inactive roster slots, duplicate membership,
  recapture as complete v2, late failure without publication and explicit-empty
  v2 restoration against populated defaults.
- `xeen_save_file` writes independent v1 bytes directly to disk, checks read
  immutability, successful v2 replacement, exact old-byte preservation under the
  existing open/write/short-write/flush/close/replace fault seams, unsupported or
  invalid target protection and rejection of unresolved new input before I/O.
  Existing cleanup, actual Windows locks and path/alias regressions still run.
- The mandatory `xeen_save_flow` Application legacy-upgrade case uses the existing
  `observeGameplay` and `show` seams. It observes resolved owners before input,
  compares all saved categories and aliases, checks byte-identical startup,
  invokes the production `SaveGameAction` handler used by F9, checks the v2 file,
  and resumes it through Application with different initial items to prove saved
  authority. No public production test API or new acceptance framework was added.
- CMake source lists and excluded consumers were inspected. No target changes
  were required; the storage tests live in existing owning test targets.

### Commands and actual results

Run from `D:/Projetos/MModern/mmodern` in PowerShell:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
cmake -S . -B build/21a -G 'MSYS Makefiles' `
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON `
  -DSCUMMVM_SOURCE_DIR=D:/Projetos/MModern/scummvm-known-good-candidate `
  -DSCUMMVM_BUILD_DIR=D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64
cmake --build build/21a --parallel 4
ctest --test-dir build/21a --output-on-failure `
  -R 'xeen_(character_|party_visual_state|save_)'
cmake --build build/21a --parallel 4 --target `
  mmodern_party_smoke mmodern_myra_smoke mmodern_save_resume_smoke `
  mmodern_phirna_smoke mmodern_who_will_smoke mmodern_graphics_smoke `
  mmodern_outdoor_object_smoke mmodern_object_visual_smoke mmodern_remove_smoke `
  mmodern_indoor_map_smoke mmodern_event_script_smoke mmodern_event_text_smoke `
  mmodern_game_flags_smoke mmodern_event_interpreter_smoke `
  mmodern_event_system_smoke mmodern_manual_event_smoke mmodern_navigation_flow_smoke
& ./build/21a/mmodern_party_smoke.exe 'F:/Games/gog/Might and Magic 4-5'
& ./build/21a/mmodern_myra_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21a/myra-regression
& ./build/21a/mmodern_save_resume_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21a/save-resume-regression
ctest --test-dir build/21a --output-on-failure
git diff --check
```

Results:

- Fresh Debug configure and build passed; final focused selection **9/9 passed**:
  six save tests (including Application/SDL/CLI), character formats, character
  rules and party visual state. The first focused run was 8/9 because the CLI
  test binary preceded its unsupported-version fixture correction; the corrected
  rebuild and focused rerun passed. Logs: `build/21a/build.log`, `focused.log`.
- **All 17 excluded targets explicitly built**, including Myra, save/resume,
  Phirna and WhoWill shared-comparison consumers. Log: `excluded-build.log`.
- Original-party smoke passed: all **1,080** slots compared to actual original
  CHR bytes across all 30 characters; **35 occupied**, **zero miscellaneous**.
  Current/max HP/SP and portrait order remain unchanged. Nonzero miscellaneous
  and inactive-population evidence is synthetic, not manufactured original data.
  Log: `party-smoke.log`.
- Existing Myra direct matrix passed: **18 cases plus revisits**, preserving
  Root-owned line 8 / offset 255 / three-instruction unsupported consumption.
  Log: `myra-smoke.log`; frames are regression outputs, not a claimed native
  visual review or physical-window test.
- Existing original cross-process regression passed for Phirna, Whistle, Myra
  and cumulative producer/consumer/fresh processes plus actual CLI resumes.
  Log: `save-resume-smoke.log`; detailed process evidence:
  `build/21a/save-resume-regression/run-34056-276710843/processes.log`.
  This reruns the M20 frontier; it is not M21D reward acceptance.
- Current full suite **53/53 passed** (`build/21a/ctest.log`); no failing tests
  remain. Diff review and `git diff --check` passed.

### Remaining boundaries and handoff

GiveEnchanted `0x2c`, take-only quest-item mode 21 and quest-flag clear mode 104
remain unsupported. No Root consumption, Q2 clearing, rewards, receipt, I-key
inspection, pending-reward persistence, equipment use or later milestone work
was activated. Event/presentation code is unchanged. Original resources were
read-only; no commercial payload was added to the repository. No dependency,
branch, commit, push or tag operation was performed.

There is no physical-window acceptance gate for 21A, and none is claimed.
Independent implementation review is still outstanding. This evidence does not
approve the remaining draft M21 decisions or authorize starting 21B.
Suggested commit message for a later authorized commit:
`Implement Milestone 21A item records and save v2 compatibility`.

## 13. 21B authorization, implementation and validation evidence

Historical implementation evidence: the pending-review status below is
superseded by the final independent approval in section 15.

**2026-09-08: 21B implemented; independent review pending.** The current
assignment explicitly states that 21A was independently
reviewed and accepted after its implementation commit. That acceptance is
separate from section 12's original local implementation evidence. The assignment
authorizes 21B and adopts its bounded policies, not every draft M21 decision.
21C and 21D remain unstarted. M20 remains the stable completed milestone.

### Verified baseline and scope

- `main`, HEAD `1dab6ee9836e25f65bd06fbfe240fe180f10f0d0`, clean working tree.
  Recent history: `1dab6ee` (21A), `b5b1e5b` (M21 planning), `3625380` (status
  reorganization), `a449bb7` (M20C closure). The committed 21A changes and current
  model/interpreter/EventSystem/Flow/presenter/Application/input tests were read
  locally. There were no pre-existing edits to preserve or baseline discrepancies.
- ScummVM remains at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, clean. The pinned
  source/build directories and UCRT64 GCC 16.2.0 / MSYS Makefiles configuration
  match section 12 and `dependencies.md`. No new reference scan or dependency
  change was needed: section 3.3 already records the relevant reference behavior.
- No change to original loading, authoritative roster ownership, rule effects,
  save v2, narrow v1 restoration, codec or file publication. Commercial resources
  were read only. No branch switch, reset, stash, clean, commit, push or tag.
- Take-side quest-item mode 21, take-side quest-flag mode 104 and GiveEnchanted
  `0x2c` remain unsupported. Existing give-side grants/sets remain supported.
  No operand decoding, production reward producer, debug grant, Myra special
  path, activation toggle, item database/use/effects, shops, RNG or later stage.

### Selected 21B policies and implementation

- `XeenPendingRewards` owns a fixed ten-entry miscellaneous array in execution
  state. Every typed nonzero ID is accepted as an opaque record; zero IDs are
  rejected and counted without occupying slots. This typed domain deliberately
  does not decode or constrain the future 21C opcode domain. Overflow leaves
  existing entries/order unchanged and increments an ignored count. Counts are
  bounded machine values with saturation; production remains instruction-bounded.
- Insertion checks slot 8 before mutation, writes slot 8 and explicitly invokes
  21A stable compaction. Earlier holes provide no capacity when slot 8 is occupied.
  Occupied fields/order, other categories and other owners remain unchanged.
- Delivery resolves current roster owners before its allocation-free mutation
  segment. It tries the valid preferred active index, then current active order,
  using `canAct()` and tail capacity after each insertion. HP/SP add no gates;
  worst-condition masking remains intact. Repeated IDs remain aliases and inactive
  owners are excluded. Accepted multi-member WhoWill sets the separate preference;
  single-member selection and cancellation preserve it. Reordering uses the
  current meaning of that active index, not a cached character copy.
- Global fullness checks all four tails for every active reference regardless of
  eligibility. Empty membership explicitly means no global warning and no
  recipient. Undeliverable records distinguish empty party, no eligible member,
  and eligible miscellaneous tails full. Overflow/invalid input have separate
  numeric feedback. No displacement, refund or exchange-wide rollback occurs.
- Explicit Exit, natural missing successor, terminating presentation, WhoWill
  cancellation and TeleportAndExit share one finalizer. Ordinary suspension and
  Call/Return retain the queue; invalid returns/targets/errors never finalize as
  success. Warning and receipt handling add no interpreter instructions.
- Phases are Running, optional Warning, synchronous delivery, Receipt and final
  completion. **Boundary 1:** delivery immediately mutates roster inventories,
  clears pending production and establishes a fixed typed receipt before formatting
  or callbacks. **Boundary 2:** only final receipt ACK returns Completed, allowing
  EventSystem to publish working camera/game flags. All-lost delivery still awaits
  a receipt. The receipt records completed results and is not a production queue.
- Interpreter failures return typed discard/delivery accounting. Flow adopts a
  suspension by nonthrowing move before refresh/reporting/layout; EventSystem
  handoffs move suspension values. Flow is noncopyable and remains the production
  continuation owner. Reporting snapshots may be copied after adoption, with the
  original already owned for failure cleanup. Tests retain execution value copies.
- Flow validates generation and response kind before finishing/consuming a request;
  stale, duplicate and wrong-kind responses preserve current state. Correctly typed
  invalid WhoWill responses retain the prior interpreter policy. Production initial
  and input dispatch are guarded before execution; explicit replacement fixtures
  now abandon before executing their replacement. No replay API or execution IDs.
- Error/abandonment discards queued records with a reason/count and preserves prior
  immediate mutations. After insertion, failure/abandonment/shutdown removes only
  transient ownership/UI and cannot requeue/refund. Cleanup precedes failure
  reporting and works before a transient layer becomes active. Manual presentation
  failure is recoverable; production automatic-error reporting remains fatal.
- Reward warning/receipt use the existing font renderer with real pagination.
  Space/Enter/Escape advance; final-page acknowledgment alone completes that phase.
  One input cannot acknowledge the next phase. Other inputs do not dismiss it.
  Rebase/redraw does not deliver. Transient layers are removed over retained
  underlay. Receipt text uses English numeric diagnostics, actual owner IDs/names,
  category, material/ID/state/frame, charges/cursed/broken and concrete loss counts.
  Control bytes in names are displayed safely instead of interpreted as font
  commands. Reference treasure timing, names, audio and NPC animation are not added.
- Application reuses active/dispatching/pending guards for F9, including warning,
  delivery and receipt. Refusal captures nothing, performs no I/O and queues no
  save. Final ACK or recoverable cleanup permits saving only under existing guards.
  Retained nonblocking labels stay saveable; fatal errors/shutdown do not.
- I is intercepted before event/presentation handling and formats actual live
  state: ordered active aliases, all 30 unique owners including inactive, all
  four nine-slot categories/raw fields, generic equipment counters versus misc
  charges, state bits, tail capacity, eligibility, Root and Q2. It updates the
  existing title seam and no-ops while blocked. Setup prints after valid first
  composition and before normal initial automatic dispatch. Resume still performs
  no initial dispatch and shows restored records before interaction. No second
  inventory model, persistent diagnostics, separate CLI or per-frame output.

### Owning tests and artifacts

New targets: `mmodern_item_reward_tests`, `mmodern_reward_execution_tests`,
`mmodern_reward_flow_tests`, `mmodern_reward_gameplay_tests`. CTest adds those
four responsibilities plus `xeen_reward_sdl`. `XeenRewardTestSupport.h` seeds only
fresh typed records in an existing ordinary suspension via a test-only friend;
it adds no production grant or continuation dispatch operation.

The model matrix checks tails/holes, slot-8 compaction, exact capacity, all
worst conditions/masking, preference/fallback, reordered/duplicate IDs, inactive
owners, full versus ineligible versus empty, ten entries/overflow/empty input,
and distinctive bystanders. Execution tests cover all successful terminal paths,
Call/Return, ordinary queue survival, invalid targets/returns, unchanged counts,
immediate grants surviving errors and both EventSystem publication routes.

Flow/presentation tests cover warning-before-mutation, typed/generation rejection,
no replay on pages/rebase, guarded dispatch, explicit abandonment, composition,
reporting, layout and diagnostic-report failures before/after insertion, and
retained-underlay restoration. Existing NPC/WhoWill/grant/flag matrices retain
their mutation assertions; their replacement fixtures explicitly abandon first,
and wrong-kind response expectations now match the nonconsuming contract.

Application tests exercise F9 during ordinary queued work, warning and receipt,
including reentrant reporting; they assert no preparation/I/O/generation change
or deferred save. Actual SDL key events cover F9 through warning and receipt,
Escape pagination and final save. Production Application resume retains inserted
records, aliases and inactive ownership without transients/initial dispatch.
Manual versus fatal automatic cleanup is tested on both sides of insertion.
Inspection checks setup ordering, changed live/resumed state, pending refusal and
unchanged dispatch counts. Existing v1/v2 and Windows filesystem matrices are reused.

Native synthetic frames used the original read-only `fnt` and palette with a
synthetic background/owners/records. Visually inspected at **320x200**:
`build/21b/reward-frames/receipt-0.bmp`, `receipt-1.bmp`, `receipt-2.bmp`,
`warning-0.bmp`, `warning-1.bmp`. All ten entries, two recipients, raw/state
fields, overflow/invalid explanations and final prompt were readable without
clipping; warning continued over two pages. Each key variant and retained-underlay
restoration also passed pixel assertions. These are synthetic presentation
evidence, not original completed-exchange or physical-window acceptance.

### Commands and results

PowerShell from the repository; all generated artifacts are under ignored
`build/21b`. Set `PATH` as in section 12. The configured dependency paths below
were verified before use.

```powershell
cmake -S . -B build/21b -G 'MSYS Makefiles' `
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON `
  -DSCUMMVM_SOURCE_DIR=D:/Projetos/MModern/scummvm-known-good-candidate `
  -DSCUMMVM_BUILD_DIR=D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64
cmake --build build/21b --parallel 4
ctest --test-dir build/21b --output-on-failure `
  -R 'xeen_(item_reward|reward_|character_|event_|manual_event|npc|who_will|quest_|save_)|sdl_input'
ctest --test-dir build/21b --output-on-failure
cmake --build build/21b --parallel 4 --target `
  mmodern_party_smoke mmodern_myra_smoke mmodern_save_resume_smoke `
  mmodern_phirna_smoke mmodern_who_will_smoke mmodern_graphics_smoke `
  mmodern_outdoor_object_smoke mmodern_object_visual_smoke mmodern_remove_smoke `
  mmodern_indoor_map_smoke mmodern_event_script_smoke mmodern_event_text_smoke `
  mmodern_game_flags_smoke mmodern_event_interpreter_smoke `
  mmodern_event_system_smoke mmodern_manual_event_smoke mmodern_navigation_flow_smoke
& ./build/21b/mmodern_reward_flow_tests.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/reward-frames
$env:SDL_VIDEODRIVER = 'dummy'
$env:SDL_RENDER_DRIVER = 'software'
& ./build/21b/mmodern_myra_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/myra-direct
& ./build/21b/mmodern_myra_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/myra-sdl sdl
& ./build/21b/mmodern_phirna_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/phirna-direct
& ./build/21b/mmodern_phirna_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/phirna-sdl sdl
& ./build/21b/mmodern_who_will_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/who-will-direct
& ./build/21b/mmodern_who_will_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/who-will-sdl sdl
& ./build/21b/mmodern_save_resume_smoke.exe 'F:/Games/gog/Might and Magic 4-5' build/21b/save-resume
git diff --check
```

- Fresh Debug configure/build passed (`build.log`). Final focused selection
  **31/31 passed** (`focused.log`); final full CTest **58/58 passed**, 9.31 seconds
  (`ctest.log`). No failing tests remain. All **17 excluded smoke targets built**
  against the final headers (`excluded-build.log`).
- Early iteration caught the new terminating-display fixture's wrong response
  kind and a one-page warning test assumption. Existing NPC source diagnostics
  were restored after cleanup was generalized. Existing WhoWill wrong-kind
  expectations and NPC/WhoWill/grant/flag copied replacement fixtures were
  updated to the explicitly selected response/abandon policy. Their previous
  mutation, error and Myra-frontier assertions were preserved. Corrected focused
  and full reruns passed; these initial failures are not outstanding concerns.
- Myra direct **18 cases plus revisits** and SDL dummy/software **18 cases plus
  revisits** passed (`myra-direct.log`, `myra-sdl.log`). Root-owned paths still
  stop at **line 8, offset 255, three instructions**, before consumption, with
  no reward delivery. No-root requests still set Q2 only after acknowledgment.
- Phirna direct/SDL (`phirna-direct.log`, `phirna-sdl.log`) and WhoWill direct/SDL
  (`who-will-direct.log`, `who-will-sdl.log`) passed. Existing original save/resume
  producer/consumer/fresh checkpoints and executable CLI resumes passed
  (`save-resume.log`); detailed process evidence is
  `build/21b/save-resume/run-15532-285964250/processes.log`.
- Native reward generation/pagination passed (`reward-frames.log`), with the
  five frames listed above visually inspected. The final additional diagnostic
  failure and SDL full-pack cases passed in the focused/full runs.
- Final diff review and `git diff --check` passed. The working tree contains only
  this task's source, tests, CMake and documentation changes; generated frames,
  logs and saves remain ignored. HEAD/branch remain the verified baseline.

### Changed files and remaining acceptance boundary

Production changes: `src/games/xeen/XeenItemRewards.{h,cpp}`;
`XeenEventInterpreter.{h,cpp}`, `XeenEventSystem.cpp`, `XeenEventPresenter.{h,cpp}`
in the same directory; `src/app/XeenEventFlow.{h,cpp}`, `XeenGameplay.cpp`,
`Application.cpp`; `src/core/PlayerAction.h`; `src/platform/sdl/SdlWindow.cpp`.

New tests: `tests/XeenItemRewardTests.cpp`, `XeenRewardExecutionTests.cpp`,
`XeenRewardFlowTests.cpp`, `XeenRewardGameplayTests.cpp`, `XeenRewardTestSupport.h`.
Updated existing tests: `SdlInputTests.cpp`, `XeenNpcTests.cpp`,
`XeenWhoWillTests.cpp`, `XeenQuestFlagTests.cpp`, `XeenQuestGrantTests.cpp`.
Build/documentation: `CMakeLists.txt`, this plan, `docs/project-status.md`,
`README.md`. The roadmap is unchanged.

No known failing check remains within 21B. This is implementation evidence,
**not independent 21B approval or full M21 completion**. Original completed
exchange acceptance, separate-process Myra reward acceptance and user-performed
physical-window validation were not performed or claimed; they remain later
stages. 21C/21D are explicitly unstarted. No commit or push was made.

## 14. Independent 21B review: retained-label P2 correction

Historical correction evidence: the pending re-review status below is
superseded by the final independent approval in section 15.

The independent review accepted the 21B reward architecture but identified a P2
visual regression: blocked navigation cleared retained presenter layers without
recomposing their visible framebuffer when camera/object state stayed unchanged.
The correction passes a one-shot forced-recomposition request from navigation
into `drive()`. Refresh still occurs after suspension ownership adoption; all
other callers and later continuation iterations keep conditional refresh.

The focused addition to `XeenVisualRemoveTests.cpp` establishes a completed,
visible label, attempts surface-blocked movement, verifies unchanged camera and
disabled-object count, and checks both clean returned/current pixels and exactly
one new composition. It failed against the reviewed production tree with
`blocked movement left retained label visible` (`review-p2-red.log`), then passed
with the correction. Section 10's stale authorization sentence was corrected
without rewriting the historical 21A evidence.

Baseline remained `main` at `1dab6ee9836e25f65bd06fbfe240fe180f10f0d0`, with the
reviewed 21B changes uncommitted. Only `XeenEventFlow.cpp/.h`, the focused test
and this plan changed in this correction; hashes confirmed the other reviewed
working-tree files unchanged. Dependency revision/configuration remained pinned.

Fresh validation in `build/21b`: Debug build passed; **13/13 focused tests** and
**58/58 full CTest** passed; all **17 excluded smoke targets** built. Myra direct
and SDL each passed 18 cases plus revisits, retaining line 8 / offset 255 / three
instructions before consumption. Phirna and WhoWill direct/SDL controls and the
existing original cross-process save/resume/CLI regression passed. Logs use the
`review-p2-` prefix; exact commands are in `build/21b/review-p2-commands.txt`.
Save/resume process evidence is under
`review-p2-save-resume/run-38788-287820421/processes.log`. `git diff --check` passed.

Reward storage, recipients, insertion, continuation ownership, cleanup, paging,
both publication boundaries, F9 and I/setup behavior were otherwise unchanged.
Take-side modes 21/104 and opcode 0x2C remain unsupported. Commercial data was
read-only; no physical-window validation, commit or push is claimed. 21C/21D
remain unstarted; M21 is not complete. **Final independent re-review is pending.**

## 15. Final independent 21B approval for commit

Historical approval record: the later 21B commit and 21C authorization/implementation
in section 16 supersede the uncommitted and unsupported-frontier wording below.

**2026-09-08 — user-supplied final independent review verdict:
APPROVE 21B FOR COMMIT.** The reviewer confirmed the retained-label P2 correction
is correct, no new findings remain, and the 21B architecture and exactly-once
lifecycle remain valid. 21B is implemented locally, corrected after its first
independent review, independently re-reviewed and approved for commit. It remains
uncommitted; this documentation step does not create a commit or push.

The reviewer confirmed these validation results:

- Debug build passed; **13/13 focused tests** and **58/58 full CTest** passed.
- All **17 excluded smoke targets** built.
- Myra, Phirna and WhoWill direct/SDL controls passed.
- Save/resume direct/SDL controls passed.
- `git diff --check` passed.

These results are attributed to the independent reviewer as supplied by the
user, not a new build or test run during this documentation-only step. The
earlier implementation and P2 correction evidence remains in sections 13 and 14;
their pending-review statements are historical and superseded by this approval.

21A is implemented, independently approved and committed. **21C and 21D remain
unstarted and unauthorized; M21 is not complete.** Production TakeOrGive take-side
modes 21/104 and opcode `0x2C` remain unsupported. Myra still stops before Root
consumption at **line 8, offset 255, after three instructions**. This approval
does not claim 21C behavior, completed Myra exchange acceptance or physical-window
validation, and does not authorize starting a later stage.

## 16. 21C implementation and validation

**2026-09-08: explicitly authorized 21C only; implemented and validated locally;
independent review pending.** The user's assignment authorizes all three bounded
operations together, including the original Myra and affected pre-exchange
save/resume regression updates. It does not authorize 21D, completed-exchange
restart certification, physical-window harness work or M21 closure. M20 remains
the last completed stable milestone.

### Verified prerequisite and scope

Initial checkout: branch `main`, full HEAD
`be98bb6dc8ab0c5ce6d02f3f3e29cf9e5563423c`, subject
`Implement Milestone 21B reward delivery lifecycle`; parent
`1dab6ee9836e25f65bd06fbfe240fe180f10f0d0` (21A). Recent history includes
`b5b1e5b`, `3625380` and `a449bb7`. Initial status, staged diff and working
diff were empty. The committed 21B finalizer, Flow ownership, retained-label /
blocked-navigation correction and section-15 independent approval were present.
Earlier statements that 21B was uncommitted describe that historical handoff.

The accepted `build/21b/CMakeCache.txt` identifies the verified UCRT64 compiler
`C:/msys64/ucrt64/bin/c++.exe`, Debug / MSYS Makefiles, ScummVM source
`D:/Projetos/MModern/scummvm-known-good-candidate` and build
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. The source HEAD is
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with clean status using command-local
`safe.directory` and `core.autocrlf=false`. The external `config.mk` confirms
`--backend=sdl --disable-all-engines --disable-detection-full` and
`SDL_CONFIG=/ucrt64/bin/sdl2-config`. Fresh `build/21c` uses these explicit
paths and links the existing artifacts. No dependency or commercial data changed.

Focused pinned-source inspection reconfirmed `Scripts::cmdGiveEnchanted`
(`engines/mm/xeen/scripts.cpp:1279`), `ItemState` in `item.h`, and
`MISC_NAMES[22]` / `SPECIAL_NAMES[74]` in `resources.h`. The deterministic
material-10/11 branch reads only code and special ID and sets one charge.
The 2..4-byte envelope and restricted code/ID domain below are **MMModern policy**,
not restrictions imposed by the original interpreter.

### Implemented behavior and owning files

- `XeenParty.h/.cpp`: checked uint32 decrement fails at zero without mutation;
  checked quest-flag clear is immediate and idempotent. Invalid API indices use
  the existing checked-access conventions.
- `XeenEventInterpreter.h/.cpp`: take-only mode 21 supports IDs 82..116 through
  `indexForItemId`, and take-only mode 104 clears indices 0..29. Other pairs
  must have both mode and value zero, including default-neutral omitted pairs.
  Shared grant/set guards validate ID/index, Clouds logical and physical sides,
  nonempty membership and line below 255 before effects. Underflow reports
  `QuestItemUnderflow`; `Application.cpp` adds only its diagnostic name.
  Each effect occurs once per party, with no canAct/HP/SP/selection/capacity gate.
- `XeenEventDecoder.h/.cpp`: typed opcode 0x2c owns item code, special ID and
  the actual zero-to-two suffix bytes. Length errors precede operand-domain
  validation; valid lengths require codes 70/71 and special IDs 1..73.
  Unrelated decoder strictness and decode-to-execution accounting remain intact.
  Execution creates a zero-initialized record, assigns material=code-60, ID and
  state=1, and enqueues once after context/membership/line guards and the existing
  dispatch-budget check. Suffix bytes never supply state/frame/quantity.
- All three operations use sequential NaturalCompletion. Missing line 15 after
  Myra line 14 reaches the unchanged 21B finalizer. The first ten productions
  retain order; later valid records count as ignored overflow. A full queue does
  not bypass decoding and does not refund consumption.
- Generic item/queue domain, character parsing, save v1/v2, finalizer, recipient
  selection, insertion/compaction, presenter, Flow, SDL and gameplay production
  code are unchanged. Errors/abandonment discard undelivered records; earlier
  quest effects and delivered items remain. Working camera/game flags publish
  only after receipt acknowledgment. No new owner, RNG or transaction layer.

The sole production edit outside Party/Decoder/Interpreter is the required
underflow diagnostic mapping in Application. No architectural deviation or new
test executable was needed. No milestone/history/roadmap rewrite was performed.

### Tests and original-resource oracles

Existing quest-grant/flag targets cover boundary indices, omitted/neutral pairs,
malformed/mixed forms, wide counters, zero underflow, duplicate aliases, inactive
conditions, Clouds sides, membership, line/budget guards and immediate effects
across suspension followed by error. Decoder tests cover codes, special-ID
boundaries, all lengths, owned suffix/source lifetime, purity and error metadata.

Reward execution tests now include real opcode decoding/enqueueing, exact fields
and order, eleven productions, overflow after consumption, malformed/unsupported
operands with ten queued records, guard failures, Call/Return, later underflow/
terminal failure and live capacity/eligibility loss. Flow/gameplay additions
exercise actual consumption/production through stale/wrong-kind responses, cache
rebase, abandonment, extra inputs, publication, reentrant/pending F9/I refusal
and post-cleanup input. Established 21B seeded terminal/alias/capacity/cleanup,
opaque item and save compatibility tests remain. Former unsupported fixtures
use genuinely unsupported operands; dedicated new tests cover underflow.

`MyraIntegrationTest.cpp` still verifies original records, offsets and resource
text, and explicitly uses `findInstructionIndex(9,11,West,15)` to assert the absent
sequential successor. The original 18-case matrix (Roots 0/1/3 x Q2 false/true x
Space/Enter/Escape) passes in each mode, plus two focused Root=1 fixtures:
all four category tails full and all active members Dead. NPC, warning and receipt
phases have separate expectations. The original return dispatches nine
instructions, consumes one Root, clears Q2 and produces five `{10,37,1,0}`
records. Warning/receipt pages add no instructions. Owners remain unchanged during
pending NPC dialogue; receipt paging cannot redeliver. Independent expected
characters preserve every modeled field and every item byte across repeated
returns, exhaustion, new requests, cache rebuilds and replacement Flow owners.
Loss fixtures preserve their prepared items and never refund the Root. Existing
original animation, ignored/repeated input, pre-ACK failure/abandonment, world,
camera, flags and resource-byte checks remain.

`SaveResumeIntegrationTest.cpp` keeps all producer files **pre-exchange** and
retains distinct producer/consumer/fresh processes and CLI checks. Root count
and Phirna removal are independent; expected character arrays and Q2 evolve
independently after startup validation. The cumulative consumer's first return
consumes the restored Root and adds five explicitly specified records; later
requests set Q2 without additional items. No production delivery computes this
oracle. All unrelated-state and cache comparisons remain. No completed exchange
is saved for a new restart scenario: this is regression evidence, **not 21D**.

### Commands and results

Commands were run in PowerShell from `D:/Projetos/MModern/mmodern`, using the
verified MSYS2 UCRT64 runtime. Each build/run was checked for nonzero exit.
Output redirects below identify the actual ignored evidence files; iterative
builds and the focused/full suites were repeated after the final test additions.

```powershell
$env:PATH='C:\msys64\ucrt64\bin;C:\msys64\usr\bin;'+$env:PATH
git status --short
git diff --stat
git diff --cached --stat
git branch --show-current
git rev-parse HEAD
git log -5 --oneline
git -c safe.directory=D:/Projetos/MModern/scummvm-known-good-candidate -c core.autocrlf=false -C D:/Projetos/MModern/scummvm-known-good-candidate rev-parse HEAD
git -c safe.directory=D:/Projetos/MModern/scummvm-known-good-candidate -c core.autocrlf=false -C D:/Projetos/MModern/scummvm-known-good-candidate status --short
cmake -S . -B build/21c -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DSCUMMVM_SOURCE_DIR=D:/Projetos/MModern/scummvm-known-good-candidate -DSCUMMVM_BUILD_DIR=D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64 > build/21c/configure.log 2>&1
cmake --build build/21c --parallel 4 > build/21c/build.log 2>&1
cmake --build build/21c --parallel 4 --target mmodern_myra_smoke mmodern_phirna_smoke mmodern_who_will_smoke mmodern_save_resume_smoke > build/21c/build-smokes.log 2>&1
ctest --test-dir build/21c --output-on-failure -R '^(xeen_(event_decoder|event_interpreter|event_system|quest_items|quest_grants|quest_flags|item_reward|reward_execution|reward_flow|reward_gameplay|reward_sdl|npc|npc_sdl|who_will|who_will_sdl|visual_remove|save_state|save_format|save_file|save_flow|save_cli|save_sdl)|sdl_input)$' > build/21c/focused.log 2>&1
& build/21c/mmodern.exe --inspect-events 'F:/Games/gog/Might and Magic 4-5' 23 9 11 west > build/21c/myra-events.log 2>&1
# Each of myra, phirna, who_will and save_resume:
& "build/21c/mmodern_${suite}_smoke.exe" 'F:/Games/gog/Might and Magic 4-5' "build/21c/${suite}-direct" > "build/21c/${suite}-direct.log" 2>&1
$env:SDL_VIDEODRIVER='dummy'; $env:SDL_RENDER_DRIVER='software'
& "build/21c/mmodern_${suite}_smoke.exe" 'F:/Games/gog/Might and Magic 4-5' "build/21c/${suite}-sdl" sdl > "build/21c/${suite}-sdl.log" 2>&1
ctest --test-dir build/21c --output-on-failure > build/21c/ctest-full.log 2>&1
git diff --check
git diff --stat
git status --short
```

Results: Debug configure/build **passed**; all **four requested excluded smoke
targets built**; focused **23/23 passed**; full **58/58 passed**. Myra **20 cases
plus revisits per mode passed**; unchanged Phirna and WhoWill direct/SDL controls
passed; original pre-exchange save/resume and CLI controls passed in both modes.
These are new 21C runs, not copied historical totals. One intermediate smoke
compile used a nonexistent test-side item `empty()` accessor; it was corrected
to the actual ID-zero convention before the successful final builds.

Separate-process evidence (four producer/consumer/fresh triples and four CLI
launches per mode) is preserved at:

- `build/21c/save_resume-direct/run-11416-290587000/processes.log`;
- `build/21c/save_resume-sdl/run-35552-290616812/processes.log`.

### Native images actually inspected and remaining boundary

**14 native 320x200 BMPs were actually visually inspected**, seven per mode:

| File prefix (under build/21c/myra-direct or myra-sdl) | Direct suffixes | SDL suffixes |
| --- | --- | --- |
| root-1-q0-enter-receipt- | 0.bmp, 1.bmp | 1.bmp, 2.bmp |
| root-1-q1-full-enter-warning- | 0.bmp | 1.bmp |
| root-1-q1-full-enter-receipt- | 0.bmp, 1.bmp | 2.bmp, 3.bmp |
| root-1-q1-ineligible-enter-receipt- | 0.bmp, 1.bmp | 1.bmp, 2.bmp |

All reward pages were readable with expected fields, owners or loss reasons,
pagination and intact scene/HUD. SDL capture suffixes count the acknowledgment
sequence across phases, whereas direct suffixes use each phase's page index.
Other emitted dialogue/rest/rebuild and restart frames were not newly visually
inspected; their automated frame/state comparisons are distinct evidence.
No physical-window validation was performed or required for this 21C task.

Final diff review and `git diff --check` passed. Only the seven production files,
eight existing test files and these three documentation files changed; no staged
changes, commit, push, tag, branch switch or history alteration. No known failing
check remains. **21C independent review is pending; 21D is unstarted and
unauthorized; Milestone 21 is incomplete.** No later implementation is authorized
by this result.

## 17. 21D bounded restart and physical acceptance

This section preserves the implementation agent's evidence and original physical
handoff. Its then-outstanding closure gates are satisfied by section 18 below.

### Authority, baseline and implementation boundary

The 2026-09-08 assignment explicitly authorized this bounded 21D implementation.
Preflight found clean `main`, HEAD and the **local tracking reference** `origin/main`
at `f80ed5148e0c97765f71ea26c9ba2d4f67e78d19`, parent
`be98bb6dc8ab0c5ce6d02f3f3e29cf9e5563423c`, preceded by `1dab6ee`, `b5b1e5b`,
`3625380` and `a449bb7`. Staged and working diffs were empty. No synchronization,
branch change or reversion was performed. Gabriel supplied Astra's **APPROVE 21C
FOR COMMIT**, with no P0/P1/P2/P3 findings; this is not an audit performed by the
21D implementation agent. Section 16's pending-review wording is historical.

Only `tests/SaveResumeIntegrationTest.cpp` changes executable behavior. It extends
the existing `mmodern_save_resume_smoke` with `myra-exchange` and
`--manual-myra-exchange <game> <save-path.mmsave>`. There is no production correction,
new executable, commercial-data CTest registration, serializer or persistence
schema change. The four pre-exchange checkpoints retain their meaning, including
cumulative's pre-exchange producer and its individual-plus-combined cache matrix.
Graphics `save-phirna/resume` and `save-idle/resume-idle` remain unchanged. The old
Graphics exchange proposal is superseded: its Root-owned resume oracle does not
apply after consuming the Root.

The harness borrows original providers and live read-only party/flags through
`Application::playGameplay` / `observeGameplay`. Camera positioning is the only
gameplay setup mutation. Its local phase checks are shared by direct input,
actual SDL key mapping (including F9), and the physical input callback. The
physical path uses one continuous `SdlWindow::showInteractive` loop, no key
injection, automatic acknowledgment/save or automatic closer. Only a completed
Myra request and completed Phirna input cause camera repositioning, after the
Application callback has returned. Console phase messages are flushed and
Application feedback/original dialogue remain visible.

### Independent oracle and producer boundaries

Each process independently loads defaults and rechecks ordered membership
`[0,18,14,11,1,6]`, 35 occupied records, all miscellaneous empty-slot bytes zero,
active eligibility and miscellaneous tail capacity. The live prerequisites are
checked again before returning to Myra; mismatches fail without fixture repair.
Expected characters start from original defaults. The sole item effect is five
explicit `{material=10,id=37,state=1,frame=0}` records at roster 0 miscellaneous
slots 0..4. Expectations never call delivery/insertion/compaction/finalization,
capture mutated owners, or use a decoded actual save as expected state.

The expected typed snapshot includes the original resource signature, Clouds
map 23 `(9,11)` West, unchanged ordered membership/all modeled fields/all 30
owners/four nine-slot categories/all four bytes including empty slots, Root
index 17 = 0, Q2 = false, all unrelated counters/quest flags/game flags unchanged,
disabled object `{Clouds,23,13}` and events `{Clouds,23,125..135}` only. Removal is
independent of Root ownership. Existing typed comparators, effective-event checks,
original geometry/entity comparisons and selection/draw checks enforce this.

The new producer genuinely completes Myra request (**5 instructions**, Q2 set),
Phirna Space/Yes/ack (**18 instructions**, one Root and removal), then Myra return
(original line 7/offset 244 pending NPC). Root/Q2/items remain unchanged until
final return acknowledgment. The typed receipt checks **9 instructions**, one
receipt report, five exact delivered entries/owners, no loss/overflow/invalid/
discard, and an already emptied transient queue after synchronous delivery.
The ordinary fixture produces no capacity warning. Existing warning/loss and
reentrant Application F9 regressions remain passing.

F9 assertions surround the Application handler itself, comparing all live durable
state, native pixels/palette, page/count, generation, report count, blocking and
all portrait-timing fields. They require production refusal and no file while
the NPC or any receipt page is pending. Later idle portrait animation is not
treated as an F9 mutation. Reporting must remain blocked throughout continuation
and finalization. Nonfinal receipt acknowledgments advance one page without
changing the execution generation; only the final acknowledgment completes.
Live `blocksGameplay`/generation/presenter checks establish that the owned
continuation has ended; an old report copy is never used as live ownership.

An idle/read-only observation after completion proves no refused save was
deferred. A **new** Application F9 must give success feedback, preserve live state
and create the intended file. Independent disk reading checks v2 and full typed
equality with the oracle before producer success. The explicit snapshot/format
contract omits transients; pending-save exclusion and clean fresh consumer owners
verify that boundary without another serializer or binary name searches. V1/v2
compatibility remains unchanged.

### Process, reconstruction and native evidence

All evidence below is under ignored output:
`build/21d-b21a82ce43c94755b4e943a67cab817f` (called `$b` below).

| Mode / run directory under `$b` | Exchange producer | Typed consumer | Fresh | Actual CLI |
| --- | --- | --- | --- | --- |
| `save_resume-direct/run-42112-294773828` | 27440 | 17652 | 36796 | 42824 |
| `save_resume-sdl/run-9844-294785203` | 33692 | 37460 | 40228 | 36112 |

Every listed process exited **0**. Each run's exact save is
`<run-directory>/myra-exchange.mmsave`. `processes.log` contains complete commands,
paths, PIDs, exit statuses and assertion markers; individual
`myra-exchange-{producer,consumer,fresh,cli}.log` files retain detailed output.
The coordinator waits for successful producer exit before starting the consumer.
Across all five checkpoints and both modes, **30 Application children plus ten
actual CLI children** passed. The separate initial producer/consumer iteration
probes are retained under `exchange-probe` and are not counted in this matrix.

Consumers use `Application::playGameplay(..., resume=true)`, no prepared party or
replayed setup. Preflight/composition, observer and first-show assertions verify
restored ownership/camera before gameplay, clean presentation/timing and **zero**
initial automatic dispatch. Independent fresh children load defaults without the
exchange file and retain **one** initial automatic dispatch. The additional
`mmodern.exe --load-game` child loads the same file, checks exact camera and setup
inventory/Root/Q2/slot diagnostics, exposes a process-owned Windows SDL window,
and closes normally via the existing launcher. This is automated Windows SDL
coverage, separate from the SDL dummy/software drivers and from human observation.
Consumer, fresh control and CLI leave every producer file byte-for-byte unchanged.

The new exchange consumer performs only **one combined eviction**, before its
first Myra revisit: map, script, text and sprite caches, then `refresh(true)`.
The original Castle question/No control warms and reuses script/text providers;
camera-only positioning restores Myra and normal input clears passive text.
The exact counter transitions in **both** modes are map `4->8`, object `3->6`,
script `2->3`, text `1->2`, sprite `22->44`. State remains Root=0/Q2=false with
five items and Phirna removed throughout this reconstruction. The fresh consumer
process reconstructs the ownership graph; `refresh(true)` reconstructs caches
and composition, not owners. First Myra revisit then selects the no-Root request,
completes five instructions, sets Q2=true and adds no item or removal. There is
no second save/restart cycle.

Native pixel/palette assertions compare the clean first scene and rebuilt scene.
The exchange resumed/fresh BMPs legitimately match at Myra despite different
typed state. Existing checkpoint-specific relationships remain unchanged.
The implementation agent **actually visually inspected** these four native BMPs
using the local image viewer (all relative to `$b`):

- `save_resume-direct/run-42112-294773828/myra-exchange-consumer-first.bmp`
- `save_resume-sdl/run-9844-294785203/myra-exchange-consumer-first.bmp`
- `save_resume-direct/run-42112-294773828/myra-exchange-rebuilt.bmp`
- `save_resume-direct/run-42112-294773828/myra-exchange-receipt.bmp`

The first/rebuilt images show a clean outdoor scene, six portraits and no NPC or
receipt overlay. The inspected receipt is page one of the ordinary two-page
receipt; automated page checks cover both. Other generated BMPs and byte/pixel
comparisons are not additional visual inspections. None is a physical test.

### Commands and automated results

The dependency paths were confirmed from `build/21c/CMakeCache.txt`; source HEAD
is clean at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Git reads used a per-command
`-c safe.directory=...` override for the sandbox account, without changing Git
configuration. Existing `config.log` records `--backend=sdl --disable-all-engines
--disable-detection-full`, `SDL_CONFIG=/ucrt64/bin/sdl2-config`; accepted external
artifacts were reused without rebuilding/modifying the dependency.

These are the executed command forms, with the actual fresh build path. External
commands were logged and exit-checked; a failure stopped its command sequence.
`validate.ps1` and `commands.log` under `$b` retain the exact validation invocation,
per-command logs and SDL environment restoration. Repetition requires new output
directories/save targets, especially the Graphics controls.

```powershell
Set-Location 'D:\Projetos\MModern\mmodern'
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
$b = 'build/21d-b21a82ce43c94755b4e943a67cab817f'
$game = 'F:/Games/gog/Might and Magic 4-5'
cmake -S . -B $b -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DSCUMMVM_SOURCE_DIR=D:/Projetos/MModern/scummvm-known-good-candidate -DSCUMMVM_BUILD_DIR=D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64
cmake --build $b --parallel 4
cmake --build $b --parallel 4 --target mmodern_save_resume_smoke mmodern_myra_smoke mmodern_phirna_smoke mmodern_who_will_smoke mmodern_graphics_smoke mmodern_party_smoke
ctest --test-dir $b -N
ctest --test-dir $b --output-on-failure -R '^(xeen_(event_decoder|event_interpreter|event_system|quest_items|quest_grants|quest_flags|item_reward|reward_execution|reward_flow|reward_gameplay|reward_sdl|npc|npc_sdl|who_will|who_will_sdl|visual_remove|save_state|save_format|save_file|save_flow|save_cli|save_sdl)|sdl_input)$'
& "$b/mmodern_party_smoke.exe" $game
foreach ($suite in @('save_resume','myra','phirna','who_will')) {
    & "$b/mmodern_${suite}_smoke.exe" $game "$b/$suite-direct"
    if ($LASTEXITCODE -ne 0) { throw "$suite direct failed" }
}
$priorVideo = $env:SDL_VIDEODRIVER; $priorRender = $env:SDL_RENDER_DRIVER
try {
    $env:SDL_VIDEODRIVER = 'dummy'; $env:SDL_RENDER_DRIVER = 'software'
    foreach ($suite in @('save_resume','myra','phirna','who_will')) {
        & "$b/mmodern_${suite}_smoke.exe" $game "$b/$suite-sdl" sdl
        if ($LASTEXITCODE -ne 0) { throw "$suite SDL failed" }
    }
    & "$b/mmodern_graphics_smoke.exe" $game save-idle escape "$b/graphics-controls/idle.mmsave"
    & "$b/mmodern_graphics_smoke.exe" $game resume-idle escape "$b/graphics-controls/idle.mmsave"
    & "$b/mmodern_graphics_smoke.exe" $game save-phirna escape "$b/graphics-controls/phirna.mmsave"
    & "$b/mmodern_graphics_smoke.exe" $game resume escape "$b/graphics-controls/phirna.mmsave"
    ctest --test-dir $b --output-on-failure
} finally {
    $env:SDL_VIDEODRIVER = $priorVideo; $env:SDL_RENDER_DRIVER = $priorRender
}
git diff --check
git diff --stat
git status --short
```

Results: fresh Debug configure/build and six required smoke builds **passed**;
CTest list contains **58 unique registrations**, focused subset **23/23**, full
suite **58/58** (not 81 unique tests). Original party passed all 1080 item-slot
byte comparisons; Myra's 18 matrix cases plus two loss fixtures/revisits passed
per mode; Phirna/WhoWill direct and SDL controls passed; all four Graphics
save/resume controls passed. Logs: `configure.log`, `build.log`,
`smoke-build-2.log`, `smoke-build-final.log`, `handoff-build.log`, `ctest-{list,focused,full}.log`,
`party.log`, `{save_resume,myra,phirna,who_will}-{direct,sdl}.log`, and
`graphics-{save-idle,resume-idle,save-phirna,resume-phirna}.log`.
An initial smoke compile found a test-side use of unsupported item `operator==`;
the local four-byte comparison corrected it before successful validation. No
production defect or failed final test remains.

Original archives were hashed before and after: XEEN.CC SHA-256
`f8a00fa2c75799c131ed62057c7c3b61e8afbc292db7d6d53264e7a41ec46636`, DARK.CC
`7cbaffab761e54c3f31a994ce25a6795f92dbc696181a5c74cc318bba688bf0e`, INTRO.CC
`d531d54abccdf8b30bd5a43b9491fa4e9ab0dd0af9e5c85e787465b7a87516d7`.
`archives-before.log` / `archives-after.log` and `dependency-before.log` /
`dependency-after-native.log` retain preservation evidence. The final dependency
comparison uses the same `C:/Program Files/Git/cmd/git.exe` as preflight. An
intermediate PATH-selected MSYS Git check (`dependency-after.log`) reported CRLF
differences because it lacked native Git's system `core.autocrlf=true`; the
same-client final HEAD/status comparison is unchanged and clean. No configuration
or source repair was performed. Final diff checks leave only
the acceptance test, README and the two M21/current-status documents changed.
No staging, commit, push, tag, dependency change or commercial-data change is part
of this handoff.

### Physical handoff and closure gate

The implementation agent has not launched or claimed Gabriel's physical test.
Use this exact PowerShell setup; it creates a unique ignored output directory and
retains `$save` for the subsequent separate process. The harness rejects a stale
target or missing parent and uses production path containment checks to exclude
the commercial installation. No stale target is deleted.

```powershell
$build = 'D:\Projetos\MModern\mmodern\build\21d-b21a82ce43c94755b4e943a67cab817f'
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
$game = 'F:\Games\gog\Might and Magic 4-5'
$run = Join-Path $build ('manual-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
$save = Join-Path $run 'myra-exchange.mmsave'
$env:SDL_VIDEODRIVER = 'windows'
Remove-Item Env:SDL_RENDER_DRIVER -ErrorAction SilentlyContinue
& "$build\mmodern_save_resume_smoke.exe" --manual-myra-exchange $game $save
if ($LASTEXITCODE -ne 0) { throw 'Physical producer did not complete its checks' }
```

After the producer window and process have fully exited:

```powershell
& "$build\mmodern.exe" --load-game $game $save
```

1. Press Space at Myra; see and acknowledge both original request pages.
2. At the positioned Phirna checkpoint, press Space, Y and acknowledge.
3. At Myra again, Space opens the return; press F9 while it is pending and
   observe refusal without dialogue advancement, then acknowledge.
4. Observe the receipt, press F9 while it is pending, then finish both pages
   without skipped or duplicate input.
5. After presentation ends, press a new F9 and observe successful saving plus
   the disk-oracle PASS. Exit the producer completely.
6. Run the separate load command. Before Space, observe a clean first scene
   without NPC/reward replay; use setup/I diagnostics to confirm five roster-0
   miscellaneous rewards, Root=0 and Q2=false.
7. Revisit Myra with Space and acknowledge the request; I must show the same
   five rewards, Root=0 and Q2=true.

The producer cannot report completed checks after early exit, a skipped pending
NPC/receipt F9 observation or an unexpected save. Its flushed output prints the
exact save path and ready-to-run CLI command. This does not certify normal travel,
and no manual capacity/all-key matrix or third fresh process is required.

At this implementation handoff, required next evidence was Gabriel's physical
producer/consumer observation, followed by Astra's independent implementation/final audit. This implementation
report is not either result, does not grant 21D final acceptance, and does not
close Milestone 21. Do not commit this candidate or create a milestone tag/push
without subsequent explicit authorization.

## 18. Final 21D acceptance and Milestone 21 closure

### Evidence provenance and independent approval

The user supplied Astra's final independent verdict:

**APPROVE 21D FOR FINAL DOCUMENTATION/CLOSURE**

**No P0/P1/P2/P3 findings.**

This documentation-only closure records that verdict and Gabriel's physical
results; it is not another implementation audit or a new validation run by the
closure agent. Astra independently verified `main` at
`f80ed5148e0c97765f71ea26c9ba2d4f67e78d19`, exactly the four expected candidate
files modified, and no production or CMake changes. It confirmed preservation
of the accepted 21A/21B/21C architecture and the continuous physical-window
harness. Astra's automated/native-file inspection is distinct from Gabriel's
physical observation; no physical-window observation is attributed to Astra.

The independent audit confirmed genuine original Myra request -> Phirna
acquisition -> Myra return state creation; refusal of F9 during pending return
and reward presentation; no deferred save; and production Application F9 writing
the acceptance save. It verified the independent durable-state oracle, distinct
producer/consumer/fresh/CLI processes, actual production `mmodern.exe --load-game`
restart, exact restored post-exchange state, no exchange/reward replay or
duplicate rewards, combined cache reconstruction/reloads, post-resume Myra
revisit and fresh-process contamination control.

### Astra's fresh validation

Audit build: `D:/Projetos/MModern/mmodern/build/21d-astra-audit-20260909`.
The supplied audit reports:

- **23/23 focused tests** and **58/58 unique full CTest registrations** passing;
  the focused tests are a subset, not 23 additional registrations.
- Both five-checkpoint coordinators passed, including direct and SDL exchange
  consumers: **30 Application children and ten actual Windows SDL CLI children**,
  all exit **0**.
- Combined provider reload counters: map **4->8**, object **3->6**, script
  **2->3**, text **1->2**, sprite **22->44**.
- Relevant native BMP evidence inspected by Astra; original archives unchanged;
  pinned ScummVM dependency clean; `git diff --check` passed.

These fresh independent results are separate from section 17's implementation
agent runs. No full test suite was rerun merely to update closure prose.

### Gabriel's physical-window acceptance

There were **two attempts**. The first advanced before the required pending F9
observation and failed with:

```text
return advanced without required pending F9/final acknowledgment
manual producer incomplete: early exit or skipped required observation
```

**The first attempt is excluded from acceptance evidence.**

The second attempt completed successfully. Gabriel physically observed the
genuine Myra request, Phirna acquisition and return to Myra; F9 refusal during
the pending Myra return and pending reward receipt; and successful F9 saving
only after reward presentation completed. He fully exited the producer, then
launched a separate production CLI load and observed a clean resumed game
without automatic exchange/reward replay.

Five expected rewards persisted. On revisiting Myra she requested the Root
again; after acknowledging that request, exactly the same five rewards remained
and no duplicate reward appeared. The visible records were **Owner 0,
Miscellaneous slots 0..4: M=10 ID=37 S=1 F=0**, with **slot 5 empty**.
This records the supplied successful physical sequence, not a claim that Gabriel
manually inspected every durable field or certified travel between checkpoints.

Astra independently decoded the retained successful manual save and reported
complete byte equality with the candidate automated and fresh-audit exchange
saves: **five compared files, each 6,974 bytes, all byte-identical**. This supports
the durable-state result separately from Gabriel's visible observations. Astra
also verified that the existing inventory diagnostic header includes Root and
Q2; no diagnostic production-code correction is required.

### Final acceptance conclusion

Gabriel's successful second physical attempt and Astra's independent approval
satisfy the final bounded 21D acceptance gates. **21D is accepted and Milestone
21 is complete; M21 is now the current stable completed milestone.** Sections
12-17 retain the earlier implementation, review and handoff evidence; their
pre-closure status statements do not override this final conclusion.

The accepted scope is the bounded original local exchange, item ownership,
reward lifecycle, save/restart durability and specified controls. General
inventory/equipment use, broader reward modes, combat, Darkside and normal-route
certification remain outside scope. M22 is the roadmap's default successor, with
its established review cadence; this closure does not authorize implementation.

The closure step changes documentation only and preserves the approved
`tests/SaveResumeIntegrationTest.cpp` candidate unchanged. Production, CMake,
test registration, dependencies and commercial resources are untouched. No
staging, commit, push or tag was performed in this documentation-only closure.
