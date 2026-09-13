# Milestone 30 - Bounded expedition encounter integration

**Status: completed and accepted.** M30A established the grouped expedition
domain and durable representation; M30B connected it to the production Journey,
presentation, SDL controls and startup restoration. Milestone 30 extends the
existing Journey rather than creating a second gameplay mode. M31 remains
responsible for Bone Whistle collection.

## Final scope and boundary

The accepted production boundary is:

```text
prepared entry -> bounded navigation and actor approach -> grouped combat
-> successful End -> guarded mutable return -> successive encounter
-> quiet explicit save -> process restart -> further navigation/item management
```

`--journey-expedition` enters Clouds map 20 at `(0,14)` East using Journey
content contract 2. The party may occupy `(0..5,14)` in all four facings, reach
the objective at `(5,14)`, and return to `(0,14)` West. Actor movement uses the
larger admitted region `x=0..8,y=13..15`; water at `(5,15)` and `(7,15)` blocks
nonflying actors. Content-edge and terrain refusals do not publish movement or
time. Returning remains a quiet mutable Journey state.

The expedition admits Skeleton record 9 and Zombie records 17, 18 and 25. It
supports mixed and same-species groups, up to three simultaneous contact actors,
joining, successive encounters, accumulated injury, Disease, equipment
breakage, XP and survivor state. It adds no Run/disengagement, ranged attacks,
spells, Disease cure, item use, healing/rest/recovery, services/training,
recruitment, map transitions, normal Vertigo travel, autosave, mid-combat save,
in-session load or Darkside gameplay.

At the Bone Whistle address, Space presents the contract-2 **Deferred** result
before script dispatch. It performs no WhoWill, grant, Remove, objective
completion, flag/event/object mutation, time charge or RNG draw. M31 owns the
authorized objective mutation and the completed connected slice.

## Content descriptor and prepared entry

One immutable `XeenJourneyContent` descriptor supplies entry camera, navigation
and actor movement bounds, influencing identities, resource predicates, day,
event disposition and persistence contract. Mutable party, world, camera, flag
and random state remain with their existing owners. Contract 1 preserves the
closed M29 Skeleton Journey. Contract 2 admits:

| Record | Type/image | Original position | HP | XP | AC/speed | Attacks/strikes/die | Preferred class/special |
| ---: | --- | --- | ---: | ---: | --- | --- | --- |
| 9 | Skeleton 8 | `(6,14)` | 20 | 250 | 5/10 | 1 / 2 / d6 | Cleric / none |
| 17 | Zombie 9 | `(8,15)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric / Disease 7 |
| 18 | Zombie 9 | `(8,15)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric / Disease 7 |
| 25 | Zombie 9 | `(1,13)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric / Disease 7 |

All 27 MOB actors retain original order and metadata. The other 23 remain
resource-initialized, unactivated and unaccounted, and continue to affect
occupancy. Movement, rendering, combat statistics and special application are
validated separately; admitting the two resource profiles does not admit every
physical monster.

Fresh construction loads the original party, items and flags once, applies the
declared preparation once, initializes all actors once and classifies without
moving. It uses WorldOfXeenClouds/Adventurer, day 8, year 610, minute 480,
`ctr24=0`, zero effects/light/resistances, and no rested/new-day work.

| Active owner | Name/class | Level | Residual XP | HP | SP | Luck |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 0 | Arturius / Paladin | 3 | 1000 | 36 | 6 | 12 |
| 18 | Tyro / Knight | 3 | 2000 | 48 | 0 | 14 |
| 14 | Badger / Ranger | 3 | 1000 | 36 | 6 | 10 |
| 11 | Zippo / Robber | 4 | 1000 | 40 | 0 | 17 |
| 1 | Rebecca / Cleric | 3 | 2000 | 21 | 21 | 14 |
| 6 | Seymour / Sorcerer | 3 | 1000 | 15 | 27 | 15 |

The level/XP preparation uses
`storedXP = 5000 - CLASS_EXP_LEVELS[class] * 2^(level-2)` for level at least 2.
HP/SP are assigned from existing derived rules only at fresh entry. Continuing
state preserves exact current HP/SP, XP, conditions and item bytes; equipment or
Disease may change derived maxima without healing, clamping or reinterpretation.
All 30 supplements retain Might, Speed, Accuracy, temporary AC and XP; contract
2 additionally requires permanent/temporary Luck from CHR offsets 32/33.
Prepared permanent Might/Speed/Accuracy values in active order are
`17/16/15; 19/16/16; 15/15/12; 14/15/18; 12/14/13; 8/14/15`.
Original occupied items retain `material/ID/state/frame` in physical slots:

| Owner | Weapons | Armor | Additional accessory |
| ---: | --- | --- | --- |
| 0 | 0=`0/6/0/1` | 0=`0/3/0/3`, 1=`0/8/0/2`, 2=`0/13/0/6`, 3=`38/10/0/9` | none |
| 18 | 0=`0/2/0/1` | 0=`0/2/0/3`, 1=`0/9/0/5`, 2=`0/13/0/6`, 3=`38/10/0/9` | none |
| 14 | 0=`0/8/0/1`, 1=`0/30/0/4` | 0=`0/2/0/3`, 1=`0/13/0/6`, 2=`38/10/0/9` | none |
| 11 | 0=`0/12/0/1`, 1=`0/12/0/0` | 0=`0/2/0/3`, 1=`38/11/0/10`, 2=`38/10/0/9` | 1=`42/1/0/8` |
| 1 | 0=`0/15/0/1` | 0=`0/2/0/3`, 1=`38/10/0/9` | 1=`42/5/0/8` |
| 6 | 0=`0/7/0/1` | 0=`0/1/0/3`, 1=`38/10/0/9` | 1=`86/1/0/0` |

All six retain common accessory slot 0 `38/2/0/12`. Unspecified slots,
ID-zero metadata and Miscellaneous arrays remain original-loaded values; fresh
entry performs no equipment upgrade or compaction.

## Group ownership, targeting and scheduling

World owns actor identity, coordinates, activation, HP, lifecycle/status and
once-only accounting. Roster owns characters, conditions, equipment, Luck and
XP. Camera and game flags keep their existing owners. One noncopyable
`XeenCombat` borrows them for the complete connected contact episode and survives
deaths, compaction and joining.

Contact membership is the live Physical Present identities in same-cell selected
slots 0..2, ordered by original record. The initiative table contains six party
participants plus up to three enemies. Speed order is descending, with ties by
active party index and then current original-ordered enemy slot; initiative uses
no RNG. New members enter unacted at the next admitted movement boundary, while
surviving identities retain HP and acted state.

Keys 1/2/3 select a displayed contact identity. Selection consumes no turn,
time or RNG, remains bound to that identity across compaction and joining, and
creates a new displayed input generation. Empty, dead, replaced and stale rows
refuse. Input from the preceding frame or SDL batch cannot attack the replacement
or newly selected actor. Space attacks; B blocks. Numeric keys retain their
inventory meaning outside a ready combat turn.

Contract 2 transfers one pending approach movement opportunity into combat at
attachment. That obligation executes exactly once, before player input, and may
join another actor. At round exhaustion, combat clears acted/blocked state,
selects due participants, performs one original actor movement opportunity,
reclassifies/joins, then charges one minute. Enemy attacks due during participant
selection publish individually before the movement/minute operation. Empty
contact after a lethal uses the source-compatible shortcut and proceeds to End
without manufacturing another round. End charges one minute, confirms that no
contact or movement obligation remains, and grants one guarded retirement.
The inherited support boundary rejects a ten-minute charge at minute 950 or
later, and Round/End at minute 959 or later, before that operation publishes.

Player lethal publication atomically sets the selected actor to HP 0,
`(-128,-128)`, inactive Defeated/accounted and awards eligible XP once:

```text
floor(monsterXP / eligibleCount) * (level < 15 ? 2 : 1)
```

Disease and Unconscious characters remain XP-eligible; Dead characters do not.
Failure preserves already published damage, Disease, breakage, actor accounting
and XP. Defeat or failure never creates quiet/save authority.

## Zombie attacks and Disease

Each Zombie turn performs two ordered resource attacks, each consisting of two
`d4` physical applications. No player input interleaves them. Attack 2 reselects
its target; a natural-20 attack keeps the same target across its two damage
applications even if the first incapacitates it.

For each resource attack, select the first eligible Cleric; otherwise draw
`U[0,5]`, and if that owner cannot act draw `U[0,n-1]` from the ascending
eligible list, including `n=1`. No eligible owner is defeat. For an awake target:

```text
r = U[1,20]
if r == 1: miss
else:
    if r == 20: applyDamage()
    hit = r + floor(hitParameter/4) + U[1,hitParameter]
    threshold = currentAC + (blocked ? floor(currentLevel/2)+15 : 10)
    if hit >= threshold: applyDamage()

applyDamage():
    damage = max(sum(strikes x U[1,die]) - powerShield, 0)
    if damage > 0 and special == Disease:
        v = statBonus(effectiveLuck) + currentLevel
        saved = U[1,v+20] <= v
        if not saved: Disease += 1
    subtractHitPoints(damage)
```

`effectiveLuck=max(permanentLuck+temporaryLuck+itemBonus(category 6),0)`.
Disease uses condition byte 4 and admits severity 0..255; an increment beyond
255 fails the complete resource-attack candidate without wrap or saturation.
Disease reduces existing INT/PER/END-derived maxima but not Might, Speed,
Accuracy, AC or Luck. Injury uses the new maximum after Disease. It sets
Unconscious when `maxHP + resultingHP > 0`, otherwise Dead, and breaks occupied
equipped armor on death or HP at most -10. Weapons and accessories do not break.

## Presentation contract

`XeenOutdoorScene` emits actors from the existing 26 selected projection slots,
with source-derived order, anchor, scale and clipping values. One-, two- and
three-actor arrangements remain in the ordered terrain/object/actor stream.
Normal MON frames use their selected-slot orders. ATT moves only the identity
responsible for the retained result to order 121; other actors stay on normal
frames. Reclassification, death or joining cannot transfer an animation to a
different identity.

| Query | Selected slots | Normal orders | X anchors | Y / scale | Two-actor X |
| ---: | --- | --- | --- | --- | --- |
| 2 | 0,1,2 | 118,112,115 | -5,-67,58 | 2 / 0 | 31,-36 |
| 7 | 3,4,5 | 94,92,93 | -7,-38,25 | 34 / 8 | 8,-23 |
| 5 | 12 | 90 | -112 | 34 / 8 | - |
| 9 | 13 | 91 | 98 | 34 / 8 | - |
| 14 | 6,7,8 | 75,73,74 | -8,-24,9 | 53 / 12 | 0,-16 |
| 12 | 14,20 | 69,70 | -65,-85 | 53 / 12 | unchanged |
| 16 | 15,21 | 71,72 | 49,65 | 53 / 12 | unchanged |
| 27 | 9,10,11 | 52,50,51 | -9,-17,-1 | 59 / 14 | -5,-13 |
| 25 | 16,22,24 | 44,42,43 | -34,-41,-26 | 59 / 14 | -27,-37 |
| 23 | 18 | 48 | -58 | 59 / 14 | - |
| 29 | 17,23,25 | 47,45,46 | 16,-16,23 | 59 / 14 | 20,-12 |
| 31 | 19 | 49 | 40 | 59 / 14 | - |

Images 8 and 9 each require eight normal MON frames and four ATT frames through
the existing shared sprite cache. Rebuild validates both sets and consumes no
gameplay RNG. Original forest occlusion is retained; actor authority never comes
from visible pixels. The production panels provide cell/facing/time, pending or
quiet state, named threats, numbered contact rows and live HP, selected target,
acting party member, Attack/Block controls, and ordered damage, Disease,
injury, breakage and XP feedback. Inventory shows exact current/max HP/SP,
Disease, Unconscious/Dead and broken equipment.

The admitted archives retain their source-backed sizes: `008.MON` 35,946 bytes,
`008.ATT` 22,004 bytes, `009.MON` 22,076 bytes and `009.ATT` 14,892 bytes.

## RNG lifetime

Contract 2 stores one world/session-owned Journey RNG continuation across all
encounters. It uses xorshift32 algorithm discriminator 1, nonzero u32 state and
a checked u64 raw-draw count. Fresh entry seeds it once from a nonzero explicit
`--combat-seed` or one sampled value with zero mapped to one. Combat copies the
continuation into an operation candidate and advances world state/count only
when that operation publishes.

Rejected conversion draws and bounded provider prefixes remain part of the
candidate. Stale/refused commands, rendering, animation, target selection,
inventory, saving, restoration, joining, Round and End consume no RNG. Restore
retains state/count exactly and performs no replay. A count of `UINT64_MAX` is
representable at quiet state; the next draw-requiring operation fails before
publication. Contract 1 retains its original seed and transient combat cursor.

## Persistence and restoration

The v4 envelope and v2 base payload remain unchanged. Journey supports exactly:

| Schema/content | Meaning |
| --- | --- |
| 1/1 | M29 record-5 Skeleton Journey, original seed semantics and four-cell/day-1 admission |
| 2/2 | M30 expedition, four keyed actor records, Luck supplements and world RNG continuation |

Crossed or unknown pairs reject. There is no implicit conversion, upgrade or
coordinate-based content inference. Ordinary v1/v2 and completed Diagnostic27
v3 retain their exact behavior. Contract-1 saves retain their 1060-byte schema-1
suffix. Schema 2 has a 1366-byte suffix:

| Offset | Field |
| ---: | --- |
| 0 | domain u8=3 |
| 1 | schema u16=2 |
| 3 | content contract u16=2 |
| 5 | context presence u8=1 |
| 6 | existing 33-byte context |
| 39 | supplement count u8=30 |
| 40 | 30 owner-ordered 41-byte supplements, including two LE i32 Luck values |
| 1270 | RNG algorithm u8=1 |
| 1271 | nonzero RNG state u32 |
| 1275 | committed raw-draw count u64 |
| 1283 | initialized map Clouds/20 |
| 1286 | original actor count u16=27 |
| 1288 | live-record count u16=4 |
| 1290 | four identity-ordered 19-byte records for 9,17,18,25 |

Party, equipment, conditions, context, XP and Luck are durable current values.
The four actor records persist coordinates, activation, HP, lifecycle/status and
accounting. Original MOB metadata, statistics, terrain and events and the other
23 unchanged actors reconstruct from compatible resources. Contact slots,
initiative, acted/blocked state, attack ordinal, selected target, pending work,
End tickets, leases, frames and generations are transient and cannot manufacture
a save boundary. A quiet living survivor must be full-HP, outside contact and
geometrically valid; damaged living actors remain in combat and are unsaveable.

Each 19-byte actor record is keyed by side (`u8`), map id (`u16`) and record index
(`u32`), followed by `x`/`y` (`i16`), current HP (`i32`) and one-byte
activated, defeated, pending-move and pending-direction fields. Live actor
positions remain within the admitted survivor envelopes: actor 9 at
`x=0..6, y=14`; actors 17 and 18 at `x=0..8, y=14..15`; and actor 25 at
`x=0..5, y=13..14`, with terrain validity enforced. Never-activated actors
remain at spawn with full HP; activated survivors may occupy another valid
position, and defeated actors use the canonical defeated representation.

Schema-2 restore also validates signed current/max HP and SP relationships,
active condition domains (`condition[4]` in `0..255`, `condition[12]` and
`condition[13]` in `0..1`, with other active conditions zero), inactive-member
preservation and at least one party member able to act.

Startup restoration decodes and validates unpublished values, reconstructs
immutable resources, applies keyed current state, prepares a pure first frame,
then transfers once into fresh final owners through the restore-only binding.
It performs no CHR/PTY preparation fallback, healing/retraining, movement,
activation, combat, Disease, XP, event, item or RNG work. Invalid successor data
fails startup without fresh fallback. `--load-game` derives the descriptor from
the save and accepts no fresh-entry seed, preparation or content override.

F9 requires a matching presented quiet generation, valid owners, zero pending
approach, empty contact, no combat/attachment/Round/End/retirement/modal/save
lease, and no unresolved or fatal presentation. Refusal performs no capture,
provider or file work and never queues a save. Successful retirement changes
coordination only; the next presented frame opens mutable input and saving.

## Deferred objective and M31 handoff

Contract 2 validates the unchanged 16-record `maze0020.evt` topology at
`(5,14)`, direction All: WhoWill `00 03`, DisplayBottom `00`, If2
`2C 01 03`, TakeOrGive `00 00 15 64`, and Remove, plus original object record 1.
The Deferred disposition recognizes the address after
current-owner, input and quiet checks and before any script-capable callback.
Navigation and actor work have priority. The notice requires a new presented
generation; saving remains closed during its handoff.

M31 may add an exclusive Journey Event/modal activity that binds the existing
EventFlow to these owners, authorizes the specific immediate grant/Remove
publications, and retains the lease through WhoWill, reward acknowledgment and
recomposition. It must preserve earlier combat/world consequences and keep quiet
saving closed while suspended. M30 does not authorize or implement that mutation.

## Final acceptance

- M30A's descriptor, grouped ownership/scheduler, targeting identity, Disease,
  Luck, RNG continuation, schema-2 and restore/capture foundation received
  independent review and acceptance.
- M30B's production integration received an independent implementation verdict
  of **ACCEPT**, with no material corrections required.
- The configured build, full automated suite, original-resource controls,
  malformed/legacy persistence checks, connected witnesses and separate-process
  restart/no-replay evidence passed.
- The maintainer completed physical SDL acceptance covering fresh entry, the
  six-cell route and return, successive/mixed/triple encounters, target selection,
  joining/multiattack, Disease/current-max HP/SP, breakage/defeat, quiet F9,
  process restart with further mutation, Deferred interaction, survivor return
  and legacy contract-1 entry/save/load.

Milestone 30 is closed. Objective collection remains the next separately planned
M31 capability.
