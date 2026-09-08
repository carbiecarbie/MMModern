# Milestone 21 - Myra's return exchange and bounded item rewards

**Status: draft awaiting approval, 2026-09-08. Planning only; no implementation
or stage is authorized by this document.**

Recommendation: retain one milestone with four small, separately authorized
stages. The necessary prerequisite is bounded character item storage plus its
save extension, not a general inventory/equipment/treasure engine. No substantial
separate roadmap milestone is justified by the inspected path. This conclusion
depends on approving the bounded presentation and error policies below. The
roadmap is unchanged; implementation must not start automatically after review.

In this document **Verified** means inspected current code, pinned reference or
the focused original-data checks recorded below. **Decision** means a recommended
MMModern contract awaiting approval. Future acceptance is not a claim of tests
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

Provide narrow reusable operations for category tail-capacity, party global
tail-capacity, bounded misc insertion/compaction and delivery result reporting.
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

Replace Myra's successful root-path assertions for `UnsupportedOperationMode`
at line 8 (including the cumulative M20 restart test). Retain NPC failure and
abandonment before acknowledgment, unrelated unsupported opcode/mode cases,
no-root/SP/Q2 variants, final Escape, source diagnostics and cache regressions.
Do not globally remove expected errors or compare all characters to initial
defaults after exchange. Tests for unknown version 2 must move to another
unsupported version; do not weaken version rejection. Synthetic modifier
initializers must explicitly supply the new ID field without shifting old values.

## 9. Final original-resource and physical acceptance

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

Each stage requires explicit authorization after plan approval. Stage completion
does not authorize the next stage. No implementation has begun.

### 21A - Authoritative item records and persistence

Objective: preserve initial and saved complete records before any new exchange
can mutate the party. Prerequisite: approval of sections 4 and 7. Likely files:
character/party/format/rules files, SaveSnapshot/Format/State, comparison helpers,
character/save tests and PartyIntegrationTest. The existing file protocol should
need only compatibility tests, not a rewrite.

In scope: four typed arrays, narrow capacity/compaction helpers, exact initial
loading, v2 codec and v1 resolution/replacement policy. Out of scope: event
consumption/clear activation, GiveEnchanted execution, receipt and item use.
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

21D must extend `mmodern_graphics_smoke` with a named `save-myra-exchange` mode
that uses original script inputs and camera-only checkpoints, then run that
mode with a new save path and the existing `resume` mode against its file.
These are proposed mode requirements; `save-myra-exchange` does not exist yet.
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

Planning changed only this new draft and the minimal project-status pointer.
Read-only baseline/source/test inspection, the original event diagnostic and
focused in-memory inventory/provenance checks were performed. The optional party
smoke executable was absent; no build/CTest, exchange execution, saved-result
acceptance, native image inspection or physical-window check was performed here.
Final document review and whitespace/scope checks are recorded in the task report.
No implementation, test/helper/config change, commercial-data write, commit,
push, tag, branch switch or later milestone work is part of this task.
