# Milestone 32 - Regional Journey foundations

**Closed.** Milestone 32 established content contract 3 as the accepted Regional
Journey foundation for the connected mainland of Clouds map 23. It retained the
existing Application, world, party, Journey/encounter, Event, Presentation and
persistence owners; it did not create another gameplay framework.

The accepted [M29 mutable-domain contract](milestone-29-plan.md),
[M30 grouped-expedition contract](milestone-30-plan.md), and
[M31 event/publication contract](milestone-31-plan.md) remain authoritative for
their respective content. M26 action/pulse scheduling, M27 publication units and
M28 retained-owner/restoration safeguards are inherited by reference.

## Final scope and acceptance boundary

M32 provides:

- `--journey-region` at Clouds map 23 `(9,11)` West;
- resource-derived, map-local party navigation over the connected mainland;
- complete ownership, activation and scheduling of all 19 original regional
  actors, including actors outside party-reachable terrain;
- exact original sign execution at `(5,9)` North through existing Event and
  Presentation authority;
- truthful terminal support boundaries for regional enemy ranged action,
  same-cell contact and unsupported temporal processing;
- checked gameplay context preparation and Journey v4 schema/content 3/3
  save/restart with exact regional continuation; and
- immutable resource-preimage and internal-identity authority that survives
  guard renewal and disposable cache reconstruction.

This is regional exploration, not unrestricted map-23 or Clouds gameplay.
Regional combat, enemy ranged resolution, player Shoot, Poison, Sleep, loot,
Run/disengagement, Myra/Phirna quest execution, recovery/services, adjacent-map
travel and disconnected party areas remain outside M32. Existing Skeleton and
Bone Whistle Journey/save domains remain unchanged.

## Original-resource contract

Production contract 3 checks the map-23 DAT/MOB/EVT sizes and CRC32 values below
and the five referenced MON-record CRC32 values. CRC32 is a compatibility check,
not authentication; typed parsing, identity and semantic validation also apply.
Commercial data remains external and unmodified.

| Resource | Bytes | CRC32 |
| --- | ---: | --- |
| `maze0023.dat` | 892 | `8f3e28ee` |
| `maze0023.mob` | 220 | `ce08a8c8` |
| `maze0023.evt` | 1440 | `e3128711` |

The observed whole-file `DARK.CC/xeen.mon` size of 5400 bytes and CRC32
`4dec3f50` are investigation evidence, not an additional regional admission
manifest requirement.

The five referenced 60-byte monster-statistics records, in type order
3/6/8/9/13, have CRC32 values `7f7e3f71`, `eb3b54b1`, `e36833c6`,
`5002c318` and `5636ae25`.

Separately, save-to-installation compatibility requires the existing complete
archive size/CRC32 signature, including required DARK.CC. A compatible archive
signature alone does not authorize changed in-memory topology; immutable
resources remain subject to retained provider guards on startup and reload.

Map 23 is an outdoor Clouds map with neighbors N/E/S/W 22/0/24/19, flags2
`0x8000`, wall-no-pass difficulty 0 and Run coordinates `(10,12)`. Neighbor
metadata supports rendering only; contract 3 admits no map transition. Run
metadata is retained for successors but grants no M32 Run action.

The MOB contains 20 objects, 19 monsters and one wall-item record. All 19
monsters are initially live, resolved, Physical and unaccounted, with identities
`{Clouds,23,originalIndex}`:

| Type / image / base HP | Original record indexes and coordinates |
| --- | --- |
| Orc / 6 / 25 | 0 `(0,13)`, 1 `(0,15)`, 2 `(1,13)`, 3 `(2,15)`, 4 `(2,12)`, 5 `(3,12)`, 6 `(4,14)`, 7/8 `(3,10)`, 9 `(6,12)`, 10 `(7,14)`, 11 `(9,15)` |
| Giant Snake / 3 / 15 | 12 `(5,0)`, 13 `(13,8)` |
| Giant Toad / 13 / 90 | 14 `(6,1)`, 15 `(7,3)`, 16 `(14,2)` |
| Skeleton / 8 / 20 | 17 `(1,2)` |
| Zombie / 9 / 30 | 18 `(1,1)` |

The EVT contains 170 records. The only admitted contract-3 event continuation is
record 56 at `(5,9)` North: line 0, opcode `04 SignLabel`, text index 16, natural
end. The physical cell has the automatic bit. All other original event addresses,
including Myra `(9,11)` West, Phirna `(8,2)`, and the map-28 transition chain at
`(10,13)`, remain unadmitted even when their cells are reachable.

## Regional navigation and admission

### Descriptor and party component

Content contract 3 fixes Clouds/map 23, anchor `(9,11)`, fresh facing West,
Swimming=false, WalkOnWater=false and Mountaineer=false. The descriptor contains
no route list, actor-coordinate boxes or stored reachability mask.

The party component is a bounded four-neighbor flood from the anchor using the
same pure destination collision query as ordinary outdoor movement. Both
endpoints must remain within `[0,15] x [0,15]`, on the same Clouds/map identity,
without wrapping. The anchor must be traversable. Middle/surface precedence is
the existing Clouds rule: mountain middle values remain blocked; middle
`0,2,4,5,8,11,13,14` consult surface collision; middle 3 retains its special
passability. With the admitted resources and capabilities the mainland contains
121 cells. That count is validation evidence, not a hard-coded whitelist.

The derived component is disposable policy data bound to exact immutable map
content and capability inputs. Cache reconstruction re-derives it under the
retained resource guard. Neighbor DAT sampling may supply view geometry, but it
never admits party travel, actor collections or gameplay on another map.

Three authorities remain deliberately separate:

| Question | Authority |
| --- | --- |
| Can party geometry reach a cell? | Derived mainland component from original party collision and fixed capabilities. |
| Can an actor move or influence the party? | Original identity/profile, live state, activation, regional terrain closure and scheduler; never party-component membership. |
| Can an event execute? | Exact Event capability bound to original address, records, resource preimage and Journey lease. Geometry alone grants no script authority. |

Actual terrain or map-local boundary blocks are nonterminal and publish no time
or new actor opportunity. A passable but unadmitted transition refuses before
camera, time, RNG or event mutation. Refusal cannot cancel already owed work.

### Production entry and controls

`--journey-region [--combat-seed <nonzero-u32>] <game-directory>
[--save-file <path>]` uses the existing strict CLI and Journey setup. The seed
initializes the existing world gameplay RNG. An explicitly supplied seed must
be nonzero and bypasses sampling; explicit zero is rejected by the CLI. Without
an explicit seed, sample once and map only a sampled zero to one. Fresh entry
uses the existing prepared-party path with day 8/year 610/minute 480/ctr24 0,
all 30 Luck-bearing supplements, original flags/quests/items and all 19 actors.
It classifies the initial view without movement or RNG and does not dispatch
Myra.

Arrows/WASD move or turn; period waits; I uses the existing inventory, transfer
and equipment UI; F9 saves only at an eligible presented Quiet boundary; Space
requests an admitted interaction. Attack, Block, Shoot, Run, Rest and
Begin/Revisit have no regional implementation. Escape/window close exits.

### Event and sign authority

The exact sign may execute automatically after ordinary navigation dispatch
opportunities, including turns and actual blocked moves, or manually through
Space. Wait and idle do not invent an additional automatic dispatch. Automatic
admission is preflighted before publishing the navigation candidate. Already
owed actor pulses complete first; then Flow transfers directly to the exclusive
Event lease without exposing a saveable quiet gap. If actor work reaches a
terminal support stop first, the pending sign dispatch is discarded without
execution. Startup restore performs no automatic dispatch.

The capability binds the original record/address, physical camera, immutable
script, retained owner preimage and Event lease. It permits only the one-record
SignLabel continuation; it grants no Call, teleport, grant, Remove, selection,
flag or reward authority. Effective None after an independently valid disabled
overlay is legal. Wrong-facing lookup is ordinary NoEvent. Unknown automatic
chains refuse before publication or stop still-current mandatory work; malformed
or changed resources are integrity failures, never empty cells.

Every other manual address refuses before interpreter entry with no effect,
time or RNG change. The sign uses the existing Event-to-Presentation handoff,
response generations and modal exclusion. No retry, frame reconstruction or
cache rebuild redispatches it.

## Complete actor ownership and scheduler

The world owns the entire original-order map-23 monster list. Every record keeps
its original identity, spawn metadata, type/image/statistics and order. Position
and activation are mutable for all live actors; HP, lifecycle, Physical status
and accounting remain authoritative world values. M32 produces no damage, death,
loot, XP or settlement.

Normal MON rendering uses each resource-derived image without requiring regional
combat/ATT readiness. Missing or malformed required appearances fail visibly;
actors are not dropped to manufacture a frame. Occupancy remains bounded to
three actors per cell.

Regional ground movement separates movement capability from ranged capability.
Types 3/6/8/9/13 use the inherited 7x7 scan, two passes, original-order inner
loop, moved-once set, direction priorities, occupancy and fallback. For actor
terrain, middle `0,2,3,4,5,6,8,11,13,14` takes the surface branch; nonflying
water/deep-water/space and mountains above wall-no-pass are blocked, not integrity
errors. Unknown modes are Unsupported. Each live position must lie in the
directed terrain closure seeded by its original spawn; the spawn remains included
even when it is not itself an admissible destination. Actors may move and exert
influence outside the party component. Actor map transfer is unsupported.

Activation follows projection query matches before occlusion and persists after
leaving the view. Activated actors outside the scheduler scan do not move;
unactivated actors still consume occupancy. Rendering and cache reconstruction
do not activate actors or advance gameplay.

Before each candidate actor step, ranged relevance is evaluated at that exact
scheduler point, including candidate positions produced by earlier actors.
An activated Physical Orc outside current contact and aligned within the scan
uses the original asymmetric outdoor ray: east tests raw wall mask `0x8`;
west/north/south test middle values `0,2,4,5,8,11,13,14`. A clear ray produces
terminal `SupportStopped` before attack setup, movement, RNG or damage. A blocked
ray continues normal scheduling.

Live same-cell contact publishes the supported movement/classification result,
then enters terminal regional-contact `SupportStopped` in the same guarded
boundary. No regional combat coordinator is constructed, including for the
regional Skeleton and Zombie.

## Publication, failure and resource authority

Each regional action/pulse retains owners, revision, activity, lease and resource
preimage, then prepares detached camera/context/actor candidates. Fallible
providers and storage preparation precede gameplay stores. Unsupported ranged,
movement or temporal work discards the whole unpublished candidate; prior
published actions remain. A supported action publishes through nonthrowing
stores and updates retained expected state before callbacks. Results and terminal
state are adopted before feedback or composition.

`SupportStopped` retires pending work and permits only exit and safe presentation
reconstruction. Reporting the stop emits complete retained inspection output
without reopening a mutable inventory panel. Navigation, Wait, interaction, item mutation,
combat-like commands and F9 remain blocked. It is not victory, combat End,
defeated accounting or a resumable disengagement. The last disk save is unchanged.

Retained guards accumulate every admitted immutable map/MOB preimage across
authorized publication, guard renewal, restore preparation and cache eviction.
Absent cache entries and neighbor resources already admitted remain part of the
preimage set. Reloaded DAT/MOB values must match both the requested key and their
internal identity. An incompatible returned value for a previously admitted key
permanently latches integrity failure before cache insertion;
mutation-and-reversion or a later matching retry cannot reopen Quiet or capture
authority. Provider I/O/allocation failures that
return no incompatible value retain the established recoverable retry policy.
EVT and referenced monster statistics/appearances remain guarded likewise.

Owner incarnation/ABA protection, copy/move restrictions, monotonic invalidation,
reentrancy checks, leases and presented-input generations remain inherited.
Destruction, stale callbacks and cache lifetime cannot recreate authority.

## Time and gameplay context

Contract 3 retains one party-owned `XeenGameplayContext`. A pure checked
preparation accepts the current context and a minute charge, then returns a
candidate plus counts for 480-minute processing, midnight, 100-day year rollover,
dawn, dusk and daily work. It uses wide intermediates and rejects u16 year
overflow. Zero charge is exact identity and performs no time processing or RNG.
ctr24 remains an independent modulo-24 step counter.

Canonical regional context requires WorldOfXeenClouds/Adventurer, day `0..99`,
minute `300..1259`, ctr24 `0..23`, no pending newDay/rested work, and zero
effects/light/resistances. Runtime publication admits only candidates requiring
no deferred temporal effect. Therefore the fresh run truthfully stops before
publishing `950 -> 960`; it does not silently skip original processing. Age and
item-derived values use the retained context year rather than a literal entry
year.

| Work | Minutes | ctr24 / actor opportunity |
| --- | ---: | --- |
| Successful Forward/Backward | 10 | +1 modulo 24; flush old opportunity, arm 3; normal pulse classifies/decrements. |
| Left/Right | 0 | +1 modulo 24; no new opportunity; existing work may continue. |
| Wait | 10 | +1 modulo 24; flush old opportunity, then consume the newly armed opportunity. |
| Terrain/boundary/admission refusal | 0 | No new charge or opportunity; old work is retained unless terminally stopped. |
| Due gameplay pulse | 0 | No ctr24 change; consume at most one due decrement/opportunity. |
| Inventory, item action, sign/refusal, save, redraw, cosmetics | 0 | No time or gameplay RNG. |

A temporal support stop preserves prior committed state, consumes no would-be
condition RNG and remains unsaveable. M32 does not implement condition aging,
daily resets, day/night transitions, Rest, services or wall-clock catch-up.

## Schema-3 persistence and restore

Contract 3 uses the existing v4 Journey envelope with schema/content 3/3. It
retains the v2 base, complete party/roster/items/flags/overlays, all 30 ordered
Luck-bearing supplements and the world-owned algorithm-1 RNG continuation.
Schemas 1/1 and 2/2 keep their exact meanings and lengths; no contract is inferred
from camera state and no legacy save is converted implicitly.

The schema-3 suffix begins immediately after the v2 disabled-event list. All
multi-byte values are little-endian and existing strict boolean/enum, 4 MiB,
envelope-length, reserved-byte, CRC32 and exact-EOF checks remain in force.

| Offset | Field |
| ---: | --- |
| 0 | domain u8 = 3 (Journey) |
| 1 | schema u16 = 3 |
| 3 | content contract u16 = 3 |
| 5 | context-present u8 = 1 |
| 6 | existing 33-byte gameplay context |
| 39 | supplement count u8 = 30 |
| 40 | 30 owner-ordered 41-byte M30 supplements |
| 1270 | RNG algorithm u8 = 1 |
| 1271 | nonzero RNG state u32 |
| 1275 | committed raw-draw count u64 |
| 1283 | initialized map: side u8 = Clouds, map u16 = 23 |
| 1286 | original actor count u16 = N |
| 1288 | saved actor count u16 = N |
| 1290 | N original-index-ordered 19-byte actor records |

Length is `1290 + 19*N`; N is 19 for contract 3, producing a 1651-byte suffix.
Decoding bounds N to `1..107`, requires equal original/saved counts and exact
remaining length before allocation, then requires all identities in
Clouds/23/index `0..N-1` order.

Each actor record is side u8, map u16, index u32, x i16, y i16, HP i32,
activated u8, lifecycle u8, status u8 and accounted u8. Lifecycle values are
Present=0, Disabled=1, Unresolved=2, Defeated=3; status Physical=0,
Unsupported=1. Context is profile u8, difficulty u8, ctr24/day/year/minutes u16,
nine effect u8 values, six light/resistance u16 values, rested/newDay u8.
Each supplement is owner u8 (owners 0..29 in order); permanent/temporary Might,
Speed, Accuracy and temporary AC as seven i32; XP u32; permanent/temporary Luck
as two i32. Profile WorldOfXeenClouds is 0; difficulty Adventurer=0, Warrior=1
structurally, with only Adventurer admitted by content 3. Every boolean must be
exactly 0/1. Structural wire bounds are x/y `[-128,31]` and HP `[0,65535]`,
followed by the stricter resource-dependent checks below. Supplement attribute
inputs and temporary AC retain byte-origin bounds `0..255`. Absent Luck,
out-of-byte-origin inputs, invalid enums/booleans and nonzero legacy seed reject.

Present actors must be
Physical, unaccounted, HP `1..baseHp`, in the actor closure; an unactivated actor
must remain at its spawn. Canonical Defeated is HP 0, `(-128,-128)`, inactive,
Physical and accounted. Occupancy is at most three. Wounded live and canonical
defeated values are representation/restore-valid even though M32 produces neither.
Unsupported status, partial removal, invalid identity/order/count, out-of-range HP
or coordinates and noncanonical lifecycle/accounting combinations reject.

Party validity retains M30's complete roster, fixed active-party membership,
order and count, checked rule inputs, exact signed HP/SP and independent current
maxima; at least one member must be able to act. Active conditions admit Disease
`0..255` and Unconscious/Dead `0..1`, with all other condition bytes zero.
Unconscious or Dead requires current HP <= 0; otherwise current HP must be > 0.
Item validity remains distinct from melee readiness. Poison/Sleep remain outside
M32 runtime admission although the base condition array can encode them.
Unknown/unequipped item bytes and all inactive-owner state remain exact.

All resource-valid disabled object/event identities and independent quest values
retain their base-save semantics. No possession-removal equivalence, paired
overlay requirement, quest-completion inference or inference from actor HP is
permitted. Each overlay identity is validated against its original resource,
including identities outside the party component; those overlays remain durable
state and do not authorize travel or events there. M32 fresh runs create no such
overlays, but independent values still round-trip.

The saved camera must be on Clouds map 23 inside the derived mainland with no
same-cell live contact, owed ranged/event/actor work, missing activation required
by current classification, modal work or unsafe presentation. Capture requires
the actual bound graph at presented Quiet with pending zero and no integrity or
fatal latch. Refusal occurs before capture, providers, preflight and file I/O.

A possible ranged attack on a future movement opportunity does not itself
create pending work; an owed but skipped attack prohibits Quiet. Pure view
reconstruction may verify required activation, but must not repair it by
activating actors.

Startup restore is atomic with respect to final owners:

1. Decode and validate the envelope, schema/content, archive signature and
   structural values.
2. Build unpublished party/context/camera/flag candidates; reload and validate
   original map-23 DAT/MOB/EVT/statistics, mainland and actor closures.
3. Construct all 19 actors, apply every saved live field/accounting bit and
   independent overlays, then validate complete current state.
4. Compose the first frame under candidate/destination guards without fresh
   CHR/PTY preparation, activation, movement, time, event, combat, item or RNG
   work.
5. Publish once into fresh owners, consume the restore-only Journey binding and
   remain unsaveable until the matching frame is presented.

Incompatibility fails startup without normalization or fresh fallback. Component,
actor closures, resource payloads, pending work, support stops, contact/event/UI
state, presentation generations and runtime capabilities are never serialized.
Legacy v1/v2, completed v3 and v4 schema/content 1/1 and 2/2 behavior remains
unchanged.

## Successor handoff and exclusions

The approved arc remains, in order:

- **M33 - Faithful mainland combat and consequences** consumes the accepted
  regional actors, HP, RNG, context and support seams.
- **M34 - Original disengagement and encounter lifecycle** consumes the same
  surviving owners; an M32 support stop is not a Run implementation.
- **M35 - Connected Myra quest and local recovery** may connect the accepted M21
  endpoints under explicit event authority. Geometry membership grants no quest
  or recovery permission.

M32 completion does not authorize any successor. General magic, Rest,
shops/temples/inns/services, training/full progression, movement abilities,
unrestricted/disconnected map-23 areas, Vertigo/mines/adjacent maps/Darkside,
original-game startup, arbitrary scripts, mid-action/modal/combat saves,
in-session loading, autosave, original save compatibility and commercial-data
modification/distribution remain excluded.

## Final acceptance

The implementation passed the full build and final full CTest suite (**93/93**),
the applicable regional original-resource and event validation, legacy Skeleton
Journey and expedition/Bone Whistle regressions, regional resource-authority
matrices, and the separate-process SDL/runtime matrix (**16/16**).
`git diff --check` passed before physical acceptance.

Independent technical review returned **ACCEPT**. The final accepted invariant is
that immutable preimages survive guard renewal/cache eviction, DAT/MOB internal
identity conflicts permanently latch failure, matching retries cannot reopen
Quiet/capture authority, and genuine provider I/O/allocation failures remain
recoverable. Fresh/restored graphs and representative equipment, transfer,
navigation and pulse renewal paths preserve that rule without regressing the
schema-3, event/sign, support-boundary or legacy Journey contracts.

The maintainer completed physical native SDL acceptance with the unmodified
original installation. This covered fresh 19-actor regional startup; truthful
Myra and map-23 transition refusal; both quiet actor-movement witnesses and their
separate-process save/restores; a post-restore inventory mutation and second
restart; ranged `SupportStopped` with later gameplay/F9 blocked; automatic sign
presentation; ordinary water/mountain collision; naturally encountered Orc
ranged support; normal actor movement/presentation; and legacy Skeleton Journey
and Bone Whistle expedition/collection smoke tests.

Milestone 32 is therefore complete and physically accepted. M33 remains the next
separately specified and authorized milestone.
