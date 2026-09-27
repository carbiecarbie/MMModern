# Milestone 40 - Bounded service-day continuation

## Status, baseline and scope

**COMPLETED AND ACCEPTED.** This closed plan is the technical home for M40's
implemented service-day, economy, RNG and persistence contract. It builds on
M39's completed combat casting and the implementation specification committed as
`f39d82772783b7cd0d0c3ac3e74c4df16139dfbe`, `Define Milestone 40 service-day
continuation`. [Project status](project-status.md) owns the stable snapshot;
[project history](project-history.md) owns chronology. M41 Training is the next
[roadmap unit](roadmap.md#near-term), requiring separate planning/specification
and implementation authorization.

M40 enables repeated admitted Ironworks armor repair through **daytime,
same-year, script-driven one-day service operations**, including all mandatory
stock regeneration, bank interest and exact world RNG continuation. Generated
wares are durable physical records; they need not be accessible to the player.
The sole production service remains Clouds map 28, Event record 0, action 1 at
`(8,4)`, using M38's sixteen-cell route and repair menu. M39 combat casting remains
available throughout the successor domain and after restart.

Exclude Buy, Sell, stock depletion, Identify, bank menus/deposits/withdrawals,
general trading/service registries, new equipment effects, unrestricted merchant
access, additional service routes, Training, Temple, rest, ordinary overnight
adventuring, general day/night or daily processing, aging/year rollover, new
quests/regions and M41 implementation. In particular, do not admit Temple's
separate two-day call. Numeric Darkside stock does not admit Darkside gameplay.
No new reference-engine dependency or commercial-data modification is needed.

Two genuinely broken armor records and the existing purse establish useful
repeated repair and surviving play opportunities without item acquisition or
new recovery. M40 established no roadmap replanning trigger; the accepted broad
review remains after M41.

## Evidence and provenance

Evidence labels used below:

- **Reference:** source interpretation at ScummVM
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, not a claim of observed DOS execution.
- **Resource:** read-only observations from the original installation through the
  existing archive adapter, not a player's mutable save.
- **Inherited behavior:** accepted M38 repair/route and M39 casting contracts.
- **Diagnostic:** independent numerical/reference or synthetic controls, distinct
  from production play and maintainer physical acceptance.
- **Contract:** implemented MMModern decisions and required maintenance invariants.

The authoritative dependency pin and configuration remain in
[dependencies.md](dependencies.md); M40 adds no runtime reference-engine access.

| Source relative to the pinned ScummVM checkout | Decisive symbols/facts |
| --- | --- |
| `engines/mm/xeen/scripts.cpp` | `Scripts::checkEvents`, `cmdDoTownEvent`, `cmdExit`: script mode, original town dispatch, terminal return |
| `engines/mm/xeen/locations.cpp`, `locations.h` | `LocationManager::doAction`, `BaseLocation` constructor/`show`, `BlacksmithLocation::doOptions/farewell`: unchanged script mode and one departure call |
| `engines/mm/xeen/party.cpp`, `party.h` | `Party::synchronize/addTime/changeTime/resetBlacksmithWares/giveBankInterest`; `BlacksmithWares::clear/regenerate/synchronize/getSlotIndex`; `BLACKSMITH_DATA1/2`; shared unsigned bank fields |
| `engines/mm/xeen/saves.cpp` | `SavesManager::newGame`: initial party load, stock generation, then final year/day assignment |
| `engines/mm/xeen/files.cpp` | `SaveArchive::reset/loadParty`: original initial-save chunks and CHR/PTY source |
| `engines/mm/xeen/character.cpp` | `Character::makeItem`, reason 0, item index 0, levels 1..6; `subtractHitPoints` and equipped armor damage |
| `engines/mm/xeen/item.h`, `item.cpp` | `ItemCategory`, `ItemState`, `XeenItem::clear/synchronize`, effectiveness and attribute/element identities |
| `devtools/create_mm/create_xeen/constants.cpp` | `MAKE_ITEM_ARR1/2/3/4/5`, `BLACKSMITH_MAP_IDS`; actual generation tables, distinct from naming/pricing tables |
| `engines/mm/xeen/xeen.cpp` | `XeenEngine::getRandomNumber` overloads: inclusive zero-based or explicit-minimum requests |
| `engines/mm/xeen/interface.cpp` | `Interface::perform/chargeStep`: inherited distinction between service return and ordinary scheduled movement |

Adapted numeric data/logic must retain GPL-3.0-or-later notices and attribution
to the ScummVM developers in upstream `COPYRIGHT`, with the pin and corresponding
source availability. Use small private compile-time numeric tables. Do not copy
the full source tree, load `mm.dat`, add runtime ScummVM source access, or embed
commercial resource bytes. Item names remain under the existing catalog policy.

The inherited route, original Event graph, art and appearance resource manifest
belong in [M38 evidence](milestone-38-plan.md#evidence-and-provenance) and
[M38 route](milestone-38-plan.md#exact-route-event-graph-and-city-consequences).
M40 retains and revalidates those resources; no new region manifest is introduced.

### Original initialization facts

The adapter reconstructs the original initial archive from `XEEN.CC` chunks
`2a0c`, `2a1c`, `2a2c`, `2a3c`, `284c`, `2a5c`, as present, following
`SaveArchive::reset`. `maze.chr` supplies the thirty original characters;
`maze.pty` supplies party inputs. Do not read `XEEN.CUR` or user saves for fresh
initialization.

Read-only original-resource observations:

| Decoded original member | Observation |
| --- | --- |
| `maze.pty` | 812 bytes, CRC32 `866d0ff1` |
| PTY `[28,604)` | All 576 Clouds stock bytes are zero, across four categories, nine physical slots and four shops |
| PTY offsets 638, 642 | Little-endian u32 carried gold 800, gems 10 |
| PTY offsets 646, 650 | Little-endian u32 shared bank gold 0, gems 0 |
| `maze0028.evt` | Inherited 7298-byte/CRC32 `28b6c20b` service graph, record 0 |
| `DARK.CC/spells.xen` | Inherited checked 937-byte/CRC32 `63568f11` learned-spell names |

The original Clouds record address is `28 + 144*category + 16*physicalSlot +
4*shop`. This input layout is not the new save layout. The short initial PTY is
not a complete two-side stock serialization: the reference's later side-1
synchronization begins beyond this 812-byte resource. Do not invent resource
observations for that absent tail or require it to initialize numeric side 1.
Both sides are subsequently replaced by reference new-game generation.

Reference `newGame` loads the party, calls `resetBlacksmithWares`, zeroes total
time, then selects year 610 and day 1 for Clouds/World of Xeen. It does not load
an already generated Vertigo offer list. M40's **prepared day-8 Regional Journey
is a deliberate adaptation**, not a replay of seven original days or full
original startup.

## Service time and admitted domain

### Exact reached reference call

`checkEvents` establishes `MODE_SCRIPT_IN_PROGRESS`; opcode `0x11` executes
`cmdDoTownEvent`, which calls `LocationManager::doAction(1)`, constructs a
`BlacksmithLocation` and runs `BaseLocation::show`. The inherited direct repair
menu avoids the unsupported Buy scratch path. Normal lobby Escape calls
`farewell`, reloads the current map, increments initially-zero `_farewellTime`
by 1440 and makes **one `Party::addTime(1440)` call**. Clouds smith farewell
adds nothing. The caller then marks stepped/refresh-icons and executes `cmdExit`;
script mode is restored only outside this chain.

The relevant reference order is:

```text
oldDay = day
minutes += charge
while minutes >= 1440:
    minutes -= 1440
    advance day (reference wraps 100 into the following year)
if day != oldDay AND (day % 10 == 1 OR charge > 1440):
    resetBlacksmithWares()       # all sides/shops, including discarded draws
    giveBankInterest()          # gold, then gems
if day != oldDay: newDay = true
if newDay AND minutes >= 300:
    if mode is neither SCRIPT_IN_PROGRESS nor INTERACTIVE7:
        resetTemps()
        clear rested if rested/sleeping; otherwise increment eligible Weak
        and display the rest message; redraw party
    newDay = false
```

The **changed-day test is mandatory**: merely being on day 11 does not reset
stock. `1440` is not strictly greater than `1440`. A single `2880` charge can
regenerate/apply interest even when its destination is not `1 mod 10`; two
separate 1440 calls do not acquire that property. Never coalesce separate service
operations, substitute `changeTime(1440)`, replay movement steps, or loop over
each skipped day to add uncalled resets/interest. Multi-day calls are outside
M40 admission; reference-boundary tests retain this distinction.

For the admitted script operation, minute of day and `ctr24` are unchanged;
year is unchanged; `newDay` is set and cleared within the operation, leaving
false. `rested` remains false. No temporary-stat reset, Weak increment, rest
message, HP/SP healing/clamping, condition tick or actor/RNG scheduling pulse
is caused by the skipped day. Stock generation is the only new RNG consumer.
No catch-up affects the inactive mainland or city. Reclassification after
departure is inherited settlement, not a new movement opportunity.

### Chosen predicates and reservation

Content 11 canonical quiet/playing context is year **610**, day **8..99**,
Adventurer/WorldOfXeenClouds, `300 <= minutes < 1260`, `ctr24 < 24`,
`rested=false`, `newDay=false`, and the inherited zero unsupported effects/light
and resistance counters. A day greater than 8 requires retained Vertigo actors,
because only the admitted service changes the date. Ordinary world/party
validation uses this domain; admission to a further smith visit requires
**8 <= starting day <= 98**, so its ending day is **9..99**.

Regeneration/interest therefore occurs on departures `10->11`, `20->21`, ...,
`90->91`. `98->99` succeeds without either; entry on day 99 refuses before a
service obligation, repair, bank, stock, RNG or date publication. Days 0..7,
100+, other years, noncanonical context, wrong service origin, exhausted
authority and unsupported content refuse or fail according to their established
validation boundary. Day 99 still permits inherited daytime play/capture.

Entry reserves a **fully prepared detached one-day successor**, including the
entire regeneration/RNG result when required, before service admission becomes
irreversible. This makes the unavoidable departure feasible in a healthy graph;
merely checking the destination date is insufficient. The publication section
defines the asynchronous preparation and failure behavior.

Contents 9/10 retain precisely M38's day 8/9 entry, ending day 9/10 and day-10
refusal. They gain no stocks, bank fields or generation. Other legacy pairs
retain their own time rules. Do not globally broaden `xeenRegionalContext`,
`xeenPrepareTime` or condition-time admission to permit ordinary midnight/dawn,
dusk, newDay processing or year rollover. Mainland movement/Wait/Shoot and
exploration casting retain their ten-minute charges, admitted indoor movement/
Wait/casting one minute, turns zero, combat round/End one minute, combat casting
no additional cast-specific time. Existing 480-minute processing and ordinary
unsupported-boundary behavior remain intact.

## Durable representation and initialization

### Sole owners

`XeenPartyState` owns a domain-specific optional `serviceEconomy` value, with
two fixed components: `wares` and `bank`. Presence is required exactly for
content 11 and absent in every legacy/ordinary/diagnostic domain. Use existing
`XeenMutableOptional`, observed scalar/item fields and fixed arrays; no registry,
shop actor, second purse or general economy owner is introduced.
The value types are `XeenServiceEconomy`, `XeenMerchantWares` and
`XeenBankBalances`; the latter two are members of the first, not additional
independently published owners.

- `wares[side][shop][category][slot]`: **2 x 4 x 4 x 9** complete `XeenItem`
  records, 288 records/1152 serialized bytes. Physical order is stable.
- Numeric side 0 is Clouds, side 1 is Darkside/World-of-Xeen numeric stock.
  Shops 0..3 map to `{28,30,73,49}` and `{29,31,37,43}` respectively, as in
  `BLACKSMITH_MAP_IDS`. These identities are compile-time descriptors, not new
  gameplay map admission. No fallback lookup is needed for M40's exact smith.
- Categories 0..3 are Weapons, Armor, Accessories, Miscellaneous. Each record
  preserves `material`, `id`, `state`, `frame` as four u8 values in that order.
  State bits 0..5 hold effectiveness/charges, bit 6 cursed, bit 7 broken.
- All nine physical slots exist. Generation inserts into at most slots 0..7;
  slot 8 stays zero. An empty generated slot is **all four bytes zero**, unlike
  carried inventories, whose inherited ID-zero metadata remains significant.
- `bank.gold` and `bank.gems` are separate observed **u32** values shared across
  both sides and all shops. They are not `monsterTreasure.gold/gems`, pending
  treasure or a side-specific ledger. Repair reads only carried gold.

No stored generation seed, restock counter, last-reset date, level tag, item
origin, presence bit per shop, price/description, initialization flag or ledger
is necessary. The optional owner and explicit content supply domain presence;
the one world-owned `XeenJourneyRandomState` supplies continuation. Once-only
service state is transient and cannot be saved while unsettled.

### Generated-state canonical form

Each category consists of an occupied prefix of length 0..8 followed by all-zero
records, including slot 8. All occupied frames are zero; cursed/broken bits are
clear. Weapon IDs are 1..33 and state 0..6; armor IDs 1..13 and accessory IDs
1..10 have state zero. Miscellaneous uses material 1..9, special ID 1..60 and
charge/state 1..8. These broad limits are necessary but **not sufficient**.

Validate each shop as a possible output of its exact twenty generation calls,
including ordered levels, item support and category-capacity discard. Do not
replay RNG to validate a save. Define `possible(level, category, fourBytes)` by
the finite accepted outcomes of the item algorithm/tables below; no RNG calls
are made. For example, level 1 cannot produce accessories, nonzero equipment
material, armor IDs 8..13 or weapon effectiveness. Level 6 cannot produce
Miscellaneous or weapon IDs 30..33.

A bounded, deterministic validator uses a Boolean dynamic program over the
twenty-call sequence and four consumed-prefix counters (each 0..8). Start with
all counters zero. For the next level, a category transition either consumes
its next serialized occupied record if `possible` holds, or discards one
possible item of that category only if its counter is already 8. No transition
may invent a hidden item before capacity, skip a stored item, consume an empty
record or reorder a category. Accept exactly when all twenty calls and all
stored occupied records have been consumed. This also enforces per-shop totals
8..20 and level-dependent category counts without serializing history.
Use fixed bounded working storage and direct finite support tables/interval
predicates; no probability search, RNG inversion or unbounded backtracking.
Test the validator against independent literal valid/invalid outputs.

Bank accepts every u32 value, including nonzero synthetic values; it is not
restricted to zero merely because current production has no deposits. Ordinary
roster inventory validators do not impose merchant canonical form or acquire
new equipment effects from merchant records.

### Fresh prepared Journey lifecycle

`XeenPartyLoader::loadInitialCloudsParty/loadFromResources` ordinary loading
remains free of merchant/bank injection and RNG. The explicit Journey
setup/resource boundary supplies checked original bank input, obtained from the
same original PTY identity at offsets 646/650. Detach the returned bytes/value
before any further provider callback. Require the supported original PTY
identity/extent (812 bytes, CRC32 `866d0ff1`) and its zero initial balances;
malformed/truncated/mismatched inputs fail initialization. Do not require or import a side-1 initial-stock
tail. Read-only initial Clouds zeros establish provenance, not offers to keep.
The narrow `XeenCharacterFormat::parseBankBalances` parser supplies an explicit
checked bank input in `XeenJourneySetup`/the existing save-resource provider
boundary. It reads these two u32 fields; it does not deserialize initial wares
or mutate a party. Only fresh content 11 consumes this initialization input.

For fresh content 11, in this order:

1. Load/validate original party, CHR, PTY context/purse/recovery, books/names,
   Event/monster/map manifests under the existing provider guards. Preserve
   original inventories; no equipment/funds are created for the witness.
2. Perform existing prepared Regional Journey setup on a detached party:
   active roster `{0,18,14,11,1,6}`, existing levels/XP, cleared admitted
   preparation temporaries/conditions, exact calculated initial HP/SP, year
   610/day 8/minute 480/ctr24 0. Retain all thirty owners/books/supplements.
3. Create the detached algorithm-1 RNG from the chosen nonzero Journey seed
   with count zero. Generate **both sides/all eight shops once** from empty
   stock. Set bank to the checked original zeros. This is a fresh initialization
   operation, **not interest** and not seven simulated `addTime` calls.
4. Validate the complete candidate, actors/environment, stock and bank, and
   prepare final-owner guards/storage before publication. Publish the prepared
   party/stock/bank and the **post-generation** world RNG continuation with the
   existing Journey initialization. No first actor opportunity or visible/input
   frame precedes completion. `XeenActorApproach::initializeJourney` must not
   overwrite that cursor with `{seed,0}` afterward.
5. Complete existing initial presentation/attachment obligations before Quiet.

The seed is sampled once per new Journey attempt; menu entry, retries,
restore and cosmetics never reseed it. If initialization fails, there is no
usable partially initialized owner graph or salvageable quiet save. Detached
work may be abandoned and fresh unpublished destinations discarded. A retry
of the same preparation retains its seed/preimage and consumes no live draws;
starting an explicitly new Journey is a new initialization.

Restore installs serialized stock/bank/RNG exactly and **never** calls this
fresh path, reads balances as replacements, regenerates stock or applies interest.
Re-entry to Ironworks also never initializes wares lazily.

## Complete stock algorithm and tables

### Replacement and iteration order

`U[a,b]` below is an inclusive accepted request; `U[0,b]` preserves the
reference one-argument call. The reference request sequence is the contract;
ScummVM's random algorithm is not imported. M40 uses MMModern algorithm 1.

Clear all 288 output records before generation. Iterate side 0 then side 1;
within each, shop 0..3; within each shop, band 0..3; within each band, the table's
number of item calls in increasing iteration order. Reset four category counts
per shop. Every shop executes twenty calls, 160 total.

| Band | Clouds counts for shops 0,1,2,3 | Side-1 counts for shops 0,1,2,3 |
| ---: | --- | --- |
| 0 | 15,5,5,5 | 10,5,0,5 |
| 1 | 5,10,5,5 | 10,5,5,5 |
| 2 | 0,5,10,5 | 0,5,5,10 |
| 3 | 0,0,0,5 | 0,5,10,0 |

Clouds level is `band+1`. Side 1 shops 0/1 also use `band+1`; shops 2/3 use
`band+3`, reaching level 6 at shop 2. Level 7, Swords' item offset and special
event/enchant reasons are not reached.

```text
for side in 0..1:
  for shop in 0..3:
    count[4] = 0
    for band in 0..3:
      level = band + (3 if side == 1 and shop >= 2 else 1)
      repeat DATA[side][band][shop] times:
        category, item = makeItem(level, reason=0, index=0)
        if count[category] < 8:
          wares[side][shop][category][count[category]] = item
          count[category] += 1
        else:
          discard the completed result
```

Always finish generating the item **before** testing category capacity. Full
categories still consume all requests, including enchantment, special and
effectiveness requests. No retry chooses another category; no ninth-slot insert,
sorting, price ordering, icon normalization, merge with previous stock or
player-inventory transfer occurs. Each item scratch record is cleared before
assignment; discarded records cannot contaminate the next category's scratch.

### Item generation, exact request order

For each level L in 1..6:

1. Draw C=`U[0,100]`, then S=`U[0,100]` for L<6 or `U[0,80]` for L=6,
   even if the selected branch will not use S.
2. Resolve category/base ID using the tables below, issuing **only** the stated
   additional ID draw. Constant IDs consume no draw.
3. Clear the selected item; set ID to the base ID. Draw E=`U[1,100]` always,
   even for unenchanted level-1 equipment or Miscellaneous.
4. Select enchantment type. Weapons/Armor: L=1 none; otherwise E<=70 material,
   E<=98 element, else attribute. Accessories: E<=20 material, E<=60 element,
   else attribute. Miscellaneous: remember base ID in material, type usable.
5. Execute the selected enchantment requests below. For non-Miscellaneous L=1,
   no enchantment request follows E. For other equipment set material to
   `(attribute ? attribute+58 : 0) + (material ? material+36 : 0) + element`.
   Only one term is nonzero. Frame remains zero.
6. For **Weapons L>1**, after enchantment draw `U[0,20]`; exactly 10 causes
   `U[1,6]` for the state counter (Dragon, Undead, Golem, Insect, Monsters,
   Animal). Other results leave counter zero. Armor/Accessories have no such
   requests. Miscellaneous replaces ID with the usable special ID and state
   with charges. Cursed/broken remain false for every generated item.

| Level/category decision | Category and base ID |
| --- | --- |
| L=1, C<=40; or L>1, C<=35 | Weapon: S<=30 `U[1,6]`; <=60 `U[7,17]`; <=85 `U[18,29]`; otherwise `U[30,33]` |
| L=1, 40<C<=85 | Armor `U[1,7]` (S was still drawn) |
| L=1, C>85 | Miscellaneous base `U[1,9]` |
| L>1, 35<C<=60 | Armor: S>70 constant ID 8, otherwise `U[1,7]` |
| L>1, C>60 | Use the following S table |

| S upper bound, first match | Category/base ID when L>1 and C>60 |
| ---: | --- |
| 10 | Armor 9 |
| 20 | Armor 13 |
| 35 | Accessory 1 |
| 45 | Armor 10 |
| 55 | Armor `U[11,12]` |
| 65 | Accessory 2 |
| 75 | Accessory `U[3,7]` |
| 80 | Accessory `U[8,10]` |
| 100 | Miscellaneous base `U[1,9]` |

For a **material** enchantment draw `U[1,100]`; <=70 selects common row,
otherwise rare row. Draw from that row/level interval and add 9 for rare.
The final equipment material byte adds another 36. No level-7 short-circuit
is admitted, so the row selector always consumes a request here.

| `MAKE_ITEM_ARR4` row | L2 | L3 | L4 | L5 | L6 |
| --- | --- | --- | --- | --- | --- |
| Common | 1..4 | 3..7 | 4..8 | 5..9 | 8..9 |
| Rare (then +9) | 1..4 | 2..6 | 4..7 | 6..10 | 9..13 |

For an **element** enchantment draw `U[1,100]`; first matching threshold
25/45/60/75/95/100 selects Fire/Electricity/Cold/Acid-Poison/Energy/Magic.
Draw its interval from `MAKE_ITEM_ARR2` and add `MAKE_ITEM_ARR1`'s offset:

| Element | Offset | L2 | L3 | L4 | L5 | L6 |
| --- | ---: | --- | --- | --- | --- | --- |
| Fire | 0 | 1..3 | 2..5 | 3..6 | 4..7 | 5..8 |
| Electricity | 8 | 1..3 | 2..5 | 3..6 | 4..7 | 6..7 |
| Cold | 15 | 1..2 | 1..3 | 2..4 | 3..5 | 4..5 |
| Acid/Poison | 20 | 1..2 | 1..3 | 2..4 | 3..4 | 4..5 |
| Energy | 25 | 1..3 | 2..5 | 3..6 | 4..7 | 5..8 |
| Magic | 33 | 1..1 | 1..1 | 1..2 | 2..2 | 2..3 |

For an **attribute** enchantment draw `U[1,100]`; select the first matching
threshold below, draw the `MAKE_ITEM_ARR3` interval and add the subtype offset.
The final material byte adds another 58. Endurance is not an attribute subtype
in this generator. The unused eleventh offset 72 is not a reached row.

| Attribute | Threshold | Offset | L2 | L3 | L4 | L5 | L6 |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| Might | 15 | 0 | 1..4 | 2..5 | 3..6 | 4..7 | 6..10 |
| Intellect | 25 | 10 | 1..3 | 2..5 | 3..6 | 4..7 | 5..8 |
| Personality | 35 | 18 | 1..3 | 2..5 | 3..6 | 4..7 | 5..8 |
| Speed | 50 | 26 | 1..3 | 2..5 | 3..6 | 4..7 | 5..8 |
| Accuracy | 65 | 34 | 1..2 | 1..3 | 2..4 | 3..5 | 4..6 |
| Luck | 80 | 40 | 1..2 | 2..3 | 3..4 | 4..5 | 5..6 |
| HP | 85 | 46 | 1..2 | 1..3 | 2..4 | 3..4 | 4..5 |
| SP | 90 | 51 | 1..2 | 1..3 | 2..4 | 3..5 | 4..6 |
| AC | 95 | 57 | 1..2 | 1..3 | 2..4 | 3..4 | 4..5 |
| Thievery | 100 | 62 | 1..2 | 1..4 | 3..6 | 5..8 | 7..10 |

For **usable Miscellaneous**, after E draw special ID from `MAKE_ITEM_ARR5[L]`,
then charges `U[1,8]`. The intervals are L1=1..15, L2=16..30, L3=31..40,
L4=41..50, L5=51..60. Reference L6/L7 entries are 61..73, but **reason-0
merchant L6 cannot select Miscellaneous**, because S is at most 80. Neither
that row nor level 7 is permission to manufacture additional merchant offers.
Material retains the earlier base-object ID 1..9; special ID and object type
are different bytes. No general item-effect support is implied.

Degenerate intervals such as `U[1,1]` and `U[2,2]` still consume a request.
Do not optimize them into constants. All omitted L1 equipment-table entries are
reference `{0,0}` and unreachable; they are not a reason to request RNG there.

### World RNG, servicing and reuse

The sole durable owner remains `XeenSessionWorldState::_journeyRandom`.
Candidates borrow a detached `XeenCombatRandom` continuation; no merchant seed,
RNG instance with independent durability or time-based restock random source is
introduced. Algorithm 1 is unchanged:

```text
require state != 0, algorithm == 1, count < UINT64_MAX before each raw attempt
x ^= x << 13; x ^= x >> 17; x ^= x << 5       # uint32 operations
count += 1
span = hi-lo+1; threshold = (0u-span) % span
if x < threshold: reject raw attempt, retain the SAME pending request
else: accept lo + x % span
```

Validate intervals before use. Both rejected and accepted raw attempts advance
the detached state/count; an accepted request advances the item state machine.
Count is not the number of items or accepted draws. Diagnostic tapes retain
existing accepted/raw modes and are never serializable durable continuations.
At cursor exhaustion, do not wrap, reseed, approximate, fall back to another RNG
or publish a prefix. Count `UINT64_MAX` can be represented but cannot draw again.

Use the existing `XeenConsequenceDraw` convention of **at most 64 raw attempts
per service invocation**, checking the retained guard around extension hooks.
The stock candidate retains side/shop/band/item ordinal, category counts,
cleared output, current item's accepted prefix/pending request, and detached
RNG across yields. Pending work is serviced by the normal idle/coordinator path;
there is no nested SDL loop. A zero budget performs no random attempt. Retain
progress on ordinary resumable yield; ordinary exceptional preparation can
discard the unpublished candidate and retry from the same retained preimage.
Neither strategy consumes the world cursor twice. Do not expose mutable
candidate references to providers or present partial stocks as committed.

Cosmetic redraw/animation, service art, menu selection, quote/refusal/repair,
interest and non-regenerating one-day departure consume no gameplay draw.
Changing display cadence, yields or retries must not change any accepted/raw
trace or final stock. Reference cosmetic randomness is outside the adopted
static-service presentation contract.

**Reuse decision:** retain `XeenMonsterDropCandidate`'s existing bounded logic.
Reuse item byte types, checked RNG/consequence servicing and small pure numeric
helpers only where exact semantics agree. A separate merchant item candidate
handles the broader reason-0 generation. The legacy drop state machine remains
unchanged. Legacy Orc production first draws drop chance `U[1,100]`
(<=10), then level-1 C/S, base ID, unconditional E; Miscellaneous additionally
draws special/charges but is deliberately lost as
`ReferenceMiscellaneousDropLoss`. Other capacity losses, pending-source proofs,
item delivery and gold credit remain unchanged. Merchant generation has neither
that initial chance roll nor that miscellaneous loss. Tests must independently
lock both traces even if a helper is factored.

### Independently checkable numerical vectors

An independent Python transcription was compared against the **exact
pinned `Character::makeItem` function** compiled in an isolated minimal harness
with MMModern's specified RNG conversion, plus the pinned stock-loop counts.
All 1152 stock bytes and final state/count agreed for the following seeds.
This harness is diagnostic only; it is not M40 gameplay code or ScummVM's RNG.
Hashes below are SHA-256 of **stock bytes only**, ordered
side/shop/category/physical-slot/M-ID-state-frame, no header or bank.

| Initial state, initial count 0 | Final state | Raw/accepted attempts | Stock SHA-256 |
| ---: | ---: | ---: | --- |
| 1 | 2477276124 | 878/878 | `bd3799d8b453a2877656c87d4add0d513914ca7679afac20e9b87dae6e031d84` |
| 7 | 1652828136 | 901/901 | `4abf1666f71af84fbdd0a8acb10749dcf78350b8b746d946a7cf3f5b88253b65` |
| 3626689381 | 7 | 886/886 | `39cbe3234d1701fc7859afbb31a5e48f7d41407c75b4fa2364f3ee87c9143b18` |
| 2732157854 | 3686439625 | 906/906 | `b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9` |

The last row is the witness restock input; with initial count 1203 it ends at
2109. Whole-shop vectors for the chosen fresh seed and this restock input:

| Side/shop | Fresh counts W,A,X,M; discarded; raw draws | Restock counts W,A,X,M; discarded; raw draws |
| --- | --- | --- |
| 0/0 | 8,5,1,2; 4; 95 | 4,8,0,3; 5; 97 |
| 0/1 | 8,8,2,1; 1; 113 | 8,8,1,3; 0; 115 |
| 0/2 | 5,8,3,1; 3; 109 | 7,8,3,2; 0; 111 |
| 0/3 | 8,6,3,1; 2; 110 | 6,8,3,2; 1; 113 |
| 1/0 | 7,8,1,3; 1; 103 | 8,7,1,1; 3; 106 |
| 1/1 | 5,8,2,2; 3; 108 | 8,8,1,1; 2; 111 |
| 1/2 | 7,8,1,2; 2; 120 | 8,8,2,0; 2; 126 |
| 1/3 | 8,4,3,1; 4; 128 | 8,6,1,1; 4; 127 |

Fresh seed 3626689381 begins with these accepted `(lo,hi,raw,value)` requests:
`(0,100,210119046,60)`, `(0,100,4284950269,19)`,
`(1,7,1823076097,4)`, `(1,100,2020938501,2)` -> first generated Armor
`(0,4,0,0)`; then `(0,100,2561548271,7)`, `(0,100,824810118,82)`,
`(18,29,1297231098,24)`, `(1,100,291535644,45)` -> Weapon `(0,24,0,0)`.
The final two generation calls (side 1/shop 3/L5) yield Weapons
`(51,13,0,0)` and `(51,31,2,0)`, including the effectiveness draw even when
capacity discards the result.

For an independent **synthetic interleaving** test, start at the last vector's
post-stock cursor `(3686439625,2109)`, cast a lethal Arrow at an eligible Orc
(R=0, resource index 6), then generate its legacy drop with empty capacity:

| Request | Raw | Accepted | Resulting count |
| --- | ---: | ---: | ---: |
| Arrow `U[1,56]` | 2493262264 | 25 | 2110 |
| Drop chance `U[1,100]` | 617549005 | 6 | 2111 |
| Category `U[0,100]` | 1871643302 | 81 | 2112 |
| Subcategory `U[0,100]` | 1957375019 | 69 | 2113 |
| Armor ID `U[1,7]` | 887542588 | 3 | 2114 |
| Enchantment `U[1,100]` | 1051044860 | 61 | 2115 |

Expect Armor `(0,3,0,0)`, original pending-gold/source consequences and final
state 1051044860. This is not the production Slime branch or a new Orc placement.
Raw-tape rejection controls must additionally cover span 101 (threshold 68):
raw 67 rejects without advancing the request; raw 68 accepts 68 for `U[0,100]`.
A singleton interval has threshold zero and still consumes one raw attempt.

Additional literal accepted-value item tapes (each request is still executed;
these are not seeded raw tapes) provide small branch oracles:

| L; accepted values in request order | Expected category and M/ID/state/frame |
| --- | --- |
| 1: C=0, S=0, `U[1,6]=1`, E=100 | Weapon `(0,1,0,0)`; E is consumed but no enchantment/effectiveness draw follows |
| 2: C=36, S=71, E=70, material selector=70, `U[1,4]=4` | Armor `(40,8,0,0)`; constant shield ID consumes no ID draw |
| 2: C=35, S=85, `U[18,29]=29`, E=98, element selector=100, `U[1,1]=1`, `U[0,20]=10`, `U[1,6]=6` | Weapon `(34,29,6,0)`; singleton and effectiveness both consume draws |
| 6: C=100, S=80, `U[8,10]=10`, E=100, attribute selector=100, `U[7,10]=10` | Accessory `(130,10,0,0)`; S request is `[0,80]`, not `[0,100]` |
| 5: C=100, S=100, `U[1,9]=9`, E=1, `U[51,60]=60`, `U[1,8]=8` | Miscellaneous `(9,60,8,0)`; E is consumed but does not select its type |

These supplement threshold-adjacent tapes for every row and branch in the
maintenance validation boundary.

## Bank interest

Reference `_bankGold` and `_bankGems` are shared unsigned fields serialized as
u32. On the inspected 32-bit-unsigned reference target, each operation is
`balance += balance / 100`, with integer division **before addition**. M40
defines this portably as `uint32((uint64(b) + uint64(b/100)) mod 2^32)` for each
field, gold then gems. It is not `b*101/100`, signed arithmetic, rounding to
nearest, saturation or checked-overflow refusal. This explicit wrapping rule
preserves the reference's uint32 result without depending on host `unsigned`.

Run interest once **after complete stock generation** on each triggering
departure, even for zeros. It draws no RNG and does not modify carried purse,
pending gold, gems, quest counters, inventories or bank ownership. Zero is a
real durable balance, not absence or a shortcut that omits the operation.

Independent single-field expected values (exercise both fields independently
and together) are `0->0`, `1->1`, `99->99`, `100->101`, `199->200`, `200->202`,
`4252442867->4294967295`, `4252442868->0`, `4252442869->1`, and
`4294967295->42949671`. Repeated interest uses the previous result; e.g.
`199->200->202`. Include equal zero input/output with an observed once-only
committed-operation count so omission is detectable without adding a durable
ledger. Count detached preparation attempts separately: a discarded preparation
can compute interest again, but only one admitted departure may publish it.

## Coordination, publication and failure

### Existing owners and narrow interfaces

The Ironworks coordinator in `src/app/XeenSmithFlow.cpp` uses the existing
`SmithContinuation` and `Work::Service`/Event/Presentation leases. EventFlow owns
the original dispatch and menu; EncounterFlow owns reservation and publication;
the party owns context/economy/roster; the world owns RNG, retained actors and
session state. Pure candidate values grant no authority. Do not introduce a
parallel transaction coordinator, calendar owner or generalized service registry.

The checked one-day script-service `XeenServiceDayCandidate` contains exact context/economy/RNG
preimages, ending context, detached economy and cursor, stock progress, and
complete/triggered facts. Its preparation/service APIs operate on detached
values, expose immutable observations, and cannot publish. `beginSmith`, idle
service and `departSmith` are the owning integration points. The same candidate
can serve a later separately admitted one-day operation without merging calls
or admitting M41 now.

### Reservation and irreversible boundaries

1. Manual service input must already pass M38's exact Event/route and concrete
   frame authority. Complete all preexisting owed work before admitting entry:
   actor/attachment, combat, reward receipt, casting/item use, Event response or
   unpresented mandatory result prevents entry. Dormant legitimate treasure
   remains unchanged and cannot pay for repairs.
2. Under the existing exclusive Event lease, preflight the content/date/origin,
   complete owners/resources, authority-generation capacity, service art/title,
   initial frame composition and continuation storage. Prepare the **entire
   detached one-day candidate**, including all stock draws and bank arithmetic.
   On a generating day this can span idle invocations; hold a noninteractive
   preparation frame if needed. Ordinary world input and F9 stay fenced. There
   is no admitted repair menu, payment or irreversible departure debt yet.
3. After candidate completion and final checks, transfer/acquire Service work
   and install the continuation plus its owed marker, changing Journey activity
   and generations as in `beginSmith`. This callback-free transition is the
   **irreversible admission point**. Failure to present the first service frame
   afterward does not make it a free visit. Merely allocating a candidate or
   beginning preflight does not arm the debt.
4. Each confirmed repair remains its own existing atomic commit: exact carried
   gold debit + selected physical broken-bit clear + fixed result/consumed quote.
   No date/stock/bank/RNG field changes at repair. Multiple repairs share the
   reserved departure. Each repair updates the expected full guard by its exact
   authorized delta; the reserved departure's context/economy/RNG preimages
   remain unchanged because no other gameplay can interleave.
5. Departure consumes a newly presented phase response. Revalidate the complete
   retained graph, Service lease, once-only operation and the reserved
   context/economy/RNG preimages. Prepare the future guard **from the current
   checked post-repair expected state**, substituting only the reserved
   successors. All allocation, candidate validation, output/result storage and
   mutation-range preparation precede the final check.
6. **One atomic departure publication** stores ending context, complete wares,
   bank and world RNG, sets the transient `departed` marker, publishes fixed
   result facts and adopts the prepared exact expected-delta guard/generations.
   These stores are callback-free and nonthrowing. At a nontriggering day the
   economy/RNG successor is byte-identical; no artificial random draw occurs.
7. Only afterward perform active-city classification, `publishArrival`, original
   terminal Event continuation, required attachment/reward work and truthful
   final world presentation. Transfer exclusive work without a Quiet gap. The
   Service lease/continuation can retire only when the inherited handoff owns
   all remaining work. Capture opens only at a successfully presented Quiet
   boundary, never just because a modal queue became empty.

**Atomicity decision:** date, stock, bank and RNG form one departure unit. The
reference evaluates date -> generation -> interest -> newDay sequentially; the
detached preparation preserves that evaluation order, but no external callback
can observe a partial departure. A separately irreversible date or stock would
need additional recovery states without useful gameplay visibility. Repairs
are deliberately outside this unit: a later departure failure never refunds
gold or restores broken armor. M37 Event prelude/reset/transition publications
and M39 spell units remain separate and unchanged.

The reserved candidate is private and bound to the exact operation and owner
incarnations. No UI field, copy of a quote/result, matching item elsewhere,
diagnostic observation or public candidate can authorize publication. Consume
responses and enter the busy/reentrancy fence before callbacks. Reserve enough
generation capacity for the immediate admission/publication transitions;
never wrap runtime generations. If a later independent operation cannot reserve
its own required authority, stop before that operation.

Diagnostic generation traces distinguish unpublished preparation attempts from
the committed operation's trace. A discarded retry may repeat calculations;
the authoritative count and published trace advance only once from the retained
preimage. A successful admitted candidate is retained across later repair and
presentation failures. This distinction must not hide an actual duplicate
world-cursor publication.

### Failure and retry contract

| Situation | Required behavior |
| --- | --- |
| Day/content/origin refusal before admission | No obligation or owner mutation. Present the accurate refusal; day-99 text names the supported-year limit, while legacy day-10 text retains its restock limit. |
| Ordinary resource/allocation failure or RNG exhaustion during pre-admission preparation | No stock/date/bank/live-RNG mutation or repair. Keep/retry the private preparation from its checked prefix, or discard it and return to healthy exploration after a truthful frame. RNG exhaustion is a deterministic support refusal, never endless automatic retry. No complete reservation means no admission. |
| Ordinary failure after admission but before a repair/departure commit | Preserve already committed repairs and the owed departure. Retry only unfinished preparation via fresh presented authority. The complete reserved stock/RNG candidate is reused, not generated afresh against a changed cursor. |
| Failure after repair publication | Keep debit/item and fixed result; show/retry that result. Reusing the consumed quote cannot charge again. |
| Failure after atomic departure publication | Keep date/stock/bank/RNG and `departed=true`; retry only remaining classification/Event/presentation settlement. Never repeat regeneration, interest, date or repair. |
| Recoverable composition failure | Remain exclusive/unsaveable; recompose the same committed facts or pending phase. No gameplay replay. Native upload/copy failure must not acknowledge an unpresented frame. |
| Fatal rendering, unsupported mandatory settlement or shutdown | Preserve committed in-memory prefix and previous disk save, remain unsaveable or terminate. No hidden autosave/deferred debt record. |
| Stale/reentrant/wrong phase/foreign frame response | Reject before providers, generation, capture or publication. The legitimate continuation remains. |
| Owner mutation, mutation then reversion, immutable mismatch, invalid canonical graph, tampered reserved candidate or impossible post-admission RNG exhaustion | Latch integrity failure and reject publication/save permanently for that graph. Equal later values, retries and cache rebuilds cannot clear it. |

Admission has already completed stock generation, so healthy departure cannot
later exhaust its RNG. Test the hypothetical altered cursor/prefix as an
integrity violation, not a license to reseed or silently release the obligation.
An ordinary failed operation never rolls back earlier commits or adopts a
visit-start snapshot. F9 must be rejected **before capture/provider invocation,
save-path preparation and I/O** throughout pre-admission exclusive work, Service,
failed settlement and unpresented handoffs. Preserve held/repeat/batched-input
fencing and the inherited semantic-epoch/concrete-frame distinction.

### Full guards and mutation observation

The existing `XeenRestoreGuard`, combat `PartyPreimage`/`exact`, capture guards
and restore final-destination preimages include complete economy presence/values,
every physical item byte and bank scalar. Party copy/move/swap, private
publication paths, equality utilities and snapshots preserve this state.
No broad whole-party reassignment may bypass marked-roster/borrow restrictions.

The checked graph still contains all thirty roster owners, books, supplements,
inventories including empty metadata, membership, context, purse/pending treasure,
quests/recovery, camera/flags/overlays, both retained regions and actor statistics,
world RNG, lifecycle, owner identity/replacement revisions and authority. Combat
guards cannot ignore stocks merely because combat never consumes them. Likewise
service cannot ignore inactive characters or mainland actors.

Use fixed inline economy arrays so all nested storage is within the observed
party range; use observed `XeenItem` bytes and bank scalars, including assignment,
reset, replacement and swap. If future changes materialize storage elsewhere,
extend the existing `XeenMutationWatch::prepareOwned/addOwned`
insertion/reconstruction handshake before any reference escapes. Registering
only at the next `current()` call is too late. Newly reconstructed Map/MOB/Event
nested storage retains the same requirement. Never clear another observer's
mutation history while adding a range.

Fresh initialization inputs are detached before providers and retained through
the existing guard boundary. The generation tables above are private immutable
compile-time data, with explicit provenance and independently tested numeric
identity; do not expose a mutable table provider. Any diagnostic descriptor
substitution must be validated/detached before candidate work. Existing required
original-resource identities, bank source, service route/art/title, maps, MOBs,
Events, monster records and spell-name preimages join/retain the same immutable
resource union across service, combat, capture, restore and cache eviction.
Changed admitted resources poison the graph even if a later load returns the
old bytes. No new parallel integrity framework is needed.

Check full preimages before and after every external callback, on exceptional
returns too. Prepare exact expected successors and mutation-range capacity
before nonthrowing publication; adopt only the authorized deltas and coordination
changes. Never renew a guard from arbitrary callback-mutated live values.
Prepared guards must bind final owner addresses before fresh/restore references
escape, including newly engaged optional stock storage.

Both scheduling and cosmetic combat clock callbacks check complete retained
preimages and mutation history before and after invocation, including exceptional
exits. Bank gold/gems, any stock byte and economy reset/reconstruction cannot
mutate and revert undetected. Integrity failure is monotonic across combat
presentation, guard replacement, retry and cache reconstruction; equal later
values cannot restore gameplay, Quiet or F9 authority. Authorized callback-free
combat publications renew only their checked mutation boundary.

## Persistence and compatibility

### Exact version choice

Keep **envelope version 4**: its existing magic, 20-byte header, Clouds side,
reserved zero byte, payload length/CRC32 and bounded base save are sufficient.
The final choice is **Journey schema 9 / content 11**. Schema 9 is necessary
because complete stock and bank do not fit schema 8; content 11 identifies the
new initialization, time, RNG and gameplay semantics. This is unrelated to the milestone number.
Do not create envelope 5, schema 40, or silently reinterpret 8/10.

The supported-pair set is exactly `{1/1,2/2,3/3,4/4,5/5,6/6,7/7,8/8,
8/9,8/10,9/11}`. Preserve ordinary v1/v2 and completed v3 domain behavior and
existing envelope-v4 legacy representations. Unsupported pairs, including
9/9, 9/10, 8/11 and 10/11, reject. No migration, backfill, old-save generation,
new-game input replacement or implicit upgrade is allowed. Fresh
`--journey-region` selects 9/11; explicit legacy test setup and restore keep the
saved pair. Diagnostic/skeleton/expedition startup gains no economy state.

### Wire contract

The schema-9 Journey suffix preserves every field/order/width of
[M38's exact schema-8 wire](milestone-38-plan.md#exact-successor-wire), with
schema=9 and content=11 in its pair header. This includes all thirty learned
books and poison records and the optional city block after recovery/treasure.
Learned-book bytes retain their original raw nonzero values (1..255); never
normalize them into Boolean 1 during capture, restoration or successor copying.
Append **exactly 1164 bytes**, after the entire optional city block:

| Order | Encoding |
| --- | --- |
| Shape | u8 side count=2, u8 shops per side=4, u8 categories per shop=4, u8 physical slots per category=9 |
| Wares | All 288 records in side/shop/category/slot order, each u8 material, u8 ID, u8 state, u8 frame; 1152 bytes |
| Bank | gold u32 LE, gems u32 LE; 8 bytes |

Identities are implicit fixed indexes, never sorted by description or encoded
as player-accessible merchandise IDs. There are no per-item lengths, occupancy
counts, price fields, padding, optional economy flag or initialization marker
on this suffix. The in-memory/snapshot optional is mandatory for 9/11 and absent
otherwise. Wrong/missing shape or any trailing byte rejects.

With N combined occupied pending monster items (inherited N<=12), exact suffix
sizes are `4278+5*N` without city, `5156+5*N` with 46 city actors, and
`5270+5*N` with 52 city actors; maximum **5330**. The whole-file bound remains
4 MiB. Schema-8 formulas/bounds remain `3114/3992/4106 + 5*N`, maximum 4166.
Legacy schema-1/2 fixed sizes and schema-3..7 rules stay unchanged. The decoder
enforces both the initial suffix envelope bound and exact schema-specific extent
checks; widening the upper bound alone cannot admit trailing interpretation.

Validate the pair before selecting wire features. Retain all base/actor/treasure
cross-owner canonical checks; apply the new date and stock canonical predicates.
Day>8 requires retained city; an absent city requires mainland camera/no city
overlays; city camera must lie in the inherited sixteen cells. Preserve the
46/52 actor forms, canonical 46..49 gaps, reset provenance/overlay record 764,
flag-9 semantics, dormant actors/treasure and original actor closure. Validate
bank as exact u32, without enforcing zero on restore. Do not validate generated
stock using equipped-party consumer restrictions or add its effects to party AC.

### Explicit capability and representation gates

`XeenJourneyContent` explicitly admits service days only for content 11, combat
casting for 10/11, Armor Repair for 9/10/11, and schema 9 only for 11. City
geometry/actors/transitions include 11 through the inherited Vertigo capability;
poison inputs and city wire features exist in supported schemas 8 and 9. Known
pair validation precedes representation feature checks. Calendar validation
separates legacy day 8..10 from content-11 day 8..99.

Party copy/move/swap, detached Event and disengagement candidates, combat
preimages, capture, restoration and final-owner guards all preserve complete
economy presence/bytes. Fresh setup carries checked bank input and publishes
the post-generation RNG. Existing names containing `Contract8` denote inherited
city capability, not permission for arbitrary schema/content inequalities.

### Quiet capture and fresh-owner restore

Capture reads only the actual party/world owners after the inherited exclusive
Quiet acquisition, full guard and resource checks. It copies all stocks, banks,
items, books, inputs, both regions, context and exact RNG continuation. No
generation, interest, canonical normalization or synthetic zero-fill occurs.
Service/menu selections, prepared candidates, debt/departed markers, leases,
operation IDs, runtime generations and frame authority are absent from saves.

Restore detaches the decoded snapshot, validates representations and installed
original-resource signatures/manifests, prepares new unpublished party/world/
camera/flags and all guard storage, then installs exact saved fields. It
validates the active view without moving/activating actors and preflights the
first frame before final checked publication. Retain checked generation-table
identity and required resource union, but do not invoke fresh economy providers
to replace saved balances. Publish once, bind fresh runtime Flow authority and
require successful presentation before input. A save at `(28,8,4)` does not
execute the service Event on load. No reseed, restock, interest, reset, spell,
condition/time work, HP/SP recomputation, book replacement or treasure replay is
allowed before first input.

Content 11 retains Magic Arrow, First Aid and Awaken under M39's exact cost,
turn, target, RNG and shared consequence rules. Service days do not restore SP
or HP, clear conditions/books, reset participation consequences or respawn
actors. Content 10 remains combat-capable but cannot cross its day-10 service
gate. Content 9 retains combat-C refusal. Use explicit regression captures and
fresh-process restores for both, not just a successor save test.

## Production acceptance boundary

### Evidence boundary

The accepted production witness uses original fresh inputs, normal
Application/Flow/SDL work and concrete presented controls. It creates no items,
funds, wounds, actors or recovery through owner edits. Automated deterministic,
original-resource/process, independent-review and maintainer physical evidence
remain distinct; synthetic fault/arithmetic controls cannot substitute for play.

Seed **3626689381** produces the independently checked fresh stock vector and
post-generation cursor **(7,886)**. This preserves M38's seed-7 gameplay prefix
with count shifted by 886; it is actual generation, not a live cursor overwrite.
Plain seed 7 instead starts play at state 1652828136. Earlier artificial
feasibility diagnostics are not part of production acceptance.

### Deterministic accepted route

Use `U` forward, `D` backward, `L/R` turn, `F` Shoot; each character means a
separate fresh presented input, with inherited mandatory work drained between
steps. Space is manual interaction, Yes accepts the existing city question.
Outside the deliberate breaking/casting policies below, settle combat with
Attack and acknowledge mandatory Event/treasure results. Do not edit live gold,
items, HP/SP, conditions, books, actors, camera, date or RNG to meet a checkpoint.

1. Fresh `--journey-region --combat-seed 3626689381` from the original
   installation. After initialization expect day 8/minute 480, RNG `(7,886)`,
   generated-stock fresh hash above, bank 0/0, carried gold/gems 800/10.
2. Follow M38's exact original-input prefix: `UFUDD`, then `LLULUU`, Space,
   Yes, settle; then `URULUUULUUU`. During the entrance-Slime fight, Block
   each ready player action until Seymour's Armor slot 0 is naturally broken,
   then Attack to settlement. This is 39 Blocks in the verified prefix.
   At `(28,13,4,West)`, Seymour is HP -11, both equipped Armor slots 0/1 have
   broken bit 128, minute 577; only original combat produced that damage.
3. Rebecca (active slot 4/roster 1) casts exploration First Aid twice on
   Seymour (active slot 5/roster 6), each paying one SP. Seymour reaches HP 1,
   Rebecca SP 19, minute 579. Walk `UUUUU` to `(28,8,4,West)`.
4. Checkpoint A: day 8/minute **584**, ctr24 **2**, carried gold **810**,
   RNG **(2732157854,1203)**; Seymour armor slot 0 `(0,1,128,3)`, slot 1
   `(38,10,128,9)`. All six active characters are alive and conditions zero.
   The earned 10 gold came from mainland Orc record 9; no purse injection.
5. Make two ordinary zero-transaction visits, departing each time, to days 9
   and 10. These reserve/pay individual calls, never one two-day charge. Stock,
   bank, minute, ctr24 and RNG stay exact. They establish the date without
   consuming either real repair opportunity; they are not the gameplay proof.
6. On day 10, enter, choose Seymour (F6), Repair armor, physical slot 0 (key
   1), quote and confirm. Pay **2** gold, reach 808, item `(0,1,0,3)`, AC +2.
   Acknowledge and depart. Checkpoint B: **day 11/minute 584/ctr24 2**, RNG
   **(3686439625,2109)**, replacement stock hash above, bank **0/0** with one
   observed interest operation. The second armor record is still broken.
7. Continue actual cleared-city navigation `DDDDDDDUUUUUUU` (seven backward,
   seven forward), returning to the smith. No contact/draw occurs. Checkpoint C:
   day 11/minute **598**, ctr24 **16**, same RNG, gold 808 and retained actors.
   Make a later visit and repair physical slot 1 (key 2) for **1** gold:
   `(38,10,0,9)`, AC +1. Depart to **day 12**, gold **807**, same minute/ctr24,
   stocks/bank/RNG. Thus real repair after day 10 is useful independently of Buy.
8. At this presented Quiet boundary, F9 to checkpoint D. **Exit the complete
   process**, start a fresh load, and compare all durable fields before the
   first input, including stock/bank/RNG and both regions. An uninterrupted
   branch from D and the fresh restored branch must execute identical inputs.
9. Rebecca casts exploration First Aid three times on Seymour, then once on
   herself. At the smith cell expect minute **602**, ctr24 16, Rebecca SP **15**,
   Seymour SP **27**, same RNG. This uses earned survival capacity and existing
   learned spells; no rest/Temple/well/resource replenishment is needed.
10. Walk `DDDDDDDLUUUU`, Space/Yes to the original exit; then `LLU`, Space/Yes
    to re-enter. Settle the inherited exit/reset/arrival work normally. Follow
    `URULU`. On the reset entrance-Slime contact, Block all ready characters
    except: at Rebecca's first ready turn cast combat **First Aid on Seymour**;
    at Seymour's next ready turn cast **Awaken**; at Seymour's following ready
    turn cast **Magic Arrow** at the displayed Slime contact. Drain mandatory
    rounds/results; do not Attack away the target before the Arrow. Learned
    physical slots are Clerical First Aid 14, Wizardry Awaken 0 and Arrow 25.
11. Checkpoint E at `(28,16,2,North)`: expected **day 12/minute 628/ctr24 12**,
    gold 807, Rebecca/Seymour SP **14/24**, RNG **(2018868320,2180)**. Slime
    slot 36 is defeated after Arrow HP **2->0**; reset city has 52 slots and
    protection overlay `(28,764)`. All six active owners survive with HP
    `{10,11,4,29,3,13}` in active order `{0,18,14,11,1,6}`, all conditions zero.
    Mainland state and stock/bank are retained. Capture/restore E too, then
    perform identical further navigation (e.g. `LR`) to prove continued input.

The original-resource/process witness verified checkpoints A-E and exact
uninterrupted/fresh-process continuation. Stock never transfers to characters.
Several mainland actors remain alive (all except initial Orc 9 in this route);
original city exit/reset supplies the later legitimate Slime life. The route
stays below dusk and the next 960-minute condition tick.

The accepted 71 post-stock raw draws end with
`U[1,50], raw=2018868320, accepted=21, count=2180` for the Slime Arrow.
First Aid/Awaken publications were at cursor `(1023461121,2143)` and drew
nothing themselves. For a compact independent trace oracle, canonicalize each
accepted line as ASCII `DRAW lo:hi:value:state:count\n`, no CR; SHA-256 of
the 71-line trace is
`cf2803fc50d6165d49bc92c942c1ca77abbc7489707821af72340ec9d8c0f6a1`.
Its first three lines are `DRAW 1:2:1:2493262264:2110`,
`DRAW 1:47:26:617549005:2111`, `DRAW 1:47:44:1871643302:2112`.
These are intervening ordinary combat requests, not immediate Arrow calls.
Compare complete traces, not only this digest or terminal cursor. Future
rejection traces must also record rejected raw attempts explicitly.

The complete compact request/value trace follows. Every lower bound is 1;
columns are `resulting count : upper bound : accepted value`. There were no
rejections. Starting state 3686439625 and algorithm 1 reconstruct every raw
value, so this table plus the formula independently determines the digest:

```text
2110:2:1   2111:47:26 2112:47:44 2113:2:2   2114:40:29 2115:40:21
2116:2:2   2117:42:34 2118:42:23 2119:2:1   2120:60:2  2121:2:1
2122:47:29 2123:47:47 2124:2:2   2125:40:29 2126:40:2  2127:2:1
2128:47:18 2129:47:35 2130:2:1   2131:40:13 2132:40:7  2133:2:2
2134:42:11 2135:42:24 2136:2:1   2137:60:5  2138:2:2   2139:47:11
2140:47:39 2141:2:1   2142:40:20 2143:40:2  2144:2:2   2145:47:19
2146:47:31 2147:2:1   2148:40:7  2149:40:25 2150:2:1   2151:42:7
2152:42:18 2153:2:2   2154:60:52 2155:60:38 2156:2:2   2157:47:45
2158:47:36 2159:2:1   2160:40:24 2161:40:34 2162:2:1   2163:47:11
2164:47:24 2165:2:2   2166:40:34 2167:40:22 2168:2:1   2169:42:5
2170:42:40 2171:2:1   2172:60:50 2173:60:30 2174:2:1   2175:47:29
2176:47:37 2177:2:1   2178:40:15 2179:40:6  2180:50:21
```

### Reproducibility and native physical boundary

`mmodern_m40_cli_witness` follows the accepted route through production owners;
`mmodern_service_day_process_tests <witness-executable> <original-installation>`
orchestrates distinct process incarnations, exact pre-input restore comparisons
and identical continuation. `mmodern_combat_clock_process_tests` covers both
combat clock paths and presentation after genuine service/full-exit/restore.
Synthetic controls are separately labeled. Deterministic seed selection is a
control, not original game data.

Maintainer physical acceptance uses normal native SDL, with no dummy driver or
witness/control environment variables:

```text
mmodern --journey-region --combat-seed 3626689381 <original-installation> --save-file <acceptance-save>
mmodern --load-game <original-installation> <acceptance-save>
```

Actual keyboard-driven repair/payment, repeated departure, navigation, quiet F9,
complete exit/fresh load and further exploration/combat casting establish the
physical boundary. Automated SDL input and screenshots do not substitute for
maintainer play. Normal Escape-in-service departure and window-close behavior
remain inherited.

## Maintenance validation boundary

The contract is maintained by generation/finite-support rules, service-day
candidates, initialization, codec/mutation tests and original-resource/process
witnesses. Coverage includes:

- Literal item/table/request vectors, all 160 calls, capacity discards, complete
  stock hashes, canonical prefix/order/level rejection and unchanged legacy drops.
- Raw rejection and singleton draws, 0/1/63/64 budgets, cursor exhaustion,
  retained/discarded preparation, exact retry and cosmetic-cadence independence.
- Original fresh bank/stock inputs and final-owner publication, malformed inputs,
  allocation/callback faults, exact nonzero bank restore and no fresh-provider replay.
- Every day 8..98 successor, canonical minute/ctr24 endpoints, trigger dates,
  single-charge reference contrasts, bank rounding/wrap and ordinary time isolation.
- Independent repair/departure commits, zero-transaction and later triggering
  visits, reservation/authority exhaustion, settlement failure/retry, native
  rendering faults, stale/reentrant input and F9 refusal before providers or I/O.
- Complete economy/owner/resource mutation observation, immediate ABA after
  insertion/reconstruction, both clock callbacks and exceptional exits, and
  monotonic failure through presentation/guard replacement.
- Exact 9/11 wire lengths and shape, malformed/crossed pairs, canonical stocks,
  retained-city/reset rules, raw learned flags and legacy 8/9 and 8/10 isolation.
- Genuine A-E repair/navigation/casting, full process exit, exact fresh restore
  before first input, subsequent gameplay and independent raw/request traces.

Process equality compares resource signature/pair, camera, membership, all
thirty characters, supplements/books/raw inventories, quest/game flags and
overlays, every actor field in both regions, recovery, context, purse/pending
treasure, all 288 stock records, bank balances and RNG algorithm/state/count.
Encoded-byte equality and field checks accompany independent expected traces;
replay followed by apparent convergence cannot satisfy the pre-input boundary.

## Final acceptance

**M40 is COMPLETED AND ACCEPTED**, within the scope and exclusions above.

- **Automated deterministic/testing evidence:** the normal all-target build and
  complete unfiltered CTest passed, **114/114**. Generation, time/bank, atomic
  publication, mutation/failure and persistence controls passed; `git diff --check`
  passed. Legacy M38/M39 behavior remains compatible.
- **Original-resource/process evidence:** the dedicated M40 witness passed
  checkpoints A-E, both genuine paid repairs, zero-transaction departures and
  further navigation/casting. Quiet save, complete process exit, exact pre-input
  fresh restore and identical subsequent continuation passed, including retained
  economy, both regions and RNG. This is separate from physical acceptance.
- **Independent technical review:** the final implementation is accepted with
  no remaining findings.
- **Focused independent re-review:** review of the corrected combat-clock ABA
  defect returned **ACCEPT**, confirming both scheduling/cosmetic paths,
  normal/exceptional exits, bank gold/gems, stock-byte
  and economy reconstruction ABA, monotonic presentation/guard failure and F9
  closure. Post-service/full-exit/restore/later-combat and healthy clock,
  presentation and casting coverage passed. The durable guarantee is specified
  in the guard contract above.
- **Maintainer physical native-SDL acceptance:** the maintainer physically played
  the normal seed-3626689381 route without dummy SDL or witness/control variables.
  Startup/controls, days 8->9->10, day-10 admission and 2-gold repair, day-11
  navigation/later 1-gold repair, day-12 departure, quiet F9, full application
  exit and fresh load all behaved as required. Further exploration First Aid,
  combat First Aid/Awaken/Magic Arrow and exit/re-entry/reset-Slime continuation
  passed with normal keyboard input. This is maintainer-performed physical
  acceptance, distinct from automated/process/reviewer evidence.

The final domain is **envelope v4 / schema 9 / content 11**. Generated numeric
side-1 stock admits no Darkside gameplay. Buy/Sell, Training, Temple and general
calendar processing remain excluded. Closure grants no M41 planning or
implementation authorization.
