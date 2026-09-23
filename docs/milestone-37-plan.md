# Milestone 37 - Vertigo entry, initial traversal and return

## Status, objective and inherited authority

**Implementation contract; not implemented or accepted.** Establish a production
Regional Journey from the admitted Clouds mainland through the original Vertigo
entrance, around a small indoor route, through the original exit, and back again.
Both regions must support quiet save, complete process exit, fresh restore and
continued play. This is one Journey with retained regional state, not another
startup mode or a camera-placement demonstration.

Planning baseline: `main`, with HEAD, `refs/remotes/origin/main` and direct
`git ls-remote origin refs/heads/main` all equal to
`f8ba96efda7abf7f0fc0bc0a19d7525e526f3272` (Complete Milestone 36 learned
exploration casting). Origin is `https://github.com/carbiecarbie/MMModern.git`;
the initial index and working tree, including untracked files, were clean.
M36 remains completed and accepted.

Use code/tests at that baseline as implementation truth, [status](project-status.md)
as accepted-state context and the [roadmap](roadmap.md) as purpose. Inherit
[M23](milestone-23-plan.md) indoor composition,
[M28](milestone-28-plan.md), [M29](milestone-29-plan.md) and
[M31](milestone-31-plan.md) ownership, publication and fresh authority;
[M32](milestone-32-plan.md) mainland admission, time and scheduling;
[M33](milestone-33-plan.md), [M34](milestone-34-plan.md),
[M35](milestone-35-plan.md) and [M36](milestone-36-plan.md) accumulated
conditions, combat, accounting, equipment, item use and learned casting.
Their formulas and legacy domains remain authoritative except for the explicit
M37 extensions below. This plan does not reopen their acceptance.

The maintainer resolved the reset policy: **no implicit reset on transition,
admission, cache reconstruction, revisit or restore; an explicit original Event
reset is authoritative gameplay.** Pre-reset actors survive indoor saves. A
committed reset becomes the saved state and is not replayed on restore or entry.
A later, separately confirmed exit may legitimately execute the reset again.

## Evidence and provenance

Original installation: `F:\Games\gog\Might and Magic 4-5`, read-only. Resources
below were inspected in memory through the existing inspection executable and
a small external read-only archive probe; no commercial files were extracted
into the repository. DAT/MOB/EVT come from the Clouds initial archive assembled
by the existing bridge from XEEN.CC's initial-save chunks, not a user's XEEN.CUR.
Text and appearance resources come from XEEN.CC; shared monster statistics
come from DARK.CC as in the accepted World-of-Xeen Clouds profile.

The [documented dependency](dependencies.md) was discovered through
`build/CMakeCache.txt`, at `D:/Projetos/MModern/scummvm-known-good-candidate`.
Its detached HEAD was verified as
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with a clean checkout. Relevant
pinned reference sources are `engines/mm/xeen/scripts.cpp` (`cmdTeleport`,
`cmdCallEvent`, `cmdReturn`, `cmdSpawn`, `cmdAlterEvent`, `cmdSetVar`,
`copyProtectionCheck`), `map.cpp`/`map.h` (logical areas, MOB loading and
`MazeMonster` defaults), `interface.cpp::chargeStep`,
`interface_scene.cpp::setIndoorsMonsters`, `combat.cpp` (`moveMonsters`,
`canMonsterMove`, `doMonsterTurn`, `doCharDamage`) and
`character.cpp::charSavingThrow`/serialization. Reference algorithms are not
a claim of independently observed DOS execution.

The original EVT contains the reset instructions described below. The pinned
reference interprets them through ordinary `cmdSpawn`; there is no Vertigo
reset patch in `patcher.cpp`. Thus the reset's existence, targets and flag-9
predicate are original-resource facts, not a ScummVM workaround. Two bounded
reference-derived policies are explicit: copy protection succeeds without a
manual-code dialog, matching the reference's default-disabled check; safe
handling of script-created slots does not reproduce unsafe null-data objects.
Neither changes the script's reset predicate or targets.

| Resource | Bytes | CRC32 | Required fact |
| --- | ---: | --- | --- |
| `maze0023.dat` | 892 | `8f3e28ee` | Accepted mainland geometry |
| `maze0023.mob` | 220 | `ce08a8c8` | All 19 mainland actors |
| `maze0023.evt` | 1440 | `e3128711` | Entrance records 136..139 |
| `aaze0023.txt` | 2368 | `58a94903` | 44 text entries, entrance text 33 |
| `maze0028.dat` | 892 | `1399bb82` | Vertigo southwest/root tile |
| `mazex109.dat` | 892 | `dec2f1e2` | Southeast geometry tile |
| `mazex110.dat` | 892 | `7bd43d60` | Northwest geometry tile |
| `mazex111.dat` | 892 | `b5351545` | Northeast geometry tile |
| `maze0028.mob` | 820 | `d2612605` | 143 objects, 46 monster records, one disabled wall item |
| `maze0028.evt` | 7298 | `28b6c20b` | 847 original records |
| `aaze0028.txt` | 3014 | `8dc60e26` | 64 text entries |

CRC32 is over decoded resource bytes. Retain the inherited archive fingerprints
and resource preimages as well as these content checks. DARK.CC/xeen.mon has
60-byte records; checked records 0 (Slime), 2 (Doom Bug) and 73 (Breeder Slime)
have CRC32 `4743814e`, `f9c6fa54` and `d14e5e01`, respectively. Only Slime gains
new active combat admission. Other profiles establish retained identity and
the influence proof, not permission to simulate all city combat.

Appearance admission uses the existing checked adapters and archive identity,
validates every referenced frame/anchor/scale before publication, and retains
immutable bytes or equivalent checked preimages. Required town files include
`town.sky` (9432 bytes, 2 frames), `town.gnd` (12481, 1), `ftown1.fwl`
(29527, 8), `ftown2.fwl` (44046, 11), `ftown3.fwl` (45232, 34),
`ftown4.fwl` (2606, 17), `stown.swl` (35930, 48), and the floor/metadata
dependencies selected by the existing indoor builder. `clouds.dat` supplies
appearance metadata (1452 bytes). Slime uses `000.mon` (17325, 8) and
`000.att` (12390, 4). Route-visible object resources include `001.obj`
(7709, 2), `004.obj` (3159, 1), `009.obj` (12450, 8), `010.obj` (4340, 1).
Do not load an unresolved disabled wall item's appearance merely because its
record exists. Names/text remain external and resource-driven.

## Exact production route and admission boundary

All identities here are Clouds (wire side 0). Directions are North 0, East 1,
South 2, West 3; Event All is 4. Record indexes and byte offsets are zero-based.

Mainland remains logical region/map 23, with the complete accepted M32-M36
domain. Vertigo is **logical region/map 28**, with a 32-by-32 logical coordinate
space and four 16-by-16 geometry tiles:

| Tile | Logical origin | North / East / South / West neighbor |
| --- | --- | --- |
| 28 | `(0,0)` | 110 / 109 / 0 / 0 |
| 109 | `(16,0)` | 111 / 0 / 0 / 28 |
| 110 | `(0,16)` | 0 / 111 / 28 / 0 |
| 111 | `(16,16)` | 0 / 0 / 109 / 110 |

Camera, Events, objects and actors retain root 28 and logical coordinates.
For example camera `(28,16,2)` samples tile 109 local `(0,2)`; it is not a
transition into actor/Event map 109. DAT names for these auxiliary tiles use
`mazexNNN.dat`, not `maze0109.dat`. Verify reciprocal topology and internal
tile identities. Sample the full logical geometry for sight/actor influence;
that does not admit player travel over the entire city.

The exact M37 player-cell set is:

```
{ (15,y) : 0 <= y <= 4 } union
{ (16,y) : 1 <= y <= 4 } union { (14,4), (13,4) }
```

All four facings, Forward/Backward, turns and Wait are supported there, subject
to original collision. Steps out of that set visibly refuse before publishing
camera, time, ctr24 or new actor work, even if the original edge is passable.
This is an explicit production support boundary, not an invisible wall in
the original data. Existing owed work is handled under the inherited rules.
At `(15,0)` walls NESW are `[0,8,7,8]`; `(13,4)` has `[8,0,8,4]`.
Reuse indoor party collision (current-cell wall must be below wallNoPass 7;
destination surface 4 blocks), including correct logical sampling at x=16.
Do not substitute outdoor collision or assume reciprocal wall bits.

The meaningful route is entry at `(15,0)`, advance up the entrance street,
resolve its actual Slime, traverse the loop through `(16,1)..(16,4)` and
`(15,4)`, visit the west-facing door label at `(13,4)`, retrace to `(15,0)`,
face South, confirm exit and return to `(23,10,12,South)`. Re-enter through
`(23,10,13)`. The loop exercises a real geometry seam, facing, occlusion,
objects, mandatory actor influence and a complete original Event landmark.
The landmark is only the outside door label; crossing to `(12,4)` and the
blacksmith service are excluded.

### Entry and selected landmark

At physical `(23,10,13)`, any facing, manual Space dispatches:

| Record / offset | Line | Opcode / operands | Meaning |
| --- | ---: | --- | --- |
| 136 / 1141 | 0 | `01 [33]` | Entrance question text |
| 137 / 1148 | 1 | `09 [44,0,3]` | Yes branches to line 3 |
| 138 / 1157 | 2 | `12 []` | No exits |
| 139 / 1163 | 3 | `07 [28,15,0]` | TeleportAndExit |

The destination is root 28 logical `(15,0)`, retaining current facing. This
cell is not an automatic Event trigger. Opcode 07 terminates this chain; it
does not run the destination script as opcode 1f would. A real north-facing
approach therefore arrives North. Do not replace the chain with camera assignment.

At `(28,13,4,West)`, record 539, byte 4471, line 0 is `02 [33]`, small door
text for the Ironworks. Its natural termination is the complete selected
chain, not a truncated service script. The cell's automatic-event bit is set;
use the inherited navigation-dispatch rules and support manual Space as well.
No other reachable physical Event is silently partially interpreted.

### Complete exit, prelude and flag 9

At physical `(28,15,0,South)`, manual Space executes the following complete
chain. Logical Call addresses do not move the camera or change the physical
origin used by AlterEvent. There is no automatic dispatch at this cell.

| Record / offset | Line | Opcode / operands | Meaning |
| --- | ---: | --- | --- |
| 760 / 6468 | 0 | `19 [75,76,0]` | Call flag prelude |
| 761 / 6477 | 1 | `01 [57]` | Exit question text |
| 762 / 6484 | 2 | `09 [44,0,4]` | Yes branches to 4 |
| 763 / 6493 | 3 | `12 []` | No exits after prelude |
| 764 / 6499 | 4 | `2f []` | Copy protection, successful policy below |
| 765 / 6505 | 5 | `18 [4,0]` | Disable physical line 4, record 764 |
| 766 / 6513 | 6 | `09 [20,9,8]` | If game flag 9 set, skip reset |
| 767 / 6522 | 7 | `19 [100,100,0]` | Call reset procedure |
| 768 / 6531 | 8 | `1b [84,2]` | Set direction South; All-facing record |
| 769 / 6539 | 9 | `07 [23,10,12]` | TeleportAndExit; All-facing record |

Prelude `(75,76)` is records 816..846, offsets 6998..7292, all directions:
line 0 `09 [20,231,2]` returns via line 2 when flag 231 is set; line 1
`08 [9,0,3]` tests the selected character's current SP >= 0; line 2 is Return.
Line 3 sets game flag 231; lines 4..27 clear flags 232..255; lines 28 and 29
clear flags 57 and 58; line 30 returns. Set/clear uses opcode 0c, action 20:
give `(0,0,20,flag)` or take `(20,flag,0,0)`. Initial script character selection
is the first active party member. Preserve the original predicate rather than
assuming this procedure is unconditionally a no-op. On the admitted nonnegative
SP domain it takes the initialization branch when 231 is clear. It executes
**before** the question, including on No; these committed flag effects must
survive a later cancellation or destination-resource failure.

The initial original game flag 9 is clear (`maze.pty` game-flags base offset
659, byte `659+9/8`, bit `9%8`). Its city setter is record 80, byte 698,
physical/logical `(9,22,All)`, line 8, `0c [0,0,20,9]`. The full surrounding
discovery chain is records 72..82: SP test, refusal/acknowledgment path,
WhoWill, move object 0, discovery text 16, acknowledgment, set flag 9,
disable lines 4 and 5. The text identifies invoice evidence concerning Joe,
monsters and the mayor. Flag 9 is **discovery progress**, not an inferred
all-monsters-defeated counter. Other city tests occur in records 7, 44, 68,
83 and 766. A targeted scan of present initial Clouds EVT resources for maps
1..99 found no other action-20 flag-9 test/set/clear. Pinned
`Party::giveTake` action 20 sets/clears the current side's game-flag bank;
the conditional uses `Scripts::ifProc` action 20, so this is Clouds game flag
9, not quest flag 9 or a per-map flag. M37 admits neither the discovery cell nor its service/quest path;
it never manufactures flag 9 or grants that discovery. A true-flag fixture is
an artificial predicate control, not a claimed production route.

Protection policy: implement opcode 2f as an explicit successful protection
capability for this supported original profile, matching pinned ScummVM's
default `copy_protection=false`; no new DRM/manual-code UI. Do not delete the
instruction. Opcode 18 must still disable record 764 durably, by original
identity, and re-entry must observe its effective None opcode.
Its reusable matching rule is physical cell plus requested line, with either
the physical current direction or All; it does not target a Call's logical
coordinates. Admit only replacement opcode 0 here. SetVar admission is limited
to mode 84 with direction 0..3; this chain uses 2. Other modes remain unsupported.

### Exact scripted actor reset

The reset procedure is `(100,100,All)`: records 770..812, offsets
`6548 + 10*line`, lines 0..42, opcode 10 with four parameter bytes;
record 813/6978, line 43, is Return. The tuples below are
`slot:x,y,unused-fourth-byte`, in script order:

```
0:1,11,0    1:1,11,0    2:2,9,0     3:3,10,0    4:3,11,0
5:3,11,0    6:3,13,0    7:3,13,0    8:3,27,0    9:4,27,0
10:4,26,0   11:4,25,0   12:4,12,0   13:4,7,0    14:4,7,0
15:4,3,0    16:4,3,0    17:4,3,0    18:5,12,0   19:9,18,0
20:25,14,0  21:28,9,0   22:30,9,0   23:30,6,0   24:29,15,0
25:8,24,0   26:8,24,0   27:7,23,0   28:7,23,0   29:8,27,0
30:8,27,0   31:9,18,0   32:6,2,1    33:7,1,1    34:6,6,1
35:7,7,1    36:15,4,0   37:22,9,0   38:21,1,0   39:22,1,0
40:30,1,0   50:7,24,0   51:6,27,0
```

Pinned `cmdSpawn` consumes slot u8, x signed byte, y signed byte (its
`readShort()` is int8); it does **not** consume the fourth byte or interpret it
as a species. It uses the slot's existing sprite/statistics identity, restores
base HP, position, Physical status and nonattacking state, and selects a
cosmetic frame 0..7. Admit the observed four-byte encoding explicitly, retaining
the unused operand as provenance; reject malformed lengths, out-of-policy slots
and coordinates. The operation is a reusable decoded Spawn/reset primitive,
not a hand-coded Vertigo teleport side effect.

Original MOB has only slots 0..45. The reference resizes for 50/51; new slots
default to sprite 0 (Slime). Model 50 and 51 as script-created Slime slots,
with the same reset semantics and their original numeric identities. Slots
46..49 are unmaterialized gaps: canonical `(0,0)`, HP 0, inactive,
Unresolved, Physical, unaccounted, no statistics/occupancy/render/attack
authority. They are never fabricated original MOB records. Runtime count is
46 before the first reset, 52 afterwards. Preserve originals 41..45 unchanged.
This finite safe representation replaces the reference's null monster-data
gaps, not the explicit Spawn instructions. It does not admit arbitrary spawning.

Resetting a materialized slot starts its new life: Present, full original HP,
specified position, inactive, Physical and unaccounted. Retire prior encounter,
attack and accounting capabilities for those slots. Existing XP, purses and
party consequences are not reversed. Defeating a legitimately reset Slime can
earn its normal XP again; restore/re-entry alone cannot do so. The selected
reset's cosmetic random frames consume **no Journey gameplay RNG**, following
the existing separation of presentation randomness from gameplay. No frame seed
is durable. This is an explicit cosmetic adaptation to the accepted RNG model.

## Existing reuse and necessary extensions

The implementation is not a new indoor engine. Baseline
`XeenMovement::applyIndoor`, `XeenIndoorScene::sampleWalls/build`,
`CloudsMapComposer`, `XeenObjectVisualResolver`, Xeen map/object/resource
adapters and M23's ordered wall/object stream already provide indoor geometry,
collision, perspective, wall predicates and object occlusion. Ordinary rendering
of a map is not production Journey admission. Existing multi-map caches do not
retain mutable gameplay state and must not acquire that responsibility.

| Category | M37 responsibility |
| --- | --- |
| Reuse | DAT/MOB/EVT parsing, original text, inventory/modal UI, combat/Flow owners, guarded providers, save envelopes, concrete-frame SDL authority |
| Generalize | Logical indoor tile sampling; regional actor collections within the existing session; active-region scheduling/capture; indoor actor composition; transition publication; schema-selected content capabilities |
| New reusable primitives | Spawn/reset, AlterEvent-to-None, SetVar direction 84, successful protection check, poison-type Slime damage/save input |
| Route data | Root 28 topology, eleven cells, original entrance/door/exit chains, full actor identity catalog and reset targets |
| Unsupported | All other city player cells/services/Events, general monster populations/combat, indoor Shoot and Run, arbitrary teleports/worlds |

Audit every `contract == 7`, actor-count 19, camera-map 23 and local-coordinate
assumption. Do not broaden `>=` checks across legacy domains without explicit
schema/content dispatch. Existing general Call/Return/Teleport interpretation
does not authorize Journey publication: extend `XeenEventSystem`,
`XeenEventPublication`, `XeenEventInterpreter` and the coordinator together.
The baseline Journey guard intentionally forbids camera/flag changes and call
stacks outside its admitted chains. Replace that restriction only with checked
capabilities for the exact transitive instruction graphs above.

### Objects, actors and influence closure

Retain all 143 original object identities and overlays; only original visibility
and M23 wall predicates choose draw commands. Relevant visible records include
27, 28, 41, 42 (resource 10), 53 (resource 9, at `(14,5)`), 74 and 78
(resource 1), and 116, 117, 119, 120 (resource 4). Objects outside the player
cell set must not disappear merely because the party cannot visit them.
Resource 9 uses directional frame ranges `[0..3,4..7,4..7,4..7]` with
directional flips `[0,0,1,0]`; extend indoor composition to use the existing
100-ms cosmetic phase/resolver. This is appearance, not NPC behavior or service
authority. Static resources keep their original directional frames/flips.

Own the complete original 46-slot actor collection on first admission. Original
record 35 is the entrance Slime at `(15,4)`; record 36 initially lies at
`(22,9)`. After the scripted reset, 35 moves to `(7,7)` and **36** occupies
`(15,4)`. Do not renumber either or respawn record 35 at the entrance on reload.

The selected route requires one live influencing Slime per visit. It cannot
honestly exclude actors. A geometry-only reachability probe finds a large
connected entrance component, including more distant actors; wall connectivity
alone is not an influence proof. The actual closure also uses the bounded
view activation predicates, retained activation state and the +/-3 scheduling
grid. Starting from original states (or the exact reset states), all player
cells/facings in this contract can activate the entrance Slime; other original
actors remain outside activation influence. Distant Doom Bugs/Breeder Slimes
must remain retained dormant state, not be silently removed or granted combat.

Make this claim an executable admission/review gate: enumerate all eleven
cells/facings, their sampled wall queries, original and reset actor positions,
and the movement/activation closure under arbitrary supported turns, waits,
route movement and revisits. Include off-route actor paths and all 46/52 slots.
Party support boundaries do not constrain monster movement. Use the reference's
edge-aware indoor `canMonsterMove` predicate (current-cell wall <= wallNoPass),
not party collision, outdoor destination terrain, or a clamp to the eleven
cells. Preserve record order, occupancy and the accepted two-pass approach
scheduling. An unexpected influencing unsupported actor is a support stop,
never an omitted actor; failure of this closure gate requires specification
review before expanding the route or combat scope.

Indoor view activation and placement must share one checked classification.
Port the pinned `setIndoorsMonsters` wall predicates/query order into the
existing actor approach and indoor draw stream. In particular same-cell,
one-forward and two-forward central activation precedes their visual wall
tests; three-forward central requires `!W27 && !W22 && !W15`. Do not activate
only drawn pixels or reuse the outdoor slot table. Reuse M23 wall sample indexes
and the reference's remaining diagonal predicates exactly. Merge MON/ATT commands
at the original indoor draw-list orders (central same-cell 156/150/153,
one-forward 132/130/131, two-forward 106/104/105, three-forward 70/68/69),
with the reference's diagonal placements, perspective and bottom clipping.
The independent review must compare the complete finite placement/predicate
table with the pinned function, not just a screenshot of one facing.

Support Slime animation effect 1 through a bounded platform-neutral draw option
mapped by the sprite bridge to the reference's effect-1 palette sequence
(`MONSTER_EFFECT_FLAGS`, row 0, eight phases). The other fourteen effects remain
unsupported. Keep options range-checked and native clipping safe; no direct
ScummVM flags leak into gameplay. MON/ATT/cosmetic phases and object animation
must not advance gameplay RNG, time, actor movement or saves.

### Minimum Slime combat extension

Original Slime profile: image 0, HP 2, AC 0, Speed 25, XP 50, two attacks,
one strike of 1d2 per target, hates-party 16, attack type Poison 5,
special attack 0, no ranged attack, flight, physical resistance, gold, gems
or item drop. Validate the complete record and appearance; no invented weaker
entrance monster. Retain the accepted player Attack/Block and injury/accounting
rules. Hates-party visits **every combat-party member in order, including
disabled/dead members**, for each attack, as both `Combat::doMonsterTurn` overloads
specify. Do not use ordinary random-target selection or filter to canAct.

Poison attack type is HP damage, not the Poison condition. It bypasses the
physical d20/hit-chance/AC branch and has no special-attack saving throw.
For each target: wake Asleep; draw 1d2; perform one poison saving throw and
halve with integer division on success; subtract party poison resistance;
while damage > 0 perform further poison saves, halving until failure or zero;
then apply `max(damage-powerShield,0)` and inherited injury/armor consequences.
Here the inherited admitted party resistance/power-shield values remain zero.
A save draws uniformly `1..(v+40)` and succeeds at <= v, where v is permanent
plus temporary poison resistance plus `itemScan(14)`. No random target or
physical attack roll is inserted. Use the existing bounded candidate/RNG
continuation mechanism across the complete two-attack sequence, with no replay
of already committed draws or effects at presentation boundaries.

Add explicitly present poison resistance inputs for all 30 roster owners,
including inactive owners. Original CHR record offsets 317 and 318 are the
permanent/temporary raw bytes. Under the inherited bounded equipment domain,
the elemental poison contribution is zero; validate that domain and use the
reusable equipment query rather than widening material admission. An absent
legacy input is not zero and must not be silently supplied. The accepted
character/condition rules determine canAct, injury and whole-party failure.
Do not turn the new damage type into general elemental combat or new magic.

## Durable ownership and transition publication

Extend `XeenSessionWorldState` inside the existing `XeenWorld` to retain regional
actor collections keyed by logical `XeenMapIdentity`. Each collection owns its
ordered mutable actors and per-identity accounted state; original/statistics
descriptors remain immutable admission data. Mainland is always admitted;
Vertigo is absent until first admission, then retained for the lifetime of this
Journey. Disabled object/Event overlays remain globally keyed by
`(side,root-map,original-record)`. Application's authoritative camera chooses
the active region. Do not duplicate an independently mutable active-map value.

There remains one party/roster, inventory/equipment, learned-book set, context,
Journey RNG, purse/pending treasure, quest/game/world flag graph, Journey Flow
and session. Camera still has one authoritative owner. First admission creates
only the previously absent regional actor collection from checked original
resources. Revisit retrieves its saved mutable state; fresh restore installs
all saved collections into unpublished owners. Neither may call fresh Journey
initialization, reset accounting, seed RNG or reapply original CHR preparation.

All full-owner guards/equality checks must include inactive regions, region
membership, slot shape, accounting, poison inputs and overlays. Cache eviction
does not evict those owners or their admission preimages. Actor handles are
qualified by root identity plus slot and current authority, never vector index
alone. Reset/transition invalidates transient handles even when coordinates,
HP or pointers happen to return to earlier values.

### Lifecycle and commit boundaries

1. **Acquire.** Only current presented exploration authority can start Space.
   Finish already owed source gameplay/treasure work before Event admission;
   no departure during combat, casting, projectiles, inventory mutation or a
   modal response. Acquire exclusive Event/transition work under the existing
   Flow before invoking any provider. Revoke old navigation/frame authority.
2. **Preflight and prelude.** Check the full source chain, transitive calls,
   text and allowed effects, not just its first instruction. Execute the exit
   prelude on a candidate and publish its flag delta atomically at Return,
   before showing the question. Check complete live preimages immediately
   before that commit. The publication does not release the Event lease.
   Entrance has no such delta. No restores or caches replay this prelude.
3. **Question.** Present original Yes/No through a concrete current modal frame.
   No ends the Event, retaining committed prelude changes and awaiting its
   final frame before Quiet. No does not admit Vertigo or reset actors.
4. **Prepare confirmed departure.** Interpret the remaining original chain
   against a detached candidate. Resolve destination topology/resources,
   first-admission or retained owners, all 43 Spawn effects when required,
   physical AlterEvent overlay, facing and destination. Validate the entire
   owner graph, exact terminal opcode and resource preimages; prepare arrival
   classification, any mandatory settlement, and composable destination frame.
   All allocation/provider calls and fallible preparation precede commit.
   Keep the question/transition lease live during preparation and failure UI.
5. **Commit once.** Revalidate source camera, region collections, all party and
   global owners, RNG/time, session/Flow generations, source Event position and
   retained immutable preimages. Publish the candidate owner changes plus
   camera/region selection and fresh authority epoch as one no-throw operation.
   Use prepared storage/swaps; no provider, presentation or other callback
   between owner writes. The whole post-Yes reset/overlay/teleport block has
   one publication boundary: there is no player-observable intermediate step
   in the original block. Do not publish 43 partially fallible live resets.
6. **Arrival and handoff.** Keep transition work held across any required
   arrival classification, ready treasure delivery, encounter attachment and
   destination presentation. Opcode 07 schedules no destination Event or extra
   movement/time charge. Classification may activate actors but does not move
   them or draw RNG; ready source treasure may not be stranded by the boundary.
   Transfer directly into encounter/other mandatory Flow work if needed.
   Release to exploration only after the current destination frame is actually
   presented and every mandatory continuation is complete. There is no saveable
   Quiet gap between camera publication and arrival work.

Every callback boundary, including a nominally read-only provider and composer,
requires a retained full preimage and generation check on return. Reentrant
input, capture, restore, Event completion or transition attempts cannot borrow
the outer operation's authority. Do not refresh a guard from arbitrary callback
state and thereby bless a mutation. Mutation-and-reversion is rejected by
epochs/retained authority in addition to value equality.

Before the main commit, a plain I/O/allocation/presentation-preparation failure
leaves camera, visited membership, all actors, overlays, party, RNG and time
unchanged from the last legitimate commit (which may include the prelude).
Discard its candidate; a current error frame may end the operation and restore
Quiet. Previously committed prelude flags are not rolled back. After the main
commit, presentation failure cannot roll back gameplay or retry the script:
retain a pending arrival/presentation lease, report failure, retry only the
unpublished presentation/owed continuation, and prohibit saving until recovery.
If recovery cannot complete, retain a truthful non-saveable stop. Never recreate
the candidate from old source values after commitment.

## Time, scheduling and accumulated consequences

Apply inherited time/condition boundary checks to the one shared context.
Inactive-region actors do not receive movement/activation/attack pulses: the
reference schedules the loaded region's MOB. They receive no elapsed-time
catch-up on return. This is not a global time freeze: party time/conditions and
inherited calendar/support-stop rules continue in the active region.

| Action/work | M37 indoor behavior |
| --- | --- |
| Successful Forward/Backward, Wait | 1 minute rather than outdoor 10; inherited ctr24 and opportunity semantics |
| Turn | 0 minutes, inherited ctr24 and already owed work |
| Blocked or support-boundary step | No new time/RNG/opportunity; preserve owed-work semantics |
| Event, transition, first admission, revisit | No intrinsic time or gameplay RNG charge |
| Arrival classification | No synthetic movement, time or RNG; complete before exploration handoff |
| Attack/Block/round settlement | Existing combat rules; existing round minute charge, no outdoor exploration charge |
| First Aid/Awaken exploration casting | M36 SP/effects/cancellation/settlement, with indoor 1-minute charge; no added ctr24 increment |
| M35 supported item use | Existing zero-minute effect and owed actor opportunity in the active region |
| Inventory/transfer/equip | Existing authority and zero-time behavior |
| Rendering, object/monster cosmetic animation | No gameplay state, time or RNG changes |

Preserve all unrelated values exactly: roster membership, inactive owners,
HP/SP/conditions, supplements and experience; all inventories/equipment and
learned books; gold/gems, pending source-mask/items and delivered accounting;
quest items/flags, unrelated game flags, world flag 16 and disabled overlays;
all mainland actor positions, wounds, activation, defeat/status/accounting;
Journey RNG algorithm/state/count and gameplay calendar. Only the explicit
prelude, protection overlay, reset and facing/camera mutations above are
transition-script changes. Combat, casting, item use and time subsequently
produce only their inherited or explicitly extended consequences.

Pending mainland treasure stays source-qualified mainland accounting. Where
inherited rules defer delivery behind a living obstruction, transition does
not clear it, reaward XP or recreate the drop. Evaluate mandatory ready
settlement under the same rule with the destination active; preserve still
pending contents. Slime contributes 50 XP and no purse/item reward. Reset only
rearms the named city slots' per-life accounting, not the mainland pending mask.
That mask and its item source bytes still refer to the twelve mainland Orc
sources; numeric city slots must never be substituted when validating or
delivering them. The inherited selected-live-threat obstruction predicate uses
the active region's checked view, while reward provenance uses its source region.

Shoot and Run indoors visibly refuse before RNG/time/camera/actor mutation;
their outdoor M34/M35 behavior remains supported. Vertigo's original run point
`(18,4)` is outside this route, so the mainland escape destination must never
be reused. Rest, services, unsupported spells, combat casting, broad NPC
interaction and arbitrary Event opcodes remain visible refusals. An unsupported
action differs from encountering unsupported mandatory actor work: the latter
uses the inherited non-saveable support stop, not an ignorable refusal.

## Persistence and compatibility

### Version decisions and exact wire

Keep the **v4 envelope**: its checksum/length/resource identity/base state and
optional Journey suffix already provide the required framing. Use Journey
**schema 8/content 8**. Schema changes because the durable representation gains
poison inputs and a retained optional second region with script-created slots.
Content changes independently because region/route, Event and indoor actor
admission/scheduling differ from 7. A milestone number alone is no reason for
either version; all other schema/content pairings remain rejected.

Preserve the complete schema-7 suffix layout, changing its pair to 8/8. The
existing initialized-map/actor block is explicitly the mainland block even when
the camera is indoors: side 0, initialized map 23, original count 19, runtime
count 19, original ordered actor identities. Append after the 30 learned books:

| Order | Encoding |
| --- | --- |
| Poison count | u8, exactly 30 |
| Poison owners | 30 entries in owner order 0..29: owner u8, permanent u8, temporary u8 |
| Vertigo present | canonical boolean u8 |
| If present: original count | u16 LE, exactly 46 |
| If present: runtime count | u16 LE, exactly 46 or 52 |
| If present: actors | runtime-count entries in slot order, same 19-byte actor wire below |

Actor wire: side u8 (0), root map u16 LE (28 for this block), slot u32 LE,
x i16 LE, y i16 LE, HP i32 LE, activated canonical u8 boolean,
lifecycle u8 (`Present=0, Disabled=1, Unresolved=2, Defeated=3`), status u8
(`Physical=0`), accounted canonical u8 boolean. Mainland retains its inherited
status/domain rules. No independent active-region field or reset counter is
needed: the base camera is authoritative and saved actor state plus overlays
is the result, not a recipe to replay. Original-vs-script slot provenance is
derived from the checked catalog and numeric index, not a mutable wire tag.

If N is the inherited pending item count (N <= 12), the full schema-7 suffix
was `3022+5*N`. The new append is 91 poison bytes, one presence byte and,
if present, `4+19*C` bytes. Thus schema 8 is `3114+5*N` unvisited,
`3992+5*N` with 46 city slots, or `4106+5*N` with 52. Reject truncation,
trailing data, bad counts/order/IDs, noncanonical booleans/enums, arithmetic
overflow, mixed version pairs and checksum/length mismatches before publication.
Do not widen the ordinary-save camera domain to make the Journey branch work.

### Canonical state and capture

Root camera is 23 in its exact inherited mainland domain, or 28 in the eleven
cells and four facings above. City camera requires a present city block;
absent city block requires mainland camera and no city overlays. A 52-slot
block requires the protection-line overlay from a completed reset exit.
A 46-slot block may also have that overlay after a flag-9-true exit. No other
new city object/Event overlay is admitted. Existing mainland overlay admission
is unchanged. Game flag 9 is preserved, not inferred from actors or the overlay.

Every materialized city actor's immutable type/base HP comes from the checked
catalog, never the save. Enforce original state or the exact reset-derived
state for noninfluencing slots, and the admitted approach/defeat position and
activation domain for influencing slots 35/36; validate their HP and lifecycle
with inherited wound/defeat/accounting invariants. The movement-domain closure
described above is computed from checked geometry and scheduler, not an
unbounded rectangle or arbitrary save-supplied location. In the 52-slot form,
46..49 have exactly the canonical gap representation and 50/51 are the exact
script-created Slime states. A reset reestablishes the target states, not
initial MOB coordinates. Reject forged active distant monsters, materialized
gaps, missing slots, swapped identities and contradictory accounting.

Quiet capture requires no Event/transition/prelude/question/arrival work,
cast, encounter, projectile, modal mutation, owed scheduler/treasure settlement
or unpresented required frame. Validate complete owner preimages, both regions,
and current active-region view/contact/activation. Inactive actors may share
numeric coordinates with the active camera: that is not same-region contact.
Retain their exact values without activation or normalization. Quiet snapshots
are valid on the mainland before entry, inside before exit, on the mainland
after the reset commits, and inside after revisit. Wounded/defeated city actors
remain durable until an actual reset Event; if a chosen active encounter cannot
become Quiet, use deterministic quiet-state fixtures for wounded coverage and
the genuine post-defeat route for process evidence.

Do not serialize leases, callbacks, frame tokens, revisions, call stacks,
modal selections, partially prepared reset deltas, cosmetics or resource caches.
F9 never writes a partial transaction. Retain the inherited atomic file-replace
behavior; failed capture leaves the old save byte-for-byte intact.

### Fresh restore and legacy domains

Restore decodes/validates into unpublished party/world/camera/flags, prepares
both admitted collections and required immutable resources, installs saved
values, validates topology and active view without activating/moving actors,
preflights presentation, rechecks provider/full-owner preimages, and publishes
once. Bind new Flow/encounter/Event/SDL authority; do not reuse serialized or
prior-process capabilities. First input requires the new concrete frame.
Resource/presentation failure before publication leaves existing live owners
unchanged. Never call fresh regional initialization, replay exit or arrival
Events, regenerate mutable actors, infer defeated rewards, reseed, change
time/RNG, relearn books or fill saved poison inputs from original CHR.

Fresh `--journey-region` selects 8/8, with the inherited M36 prepared party and
mainland start plus explicitly loaded poison inputs. It admits no city state
until the original entrance succeeds. Existing saves stay in their own domains:

| Save domain | Required unchanged behavior |
| --- | --- |
| Ordinary v1/v2 | Exact representation, absence semantics and ordinary map admission |
| Completed Diagnostic27 v3 | Exact completed encounter contract and accounting |
| Journey v4 1/1, 2/2, 3/3 | Original seeds/RNG, supplements, actor counts, cameras and support stops |
| Journey v4 4/4, 5/5, 6/6 | Original resistance/treasure/recovery representation and behavior |
| Journey v4 7/7 | Exact learned books, mainland admission, casting and 3022+5*N suffix |

No implicit upgrade, missing-field injection, widened legacy region admission
or loss of original bytes. Loading 7/7 does not grant the M37 entrance even
though a fresh session now selects 8/8; show the inherited unsupported
interaction result. A migration utility is not M37. The old command modes keep
their existing content choices. Regression controls must compare legacy bytes
and subsequent behavior, not merely successful decode.

## Flow, native input and failure classes

Reuse `XeenJourneyFlow`, `XeenEventFlow`, `XeenGameplay` and the actual
Application/native-SDL frame-presented path. A transition adds an exclusive
work kind/continuation to those owners, not a parallel event loop. Save denial
must cover acquisition, resource callbacks, prelude publication, question,
post-Yes preparation, main commit, classification, encounter handoff and final
presentation. A temporary empty modal queue does not imply Quiet.

Frame authority includes session identity, root region, camera/owner revision,
Flow generation, operation/response generation and the concrete presented
frame's identity. Copied/equal-looking snapshots are not frame capabilities.
Old-source frames, delayed Yes responses, repeated completion, a previously
valid frame from a prior visit, callback recursion and retained actor handles
cannot publish or command a newer state. Publish-to-present handoff fences held
or batched keys; do not let the departure Space/Yes or old movement key operate
the destination. Recomposition/cosmetic redraw must not mint a response for a
different operation or release its lease. Apply the same rules after restore.

| Result | Preservation and saveability |
| --- | --- |
| Unsupported optional action/boundary | Visible refusal; no new durable change; current healthy Quiet may resume after current frame |
| Ordinary provider/I/O/allocation failure before commit | Discard candidate; preserve last committed state; recoverable error may return to Quiet after settlement/presentation |
| Stale/copy/delayed/reentrant capability | Reject with no owner/RNG/time publication; never reauthorize itself; current valid operation retains its work |
| Immutable mismatch, owner preimage violation, broken canonical topology | Latch integrity failure; no candidate publication and no new save; matching retry cannot clear the latch |
| Failure after main transition commit | Keep committed result; no script/reset replay or rollback; no save until mandatory work and presentation recover |
| Unsupported mandatory actor/time consequence | Existing truthful support stop, not Quiet and not victory |

## Implementation responsibilities and review gates

Keep changes bounded to these existing responsibilities; helpers may be added
beside them, but no second world/session/Journey owner is justified:

1. `src/games/xeen/XeenWorld.*`, `XeenJourneyContent.h`, `XeenRegionalRules.*`,
   `XeenActorApproach.*`: regional durable collections, immutable descriptor,
   tile/actor influence closure and active-region scheduling. Preserve legacy
   access semantics through explicit contract dispatch.
2. `XeenMovement.*`, map/resource adapters, `XeenIndoorScene.*`,
   `CloudsMapComposer.*`, object resolver, `XeenSpriteDrawOptions.h` and
   `ScummVmXeenBridge.*`: logical sampling, actor/animated-object stream and
   bounded Slime palette option, all through checked resources.
3. `XeenEventDecoder.*`, `XeenEventInterpreter.*`, `XeenEventSystem.*`,
   `XeenEventPublication.*`, `XeenEventFlow.*`, `XeenJourneyFlow.*`,
   `XeenGameplay.*` and Application: complete graph admission, new primitives,
   candidate/prelude/transition publication and concrete native-frame handoff.
4. Character/statistics/equipment adapters and `XeenCombatRules.*`: explicit
   poison inputs and exact Slime attack sequence using existing candidate,
   RNG, encounter and accounting owners.
5. `XeenSaveSnapshot`, `XeenSaveState.*`, `XeenSaveFormat.*`,
   `XeenRestoreGuard.h`, `XeenJourneyCapture.h`, state equality and associated
   guards: exact 8/8 wire, both regions, fresh restore and legacy isolation.

Before combining implementation, independently review (a) resource/script and
flag provenance including 46-to-52 slot semantics, (b) influence closure and
indoor predicates, (c) multi-owner publication/ABA/exception boundaries and
(d) wire/legacy admission. These are technical gates, not permission to change
the plan's scope. A concrete contradiction that invalidates closure or demands
another monster behavior must be reported with its triggering original data;
do not quietly hide the actor, truncate a script or implement the full city.

## Acceptance specification

The following are four separate evidence classes. None has been completed for
M37 by this planning task.

### 1. Automated deterministic validation

Extend the existing indoor, navigation, Journey Event/persistence, regional
consequence and concrete-frame suites. Relevant baseline entry points are
`IndoorMapIntegrationTest`, `IndoorObjectIntegrationTest`, `XeenIndoorSceneTests`,
`XeenIndoorComposerTests`, `NavigationFlowIntegrationTest`,
`XeenJourneyEventTests`, `XeenJourneyPersistenceTests`,
`XeenRegionalPersistenceTests`, the regional CLI witnesses and M36
`XeenM36CliWitness`/process controls. Required independent assertions include:

- Both directions; first visit versus revisit; No at entrance and exit; flag
  231 prelude branches; flag 9 false and true; disabled protection opcode;
  full 43-slot reset, unused fourth operand, untouched 41..45, safe gaps and
  script-created 50/51. No repeated publication from duplicate responses.
- Every route cell/facing, logical seam in both directions, collision and
  support boundary, door text dispatch, full wall/object/actor command ordering,
  animated object 53, Slime MON/ATT/palette phases, clipping and occlusion.
- All original and reset actor influence closure, original 35 versus reset 36,
  retained off-route state, inactive mainland freeze with advancing party time,
  exact Slime draws/damage including sleeping/disabled/dead targets, zero-damage
  saves, XP once per life, legitimate later reset and no item/purse reward.
- Sentinel values in every mutable owner, including inactive roster books,
  poison bytes, inventory and equipment, wounds, flag 16, pending treasure,
  unrelated game flags and inactive-region activation/accounting. Compare full
  preimages, not just camera/actor count. Include ABA mutation-and-reversion.
- F9/capture attempts in every lifecycle phase and provider/presentation
  callback; copied, delayed, held/batched, stale and reentrant frames/responses;
  destination cannot be commanded by source frame. Resource mismatch and plain
  failures at each preparation point; presentation failure after commitment
  cannot replay reset or reopen Quiet prematurely.
- All three 8/8 wire lengths; exact bytes/round trips for unvisited, visited46,
  visited52 with active mainland/city; malformed identities/counts/booleans,
  holes, HP/lifecycle/accounting, invalid camera/tile aliases, missing city,
  illegal overlays, truncated/appended data, mixed version pairs. Reject forged
  dormant activation and incorrect physical/logical Event identity.
- Exact ordinary/Diagnostic27/Journey 1..7 representations and behavior;
  absent poison/books retain absence. Fresh restore under all injected provider
  faults leaves existing owners unchanged; new capabilities are required.

Synthetic cameras, actor wounds, true flag 9, failed providers and tailored RNG
are valid controls and must be labeled artificial. They do not prove the
production route is reachable. Do not use final-candidate output as its own
oracle: assert original instruction/target tables and independent expected
owner deltas/draw traces.

### 2. Genuine original-resource and distinct-process evidence

Use the committed `src/main.cpp` positional syntax, verified during planning:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
$game = 'F:\Games\gog\Might and Magic 4-5'
$save = Join-Path $env:TEMP 'mmodern-m37.sav'
& .\build\mmodern.exe --journey-region --combat-seed 56 $game --save-file $save
# F9 at the selected quiet checkpoint, then fully exit the process.
& .\build\mmodern.exe --load-game $game $save
```

The game directory precedes the save path for `--load-game`; the seed precedes
the game directory for `--journey-region`, and `--save-file` is last. Seed 56
is a reproducibility control, not a guarantee of success independent of input
and pulse timing. The implementation witness must record the successful exact
input/settlement trace for this route and compare an uninterrupted branch with
each resumed branch under that same trace. No production shortcut or direct
camera assignment is permitted.

Start at the inherited prepared mainland `(23,9,11,West)`. Turn to East,
advance to `(10,11)`, turn North and advance to `(10,12)`, resolving any
ordinary mainland actor work/combat with accepted controls before proceeding.
Checkpoint A is Quiet `(23,10,12,North)` with retained mainland consequences.
Advance to `(10,13)`, Space, Yes, arrive `(28,15,0,North)`. Checkpoint B is
Quiet inside the route after settling the entrance Slime and reaching
`(16,2)`; preserve its defeat/accounting and mainland state. Continue via
`(16,4)` to `(13,4,West)`, observe the original door label, then return along
the route to `(15,0,South)`. Space/No demonstrates the prelude; Space/Yes
executes the complete exit and reaches `(23,10,12,South)`. Checkpoint C is
Quiet after any mandatory arrival settlement. Re-enter, observe/resolve the
reset entrance slot 36, and take checkpoint D at `(16,2)` again. Continue
traversal and return after restoring D.

For each A/B/C/D: genuine F9, full process exit, a distinct fresh `--load-game`
process, checked restored state before first command, then continued movement,
Events and return/revisit. Preserve separate save/oracle files outside the
repository and original installation. Compare exact durable state and encoded
bytes at equivalent quiet checkpoints, including time, RNG state/draw count,
roster, books, treasure, both actor collections, flags and overlays. Cosmetic
clock/frame phase is deliberately excluded. In particular B restore must not
reset; C restore/re-entry must not rerun the committed exit; a new later exit
must reset when flag 9 remains clear.

Reuse `XeenJourneyProcessTests` and M35/M36 process/oracle patterns: child
executables exercising the real CLI/Application, actual F9 event delivery,
explicit process-exit boundaries, emitted full-state/draw oracles and distinct
PIDs. Add M37-specific witnesses/CTest registration within that pattern; do
not invent a production `--vertigo` or test-camera command. Automated SDL event
injection/dummy driver is process evidence only. The registered M37 process
tests must run as part of the complete CTest command below; their exact target
names are implementation details, not existing commands claimed by this plan.

Read-only resource inspection can be reproduced now with:

```powershell
& .\build\mmodern.exe --inspect-events $game 23 10 13 all
& .\build\mmodern.exe --inspect-events $game 28 15 0 south
& .\build\mmodern.exe --inspect-events $game 28 75 76 all
& .\build\mmodern.exe --inspect-events $game 28 100 100 all
& .\build\mmodern.exe --inspect-events $game 28 13 4 west
& .\build\mmodern.exe --inspect-events $game 28 9 22 all
```

During planning, the existing entry/exit/door inspections and existing
`mmodern_indoor_map_smoke` and `mmodern_navigation_flow_smoke` executables were
used successfully as investigation evidence. They establish existing indoor
reuse, not M37 acceptance. No full build/CTest is required to validate this prose.

### 3. Independent technical review

Require an independent reviewer to inspect original route/provenance, full
scripts, slot defaults/gaps, flag predicates, all-facing influence closure,
map identity separation, Slime rules, owner/preimage completeness, commit and
failure boundaries, exact wire/legacy behavior, and genuine process traces.
Review the actual final implementation and tests against this contract, not
only plan prose or a successful route video. Resolve all blocking/major/minor
correctness findings before milestone closure; document any bounded adaptation
at its natural contract location. Specification review of this plan is a
separate gate and grants no implementation authorization.

### 4. Maintainer physical native-SDL acceptance

The maintainer personally executes the A-D route with ordinary keyboard input
in a visible native SDL window and real original resources. Observe correct
entry/facing; coherent street, seam, objects, occlusion and Slime behavior;
readable original door/exit text; honest cell/service/Shoot/Run refusals;
inherited inventory, item and learned-cast behavior indoors; denied F9 during
modal/transition/arrival work; held/batched keys not leaking through handoff;
and return to the correct mainland facing with retained consequences.

Perform real F9, close the process completely, launch the exact load command
and continue from both sides and after revisit. Confirm no restoration reward,
healing, clock jump or implicit monster reset. Observe that a confirmed exit
with flag 9 clear legitimately places a new entrance Slime for the next visit.
Same-process reload, artificially positioned cameras, dummy-SDL input,
automated screenshots and reviewer inspection do not count as this acceptance.

At the later implementation-candidate closure, require a full build, complete
unfiltered CTest and whitespace check, in addition to all four evidence classes:

```powershell
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
git diff --check
```

## Exclusions and closure criteria

Exclude full Vertigo/city simulation, M38 blacksmith/economy, guild/spell
acquisition, temple, training and other services, broad NPC interaction,
unrestricted indoor combat, unrelated magic, Darkside, general Clouds startup
and arbitrary cross-world travel. Loading geometry or showing a service label
does not authorize that service. The unavoidable Slime, animated visible object,
logical seam and complete exit procedure are the smallest justified additions
to the original entry/traversal/return objective; they do not replan M38.

Close M37 only when the real route and return/revisit are independently useful,
every required script/actor influence is supported, retained states and explicit
resets are distinguishable in code/tests/save behavior, fresh-process play
matches uninterrupted play, legacy domains remain exact, independent review is
accepted and the maintainer's physical route passes. Planning establishes no
such result. No remaining maintainer policy decision is required: the scripted
reset policy has been resolved; the source-derived slot and protection policies
above are explicit reviewable parts of this contract.
