# Milestone 26 - Visible original actor approach and engagement

## Status, objective and authority

**Investigation/specification only. M26 is not implemented or complete. No M26
unit is implementation-authorized.** This specification is ready for architectural
review, not approved implementation. After maintainer review and an explicitly
authorized specification commit/push, the architecture/specification agent should
review that fixed SHA against the accepted architecture and pinned reference,
resolve or reject unsafe assumptions, and produce the first implementation prompt
only if ready. This document is not that prompt.

The [approved roadmap](roadmap.md) selects Clouds map 20, original monster record
5, initially `(13,2)`, Skeleton type 8, party entry `(13,1)` North, and the four-cell
envelope `x=13..14, y=1..2`. Investigation retains that choice. The deliverable is a
visible resource-derived actor, its original activation and bounded approach, and
a readable, terminal engagement boundary before combat execution. Seeing an image,
parsing statistics, or showing a state model alone does not complete M26.

Use the World of Xeen Clouds-side behavior profile and original Adventurer
difficulty. This is diagnostic entry, not normal-start navigation or certification
of map 20. Keep all commercial resources external and read-only.
Reference controls use default options, with debugger intangibility/invincibility
and other cheats disabled. Resource availability does not select the profile.

Excluded: Attack/Block, initiative, hit/damage, injury, XP/rewards, combat equipment
effects, Run, combat rotation/disengagement, spells, ranged attacks, recovery,
indoor actors, connected travel, a general calendar, and M28 save serialization or
migration. Inventory operations remain accepted outside this encounter; M26 does
not admit them during this diagnostic session. No M27/M28 implementation is implied.

## Evidence and accepted integration baseline

The investigation verified MMModern `main`, clean working tree and empty index,
with HEAD, origin/main and direct remote main all at
`b3edfbff32db5890c68c5ff5d82c1911f8dfc4e9`. The origin was
`https://github.com/carbiecarbie/MMModern.git`. That committed state records both
M25 stages as accepted, including independent review and maintainer physical
acceptance, and contains the M26-M28 roadmap. [Project status](project-status.md)
remains the stable M25 authority; its older next-direction prose is not authority
over the subsequently approved roadmap. It is intentionally unchanged here.

Reference: ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, clean source checkout.
Resolve dependency locations using [dependencies](dependencies.md) and explicit
configured cache entries. The inspected applicable configuration selected this
pin and a separate build with the documented SDL/minimal-library flags. The
top-level legacy build cache instead named an old unversioned tree; it was not
used as reference or executable evidence. No existing MMModern or ScummVM engine
binary was treated as freshly built.

Evidence classes obtained:

- **Original-resource observation:** read-only CC index/payload decoding in an
  external Python probe, following pinned `BaseCCArchive` and `SaveArchive::reset`.
  The identities, fields, hashes and complete relevant actor controls below come
  from the supplied installation, not a modeled Skeleton fixture.
- **Source-derived caller trace:** engine startup/game loop, `Interface::perform`,
  `chargeStep`, `stepTime`, `draw3d`, scene activation, monster movement and party
  time. This establishes the scheduling edges that isolated movement inspection
  would miss.
- **Execution of extracted pinned functions in a constrained harness:** the
  unchanged bodies of `Combat::moveMonsters`, `canMonsterMove`, `moveMonster`,
  `InterfaceScene::setOutdoorsMonsters` and `Interface::chargeStep`, together with
  the pinned positioning/grid constants, were compiled and exercised externally.
  Lightweight stand-ins supplied containers, party/camera, map queries and draw
  slots. The driver reproduced selected input branches and draw countdowns; it
  did not execute the full `perform`/`draw3d`, SDL, rendering, or party-time bodies.
  Terrain queries returned the observed admitted-cell middle/surface values;
  time addition was simple arithmetic. Ranged execution trapped; status-overcome
  was a no-op because all exercised damage states were physical. This is partial
  reference execution with an independently written driver, not a full observed
  ScummVM encounter and not an independent oracle for that driver's scheduling.
- **Automated baseline code observation:** freshly compiled, unchanged
  `XeenObjectSpriteSafety.cpp` accepted all eight `008.mon` frames. This proves
  compatibility with structural sprite preflight, not rendered appearance.
- **Not obtained:** full reference-engine encounter execution, M26 production
  execution, pixel/image acceptance, or maintainer physical M26 acceptance.
  These are not reported as passed. The source caller trace plus extracted-body
  results resolve the specification contracts; production and physical evidence
  remain mandatory implementation acceptance, not substitutes for review.

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

### Existing seams and missing capability

| Inspected owner/module | Reuse and necessary extension |
| --- | --- |
| `XeenMapFormat::parseDat/parseMob`, `XeenMapLoader::loadObjects`, `XeenWorld::objectFile` | Already preserve immutable monster lists and compact monster table resolution. `objectFile` is the existing whole-MOB provider despite its name; do not add another MOB loader/cache. Live actors and monster statistics do not yet exist. |
| `XeenWorld`, `XeenSessionWorldState`, `XeenRecordIdentity.h` | Extend the session world owner with a distinct original monster identity and actor store; ordinary-object/event Remove remains independent. `discardMapCache` currently clears maps/MOB files only. |
| `XeenPartyLoader`, `XeenPartyState`, `XeenCharacterFormat` | Preserve all 30 roster owners, ordered membership and exact item/HP/SP/condition bytes. Add explicitly present bounded encounter context, not another party. Existing loading does not own gameplay minutes/difficulty. |
| `XeenMovement::apply`, `XeenNavigationFlow::processNavigationAction` | Reuse pure candidate camera/terrain evaluation. The latter currently dispatches automatic events even following blocked movement; encounter routing must intercept before that combined path. |
| `XeenOutdoorScene`, `XeenObjectVisual`, `CloudsMapComposer` | Reuse samples, one sorted command stream and raster infrastructure. Monster projection is distinct from ordinary-object animation metadata and placements. |
| `XeenAssetSource`, `ScummVmXeenBridge`, `XeenObjectSpriteSafety` | Add one checked explicit DARK statistics read and a semantic monster drawing entry over the existing sprite cache/validator/drawer. Do not create another archive or decoder owner. |
| `Application::gameplay/playGameplay`, `XeenGameplayServices`, `XeenEventFlow` | Application owns committed camera/flags and lifetime; Flow owns continuation/modal routing. Add a bounded encounter coordinator at this seam, outside the event interpreter. |
| `PlayerAction.h`, `SdlWindow::playerAction/showLoop`, `main.cpp` | One existing loop, repeat suppression, input/status/idle callbacks. W currently means forward in MMModern; a distinct wait key is required. |
| `XeenSaveState::capture`, `XeenGameplay.cpp`, `XeenSaveFile::write` | Production F9 currently captures, preflights and writes when Flow is idle. Add session and capture guards before any partial snapshot; no codec changes. |

Accepted persistence and ownership are in [M20](milestone-20-plan.md),
[M21](milestone-21-plan.md#21a-item-ownership-and-storage); ordinary timing in
[M22](milestone-22-plan.md#explicit-reset-advance-and-refresh-rules); scene order
and disposable visual values in [M23](milestone-23-plan.md#projection-and-occlusion-contract);
inventory/transfer in [M24](milestone-24-plan.md#transfer-rules-and-publication);
equipment publication and certificates in [M25](milestone-25-plan.md).

## Verified original encounter and approach

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

### Reproducible reference trace

Notation: P is party `(x,y,facing)`, A is record 5; T is original minutes; C is
`_tillMove`. All starts load fresh original records, physical damage-state,
inactive latch, HP 20, no pending work; startup classification activates A.
The harness's scene-only inspection does not decrement C. The explicit draw
countdown rows below reproduce the caller branch before scene classification.

| Transition | P | A / latch | T | C / selected slots |
| --- | --- | --- | ---: | --- |
| Startup scene | `(13,1,N)` | `(13,2)`, active | 480 | 0; slot 3 = 5, slots 0..2 empty |
| Any idle draws without a charge | unchanged | unchanged | 480 | 0; no movement authorization |
| Reference W: `chargeStep`, then explicit `moveMonsters`, then `stepTime` | unchanged | `(13,1)`, active | 490 | charge sets 3; explicit move clears to 0; next scene slot 0 = 5 |
| Next scene/engagement check | unchanged | unchanged, HP 20 | 490 | stop before `doCombat`; no combat turn |

Separate delayed-work control, starting fresh:

| Transition | P | A | T | C / selection |
| --- | --- | --- | ---: | --- |
| Right | `(13,1,E)` | `(13,2)`, still active, now outside view | 480 | 0; none |
| Forward East, at end of `chargeStep` | `(14,1,E)` | `(13,2)` | 490 | 3 |
| Next logical draw countdown | unchanged | unchanged | 490 | 2 |
| Following logical draw countdown | unchanged | unchanged | 490 | 1 |
| Third countdown: consume pending move, then classify scene | unchanged | `(13,1)` | 490 | 0; none, actor behind party |
| Left twice, with no new charge | `(14,1,W)` | `(13,1)` | 490 | 0; slot 3 = 5, actor visible again |
| Reference W | unchanged | `(14,1)` | 500 | 0; slot 0 = 5, engagement |

The harness exercised the same delayed movement, a one-left variant ending North,
the immediate wait, and the following controls. If a second charged action arrives
with C nonzero, `chargeStep` first adds ten minutes and flushes the old pending
movement using the already changed party camera; it then sets C=3. W additionally
calls `moveMonsters` immediately. Thus rapid Right -> Forward -> W can make A move
`(13,2)->(13,1)->(14,1)` during the second charged action at T=500. These are two
distinct authorized movement opportunities, not duplicate consumption of one
token. Attacker selection normally refreshes at scene classification afterward.

Forward North directly from entry moves the party onto A: P becomes `(13,2,N)`,
T=490 and charge initially leaves C=3. The next scene selects slot 0=5 and stops
before `doCombat`. This is valid engagement, but it is **not** the visible actor
approach acceptance sequence, because the party moved and the actor did not.

`perform` calls `draw3d` while waiting for input. `draw3d` decrements C before
`drawScene`; the third eligible decrement runs movement. `chargeStep`, not elapsed
wall time, authorizes that work. The reference usually pauses two 50 ms frame
counts per draw; arbitrary redraw frequency is not a gameplay-turn oracle.
Turning invokes `stepTime` but not `chargeStep`; actual collision refusal invokes
neither. Both may be followed by an eligible draw consuming previously pending
work. A wait may therefore move an actor immediately, while idle can finish an
earlier step without charging another ten minutes.

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

Add a checked `XeenMonsterFormat` parser for fixed 60-byte statistics records:

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

Extend the existing lazily owned Dark archive with an explicitly named checked
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

## State, ownership and lifecycle

The following names are proposed interfaces, not existing production APIs.
Prefer small value types and concrete functions over a generic entity framework.

| Value | Owner, initialization and mutation | Lifetime / reconstruction / persistence classification |
| --- | --- | --- |
| Original DAT/MOB/statistics/sprite bytes | Existing resources/MOB provider; immutable checked reads | Disposable caches; reload bytes without changing live state; resource-derived, not serialized |
| `XeenMonsterIdentity {mapId, recordIndex}` and actor positions | Actor collection added to `XeenSessionWorldState`, accessed/mutated only through `XeenWorld`; initialize in original record order on explicit encounter map entry | Session/map-visit authority; never held inside `_objects`/`_maps` caches; moved positions/removal identity are M28-facing durable requirements |
| Activation latch, lifecycle, live HP | Same actor owner; initial unactivated, present, resource HP, physical status; coordinator publishes activation/movement; M26 never damages/removes an actor | Preserve over all cache rebuilds and UI refreshes; activation/HP are live map-visit facts. HP/activation reset on an actual reference map load is distinct from cache rebuild; future persistence must honor that boundary |
| Roster, membership, items, HP/SP/conditions, quests | Existing `XeenPartyState` | Existing authority and save compatibility; no encounter-owned copies or M26 mutations |
| Bounded profile/difficulty/day/year/minutes/ctr24/rested/newDay | Optional `XeenGameplayContext` in `XeenPartyState`, populated only for this mode from initial PTY plus startup-derived newDay | Session gameplay authority, coordinator is sole mutation caller; unaffected by caches; M28-facing context, omitted from current format only because the entire session is unsaveable |
| Committed camera and game flags | Existing Application locals, borrowed by Flow/coordinator | Camera publishes at the specified transition boundary; game flags unchanged in this domain; never moved into an actor controller |
| Irreversible encounter-session marker | `XeenSessionWorldState`; set before actor/context admission can expose gameplay | Survives stopped/interrupted state and cache/Flow presentation cleanup; only destroying the entire session clears it. Save eligibility policy, not an M26 wire field |
| Phase, pending move token/count/deadline, transition generation, selected engagement identity/reason | One `XeenEncounterFlow` owned by the existing `XeenEventFlow`, borrowing world/party/camera; only Application's serialized action/idle route drives it | Session-long coordinator, not reconstructed with caches; no active/pending session save. Terminal latches survive notice closure attempts. Destroying it ends the encounter session; constructing another is not an in-session reset |
| Occupancy, view slots, commands, pixels, normal-frame phase | Derived occupancy/view values plus Flow-owned cosmetic phase | Recompute occupancy/view from current actors/camera. Cosmetic phase never authorizes movement. No persistence of pixels, commands, RNG cosmetic phase or deadline |

Unresolved/disabled original records keep raw identity and coordinates. Occupancy
counts every valid `0..31` position, regardless of visibility; invalid coordinates
do not index arrays. No use of `disableObject`, `applyRemove` or event-disable sets
for actor lifecycle. Keep a distinct typed identity so object record 5 and monster
record 5 cannot alias.

Use signed coordinates capable of preserving the original int8 values without
wrapping; live HP is a signed 32-bit value initialized from the unsigned resource
HP, matching the reference's signed live integer rather than an unsigned damage
accumulator. M26 only preserves that value. Retain typed present/opaque lifecycle
and physical status; death/removal/status mutation is not an exposed M26 command.
Context day/year/minutes/ctr24 are checked unsigned values with the stated bounds;
profile/difficulty are explicit enums. Pending count is 0..3 with an optional
generation-bearing token and monotonic millisecond deadline. Transition and
cosmetic counters use checked wide integers; overflow stops safely rather than
wrapping a stale token into validity. Do not retain resource pointers in actors.

Minimal boundaries:

- `XeenWorld::initializeEncounterActors` accepts fully validated original data and
  installs one collection once, before gameplay. A second initialization on that
  live visit refuses; cache fetches never call it. Read access returns immutable
  actor views with stable identities, not pointers into disposable resource data.
- A domain helper such as `XeenActorApproach` computes a prepared movement result
  from world actors, camera and admitted terrain. It preserves the literal scan,
  moved bits and occupancy ordering. It may use a bounded temporary actor copy
  to prepare a nonthrowing publication; that copy is not another live owner.
- A pure outdoor actor classification helper returns ordered projection slots and
  activation candidates. The coordinator commits new activation latches at
  startup and after logical camera/actor transitions. The compositor reads the
  same pure classification, never commits it. Scene reconstruction alone cannot
  activate, move, initialize or engage anything.
- `XeenEncounterFlow::handle` and `update(now)` produce owned typed outcomes
  (moved/blocked/refused/engaged/stopped and transition generation). They borrow
  the one world, party and committed camera. No callback into the event interpreter
  and no callback-driven transaction/publication.
- Flow exposes a terminal/encounter block in its existing modal admission checks,
  including direct `acceptManual/acceptAutomatic/respond` entry. Application
  checks the session marker independently for saving. Guarding only SDL keys is
  insufficient for service/direct-flow tests.

An actual map entry/load and a new game are explicit lifecycle events. M26 allows
one entry only; no teleport, re-entry, resume or live map replacement. For M28's
future contract, the reference writes positions/removal into MOB but initializes
surviving HP and activation again on real map load. Record these separate facts;
do not implement a file format or an HP reset on `discardMapCache`. A fresh game
constructs new owners and original positions/HP. Process exit discards this
unsaved session; that is neither victory nor durable defeat.

## Actions, time and explicit scheduling

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

### Transition table

`Exploring` includes an activated actor and possibly one pending movement token.
`Engaged`, `SupportStopped` and fatal presentation state are terminal. A separate
support-boundary reason is never presented as original terrain collision.

| Input/event in Exploring | Camera/time/ctr24 | Movement and publication |
| --- | --- | --- |
| Arrow Up/Down (W/S aliases retained), passable destination inside envelope | Publish candidate camera, +10 minutes, ctr24 +1 mod 24 | At charge, consume any old pending opportunity using the destination camera, then arm a new count 3; classify at the action's logical presentation pulse |
| Arrow Left/Right (A/D aliases retained) | Facing changes, minutes unchanged, ctr24 +1 mod 24 | No new movement token; existing token may consume one countdown at the action pulse; classify/activate for the new facing |
| Actual terrain-blocked Up/Down | Camera, minutes, ctr24 unchanged | Report original collision; no charge or new token. One action pulse may consume previously authorized pending work |
| Dedicated Wait (`.`) | Camera unchanged, +10 minutes, ctr24 +1 mod 24 | Flush old pending opportunity if present; arm new opportunity; consume it immediately as reference W does; classify and possibly engage |
| Passable step outside the envelope | No candidate camera/time/actor publication | Latch SupportStopped with attempted destination and diagnostic-envelope reason; retire pending work without executing it |
| Charge reaching 960 or failed domain validation | No action publication | Latch SupportStopped before unsupported effects; retire pending work; no rollback of earlier completed actions |
| Due encounter idle pulse with pending work | No minutes/ctr24/camera change | Decrement count once; at zero consume token once, publish movement, classify and latch engagement if appropriate |
| Idle with no pending work | No gameplay change | Normal actor cosmetic animation may update; never activate a new move |
| Redraw/expose/cache refresh | No gameplay change | Reconstruct presentation only; cannot decrement a token or classify as a fresh transition |
| Space, I, T, E, Enter, member/slot/Yes/No keys | No gameplay change | Readable mode-specific refusal; no event, inventory, equipment, wait, pulse or deferred action |
| F9 | No gameplay change | Immediate session-wide refusal before capture or I/O; not an encounter pulse |
| Escape / window close | No final movement or time charge | Exit the whole session with no save; do not return to ordinary exploration |

Evaluate real collision on a candidate camera before classifying an outside target
as a support stop. Thus entry Down is a real space-blocked attempt, while East
from `(14,1)` is a support boundary. Loading adjacent geometry for that read-only
test never authorizes traversal or adjacent actor loading. Invariants over the
whole admitted envelope are checked before startup and again before publication
if an external test/service changes authority unexpectedly.

### Adaptation of rendering-coupled work

Use explicit logical presentation pulses, separate from drawing. A supported
navigation/wait action has exactly one post-action pulse corresponding to the
next `perform` draw. It first decrements already armed count 3 to 2 (if any),
then classifies/activates, tests engagement, and requests a frame. Arm the next
encounter deadline at `now+100 ms`. Each due idle callback delivers one pulse and
rearms from its current time; never replay a backlog. In a normally serviced
single-step trace, count is 2 on return from the action, 1 at +100 ms, and movement
occurs at +200 ms. These milliseconds are MMModern's explicit adaptation of
reference logical draws/two-frame pauses, not a claim of exact reference input
latency. The harness table reports the charge's intermediate count 3 separately.

A fresh accepted navigation action before a deadline supplies its own pulse and
rearms it, as an input-driven reference draw can do. Due input and idle in one
SDL iteration must not decrement twice: serialized input publication rearms the
deadline before idle. Stale generation/tokens and repeated same-time idle calls
are no-ops. A later, distinct physical keypress is a new supported action;
SDL auto-repeat remains ignored. Do not drop valid rapid actions by treating them
as the same token, nor accidentally repeat the action after a draw failure.

Do not read `containsOrdinaryAnimation`, ordinary phase, NPC phase, sprite-cache
misses, paint count, wall-clock minutes or cosmetic RNG to decide actor movement.
The encounter countdown exists even when the actor is offscreen and no ordinary
object animates. Long idle consumes only the finite pending opportunity, without
charging game time or creating more opportunities. No event/inventory modal is
admitted here; attempts cannot suspend work behind an interactive dialog. The
nonmodal refusal/status display does not pause an already pending token. Terminal
engagement/support notices retire it permanently. An OS pause simply postpones
the next bounded callback, with no elapsed-time catch-up.

The reference refreshes attacker selection on the next scene, including after
both wait movement opportunities. Preserve that ordering for a prepared action;
do not manufacture an intermediate combat call. In this admitted one-actor
domain, if the first opportunity already reaches the party, the second would
only use the center zero delta. The final published engagement retires any newly
armed token and prohibits subsequent combat setup.

## Outdoor presentation

Add a typed monster draw payload to `XeenOutdoorDrawCommand`. It carries original
monster identity, normal sprite name/frame and scale/clip options; it is not an
`XeenObjectVisual` with an invented ordinary-object identity. Reuse the checked
normal sprite drawer behind a semantic AssetSource entry. Preserve one
`originalOrder` stable-sorted terrain/object/monster command stream, then existing
interface layers. No second scene, rasterizer or independent actor overlay pass.

Only these monster-relative cells are reachable within the four-cell actor/party
domain (F is forward, L/R lateral). All camera directions use the pinned offsets:

| Relative cell / query | Single actor draw order | Anchor X,Y | Scale | Flags / engagement |
| --- | ---: | --- | ---: | --- |
| Same cell / 2 | 118 | -5,2 | 0 | Scene + bottom clip; selected attacker 0; terminal engagement |
| 1F / 7 | 94 | -7,34 | 8 | Scene clip; selected slot 3; visible activation only |
| 1F1L / 5 | 90 | -112,34 | 8 | Scene clip; selected slot 12 |
| 1F1R / 9 | 91 | 98,34 | 8 | Scene clip; selected slot 13 |

Sideways on the same row and behind the party have no monster draw, but an
already active actor remains active. Pure classification must recognize all
twelve original activation queries to verify bystander isolation, even though
only these four require emitted monster commands in M26. A resource change that
requires other monster placements in an admitted view fails admission instead
of silently omitting an actor. Multi-actor selection/order/capacity controls are
synthetic domain tests; this diagnostic does not claim multi-actor presentation.

These are monster placements: ordinary one-forward objects use different Y and
scale. Preserve original resource cell offsets and original scale masks; anchor
coordinates are drawer inputs, not final bitmap corners. No directional `.obj`
metadata, mirrored facing heuristic, sprite stretching, enlargement, invented
attack lunge, health bar or attack overlay. Existing scene clipping uses
`[8,223) x [8,141)`; bottom clipping uses the existing 140 limit. Terrain/object
commands before/after each monster order provide occlusion; pixel occlusion does
not undo source-derived activation. Border/HUD stays drawn afterward.

Skeleton normal animation has eight frames and loopAnimation=0, so use
`phase % 8`. Choose deterministic initial phase 0 and independent 100 ms cosmetic
updates, one update per due callback, no backlog. This deliberately replaces only
the reference's random initial cosmetic offset; it does not alter gameplay RNG.
Keep this phase across cache rebuilds, camera turns and actor moves. It may share
the clock provider, not M22's phase/deadline or movement token. Drawing receives
an explicit frame and never increments it. At engagement/support stop retain the
current normal frame; no `.att`, audio, floating/effect animation or attack phase.

The first screen must show the original one-forward Skeleton and a persistent
in-frame diagnostic label/control hint. After Wait it must show the near/same-cell
normal sprite and a readable notice: `Engaged: Skeleton. M26 stops before combat.`
Use the resource-derived name with a bounded numeric fallback; English diagnostic
text is development UI, not replacement commercial narrative. Place the notice in
existing text/window facilities without hiding the entire actor. Show position/time
and unsaveable status in bounded diagnostic text or status area so actual movement
is distinguishable from frame animation. A support stop instead names the
diagnostic boundary/reason and attempted cell, never `wall` or `combat complete`.

## Production entry, input, failure and save policy

### Entry and routing

Proposed CLI (does not exist at this baseline):

```text
mmodern --encounter-26 <game-directory>
```

Exactly one directory, no positional camera overrides. Reject combinations with
`--load-game`, `--save-file`, `--render-map` or any other mode/extra arguments
before session creation. Keep ordinary CLI invocations unchanged. The new option
calls the same `Application::gameplay/playGameplay`, resource services, Flow and
SDL path with a typed encounter-entry option; it must not create another demo
application. Mark the session unsaveable before creating actors or its first Flow
frame. Validate all resources/context and compose the initial view before exposing
an interactive window. If validation fails, report and exit without a fallback.

Add a distinct `WaitAction`, mapped to period (`SDLK_PERIOD`). W remains forward,
S backward, A/D turns throughout MMModern; do not change the global W meaning to
match reference W. Period is enabled only in encounter routing and is a no-op
outside it. Arrows are the recommended physical recipe to avoid ambiguity.
Space remains the ordinary manual-event key outside this mode; inside it is an
unsupported-command refusal and never wait or automatic/manual dispatch.

In encounter mode, Flow routes navigation/Wait to its encounter coordinator
before normal NavigationFlow/EventSystem entry. Original no-event admission
establishes that suppressing dispatch does not hide an admitted-cell event.
Initial automatic dispatch is likewise replaced by explicit encounter startup
classification after admission. Do not change automatic or Space dispatch
globally, including Bone Whistle on map 20 `(5,14)` North.

All inventory entry, transfer, equipment, member/slot selection and direct event
acceptance/resumption are refused for the entire encounter session. This is the
smallest safe M26 admission, keeps the initial party unchanged, and avoids
inventing combat-time equipment rules. No inventory certificate, reward queue,
temporary character or event working camera is created. Unexpected already-open
modal state at entry is an error, not something to dismiss and continue.

Terminal `Engaged`/`SupportStopped` accepts only F9 refusal, exit and presentation
refresh. Navigation, wait, Space, Enter, I, E, T, selection and Escape-as-dismissal
cannot resume exploration. Escape follows the normal top-level exit path
(`handlesEscape` false for the terminal notice); SDL_QUIT also terminates. Closing
a notice programmatically does not clear terminal authority. No restart button,
in-session re-entry, save-on-exit or implied successful fight.

### Publication and failures

Use the existing single-threaded dispatch/idle guard. Prepare candidate movement,
time, actor result, view classification and fixed result facts before stores.
Validation/allocation failures before publication leave camera, actors, context
and old token unchanged; then latch a support/fatal stop and prohibit further
work. A token is consumed and replacement/generation installed in the same
nonthrowing publication as its actor/camera/context changes. No observer, logging,
string formatting, cache allocation or draw call belongs between these stores.

For a step with old pending work, compute it using the candidate destination
camera, just as the reference moves party coordinates before `chargeStep`.
Publication adopts the final prepared action once, including two opportunities
for wait where required. This bounded atomic preparation does not change existing
event semantics: encounter routing contains no event transaction; outside it,
movement still commits before event dispatch, and immediate party/Remove changes
still survive later event failures.

After publication, reporting/composition/upload failure never rolls back or
reexecutes an action. Retain the generation/result and latch a terminal failure;
if the error is within Flow composition, one clean presentation rebuild from
current authority may show the failure notice. It must not recreate a move or
rearm a deadline. If rebuilding or SDL upload fails, exit through existing fatal
presentation handling. Recovery success permits exit/refused save only. A failed
engagement notice is not permission to keep walking. Startup failure exposes no
partially admitted window. Quit during pending work discards the unsaved session
without consuming the token or executing a final transition.

### Unsaveable session and compatibility

The diagnostic session is unsaveable from startup through approach, pending work,
engagement, interruption, support stop and presentation failure. Eligibility must
not be inferred from `blocksGameplay()==false` or from the presence of a target.

Production capture/write route at this baseline is F9 in `XeenGameplay.cpp`:
`XeenSaveState::capture` -> candidate restore/composition preflight ->
`XeenSaveFile::write` -> existing protected replacement. There is no autosave,
save-on-exit or in-session load. Enforce both boundaries:

1. Application checks the irreversible world session marker before ordinary F9
   handling. Return a readable refusal immediately: no capture, resource
   preflight, target resolution/read, file I/O, token/presentation advancement or
   deferred save. Direct service injection of a save target must not bypass this.
2. `XeenSaveState::capture` itself rejects an encounter-marked world, or present
   encounter context/actors, before constructing a partial snapshot. This guards
   direct production helper use and detects inconsistent marker/context state.
   An initialized world cannot become capture-eligible through cache discard or
   ordinary overlay restoration. Preserve the marker across such operations;
   reject restoration into a live encounter owner.

`XeenSaveSnapshot`, `XeenSaveFormat` and the writer's file format remain unchanged.
The low-level writer accepts snapshots and has no live world; do not add a fake
partially populated encounter snapshot or a global writer switch. No reachable
production encounter path may produce one. `--load-game` cannot opt into this
mode: v1/v2 lack its actor/time facts, and M26 must not reinterpret a legacy save
or normalize its roster/loadout. Ordinary v1/v2 loading, explicit F9 and all M25
item bytes retain existing behavior outside encounter mode.

Future persistence must account for moved/present/removed original actor identity,
map-visit HP/activation semantics, bounded gameplay context and committed party
facts. Pending opportunities, in-flight publication and engagement are not safe
M26 snapshot boundaries. This records the M28-facing requirement only; it neither
chooses a new format nor declares an encounter state saveable.

## Implementation units

Two units are justified by independent contracts: the resource/world/scheduling
foundation can be tested without SDL; production composition/routing must then
close the visible acceptance boundary. One monolithic unit would hide ownership
and delayed-work errors behind UI evidence. A third parser-only stage would add
an artificial handoff without a complete domain contract.

### 26A - Resource-derived actor authority and deterministic approach

**Recommended first unit for a later implementation prompt**, after specification
review and separate authorization. Deliver checked monster/context interpretation,
world-owned actor identities/state, pure activation/occupancy classification,
prepared approach transitions and the save-capture protection. Do not expose the
CLI, enable actors in ordinary `--render-map`, or claim visible gameplay yet.

Necessary reading: this complete plan, stable status, dependencies, M20/M21 owner
and persistence sections, M22 timing separation; inspect the current baseline
again. Intended modules: `XeenMapFormat` only where a proven parser correction is
needed, new `XeenMonsterFormat.h/.cpp` and bounded PTY context reader,
`XeenAssetSource`, `ScummVmXeenBridge`, `XeenMapLoader` provider reuse,
`XeenRecordIdentity`, `XeenWorld`, `XeenParty.h`/loader, new
`XeenActorApproach.h/.cpp`, `XeenSaveState`, focused tests and CMake registration.
No damage calculations, ordinary-event changes, save codec changes or temporary
production demonstration path.

Keep storage versus simulation admission distinct. Domain operations accept
explicit camera/action/countdown values and return prepared typed facts; they do
not own a clock, redraw, party or world. Supply the pure four-cell view and full
activation-query classification needed by 26B now, not a second UI implementation
of movement. Capture refusal and one-time initialization are part of this unit so
new live state cannot leak through an existing direct capture.

Focused evidence: synthetic parser/malformed tests, compact-table identity,
world-cache/lifecycle, literal movement/occupancy/fallback order, admission and
time bounds, capture refusal; optional original-data records/approach comparisons.
Run relevant existing map/world/movement/party/save tests. Stop with independently
testable domain operations and unchanged production behavior. This unit does not
complete M26 and does not authorize 26B.

### 26B - Production visible approach, coordination and terminal engagement

Depends on accepted 26A, this plan's resolved interfaces and M22/M23 composition,
M24/M25 modal/publication rules. Deliver typed monster commands and checked sprite
drawing, explicit cosmetic/pending deadlines, `XeenEncounterFlow` under existing
Flow, period input, diagnostic CLI, readable permanent stop and the Application
F9/session guard. Complete the actual production recipe and all bypass/failure
controls. No combat action or serialization implementation.

Intended modules: `XeenOutdoorScene.h/.cpp`, new small actor visual/classification
adapter if necessary, `CloudsMapComposer`, AssetSource/bridge semantic drawing
entry, `XeenEventFlow.h/.cpp`, proposed `XeenEncounterFlow.h/.cpp`,
`XeenGameplayServices.h`, `XeenGameplay.cpp`, `Application.h/.cpp`,
`PlayerAction.h`, `SdlWindow.cpp`, `main.cpp`, tests and CMake. Reuse
`XeenTextRenderer`/frame/window facilities for the terminal notice. Modify
`XeenInventoryFlow` only if direct admission guards require it; do not refactor
accepted equipment or transfer behavior.

Evidence: controlled-clock Flow tests, exact production synthetic input traces,
headless SDL routing and failure tests, independent original-data composition
images and approach trace, negative save file checks, ordinary checkpoint
regressions, full build/CTest at closure, independent review and maintainer
physical acceptance. The stopping point is an observable, unsaveable encounter
boundary in the real application. Later milestones remain separately specified
and authorized; do not continue into damage or migration because 26B passes.

## Verification and acceptance

All new names/targets below are **proposed**, and no planned test is reported as
passed. Ordinary CTest must use synthetic resources only. Expected values come
from literal tables/traces here and pinned sources; never ask the implementation's
movement/view helper to generate its own expected result.

| Consequential requirement | Independent evidence / focused tests |
| --- | --- |
| Correct parsing and identities | Synthetic 60-byte records with nonmatching record/slot/type/image IDs; empty/malformed table/list/EOF/offset/short-read cases; compare original anchors separately. Reject unresolved relevant records without deleting preceding placeholders. |
| One actor owner and lifecycle | Initialize once, move, change synthetic live HP, discard map/MOB/sprite caches, recompose and compare exact state/identity. Reinitialization refuses. Fresh independent session restores originals. Actor operations never change object/event Remove sets. |
| Activation and ordering | Literal query-2 versus query-7 checks, all four facings and side/behind controls; activation survives loss of view/occlusion. At most three occupants; fourth rejected; original record ordering, two passes/one move, primary terrain fallback and occupancy-no-fallback controls. |
| Isolation across transitions | All 27 original identities, 64 view/state controls and bystanders forcibly active as a test-only scan control. Add synthetic offscreen in-range actor, ranged actor, occupied destination and record-order permutation: admission must detect relevance, not skip it. |
| Costs and pending work | Literal fresh Wait, direct Forward, delayed East and rapid-W traces. Fixed clocks 0/99/100/199/200 and long jumps; action pulse count 3->2, exactly one consumption, no pending idle movement, turns/collision without time, ctr24 wrap, 950->960 refusal before old-token flush. |
| Repeated/stale input and publication | SDL repeat ignored; distinct rapid keys processed; old generation replay/no second engagement; same-time idle no duplicate. Inject failures during preparation, result adoption/reporting, compose, notice rebuild and SDL upload; compare all owners and consumed tokens, not only pixels. |
| Visual semantics and ordinary regression | Synthetic distinguishable frames/occluders at orders before/after 90/91/94/118, scene/bottom clipping, all four reachable placements and facings, original normal frames 0..7, cache equality at same phase. Existing object, ordinary animation, indoor and Remove tests remain valid. |
| Modal/event/inventory bypass | Initial/direct automatic/manual dispatch, Space, direct accept/respond, I/T/E/member/slot keys, repeated Enter/Escape, synthetic stale inventory certificates, abandonment/refresh and terminal notice clearing. Counters prove no event/item operation was invoked. |
| Save prohibition | Preexisting valid v1/v2 save bytes and a configured service target; F9 at startup/approach/pending/terminal/interruption/failure; direct capture refusal. Capture/preflight/read/write spies all stay zero for F9. Compare full file bytes and absence of new siblings afterward; close/cleanup/refresh then F9 still refuses. |
| Ordinary compatibility | Existing save/restore/CLI/SDL, inventory/transfer/equipment and event tests. Original Bone Whistle on map 20, Myra/Phirna, Nightshadow and Castle Basenji checkpoints remain ordinary mode, with no actors activated and unchanged F9/load behavior. |

The bounded time refusal should be exercised with controlled admitted context at
950 and explicit pending work; do not require a player to evade an active actor
for 47 charges merely to reach it. The production CLI does not expose a time
override. A genuinely reachable long-input trace, if added, supplements this
boundary test rather than weakening the time contract.

Suggested grouping: `XeenMonsterFormatTests`, `XeenActorApproachTests`,
`XeenEncounterFlowTests`, `XeenEncounterGameplayTests`, and extensions to existing
SDL/save/scene tests. Add an optional original-data `mmodern_encounter_smoke`
target excluded from ordinary CTest; it runs production services as well as
domain/resource checks, requires a user-supplied installation, and emits evidence
outside tracked source. Reuse existing save/SDL test facilities rather than a
second persistence harness. Build only relevant targets during iteration; M26
closure requires a fresh applicable full build and full CTest plus required
manual acceptance. No full suite was run merely for this specification prose.

### Reproducing material investigation

Verify both Git pins/clean source first. Decode CC via the existing bridge or a
small read-only probe: uint16 LE entry count, eight-byte entries, index byte
`i` decoded as `(rol8(raw,2)+0xac+i*0x67) & 255`; entry ID uint16, offset uint24,
length uint16, final reserved byte zero. Use pinned name hashing and XOR rules,
concatenate the present initial blocks listed above, then read only the named
members. Check lengths/hashes, parse MOB lists without filtering/reordering,
statistics record 8, PTY offsets and event addresses. No extracted payload needs
to enter the repository or durable plan.

To reproduce partial reference execution without a full engine build, copy the
five function bodies named in the evidence section directly from the verified
source into an external translation unit, with the pinned grid/position constants.
Supply the listed 27 coordinates, physical status, no ranged path, ordinary-ground
queries for admitted destinations, an initial inactive latch and count 0. Execute
the table's input/charge calls and explicit countdown-before-classification
pulses. Assert every intermediate P/A/T/C/slot, then all 64 controls and bystander
coordinates. Trap unsupported calls rather than returning success. Review the
driver against `perform`/`draw3d` separately: this harness cannot validate its
own scheduling model. Full engine execution, if used later, must record profile,
options, resource source, exact build provenance, inputs and interception before
combat; a local debugger teleport is disclosed diagnostic entry, not normal play.

### Future maintainer physical acceptance

Prerequisites: accepted 26A/26B candidate, fresh verified build using the pinned
dependency, legal original installation, no modified original data, desktop SDL
window. The command below is proposed until 26B exists:

```powershell
& '<verified-build>/mmodern.exe' --encounter-26 'F:/Games/gog/Might and Magic 4-5'
```

1. At `(13,1)` North confirm the original Skeleton one cell ahead, diagnostic
   label, original six portraits, T=480 and unsaveable-session hint. Wait several
   seconds without input: normal frames may change, but actor position/time do not.
2. Press F9, I and Space separately. Read save/inventory/event refusals; none moves
   the actor or changes time. Press Down once: actual space collision, still
   `(13,1)`, T=480; this is not a diagnostic boundary.
3. Press **period once**. Observe actual actor approach into the party cell, larger
   near sprite, T=490 and `Engaged ... M26 stops before combat`. No damage, turn
   highlight, attack animation or reward occurs. Do not substitute Forward for
   this acceptance step.
4. Try period, arrows, Space, Enter, I, E, T and F9. The engagement remains terminal
   and saving remains refused. Escape exits the application; it does not dismiss
   the stop into exploration. Reopen with the same command only as a fresh process.
5. Separate delayed control: Right, Up, then hands off for at least one second.
   At `(14,1)` East, T=490, actor moves offscreen to `(13,1)` once and stays there.
   Left twice reveals it one forward cell West; period yields engagement at
   `(14,1)`, T=500. Diagnostic position text and automated trace distinguish this
   offscreen move from the independent visible approach in step 3.
6. Separate boundary control: fresh process, Right, Up, wait one second, Up again.
   Attempted `(15,1)` is passable original geography but must show a diagnostic
   support stop with P still `(14,1)`, T=490. F9/Space/I/Enter cannot unlock it;
   close the window to terminate without saving.
7. Exercise ordinary Bone Whistle and the accepted M25 equipment/save/restart
   checkpoint through their existing commands, outside encounter mode. Confirm
   encounter guards did not globally change those paths.

Negative CLI/save-byte controls are automated separately: reject added
`--save-file <existing-save>` and `--load-game` before gameplay; a service-level
injected target also cannot enable a write. Maintainer observation complements
the byte comparisons and failure-injection tests, not vice versa. Keep screenshots,
headless images, trace outputs, independent review and physical acceptance clearly
distinguished in eventual closure evidence.

## Readiness and replanning triggers

**Ready for architectural review.** The approved entry/envelope survives the
bounded investigation. No encounter replacement or roadmap scope change is
required. The critical new decisions are the same-cell engagement edge, charged
delayed movement that may finish during idle, distinct zero-cost turns/blocked
steps, the 16:00 time-effect boundary, explicit production coordination and an
irreversible unsaveable session.

Evidence limits are explicit: the critical movement/classification bodies were
executed with stand-ins, while caller scheduling/time and full raster behavior
were traced from source. A review that identifies an ambiguity in those stand-ins
must block the affected unit and resolve it with the smallest targeted probe of
the disputed caller/query, or a pinned-engine run intercepted before `doCombat`.
Do not treat a newly written model as an independent oracle or report this as
maintainer-observed reference combat.

Replan before implementation if a relevant actor enters the scan/activation
domain, resource variants change admission, terrain/events invalidate the local
window, the pinned call chain contradicts token ordering, or new modal/save entry
paths can omit live authority. If the four-cell anchor needs correction, state
the smallest evidenced change and obtain roadmap approval; do not silently move
the encounter or freeze another actor. Additional maps, attacks, equipment
effects, time effects or persistence require their own authorization.
