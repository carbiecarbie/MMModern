# Milestone 27 - Playable Attack/Block encounter with original outcomes

**Milestone 27A is accepted; M27 remains open.** The
[27A acceptance record](#27a-acceptance) covers the headless domain only.
M27B and M27C remain future work requiring separate implementation authorization.

## Objective, authority and evidence

Continue the accepted M26 encounter into a real melee Attack/Block fight: a named
acting character and target, misses and damage, mandatory enemy turns, injury,
incapacitation/death, and victory or party defeat. Victory removes the selected
original monster and awards applicable XP exactly once within the session.

The inspected MMModern baseline is `49c2adfe303c1f9e66d5da17cb6b06004eef021c`,
branch `main`. HEAD, origin/main and direct remote main agreed, with an empty
index and clean working tree before this investigation. Its committed M26 plan
records independent review and maintainer physical acceptance. The
[stable status](project-status.md), [closed M26 contract](milestone-26-plan.md),
[M25 equipment contract](milestone-25-plan.md) and [roadmap](roadmap.md) remain
the accepted foundation. This plan does not revise their completed capabilities.

Reference: ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, verified exact HEAD
and clean source checkout. Source and library locations were resolved from the
configured `build/26a/CMakeCache.txt`, following [dependencies](dependencies.md).
That configuration uses the pinned external source, UCRT64 compiler and separate
library build; its saved ScummVM configuration is `--backend=sdl
--disable-all-engines --disable-detection-full`. The top-level legacy build cache
points to a different source/build pair and is not evidence for this work.
No existing MMModern or ScummVM executable was selected or run. Temporary probes
were freshly compiled with the configured UCRT64 G++ from inspected source.
Commercial archives were read only, with temporary outputs outside both source
trees and the installation. No full ScummVM combat run is claimed.

Evidence labels used below:

| Label | Meaning and limits |
| --- | --- |
| R | Original-resource observation from checked CC member extraction; compact observations, not bundled fixtures |
| S | Pinned-source derivation, including callers and branches; not an executed engine |
| E | Executed unchanged pinned function bodies in the bounded C++ probe, with explicitly listed stand-ins |
| M | Independently written arithmetic/RNG/outer-loop model; does not independently prove its own scheduling |
| C | Current MMModern code or focused execution; not future combat acceptance |
| F | Required future implementation tests, original-data checks or physical acceptance; not passed by this investigation |

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

## Accepted integration seams and chosen entry

The implementation currently supplies these reusable seams, not combat:

| Current seam | Use in M27 |
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

Proposed syntax (not implemented):

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
`new roster inputs` below means a proposed optional supplemental payload **inside
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

## Reproducible traces and evidence boundaries

The C++ probe extracted function bodies by signature and balanced braces from
the pinned source, without editing their bodies: character `worstCondition`,
`isDisabledOrDead`, `subtractHitPoints`; combat `setupCombatParty`, `setSpeedTable`,
`allHaveGone`, `charsCantAct`, `block`, both `doMonsterTurn` overloads,
`giveExperience`; party `checkPartyDead`; interface `nextChar`.

A second focused probe executed unchanged `getWeaponDamage`, `hitMonster`,
`getMonsterDamage` and `Character::statBonus`, using the pinned literal tables,
original Might/Accuracy values and simple level/stat/party/map stand-ins. It
confirmed the Arturius7-damage, exploding20, bad-weapon base-dice, bare-attack and
Seymour zero-after-resistance controls. The last resistance multiplication was
driver arithmetic; neither `attack` nor `attack2` was executed by this probe.

Stand-ins supplied vectors/points, map metadata, original numeric character
inputs, equivalent armor-sum/Speed AC, maximum HP and level getters, VM mode,
inclusive scripted RNG, sound and draw no-ops. `doCharDamage` was a **bounded
physical-path stand-in**, executing two dice requests, wake and the extracted
`subtractHitPoints`; it was not the full pinned function. Special attacks,
resistances, debugger, pause/windows and sound were bypassed as unreachable or
cosmetic here. The armor stand-in encoded AC strengths in its item ID field,
so its execution proves affected equipped slots and arithmetic, **not original
armor ID bytes**; byte preservation remains an S contract and F test.

The driver invoked extracted `nextChar` for participant scheduling; it did not
replace that function with its own order. It mirrored the outer round branch of
`doCombat` and used arithmetic minute addition. Full `doCombat`, party time,
Attack/attack2, rendering and SDL were not executed. Those contracts are S plus
the explicitly labeled M traces below. This evidence limit does not leave their
formulas unspecified; future independent tests must execute the production path.

Reproduce with a C++17 harness containing those functions and the stated stand-ins,
the R table above, one actor with the R monster fields and fresh shared references.
No probe binary, temporary path or commercial payload is needed to reconstruct
these controls. All traces use record `(Clouds,20,5)`, live HP 20 initially, at
the party cell after fresh Wait, day1/year610/minute490/ctr24=1 unless stated.
SP, membership, flags, quests and all bystanders stay unchanged throughout.

### Entry, a full round and victory (S/M)

Original equipment, no ring equip. Initial order as above; no RNG or time at
combat setup. In round 1 perform:

| Acting owner | Action and ordered supplied RNG | Arithmetic and publication | Next |
| --- | --- | --- | --- |
| 0 | Attack: four `U[1,2]=1`, `U[1,20]=10` | Hit 17>=15; pre-resistance15 ->7; monster20->13 | 18 |
| 18 | Block, no RNG | blocked[1]=true; no HP/time | 14 |
| 14 | Attack: two `U[1,3]=1`, `U[1,20]=10` | Hit15>=15; `(6+2)/2=4`; monster13->9; bow ignored | 11 |
| 11 | Attack: two `U[1,2]=1`, `U[1,20]=1` | Hit9<15; dice already consumed, monster9 | 1 |
| 1 | Block | blocked[4]=true | 6 |
| 6 | Block | blocked[5]=true | Required enemy turn |
| Monster | Prefers owner1, no target RNG; `U[1,20]=15`, `U[1,4]=4` | v20<blocked threshold21; miss, no damage dice | Round reset |

Every action leaves time490 until the separate round transition advances to491.
Flags clear; owner0 acts. Attack with four `U[1,2]=2`, `U[1,20]=10` deals13 to
remaining9: actor HP0, removed, XP+82 to every owner, no gold/gems/items. End
transition advances491->492 and publishes Victory. No enemy retaliation follows.

Block comparison: the same enemy roll15/4 against unblocked Rebecca has threshold
16, so it hits; subsequent dice3/4 cause HP7->0, Unconscious=1, armor unchanged.
After a new-round reset a previously blocked character uses the unblocked threshold
until they Block again. Player exploding control: Arturius's four ones followed
by hit rolls20,1 gives score28 and still damage7, with two hit RNG requests.

### Critical hit, intermediate breakage and complete defeat (E/S)

Original loadout, every eligible character Blocks. Round enemy RNG, with no
player RNG, is listed below. `20;6,6;4;6,6` means d20=20, first 2d6=6/6,
hit parameter U[1,4]=4, second 2d6=6/6. Target requests precede this sequence.

| Round / start time | Target RNG / owner | Ordered enemy sequence | HP and condition after each application | End |
| --- | --- | --- | --- | --- |
| 1 /490 | Preference /1 | `20;6,6;4;6,6` | 7->-5 Unconscious; ->-17 Dead, Unconscious retained; both equipped armor slots break | time491, next0 |
| 2 /491 | `U[0,5]=0` /0 | `20;6,6;4` | 12->0 Unconscious; AC13+15=28>25, so no second dice/application | time492, next18 |
| 3 /492 | `U[0,5]=1` /18 | `20;6,6;4;6,6` | 16->4 Good; ->-8 Unconscious, no break | time493, next14 |
| 4 /493 | `U[0,5]=2` /14 | `20;6,6;4;6,6` | 12->0 Unconscious; ->-12 Dead and equipped armor breaks | time494, next11 |
| 5 /494 | `U[0,5]=3` /11 | `20;6,6;4;6,6` | 10->-2 Unconscious; ->-14 Dead and equipped armor breaks | time495, next6 |
| 6 /495 | `U[0,5]=5` /6 | `20;6,6;4;6,6` | 5->-7 Dead and armor breaks; AC becomes1; ->-19 Dead | Defeat at495, no round charge |

This was executed through pinned `nextChar` and enemy/HP functions with the
stand-ins above. Actor HP remains20, XP remains0 in actual defeat semantics.
The probe separately called `giveExperience` on that condition set as a predicate
control: only owners0 and18 receive250 each. **That artificial call is not a
defeat reward or part of the production trace.**

Additional S injury control extends round1 above, with Rebecca already disabled:
in round2 Tyro attacks and misses (weapon dice1,1, d20=1), all other eligible
owners Block. Enemy target `U[0,5]=1`, d20=18, hit-parameter2, dice2,2 hits his
unblocked threshold20 with score21, HP16->12; monster HP stays20, time becomes492.
In round3 all eligible owners Block. Target `U[0,5]=1`, d20=20, first dice6,6,
hit-parameter4 and second dice5,5 give HP12->0->-10; time becomes493.
`maxHP16 + (-10)=6`, so he is Unconscious
with broken equipped armor, not Dead. This distinguishes the -10 break threshold
from death. Seymour's first critical application in the table independently
shows armor breaking before the second hit's AC lookup. Future tests must inspect
the two ordered damage subresults and original raw armor bytes, not just total HP.

E XP control on a separate synthetic condition state: four Good, Rebecca
Unconscious, Seymour Dead yields count5, `250/5*2=100` for each of the first five
and zero for Seymour. S/E establishes that Unconscious participates in XP and
Dead does not. With six eligible, `250/6=41`, then World/level<15 doubles to82;
standalone Clouds would leave41. No automatic level/training call follows.

### Proposed seeded production controls (M, E for loss scheduling)

The seed algorithm is specified below, not a ScummVM seed-replay claim. Seed1,
original equipment: Block for the six round-1 players, then Attack for every
eligible displayed player. The independent model gives these literal observations:

```text
R1 enemy: d20=10, hit=2 -> miss; time491.
R2 owner0 dice2,2,2,1 hit3 -> miss; owner18 dice1,3 hit13 -> 8 damage.
   owner14 dice1,3 hit3 -> miss; owner11 dice1,2 hit14 -> 5 damage.
   owner1 die5 hit5 -> miss; owner6 die1 hit18 -> 0 damage after resistance.
   enemy owner1: d20=20, dice6,2, hit4, dice1,2 -> HP7->-1->-4,
   Unconscious, no broken armor; time492; monster HP7.
R3 owner0 dice2,1,2,1 hit4 -> miss; owner18 dice2,3 hit1 -> miss;
   owner14 dice3,1 hit15 -> 7 damage -> lethal; XP82 each including owner1;
   Victory time493; final HP [12,16,12,10,-4,5].
```

Seed19, remove **all Armor-category equipment** in preparation, leave weapons
and accessories unchanged, then Block every eligible displayed turn. The model
and extracted pinned scheduling/target/HP probe agreed: 35 Block actions,
11 enemy turns, defeat at500, HP `[-7,-1,-1,-5,-3,-7]`; first five Unconscious,
Seymour Dead. No armor is equipped to break and no XP is awarded. Enemy requests
by round (target rolls first, `h`=U[1,4], `d`=two U[1,6]):

```text
1 pref1: r19 h1 d6,4
2 U[0,5]=0: r20 d6,4 h4 d6,3
3 U[0,5]=1: r15 h2 d4,4
4 U[0,5]=1: r19 h2 d6,3
5 U[0,5]=1 U[0,2]=0: r2 h4 (miss)
6 U[0,5]=4 U[0,2]=0: r16 h2 d2,4
7 U[0,5]=4 U[0,2]=1: r19 h2 d1,5
8 U[0,5]=2: r17 h2 d1,6
9 U[0,5]=3: r16 h1 d4,5
10 U[0,5]=3 U[0,0]=0: r18 h4 d1,3
11 U[0,5]=2 U[0,0]=0: r17 h4 d2,6
```

Per-round damaged owner HP is respectively `1:-3,0:-7,18:8,18:-1,unchanged,
14:6,11:4,14:-1,11:-5,6:1,6:-7`. Rounds1..10 charge one minute afterward;
round11 ends in defeat before the enemy is marked gone, so no round reset/time.
These are reproducible reference/model expectations, **not yet production
keystroke guarantees**. Stage 27C must match both through real Flow/services
before giving the recipe to the maintainer as a verified production control.

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

Add only a focused combat domain (`XeenCombat` is a proposed name) and extend the
existing encounter coordinator. Proposed operations: `prepareSessionInputs`,
`beginApproach`, `beginCombat`, `command(Attack|Block, turnTicket)`,
`advanceAutomatic(workTicket)`, and `stopCurrent(ticket, reason)`. They borrow
the existing owners and return fixed typed results: Accepted, Refused, Stale,
Pending, Advanced, Victory, Defeat, SupportStopped or Failed, with old/new
revision, acting/target identities, ordered damage facts, condition/item deltas,
XP receipt and time delta as applicable. Resource queries and RNG interfaces are
injected; no callback is stored in a result and no public apply-prepared-result
API lets a caller publish against a different owner.

| Operation | Required input/phase | Publication and supplied result |
| --- | --- | --- |
| `prepareSessionInputs` | Marked Diagnostic27, fresh roster, checked initial CHR/provider | Install six supplemental records once; return admitted input metadata or startup failure, never partial interactive state |
| `beginApproach` | Current closed-preparation ticket, original item conservation and supported loadout | Close selections, invoke one-time M26 initialization; return approach result at480 with original live actors |
| `beginCombat` | Retained current Engaged approach ticket and record5 identity | Advance shared revision, consume handoff, install combat progress; return first participant and target |
| `command` | PlayerReady ticket including owner/index/target, Attack or Block | Install one consumed intent, then publish prepared action and next-work kind; fixed ordered damage/XP facts, no externally applicable candidate |
| `advanceAutomatic` | Exact pending kind/id, revision/generation and due service token | Finish only that random prefix, enemy, round or end transition; return next pending/ready/terminal state |
| `stopCurrent` | Retained ticket and typed reason | Stop only still-current nonterminal work; preserve already published terminal outcome and all prior effects |

Results use bounded value storage: at most two enemy damage subresults, six XP
recipient entries and nine armor-slot changes for the targeted character. They
own IDs and scalar facts, not borrowed vectors or pointers. Failed/Refused/Stale
results do not grant continuation authority; only the coordinator's newly adopted
pending work can do so. Detailed text is formatted later from these values.

`XeenActorLifecycle::Defeated` is the proposed additional explicit actor state.
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
state. Proposed reproducible generator: xorshift32, unsigned operations
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

The proposed supplemental payload belongs to `XeenRoster`, outside the existing
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

R: `008.att` has four frames. C: all four passed the current
`validateXeenObjectSprite` compiled directly from the baseline source, including
their complete row streams. The resource's frame cells are:

| ATT frame | Cell offset and `(x,width,y,height)` |
| --- | --- |
| 0 | 18 `(0,250,21,129)`; 2618 `(0,250,0,140)` |
| 1 | 18 `(0,250,21,129)`; 8244 `(0,250,6,135)` |
| 2 | 18 `(0,250,21,129)`; 12853 `(0,250,21,120)` |
| 3 | 17317 `(0,250,26,124)` |

S: monster logical frames0..7 select MON; 8..11 select ATT frame-8. Enemy attack
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
No general particle/audio system is needed. M27 may omit weapon POW overlays,
portrait flash and voice/sound; original MON/ATT and accurate readable text/party
injury feedback are required. Structural safety and source placement are obtained
evidence; actual composited attack-frame readability/clipping remains F original-
data and physical acceptance. Do not claim raster acceptance from the validator.

## Proposed implementation units

Three units provide separate acceptance boundaries. 27A is accepted below;
27B and 27C remain specifications for separate authorization. Each must preserve
ordinary/M26 regressions; no unit alone is M27 completion.

### 27A - Authoritative bounded combat domain

#### 27A acceptance

**27A is accepted following independent review.** It delivers the authoritative
headless bounded combat domain with existing world, party, roster and camera
owners. Supplemental combat inputs and XP belong to irreversible marked roster
state. The one-time M26 Engaged-to-combat handoff preserves the terminal latch
and shared world revision authority.

Accepted behavior includes Attack/Block, mandatory enemy work, injury and armor
breakage, deterministic gameplay RNG, retained continuations, atomic removal/XP,
VictoryAwaitingEnd followed by separate Victory, Defeat and bounded time
transitions. Fixed inert results retain operation, actor, target and outcome
facts for later production Flow. Direct persistence guards protect encounter
authority in world, context and roster, including detached marked rosters;
ordinary v1/v2 persistence and Diagnostic26 behavior remain unchanged.

The independent-review findings concerning stale Diagnostic27 authorization
publishing an approach stop and missing attack-outcome/target observations were
corrected and independently re-reviewed before approval. The configured build,
complete CTest (77/77), focused regressions (13/13), original-data combat smoke,
M26 domain and production/dummy-SDL regression smokes, and whitespace checks
passed. Original-data controls reached Victory 492, Defeat 495, Victory 493,
Defeat 500 and delayed/ring Victory 503. The last control enters combat at 500,
completes two nonterminal rounds to 502, then completes the victory-end minute
at 503.

This acceptance is headless/domain-level. Physical M27 combat acceptance has
not occurred and is not required for 27A. M27B retains production Preparation,
CLI/SDL coordination and presentation; M27C retains original MON/ATT appearance
and final physical acceptance of the combined implementation. **M27 remains
open.**

#### Domain integration and downstream consumer contract

The headless boundary uses one noncopyable `XeenCombat`, borrowing the existing
world, party and committed camera. The world privately binds Diagnostic27 to
that coordinator and its retained approach state once. This discriminator is
separate from the public production entry enum. Destruction does not clear the
binding or roster marker. Public Diagnostic26 initialization/action/stop calls
cannot acquire the reserved Diagnostic27 state.

Delegated approach publication, including failure stops, retains both M26
authorization and the Diagnostic27 ticket's boundary generation. A stale
Diagnostic27 callback cannot stop or retire M26 pending work. Current domain or
provider failures retain the existing stop semantics; ordinary Diagnostic26
requires no additional coordinator authority.

Preparation uses coordinator tickets and calls the existing M24 transfer and
M25 equipment operations. Its exact preimage certificate starts with the loaded
CHR and adopts only their fixed array/frame publications. This establishes
duplicate quantities, carrying, compaction and legacy-frame reachability without
an item-identity heuristic or a second inventory. Preparation leaves world
revision zero; successful Begin calls real M26 initialization, publishing one.
The certificate is validation evidence and is never assigned back to live owners.

`XeenCombatBoundary` is bound to the same owner addresses. Generation-bearing
leases represent inventory, armed certificates, events, rewards and irreversible
presentation failure. Preparation mutations allow an open inventory but require
other authority to be absent. Begin/handoff/combat require the closed, quiescent
boundary. M27B must bind these leases to actual Flow work, consume the displayed
ticket, and preserve adopted fixed results before any reporting or composition.
The headless API does not inspect an unrelated Flow or accept a caller's boolean
assertion that it is safe. Explicit owner/array replacement, including identical
bytes, calls `invalidate()` before subsequent work; equality does not detect ABA.

Supplemental inputs remain inside `XeenRoster`. Marked roster and enclosing-party
copy/move construction, assignment and ADL/explicit `std::swap` refuse before
changing either operand. Ordinary values retain their existing use. Save restore
alone has private, prevalidated, nonthrowing ordinary publication; neither that
publication nor world overlay publication exchanges encounter markers. Direct
capture and both restore graphs check world, context and roster authority around
resource/preflight callbacks. No ordinary codec fields or versions change.

`command` consumes a player intent before random preparation; `service` handles
one automatic publication or at most 64 draws. A value cursor and one bounded
candidate retain rejection/explosion prefixes without advancing live RNG. Tickets
bind coordinator, generation, shared revision, phase, pending kind and boundary
generation. Fixed results and retained preparation receipts are observations,
not publication or continuation capabilities. A separate current `fail` ticket
cannot rewrite Victory/Defeat; stale failure cannot replace newer authority.

Fixed combat results identify the operation, acting roster owner or monster,
selected target, and typed attack outcome (miss, zero-damage hit or positive hit).
Pending attacks are explicit, and Block/non-attack transitions have no attack
outcome. Critical observations retain the selected target, critical flag and
ordered injury subresults. These owned facts remain usable after later turns or
owner destruction without replaying rules or inspecting gameplay RNG.

M27B must host this same coordinator, retire approach deadlines on the typed
Engaged-to-PlayerReady transition, and borrow const owners or scalar observations
instead of copying a marked party through Diagnostic26's `observedParty` path.
No public CLI, production clock, SDL action, preparation UI or appearance is
part of this domain contract. M27C retains MON/ATT and physical acceptance.

Deliver a headless, independently testable encounter from a retained M26 Engaged
state through original Attack/Block turns to once-only removal/XP or defeat.
This includes combat-input parsing/roster attachment, loadout/reachable-state
admission, shared revision handoff, all rules, exact RNG cursor/continuation seam,
turn/round/end time, failure/publication and direct save/restore guards. It is
not a statistics-only stage and has no public production entry yet.

Necessary reading: this full plan, M26 ownership/action publication, M25 equipment,
M24 transfer, M20/M21 persistence and the pinned combat/character/interface/party
symbols above. Likely touched modules: formats/character and monster accessors,
`XeenRoster`/party, `XeenCharacterRules`, actor/world private authority,
`XeenActorApproach` handoff integration, proposed `XeenCombat`, and
`XeenSaveState`; focused synthetic tests and optional original-data smoke target.
Do not alter ordinary codecs or implement rendering, M28 or other combat verbs.

Supply fixed typed action/automatic results, tickets, read-only presentation
observations and deterministic RNG control for later Flow. Keep HP/items in
existing characters and RNG/turn progress in one coordinator. Tests must cover
complete mixed victory and Block defeat, not only helpers; fixed literals and
the extracted/source traces supply expected values. Original-data tests compare
all active inputs/item kinds, ring/rearrangements and unchanged bystanders.
Stop after domain and capture/restore boundary pass review; production SDL still
stops at M26 and must not advertise playable M27.

### 27B - Explicit production preparation and combat coordination

Depends on accepted27A interfaces. Deliver proposed CLI and typed Diagnostic27,
real preparation via existing inventory/transfer/equipment, single-use Begin,
automatic engagement handoff, tickets across Application/services/Flow/SDL,
mandatory idle-serviced enemy/round work, mode-specific controls, in-frame textual
outcomes and irreversible F9 refusal. Use normal actor presentation initially;
label attack appearance/physical acceptance unfinished until27C.

Necessary reading adds `XeenGameplay`, `Application`, `XeenEventFlow`,
`XeenInventoryFlow`, `XeenEncounterFlow`, `XeenGameplayServices`, `PlayerAction`,
SDL event batch/handoff code and M22 timing. Extend those owners instead of
introducing another app or loop. Supply appearance/result observations to27C.
Exercise actual services/Flow with callbacks that inspect phase, result, ordinary
phase, deadlines, owner tickets and buffers. Cover preparation cancellation,
transfer/equip generations, direct routing bypasses, strict CLI, buffered input,
every presentation-failure boundary and zero-save-side-effect assertions.
Optional original-data headless runs must play real commands to both outcomes.
Stop at reviewed production coordination; do not close the milestone yet.

### 27C - Original combat appearance and complete acceptance

Depends on accepted27A/27B. Deliver MON/ATT selection, slot relocation/clipping,
bounded cosmetic sequences and readable final combat UI through the existing
scene/assets/cache path. Likely modules: `XeenOutdoorScene`, `CloudsMapComposer`,
`XeenAssetSource`, `ScummVmXeenBridge`, encounter feedback and integration tests.
No new combat formulas/owners or audio/effects framework. Verify any necessary
interface adjustment against already accepted ticket/publication contracts.

Add original-data image/composition and deterministic production traces, then
independent review, full build/CTest and maintainer physical victory/loss/regression
acceptance. Prove cache discard preserves partial HP and once-only completion.
Only this combined boundary may establish M27 complete, followed by separately
authorized closure documentation/Git actions. With 27A accepted, the next unit
is **27B**, requiring separate explicit implementation authorization.

## Requirement-to-evidence and acceptance plan

Synthetic ordinary CTest must remain independent of commercial resources.
Expected values must be literal arithmetic/bytes and checked RNG request tapes,
not values generated by the production helper under test.

| Contract | Obtained evidence | Required future verification |
| --- | --- | --- |
| Baseline, accepted seams and M26 terminal | C source/tests + committed docs | Unchanged ordinary/M26 suites and physical pre-combat mode |
| CHR/MON inputs, item domain and profile | R offsets/tables, S layouts | Truncation/invalid enum/bounds; all originals and representative legal transfers/equips; reject changed/opaque domain without altering ordinary storage |
| Ring/bow/bare/bad items | R item list, S literal scans | Ring on every eligible owner, tie changes/AC; bow-only/unarmed zero damage; broken armor; bad-weapon helper source quirk or explicit narrowed helper contract |
| Participant order/skip/Block/retaliation | E nextChar/speed/enemy functions, S outer caller | Exact order/ties, initial selection, flags reset, lethality before enemy, no-input automatic continuation |
| Player hit/damage and resistance | E weapon/hit/stat functions, S application formula and M traces | Literal dice-before-hit tape, exploding20, natural1, zero damage, rounding, original weapons/difficulty; no hidden RNG |
| Enemy critical/injury/breakage | E bounded physical path + S full function | Separate subcalls, post-first-hit AC, negative/zero HP, simultaneous conditions, exact raw armor bytes, death versus unconscious break |
| Victory/removal/XP/defeat | E eligibility/defeat, S attack2, M victory | Full real lethal path; partial/terminal cache rebuild; original identity and all bystanders; recipient denominator/multiplier; no training/loot |
| Publication/reentrancy/RNG | C retained M26 model, proposed explicit contracts | Current versus stale failure at every callback; copied results and cursor prefixes; repeated/refused inputs; overflow; no reroll or duplicate award |
| Time and boundary | S connected doCombat/changeTime; M/E driver distinctions | 490/500 entry, round+1/end+1, defeat0, ctr24 unchanged, 959 boundary after prior effects, zero condition RNG before960 |
| Preparation/CLI/save isolation | C existing inventory/save/entry seams | Real M24/M25 workflow and generation invalidation; all forbidden combinations; F9 before capture/target/I/O in every phase; direct roster/context/world guards |
| Original frame safety/placement | R ATT dimensions, C validator, S draw relocation | All normal/attack frames, order118/121, exact anchor/scale/clipping, same-cell readability and retained initial forest occlusion |
| SDL input/results/continuations | C existing single-loop tests | Repeat/batch/generation tests; rapid presses; no arbitrary idle turns; no modal erasure of enemy work; readable injury/XP/terminal feedback |

Extend the existing encounter test support deliberately: its synthetic characters
do not contain original combat attributes/classes/items, and its monster records
mostly set HP/image. They are not combat fixtures merely because M26 accepts
them. Composition fakes must observe every consequential new appearance, phase,
ordinary-phase, deadline and feedback input. Separate synthetic domain tests,
Flow/service tests, optional original-data checks, headless SDL/images and physical
acceptance. Existing targets include `xeen_encounter_flow`,
`xeen_encounter_gameplay`, `xeen_encounter_save`, actor approach tests and
`mmodern_encounter_domain_smoke`/`mmodern_encounter_smoke`. Accepted 27A coverage
adds `xeen_combat`, `xeen_combat_authority`, `xeen_combat_persistence` and
`mmodern_combat_smoke`; production combat verification remains future work.

Test failure injections before acceptance, during RNG prefixes, before each
publication, after nonlethal damage, lethal accounting, end time and terminal
notice, plus resource/clock/reporter/composer/frame-copy/recovery/SDL handoff
reentrancy. Verify exact owner identity/revision and typed result, not merely
frame existence. No stale callback can stop a newer session/action or overwrite
terminal outcome. No failed redraw can heal, respawn or repeat XP. Ordinary
M20-M25 save/item/event/animation behavior and unchanged Diagnostic26 must pass.

### Future physical recipe

Before execution, identify the authorized implementation SHA, compiler, selected
CMake build directory, pinned ScummVM source and configured library build. Build
that exact source and verify the optional original-data production tests match
the seeded traces above. Do not select a binary by modification time. All paths
and launch syntax below are proposed until27B/27C exist:

```text
<verified-build>/mmodern.exe --encounter-27 --combat-seed 1 "<game-directory>"
<verified-build>/mmodern.exe --encounter-27 --combat-seed 19 "<game-directory>"
<verified-build>/mmodern.exe --encounter-26 "<game-directory>"
```

1. Seed1 victory: keep original equipment. Confirm Preparation/unsaveable notice,
   F9 refusal and that idle has no actors moving. Enter at the closed preparation
   screen; confirm accepted map20 entry. Period Wait engages at490. Block each
   displayed first-round character using B; observe mandatory enemy miss and
   round/time change. Then use Space on each fresh ready frame until Victory.
   Observe original attack appearance, actor/target changes, damage, Rebecca's
   injury and actual XP. Expected verified control is round3/time493, +82 each,
   Rebecca HP-4 Unconscious. Do not substitute setting HP/XP or scripting Victory.
2. Seed19 loss: in Preparation use I, F1-F6 and the Armor category. For each
   occupied equipped armor slot, explicitly select it then E to remove; do not
   transfer/discard or alter weapons/accessories. I from Browse closes inventory.
   All original armor records remain carried with frame0. Enter from closed
   Preparation, then period. B on each fresh ready frame produces the
   verified35-action control; observe forced retaliation,
   injury, skipped owners and Defeat at500, without XP or healing. No random wait
   for a rare critical/breakage event is part of physical acceptance.
3. Separately verify the preparation ring can be equipped/transferred through
   M24/M25 operations with the Diagnostic27 cancellation adaptation and changes
   the displayed combat order as specified. In separate preparation control
   sessions, verify the [cancellation contract](#production-entry-and-preparation):
   I from ChooseDestination and Confirm returns to Browse without closing, the
   next I closes, and N still cancels Confirm. Check truthful I/N feedback,
   invalidated transfer confirmation/selection certificates, no owner/item
   mutation or queued Begin on cancellation, and Enter in Confirm confirming only
   the transfer. Verify
   Escape exits the entire session even with preparation inventory open.
   Check delayed approach still engages at500 and no preparation action is
   available after Begin. These are control sessions, not the seeded recipe.
4. In both terminals, F9 refuses; Space/B/Enter/I/navigation cannot restart or
   return to exploration. Escape/window close exits. Verify forbidden CLI
   combinations refuse without touching save paths.
5. Launch unchanged26: initial forest occlusion, period terminal engagement,
   diagnostic envelope/terminal/F9 controls remain accepted. Check relevant
   ordinary navigation, events, inventory/equipment and save/restart controls.

Physical evidence establishes a real readable exchange and terminal behavior.
Deterministic automated oracles establish rare critical/breakage, arithmetic,
failure, stale input and once-only boundaries. Both are required, with independent
technical review and full CTest at closure. A helper, static combat image or
unopposed damage demonstration cannot close M27.

## Review readiness and replanning triggers

The [accepted 27A boundary](#27a-acceptance) establishes the headless domain and
persistence protections. Later production coordination, MON/ATT composition and
physical acceptance must satisfy their separate contracts above; headless
evidence does not establish those results.

Reopen only for evidence: changed original resources/item domain, contradictory
connected reference execution, extra actor admission, an item/condition/time
branch outside this coverage, an unsafe handoff/publication/RNG/save boundary, or
failure of original ordered composition to provide readable engagement. Report
the smallest affected contract and required scope approval; do not silently
change the anchor, freeze bystanders, omit an item effect or expand into M28.

M27B and M27C implementation, milestone closure and Git publication each require
separate authorization. Acceptance of 27A does not authorize those actions.
