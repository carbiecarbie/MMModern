# Combat and character capacity

This chapter describes the pinned reference's rules and the explicitly identified
DOS confirmations below. **Reference only is not a claim of original DOS
verification.** MMModern implementation coverage is narrower than this catalogue.
The reference is ScummVM commit
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`; local implementation cross-checks use
MMModern `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`.

`R(a,b)` means one inclusive random draw. Integer division truncates toward zero;
for the nonnegative examples below this is floor division. Distinguish a monster's
resource ID from its map actor index: saving throws use the resource ID. A
character's current level is `L = max(permanent level + temporary level, 0)`.

## Sources and evidence boundaries

**Evidence: Reference only, except the DOS-static findings explicitly
identified in the spell interaction and treasure sections.** Source links are pinned, so future
upstream changes cannot silently change this chapter.

| Topic | Pinned source and symbols |
| --- | --- |
| Player attack | [combat.cpp, lines 1197-1590](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L1197): `Combat::attack`, `attack2` |
| Hit and weapon calculation | [combat.cpp, lines 1636-1805](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L1636): `hitMonster`, `getWeaponDamage`, `getMonsterDamage`, `getMonsterResistance` |
| Enemy attack | [combat.cpp, lines 807-967](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L807): both `doMonsterTurn` overloads |
| Damage and secondary effects | [combat.cpp, lines 160-461](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L160): `giveCharDamage`, `doCharDamage` |
| Attribute, HP, SP, AC, saves | [character.cpp, lines 285-554](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp#L285): `getMaxHP`, `getMaxSP`, `getStat`, `statBonus`, `charSavingThrow`, `getArmorClass`, `getCurrentLevel` |
| Capacity tables | [constants.cpp, race tables](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp#L259) and [class/stat tables](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp#L499): `RACE_HP_BONUSES`, `RACE_SP_BONUSES`, `BASE_HP_BY_CLASS`, `STAT_VALUES`, `STAT_BONUSES` |
| Injury | [character.cpp, line 1264](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp#L1264): `subtractHitPoints` |
| Rewards | [combat.cpp, line 1807](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L1807): `giveExperience`; [party.cpp, line 701](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp#L701): `giveTreasure`, `arePacksFull` |

## Attribute bonus, HP, SP and armor class

**Evidence: Reference only.** These formulas use effective attributes, after age,
equipment, temporary modifiers and conditions. Do not copy ScummVM's condition
modifier implementation wholesale: retained DOS evidence establishes different
Insane, In Love, Drunk and unsigned-counter rules; see
[original quirks](../original-quirks.md#reference-behavior-not-reproduced).

### Attribute conversion

`B(s)` is the bonus corresponding to effective attribute `s`. It is a step table,
not `(s - 10) / 2`.

| Attribute | Bonus | Attribute | Bonus | Attribute | Bonus |
| --- | ---: | --- | ---: | --- | ---: |
| 0-2 | -5 | 17-18 | 3 | 75-99 | 11 |
| 3-4 | -4 | 19-20 | 4 | 100-124 | 12 |
| 5-6 | -3 | 21-24 | 5 | 125-149 | 13 |
| 7-8 | -2 | 25-29 | 6 | 150-174 | 14 |
| 9-10 | -1 | 30-34 | 7 | 175-199 | 15 |
| 11-12 | 0 | 35-39 | 8 | 200-224 | 16 |
| 13-14 | 1 | 40-49 | 9 | 225-249 | 17 |
| 15-16 | 2 | 50-74 | 10 | 250 or more | 20 |

The last table entry is a catch-all. There are no bonuses 18 or 19. Calculating
an effective attribute clamps its final result to at least zero.

### HP capacity

`maxHP = max(max(class HP + B(Endurance) + race HP + Bodybuilder, 1) * L
+ equipment HP, 0)`.

Bodybuilder contributes 1 if the skill is present, regardless of its count.
The minimum per-level amount is applied before multiplying by level. Equipment
HP is added afterward. This is a recomputed capacity, not a random HP gain on
level-up.

| Class | HP per level | Melee attack divisor | Hit level divisor |
| --- | ---: | ---: | ---: |
| Knight | 10 | 5 | 1 |
| Paladin | 8 | 6 | 2 |
| Archer | 7 | 6 | 2 |
| Cleric | 5 | 7 | 3 |
| Sorcerer | 4 | 8 | 4 |
| Robber | 8 | 6 | 2 |
| Ninja | 7 | 5 | 2 |
| Barbarian | 12 | 4 | 1 |
| Druid | 6 | 7 | 3 |
| Ranger | 9 | 6 | 2 |

| Race | HP adjustment | Intellect SP adjustment | Personality SP adjustment |
| --- | ---: | ---: | ---: |
| Human | 0 | 0 | 0 |
| Elf | -2 | 2 | 0 |
| Dwarf | 1 | -1 | -1 |
| Gnome | -1 | 1 | 1 |
| Half-orc | 2 | -2 | -2 |

Example: level-5 Dwarf Knight, Endurance 15, Bodybuilder and +7 equipment HP:
`(10 + 2 + 1 + 1) * 5 + 7 = 77` maximum HP.

### SP capacity

A character without the reference's `hasSpells` flag returns zero immediately,
including before equipment SP. For each applicable attribute, calculate
`max(B(attribute) + 3 + race SP + skill bonus, 1) * L`.
The relevant skill adds 2 when present.

| Class | Attribute | Skill | Subsequent operation |
| --- | --- | --- | --- |
| Sorcerer | Intellect | Prestidigitation | Full amount |
| Archer | Intellect | Prestidigitation | Divide by 2 |
| Cleric | Personality | Prayer Master | Full amount |
| Paladin | Personality | Prayer Master | Divide by 2 |
| Druid | Personality and Intellect | Astrologer | Calculate both, then average with integer division |
| Ranger | Personality and Intellect | Astrologer | Halve each separately, then average with integer division |

Add equipment SP after these operations and clamp to zero. For Ranger, averaging
first and halving later is not equivalent for every combination of odd amounts.
Example: level-3 Human Ranger, Personality 13, Intellect 15, no Astrologer:
Personality gives `4 * 3 / 2 = 6`; Intellect gives `5 * 3 / 2 = 7`;
maximum SP is `(6 + 7) / 2 = 6`, before equipment.

### Armor class

`AC = max(B(Speed) + equipment AC + party Blessed + temporary AC, 0)`.
The sheet's base-only variant omits Blessed and temporary AC, but still asks for
the effective Speed attribute. Combat Block adds a separate hit threshold; it
does not change this stored or displayed AC formula.

## A character's melee attack

**Evidence: Reference only.** Symbols: `attack`, `hitMonster`, `getWeaponDamage`,
`getMonsterDamage`, `attack2`, `getMonsterResistance`.

The action makes `floor(L / melee attack divisor) + 1` attempts against its
selected monster, then applies one accumulated damage result. Each attempt rolls
weapon damage before its hit roll, including attempts that miss.

1. Scan weapon slots in inventory order. Melee includes frames 1 and 13;
   ranged includes frame 4. Base dice come from `WEAPON_DAMAGE_BASE` (number of
   dice) and `WEAPON_DAMAGE_MULTIPLIER` (sides), despite the table names.
2. A good elemental material below 37 selects the eventual elemental bonus.
   A good metal material 37-58 assigns its metal hit modifier and metal damage
   modifier, then the dice are added. Attribute materials do not supply a direct
   metal or elemental component. Heroism is added to the hit modifier for each
   matching weapon slot. A later metal weapon can overwrite accumulated damage
   and hit modifiers; do not replace the scan with an unordered sum.
3. Roll each matching weapon's dice as `R(1, sides)`. Clamp the resulting weapon
   damage to at least zero. Adventurer adds 5 to hit and triples this weapon
   amount; it does not triple the later Might bonus.
4. Set hit total to `B(Accuracy) + weapon hit modifier + floor(L / hit level
   divisor) - Cursed count`. Add `R(1,20)`, drawing and adding again after each
   20, until the first non-20. There is no automatic player miss on 1.
5. Add 20 if the monster's current damage/status type is not Physical. Hit if
   the total is at least `monster AC + 10`.
6. On a hit, add `max(B(Might) + weapon damage, 1)` to the action total. On a
   miss, add zero. Repeat from weapon calculation for the next attempt.
7. Scan all equipped weapons for matching effectiveness counters. Dragon,
   Undead, Golem, Insect, Monsters or Animal effectiveness each multiplies the
   accumulated total by 3 when the monster's type matches. Then apply the
   downstream damage rules below.

**Bad-item boundary:** the reference suppresses material processing for cursed or
broken weapons but still rolls the matching slot's base dice and adds Heroism.
Its effectiveness scan tests only `frame != 0`, including equipped ranged and bad
weapons. These are reference observations awaiting DOS confirmation; they are not
permission to invent item restrictions. See the question list at the end.

Example, with no material or party effects: a level-10 Knight has three attempts.
At Accuracy 15 the static hit bonus is `2 + 10 = 12`; against AC 8, a first d20
of 6 hits exactly. At Might 15, weapon dice totaling 7 contribute 9 damage in
Warrior and 23 in Adventurer (`7 * 3 + 2`). If all three attempts hit and roll 7,
the totals before resistance are 27 and 69. At 25% physical resistance they
become 20 and 51, respectively.

### Applying weapon damage

**Evidence: Reference only.** A zero accumulated input skips the entire positive
input branch: no elemental addition and no monster saving throw.

For an ordinary positive melee input, physical resistance applies first:
`physical = total * (100 - physical resistance) / 100`; resistance 100 sets it to
zero. This reduction is bypassed if the selected attack weapon ID is at least
Xeen Slayer Sword (34). The source also has explicit Lord Xeen handling; see below.

Add the elemental weapon contribution once per applied action, after physical
resistance. It is `ELEMENTAL_DAMAGE[material] * (100 - elemental resistance) / 100`,
with resistance 100 yielding zero. Material groups select Fire 1-8,
Electrical 9-15, Cold 16-20, Poison 21-25, Energy 26-33, or Magic 34-36.
The contribution is not multiplied by Adventurer, number of successful attempts,
or effectiveness. A positive input can therefore be reduced to zero physical
damage and still do elemental damage. Melee does not draw the monster save.

Ordinary shooting performs one hit attempt per still-flying shooter at each
visited target, uses weapon damage without Might, and enters the same damage
application as `RT_HIT`. A miss can continue to a later target; a hit clears that
shooter's flight row. Geometry and range still limit which targets are visited.
Unlike melee it draws `R(1, 50 + resource ID)` and halves damage when the result is
at most the resource ID. That halving is after physical and elemental components
have been combined. No melee multi-attack count is used for each arrow.

Example: an arrow supplies 11 physical damage against 25% physical resistance,
plus 5 elemental damage against 40% corresponding resistance. The pre-save result
is `floor(11 * 75 / 100) + floor(5 * 60 / 100) = 8 + 3 = 11`; a successful
monster save reduces it to 5.

### Spell interactions and retained DOS evidence

**Evidence: DOS static (retained evidence) for the three explicitly named rules;
Reference only for general pipeline context.** These are summaries of already
recorded analysis, not fresh binary verification by this chapter.

- Monster elemental resistance does not reduce the base damage of elemental
  spells: `getMonsterResistance` starts its spell-path contribution at zero.
  Retained `WORLD/XEEN.DAT` offsets `0x2c675`, `0x2c70c`, `0x2c873`, `0x2bf8a`
  confirm this. Saving throws remain separate.
- Magic Arrow retains the monster saving-throw draw even though the result cannot
  change its 8 damage. Retained offsets `0x30fa8`, `0x2cde4`, `0x2c037`, `0x2c59a`.
- Pain prepares 8 physical damage in DOS, while the pinned spell implementation
  prepares zero. Retained offsets `0x33558`, `0x2f5e9`, `0x2cd81`.

All three findings and their implementation locations are recorded in
[original quirks](../original-quirks.md). Spell admission, target selection and
spell costs must be consulted separately; these exceptions do not establish all
spell behavior.

## Monster targeting and hit resolution

**Evidence: Reference only.** Symbols: both `doMonsterTurn` overloads. A monster's
`numberOfAttacks` repeats target selection and resolution; `strikes` is instead
the number of damage dice within one successful damage application.

For each attack, hatred Party (15) attacks every combat participant, including
disabled and dead members. Otherwise the reference selects the first eligible
member of a hated class or the first eligible Dwarf for hatred 12. Class value 1
(Paladin) bypasses the preference logic entirely. Eligible here excludes worst
conditions Paralyzed, Unconscious, Dead, Stoned and Eradicated; Sleep is eligible.

When no preferred target exists, draw a uniform index over all current combat
participants, including disabled ones. If that choice is ineligible, draw again
over an ordered list of eligible participants. This second draw is conditional;
pre-filtering before the first draw changes RNG consumption. If no eligible
participant exists, the reference marks party defeat.

Nonphysical monster attacks and attacks against a sleeping target bypass the AC
hit roll and enter damage directly. For physical attacks against awake targets:

1. Draw `d = R(1,20)`. A 1 misses immediately, without a hit-parameter draw.
2. A 20 immediately performs a complete damage application. It then continues
   to the following step rather than returning.
3. Draw `R(1, monster hitChance)`, add `floor(hitChance / 4)` and the original d20.
4. Compare to `AC + 10`, or while Blocking, `AC + floor(L / 2) + 15`.
   Equality hits. A passing result performs a complete damage application.

A natural 20 can thus damage twice. The first application occurs before the
hit-parameter draw and before the second AC calculation. If it breaks armor,
the second check sees the reduced AC. This is already represented by MMModern's
`XeenEnemyAttackCandidate` and `XeenCombatTests.cpp`, but those tests establish
implementation behavior, not independent DOS evidence.

Example: AC 8, level 10, monster hitChance 12. An unblocked threshold is 18;
Block raises it to 28. A d20 of 12 and hit-parameter draw of 7 give
`12 + 3 + 7 = 22`: hit unblocked, miss while Blocking. A d20 of 20 always performs
its first damage application regardless of either threshold.

## Monster damage, saving throws and secondary attacks

**Evidence: Reference only**, except that the resistance save shape has supporting
retained DOS evidence in the existing Poison/Disease analysis. Applying that
shape to every attack caller remains a reference-derived statement.

`doCharDamage` first clears Sleep, then rolls `strikes` draws of
`R(1, dmgPerStrike)` and sums them. A Physical attack skips elemental saves.
For any other damage type, the exact order is:

1. Draw one character save and halve damage on success.
2. Subtract the matching party protection for Fire, Electrical, Cold or Poison.
   Magical and Energy do not subtract one of these party protection fields.
3. While damage is positive, draw another save. Halve on success and repeat;
   stop on the first failed save. Even failure of the initial save does not
   prevent entering this second saving loop.
4. Subtract Power Shield and clamp to zero.
5. If positive damage remains and a secondary attack exists, draw a Physical
   character save. A failed save applies the secondary effect.
6. Subtract the resulting HP damage, after applying the secondary effect.

A character's Physical save uses `v = B(Luck) + L` and succeeds when
`R(1, v + 20) <= v`. It is the save against secondary effects, not an ordinary
reduction to every physical hit. An elemental/magical save uses permanent plus
temporary plus equipment resistance `r`, and succeeds when
`R(1, r + 40) <= r`. Party protection is a separate subtraction and is not part
of `r`. At resistance zero, the draw still occurs but cannot succeed.

Example: 23 rolled Fire damage, successful initial save, party Fire protection 3,
then one successful and one failed loop save, Power Shield 2:
`23 / 2 = 11`, `11 - 3 = 8`, `8 / 2 = 4`, then `4 - 2 = 2` damage.

**Do not merge environmental and monster damage algorithms.** `giveCharDamage`
subtracts party protection before its loop, evaluates the saving throw before
checking whether damage is positive, and carries the mutated damage variable
through successive recipients. `doCharDamage` has an initial save, checks positive
damage before each repeated save, and rolls fresh dice per target. The same
nominal damage can therefore produce different outcomes and draw counts.

### Secondary effects

**Evidence: Reference only.** `doCharDamage`, plus
[InventoryItemsGroup helpers](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp#L831).
Every row below requires positive post-shield damage and a failed Physical save.

| Secondary attack | Effect before HP subtraction |
| --- | --- |
| Poison, Disease, Insane, Sleep, Curse, Paralyze, Unconscious, Confuse, Weaken | Increment the corresponding condition; the reference replaces a zero result with -1 |
| Curse Item | Curse all inventory categories; weapon IDs 34 and above are exempt |
| Drain SP | Set current SP to zero |
| Break Weapon | Mark equipped, nonempty weapons with ID below 34 broken |
| Aging | Increment temporary age |
| Death / Stone | Increment the condition and set positive current HP to zero |
| Eradicate | Increment Eradicated, break inventory except weapons with ID at least 34, and set positive current HP to zero |

The reference stores conditions as integers although saved conditions are bytes;
this table alone cannot establish DOS wrap/saturation for extreme counters.
MMModern's admitted secondary counters saturate at 255. That remains separate
from the DOS-confirmed doubling/wrapping behavior of periodic Poison/Disease.
The reference also explicitly retains equipment frames in its Break Weapon and
break-all helpers as a port workaround. Original unequipping behavior needs DOS
verification before extending these abilities; do not treat the workaround as
DOS evidence.

### Injury thresholds

**Evidence: Reference only.** `Character::subtractHitPoints`, with the reference's
optional durable-armor extension disabled.

After subtracting damage, HP at least 1 leaves unconscious/death thresholds
untouched. Below 1, if `maxHP + currentHP >= 1`, set Unconscious to 1; otherwise
set Dead to 1. Thus a character with maximum HP 20 is unconscious at HP 0 through
-19 and dead at -20 or less. The function does not clear a pre-existing terminal
condition when assigning another condition.

Break all nonempty equipped armor if current HP is at most -10, or if this
application reaches the Dead branch even above -10. It sets each broken bit;
it does not discard the items or clear equipment frames. A character with
maximum HP 6 therefore breaks equipped armor when dying at -6.

## Initiative, Block, Run and monster recovery

**Evidence: Reference only.** Sources:
[setSpeedTable](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L1084),
[block and run](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L1593),
[monsterOvercome](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp#L792).

The speed table sorts combat members and the three front monster slots by
descending effective Speed/resource speed, without an initiative roll. Ties keep
insertion order: party members first in combat order, then front monster slots
0, 1 and 2. Speed zero is omitted from this table. The table alone does not grant
an action to a disabled character; action eligibility is handled separately.

Block marks the acting member blocked, changing the later monster hit threshold
as described above. Run draws `R(1,100)` and succeeds only when the draw is
strictly less than the map's chance-to-run field. For threshold 50, rolls 1-49
succeed; threshold 100 is 99%, not guaranteed. Success removes that member from
the combat party and rebuilds the speed order. The map supplies the party's
retreat position; the roll does not select a random retreat cell.

When `monsterOvercome` is called by monster movement, it scans map monsters in
map actor order. Nonphysical statuses except Dragon Sleep draw
`R(1, resource ID + 50)` and reset to Physical if the draw is **at least** the
resource ID. This is a different comparison from `monsterSavingThrow`, which
succeeds when a draw in the same interval is **at most** the ID. Do not unify
these comparisons. A damaged sleeping/dragon-sleeping monster is woken on entry
to `attack2`'s positive-input branch, even if later resistance removes the damage.

## Experience and monster treasure

**Evidence: Reference only.** Symbols: `giveExperience`, the death branch in
`attack2`, `MONSTER_ITEM_RANGES`, `Party::giveTreasure` and `arePacksFull`.

On a kill, XP is divided by the eligible recipient count with integer division.
Combat uses members still in the combat party, excluding those who ran;
exploration uses the active party. Dead, Stoned and Eradicated members receive
nothing, while other conditions do not exclude XP. Each recipient below
permanent level 15 has its share doubled when the engine game ID is not Clouds.
This tests game ID, not the currently loaded Clouds/Darkside resource side.
Example: 101 XP across six eligible members gives 16 each; a qualifying low-level
recipient gets 32. No XP remainder is distributed.

Outside map 85, add the monster's fixed gold and gems to staged treasure. A zero
item-drop field makes no drop-chance draw. A nonzero item-drop field selects a
chance and also supplies the generation level to `Character::makeItem`.

| Item-drop field | Chance from one `R(1,100)` |
| --- | --- |
| 1 | 10% |
| 2 | 20% |
| 3 | 50% |
| 4, 5, 6 | 100% |
| 7-21 | 0% (the chance draw still occurs) |

Generation precedes checking for an empty staged category slot, so generation
RNG remains consumed if staging is full. Weapons, armor and accessories use the
first empty slot of their own category. The pinned Misc branch incorrectly copies
the temporary character's accessory slot into staged accessories, losing the
actually generated Misc item.

**DOS static (2026-10-12), high confidence:** the original instead copies the
generated Misc container, power and state into the separate Misc treasure array.
Its category-3 branch is at file `0x2c4f5`; the three writes are at `0x2c513`,
`0x2c524` and `0x2c535`. See [monster treasure evidence](evidence.md#monster-treasure-staging).
MMModern's `ReferenceMiscellaneousDropLoss` still follows the port defect at this
baseline. Staging is settled; this does not establish all downstream delivery
and full-backpack behavior in DOS.

Treasure delivery scans categories and slots in order. It prefers the Event's
selected recipient when valid, then eligible party members with space in that
category. Its final fallback still excludes disabled/dead members despite a
comment suggesting otherwise. Global backpack fullness is checked before each
slot, including empty slots. Ordinarily it clears the entire remaining staged
treasure, including currency. There is a special branch that can clear the first
member's final weapon slot when the staged weapon at the corresponding slot
index has ID at least 34; this is not a general promise that quest items survive
all full-pack cases. See the existing full-backpack quirk before changing delivery.

### Lord Xeen and scripted death hooks

**Evidence: Reference only; not a complete boss rule.** The Clouds-side branch of
`attack2` zeros non-melee damage against resource ID 89 and adds 100 to positive
melee damage. On its death outside map 85 it stages weapon ID 90, clears its
material/state, and increments quest-item slot 8. This does not by itself prove
which attacks can reach positive melee damage: hit chance, weapon ID and physical
resistance gates remain relevant.

The same death branch has map-and-actor-index hooks for Clouds maps 41, 78 and
29. They are script-progression rules separate from generic monster species
statistics. Consult `attack2` before replacing death handling with only XP/loot.

## Implementation cross-check and remaining DOS questions

**Evidence: implementation inspection plus Unknown for the unresolved DOS
questions.** This section distinguishes current code from the larger reference.

At the inspected MMModern baseline,
[XeenCombatRules.cpp](../../src/games/xeen/XeenCombatRules.cpp) implements admitted
plain-weapon attacks, the two-stage natural-20 enemy application, target selection,
Run, typed consequences and supported spell damage.
[XeenCharacterRules.cpp](../../src/games/xeen/XeenCharacterRules.cpp) owns shared
character capacities. [XeenCombatTests.cpp](../../tests/XeenCombatTests.cpp) checks
ordered critical damage and armor changes;
[XeenCombatAuthorityTests.cpp](../../tests/XeenCombatAuthorityTests.cpp) checks
class attack counts and atomic consequence publication. These are pointers to
existing evidence, not a claim that this research ran those tests or that every
reference ability is playable.

Highest-value unresolved original-program checks:

1. Does the original natural-20 branch also apply damage before a second hit
   check, allowing armor breakage to affect that check? This is already playable
   behavior in MMModern and merits direct calibration.
2. Does original full-backpack treasure delivery discard staged currency and
   apply the same recipient fallback? Misc staging itself is now DOS-confirmed.
3. Which equipment frames survive Break Weapon and Eradicate, and does the DOS
   damage scan include bad weapons' base dice and equipped missile effectiveness?
4. Does the original hatred value 1 bypass Paladin preference? Is the source's
   uniform initial targeting distribution equivalent to the original party-size
   switch for every party size?
5. Are elemental weapon damage and effectiveness applied once after accumulating
   multi-attacks exactly as shown? Establish this before admitting enchanted
   equipment to combat.
6. Verify monster recovery's inclusive `>= ID` boundary, the strict Run comparison,
   and the game-ID criterion for doubled low-level XP in World of Xeen CD.

No DOS-width, overflow or malformed-resource behavior beyond the retained
findings is inferred from ScummVM's wider host integers. Exact timing, monster
movement geometry, all spell effects and item generation tables are outside this
chapter's verified coverage.
