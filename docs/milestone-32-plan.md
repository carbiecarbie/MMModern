# Milestone 32 - Regional Journey foundations

**Proposed technical contract; awaiting review.** This plan does not authorize
implementation or record milestone completion. Baseline:
`4e03c36c30ad2c029001c00ddbf538b50c1aeb27` (`docs: record approved post-M31 roadmap`).
The architecture/specification investigation verified `main`, HEAD, origin/main
and direct remote main at that SHA, with a clean tree and index before work.

## Objective and acceptance boundary

Establish a resource-driven, map-local Journey domain for the connected mainland
of Clouds map 23. Replace corridor-shaped admission for this new domain with
reusable geometry, traversal and actor rules, complete current actor ownership,
checked time/context advancement, and exact save/restart continuity. Retain the
existing Application, world, party, Journey/encounter, event, presentation and
persistence owners. Do not create another Journey framework.

The [approved roadmap](roadmap.md#approved-m32-m35-arc) owns successor scope.
M32 supplies foundations for that arc; it does **not** certify unrestricted
mainland gameplay, mainland combat or the connected Myra quest. Real actors stay
present and can end an M32 run at a truthful support boundary. Geometry coverage
and runtime traversal with actors are separate acceptance obligations.

The accepted [M29 mutable-domain contract](milestone-29-plan.md),
[M30 grouped expedition contract](milestone-30-plan.md), and
[M31 event/publication contract](milestone-31-plan.md) remain authoritative for
their respective content. M26's action/pulse and actor rules, M27's publication
units and M28's retained-owner/restoration safeguards are inherited by reference:
[M26](milestone-26-plan.md#final-ownership-and-lifecycle),
[M27](milestone-27-plan.md#ownership-phases-and-publication),
[M28](milestone-28-plan.md#fresh-owner-restoration).

## Verified facts and source interpretation

### Implemented baseline

The following are code findings, not assertions that regional play already works.

| Component | Relevant implemented behavior and M32 consequence |
| --- | --- |
| [`XeenJourneyContent.h`](../src/games/xeen/XeenJourneyContent.h) | Contracts 1/2 describe map-20 rectangles, one/four influencing identities, hard-coded actor boxes, two blocked terrain cells and fixed days. A third descriptor alone cannot generalize Journey. |
| [`XeenMovement::apply`](../src/games/xeen/XeenMovement.cpp), [`XeenWorld::sampleCell`](../src/games/xeen/XeenWorld.cpp) | Outdoor collision uses destination middle/surface values. Sampling resolves the 3x3 neighbor plane, Y before X, and movement can commit the sampled neighbor's map ID. Regional movement must impose its local boundary before that commit. |
| [`XeenActorApproach`](../src/games/xeen/XeenActorApproach.cpp) | `actorsFromResources` already constructs a complete, original-order collection. Initialization, `bounded(..., kEntry)`, domain/environment validation, terrain queries and `transition` still assume map 20, 27 actors and the old footprints. Non-influencers must remain unchanged. |
| [`XeenMonsterFormat`](../src/formats/xeen/XeenMonsterFormat.cpp) | `supportsMovement()` currently rejects every ranged profile through raw byte 32. Rendering is a separate predicate. `validateCombat()` accepts only the bounded Skeleton/Zombie statistics. Movement admission cannot continue treating ranged capability as an unconditional loading/movement rejection. |
| [`XeenJourneyFlow.cpp`](../src/app/XeenJourneyFlow.cpp) | Fresh/restore bindings, leases and capture authority already exist. Action entry currently demands melee readiness whenever any admitted enemy lives, even away from contact. Regional navigation must select its own action capability without invoking the map-20 melee consumer. |
| [`XeenGameplay.cpp`](../src/app/XeenGameplay.cpp), [`XeenEventFlow.cpp`](../src/app/XeenEventFlow.cpp), [`main.cpp`](../src/main.cpp) | Fresh event loading still requests map 20; sprite preparation iterates descriptor identities; notices/CLI discriminate only two contracts. Journey intercepts ordinary automatic dispatch. Manual non-objective interaction currently manufactures NoEvent because its old footprint was proved event-free. That shortcut is invalid on map 23. |
| [`XeenSaveState.cpp`](../src/games/xeen/XeenSaveState.cpp), [`XeenJourneyCapture.h`](../src/games/xeen/XeenJourneyCapture.h) | Capture/restore assume 27 actors, map 20, one/four saved actors, and full-HP living survivors. Restore reconstructs resources into unpublished owners, applies live state, then binds once without replay. |
| [`XeenSaveFormat.cpp`](../src/formats/xeen/XeenSaveFormat.cpp) | V4 explicitly accepts only schema/content 1/1 or 2/2 and suffix lengths 1060/1366. The 19-byte actor record actually ends with activated, lifecycle, status, accounted. The M30 paragraph describing defeated/pending-move/pending-direction bytes is inconsistent with codec, snapshot and tests; use the implemented meanings. Closed plans remain unchanged in this task. |
| [`XeenGameplayContext.h`](../src/games/xeen/XeenGameplayContext.h), [`XeenJourneyRules.cpp`](../src/games/xeen/XeenJourneyRules.cpp) | The context already stores a calendar, ctr24, effects and rested/newDay. Validators restrict it to a fixed day/year and minutes 480..959; it is not a general time-effect implementation. |

Relevant existing regression homes are `XeenMovementTests`, `XeenWorldTests`,
`XeenActorApproachTests`, `XeenJourneyTests`, `XeenJourneyPersistenceTests`,
`XeenJourneyProcessTests`, `XeenJourneyGameplayTests`, `XeenExpeditionTests`,
`XeenExpeditionGameplayTests`, `XeenJourneyEventTests`, and the save/restore and
navigation/event suites under [`tests/`](../tests). They establish legacy
behavior, not regional acceptance.

### Original-resource facts

Read-only investigation used the supplied World of Xeen installation. Original
initial members are reconstructed in memory through the same block order as
`ScummVmXeenBridge::Impl::initial`: outer XEEN.CC members `2a0c`, `2a1c`, `2a2c`,
`2a3c`, `284c`, optional `2a5c`; decode the inner CC index, whose payload is
already plaintext. No extracted game data belongs in the repository.

| Decoded member | Bytes | CRC32 | SHA-256 |
| --- | ---: | --- | --- |
| `maze0023.dat` | 892 | `8f3e28ee` | `5227abf98660ca6e90e89392669fad37a809820eccacdde6e6af4f5e63d3c1e4` |
| `maze0023.mob` | 220 | `ce08a8c8` | `ba45d2deeee52fe3f8938a6352e3dc598b16e670d0fee2d6005c03710a44508a` |
| `maze0023.evt` | 1440 | `e3128711` | `b3ee7d9fbd8dcb503983f2f3e201f28a2dce7c7fb5f1e06028e366fe8b223ed4` |
| `DARK.CC/xeen.mon` | 5400 | `4dec3f50` | `fd04e2408a444cb9ff8b3f5f16badfc6733bc3d973e56c3275edfa787daef52b` |

The DAT has ID 23, neighbors N/E/S/W = 22/0/24/19, flags 0,
flags2 `0x8000`, wall-no-pass difficulty 0 and Run coordinates `(10,12)`.
Run metadata is evidence for the successor, not an M32 relocation command. In the
decoded map-23 DAT, `runX` is at offset `815`, the `MazeDifficulties` bytes are at
`816..823` as `00 00 00 0A 00 00 00 64`, and `runY` is at offset `824`.
Run remains an M34 concern.
With ordinary Clouds collision and no effective Swimming/Walk on Water/
Mountaineer, the component containing `(9,11)` has **121 cells**. Other component
sizes are 1, 4, 1, 15, 9 and 2. Both Myra `(9,11)` and Phirna `(8,2)` belong to
the mainland. Its actual surface/middle pairs and counts are:

| Actual surface | Middle | Cells |
| --- | ---: | ---: |
| Dirt (1) | 0 | 17 |
| Dirt (1) | 3 | 16 |
| Grass (2) | 0 | 40 |
| Grass (2) | 2 | 23 |
| Road (7) | 0 | 25 |

These are observations, **not a 121-cell whitelist or admission algorithm**.
There is no mainland desert, lava, sky, cloud, water or space step effect to
implement. Top/overlay values affect existing composition, not this collision
decision. Middle 3 must retain its existing special passability; do not replace
the literal movement predicate with a generic surface-only test.

The MOB has 20 objects, **19 monsters**, and one wall-item record. Monster-table
slots 0..4 resolve to types 6, 3, 13, 8, 9. All 19 monsters are initially live,
resolved, Physical and unaccounted. Identities are `{Clouds,23,originalIndex}`:

| Type / image / base HP | Original record indexes and coordinates |
| --- | --- |
| Orc / 6 / 25 | 0 `(0,13)`, 1 `(0,15)`, 2 `(1,13)`, 3 `(2,15)`, 4 `(2,12)`, 5 `(3,12)`, 6 `(4,14)`, 7/8 both `(3,10)`, 9 `(6,12)`, 10 `(7,14)`, 11 `(9,15)` |
| Giant Snake / 3 / 15 | 12 `(5,0)`, 13 `(13,8)` |
| Giant Toad / 13 / 90 | 14 `(6,1)`, 15 `(7,3)`, 16 `(14,2)` |
| Skeleton / 8 / 20 | 17 `(1,2)` |
| Zombie / 9 / 30 | 18 `(1,1)` |

Orcs 1/3/6/10/11 start on mountain middle 1 outside party traversal; Snake 13
and Toad 16 start on disconnected land. These remain simulation participants.
The five 60-byte statistics records have CRC32, in type order 3/6/8/9/13,
`7f7e3f71`, `eb3b54b1`, `e36833c6`, `5002c318`, `5636ae25`.
Orc raw byte 32 (range) is 1; all five profiles have flying byte 46 zero.
Special byte 30 is Snake=5, Toad=9, Zombie=7. Loading those facts does not execute
Poison, Sleep or Disease. Type 59 is absent.

The EVT has 170 records. Mainland physical event addresses are `(0,1) All`,
`(4,5) W`, `(5,9) N`, `(5,13) N`, `(7,7) All`, `(8,2) All`, `(8,10) N`,
`(9,11) W`, `(10,13) All`, `(12,12) All`. Only mainland cell `(5,9)` has the
automatic bit `0x10`: record 56, offset 495, line 0, opcode `04 SignLabel`,
operand `10` hex (text index 16), with no line 1. Object 7 is the sign.
`XEEN.CC/aaze0023.txt` has 2368 bytes and 44 NUL-terminated entries; index 16
is present and nonempty (10 bytes). Text is read from the outer archive through
the existing text loader, independently of the initial DAT/MOB/EVT archive.
Records 0/2 at `(5,13)`/`(8,10)` use unsupported opcode `11` with operands
`0A`/`0C`. Records 136..139 at `(10,13)` include a prompt and teleport to map
28 `(15,0)`. None becomes safe merely because its cell is reachable.
The [M21 endpoint contract](milestone-21-plan.md#myra-script-and-exchange) owns
Myra/Phirna's already accepted script details; M32 does not re-specify them.

Initial PTY context is day 1/year 610/minute 480/ctr24 0, zero effects and rested
false. CHR skill bytes begin at record offset 39: effective party Swimming is
false (only owners 0/1 have it); Mountaineer and Navigator are absent; only owner
14 has Pathfinder. Pinned `Party::checkSkill` requires all members for Swimming,
two for Mountaineer/Pathfinder and one for Navigator. The declared effective
capabilities below therefore do not strip a functioning original traversal skill.

### Pinned reference facts

The external checkout was verified clean at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, as required by
[dependencies](dependencies.md). Paths below are relative to its
`engines/mm/xeen/` directory:

- `interface.cpp`, `checkMoveDirection`, `chargeStep`, `stepTime`, `doStepCode`:
  Clouds destination collision, outdoor ten-minute charges, ctr24 modulo 24,
  and surface effects are separate operations. Turning calls step code without
  a ten-minute charge. Desert adds time in step code; it is absent here.
- `combat.cpp`, `moveMonsters`, `canMonsterMove`, `moveMonster`: original-order
  occupancy, two-pass local scheduling, persistent activation, terrain fallback
  and at most one move per actor per opportunity. Actor collision differs from
  party collision and can leave an initially non-traversable spawn.
- `combat.cpp`, `moveMonsters`, `setupMonsterAttack`, `stopAttack`: an activated,
  not-yet-moved ranged actor in the 7x7 scan, aligned on either axis and outside
  the three current contact identities, can shoot **before** its movement.
  Physical status is required. Outdoor `stopAttack` is asymmetric: east rays
  test raw wall-word mask `0x8`; west/north/south rays test middle values against
  `0,2,4,5,8,11,13,14`. Facing changes projectile presentation, not whether an
  unobstructed attack from behind is relevant. Do not substitute rendered
  visibility, activation queries, Euclidean distance or a symmetric ray test.
- `party.cpp`, `changeTime`, `addTime`, `resetTemps`: 480-minute boundaries run
  stat/condition processing, including RNG under the literal zero-Poison and
  zero-Disease branches. Midnight rolls minutes at 1440, days at 100 and sets
  newDay; processing at/after minute 300 includes temporary resets and rested/
  weakness behavior. Calendar changes can also reach shops/bank processing.
  Night presentation changes at 300/1260. A larger integer alone does not
  implement these effects.

## Regional navigation and production entry

### Descriptor, geometry and traversal

Add **content contract 3**, selected explicitly as a Regional Journey. Keep
contracts 1/2 exact. The immutable descriptor supplies Clouds/map 23, anchor
`(9,11)`, fresh facing West, effective traversal capabilities, compatible
resource policy and action/event admissions. It contains no actor-coordinate
boxes, route list or stored reachability mask.

Factor the existing outdoor collision calculation into a pure query reusable
by ordinary movement, regional candidate movement and component derivation.
Retain legacy results and precedence. Its inputs are checked original geometry,
source/destination coordinates and a declared capability value. The regional
capabilities are Swimming=false, WalkOnWater=false, Mountaineer=false;
Navigator/Pathfinder do not add terrain or time exceptions in this region.
They are immutable contract inputs, not new mutable skill owners or serialized
aggregate booleans. Fresh loading verifies the effective CHR/PTY prerequisites.
Per-character skill progression and ability acquisition remain outside the arc.

Build the mainland by a bounded four-neighbor flood from the anchor using that
query, with both endpoints in local `[0,15] x [0,15]`, same Clouds/map identity,
and no wrapping. Check the anchor itself is traversable. For this destination-
based outdoor predicate the admitted edges are bidirectional between passable
cells; do not infer that property for future directed/indoor movement.
The derived component is disposable policy data tied to exact immutable map
content and capability inputs. Rebuild after cache discard and compare against
the retained compatible content; never use a stale map pointer as its identity.
An in-memory bitset is acceptable; a hard-coded or serialized cell set is not.

The distinctions are explicit:

| Question | Authority / rule |
| --- | --- |
| Can the geometric component contain this cell? | Local adjacency plus original party collision and declared capabilities; excludes the six disconnected components. |
| Can the party perform this input now? | Component membership plus live party validity, time admission, actor work, event/transition admission and presented input authority. |
| Can an actor move or influence the party? | Its original identity/profile, live state, activation, original local scheduler and actor terrain/ranged rules; never party-component membership. |
| Can an interaction/destination execute? | Explicit event/action capability checked against the original physical/logical address and records, independent of geometry. |

Neighbor DAT sampling for outdoor rendering remains legal through
`XeenWorld::sampleCell`, including Y-before-X diagonals. It grants neither actor
collections from neighboring maps nor party travel. Regional movement checks
local bounds before a neighbor destination can commit and reports a map-local
boundary. Rendering still uses the original neighbor geometry. Missing/malformed
required view resources fail under existing presentation guards, without
substituting terrain or admitting the neighbor as gameplay.

The new mainland movement support is ordinary grass/road and the existing
middle-2/middle-3 branches, regional actor terrain, event dispatch ordering and
map-local admission. No new party movement ability or hazard damage is needed.
Passable but unsupported travel refuses without camera/time/RNG mutation;
ordinary collision preserves its existing result and actor-pulse scheduling.
Refusal is not a way to cancel already pending work.

### Fresh construction and controls

Add `--journey-region [--combat-seed <nonzero-u32>] <game-directory>
[--save-file <path>]`, through the existing strict CLI and
`Application::playGameplay` Journey setup. The historical seed option name is
retained for consistency; it seeds the world gameplay RNG, not a new encounter.
An explicit seed bypasses sampling; otherwise sample once and map zero to one,
as in M29/M30. No save target means no F9 destination. Existing commands and
resume meanings remain unchanged; `--load-game` rejects fresh contract/seed overrides.

Fresh contract 3 starts at Clouds/23 `(9,11)` West. Use the same explicit active
level/XP/HP/SP preparation and original item preservation as
[M30 prepared entry](milestone-30-plan.md#content-descriptor-and-prepared-entry),
including day 8/year 610/minute 480/ctr24 0 and all 30 Luck-bearing supplements.
Factor that preparation for reuse; do not duplicate it in a regional constructor.
This is a declared prepared entry, not original-game startup or earned training.
Load original flags, quest values, inactive characters and item bytes once;
zero no extra flags, remove no objects, upgrade no gear. Initialize all 19 actors
once, classify/activate the starting view without movement or RNG, bind the
existing Journey coordinator and require the first successful presented frame.
Do not dispatch Myra automatically at entry.

Arrows/WASD move/turn; period Wait uses the existing action/pulse path. I and the
existing inventory/transfer/equipment controls remain available at quiet
presented boundaries; F9 saves only with inventory closed. Space requests an
admitted interaction. Attack, Block, Shoot, Run, Rest and Begin/Revisit are not
regional commands in M32. Escape/window close exits normally. Do not add a
production actor-disable, teleport, invulnerability or support-stop-resume key.

The native view must show a readable regional boundary/action notice, map and
cell/facing, game time and available controls. Existing I inspection reports
contract, context, RNG state/count and all keyed actor coordinates/HP/activation/
lifecycle/accounting, with no hard-coded `contract=2` label. Unsupported-action
notices identify action, actor or event record and coordinate where applicable.
Cosmetic rebuilding and idle animation never advance gameplay.

### Events and transitions

Keep `XeenEventFlow`/`XeenEventSystem`/`XeenEventInterpreter` as the event owners.
For contract 3 admit just original sign record 56 at `(5,9)` North, through the
existing SignLabel/text presentation path, automatic or manually requested.
Load `aaze0023.txt` index 16 through the normal text provider. Require the exact
one-record line-0/natural-end contract; effective None after an independently
valid disabled overlay is also legal. Grant, Remove, Call, teleport, reward,
selection and flags are not permissions of this sign continuation.

Use an explicit read-only event admission in `XeenEventPublication`, preserving
the contract-2 Bone Whistle capability exactly. Bind original record/address,
physical camera, immutable script, owner preimage and Event lease. No generic
"all implemented opcodes are safe" admission, detached result injection or
Journey bypass of the publication capability is allowed.

Automatic dispatch must be considered at fresh initialization and after ordinary
navigation dispatch opportunities, including turns and actual blocked moves,
using the existing automatic bit and first-match/facing rules. Fresh regional
entry has no trigger; startup restore performs none. Wait and idle do not invent
an additional automatic dispatch. For a navigation-triggered sign, retain one
transient pending address/input generation, finish already owed approach pulses
first, then transfer directly to Event when quiet; hold new gameplay and saving
throughout, with no presented quiet gap. If actor work stops, discard that
pending dispatch without executing it. This extends M31's actor-priority rule
to automatic work; it does not suppress movement to reach a sign.

Preflight automatic destination/facing admission before the navigation candidate
publishes. An unknown active automatic chain refuses that movement/turn (or
stops current mandatory work if discovered after publication); it is never
silently ignored. Wrong-facing lookup with no matching line 0 follows ordinary
NoEvent semantics. Validate immutable original topology even when effective
records are disabled. Malformed or changed resources fail compatibility rather
than being interpreted as an empty cell.

All other mainland manual addresses above refuse **before interpreter entry**
with a supported-boundary notice and no effects/time/RNG. This includes Myra,
Phirna, containers, recovery/services and every transition. Walking onto such
manual-only cells is allowed: the automatic bit is absent, so no required
automatic gameplay has been skipped. `(10,13)` never opens its question and then
fails after acknowledgment; the entire unsupported chain is refused up front.
A cell/facing with no original matching event reports actual NoEvent from
checked lookup, not the legacy fabricated value.

Every exit/teleport/call destination is denied in contract 3, even when another
ordinary mode already implements it, including map-local teleports. Vertigo,
mines, map 28 and adjacent-map/Darkside entry remain excluded. No accidental
initial dispatch, manual bypass, event resume or restore can cross this gate.

The sign uses M31's Event-to-Presentation handoff and response generation rules.
Its intermediate pages, if any, are exclusive; no item/navigation/save input can
cross them. Terminal presentation retains legitimate sign text without holding
an event cursor. Recoverable manual failures require a fresh recovery frame;
automatic failure retains the existing fatal policy. Neither retries nor cache
reconstruction redispatch the sign. No M32 event publishes durable mutations.

## Regional actors and authority

### Loading, movement and influence

Load the whole original MOB monster list into `XeenSessionWorldState::_actors`
through `actorsFromResources`, once. Keep every record and original index,
including actors on excluded party terrain. Validate the map-23 original count,
type/table identities, coordinates, statistics and ordering independently from
live fields. Never select actors using current visibility, a mainland-cell test,
or an M32 combat allowlist. Keep ordinary object/event disabled sets independent.

M32 makes x/y and persistent activation mutable for **all** regional live actors.
Current HP, lifecycle, Physical status and accounted identity remain authoritative
world values that movement and reconstruction preserve; M32 produces no damage,
death or rewards. Name, resource/type/image, spawn metadata, full statistics and
original record order remain resource-derived immutable values. No redundant
monster-statistics owner or per-view actor copy becomes gameplay authority.

Reuse all existing outdoor MON placements, original-order selection and
occlusion. The five region profiles' normal sprites are selected through their
resource image fields, without requiring `validateCombat` or ATT readiness.
Validate selected rendering resources and fail visibly if unavailable; do not
drop an original threat to produce a frame. Disabled/Unresolved/Defeated states
remain excluded from active selection under their real lifecycle, not by
overloading an "unsupported combat" status. Keep occupancy's inherited checked
coordinate behavior and three-actor capacity.

Separate normal-ground movement capability from ranged action capability in the
existing monster-profile predicates. Region types 3/6/8/9/13 may activate and
move with Physical status and nonflying statistics. Preserve the existing 7x7
scan, two passes, original-order inner loop, moved-once set, direction priorities,
zero deltas, occupancy and fallback behavior. Grass/road are real actor terrain,
not "unsupported because not dirt". The regional destination predicate follows
pinned `canMonsterMove`: middle `0,2,3,4,5,6,8,11,13,14` takes the surface branch;
water/deep water requires flying/type 59, space requires flying, ordinary land
admits non-59 Clouds actors. Other middle values compare with wallNoPass.
Nonflying water/space and mountain values above the threshold are legitimate
Blocked results, permitting the literal fallback; they are not integrity errors.
Unknown movement modes or semantics are Unsupported, never silently Blocked.

Actor support is the **whole local map**, not the mainland component. A monster
on mountain or disconnected land can activate, move onto allowed adjacent land,
and affect the party when the scheduler makes it relevant. Validate moved
positions against the actor's directed terrain-reachability closure seeded by
its original spawn (include that spawn even when it is not an admissible
destination). This derived per-profile/spawn closure checks geometric possibility,
not a claim to reconstruct its actual movement history. Do not serialize it.
The 19 actors' in-bounds spawns and pursuit toward an in-bounds party need no
actor map transfer. Any attempted out-of-map actor destination is a support stop
before publication, not a neighbor spawn or clamped coordinate.

Activation still follows projection query matches before occlusion and sticks
after leaving the view. An activated actor outside the scan does not move under
the pinned rule; that legitimate inactivity is distinct from freezing a relevant
unsupported actor. Unactivated actors can occupy terrain and consume capacity.

### Unsupported actions and ordering

Before each candidate actor step in the literal scheduler, evaluate ranged
relevance using the pinned alignment/contact/activation/status and `stopAttack`
tests described above, at that scan point. Include both passes and candidate
positions created by earlier actors; do not check only the pre-op actor array.
For an unobstructed Orc ranged attack, publish terminal **SupportStopped** before
attack setup, movement of that operation, random draws or damage. A blocked ray
continues to ordinary movement. Newly activated actors become eligible when the
original ordering next supplies a movement opportunity, not immediately during
pure composition.

M32 does not attach **any** regional combat coordinator. Same-cell live contact
reached after a supported movement/classification publishes the resulting
position/time/activation facts, then a terminal regional-combat support stop in
that same guarded boundary. This applies also to the region's Skeleton/Zombie:
their old combat consumer is still map-20-bound. It does not imply their resource
statistics are unknown. Their legacy Journey combat remains fully available.
M33 will admit regional combat through that existing consumer rather than a
second regional implementation.

Publication rules are precise:

1. Retain entry owners, revision, activity, lease and preimage. Prepare a
   detached action/pulse candidate, including checked camera/context and any
   old pending opportunity. All fallible providers and storage preparation
   precede gameplay stores.
2. A discovered unsupported ranged/movement/temporal operation discards that
   entire unpublished action/pulse candidate and stops only still-current work.
   Earlier *published* actions, activation, actor movement, items and other
   effects survive. For example, a refused Wait does not charge ten minutes,
   while a prior successful Forward remains committed.
3. A supported action/pulse publishes its known facts with nonthrowing stores
   and updates the retained expected state before a callback can run. Contact
   termination includes those facts; it does not roll back a committed move.
4. Adopt result/terminal state before formatting, reporting or composition.
   Clear pending work as a terminal retirement of that work only, never as
   successful combat End, victory, defeated accounting or quiet authority.

SupportStopped permits exit and safe presentation-only reconstruction. Subsequent
movement, Wait, interaction, item mutation, Attack/Block/Shoot/Run and F9 refuse;
there is no escape-from-support-stop gameplay. A truthful stop frame and complete
inspection output must remain observable: emit the complete retained inspection
when reporting the stop, without reopening a mutable inventory panel.
Last valid disk save is unchanged.
No pending save is queued and no save provider/file access occurs at a stop.

Keep M26-M31 retained preimages, owner incarnation/ABA protection, marked owner
copy/move restrictions, monotonic invalidation, callback/reentrancy checks,
borrow ordering, boundary leases and successfully presented input generations.
Stale work cannot fail a newer operation; restoring equal bytes cannot revive a
failed guard. Destruction cannot reopen Quiet. Integrity/fatal presentation
failure permanently closes the affected graph. A recoverable composition retry
rebuilds the adopted facts only. Saving and modal exclusion cover direct APIs
as well as Application-intercepted SDL F9 and same-batch stale keys.

## Durable state and persistence

### Owners and compatibility policy

Retain the v2 base (complete characters/items, ordered membership, quest counters,
quest flags, game flags and separate disabled object/event sets). The party owns
the one `encounterContext`; roster owns all 30 supplements including Luck and XP;
world owns contract, the complete actor vector, identity accounting and the one
M30 algorithm-1 RNG continuation. Preserve seed/state/raw-draw-count semantics.
M32 navigation/sign/actor movement draws no gameplay RNG. Cosmetics have none of
its authority.

Use **v4 envelope, Journey schema 3/content 3**. V4 already identifies an
extensible Journey domain, so a new envelope is unnecessary. Schema 2 cannot be
silently reused: its codec requires exactly four specified map-20 records and a
1366-byte suffix. Schema 3 explicitly means full initialized-map actor coverage.
Keep 1/1 and 2/2 byte-for-byte; reject other crossed/unknown pairs. Never infer a
contract from the camera or convert a legacy save on load/save.

For fresh production compatibility, check the decoded map-23 DAT/MOB/EVT sizes
and CRC32 values above and the five referenced MON record CRC32 values, along
with typed format/identity/semantic validation. These are a content manifest,
not authentication or a stored resource payload. Reuse existing CRC32 facilities.
The algorithm still derives geometry from those resources; the manifest does not
replace it with cells. Tests of generic component/rule logic may use synthetic
resources, but production cannot bypass this compatibility gate.

Save-to-installation compatibility retains the existing complete archive
size/CRC32 signature, including required DARK.CC. Rebuild and validate immutable
DAT/MOB/EVT/statistics on startup and cache reload under the retained provider
guards. A compatible archive signature alone does not authorize changed
in-memory topology. Disabled overlays do not modify the immutable comparison.
Referenced neighbor geometry remains original and covered by archive signature
and guarded reloads; it never initializes another live actor collection.

### Exact schema-3 encoding

The extension starts immediately after the v2 disabled-event list. All multi-byte
numbers are little-endian; strict boolean and enum encodings retain their existing
meanings. The 4 MiB bound, envelope length, reserved bytes, CRC32 and exact EOF
checks apply unchanged.

| Offset | Field / representation |
| ---: | --- |
| 0 | domain u8 = 3 (Journey) |
| 1 | schema u16 = 3 |
| 3 | content contract u16 = 3 |
| 5 | context-present u8 = 1 |
| 6 | existing 33-byte context, unchanged field order |
| 39 | supplement count u8 = 30 |
| 40 | 30 owner-ordered 41-byte M30 supplements, owners 0..29 |
| 1270 | RNG algorithm u8 = 1 |
| 1271 | RNG state u32, nonzero |
| 1275 | committed raw-draw count u64 |
| 1283 | initialized map: side u8 = Clouds (0), map u16 = 23 |
| 1286 | original actor count u16 = N |
| 1288 | saved actor count u16 = N |
| 1290 | N original-index-ordered 19-byte actor records |

Length is `1290 + 19*N`; the verified contract-3 resource has N=19 and a
**1651-byte suffix**. Decode N with bounds `1..107`, require the two counts to
match, check exact remaining length before allocation, then require the
contract/resource count 19. This is full coverage, not an omission-means-spawn
delta encoding. Every identity must be Clouds/23/index `0..N-1` in order.

Each actor record is side u8, map u16, index u32, x i16, y i16, HP i32,
activated u8, lifecycle u8, status u8, accounted u8. Lifecycle values are
Present=0, Disabled=1, Unresolved=2, Defeated=3; status Physical=0,
Unsupported=1. Context is profile u8, difficulty u8, ctr24/day/year/minutes u16,
nine effect u8 values, six light/resistance u16 values, rested/newDay u8.
Each supplement is owner u8; permanent/temporary Might, Speed, Accuracy and
temporary AC as seven i32; XP u32; permanent/temporary Luck as two i32.
Profile WorldOfXeenClouds is 0; difficulty Adventurer=0, Warrior=1 structurally,
with only Adventurer admitted by content 3. Every boolean must be exactly 0/1.
Retain the existing wire bounds x/y `[-128,31]` and HP `[0,65535]`, followed by
the stricter resource-dependent lifecycle/position/HP checks below.
Reject absent Luck, out-of-byte-origin supplement inputs, invalid enums/booleans,
duplicate/missing/extra/reordered IDs, wrong map/side and nonzero legacy seed.

Do not serialize the region component, per-actor geometric closure, original
statistics, capabilities already fixed by contract, pending movement, support
stop, contact slots, event cursor, modal state, time-effect candidate, combat
round, Run state, presentation generation, owner pointer or capture certificate.
No gold/loot/treasure state is produced in M32; their M33 owners/encoding must be
specified with their actual consumer, preserving this schema's reader. There is
no claim that schema 3 is the final format for every successor consequence.

### Value admission versus produced states

Regional camera must be on map 23, in the derived mainland, with facing 0..3 and
an admitted automatic-event boundary. It need not lie on a chosen witness route.
There must be no same-cell live contact at quiet capture/restore, no pending
mandatory actor/event work, and no missing activation required by pure current
classification. Rebuilding a view may verify this; it may not repair it by
activating actors. A possible ranged attack on a *future* movement opportunity
does not itself invent pending work; an owed but skipped attack prohibits Quiet.

For Present actors admit Physical, unaccounted, HP `1..resource.baseHp`, local
coordinates in the derived actor closure; an unactivated actor remains at its
original coordinates. HP may be below maximum even when unactivated. Wounded
living actors are valid durable values and movement preserves their HP exactly.
M32 has no damage producer; synthetic wounded-value tests must be labeled
representation/restore tests, not an M32 combat witness. This removes the
full-HP-survivor assumption before M33/M34 consume it.

Defeated is HP 0, `(-128,-128)`, inactive, Physical and accounted, with a matching
original identity. M32 has no producer of this settlement; resource-valid
canonical values may be restored without manufacturing runtime End authority.
Present+accounted, zero-HP Present, HP above base, partial removal sentinels and
Unsupported status reject. Disabled/Unresolved are structurally representable
but require corresponding immutable original records; this installation has
none, so they cannot replace its live actors. Occupancy is checked at three per
cell, and original metadata is never rewritten to justify a live position.

Party validity retains M30's complete roster, fixed active order/count, checked
rule inputs, exact signed HP/SP and independent current maxima; at least one
member can act. Retain M30's Disease/Unconscious/Dead condition domain and item
validity rather than conflating it with melee readiness. Poison/Sleep remain
outside M32 runtime admission although the base condition array can encode them.
Unknown/unequipped item bytes and all inactive owner state remain exact.

All resource-valid disabled object/event identities and independent quest values
retain their base-save semantics. No possession-removal equivalence, paired
overlay requirement, "quest complete" bit, or inference from an actor's HP is
permitted. Validate each overlay identity with its original resource, including
valid identities outside the party component; their presence does not admit
travel/events there. M32 fresh runs create no such overlays, but independent
values still round-trip. Legacy ordinary domains retain their own meanings.

### Capture and startup restore

Generalize capture's collection/identity checks and `admittedActors` handling:
all 19 regional actors are mutable live records; immutable facts remain checked
for all, rather than treating most as fixed bystanders. F9 requires the actual
bound graph, presented Quiet, pending zero, no contact/attachment/combat/Event/
automatic dispatch/inventory/save/frame obligation, and no integrity/fatal latch.
Eligibility refusal happens before capture, providers, preflight and file I/O.

Restore remains startup-only and atomic with respect to final owners:

1. Decode and check envelope, schema/content, signature and structural values.
2. Construct unpublished party/context/camera/flag candidates. Load compatible
   map-23 originals, derive region/actor validation data and construct all actors.
3. Apply **every** saved live actor field and accounting bit, and independent
   world overlays; validate regional values and complete current state.
4. Compose a pure first frame under candidate/destination guards. No CHR/PTY
   preparation, movement, activation, event dispatch, HP adjustment, XP, RNG,
   time advancement, encounter attachment or settlement may occur.
5. Prepare storage and publish once into fresh final owners as Unbound Journey;
   consume the existing restore-only Flow binding before the ordinary gameplay
   borrow, as required by M29/M31. Remain unsaveable until matching presentation.

Reject incompatibility without fresh fallback or normalization. Separate-process
resume must recreate no live authority from saved pointers or phases. It must
continue subsequent movement and item mutation on restored owners. Ordinary
v1/v2, completed Diagnostic27 v3, and both legacy v4 Journey pairs keep their
existing read/write behavior and startup dispatch policies.

## Time and context foundations

### Representation and supported advancement

Keep **one** party-owned `XeenGameplayContext`; the existing 33-byte representation
is sufficient for calendar/time foundations. Name/document its fields as durable
gameplay context even though its member is called `encounterContext`. No second
regional elapsed-time counter, deadline field, inferred day, pending-tick save
or unbounded total-minutes counter is needed.

Introduce one pure checked time-preparation operation used by regional actions.
It accepts current context and an explicit caller charge; returns either a
candidate context plus typed required temporal work, or an overflow/support
refusal. It cannot publish by itself. Factor calendar arithmetic and boundary
detection out of the old inline `minutes += 10` pattern. Do not switch legacy
contracts to broader admission during this milestone.

Canonical representation is day `0..99`, minute `0..1439`, year u16, ctr24
`0..23`, explicit profile/difficulty, exact effect/light counters and booleans.
Use wide checked intermediates; compute minute/day/year rollover before narrowing.
Year overflow rejects rather than wrapping. A pure calendar candidate must
identify midnight/newDay, 100-day year rollover, 300/1260 daylight boundaries,
each crossed 480-minute processing boundary and applicable daily processing.
Checking only the final minute's remainder is insufficient, including for a
multi-day proposed charge. Zero charge creates no `changeTime` call or RNG.
ctr24 is a distinct modulo-24 step counter, not elapsed minutes or a day index.

M32's **runtime advancement** admits only paths requiring no deferred time
effects: daytime, no pending newDay/rested work, zero party effects/light/
resistances, supported active conditions, and no 480-minute or daylight/calendar
crossing. Regional context value validation accepts canonical daytime dates
(`300 <= minutes < 1260`, valid year for every active character's rule use),
with those same state restrictions; it is no longer tied to the entry's day 8
or year 610. All age-dependent regional display/item calculations use the live
context year, not a literal 610. Wider wire representation is not permission to
restore unsupported night/effect/condition state.

Consequently the fresh prepared run's first unsupported ten-minute charge is
still **950 -> 960**. That is an original processing boundary, not an arbitrary
expedition deadline to extend. Restored, otherwise admitted daytime state in a
different processing interval uses that interval's next boundary. M32 does not
claim to have produced dates/times it cannot advance to. Tests must distinguish
calendar candidate/representation coverage from live publication coverage.

This deliberate remaining boundary is compatible with the approved foundation:
a geometry-only shortest Myra-to-Phirna route is 16 steps (32 for an immediate
return), but runtime actors and later combat/recovery may add enough time to
require temporal processing. M32 does not certify that later route's timing.
When M33-M35 route evidence requires crossing, its consumer must implement the
required condition/RNG/daily work or retain a truthful stop. The calendar/context
encoding and once-only publication interface already represent that result;
removing a stop must not require replacing context with another clock. No
deferred effects are silently treated as already executed.

### Charges, ordering and limits

| Action/work | Minutes | ctr24 / actor scheduling |
| --- | ---: | --- |
| Successful regional Forward/Backward | 10 | +1 modulo 24; flush old opportunity at candidate destination, arm count 3; separate normal pulse classifies/decrements as in M26 |
| Left/Right | 0 | +1 modulo 24; no new opportunity; normal pulse can service existing work |
| Period Wait | 10 | +1 modulo 24; flush old opportunity, then consume the newly armed opportunity |
| Actual terrain/boundary block or admission refusal | 0 | No new ctr24 charge/opportunity; ordinary nonterminal scheduling cannot discard old work |
| Due gameplay pulse | 0 | No ctr24 change; consume at most one due decrement/opportunity |
| Inventory/transfer/equipment, sign/manual refusal, save, redraw, idle cosmetics | 0 | No new movement/time/RNG; existing exclusivity/deadlines apply |

M31's zero-cost interaction adaptation remains explicit; do not copy the
reference's Space-as-Wait behavior into this regional UI. Existing combat
Round/End one-minute consumers remain in legacy domains. The checked operation
supports preparing a one-minute charge for tests/future callers, but M32 does
not introduce regional combat work.

Time preflight precedes camera/context/ctr24, old pending movement and actor
publication. A temporal support stop preserves the entire prior committed state,
retires pending work unsaveably and names the unimplemented boundary. It consumes
no would-be condition RNG. This includes the literal zero-condition random
branches in the pinned reference, not just visibly poisoned parties.
Confused/Paralyzed per-charge processing, full condition aging, Rest, services,
temporary-effect reset and recovery are deferred. No timer based on wall-clock
idle or save/load catches up those effects.

Retain checked u64 revisions/generations and RNG draw-count exhaustion, bounded
pending `0..3`, safe deadline arithmetic and backward-clock refusal. Due idle
delivers at most one logical pulse and rearms from now; no elapsed backlog replay.
M22 ordinary-object animation and actor cosmetics remain separate, using their
existing reset/advance/reconstruction rules. Cache removal, stop presentation,
modal redraw and separate-process loading consume zero game time and RNG.

## Dependency-ordered implementation units

These are exit criteria within M32, not authorization for successor milestones.

| Unit | Affected components | Exit criterion |
| --- | --- | --- |
| 32A - Admission and checked context rules | `XeenJourneyContent`, `XeenMovement`, `XeenGameplayContext`, `XeenJourneyRules`, map/profile query helpers | Contract 3 and immutable resource checks; reusable local component/actor-terrain queries; normalized calendar candidate and explicit unsupported-effect detection; legacy predicates unchanged. |
| 32B - Complete regional owner and approach domain | `XeenActorApproach`, `XeenMonsterFormat`, `XeenWorld`/session, retained equality/guards | All 19 original actors retained; activation and whole-map movement; pre-move ranged support stop and contact stop; atomic candidates, exact retained effects and terminal authority tests pass. |
| 32C - Regional persistence | `XeenSaveSnapshot`, `XeenSaveFormat`, `XeenSaveState`, `XeenJourneyCapture`, restore-only binding | Schema 3 exact wire, full actor coverage including wounds, region/context/resource validation, no-replay unpublished restore, legacy byte/regression checks pass. |
| 32D - Production Flow and event boundary | `main`, `Application`, `XeenGameplay`, `XeenEncounterFlow`/`XeenJourneyFlow`, `XeenEventFlow`/publication, existing SDL/presenter/compositor | Fresh/resume CLI, actor-aware controls/notices, truthful manual refusals, original automatic sign and exclusive handoff; stop cannot save or resume; no parallel owner/scheduler. |
| 32E - Connected validation and closure | Focused synthetic/original-resource/gameplay/process tests and durable documentation | Build/full CTest, all layers below, independent review and maintainer physical acceptance pass before recording stable completion. |

## Acceptance requirements

### Automated rules, authority and persistence

- Derive the component on synthetic maps with changed passages, disconnected
  islands, an impassable anchor and loops. Check the original 121-cell result
  separately. Cover every local cell/facing/forward/backward edge, mountain,
  water/deep-water/space, middle-3 surface bypass, borders/corners and neighbor
  sampling. Synthetic alternative geometry must change the derived answer, not
  select another fixture whitelist. Production manifest mismatches reject.
- Exercise meaningful alternative paths and cycles on the mainland, including
  upper/eastern choices and the `(5,6)` connection toward the south. Enumerate
  all mainland automatic/manual event addresses and all directions; prove sign
  dispatch and wrong-facing/no-event behavior, manual refusal, and no transition
  publication. Unknown automatic chains cannot slip through navigation, blocked
  inputs, turns, initial entry, direct dispatch, resume or overlays.
- Keep all 19 identities/metadata and normal appearances independent of support
  for combat. Cover mountain-spawn exits, legitimate terrain immobility,
  disconnected actors, original-order occupancy of three, fallback/no fallback
  on occupancy, activated actors leaving view/scan and inactive occupancy.
- Ranged tests cover an active Orc aligned before movement with clear/blocked
  east and other-axis rays, attack from behind, nonalignment, unactivated actors,
  scan limit, moved-once/two-pass ordering and candidate changes before later
  actors. No range draw/damage is performed. Contact tests cover each profile
  and multiple same-cell actors; all stop without regional combat/End/accounting.
- Check action/pulse publication order and complete pre/post state on every
  support boundary, including old pending work, two Wait opportunities and
  failure after a prior committed actor move. Block all subsequent mutation and
  saving. Test allocation/provider/report/presentation faults, stale callbacks,
  reentrant replacement, retained-preimage mutation-and-reversion, lifetime ABA,
  wrong owners, copied results, newer leases and same-batch/repeated SDL input.
- Reconstruct map/MOB/EVT/sprite/scene caches in Quiet, pending approach, sign
  presentation, stopped and restored states. Compare full live state, context,
  RNG and overlays; only derived visuals/caches may differ. Fail changed warm or
  reloaded topology/statistics even when removed events are effective None.
- Test calendar candidates at 23->0 ctr24, 479->480, 959->960, 1259->1260,
  1439->0, day 99->0/year+1, pre-dawn 299->300, multi-boundary charges and year
  overflow. Live tests distinguish permitted in-interval ten/one-minute
  advancement from unsupported processing. Verify no RNG/conditions are
  fabricated at refused boundaries, no day/night frame is falsely published,
  and every region rule/display reads the same retained context.
- Codec negatives include every count/order/identity/schema/content mismatch,
  truncation/trailing bytes, length/checksum/4 MiB failures, booleans/enums,
  supplement/Luck/RNG failures, camera outside mainland/side/map, illegal actor
  coordinate/terrain/HP/status/lifecycle/accounting, missing activation, occupied
  camera, incompatible resources and unsupported context/party conditions.
  Structural decoding and resource-dependent rejection remain distinguishable.
- Round-trip all actor fields, wounds outside contact, canonical defeated
  values, inactive characters, exact item holes/metadata, overlays in independent
  combinations, noninitial context and RNG state/count. Representation fixtures
  do not stand in for gameplay-produced saves. Preserve v1/v2/v3 and v4 1/1,
  2/2 meaning, exact suffixes and connected M29-M31 regressions.

### Original-resource and separate-process runtime evidence

Keep tests using commercial archives opt-in/external. Reproduce resource
size/digest/record facts above and compare production queries to an independent
literal reference oracle. No actor-disabled walkthrough or static BFS can satisfy
the runtime layer. All original actors, normal collision and production controls
must remain active in its witnesses.

The following **investigation-derived candidate schedules** make acceptance
concrete. A read-only independent rule probe predicted these states; they are
not implemented M32 results. Implementation must run them through production
Flow/SDL scheduling and reconcile any discrepancy with the pinned rules before
closure. `L/R` mean quarter turns, `F` Forward; fully settle ordinary pending
approach after each action unless a test explicitly varies timing.

| Witness from fresh `(9,11)` West | Required observation |
| --- | --- |
| `F` | Quiet at `(8,11)` West, minute 490/ctr24 1; Orc 9 moved `(6,12)->(6,11)`, activated, HP 25. Save this genuine moved-actor state. |
| Previous witness, then Wait | Orc 9's unobstructed ranged operation stops. The Wait candidate does not publish minute 500, ctr24 2 or another actor move; the committed 490/1 state survives. Stop remains unsaveable through input and reconstruction. |
| `R R F L F R F R F F R F R F` | Longer eastern loop reaches `(10,11)` North, minute 550/ctr24 14. All 19 original actors remain owned; the independent probe predicts Snake 13 moves `(13,8)->(12,8)` on disconnected land, with Orcs 9/10/11 activated. Require production confirmation of complete state, not only those selected fields. |
| `L F F R F` | Central-route frontier: final Forward candidate publishes `(8,9)` West, minute 510/ctr24 5; Orc 9 is at `(8,12)`. The ranged `SupportStopped` boundary occurs on the third subsequent pulse, and pending work is retired unsaveably. Keep the existing action/pulse scheduler and do not rollback the published Forward state. |
| `L F F F R F F F F R F` | Reach mainland sign: published path reaches `(5,9)` North at minute 560/ctr24 11 with normal live actors. Require production automatic-sign execution and maintainer physical acceptance of the actual sign handoff. |

Also exercise an alternative continuation toward the mainland's central/southern
connection until its first real actor
support boundary, recording the responsible identity/action. If the threatened path differs
from the produced `L F F F R F F F F R F` witness, reconcile production discrepancy
against pinned rules rather than weakening acceptance. Synthetic sign coverage remains
mandatory; never disable or relocate actors and call the result regional runtime
acceptance.
Failure of the two quiet witnesses to be achievable under the approved rules
requires correcting the implementation/oracle or reporting the acceptance
conflict, not silently weakening runtime acceptance to a geometry probe.

For the two quiet witnesses, use the real F9 save path and distinct producer/
consumer processes. Compare decoded **complete** state and canonical serialized
bytes immediately before save, after startup presentation and after an explicit
new capture. Restore must have zero calls for initial party/context preparation,
activation, movement, events, combat, HP/XP settlement, time and RNG advancement.
Compare uninterrupted versus resumed continuations, including the same ranged
stop and a different legal navigation choice plus inventory/equipment mutation
and another save/restart. Mutations must touch the restored owners and persist;
a static restored frame or same-process codec test is insufficient.

### Maintainer physical acceptance and closure

The maintainer must use the native SDL application with the external unmodified
installation: enter `--journey-region`, inspect all actors/context, navigate both
quiet witnesses with normal controls, observe original actors/terrain, save/exit,
restart via `--load-game`, continue navigation and item mutation, then observe a
readable ranged/contact support stop and blocked F9/subsequent gameplay. Check
manual Myra/transition refusal, route/terrain boundaries, actor motion without
render-dependent activation loss, readable notices, and ordinary animation.
Exercise the sign physically when reachable under the runtime boundary above.
Keep the existing Skeleton Journey and Bone Whistle expedition/collection usable.

Automated input, screenshots/native-frame inspection and independent review are
distinct from maintainer-performed physical acceptance. Build and run relevant
tests during implementation, then the full CTest suite at M32 closure. Obtain
required independent review and maintainer acceptance before updating stable
status/history/roadmap and condensing this plan. No current physical/runtime
acceptance is claimed by this planning document.

## Successor handoff and exclusions

- **M33:** consume the complete region, actor identities/current HP, world RNG,
  checked time seam and support predicates to admit Orc/Snake/Toad combat,
  ranged attacks/Shoot, Poison/Sleep, gold/loot and wounded living enemies.
  Generalize existing combat's remaining map-20 assumptions, including its
  Round movement predicate; M32 deliberately never invokes that consumer on
  regional actors. Specify any additional durable consequence fields with it.
- **M34:** consume those same live survivors and owners for Run, partial
  participation, relocation and non-victory retirement. A support stop remains
  a failure boundary and cannot serve as an early disengagement implementation.
- **M35:** connect the accepted M21 endpoints under M31-style event authority and
  select route-justified local recovery/antidote use. Geometry membership and
  M32's read-only sign permission grant no quest/recovery script permission.

No separate M33-M35 specification or implementation is included. General magic,
Rest, shops/temples/inns/services, training/full progression, Swimming/Walk on
Water/Mountaineer, disconnected party areas, Vertigo/mines/adjacent-map/Darkside
travel, original-game startup, arbitrary scripts, mid-action/modal/combat saves,
in-session loading, autosave, original save compatibility and commercial-data
modification/distribution remain excluded.

## Planning validation record

This pass read the applicable contracts and targeted implementation/tests,
verified both Git baselines, decoded map-23 resources read-only in memory,
computed component/event/actor facts and digests, and ran an independent
rule-level movement/ranged schedule probe. It did not build MMModern, run CTest,
implement regional behavior or perform maintainer physical acceptance. The
complete plan and local references are reviewed as documentation, including
explicit inspection of this new untracked file and `git diff --check`.
The candidate is left uncommitted for review; implementation and final acceptance
remain future work.
