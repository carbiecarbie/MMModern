# Milestone 41 - Vertigo Training and progression

## Candidate status and authorized boundary

**CANDIDATE FOR INDEPENDENT SPECIFICATION REVIEW.** This document specifies
future implementation; it records neither implementation authorization nor M41
acceptance. Planning baseline: `main`,
`7d22ac0139fe41e8a68b50b33f75e6733fe78eba`. Initial HEAD, local `origin/main`
and direct remote `refs/heads/main` matched that commit, with a clean worktree
and empty index. [Project status](project-status.md) and the
[closed M40 contract](milestone-40-plan.md) close M40;
[the roadmap](roadmap.md#m41---vertigo-training-and-progression) selects M41
as the immediate next planning unit. No roadmap change is proposed.

Implement the connected loop: fresh Regional Journey, legitimately earned XP,
original Vertigo Training, carried-gold payment, permanent progression and
original reset/refill, distinct-member days, separate departure, further
combat/casting, Quiet save, complete process exit and exact fresh-process
continuation. Include different trainees, returning visits and a stock boundary.
Reuse M40's party economy, world RNG and individual one-day candidates; retain
M39's casting and M38's armor repair.

Exclude other trainers/towns, class changes, skills or spells for purchase,
guild membership, Temple, Buy/Sell, player stock access, general rest or
replenishment, new quests, unrestricted city/Clouds travel, general calendar
processing, Darkside gameplay and selection of a post-M41 milestone. Commercial
resources stay external, read-only and resource-driven. No reference-engine
runtime dependency or commercial dialogue copied into native code is needed.

## Evidence and interpretation

The labels below distinguish five kinds of evidence. A reference interpretation
is not an observation of DOS execution.

| Label | Meaning and evidence home |
| --- | --- |
| Current implementation/test evidence | Baseline source and tests establish existing behavior, not future Training support. Principal owners are `src/games/xeen/XeenCharacter.h`, `XeenCombatInputs.h`, `XeenParty.h`, `XeenCharacterRules.cpp`, `XeenJourneyContent.h`, `XeenServiceDay.{h,cpp}`, `XeenArmorRepair.h`, `XeenVertigoRoute.cpp`, `XeenVertigoWorld.cpp`, `XeenJourneyRules.cpp`, `XeenJourneyCapture.h`, `XeenSaveState.cpp`; `src/formats/xeen/XeenSaveFormat.cpp`; and `src/app/XeenSmithFlow.cpp`, `XeenEncounterFlow.h`, `XeenEventFlow.h`, `XeenGameplay.cpp`. |
| Pinned-reference interpretation | ScummVM commit `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, as required by [dependencies](dependencies.md#pinned-scummvm-revision). Configured source `D:/Projetos/MModern/scummvm-known-good-candidate` was verified at that SHA with clean status; configured build is `D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. Relevant files are `engines/mm/xeen/locations.{h,cpp}`, `character.cpp`, `party.cpp`, `scripts.cpp`, and `devtools/create_mm/create_xeen/constants.cpp`. |
| Read-only original-resource observation | Decoding the installation `F:/Games/gog/Might and Magic 4-5` through the existing archive/map/event/character adapters. Byte sizes and CRC32 below identify decoded resource bytes, not archive-container offsets. No original file was changed. |
| Diagnostic calculation | Detached closure enumeration, geometry/sprite raster checks, arithmetic and an input-driven baseline process probe. These establish bounded planning facts, not prospective M41 acceptance or physical play. |
| Proposed contract/adaptation | The future content-12 admission, native menu, continuation, atomic publication, tests and acceptance requirements specified here. |

Focused test evidence includes `tests/XeenCharacterRulesTests.cpp`,
`XeenServiceDayProcessTests.cpp`, `XeenServiceDayInitializationTests.cpp`,
`XeenCombatClockProcessTests.cpp`, `XeenVertigoProcessTests.cpp`,
`XeenSmithProcessTests.cpp`, the M40 CLI witness/evidence helpers, regional
persistence and save-format tests. Actual executable/CTest registrations are in
`CMakeLists.txt`. These were inspected selectively; no full build or CTest run
is part of this documentation task.

Inherit [M37](milestone-37-plan.md)'s complete city identities, exit/reset graph,
logical-map seams and two-region persistence, [M38](milestone-38-plan.md)'s
repair and exclusive service ownership, [M39](milestone-39-plan.md)'s combat
casting, and [M40](milestone-40-plan.md)'s stock generation, interest, service
context, guard history and schema-9 layout. The stock algorithm is unchanged;
this plan does not reopen its accepted tables or generation proof.

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

A diagnostic built and rasterized geometry/object commands for all 28 cells,
four facings and eight cosmetic phases with the current resolver/composer;
it found no unsupported visual diagnostic. Training and control frame 0 also
decoded and rasterized. This is neither physical visual acceptance nor a claim
that the current executable already admits the route. Future tests also render
the dynamic actor states below. Admit the immutable union during fresh setup
and restore validation, retain it across services/cache reconstruction, and
verify identity before provider references can escape. Missing/corrupt resources
cannot be substituted with generated art or ignored to admit the service.

## Fresh dynamic actor-closure certification

### Method and complete result

Diagnostic calculation used current `XeenIndoorScene::classifyActors`,
`XeenActorApproach::move` and `xeenIndoorActorTerrain`, the decoded complete MOB,
and the exact Event reset positions. It enumerated each actor's
`(x,y,activated)` fixed point over every admitted player cell and facing, in
both initial and reset forms. It allowed arbitrary successive camera choices,
retained stutters and removed other-actor occupancy blockers. This is a
conservative superset of every legal sequence of movement, turns and Wait:
activation depends on geometry/camera, and occupancy can prevent the selected
step but cannot choose a different step. No static-corridor assumption or
single successful route is used as closure evidence.

A second enumeration retained every original/form-specific actor and real
occupancy, varying each influencing actor in turn. It produced the identical
four sets below. The two moving sets are disjoint, so their joint movement
cannot add an occupancy-dependent transition omitted by that enumeration.
Arbitrary-camera enumeration is deliberately conservative about the player's
travel history; the listed sets are the certified canonical closure, not a
claim that every arbitrary camera sequence is a legal play trace.

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
content 12. The current closure cache's repair/nonrepair key is insufficient
for a third route: add a distinct content-12 cache dimension without changing
legacy closure results.

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

Independent arithmetic examples:

| Input | Outcome |
| --- | --- |
| Knight L3, XP 2999 / 3000 | Missing 1 / eligible; threshold is stored XP 3000, not 6000 |
| Tyro or Rebecca L3, XP 3280, gold 870 | Cost 90; L4, XP 280, gold 780 for this single purchase |
| Robber L4, XP 1000 | `B=4000`, `N=8000`, missing 3000, price 160 when eligible |
| Knight L3, XP 9000 | First purchase: L4/XP6000/cost90; next purchase: L5/XP0/cost160; one member-day in this visit, reset/refill on both purchases |
| Knight L9, XP 192000 | Eligible L10 for 810; next request is cap refusal regardless of remaining gold |
| Knight L3, XP `4294967295` | Wrapped current XP 2999, missing 3001; a widened sum would incorrectly admit Training |

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

### Currently omitted temporary inputs

Read-only `maze.chr` verification covered **all thirty records**, not only the
fields the current parser retains. Record-relative temporary resistance bytes
312/314/316/318/320/322 are fire/cold/electrical/poison/energy/magic respectively;
every byte was zero in every record. The current parser explicitly retains
cold/electrical (`313..316`) and poison (`317..318`); omitted temporary fire,
energy and magic therefore need an explicit fresh/resource zero assertion.

Content-12 setup and restore resource admission must validate those omitted
bytes before discarding source inputs. Resource signature verification also
binds them. No admitted fresh preparation, combat, First Aid, Awaken, Magic
Arrow, repair, Event, equipment action or service sets these omitted temporary
values; schema 9 has no corresponding durable input. This closes the fresh ->
play -> save -> restore zero domain. A changed original source containing a
nonzero omitted temporary input is rejected, not silently zeroed. No wire
extension is needed. Modeled nonzero temporary fields remain represented,
guarded and reset according to the matrix; codec tests must exercise them.
Guarding thirty characters does not authorize resetting thirty characters.

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

| Live visit day | Permitted result |
| --- | --- |
| 97 | Admission reserves 98; first new trainee can consume 98 and reserve 99 |
| 98 after that training | Another new trainee is refused before debit; further eligible levels of an already trained member remain possible; departure to 99 stays guaranteed |
| 98 at new admission | Empty/refusal-only visit may depart to 99; no new trainee can reserve a subsequent departure to 100 |
| 99 outside a visit | Refuse entry with no service obligation or time/payment/RNG mutation |
| 9, new trainee | Publish 9->10; privately prepare/reserve the triggering 10->11 stock/interest successor before publication |
| 10, new trainee | Consume the already prepared 10->11 trigger once as the member-day; reserve nontriggering 11->12 departure |

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
test each counter independently at the threshold. Preparation slices, retries
of an unchanged semantic result, idle/cosmetic frames and rejected/stale keys
must not consume additional semantic revisions. Concrete frame tokens remain
fresh but do not spend another operation/input revision for an identical phase.
When another optional action cannot preserve the suffix, show departure-only
mode using the reserved authority. Wrong keys cannot exhaust its last frame.
After admission no counter exhaustion can make the owed departure unpayable.

## Architecture, phases and publication boundaries

### Narrow continuation and native input

Extend existing Event/Service/Presentation architecture, without a parallel
service scheduler or nested SDL loop. `XeenEncounterFlow` owns a transient
Training continuation: lease, selected roster ID, quoted immutable preimages,
operation/revision, trained-member bits, current complete departure reservation,
optional pending level/next reservation, fixed result, published-level marker
and departed marker. `XeenEventFlow` owns menu/presentation state and routes
typed responses to the Encounter owner. The Event owns exclusivity before
service admission and after service retirement. No ownership handoff exposes
Quiet until the final valid exploration/combat presentation is acquired.

Narrow interfaces mirror the existing smith responsibilities: begin Training
with guarded preflight; service bounded preparation; authorize/consume Training
frame; select/quote member; confirm one level; service its candidate; acknowledge
the fixed result; request/settle departure. Detached rule preparation contains
no provider, live mutation or service-loop authority. Add only the explicit
`0x11/5` Event dispatch and content-12 capability at the original cell.

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

### Transition/publication table

| Boundary | Preconditions / consumed authority | Owner deltas and once-only marker | Retry behavior |
| --- | --- | --- | --- |
| Event -> preparation | Current manual record 3, content 12, complete inherited guards, one consumed Event response | Exclusive preparation only; no payment/day/debt | Resource/allocation failure may retry or retire this unadmitted Event with truthful presentation |
| Preparation -> admitted visit | Complete `R(D)`, prepared first frame/resources, suffix budget, unchanged preimages | Acquire Service lease, admitted/debt marker, coordination revision only | Failed subsequent presentation retains admitted debt and reservation |
| Select/quote/refuse | Fresh consumed Training frame, healthy lease, supported exact owners | Transient selected owner/quote/result and operation revision only | No gold, XP, level, reset, day, RNG or marker change; show fresh response |
| Confirm -> pending candidate | Fresh quoted frame consumed before callbacks, quoted preimages still exact | Freeze one operation; detached preparation only | Ordinary failure can cancel this pending purchase back to healthy menu; old departure survives |
| Publish first level for a member | Complete level/reset/refill delta, `M=R(D)`, complete next departure, prepared expected guard/result, enough authority | **One atomic unit:** debit gold, XP debit, permanent level +1, install member-day context/economy/RNG, reset active temporary state, assign selected HP/SP, set trained bit and published operation/result, install next reservation and expected guard, advance coordination | Partial payment/progression/member-day/reset is unobservable. Post-unit failure retains every committed effect and retries presentation only |
| Publish additional level for same member | Same checks except no `M`/next day; retained reservation still valid | **One atomic unit:** payment, XP/level, reset/refill, fixed result/published operation and expected guard; no additional day | Exactly one level/payment; next level requires a new quote/frame |
| Result/refusal acknowledgement | Fresh result frame | Transient phase change; no durable replay | Presentation retry displays retained result; switch-away/back preserves trained bits |
| Publish departure | Debt, complete matching reservation, prepared expected guard, unused departure marker | **Separate atomic unit:** context, wares, bank, world RNG and `departed=true`; no progression/refill | If already departed, skip this unit and retry only settlement |
| Service -> Event -> presentation | Departed marker and guarded classification; mandatory Event work retained | Arrival/activation publication under existing owner, Service release with Event already exclusive, terminal Event retirement, then final presentation | Failures retain committed prefix and save denial; no repeated day/interest/RNG or Event entry |

Sequential levels are independent publications. A failure before the second
preserves the first. Switching members changes no earlier payment/day. Member-day
and departure are distinct units separated by a live owed-departure obligation;
failure there preserves trained results and cannot roll back the visit. Departure
and final display are also distinct; a failed final render is never permission
to charge another day. There is no observable paid-but-unleveled or
leveled-but-unsettled-member-day state inside a successful level unit.

Ordinary preparation/presentation faults retain the healthy continuation and
permit only its valid retry/cancel action. A mutation-history/preimage/resource
integrity violation is permanent failure, with no guard rebasing, recovery to
Quiet or saving. Window close/fatal rendering may terminate the unsaved process;
they do not publish a free departure, auto-save, cancel debt into Quiet or undo
committed levels. The last earlier valid save remains the restart point.

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
full-exit/restore regression coverage while adding Training to those paths.

## Persistence and inherited capability admission

Choose **envelope v4, schema 9, content 12**, adding exactly supported pair
`9/12`. Schema 9 is sufficient: permanent level/current HP/SP/temporary fields
already serialize in characters; XP and resettable combat fields in supplements;
gold/pending treasure, context, economy, RNG, both regions, overlays and flags
already have durable owners. Selection, trained bits, quotes, debt and results
are transient because capture is impossible during a visit. No durable recovery
requires serializing a live service. No new schema, migration, backfill or
implicit upgrade is authorized.

Preserve every schema-9 width/order and the inherited economy layout (1152 stock
bytes plus two u32 balances and its existing presence encoding). Preserve raw
book values, including nonboolean bytes. Change only explicit supported-pair,
content/capability, canonical city closure, route/resources and necessary
content-aware context admission. The original immutable `maze.chr` remains the
checked resource source, never a replacement for saved progressed character data.

Audit these current explicit restrictions deliberately; an arithmetic `>= 11`
substitution is not an admission design:

| Current location | Required successor change |
| --- | --- |
| `XeenJourneyContent.h`: supported pair, `serviceDays`, descriptor, schema and route | Add explicit Training capability/content 12; inherit service/repair/casting capabilities and only content 12's added cells |
| `XeenServiceDay.cpp`: `content != 11` | Accept exactly 11/12; constructor callers must pass the actual content, not depend on its current default 11 |
| `XeenArmorRepair.h::xeenPrepareSmithDeparture` | Admit content 12 to same-year day-98 successor limit; retain contents 9/10 limits |
| `XeenWorld.h::regionalContract8`, `XeenVertigoRoute.cpp` allowlist, `XeenVertigoWorld.cpp` closure/cache and canonical actor checks | Explicitly include the successor and both influencing actors' separate closures; keep legacy cache keys/semantics |
| `XeenLearnedSpellRules.cpp::supportedForContent` | Retain exploration First Aid/Awaken and combat First Aid/Awaken/Magic Arrow for 12 |
| `XeenSaveFormat.cpp::validate`, Journey rules, capture/restore context/resource predicates | Apply M40 day/economy rules to 12 and M41 route/actor/resource rules only to 12 |
| `src/app/XeenGameplay.cpp` fresh Regional Journey selection | Fresh production uses 12; explicit legacy restore/test contracts remain selectable only through their existing paths |

Capture copies exact saved levels, XP, temporary fields, current HP/SP, books,
equipment, context, economy, actors and RNG. Restore validates the complete
snapshot/resources before publication and installs these exact values before
first input. It does not retrain, re-earn XP, refill, reset, clamp current HP/SP,
reseed, regenerate stock, apply interest, advance a day, reopen Training or replay
Events. Deriving display maxima from restored inputs is read-only and must not
overwrite saved current values. First-input tests compare complete encoded state,
not just visible level and date.

Explicitly preserve `8/8`, `8/9`, `8/10`, `9/11` and older supported pair semantics.
They gain neither Training nor the expanded route, omitted-field normalization,
service economy backfill or new calendar authority. Content 12 must pass
continuation tests for ordinary combat, already learned First Aid/Awaken/Magic
Arrow, armor repair, repeated individual service days, stock regeneration,
bank interest and exact restart. New levels feed existing consumers, including
Magic Arrow's level input and subsequent combat XP eligibility/partitioning.

## Earned-production feasibility and acceptance witness

### Baseline-executed fresh prefix

Diagnostic evidence used the real baseline Application/Event/Encounter/SDL
path, original resources and typed actions against presented frames. The driver
observed state to choose movement and legal player actions; it did not inject
XP/gold, edit owners/actors/date/RNG, manufacture items, synthesize recovery or
replace a cursor. It used dummy SDL and is not physical acceptance. Existing
build targets were checked for source freshness by a make dry run; temporary
diagnostics were linked against those objects. No Training code was implemented.

Choose **seed 7**, retaining the production fresh stock-generation prefix.
Unlike M40's witness seed/policy, this route earns sufficient XP for two members
while all six survive. Original raw CHR records start L1/XP0; the existing
intentional Regional Journey preparation supplies these actual gameplay inputs:

| F key / owner | Member / class | Level / stored XP | Missing XP | Initial HP / SP |
| --- | --- | --- | ---: | --- |
| F1 / 0 | Arturius / Paladin | 3 / 1000 | 3000 | 36 / 6 |
| F2 / 18 | Tyro / Knight | 3 / 2000 | 1000 | 48 / 0 |
| F3 / 14 | Badger / Ranger | 3 / 1000 | 3000 | 36 / 6 |
| F4 / 11 | Zippo / Robber | 4 / 1000 | 3000 | 40 / 0 |
| F5 / 1 | Rebecca / Cleric | 3 / 2000 | 1000 | 21 / 21 |
| F6 / 6 | Seymour / Sorcerer | 3 / 1000 | 3000 | 15 / 27 |

Fresh checkpoint: `(23,9,11,West)`, year 610/day 8/minute 480, gold/gems 800/10,
bank 0/0. After fresh stock generation RNG algorithm 1 has state `1652828136`,
count `901`. Seed 7 is the initialization seed, not the post-generation cursor.

Input policy: after each action, settle pending combat/round/reward/Event work
and acquire its fresh frame. In **mainland** combat, Tyro and Rebecca always
Attack at their own PlayerReady turns; Arturius, Badger, Zippo and Seymour use
individual Run on each of their turns until successful. This is not full-party
escape; never remove the last two fighters. It need not make all four escape
before every kill. Run changes participation and XP recipients: two kills here
have three eligible recipients and the others have two. Seymour earns 132 twice;
Tyro and Rebecca earn 1264 each over the seven kills. Do not award every kill to
all six or reproduce these checkpoints with synthetic XP distribution.

For route notation, `U` is forward and `R` is turn right, with the current
facing carried across rows. Use arrow Up/Right (or the corresponding navigation
keys); action `R` during combat means Run, not navigation. Enter acknowledges
and settles combat/results. Never send navigation until exploration is current.

| Destination / action | Navigation string | Minute | Carried gold | Tyro / Rebecca stored XP | RNG state : count |
| --- | --- | ---: | ---: | ---: | --- |
| `(7,7)` well | `RRRUUURUURRRU` | 540 | 800 | 2000 | `1652828136:901` |
| Use original well | Space then F1 and settle; repeat Space/F2, Space/F3, Space/F4, Space/F6, settling each; **skip F5** | 540 | 800 | 2000 | Unchanged |
| `(7,11)` | `RRUUUU` | 581 | 800, with pending 10 not yet collected | 2132 | `2146145226:916` |
| `(5,12)` | `URRRUU` | 613 | 810 | 2332 | `2148587634:955` |
| `(3,12)` | `UU` | 638 | 860 | 3064 | `1341183770:1029` |
| `(1,13)` | `UURU` | 671 | 870 | 3264 | `2762662790:1060` |
| `(10,13)` | `RRURRRUUUUUUUUURRRU` | 781 | 870 | 3264 | Unchanged |

The well is the existing once-per-owner original +25 HP opportunity (flag 16),
not SP recovery or an invented rest. Above-maximum HP is retained. The seven
mainland kills are original slots `0,2,4,5,7,8,9`; all other mainland identities
remain retained, including unreachable island actors. This policy requires no
new travel, item sale, loot manufacture or inventory expansion. Original gold
collection supplies the final 870 purse; pending treasure must remain distinct
until its normal collection point.

The executed prefix produced no broken armor or acquired item prerequisite;
inventory pressure does not block it. Retained M40 repair coverage therefore
uses its own accepted earned-damage witness, also exercised under content 12.

At `(10,13)`, cast Rebecca's already learned First Aid on herself: C, F5,
Down to First Aid, Enter to choose, fresh Enter to confirm, F5 target, then
acknowledge/settle. Her HP 9 becomes 15, SP 21 becomes 20; minute becomes 791.
Face North with right turns, Space and Yes for the inherited city entrance.
Advance to `(28,15,4)` and use Attack for **all** members against the entrance
Slime, acknowledging/settling each phase. This baseline prefix executed to:

```text
day 8, minute 796, camera (28,15,4,North)
gold/gems 870/10; pending mask/gold 0/0; bank 0/0
RNG algorithm 1, state 799325555, count 1101
active IDs:  0    18    14    11     1     6
HP:         58   67    58    62    11    35
SP:          6    0     6     0    20    27
stored XP: 1016 3280  1016  1016  3280  1280
levels:      3    3     3     4     3     3
all conditions zero; initial-form entrance Slime 35 accounted dead
```

The route thus establishes XP, funds and survival without new prerequisites.
No service day is assumed to heal anybody. Its total time remains before the
next 960-minute ordinary processing boundary. Do not substitute M40's accepted
route or seed without recalculating participation, survival and RNG.

### Prospective M41 continuation and independent projections

The baseline stops at the currently admitted `(15,4)` boundary. Everything in
this subsection is a proposed future witness/reference-derived projection,
not an already passed M41 process or native test.

1. Follow the twelve-step route to `(10,11,North)`, automatically acknowledge
   the label at `(10,8)` and acquire Quiet. Expect minute **808**, same RNG and
   gameplay values as above. The extra small Slime may move/activate but cannot
   cause contact, damage or RNG consumption. Save checkpoint **A** here before
   opening Training. This is the branch point for exact paid-progression replay.
2. Space opens Training. F2/Enter/fresh Enter trains Tyro; acknowledge result.
   Expect permanent L4, XP280, gold780, HP64/SP0, day9/minute808. Other HP/SP
   values are unchanged. The UI must show the 90 cost and HP overwrite 67->64.
3. F5/Enter/fresh Enter trains Rebecca; acknowledge. Expect L4, XP280, gold690,
   HP28/SP28, day10/minute808. This demonstrates useful healing and SP refill,
   derived maxima rising 21->28, different-member bookkeeping and a second day.
4. Switch back to Tyro and request the next quote. Missing XP is 5720, next
   cost would be 160; refusal adds no day/payment/reset. Escape from the menu
   settles the **separate** departure to day11/minute808. Capture checkpoint
   **B**, with both L4/XP280 results, gold690 and the complete actor/context state.
5. This departure crosses the stock boundary. An independent detached call of
   the existing M40 candidate, starting at the projected cursor above, produces
   ending RNG `2959920300:2009`; stock-byte CRC32 is `79dec2de` in the inherited
   side/shop/category/slot/M-ID-state-frame order. Bank remains 0/0. This is a
   diagnostic projection of unchanged stock code, not an independent new stock
   algorithm proof. Future evidence must compare complete bytes and requests;
   a matching checksum alone is insufficient. Exactly one generation/interest
   operation occurs across the two member-days and departure.
6. From B, continue down the same corridor to `(15,0,South)`, use the original
   exit and settle its legitimate reset to `(23,10,12,South)`. Turn twice and
   move forward to `(23,10,13,North)`, then Space/Yes to re-enter. Advance to the
   reset-form entrance Slime. All members Block until Rebecca has HP below her
   new maximum and reaches her own PlayerReady turn. She casts First Aid on
   herself once. Thereafter Block until Seymour's next PlayerReady turn; he
   casts Magic Arrow at target 1, repeating on his later turns if the Slime
   survives, while the other members keep Blocking. Never use individual Run
   or a physical attack in this encounter. This policy orders useful recovery
   before the killing spell and retains all earned XP, wounds and SP costs.
   Combat casting uses C for the current actor, arrows to the named already
   learned spell, then Enter to review. First Aid takes a fresh confirmation
   before its post-debit F5 member target. Magic Arrow takes its precommit
   enemy choice 1 before final confirmation. Settle each spell result normally.
7. Return to Training along the same route. Select both progressed members,
   show their actual insufficient-XP refusals, then depart with no successful
   training. Expect one additional departure day, **12**, no additional stock
   generation/interest/RNG for the service itself, and no refill from the empty
   visit. This is checkpoint **C** after further gameplay. Refusals use current
   XP (a six-recipient Slime kill at these levels grants 16 each), not hard-coded
   menu text. Display all actual postcombat values.

This supplies later-return and stock-boundary continuity without requiring
another 5720 XP or pretending that same-member multilevel training is reachable
in the short production route. Repeated levels and near-cap behavior have
independent deterministic rule/flow fixtures, clearly separated from the
earned witness. Casting Awaken on its existing supported targets/refusal path,
repair and the full retained M39/M40 scenarios remain required regression
evidence, not new recovery assumptions for this prefix.

### Full process split and physical controls

For A and B, press Quiet F9, verify the actual save write, completely exit the
process, start a new process with that save and compare **before first input**
against the captured snapshot. Replay A->B paid progression against an
uninterrupted A branch; replay B->C gameplay and return service against an
uninterrupted B branch. Compare full encoded snapshots and every durable field,
including all thirty owners/books/supplements, both actor regions/identities,
activation/wounds/deaths, overlays/flags, pending treasure, context, all stock
bytes/bank, and RNG algorithm/state/count. Record and compare bounded-request
and raw/rejection draw traces plus individual service-call/interest traces.
Do not accept matching only final level, date, checksum or displayed HP.

Use the exact input policy above, fresh-frame gating and normal idle scheduling.
The diagnostic driver yielded idle instead of issuing a command every third
loop iteration, allowing scheduled combat presentation to settle; it did not
alter gameplay time or RNG. Future process helpers must likewise avoid input
starvation and must not call private gameplay mutations to reach checkpoints.
Record the actual input/event/request trace and checkpoint bytes as evidence.

Future native launch commands, verified against current `src/main.cpp` parsing:

```powershell
.\build\mmodern.exe --journey-region --combat-seed 7 'F:\Games\gog\Might and Magic 4-5' --save-file 'D:\Projetos\MModern\mmodern\build\m41-acceptance.mmsave'
.\build\mmodern.exe --load-game 'F:\Games\gog\Might and Magic 4-5' 'D:\Projetos\MModern\mmodern\build\m41-acceptance.mmsave'
```

Installation precedes save file on restore. Native physical acceptance uses
the real SDL window, with dummy video/automated test injection disabled. Arrow
navigation, Space interaction/Attack, B Block, F1..F6 selection, C casting, Enter
confirmation/acknowledgement, combat R Run, Escape service departure and Quiet F9 are the
required visible controls. Automated input/screenshots do not replace the
maintainer physically playing, reading results and performing save/exit/restore.

## Validation and acceptance boundary

### Deterministic, fault and canonical tests

Add the smallest focused Training rule and Flow tests, with literal independent
expected values, then integrate the existing actual targets from CMake:
`mmodern_character_rules_tests`, `mmodern_combat_casting_rules_tests`,
`mmodern_learned_spell_rules_tests`, `mmodern_service_day_initialization_tests`,
`mmodern_service_day_process_tests`, `mmodern_combat_clock_process_tests`,
`mmodern_vertigo_original`, `mmodern_vertigo_process_tests`,
`mmodern_smith_process_tests`, `mmodern_regional_persistence_tests` and
`mmodern_save_format_tests`. Preserve their legacy contract variants.

Required cases are:

- Every class, L1/3/4/9/cap, XP threshold-1/exact/+1, u32 wrap examples,
  invalid level/enum and checked derived-stat overflow. Temporary-level
  independence, retained XP remainder, successive-level thresholds, exact/
  insufficient funds and cap/XP/condition/gold/support-limit precedence.
- Reset matrix with distinct nonzero modeled temporaries on selected,
  nonselected active and inactive owners; preserve temporary age, conditions,
  permanent fields, every equipment byte and raw books. Selected above-maximum
  overwrite, nonselected above-maximum preservation, noncaster SP zero and
  post-reset rather than pre-reset derivation. Reject nonzero omitted source
  resistance bytes before discard; test all thirty original records.
- One day per distinct owner per visit; repeated level and switching away/back;
  later visit resets bookkeeping; rejected/cancelled purchases set no bit;
  zero-training departure still one day. No automatic healing from any day.
- All 28 cells/facings, original collision/ceiling/seam/labels and exact Event
  graph/resource signatures. Independently regenerate actor fixed points for
  both forms and compare complete sets, including off-route influence, every
  dormant original actor, real occupancy, blocked small-Slime joining and
  original exit/reset/preserve conditions. Corrupt each relevant signature,
  Event edge, actor identity/position/activation/wound/death/accounting state.
- Complete reservation chaining across 9->10->11 and 10->11->12; stock is
  generated once even if its candidate changes role from departure to member
  day. Bank interest follows each triggered stock call. Exercise nonzero bank
  rounding/wrap through inherited vectors. Compare full candidate preimages,
  RNG/request order and repeated one-day calls, never an aggregated charge.
- Admission at days 97/98/99; first/new/already-trained members near the end;
  every authority counter threshold; generation/rejection-draw/RNG exhaustion
  while reserving mandatory departure. Refuse an optional level with the old
  complete reservation still executable. No optional key drains suffix budget.
- Faults before/after payment/progression unit, member-day, next-level quote,
  member switch, result draw/present/acquire, departure unit, classification,
  Service/Event release, Event retirement and final frame. Test allocation,
  provider, stock-complete/bank-prepared, hook and native presentation faults
  on normal and throwing paths. Assert atomic boundaries have no observable
  partial state, later faults preserve committed prefixes, and retries never
  duplicate XP debit/payment/refill/day/stock/interest/RNG or release free debt.
- Held/repeated/batched/stale/wrong keys, identical semantic phase with a new
  frame, reentrant callbacks, cancel/close/fatal render and F9 at every pending
  phase. Assert zero capture/provider/path/I/O calls on denied F9, including
  pre-admission preparation and post-departure unpresented handoff.
- Full mutation->reversion probes for every guard family, including inactive
  character/supplement, raw books, pending reward, bank/stock, camera, both
  actor regions, flags/context/RNG and immutable resource caches. Test insertion
  and reconstruction before references escape, authorized guard replacement,
  unchanged-value assignment and exceptions. Retain the M40 scheduling/cosmetic
  combat-clock regressions after Training and after complete process restart.
- Encode/decode/canonical rejection for 9/12, progressed levels/XP and all
  temporary/current fields, exact economy and actors, missing/extra owners,
  invalid pairs/resources and malformed/truncated bytes. Restore before first
  input performs no training/reset/refill/clamp/time/stock/interest/Event replay.
  All 8/8, 8/9, 8/10 and 9/11 route/time/casting semantics remain unchanged.

### Separate acceptance classes

Implementation closure requires all four classes, with evidence attributed to
the correct role:

1. **Automated implementation validation:** normal project build and complete
   unfiltered CTest, with no failing required test. Targeted tests are sufficient
   during iteration, not at milestone closure.
2. **Original-resource/process evidence:** the earned seed-7 route, paid results,
   distinct-member/departure days, return and stock boundary, native feedback,
   continued casting/combat, real Quiet save and separate processes, exact
   pre-input restore and uninterrupted-branch comparison. No injected gameplay
   state may substitute for this class. Retain M40/M39 inherited process evidence.
3. **Independent technical implementation review:** rule arithmetic, closure,
   resource domain, owner/guard authority, reservation/atomic failure semantics,
   persistence, test independence and witness provenance reviewed against this
   specification and the actual implementation candidate.
4. **Maintainer-performed physical native-SDL acceptance:** personally execute
   the connected route, read cost/level/maxima/refill/refusal/departure feedback,
   verify input modality, continue play, F9, full exit and restore. Automated
   clicks, screenshots and reviewer source inspection do not establish this.

The planning diagnostics establish feasibility and reference/resource facts;
none of these four future M41 acceptance classes is claimed complete here.

## Future implementation sequence and scope review

After separate implementation authorization:

1. Add explicit content-12/resource/route/actor admission and its independent
   closure/canonical tests; preserve all legacy capabilities and save meanings.
2. Add detached Training arithmetic/reset/refill preparation on existing owners,
   with independent numerical/matrix tests and omitted-input zero validation.
3. Add the narrow Training continuation and chained complete day reservation;
   implement prepared expected guards, nonthrowing publications and finite
   authority suffix checks before connecting the native controls.
4. Connect original Event 3/action 5, resource-driven menu/results and main-loop
   preparation/presentation; close modal/F9/retry/ABA/native-failure tests.
5. Enable fresh 9/12 selection, verify exact capture/restore and inherited
   combat/casting/repair/day consumers, then execute the earned process witness
   and the separate review/physical acceptance requirements above.

No material roadmap deviation is established. The twelve added cells, one
additional blocked Slime influence, active-only reset with omitted inputs proved
zero, schema-9 ownership, and earned seed-7 witness fit the accepted M41 unit.
The reference member-day-before-reset ordering and reservation chain are
resolved details, not scope expansion. Broad roadmap review remains after M41;
this plan chooses no successor consumer.

Reopen only on demonstrated contradiction: materially wider actor/Event
influence, a reachable unrepresented temporary input, loss of the earned
survival/funding route, or a genuinely new durable owner/acceptance prerequisite.
Report the exact evidence and smallest bounded follow-up instead of adding
travel, recovery, purchases or migration silently. Ordinary implementation
difficulty is not such a trigger. This candidate has no unresolved material
formula, reset-population, actor-admission, reservation, persistence or earned-XP
feasibility decision.
