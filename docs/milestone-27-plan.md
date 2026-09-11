# Milestone 27 - Playable Attack/Block encounter with original outcomes

**Milestone 27 is complete.** Stages 27A, 27B and 27C are accepted. This
closed plan records the admitted domain, durable technical contracts,
persistence boundary and final acceptance.

## Objective and reference authority

Continue the accepted M26 encounter into a real melee Attack/Block fight: a named
acting character and target, misses and damage, mandatory enemy turns, injury,
incapacitation/death, and victory or party defeat. Victory removes the selected
original monster and awards applicable XP exactly once within the session.

The accepted foundation is the stable project status, closed M26 actor/approach
contract, M24 transfer and M25 equipment contracts, and pinned ScummVM revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` recorded in
[dependencies](dependencies.md). The implementation and
literal tests are authoritative for the admitted behavior below. Reference code
and original-resource observations define the bounded compatibility target; no
full ScummVM execution or commercial payload is part of this repository.

### Pinned source map

All paths and symbols in the reference columns below refer to this revision:

| Source | Authority |
| --- | --- |
| [interface.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp), `doCombat`, `nextChar`, engagement callers, `draw3d` | Setup, action dispatch, participant advancement, round/end time and exit ordering |
| [combat.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp), `setupCombatParty`, `setSpeedTable`, `allHaveGone`, `charsCantAct`, `attack`, `hitMonster`, `getWeaponDamage`, `getMonsterDamage`, `attack2`, `getMonsterResistance`, `block`, both `doMonsterTurn` overloads, `doCharDamage`, `giveExperience`, `endAttack` | Connected melee, enemy, damage, removal and reward rules |
| [character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp), `synchronize`, `getStat`, `statBonus`, `getAge`, `conditionMod`, `getArmorClass`, `itemScan`, `getMaxHP`, `subtractHitPoints`, condition predicates; [character.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.h) | Original input widths, effective statistics, injury and armor breakage |
| [party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp), `changeTime`, `addTime`, `checkPartyDead`, `giveTreasure` | Time effects and defeat; empty treasure produces no reward system requirement |
| [item.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.h), `ItemState`, `isBad`, `isEquipped`; [item.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp), attribute category and equipment methods | State bits and scans; M24/M25 continue owning transfer/equip legality |
| [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp), `STAT_VALUES/BONUSES`, `AGE_RANGES/ADJUST`, `ATTRIBUTE_CATEGORIES/BONUSES`, `WEAPON_DAMAGE_BASE/MULTIPLIER`, `METAL_DAMAGE/PERCENT/LAC`, `ARMOR_STRENGTHS`, outdoor tables | Literal numeric tables; preserve ScummVM GPL attribution when adapting them |
| [map.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp), `MonsterStruct::synchronize`, `MazeMonster`, MOB load; [interface_scene.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface_scene.cpp), `setOutdoorsMonsters`, `drawOutdoorsScene`, `animate3d` | Statistics, removal coordinates, attack frames and draw-slot relocation |
| [xeen.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/xeen.cpp), `getRandomNumber`, extended-option loading | Inclusive random bounds; DurableArmor defaults false |

## Admitted domain and exclusions

Retain Clouds map 20, original monster record 5, initially `(13,2)`, resolved type
8/image 8, party entry `(13,1)` North, and the four cells `x=13..14,y=1..2`.
Retain all 27 original monster records and M26's isolation checks. Combat does
not freeze, delete or synthesize bystanders to obtain isolation. No movement or
rotation during combat means the accepted scan cannot recruit another actor.

Use the explicit **World of Xeen, Clouds side, Adventurer** rules profile. Use
default reference options, particularly DurableArmor=false, invincibility=false,
superStrength=false. Archive availability does not select a different profile.
Active roster owners remain exactly `[0,18,14,11,1,6]`, uniquely and in that order.

Initial admission requires the original Good party and its original items, with
all successful existing M24 transfer and M25 equipment rearrangements among these
six owners admitted. Combat subsequently admits exactly the physical injuries
and equipped-armor breakage reachable here. It does not reapply the all-Good
startup predicate after injury. Equipment preparation is a disclosed diagnostic
entry adaptation; no claim is made that original exploration suspends monsters.

Excluded: Run, rotation/disengagement, Shoot/ranged attacks, spells, consumables,
combat inventory/equipment mutation, shops, repair, identification, rest/healing/
resurrection, recruitment, broader navigation, indoor combat, general calendar,
random loot, M28 serialization, normal-start travel and other monsters/maps.
Opaque ordinary items, aliases, legacy loadouts and unsupported conditions retain
their existing storage/inspection behavior outside this diagnostic. They are not
silently repaired into combat eligibility.

No contradiction requiring a different roadmap anchor was found. Three source
quirks matter: bare attacks can produce zero damage after resistance; broken or
cursed weapons still contribute their base dice in the pinned melee scan; enemy
natural 20 can execute two separately rolled damage applications. None authorizes
ignoring a supported item bonus or making the Skeleton harmless.

## Accepted integration seams and entry

M27 extends these existing seams without creating parallel owners:

| Existing seam | Accepted M27 use |
| --- | --- |
| `XeenActorApproach`, `XeenEncounterState`, `XeenWorld::sessionState` | Preserve one-time initialization, original identity, live HP/position and revision authorization; accepted approach remains intact |
| `XeenPartyState::roster`, `XeenRoster::at`, `XeenCharacter` | Keep HP/SP/conditions/items in their current owners; add only missing encounter inputs to this roster |
| `XeenCharacterRules`, `XeenEquipment`, `XeenItemTransfer` | Extend existing effective-stat calculations; reuse transfer, frame changes and selection safety |
| `XeenEncounterFlow`, `XeenEventFlow`, `XeenGameplayServices`, `Application::playGameplay` | Extend typed entry and phase routing, retained tickets, feedback and one SDL loop |
| `XeenOutdoorScene`, `CloudsMapComposer`, `XeenAssetSource`, bridge/sprite cache | Extend typed actor appearance; no renderer-owned combat mutation |
| `XeenSaveState`, irreversible world encounter marker | Extend refusal guards for new roster-only encounter inputs; preserve ordinary v1/v2 behavior |

`XeenSessionWorldState` is declared in `XeenWorld.h`; the character/context/monster
formats and sprite safety validator are under `src/formats/xeen`. A new parallel
world, inventory, event engine, ECS or general combat framework is unnecessary.

### Production entry and preparation

Production syntax:

```text
mmodern --encounter-27 [--combat-seed <nonzero-u32>] <game-directory>
```

Add `XeenEncounterEntry::Diagnostic27`. Preserve `--encounter-26` and
`Diagnostic26` unchanged, including its terminal notice and inventory refusal.
Move the shared entry-kind declaration to a small domain header if world
authorization needs it; the domain must not include an Application/Flow header.
Marking is idempotent for the same kind and cannot overwrite or downgrade a
previously marked kind.
Reject duplicate options, zero/out-of-range/malformed seeds, extra paths, camera
overrides and combinations with `--load-game`, `--save-file`, `--render-map` or
another entry mode before opening those paths or creating gameplay owners.

Diagnostic27 starts in **Preparation**, already irreversibly unsaveable. Mark
the world with the typed entry kind before any fallible startup work. Load the
ordinary initial party, flags and missing combat inputs from the initial CHR;
validate resources/profile and preflight sprites. No live actor list, activation,
approach countdown or gameplay deadline exists yet. Read-only scenery may use
the fixed camera; the preparation notice explicitly says actors have not begun.

I opens the existing inventory panel. M27 reuses M24/M25 inventory mutation,
selection-safety and confirmation semantics, with a Diagnostic27-specific
cancellation key adaptation because Escape exits the whole diagnostic session.
F1-F6, categories/slots, T/confirmation, equipment E and successful transfer remain
the existing M24/M25 operations.

**Diagnostic27 Preparation inventory cancellation:** Escape is always the
top-level exit for the entire diagnostic session, including while preparation
inventory is open; do not route Escape into the inventory state machine.
In Browse, I closes inventory using the existing close behavior. In
ChooseDestination or Confirm, I cancels the current transfer selection or
confirmation and returns to Browse, preserving the owner/item mutation semantics
of existing Escape cancellation. That first I does not also close inventory;
a subsequent I from Browse closes it. N retains existing Confirm cancellation.
Cancellation invalidates/disarms the existing transfer confirmation and transient
selection certificates exactly as the accepted inventory state machine requires.
It publishes no transfer, equipment mutation, gameplay time, actor initialization
or encounter action. I creates no queued Begin action. ChooseDestination/Confirm
feedback must identify I as the preparation cancel control and must not advertise
`Escape to cancel`; Confirm may also advertise N where appropriate. Outside
Diagnostic27 Preparation, existing inventory/Escape/I behavior is unchanged.

Enter can mean **Begin encounter** only at the closed Preparation screen; Enter
in Confirm remains transfer confirmation and cannot also Begin. Beginning
consumes the preparation generation, closes/invalidates every
inventory/equipment/transfer certificate, then revalidates membership, immutable
initial character inputs, exact item conservation and legal resulting loadout.
All supported rearrangements remain
available; there is no synthetic-loadout-only exception.

The multiset of occupied `(category,material,id,state)` records across these six
owners must equal the original multiset, including duplicates. Inactive owners
and non-item state must equal their loaded baseline. Frames may only be original
frames or results of supported equip/remove/transfer; validate subtype, class
restrictions and conflicts for equipped records against the resulting arrangement,
without modifying it. Carrying an unequipped weapon that its recipient cannot
equip remains admitted. Fresh-session routing and existing operations establish reachability; no
arbitrary v1/v2 save enters this phase. Empty slots follow M24's explicit
compaction rules, not an extra normalization pass.

Only successful Begin calls the existing one-time approach initialization at
480/ctr24=0. It then uses the accepted action/pulse path. A failed Begin publishes
a support/failure stop; it does not create an ordinary interactive fallback.
No preparation can be reopened once approach begins.

### One-time combat handoff

Add a private, typed `beginCombat` operation over the same world, party, camera
and encounter coordinator. Its authority is all of:

1. World marked **Diagnostic27**, initialized exactly once, no previous combat
   handoff/outcome; identical borrowed owner addresses.
2. The retained approach state/revision is still authoritative and exactly
   `Engaged`, reason None, pending 0; Flow ticket/generation is current and has
   no presentation failure. The selected view resolves record 5 in slot 0, at
   the party cell, Present/Physical with positive live HP.
3. Current camera/context, original actor metadata, isolation and admitted party
   match the captured inputs. No pending event, reward or inventory certificate.

Prepare the six roster references, initial speed table, selected target, fixed
result and presentation inputs; recheck authority after every provider callback.
Publish combat-entered plus a new shared world encounter revision and a new Flow
generation in a nonthrowing segment. Retire approach deadlines and callbacks.
Keep `_encounterTerminal` latched: it means the approach has ended, and is never
cleared to resume it. Add the minimal private combat-entered/outcome facts in
`XeenSessionWorldState`; combat operations, not approach operations, advance the
same revision from this point. The old approach state retains its old revision
and can never adopt combat revisions. `authoritative`, tickets, composition and
Application handoff must dispatch by the active phase/owner, rather than pretending
the old approach state is still current.

This permits exactly one M27 handoff, not a generic terminal reset. Ordinary
Diagnostic26 Engaged, SupportStopped, failed presentation and stale tickets
cannot enter combat. Constructing a fresh Flow cannot reinitialize actors or
clear a consumed handoff. No reset-world, restore-HP, clear-terminal or resume
operation is added.

Actual approach facts carry through unchanged: fresh Wait or Forward engages at
490; accepted delayed East approach engages at 500. No combat entry charge and
no reset to 480. Admission remains split into initial Good/loadout validation,
unchanged M26 approach validation, and combat reachable-state validation.

## Resource observations and required storage

R confirms the M26 archive chain: initial `maze0020.mob/.dat`,
`DARK.CC/xeen.mon`, `XEEN.CC/008.mon`, and now `008.att`. The existing M26 hashes
for PTY and monster statistics match. Additional decoded-member SHA-256 anchors:

```text
maze.chr 10620 bytes c1bb681d2a9c5b3b29b2b75ad314328debfd6f512d942d699c4891a5f6da227a
008.att  22004 bytes 1d0a8cdcfeb4239cebd4d562523f9490931897fa46b499f1b67179ed918acc3d
```

These are reproduction aids, not new save fingerprints or a hardcoded entity.
Use checked resource parsing and explicit supported-domain checks. Keep every
original record and unneeded byte opaque; never add combat enum validation to
ordinary storage merely because this encounter requires it.

### Field-to-owner contract

Offsets are zero-based within one CHR (354 bytes), MON (60 bytes), or PTY record.
`new roster inputs` below means an optional supplemental payload **inside
XeenRoster**, indexed by original owner, containing only fields not already
represented. It contains no copied HP/SP, items, conditions, level or membership.
This avoids both a second combat character and silently extending always-live
ordinary character state omitted by v2. Install it only for Diagnostic27 after
marking encounter authority; it cannot be cleared during that owner's lifetime.
Expose const owner-indexed reads and private combat XP publication.
Only the six active owners receive these supplemental inputs; inactive owners
and every ordinary-session roster have none. Existing character payloads remain
unchanged in all thirty slots.

| Field | Original offset/type | Storage / owner | Validation and use | Persistence classification |
| --- | --- | --- | --- | --- |
| Might permanent/temporary | CHR 20/21, unsigned bytes widened to int | New roster inputs | Exact initial values; effective Might/damage | Reconstructible initial input; M28 must preserve if mutable later |
| Speed permanent/temporary | CHR 28/29, unsigned bytes widened to int | New roster inputs | Exact initial values; initiative and AC, including ring | Same |
| Accuracy permanent/temporary | CHR 30/31, unsigned bytes widened to int | New roster inputs | Exact initial values; melee hit bonus | Same |
| Temporary AC | CHR 34, unsigned byte widened to int | New roster inputs | Initially 0, immutable here; AC | Same |
| XP | CHR 348..351, uint32 LE | New roster inputs, mutable XP per original owner | Read original value, initially 0; once-only addition | New authoritative outcome, protected by unsaveable session |
| INT/PER/END, level/temp level, temp age, birth year, max-stat skills | Existing CHR offsets 22..27,35/36,38,346/347 and skills | Existing `XeenCharacter` / rules | Reuse, no duplicated combat copy; original level 1, age 18 | Existing v2 values |
| Current HP/SP, conditions, all four item arrays | CHR 342/344 i16 LE; 323..338 bytes; 166/202/238/274 four-byte records | Existing roster characters | Preserve exact values; injury updates HP/conditions/armor only | Existing v2 categories, but encounter remains unsaveable |
| Base XP / HP | MON 16..19 u32 LE /20..21 u16 LE | Existing `XeenMonsterRecord::raw` + typed accessors; live HP remains actor i32 | Anchor 250/20; positive HP, safe XP-to-intermediate conversion | Immutable resource inputs / live HP is world state |
| AC / speed / number of attacks / preferred class | MON 22/23/24/25 u8 | Same immutable record | Anchor 5/10/1/3 (Cleric); validate before use | Reconstructible |
| Strikes / damage die / attack type | MON 26..27 u16 LE /28 u8 /29 u8 | Same | 2 dice, 6 sides, Physical=0; distinct from one attack | Reconstructible |
| Special / hit parameter / ranged / category | MON 30/31/32/33 u8 | Same | 0/4/0/4 (Undead); hit parameter is not a percent | Reconstructible |
| Fire/electric/cold/poison/energy/magic/physical resistances | MON 34..40 u8, percent | Same | 50/50/50/50/0/0/50; bounded 0..100 for interpreted fields; only physical participates here | Reconstructible; byte 41 remains opaque |
| Gold / gems / item drop | MON 42..43 u16 LE /44/45 u8 | Same | All zero; reject unsupported loot before play | Reconstructible; no treasure owner added |
| Flying/image/loop/effect | MON 46..49 u8 | Existing accessors | 0/8/0/0, retain approach safety | Reconstructible |
| Difficulty/profile/day/year/minutes/ctr24/effects | Existing PTY 27,610/612/614/616 and effect prefixes | Existing party `encounterContext` | World/Adventurer/day1/year610, admitted window; effects/light/resistance zero | Existing encounter-only authority, absent from v2 |
| Initiative, acted/blocked, target, RNG, pending work, frames | No new original payload | Transient coordinator; actor identity references only | Valid phase/revision and bounded indexes | Not a new save format; M28 must choose future transient eligibility separately |

No new Luck or resistance model is needed: physical attacks with special=0 do
not reach saving throws. No additional skill affects these melee/AC paths;
existing Bodybuilder and other max-stat rules remain authoritative. All temporary
attributes, temporary AC/level/age, condition bytes and party buffs are zero at
admission. Reachable Unconscious/Dead do not change attribute arithmetic through
`conditionMod`. Birthdays do not affect this bounded age calculation.

R, in active order (all age 18, level 1, initial XP 0):

| Owner / name / class | Might | Accuracy | Speed | HP=maximum / SP | Original AC |
| --- | ---: | ---: | ---: | --- | ---: |
| 0 Arturius / Paladin | 17 | 15 | 16 | 12 / 2 | 13 |
| 18 Tyro / Knight | 19 | 16 | 16 | 16 / 0 | 10 |
| 14 Badger / Ranger | 15 | 12 | 15 | 12 / 2 | 8 |
| 11 Zippo / Robber | 14 | 18 | 15 | 10 / 0 | 8 |
| 1 Rebecca / Cleric | 12 | 13 | 14 | 7 / 7 | 6 |
| 6 Seymour / Sorcerer | 8 | 15 | 14 | 5 / 9 | 4 |

Names come from the resource; this table is evidence, not a production string
table. The generic parser must not fabricate XP zero: it is an observation of
this party, and malformed/truncated combat-input reads fail admission.

### Complete bounded inventory-effect coverage

All original occupied state bytes are zero. Coordinates below use physical
slots from the initial load; transfer may change owner/slot. All unlisted slots
are empty. Record notation is `(material,id,state,frame)`.

| Category / original holders | Exact records or distinct kind | Required effect for any admitted rearrangement |
| --- | --- | --- |
| Melee, owners 0/18/14/11/1/6 | `(0,6,0,1)`, `(0,2,0,1)`, `(0,8,0,1)`, `(0,12,0,1)`, `(0,15,0,1)`, `(0,7,0,1)` | Dice respectively 4d2,2d3,2d3,2d2,1d6,1d3; no metal, elemental or effectiveness bonus |
| Extra weapon, owner 11 slot 1 | `(0,12,0,0)` | Same 2d2 if legally equipped; unequipped contributes nothing |
| Bow, owner 14 slot 1 | `(0,30,0,4)` | Excluded by melee frame scan; no ranged input. Its material/counter have no separate bonus here |
| Body armor | Owner 0 `(0,3,0,3)`; owners 18,14,11,1 `(0,2,0,3)`; owner 6 `(0,1,0,3)` | Good equipped AC +5/+4/+2 respectively; zero when removed or broken |
| Shield / helm | Owner 0 `(0,8,0,2)`; owner 18 `(0,9,0,5)` | Good equipped AC +4/+2; obey M25 conflicts/proficiency |
| Gauntlets | Owners 0,18,14 `(0,13,0,6)` | Good equipped AC +1 |
| Boots / cloak | Every owner `(38,10,0,9)`; owner 11 `(38,11,0,10)` | AC +1 each while good/equipped; metal 38 has LAC 0 |
| Necklaces | Every owner `(38,2,0,12)` | No physical combat/stat effect; metal accessory does not add armor AC |
| Ordinary rings/medal | Owner 11 `(42,1,0,8)`; owner 1 `(42,5,0,8)` | No physical combat/stat effect; material 42 is not an attribute enchantment |
| Attribute ring | Owner 6 slot 1 `(86,1,0,0)` | When equipped, material index 27 gives +3 Speed via attribute category 3 mapped to stat 4. Affects initiative and AC for whichever owner receives it |
| Miscellaneous | None | Transfer/storage infrastructure stays intact; no effects or consumables introduced |
| Bare / bow-only melee | Reachable by Remove or transfer | No weapon dice; Adventurer still gives +5 hit bonus; Might-only damage, minimum 1 before resistance, possibly 0 afterward |
| Reachable broken armor | Original equipped armor state becomes `state OR 0x80` | Preserve frame/material/id/other bits. Entire armor AC/material/attribute contribution is suppressed. No repair or automatic unequip |

M25 may legally equip/transfer any of these records to another permitted original
owner. Do not restrict the ring to Seymour, or permit only the initial loadout.
All such effects are covered above. No initial curse, weapon breakage, elemental
weapon, slayer counter, HP/SP enchantment or attribute other than Speed is
reachable from these records and the Skeleton. Those branches need explicit
rejection if introduced by a changed installation; ordinary storage stays broad.

### Admission versus reachable-state validation

The fixed diagnostic entry checks the observed anchor's combat parameters against
this supported contract, including one attack, 2d6 physical, hit parameter4,
Cleric preference, physical resistance50 and zero special/range/loot. Generic
MON parsing retains other values; the formulas consume typed resource fields,
not a Skeleton-specific implementation. Numeric bytes widen without sign
extension; validate every divisor/table index before use (in particular positive
die sides and hit parameter). A modified resource that changes the promised
domain is a specific admission refusal and scope-review trigger.

After Begin, retain an immutable admission certificate of original identities,
rule inputs and the prepared loadout's exact item bytes. It is validation evidence,
not a second live party owner. Runtime validation checks:

- Membership, classes/races, level/age/max-stat skills and all new inputs except
  XP are unchanged. SP, inactive owners, flags/quest state and item ownership,
  material/ID/frame remain unchanged. Only equipped Armor state may monotonically
  acquire bit7; every other state bit is exact. No item disappears or moves.
- Conditions outside Unconscious/Dead remain zero. Reachable states are Good with
  positive HP, Unconscious=1 with HP<=0 and maxHP+HP>0, or Dead=1 with maxHP+HP<=0
  and Unconscious either0 or1. Existing condition flags are never cleared. HP is
  no greater than initial HP and no less than-23. Preserve the exact published
  values, using the action's preimage/postimage and shared revision to reject
  unaccounted changes rather than accepting an arbitrary value within the bounds.
- Actor5 remains Physical, at the engaged camera cell with 1..baseHP before
  death, or Defeated/HP0/removal coordinates afterward. Every bystander retains
  its original identity, raw statistics and all initialized live facts, including
  HP, position, activation/status/lifecycle. The immutable map/envelope/event
  isolation contract remains valid after cache reconstruction.
- XP matches the once-only accounting receipt or its initial resource value;
  context is unchanged except permitted minute charges, still day1/year610,
  ctr24 unchanged in combat, no buffs and no newDay/rested changes. A coherent
  terminal defeat/removal is valid runtime state, not failed initial admission.

Combat never calls the Good-only `validateDomain` as an injury predicate. Extract
or share its immutable terrain/isolation checks where useful, while keeping its
ordinary Diagnostic26 behavior unchanged.

## Literal combat rules

### Effective statistics and integer policy

Reuse the existing checked stat/age/item-bonus machinery. Extend it to read the
new roster inputs for Might, Speed and Accuracy. Original formula:

```text
age = min(uint32(year - birthYear), 254) + temporaryAge
ageIndex = first index with AGE_RANGES[index] > age
effective = max(permanent + ageAdjustment + equippedAttributeBonus
                + applicableConditionModifier + temporary, 0)
bonus(stat) = STAT_BONUSES[first i with STAT_VALUES[i] > stat;
                           use last index if none of preceding 23 matches]
AC = max(bonus(effectiveSpeed) + armor/equipment AC + temporaryAC + blessed, 0)
level = max(permanentLevel + temporaryLevel, 0)
```

At age 18, physical age adjustment is zero. Existing tables are already adapted
in `XeenCharacterRules.cpp`; reuse them. Their complete thresholds/bonuses are:

```text
thresholds 3 5 7 9 11 13 15 17 19 21 25 30 35 40 50 75 100 125 150 175 200 225 250 65535
bonuses   -5 -4 -3 -2 -1 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 20
```

`itemScan` considers all nine slots, nonzero frame and not bad (`state & 0xc0`
is zero), without adding an ID-nonzero condition to its existing attribute scan.
Armor AC additionally uses `ARMOR_STRENGTHS[id]` and metal LAC for 37..58.
Accessories do not get that base/metal armor term. The coverage table bounds all
indexes actually reachable. A broken armor record contributes no AC at all.

Use checked signed 32-bit intermediates for source `int` calculations and
unsigned 32-bit XP; preserve truncation toward zero at each integer division.
Use checked wide arithmetic to detect overflow before publishing; do not rely on
C++ signed overflow or reinterpret an out-of-range unsigned XP as signed. For
this anchor XP=250 and one attack per level-one character make that restriction
irrelevant to legitimate play. Preserve the original division/multiplication order.

The pinned runtime holds character HP in `int` although CHR serializes i16;
MMModern currently stores i16. Supported damage is nonnegative, at most two
12-point enemy applications per turn, and disabled characters are not selected
for a later turn. From a positive HP of at least 1 the lowest reachable result
is -23, safely within i16. Validate representability before its store. Actor HP
remains i32, clamped to zero on lethal player damage. Do not widen the save schema,
wrap, saturate character HP, or clear simultaneous condition bytes.

### Player Attack

Only the displayed eligible roster owner can act against the live selected
record. There is one target, so it is selected automatically by identity, not a
mutable pointer into a rendering vector. No action can target a defeated actor.

The RT_SINGLE branch performs the following, in this order:

1. Set physical damage. Attack count is `level / divisor + 1`: Barbarian 4;
   Knight/Ninja 5; Paladin/Archer/Robber/Ranger 6; Cleric/Druid 7; Sorcerer 8.
   Every admitted character currently has one attack.
2. For each attack, reset weapon damage/hit bonus/element material to zero.
   Scan **all nine weapon slots in ascending order** for frame 1 or 13. Frame 4
   bows are not included. For a good matching weapon: material <37 selects the
   element; 37..58 assigns metal hit and base-damage values. Regardless of bad
   state, matching records assign weapon ID/dice and roll each base die, adding
   to damage. Thus a bad weapon loses material benefits, not its base dice in
   this pinned code. No such bad weapon is reachable here, but do not implement
   a contradictory generic helper. M25 legal arrangements permit one melee
   weapon; the source's scan must not become an unexplained first-item lookup.
3. Add heroism per matching slot (zero here). Clamp negative weapon damage to
   zero; Adventurer adds **5** to hit bonus and multiplies weapon damage by **3**,
   even if no weapon matched. Warrior is not admitted and supplies neither bonus.
4. Hit score is `bonus(Accuracy) + weaponHitBonus + level / hitDivisor - Cursed`.
   Hit divisors: Knight/Barbarian 1; Paladin/Archer/Robber/Ninja/Ranger 2;
   Cleric/Druid 3; Sorcerer 4. Add `U[1,20]`, repeating and adding while the roll
   is 20. A 1 is not an automatic player miss. Compare `score >= monsterAC+10`.
   A 20 is an exploding hit roll, not a player double-damage rule. No monster
   status bonus is reached: actor status stays Physical.
5. On each hit add `max(bonus(Might) + weaponDamage, 1)` to accumulated damage;
   on a miss add zero. Weapon dice are consumed **before** the hit roll, even
   on misses. After all attacks, the source scans equipped weapon effectiveness
   counters for category multipliers. All admitted counters are zero, so none
   applies, including on the equipped bow.
6. Call the damage application once with the accumulated amount. Zero is a miss
   without a minimum-damage override. For nonzero physical damage and weapon ID
   below 34, apply `damage * (100 - physicalResistance) / 100`. Here this is
   `damage * 50 / 100`. No minimum follows this reduction. Elemental addition
   is zero for every admitted melee record. Damage below 1 leaves actor HP
   unchanged and reports no damage. Otherwise subtract, clamping actor HP to 0.
7. Recalculate the speed table, preserving the current participant identity, and
   advance with the reference participant rules. A lethal action removes the
   target and accounts XP before any later enemy opportunity; it must not invent
   retaliation after victory.

In the source, `_attackWeaponId` is not reset when no weapon matches. It remains
0 or a previously used admitted ID below 34; this does not change physical
resistance here. MMModern may use explicit no-weapon ID 0 for presentation,
documented as an adaptation with identical admitted damage. There is no elemental
carryover because `_weaponElemMaterial` is reset on every scan.

Useful independent values: original Arturius rolls four ones and hit 10:
`weapon=4*3=12`, Might bonus 3, hit score `2+5+0+10=17`, damage `15*50/100=7`.
Four twos yield 13 damage. Original Tyro's two threes produce
`(6*3+4)*50/100=11`. Bare Arturius deals `3/2=1` on a hit; bare Rebecca deals
`max(0,1)/2=0`. This is not an unsupported-state error. An equipped bow alone
does not change those melee results.

### Initiative, acted state and Block

`setupCombatParty` borrows the six active roster owners. It never copies their
mutable characters. `setSpeedTable` sorts positive effective speeds descending,
ties by participant index ascending: active indexes 0..5, then monster slots
6..8. The one occupied enemy slot is 6; no holes occur in this encounter.
No initiative RNG exists. The original order is owners
`0,18,14,11,1,6,monster5`. With the +3 Speed ring on Seymour the order begins
`6,0,18,14,11,1,monster5`. All admitted speeds exceed the Skeleton's 10, so this
domain has no initial enemy attack before the first player. The scheduler still
services any due enemy participant rather than assuming all players always precede
monsters in its generic rule implementation.

At entry clear acted and blocked, initialize speed/turn cursors to -1 and call
the equivalent of `nextChar`. That function marks the previous participant gone,
advances cyclically through the speed table, and skips characters for which
`isDisabledOrDead` holds. It services a monster immediately when its entry is
reached, then continues. `allHaveGone` ignores an unacted character whose worst
condition is Paralyzed..Eradicated; Good or a condition below Paralyzed still
counts. With the admitted states, this is precisely Good versus Unconscious/Dead.

Block sets only the current character's blocked flag and advances the turn.
It has no RNG, damage reduction, HP gain or individual time charge. It changes
the enemy hit threshold from `AC+10` to `AC + level/2 + 15`, a +5 improvement at
level 1. The flag persists through subsequent participants/enemy work and is
cleared with acted state at the next round reset. It is not a permanent AC buff.

When the round is exhausted: clear acted/blocked, recalculate speeds, reset
cursors and select the next participant; run the source-equivalent movement
opportunity/classification, then `changeTime(1)`. Reuse the pure approach movement
kernel for that opportunity over all original actors, without reopening approach
admission or granting a player move. The same-cell Skeleton has zero delta;
bystanders remain outside the scan. Medusa HP reset is unreachable and omitted.
No actor HP reset follows table recalculation or cache rebuild.

### Enemy target, hit and physical injury

For each resource attack (one): search active combat references in order for an
eligible preferred class, excluding worst conditions Paralyzed..Eradicated.
The Skeleton prefers Cleric, initially Rebecca, without a target-selection roll.
After she is disabled, roll `U[0,5]`. If that choice is disabled, construct the
ascending list excluding Paralyzed, Unconscious, Dead, Stoned and Eradicated,
then roll `U[0,ableCount-1]`. Even `U[0,0]` is a request. No eligible target means
party defeat. Never use modulo over a reduced list instead of the source's two
possible requests.

Physical attack against the admitted awake target:

```text
r = U[1,20]
if r == 1: miss; no hit-parameter roll and no damage dice
else:
    if r == 20: applyPhysicalDamage()  # first complete call now
    v = r + floor(hitParameter/4) + U[1,hitParameter]
    threshold = currentAC + (blocked ? floor(level/2)+15 : 10)
    if threshold <= v: applyPhysicalDamage()  # a separate call
    else: ordinary hit check misses (does not undo the first call)

applyPhysicalDamage():
    clear Asleep
    amount = sum(strikes copies of U[1,damagePerStrike])
    amount = max(amount - powerShield, 0)  # shield zero here
    subtractHitPoints(amount)
```

Here `hitParameter=4`, so `v=r+1+U[1,4]`. Enemy damage is 2d6 and is **not**
tripled or reduced for Adventurer. No saving throw, elemental resistance roll,
special-condition roll or luck input occurs. A natural 20 causes its first damage
even through Block; it does not guarantee the second check. AC is recalculated
after that first application, including any armor it broke. The second application
does not reselect or skip its target because the first made it unconscious/dead.
Preserve both intermediate results, separate dice requests and condition writes.

`subtractHitPoints` subtracts first, without clamping current HP. If resulting HP
is below 1, calculate `maxHP + currentHP`: positive means set Unconscious=1;
zero or negative means set Dead=1. Existing Unconscious is not cleared on death.
Set every occupied, equipped armor record's broken bit if HP <= -10 or death was
set, preserving its frame and every other byte. The alternate -80 option is not
enabled. Weapons/accessories do not break from this physical-injury path. No
normalization at zero, no healing on exit and no resurrection after cache rebuild.

`XeenCharacter::canAct` matches `!isDisabledOrDead` in this domain and may be
reused after validation. Do not use it for XP. Defeat follows
`Party::checkPartyDead`: there is no member whose worst condition is Good or
<=Confused. With reachable states, a party of unconscious/dead members is
defeated even if some are not dead. Do not wait for all six Dead flags.

## Deterministic reference controls

Literal synthetic tests cover the formulas and exact RNG request order without
commercial resources. Original-data production controls use the admitted map,
party, items and monster resources rather than replacing HP, XP or outcomes.
The retained deterministic controls are:

- Seed 1 with original equipment: Block the first round, then Attack with every
  eligible displayed owner. The enemy misses in round one; later attacks and a
  critical injury leave Rebecca at -4 HP and Unconscious. Victory completes at
  minute 493 with the Skeleton removed and +82 XP for each owner.
- Seed 19 after removing all Armor-category equipment in Preparation: Block every
  eligible displayed turn. Thirty-five player actions and eleven enemy turns end
  in Defeat at minute 500, with HP [-7, -1, -1, -5, -3, -7], no equipped armor to
  break and no XP.
- Separate literal controls cover exploding player rolls, zero-damage hits,
  preferred/fallback enemy targets, two separately applied critical-hit injuries,
  the -10/death armor-break rules, XP eligibility/rounding, the delayed approach
  and the equipped/transferred Speed ring.

These controls are acceptance oracles for the bounded production path, not a
claim that the optional xorshift32 seed reproduces the original engine's PRNG.

## Ownership, phases and publication

### State table

| State | Sole authority / lifetime |
| --- | --- |
| Original actor identity/statistics, live i32 HP/coordinates, lifecycle | Existing `XeenSessionWorldState`/`XeenWorld`; survives disposable cache and Flow presentation rebuilds |
| HP/SP, conditions, exact equipment bytes | Existing roster characters; never copied into an authoritative combat-party array |
| New Might/Speed/Accuracy/temporary AC and XP | Optional supplemental roster payload installed only in marked Diagnostic27; XP mutable in place by owner identity |
| Gameplay context | Existing party `encounterContext`; actual approach time retained |
| Encounter kind, combat-entered, terminal outcome and defeat/XP accounting identity | Private world encounter facts plus the shared monotonic encounter revision; irreversible |
| Current participant, speed table, acted/blocked, target identity, pending automatic kind | One noncopyable encounter coordinator inside existing Flow; references/indices, not copied characters |
| Gameplay RNG state, pending random prefix/action candidate | Same coordinator, separate from cosmetic/NPC sources; not renderer state |
| Result, presentation frame/step/deadline and input-generation ticket | Flow; results are owned fixed observations, not authority to replay effects |

The focused `XeenCombat` domain extends the existing encounter coordinator.
Its operations prepare the session, begin and run approach, perform the one-time
combat handoff, accept `command(Attack|Block, ticket)`, service automatic work
and fail only current authority. They borrow
the existing owners and return fixed typed results: Accepted, Refused, Stale,
Pending, Advanced, Victory, Defeat, SupportStopped or Failed, with old/new
revision, acting/target identities, ordered damage facts, condition/item deltas,
XP receipt and time delta as applicable. Resource queries and RNG interfaces are
injected; no callback is stored in a result and no public apply-prepared-result
API lets a caller publish against a different owner.

| Operation | Required input/phase | Publication and supplied result |
| --- | --- | --- |
| constructor | Marked Diagnostic27, fresh roster, checked initial CHR/providers | Install six supplemental records once or fail startup, never partial interactive state |
| `equipment` / `transfer` | Current Preparation ticket and boundary lease | Reuse M25/M24 validation and publication while advancing combat preparation authority |
| `beginApproach` | Current closed-preparation ticket, original item conservation and supported loadout | Close selections, invoke one-time M26 initialization; return approach result at480 with original live actors |
| `approachAction` / `approachPulse` | Current retained Approach ticket | Reuse M26 action/pulse publication with the shared Diagnostic27 boundary generation |
| `beginCombat` | Retained current Engaged approach ticket and record5 identity | Advance shared revision, consume handoff, install combat progress; return first participant and target |
| `command` | PlayerReady ticket including owner/index/target, Attack or Block | Install one consumed intent, then publish prepared action and next-work kind; fixed ordered damage/XP facts, no externally applicable candidate |
| `service` | Exact pending kind/id, revision/generation and due service token | Finish only that random prefix, enemy, round or end transition; return next pending/ready/terminal state |
| `fail` | Retained ticket and typed reason | Stop only still-current nonterminal work; preserve already published terminal outcome and all prior effects |

Results use bounded value storage: at most two enemy damage subresults, six XP
recipient entries and nine armor-slot changes for the targeted character. They
own IDs and scalar facts, not borrowed vectors or pointers. Failed/Refused/Stale
results do not grant continuation authority; only the coordinator's newly adopted
pending work can do so. Detailed text is formatted later from these values.

`XeenActorLifecycle::Defeated` is the explicit terminal actor state.
On lethal hit retain original metadata/id, set live HP0 and live coordinates
`(-128,-128)` (reference removal marker `0x80,0x80`), clear targeting/occupancy/
composition participation, and latch accounting for that original identity.
Do not insert it into ordinary object/event Remove sets. Actor vector length and
all bystanders remain exact. Initialization is never rerun on cache discard.

### Publication units

A player command is consumed when its current turn ticket is accepted and a
PendingAction intent is installed, before random/provider callbacks. Refused,
unsupported, repeated or stale inputs consume no gameplay action, time or RNG.
Preparing its owned candidate does not modify live HP/items/XP. An accepted
intent can finish once or end in a legitimate failure; it cannot be retried as
a fresh roll. Candidate state is bounded to one operation, never an entire fight.

Publish one player action or one enemy attack as a bounded nonthrowing operation
after all its fallible values are ready. An enemy critical performs both damage
calls **sequentially on the candidate**, records each intermediate HP/condition/
armor result, and reads the second AC from that updated candidate. This preserves
source results while preventing callbacks from observing half a critical action.
It is not equivalent to replacing the two calls with doubled damage.

Lethal player publication includes actor HP/removal, every prepared recipient XP
addition and once-only accounting together. XP allocation/overflow checks precede
this store; failure before it publishes none of those effects. The result is
`VictoryAwaitingEnd` until the separate source end-time transition succeeds.
Previous nonlethal hits are never rolled back. A subsequent failure must retain
removal/accounting, cannot invent a completed Victory, and cannot grant XP again.

Nonlethal action publication authorizes advancement only once. Turn advancement
may create PendingEnemy, PendingRound or the next PlayerReady state; no incoming
player command can cancel required automatic work. Enemy publication may produce
Defeat immediately. Round and end-time publications are separate operations with
their own retained tickets and fixed results. Successful end-time publication
latches Victory. Post-publication feedback failure preserves Victory/Defeat and
every effect; no failed notice converts one terminal outcome into another.

### Phases and action routing

| Phase | Accepted action/work | Refused / resulting boundary |
| --- | --- | --- |
| Preparation | Inventory with the [preparation cancellation adaptation](#production-entry-and-preparation), Enter to Begin only when closed, Escape exits session | Navigation/events/combat/saves; no actor work to pause |
| Approach | Existing M26 movement/turn/Wait and authorized pulse | Inventory/events/Attack/Block/save; Engaged enters one-time handoff only for Diagnostic27 |
| PlayerReady | Attack or Block bearing displayed owner/turn generation | Navigation/Wait/events/inventory/save; no manual turn skipping |
| PreparingAction / PendingEnemy / PendingRound / PendingEnd | Authorized bounded continuation only; exit | Player inputs cannot queue for a later owner; refusal does not erase automatic work |
| Result presentation | Cosmetic progression and subsequent authorized automatic continuation; exit | No acknowledgment required to cause damage or service an enemy; no action replay |
| Victory / Defeat / SupportStopped / Failed | Readable fixed result, F9 refusal, exit | No exploration, inventory, recovery, new fight or generic resume |

### RNG and reentrancy

Keep gameplay RNG entirely separate from M26 cosmetic phase, M22 object animation,
NPC random frames and draw frequency. The production adapter owns a nonzero u32
state. The reproducible generator is xorshift32, using unsigned operations
`x ^= x<<13; x ^= x>>17; x ^= x<<5`, storing x after each draw. For inclusive
`[lo,hi]`, let unsigned `span=hi-lo+1`, `threshold=(0u-span)%span`; discard raw
outputs below threshold and return `lo + raw%span`. Validate positive bounded
span. Even a one-value range consumes one raw draw. Default seed is sampled once
before interactive startup; an optional CLI seed is explicitly a diagnostic
reproduction control, not original PRNG replay or an HP/XP cheat.

Tests inject a cloneable value-type random cursor producing checked inclusive
requests/values. Preserve the exact request order in the traces: target selection
if needed, enemy d20, first critical dice if applicable, hit-parameter roll,
second dice only if applicable; player weapon dice precede exploding hit rolls.
Block, refusal, view reconstruction and time inside the admitted window use no
gameplay RNG. No loot RNG occurs.

At accepted intent, clone the gameplay RNG cursor into the operation candidate.
Preparation consumes that cursor, not the live cursor. On successful publication
adopt the prepared cursor with the action. A current preparation exception or
invalid provider value ends the encounter in Failed, retaining earlier committed
facts and live RNG, with no retry. Reentrant entry is refused by the same busy
guard; after **every** provider callback, verify retained owner addresses, shared
revision, generation, phase, action identity and relevant preimages. A provider
which directly changes borrowed state invalidates the candidate. Never obtain a
new current ticket in a catch block to authorize an old failure.

An exploding d20 or range-conversion rejection chain has no mathematical fixed
length. Bound each service call to 64 raw/provider draws, retain its uncommitted
candidate/cursor as PendingAction and continue on a new explicitly authorized
service pulse. This is an implementation scheduling adaptation, not a cap on
original rolls. Do not reroll the retained prefix. Checked integer overflow
publishes a current failure before action effects; stale failure publishes nothing.
Exit discards unpublished preparation without inventing an action result.

Resource, RNG, clock, composition, reporting, result/frame copy, recovery and SDL
handoff callbacks all obey retained authorization. A stale operation cannot
publish, stop, replace or retire newer work. A still-current failure must publish
its legitimate stop and retire only its own pending work. Allocation and fixed
result construction occur before nonthrowing stores; adopt those results before
reporting/drawing. A current presentation failure may rebuild presentation once
from live facts, without replaying RNG, mutation or failed reporting. Failed
recovery/upload exits. Terminal Victory/Defeat survives such presentation failure;
preterminal failure does not become Victory merely because an actor is absent.

## Time and continuation scheduling

S caller trace: `doCombat` setup has no `changeTime`; Attack and Block dispatch
call their combat operation then `nextChar`. The new-round branch clears round
state/selects next, moves/classifies monsters, then calls `party.changeTime(1)`.
The subsequent no-front-attacker branch calls `changeTime(1)`, redraws and exits
when no front attacker remains. `endAttack` clears projectile presentation; it
does not add a hidden melee time cost. Empty `giveTreasure` is not a time/reward
subsystem requirement. Defeat in the enemy `nextChar` path breaks before marking
that enemy gone and does not create a new-round or victory-exit minute.

Retain day1/year610 and the **[480,960)** window. A normal combat start is490 or
500; a completed nonterminal round adds1 and a successful victory exit adds1.
Individual commands, critical subcalls, target selection, presentation and input
refusals add0. Combat does not call outdoor `chargeStep` or increment ctr24.

The first unsupported `changeTime(1)` is959->960. `Party::changeTime` compares
integer division by480; crossing invokes stat/condition processing, poison/
disease RNG even with zero condition bytes, and Dead severity increments. Within
the window, newDay=false, no buffs/light/party resistances exist, and reachable
Unconscious/Dead trigger none of the per-call Confused/Paralyzed branches. Thus
checked minute addition is sufficient only strictly inside this window; this is
reverified for injured states rather than copied from M26's Good-only claim.

Boundary ordering follows the caller, not a guessed per-key charge:

- At959, a player's accepted nonlethal action and the enemy work it makes due may
  publish. When PendingRound would perform the unsupported time transition, stop
  before its round-reset/movement/time publication. Earlier damage/RNG remain.
  Refusal deliberately precedes the round's otherwise unobservable bookkeeping;
  it does not roll back a previous enemy turn or silently freeze rounds.
- A lethal action at959 publishes removal/XP and VictoryAwaitingEnd. PendingEnd
  refuses960 and produces SupportStopped with truthful already-awarded XP and
  removed actor, not a completed Victory. It is unsaveable and cannot resume.
- If an enemy action defeats the party before that boundary, publish Defeat with
  its actual time and no fabricated final minute. Refusals never consume the
  condition-processing RNG at960.

Automatic combat work uses explicit bounded continuations in the existing SDL
idle path, never a blocking combat loop or a nested modal loop. One due callback
services at most one logical automatic publication or 64-draw preparation chunk.
After a result frame, set a monotonic **100 ms** next-service/presentation deadline
from the observed current time. No elapsed backlog is replayed. An already-due
enemy does not require another key or modal acknowledgment and cannot vanish
through inactivity. Repeated same-time callbacks and same SDL cycle cannot service
it twice. Backward/overflowing clock observations cannot rearm obsolete work.

Keep gameplay work and cosmetic steps distinct even if they share a deadline.
No monster turn is created by an idle with no pending combat work. Normal sprite
cycling and attack sequences consume no gameplay RNG. Existing M26 action/pulse,
same-cycle arbitration and delayed approach remain unchanged. M22 ordinary-object
phase retains its existing navigation reset/advance semantics in approach and its
independent 100 ms no-backlog idle cadence during combat; Attack/Block do not
pretend to be navigation actions or reset it. Terminal states freeze encounter
presentation as in M26; cache refresh preserves phase and deadlines.

## Victory, defeat, XP and persistence safety

XP is derived from the resource value and current combat references at lethal
publication. Count recipients whose **worst** condition is not Dead, Stoned or
Eradicated. Unconscious remains eligible. Divide base XP by that count first;
for each eligible owner with permanent level<15 outside standalone Clouds,
multiply its quotient by2, then add to its original uint32 XP. Zero eligible
recipients is an invalid lethal-action precondition in this domain (no able
attacker); fail without dividing or inventing rewards. Checked accumulation must
not wrap. The initial250 and zero XP cannot overflow in this one fight.

The whole lethal award/removal is accounted once by original monster identity,
not by a message acknowledgment or missing draw command. No quest flags, gold,
gems, items, reward queue, training or level change occurs. Cache discard cannot
respawn the actor or reconstruct XP from initial CHR. Defeat preserves negative
HP, conditions, equipped/broken bytes and living actors; it awards nothing.
Victory and Defeat have no supported route back to exploration. Escape/window
close exits this unsaved session, without Run, a final enemy pulse, healing or
save-on-exit. Already published facts remain final until owner destruction;
unpublished pending work is abandoned because the process/session is ending.

M27 remains unsaveable in preparation, approach, combat, pending work, interrupted
presentation, victory, defeat and every stop/failure. F9 must check irreversible
authority before capture/preflight, target-path handling or I/O and create no
deferred save. Extend the existing check to the roster supplemental-input marker
as well as world/context, so separating a marked roster from its original world
cannot make it capturable. Direct capture and restore must reject either graph
with any encounter-only inputs, before and after callbacks/preflight. Ordinary
initial-party providers may not supply those inputs. No operation can clear the
marker by discarding Flow/presentation or restoring world overlays.

The supplemental payload belongs to `XeenRoster`, outside the existing
snapshot's `array<XeenCharacter,30>`, so ordinary snapshots/codecs retain their
exact field set. Guard capture before constructing a partial snapshot. Do not
add codec fields, schema/version, migration, placeholder XP or a partially
saveable post-victory state. Ordinary v1/v2 loading, exact item bytes and existing
field preservation remain unchanged.

M28-facing requirements only: preserve original actor identity/removal and moved
position, current HP/conditions/broken and equipped items, new XP/character inputs,
and admitted gameplay context. Living actor HP survives disposable cache rebuild;
the pinned reference resets surviving monster HP from resources on **true map
load/re-entry**, including restart. That is different from cache rebuilding and
fresh new game. Turn cursors, blocked/acted, selected target, uncommitted random
work and visual deadlines are transient at the future quiescent boundary. This
plan selects no M28 format or migration policy.

## Production input and presentation

Keep the existing Application/services/Flow/SDL route and one SDL loop. Add typed
Attack and Block actions. Choose **Space=Attack, B=Block in PlayerReady**; the
combat hint says so. Space remains ordinary interaction outside combat, A/D remain
navigation turns outside combat and are refused inside it, E remains inventory
equipment only in preparation. Period is approach Wait and refused in combat.
This avoids changing the global meaning of A or allowing an inventory key to
double as a combat action. Direct Flow/service calls enforce the same gates.

Preparation input and transfer feedback follow the
[Diagnostic27 cancellation contract](#production-entry-and-preparation): route
Escape to session exit before inventory dispatch, and adapt I only in that phase.

SDL must attach the displayed input/turn generation to physical actions. At the
start of a poll batch retain the current displayed generation; buffered events
in that batch cannot acquire a later character's ticket after the first action.
Ignore repeats, reject keys queued before the new ready frame, and require a
fresh press after release for Space/B. A rapid second press observed while work
is pending is refused, not queued. Only a new physical press after the next
PlayerReady frame is admitted for its displayed owner. Synthetic handler tests
must use this same ticket seam rather than calling an unticketed bypass.

Use existing fonts, frames and party composition. At minimum display:

- Original character name, active slot, selected monster name and current/live HP;
  clear Space/Block hints only when an action is currently admissible.
- Fixed Attack miss/no-damage/damage results, Block, enemy target/hit/damage, and
  both damage subresults for a critical when applicable.
- Signed current HP and condition changes, concise broken-equipment summary,
  readable pending enemy/round state, minute and unsaveable marker.
- Unmistakable Victory with actual per-recipient XP (including ineligible owners)
  or Defeat without healing; support/failure diagnostics must not resemble victory.

No console-only feedback. Keep result facts until the next accepted action or
automatic result; the next ready frame may retain the preceding result beneath
its new actor hint. Avoid an acknowledgment modal that can suppress retaliation.
Large result text may use bounded retained pages, but paging is cosmetic and
cannot gate due automatic work; latest HP/terminal status must remain visible.
Physical acceptance decides readability, not whether the underlying damage runs.

### Original attack appearance

The original `008.att` has four frames. All four pass the production
`validateXeenObjectSprite` path, including their complete row streams. The
resource's frame cells are:

| ATT frame | Cell offset and `(x,width,y,height)` |
| --- | --- |
| 0 | 18 `(0,250,21,129)`; 2618 `(0,250,0,140)` |
| 1 | 18 `(0,250,21,129)`; 8244 `(0,250,6,135)` |
| 2 | 18 `(0,250,21,129)`; 12853 `(0,250,21,120)` |
| 3 | 17317 `(0,250,26,124)` |

In the pinned reference, monster logical frames0..7 select MON and 8..11 select
ATT frame-8. Enemy attack
sets frame8/delay3. Each logical animation advance gives9,10,10,10,0. Player
nonzero physical hit selects frame11/delay5; five subsequent advances return it
to0. Lethal removal may end that effect immediately. Represent these as bounded
cosmetic sequences driven by Flow's monotonic100 ms steps, not mutable actor HP
or redraw counts. The timing adaptation does not claim exact reference audio/
blocking-draw latency. Use explicit appearance kind and bounded frame in the actor
draw payload; validate all required normal/attack frames on admission/rebuild.

For the single same-cell actor, retain anchor `(-5,2)`, scale0, scene and bottom
clipping. `drawOutdoorsScene` relocates the front monster command from original
draw slot118 to121 when logical frame>=8 (with adjacent layer slots119/120 moved
to122/123). For this one-actor payload, emit the attack command at order121,
normal at118; do not overlay it after the scene or enlarge/shift its anchor.
The cell-internal offsets above still apply. Other actor placements during
approach retain M26's orders/anchors and accepted forest occlusion.

Extend `XeenAssetSource`/`ScummVmXeenBridge` normal-monster safety/cache path with
an explicitly typed four-frame attack resource, using the same decoder/cache
owner. Do not pass ATT through the existing eight-normal-frame check unchanged.
Keep current interface layers after the shared terrain/object/actor stream.
No general particle/audio system is needed. M27 omits weapon POW overlays,
portrait flash and voice/sound; original MON/ATT and accurate readable text/party
injury feedback are retained. Structural validation alone does not establish
raster readability; original-data composition evidence and the maintainer's
physical acceptance establish that combined boundary.

## Implemented units and final result

### 27A - Authoritative bounded combat domain

27A established one noncopyable combat coordinator over the existing world,
party, roster and camera owners. It added the admitted combat inputs, exact
Attack/Block and enemy rules, retained deterministic RNG continuations,
conditions and armor breakage, bounded time transitions, atomic actor removal/XP,
terminal Victory/Defeat and direct persistence guards. Fixed results retain
operation, actor, target, ordered injury and XP facts without becoming replay
capabilities.

### 27B - Production preparation and coordination

27B connected Diagnostic27 through the existing Application/Flow/SDL path. It
added strict CLI entry, real M24/M25 inventory preparation, one-time Begin and
engagement handoff, generation-bearing displayed input, mandatory idle-serviced
enemy/round/end work, readable typed outcomes and irreversible F9 refusal.
Existing ordinary and Diagnostic26 behavior remains separate and unchanged.

### 27C - Original combat appearance

27C added an explicit disposable normal/attack appearance value, four-frame ATT
resource validation beside the eight-frame MON path, and both resources in the
shared sprite cache owner. The original same-cell attack command uses order 121,
anchor (-5,2), scale 0 and existing scene/bottom clipping; normal appearance
retains order 118. Flow starts only published enemy attacks or positive player
hits, advances the bounded source-derived sequences at 100 ms without backlog,
and never consumes gameplay RNG or changes combat authority.

The combat layout keeps the central scene visible while showing retained outcome
text and each owner's HP, simultaneous conditions, broken-armor count and terminal
XP. Admission validates both MON and ATT before gameplay. Cache discard and
presentation reconstruction revalidate resources and reproduce the current
appearance while preserving actor HP, combat revision, RNG position, service and
cosmetic deadlines, terminal state and once-only accounting.

## Final acceptance

Milestone 27 completed its combined acceptance boundary:

- The implementation agent delivered the accepted 27A headless combat domain,
  27B production preparation/coordination and 27C original MON/ATT appearance.
- The configured build and complete CTest suite passed for the accepted implementation.
  Synthetic tests cover literal rules, RNG ordering, publication and failure
  boundaries without commercial data; original-data controls reached the expected
  deterministic victory, defeat, delayed-approach, ring and combat-control results.
- The independent reviewer approved the complete implementation, including the
  authoritative ownership/ticket model, deterministic control boundary,
  presentation/cache invariants and preservation of ordinary/Diagnostic26 behavior.
- The maintainer personally completed physical SDL victory, loss and regression
  acceptance, confirming a readable real exchange, original attack appearance,
  injury/terminal feedback and the required controls. This physical acceptance is
  the maintainer's result, not an automated or independent-review claim.

The accepted result is a bounded playable encounter, not general Xeen combat or
map-20 traversal. It adds no save fields or version. Diagnostic27 remains
unsaveable through Preparation, approach, combat, terminal outcomes and failures.
[M28](roadmap.md#m28---durable-bounded-encounter-completion-and-revisit) remains
responsible for selecting a compatibility policy and persisting actor lifecycle,
combat-derived roster/progression values and admitted context at a safe completed
boundary. M28 planning and implementation require separate authorization.

## Replanning triggers

Reopen this contract only for evidence of changed original resources/item domain,
contradictory connected reference behavior, extra actor admission, an unsafe
handoff/publication/RNG/save boundary, or a failure of original ordered composition
to provide the accepted readable exchange. Report the smallest affected contract;
do not silently widen the domain, repair unsupported items, or fold M28 work into
M27.
