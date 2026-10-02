# Milestone 43 - Bounded Vertigo Temple Heal and resurrection

## Completed scope and acceptance boundary

**M43 is completed and accepted.** Content 14 connects the existing Regional
Journey to the original Vertigo Temple: reach it with survivors, pay original
prices for selected living recovery or resurrection, depart, resume useful play
and retain exact consequences through process restart. The route adds 21 cells
for 49 total, two ordinary object appearances and a bounded two-day service
operation, without a new monster species, character owner or save schema.

The accepted boundary includes supported conditions, selected temporary reset,
carried-gold payment, one- or two-day departure, retained city state, M42 depleted
stock and Quiet save/restart. It excludes Uncurse, donations, Stoned/Eradicated,
new conditions, SP restoration, Guild, Sell, Rest/food, normal original startup,
general calendar/year rollover, new quests, other Temple sites and unrestricted
city travel. Presentation limitations are recorded under [final acceptance](#final-acceptance).

## Original behavior and provenance

Original commercial resources remain external and unmodified. Reference
interpretation uses the [pinned ScummVM revision](dependencies.md#pinned-scummvm-revision)
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`:

- [locations.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp):
  `BaseLocation::show` and `TempleLocation::createLocationText/doOptions`.
- [party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp):
  `Party::addTime`, complete stock replacement and bank interest.
- [character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp):
  current level, maximum HP, stat and condition ordering.
- [scripts.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/scripts.cpp),
  [locations.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.h)
  and [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp):
  terminal town dispatch, `TEMPLE = 4` and `tmpl` shape selection.

## Exact route, Events and resources

### Player domain and collision

Retain all 28 M41/M42 cells and add exactly `{(15,y): 8 <= y <= 28}`.
The total is **49 cells**, all four facings. The inherited sets remain
`(15,0..7)`, `(16,1..4)`, `(8..14,4)`, `(10..14,7)`, `(10,8..11)`.
All coordinates have logical root 28; north increases Y.

From `(15,4,North)`, 24 forward steps reach the service; each costs one
minute. The added corridor runs north from `(15,7)` for 21 steps and is passable
in both directions. Turns cost zero minutes with inherited scheduling/ctr24
behavior; Wait, backward movement and ordinary condition-time rules are unchanged.

The seam is `(15,15) <-> (15,16)`: physical tile 28 local `(15,15)` to tile 110
local `(15,0)`. Logical `(15,28)` samples tile 110 local `(15,12)`.
Preserve the complete reciprocal 28/109/110/111 topology in
[M37](milestone-37-plan.md#exact-production-route-and-admission-boundary), including
the inherited x=16 seam. Camera, Event, object and actor identities never become
110. Geometry/sight/actor movement must sample beyond admitted player cells.

Read-only added-cell walls in N/E/S/W order:

| Logical cells | Walls | Raw attributes |
| --- | --- | ---: |
| `(15,8..20)` | `0/0/0/0` | 1 |
| `(15,21)` | `4/8/0/8` | 17 |
| `(15,22)` | `0/8/4/8` | 10 |
| `(15,23)` | `2/0/0/0` | 10 |
| `(15,24..26)` | `2/0/2/0` | 10 |
| `(15,27)` | `0/0/2/0` | 10 |
| `(15,28)` | `10/8/0/8` | 10 |

Player collision checks the source wall `< 7` and refuses destination surface 4.
Do not infer reciprocal walls or substitute the actor `<= difficulty` predicate.
Walls 4/2 require no new door mechanic. The northern wall at the service does
not open. The original ceiling bit is set from y=22 through y=28; the street
and outside label use open sky. Boundary refusal precedes new movement, time
and actor work. New-cell Space must retain directional special-interaction and
Event selection semantics; there is no hidden gate, lock or quest prerequisite.

### Complete Event admission

The unchanged `maze0028.evt` contains 847 records, 7,298 decoded bytes,
CRC32 `28b6c20b`. Exactly two records lie on the added cells:

| Record / offset | Address | Exact operation |
| --- | --- | --- |
| 543 / 4499 | `(15,21,North,line 0)` | Length byte 6; opcode `0x02`, parameter `37`, resource label `aaze0028.txt[37]`; natural script end. |
| 6 / 44 | `(15,28,All,line 0)` | Length byte 6; opcode `0x11`, parameter `4`, original Temple dispatch; terminal town Event. |

Attribute 17 admits the label's automatic activation on the North-facing
approach, using the existing label path. Manual lookup uses the same direction.
Attribute 10 does not auto-enter Temple: Space dispatches from any of four
facings. `cmdDoTownEvent` terminates via `cmdExit`; there is no additional service
branch to resume. Departure retains `(28,15,28)` and its facing, classifies the
retained city, retires the Event and acquires the final presentation before Quiet.
It never immediately reopens Temple or replays entry on restore.

Validate the union with all inherited graphs, not just the two new records:
mainland records 136..139; Smith 0/539; Training 3/538; exit 760..769; reset
770..813; prelude 816..846. Their exact arguments, direction/line/order, flag-9
branch, instruction-764 protection overlay, No-path prelude and reset slot
semantics remain owned by [M37](milestone-37-plan.md#complete-exit-prelude-and-flag-9),
[M38](milestone-38-plan.md#exact-route-event-graph-and-city-consequences) and
[M41](milestone-41-plan.md#complete-relevant-event-graph). Reject duplicates,
mutated/truncated records and other `0x11` services. Neither admitting Temple
nor revisiting it grants the unearned discovery flag 9.

### Appearance and resource admission

Use original `aaze0028.txt[37]` as the title, original roster names,
`tmpl1.twn` frame 0 and inherited `esc.icn` frame 0. Author controls and numerical
feedback in English. This static presentation needs no other Temple animation
file, voice or gameplay RNG. Resource loading uses existing asset/cache owners.

The complete manifest adds these decoded identities:

| Resource | Bytes | CRC32 | Frame count / usage |
| --- | ---: | --- | --- |
| `tmpl1.twn` | 21187 | `b9ffe574` | 8; service uses frame 0 |
| `002.obj` | 6771 | `e75e3929` | 3; metadata-directed static views |
| `012.obj` | 24366 | `dd0c8515` | 4; metadata-directed static views |

Both new objects use relative-direction initial frames `0,1,2,1`, with no frame
advance. Flip flags are `0,0,0,1` for type 2 and `0,1,0,0` for type 12.
They need new immutable appearance admission, not a new object behavior.
The complete visible ordinary-object union over the 49 cells/four facings is:

| Resource | Original object record indexes |
| --- | --- |
| `002.obj` | 59,60,61,65,66,67,68,69,70,73 |
| `004.obj` | 115..122 |
| `006.obj` | 102 |
| `008.obj` | 56 |
| `009.obj` | 53,55 |
| `010.obj` | 27..42 |
| `011.obj` | 81,83,86 |
| `012.obj` | 87 |

Retain M41's geometry/MOB/text/character signatures and all inherited terrain,
sky, font, ordinary-animation and MON/ATT resources. The added appearances reuse the existing checked indoor adapters.
Require admission on fresh setup and restore, all four facings, both city forms,
dynamic actor views and cache reconstruction. A missing/corrupt dependency is
not replaced by silently hiding an object or removing an actor.

## Complete actor influence and retained city state

Both city forms have independently enumerated activation/movement closures
over all 49 cameras/four facings, with and without other-actor occupancy.
These conservative camera-choice sets include off-route movement and stutters;
they do not claim that one walk visits every state.

| City form | Actor | Spawn | Exact live closure |
| --- | ---: | --- | --- |
| Initial | 34, Slime type 0 | `(7,7)` | 4 states: dormant spawn and the M41 three-position activated set |
| Initial | 35, Slime type 0 | `(15,4)` | 103 states: dormant spawn plus 102 activated positions below |
| Reset | 35, Slime type 0 | `(7,7)` | Same four-state small closure |
| Reset | 36, Slime type 0 | `(15,4)` | Same 103-state entrance closure |

Entrance Slime activated positions:

| Y | X |
| ---: | --- |
| 0 | 15 |
| 1 | 9..16 |
| 2 | 13..16 |
| 3 | 14..16 |
| 4 | 8..16 |
| 5 | 9,10,11,14,15,16 |
| 6 | 13..16 |
| 7 | 9..16 |
| 8 | 10,14,15,16 |
| 9 | 10,12,14,15,16 |
| 10..12 | 10,11,12,14,15,16 |
| 13..20 | 14,15,16 |
| 21..28 | 15 |

All remaining materialized actors stay dormant at their original/form-specific
positions: initial slots 0..33 and 36..45; reset slots 0..34, 37..45 and 50/51.
Preserve all 46 initial slots and the reset 52-slot shape, including canonical
unmaterialized gaps 46..49. Activation is per actor, independent of another
actor consuming a visible selection slot. Other actors cannot become active
from these camera positions; the two mobile closures are disjoint. Do not erase
off-route actors or constrain their terrain queries to the player corridor.

The small Slime remains blocked at `(7,6)`, `(7,7)`, `(8,7)` by M41's original
wall-9 rule. It cannot contact, be wounded, be killed or award XP. The entrance
Slime may follow the new route through the seam and into Temple's cell. It
retains HP 1..2, original Slime attacks, ordinary activation/contact and the
existing accounted-dead sentinel. It gives 50 base XP and no gold/item reward.
Support departure reclassification into actual contact; the service itself
does not move actors. No new profile, projectile, indoor Shoot/Run or monster
effect is required. An occupied approach is resolved through existing combat,
not suppressed to make Temple reachable.

Content 14 has a distinct content/form closure entry, preserving legacy sets. Canonical restore must reject every live state outside the exact
sets and reject wounded/dead/accounted dormant or small actors. Preserve M37's
reset and flag-9 behavior: only the real exit reset rewrites named slots;
Temple entry, payment, departure, cache eviction, revisit and restore do not.
The inactive mainland receives no catch-up actor pulses.

## Selected recovery rules

### Eligibility and original price

Allow selection of any active roster member, including Asleep, Unconscious or
Dead. **Do not apply Training's `canAct` refusal to the selected recipient.**
Entry still requires the existing valid surviving-party boundary, exclusive
authority of the original dispatch Event with no competing combat/reward work,
the correct site and complete departure admission.
A defeated party cannot enter or be rescued through this service.

Keep the current active-character condition domain: Poisoned and Diseased
0..255, Asleep and Unconscious 0..1, Dead 0..255; every other condition is zero.
Preserve existing HP/sign and survivor validation. Stoned, Eradicated, curse and
other unsupported conditions are admission failures, not newly implemented
Temple options. Permanent/temporary levels, derived rules and numeric storage
retain their existing validated domains. No class, guild, XP or trainer-cap
restriction applies to Heal.

For the selected preimage, let `L = max(permanentLevel + temporaryLevel, 0)` and
`H = getMaxHP(preimage)`, using existing year-610 character rules. At Vertigo all
location surcharges `_v10.._v13` are zero. In the admitted domain:

```text
price = (currentHP < H ? 10*L : 0)
      + 10*L * count_nonzero(Poisoned, Diseased, Asleep, Unconscious)
      + (Dead != 0 ? 100*L + 50*Dead : 0)
```

Condition severity is charged only by the Dead term; Poison/Disease severity
does not multiply their 10*L charge. Simultaneous Dead and Unconscious are
additive, as is the low-HP charge. Use checked wide arithmetic, then the exact
representable carried-gold debit; the admitted inputs do not need saturating
prices or signed overflow. Show the full quote before confirmation.

After authority/state validation, refusal precedence is zero price (no paid
recovery), insufficient carried gold, then complete-successor/authority capacity.
Zero price is a no-op, including full-HP members with low SP or only temporary
bonuses; it must not clear temporaries or make the visit paid. A cancelled quote
and insufficient funds likewise change no durable state. Gems, bank and pending
treasure cannot fund Heal. Requote from current selected state after each paid
operation; a fresh confirmation is necessary for each payment.

### Exact selected mutation order

Prepare the following on detached copies and publish them atomically with payment:

1. Clear the selected member's temporary seven attributes, temporary level,
   temporary AC and six temporary resistances, exactly as the reference.
2. Set selected current HP to `getMaxHP` of that reset copy **while its original
   conditions are still present**.
3. Clear condition indexes 1..15 (`HeartBroken` through `Eradicated`); supported
   admission already requires unimplemented conditions to be zero. Preserve
   index 0 and every inventory byte; Heal is not Uncurse.

Step 2 precedes step 3 in `TempleLocation::doOptions`. Disease may lower the
HP assigned before its removal; the subsequently displayed healthy maximum may
therefore exceed current HP. Do not move the condition clear before HP derivation,
clamp to the final maximum or silently perform a second Heal. A later explicit
Heal can have a new nonzero quote. Dead suppresses condition attribute modifiers
under the inherited reference rules; use those rules without a resurrection
special-case maximum. The pure rule permits zero maximum HP, matching the
reference clamp. The Service coordinator refuses a zero-HP cleared character as `HpSupportLimit`
before debit because it violates Journey HP/condition admission; unrepresentable
HP is never published.

Only the selected owner resets. Existing modeled fields cover intellect,
personality, endurance and level on the character, and might, speed, accuracy,
luck, AC, cold/electrical/poison temporaries on its supplement. Fire/energy/magic
temporaries are omitted from settled schema 9 and proven zero by the inherited
immutable `maze.chr` check at offsets 312/320/322 for all thirty records.
Retain that proof and zero-only omitted-input policy; do not invent fields,
backfill bytes or discard newly nonzero inputs. No supported producer changes them.

Preserve SP exactly, even if temporary reset lowers its displayed maximum;
preserve temporary age, permanent attributes/resistances/level, XP, skills,
raw spell-book bytes, inventory/equipment state, membership and all other owners.
No aging, XP penalty, level loss, curse removal, condition-time tick, party buff,
SP refill or resurrection RNG is added. The paid visit marker is set once;
further paid recoveries do not add further departure days.

## Service time, stock, bank and reservation

Reuse [M40's exact service operation](milestone-40-plan.md#exact-reached-reference-call),
[complete stock/RNG contract](milestone-40-plan.md#complete-stock-algorithm-and-tables)
and [M42 current-stock admission](milestone-42-plan.md#complete-generation-and-purchase-depleted-stock).
A visit with any successful Heal owes **one `addTime(2880)` on departure**;
an unpaid visit owes one `addTime(1440)`. Individual Heal publication costs no
time; additional paid recoveries share the same departure.

The supported context remains year 610, quiet days 8..99, daytime minutes
300..1259 and ctr24 < 24, with zero unsupported effects/light/resistance counters.
Departure preserves minute, ctr24 and year, leaving `rested=false` and
`newDay=false`. It runs no ordinary rest, condition ticks, attribute reset,
HP/SP changes or actor catch-up.

| Starting day | Unpaid departure | Paid departure |
| --- | --- | --- |
| 8 | 9, no restock/interest | 10, one complete restock/interest |
| 9 | 10, none | 11, one |
| 10 | 11, one | 12, one |
| 11 | 12, none | 13, one |
| 19 / 20 | 20 none / 21 one | 21 one / 22 one |
| 97 | 98, none | 99, one |
| 98 | 99, none | Refuse Heal before payment; retain one-day exit |
| 99 | Refuse entry before debt/mutation | Refuse entry |

Every paid departure from days 8..97 regenerates once because the charge is
greater than 1440 and changes the day. It is not two one-day calls: 8->9->10
would not regenerate. There is no extra intermediate-date trigger or year-611
admission. Training's member-days and separate departure retain individual
1440-minute calls. Day-99 daytime play and Quiet capture remain supported.

Stock replacement covers all eight shops, including discarded insertions and
rejection draws. Bank interest follows, gold then gems, as
`u32(uint64(balance) + floor(balance/100))`. A triggering successor contains
complete generated stock; a nontriggering successor preserves exact depleted
bytes/order, bank and RNG. No ledger, stock merge or regeneration on restore
is introduced.

`XeenServiceDayCandidate` distinguishes only admitted one-day and Temple-paid
charges; legacy callers retain one day. Entry completes the one-day departure,
resource/UI preflight and mandatory settlement authority before admission.
The first Heal prepares a replacement two-day candidate from the same entry
context/economy/RNG, never from the one-day successor. If that reservation already
regenerates, its stock, interest and RNG result are reused with the checked
two-day context. Otherwise the replacement is generated privately in at most
64 raw RNG attempts per idle slice while the complete one-day obligation remains.

Selected cure/payment and the complete replacement publish together. Later
Heals reuse it; departure publishes it once. Escape during an unpublished
upgrade cancels that optional Heal and retains the one-day exit. Deterministic
date/RNG-count/storage exhaustion refuses before payment; optional failure
cannot cancel mandatory departure. Only stock-operation overflow maps to
`SupportLimit`; guard/boundary faults retain their guarded retry behavior.

## Ownership, publication, input and persistence

[XeenTempleHeal](../src/games/xeen/XeenTempleHeal.cpp) computes detached quote
and recovery values. Existing roster/supplement, purse, economy/context, world
RNG/actors, camera and flags remain the sole authoritative owners.
`XeenEncounterFlow` reuses its Smith Service continuation for Temple; `XeenEventFlow`
owns UI and typed responses. The existing Event/Service/Presentation leases and
idle loop remain in use, without a second scheduler or generic transaction owner.

Quotes bind selected roster identity, full preimages, site/content, price, purse,
context/resources and reservation/operation identity. Preparation allocates and
validates all storage, candidates, fixed results and expected guards before two
callback-free, nonthrowing publication units:

- Heal publishes selected character/supplement changes, carried-gold debit,
  fixed result, paid/operation markers, replacement reservation and checked
  coordination revisions together.
- Departure publishes context, all wares/bank, world RNG, departed marker and
  expected guard once; arrival/Event/final-presentation settlement follows.

Post-publication faults retry only feedback or unfinished settlement. The final
BeforeHeal callback is followed by another full guard check. Observation covers
all thirty owners/books/items, membership, treasure, economy, context/RNG, both
actor regions, flags/overlays, camera and resources, including mutation followed
by reversion and newly reconstructed cache storage.

[M42's authority/input contract](milestone-42-plan.md#ownership-atomic-publication-and-departure)
remains in force: independent checked u64 counters; mandatory suffix headroom
16 Encounter/Journey, 8 Event input and 4 boundary revisions, with admission
20/10/5. Optional operations retain their own increments and operation/reservation
room as well as the suffix. Temple confirmation reserves both Upgrade and Result
UI revisions before payment; idle publication rechecks Result room. Unchanged
departure retries, private slices and cosmetic frames consume no semantic
generations. Exhaustion retains an actionable departure path without wrapping.

Initial selection is F1. F1-F6 selects the recipient; Enter quotes, a fresh Enter
confirms and Enter acknowledges the fixed result. Escape cancels a quote or
unpublished upgrade; menu Escape departs. Quotes show projected assigned HP and
final healthy maximum, preserved SP, price/gold and the owed one/two-day charge.
Refused results show unchanged actual HP/maximum/SP/gold through presentation-only
normalization. Original no-charge/insufficient-gold refusals and support limits
remain distinct. Failed departure exposes its retry action.

Concrete/semantic frame authority is consumed before providers/hooks. Existing
make-before-break cosmetic handoff, first-edge responsiveness and origin-bound
queued input prevent held, repeated, batched, stale or reentrant input from
crossing phases or authorizing unseen frames. F9 refuses before providers,
save-path preparation or I/O throughout admission, modal work, private generation,
debt, failure/retry and incomplete handoff. Only final acquired Quiet authority
permits capture. Shutdown preserves the prior save; integrity failure permanently
closes publication/save authority.

Fresh Regional Journey uses **envelope 4 / schema 9 / content 14**. The unchanged
[M42 schema-9 wire](milestone-42-plan.md#persistence-and-compatibility) already
stores every changed settled value. The `templeRecovery()` capability gates the
new corridor, exact Event operands/addresses and 32-by-32 object selection;
legacy contents retain their prior Event camera/object-selection bounds.
Logical city geometry still samples the existing 32-by-32 region under root 28.

All supported former pairs retain their meanings without implicit upgrade:
9/13 does not gain Temple, the new cells, expanded Slime closure or two-day calls.
Day-8 Quiet requires complete stock; later contents 13/14 admit the inherited
deletion relation. Restore validates the complete resource/state union before
publishing exact owners, including overmaximum HP/SP and raw learned books.
It never heals, resets temporaries, normalizes HP/SP, replays payment/Event/reset,
regenerates stock, reseeds, applies interest or advances time. Service UI/debt
remains transient and unsaveable.

## Production acceptance boundary

The connected witness starts a fresh content-14 Regional Journey with seed
**3626689381**, inherited prepared-party initialization and actual gameplay
injury/death. It does not substitute HP, conditions, money or RNG. The Orc earns
10 gold; blocking the entrance Slime until the sixth member is Dead establishes
the recovery preimage. The executable action sequence and exact comparisons live
in [XeenM43CliWitness](../tests/XeenM43CliWitness.cpp).

| Boundary | Durable witness values |
| --- | --- |
| Temple arrival / Quiet A | `(28,15,28,North)`, day 8/minute 604, ctr24 0, gold 810; RNG `2923728173:1239`. Owner 6: HP -15/SP 27, Unconscious 1/Dead 1; owner 1: HP 0/SP 21, Unconscious 1. |
| First Heal | F6/owner 6 costs **410** (30 low HP + 30 Unconscious + 300 level + 50 Dead); HP 15/SP 27, conditions clear, gold 400. |
| Second Heal | F5/owner 1 costs **60**; HP 21/SP 21, conditions clear, gold **340**. All six can act; a new F6 quote is zero/no-op. |
| Single paid departure / Quiet B | Day **10**/minute **604**, ctr24 0, gold **340**, bank 0/0; RNG **2583579601:2144**; all 1152 stock bytes match the reference, CRC32 **37e8cc62**. |
| Unpaid branch from A | Day 9/minute 604; original stock, bank, RNG, gold and recovery preimage retained. |

The witness saves B, exits completely and restores fresh owners before first
input. Uninterrupted/restored branches return south, execute original reset and
re-enter; the resurrected owner uses learned Magic Arrow in reset-Slime combat.
Return Heal/visit checkpoint E is restarted in another process and continued
through F, with exact encoded bytes and owner comparisons. See
[XeenM43ProcessTests](../tests/XeenM43ProcessTests.cpp).

A distinct fresh content-14 seed-7 production Buy witness preserves genuine
M42 depletion, then restarts into paid Temple replacement, unpaid nontriggering
departure and Training's separate member-day/departure branches. It compares all
stock bytes and bounded/raw/rejection draw traces against the pinned-reference
oracle; it never relabels a legacy save. See
[XeenM43DepletedProcessTests](../tests/XeenM43DepletedProcessTests.cpp).

Rule/Flow fixtures cover date and condition extremes, zero HP, Disease ordering,
SP above the recomputed maximum, untouched owners/equipment, omitted-input proof,
insufficient funds, stock/bank arithmetic, reservation reuse, count exhaustion,
faults/ABA, finite authority, native frame input and refused-result truthfulness.
These synthetic cases do not establish earned production progress. Original-data
coverage independently enumerates route facings, both seams/forms, the exact
4/103 actor closures, dormant identities, resources and cache reconstruction;
compatibility tests preserve every legacy pair and reject crossed/unknown pairs.
See [rules](../tests/XeenTempleRulesTests.cpp),
[Flow](../tests/XeenTempleFlowTests.cpp),
[original resources](../tests/XeenTempleOriginalTests.cpp) and
[save-format coverage](../tests/XeenSaveFormatTests.cpp).

## Final acceptance

- **Automated validation:** the implementation agent completed the normal build
  and final full, unfiltered CTest run: **141/141 passed** after the last
  correction. Whitespace validation passed. SDL dummy/software process witnesses
  and raster checks establish automated state/presentation evidence.
- **Independent review:** the independent reviewer returned final **PASS**, with
  no remaining BLOCKER, MAJOR or MINOR findings after re-verification of finite
  authority, quote/result presentation and process-continuation corrections.
- **Maintainer physical acceptance:** native-SDL acceptance **passed**, separately
  from automated evidence and independent review. It covered the connected fresh
  seed-3626689381 witness, new route, 410/60 quotes, final gold 340, one two-day
  departure to day 10/minute 604, Quiet F9, complete process exit and restart.
- **Accepted presentation limitation:** static `tmpl1.twn` frame 0 is loaded but
  fully obscured by the opaque Temple text panel in
  [XeenSmithFlow](../src/app/XeenSmithFlow.cpp). The maintainer deferred its
  visibility to broader [art/animation work](roadmap.md#deferred-presentation-work).
  This is not an open acceptance defect or M43 prerequisite. Audio and animated
  service characters are likewise outside the accepted boundary.

[Project status](project-status.md) owns current capabilities;
[project history](project-history.md) owns chronology. Closure reaches the
[roadmap broad-review boundary](roadmap.md#replanning-and-review-cadence) without
selecting or authorizing M44.
