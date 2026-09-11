# Milestone 26 - Visible original actor approach and engagement

**Milestone 26 is complete. Stages 26A and 26B are accepted. Independent
technical review and maintainer physical acceptance are complete.**

## Final objective and bounded encounter

M26 delivers a resource-derived outdoor actor, original-style activation and
bounded delayed approach, and readable terminal same-cell engagement before
combat. M26A supplies actor authority and the deterministic approach domain;
M26B integrates production composition, Flow timing/input and session guards.
The public entry is `mmodern --encounter-26 <game-directory>`.

The diagnostic uses Clouds map 20, original monster record 5 initially `(13,2)`,
Skeleton type 8, party entry `(13,1)` North, and the four-cell envelope
`x=13..14, y=1..2`. It uses the World of Xeen Clouds-side behavior profile and
original Adventurer difficulty. Reference controls use default options with
cheats disabled. Resource availability does not silently select another profile.
This does not certify normal-start navigation, map-20 travel or combat.
Commercial resources remain external and unmodified.

[Project status](project-status.md) owns the technical snapshot;
[history](project-history.md) records completed milestones and the
[roadmap](roadmap.md) owns the provisional M27 -> M28 direction.

## Source and reference provenance

The reference is ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
[Dependencies](dependencies.md) owns configuration and licensing provenance.
Original-resource observations and source-derived caller traces establish the
resource, activation, timing and stopping contracts below.

Bounded extracted-harness evidence exercised unchanged bodies of
`Combat::moveMonsters`, `canMonsterMove`, `moveMonster`,
`InterfaceScene::setOutdoorsMonsters` and `Interface::chargeStep`, with pinned
position/grid constants. Stand-ins supplied containers, camera, terrain and draw
slots. A separately written driver reproduced selected input/countdown branches;
time addition was arithmetic, ranged execution trapped and status-overcome was a
no-op for the admitted physical states. Full `perform`/`draw3d`, party-time,
rendering and SDL bodies were not executed by that harness. Caller scheduling
was checked against source, not established by the driver's own results.
**No full ScummVM encounter execution is claimed.** Production MMModern evidence
and maintainer native SDL acceptance are recorded separately below.

### Source provenance

All reference links are pinned. Symbols, rather than temporary probe paths, are
the reproducible authority:

| Source | Material contract |
| --- | --- |
| [map.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp), `MobStruct::synchronize`, `MonsterStruct::synchronize`, `MonsterObjectData::synchronize`, `Map::load` | Record layout, sprite indirection, live initialization, actual map-load reset, archive/profile selection |
| [interface_scene.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface_scene.cpp), `OutdoorDrawList`, `drawScene`, `setOutdoorsMonsters`, `animate3d` | Activation, ordered attacker slots, anchors, scale, frame selection and composition order |
| [combat.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp), `moveMonsters`, `canMonsterMove`, `moveMonster`, `monsterOvercome` | Two-pass spatial scan, one move per record, occupancy capacity and excluded effects |
| [interface.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp), `startup`, `perform`, `checkMoveDirection`, `chargeStep`, `stepTime`, `doStepCode`, `draw3d`, `doCombat` | Input, delayed work, immediate wait, actual stopping edge before combat setup |
| [party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp), `synchronize`, `changeTime`, `addTime`, `handleLight` | Initial context, eight-hour condition boundary, new-day and temporary-effect constraints |
| [xeen.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/xeen.cpp), startup and `gameLoop`; [events.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/events.cpp), `pollEvents`, `ipause5`; [events.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/events.h) | Movement enablement, automatic-event caller, 50 ms reference game-frame counter and two-frame drawing pause |
| [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp), `SCREEN_POSITIONING_X/Y`, `OUTDOOR_MONSTER_INDEXES/Y`, `MONSTER_GRID_BITMASK` | Literal spatial/projection constants; movement grid arrays are in `combat.cpp` |
| [cc_archive.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/shared/xeen/cc_archive.cpp), [files.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/files.cpp), [sprites.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/shared/xeen/sprites.cpp) | CC decoding, initial archive construction, original sprite decoder and clipping |

## Original resources and admission

### Resource observations and isolation

The initial Clouds archive is the concatenation, in order, of present XEEN.CC
members `2a0c,2a1c,2a2c,2a3c,284c,2a5c`. This installation lacks `2a5c`;
skip an absent block as the existing bridge does. The outer payload uses XOR
`0x35`; the inner archive's member payloads are already plaintext. Do not use a
user's original save as the initial archive.

| Member | Size | Observation |
| --- | ---: | --- |
| Initial `maze0020.mob` | 200 | Seven ordinary records; 27 monster records. Monster table begins `8,9`, then fourteen `255` entries. Record 5 is signed `(13,2)`, local slot 0, direction byte 0, resolved type 8. |
| Initial `maze0020.dat` | 892 | Map ID 20; neighbors N/E/S/W `19,24,0,16`; flags 0, flags2 `0x8000` (outdoors). Each admitted cell has word `0x0031`, attributes 0: surface index/type 1, middle 3, no cell hazard/automatic-event bits. |
| Initial `maze0020.evt` | 128 | Sixteen records; addresses only `(4,4)`, `(5,14)`, `(9,14)`. No admitted-cell event in any facing. |
| DARK.CC `xeen.mon` | 5400 | 90 complete 60-byte records. Type 8 has HP 20, image 8, nonflying, no range/special attack, loopAnimation 0, animationEffect 0. |
| XEEN.CC `008.mon` / `008.att` | 35946 / 22004 | Eight normal frames / four attack frames. M26 uses only the normal resource. |
| Initial `maze.pty` | 812 | Counts 6/6; active owners `0,18,14,11,1,6`; Adventurer 0; day 1, year 610, minutes 480, ctr24 0, rested 0. |

Compact member SHA-256 anchors (decoded member bytes, not entire archives):

```text
maze0020.mob 443f7b285ba4c92874f0d6f9bcf200293a44c97dceb39619be9b63c3f2122461
maze0020.dat 72dee66b3cd65df5a01d844ff3730950ddc26fd6821538fcc57e3d776995916c
maze.pty    82c14b2068e896e0d7a9c05b9d12b3743a82b69a89b5f084f99f631fa437cd52
xeen.mon    fd04e2408a444cb9ff8b3f5f16badfc6733bc3d973e56c3275edfa787daef52b
008.mon     a8cb2b23711cbf1bc54bb9fddcaaca135f19dae5ddce517a6c0b28129e9b6d59
```

The complete monster placement control is below; indices are zero-based physical
monster-record positions, not table slots. Repeated coordinates are intentional.

```text
type 8:
0 (3,3)   1 (3,5)   2 (5,5)   3 (5,3)   4 (3,8)   5 (13,2)
6 (15,8)  7 (10,6)  8 (8,9)   9 (6,14)
10,11,12 (15,15)    13,14,15 (11,15)     16 (13,14)
type 9:
17,18 (8,15)       19 (3,4)  20 (4,5)  21 (5,4)  22 (4,3)
23,24 (6,9)        25 (1,13)             26 (15,8)
```

Across all four admitted party cells, the union of the reference seven-by-seven
scan is `x=10..17,y=-2..5`. Only record 5 occurs there. The closest northern
control, record 7 `(10,6)`, is outside even at `(13,2)`. The outdoor activation
queries reach at most three forward and two lateral cells; none of the other
records activates for any admitted facing. Neighbor map geometry may be sampled
for scenery, but the reference's current MOB supplies actors, not adjacent-map
MOB lists. Do not turn existing neighbor terrain sampling into actor migration.

The extracted harness checked every party cell/facing with record 5 in every
envelope cell: 64 configurations. No bystander activated. A stronger movement
control marked all bystanders attacking and still verified that all 26 stayed at
their original coordinates, because they were outside the scan. This artificial
activation is a test control, never production initialization. The target's
preferred moves reduce a coordinate difference toward the party, and its legal
destinations remain in the rectangle. Therefore repeated admitted transitions
cannot recruit a new actor; isolation is not just a startup assertion. Preserve
every original record and include every in-range coordinate in occupancy.

South of `(13,1)` and west of `(13,1)` are real space cells, word `0x000f`.
The east continuation `(15,1)` and north continuation `(13,3)` are ordinary,
passable `0x0031` cells outside the diagnostic envelope. This distinction supplies
real terrain-blocked and diagnostic-boundary controls without inventing a wall.

## Resource interpretation and admission

Keep the existing DAT/MOB parsing behavior, including original ordering,
unresolved placeholders and disabled coordinates. DAT is exactly 892 bytes:
256 little-endian uint16 cell words, 256 attribute bytes, followed by the existing
checked map metadata. No monster interpretation belongs in DAT terrain flags.

MOB starts with three 16-byte tables (objects, monsters, wall items). Records are
four bytes: signed int8 X, signed int8 Y, uint8 table index, uint8 direction.
Termination tests X=`-1` and index=`255`, independently of Y/direction. Empty
object/monster lists use two markers; wall lists use one. Existing parsing rejects
truncation, missing markers, trailing bytes and nonterminator indices >=16;
retain that behavior. Monster tables compact non-255 entries before local-slot
lookup, unlike the ordinary object table. Keep physical record index even when
an entry has no resolved type; do not copy reference omission into index drift.
M26 admission requires resolved identities for every actor that could affect it.
Reject encounter admission above the pinned movement capacity of 107 monster
records (`combat.h::MAX_NUM_MONSTERS`); do not truncate the original list or let
record indices overrun fixed movement scratch arrays. Generic immutable parsing
need not discard otherwise structurally valid larger lists.

`XeenMonsterFormat` is a checked parser for fixed 60-byte statistics records:

| Offset | Representation / meaning |
| ---: | --- |
| 0..15 | Name bytes; bounded display as reference (byte 15 terminates display); retain raw bytes |
| 16 | uint32 LE XP, opaque to M26 |
| 20 | uint16 LE base HP |
| 22..25 | uint8 AC, speed, attack count, preferred class; opaque to M26 |
| 26 | uint16 LE strikes; opaque |
| 28..33 | uint8 damage per strike, damage type, special attack, hit parameter, ranged flag, monster category |
| 34..41 | Eight uint8 resistance/unknown fields; opaque |
| 42 | uint16 LE gold; opaque |
| 44..50 | uint8 gems, item drop, flying flag, image number, loopAnimation, animationEffect, FX |
| 51..59 | Nine bounded attack-voice bytes; not loaded or played |

There is no count header or terminator. Require nonempty exact multiples of 60
within the CC member bound of 65535 bytes, sufficient records for every resolved
type used by admission, and no implicit extra EOF record. Preserve opaque fields;
do not validate future combat enums by indexing unimplemented tables. Interpret
only name, base HP, image and the flags needed for movement/visual admission.
Zero HP, missing type/image, unsupported flags or unsafe normal sprite data on a
relevant actor are explicit admission failures. Out-of-domain records remain
available as original metadata, not silently simulated or converted to Skeletons.

Identity chain: `(Clouds,20,monsterRecord=5)` -> raw local slot `0` -> compact
table type `8` -> statistics record `8` -> image `8` -> `008.mon`. Only the
diagnostic selection may name map/record/entry; the engine must derive type, HP,
name and artwork. Do not embed the commercial record, restrict a generic parser
to one hard-coded name, or create a special Skeleton entity class.

The existing lazily owned Dark archive exposes an explicitly named checked
`readCloudsMonsterStatisticsFromDarkArchive` operation through AssetSource/bridge.
No fallback to XEEN.CC, standalone-Clouds tables, dark.mon, a user's save, or a
different rules profile. Missing archive/member is distinct from malformed data;
both prevent this encounter startup. Validate indexed extent and complete read
before upstream assertion/fatal short-read paths, as the checked material-name
read already does. Bound index/member handling for this path; do not overhaul
unrelated archive access. `.mon` uses the existing XEEN sprite cache, all eight
frames preflighted before admitting this session and revalidated on reconstruction.
Do not require or load `.att` at runtime: M26 cannot select frames 8 and above.

Diagnostic admission is fresh-session-only, Clouds map 20 at the fixed entry,
with all original MOB records retained. Require ordinary-ground/no-event/no-hazard
properties throughout the four cells, plus isolation for all facings/transitions
as above. Resource hashes are reproducibility anchors, not a substitute for
structural/admission checks or new save fingerprints. A changed installation that
violates these properties fails with a specific support diagnostic; do not repair
or prune it. Matching hashes are not needed for ordinary non-encounter gameplay.

Load the original roster through the existing loader. Admit exactly the original
six unique owners, all sixteen conditions zero, positive current HP, the original
initial gameplay context, no active party buffs/resistances/light/water-walking,
and no pending rewards/events. Initial observed HP/SP in active order are
`12/2,16/0,12/2,10/0,7/7,5/9`. No item normalization or combat-effect calculations
are needed: inventory is closed and mutation is not admitted during this mode.
Other sessions keep accepted alias/opaque-item/legacy-load behavior unchanged.

## Final ownership and lifecycle

| State/value | Owner and lifetime |
| --- | --- |
| Original-order live actors | `XeenSessionWorldState` / `XeenWorld`; immutable original metadata and owned statistics, signed live coordinates, signed 32-bit HP, activation, Present/Disabled/Unresolved lifecycle and Physical/Unsupported status. Initialized once; never stored in disposable DAT/MOB caches. |
| Monster identity | `XeenMonsterIdentity`: side/map and original monster-record index, distinct from ordinary object/event identity, sprite/type and visible slot. |
| Roster, membership, items, HP/SP, conditions and quest state | Existing `XeenPartyState`; no duplicate party or M26 combat mutation. |
| Admitted gameplay context | Optional `XeenPartyState::encounterContext` (`XeenGameplayContext`): explicit profile/difficulty, day/year/minutes, ctr24, rested/newDay and checked effects. Ordinary loading does not install it. |
| Committed camera/game flags | Existing Application/session owners, borrowed by Flow; flags remain unchanged in this domain. |
| Irreversible encounter marker, revision and terminal latch | World session authority. Marked before admission can expose gameplay; failure, cache discard and presentation cleanup cannot make the session saveable. Only destruction of the session ends it. |
| Phase/reason, pending count and owner/revision bindings | `XeenEncounterState`, coordinated by `XeenEncounterFlow` under `XeenEventFlow`; transient coordination over the same world/party/camera. |
| Input-cycle identity, retained results/tickets, deadlines and normal-frame phase | Transient encounter coordination; no second scheduler, actor collection, world or party. Flow retains ownership of ordinary-object animation. |
| Resources, occupancy, view slots, commands and pixels | Derived values and existing `XeenAssetSource` / `ScummVmXeenBridge` caches, decoder and drawing facilities. Reconstruction never initializes, resets or advances live actors/context. |

Unresolved/disabled records retain original identity and signed coordinates.
Every valid `0..31` coordinate participates in occupancy; invalid coordinates
never index arrays. Actor lifecycle does not use `disableObject`, `applyRemove`
or event-disable sets. M26 does not damage or remove an actor.

`XeenActorApproach::initialize` is the one-time synchronous publication boundary;
`initializeFromResources` reads explicit statistics/context/event inputs through
existing providers. A failed attempt may leave only the irreversible safety
marker. A second initialization cannot replace live actors or reset revisions.
The compositor uses pure classification; only domain startup and logical
transitions publish activation. Cache rebuilds cannot activate or engage actors.

Actual map entry/load is distinct from cache reconstruction. M26 admits one fresh
entry only, with no teleport, re-entry, resume or live map replacement. A fresh
session creates new owners and original positions/HP. Exit discards unsaved state.

### Activation, occupancy and exact stop

`setOutdoorsMonsters` first clears derived attacker slots, then scans records in
original order. Matching a projection query sets `_isAttacking=true`, even before
rasterization and even if later terrain covers its pixels. Leaving the view does
not clear it. This is an activation latch, not proof of an attack already executed.

Query 2 has offset `(0,0)` for every facing. Its first three matching records fill
attacker slots 0,1,2 (draw orders 118,112,115). Query 7 is one cell forward and
fills slots 3,4,5; it activates the actor but does not enter melee combat.
Queries `5,9,14,12,16,27,25,23,29,31` account for the remaining outdoor activation
positions. Ordered selection and activation do not depend on pixel visibility.

After scene classification, `draw3d` calls `doCombat` when any slot 0..2 is occupied
and interactive/sleeping mode permits it, with movement enabled and no ranged
attack/shooting in progress. **M26 stops at this edge, before `doCombat`** changes
mode, creates a combat party, sets speeds or calls `nextChar`. It latches
`Engaged` with the selected original identities. In the admitted domain this is
exactly record 5 sharing the party cell. HP remains 20; party HP/SP/items remain
unchanged. There is no adjacent-cell substitute and no extra approach step.

Movement builds a 32x32 occupancy count from all records with both coordinates
in `0..31`, then makes two passes: y difference `+3` down to `-3`, x difference
`-3` up to `+3`, original record order innermost. A per-pass-operation moved set
persists across both passes, so a record moves at most once. A destination must
have fewer than three actors; physical damage-state and movement-enable checks
apply. A blocked primary terrain direction permits the reference fallback;
occupancy failure inside `moveMonster` does not itself cause fallback.

For N/S, prefer one x step toward the party when x differs, otherwise one y step;
fallback is y (or x in the same-row band). E/W reverses those priorities.
The center delta is zero. Do not turn the two passes into two steps per monster,
or treat zero delta as a fresh approach. `canMonsterMove` uses destination middle
and surface, not the party collision abstraction: middle `0,2,3,4,5,6,8,11,13,14`
uses surface/flying/type-59 tests, other values compare to wallNoPass. Only the
nonflying Clouds ordinary-ground branch is reachable here. Unsupported reachable
terrain, flying, range or status behavior refuses admission before publication;
it is not approximated using the party's collision rules.

## Domain operations and publication

`XeenActorApproach::classify`, `occupancy` and `move` prepare values without live
publication. `action` and `pulse` are separate synchronous authoritative
publications, not one rollback transaction. No public apply-prepared-result API
escapes the domain. M26B consumes these operations without duplicating movement.

Resource calls, actor copies, terrain checks, validation and fixed result facts
are prepared before nonthrowing actor-vector swap and camera/context/count/revision
stores. The caller adopts each result before attempting a later pulse, timing
coordination, reporting or composition. A charged action's count 3 remains an
observable publication even though its supplied pulse normally returns count 2.

Owner addresses, entry coordination facts and world revision are retained across
provider callbacks and revalidated immediately before mutation, including failure
stops. Reentrant initialization or transitions cannot be overwritten by older
work. Stale callbacks cannot stop or retire newer pending work. Current
authoritative failures publish their legitimate support stop and retire pending
work; completed earlier action publications remain intact. Engagement/support
states remain terminal.

Flow serializes dispatch/idle and retains generation-bearing tickets through
composition, reporting, return-frame copying and Application/SDL handoff. Fallible
feedback never rolls back or repeats an action/pulse. A current composition
failure may attempt one clean presentation rebuild from live authority, without
replaying failed reporting or gameplay. Failed recovery/upload exits through fatal
presentation handling. A stale failure cannot borrow newer authority to justify
recovery or stopping it. Startup failure exposes no partial interactive window.

## Actions and independent timing

### Bounded original context

PTY offsets are zero-based: difficulty byte 27; after 28 bytes plus 576 bytes of
Clouds shop records, ctr24 uint16 LE at 610, day at 612, year at 614, minutes at
616. Light/torch and elemental party resistances occupy words at 620..630;
rested is byte 658. Relevant initial party-effect bytes 18..26 are zero.
Read checked prefixes through 658; no new shop model is required. Source startup
sets `newDay = minutes < 300`, hence false at 480; daytime is also established.

Admit the closed-open minute window **[480,960)** on day 1/year 610. Outdoor
successful steps and wait cost ten minutes. The first refused charge is
950 -> 960 (16:00), not midnight: `Party::changeTime` compares integer divisions
by 480 and enters condition/stat-death processing on that crossing, including
reference poison/disease random branches even for zero conditions. Do not
silently omit those effects or consume their RNG. Earlier confusion/paralysis
per-charge branches are excluded by the all-Good admission; newDay is false;
all light/buff counts and cell hazards are absent. Thus simple checked minute
addition is sufficient inside this window. Turns increment ctr24 modulo 24 via
`stepTime` without adding minutes; successful steps and wait also increment it.
Actual blocked steps do neither. No calendar rollover or general elapsed-time
simulation is added. Refuse a charge that would leave the window before camera,
time, old pending movement or ctr24 publication; enter terminal `SupportStopped`.

### Action and pulse contract

| Operation while Exploring | Camera/time/ctr24 | Authorized monster work |
| --- | --- | --- |
| Successful Forward/Backward inside envelope | Destination camera, +10 minutes, ctr24 +1 modulo 24 | Flush old opportunity using the destination camera, then arm count 3. Separate post-action pulse decrements to 2 and classifies/activates/tests engagement. |
| Left/Right | Facing changes, minutes unchanged, ctr24 +1 modulo 24 | No new opportunity; the supplied pulse may consume old work and classifies the new view. |
| Actual blocked Forward/Backward | No camera, minutes or ctr24 change | No new opportunity; the normal supplied pulse may process old pending work. Readable nonmodal collision feedback follows adoption. |
| Period Wait | Same camera, +10 minutes, ctr24 +1 modulo 24 | Flush old opportunity, arm and immediately consume the new opportunity, then classify. Fresh Wait reaches terminal engagement; no further pulse is supplied after a terminal action. |
| Passable destination outside envelope | No candidate camera/time/actor publication | Terminal SupportStopped with attempted cell; pending work retired without execution. |
| Charge reaching 960 or failed domain validation | No attempted gameplay change | Current authority publishes SupportStopped before unsupported effects or old-work flush. Earlier publications are preserved. |
| Due explicit idle pulse with pending work | No camera/minutes/ctr24 change | Decrement once; at zero consume one opportunity, classify and engage if applicable. |
| Idle without pending work, redraw/cache reconstruction | No gameplay change | Cannot authorize movement or decrement a gameplay countdown. |

Real terrain collision is checked before classifying an outside destination as a
support stop: fresh Backward at `(13,1)` North hits original space; East from
`(14,1)` attempts passable `(15,1)` and stops at the diagnostic envelope.
`Movement blocked by terrain.` uses retained blocked-action facts, never another
movement query/publication. It creates no event/presenter continuation, save
eligibility or pause of old work. If the action's pulse reaches Engaged or
SupportStopped, the terminal notice wins. A later action replaces ordinary
collision feedback.

Six categories remain separate: physical gameplay action, pending opportunity,
explicit gameplay pulse, actor cosmetics, ordinary-object animation, and
redraw/cache reconstruction. No general world clock/scheduler was introduced.
The reference's render-coupled countdown is adapted to explicit logical pulses.
After a nonterminal supported action, subsequent pending work is due at
`now+100 ms`. Each due idle delivers at most one pulse and rearms from current
time; it never replays elapsed backlogs. A normally serviced charged step returns
count 2, reaches 1 at +100 ms and moves at +200 ms. This is an explicit adaptation
of reference logical draws/two-frame pauses, not exact reference input latency.

A distinct physical navigation action supplies its own pulse and rearms the
pending deadline. An SDL cycle identity prevents the enclosing batch's idle from
supplying another gameplay pulse even if composition took longer than 100 ms.
Repeated same-time idle and obsolete authorization cannot replay work; distinct
rapid keypresses remain distinct actions and SDL repeat is ignored. Nonmodal
refusals do not suspend existing work. Terminal states retire it; exit supplies
no final pulse. Offscreen work is independent of emitted animation commands.

Pending count is bounded to 0..3. Checked wide revisions/generations and deadline
arithmetic stop safely on overflow; a backward clock observation cannot rearm
old work or wrap authorization into validity.

The existing [M22 policy](milestone-22-plan.md#explicit-reset-advance-and-refresh-rules)
remains owned by `XeenEventFlow`, using one shared ordinary-phase policy in ordinary
and encounter routes. Successful same-facing Forward/Backward and actual blocked
movement each request exactly one ordinary phase advance. Committed Left/Right
resets phase to zero; reset wins over action advance. Both rearm the ordinary
deadline to current time +100 ms before composition. Cache refresh preserves
phase/deadline. Gameplay pulses add no ordinary action/reset cause; due ordinary
idle advances once without backlog. Ordinary timing decisions follow adopted
action/pulse facts, so failed composition cannot replay gameplay or that visual
cause. Ordinary turn reset does not reset actor cosmetic phase/deadline.

### Accepted transition controls

Fresh startup activates record 5 at `(13,2)`, selects slot 3, leaves minutes 480
and pending count zero; idle alone never moves it. Fresh Wait moves it to party
cell `(13,1)`, minutes 490, slot 0, HP 20, and stops before combat. Forward North
instead moves the party onto the actor at `(13,2)` and engages at 490; that proves
engagement, but does not substitute for visible actor approach acceptance.

Right -> Forward East reaches `(14,1)` at 490 with count 3 then pulse count 2.
Two due pulses move the activated offscreen actor `(13,2)->(13,1)` once. Turning
West selects it one forward; forest occlusion may still hide it. Wait then moves
it into `(14,1)` and engages at 500. Rapid Right -> Forward -> Wait can consume
two distinct opportunities during Wait: old `(13,2)->(13,1)` and new
`(13,1)->(14,1)`. Selection refreshes after the prepared movement, without an
invented intermediate combat call.

## Outdoor presentation

`XeenOutdoorActorDraw` is a typed payload in `XeenOutdoorDrawCommand`, separate
from ordinary objects. Resource-derived image/frame, original monster identity
and selected slot travel through one stable-sorted terrain/object/actor stream.
`CloudsMapComposer` draws that stream through the existing assets/bridge, then
existing interface layers. Actors are not overlays.

| Reachable relative cell / query | Selected slot | Order | Anchor | Scale | Clipping |
| --- | ---: | ---: | --- | ---: | --- |
| Same cell / 2 | 0 | 118 | `(-5,2)` | 0 | Scene + bottom |
| One forward / 7 | 3 | 94 | `(-7,34)` | 8 | Scene |
| One forward-left / 5 | 12 | 90 | `(-112,34)` | 8 | Scene |
| One forward-right / 9 | 13 | 91 | `(98,34)` | 8 | Scene |

The four placements apply in all facings. Sideways/behind actors emit no draw but
retain activation. All twelve activation queries remain classified for isolation;
other relevant placements/actors fail diagnostic admission rather than disappear.
Synthetic capacity/multi-actor controls do not imply wider actor presentation.

Original cell offsets, scale masks and scene clip `[8,223) x [8,141)` are retained;
bottom clipping uses limit 140. Anchors are decoder inputs, not bitmap corners.
No ordinary `.obj` metadata, facing mirror, enlargement, health bar or attack
lunge is substituted. All eight normal `008.mon` frames are structurally checked
before startup and on reconstruction through the existing sprite cache.
`.att` is neither required nor loaded.

Actor cosmetics start deterministically at phase 0, select `phase % 8`, and
advance once per due independent 100 ms callback without backlog. This replaces
only the reference random initial cosmetic offset, not gameplay RNG. Turns,
movement and cache rebuilds preserve that phase; terminal engagement/support stop
freezes the current normal frame. Drawing consumes the frame without advancing it.

### Accepted initial occlusion

The initial `(13,1)` North frame emits original monster record 5's resource-derived
normal command at query 7 / slot 3, order 94, anchor `(-7,34)`, scale 8 and original
scene clipping. Later source-faithful terrain almost completely occludes it.
Across normal frames approximately **6-9 Skeleton pixels** survive (frame 0..7:
`8,6,6,6,8,9,9,9`). Ordered prefix controls establish that this is the original
composition, not incorrect resource/frame/anchor/scale/clipping/order.

This is accepted behavior. The initial view does not guarantee a recognizable
Skeleton; occlusion does not cancel activation or actor authority. Do not reorder,
overlay, shift, rescale or suppress occluders to improve readability. Observable
visual acceptance is the identifiable original Skeleton at approach/engagement,
confirmed by maintainer physical SDL acceptance. An unidentifiable engagement in
future changes is a separate failure to report, never grounds for a workaround.

## Production routing and terminal boundary

```text
mmodern --encounter-26 <game-directory>
```

Exactly one game directory is accepted, with no camera overrides or combination
with `--render-map`, `--load-game`, `--save-file` or another mode. The typed
`XeenEncounterEntry::Diagnostic26` uses existing Application gameplay services,
Flow and the single SDL loop. World of Xeen resources, context and all normal
sprite frames are admitted before the first composition/window. Failure reports
and exits without an ordinary-gameplay fallback.

Period (`WaitAction`, `SDLK_PERIOD`) is Wait here and a no-op outside encounter
routing. Arrows and W/S/A/D retain movement/turn meanings; W is not Wait.
Encounter routing intercepts before ordinary navigation/event dispatch, including
initial automatic dispatch. Space, I, inventory/transfer/equipment and direct
event acceptance/resumption cannot bypass it. Mode hints/refusals are nonmodal;
no event continuation, inventory certificate or combat party is created. Ordinary
`--render-map`, events and inventory remain unchanged outside this mode.

The selected same-cell/slots-0..2 condition latches Engaged before `doCombat`:
no combat setup, initiative, attack, damage, retaliation or rewards. The in-frame
notice uses the bounded resource name, with numeric fallback:
`Engaged: Skeleton. M26 stops before combat.` Position, time and unsaveable status
remain readable without obscuring the entire actor. Support stops identify their
reason; the envelope control reads `Support stop: diagnostic envelope (15,1)`.

Engaged/SupportStopped cannot return to exploration through navigation, Wait,
Space, Enter, I, E, T or selection. F9 remains refused. Escape/window close exits
the whole unsaved session rather than dismissing its terminal notice. Presentation
cleanup never resets authority and there is no in-session restart or save-on-exit.

## Unsaveable session and M28 boundary

The entire diagnostic session is unsaveable, from startup through pending work,
engagement, interruption, support stop and presentation failure. Production F9
checks irreversible encounter authority before ordinary busy/modal/target handling,
capture, restore/preflight or I/O. It reports refusal without advancing gameplay,
creating a deferred save or touching an injected save target.

Direct `XeenSaveState::capture` rejects marked, actor-bearing or context-bearing
owners before snapshot construction. Restore rejects live encounter destinations
and unexpected context from ordinary initial-party providers, and rechecks both
owner graphs after preflight. World overlay restoration cannot erase encounter
authority. `XeenSaveSnapshot`, codec and writer format remain unchanged: v2 writes,
v1/v2 reads for ordinary supported sessions. No partial encounter snapshot exists.

M28 must address original actor positions/presence/removal identities, map-visit
HP/activation semantics, admitted gameplay context and committed party outcomes.
The reference serializes positions/removal through MOB but reloads surviving
monsters at resource HP/initial activation on actual map load, including restart.
That lifecycle reset must never follow disposable cache reconstruction. Pending
opportunities, in-flight publication and unresolved encounter outcomes are not
safe persistence boundaries. M26 chooses no new wire version or migration policy.

## Final acceptance

The applicable build and complete **74/74 CTest** suite passed, including focused
monster/context parser, approach/domain/reentrancy, save refusal, production
encounter Flow/gameplay and ordinary outdoor-animation regression coverage.
SDL dummy/software checks and `git diff --check` passed; existing line-ending
conversion warnings were informational. Ordinary CTest uses synthetic resources.

Original-data validation with the legal World of Xeen installation and pinned
dependency passed the `xeen.mon` identity/statistics chain, all 27 map-20 identities,
64 isolation controls and four M26A transition traces. Production encounter smoke
passed original normal-frame composition/occlusion, fresh Wait engagement, delayed
East approach/view selection/final engagement, collision feedback, support stop,
terminal routing, F9 refusal and authority-preserving cache reconstruction.
Relevant ordinary Bone Whistle, graphics, equipment and save/restart regressions
passed during implementation/review. Domain smoke and production smoke are
separate evidence; neither claims full ScummVM encounter execution.

Independent technical review identified two MEDIUM integration issues: encounter
navigation bypassed M22 ordinary-animation action/reset semantics, and real terrain
collision lacked readable feedback. Both were corrected. Focused independent
re-review found both resolved, no new findings, reran relevant focused/full and
original-data evidence, and returned **APPROVE MILESTONE 26B CANDIDATE**.

The maintainer completed physical acceptance in the native production SDL window:

- Main approach: correct entry/forest presentation, real collision and refusal
  paths; period Wait produced a visibly identifiable original Skeleton at terminal
  engagement, without combat. Terminal inputs did not resume exploration and
  Escape exited.
- Delayed approach: Right -> Up followed by idle allowed offscreen approach.
  Turning West could leave the one-forward actor hidden by accepted forest
  occlusion; period produced the visible terminal Skeleton engagement.
- Diagnostic envelope: attempted `(15,1)` displayed exactly
  `Support stop: diagnostic envelope (15,1)`. The state remained terminal and
  F9, Space, I and Enter did not unlock exploration.

These are maintainer-observed native SDL results, distinct from automated,
extracted-harness, screenshot/image and independent-review evidence.

## Exclusions and successor

M26 does not implement combat: Attack, Block, initiative, hit/damage, retaliation,
injury, XP/rewards, combat item effects, armor breakage, Run, ranged attacks,
spells and recovery remain outside it. Indoor actors, wider map travel, normal-start
navigation, a general calendar and encounter persistence are also excluded.

M27 is the next milestone for separate planning of the existing bounded
Attack/Block objective; M28 remains the provisional persistence boundary in the
[roadmap](roadmap.md). M26 acceptance does not expand or authorize either stage.
Changes to isolation, terrain/events, profile/context, scheduling or modal/save
entry points require focused contract review before widening the admitted domain.
