# Milestone 42 - Bounded Ironworks equipment purchase

## Proposed scope and authorization boundary

This is the proposed M42 implementation contract, ready for independent
specification review. It records no M42 implementation or acceptance. The
maintainer authorized targeted investigation and specification after accepting
the post-M41 roadmap direction; implementation requires separate authorization.
The baseline is `main` at `e27262984a32f3a8b60f4c6d874fd59baa1582e3`,
`Update post-M41 roadmap with equipment purchase and recovery priorities`,
whose sole parent is accepted M41 implementation
`0c9877efb2e92bd8b6b1049c0d369cc13da2c7b9`. The baseline commit changes only
`docs/roadmap.md`. HEAD, local `origin/main` and direct remote main agreed, with
a clean worktree and empty index, before investigation.

M42 connects actual generated Ironworks stock to carried-gold payment, physical
inventory delivery, stock depletion, a valid mandatory departure, legal existing
transfer/equipment, an existing gameplay improvement and exact quiet restart.
Repeated purchases, cancellation/refusal, return visits and later full restock
are part of this boundary. A menu alone does not complete it.

The successor is **envelope v4 / Journey schema 9 / content 13**. It inherits
content 12's prepared Regional Journey, twenty-eight Vertigo cells, original
Events and actor closures, Repair, Training, casting and consequences. Only the
original Clouds Ironworks `(28,8,4)`, Event record 0/action 1, gains Buy. There
is no new route, actor admission or unrestricted original startup.

The natural homes for inherited machinery are:

- [Stable capabilities](project-status.md), [accepted direction](roadmap.md#near-term)
  and [dependency/provenance policy](dependencies.md).
- [M38 repair, route and ownership](milestone-38-plan.md), including its
  [exact repair rules](milestone-38-plan.md#exact-repair-rules).
- [M40 complete generation](milestone-40-plan.md#complete-stock-algorithm-and-tables),
  [service publication](milestone-40-plan.md#coordination-publication-and-failure)
  and [wire contract](milestone-40-plan.md#wire-contract).
- [M41 Training](milestone-41-plan.md#training-rules-and-state-ownership),
  [individual service days](milestone-41-plan.md#individual-service-days-and-departure-reservation)
  and [native input](milestone-41-plan.md#narrow-continuation-and-native-input).
- [M24 inventory/transfer](milestone-24-plan.md) and
  [M25 equipment](milestone-25-plan.md#equipment-domains-and-frame-contract).

Those contracts retain their meanings for legacy content. Explicit changes
below apply only to content 13 and its inherited consumers.

## Evidence and decisive findings

### Sources and evidence classes

Repository implementation/tests were inspected at the verified baseline.
Configured dependency paths were discovered from the existing CMake cache,
then the external ScummVM checkout was verified at detached, clean
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with configured `config.h` and the
four required build artifacts. No dependency or commercial file was changed.
Paths are configuration, not part of this contract; see [dependencies](dependencies.md).

| Evidence | Decisive inspected sources |
| --- | --- |
| Current Smith/Event ownership | [Smith Flow](../src/app/XeenSmithFlow.cpp), [Training Flow](../src/app/XeenTrainingFlow.cpp), [Encounter interface](../src/app/XeenEncounterFlow.h), [Event interface](../src/app/XeenEventFlow.h), [inventory Flow](../src/app/XeenInventoryFlow.cpp) |
| Current economy/admission | [Economy validator](../src/games/xeen/XeenServiceEconomy.cpp), [generation](../src/games/xeen/XeenMerchantGeneration.cpp), [tables](../src/games/xeen/XeenMerchantTables.h), [service-day candidate](../src/games/xeen/XeenServiceDay.cpp), [content policy](../src/games/xeen/XeenJourneyContent.h), [Journey rules](../src/games/xeen/XeenJourneyRules.cpp), [initialization](../src/app/XeenJourneyFlow.cpp), [route](../src/games/xeen/XeenVertigoRoute.cpp) |
| Physical items and consumers | [Tail/compaction primitives](../src/games/xeen/XeenCharacter.cpp), [transfer](../src/games/xeen/XeenItemTransfer.cpp), [equipment](../src/games/xeen/XeenEquipment.cpp), [physical combat](../src/games/xeen/XeenCombatRules.cpp), [character rules](../src/games/xeen/XeenCharacterRules.cpp), [inventory display](../src/games/xeen/XeenInventoryView.cpp) |
| Persistence and integrity | [Save format](../src/formats/xeen/XeenSaveFormat.cpp), [capture/restore](../src/games/xeen/XeenSaveState.cpp), [Quiet certificate](../src/games/xeen/XeenJourneyCapture.h), [full guard](../src/games/xeen/XeenRestoreGuard.h), [state equality](../src/games/xeen/XeenStateEquality.h), [party/roster protections](../src/games/xeen/XeenParty.cpp) |
| Inherited tests | [Economy vectors](../tests/XeenServiceEconomyTests.cpp), [original initialization](../tests/XeenServiceDayInitializationTests.cpp), [Smith process](../tests/XeenSmithProcessTests.cpp), [service process](../tests/XeenServiceDayProcessTests.cpp), [Training rules](../tests/XeenTrainingRulesTests.cpp), [Flow](../tests/XeenTrainingFlowTests.cpp), [input](../tests/XeenTrainingInputTests.cpp), [allocation](../tests/XeenTrainingAllocationTests.cpp), [resources](../tests/XeenTrainingOriginalTests.cpp), [process continuation](../tests/XeenTrainingProcessTests.cpp), [shared scheduling](../tests/XeenInputSchedulingTests.cpp), [first-key responsiveness](../tests/XeenNativeResponsivenessTests.cpp) |

Pinned-reference interpretation uses only these relevant upstream files:

- [ItemsDialog](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp):
  `execute`, `loadButtons`, `setEquipmentIcons`, `calcItemCost`, `doItemOptions`.
- [Party/BlacksmithWares](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp):
  `getSlotIndex`, `blackData2CharData`, `charData2BlackData`, `subtract`, `addTime`.
- [Locations](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp):
  `BaseLocation::show`, `BlacksmithLocation::doOptions/farewell`.
- [Items](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp):
  `XeenItem::clear`, `InventoryItems::isFull/sort`, restriction/equipment paths.
- [Numeric constants](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp):
  `BLACKSMITH_MAP_IDS`, weapon/armor costs, `ITEM_SKILL_DIVISORS`, damage and armor tables.

Reference source interpretation, original-resource observations, MMModern
adaptations and temporary numerical/route diagnostics are distinct. None is
physical DOS observation. Later evidence must distinguish deterministic
rule/Flow/fault tests, original-resource/reference checks, automated application
process continuation and maintainer physical native-SDL acceptance.

### Original Buy sequence and scratch writeback

The location selects an active recipient with F1..F6; Browse opens Buy, initially
Weapons. `getSlotIndex` selects the current numeric side and matching original
map: Clouds map 28 is side 0/shop 0 in `{28,30,73,49}`. M42 admits that exact
identity, rather than the reference's unmatched-map fallback to shop 0.

`blackData2CharData` copies **all four categories and all nine records** from
that shop to `_itemsCharacter`. Its class is the selected recipient's class.
`setEquipmentIcons` then writes scratch frames for Weapons and Armor, including
empty records; Accessories ID 1 is actually changed to ID 8, while other
accessory branches assign frames. It is not a read-only icon projection.
Restriction checks affect display icons but do not gate the Buy transaction.
Switching the recipient changes `_oldCharacter`/`startingChar` and scratch class,
without reloading wares. Buy can switch to Repair within the same item dialog.

The reached Buy branch checks the selected recipient's category **last slot**
first. If it is occupied, it refuses before confirmation or funds testing.
Otherwise it calculates cost, asks confirmation, and on Yes subtracts party
gold. Success copies the selected scratch record to recipient slot 8, clears
its delivery frame to zero, clears the scratch source, then stably compacts both
categories. Cancellation or insufficient funds performs no Buy transfer/debit.
There is no recipient `canAct`, class-proficiency or automatic-equip check here.

On normal item-dialog exit, `updateStock`, set on initial Buy entry, causes
`charData2BlackData` to write **all** scratch categories back, including scratch
normalization unrelated to the selected purchase. Escape can therefore write
stock even without a transaction. `BlacksmithLocation::farewell` adds no Clouds
extra time/audio; the existing location departure owes its one `addTime(1440)`.

**Bounded adaptation:** retain authoritative M40 merchant bytes, and draw an
immutable catalog/UI projection instead of entering this scratch writeback
path. Do not persist equipment-icon frames, mutate accessory IDs, normalize
unsupported records or rewrite untouched categories merely to browse. Only an
admitted successful purchase removes a selected stock record. This is an
intentional deviation from scratch/display side effects, not a claim that
original browsing/cancellation is mutation-free. Reproduce the actual admitted
Buy price, recipient, tail-capacity, unequipped delivery and stable compaction
rules. Continue M38's direct Repair path without importing Buy scratch effects.

### Verified initialization and current reachable route

A read-only diagnostic through the existing original-resource adapters and
fresh content-12 initialization confirmed:

| Fact | Value |
| --- | --- |
| Input seed | `7`, count initially zero |
| Published post-generation world cursor | algorithm 1, state `1652828136`, raw count `901` |
| Original prepared carried purse | gold 800, gems 10; shared bank 0/0 |
| Side 0/shop 0 Weapons, occupied order | `(0,10,0,0)`, `(0,6,0,0)`, `(0,15,0,0)`, `(0,10,0,0)`, `(0,6,0,0)`, `(0,4,0,0)`, `(37,20,0,0)`, `(40,16,0,0)` |
| Side 0/shop 0 Armor, occupied order | `(0,6,0,0)`, `(0,4,0,0)`, `(0,6,0,0)`, `(0,3,0,0)`, `(0,5,0,0)`, `(40,8,0,0)`, `(48,6,0,0)` |
| Other shop-0 categories | Accessory `(21,2,0,0)`; Miscellaneous `(6,1,7,0)`, `(9,8,8,0)`, `(1,6,4,0)`, `(8,13,3,0)` |
| Remaining records | All-zero tails, including slot 8 in every category |
| Tyro (owner 18, Knight), original equipped item | Weapon `(0,2,0,1)`; body armor `(0,2,0,3)` |
| Existing useful consumers | Weapon 2 is `2d3`, weapon 6 is `4d2`; armor 2 contributes 4 AC, armor 3 contributes 5 AC |

The independent stock vector already recorded in
[M40](milestone-40-plan.md#independently-checkable-numerical-vectors) agrees with
seed 7's state/count. Seed `3626689381` producing **post-generation** state
`7`/count `886` is a different input and witness; do not substitute that fixture
for fresh seed 7.

A temporary application diagnostic reused the existing
[M41 production prefix](milestone-41-plan.md#production-acceptance-boundary)
and its [input policy](../tests/XeenM41CliWitness.cpp), stopping before service.
After the original well, earned mainland combat/treasure, learned First Aid,
Vertigo entrance and entrance-Slime fight, it reached `(28,15,4,North)` at day
8/minute 796/gold 870, cursor `799325555:1101`. Turning left and taking seven
ordinary corridor steps reached `(28,8,4,West)` at minute **803**, with the same
gold/cursor and Tyro's intact original armor and **AC 10**. Quiet F9 succeeded
at this pre-visit endpoint. No stock, purse, camera, actor or character was
rewritten to reach it. This automated diagnostic establishes existing route
feasibility, not M42 purchase, continuation or physical acceptance.

## Offer domain, price and exact item movement

### Separate predicates

All indexes below are zero-based; native item labels are physical slot +1.
The four bytes are `(material, id, state, frame)`, not a catalog description.

| Predicate | Exact meaning |
| --- | --- |
| Visible | Every physical row 0..8 in the four read-only Buy category views. ID 0 is shown as empty; occupied rows show catalog/raw identity and bounded support status. No filtering, rerolling or reordered price list. |
| Selectable | A current presented Buy frame may select its category/physical row and an active roster recipient. Empty and unsupported rows may be inspected; selection is not purchase authority. |
| Supported offer | Side 0/shop 0, category Weapons with ID 1..33 or Armor with ID 1..13, **material 0, state 0, frame 0**, in canonical content-13 wares. |
| Purchasable | A supported, deliverable occupied selection with current quote/preimages, active recipient identity, fresh confirmation, sufficient carried gold and protected continuing-departure/authority capacity. No class or condition-based Buy refusal. |
| Deliverable | Recipient category slot 8 has ID 0 before insertion, and the detached resulting inventory/party is valid under content 13 and existing character/equipment rules. A hole elsewhere does not substitute for a free tail. |
| Equippable | After delivery, the existing equipment operation accepts that owner/category/physical slot, proficiency, frame conflicts and arrangement. Purchase does not promise immediate equipability. |
| Supported gameplay contribution | When legally equipped, plain Weapons 1..29 use current melee dice; 30..33 use current physical Shoot on its inherited admitted mainland path. Plain Armor 1..13 uses current combat AC. Existing action/context and bad-state restrictions still apply. |

This is the principled **plain ordinary equipment** class, rather than just
two witness IDs or every catalog/equipment-accepted record. ID 34 is a special
weapon and not generated merchant stock. Accessories and Miscellaneous are
visible but cannot be purchased. Modified materials, weapon effectiveness
counter values, unknown IDs and other unsupported effects cannot be purchased.
Material 38 armor remains repairable under M38 but is outside M42 Buy's plain
domain. Future effect support must not creep in through a catalog or equipment
acceptance predicate.

Generated unsupported records retain exact bytes, ordering and deterministic
labels such as "Purchase unsupported". Do not invent a price for them from
the admitted price table, replace them with plain equivalents, discard them,
hide them to suggest a fully supported shop, or refill empty rows. Canonical
merchant generation/depletion never produces cursed/broken records or nonzero
frames: such bytes in authoritative wares are an integrity/restore rejection,
not an ordinary purchasable offer. Raw corrupt diagnostic views confer no
publication authority. Curses/breakage or other later legal changes to purchased
**carried** items follow existing inventory/combat/repair rules independently.

The recipient must be a stable current active roster ID, resolving the ordered
membership `{0,18,14,11,1,6}`. Any such owner may receive an item even when
individually unable to act or not proficient; the overall Journey must still
pass service admission. Do not add Training's `canAct` refusal to Buy. Inactive
owners are not selectable. Proficiency feedback may be informative but cannot
replace or enlarge the inherited equip/transfer rules.

### Pricing, confirmation and refusal precedence

`calcItemCost` forces Buy divisor index 0 independently of its skill argument;
`ITEM_SKILL_DIVISORS[0] = 1`. Material 0 has zero elemental addend, and admitted
state 0 has no effectiveness contribution. Hence:

```text
P = max(1, floor(baseCost[category][id] / 1))
require G >= P before debit
goldAfter = G - P                       # exact u32, no wrap/underflow
```

Use validated indexes, widened unsigned intermediates and full u32 comparisons.
There is no Merchant skill, level/class/condition surcharge, quantity discount,
bank payment, gem conversion or pending-treasure credit.

| Category and IDs | Base costs in matching ID order |
| --- | --- |
| Weapons 1..11 | 50, 15, 100, 80, 40, 60, 1, 10, 150, 30, 60 |
| Weapons 12..22 | 8, 50, 100, 15, 30, 15, 200, 80, 250, 150, 400 |
| Weapons 23..33 | 100, 40, 120, 300, 100, 200, 300, 25, 100, 50, 15 |
| Armor IDs 1..13 | 20, 100, 200, 400, 600, 1000, 2000, 100, 60, 40, 250, 200, 100 |

Weapon 7 costs 1, weapon 6 costs 60 and armor 3 costs 200. The admitted maximum
price is 2000. The positive minimum rule is retained even though these supported
base entries already exceed zero. Modifier arithmetic is deliberately not
admitted; do not import its tables or reverse its rounding to price excluded offers.

After frame/owner/guard validation, apply this order:

1. Validate site/category/slot and recipient; distinguish empty from unsupported
   occupied selection. Invalid live canonical state is an integrity failure.
2. For a supported offer, test recipient tail capacity **before** confirmation
   and funds. Full tail yields a fixed refusal; no debit or compaction occurs.
3. Quote the original price, owner, category, actual physical slot, exact record,
   current purse, gold-after or shortfall. An insufficient quote may be shown;
   do not underflow its preview. Cancel returns to Buy with no durable mutation.
4. A fresh confirmation consumes authority before callbacks. Recheck complete
   quote/guard/reservation identity, then funds. Exact funds succeeds to zero;
   one-less refuses. Pending gold cannot cover a shortfall.
5. Prepare delivery, exact depletion, continuing reservation, expected guard,
   fixed result and authority. Technical/capacity refusal preserves owners and
   the old mandatory departure. Publish only when all preparation succeeds.

A selected description, supplied price, byte-identical offer elsewhere or stale
slot cannot satisfy a changed preimage. Selection/category/recipient/mode
changes and any successful purchase invalidate the outstanding quote.

### Inventory and merchant transformations

Use `xeenItemHasTailCapacity`, `xeenInventoryItems`, `xeenSameItem` and
`xeenCompactItems` where their actual semantics match. Do not call the live
party-to-party transfer operation as though the merchant were another character.

Let `A` be the recipient's complete nine-record category and `S` the complete
selected merchant category, with occupied length `nS <= 8`. For a selected
`slot < nS` and `A[8].id == 0`, prepare:

```text
delivered = S[slot]; delivered.frame = 0
A1 = A; A1[8] = delivered; stableCompactByNonzeroId(A1)
S1 = S; S1[slot] = (0,0,0,0); stableCompactByNonzeroId(S1)
```

Occupied records keep relative order and all four bytes, except the delivered
frame becomes zero (already zero for admitted authoritative stock). Every empty
record in each **touched** category becomes all zero, matching original `sort`.
Empty metadata in untouched categories/owners remains exact. With `nA` occupied
recipient records before insertion, the delivered physical index after stable
compaction is `nA`, even when earlier holes existed. An occupied slot 8 refuses
despite holes at slots 0..7; an empty slot 8 accepts despite those holes.
Neither refusal nor browsing/cancellation compacts recipient or merchant data.

Merchant result is an occupied prefix of length `nS-1`, then zero records through
slot 8. Removing slot 3 from the verified Armor vector leaves IDs/materials
`(0,6),(0,4),(0,6),(0,5),(40,8),(48,6)`, then three all-zero records. A second
purchase addresses this resulting array, not the old row. The two weapon-6
offers are separate physical quantities; after purchasing slot 1, old slot 4
has shifted to slot 3. Old input/quote must not buy the item now at slot 1,
even if another record is byte-identical.

Only the recipient category, selected merchant category and carried-gold scalar
change durably at purchase. Preserve all other inventory/equipment, current
HP/SP (including above-maximum values), conditions, levels/XP, raw books,
membership, pending treasure, gems, bank and other shops/sides. No automatic
equip, healing, clamping, time, actor opportunity or gameplay RNG occurs.
Later transfer uses existing active-owner/tail/cursed-item restrictions; later
Remove/Equip uses existing class and conflict rules. An item cannot be equipped
over an existing conflicting body armor or melee weapon without legal removal.

## Canonical complete and purchase-depleted stock

### Representation and proof boundary

Retain `XeenServiceEconomy` and its fixed `2 x 4 x 4 x 9` four-byte records.
No history, seed, source ID, purchase ledger, occupancy field or second stock
owner is introduced. All categories retain zero frames, generation scalar
support, occupied prefixes of length 0..8 and all-zero tails including slot 8.
Only side 0/shop 0 Weapons and Armor can lose supported plain records.

The existing generated-only validator remains a distinct contract: each shop
must be a possible complete result of its ordered twenty calls, with insertions
until category count 8 and later capacity discards. Scalar bounds cannot replace
its schedule/order/capacity proof. Fresh initialization and every restock must
still pass this complete-output validation for **all eight shops**.

Content-13 settled wares require the other seven shops to pass that unchanged
proof, and shop 0/0 to pass the bounded deletion-closure proof below. Within
shop 0/0, Accessories/Miscellaneous cannot be omitted or modified under the
purchase relation. Runtime transitions additionally compare the complete exact
before/after arrays: this is what proves **actual** untouched records stayed
untouched, rather than a different possible generation history.

Snapshot validity proves existence of some permitted generated stock and a
sequence of supported deletions yielding the current bytes. It cannot prove the
actual historical seed/stock, how many past purchases occurred, payment history
or which duplicate used to occupy a slot. It also cannot detect every arbitrary
byte change that happens to produce another admitted possible state. This limit
already applies to complete generated-state validation. Do not label unsupported
deletion/mutation a legal runtime purchase: exact delta validation and full
mutation-history guards prohibit it. Do not claim a stronger historical guarantee
on restore. No new durable provenance is necessary for this bounded admission.

### Deterministic deletion-closure algorithm

Use the inherited finite `possible(level, category, record)` support predicate
and exact shop-0/0 schedule: **fifteen level-1 calls, then five level-2 calls**.
`anyPossible(level, category)` means that the unchanged generator can produce
some record in that category at that level. `deletable(level, category)` means
there exists a `possible` record also satisfying M42's supported-offer predicate.
For this schedule it is true only for level-1 Weapons/Armor; level-2 equipment
has nonzero material. These existence predicates do not call RNG.

First validate all nine records of each category and compute retained prefix
lengths `ellW, ellA, ellX, ellM`. The state for each call layer is:

```text
(gW, gA, kW, kA, kX, kM)
0 <= kW <= gW <= 8; 0 <= kA <= gA <= 8
0 <= kC <= ellC for C in W,A,X,M
generated insertion counts g = (gW, gA, kX, kM)
retained consumed-prefix counts k = (kW, kA, kX, kM)
```

Start with all counters zero. For each of the twenty ordered calls, consider
each category `C` independently from every reachable state:

1. If `g[C] == 8` and `anyPossible(L,C)`, discard that completed generated
   outcome: next state has unchanged counters. This is **generation capacity**,
   not a purchase deletion. Finish the call; do not refill a purchased hole.
2. If `g[C] < 8`, `k[C] < ellC` and `possible(L,C,stock[C][k[C]])`, retain that
   next physical record: increment its generated and retained counts. For
   X/M these are the same single counter.
3. If `g[C] < 8`, `C` is W/A and `deletable(L,C)`, admit a hidden **inserted
   plain offer subsequently purchased**: increment only `g[C]`. No retained
   record is consumed. No analogous omission exists for X/M or unsupported
   equipment. It counts toward the original generation capacity even though
   later purchase removed it.

Accept after exactly twenty layers iff all four retained counts equal their
lengths in at least one reachable state. Generated W/A counts may exceed their
retained counts by admitted deletions. Do not skip, reorder or normalize any
retained record. Do not discard below original capacity, generate after a
purchase freed space, or use the current retained count as generation count.
This recognizes any number of sequential purchases before the next restock,
including zero purchases and duplicate records, without RNG inversion.

Two fixed Boolean layers of at most `9^6 = 531441` states suffice for this one
shop; the other shops retain the `9^4` proof. Use bounded working storage
prepared outside publication, not unbounded backtracking or a large unsafe
stack allocation. Enumeration order and acceptance must be deterministic.
Equivalent compressed representations are permissible only with the same
transition relation and independent tests. No new authoritative cache is needed.

Examples/oracles:

- Every complete generated output is admitted with zero omission transitions.
  Every exact supported one-item depletion of an admitted state stays admitted.
- The verified seed-7 Armor array after removing slot 3 is valid, with other
  categories/shops unchanged. Legacy 11/12 cannot gain deletion transitions.
- An entirely empty **shop 0/0** can be valid: generate eight plain Weapons and
  seven plain Armor in level 1, discard five level-2 Weapons at original full
  capacity, then buy all fifteen inserted plain records. This is different
  from accepting all eight empty shops or deleting unsupported stock.
- More than five retained Accessories at shop 0/0 is impossible: level 1
  generates none, and only five level-2 calls exist. A ninth occupied slot,
  occupied-after-empty, nonzero empty metadata/frame, bad status bits, impossible
  level/material ordering or malformed unsupported category remains invalid.
- A complete literal shop whose twenty calls all yield Armor can deplete its
  eight inserted plain entries; seven such entries without a deletion proof
  fails legacy generated-only semantics. Use independent literal schedules,
  not the production validator itself, to establish expected results.

### Admission at every consumer

Keep generated-output validation explicit and content-independent. Introduce a
separate, explicit content-aware **current economy** admission: 11/12 retain
generated-only semantics; 13 permits the deletion closure; unknown content
rejects. Never change an old validator globally to accept missing records.

Thread the actual content through Journey party/melee validation, Smith
preparation/departure, shared service-day candidate original/ending economy,
Training preparation/quotes/publication/departure, capture, SaveFormat validation,
decode and SaveState restore/value validation. Fresh/restocked output uses the
strict generator proof before publication even under content 13. Equality,
combat/Event/disengagement preimages and restore guards retain every economy
byte and presence; they must neither regenerate nor classify successor stock
under legacy semantics.

`XeenTrainingFlow.cpp` currently passes literal 12 in multiple departure/party/
route/candidate calls; `xeenPrepareTraining` also validates under 12. Replace
those semantic assumptions with the checked actual Training-capable content,
explicitly `{12,13}`. Do not execute content 13 after silently mapping it to 12.
`XeenServiceDayCandidate` and `xeenPrepareSmithDeparture` currently explicitly
admit 11/12 and 9..12 respectively; extend only their successor capability.
`XeenJourneyContent`, explicit supported-pair checks and the Vertigo route
whitelist also need 13. Preserve content-specific route/actor/resource closure
selection: 13 inherits the Training route/closure, while 11 and earlier retain
their original domains. Fresh/restore Training source checks remain required.

For **Quiet saved day 8**, stock must still satisfy the complete generator proof:
no admitted visit can settle without advancing at least one day. Depletion at
day 8 is legitimate inside an active Smith visit, which is unsaveable. From
saved day 9 onward use content-13 deletion closure and inherited retained-city
requirements. This is a current-state necessary condition, not stored history.

## Ownership, quotes and purchase publication

The sole owners remain existing roster inventories, carried purse and pending
treasure, party-owned `serviceEconomy` and context, world-owned Journey RNG and
actors, and existing Event/Service/Presentation coordination. Extend the Smith
continuation and EventFlow UI, not a new shop scheduler, wallet, merchant
inventory/character, transaction ledger or generalized trading framework.
Detached rule candidates and immutable UI projections cannot publish.

A quote binds current owner incarnations, visit/operation/phase, concrete
presented origin, actual content/site/side/shop/category/physical slot, complete
selected stock category and all four selected bytes, recipient active index and
stable roster ID/order, complete recipient category, carried gold/price, full
guard, coordination revisions and the complete departure reservation identity.
Retain the complete economy/context/RNG preimages through that reservation and
the full owner/resource guard. A matching name or item elsewhere is irrelevant.

A pure Buy candidate prepares exact `A1`, `S1`, gold-after, delivered physical
slot and fixed outcome. Before publication:

1. Consume current confirmation and fence reentrancy; check retained full guard,
   exact quote identity, membership, tail, funds and old complete reservation.
2. Construct detached touched-category/purse/economy successors. Validate the
   exact one-removal transformation, all untouched bytes, current content-13
   stock and resulting character/party/equipment domain. Do not obtain broad
   assignment authority over marked or borrowed live owners.
3. Prepare the replacement departure described below, its identity increment,
   fixed result storage, complete expected-delta guard, mutation observation
   storage and coordination counter headroom. Compose any required precommit
   resources/feedback while the old reservation and full guard remain retained.
4. After the last callback, recheck old history/preimages and capacity. No
   allocation, validation, provider, rendering, observer callback or throwing
   operation may remain inside publication.

The one callback-free, nonthrowing success unit writes **only** the recipient
category, merchant category and carried-gold debit; installs the complete new
departure reservation/identity, fixed result and exactly-once published operation
marker; advances checked coordination revisions and adopts the precomputed
expected guard. Prevalidate physical indexes; use the existing observed item
arrays/scalars and not whole-party/roster swaps. Prepare nonthrowing storage
operations and lifetime/destruction requirements before entering the unit.

No observer can see payment without delivery, delivery without depletion,
depletion without payment, or committed purchase without a feasible departure.
Result facts include owner/category, original offer slot/record, resulting
recipient slot, exact before/after category values and gold/price, operation and
reservation identity. Fallible display/reporting follows publication and reads
that fixed result. Expected-guard replacement retains prior failure/history and
immutable resource union; it cannot bless callback-mutated live values.

Each purchase and each existing repair is a separate committed unit. Later
refusal, cancellation or technical failure preserves earlier payments/items/
stock. After publication, retry only unfinished feedback/continuation; never
debit, deliver, deplete or renew the reservation again. A new purchase requires
result acknowledgment, fresh selection/quote and a new confirmation edge.

## Complete departure reservation and service continuation

### Checked rebind of the existing Smith obligation

M40 Smith already reserves a complete one-day candidate, but `departSmith`
compares its **complete pre-purchase economy** to live economy. Keeping that
candidate unchanged after depletion is invalid. M41 demonstrates preparing a
replacement while retaining an old obligation, but its extra member-day chain
is unnecessary for Buy: purchase changes neither context nor RNG.

Use a narrow **checked purchase rebind** of the already-complete Smith candidate.
Do not run generation again or transplant Training's member-day machinery.
Let the retained reservation be:

```text
R = (identity, beforeContext C, beforeEconomy E, beforeRandom Q,
     endingContext C+, endingEconomy E+, endingRandom Q+, triggered, complete)
```

Require `R.complete`, exact checked live `C/E/Q` and the owning visit/lease.
Prepare a detached `E1` that differs from `E` by precisely the selected supported
stock deletion. Bank and every other category/shop/side remain exact. Prepare
`R1` with a fresh reservation revision:

| Field | Nontriggering departure | Triggering departure |
| --- | --- | --- |
| Before context/random | Same `C/Q` | Same `C/Q` |
| Before economy | Exact `E1` | Exact `E1` |
| Ending context | Same reserved `C+` | Same reserved `C+` |
| Ending wares | `E1.wares`, preserving depletion | Exact already-generated `E+.wares`, replacing all stock |
| Ending bank | `E1.bank == E.bank` | Exact already-prepared interest result `E+.bank` |
| Ending random | Same reserved `Q+ == Q` | Exact already-prepared `Q+` |
| Generation/interest work | None | Reuse all completed work; none repeated |

Bind the rebind operation to both reservation identity and complete original
candidate values; validate the exact authorized delta and all these equalities.
A narrow pure helper/factory on the existing candidate may supply this
transformation. It must not be a public "replace preimage from live owners"
escape hatch or expose mutable candidate storage to providers.

Allocate/copy/validate `R1` while `R` is intact. Immediately before purchase
publication check `R` and old full guard again. Only the same atomic purchase
unit replaces `R` with `R1`. Repeat this relation for every later Buy. If
preparation fails, discard only the optional candidate and retain `R` against
the actual last committed economy. Repairs only change purse/armor and require
no economy rebind; they still renew their exact full-owner guard and verify the
retained current departure.

At admission, complete stock/interest preparation and reserve authority before
arming debt, exactly as M40/M41. Triggering preparation uses the existing idle
loop and at most **64 raw RNG attempts per slice**, retaining private prefix,
pending rejection request and checked original cursor. No admitted optional
purchase may begin against incomplete departure generation. Purchase rebind
itself needs no RNG slices; any fallible preparation remains exclusive and
bounded. Never consume the world cursor early to make reservation work easier.

### Departure publication, coexistence and authority budget

One Smith visit has one Service lease, one current complete departure and one
departure marker. Offer Buy and existing Armor Repair from the same lobby.
Escape from either browser returns to the lobby; switching mode there does not
settle or admit another visit. Inherit Repair's price/bit-clear/refusal and
exact-slot behavior. Older content retains its Repair-only lobby. No Sell or
Identify mode can be reached through these controls.

Departure consumes a fresh exit response and matches the current reservation's
full `C/E/Q`, visit/lease/identity and guard. Prepare the expected owner/resource
guard and fixed context/economy/RNG destinations before a separate callback-free,
nonthrowing publication. Store `C+/E+/Q+`, set `departed=true` and adopt checked
revisions/guard exactly once. Classification, arrival, Service-to-terminal-Event
handoff, original Event retirement/reporting and final acquired presentation
then settle under inherited authority, with no Quiet gap. The camera/facing
stays at the service cell. A failed final display cannot repeat the day/restock.

Retain M41's conservative mandatory suffix headroom: **16 Encounter/Journey
revisions, 8 Event input revisions, 4 boundary revisions**. Check each relevant
u64 counter independently with subtraction-based arithmetic; never wrap.
For content-13 Smith, after the original Event has become exclusive and before
starting service preparation, require **20 Encounter/Journey, 10 Event input
and 5 boundary revisions**. This reserves a preparation/admission/first-frame
prefix of at most 4/2/1 respectively, in addition to the 16/8/4 suffix. Recheck
the unspent margin before arming debt. Bounded preparation slices and retries
must reuse semantic identities, so the prefix does not grow with elapsed work.
Do not reinterpret legacy admission thresholds. Each optional action must
reserve its maximum consumption **in addition** to the remaining suffix:

| Optional work | Conservative additional headroom before accepting |
| --- | --- |
| Effective recipient/category/slot/mode selection, quote cancellation or result acknowledgment | 2 Encounter/Journey, 2 Event input; no boundary consumption |
| New quote/refusal operation | Same, plus one representable operation-ID increment |
| Confirmed Buy preparation/publication/result transition | 3 Encounter/Journey, 2 Event input, plus one reservation-revision increment |
| Existing repair publication/result transition | 3 Encounter/Journey, 2 Event input; preserve the same successor suffix |

These are headroom bounds, not dummy increments. The complete mandatory suffix
contains result/recovery and departure feedback/response, atomic departure,
arrival publication, Service-to-Event handoff, terminal retirement, final
presentation/acquisition and inherited guard margins; it must fit those limits.
Include the Smith UI semantic revision, operation/reservation IDs and inherited
Event/ticket counters in checked-capacity analysis; a private UI revision cannot
wrap to revive an earlier quote. The implementation/review must enumerate actual
increment sites and prove the specified prefix/action/suffix bounds, with
threshold tests at one-less/exact/one-more. If an optional action
cannot preserve its suffix, use reserved authority to present departure-only
mode. Operation/reservation exhaustion refuses a new optional operation, not
the existing debt.

Stale/reentrant/repeated/invalid keys, reselecting an unchanged value, unchanged
semantic retries, cosmetic frames and preparation slices spend no additional
semantic revisions. Concrete presentation identity can renew without spending
another operation/input revision for an identical phase. Mandatory retry uses
the same semantic result/suffix and a newly acquired concrete frame; wrong keys
cannot exhaust the last departure frame. No optional repetition may make an
already admitted obligation unpayable.

## Time, RNG and inherited services

Buy entry/browsing/selection/quote/cancel/refusal/success/result and Repair add
no intrinsic time or actor work. Each admitted Smith visit owes its separate
one **1440-minute script-service** departure, including refusal-only visits.
Context remains year 610, day 8..99, minutes 300..1259, `ctr24 < 24`, Adventurer/
WorldOfXeenClouds, rested/newDay false and existing zero unsupported effects.
Entry days 8..98 can reserve days 9..99; day 99 refuses before debt or mutation.
Day-98 Buy, including multiple purchases, remains possible after admission;
it adds no member-day. Ordinary time/casting/combat limits remain unchanged.

| Sequence | Required result |
| --- | --- |
| Several purchases at day 8, then departure | All purchases persist; day 9, same minute/ctr24/RNG/bank and depleted stock |
| Refusal/cancel-only day 9 visit | Day 10; no purchase delta, depletion retained, no interest/RNG |
| Purchases at day 10, then departure | Delivery/payment persist; day 11 publishes the already-prepared **full** restock and bank interest; purchased stock depletion no longer belongs to the new generation |
| Later return without a trigger | Current depleted stock persists; no entry refill or lazy initialization |
| Training with depleted stock | Each first successful distinct member still consumes its own complete one-day candidate; a nontrigger preserves depletion, a trigger replaces all stock; departure remains a separate call |
| Day 97/98 Training limits | M41 limits remain: a first member-day needs the next feasible departure; a fresh day-98 visit cannot train a new member but can depart to 99 |
| Day 99 service attempt | No admission, debit, item/stock mutation, interest, RNG or time; quiet daytime play/capture remains admitted |

Do not merge Training member-days with departure, replay ordinary skipped-day
condition ticks/reset/healing/actor catch-up, or add purchase-specific restocking.
For a changed one-day service call, only destination days `11,21,...,91` trigger.
The reference's separate multi-day predicate remains outside M42 admission.
Generate both sides/all eight shops in M40 order with 160 complete item calls,
including discarded-item requests, then apply shared bank interest **gold then
gems**:

```text
bankAfter = u32(uint64(bankBefore) + floor(bankBefore / 100))
```

Purchase never touches bank. Zero/zero production interest is still one executed
operation; use call traces as well as values to detect duplication. Preserve
M40's explicit u32 wrap examples and all reference generation tables/requests.

The sole durable random cursor remains algorithm 1/world-owned:

```text
require algorithm == 1, state != 0, count < UINT64_MAX before every raw draw
x ^= x << 13; x ^= x >> 17; x ^= x << 5       # uint32 operations
count += 1
span = hi - lo + 1; threshold = (0u - span) % span
if x < threshold: reject, retaining the same pending request
else: accept lo + x % span
```

Accepted and rejected attempts count; singleton intervals still draw. Validate
intervals before draws. Yield/retry does not change the accepted/raw/rejection
trace or final cursor. Exhaustion during pre-admission complete reservation is
a deterministic support refusal without debt, not an endless retry. A completed
healthy reservation cannot later exhaust RNG. Buy/rebind, bank arithmetic,
nontriggering departure and static/cosmetic service presentation consume no
gameplay draw. Never reseed, approximate, wrap count, reroll unsupported offers
or publish a private prefix. Mark discarded diagnostic preparation traces
separately from committed continuation.

Repair and Training must accept **already-depleted content-13 stock**, both
before and after fresh-process restore, without granting legacy content Buy or
depletion. Cover purchases after prior nontriggering/triggering Smith days and
Training, then Training/Repair after purchases. A later legitimate restock
replaces all stock by M40's existing operation; it does not merge, replenish
selected slots or infer anything from bought item inventories.

## Native presentation, failures and save authority

Use the existing 320x200 indexed world, party strip, fonts, bounded panels and
single Application/Flow/SDL idle/input loop. Original Ironworks title remains
`aaze0028.txt[33]`; roster/item names remain resource/catalog-driven. Reuse
`blck1.twn` frame 0, existing escape icon if shown, admitted manifests and
resource owners. No additional commercial art, frame, text or archive member is
required: authored English text controls suffice; `buy.icn`, `equip.icn`, shop
animation/audio and a copy of the original item-dialog UI are not prerequisites.

Only small private weapon/armor numeric price tables/rules are additionally
needed. Reuse existing armor costs where a shared pure table has the same
meaning. Preserve pinned ScummVM developer attribution, GPL-3.0-or-later notices,
exact source/pin and corresponding-source availability as in [dependencies](dependencies.md).
Do not embed commercial assets, add `mm.dat` or a runtime reference-tree reader.
Original installation and pinned dependency remain external and read-only.

| Phase | Controls and authority |
| --- | --- |
| Exclusive admission/preparation | Noninteractive bounded preparation; existing Event owns exclusivity, no debt until complete admission; F9 denied |
| Smith lobby | Selected recipient/purse, Buy and Armor Repair, explicit one-day departure. `B` selects Buy; `R` or Enter selects Repair, preserving the old Enter path; F1..F6 recipient; Escape requests departure |
| Buy browse | Weapons initially; Left/Right cycles the four physical category views; F1..F6 recipient, 1..9 physical row, Enter requests quote/refusal, Escape lobby |
| Repair browse/quote/result | Existing M38 controls and semantics; same service/departure, no scratch stock writeback |
| Buy quote | Exact recipient/category/physical offer/price/purse and gold-after or shortfall; fresh Enter/Yes confirms, Escape/No cancels to Buy |
| Optional candidate preparation | No concurrent selection/second confirmation; hold the current service and old complete reservation; no save authority |
| Buy result/refusal | Fixed paid/delivered/depleted result or truthful refusal; Enter/Escape acknowledges into fresh Buy browse; after success clear offer selection rather than following a shifted row |
| Departure/recovery | Enter/Escape retries only unfinished mandatory settlement; no optional selection or world controls |
| Event retirement/final presentation | Remain exclusive; no Quiet until all inherited settlement and actual final frame acquisition succeed |
| Returned Quiet | Existing inventory/transfer/Remove/Equip and F9; same cell/facing, manual Space needed for a new visit |

Add only narrow typed Smith-mode selection if the existing action vocabulary
needs it; reuse category/member/physical-slot actions and contextual native
input routing. No replacement input owner or scheduler. Explicit unsupported
choice feedback has no hidden Sell/Identify action. Fit nine physical rows,
selected raw identity/support status, recipient and price/purse feedback using
existing bounded elision/wrapping; never elide required numeric facts or identity.

Inherit [M41's make-before-break cosmetic authority](milestone-41-plan.md#narrow-continuation-and-native-input)
and shared responsiveness correction, with M42 semantic identity additionally
including mode/category/physical offer, recipient, operation/quote and
reservation revision. Consume phase and concrete frame before validation or
callbacks. Batched selection/Enter/Enter, held/repeated B/R/F-key/number/Enter/
Escape, stale/reentrant/foreign responses and a recurring semantic phase cannot
cross to a new actionable frame. Compaction invalidates all prior offer quotes,
even when bytes recur. A failed composition/upload/copy/acquisition does not
authorize an unshown successor. A retry obtains a new concrete frame for the
same retained result; it cannot revive spent confirmation.

Cosmetic replacement keeps the acquired old frame actionable until its successor
is successfully acquired. Queued input retains its exact old concrete origin;
it is not relabeled for the successor. A semantic action supersedes an obsolete
cosmetic successor. After handoff stale old-origin input cannot authorize a new
action. Cosmetics neither discard a legitimate first key edge repeatedly nor
consume semantic headroom. Retain held/repeat/key-up/batch/close ordering and
the existing real-clock navigation/first-press tests, not just new modal tests.

F9 must refuse **before capture, resource/provider preflight, save-path preparation
or I/O** throughout exclusive preparation, every modal phase, debt, optional
candidate work, failed settlement and unpresented handoff. An empty queue, paid
purchase or released predecessor lease is insufficient. Do not queue a deferred
save, acknowledge feedback or flush debt through F9. Denial must also hold at
direct/test/reentrant save entry points and both sides of each ownership handoff.

Retain full guards for all thirty characters/supplements/books, every inventory
byte including untouched empty metadata, active membership and owner replacement
history, carried/pending treasure, complete stock/bank presence/bytes, context,
RNG, camera, flags/quests/recovery/overlays, both actor regions/statistics and
the complete admitted immutable resource union. Guard providers, boundary hooks,
clock/scheduling/cosmetic/render/presentation calls before and after, including
exceptional exits. Value equality cannot detect mutation then reversion.

Extend existing `prepareOwned`/`addOwned` insertion/reconstruction observation
before newly materialized stock/roster supplements/resource/Map/MOB/Event nested
storage references escape. Equal reconstructed data/cache hits cannot cleanse
failure. Expected-delta adoption preserves the old monotonic history/failure
chain and resource union; never rebase from arbitrary callback-mutated owners.
This also applies to inherited combat guards after purchase/restore.

| Failure | Required authority/effect |
| --- | --- |
| Empty/unsupported offer, full tail, insufficient gold, cancelled quote | No purchase mutation, RNG/time or normalization; admitted visit still owes its existing departure |
| Ordinary fallible optional preparation | Keep old reservation and all earlier purchases/repairs; discard/retry only pending candidate via fresh authority |
| Post-purchase feedback/presentation fault | Keep exact payment/delivery/depletion, result, consumed operation and rebound departure; retry only remaining presentation/continuation |
| Pre-departure settlement preparation fault | Keep debt and current complete reservation; no free Quiet return |
| Post-departure classification/Event/report/presentation fault | Keep departed marker/date/stock/bank/RNG and purchase prefix; retry only unfinished suffix |
| Stale/reentrant response | Reject before providers/publication; do not interpret a shifted or matching offer as the old quantity |
| Owner/resource/preimage/canonical violation, mutation then reversion, tampered candidate | Permanent integrity failure; no new publication, gameplay/save or guard rebasing, even after values revert |
| Fatal native rendering/window close/shutdown | Unsaved termination or existing unavailable state; preserve previous disk save, no hidden autosave/refund/deferred transaction |

Recoverable I/O/allocation/presentation faults that return no incompatible value
are distinct from integrity failure. Neither permits replay, rollback of earlier
commits or release of an unpaid obligation into Quiet.

## Persistence and compatibility

Schema 9 already represents settled purchased inventory, carried-gold debit,
complete merchant bytes/bank, context, progression, books, actors/flags and RNG.
Its sufficiency is both **wire** and **semantic**: no durable new owner or
provenance is required, and content 13 explicitly changes stock admission/Buy
capabilities. Byte capacity alone would not authorize reinterpreting content 12.

Retain envelope v4 and every schema-9 field order/width and exact extent:
four shape bytes `2,4,4,9`, 1152 side/shop/category/physical-slot item bytes and
two LE u32 bank balances, appended after the inherited city block. Economy is
mandatory for 9/11, 9/12 and **9/13**, absent in legacy non-economy domains;
wrong/missing shape, presence, counts, trailing bytes and unknown pairs reject.
Suffix sizes remain `4278+5*N`, `5156+5*N` or `5270+5*N` for absent/46/52 city
forms, with inherited combined pending-item `N <= 12`, maximum 5330 and existing
whole-file bound. Base character/item, raw learned bytes and all actor/treasure
cross-owner constraints remain unchanged.

Supported Journey pairs are exactly `1/1` through `8/8`, `8/9`, `8/10`, `9/11`,
`9/12`, `9/13`. Reject `8/13`, `10/13` and other crossed/unknown pairs. Preserve
ordinary v1/v2 and completed v3 meanings and exact legacy Journey wire/behavior.
Fresh Regional Journey selects 9/13; explicit legacy setups/load retain their
saved pair. No migration, implicit upgrade, missing-field backfill, depleted
legacy stock acceptance, new legacy capability or reload of original inventories
over paid/saved items is authorized.

Content 13 inherits all content-12 resource/route/actor/Training/Repair/casting
and year/day admission, including original CHR omitted-temporary source checks.
The current-stock validator is selected by **content**, while schema selects
wire representation. Complete-generation validation remains separately required
on fresh/restocked results. Quiet day-8 saves require complete generation as
specified above; later saves admit the canonical deletion closure.

Eligible checkpoints are explicit:

- **Pre-visit Quiet:** debt has not been admitted, all earlier work settled and
  the world frame acquired. At the service cell, F9 is valid before Space.
- **Post-settlement Quiet:** purchase/repair results acknowledged, mandatory
  departure published, classification/Event/attachment/reward obligations
  settled and final frame acquired. Legal inventory changes may then settle and
  be saved at their own inherited Quiet boundary.

There is no active-service, confirmed-but-unpublished, paid-but-unsettled,
unpresented-result or transient-reservation checkpoint. "After purchase" alone
does not mean saveable. UI phases, quotes, leases, result/published/departed
markers, operation/reservation revisions and debt remain transient and absent
from the file.

Capture copies exact current sole owners after Quiet acquisition and full guards.
Fresh-owner restore detaches/validates the snapshot and original resource union,
prepares destination guards/storage and first presentation, then installs exact
saved values before first input with fresh runtime/frame authority. It must not
purchase, reprice, re-earn, relearn, equip, normalize/compact, refill, regenerate,
apply interest, reset/reseed, clamp HP/SP, advance time/actors, reopen either
service or replay Events. Catalog display/derived-stat calculation is read-only.
Later legal changes to purchased inventory are preserved independently of stock;
no restore logic matches an item back to its old merchant slot or "repairs"
apparent stock/inventory mismatches.

## Future connected acceptance witness

### Production path and exact observables

The following is a future acceptance design, not a claim of passed M42 play.
Begin fresh 9/13 with input seed 7, actual generated stock, original prepared
purse 800/10 and bank 0/0. Fresh generation/route/prepared-party semantics are
unchanged from the verified content-12 evidence. Use the accepted M41 mainland
well/combat/treasure/First Aid/entrance-Slime prefix and its real actions, then
the seven-step Ironworks corridor described above. Earned gold makes the
pre-visit purse 870; no funds/stock/camera/HP/actor rewrite, teleport or synthetic
fixture substitutes for that production route.

Use **one primary Armor path**, with focused Weapons coverage separately:

1. At pre-visit Quiet `(28,8,4,West)`, day 8/minute 803, save checkpoint **A**
   through actual F9. Expect cursor `799325555:1101`, bank 0/0, gold 870 and
   the verified original shop stock. Record complete owners, not just hashes.
2. Space admits the actual Ironworks Event/service. Select Arturius (F1), Buy,
   Armor, native row 4 (physical slot 3). Quote armor `(0,3,0,0)` at 200;
   cancel once and verify stock/recipient/gold unchanged. Select/quote again,
   then fresh confirmation purchases exactly once. Expect **870 -> 670**, new
   Arturius Armor physical slot 4 `(0,3,0,0)`, exact six-record depleted stock
   vector above and every other owner/economy/context/RNG field unchanged.
3. In the same visit, confirm an actual remaining plain Armor ID 6 quote
   (1000) and observe insufficient funds without undoing the first purchase.
   Return through the fresh result/browser/lobby and depart once. Expect
   **day 9/minute 803**, same ctr24/cursor/bank and exact depleted stock.
   Capture post-settlement Quiet **B**; modal F9 attempts throughout do no work.
4. Through existing `I`/member/category/physical-slot/`T` confirmation controls,
   transfer the new unequipped Armor from Arturius to Tyro (F2). It reaches
   Tyro's physical Armor slot 4; existing occupied order/bytes persist. A direct
   Equip attempt while body armor slot 0 is equipped must give the existing
   conflict. Remove that original body armor legally, then equip slot 4 to
   frame 3. Existing inventory **Speed/AC** display must show **AC 11** versus
   pre-upgrade **10**, with HP67/SP0 and all conditions/other equipment retained.
   Save settled inventory checkpoint **B1**.
5. A later refusal/cancel-only Smith visit departs 9->10, preserving stock and
   gold. Save Quiet **C**. The following visit departs **10->11** and replaces
   all wares once, applying interest once. With no intervening gameplay draws,
   expect **state `2959920300`, count `2009`**, the same full restock bytes as
   [M41's trigger vector](milestone-41-plan.md#production-acceptance-boundary),
   stock CRC32 `79dec2de` as a secondary diagnostic and bank still 0/0. Save **D**.
   Purchased/transferred/equipped Armor and gold 670 persist; do not restore its
   old shop depletion into the newly generated stock.
6. Exit Vertigo through the original flag/prelude/reset question and return
   through admitted play to the reset entrance Slime, settle real continued
   combat and return to a valid Quiet checkpoint **E**. Preserve actual ensuing
   wounds, equipment breakage, XP/treasure and RNG; no healing/clamping is added
   for the witness. Use inherited recovery only if genuinely needed and record
   its separate consequences. Continue to exercise valid inventory/Repair or
   Training against successor state in appropriate companion branches.

The improvement is the existing derived consumer's exact +1 AC from body
strength 4 to 5, visible before subsequent damage can legitimately break armor.
The current enemy noncritical threshold is
`combatArmorClass + (blocked ? currentLevel/2 + 15 : 10)`; it increases by one
with this intact upgrade. Verify this dependency independently and trace actual
current consumer inputs in continued combat. Do not require a lucky hit/miss
or claim one damage roll proves improvement. Later breakage may remove the
contribution under existing rules; preserve that truthful result.

Weapons focused integration must purchase actual plain stock, deliver/transfer/
equip under current masks and conflicts, and feed current melee/Shoot consumers.
Weapon 6 versus Tyro's 2 has independent `4d2` versus `2d3` dice support; retain
current per-swing tripling, Might, hit and resistance arithmetic. It need not be
forced into the primary production route. Repeated-purchase coverage must include
the seed-7 duplicate weapon-6 physical records and shifted-slot invalidation.

### Fresh-process comparison and physical boundary

At A, B, B1, C and D, complete full application exit and start a **new process**
using the saved pair/content. Before first gameplay input compare complete
relevant state and encoded fields against the checkpoint: all thirty characters,
supplements/raw books/items/order/frames, membership, carried/pending treasure,
complete stock/bank, context, both actor collections/accounting, flags/overlays/
recovery, camera and exact algorithm/state/raw count. Assert zero gameplay,
fresh-generation, purchase, time, interest, equipment or Event replay.

Continue restored A through the purchase/departure/inventory branch to B/B1;
restored B and B1 through C/D; restored C through triggering D; restored D through
E. Compare with uninterrupted branches after identical meaningful actions,
including every stock byte and actual postcombat owner consequences. Compare
operation/price/payment/delivery/depletion, departure, interest-count, bounded
request/raw/rejection and subsequent combat traces with committed versus
discarded preparation labeled. Use independent literal pre-restock/depletion
oracles and pinned/numerical generation vectors as well as field comparisons.
An encode/decode self-round-trip, hash or "save loaded" message alone is not proof.

Automated process/native-input execution is distinct from maintainer physical
acceptance. The maintainer must perform the connected primary path in an
ordinary visible SDL window, observe truthful stock/price/purse, cancellation,
payment/delivery/depletion, mandatory departure, legal transfer/conflict/removal/
equip and AC improvement, full exit/fresh restart, later restock and continued
play. Check first-press recipient/category/offer/quote/confirmation/result/exit
responsiveness and early modal F9 refusal. Images and injected inputs do not
stand in for this physical boundary.

## Required later validation and closure

The implementation must add/update tests for changed behavior in existing
subsystems. Keep synthetic controls clearly separate from production evidence.
Independent expected outcomes must come from literal data, reference formulas,
explicit generation tapes/schedules or an independently implemented oracle,
not solely the production validator/candidate or self-round-trips.

| Area | Minimum focused coverage |
| --- | --- |
| Offers/prices/funds | All supported IDs/base prices, minimum weapon-7 price, weapon-6 59/60/61 and armor-3 199/200/201 funds, u32-high purse, pending/bank/gems nonpayment, unsupported/empty categories/records, recipient condition/proficiency independence |
| Physical movement | Tail occupied with earlier holes refuses; free tail with holes inserts/compacts stably; delivered frame 0, exact occupied bytes, touched versus untouched empty metadata; duplicate offers and every removal position; shifting invalidates old authority |
| Depleted validity | Complete stock accepted, one/many/all permitted deletions, independent schedules and actual seed-7 vectors; original generation capacity retained after omissions; malformed shape/tails/frames/status/order/counts, no X/M omission transition, other shops/sides strict; legacy rejection where generated-only proof fails |
| Services/time | Repeated purchases, cancel/refusal after success, Buy/Repair mode coexistence and one departure, depleted Repair/Training before/after restore, separate new-member days, triggering/nontriggering visits, prior Training/restock, day 97/98/99 and unchanged minute/ctr24/actors |
| Publication/reservation | Atomic purse/delivery/depletion/result/guard/rebind, both rebind cases, exact old/full candidate checks, old obligation survives optional faults, no live RNG during preparation, all-stock replacement and interest once; postcommit retry never repeats mutation |
| Limits/faults | Allocation sweeps through quote/candidate/rebind/result/guard/departure preparation; provider/composition/upload/copy/acquisition/report/classification faults before/after publication; RNG rejection/yield/count exhaustion; independent one-less/exact/one-more authority counter thresholds, departure-only fallback |
| Input/save | M42 phases, stale prior visits/quotes/results and reentrant callbacks, held/repeat/batched recipient/category/number/B/R/Enter/Escape, compaction-invalidated response, native first edge and cosmetics, wrong-key suffix preservation, failed frame/retry, F9 denial before capture/provider/path/I/O at every lease handoff |
| Integrity | Every stock/purse/bank/inventory field and presence reset/reinsert/swap, unrelated inactive owner/book/actor/resource mutation then reversion, exceptional callbacks, new nested cache insertion/reconstruction before reference escape, expected-delta renewal preserves failure/history, postpurchase/restored combat scheduling/cosmetics |
| Persistence/consumers | Literal schema-9 extents and 9/13 capability/presence, Quiet day-8 complete proof, malformed depleted snapshots, exact later carried-item changes, all legacy pairs and their action/refusal semantics, fresh-owner no-replay restoration, legal equip/transfer and existing AC/dice/combat benefit |
| Production/process | Actual fresh seed-7 route/stock/funds, A/B/B1/C/D/E relevant comparisons, new-process identity and exact pre-input state, uninterrupted/restored continuation including restock and later combat; separate maintainer native-SDL path |

Run the normal build, relevant rule/Flow/resource/process/allocation/native-input
and inherited shared scheduling/responsiveness tests, then **the complete
unfiltered CTest suite** at implementation closure. Do not declare completion
with failing tests. Obtain independent **contract -> implementation -> tests/
evidence** review of the finished candidate, including both depleted validation
and publication/reservation proof, plus separate maintainer physical acceptance.
That implementation review/acceptance has not been performed by this planning
task. Apply [AGENTS.md closure/documentation rules](../AGENTS.md) only after the
required gates establish a durable accepted implementation.

## Exclusions and readiness limits

Exclude Sell, Identify, Merchant-skill acquisition/state, other shops, bank
menus/deposit/withdrawal, Guild, Temple, normal startup, wider Vertigo travel,
new regions/dungeons, modified-equipment/effect expansion, item-spell expansion,
generic trading infrastructure and Darkside gameplay. Numeric side-1 stock
remains part of inherited complete restocking/persistence only. M43 investigation
or another roadmap reassessment is not needed for the chosen M42 architecture.

The investigation supports a complete bounded contract without a new durable
field/schema, new effects or route expansion. Snapshot validation deliberately
proves possible current stock, not unrecorded historical payment/generation
provenance; live exact deltas and mutation guards enforce actual publication.
The new input/publication/reservation and process witness still require future
implementation, independent review and physical acceptance. Specification
readiness does not authorize implementation or claim those future gates passed.
