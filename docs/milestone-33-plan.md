# Milestone 33 - Faithful mainland combat and consequences

## Objective and acceptance boundary

Candidate specification against MMModern commit
`5e15e1d9436b01cd491b2831f805029182595854` (Complete Milestone 32 regional
Journey foundations). This document specifies one implementation task with
dependency-ordered internal checkpoints. It does not record implementation or
acceptance, and does not authorize M34 or M35.

Extend the existing Regional Journey to original Orc, Giant Snake and Giant Toad
combat, alongside Skeleton/Zombie, enemy ranged attacks, player physical Shoot,
Poison/Sleep, gold and bounded level-1 monster treasure. Consequences must remain
exact through attachment, victory, reward delivery, mutable exploration, explicit
save, fresh-process restoration and further play. A living actor wounded by Shoot
outside contact must retain its HP, position, activation and identity through that
entire path. Shoot is required production gameplay.

Use the [M32 regional foundation](milestone-32-plan.md), the
[M30 grouped encounter](milestone-30-plan.md),
[M29 Journey ownership](milestone-29-plan.md),
[M28 completion/persistence safeguards](milestone-28-plan.md), and
[M31 Event authority](milestone-31-plan.md). M27 remains authoritative for its
diagnostic and inherited physical arithmetic. M24/M25 retain inventory, transfer
and equipment ownership and legality. Changes below are scoped to new content
contract 4 unless explicitly described as a behavior-preserving shared refactor.

The accepted boundary will be:

```text
prepared regional entry -> resource-derived mainland navigation
-> Shoot / enemy ranged work / contact and joining / Attack or Block
-> exact injury, conditions, actor death, XP and treasure production
-> successful End and guarded retirement when contact ends
-> delivery when no selected threat remains -> presented mutable Journey
-> explicit save -> fresh-process restore -> further mutation and encounters
```

Victory means the end of the current contact episode, not defeat of every actor
on map 23. No actor is healed, frozen, relocated or regenerated to make a witness
possible. All 19 world-owned actors and the M32 terrain closures remain active
participants in the regional scheduler, including off-mainland actors. Geometry
does not authorize scripts: only M32's exact sign is admitted. Myra, Phirna,
transitions and all other event addresses retain their current refusal.

## Evidence and compatibility authority

Evidence labels used here have distinct meanings:

- **ORIGINAL:** values read from the external commercial resources. Resource
  parameters establish data, not executable algorithms.
- **REFERENCE:** algorithms and constants inspected in the clean configured
  ScummVM checkout at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
- **IMPLEMENTED:** code/tests at the MMModern baseline above.
- **INFERENCE:** the explicit bounded integration decision in this contract.

The configured source is `D:/Projetos/MModern/scummvm-known-good-candidate`;
the configured dependency build is
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. HEAD and clean source status
were verified. These are investigation locations, not required future directory
names. [Dependencies](dependencies.md) owns the pin, configuration and GPL
attribution. No ScummVM engine linkage or dependency update is needed.

All reference paths in this document are relative to that pinned source:

| Reference | Decisive functions/data |
| --- | --- |
| [engines/mm/xeen/combat.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp) | `moveMonsters`, `setupMonsterAttack`, `stopAttack`, `monstersAttack`, both `doMonsterTurn` overloads, `doCharDamage`, `setSpeedTable`, `attack`, `attack2`, `hitMonster`, `getWeaponDamage`, `getMonsterResistance`, `giveExperience`, `rangedAttack`, `shootRangedWeapon`, `areMonstersPresent`; `MONSTER_ITEM_RANGES`, `MONSTER_SHOOT_POW`, `COMBAT_SHOOTING` |
| [engines/mm/xeen/character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp) | `synchronize`, `getStat`, `conditionMod`, `charSavingThrow`, `subtractHitPoints`, `hasMissileWeapon`, `isDisabledOrDead`, `makeItem` |
| [engines/mm/xeen/party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp) | `changeTime`, `addTime`, `checkPartyDead`, `canShoot`, `giveTreasure`, `giveTreasureToCharacter`, `arePacksFull`, `Treasure::clear/reset`, `synchronize` |
| [engines/mm/xeen/interface.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp) | Shoot branch of `perform`, `chargeStep`, `stepTime`, `doCombat`, `nextChar` |
| [engines/mm/xeen/interface_scene.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface_scene.cpp) | `OutdoorDrawList`, `setOutdoorsMonsters`, projectile frame selection and `animate3d` |
| [devtools/create_mm/create_xeen/constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp) | `WEAPON_DAMAGE_BASE/MULTIPLIER`, `ARMOR_STRENGTHS`, `STAT_VALUES/BONUSES`, `MAKE_ITEM_ARR5`, equipment restrictions |
| `engines/mm/xeen/map.cpp`, `combat.h`, `item.cpp`, `item.h`, `xeen.cpp` | `MonsterStruct::synchronize`, special/hated-class/POW enums, equipment predicates, item state, inclusive `getRandomNumber` |
| `engines/mm/shared/xeen/cc_archive.cpp`, `engines/mm/xeen/files.cpp` | CC member decoding and `SaveArchive::reset` initial-block order |

**IMPLEMENTED:** the decisive seams are
[`XeenCombat.cpp`](../src/games/xeen/XeenCombat.cpp),
[`XeenActorApproach.cpp`](../src/games/xeen/XeenActorApproach.cpp),
[`XeenRegionalRules.cpp`](../src/games/xeen/XeenRegionalRules.cpp),
[`XeenJourneyRules.cpp`](../src/games/xeen/XeenJourneyRules.cpp), and
[`XeenJourneyFlow.cpp`](../src/app/XeenJourneyFlow.cpp). Today regional contact
clears pending work and stops, regional ranged relevance throws before candidate
publication, attachment explicitly rejects contract 3, and the combat constructor
uses world RNG only for contract 2. None is an implemented M33 capability.

Other necessary current limitations are the two-injury/one-target combat result,
six-ID melee weapon switch, cached initial player speeds, literal year 610 in
combat consumers, `canAct` used for both defeat and enemy targeting, and strict
Disease/Unconscious/Dead-only regional validation. Generalization must replace
these assumptions selectively; changing only profile validation is insufficient.

### Original resource identities

Retain M32's exact map-23 DAT/MOB/EVT manifest, all original actor indexes,
resource-derived 121-cell party component, actor closures and immutable guards.
The original-data probe reconfirmed that manifest and all 19 actor records.
`DARK.CC/xeen.mon` contains 90 records of 60 bytes (5400 bytes, CRC32
`4dec3f50`). Only the five M32 record CRCs are admission requirements; do not
expand admission to unrelated monsters.

**ORIGINAL**, decoded monster records:

| Type/image | Regional indexes | HP | XP | AC | Speed | Attacks | Strikes x die | Hit parameter | Hates | Special | Ranged | Gold/gems/drop | Physical resistance |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | --- | ---: |
| Orc 6 | 0..11 | 25 | 200 | 5 | 17 | 1 | 1 x d10 | 5 | 1 | 0 | 1 | 10 / 0 / 1 | 0% |
| Giant Snake 3 | 12,13 | 15 | 100 | 6 | 18 | 1 | 1 x d10 | 2 | 16 | 5 | 0 | 0 / 0 / 0 | 0% |
| Giant Toad 13 | 14,15,16 | 90 | 500 | 6 | 17 | 1 | 3 x d8 | 8 | 16 | 9 | 0 | 0 / 0 / 0 | 0% |
| Skeleton 8 | 17 | 20 | 250 | 5 | 10 | 1 | 2 x d6 | 4 | 3 | 0 | 0 | 0 / 0 / 0 | 50% |
| Zombie 9 | 18 | 30 | 300 | 2 | 4 | 2 | 2 x d4 | 5 | 3 | 7 | 0 | 0 / 0 / 0 | 50% |

Record offsets: XP LE u32 at 16, HP LE u16 at 20, AC/speed/attacks/hates at
22..25, strikes LE u16 at 26, die/damage type/special/hit/ranged/type at
28..33, resistances 34..40, gold LE u16 at 42, gems/drop/flying/image/effects
at 44..49. All five damage types are physical (29=0), flying/effects are zero;
monster kind is animal=1 for Snake/Toad, humanoid=3 for Orc, undead=4 for
Skeleton/Zombie. Orc resistances 34..40 are `50,50,50,0,0,0,0`; Snake/Toad
are all zero. Skeleton/Zombie retain their complete checked M32 records.

**REFERENCE:** hates=1 is the no-preferred-target sentinel, despite sharing
Paladin's enum value. Hates=16 means the entire party, including incapacitated
and dead members. Hates=3 selects a Cleric. Special=5/7/9 means Poison/Disease/
Sleep, respectively. There is no separate percentage-proc parameter for these
specials. Do not invent one or generalize the other special enum values.

**ORIGINAL**, `XEEN.CC` appearances (bytes; CRC32):

| Image | MON, 8 frames | ATT, 4 frames |
| ---: | --- | --- |
| 3 | 19145; `c9df78af` | 13045; `279fe4ac` |
| 6 | 19022; `0b8c61d5` | 15639; `975cca6f` |
| 8 | 35946; `1d238d62` | 22004; `f73997b9` |
| 9 | 22076; `ceb119e6` | 14892; `e968a6c6` |
| 13 | 10856; `c3aabd05` | 19479; `1402fd81` |

Player `pow11.icn` has 451 bytes, 3 frames, CRC32 `67f7f690`; physical enemy
ranged uses `pow12.icn` (358 bytes, 3 frames, CRC32 `d977e765`). The latter's
reference name is Magic Arrow; using its picture does not make Orc damage magical.
Preflight both resources and all required MON/ATT frames through the existing
checked sprite path. Byte counts/CRCs here are investigation evidence; retain
the existing archive-signature plus exact loaded-resource guard policy rather
than adding an unrelated whole-installation manifest.

## Regional ownership, attachment and combat lifecycle

**INFERENCE:** introduce content contract 4 over the same M32 regional descriptor
and owners. Fresh `--journey-region` selects 4; loading 3/3 continues the closed
M32 behavior, including its support stops. Do not change the meaning of 3/3.
`--journey-skeleton`, `--journey-expedition`, diagnostics and ordinary saves
retain their current domains. No new regional combat coordinator is permitted.

World/session remains the sole owner of all actor identity, current HP,
coordinates, activation, lifecycle/status and defeated accounting. Party/roster
owns current HP/SP, conditions, equipment, supplements and XP. Application owns
camera/flags. The added purse and pending monster treasure belong to
`XeenPartyState`. Flow owns only transient coordination, receipts and presentation.
Detached candidates/preimages are not replacement live parties.

### Contact episode

1. Classify using the existing projection query before pixel occlusion. The
   same-cell contact set contains up to three live Physical identities in
   original-index order. Preserve the approach action and pulse as distinct
   publication boundaries.
2. For contract 4, contact publishes `Engaged`, then transfers directly to
   `Attachment`; it does not publish `SupportStopped` or a Quiet frame. Carry
   any nonzero approach countdown as exactly one owed movement opportunity,
   as in contract 2. A pulse that already consumed it owes none. Do not retain
   M32's contact-time clearing as cancellation authority.
3. `attachJourney` preflights the admitted current party/equipment, original
   profiles/resources and sprites, then constructs the existing noncopyable
   `XeenCombat` borrowing the same graph. No CHR/PTY initialization, new HP,
   treasure clearing, reseeding, artificial party cell or per-species encounter
   is allowed. A wound remains a wound at attachment.
4. Preserve M30 initiative tie-breaking: positive effective Speed descending;
   ties by active party index, then original-ordered contact index. Recompute
   player speeds from current conditions after each condition publication and
   before rebuilding initiative. Track acted/blocked state by participant
   identity, not replaceable contact slot. Zero Speed is legal current state
   and does not enter initiative; it is not an integrity failure.
5. Preserve M30's selection-before-movement order. Due enemy resource attacks
   publish before the transferred attachment movement or ordinary round
   movement. Complete that movement, its queued ranged work and joining before
   displaying a player-ready frame. New contacts enter unacted; surviving
   contacts retain their HP and acted status. Movement can make an Orc both an
   owed ranged attacker and a new contact; joining does not cancel that shot.
6. At ordinary round exhaustion, clear acted/blocked state, select due
   participants, service due enemy work, perform exactly one regional movement
   opportunity with ranged consequences, classify/activate/join, and charge
   one minute using the condition-aware time operation below. The selected
   surviving eligible player remains selected across that movement, as in M30.
7. Attack and Block retain Space and B; 1/2/3 select a displayed contact
   identity without RNG or time. Identity-bound selection survives compaction;
   killing/replacing a row invalidates old input, never redirects it.

Empty contact after a lethal uses the inherited short path to `VictoryAwaitingEnd`
without inventing another round. If movement/ranged work is still owed, service
that obligation first: newly joined contacts resume the same combat object,
and no victory-ready input is exposed in between. A transferred attachment
opportunity does not become a charged ordinary round merely because the last
old contact died. End is allowed only after every transferred
movement/ranged obligation has completed, no contact remains, and this episode
has actually published a lethal (not merely an older nonempty accounted set).
End charges one minute, may produce time consequences, and grants one retirement
only if the party is not defeated. Retirement preserves all actors, inventory,
XP, context, RNG and pending treasure and transfers through Presentation to
mutable Journey. If treasure delivery is now eligible, transfer directly to
Reward before any Quiet input. Otherwise its durable waiting state survives
retirement. End and retirement themselves neither generate nor duplicate rewards.

Defeat means no member whose worst condition is below Paralyzed or Good remains
(`Party::checkPartyDead`). Sleep alone is not defeat. Defeat/fatal failure is
terminal and unsaveable; retain published injuries, deaths, accounting and
treasure for inspection, allow exit, and create no retirement or recovery path.
Failure after publication never clears treasure as a substitute for ownership.

All-asleep contact continues automatic enemy work. Follow `Interface::nextChar`'s
automatic exhausted-cycle reset when nobody can act: reset acted/action state
and initiative/cursors while preserving blocked state, then continue enemy
selection without manufacturing an ordinary
Round movement/minute in this inner cycle. Yield through existing idle service,
not a nested loop, and present completed observations. Damage can wake a target;
a Toad may immediately put it back to sleep. Resume player input only for an
awake, positive-Speed participant. No eligible target after genuine terminal
conditions produces Defeat. Zero-Speed/Poison cases use the same bounded service
and may end in injury or the explicit temporal boundary, never spin within one
SDL call.

### One shared physical consequence resolver

Factor the existing arithmetic/candidate steps so contact, regional ranged work
and Shoot share character damage, actor damage and lethal production. The
resolver is a pure bounded candidate helper, with no world lifetime, independent
RNG, UI, save authority or combat state machine. `XeenCombat` publishes contact
work; the guarded Journey/approach operation publishes exploration work.

A lethal candidate atomically contains actor HP=0, x=y=-128, inactive,
Physical/Defeated, insertion into the world accounted set, all eligible XP,
monster treasure production/loss facts and the advanced RNG. No separate
later scan discovers deaths from HP. The existing `xeenPrepareJourneyLethal`
is the natural shared preparation seam. Before any store, check capacity,
complete owner/resource preimages, integer bounds and all allocations. An
overflow or stale candidate publishes none of that lethal's effects.

XP retains `floor(baseXP / eligibleCount) * (permanentLevel < 15 ? 2 : 1)`
per eligible active owner, added with checked u32 arithmetic. Disease, Poison,
Sleep and Unconscious remain eligible; Dead/Stoned/Eradicated do not. Zero
eligible recipients is a terminal inconsistency for an attempted player lethal,
not permission to drop XP. With all six eligible, increments are Orc=66,
Snake=32, Toad=166, Skeleton=82 and Zombie=100. Physical resistance is applied
before deciding death. Repeated observations, End, cache rebuild, restart and
later engagement cannot account a defeated identity again.

Nonlethal damage changes only that identity's HP plus its operation's other
explicit consequences. A live actor remains at HP `1..baseHP`; never reset it
on becoming invisible, leaving range, attachment, round start or retirement.
No Run or non-victory combat retirement is needed for these survivors: Shoot
can wound them before contact and leave ordinary exploration active.

## Enemy attacks and regional ranged scheduling

### Target and damage contract

Use the following **REFERENCE** sequence per resource attack, evaluated from
the latest candidate state. Zombie's two resource attacks remain separately
published operations with automatic target reselection and no player input
between them. A Snake/Toad resource attack is one candidate spanning all six
targets in active order `[0,18,14,11,1,6]`, including dead/incapacitated targets;
do not truncate it when an earlier member falls. It needs up to 12 damage
applications and 54 distinct armor-slot changes, not today's 2/9 result bounds.

For Orc, draw `U[0,5]`; there is no Paladin preference. For Skeleton/Zombie,
select the first Cleric whose worst condition is outside Paralyzed..Eradicated,
including a sleeping Cleric; if none, draw `U[0,5]`. If the random selection
is Paralyzed..Eradicated, draw `U[0,n-1]` from the ascending targetable list,
even when n=1. Sleep is targetable. Hates-party bypasses these selection draws.
If n=0, mark terminal defeat without an invalid fallback interval, hit or
damage draw. Within an already prepared ranged opportunity, finish visiting
the remaining queued Orcs as `monstersAttack` does: each still consumes its
initial `[0,5]` draw and then takes this no-target path. Publish the complete
opportunity and terminal state together. Contact retains M30's terminal boundary
after a complete resource attack; it does not schedule another Zombie attack
after defeat. This inherited terminal cutoff is distinct from truncating an
in-progress hates-party attack or already admitted ranged opportunity.

```text
physicalAttack(target):
    if target.Asleep != 0:
        applyDamage(target)                 # no d20 or hit-parameter draw
        return
    r = U[1,20]
    if r == 1: return Miss
    if r == 20: applyDamage(target)          # same identity retained
    h = U[1,monster.hitParameter]
    threshold = currentAC(target) +
        (blocked(target) ? floor(currentLevel(target)/2)+15 : 10)
    if r + floor(monster.hitParameter/4) + h >= threshold:
        applyDamage(target)

applyDamage(target):
    target.Asleep = 0                       # before dice and special
    damage = max(sum(strikes x U[1,die]) - powerShield, 0)
    if damage > 0 and admitted special != None:
        v = statBonus(effectiveLuck(target)) + currentLevel(target)
        if U[1,v+20] > v: increment the selected condition
    subtract damage; apply inherited injury/breakage using new conditions
```

In M33 powerShield is zero. A natural 20 does not skip the hit-parameter draw;
the second threshold uses AC after the first application, including Poison's
Speed reduction or broken armor. If the first application puts the target to
sleep, the already-entered awake branch still performs its normal hit test.
Each positive application has its own special save; no special draw on a miss
or zero damage. A successful physical save is `draw <= v`, including equality.
Use signed wide intermediates: low-stat bonuses may be negative. Check `v+20`
as a valid positive interval, not an unsigned cast of a negative value.

Subtract HP with checked i16 bounds. At resulting HP<1, set Unconscious if
`maxHP(newConditions)+HP >= 1`, otherwise set Dead. Preserve simultaneous
conditions. Break every occupied equipped armor slot if HP<=-10 or the injury
sets Dead; preserve frame, material and ID and OR state with `0x80`. Do not
break weapons/accessories. Death due solely to the time stat check below is a
different path and does not invent HP subtraction or armor breakage.

### Ranged opportunity

Preserve the M32 7x7 scan, two passes, y difference `3..-3`, x difference
`-3..3`, original-index inner loop, occupancy, moved-once rules and terrain
fallback. Speed does not give Orc additional exploration movement. At each
existing before-movement callback, use the candidate collection including
earlier actors' candidate positions. An eligible shooter is activated, Present,
Physical, not already moved, Orc/ranged=1, not a current same-cell contact,
aligned on party X or Y, within the scan, and not previously range-tested in
this opportunity. The per-opportunity range-tested flag is set after the
attempt, even for an obstructed ray; it is independent of moved-once.

Use `xeenOutdoorRangedRay` unchanged: sample from the party toward and including
the actor, excluding the party cell. East rejects raw wall mask `0x8`; west,
north and south permit only middle `0,2,4,5,8,11,13,14`. Surface water does not
itself obstruct this ray. Facing affects visible projectile lanes, not damage
eligibility. Do not replace this asymmetric rule with party collision, symmetric
line-of-sight, pixels or mainland membership.

A clear ray appends a retained identity/profile/origin/distance/facing result
to the opportunity queue, at most once per actor. It still permits that actor's
ordinary move. Finish both movement passes first, then resolve queued Orc
attacks in append order, using the shared target/damage sequence and candidate
party state after earlier attacks. No extra ranged hit formula, accuracy penalty,
damage roll or special chance is added. A later move into contact does not erase
the queued attack; use its retained source identity, not whichever actor now
occupies that slot. Queue bound is 19 identities (only 12 can be Orcs here).
The admitted opportunity therefore needs space for 12 shot observations,
24 damage applications and up to 54 distinct armor-slot changes. It can consume
up to 72 accepted target/hit/damage draws, before conversion rejections; a
64-raw-draw service budget must yield without prematurely publishing this unit.

**INFERENCE:** keep a whole movement opportunity, its queued ranged damage,
activation/classification and RNG in one checked publication candidate. This
preserves M32's all-or-nothing action/pulse boundary; it does not publish an
early actor prefix and then falsely report the rest as a harmless failure.
Fallible preparations may yield after at most 64 raw draw attempts per service
call. Such a candidate retains its scan/result cursor and lease and closes
navigation, Shoot, inventory, Event and saving until it publishes or fails.
When a charged Wait also flushes old work and consumes its new opportunity,
prepare the two opportunities in that order in the same action candidate.
Its aggregate result must accommodate 24 shots/48 damage applications, the
54 distinct armor slots and the time-tick observations; an upper bound of
168 accepted draws includes both opportunities and a maximum 24-draw tick.

For an exploration charged move/Wait, time effects precede old/opportunity work,
as in M32/`chargeStep`; for Round, movement/ranged effects precede its one-minute
charge. Earlier published actions remain intact if a later pulse fails. Owed
ranged work is part of pending work: a zero movement countdown alone never
proves Quiet. A possible shot on a future opportunity is not currently owed.

At the end of publication, defeat has priority over attachment; otherwise
same-cell contacts transfer to the existing combat path. A queued ranged-only
attack does not create a combat episode or require combat End. No free player
input, reward delivery, automatic sign dispatch or capture may interleave the
opportunity's ranged attacks.

## Player physical Shoot

**REFERENCE:** `Interface::perform` exposes Shoot in exploration, not the combat
menu. M33 follows that boundary. Add a typed `ShootAction`, bound to **F** in
regional exploration; S remains the accepted backward movement key. F has no
Shoot meaning in contact combat or legacy contracts. No ammunition, target
picker, spell variant or new projectile simulation framework is required.

### Eligibility, rows and draws

A volley includes every active owner in active order who has a raw frame-4
missile weapon and whose worst condition is neither Asleep nor
Paralyzed..Eradicated. Use existing M25 legality for weapon IDs 30..33. The
reference predicate checks the frame and disability, not broken/cursed state;
bad weapons retain base dice in the weapon scan. Preserve that rule for admitted
records. Validate consumed weapon IDs/frames/material contributions before
starting; an unknown equipped contribution refuses with owner/category/slot
feedback, never consumes RNG or silently excludes that shooter. Carrying
unknown unequipped items remains valid.

Capture the eligible shooter set and forward target rows at the checked firing
boundary. On each row, visit up to three current live identities in original
order, and for each target visit still-unspent shooters in active order. Each
shooter gets at most one hit in a volley. A miss leaves its shot available for
the next target, including the next actor in the same row; reset the reference
missed-shot marker at each new target. A hit consumes its shot even if resistance
or the monster save makes damage zero. When a target dies, stop that target's
shooter loop and move to the next retained live target; do not redirect remaining
shots through `attack2`'s mutable global contact selection. This identity-safe
adaptation preserves the exploration reference's no-contact lethal early return.

Forward center rows are distance 0/1/2/3, corresponding to projection queries
2/7/14/27 and contact slots 0..2 / 3..5 / 6..8 / 9..11. Distance 0 is normally
empty because contact attaches before exploration input. Before entering rows
1, 2, 3, test that destination row's middle; `1,3,6,7,9,10,12` stops the entire
volley before targets on that row. All other middle values pass, including 15;
do not substitute the enemy ray predicate. No fourth forward step, diagonal,
lateral selected slot, wrap or neighbor-map target is admitted. A ray reaching
the map edge ends harmlessly there. Resolve target authority from the world and
checked projection, never from visible pixels. Occluded actors do not disappear
from the targeting contract.

Bound the retained volley to six shooters and four rows of three identities
(72 shooter/target attempts before spent-shot elimination). Exploding hit dice
and raw conversion rejection use continuation, not an arbitrary roll cap or
an unbounded SDL call. Recheck retained target liveness/identity before each
attempt; already published lethal removal skips that target without a draw.

For every attempted shooter/target pair, draw weapon dice **before** hit dice.
Let B be the sum of every frame-4 weapon's base dice in ascending physical slot
order (ordinary admitted weapons have material 0 and no elemental contribution).
`weaponDamage = 3 * max(B,0)` in Adventurer. Missile IDs 30/31/32/33 use
`3d2 / 5d2 / 4d2 / 2d2`. Hit chance is:

```text
statBonus(effectiveAccuracy) + 5 + floor(currentLevel / classDivisor)
    + sum(exploding U[1,20] until the first value != 20)
```

Class divisors in class-ID order 0..9 are `1,2,2,3,4,2,2,1,3,2`.
Hit iff chance >= monster AC+10. There is no automatic player miss on a 1,
no Might damage bonus, and no melee level-based extra attack count for Shoot.
On a hit with pre-resistance damage>0:

1. `damage = floor(weaponDamage * (100-physicalResistance)/100)`.
2. Draw `U[1,50+monsterTypeId]`; if draw<=monsterTypeId, halve damage with
   integer truncation. This `attack2(RT_HIT)` monster saving throw is required
   even though the attack is physical. Do not apply it to melee. If physical
   resistance already reduced damage to zero, the entered positive-damage
   branch still takes this draw.
3. Apply damage, and if lethal immediately prepare XP, gold/drop and accounting
   before any next target or shooter draw.

No magic-resistance roll or elemental damage applies to the admitted ordinary
missile weapons. All rejected xorshift conversions still count as raw draws.

### Time and publication

Only a current presented exploration generation accepts F. First perform the
ordinary checked eligibility/admission. If old actor work is owed, finish it
before firing; if it attaches combat, discard this Shoot intent and require a
new exploration F after retirement. Do not queue a delayed shot across combat.
This is the same stale-intent safety adaptation used by the existing Flow.

An empty/invalid volley refuses before time, ctr24, actor opportunities or RNG.
An eligible volley into empty space, a wall, or with every shot missing still
costs ten minutes and arms pending=3. **Shoot does not increment ctr24**:
the reference calls `chargeStep` and `doStepCode`, not `stepTime`. Process shots
and lethal production first; attempt any eligible treasure delivery before the
ten-minute charge, following `rangedAttack`'s `giveTreasure` ordering. The charge
and new actor opportunity remain owed while that receipt is open. After final
acknowledgment, service the charge/time candidate, arm pending=3 and use normal
100 ms pulses. No duplicate charge on cosmetic replay. No automatic sign event
is invented by Shoot or its empty-space result.

Preflight the calendar support boundary before accepting the volley, so a shot
at a time where its required charge would cross dusk cannot kill an actor and
then evade the required charge. Prepare any condition draws only at their true
post-volley position in the RNG sequence. If later fallible work fails, retain
published shooter-target consequences and leave the owed charge terminally
unsaveable; do not refund them or reopen input.

Publish each completed shooter-target attempt (including its lethal production)
as one operation under the exclusive Shoot activity. Bounded draw continuation
never exposes a partial hit or partial reward. The overall volley is not a
rollback transaction: a later failure preserves earlier published attempts,
their accounted identities and RNG. The retained target list, shooter-spent
bits, charge obligation and projectile frames are transient and unsaveable.

## Conditions and the minimum time extension

### Condition representation and rules

Use the existing 16 condition bytes: Poison=3, Disease=4, Sleep=8,
Unconscious=12 and Dead=13. Contract 4 admits Poison/Disease `0..255`, Sleep
and Unconscious `0..1`, Dead `0..255`; all other active conditions remain zero.
Inactive owners preserve their exact existing bytes. A special increment beyond
255 rejects the complete resource-attack candidate, as M30 does for Disease;
no wrap, saturation or signed reinterpretation. Sleep cannot accumulate above
1 through the admitted attack path because every application clears it before
its possible increment. No passive wake decrement or recovery action exists.

`conditionMod` reduces Might, Speed and Accuracy by Poison severity and reduces
Intellect, Personality and Endurance by Disease severity. Luck is unchanged.
Dead suppresses these condition modifiers, as in the existing derived-stat
rules. Effective attributes are `max(permanent + temporary + age adjustment +
admitted item bonus + condition modifier, 0)`; Luck has no age adjustment.
Use the retained context year everywhere, including combat AC, injury maxima
and refreshed initiative. Poison therefore reduces melee damage/hit, Shoot hit,
initiative and Speed-derived AC; it does **not** directly tick HP or reduce
INT/PER/END maxima. Disease retains its inherited current-versus-max behavior.
Never clamp/heal current HP/SP when a maximum changes.

Action eligibility, enemy targetability, XP eligibility and party defeat are
different predicates. Sleeping members cannot Attack/Block/Shoot or receive
treasure; enemy attacks can target and wake them; they can receive XP. Outside
contact, a party with sleeping but nonterminal members can still navigate/Wait,
inspect and use inherited inventory controls, as in the reference's party-level
exploration commands. It is not auto-defeated or auto-woken. Quiet saves may
retain Sleep, including an all-asleep party when no mandatory work is owed;
restore preserves it. Individual equipment/transfer restrictions remain M24/M25,
which intentionally do not add a `canAct` gate.

### Why 950 -> 960 must become supported

**ORIGINAL/IMPLEMENTED:** the checked mainland paths include a 16-move route
from `(9,11)` to `(8,2)` and return; combat rounds, Shoot and return branches
add time. Forty-eight ten-minute charges reach the first post-entry tick even
without combat. Repeated regional play and the required restart/continued-play
witness cannot truthfully retain a permanent stop before 960 while claiming
these consequences are supported. M33 implements that tick, not a deferred
flag or skipped-effect clock advance.

**INFERENCE:** retain M32's calendar domain `day 0..99`, year u16, minutes
`300..1259`, ctr24 `0..23`, WorldOfXeenClouds/Adventurer, effects/resistances/
light zero and rested/newDay false. Retain the checked pure `xeenPrepareTime`.
Allow its 480-minute processing count; continue to refuse before dusk 1260,
dawn, midnight/year rollover, daily work or nonzero unsupported effects. Only
charges 1 and 10 occur here, crossing at most one 480-minute boundary. This
covers 480 for valid earlier daytime restores and 960; fresh entry at 480 does
not retrospectively process 480. Zero-time turns, cosmetics, inventory, saving
and restore never call `changeTime(0)` or draw RNG.

**REFERENCE, deliberate quirk preservation:** pinned `Party::changeTime` uses
`if (!Poisoned)` and `if (!Diseased)`. The predicates are inverted relative to
the comments. M33 follows the verified executable reference logic, not its
comments or an invented cure system. At each tick, in active-owner order:

1. If not Dead, test effective Might, Intellect, Personality, Endurance, Speed,
   Accuracy and Luck in that order. A value<1 sets Dead=1 immediately. Later
   attribute reads observe Dead's suppression of condition modifiers. Preserve
   HP/SP; this path does not call `subtractHitPoints` or break armor.
2. If Poison==0, draw `U[1,10]`. Only on 1 draw an electrical save
   `U[1,R_e+40] <= R_e`. Either branch leaves Poison zero (zero doubled or
   zero cleared). If Poison!=0, draw nothing and retain its exact severity.
3. If Disease==0, draw `U[0,9]`. Only on 1 draw a cold save
   `U[1,R_c+40] <= R_c`. Either branch leaves Disease zero. If Disease!=0,
   draw nothing and retain severity. These checks still run for Dead members.
4. If Dead!=0, increment it with the checked byte bound. An owner newly killed
   by step 1 becomes Dead=2 at this tick. Other reference tick branches have
   zero admitted inputs and no effect/draw. Sleep is unchanged.
5. After all members, publish the checked context increment and complete
   character/RNG delta together. No per-character partially published tick.

Electrical/cold save values are the character's permanent+temporary resistance
plus equipment `itemScan(12/13)`. Admitted ordinary loot and original permitted
equipment give no elemental resistance bonus. Retain the four CHR byte inputs
per owner: cold at offsets 313/314, electrical at 315/316. Do not substitute
Luck, level, party resistance fields or literal zero. **ORIGINAL:** active
cold/electrical permanent values are both `[7,10,2,5,7,0]`, temporaries zero.
All 30 owners must retain their resource-loaded inputs, not just active six.

The tick requires at most 24 accepted draws (plus rejected conversions). Process
it through the same bounded world RNG continuation. Structural/calendrical
refusal precedes draw preparation; stale/overflow/preparation failure commits
neither condition nor time/RNG changes. If the complete tick leaves no
nonterminal member, publish its time/conditions/RNG then terminal Defeat;
do not create a new actor opportunity or mutable frame from the dead party.
For Round/End, earlier separately published enemy actions remain intact.

Allow Dead with positive HP because the stat-death path preserves HP. Still
require Unconscious=>HP<=0; for a member with neither Unconscious nor Dead,
require HP>0. Do not infer the cause of a saved Dead byte or recalculate death
on load. At least one nonterminal member is required for a quiet save, rather
than the old requirement that at least one member can act immediately.

## Gold, monster treasure and equipment closure

### Production and exact level-1 generation

**REFERENCE:** `Combat::attack2` produces XP, then monster gold/gems, then
tests/generates a drop, then removes the actor. That entire logical lethal is
one atomic MMModern publication as specified above. The five admitted profiles
produce no gems. Each Orc produces exactly 10 gold and tests drop-level 1;
the other profiles produce no money/items and take no drop draws.

For each Orc lethal, after its damage/save draws:

```text
drop = U[1,100]
if drop <= 10:
    category = U[0,100]          # inclusive: 101 possible results
    subcategory = U[0,100]       # drawn even for armor or miscellaneous
    if category <= 40:
        if subcategory <= 30: id = U[1,6]
        elif subcategory <= 60: id = U[7,17]
        elif subcategory <= 85: id = U[18,29]
        else: id = U[30,33]
        kind = Weapons
    elif category <= 85:
        id = U[1,7]; kind = Armor
    else:
        wandMaterial = U[1,9]; kind = Miscellaneous
    ignoredEnchantment = U[1,100]  # always consumed at level 1
    if kind == Miscellaneous:
        specialId = U[1,15]
        charges = U[1,8]
```

Drop probability is exactly 10%. Conditional generated categories are
41/101 Weapons, 45/101 Armor, 15/101 Miscellaneous; weapon subcategory weights
are 31/101, 30/101, 25/101 and 15/101. Reachable ordinary delivered equipment
is Weapons IDs 1..33 or Armor IDs 1..7, material=state=frame=0. Level 1 draws
no metal, elemental/attribute enchantment, slayer counter, curse or breakage.
There are no accessories, shields, helms, boots or gauntlets generated at this
level. Do not generalize `makeItem` to levels 2..7.

**REFERENCE loss, intentional:** the pinned `attack2` Miscellaneous case copies
`tempChar._accessories[0]` into the accessory treasure queue, not the generated
miscellaneous item. The temporary character initializes that accessory empty.
Consequently the generated wand (material 1..9, special ID 1..15, charges 1..8,
frame 0) never becomes an inventory item through this monster path. Preserve
all six generation draws after the successful drop draw (seven total), including
its ignored enchantment, and record a `ReferenceMiscellaneousDropLoss` result.
Do not deliver the wand through M21's miscellaneous queue or invent an item-use
dependency. The loss is established by the pinned reference, not asserted as
independently verified DOS executable behavior. A later decision to fix that
reference quirk would require a separate explicit contract change.

Append a generated nonempty weapon/armor to the first free physical slot of its
ten-slot treasure category. Full-category overflow loses that new record while
preserving XP/gold and every generation draw; it must be reported as a capacity
loss. Do not replace the oldest record, reroll category, or suppress generation
when the queue is full. `hasItems` is derived from nonempty admitted records;
the reference's true flag after an empty miscellaneous copy adds no gameplay
effect here because every such Orc also produced gold.

### Owners and settlement state

Add one optional contract-4 consequence value to `XeenPartyState`, present only
in this domain. It contains:

- carried gold u32, initialized once from original `maze.pty` LE u32 at 638;
- carried gems u32, retained inert from offset 642 to preserve the original
  purse (no gem production, spending or services are introduced);
- pending Orc production mask, keyed by original indexes 0..11;
- pending gold u32, exactly ten times that mask's population count;
- ten weapon and ten armor treasure slots, each occupied record carrying its
  original Orc index and exact four item bytes.

**ORIGINAL:** the initial purse is 800 gold and 10 gems. Read it from the same
checked initial archive used by the existing party loader; do not hard-code
those observations as a replacement source. The nested initial CC data is
assembled by the existing bridge from blocks `2a0c,2a1c,2a2c,2a3c,284c,2a5c`;
no extraction into the repository is needed. Fresh pending values are empty.
Restoration never reads PTY to repair missing purse data.

The production mask is a pending-obligation subset of the world defeated set,
not a second death owner. Set its bit with the lethal; reject duplicate
production. Each retained item source is a unique member of that mask across
both categories. An Orc with no item/drop loss/overflow still contributes its
gold bit. Settlement clears the mask, but the world defeated set remains forever.
Do not infer an item's source from its name, current inventory position or a
future actor scan. Receipts retain source identities as observation only.

There are at most 12 pending Orcs, 120 pending gold and 12 generated nonempty
items overall, with ten per-category capacity. Retain checked u32 purse
arithmetic: before publishing a lethal verify that carried+pending+new gold
fits u32. Overflow rejects that complete lethal rather than wrapping or
discarding money. Arbitrary valid retained purse values need not be derived
backwards from XP/defeated counts. Gems remain exact, unspent current state.

**REFERENCE:** treasure delivery waits while any of the 26 selected monster
projection slots is occupied (`areMonstersPresent`), not merely while contacts
exist or while combat mode is active. Use the checked unoccluded classifier.
An off-contact wound or living visible bystander can therefore delay collection
across a combat retirement, successive encounters and ordinary movement/turns.
Do not force another encounter, clear distant actors, or deliver early solely
because contact is empty.

**INFERENCE, explicit save-boundary extension:** pending *uncollectable*
monster treasure is durable world-play consequence state. A presented Quiet
boundary may save it when no action/pulse/contact/event/receipt work is owed
and at least one selected living actor still makes delivery ineligible. It is
not the transient M21 event reward queue or a partially executed delivery.
Serialize the pending value below so turning away after restart can collect it
once. Banning these saves would unnecessarily make otherwise safe ranged
survivor/continued-play states unsaveable; dropping the value would lose rewards.
An eligible but unserviced delivery is mandatory work and forbids Quiet/save.

At the end of each relevant classification/publication, and at successful
combat End, check for pending treasure and empty selected slots. If eligible,
acquire `XeenCombatBoundary::Work::Reward` under a Journey Reward activity
before callbacks or player input. Complete prior owed ranged/attachment work
first. In Shoot, its own delivery point precedes the owed charge as specified
above. A pending sign transfers through Reward first, then Event, without a
Quiet gap. Script authority is not shared with treasure production.

### Delivery and acknowledgment

Reuse the M21 receipt/pagination presenter and M24 category insertion primitives,
but implement the typed monster delivery path on the party-owned queue. Do not
pretend the current `XeenPendingRewards` ten-miscellaneous array owns monster
gold or ordinary equipment. Shared presentation/helpers are welcome; neither
event finalization nor stale WhoWill state may claim this queue.

Delivery visits Weapons then Armor, each in retained production-slot order.
There is no preferred recipient for monster treasure. For each record, choose
the first active owner who can act and whose matching category tail (slot 8)
is empty. Sleeping/unconscious/dead owners cannot receive it; an earlier hole
with an occupied tail is not capacity. Proficiency does not govern receiving
or carrying an item. Insert into slot 8 and stable-compact that one category,
preserving occupied order and item bytes. Generated frames remain zero. Do
not auto-equip, merge duplicates, compact other categories or heal HP/SP.

The original global-full test covers all four category tails of every member,
irrespective of eligibility. Present its warning when true. When globally full,
clear remaining item treasure with explicit loss facts; this does **not** clear
gold, because `Treasure::clear` clears arrays only. A category with no eligible
recipient loses its undeliverable records even if another category has room.
Report `NoEligibleRecipient` or `CategoryTailsFull`; no arbitrary fallback to
an unconscious member. The source's fallback comment says otherwise, but its
actual predicate still calls `!isDisabledOrDead`. No special quest/sword
capacity eviction applies to these level-1 IDs.

Prepare the entire bounded item-delivery delta and receipt before a nonthrowing
publication. Publish all recipient categories, clear those pending item slots
and adopt the receipt phase before fallible text/frame work. Delivery occurs
once before the receipt acknowledgment, as in the existing event reward
adaptation. Credit pending gold to the purse and clear pending gold/mask on the
**final** receipt acknowledgment, matching the reference's later gold credit.
This is a separate guarded, checked publication. Intermediate pages/warning
acknowledgments do not credit gold. Escape follows the existing reward-page
acknowledgment semantics, not a reward cancellation/refund. Window close exits.

No save is possible during warning, item delivery, receipt, gold credit or its
Presentation handoff. A failure after item delivery preserves those inventory
items and cannot repeat them; a failure after final gold credit preserves the
purse and cannot recredit it. A recovered frame re-presents observation, not
production/delivery. A fatal failure remains terminal; closing the process
does not retroactively alter the previous explicit disk save.

### Close every obtainable equipment path

For contract 4, broaden the bounded physical consumer to all ordinary Weapons
1..33, Armor 1..7, plus the existing admitted original equipment. Preserve
M25's exact class masks, frames, two-handed/shield conflicts and removability;
do not broaden equipment legality merely because a catalog can name an ID.
Weapon 34 remains equip-supported by M25 but is not generated by this path;
its physical base consumer may share the same bounded 0..34 table below.
All lawful arrangements of generated equipment must pass combat readiness,
including transferring loot to another active owner and equipping it there.

Use the pinned weapon tables in ID order 0..34:

```text
dice = 0,3,2,3,2,2,4,1,2,4,2,3,2,2,1,1,1,1,4,4,3,2,4,2,2,2,5,3,3,3,3,5,4,2,6
die  = 0,3,3,4,5,4,2,3,3,3,3,3,2,4,10,6,8,9,4,3,6,8,5,6,4,5,3,5,6,7,2,2,2,2,4
```

Melee scans frames 1/13 in physical-slot order, draws their base dice before
each exploding hit roll, and uses M27/M30's class attack count and hit formula.
For each successful melee attempt add `max(statBonus(effectiveMight) +
3*baseDiceTotal, 1)`; sum successful attempts, then truncate once for physical
resistance. Do not apply Shoot's monster saving throw or multiply Might by
three. There is no generated metal/elemental/slayer behavior to implement.
Retain original supported item-derived stat bonuses and armor contributions.
Armor strengths indexed 0..13 remain `0,2,4,5,6,7,8,10,4,2,1,1,1,1`;
AC uses current Speed bonus, temporary AC and existing item AC bonus, with
broken/cursed armor contributions skipped, then clamps at zero.

Consumed ordinary weapons admit state 0 and preserved bad-state bits `0x40`,
`0x80`, `0xc0` with counter zero. Their base dice still apply even when bad;
badness suppresses special/material contributions in the reference, not base
dice. Ordinary generated material is zero. Original supported accessory/armor
materials and the legacy medal exception retain their accepted semantics.
Unknown counters/materials remain storable/inspectable, but are not silently
interpreted as ordinary equipped contributions. M24 inspection must show the
newly delivered record, selected exact bytes and recipient; subsequent transfer,
legal Equip/Remove, melee/Shoot and persistence must all consume that same record.

## Publication, failure and resource authority

All draw consumers use world-owned algorithm-1 `XeenJourneyRandomState` in
contract 4. No reseed at contact, retirement, Shoot, time ticks, delivery or
restart. `XeenCombatRandom` retains xorshift32 and rejection conversion:
for span=hi-lo+1, threshold=`(0u-span)%span`; each raw draw advances the checked
u64 count, values below threshold retry, otherwise result=`lo+raw%span`.
The full count includes rejected conversions. Count UINT64_MAX is a valid
quiet saved value; the next draw-requiring operation fails before publication.

Each operation captures actual owner identities/incarnations/replacements,
revision, current activity/lease/input generation, complete mutable preimage
and all admitted immutable preimages. Copy draw state into its candidate;
after each callback/provider/draw check current authority. Prepare result
storage and next preimages before publication. Publish by scalar/fixed stores
or prepared swaps, then adopt expected state/result/phase before fallible
feedback. A probe gets observations, never a replay/publication capability.

The concrete publication units are:

| Unit | Atomic gameplay delta | May already have published predecessors |
| --- | --- | --- |
| Contact player Attack | All melee attempts against one identity, damage/lethal/XP/treasure/RNG, acted state | Earlier participants |
| Enemy resource attack | All hated-party targets or the selected target, up to two damage applications each, conditions/breakage/RNG | Zombie attack 1; earlier participants |
| Regional action/pulse opportunity | Camera/context as applicable, complete movement passes, queued ranged effects, activation, RNG, next work | Prior action or pulse |
| Shoot attempt | One shooter/target outcome, actor damage/lethal/XP/treasure/RNG and spent bit | Earlier volley attempts |
| Time charge | All six members' tick effects, context and RNG; appropriate next-work state | Volley, earlier round enemy work |
| Item delivery | All bounded inventory insertions/losses, cleared item queues and receipt phase | Lethal production |
| Final receipt acknowledgment | Gold credit, cleared pending obligation and handoff | Item delivery |
| End/retirement | Inherited checked time/terminal/coordination transitions | Every combat consequence |

When time is part of an action or Round candidate, its row is included in that
larger operation, not published twice. The explicit post-volley charge is a
separate operation because the reference permits intervening delivery.
Enemy/ranged result capacities must cover their entire declared unit. Observation
can paginate; gameplay cannot truncate because a panel has fewer rows.

Retain M28/M31/M32 stale and ABA rules across every new field and operation.
Changed-then-reverted purse, condition, pending item/source, resistance, RNG,
actor or owner replacement invalidates the old capability. Reentrant callbacks
may not authorize the outer operation to overwrite or stop newer work. Zero
pending countdown with a live candidate, queued shot, owed Shoot charge,
eligible delivery, attachment, End or modal lease is never Quiet.

Retain the union of immutable map/MOB preimages through action, pulse, Shoot,
combat construction, combat publication, delivery, retirement, inventory renewal,
capture and restore. A fresh guard may adopt authorized mutable changes but
must carry the prior admitted resource set even if caches are empty. Validate
requested key **and** internal identity before insertion. Changed known DAT/MOB,
EVT, monster record or admitted sprite bytes permanently latch integrity failure;
matching retries and mutation-and-reversion cannot revive authority.

Sprite cache reconstruction needs the same retained compatibility discipline as
the new projectile/ATT preflight; never treat absence after eviction as permission
to accept a different value. Missing/malformed required resources are visible
failures, not absent monsters/projectiles. Optional material-name fallback stays
M24's existing bounded fallback and does not alter item state. Provider I/O or
allocation failure that returned no incompatible value retains the established
recoverable presentation retry policy (one retry in the current Journey path).
Recompose from published observations and retained work; retry never repeats
damage, random draws, gold or delivery. Exhausted/fatal presentation failure
keeps capture/input closed. The existing corrupt-archive-index limitation remains.

## Persistence and entry/restore policy

### Exact new state and wire layout

Use the unchanged v4 envelope and unchanged v2 base payload, with the new exact
Journey schema/content pair **4/4**. Reuse schema 3's 1651-byte regional prefix
layout, except the explicit schema and contract fields are both 4. The existing
30 Luck-bearing supplements, context and 19 actor records retain their byte
encodings. Append the fields below immediately after the actor table. All
offsets are relative to the beginning of the Journey suffix, not the file.

| Offset | Field |
| ---: | --- |
| 0..1650 | M32 prefix: domain=3, schema=4, content=4; context present=1; 30 supplements; algorithm/state/count; Clouds/23; original/saved count=19; 19 actors |
| 1651 | resistance supplement count u8=30 |
| 1652..1801 | 30 records of 5 bytes: owner u8 in 0..29 order, cold permanent u8, cold temporary u8, electrical permanent u8, electrical temporary u8 |
| 1802 | carried gold u32 |
| 1806 | carried gems u32 |
| 1810 | pending Orc production mask u32 |
| 1814 | pending gold u32 |
| 1818 | pending weapon count W u8 |
| 1819 | pending armor count A u8 |
| 1820 | W weapon records, then A armor records, each 5 bytes: source original index u8, material/id/state/frame u8 each |

Suffix length is exactly `1820 + 5*(W+A)`; W/A are 0..10 and their sum<=12.
Read both counts and validate remaining length before allocation. Entries are
packed in actual retained production order within their category; no sorting by
ID/source and no holes are accepted on this wire. Runtime unused treasure slots
are canonical empty. These newly defined queue bytes do not normalize existing
inventory holes or ID-zero metadata in the base payload.

Pending mask uses only bits 0..11; every set bit must identify a canonical
defeated/accounted Orc. Pending gold must equal `10*popcount(mask)`. Item sources
must be distinct across both queues and set in the mask. Pending records must
be material/state/frame zero and Weapons IDs 1..33 or Armor IDs 1..7. No
Miscellaneous/accessory/gem treasure, empty ID, duplicate source, noncanonical
item or cross-category source duplication is admitted. Empty mask requires
zero pending gold/counts. Carried gold+pending gold must fit u32. No inferred
equivalence connects carried purse, character inventories, XP or quest values
to actor accounting; only the pending-obligation consistency above is required.

Represent the new resistance inputs explicitly as optional presence in the
roster supplement, and the purse/treasure value as optional presence in the
party. Contract 4 requires them all. Legacy contracts require them absent,
not synthesized zero values. A truncated/absent new field, missing owner,
invalid count, unknown schema, crossed schema/content pair, extra byte or
overflow rejects the file. Existing 4 MiB, CRC32, exact payload/EOF, strict
boolean/enum, archive fingerprint and reserved-byte checks remain unchanged.

### Current-state validation and safe capture

Resource-dependent actor rules remain M32: complete original order, exact
metadata/profile, live HP `1..baseHP`, Physical/Present/unaccounted, valid closure,
unactivated actors at spawn; canonical defeated HP=0, x=y=-128, inactive,
Physical/accounted. Occupancy is at most three. An unactivated actor can retain
a wound at its spawn if it was hit before an activation boundary; do not infer
activation from damage. Save requires no same-cell live contact and all
activation required by the current classification already published.

Party validation uses the new condition domains, stat-death HP exception,
all-sleep policy and complete 30-owner supplements above. Retain original
membership/order, signed HP/SP and current independent maxima, all exact item
arrays, flags, counters and independent overlays. Item storage validity remains
distinct from readiness to use an equipped contribution. Reject unsupported
equipped contributions at their consumption/admission boundary, not by deleting
the item or making a valid carried loot record unsaveable.

Capture requires the actual bound graph at successfully presented Quiet,
pending=0, no Shoot/ranged/time candidate or charge, no contact/combat/End/
retirement, no Event/inventory/receipt/save/presentation lease and no failure
latch. Uncollectable pending treasure is allowed only under the explicit
selected-threat predicate above. A collectable pending queue/money must be
settled before Quiet; F9 cannot force or skip settlement. Refused/stale F9
performs no capture, provider, preflight, file I/O or queued future save.

### Atomic restoration and legacy behavior

Trace every new field through `XeenSaveSnapshot`, format validation/codec,
`XeenSaveState::capture/validateJourneyValues/restoreJourney`,
`XeenPartyState::publishCompleted`/private swaps, `XeenRestoreGuard` construction,
`prepareJourneyPublication`, shared `sameInputs`/state equality, combat's local
party/input preimages, Journey capture and post-publication renewal. Update
copy/move refusal for marked owners and detached-supplement guards; public copy
must not become a route to replace an active graph with byte-equal state.

Restore proceeds entirely on unpublished candidates:

1. Decode the exact envelope/schema, signature and structural fields. Select
   contract from the saved pair, never from camera/map or command-line override.
2. Construct complete saved party/context/purse/queue/supplement candidates,
   saved camera/flags and independent overlays; validate byte/current-state rules.
3. Reload/verify original map-23 DAT/MOB/EVT and the five profiles, reconstruct
   mainland and all 19 actor closures, and apply all saved actor fields/accounting.
   Apply the existing retained provider/internal-identity checks to every load.
4. Validate current contact/activation and treasure delivery eligibility. A save
   purporting to be Quiet with immediately collectable pending treasure is
   malformed; do not fix it by delivering during startup.
5. Preflight the first frame and required sprites under candidate and destination
   guards; prepare final-owner storage/binding and immutable-preimage union.
6. Publish once into fresh final owners, consume the restore-only binding, and
   keep input/capture closed until the matching frame is successfully presented.

No CHR/PTY fallback, initial party preparation, healing, death reclassification,
activation, movement, condition tick, RNG draw, treasure generation/delivery,
event dispatch, combat attachment or End occurs at startup. New resources may
validate an input, never replace a missing saved value. Resume inventory is
closed and projectile/receipt/combat UI is absent. No second RNG or monster HP
initialization overrides restored values.

Preserve exact legacy semantics:

| Format/domain | Required behavior |
| --- | --- |
| v1 ordinary | Existing missing-item-field restoration only; no M33 purse/resistance/treasure inference |
| v2 ordinary | Existing complete arrays and ordinary domain; no conversion |
| v3 completed Diagnostic27 | Existing completed supplement/retirement/revisit semantics; no mutable region |
| v4 1/1 | Exact 1060-byte suffix, legacy seed semantics and Skeleton Journey |
| v4 2/2 | Exact 1366-byte suffix, M30/M31 expedition, Luck/RNG and collection |
| v4 3/3 | Exact 1651-byte suffix, complete M32 region, existing ranged/contact/time support stops |
| v4 4/4 | This consequence-aware regional contract, exact variable length above |

Fresh contract 4 uses the existing prepared active party/levels/HP/SP/loadout
from M30/M32, day 8/year 610/minute 480/ctr24 0, all original 19 actors, the
resource-loaded purse/resistances and empty pending treasure. All 30 initial
supplements are read once; no weapon upgrades or free recovery are introduced.
Explicit nonzero `--combat-seed` and sampled-zero-to-one behavior remain M32.
`--load-game` accepts no fresh seed, preparation or content override. No migration
command or implicit old-save upgrade is part of M33.

Do not serialize pending movement/ranged queues, scan cursors, initiative,
acted/blocked members, selected targets, combat/End flags or capabilities,
volley spent bits, receipt phases/pages, input generations, projectile frames,
deadlines, cache payloads, component masks or actor closures. Durable delayed
treasure is the sole explicit extension to previously empty-reward Quiet saves;
it stores produced consequences, never partially executed modal work.

## Player integration and presentation

Reuse `XeenEncounterFlow`, `XeenJourneyFlow`, `XeenEventFlow`'s presenter helpers,
`XeenGameplay`, `Application::playGameplay`, `PlayerAction` and the single SDL
input/idle loop. F9 interception must share the same presented-generation and
lease guard as every other control. Add F to the existing per-key held/repeat/
timestamp/SDL-batch rejection path. A key sampled before the new frame cannot
Shoot, acknowledge a later receipt, attack a joined replacement or save it.

Required feedback is bounded and factual:

- name/identity and live HP for selected threats/contacts; automatic attachment,
  acting member, target selection and Attack/Block controls;
- F=Shoot while exploration permits it, eligible shooters, no-missile/condition/
  unsupported-equipment refusal, misses/hits/obstruction and source/target identity;
- enemy ranged source/direction, damage targets, Poison/Sleep/Disease changes,
  wake-up, Unconscious/Dead and armor breakage;
- carried gold and pending gold/items; receipt recipient/item/raw bytes and
  explicit generation/capacity/recipient losses; final credited gold;
- readable pending, delayed-treasure, terminal defeat, unsupported-time/content
  and resource-integrity states, without advertising excluded recovery or Run.

Show all relevant condition severities in inventory/party inspection, not just
`worstCondition` (a sleeping poisoned character has both). Current/max HP/SP,
new Speed/AC implications and exact stored items must remain inspectable.
Extend full retained inspection with purse, pending provenance and resistance
inputs so automated comparisons can observe complete durable state.

MON/ATT uses M30's 26-slot terrain/object/actor order and existing attack
identity, order-121 relocation and 100 ms cosmetic service. Preflight all five
images; do not transfer an ATT animation to a newly joined actor in the same
row. An all-party hit may need sequential portrait effects and paginated damage
feedback, but neither presentation order nor duration changes its atomic damage.

Projectiles are disposable typed draw commands in the existing ordered outdoor
stream. Reuse the six active-member lanes, source row and checked original POW
sprites; no physics, persistent missile entity or general spell renderer.
From `OutdoorDrawList`, use row command bases 124/95/76/53 (distance 0/1/2/3)
and six lanes at base+0..5. Their X/Y anchors are:

| Row | Lane 0..5 X | Lane 0..5 Y |
| ---: | --- | --- |
| 0 | 72,72,93,51,97,47 | 43,43,48,48,36,36 |
| 1 | 72,72,85,59,89,55 | 48,48,53,53,41,41 |
| 2 | 72,72,77,67,81,63 | 53,53,58,58,47,47 |
| 3 | 72,72,69,75,73,71 | 58,58,63,63,53,53 |

Odd lanes are horizontally flipped; retain scene clipping and the existing
framebuffer coordinate convention. Player row scales are 0/4/8/12, frames
0/1/2/0, outward; enemy scales are 3/7/11/15, frames 0/2/1/0, inward. The
reference uses `COMBAT_SHOOTING={1,1,2,3}`: a facing-aligned enemy distance
1/2/3 starts in row 0/1/2, respectively. Side/rear attacks have no fabricated
front-facing projectile; their damage and directional feedback still occur.
The first free visual lane represents a queued shot, not its chosen victim.
Cosmetic lane capacity does not cap damage or discard a queued identity.

Published result sequences drive animations. Rendering does not call hit/damage,
award, activation, condition or RNG functions. At recovery, safe recomposition
may reconstruct or finish a retained animation; it cannot recompute the outcome.
Keep the existing ordinary-object/NPC clocks independent. No general audio,
HUD or inventory redesign is required.

## Concrete integration and implementation order

These are internal checkpoints of one M33 implementation task, not separately
accepted releases. Keep contract 3 regressions executable throughout. Implement
new-domain admission only when its consumers and guards are ready; do not expose
a partially supported production contract 4.

| Existing surface | Required change |
| --- | --- |
| `src/games/xeen/XeenCombat.h/.cpp`, `XeenCombatRules.h`, `XeenJourneyProgression.h` | Attach contract 4, borrow world RNG, refresh initiative, distinguish eligibility predicates, enlarge bounded results and factor shared physical/lethal candidate preparation. Add `XeenCombatRules.cpp` for non-inline shared arithmetic; it owns no live state. |
| `XeenActorApproach.h/.cpp`, `XeenRegionalRules.h/.cpp`, `XeenJourneyRules.h/.cpp` | Preserve regional scheduler/order; add opportunity-local ranged queue and complete candidate effects, condition-aware time and contract-specific admission. Preserve contract 3 refusal paths. |
| `XeenParty.h/.cpp`, `XeenCombatInputs.h`, `XeenCharacterRules.cpp`, party/character loaders and `src/formats/xeen/XeenCharacterFormat.cpp` | Optional consequence owner and four resistance inputs; resource initialization; current-condition consumers, equality and marked-owner protection. Keep existing character item/condition bytes and supplement ownership. |
| New `src/games/xeen/XeenMonsterTreasure.h/.cpp` | Pure bounded generation/delivery candidates and typed observations over the party-owned value; no second reward owner or RNG. Share low-level insertion/receipt helpers with `XeenItemRewards` without widening Event authority. |
| `XeenEquipment.cpp`, item transfer, inventory model/catalog/presenter | Complete ordinary weapon consumer and generated armor readiness; reuse legality, exact inspection, transfer and Equip/Remove. Expose all relevant conditions and money. |
| `src/app/XeenJourneyFlow.cpp`, `XeenEncounterFlow`, `XeenEventFlow` presenter helpers and `XeenGameplay` | Shoot operation, ranged/time idle continuation, generalized attachment, mandatory delivery, input/capture exclusion and post-publication recovery. Keep a single combat Flow and regional owner graph. |
| `src/core/PlayerAction.h`, `src/platform/sdl/SdlWindow.cpp`, application gameplay dispatch and outdoor draw-command composition | Typed F input under existing generation guards; bounded projectile/ATT/damage observations and terminal/refusal feedback. |
| `XeenJourneyContent.h`, `XeenJourneyCapture.h`, `XeenSaveSnapshot.h`, `XeenSaveState.cpp`, `src/formats/xeen/XeenSaveFormat.cpp`, `XeenRestoreGuard.h`, `XeenStateEquality.h` | Explicit 4/4 selection; suffix, presence and current-state checks; full preimages/capture/atomic restore; retained immutable-resource union. |
| Existing combat, regional, Journey, equipment, save/SDL/process tests and original-resource witnesses | Independent arithmetic/order oracles, domain and failure regressions, genuine input-driven routes, separate-process comparisons. Register focused new tests in existing CMake test structure during implementation. |

1. **State and pure rules.** Introduce optional values, resource readers, shared
   physical calculations, condition-aware time candidate and treasure candidates.
   Gate: exact profile/tables, all condition/save/draw branches, item reachability,
   capacity and integer boundaries; old-domain presence/equality unchanged.
2. **Contact generalization.** Attach the existing combat object to contract 4,
   generalized targets/results, current initiative and shared lethal production.
   Gate: mixed grouped/joining identities, wound preservation, owed movement,
   all-party attacks, all-sleep automatic cycles, genuine End/defeat, inherited
   Skeleton/Zombie arithmetic and M30 selection/movement ordering.
3. **Exploration consequences.** Add regional ranged candidate work and Shoot,
   time/RNG continuation and unified mandatory delivery. Gate: exact scans,
   rays, volleys, no-contact deaths, prior-candidate effects, delayed collection,
   no false Quiet and pre/post-publication failure behavior.
4. **Persistence and mutable closure.** Complete 4/4 codec, guards, restored
   ownership and item-consumer closure. Gate: literal wire fixtures, malformed
   and missing-field rejection, legacy byte semantics, wound/pending-reward/
   condition round trips and a separate-process continued-mutation witness.
5. **Production presentation and acceptance.** Wire F and retained projectile/
   receipt/condition observations, execute the original-resource journeys below,
   then build and run the complete CTest suite. Gate: independent review and
   maintainer physical native-SDL acceptance, with every mandatory feature
   covered. No checkpoint is permission to claim M33 completion on its own.

## Acceptance contract

All items in this section are **future implementation acceptance**, not tests
claimed to have run during this investigation. Arithmetic probes below establish
expected values only. A successful fixture cannot replace a production route.

### Comparison and replay discipline

Every input-driven witness uses the external original installation, fresh
`--journey-region --combat-seed <seed>` or unmodified `--load-game <save>`, and
the existing prepared party/loadout. Test output/save/log locations are outside
the source tree and commercial installation. No edited actor/party/context,
injected draw tape, regeneration, free healing or special test entry is allowed
in these gameplay witnesses. Artificial inputs belong only to named rule/fault
tests. CLI harnesses must drive the same typed action/idle/Flow boundaries as
SDL, never call the lethal or delivery publisher as a shortcut.

Use explicit action/idle transcripts: after each exploration action service
one pulse at a time through the owed three ticks and any mandatory work, then
act only on a successfully presented generation. Zero-time turns still use the
accepted ctr24 and classification rules. Do not add speculative Wait inputs to
advance animation. In combat service every automatic step; Space attacks the
lowest original-index live contact unless a witness specifies another target;
acknowledge genuine End and all receipt pages. No Run or recovery is available.

At every named checkpoint record and compare the complete semantic snapshot:
all 30 characters and supplements (including inactive owners), active order,
exact four-category item arrays/holes/metadata, HP/SP/conditions/XP, purse and
pending source-tagged treasure, camera/flags/quests/overlays, context, all 19
actor identities/HP/positions/activation/lifecycle/status/accounting, and RNG
algorithm/state/raw count. Compare selected/owed runtime work separately where
it exists; do not serialize it. Save files must additionally round-trip exactly.
Compare an uninterrupted control to a fresh-process resume at each save, then
apply the same further actions to both and compare again. Visible HP or a file
hash alone is not sufficient evidence of restored owner authority.

### Fixed connected wound, contact, equipment and restart witness

Seed **3**, default entry `(9,11)` West, minute 480, ctr24 0, original prepared
loadout (Badger alone has equipped Bow ID 30). Use Up for Forward; **F here
always means Shoot**, unlike the single-letter Forward notation in old probes.

1. Up, then service all three pulses: camera `(8,11)` West, minute 490,
   ctr24 1, Orc identity 9 at `(6,11)`, HP25. This approach prefix is already
   covered by the M32 resource oracle; no random draw is owed by the move.
2. F and finish the new opportunity. Badger rolls `2,2,2`, hit d20=14,
   monster save=4 on `[1,56]`: 18 damage is halved to 9, so Orc 9 has HP16.
   It moves to `(7,11)` and fires: random target=1 (Tyro), d20=4 and
   parameter=2 miss. At presented Quiet: minute 500, ctr24 1, unchanged party
   HP, no defeated/accounted actor or treasure; RNG state `0x4a767d04`, count8.
3. F9, close the process, load this save in a new process. Inspect the same
   HP16 actor outside contact and all exact values above. This is the required
   surviving wound save, not a Run-produced survivor.
4. Up enters `(7,11)`; the next classification attaches Orc 9 with HP16 and
   carries one owed movement. Orc acts before the party: target=1, d20=16,
   parameter=4, damage die=5. Tyro becomes HP43. The attachment obligation
   completes before Arturius's ready frame. Space: Arturius's dice `2,1,2,2`,
   hit d20=8 produce 24 physical damage and kill identity 9 once.
5. Orc drop=8; category=48, subcategory=50, armor ID=2, ignored enchantment=46.
   The item is ordinary Armor `0/2/0/0`; delivery chooses Arturius. Complete
   End and receipt acknowledgment. At Quiet: minute511, ctr24 2; party HP
   `[36,43,36,40,21,15]`; all six XP increased by66; gold810, gems10; empty
   pending treasure; actor9 canonical defeated/accounted. RNG state
   `0x692b3851`, count22. No other actor was killed to make collection possible.
6. Inspect the delivered item; transfer it from Arturius to Tyro using I,
   category/slot selection, T, recipient selection and Enter. Remove Tyro's
   original equipped Armor ID2, then equip the received record with E. Distinguish
   the records by their transfer/slot history and complete arrays, not name.
   Class/frame legality succeeds; HP/XP/money/time/RNG are unchanged.
7. Save, restart again and compare the whole state. Remove/re-equip the received
   armor, turn East and Shoot into empty center rows, then finish its charge and
   pulses. This additional legal mutation must succeed with no duplicated XP,
   gold or item and no revived Orc9. Save and compare against the uninterrupted
   branch again. Further encounters use the same RNG continuation and owners.

The consecutive accepted draw intervals/values through step5 are:

```text
[1,2]:2, [1,2]:2, [1,2]:2, [1,20]:14, [1,56]:4,
[0,5]:1, [1,20]:4, [1,5]:2,
[0,5]:1, [1,20]:16, [1,5]:4, [1,10]:5,
[1,2]:2, [1,2]:1, [1,2]:2, [1,2]:2, [1,20]:8,
[1,100]:8, [0,100]:48, [0,100]:50, [1,7]:2, [1,100]:46
```

A separate branch loads step3 and presses F instead of Up. Its next dice are
`2,2,2`, d20=15, monster save=20, giving18 damage and a ranged kill. Drop=59
produces no item. After the receipt and owed charge/pulses: camera `(8,11)`
West, minute510, ctr24 1, original party HP, gold810/gems10, XP+66 each,
actor9 defeated, empty pending treasure, RNG `0xb8d3b48a`, count14. There was
no combat attachment, End or retirement. Save/restart, turn and Shoot again;
the death, gold and XP must remain once-only. This independently closes the
out-of-contact kill path.

### Mainland conditions, grouped combat and time journeys

The following coordinate routes are resource-checked mainland paths. They do
not promise that an actor remains at its spawn until the party arrives: the
normal scheduler may attach it earlier. Face the next coordinate using the
shortest turn sequence (Right for a 180-degree tie), then Up; drain work after
each action. Contacts preempt routing and use the combat policy above. Resume
the route from the unchanged party cell after retirement. A sign may open its
accepted modal; acknowledge it without bypassing the M31 Event path.

Use these exact route families, each from a fresh prepared entry:

```text
Common southern corridor C:
(9,11),(9,10),(9,9),(9,8),(8,8),(7,8),(7,7),
(6,7),(5,7),(5,6),(5,5)

Snake route: C,(5,4),(5,3),(5,2),(5,1),(5,0)
Toad route:  C,(6,5),(7,5),(7,4),(7,3),(6,3),(6,2),(6,1)
Undead route: C,(5,4),(5,3),(5,2),(4,2),(3,2),(2,2),(1,2),(1,1)
```

For repeatable coverage without claiming an unexecuted seed already wins these
new fights, the original-resource replay test deterministically enumerates seeds
1..256 in ascending order for each family, using exactly this policy and no
mid-run reload. Each run stops at terminal defeat, unsupported boundary, or
completion. Retain the first successful transcript satisfying each predicate
below; rerun that exact seed/transcript in a separate process. This bounded
enumeration is an acceptance-fixture selection algorithm, not permission to
change formulas, loadout, routes or arrange state. If it finds no witness for a
required predicate, acceptance fails; injected arrangements cannot fill the gap.

- Snake: naturally attach identity12, observe at least one failed save producing
  Poison and changed derived values, win, retire with a living poisoned member,
  save/restart, and continue navigation/equipment mutation without cure/reset.
- Toad: naturally attach identity14 or15, observe party-wide targeting and Sleep,
  skipped sleeping turns and at least one damage wake (including immediate
  reapplication when its save fails). Win and retire; save/restart exact surviving
  conditions and continue. Keep a distinct terminal-defeat transcript as a
  negative control, not a substitute for the victory witness.
- Mixed continuity: retain a route transcript with a second contact joining an
  existing episode, or grouped contacts, original identity selection/compaction
  and successful return. Independently require both regional Skeleton17 and
  Zombie18 combat and Zombie Disease, as well as unchanged M30 expedition tests.
- At least one successful family transcript must include two completed episodes
  with a save/restart between them and a later published attack/item mutation.
  If its first victim carries pending rewards delayed by a selected survivor,
  preserve that queue across the save and verify later collection once.

For the 960 tick, take the successful Snake transcript with persistent Poison
and, after its required victory, follow the same route backwards to `(9,11)`.
At each quiet boundary use Wait until the next ten-minute charge crosses 960;
combat may preempt these waits and is completed normally. Exact minutes may be
`951..959 -> 961..969` because End/Round costs one minute. Save before and after
the crossing, fresh-process resume each, compare full condition/time/RNG state
and continue one further charged action. No clock assignment or removal of
actors is allowed. A separate pure-rule test covers exactly `950 -> 960` and
Round/End `959 -> 960`. Defeat/dusk before the crossing is a failed candidate
transcript; the same bounded seed enumeration must retain a successful one.

Level-1 loot coverage also runs the fixed first-Orc route for seeds1..4096 with
the same actions and contact policy, retaining the first actual delivered
weapon, armor and missile weapon (IDs30..33). The seed3 armor result is the
fixed primary witness. Inspect and transfer each retained generated weapon to
Arturius, remove conflicting melee/shield/missile equipment as needed, equip
legally, use melee or Shoot as appropriate, then save/restart and use it again.
No item injection supplies this production consumer witness. Exact full catalog
coverage and rare generation/capacity branches are deterministic rule tests.

### Deterministic rules, authority and negative controls

Use literal independent draw tapes for pure/candidate tests, in addition to
seeded world-RNG tests. Assert every requested interval, accepted value, rejected
raw conversion, resulting count and operation boundary. At minimum:

| Area | Required controls and expected consequences |
| --- | --- |
| Monster profiles and target order | All five exact records; hates1 random rather than Paladin bias; Cleric preference including Sleep; fallback draw even with one target; hates16 all six including Dead, no target-selection draw or mid-party early exit; Zombie two distinct resource attacks. |
| Physical attack | Natural1 stops before parameter/damage; natural20 first damage then parameter and possible second damage; Poison/armor changes from first application affect second threshold; asleep skips hit draws and wakes before damage/special; zero damage has no special-save draw. |
| Conditions | Save equality succeeds; failed positive hit increments Poison/Disease or sets Sleep; checked255 refusal; Poison modifies exactly three stats, Disease the other three; current HP/SP never clamped; initiative refresh/zero Speed; all-sleep automatic service versus actual terminal defeat. |
| Block across inner reset | Deterministic tape: Block -> Sleep -> all-asleep inner reset -> Block remains active -> wake -> subsequent enemy attack, before an ordinary round transition, still uses `currentAC + floor(currentLevel/2) + 15`. Choose a noncritical hit total at least `currentAC+10` but below that blocked threshold: it must miss. A separate ordinary round transition clears acted and blocked state; the same awake-target attack then uses `currentAC+10` and hits. |
| Time | Clean six-member tick with Poison branch2 and Disease branch0 consumes exactly12 accepted draws; each branch1 adds its correct resistance interval, including `[1,40]` at zero resistance. Nonzero Poison/Disease skip their respective branch draws. New stat death becomes Dead2 with HP unchanged; existing Dead increments; Dead255 refuses atomically; unchanged Sleep. No draws for zero-time operations. |
| Shoot | All four missile dice sets, class divisors and Accuracy; dice before exploding hit; no Might or melee count; each miss continues to the next original-ordered target/row; hit spends even zero damage; physical resistance then monster save; lethal drop before next shooter; multiple shooters, empty eligibility, empty ray, all misses, blocked rows, edge and middle15 contrast with enemy ray. |
| Ranged scheduling | All four directions, distances1..3, facing independent, raw east mask asymmetry, disconnected actor closures, failed-ray tested-once, full scan/pass ordering, earlier actors changing later occupancy/contact eligibility; shot plus move-to-contact; two opportunities in a charged Wait; no input/capture gap before queued damage. |
| Lethal production | XP eligibility/division/levels, u32 XP/purse overflow, nonlethal HP persistence, ranged/contact shared identity, repeated observation and End do not regenerate; each drop boundary10/11, category40/41/85/86, subcategory30/31/60/61/85/86, exact ignored draws and six-draw miscellaneous-generation loss. |
| Delivery | Ten slots per category, eleventh retained-production loss, full generated draws despite overflow, all four tails globally full, category-only full, disabled recipients, first eligible recipient, holes with full tail, sequential compaction and exact bytes; warnings do not lose gold; item delivery before receipt and gold only at final acknowledgment. |
| Equipment closure | Every generated ID through inspection/transfer/legal equip, all class masks and conflicts; each weapon dice table entry; original material exceptions; bad-state base damage and armor suppression; unknown carried item remains storable while unsupported consumed record refuses explicitly. |
| Publication and failure | Stale input, changed/reverted preimage, owner ABA, reentrant provider/presenter, callback throwing before preparation and after each publication; no partial party-wide hit/opportunity/tick/delivery; committed predecessor retained; no reroll/recredit on retry or receipt page replay; bounded raw-rejection/exploding-die service. |
| Save exclusion | Every pending pulse/ranged queue/Shoot attempt/charge, contact/attachment, player turn, all-sleep automatic work, victory awaiting End, retirement, eligible reward, receipt, Event/inventory, presentation and failure latch refuses F9 before provider or file calls. Delayed uncollectable treasure alone is expressly saveable. |
| Persistence | Literal 4/4 offsets and minimum/maximum lengths; all truncations, extra bytes, count/source/order/mask mismatches, unknown pair, missing resistances/purse, noncanonical actors, bad condition/time/HP correlations, overflow, immediately collectable pending queue and resource identity changes reject atomically. Old optional absence stays absent. |
| Resource authority | Existing M32 wrong-key/internal-ID, mutable known DAT/MOB after cache clear and mutation/reversion controls across Shoot/combat/reward/inventory/capture/restore; EVT and profile/POW/MON/ATT compatibility; known mismatch permanently latches, compatible I/O retry remains distinct. |

Some rare controls (ten-item category overflow, saturated purse/conditions,
all-sleep saves, pending-treasure saves and mixed candidate arrangements) may
use expressly labeled artificial fixtures. They are additional correctness
evidence, never proof that those arrangements were reached by the genuine
route suite. The pending-treasure restore control keeps another selected living
actor, saves at the resulting legal Quiet boundary, restarts and turns away
until selection is empty; verify mandatory delivery and once-only credit. Its
fixture provenance must remain explicit. The fixed seed3 connected and ranged
branches supply genuine production death/reward/save evidence independently.

Legacy regression gates retain all M27-M32 tests, particularly M30 grouped
attachment/selection/owed movement and exact combat arithmetic, M31 sign/Event
owner and post-publication authority, M32 312-case original regional oracle,
schema3 restore/closures and immutable cache authority. Add explicit old 3/3
load controls reaching its original contact/ranged/time support stops; fresh
entry4 does not retroactively alter their expected outcomes. Run unchanged
v1/v2/v3 and v4 1/1,2/2,3/3 fixtures through their established entry/restore
policies, including malformed files and forbidden command-line combinations.

### Acceptance responsibilities

Automated acceptance covers rule tapes, arithmetic, state/authority/fault tests,
wire/legacy tests and complete CTest. Original-resource acceptance separately
covers immutable manifests, input-driven journeys and fresh-process durable
comparisons. Record actual seeds/transcripts, commands and results in the task
report; do not turn this plan into an operational log.

An independent reviewer must check the reference derivation, all mandatory
features, complete consequence/save chain and implementation/tests. The
maintainer must physically execute native SDL acceptance: the fixed seed3 wound
and both restart branches, at least one retained Poison/Sleep route, generated
equipment use, enemy/player projectiles and reward pages, sign regression and
terminal/blocked-save controls. Record which work is physical versus automated
or image-based. Screenshots or a headless driver do not claim physical acceptance.

M33 closes only after these gates pass. Then update durable status/history/
roadmap and condense this plan under AGENTS.md; that closure work and Git actions
require their applicable authorization. This candidate changes none of them.

## Non-goals and successor handoff

M34 owns Run/disengagement, failed/run-outcome policy and non-victory retirement.
It must consume the same wound/accounting/treasure owners and preserve pending
obligations; M33 grants retirement only for genuine successful End. Do not add
Run now to produce saves or escape unwinnable acceptance arrangements.

M35 owns connected Myra -> Phirna -> Myra authority and selected recovery/item
use. It receives exact Poison/Sleep/Disease/injury bytes, resource-derived purse,
ordinary inventory and continuous time/RNG. It must not reconstruct consequences
from old encounter UI. The miscellaneous-drop reference loss is not authorization
for a wand/item-use subsystem or an event-reward rewrite.

General magic, Rest, manual recovery, training/services, shops/banking, gem
economy, ammunition, arbitrary treasure levels/materials/special weapons,
movement abilities, additional maps/scripts, daily/midnight clock processing,
save-anywhere combat snapshots and UI redesign remain excluded. Conditions may
therefore persist without an M33 cure. Defeat and the explicit temporal boundary
remain visible terminal/support limits, not implicit healing opportunities.

## Investigation performed and limits

The baseline and configured ScummVM source were verified at the exact commits
identified above; the evidence is tied to those revisions.

Investigation read the current owners/Flows/rules/codec/guards and relevant tests,
the current status/roadmap/M32 contract, and the inherited contracts selectively.
Read-only disposable probes decoded the external monster records, required
MON/ATT/POW frames, initial nested PTY purse and CHR resistances, and checked the
listed mainland coordinate paths. No commercial bytes were written or copied
into the repository. The existing `mmodern_regional_original` diagnostic passed
its 312 fresh/restored cases, resource/closure/scheduler comparisons and resource
authority fault controls. A scalar xorshift/arithmetic probe checked the fixed
seed3 draw tape and RNG checkpoints; it was not an M33 Flow execution.

No M33 code exists or was run as part of planning. No build/full CTest, native
SDL physical acceptance, new-route combat execution, DOS executable comparison
or independent implementation review is claimed. In particular, condition/loot
route selection above remains a required future acceptance run; reference quirks
are attributed to the verified pinned implementation, not asserted as independently
observed original executable behavior. This is an uncommitted candidate contract
for review, not a milestone completion record.
