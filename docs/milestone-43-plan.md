# Milestone 43 proposal - Bounded Vertigo Temple Heal and resurrection

## Decision and approval boundary

**PROPOSED FOR APPROVAL; implementation is not authorized.** Recommend proceeding
with one independently acceptable M43: reach the original Temple with survivors,
pay for selected supported recovery, depart by the original service-time operation,
resume useful combat and preserve exact continuation through a process restart.
The investigation found no reason to split recovery from resurrection or to reopen
the broader roadmap. The required route adds 21 player cells, no monster species,
two ordinary object appearances and a bounded two-day service operation.

The inspected MMModern baseline is
`b3b3679f42b45fa18e9683f2f3f54f6107c545dd`. Before investigation, branch `main`,
HEAD, `origin/main` and direct remote `refs/heads/main` all matched that SHA;
the working tree and index were clean. M42 remains the stable accepted state.
This document neither approves itself nor records M43 completion. Implementation,
independent review, maintainer physical acceptance and Git publication are separate
gates. The [roadmap review cadence](roadmap.md#replanning-and-review-cadence)
still applies after the first accepted recovery boundary.

Included: the route and label below, selected Heal of supported living injuries/
conditions and Dead, selected temporary reset, original prices and carried-gold
payment, one- or two-day departure, retained city state, M42 depleted stock,
Quiet capture and exact restart. Excluded: Uncurse, donations, Stoned/Eradicated,
new conditions, SP restoration, Guild, Sell, Rest/food, ordinary startup, general
calendar/year rollover, new quests, other Temple sites and unrestricted city travel.
Static Temple artwork follows the accepted Smith/Training presentation adaptation;
audio and animated service characters are not prerequisites.

## Evidence and source boundaries

Reference interpretation uses only ScummVM
[`6814ee9ba54582f5b5adcffab49efbbd8f589edd`](https://github.com/scummvm/scummvm/tree/6814ee9ba54582f5b5adcffab49efbbd8f589edd),
verified locally with an empty dependency worktree status. See
[dependencies](dependencies.md#pinned-scummvm-revision). Relevant reference sources:

- [locations.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp):
  `BaseLocation::show`, `TempleLocation::createLocationText/doOptions`.
- [party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp):
  `Party::addTime`, stock replacement and interest; [character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp):
  `getCurrentLevel/getMaxHP/getStat/conditionMod`.
- [scripts.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/scripts.cpp):
  `cmdDoTownEvent/cmdExit`; [locations.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.h):
  `TEMPLE = 4`; [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp):
  `TOWN_ACTION_SHAPES`, selecting `tmpl`.

Implemented ownership/admission was inspected in
[content policy](../src/games/xeen/XeenJourneyContent.h),
[route validation](../src/games/xeen/XeenVertigoRoute.cpp),
[city canonical state](../src/games/xeen/XeenVertigoWorld.cpp),
[indoor classification](../src/games/xeen/XeenIndoorScene.cpp),
[regional rules](../src/games/xeen/XeenRegionalRules.cpp),
[character rules](../src/games/xeen/XeenCharacterRules.cpp),
[Training rules](../src/games/xeen/XeenTraining.cpp),
[Training coordination](../src/app/XeenTrainingFlow.cpp),
[service candidates](../src/games/xeen/XeenServiceDay.cpp) and
[economy validation](../src/games/xeen/XeenServiceEconomy.cpp).
Relevant existing tests include
[original route closure](../tests/XeenTrainingOriginalTests.cpp),
[Training flow](../tests/XeenTrainingFlowTests.cpp),
[service initialization](../tests/XeenServiceDayInitializationTests.cpp),
[service process continuation](../tests/XeenServiceDayProcessTests.cpp) and
[purchase process continuation](../tests/XeenPurchaseProcessTests.cpp).

Investigation evidence is deliberately separated:

| Evidence | What was established |
| --- | --- |
| Code/tests and closed plans | Existing interfaces, owner boundaries, accepted legacy behavior and reproducible input prefixes; not a new full test-suite run. |
| External original resources, read-only | Geometry, Event addresses/records, object union, resource identities and initial/reset actors below. No commercial resource was changed or copied into the repository. |
| Automated investigation probes | Shortest-path search; all 49 cameras in four facings; actor fixed points with/without other-actor occupancy; 1,568 in-memory route rasters over eight cosmetic phases, without unsupported object diagnostics. Temporary probes linked the existing local build; a fresh implementation build remains required. |
| Automated native production prefix | Normal content-13 initialization and SDL dummy/software execution, driven through presented-frame gameplay input, obtained the real injury/death and reached the existing route frontier. No owner-state injection supplied these results. |
| Detached calculation | Complete stock generation from the observed prefix RNG, and reference-derived future Heal/date results. These are expected M43 results, not executed M43 gameplay. |
| Physical observation/review | No DOS observation, maintainer physical acceptance or independent review of M43 has occurred. |

## Exact route, Events and resources

### Player domain and collision

Retain all 28 M41/M42 cells and add exactly `{(15,y): 8 <= y <= 28}`.
The total is **49 cells**, all four facings. The inherited sets remain
`(15,0..7)`, `(16,1..4)`, `(8..14,4)`, `(10..14,7)`, `(10,8..11)`.
All coordinates have logical root 28; north increases Y.

A multi-source shortest-path search using the actual directional player collision
predicate found 21 steps from the admitted domain: `(15,7)` straight north to
`(15,28)`. This also attains the Manhattan lower bound from that domain. Every
edge on this selected path is passable in both directions. From `(15,4,North)`,
24 forward steps reach the service; retrace with 24 backward steps or turn and
walk south. Each step costs one minute; turns cost zero minutes but retain the
existing scheduling/ctr24 behavior. Wait, backward movement and ordinary
condition-time boundaries retain their existing semantics.

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

Add these decoded identities to the inherited complete manifest:

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
sky, font, ordinary-animation and MON/ATT resources. The new views composed and
rastered through current adapters without an unsupported object diagnostic.
Require admission on fresh setup and restore, all four facings, both city forms,
dynamic actor views and cache reconstruction. A missing/corrupt dependency is
not replaced by silently hiding an object or removing an actor.

## Complete actor influence and retained city state

Certification enumerated `(x,y,activated)` fixed points against every camera in
the full 49-cell domain and every facing, including activation without visible
pixels, stutters and off-route movement. It used the existing original wall/
classification and movement predicates. Repeating with actual dormant-actor
occupancy and with other actors removed produced identical sets. This is a
conservative camera-choice closure, not a claim that one walk visits every state.

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

Implement a distinct content/form closure entry for M43, preserving legacy
closure sets. Canonical restore must reject every live state outside the exact
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
special-case maximum. Reject an unrepresentable prepared HP before debit.

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

### Complete supported successor

Reuse [M40's exact script-service operation](milestone-40-plan.md#exact-reached-reference-call),
[generation/RNG contract](milestone-40-plan.md#complete-stock-algorithm-and-tables)
and [M42's current-stock admission](milestone-42-plan.md#complete-generation-and-purchase-depleted-stock).
The only new operation is **one `addTime(2880)` on departure after any successful
Heal in this visit**. A visit with only refusals/cancellations/browsing owes one
`addTime(1440)`. Nothing is paid in time at the individual Heal publication.

Retain year 610, quiet days 8..99, daytime minutes 300..1259, ctr24 < 24 and the
same zero unsupported effects/light/resistance counters. Script departure leaves
minute, ctr24, year, `rested=false` and final `newDay=false` unchanged. It performs
no ordinary rest, Weak/Dead/Disease progression, attribute reset, HP/SP change or
actor pulse. Selected reset belongs only to the paid Heal operation.

| Starting day | Refusal-only exit | Paid exit |
| --- | --- | --- |
| 8 | 9, no restock/interest | 10, one complete restock/interest |
| 9 | 10, none | 11, one |
| 10 | 11, one | 12, one |
| 11 | 12, none | 13, one |
| 19 / 20 | 20 none / 21 one | 21 one / 22 one |
| 97 | 98, none | 99, one |
| 98 | 99, none | Heal support refusal before payment; retain one-day departure |
| 99 | Refuse admission before debt/mutation | Refuse admission |

All paid entries 8..97 end within 10..99 and regenerate once because the original
charge is greater than 1440 and the day changes. Do not apply an extra trigger
at an intermediate date. No wrap into year 611 is admitted. Day 99 still allows
inherited daytime play and Quiet saving. Two one-day calls 8->9->10 do not
regenerate; replacing a single paid 8->10 by that sequence is incorrect.
Training's first distinct-member days and separate departure remain individual
1440 calls, including when two such calls reach the same final date as Temple.

Restock replaces all eight shops through the existing complete generator,
including discarded insertions/draws; interest follows, gold then gems with
`u32(uint64(balance) + floor(balance/100))`. Current purchase-depleted stock is a
valid preimage in M43. A triggering successor has strictly complete generated
stock; a nontriggering successor preserves exact depleted bytes, order, bank
and RNG. No stock merge, repurchase ledger or regeneration on restore is added.

### Reservation replacement without extra operations

Extend the existing `XeenServiceDayCandidate` narrowly to distinguish the
admitted 1440/2880 script charge; its default and every legacy caller remain
one day. The content/site coordinator grants the two-day capability only to
M43 Temple. Do not introduce arbitrary-duration/calendar admission or a second
stock generator/service scheduler.

1. Before entry becomes irreversible, complete and retain the ordinary one-day
   departure candidate, resource/UI preflight and mandatory settlement authority.
   Pre-admission failure leaves no charge, cure, debt or live RNG prefix.
2. Before the first successful Heal, prepare a complete **replacement two-day
   candidate from the same live entry context/economy/RNG**, alongside the selected
   cure/payment result. It is not a successor of the one-day candidate.
3. If the retained one-day reservation already regenerates, reuse its complete
   stock, interest and RNG result with a checked new two-day context/charge.
   Both reference operations would generate once from identical inputs. Do not
   redraw, reapply interest or spend an extra reservation as a day.
4. Otherwise prepare the new generation privately in at most 64 raw RNG attempts
   per idle slice. Keep the old complete one-day obligation until atomic Heal
   publication swaps it for the new complete two-day obligation.
5. Further Heals retain the same complete two-day obligation. No additional time,
   interest, stock generation or RNG work is needed for them.
6. Departure publishes the retained complete context/economy/world-RNG successor
   once, then finishes the existing guarded arrival/Event/presentation settlement.

A failed optional preparation can retain its checked private prefix or cancel
that unpublished Heal. Earlier paid recoveries and the current complete
obligation survive. Rejection draws are retained, not rerolled. Deterministic
date/RNG-count/storage/counter exhaustion is a support refusal, not an infinite
retry. In particular day-98 entry can remain refusal-only but cannot collect a
Heal payment. A date check or guessed fixed RNG margin never substitutes for a
complete successor. A mandatory departure cannot be cancelled into a free return.

## Ownership, publication, input and persistence

Keep roster/character supplements, carried purse, party economy/context, world
RNG/actors, camera and flags as their sole existing owners. A bounded Temple
continuation belongs to `XeenEncounterFlow`; `XeenEventFlow` owns presentation and
typed response routing. Reuse the Event/Service/Presentation leases and idle loop.
Detached recovery/candidate values have no publication authority. A narrow pure
recovery rule module is appropriate, analogous to Training, without a generic
transaction framework, alternative character owner or parallel scheduler.

The transient continuation needs selected roster identity, quote/preimages,
visit/operation/input/concrete-frame authority, current complete departure,
optional replacement/cure, fixed result, paid-visit and exactly-once operation/
departure markers. It needs no Training member-day bits. Quotes bind full
preimages, site/content, actual recipient, price, purse, context, resources and
reservation identity, not merely the selected row's name or displayed amount.

Use two callback-free, nonthrowing publication units:

- Heal: selected character/supplement changes, carried-gold debit, fixed result,
  paid/operation markers, replacement departure reservation, expected guard and
  checked coordination revisions together.
- Departure: full context, all wares/bank and world RNG plus departed marker and
  expected guard together; subsequent settlement cannot repeat this unit.

Allocate/validate every candidate, result, owned storage, expected delta and
guard before either publication. No provider, observer callback, renderer or
throwing work may run inside it. Post-publication faults retry only feedback or
remaining settlement; there is no refund/reheal/redraw/day replay. Retain the
full mutation-history and resource chain when renewing guards, including
mutation->reversion and storage insertion/cache reconstruction before references
escape. Observe all thirty characters/supplements/books/items, active membership,
purse/pending treasure, economy, context/RNG, both actor regions, flags/overlays,
camera and resources even though only one owner is healed.

Inherit [M42's authority and native-input contract](milestone-42-plan.md#ownership-atomic-publication-and-departure):
independent checked u64 counters; mandatory suffix headroom 16 Encounter/Journey,
8 Event input, 4 boundary revisions; admission headroom 20/10/5. Optional UI and
Heal transitions must retain that suffix plus their own checked increments and
operation/reservation identity room. Prove actual increment sites and threshold
tests against these budgets; retries/private slices/cosmetics do not spend
unchanged semantic generations. When optional room runs out, retain an actionable
departure-only path. Never wrap a counter or rebase from callback-mutated state.

Initial selection is F1. F1..F6 selects the active recipient, Enter opens the
Heal quote, a fresh Enter confirms, Escape cancels to the menu; Enter acknowledges
the fixed result, and menu Escape departs. Label the selected member's conditions,
HP/current and final derived maximum, preserved SP, gold/price and whether one
or two days are owed. Explicitly distinguish original refusals from a support
limit. Show the Disease ordering result accurately. Failed settlement exposes
only its inherited retry action. Do not add donation/Uncurse controls.

Consume concrete/semantic frame authority before providers/hooks. Keep the
accepted make-before-break cosmetic handoff, first-edge responsiveness and
origin-bound queued input. Held/repeated/batched/stale/reentrant input cannot
cross phases or authorize unseen frames. No nested SDL loop is introduced.

F9 must fail before capture, resource providers, save-path preparation or I/O
through exclusive entry preparation, every modal phase, private generation,
debt, failure/retry and unfinished handoff. Quiet becomes available only after
the final valid exploration/combat presentation and required settlement. Fatal
window/render shutdown preserves the earlier disk save; integrity violations
permanently close publication/save authority without a free departure.

Choose **envelope 4 / schema 9 / content 14** for fresh M43 Journey. Schema 9
already holds all changed settled values; no schema increment, field omission,
representation widening or new retained region is justified. Add explicit 9/14
admission throughout setup, runtime, candidates, capture, codec and restore.
Content 14 includes M42 Buy/deletion closure, Training, repair and inherited
casting/recovery, plus this route/actor/resource/Temple capability. Preserve
all former supported pairs and their exact meanings; 9/13 does not gain Temple,
the new cells, the larger Slime closure or two-day calls. No implicit upgrade.

Preserve [M42's exact schema-9 wire](milestone-42-plan.md#persistence-and-compatibility),
including economy extents/order and all raw book bytes. Day-8 Quiet still requires
complete stock; later stock admits the inherited deletion relation for contents
13/14 only. Restore validates the complete candidate and resource union before
publishing exact saved owners. It does not heal, reset temporaries, normalize
current HP/SP, resurrect, replay payment/Event/reset, regenerate stock, reseed,
apply interest or advance time. Service UI/debt stays transient and unsaveable.

## Reproducible production witness and acceptance

### Real injury/death prefix and proposed paid continuation

Use fresh M43 Regional Journey with `--combat-seed 3626689381`. Initial stock
generation and prepared party are the inherited production initialization,
not original level-1 startup. The prefix below was executed during investigation
under unchanged content 13 using the real native application/SDL loop and
automatic fresh-frame inputs. It changes gameplay state only through production
actions. M43 must reproduce it under content 14.

Notation is inherited from [M40's witness](milestone-40-plan.md#deterministic-accepted-route):
`U/D` forward/backward, `L/R` turns, `F` physical Shoot; settle mandatory work
between inputs. Attack in ordinary encounters and acknowledge results.

1. Follow `UFUDD`, then `LLULUU`, Space, Yes; settle the city entrance.
   The original Orc encounter earns 10 gold, producing carried gold 810.
2. Follow `URULUUULUUU`. In the entrance Slime fight, Block every ready member
   until Seymour (F6/roster 6) has Dead, then Attack to settlement. This prefix
   required **44 Blocks**, not M40's earlier 39-Block armor-break stop.
3. At `(28,13,4,West)`, day 8/minute **578**, ctr24 **21**, gold **810**,
   RNG algorithm 1/state **2923728173**/count **1239**, the observed active state is:

| Owner | Current HP | Current SP | Current maximum HP | Conditions |
| --- | ---: | ---: | ---: | --- |
| Arturius 0 | 14 | 6 | 36 | Good |
| Tyro 18 | 15 | 0 | 48 | Good |
| Badger 14 | 8 | 6 | 36 | Good |
| Zippo 11 | 30 | 0 | 40 | Good |
| Rebecca 1 | 0 | 21 | 21 | Unconscious 1 |
| Seymour 6 | -15 | 27 | 15 | Unconscious 1, Dead 1 |

4. `DDRUUU` reaches `(28,15,7,North)`, minute **583**, ctr24 **3**, with those
   four acting survivors. This frontier was also executed. No First Aid, well,
   Training, HP/condition injection or gold override substitutes for recovery.
5. **Implementation acceptance begins beyond that current frontier:** take 21
   more northward steps, acknowledge the original Temple label and use Space at
   `(28,15,28,North)`. Expected arrival is day 8/minute **604**, ctr24 **0**,
   with unchanged gold/RNG/conditions. The entrance Slime is already defeated;
   certified remaining actor influence introduces no new encounter on this leg.
6. Save Quiet checkpoint A before entry. Enter Temple, select F6, quote and
   confirm **410 gold**: 30 low HP + 30 Unconscious + 300 level + 50 Dead.
   Seymour becomes HP **15**, SP **27**, conditions zero; gold **400**. The other
   five owners remain byte-exact. Select F5 and pay **60** for Rebecca; HP **21**,
   SP **21**, conditions zero, gold **340**. All six can act. Multiple payments
   still owe only one two-day departure. A fresh F6 quote is zero/no-op.
7. Depart once to day **10**, minute **604**, ctr24 **0**, gold **340**, bank 0/0.
   Expected RNG is **2583579601:2144**; generated 1152 stock bytes have CRC32
   **37e8cc62**. This stock result was calculated by a detached complete generator
   from the observed prefix cursor; it is not evidence of a paid M43 runtime.
   Acceptance must compare every byte and bounded/raw/rejection request trace,
   not only the checksum. Require exactly one generation/interest operation.
8. Save Quiet B, exit the entire application and restore in a fresh process.
   Compare every encoded owner/state byte before first input and replay the same
   continuation against an uninterrupted B branch. No payment/refill/day replay.
9. Retrace south to `(15,0)`, face South, Space/Yes; perform the original reset
   exit to `(23,10,12,South)`. Turn north, step to `(23,10,13)`, Space/Yes to
   re-enter. Advance until reset-Slime contact. Other ready members Block while
   the recovered Seymour uses his existing learned Magic Arrow at the live Slime
   on his first eligible turn; settle normally. This is useful resumed play by
   the resurrected owner, spends existing SP and must retain real wounds/XP/RNG.
   Return to Temple and demonstrate another selected recovery or a zero-price
   refusal, with the corresponding departure, then capture/restart again.

Steps 5..9 are the executable acceptance prescription, not already observed
gameplay. If actual new implementation cannot reproduce the expected prefix,
arrival, affordable recovery or useful continuation, investigate the discrepancy
before claiming acceptance; never edit state to make the witness pass.

Also retain the shorter living-condition branch: stop blocking at Seymour's
natural broken Armor slot 0 (39 Blocks). Observed at the same west-facing label:
day 8/minute 577, HP -11/Unconscious 1, gold810, RNG2732157854:1203; the other five
members are alive. It reaches the existing frontier at minute582. Temple arrival
is expected at603; selected Heal costs60, restores HP15, preserves SP27 and leaves
gold750. Broken armor stays broken. This is distinct from a synthetic HP fixture.

### Required tests and independent review

Implementation must add the smallest focused suites sufficient for changed
behavior, run their real-data/process variants, then build normally and run the
complete unfiltered CTest suite for closure. Required cases:

- Route: all 49 cells/facings, both directed seam crossings, original collision,
  out-of-domain refusal without new work, label automatic/manual selection,
  Temple All dispatch/return, exact Event union and mutated-record rejection.
- Actor/resource proof: independently enumerate both city forms, with/without
  occupancy; assert the exact 4/103 sets and all dormant identities; enumerate
  all 2048 coordinate/activation candidates per relevant actor; test live HP,
  defeated/accounted sentinel, inactive regions, flag-9 preserved/reset branches,
  cache reconstruction, corrupt/missing resources and dynamic actor rasters.
  Admission cannot be certified solely by a successful walk or by calling its
  own hard-coded closure validator as the expected oracle.
- Recovery rules: all classes and level/temporary-level extremes; full/low/
  overmaximum/negative HP, zero quote, exact/insufficient funds, each supported
  condition/severity and combinations, Dead with Unconscious, unsupported
  conditions, selected unable-to-act versus no surviving party. Verify reset
  before HP before condition clear, including Disease crossing an Endurance
  bonus threshold and an optional second paid Heal. Compare selected and all
  untouched fields, SP above recomputed maximum, omitted temporary-input proof,
  inactive owners, temp age, equipment/curse preservation and numeric limits.
- Time/economy: the complete table above, especially 8->10 versus two one-day
  calls, 97->99, unpaid98->99, refused paid98 and entry99; minute bounds,
  ctr24 preservation, no condition ticks or ordinary daily work; all 1152 stock
  bytes, bank overflow/floor arithmetic and generation/raw/rejection traces.
  Cover partial preparation, RNG count exhaustion and checked finite authority.
- Reservation/publication: one-day nontrigger/trigger to first paid upgrade;
  reuse of an already-generated reservation; multiple recipients/repeat Heal;
  failed upgrade retaining an exit; no payment before complete successor;
  allocation/provider/render faults before and after each publication; retry
  without duplicate debit/cure/day/interest/draw; full mutation->reversion and
  exception guards, cache/provider insertion and final-frame failure.
- Input/save: native Space/F1..F6/Enter/Escape/F9, first-key responsiveness,
  cosmetic handoff and held/repeated/batched/stale/reentrant origins; denial
  before any save provider/path/I/O in every non-Quiet phase; no authority wrap,
  free exit, hidden autosave or saveable intermediate payment/departure state.
- Compatibility: explicit 9/14 roundtrip/restore in initial/reset forms, exact
  paid and refusal continuation in separate processes, every old supported pair,
  unknown/crossed pair rejection, legacy route/closure/one-day meaning, raw books
  and preserved overmaximum HP/SP. Re-run inherited Smith/Training/casting and
  post-service combat-clock regressions.
- M42 depletion: start a fresh **content-14** seed-7 run using the accepted
  [M42 purchase prefix](milestone-42-plan.md#production-acceptance-boundary), buy
  the real ordinary offer, settle to depleted stock and save/restart. Then pay
  Temple recovery and prove full replacement from that exact depleted preimage.
  Branch to unpaid/nontriggering Temple departure and Training's distinct
  member-days as well. Do not relabel a legacy 9/13 save as 9/14 to obtain access.

Near-date, severity-255, nonzero bank, rejection/count-exhaustion, omitted-input,
mutation/fault and Disease-ordering fixtures may be synthetic and must be labeled
as such. They do not establish earned money, death, learned spells or production
progress. Keep actual stock depletion and the main death/recovery loop as separate
production witnesses.

Obtain independent technical review of the route/actor certification, selected
HP ordering and eligibility, single-operation departure/reuse, full mutation
history, atomic publication and legacy wire/capability separation. Reviewers
must distinguish the observed prefix, detached predictions, new automated
runtime evidence and physical acceptance. Resolve blocking findings before
requesting final closure; an implementation agent's own tests are not that review.

### Maintainer physical native-SDL acceptance

The maintainer should perform/observe the connected fresh-seed witness with
native controls: real combat injury/death and survivor status; northbound route,
seam, scenery/ceiling and label; selecting the nonacting recipients; displayed
410/60 quotes; cancellation/refusal; exact HP/conditions/gold and unchanged SP;
one two-day exit; responsive controls; Quiet F9; complete process exit/restart;
southbound return/reset, recovered Seymour's useful combat casting, return visit
and another restart. Inspect both orientations across the new seam and the final
service-cell view. Observe the inherited Buy-depleted-stock continuation too.

A separate refusal-only branch from A must owe one day rather than two, with
8->9 retaining stock/RNG. Paid two-recipient A->B must owe two days total rather
than one day per recipient plus another day. Physical evidence does not replace
byte/trace comparison, and SDL dummy/process or raster evidence must not be
recorded as maintainer-performed acceptance.

## Remaining uncertainties and implementation order

No essential route, actor-profile, affordability or representation blocker was
found. The paid Temple flow and full post-recovery production witness do not
exist yet; their execution, fault behavior, new contract admission, native layout
and physical acceptance are implementation obligations, not completed evidence.
Fine UI layout and internal names can be resolved during implementation within
the existing architecture. Any unexpected nonzero omitted input, new influencing
actor/mechanic, or inability to complete the honest witness is a concrete stop
and focused replanning trigger, not permission to broaden scope silently.

After maintainer approval and separate implementation authorization, implement
in order: content/route/resource/actor admission and its independent tests;
pure selected recovery and bounded 2880 candidate/upgrade; existing-owner Temple
coordination/UI and fault/input coverage; explicit 9/14 persistence/legacy
regressions; original-resource paid/depleted-stock/process witnesses; independent
review and physical acceptance. These are implementation steps inside one
milestone, not separately approved capabilities or premature closure points.

At closure follow [AGENTS.md](../AGENTS.md): required build/full tests/review and
physical acceptance precede stable-status/history updates and plan condensation.
This proposal changes neither current capabilities nor future roadmap selection.
