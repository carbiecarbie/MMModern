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
identified in the spell interaction, treasure and monster movement terrain
sections.** Source links are pinned, so future
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

**Implemented (M67), Reference only:** the monster hit threshold, the character
sheet and the injury observations share one equipment scan
(`XeenCharacterRules::equipmentArmorClass`): `ARMOR_STRENGTHS[id]` plus
`METAL_LAC[material - 37]` for good equipped metal armor 37-58, plus the AC
attribute bonus (category 9) of every good equipped weapon, armor or accessory.
Party Blessed comes from the gameplay context; base-only AC omits it. Example: Speed 15 (`+2`), padded armor (2) of
material 45 (`+8`) and a ring of material 117 (`+4`) give AC 16; the same armor
broken gives 6, and material 38 adds 0.

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
weapons. **DOS static, high confidence:** bad-state gate `0x2e057..0x2e05d`
skips material but reaches base dice `0x2e0e9..0x2e134`; effectiveness tests
frame at `0x2c950`, selector at `0x2c96f`, and multiplies at `0x2caed..0x2cb00`.
**Implemented (M67), Reference only for the order:** `XeenPhysicalPlayerCandidate`
applies the metal hit and damage modifiers, the elemental selector, the
effectiveness multiplier (once per matching equipped weapon, bad and missile
weapons included; never in Shoot) and the Slayer Sword bypass below;
`xeenValidateJourneyMelee` admits every material 0-130 and counter 0-6. A
later metal slot assigning rather than adding its damage modifier is
unreachable: one melee weapon (frames 1/13) or one missile weapon (frame 4) is
equipped at a time. Heroism is read from the gameplay context.

M67 examples (Warrior, level-1 Knight, Accuracy 12, Might 15, monster AC 5):

- Long sword (3d3) of material 45 (`METAL_DAMAGE[8] = 10`,
  `METAL_DAMAGE_PERCENT[8] = 6`): dice 2+2+2 give weapon damage 16; the hit
  total is `0 + 6 + 1 + d20`, so 8 hits exactly; damage `16 + 2 = 18`. In
  Adventurer the weapon amount is 48 and the hit total starts at 12: damage 50.
- Short sword (2d3) of material 38 (`-6` damage, `-4` hit): dice 1+1 give
  `-4`, clamped to 0; a hit needs a d20 of 18 and does `max(0 + 2, 1) = 2`.
- Long sword of material 0 with effectiveness counter 4 (Insect) against an
  Insect (type 2): dice 6, `6 + 2 = 8`, tripled to 24; against an Animal 8.
  Two matching equipped weapons give 72; a broken effective weapon still triples.

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

**Implemented (M67), Reference only:** the candidate publishes the physical
share, the elemental share and the total; the monster record supplies the six
element resistances (`XeenMonsterRecord::elementalResistance`). Presentation
follows `attack2`/`interface_scene.cpp`: the hit splat is armed only when the
physical share is positive, scaled by the total; the elemental frame
(category + 1) draws beside it, scaled by the elemental amount. Elemental-only
damage therefore shows no splat (see [original quirks](../original-quirks.md)).
Example: long sword of material 5 (Fire 10), dice 6, Might `+2`, one hit:
`8 * 75 / 100 = 6` against 25% physical resistance, plus `10 * 50 / 100 = 5`
against 50% fire resistance: 11 damage, elemental frame 1; fire resistance 100
leaves 6. A level-8 Barbarian's three hits of 5 give `15 + 10 = 25`: the
elemental term is added once.

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
monster save reduces it to 5. **Implemented (M67):** crossbow (3d2) of material
5, dice 6, hit at `1 + 14`: `6 * 50 / 100 = 3` plus 10 fire gives 13; a save
draw of at most the resource ID halves it to 6. A crossbow of material 38 with
dice 3 clamps to 0 and draws no save. Effectiveness never applies to Shoot.

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
(Paladin) bypasses the preference logic entirely. **DOS static, high confidence:**
`0x2d84b..0x2d852` branches directly to random dispatch `0x2d9a3`.
Eligible here excludes worst
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
`XeenEnemyAttackCandidate` and `XeenCombatTests.cpp`. **DOS static, high
confidence:** natural-20 test/damage `0x2db45`/`0x2db55`, continuation
`0x2db65`, fresh AC `0x2dbd7`, injury armor break `0x1f8a7` and bad-armor
gate `0x21275..0x21281` independently confirm that sequence.

Example: AC 8, level 10, monster hitChance 12. An unblocked threshold is 18;
Block raises it to 28. A d20 of 12 and hit-parameter draw of 7 give
`12 + 3 + 7 = 22`: hit unblocked, miss while Blocking. A d20 of 20 always performs
its first damage application regardless of either threshold.

## Monster damage, saving throws and secondary attacks

**Evidence: DOS static, high confidence (M68 Part A, 2026-10-15)** for the
sequence below: `WORLD/XEEN.DAT` `doCharDamage` at `0x2d22d`, saving-throw
helper `0x2d0c2`, special dispatch table `0x2d6d9`. See
[evidence](evidence.md#monster-damage-and-secondary-attacks). The pinned
`doCharDamage` differs in two places, marked below. **Implemented (M68 Part A)**
by `XeenEnemyAttackCandidate` for contact and ranged attacks of every damage
type, in combat, exploration and Rest.

The application first clears Sleep, then rolls `strikes` draws of
`R(1, dmgPerStrike)` and sums them. A Physical attack draws no saving throw
before its secondary save. For any other damage type, the exact order is:

1. Draw one **Physical** character save (`0x2d2e4..0x2d2f7`: selector 0, not the
   element) and halve damage on success. The pinned reference uses the element
   here; DOS does not.
2. Subtract the matching party protection for Fire, Electrical, Cold or Poison.
   Magical and Energy do not subtract one of these party protection fields.
   (No protection spell has an owner yet; the term is zero.)
3. Enter the element save loop (`0x2d35e..0x2d374`): draw the element save once
   unconditionally, even at zero damage; on success halve and repeat while the
   damage stays nonzero; stop on the first failed save. The pinned reference
   tests `damage > 0` before every draw and therefore draws nothing at zero.
4. Subtract party Power Shield and clamp to zero (`0x2d3a9..0x2d3b5`).
5. If positive damage remains and the special attack byte is nonzero, draw a
   Physical character save (`0x2d3ed`). A failed save applies the secondary
   effect.
6. Subtract the resulting HP damage, after applying the secondary effect.

A character's Physical save uses `v = B(Luck) + L` and succeeds when
`R(1, v + 20) <= v` (`0x2d0cd..0x2d101`). An elemental/magical save uses
permanent plus temporary plus equipment resistance `r`, and succeeds when
`R(1, r + 40) <= r` (`0x2d116..0x2d21d`; the selector maps 1 Magical, 2 Fire,
3 Electrical, 4 Cold, 5 Poison, 6 Energy to the character's resistance pairs).
Party protection is a separate subtraction and is not part of `r`. At
resistance zero the draw still occurs but cannot succeed.

Example: 23 rolled Fire damage against Luck 15 (`+2`) at level 3 (`v = 5`): an
initial draw of 1..25 at most 5 halves to 11; party Fire protection 3 leaves 8;
with fire resistance 10 a loop draw of 1..50 at most 10 halves to 4, a second
success to 2, a failure stops; Power Shield 2 leaves 0 and no secondary save is
drawn. The same 23 with a failed initial save and a failed first loop draw
applies 20 (`23 - 3`) and, with a nonzero special, draws the secondary save.

**Do not merge environmental and monster damage algorithms.** `giveCharDamage`
subtracts party protection before its loop, evaluates the saving throw before
checking whether damage is positive, and carries the mutated damage variable
through successive recipients. `doCharDamage` has an initial save, checks positive
damage before each repeated save, and rolls fresh dice per target. The same
nominal damage can therefore produce different outcomes and draw counts.

### Secondary effects

**Evidence: DOS static, high confidence (M68 Part A).** Dispatch table
`0x2d6d9` indexes `special - 5` for specials 5..22; specials 1-4 fall outside
the table and 6 and 11 jump straight to the HP subtraction (`0x2d6b5`). Every
row below requires positive post-shield damage and a failed Physical save.
**Implemented (M68 Part A)** in `xeenApplySpecialAttack`.

| Special | Secondary attack | Effect before HP subtraction (DOS offsets) |
| ---: | --- | --- |
| 5, 7, 8, 9, 13, 14, 15, 16, 18 | Poison, Disease, Insane, Sleep, Curse, Paralyze, Unconscious, Confuse, Weaken | Increment the condition byte; `inc al; jnz` then `0xff`: a byte that reaches zero is set to 255, so the counter saturates (`0x2d442` and siblings). Sleep re-applies after the wake at entry. |
| 10 | Curse Item | Set the cursed bit (`0x40`) in all nine slots of every category, empty slots included; a weapon slot holding ID 34 is left as is; frames stay (`0x2d671..0x2d6ab`). The reference instead clears that weapon's cursed bit. |
| 12 | Drain SP | Current SP becomes zero (`0x2d561`). |
| 17 | Break Weapon | Equipped (`frame != 0`), nonempty weapons whose ID is not 34: broken bit and frame cleared (`0x2d508..0x2d53e`). |
| 19 | Eradicate | Increment Eradicated, then the helper at `0x2bd6a`: every weapon slot except ID 34 gets the broken bit and frame 0; all nine armor and accessory slots get the broken bit and frame 0; all nine miscellaneous slots get the broken bit and keep their frame (charges); empty slots are written too. Positive HP becomes zero before the subtraction (`0x2d5c6..0x2d60e`). |
| 20 | Aging | Plain byte increment of the temporary age (`0x2d592..0x2d59b`): 255 wraps to 0. |
| 21, 22 | Death, Stone | Increment the condition; positive HP becomes zero before the subtraction (`0x2d641`, `0x2d611`). |
| 1-4, 6, 11 | none (11 is the reference's unused "In Love") | The secondary save is drawn; nothing else changes. HP damage applies. |

The pinned reference keeps equipment frames in Break Weapon and its break-all
helper as a port workaround and stores conditions as integers; DOS unequips and
saturates the byte as above. The periodic Poison/Disease doubling is a separate,
already confirmed rule. Weapon ID 34 (Xeen Slayer Sword) is the only ID the DOS
code tests; IDs above 34 do not occur in the weapon tables.

**Legacy medal (M68 Part A).** The original roster's medal on member 1 sits in a
ring frame (8). Curse Item keeps that arrangement with the cursed bit;
`xeenValidateCompletedEquipment` admits exactly the plain and the cursed state of
that item and no other arrangement change.

### Injury thresholds

**DOS static, high confidence (M63 Part B):** presentation surrounds injury,
without using sound length as a delay. Shoot keeps simultaneous lanes and depth
frames `0,1,2,0`; every travel/impact draw advances the remaining arrows and only
the arriving lane is consumed. Install the pose/splat/tint, draw before HP changes,
publish the injury, then draw again with the victim still visible. Award, drop and
remove a lethal victim only afterward, followed by another draw
(`0x1d867..0x1dec7`, `0x2bdf8..0x2c591`). Arrow hits use the original splat slots.
Monster attacks publish two pose draws before damage. Splat/pose counters advance
per semantic draw, while reconstruction reads retained values. Recipient damage
holds and sequential sparkles are described under
[presentation intervals](time-and-conditions.md#synchronous-presentation-intervals).

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

**DOS static (2026-10-17), high confidence: re-enabled members.** The
next-actor search continues from the speed-order cursor, rather than scanning
from the beginning. Advancing marks the previous actor's action flag, including
a disabled member whose slot was skipped (`0x2dd6d..0x2dd9f`). The condition
switch (`0x2de09`, table `0x2de5e`) skips Asleep, Paralyzed, Unconscious,
Dead, Stoned and Eradicated. Therefore a member re-enabled before their slot
arrives can act this round; one re-enabled after it was skipped waits until
the next round's flag reset. This applies to Awaken, Cure Paralysis, Stone to
Flesh and revival, provided no other disabling condition remains.

When only completed/disabled actors remain, the completion predicate
`0x2cfe3..0x2d06d` ends the round; the combat loop clears action/Block flags,
rebuilds the order and resets the cursor (`0x2ea0f..0x2ea47`). A final terminal
slot need not receive an action-flag write: the predicate already treats
conditions 11..15 as completed. Asleep instead gets marked as the skip loop
advances. Neither case offers another player action between the skipped tail
and the round reset. The all-disabled inner path also resets at
`0x2ddb6..0x2ddcf`; party defeat/exit can supersede round continuation.

Calibration reproduced Run's strict threshold, removal and order rebuild
(`0x2e1c0..0x2e1f2`). The pin's `Interface::nextChar` agrees. **Implemented (M75 Part A):**
`XeenCombat.cpp::selectNext` marks skipped disabled participating party slots
as done. Its scan can restart after a re-sort without granting them a late
action; monster-contact handling is unchanged. See [healing evidence](evidence.md#party-healing-spells)
for the complete trace. **HP alone is not an admission gate:** with no disabling
condition, a zero/negative-HP member receives a turn; the condition-only defeat
check (`0x24f29..0x24f8f`), portrait focus and ordinary command dispatcher do
not reject it. Attack is admitted at `0x2e8c3`; complete Block/Run paths
(`0x2e205..0x2e210`, `0x2e1c0..0x2e204`) also have no HP test. The pin
agrees. This establishes turn/command admission, not all downstream effects.

Block marks the acting member blocked, changing the later monster hit threshold
as described above. Run draws `R(1,100)` and succeeds only when the draw is
strictly less than the map's chance-to-run field. For threshold 50, rolls 1-49
succeed; threshold 100 accepts outcomes 1..99 and fails 100. **DOS static, high
confidence:** draw `0x2e1c0..0x2e1c4`, unsigned threshold
`0x2e1cc..0x2e1d0`, comparison/JGE failure `0x2e1d2..0x2e1d4`. This is
an integer boundary, not a measured DOS random-distribution claim. Success removes that member from
the combat party and rebuilds the speed order. The map supplies the party's
retreat position; the roll does not select a random retreat cell.

**DOS static, high confidence:** monster movement scans actors, skipping status
0 and Dragon Sleep (13), and calls recovery with resource ID (`0x20ca9..0x20cf1`,
ID argument `0x20cd2`). The helper draws `R(1, resource ID + 50)` and returns
success at **draw <= ID** (`0x2e1b2..0x2e1b4`); the caller clears status at
`0x20ce1`. The pin instead uses `>= ID`; equality succeeds in both. **Implemented (M73 Part A):** `XeenRegionalOpportunityCandidate::service` scans
all current-map slots in original order, including offscreen, inactive and dead
slots; Physical and Dragon Sleep never draw. Recovery is resumable and precedes
the first queued shot (`0x20167`, then `0x20171`), after movement/ranged selection.
Recovered actors cannot retroactively move or join that queue; later melee sees
the recovered status. Exploration, combat rounds, item response and Rest share
this opportunity. Disabled movement or characters shooting skips the entire
opportunity. The bounded DOS check `0x20ca9..0x20cf1` has no HP, lifecycle or
position filter, calibrated against status +7, species +6 and MOB count `0x9c59`.
Learned casts and item Use now apply the statuses (M73). Damage saving throws
also use `<= ID`. **Implemented (M73 Part A), Reference only:** melee, Shoot and existing spells
wake Sleep/Dragon Sleep on nonzero entry damage, before physical resistance and
the damage save, even if final damage becomes zero. Misses, failed magic gates
and zero entry damage do not wake; Beast Master/Hypnotize survive hits. Every
non-Physical status skips movement, ranged selection and melee without target
or damage draws, while occupancy, visibility, targeting and front-slot combat
contact remain. The hit bonus in step 5 above follows the exploding d20. The
whole monster frame block freezes, including frame 11 and its delay; impact FX
continue animating. These presentation/waking rules await DOS play confirmation.

**DOS static, high confidence (M69 A2):** monsters do not recover between map
visits by regeneration but by reloading. Every map load (`0x0f542`) zeroes the
whole monster array (`0x0f5be..0x0f5c7`), so the wake (attacking) byte and the
status byte start cleared, and sets each monster's HP from its MON record
(`0x0fa99..0x0faa8`). Saves keep monster positions only (`0x266e3..0x2671c`),
and loading a game re-enters the main loop, which loads the current map
(`0x0b819..0x0b839`, `0x0d086..0x0d08c`). A wounded survivor is therefore at
full HP and asleep after the party leaves its map and returns, and after a game
is loaded; while the party stays on the map, its wound persists. The arrival
view then wakes the monsters it sees. **M73 Part A:** every slot
clears status, regardless of lifecycle, and Spawn resets it too. The save
validates live statuses and captures Physical, retaining
its existing bytes without replaying RNG or time. Same as the pin
([`MonsterObjectData::synchronize`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp#L500)).
The maintainer's DOSBox check is pending. Offsets and calibration are in
[original quirks](../original-quirks.md#original-findings-and-current-discrepancies).

### Items and Use in combat

**DOS static (2026-10-18), high confidence.** The main-screen Use button and
the character sheet's Items button open the same Items dialog on the acting
member or the chosen combat-party member. Equip, Remove, Discard, transfer
and Quest never spend the turn or pass time; only a successful Use does,
through the ordinary item-use path (charge spent, monster response, turn
cursor cleared, next actor). Use is refused from the character sheet in
combat, and the main-screen Use dialog ignores F1-F6. The rules, refusals and
offsets are in [the Items dialog in combat](items.md#the-items-dialog-in-combat);
Town Portal ends combat at the first attacker re-evaluation after arrival,
with the ordinary end-of-combat minute (maintainer decision, 2026-10-19).

## Monster movement terrain

**DOS static, high confidence** (M68 Part B, 2026-10-15). `canMonsterMove` is
`WORLD/XEEN.DAT` `0x1fc91..0x1fda1`. Calibration: the threshold byte `ds:0xc0fc`
is map difficulty byte 0 (no-pass), because Run reads byte 7 at `ds:0xc103`
(`0x2e1cc`, above) and the player's wall checks (`0xcf5a`, `0xd523`, `0x1db71`)
read `ds:0xc0fc` with the reference's player rule. The surface type is
`ds:0xc0ea[current surface ID]`, the primary map's 16 surface types stored just
before the difficulties. A monster's flying byte is offset `0x2e` of its 60-byte
record, and species means the resource (sprite) ID.

The test runs once for each step the shared approach tries:

- **Indoors** there is no flight rule. The wall nibble of the monster's current
  cell, in the step direction, passes only when it is **below** no-pass
  (`0x1fd8f..0x1fd9b`: `cmp ax,dx; jle` refuses equality). Walkers and flyers
  share it.
- **Outdoors** the destination's middle value decides (jump table `0x1fda3`):

| Middle | Rule |
| --- | --- |
| 0, 2, 3, 4, 5, 6, 8, 11, 13, 14 | The destination's surface type decides (`0x1fcdb`). Water (0) and Deep Water (8) admit flyers and species 59 (`0x1fd21`, `0x1fd2d`); Space (15) admits only flyers (`0x1fd67`); any other surface admits every species on Darkside and all but species 59 on Clouds (`0x1fd72..0x1fd85`, archive selector `ds:0x94`) |
| 1, 7, 9, 10, 12 | Always refused, flyers too (`0x1fd8b`) |
| 15 | Below no-pass, as indoors |

Species 59 is the Clouds Water Dragon, which stays on water, and the Darkside
Vulture Roc, a flyer that the Clouds-only land refusal does not touch. The
surface rule matches pinned `Combat::canMonsterMove`. The pin differs in two
ways: it compares middles 1, 7, 9, 10 and 12 with the threshold, and it admits
a value equal to no-pass. Both differences are listed in
[original quirks](../original-quirks.md). The archive-selector identification
is medium-high confidence: it rests on the structural match, and the file-name
strings were not resolved.

Everything else in the approach is shared by walkers and flyers. That covers
the two passes, the actor scan order, the facing primary and fallback steps,
the pre-movement ranged scheduling and the three-actor occupancy cap (no flight
layer). A refused occupancy never tries the fallback, and contact entry is the
same. Movement makes no random draw. The only drawing difference is the
existing float of flying monsters.

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
**DOS static (2026-10-16), high confidence:** the drop field at monster-record
`+0x2d` is loaded at `0x2c36c..0x2c370`, the chance comparison is at
`0x2c38b..0x2c39a`, and the field is passed again at `0x2c3e2..0x2c3ec`
to makeItem (far target `1b98:4c68`, file `0x25ca8`). **M81 Part 0, calibrated
DOS static, high confidence:** map 85 skips currency and chance at
`0x2c283..0x2c28a`; fixed gold/gems accumulate as 32-bit values before the
item check (`0x2c2be..0x2c307`). Zero skips the draw; nonzero fields,
including guaranteed fields 4-6, consume it. The same calibrated
inclusive random helper supplies the chance and item draws. **Implemented
(M71):** supported level-1 drops share `XeenItemCandidate` with shops and
Events: category/subcategory `R(1,100)` and Misc charges `R(1,8)+2`.
**M81 Part A:** staging has the four
ten-slot DOS arrays (accessories included, `0x2c4a6`) and 32-bit gold/gems
that wrap; it is running-program state that no save holds
([load boundary](evidence.md#save-and-load-boundary)). **Implemented (M81 Part
C):** every lethal path (melee, Shoot, combat and exploration spells, single
and group targets) stages the record's gold and gems, draws the chance of a
nonzero field and generates at that field's level, finishing each target
before the next; map 85 stages and draws nothing. Original Orc and Barbarian
melee and group-spell kills, an Arena kill and a live kill before a treasure
Event are covered by `XeenPartyInterfaceTests` and `XeenArenaTests`.

| Item-drop field | Chance from one `R(1,100)` |
| --- | --- |
| 1 | 10% |
| 2 | 20% |
| 3 | 50% |
| 4, 5, 6 | 100% |
| 7-21 | Unused by Clouds records; refused as not supported yet |

Fields 1-6 are **DOS-confirmed static**: lookup `0x4d650`, gate
`0x2c373..0x2c39a`. No Clouds record uses 7-21; the DOS lookup has no bound
(7-17 read 0, 18-21 read 5/4/5/5) and the pin uses 0, so neither establishes
generation at those levels and MMModern refuses such a record before any
target selection or draw. Darkside's Doom Knight and Sandro use field 7,
outside the Clouds scope. Staged and purse
gold and gems are 32-bit sums, as in DOS.
[Evidence](evidence.md#m81-monster-treasure).

Generation precedes checking for an empty staged category slot, so generation
RNG remains consumed if staging is full. Weapons, armor and accessories use the
first empty slot of their own category. **M81 Part 0, DOS static high confidence:**
all four arrays have ten four-byte slots; bounds are at
`0x2c44e/0x2c49d/0x2c4ed/0x2c53c`. A full category discards the newly generated
item without changing readiness or refunding draws. The pinned Misc branch incorrectly copies
the temporary character's accessory slot into staged accessories, losing the
actually generated Misc item.

**DOS static (2026-10-12), high confidence:** the original instead copies the
generated Misc container, power and state into the separate Misc treasure array.
Its category-3 branch is at file `0x2c4f5`; the three writes are at `0x2c513`,
`0x2c524` and `0x2c535`. See [monster treasure evidence](evidence.md#monster-treasure-staging).
**Implemented (M61 Part C):** MMModern stages and delivers Misc through its
shared rules without changing generation draws or their order.
**Implemented (M61 Part D), DOS static high confidence:** delivery uses valid
preferred/eligible selection followed by unrestricted capacity fallback; absent
or invalid preference enters the unrestricted scan directly. Sleeping, disabled
and dead members can therefore receive loot (`0x243d5..0x24565`). Category-only
fullness replaces the first member's final item, retaining its equipped frame
(`0x24567`, weapon writes `0x245aa..0x245cc`). Completely full packs discard
remaining items but still credit gold/gems (`0x2433e..0x2437b`,
`0x2486a..0x24884`). The check occurs before each slot. See
[delivery evidence](evidence.md#treasure-delivery) for all category writes and
the special sword/quest branches outside current ordinary reward content.

**M81 Part 0, DOS static high confidence:** full-party Run clears staged gold,
gems and hasItems at `0x2e932..0x2e946`, retaining all four item arrays. Their
items remain dormant until a later currency/readiness trigger; they still occupy
staging slots. Saving and in-process reload are distinct from persisting these
items in a file; see [the full boundary](evidence.md#save-and-load-boundary).

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
[XeenCombatRules.cpp](../../src/games/xeen/XeenCombatRules.cpp) implements plain and,
since M67, enchanted weapon attacks (metal, elemental, attribute materials,
effectiveness, the Slayer Sword bypass), the two-stage natural-20 enemy application,
target selection, Run, typed consequences, supported spell damage and, since M68
Part A, the DOS monster damage sequence for every damage type with every
secondary effect (`XeenMonsterAbilityTests.cpp` holds the draw tapes). Since
M68 Part B, [XeenRegionalRules.cpp](../../src/games/xeen/XeenRegionalRules.cpp)
implements the DOS monster terrain test above for walkers, flyers and species 59
(`XeenRegionalRulesTests.cpp` holds the matrix).
[XeenCharacterRules.cpp](../../src/games/xeen/XeenCharacterRules.cpp) owns shared
character capacities. [XeenCombatTests.cpp](../../tests/XeenCombatTests.cpp) checks
ordered critical damage and armor changes;
[XeenCombatAuthorityTests.cpp](../../tests/XeenCombatAuthorityTests.cpp) checks
class attack counts and atomic consequence publication. These are pointers to
existing evidence, not a claim that this research ran those tests or that every
reference ability is playable.

Highest-value unresolved original-program checks:

1. (Settled, M68 Part A) Eradicate clears weapon, armor and accessory frames and
   keeps miscellaneous frames; Break Weapon unequips.
2. Is the source's uniform initial targeting distribution equivalent to the
   original party-size switch for every party size? Hatred 1's bypass is confirmed.
3. Are elemental weapon damage and effectiveness applied once after accumulating
   multi-attacks exactly as shown? M67 implements the reference order and
   marks it Reference only; the maintainer's DOSBox comparison of a metal, an
   elemental and an effective weapon against the same Vertigo monster settles it.
4. Verify the game-ID criterion for doubled low-level XP in World of Xeen CD.

No DOS-width, overflow or malformed-resource behavior beyond the retained
findings is inferred from ScummVM's wider host integers. Exact timing, monster
movement geometry beyond the terrain test, all spell effects and item generation
tables are outside this chapter's verified coverage.
