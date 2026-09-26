# Milestone 38 - Vertigo Ironworks armor repair

## Completed scope and acceptance boundary

**COMPLETED AND ACCEPTED.** M38 connects bounded Vertigo Ironworks Armor Repair
to the production Regional Journey. Automated deterministic validation,
original-resource/process validation, independent technical review and
maintainer physical native-SDL acceptance passed; see [final acceptance](#final-acceptance).

The M38 slice is the original Ironworks **repair of carried armor**, IDs
1..13 with material 0 or 38, reached through its actual town-service Event. It
connects earned gold, physical inventory records, broken equipment, existing
armor-class/combat consumers, retained city consequences and quiet saving.
It adds five player cells beyond M37's outside-door endpoint. It does not add
purchasing, selling, paid identification or merchant stock.

Departure from this original service costs **1440 minutes even if no repair was
made**. M38 implements that bounded script-mode calendar operation. Visits may
start on Journey day 8 or 9 and finish on day 9 or 10 respectively. A new visit
on day 10 is visibly refused before admission: its departure would require the
unsupported day-11 stock regeneration and bank-interest path. Multiple repairs
within one admitted visit are allowed. This is a support boundary, not an
original shop opening rule. Day numbers here are stored context values, not a
new calendar display convention. It permits two independently useful visits and an
exact restart/revisit witness without inventing a general calendar or skipping
mandatory service work.

This completes the economy step of the M36-M38 arc: an actual
damaged, equipped item becomes useful again in the production Journey in
exchange for carried gold. The reusable part is a checked item/purse publication
path integrated with existing owners, not a generic transaction engine or
service registry. No mandatory stock dependency remains in the selected path.

Inherited contracts remain authoritative except for explicit content-9 changes
below:

- [M24 catalog, inventory and transfer](milestone-24-plan.md),
  [M25 equipment](milestone-25-plan.md).
- [M33 purse, item production, consequences and time](milestone-33-plan.md),
  [M34 lifecycle and dormant treasure](milestone-34-plan.md).
- [M35 Event recovery/item use](milestone-35-plan.md),
  [M36 learned casting and publication](milestone-36-plan.md).
- [M37 route, reset, retained regions and authority](milestone-37-plan.md),
  [dependencies and resource adapters](dependencies.md).

## Evidence and provenance

### Evidence classes

1. **Original-resource facts:** decoded initial DAT/MOB/EVT/PTY/CHR and archive
   appearance/text resources from the external installation. The existing
   `XeenAssetSource` bridge assembles the initial archive from XEEN.CC initial-save
   chunks. No player's mutable XEEN.CUR supplies initial state. No commercial
   payload, extracted fixture or original file is a repository deliverable.
2. **Pinned-reference-derived behavior:** ScummVM
   `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, as configured in
   [dependencies](dependencies.md). This is
   reference interpretation, not independently observed DOS execution.
3. **MMModern decisions:** the armor/material boundary, direct repair menu,
   static service illustration, temporal admission gate, owner/publication
   design and schema/content separation below.
4. **Artificial deterministic controls:** the supported CLI seed and injected
   typed inputs used to establish the production witness; fault, funds,
   inventory, stale-authority and condition fixtures. These are separate from
   original resources and must never be described as original game records.

At the dependency pin, the decisive source chain is:

| Source relative to ScummVM | Functions/data and question answered |
| --- | --- |
| `engines/mm/xeen/scripts.cpp` | `Scripts::checkEvents`, `cmdDoTownEvent`, `cmdExit`: script mode, dispatch and terminal return |
| `engines/mm/xeen/locations.cpp`, `locations.h` | `LocationManager::doAction`, `BaseLocation::show/drawAnim`, `BlacksmithLocation` constructor, `doOptions`, `farewell`: menu, art, stock access boundary and departure |
| `engines/mm/xeen/dialogs/dialogs_items.cpp` | `ItemsDialog::execute`, `calcItemCost`, `doItemOptions`, `setEquipmentIcons`: buy scratch inventory, repair, sell, identification, ordering and raw item mutation |
| `engines/mm/xeen/item.cpp`, `item.h` | `XeenItem`, inventory descriptions, identified details/attributes and item state bits |
| `devtools/create_mm/create_xeen/constants.cpp` | `ARMOR_BASE_COSTS`, `ITEM_SKILL_DIVISORS`, `ELEMENTAL_DAMAGE`, `BLACKSMITH_MAP_IDS`, `TOWN_ACTION_SHAPES/FILES`, `TOWN_MAXES` |
| `engines/mm/xeen/party.cpp` | `Party::subtract/addTime/changeTime/resetBlacksmithWares/giveBankInterest`; `BlacksmithWares::clear/regenerate/getSlotIndex/synchronize/blackData2CharData/charData2BlackData`, `BLACKSMITH_DATA1/2` |
| `engines/mm/xeen/saves.cpp`, `character.cpp` | `SavesManager::newGame`, `Character::makeItem`: stock initialization and generation dependence |
| `engines/mm/xeen/interface.cpp`, `dialogs/dialogs.h` | `Interface::perform`, `chargeStep`, `draw3d`, inline `ButtonContainer::clearEvents`: service return, input consumption and actor scheduling |

Adapted numerical tables/logic retain ScummVM developer attribution and
GPL-3.0-or-later provenance under the existing dependency policy. No reference
engine instance or additional dependency checkout is introduced.

### Reproducible original resource findings

The complete inherited region/resource manifest remains in
[M37 evidence](milestone-37-plan.md#evidence-and-provenance). The decisive
additional checks used decoded-resource CRC32, not archive offsets:

| Resource | Bytes | CRC32 | Finding |
| --- | ---: | --- | --- |
| `maze0028.evt` | 7298 | `28b6c20b` | Record 0 is the actual blacksmith dispatch |
| `maze.pty` | 812 | `866d0ff1` | Initial purse 800 gold/10 gems; initial Vertigo stock slots are zero |
| `011.obj` | 7355 | `9eea738b` | Additional corridor view resource |
| `blck1.twn` | 42348 | `70459675` | Eight decodable frames; frame 0 supplies the service illustration |
| `blck2.twn` | 40309 | `37e6459c` | Eight decodable frames; investigated, not required by static presentation |
| `esc.icn` | 792 | `096b68b7` | Two decodable frames; existing-style departure icon |
| `buy.icn` | 8566 | `a8c84c02` | Twenty decodable frames; investigated, not a required buy UI |

The title remains entry 33 of `aaze0028.txt` (3014 bytes, CRC32 `8dc60e26`).
Item names use M24's checked English catalog and external DARK.CC `mae.xen`,
including its existing bounded fallback. Do not hard-code original item or
location text. New controls/refusals are authored English application text.

Original-resource validation uses the existing archive, map, Event, actor,
compositor and sprite adapters. The production witness uses the real
Application/CLI path and frame-bound input without overwriting live owners.
Artificial fault/state controls remain distinct from that witness and from
maintainer physical acceptance.

## Exact route, Event graph and city consequences

All map identities here are Clouds (side 0); directions are N=0, E=1, S=2,
W=3 and Event All=4. Record indexes and offsets are zero-based. Logical region
28 and its four geometry tiles retain M37's meaning; each new cell lies in
physical root tile 28 at the same local coordinates.

Content 9 admits the sixteen-cell set:

```
{ (15,y) : 0 <= y <= 4 } union
{ (16,y) : 1 <= y <= 4 } union
{ (x,4) : 8 <= x <= 14 }
```

All four facings and inherited movement/turn/Wait rules apply. From the
west-facing outside label `(28,13,4)`, walk forward five times to `(28,8,4)`.
No special service hotkey or outside-door interception is allowed.

| Added cell | Walls N/E/S/W | Surface | Flags / raw attributes |
| --- | --- | ---: | --- |
| `(12,4)` | `8,4,8,0` | 2 | `8 / 10` |
| `(11,4)`, `(10,4)`, `(9,4)` | `0,0,0,0` | 2 | `8 / 10` |
| `(8,4)` | `8,0,8,12` | 2 | `8 / 10` |

Values are decimal. Flags 8 indicate the ceiling, not automatic Event dispatch.
The M37 outside cell has walls `8,0,8,4`, surface 1 and automatic-event flag 16.
Wall 4 permits passage; wall 12 at the west end blocks. Reuse current-cell
wall `< wallNoPass 7` and destination-surface collision, including asymmetric
walls. Original open north/south edges in the corridor do not admit neighboring
rooms: a step outside the cell set visibly refuses before new camera/time/actor
publication. This is an explicit support limit, not altered geometry.

The complete production graph is below. Opcodes are hexadecimal; record indexes,
offsets, lines and operands are decimal.

| Physical site | Records / byte offsets | Decoded instructions and closure |
| --- | --- | --- |
| Mainland `(23,10,13,All)`, manual | 136/1141, 137/1148, 138/1157, 139/1163 | Lines 0..3: `01 [33]`, `09 [44,0,3]`, `12 []`, `07 [28,15,0]`. Yes teleports retaining facing and terminates; No exits. No automatic arrival dispatch. |
| Outside `(28,13,4,West)`, automatic or manual under inherited rules | 539/4471 | Line 0 `02 [33]`; natural end after the complete label instruction. |
| Ironworks `(28,8,4,All)`, **manual Space only** | **0/0**, total 7 bytes, length field 6 | Line 0, **opcode `11`, operand `[1]`**. `cmdDoTownEvent` dispatches `LocationManager::doAction(BLACKSMITH=1)` and then `cmdExit`; no call stack, conditional branch, reward, prelude or following instruction. |
| City departure `(28,15,0,South)`, manual | 760..769 plus transitive records | Exactly [M37's exit/prelude](milestone-37-plan.md#complete-exit-prelude-and-flag-9), including all flag branches and the reset call; unchanged. |

The service operand is a location action, not a text/item/map ID. A successful
normal service departure returns result 0, marks the reference `_stepped` and
icon-refresh work, and aborts the script through `cmdExit`. MMModern represents
that terminal continuation explicitly. It does not resume at line 1, run a
different service or teleport. Camera and facing remain at `(28,8,4)`; return
to exploration, walk east and retrace the admitted route. Manual entry means
the service does not reopen merely on return, redraw or restoration.

Validate the exact original record, decoded operand, uniqueness of the physical
address/line and the complete route manifest. Other location operands and
off-route Events remain unsupported. No mandatory instruction may be silently
ignored to reach a menu. Optional Buy, Sell and Identify are explicitly outside
the selected menu, rather than successful no-ops.

### Added views and actor influence

All five added cells, four facings and eight cosmetic phases were composed and
rastered through the current indoor adapters without unsupported object
diagnostics. Visible object identities are `53:009.obj`, `81:011.obj` and
`115..120:004.obj`. Reuse the inherited town sky/ground/wall resources and
original object placement/animation policy; `011.obj` is the added appearance
dependency. No merchant actor is synthesized from a decorative object.

The influence check was rerun over **all sixteen cells and four facings**, for
both the original 46 actors and the exact reset-derived 52-slot collection.
It explored `(x,y,activated)` states with `XeenIndoorScene::classifyActors`,
`XeenActorApproach::move` and `xeenIndoorActorTerrain`, retaining other actors
for occupancy and sampling the full logical city geometry. Both cases yielded
26 reachable selected-Slime states and no additional actor activation: slot 35
in original state, slot 36 after reset. Actors were not constrained to the
player-cell set. Content-selected production validation covers wounded/defeated
selected states and continued reset/revisit; content-8-only cache authority cannot
authorize content 9.

M37's complete reset and retained-region rules remain in force: original 46
slots, exact reset targets, canonical gaps 46..49, script-created Slimes 50/51,
and the flag-9 predicate. The exit prelude's independently published flags and
record-764 disabling remain unchanged. Service entry/exit, cache reconstruction,
city revisit and restore never reset, replenish or respawn actors. Only the
actual original reset Event does so. Service return's reference `map.load` is
adapted through M37's existing retained-world policy, not a new reset trigger.

## Transaction choice, price and item contract

### Stock boundary and direct repair menu

Repair connects earned gold to existing equipment effects without generating,
inserting or removing items. Purchase would require original stock generation
and RNG; sale would require pricing/removal and would not itself give gold a use.
Paid identification is a details operation, not a durable identification bit.
All three remain excluded.

Original `BLACKSMITH_MAP_IDS` identifies Clouds map 28 as shop slot 0. Original
wares contain four categories, two sides, four shops and nine physical slots,
each with four item bytes. In initial `maze.pty`, the Clouds Vertigo record at
category c/slot s is at `28 + 144*c + 16*s` (four bytes, c=0..3, s=0..8);
all 36 are zero. These zeros are not a production offer list:
`SavesManager::newGame` calls `Party::resetBlacksmithWares`, which clears and
regenerates wares through `BlacksmithWares::regenerate` and `Character::makeItem`,
including other shops and RNG use. M38 does not infer stock from a player's save
or run that excluded global initialization.

The smith constructor and location lobby do not read or mutate wares. In the
reference, Browse opens `ITEMMODE_BUY`, copies wares into `_itemsCharacter`,
calls `setEquipmentIcons`, and can write that scratch inventory back even after
switching modes. That optional buy path is not harmless: icon normalization can
change raw metadata, and the accessory-ID-1 branch even assigns an ID. M38
**bypasses the unsupported buy scratch path** by offering Repair armor directly
inside the admitted original service. This is an explicit presentation/control
adaptation, not a claim that the original default Browse is stock-free.

The reached repair branch itself neither accesses nor modifies wares. Therefore
there is **no durable merchant field**, stock presence bit, initialization flag,
restock counter, sold-item retention or transaction ledger in M38. Closing,
revisiting, rebuilding resources and restoring have nothing to regenerate.
The service identity is immutable `(Clouds,28,Event record 0,action 1)`;
selection/quote state is transient. The temporal gate below excludes the one
mandatory path that would regenerate stock.

An ordinary inventory slot has no surviving merchant or monster-source identity.
A pending monster item still has its inherited source proof and is not a carried
repair candidate, even if all four bytes match an inventory item. Only carried
`monsterTreasure->gold` is spendable; pending, uncredited or forfeited monster
gold never supplies a shortfall. No lookup by description or byte equality may
convert pending treasure into an inventory selection.

### Exact repair rules

The selected quantity is exactly one occupied **Armor** physical slot, index
0..8, belonging to one current active roster owner. ID 0 is empty irrespective
of its other three bytes. Supported occupied records have ID 1..13 and material
0 or 38. All nine slots are inspectable, including items after holes; this
preserves M24's physical-slot contract rather than reproducing the reference
`items[0].empty()` assumption about compacted inventories.

`calcItemCost` selects repair divisor index 3, whose value is 10, regardless of
the passed skill value. `ELEMENTAL_DAMAGE[0]` is zero. Material 38 divides the
base armor value by four before the repair division. The precise prices are:

| Armor ID | Base cost | Material 0 repair | Material 38 repair |
| ---: | ---: | ---: | ---: |
| 1 | 20 | 2 | 1 |
| 2 | 100 | 10 | 2 |
| 3 | 200 | 20 | 5 |
| 4 | 400 | 40 | 10 |
| 5 | 600 | 60 | 15 |
| 6 | 1000 | 100 | 25 |
| 7 | 2000 | 200 | 50 |
| 8 | 100 | 10 | 2 |
| 9 | 60 | 6 | 1 |
| 10 | 40 | 4 | 1 |
| 11 | 250 | 25 | 6 |
| 12 | 200 | 20 | 5 |
| 13 | 100 | 10 | 2 |

Compute `max(1, B/10)` or `max(1, (B/4)/10)` with positive integer truncation
in that order. There are no zero-cost repairs, elemental/metal addends outside
these two materials, quantity discounts, merchant-skill, class, level, equipped,
curse or condition surcharges. Use validated table indexes and checked widened
unsigned intermediate arithmetic; compare/subtract against the full u32 carried
gold without a signed conversion. Price is at most 200 in this domain. A caller
cannot supply a price or substitute a catalog string for the item record.

Reference repair checks broken, offers confirmation and only then calls
`Party::subtract(CONS_GOLD,cost,WHERE_PARTY)`. M38 uses this order after authority
validation: supported owner/slot/category and nonempty supported record;
broken bit; quote; cancellation or confirmation; fresh authority/preimage check;
funds; candidate validation/publication. An intact item refuses before a quote
or funds test. An insufficient quote may be shown, but confirmation refuses
without payment or item change. Exact funds succeed and leave zero. Cancellation
before confirmation performs no repair. Repeating after success refuses because
the item is no longer broken; it cannot charge again.

The four bytes are `(material,id,state,frame)`. Success changes only
`state := state & 0x7f`. Material, ID, frame and the lower seven state bits are
unchanged. Cursed items may be repaired under the original rule, but remain
cursed; no counter/charge or curse is cleared. Do not confuse transaction
eligibility with equip/canAct/combat eligibility. Inactive roster owners cannot
be selected; current active owners may be selected even if individually unable
to act, provided the Journey itself is in its admitted healthy service state.

No transfer, insertion, removal, compaction, sorting, slot swapping or capacity
check occurs. A full inventory is repairable. Empty metadata, neighboring
records, other categories and all inactive owners remain byte-for-byte intact.
Two byte-identical records are separate quantities selected by owner/category/
physical slot. No global item ID is needed. Retained handles, aliases or copied
items never authorize mutation of another owner or slot.

HP, SP, conditions, spells, stats and all unrelated state are preserved. Existing
equipment rules derive the repaired contribution from the same frame and item;
no cached AC is patched. For canonical state-128 equipped armor, state 0 restores
`XeenCharacterRules::combatArmorClass` contribution. Existing M25 arrangement
rules and M33 combat admission still apply. Repairing an unequipped cursed or
counter-bearing record must not silently expand combat/equipment support; a
remaining unsupported contribution retains the inherited refusal. The useful
production witness has state 128 only and uses already supported consumers.

### Reachable production witness

The real `--journey-region --combat-seed 7` path starts from original resources.
Mainland combat earns 10 gold (800 to 810); original entry and entrance-Slime
combat naturally break Seymour's equipped Armor slots 0 and 1. Rebecca's
existing learned First Aid twice restores Seymour to HP 1, leaving her SP 19.
No gold, item, condition, camera or RNG field is overwritten after initialization.
The seed and injected frame-bound inputs are deterministic controls, not original
resource facts. `tests/XeenM38CliWitness.cpp` owns the reproducible input sequence.

After the five-cell corridor extension, service entry is day 8/minute 584,
gold 810, RNG draw count 317. Seymour (roster owner 6) has slot 0
`(0,1,128,3)` and slot 1 `(38,10,128,9)`. The first repair costs 2 gold,
produces `(0,1,0,3)`, restores 2 AC and leaves gold 808. Departure reaches
day 9/minute 584. A second legitimate repair costs 1 gold, produces
`(38,10,0,9)`, restores 1 AC and leaves gold 807; departure reaches day 10.
The next admission refuses. Neither visit consumes gameplay RNG.

Separate original-input branches validate zero-transaction departure and actual
city exit/reset, mainland return and revisit. Their extra movement/combat time
and RNG remain branch-specific. Complete fresh-process comparisons preserve
all durable owners before first input and after identical continuation.

## Time, departure and scheduling

The reference call chain runs under `MODE_SCRIPT_IN_PROGRESS` from
`Scripts::checkEvents`; the reached smith/repair dialog does not replace that
mode. The location constructor starts `_farewellTime=0`; Clouds smith farewell adds
no extra time. Normal Escape departure reloads the current map and calls
`Party::addTime(1440)`. Entry, browsing, owner/item selection, quotation,
confirmation, successful repair, ordinary refusal and inner-dialog cancellation
have no intrinsic time/ctr24 charge or actor opportunity. Cancelling the lobby
is departure and does owe the day, including a visit with zero transactions.

**The departure uses the bounded `addTime` semantics, not `changeTime(1440)`
or 1440 ordinary steps.**
`addTime` advances the date while retaining minute of day. On a changed day,
destination `day % 10 == 1`, or a charge strictly greater than 1440, invokes
stock reset and bank interest. It sets newDay; at minute >=300, script mode
skips resetTemps, Weak/rested handling and the rest message, then clears newDay.
There is no condition-tick replay, ctr24 advancement or gameplay RNG draw on
the admitted day-8/9 departure path. The existing regional daytime context
already has minute 300..1259, rested=false, newDay=false and zero unsupported
effects. M38 keeps these invariants and limits content 9 to year 610/day 8..10.

The bounded **smith departure candidate** uses the existing context: day+1,
all other context values unchanged, newDay=false. Its preflight
requires content 9, service origin, day 8 or 9, canonical inherited daytime
context, no unpaid prior continuation, and a destination without restock/bank
work. No new general calendar owner/API is needed. Run the same predicate before
service admission so an accepted visit can always pay its obligatory departure
under normal canonical state. At day 10 refuse before opening the service or
arming a charge; leave gold/items/date/RNG/actors unchanged.

The reference's Space is consumed by `Scripts::checkEvents` clearing interface
events, including `_buttonValue`. Do not append a normal Wait charge after the
service Event. Likewise do not create actor pulses for the skipped day.
After departure, reclassify the current retained region and complete inherited
view/contact/attachment settlement before the final world frame. Never move
the inactive mainland, replay missed temporal ticks or normalize actor state.
Any actually owed preexisting work precedes admission under M34-M37; a ready
treasure receipt, combat/attachment, deferred pulse, cast/item-use/Event or
unpresented mandatory result prevents opening. Dormant pending items that are
legitimately Quiet remain dormant, with their provenance unchanged. Repair
cannot wake, credit, forfeit or consume them.

Ordinary movement/combat/casting before and after a visit still uses inherited
condition ticks and support stops, including unsupported dawn/dusk and other
temporal boundaries. The special addTime path does not authorize ordinary
calendar crossing, bank interest, restocking, aging or rest. Integrity/canonical
failure after admission does not waive departure; it blocks further gameplay
and saving until truthful recovery, or terminates in the existing unsaveable
failure/support-stop state.

`BaseLocation::drawAnim` can use reference engine randomness while window 11 is
enabled. M38 uses a static original frame and the existing cosmetic presentation
domain; it does not reproduce animation calls against Journey combat RNG.
This deliberate presentation adaptation is not evidence that the reference
service performs no cosmetic random calls. No music, speech or full shop
animation is required.

## Owners, authority and publication

### Responsibilities and bounded interfaces

| Existing owner/path | M38 responsibility |
| --- | --- |
| `XeenPartyState`, roster/characters | Sole carried gold in `monsterTreasure->gold`; sole physical item arrays and existing context. No second wallet. |
| `XeenMonsterTreasure` | Preserve pendingGold, pendingMask, gems, source records and accounting. A merchant debit is not monster production/settlement. |
| `XeenRegionalRules`, `XeenJourneyContent`, `XeenVertigoRoute` | Explicit content-9 cell/service/time admission; exact record/manifest and actor-closure checks. Preserve content 8. |
| Event decoder/interpreter/publication | Typed bounded town-service request for opcode 0x11/action 1 at the exact admitted site, suspended terminal continuation and checked completion. No generic service registry. |
| `XeenEventFlow` | Own modal service continuation, selected owner/slot, quote/result phases and concrete frame/input authority in the existing UI flow. |
| `XeenEncounterFlow` / Journey coordinator | Own service admission, exclusive work, guard checks, detached repair/departure candidates, indivisible publications and post-exit settlement. |
| `XeenWorld` / session | Keep both actor regions, overlays, Journey context binding/RNG and runtime authority. No city/service world clone or durable merchant store. |
| `XeenRestoreGuard`, mutation observation | Cover all old/new authority and immutable service-resource boundaries; prepare known post-publication preimages. |
| Application / `XeenGameplay` / native SDL | Route typed actions and presented-frame acknowledgments through the single loop, exclusive modals and early F9 denial. |
| Save capture/format/restore | Encode schema 8/content 9 explicitly, validate content-specific state, prepare fresh unpublished owners and runtime capabilities. |

`XeenArmorRepair.h` supplies the pure repair/departure rule and candidate
helpers beside `XeenEquipment` and `XeenItemTransfer`. Inputs are a checked
operation/owner/physical slot and detached item/purse preimages; output is a fixed typed result, exact
price, expected before values and after values. It has no provider, RNG, live
mutation, UI, save or merchant owner. Public low-level calculation is not a
publication capability. Only the live coordinator can commit it.

The coordinator retains the fixed transaction result until acknowledgment:
operation generation, outcome enum, owner/category/slot, four before/after item
bytes, price and before/after carried gold. `XeenEventFlow` presents a read-only
view of that result and owns its UI phase, not a second payable transaction.
Refusals use the same bounded result channel with no success delta. All strings
and other fallible display work are prepared separately from publication.

The existing `XeenCombatBoundary` and Journey activity include **Service**
work within the same modal system. `XeenEventFlow` holds the service
continuation while the existing coordinator owns its exclusive authority.
Transfer Event -> Service and Service -> terminal Event/settlement/presentation
without releasing into Quiet. Acquire exclusive work before any new callback,
resource access, quote preparation or UI provider. Empty UI queues and a
released predecessor lease do not establish save eligibility.

The implementation retains the predecessor Event lease while acquiring Service,
then releases Service only after returning to the terminal Event continuation.
Final world presentation retains the inherited presentation lease. Service
catalog values and prepared art pixels are privately owned detached values;
providers never receive references to the retained service storage. The existing
world cache insertion handshake observes reconstructed map/MOB storage before
references escape. Schema selection uses the explicit supported-pair predicate;
content 9 selects schema 8 without changing legacy content selection.

Automated integration is registered as `xeen_smith_process`, backed by
`mmodern_m38_cli_witness` and the normal Application/SDL path. Test-only
branch/fault controls are absent from the ordinary executable.

### Validation order and commit units

1. Accept only a response bound to the current live session/owner identities,
   content, region/camera, operation and phase generations, and **concrete
   presented frame**. Reject foreign, copied, expired or already-consumed
   authority before provider calls. Consume the response once and fence
   reentrancy before invoking fallible work.
2. Check the retained full-owner/resource guard and admission conditions. A
   quote records active membership/order, roster owner, category, physical slot,
   all four item bytes, the complete touched armor array, carried gold, and
   current runtime revision/incarnation. No quote trusts the UI's price or name.
3. Apply the rule/refusal order above. Prepare detached candidate values,
   canonical party/equipment validation, exact price/purse feedback, fixed
   result storage and any necessary allocation/resource/render preflight.
   Confirmation revalidates the quote and complete guard; selecting a matching
   byte string elsewhere does not rescue stale authority.
4. Immediately before publication, recheck authority, all preimages and mutation
   observations after the final callback. Prepare the exact anticipated guard
   values in already allocated storage. In a callback-free, nonthrowing unit,
   subtract carried gold, clear the selected broken bit, publish the fixed
   success result and consume the operation. No observer can see payment without
   repair or repair without payment. There is no stock delta.
5. Rendering/logging/acknowledgment observes that fixed result. Failure or retry
   cannot re-run payment. On result acknowledgment, open a new browse phase with
   a new frame capability; the old confirmation is permanently spent.
6. Departure separately prepares and publishes its exact context candidate once,
   then completes terminal Event, active-region settlement and final presentation.
   Earlier successful repairs are already committed. Cancellation or failure
   here cannot refund them, restore broken bits or replay the service.

Before service admission completes, ordinary resource/allocation failure may
return a visible error to healthy exploration without a day charge. Admission
completes only after required service resources, exit feasibility, continuation
storage and initial presentation preparation pass their guards; it arms the
departure obligation. Failure after that point retains the service and its
obligation. An unpresented first service frame can be retried or safely departed,
using a newly presented recovery/error frame for any user response; old world
input cannot authorize that departure. It cannot be treated as a free
never-entered visit. Application termination
never writes a modal save or invents a deferred transaction.

Do not impose exchange-wide rollback: payment+repair is one commit, each later
repair is another, and departure date advancement is another. M37's independent
Event prelude/reset/transition units remain independent. Precommit failure
preserves the last committed unit, not a fabricated visit-start snapshot.

### Full preimages, mutation observation and failures

Keep all thirty characters and supplements, raw item bytes including empty
metadata, membership, purse and pending treasure, learned books, context, flags,
recovery, overlays, both regional actor collections, RNG, camera and coordination
authority in the existing complete guard domain. Inactive owners/regions are
not omitted because the current menu cannot display them.

Admit checked service art/text/route/catalog resource preimages before exposure;
keep the union with inherited map/object/Event/monster resources across cache
eviction and reconstruction. Raw or typed service-cache storage and nested
mutable values must be observed before a reference can escape to a provider.
Extend the existing insertion/reconstruction observation handshake where needed:
an immediate mutation then reversion in newly inserted storage must be detected,
including a callback that inserts a cache and changes it before returning.
Registering ranges only on the next guard check is insufficient. Retained equal
bytes do not clear a mutation violation, and a matching cache hit cannot replace
the admitted immutable preimage.

Guard adoption is private to checked callback-free publication and explicit
coordination transitions. Prepare the known future values in retained storage;
never renew from arbitrary callback-mutated live state. Failure is monotonic
through service retries, cache reconstruction, presentation and save attempts.
Every new provider boundary checks before and after, including admission art,
catalog/text, quote display, result display, exit geometry and final world frame.

| Failure class | Required effect and continuation |
| --- | --- |
| Ordinary refusal: empty/unsupported/intact item, insufficient gold, optional excluded choice, day-10 entry | No candidate payment/item/date/RNG mutation. Refusal returns to the appropriate current phase after its frame. A refused new admission incurs no day; an admitted visit still owes departure. |
| Ordinary I/O/allocation failure before a commit | Discard only the uncommitted candidate. Preserve previous successful repairs and pending departure. Retry preparation or cancel that quote; admission failure before admission may return to exploration. |
| Stale slot/owner/quote/frame or recursive response | No provider work or new publication from that response; retain current valid phase. A detected unauthorized owner mutation is the integrity case below, not a harmless stale quote. |
| Immutable mismatch, full-owner preimage/ABA violation, forged topology or canonical state | Latch integrity failure, reject publication, deny all new saves. Restoring old values or retrying with matching bytes never clears it. |
| Presentation failure after payment | Keep exact paid/repaired result, remain modal/unsaveable; retry only its presentation or proceed through a valid newly presented phase. Never charge again. |
| Departure preparation/mandatory settlement failure | Keep earlier repairs. Before time commit, keep the unpaid departure obligation; after time commit, retain paid date and resume only remaining settlement. No second day charge, reset replay or premature Quiet. Unsupported mandatory work is an explicit unsaveable support stop. |

## Native UI, input and save eligibility

Use the current native **320x200** scene, font, party strip, modal panels and
single SDL loop. The lobby uses `blck1.twn` frame 0, with the reference location
anchor `(8,8)` adapted to the existing viewport, and the resource-driven
Ironworks title. Validate clipping/frame bounds before admission. Reuse the
escape icon if shown. Do not require `blck2.twn`, `buy.icn`, audio or full shop
animation for this bounded presentation.

Required phases and controls:

| Phase | Visible information and accepted controls | Save |
| --- | --- | --- |
| Exploration before admission | Existing world/label; Space at the exact service site starts admission | F9 only if inherited Quiet checks pass |
| Admission/preparation | Exclusive Event/Service continuation, no world actions while providers run | Denied |
| Lobby | Ironworks title, selected active owner, carried gold, Repair armor, departure costs one day; Enter opens repair, F1..F6 owner, Escape departs | Denied |
| Armor browse | Nine numbered physical slots, names, broken/cursed/equipped status, selected owner and purse; F1..F6 owner, 1..9 item, Enter quotes selected item, Escape lobby | Denied |
| Quote/confirmation | Exact owner/item/slot, repair price, current gold and gold-after; Enter/Yes confirms, Escape/No cancels to browse | Denied |
| Result/refusal | Explicit outcome and unchanged or new purse/item state; acknowledgment returns to freshly presented browse/lobby, or exploration for refused admission | Denied |
| Exit preparation/time settlement/Event completion | Departure remains owned; no transaction or navigation responses | Denied |
| Final world presentation | Correct retained world at the service cell, settled date and purse | Denied until the actual required frame is presented and all inherited work settles |
| Returned exploration | No service continuation; same camera/facing, original movement/casting/inventory consumers | F9 if Quiet; manual Space is required for another visit |

The menu offers Armor Repair only; no action invokes Buy, Sell or Identify.
Long names/details must fit or use the existing bounded scrolling layout. Price, purse and selected item cannot
be obscured by a success/refusal overlay. Never infer a transaction from merely
displaying an item, selecting a member or acknowledging a result.
For insufficient funds, display the shortfall or an unavailable gold-after
value; never compute an unsigned underflow for the preview.

Every actionable response is tied to the current owner identities, operation
and phase generation and actual presented frame identity. A structurally equal
frame, prior visit's frame, stale owner/slot or direct response seam is not
authority. All direct/test response entry points enforce the same checks as
native input. Cosmetic redraw neither mints a new operation nor invalidates a
valid current phase unnecessarily; a required new phase needs its own real
presentation acknowledgment. Failed presentation cannot accidentally authorize
its unshown frame.

Fence held/repeated/batched keys at admission, quote, result, exit and world
handoffs. One input batch cannot select, confirm and acknowledge, nor can held
Enter repair several slots or held Escape depart and then move/Wait. Discard
stale batch responses, require the inherited release/fresh-input boundary, and
bind a new displayed-input capability after restore. SDL resize/expose and
cosmetic redraw do not execute gameplay or consume combat RNG.

F9 refusal in every non-Quiet phase must occur **before** capture, guard/provider
preflight, save-path preparation or file I/O. It cannot cancel a dialog, flush a
quote, acknowledge a result, advance departure, queue a deferred save or alter
an existing file. This includes reentrant provider F9, result retry, the empty
modal-queue interval and both sides of every lease handoff. Service state is
never serialized; quiet saving becomes possible only after truthful departure
and presentation completion.

## Persistence and legacy isolation

### Independent version decisions

Keep the **v4 envelope** and **Journey schema 8**. The base character/item fields,
purse/context fields and retained-region suffix already encode every durable
repair/departure result. There is no new durable merchant field or wire layout.
Use **content contract 9** because route admission, service behavior and the
bounded calendar continuation change. The successor to 8/8 is therefore
**8/9**, not an automatic 9/9.

The accepted v4 pairs are exactly 1/1 through 8/8 plus 8/9. An explicit
supported-pair mapping governs codec, capture and restore boundaries;
version checks are not globally relaxed.
Schema selects representation; content selects gameplay admission. Fresh
`--journey-region` selects 8/9. Loading 8/8 keeps its eleven-cell route and
unsupported service boundary, even in the new executable. No implicit upgrade,
missing-field injection, migration utility or normal unrestricted startup is
authorized.

### Exact successor wire

Envelope magic/header length/checksum, archive signatures, base roster/item
records, membership, flags and sorted overlays keep their exact accepted v4/v2
meanings. The Journey suffix is the following, with little-endian multi-byte
integers and no padding:

| Order | Encoding and bounds |
| --- | --- |
| Domain and pair | kind u8=3; schema u16=8; content u16=9; context-present u8=1 |
| Context | profile u8=0; difficulty u8=0; ctr24/day/year/minutes each u16; effects 9*u8; light/resistances 6*u16; rested/newDay canonical u8 booleans |
| Supplements | count u8=30; owners 0..29 in order: owner u8; Might permanent/temporary, Speed permanent/temporary, Accuracy permanent/temporary, temporary AC each i32; XP u32; Luck permanent/temporary each i32 |
| RNG | algorithm u8=1, nonzero state u32, draw count u64; exact saved continuation |
| Mainland header | initialized side u8=0, map u16=23, original count u16=19, runtime count u16=19 |
| Mainland actors | 19 ordered 19-byte actor records, inherited identity/status/accounting rules |
| Resistances | count u8=30; each owner u8, cold permanent/temporary and electrical permanent/temporary each u8 |
| Treasure/purse | carried gold u32, gems u32, pending mask u32, pending gold u32; weapon count u8, armor count u8; occupied pending weapons then armor, each source u8 and four item bytes M/ID/state/frame |
| Recovery | worldFlag16 canonical u8 boolean |
| Learned books | count u8=30; each owner u8 plus 39 canonical learned-flag bytes |
| Poison | count u8=30; each owner u8, permanent u8, temporary u8 |
| City presence | canonical u8 boolean; absence has no following city bytes |
| Present city | original count u16=46; runtime count u16=46 or 52; that many ordered 19-byte actor records |

Actor encoding remains side u8, root map u16, original/script slot u32, x/y
i16 each, HP i32, activated u8 boolean, lifecycle u8
(`Present=0, Disabled=1, Unresolved=2, Defeated=3`), status u8 and accounted
u8 boolean. City status is Physical=0; mainland keeps its inherited domain.
No actor statistics, stock record or UI identity is appended.

Pending counts, source order/uniqueness, dormant/ready meaning and masks retain
M33/M34's exact canonical rules; combined occupied pending count N is at most
12. Suffix length is exactly `3114+5*N` without city, `3992+5*N` with 46 city
slots, or `4106+5*N` with 52. The existing base prefix remains variable-length
under its own bounds. Reject wrong counts/order/IDs, truncation/trailing data,
bad booleans/enums, impossible presence, overflow, malformed length/checksum
and unsupported pairings, including 9/9.

### Canonical state, capture and restore

Content 9 retains all M37 canonical state and cross-owner constraints, with
these explicit changes: the sixteen-cell city camera domain; existing daytime
context plus year=610 and day in 8..10; a day greater than 8 requires a present
city collection because admitted departure is its only new day-changing path.
An absent city requires mainland camera and no city overlays; a city camera
requires its collection. Day 8 may contain original or reset-derived city state.
A 52-slot collection still requires the completed-reset protection overlay;
46 slots may carry it following a flag-9-true exit. Do not infer or set flag 9.
Validate selected actor closure under content 9 and original/reset fixed states
for noninfluencing slots. Preserve exact gap/script provenance and all dormant
treasure/accounting rules. No ledger is needed to validate a particular price
history; only canonical current owners and their authorized transitions matter.

Capture reads the existing sole owners only after exclusive Quiet acquisition
and full preimage/resource checks. Gold/item/date changes are already present
in existing fields. Capture, codec and restore must all use the explicit 8/9
pair, content-aware camera/time/actor validation and all thirty supplements/
books, including inactive owners. Journey content, rules, Event publication,
guards, regional flow and save paths use explicit capability decisions,
not broad unchecked `>=8` admission.

Restore prepares unpublished fresh party/world/camera/flags and checked original
resources, installs the exact saved values, validates both regions and active
view without moving/activating, preflights first presentation, rechecks full
preimages, then publishes once. Bind fresh Flow/Service/SDL incarnations and
require the new concrete frame before input. Restoring quietly at `(28,8,4)`
does not execute opcode 0x11. A saved day-9 repair remains paid and repaired;
the next legitimate visit may perform another repair and later advance to day
10. Restoring day 10 retains the temporal admission refusal.

Never call fresh initialization over saved state, recharge a price, regenerate
stock/items, replay service/arrival/reset scripts, reseed, recompute HP/SP,
replace saved supplements/books or advance time/RNG. Resource cache rebuild is
disposable reconstruction subject to retained immutable authority. UI phases,
selections, quotes, result continuations, departure obligations, leases, frame
capabilities and mutation epochs are runtime-only and absent from quiet saves.

Ordinary v1/v2, completed Diagnostic27 v3, Journey 1/1..7/7 and Journey 8/8
retain exact bytes, absence semantics, initialization, admission and subsequent
behavior. Older saves cannot gain the new route, repair, day jump or poison/
city fields by loading in a newer binary. Require both byte-level regression
and identical later action/refusal behavior. In particular an 8/8 game still
refuses the step west from M37's outside endpoint; recapture remains 8/8.

## Final acceptance

All four acceptance classes passed. These results certify only the bounded
contract above.

- **Automated deterministic validation:** full build and complete unfiltered
  CTest passed **104/104**; `git diff --check` passed. Coverage includes exact
  repair rules, payment/item atomicity, separate once-only departure,
  schema/content compatibility, native input/F9 and concrete-frame authority,
  failures/retries, complete-owner guards, immediate mutation/reversion (ABA)
  and reconstructed-cache authority.
- **Original-resource and process validation:** all five supplemental targets
  (`mmodern_regional_original`, `mmodern_consequence_original`,
  `mmodern_journey_original`, `mmodern_expedition_original`,
  `mmodern_regional_event_original`) passed. The genuine production witness
  used original initialization, earned gold, naturally broken supported armor
  and existing First Aid. `xeen_smith_process` completed **53 distinct terminated
  child processes**, comparing complete durable continuation before first input
  and after identical actions without replay. Original/reset corridor views,
  exact service Event/resource identity and actor influence were validated.
- **Independent implementation review:** **ACCEPT**, with no material or
  actionable minor findings. The reviewer inspected the complete implementation,
  verified baseline and file hashes, independently checked decisive pinned
  reference behavior, and reran the full build, 104/104 CTest, 53-process
  acceptance and all five original-resource targets. Review confirmed 8/9 versus
  8/8 isolation, payment/departure publication, Flow/frame/F9 authority,
  immediate ABA/cache safeguards and fresh restoration.
- **Maintainer physical native-SDL acceptance:** passed in an ordinary visible
  SDL window. The maintainer loaded the legitimate production checkpoint,
  entered through the actual service interaction, selected Seymour and broken
  armor, read price/purse feedback, canceled without payment, then confirmed
  the 2-gold repair (810 to 808). Intact-repeat and modal F9 refusals worked.
  Normal departure advanced day 8 to 9 with minute 584 retained. After quiet
  F9, complete process exit and fresh load, the second legitimate repair cost
  1 gold (808 to 807); departure reached day 10 and the next service attempt
  refused. Repaired items retained their expected inventory/equipment state.
  The maintainer reported no blocking gameplay/input issue. This is physical
  acceptance, distinct from automated input, process assertions or images.

## Explicit exclusions and remaining limitations

Exclude full Vertigo, other shops/services, unrestricted trading, merchant stock
generation/depletion/restock, sales, paid identification, weapon/accessory/misc
repair, broader item materials, general item use, temple/training/guild systems,
spell acquisition, unrelated quests, broad time/calendar/bank work, Darkside and
normal unrestricted Clouds startup. Preserve every inherited support boundary
not explicitly changed above.

The production witness uses prolonged legitimate combat and two existing heals
to obtain useful broken armor; it is deterministic and reachable, not a promise
that every player visits with a broken item. The original service's one-day exit
means content 9 admits at most two visits before restocking would be required.
Expanding that boundary needs a later specification/authorization. This is a
completed bounded M38 slice; its closure does not authorize successor work.
The next planning activity is the [focused post-M38 systems reassessment](roadmap.md#near-term),
not an already selected or authorized M39.
