# Milestone 42 - Bounded Ironworks equipment purchase

Historical record; not current rules or scope.

## Completed scope and acceptance boundary

**COMPLETED AND ACCEPTED.** M42 connects actual generated Ironworks stock to
carried-gold payment, physical inventory delivery and depletion, mandatory
departure, legal existing transfer/equipment, useful continued combat and exact
Quiet restart. Independent implementation review returned ACCEPT with no material
findings or unresolved blockers. Separately, the maintainer reported successful
physical native-SDL acceptance; see [final acceptance](#final-acceptance).

Fresh Regional Journey selects **envelope v4 / Journey schema 9 / content 13**.
It inherits content 12's prepared Journey, twenty-eight Vertigo cells, Events,
actor closures, Repair, Training, casting and consequences. Only the original
Clouds Ironworks `(28,8,4)`, Event record 0/action 1, gains Buy. Repeated
purchases, cancellation/refusal, return visits and later full restock are included.
No new route, actor admission or unrestricted original startup is introduced.

Inherited contracts retain their legacy meanings:

- [M24 inventory/transfer](milestone-24-plan.md) and
  [M25 equipment](milestone-25-plan.md#equipment-domains-and-frame-contract).
- [M38 repair, route and ownership](milestone-38-plan.md), including
  [exact repair rules](milestone-38-plan.md#exact-repair-rules).
- [M40 complete generation](milestone-40-plan.md#complete-stock-algorithm-and-tables),
  [service publication](milestone-40-plan.md#coordination-publication-and-failure)
  and [wire contract](milestone-40-plan.md#wire-contract).
- [M41 Training](milestone-41-plan.md#training-rules-and-state-ownership),
  [individual service days](milestone-41-plan.md#individual-service-days-and-departure-reservation)
  and [native input](milestone-41-plan.md#narrow-continuation-and-native-input).

## Original behavior and provenance

Reference interpretation uses ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, as configured in
[dependencies](../dependencies.md). Decisive pinned sources are
[ItemsDialog](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp),
[Party/BlacksmithWares](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp),
[Locations](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp),
[Items](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp)
and [numeric constants](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp).
These establish Buy pricing, tail capacity, unequipped delivery, stable sorting,
stock removal and service departure. They are reference interpretation, not
physical DOS observation.

The reference copies all four shop categories to a scratch character, modifies
icon frames and an accessory ID, and writes scratch stock back on dialog exit,
even after cancellation. M42 deliberately uses an immutable physical-stock
projection: browsing never normalizes authoritative frames, accessory IDs or
untouched records. Only a successful admitted purchase deletes stock. Repair
retains its direct path. This preserves admitted transaction semantics while
excluding the reference's unrelated scratch/display writeback side effects.

The service identity is side 0/shop 0 at the exact Ironworks Event, without the
reference's unmatched-map fallback. Original title `aaze0028.txt[33]`, catalog
item names and roster names remain resource-driven. Smith uses static
`blck1.twn` frame 0; inherited Training uses static `trng1.twn` frame 0. Original
resources remain external and read-only. Numeric adaptations retain ScummVM
attribution and GPL-3.0-or-later provenance; no commercial payload, `mm.dat`,
additional dependency checkout or runtime reference-tree reader is embedded.

## Offer, pricing and physical movement

Indexes are zero-based; native labels are physical slot +1. Item bytes are
`(material,id,state,frame)`. All nine physical rows in all four categories remain
visible, with raw identity and truthful support status. Selection alone confers
no purchase authority.

Supported offers are ordinary **Weapons 1..33 or Armor 1..13**, with material,
state and frame all zero, from actual canonical side-0/shop-0 wares. Accessories,
Miscellaneous, modified equipment, effectiveness counters, unknown IDs and
special weapon 34 cannot be purchased. Unsupported generated records retain
exact bytes/order; they are neither replaced nor rerolled. Noncanonical merchant
frames, status or tails are integrity/restore failures, not ordinary offers.
Material-38 Armor remains repairable under M38 but is outside Buy.

Any current active roster owner may receive an item, irrespective of individual
condition or proficiency, provided overall service admission remains valid.
Capacity requires recipient slot 8 to have ID zero; an earlier hole cannot
substitute for an occupied tail. Subsequent equipability is decided separately
by existing proficiency, frame-conflict and arrangement rules. Delivery is
unequipped. Plain Weapons 1..29 feed existing melee, 30..33 feed admitted
mainland Shoot, and Armor 1..13 feeds existing combat AC after legal equipment.

Buy forces divisor index 0, whose value is 1. For admitted records:

```text
price = max(1, baseCost[category][id])
require carriedGold >= price
goldAfter = carriedGold - price
```

Validated indexes and widened unsigned arithmetic preserve the full u32 purse.
Bank, gems and pending treasure cannot pay. There is no Merchant-skill, class,
level or condition surcharge, quantity discount or modified-item arithmetic.

| IDs | Base costs in matching ID order |
| --- | --- |
| Weapons 1..11 | 50, 15, 100, 80, 40, 60, 1, 10, 150, 30, 60 |
| Weapons 12..22 | 8, 50, 100, 15, 30, 15, 200, 80, 250, 150, 400 |
| Weapons 23..33 | 100, 40, 120, 300, 100, 200, 300, 25, 100, 50, 15 |
| Armor 1..13 | 20, 100, 200, 400, 600, 1000, 2000, 100, 60, 40, 250, 200, 100 |

After frame/owner/guard validation, distinguish empty/unsupported offers, test
supported-offer tail capacity before confirmation/funds, and quote the actual
record, owner, slot, purse, price and gold-after/shortfall. A fresh confirmation
rechecks quote, guard and reservation before funds and candidate preparation.
Exact funds succeeds to zero; cancellation and insufficient funds mutate nothing.

For complete recipient category A and merchant category S, prepare:

```text
delivered = S[slot]; delivered.frame = 0
A1 = A; A1[8] = delivered; stableCompactByNonzeroId(A1)
S1 = S; S1[slot] = (0,0,0,0); stableCompactByNonzeroId(S1)
```

Occupied order and bytes persist except the delivered frame. Empty records in
these two touched categories become all zero; untouched empty metadata, owners
and categories remain exact. Delivery lands at the recipient's prior occupied
count. Duplicate records are distinct physical quantities. Compaction and any
selection/category/recipient/mode change invalidate earlier quotes.

## Complete generation and purchase-depleted stock

`xeenValidateCurrentServiceEconomy` is distinct from complete-generation
validation. Fresh generation and every full restock remain strict for all eight
shops. Legacy contents 11/12 retain generated-only admission. Content 13 permits
only shop 0/0 Weapons/Armor deletion closure; the other seven shops and
Accessories/Miscellaneous retain their complete proof.

Shop 0/0 has twenty ordered generation calls: fifteen level 1, then five level 2.
The deterministic proof uses six counters `(gW,gA,kW,kA,kX,kM)`: original W/A
insertions and four retained-prefix counts. For each call, a reachable state may:

1. Discard a possible generated category outcome only when its **original**
   insertion count is already eight.
2. Retain the next physical record if the original count is below eight and the
   inherited level/category support predicate admits that record; advance both
   insertion and retained counters (one shared counter for X/M).
3. Omit an inserted subsequently purchased plain W/A offer, advancing only its
   original insertion counter. Such an offer exists only at level 1; level-2
   equipment has nonzero material. X/M has no omission transition.

Acceptance requires exactly twenty layers and consumption of all retained
prefixes, preserving canonical zero tails, order and bytes. Original insertion
counts prevent omissions from creating new generation capacity. The finite
relation needs at most two Boolean layers of `9^6` states for this shop, with
bounded working storage outside publication; other shops retain the `9^4` proof.
It admits zero, one, repeated or all supported deletions, including duplicates,
without RNG inversion. Even an empty shop 0/0 can be possible after purchasing
all inserted plain stock; eight empty shops are not thereby admitted.

Snapshot admission proves **possible current stock**, not the actual historical
seed, original stock, payment history, purchase count or former duplicate slot.
It cannot detect every byte change that yields another possible state. Runtime
exact deltas and full mutation-history guards enforce actual transitions; no
historical provenance ledger is inferred or stored.

Actual content is threaded through Smith, Training, shared service candidates,
Journey/party admission, capture, decoding and restore. Content 13 uses the
inherited Training route/closure and resources. Quiet saved day 8 requires
complete stock because no admitted visit settles without a day charge; live
day-8 depletion inside Smith is valid but unsaveable. Later saved stock uses
the deletion proof and inherited retained-city requirements.

## Ownership, atomic publication and departure

Existing roster inventories, carried purse/pending treasure, party economy and
context, world RNG/actors and Event/Service/Presentation coordination remain sole
owners. Detached candidates and immutable UI cannot publish. There is no new
wallet, merchant character, scheduler or transaction ledger.

Quotes bind owner incarnations, active membership, visit/operation/phase,
concrete presented origin, content/site/category/physical record, complete touched
preimages, purse/price, full owner/resource guard, coordination revisions and
complete departure reservation. Matching labels or bytes elsewhere cannot replace
that binding. Every fallible allocation, validation, guard/result/rebind preparation
and physical pointer lookup precedes the last callback and final checks of the
old full mutation-history guard and complete reservation.

A callback-free, nonthrowing publication unit changes the two touched categories,
carried gold, fixed result, exactly-once marker, replacement departure obligation,
checked revisions and expected guard together. Guard renewal retains prior
integrity failure/history and the resource union. Rendering/reporting after
publication reads fixed facts; retries finish feedback/continuation without
repeating payment, delivery, depletion or rebind.

The complete reserved Smith candidate binds before/ending context, economy and
RNG. Each purchase prepares a narrow checked rebind for the exact single stock
deletion while retaining the old obligation until publication succeeds:

| Departure | Rebound successor |
| --- | --- |
| Nontriggering | Exact depleted economy, same reserved ending context/RNG |
| Triggering | Already-prepared full replacement stock, interest and RNG successor; no regeneration, reroll or repeated interest |

Preparation failures preserve the current obligation and earlier purchases or
repairs. Multiple purchases independently renew it. Buy and Repair share one
visit/lease/departure; browser Escape returns to the lobby. Repair preserves its
own rules and requires no stock rebind. Depleted stock remains compatible with
Training's separate member-days, restock and departure, including after restore.
Departure publication is separately once-only; later arrival/Event/final-frame
failures cannot repeat its date, stock, bank or RNG consequences.

Authority uses checked independent counters without wrap: mandatory suffix
headroom is 16 Encounter/Journey, 8 Event input and 4 boundary revisions;
content-13 admission reserves 20/10/5. Optional selection/quote/result transitions
reserve an additional 2/2, and Buy/Repair publication 3/2, plus checked operation,
UI and applicable reservation identities. Optional exhaustion leaves departure-only
settlement available. Semantic increment sites are:

| Transition | Encounter/Journey | Event input | Boundary |
| --- | --- | --- | --- |
| Preparation, unchanged retry/cosmetic frame | 0 | 0 | 0 |
| Admission | 1 each | At most 2 initial frames | 1 hold |
| Effective selection/quote/cancel/acknowledgment | 1 each | 1 changed-phase frame | 0 |
| Buy/Repair publication | 1 each | 1 result frame | 0 |
| Departure publication | 1 each | At most 1 changed feedback frame | 0 |
| Service-to-Event handoff | 1 each | Retained terminal continuation | 1 release |
| Terminal Event retirement | 1 each | Final frame | 1 release |
| Final acquired Journey presentation | 1 each | 0 on acquisition | 0 |

At most three recovery transitions precede the four mandatory Encounter/Journey
increments; unchanged mandatory retries reuse identities. Independent one-less,
exact and one-more threshold tests maintain these bounds, including the original
Smith Event ticket's checked increment before preparation.

## Service time and RNG

Buy/Repair browsing, quotes, cancellations, refusals, payment and feedback add no
intrinsic time, actor work or gameplay RNG. Every admitted Smith visit owes one
1440-minute script-service departure, including refusal-only visits. Year 610,
days 8..99, daytime minutes 300..1259, `ctr24 < 24`, Adventurer/WorldOfXeenClouds
and inherited unsupported-effect restrictions remain unchanged. Entry days 8..98
reserve ending days 9..99; day 99 refuses before debt/mutation.

Individual calls ending on 11,21,...,91 replace all wares and apply shared bank
interest, gold then gems, using `u32(uint64(balance) + floor(balance/100))`.
Training member-days and departure remain separate calls. No skipped ordinary
condition/day/actor work, healing or SP refill is introduced by Smith departure.

The sole world-owned RNG remains algorithm 1, uint32 xorshift 13/17/5 with a
checked u64 raw count. Rejection uses `(0u-span)%span`; accepted and rejected
attempts count, including singleton draws. Triggering preparation uses private
state and at most 64 raw attempts per idle slice; yield/retry preserves the exact
request/raw/rejection trace. No live prefix is published early. Pre-admission
exhaustion refuses without debt; completed reservations do not redraw. Buy/rebind,
interest arithmetic, nontriggering departure and cosmetics consume no draw.

## Native presentation, integrity and save authority

The existing 320x200 indexed world, party strip, fonts and single SDL idle/input
loop present the lobby (B Buy; R/Enter Repair; F1-F6 recipient; Escape departure),
physical Buy browser (Left/Right category; 1-9 row; Enter quote; Escape lobby),
quote (fresh Enter/Yes confirmation; Escape/No cancellation) and fixed result
(Enter/Escape acknowledgment). After success, offer selection clears. Departure
recovery accepts only unfinished mandatory settlement. Returned Quiet retains
normal inventory, transfer, Remove/Equip and manual service re-entry.

Make-before-break cosmetics retain acquired old-frame authority until successor
acquisition. Queued input retains its exact origin. Held/repeated/batched/stale,
foreign or reentrant input cannot cross a semantic or concrete-frame boundary;
failed composition/upload/copy/acquisition never authorizes an unseen successor.
First-edge responsiveness and inherited scheduling remain obligations.

F9 refuses before capture, providers/resource preflight, path preparation or I/O
through exclusive preparation, modal work, debt, optional candidates, failed
settlement and unfinished Service/Event/presentation handoffs. No deferred save,
autosave or free Quiet gap exists. Full guards cover all thirty owners/books/items,
untouched metadata, economy presence/bytes, context/RNG, camera, flags/recovery,
both actor collections and admitted immutable resources. Normal/exceptional
callbacks and nested insertion/reconstruction are observed before references
escape. Mutation then reversion remains permanent integrity failure.

Ordinary optional allocation/presentation failure preserves prior committed work
and the old obligation. Post-publication faults preserve the fixed committed
prefix and retry only the suffix. Canonical/preimage/resource/history violation
closes gameplay/publication/save authority permanently. Fatal native failure or
shutdown preserves the earlier disk save without refund or hidden transaction.

Broader terrain, wall-item and indoor scenery animation remains outside implemented
scope. Static Smith/Training illustrations are intentional. Previously admitted
ordinary object, portrait and combat animations remain supported obligations.

## Persistence and compatibility

Envelope 4/schema 9 wire order, widths and extents are unchanged. The economy
suffix retains shape bytes `2,4,4,9`, 1152 item bytes and two LE u32 bank balances
after the city block. It is mandatory for 9/11, 9/12 and 9/13. Suffix extents remain
`4278+5*N`, `5156+5*N`, `5270+5*N` for absent/46/52 city forms, `N <= 12`,
maximum 5330; the whole-file bound remains 4 MiB.

Supported pairs are exactly 1/1 through 8/8, 8/9, 8/10, 9/11, 9/12 and 9/13.
Crossed/unknown pairs, malformed presence/shape/tails/trailing bytes reject.
Schema selects representation; content selects capability/current-stock admission.
Ordinary v1/v2, completed v3 and legacy Journey meanings remain unchanged. No
implicit upgrade, backfill or legacy depletion/Buy capability is granted.

Only final acquired Quiet boundaries persist exact inventory, purse, stock/bank,
context, progression/raw books, both regions/flags and algorithm/state/count.
Quotes, selections, leases, operation/reservation IDs, debt and results are
transient. Fresh-owner restore installs exact values before first input, without
initialization, generation, payment, equipment, Training/refill, time, interest,
Event/reset or combat replay. It neither normalizes later carried-item changes
nor matches purchased items back to merchant slots.

## Production acceptance boundary

The production witness uses fresh seed 7, original prepared purse 800/10 and
bank 0/0, actual generated stock and the genuine M41 well/combat/loot/First Aid/
entrance-Slime prefix. Funds, items, camera, HP, actors and stock are not injected.
Initial post-generation cursor is `1652828136:901`. The seven-step Ironworks
corridor reaches the accepted checkpoints:

| Checkpoint | Day/minute | Gold | RNG state:raw count | Result |
| --- | --- | --- | --- | --- |
| A | 8/803 | 870 | 799325555:1101 | Quiet before service, original stock |
| B | 9/803 | 670 | 799325555:1101 | Armor 3 bought for 200, seven-to-six Armor stock, one departure |
| B1 | 9/803 | 670 | 799325555:1101 | Legal transfer/remove/equip to Tyro; AC 11, HP67/SP0 |
| C | 10/803 | 670 | 799325555:1101 | Later refusal/cancellation and Buy/Repair coexistence retain depletion |
| D | 11/803 | 670 | 2959920300:2009 | One full restock and interest operation; equipped purchase retained |
| E | 11/827 | 670 | 1045864998:2049 | Actual reset-Slime continuation; Tyro HP65/SP0/XP3296, Armor 3 intact |

Seed-7 shop-0 occupied prefixes (all remaining rows are zero) are:

```text
Weapons: (0,10,0,0) (0,6,0,0) (0,15,0,0) (0,10,0,0)
         (0,6,0,0) (0,4,0,0) (37,20,0,0) (40,16,0,0)
Armor A: (0,6,0,0) (0,4,0,0) (0,6,0,0) (0,3,0,0)
         (0,5,0,0) (40,8,0,0) (48,6,0,0)
Armor B/C: (0,6,0,0) (0,4,0,0) (0,6,0,0) (0,5,0,0)
           (40,8,0,0) (48,6,0,0)
```

The purchase selects original Armor physical slot 3/native row 4. Other
categories/shops remain exact until full restock. Independent literal controls
and complete-byte comparisons accompany these vectors.

The corrected production witness preserves real M41 loot: Arturius Armor ID 1
`(0,1,0,0)` remains at index 4. Purchased Armor 3 is delivered at **index 5/native
row 6**, then transferred to Tyro **index 4/native row 5**. Existing equipped
body armor first causes the genuine conflict, then legal Remove/Equip gives
frame 3 and **AC 10->11**. The independent body contribution is 4->5; actual
continued enemy construction receives intact `(0,3,0,3)`, AC11 and nonblocked
threshold21 instead of20. A lucky damage roll is not the improvement proof.
At E camera is `(28,15,2,North)` and ctr24=9; genuine wounds/XP/treasure persist.

Fresh processes restore A/B/B1/C/D before input with zero initialization/action/
draw/time/Event/inventory/equipment/interest replay. Fifteen complete continuation
file comparisons and five trace comparisons cover all thirty characters/raw
books/items/supplements, membership, signatures, camera/context/RNG, purse/pending
treasure, recovery/flags/overlays, both actor collections/accounting, all 1152 stock
bytes and bank, with encoded fields and uninterrupted continuation through E.
Fourteen process incarnations launch and fully exit.

Independent literal prices/stats/stock/deletion vectors and the separate pinned
reason-0/xorshift oracle in `tests/XeenM42Evidence.h` verify every generation byte
and all 908 restock raw draws. CRC32 `79dec2de` is supplementary. Separate actual
Weapons integration buys duplicate plain Weapon6 at physical1 then shifted3 for
60 each, transfers/equips Tyro and reaches real melee construction (`4d2` versus
original `2d3`). Separate Shoot integration buys actual restocked Weapon32 at
physical5 for50, departs11->12 without restock, equips frame4 and reaches real
owner18 missile construction; S/S1/SE saves compare full fields to disk.

Synthetic controls separately cover funds/prices/tail holes, every deletion
position, impossible stock schedules, legacy rejection, depleted Training across
separate days/restock, buying prepared restock, restored Repair, faults/ABA,
allocation sweeps, independent counter thresholds and native authority/F9.
Six native SDL upload/copy failure cases at admission, purchase and departure
preserve committed prefixes and earlier disk A while denying failed-frame/F9
authority. Automated inputs/images are separate from physical acceptance.

## Final acceptance

**M42 is COMPLETED AND ACCEPTED**, within the scope and exclusions above.

- **Automated deterministic evidence:** normal build and final unfiltered serial
  CTest passed **135/135**, including inherited scheduling and casting-admission
  checks, with the 80/80 scheduling assertion and production protections retained.
- **Original-reference/resource and automated process evidence:** pinned pricing,
  independent stock/RNG vectors, earned production purchase/equipment/combat,
  exact continuation and native failure preservation passed within the
  [production acceptance boundary](#production-acceptance-boundary). These establish
  automated behavior, not maintainer observation.
- **Independent implementation review:** ACCEPT, with no material findings or
  unresolved blockers. The reviewer accepted the loot-preserving witness correction,
  independently passed pinned-reference pricing, purchase Flow/fault/counter and
  input-authority checks, and verified the saved item bytes, exact continuation
  comparisons, process exits and native failure preservation described above.
- **Maintainer physical native-SDL acceptance:** the maintainer completed the
  supplied native-SDL walkthrough and reported that everything worked. Static
  vendor/trainer illustrations and unanimated scenery torches were the reported
  observations; everything else passed. This is maintainer-reported physical
  evidence, not an automated observation or a separate per-assertion transcript.
  No screenshot, timing or measurement is attributed to that report. Static
  Smith `blck1.twn` frame 0 and Training `trng1.twn` frame 0 are permitted; broader
  scenery animation is outside scope. The exact observed torch resource was not
  individually identified. Already admitted animations remain supported.

## Exclusions

Sell, Identify, Merchant-skill acquisition/state, other shops, bank menus,
Guild, Temple, general recovery/Rest/calendar processing, normal startup, wider
Vertigo travel, new regions/dungeons, modified equipment/effects, item-spell
expansion, generic trading infrastructure and Darkside gameplay remain excluded.
Numeric side-1 stock is inherited generation/persistence only. M42 completion
authorizes neither M43 implementation nor a roadmap reassessment. Commercial
game data remains external and unmodified; original-data checks require a legally
obtained installation. See [stable status](../project-status.md),
[history](../project-history.md#m42---bounded-ironworks-equipment-purchase) and
[future direction](../roadmap.md#near-term).
