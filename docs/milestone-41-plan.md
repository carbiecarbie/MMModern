# Milestone 41 - Vertigo Training and progression

## Closed scope

**COMPLETED AND ACCEPTED.** M41 connects a fresh Regional Journey, legitimately
earned XP, original Vertigo Training, carried-gold payment, permanent progression,
active temporary reset and selected refill, individual Training days, separate
departure, further combat/casting, Quiet save and exact fresh-process continuation.
Different trainees, return visits and the stock boundary are included.

The slice retains M39 casting, M38 armor repair and M40 party economy, world RNG
and complete individual one-day service candidates. Excluded are other trainers/
towns, class changes, purchased skills/spells, guild membership, Temple, Buy/Sell,
player stock access, general rest/replenishment, new quests, unrestricted city/
Clouds travel, general calendar processing and Darkside gameplay. Commercial
resources remain external, read-only and resource-driven; native controls and
numeric feedback are authored English interface text.

Original rules are interpreted from the pinned ScummVM revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, whose configuration/provenance belongs
to [dependencies](dependencies.md#pinned-scummvm-revision), and bound to decoded
original resources below. Sizes, offsets and CRC32 identify decoded resource bytes. Reference interpretation and resource/closure
calculations are distinct from physical DOS observation. Inherited city/reset,
repair, casting and stock contracts remain in [M37](milestone-37-plan.md),
[M38](milestone-38-plan.md), [M39](milestone-39-plan.md) and
[M40](milestone-40-plan.md); M41 does not reopen their algorithms.

## Route, Events and resources

### Exact player domain

Content 12 adds twelve cells to M40's sixteen-cell Vertigo domain. All four
facings are admitted at every cell. Coordinates use logical map 28, north = +Y.

| Set | Cells |
| --- | --- |
| Inherited sixteen | `(15,0..4)`, `(16,1..4)`, `(8..14,4)` |
| Added twelve | `(15,5..7)`, `(10..14,7)`, `(10,8..11)` |

The logical city remains the original 32-by-32 map rooted at 28, with physical
tiles 28/109/110/111. New cells are all on physical map 28 with identical local
coordinates. The inherited X=16 seam still resolves through tile 109 without
changing Event or actor identity to 109. Keep original adjacency, overlays,
visited/cell flags, wall rules and projection; do not flatten the city or clip
world geometry at the player boundary. An attempted move outside the admitted
cell set is refused before time, activation or movement publication.

Read-only new-cell wall nibbles, in N/E/S/W order:

| Cells | Walls | Raw attributes |
| --- | --- | --- |
| `(15,5..7)`, `(14,7)` | `0/0/0/0` | 1 |
| `(13,7)` | `8/0/0/0` | 1 |
| `(12,7)`, `(11,7)` | `8/0/8/0` | 1 |
| `(10,7)` | `0/0/8/0` | 1 |
| `(10,8)` | `4/8/0/8` | 17 |
| `(10,9)` | `0/8/4/8` | 2 |
| `(10,10)`, `(10,11)` | `2/0/2/0` except `(10,10)` south = 0 | 2 |

Walls 4 and 2 on this approach pass the inherited player collision policy.
Use the original raw ceiling bit, not an inferred indoor-room ceiling: these
new cells do not acquire the smith's ceiling. From `(15,4,North)`, forward three,
turn left, forward five, turn right, forward four reaches `(10,11,North)`.
Turns cost zero; the twelve successful indoor steps cost twelve minutes.
Movement, turning, Wait and presentation retain their existing scheduling and
activation semantics. Neither service UI nor stock preparation advances actors.

### Complete relevant Event graph

Read-only `maze0028.evt` is 7,298 bytes, CRC32 `28b6c20b`, 847 records.
The only records on newly admitted cells are:

| Record / decoded file offset | Trigger | Operation and result |
| --- | --- | --- |
| 538 / 4464 | `(10,8,North,line 0)` | `0x02`, parameter 32: resource label `aaze0028.txt[32]`; natural script end. Cell attribute 17 permits its inherited automatic activation; manual activation follows the same directional Event lookup. |
| 3 / 23 | `(10,11,All,line 0)` | `0x11`, parameter 5: `DoTownEvent(TRAINING)`. Length byte 6, seven total bytes. Attribute 2 does not auto-trigger the service. Space invokes it from any facing. |

Pinned `Scripts::cmdDoTownEvent`, the `TRAINING = 5` dispatch in `locations.h`
and `TrainingLocation` establish the service meaning. Town dispatch terminates
the script through `cmdExit`; there is no additional Training Event branch to
execute after departure. Leave the player on `(28,10,11)` with the same facing,
classify the retained city and retire the Event before presenting exploration.
Do not reopen the service automatically or replay its entry on restore.

Retain the complete inherited transitive graph: mainland entry records 136..139,
smith record 0/action 1, smith label 539, city exit records 760..769 and their
original `(100,100)` reset/prelude operations and flag-9 branch. Admission must
validate this union, including opcode, arguments, coordinates, direction, line,
ordering and terminal behavior. A matching service record alone is insufficient.
Do not admit other service records merely because their opcode is `0x11`.

### Presentation and immutable admission

Use the original Training title from `aaze0028.txt[32]`, original roster names,
`trng1.twn` frame 0, `train.icn` frame 0 and `esc.icn` frame 0. A static artwork
frame is sufficient; animated trainer/audio fidelity is outside this slice.
Native English controls and numerical feedback are authored interface text,
not hard-coded copies of commercial dialogue. Show selected name, permanent
level, stored XP and missing XP, quoted cost, carried gold, days owed, and
current/derived maximum HP and SP. The paid result shows old/new level and XP,
gold debited and remaining, old/new maximum HP/SP and actual refilled HP/SP.
Make party reset and the separate departure charge visible. A nonselected
member above its recomputed maximum must still display the preserved current
value. No diagnostic log is required to understand the transaction.

In addition to the inherited complete M37/M38/M40 manifest, require these
observed decoded resources and frames:

| Resource | Bytes | CRC32 | Frames |
| --- | ---: | --- | ---: |
| `trng1.twn` | 27998 | `a4e3bbdb` | 8 |
| `train.icn` | 1614 | `76c6ac78` | 4 |
| `esc.icn` | 792 | `096b68b7` | 2 |
| `004.obj` | 3159 | `bf5e5f91` | 1 |
| `006.obj` | 5534 | `4eb51842` | 1 |
| `008.obj` | 18331 | `ff9b7e6d` | 10 |
| `009.obj` | 12450 | `3deef973` | 8 |
| `010.obj` | 4340 | `bc3a2ad6` | 1 |
| `011.obj` | 7355 | `9eea738b` | 2 |
| `maze.chr` | 10620 | `81a2dd16` | 30 records of 354 bytes |

The object rows are the union reached by the expanded camera domain, including
already admitted resources. Required city geometry remains `maze0028.dat`
(892, `1399bb82`), `mazex109.dat` (892, `dec2f1e2`), `mazex110.dat` (892,
`7bd43d60`), `mazex111.dat` (892, `b5351545`), `maze0028.mob` (820,
`d2612605`, 143 objects and 46 initial monsters), and `aaze0028.txt` (3014,
`8dc60e26`, 64 strings). Preserve the inherited terrain/sky and monster profile
signatures; no altered object or monster data is accepted under content 12.

Admit the immutable resource union during fresh setup and restore validation,
retain it across services/cache reconstruction, and verify identity before
provider references escape. Missing or corrupt resources cannot be substituted
or ignored to admit the service. Resource and actor admission covers every
admitted camera/facing/cosmetic phase, including dynamic states.

## Dynamic actor closure

Closure certification enumerates each actor's `(x,y,activated)` fixed point over
all 28 admitted camera cells and four facings, separately for initial and reset
forms, including stutters and occupancy-free conservative movement. Independent
resource-driven tests also retain real actor occupancy and compare the complete
sets. Admission is based on these exact sets, including off-route influence,
not a successful route or rectangular corridor approximation.

| City form | Original slot | Type / starting cell | Reachable states |
| --- | ---: | --- | --- |
| Initial | 34 | Slime 0, `(7,7)` | Dormant spawn plus three activated positions below: 4 total |
| Initial | 35 | Slime 0, `(15,4)` | Dormant spawn plus 52 activated positions below: 53 total |
| Reset | 35 | Slime 0, `(7,7)` | Same 4-state small closure |
| Reset | 36 | Slime 0, `(15,4)` | Same 53-state entrance closure |

Every other retained actor remains at its corresponding original/reset position
and cannot become activated from any enumerated camera/state. Preserve all 46
initial slots. The inherited reset form retains 52 slots, with the existing
unmaterialized gap 46..49 and additional original reset Slimes 50/51; do not
drop actors, renumber identities or invent monsters for the gap. Reset slot 34
is at `(6,6)` and does not inherit initial slot 34's activation permission.

The small Slime's activated positions are exactly `(7,6)`, `(7,7)`, `(8,7)`.
The entrance Slime's activated positions are:

| Y | X values |
| ---: | --- |
| 0 | 15 |
| 1 | 9..16 |
| 2 | 13..16 |
| 3 | 14..16 |
| 4 | 8..16 |
| 5 | 9, 10, 11, 14, 15, 16 |
| 6 | 13..16 |
| 7 | 9..16 |
| 8 | 10 |
| 9 | 10, 12 |
| 10 | 10, 11, 12 |
| 11 | 10, 11, 12 |

Both closures include positions outside player admission. They are disjoint,
and all other actors remain fixed; retaining real occupancy can only remove
some enumerated transitions. Runtime canonical admission must use these exact
sets and original/form-specific identities, not their enclosing rectangles.
Keep the existing initial/reset distinction and certify it independently for
content 12. Content-12 closure caches have a distinct route/form dimension;
legacy closure results and admission remain unchanged.

### Contact, rewards and returns

The small Slime is additional **influencing** admission, not an additional
reachable kill. Wall 9 east of `(8,7)` blocks it under the original actor test
against the map's actor difficulty; do not substitute the player wall
threshold. It has no ranged attack, never occupies an admitted player cell,
cannot join contact through the wall, and cannot be targeted by exploration
Shoot/Run or noncombat Magic Arrow. Its HP remains 2, with no wounds, kill
accounting or rewards; position/activation must persist exactly.

The entrance Slime can occupy/contact the route and retains existing combat
joining, wounds, kill and XP rules. Admit live HP 1..2 and the inherited
accounted dead sentinel `(-128,-128)`, HP 0, inactive, once-only reward state.
Its base XP is 50 and it yields no item or gold. Joining remains based on actual
positions/visibility and existing combat admission, never on membership in an
overlapping static bounding box. There is no new reachable city species.

On a legitimate exit, preserve flag 9, overlay and exact reset conditions.
The reset branch rewrites its specified actors and supplies the reset form;
the preserved branch retains accumulated city state. Service departure,
classification, cache reconstruction and save/restore are not city resets.
Return visits certify both forms and every retained live/wounded/dead entrance
state. The inactive actor region gets no pulses or progression. Mainland's 19
identities and restrictions remain unchanged. Legacy routes do not gain the
expanded player cells or this additional actor influence.

## Training rules and state ownership

### Eligibility, arithmetic and refusal precedence

Pinned evidence: `TrainingLocation::maxLevel/createLocationText/doOptions`,
`Character::nextExperienceLevel/getCurrentExperience/experienceToNextLevel`,
`Character::noActions`, and `CLASS_EXP_LEVELS`. Vertigo's cap is permanent level
10. All ten existing classes may train; there is no guild/skill/class filter.
Temporary level is irrelevant to XP eligibility, price and the level increment.
An already supported character at permanent level 10 or above receives the
trainer-cap refusal; it is not rejected from Journey merely for exceeding 10.

For a trainable permanent level `L` in 1..9, let `X` be the sole stored u32 XP
supplement and `C` the class base:

| Class | C |
| --- | ---: |
| Knight, Cleric, Ninja, Barbarian, Druid | 1500 |
| Paladin, Archer, Sorcerer, Ranger | 2000 |
| Robber | 1000 |

```text
N = C * 2^(L-1)
B = (L == 1) ? 0 : C * 2^(L-2)
currentExperience = u32(B + X)
missing = (currentExperience >= N) ? 0 : N - currentExperience
cost = 10 * L * L
on success: storedXP = X - (N - B); permanentLevel = L + 1
```

The u32 conversion explicitly reproduces the reference's unsigned wrap; it is
not saturation, undefined signed overflow or a widened eligibility comparison.
Use checked/wide intermediates and explicit modulo conversion. `N <= 512000`,
`B <= 256000`, and cost <= 810 here. Eligibility implies sufficient stored XP
for the bounded debit. Do not evaluate a shift for invalid level 0 or a
cap-refused level. Invalid enums, unsupported conditions/effects, missing
supplements or invalid numeric/derived domains are MMModern admission/integrity
failures, distinct from an original trainer refusal. Existing XP reward
publication remains its sole earning path; Training does not reinterpret saved
XP as a lifetime total.

After validating authority and supported state, presentation/confirmation uses
this precedence: permanent cap; missing XP; inability to act; insufficient
carried gold; then M41 service/capacity support limits. Cap takes precedence
over a missing-XP calculation. `noActions`/existing `canAct` excludes Asleep,
Paralyzed, Unconscious, Dead, Stoned and Eradicated; other conditions do not
create a new trainer-specific exclusion. Do not broaden the admitted Journey
condition domain just to exercise reference conditions. Unsupported service
capacity is shown separately from missing XP or gold, never as an original
opening-hours rule. Pre-admission inability to reserve departure refuses entry.

Quote current owner identity, class, permanent level, XP, condition state,
purse, reset population, post-reset derivation inputs, visit revision and
reservation revision. A quote authorizes one level only. Selection, an accepted
level, cancellation or any relevant owner/revision change invalidates it.
Confirmation revalidates exact preimages and the retained mutation history;
it cannot silently reprice against changed live state. Exactly sufficient gold
is accepted and becomes zero. Insufficient funds leave everything unchanged.
Payment never draws on pending treasure, gems or bank balances.

For example, Knight L3/XP3000 exactly qualifies, while XP2999 is missing one.
Knight L3/XP9000 can buy L4/XP6000 for 90 and L5/XP0 for 160 in the same visit,
with reset/refill on each purchase and only one member-day. Knight L3/XP
4294967295 wraps current experience to 2999 and is missing 3001.

### Existing owners and derivation

`XeenCharacter` owns permanent/temporary level, temporary age, intellect,
personality, endurance, conditions, current HP/SP, birth year, skills, raw
learned books and physical equipment. The roster's `XeenCombatInputs`
supplement owns XP, might, speed, accuracy, temporary AC, luck, cold/electrical
and poison inputs. `XeenParty` retains those roster owners and active membership.
Carried gold is the existing `monsterTreasure->gold` field; pending reward
fields in the same owner are separate. Service wares/bank belong to the party;
the gameplay RNG belongs to the world. Do not introduce another XP, level,
wallet, maximum-stat cache or duplicate character owner.

`XeenEncounterFlow` is the mutation authority. Prepare detached scalar/array
deltas, then write existing observed owners through narrow Training operations.
Respect marked-roster, gameplay-borrow and active-combat restrictions; a service
does not obtain broad assignment/swap authority over a marked live roster.
Validate active owner IDs and deduplicate the reset population by roster ID.
Production has fixed distinct active owners `{0,18,14,11,1,6}`.

Use `XeenCharacterRules::validateForUse`, existing maximum HP/SP and combat
derivation with the post-level, post-reset character and resulting context.
Prepare and validate maxima before mutation, including signed-current-stat
representability in int16. Unsupported derivation refuses the purchase without
payment. Permanent level/temporary values feed existing physical attack counts,
damage, casting and recovery consumers; no Training-specific formulas replace
them. Derived values are recomputed on use, never serialized as new owners.

### Exact reset/refill matrix

Pinned `Party::resetTemps` iterates `_activeParty`, not the roster. Training
performs payment/XP/permanent level, the first-member `addTime(1440)`, then
`resetTemps`, then selected-member maximum HP/SP assignment. Preserve this
evaluation order inside the prepared atomic result described below.

| Field / owner | Selected active member | Other distinct active members | Inactive roster members |
| --- | --- | --- | --- |
| Permanent level / XP | +1 / bounded debit above | Preserve | Preserve |
| Temporary level | Zero | Zero | Preserve |
| Temporary might, intellect, personality, endurance, speed, accuracy, luck | Zero in their existing character/supplement owners | Zero | Preserve |
| Temporary AC | Zero | Zero | Preserve |
| Temporary cold, electrical, poison resistance | Zero in existing supplement | Zero | Preserve |
| Temporary fire, energy, magic resistance | Implicit zero domain, proved below | Same | Original omitted inputs also verified zero; no roster-wide reset |
| Temporary age | Preserve | Preserve | Preserve |
| All permanent attributes/resistances, class/race/sex/birth year | Preserve | Preserve | Preserve |
| All sixteen condition bytes | Preserve | Preserve | Preserve |
| Current HP/SP | Assign post-reset/post-level derived maximum HP/SP | Preserve exactly, even above the reduced maximum | Preserve |
| Learned books, skills, equipment, inventory order and all other supplement fields | Preserve raw bytes/values | Preserve | Preserve |
| Party temporary effects/light/resistances | Reset the reference-listed party fields; admitted domain already requires all relevant fields zero | Shared party owner | No per-roster effect |
| Carried/pending treasure | Debit only selected level's carried-gold cost; retain all other fields | Shared party owner | No duplicate purse |

The reference party fields cleared are poison/cold/electrical/fire resistance,
light, levitation, water walking, wizard eye, clairvoyance, heroism, holy bonus,
power shield and blessed. `XeenGameplayContext::effects[9]` and
`lightAndResistances[6]` remain canonically zero under M40's admitted domain;
neither Training nor any admitted spell introduces a nonzero writer. Retain
`rested`, `newDay`, minute, year and `ctr24` under the one-day contract.

This is no resurrection, condition cure, skill/spell grant, free rest or general
clamp. A selected above-maximum HP value can decrease: the production Tyro
checkpoint is 67 HP, and L4 refill sets it to 64. A nonselected member at 61/36
stays at 61/36. Do not recompute stored current values when merely selecting,
quoting, refusing, departing, loading or redrawing.

### Omitted temporary inputs

Content-12 fresh and restore resource admission verifies all thirty original
`maze.chr` records before discarding omitted inputs. Record-relative temporary
resistance bytes 312/314/316/318/320/322 are fire/cold/electrical/poison/energy/magic;
all are zero in the admitted source. The parser retains cold/electrical and
poison; omitted fire/energy/magic bytes 312/320/322 have explicit zero checks as
well as resource-signature binding. Changed nonzero omitted inputs are rejected.

No admitted preparation, combat, recovery, casting, repair, Event, equipment
action or service writes an omitted temporary resistance, and schema 9 has no
such durable field. This preserves the zero domain through play/save/restore
without a wire extension. Modeled nonzero temporaries remain represented,
guarded and reset by the matrix. Guarding thirty characters never authorizes
resetting thirty characters.

## Individual service days and departure reservation

### Reference sequence and supported dates

`TrainingLocation::doOptions` charges one `addTime(1440)` on the first
successful level of each active member in a visit. Further levels for that
member, including after switching away and back, do not charge another day.
Track a transient bitset keyed by stable roster ID, not UI index. Set a bit
only on successful publication; clear it only when beginning a later visit.
Every successful level still resets active temporary state and refills its
selected member. Refusals and cancelled quotes do none of these things.

`BaseLocation::show` adds its separate 1440-minute departure, including a visit
with no training. Training has no extra farewell-time override. The reference
evaluates each call independently; three one-day calls must not become one
4320-minute call. Inherit M40's exact stock generation order, 160 item calls,
discard rules and RNG requests, followed by shared-bank interest on a triggered
call. With charge 1440, only destination days `11,21,...,91` trigger. A
nontriggering day consumes no RNG or interest. Zero bank balances remain zero
but the triggering interest operation is still executed. Party reset/refill is
after the member-day's generation and interest, not before.

Interest retains M40's gold-then-gems order and explicit u32
`balance + floor(balance/100)` wrap. It never touches carried or pending gold.

Content 12 retains year 610, canonical day 8..99, minutes 300..1259, `ctr24 < 24`,
Adventurer/WorldOfXeenClouds, zero unsupported effects and false rested/newDay.
Ordinary movement/casting/combat time restrictions remain unchanged. Do not
enable midnight, unrestricted 480-minute processing, year rollover, aging or
multi-day services. Admission requires a complete one-day departure successor
and authority to settle it; day 99 cannot admit a service.

### Chained complete candidates

M40's smith holds a fixed complete departure successor. Reusing that successor
unchanged after Training advances a member-day would be invalid. Use this
specific chain:

1. While the Event remains exclusive, prepare complete candidate `R(D)` from
   live context/economy/RNG to its one-day successor. It contains all stock,
   interest and RNG consequences. No debt exists until this is complete and
   the first service frame/resources and settlement authority are prepared.
2. On admission retain `R(D)` as the unavoidable departure reservation.
3. For a new trainee, treat **this very candidate** as the proposed member-day
   `M`. Prepare `R(D+1)` from `M`'s ending context/economy/RNG. Retain `R(D)` and
   the old debt throughout this work. Do not regenerate `M`, publish a private
   cursor, or consume world RNG while constructing either candidate.
4. Prepare the level/payment, active resets, selected refill, complete expected
   guard and fixed result against the checked old owners and `M`'s context.
   Only after `R(D+1)` is complete, canonical and has a guaranteed settlement
   suffix may confirmation publish. The one nonthrowing unit installs the
   progression/payment/reset/refill and `M`'s context/economy/RNG, marks the
   member trained, and replaces the reservation with `R(D+1)`.
5. For an already trained member, prepare the same progression/reset/refill
   unit without a day. The retained reservation remains valid against unchanged
   context/economy/RNG; verify it rather than regenerating it.
6. Departure publishes the retained complete reservation once, then performs
   guarded actor classification/Event retirement/presentation settlement.

Candidates are detached values, never replacement mutation owners. Their
preimages include full context, every economy byte and RNG algorithm/state/count;
full guards additionally retain unrelated owner mutation history. Replacing an
authorized expected guard must carry that history/resources forward, not erase
an earlier mutation->reversion. Preallocate every result, reservation, expected
delta guard and publication storage. No allocation, provider, observer callback,
validation throw or rendering occurs inside either publication unit.

Use the main idle loop and M40's maximum 64 raw RNG attempts per service slice.
Yielding preparation keeps the Event/Service lease and save denial. An ordinary
preparation failure can retain a checked private prefix or discard the pending
purchase; it leaves the old complete departure candidate intact. Retrying the
same confirmation cannot publish twice. Rejected conversion draws are retained
in the private candidate's exact continuation, not rerolled from live RNG.

At day 97, entry reserves 98; a new trainee may consume 98 and reserve 99.
At day 98, further eligible levels for an already trained member remain possible,
but a new trainee is refused before debit because departure to 100 cannot be
reserved. A fresh refusal-only day-98 visit can still depart to 99; day 99
refuses admission. A prepared triggering 10->11 candidate is consumed once,
whether its final role is member-day or departure.

Feasibility is not a date check alone. Validate complete candidates, full
generation/request-count capacity, canonical stock, post-level derivation,
storage and authority. RNG count exhaustion while preparing an additional
trainee is a deterministic support refusal with the old departure retained;
it must not cause endless automatic retries or release debt. A trigger candidate
may contain a variable number of rejection draws, so a guessed fixed RNG margin
cannot replace completing it.

### Finite authority budget

Use checked u64 arithmetic as in `xeenSmithAuthorityRoom`; never wrap ticket,
operation, input, Journey or boundary generations. Reserve conservative explicit
headroom: before admission, 16 EncounterFlow/Journey revisions, 8 Event input
revisions and 4 boundary revisions; before accepting any optional selection,
quote, acknowledgement or level, require its own consumed revision(s) **plus**
that same remaining suffix budget. A Training operation ID needs one additional
representable increment before a new quote. These are headroom requirements,
not dummy generation increments.

The mandatory suffix has no more than: result/refusal presentation and response,
departure presentation and response, atomic departure, arrival classification
publication, Service-to-Event handoff, Event retirement, final presentation and
its acquisition, plus inherited guard margins. It must fit the reserved budget;
each counter is independently checked at the threshold. Preparation slices, retries
of an unchanged semantic result, idle/cosmetic frames and rejected/stale keys
must not consume additional semantic revisions. Concrete frame tokens remain
fresh but do not spend another operation/input revision for an identical phase.
When another optional action cannot preserve the suffix, show departure-only
mode using the reserved authority. Wrong keys cannot exhaust its last frame.
After admission no counter exhaustion can make the owed departure unpayable.

## Architecture, phases and publication boundaries

### Narrow continuation and native input

Training uses the existing Event/Service/Presentation architecture without a
parallel service scheduler or nested SDL loop. `XeenEncounterFlow` owns a transient
Training continuation: lease, selected roster ID, quoted immutable preimages,
operation/revision, trained-member bits, current complete departure reservation,
optional pending level/next reservation, fixed result, published-level marker
and departed marker. `XeenEventFlow` owns menu/presentation state and routes
typed responses to the Encounter owner. The Event owns exclusivity before
service admission and after service retirement. No ownership handoff exposes
Quiet until the final valid exploration/combat presentation is acquired.

The continuation uses guarded preflight, bounded preparation and consumed
Training frames for selection/quote, one-level confirmation, fixed-result
acknowledgment and departure settlement. Detached rule preparation has no
provider, live mutation or service-loop authority. The explicit `0x11/5`
dispatch/content-12 capability applies only at the original Training cell.

Fresh selection is F1 (Arturius). F1..F6 selects that active member and presents
its eligibility; Enter opens the one-level quote, Enter on a **fresh quote
frame** confirms, Escape cancels a quote back to the menu. Enter acknowledges
a result/refusal; only a subsequent fresh selection/quote can buy another level.
Escape from the menu requests departure, including zero-training visits. During
candidate preparation no second purchase/selection is accepted. A departure
failure offers Enter/Escape to retry remaining settlement, not a free return.
This two-step native confirmation is an adaptation; original arithmetic and
service charges remain exact.

Consume phase and concrete frame authority before validation/providers/hooks.
Semantic identity includes visit, operation, phase, selected owner and quote
revision; concrete identity includes the successfully presented frame token.
Never accept an old menu response merely because its semantic phase recurs.
Held keys, SDL repeats, batched Enter/F-key/Escape messages and reentrant
callbacks may consume at most the original frame; later actions require a
newly acquired frame and key edge. Resize/expose/cosmetic redraw cannot revive
spent authority. Exceptions get post-callback guard checks too.

Cosmetic presentation uses **make-before-break input authority**. The old
actionable concrete frame remains authoritative while its cosmetic successor
is composed, uploaded and acquired. A bounded queued input batch retains its
exact old concrete origin through the handoff; it is never relabeled as input
for the new frame. A semantic action from that old frame supersedes an obsolete
cosmetic successor and its successor must pass the normal presentation fence.
After handoff, stale old-frame input cannot authorize the new frame.

Cosmetic replacement preserves unchanged gameplay semantic generations and
does not repeatedly retire legitimate first key edges or exhaust the mandatory
settlement budget. Genuine semantic/ticket changes retain strict fencing.
Held-key, repeat, key-up, batch ordering, close ordering and stale-origin checks
remain enforced. An already consumed frame cannot regain authority through a
cosmetic refresh.

### Publication and failure boundaries

- Event preparation creates no payment, day or debt. Admission requires a complete
  departure reservation, prepared resources/frame and settlement authority; it
  acquires the Service lease and admitted obligation before fallible presentation.
- Selection, quote, refusal and cancellation change only transient phase/owner/
  operation authority. They perform no progression, payment, reset/refill, day,
  RNG or trained-bit publication.
- A successful new-member level is one nonthrowing unit: carried-gold and XP
  debit, permanent level +1, reserved member-day context/economy/RNG, active
  temporary reset, selected refill, trained bit, published operation/fixed result,
  next departure reservation, expected guard and coordination revision.
  Additional same-member levels publish the same progression/reset/refill unit
  without a day or replacement reservation.
- Departure is a separate nonthrowing context/wares/bank/world-RNG unit with an
  irreversible departed marker. Guarded arrival classification, Service-to-Event
  handoff, terminal Event retirement and final concrete presentation then settle
  under existing authority without a Quiet gap.

All allocation, result/storage preparation, validation and expected-guard setup
precede either publication. No provider, observer callback, render or throwing
work occurs inside it. A successful level has no observable paid-but-unleveled
or leveled-but-unsettled-member-day state. Sequential purchases are independent:
failure on a later level preserves earlier levels, payments and member-days.

Failures after a level or departure retry only unfinished presentation/settlement,
never progression, reset/refill, payment, time, stock, interest or RNG. The old
complete reservation survives ordinary pending-purchase preparation failure;
a failed final display cannot charge another departure. An admitted obligation
cannot be cancelled into a free Quiet return.

Ordinary preparation/presentation faults retain a healthy continuation and its
valid retry/cancel path. Integrity/preimage/resource violations permanently fail
authority without rebasing or saving. Window close/fatal rendering may terminate
the unsaved process; it neither undoes committed levels nor publishes a free
departure or autosave. The last earlier valid save remains the restart point.

### Guards and save authority

F9 must be denied **before capture/provider invocation, save-path preparation
or I/O** during exclusive pre-admission preparation, every Training UI phase,
private stock work, debt, failed settlement and unpresented handoffs. Enforce
this at the existing save acquisition boundary, not merely by hiding a button.

Retain full guards for all thirty characters and supplements, active membership,
XP/levels/HP/SP/conditions, every equipment byte and all 39 raw learned-book
bytes, carried and pending treasure, all economy bytes, gameplay context,
world RNG, both actor regions, overlays/flags, camera and admitted resources.
Active-only reset does not reduce observation coverage. Each provider, hook,
render/presentation call, scheduler and exceptional exit checks mutation history,
including change->reversion within the callback. Value equality alone fails.

Extend observation during insertion/reconstruction **before** new storage
references escape (`prepareOwned`/`addOwned`, roster supplement replacement,
world map/object/actor cache insertion and resource caches). Checking a later
`current()` is too late. Authorized publication prepares expected deltas and
observed storage before writes, then replaces the guard without resetting the
retained failure/history chain. Never build an accepted baseline from arbitrary
callback-mutated live data. Reconstructing an identical cache must not make a
previous unauthorized mutation disappear.

Explicitly preserve M40's combat-clock fix: scheduling and cosmetic callbacks
are guarded on both normal and exceptional paths, and a mutation->reversion
cannot be hidden by the next combat preimage. Retain its post-service and
full-exit/restore regression coverage, including Training on those paths.

## Persistence and inherited capability admission

Fresh Regional Journey uses **envelope v4 / schema 9 / content 12**, with the
explicit supported pair `9/12`. Schema 9 already serializes permanent levels,
current HP/SP and modeled temporary fields in characters; XP/resettable combat
fields in supplements; carried/pending treasure, context, economy, world RNG,
both actor regions, overlays and flags in their existing owners. Selection,
trained bits, quotes, debt and results are transient because capture is denied
throughout a visit. There is no live-service save, new schema, migration,
backfill or implicit upgrade.

Every schema-9 width/order and its economy representation remains unchanged:
1152 stock bytes, two u32 bank balances and inherited shape/presence encoding.
All raw learned-book bytes, including nonboolean values, remain exact.
Content-12 admission adds only its explicit Training capability, route, actor
closure, resources and inherited service/context capabilities. The original
immutable `maze.chr` is a checked resource, never a replacement for saved
progressed characters.

Capture copies exact levels, XP, temporary fields, current HP/SP, books,
equipment, context, economy, actors and RNG. Restore validates the complete
snapshot and resources before publication and installs these values before
first input. It does not train, re-earn XP, refill, reset, clamp, reseed, regenerate
stock, apply interest, advance time, reopen Training or replay Events. Display
maxima are derived read-only without overwriting saved current values.

Supported pairs remain exactly `1/1` through `8/8`, `8/9`, `8/10`, `9/11`
and `9/12`. Legacy domains gain neither Training nor the expanded route,
omitted-field normalization, economy backfill or new calendar authority.
Content 12 retains M39 combat First Aid/Awaken/Magic Arrow, exploration recovery,
M38 repair and M40 stock/interest/RNG behavior. Progressed levels feed the existing
combat, casting and XP consumers. The inherited wire layout belongs to
[M40](milestone-40-plan.md#persistence-and-compatibility).

## Production acceptance boundary

The production witness begins with a fresh **seed-7** content-12 Regional Journey,
its original prepared party and fresh stock generation. It earns missing XP and
gold through admitted mainland combat, ordinary treasure collection and the
entrance Slime, using the original selected well and Rebecca's learned First
Aid for survival. Tyro and Rebecca Attack while the other mainland participants
attempt individual Run; this changes legitimate XP recipients rather than
granting synthetic progression inputs. All six survive. The witness uses the
original +25 HP well once for each selected owner, skipping Rebecca; it supplies no SP.

At `(28,15,4,North)` on day 8/minute 796, Tyro and Rebecca each have stored
XP3280; carried gold is 870. Tyro has HP67/SP0 and Rebecca HP11/SP20.
The twelve new steps reach Training at minute 808. The accepted checkpoints are:

| Checkpoint | Durable result |
| --- | --- |
| A, before Training | Day 8/minute 808, gold870; world RNG algorithm 1/state 799325555/count 1101 |
| Tyro, first successful level | Day 9, gold780; L4/XP280/HP64/SP0 |
| Rebecca, first successful level | Day 10, gold690; L4/XP280/HP28/SP28 |
| B, separate departure | Day 11/minute 808; both progressed members retained, gold690; RNG 2959920300/count 2009 |
| C, later refusal-only departure | Day 12, gold690; postcombat XP, wounds and SP retained without training/refill replay |

The insufficient-XP request after Tyro's level charges no payment or member-day.
Exactly one stock generation/interest operation occurs across the two member-days
and departure; stock CRC32 `79dec2de` identifies the witness output, while
acceptance compares complete stock bytes and request/draw traces rather than
only the checksum. Original exit/reset/re-entry is followed by useful combat
First Aid and Magic Arrow, a return to Training and actual insufficient-XP
refusals for the progressed members.

Quiet F9 at A and B, full application exit and fresh-process restore compare
complete encoded state before first input, then compare paid A->B and later B->C
continuation against uninterrupted branches. All thirty characters/supplements/
books, both actor regions, accounting, overlays/flags, pending treasure, context,
stock/bank and RNG remain exact. Input, service/interest and bounded/raw/rejection
draw traces also match. No gameplay injection substitutes for this earned route.

Deterministic rule/flow/fault tests separately cover all classes and thresholds,
successive same-member levels, cap and wrap behavior, reset/refill distinctions,
day 97/98/99 and counter/RNG capacity, resource/actor canonical rejection,
atomic publication and retry boundaries, stale/reentrant/native input, denied
F9, allocation/presentation failure and mutation->reversion guards. Same-member
multilevel and near-cap fixtures are distinct from the earned production route.
Inherited M38/M39/M40 consumer and restart regressions remain required.

## Final acceptance

**COMPLETED AND ACCEPTED.** All required M41 gates passed:

- **Automated deterministic/testing evidence:** normal build, required focused
  M41 tests, complete unfiltered CTest **127/127**, and `git diff --check`.
  Input-responsiveness checks accepted W/A/S/D at 20/20 each, repeated real-clock
  runs spanning 71/72 cosmetic replacements, six redraw-boundary cases at 6/6,
  and representative Space/F-key/Training Enter cases at 15/15. Prior stale-frame
  regressions remain passing.
- **Original-resource/process evidence:** the legitimate seed-7 earned-XP/gold
  route, exact Training/refusal/day/stock results, inherited consumers and
  separate-process save/restore continuation passed, including full encoded-state
  and trace comparisons. Automated SDL/process execution is distinct from
  physical acceptance.
- **Independent technical review:** the implementation and the focused native
  input-scheduling correction received ACCEPT after corrections, with no
  remaining BLOCKER, MAJOR or MINOR finding.
- **Maintainer physical native-SDL acceptance:** a first-press responsiveness
  smoke test and the full connected seed-7 route passed. The maintainer observed
  both L4/XP280 results and exact HP/SP/gold, distinct-member days, refusal without
  extra payment/day, departure to day 11, Quiet F9, full exit/fresh-process
  restore without replay, continued combat/casting, original exit/reset/re-entry,
  return/refusal-only departure to day 12 and responsive physical input throughout.

M41 closes the accepted M39-M41 arc. The scheduled post-M41 broad roadmap
reassessment is now due as a separate task; no successor milestone is selected
or authorized here. See the [roadmap](roadmap.md#near-term),
[stable status](project-status.md) and [concise history](project-history.md#m41---vertigo-training-and-progression).
