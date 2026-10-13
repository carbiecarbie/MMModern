# Magic

This chapter describes original-game rules, not MMModern's current support
list. The baseline is MMModern `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`
and ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
**Reference only** means verified in that source, not established in DOS.
**DOS static (retained)** means calibrated original-program evidence already
recorded in [original quirks](../original-quirks.md); this chapter did not
independently repeat that analysis. **DOS static (2026-10-12)** identifies
new calibrated read-only findings from this research. **DOS play** identifies a retained
maintainer observation. **Unknown** marks a question that source reading
alone cannot settle. Integer divisions below truncate; `U(a,b)` is one
inclusive bounded draw and `L = max(permanent level + temporary level, 0)`.

## Spell identity, books and costs

**Evidence: DOS static (2026-10-12) for the 76 SP/gem table entries;
Reference only for book layout and Guild pricing.** Sources: [spell cost/dispatch routines][spells],
`Spells::calcSpellCost`, `calcSpellPoints`, `subSpellCost`, `addSpellCost`;
[constants][constants], `SPELLS_ALLOWED`, `SPELL_COSTS`, `SPELL_GEM_COST`;
[character rules][character], `getSpellsCategory`, `getSpellsExpenseFactor`,
`getCurrentLevel`.

Global IDs 0-75 identify spells. A character's learned flags and saved current
spell instead address a slot in a class book; they are not global spell IDs.
Cleric/Paladin share Clerical, Sorcerer/Archer share Wizardry, and Druid/Ranger
share Druidic. Each has 39 real slots and a terminating table sentinel.
For example, First Aid is global 26, Clerical slot 14, Druidic slot 11.
Item powers use a third namespace, mapped below.

A nonnegative SP table entry is a fixed cost; a negative entry means its
absolute value times `L`. Gems are a fixed party debit. Check SP first, then
gems; failure of either check spends neither. On admission subtract both
before executing the spell. A level-zero scaled spell therefore costs zero
SP but retains its gem requirement.

Guild price is `100 * fixed SP`, or `500 * abs(scaled SP coefficient)`.
Paladin, Archer and Ranger pay twice that amount; Cleric, Sorcerer and Druid
pay the base. Price is independent of current level. Thus level-7 Energy
Blast costs 7 SP and 1 gem to cast, but 500 gold for a Sorcerer to learn and
1,000 for an Archer. Gem requirements do not enter the purchase formula.

The following is a compact index of all executable spell IDs. `L`, `2L`
and `3L` indicate scaled costs, not additional flat charges. These numbers
match the original signed-word SP table `[0x4f752,0x4f7ea)` and gem-byte table
`[0x4f7ec,0x4f838)`, compared entry by entry with the pinned constants.
The cost and refund routines index these tables at `0x2f2de..0x2f2e9` and
`0x2f353..0x2f35f`; see [evidence](evidence.md#spell-and-item-control-flow).

| ID | Spell | SP | Gems | ID | Spell | SP | Gems |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Acid Spray | 8 | 0 | 38 | Insect Spray | 5 | 1 |
| 1 | Awaken | 1 | 0 | 39 | Item to Gold | 20 | 10 |
| 2 | Beast Master | 5 | 2 | 40 | Jump | 4 | 0 |
| 3 | Bless | 2L | 1 | 41 | Levitate | 5 | 0 |
| 4 | Clairvoyance | 5 | 2 | 42 | Light | 1 | 0 |
| 5 | Cold Ray | 2L | 4 | 43 | Lightning Bolt | 2L | 2 |
| 6 | Create Food | 20 | 5 | 44 | Lloyd's Beacon | 6 | 2 |
| 7 | Cure Disease | 10 | 0 | 45 | Magic Arrow | 2 | 0 |
| 8 | Cure Paralysis | 12 | 0 | 46 | Mass Distortion | 75 | 10 |
| 9 | Cure Poison | 8 | 0 | 47 | Mega Volts | 40 | 10 |
| 10 | Cure Wounds | 3 | 0 | 48 | Moon Ray | 60 | 10 |
| 11 | Dancing Sword | 3L | 10 | 49 | Nature's Cure | 6 | 0 |
| 12 | Day of Protection | 75 | 10 | 50 | Pain | 4 | 0 |
| 13 | Day of Sorcery | 40 | 10 | 51 | Poison Volley | 25 | 10 |
| 14 | Deadly Swarm | 12 | 0 | 52 | Power Cure | 2L | 3 |
| 15 | Detect Monster | 6 | 0 | 53 | Power Shield | 2L | 2 |
| 16 | Divine Intervention | 200 | 20 | 54 | Prismatic Light | 60 | 10 |
| 17 | Dragon Sleep | 10 | 4 | 55 | Protection from Elements | L | 1 |
| 18 | Elemental Storm | 100 | 10 | 56 | Raise Dead | 50 | 10 |
| 19 | Enchant Item | 30 | 20 | 57 | Recharge Item | 15 | 10 |
| 20 | Energy Blast | L | 1 | 58 | Resurrection | 125 | 20 |
| 21 | Etherealize | 30 | 10 | 59 | Revitalize | 2 | 0 |
| 22 | Fantastic Freeze | 15 | 5 | 60 | Shrapmetal | L | 0 |
| 23 | Fiery Flail | 25 | 5 | 61 | Sleep | 3 | 1 |
| 24 | Finger of Death | 10 | 4 | 62 | Sparks | L | 1 |
| 25 | Fireball | 2L | 2 | 63 | Star Burst | 200 | 20 |
| 26 | First Aid | 1 | 0 | 64 | Stone to Flesh | 35 | 5 |
| 27 | Flying Fist | 2 | 0 | 65 | Sun Ray | 150 | 20 |
| 28 | Frostbite | 7 | 0 | 66 | Super Shelter | 15 | 5 |
| 29 | Golem Stopper | 20 | 10 | 67 | Suppress Disease | 5 | 0 |
| 30 | Heroism | 2L | 3 | 68 | Suppress Poison | 4 | 0 |
| 31 | Holy Bonus | 2L | 1 | 69 | Teleport | 10 | 0 |
| 32 | Holy Word | 100 | 20 | 70 | Time Distortion | 8 | 0 |
| 33 | Hypnotize | 15 | 4 | 71 | Town Portal | 30 | 5 |
| 34 | Identify Monster | 5 | 0 | 72 | Toxic Cloud | 4 | 1 |
| 35 | Implosion | 100 | 20 | 73 | Turn Undead | 5 | 2 |
| 36 | Incinerate | 35 | 10 | 74 | Walk on Water | 7 | 0 |
| 37 | Inferno | 75 | 10 | 75 | Wizard Eye | 5 | 2 |

## Admission, targeting and cancellation

**Evidence: Reference only**, except the explicitly marked DOS corrections.
Sources: [casting][spells] `Spells::castSpell`, [dialogs][dialogs]
`SpellOnWho::execute`, `SelectElement::execute`, `CastSpell::execute`.

Learned casting refuses Enchant Item, Etherealize, Jump, Lloyd's Beacon,
Super Shelter, Teleport, Town Portal and Wizard Eye in combat. Resource
checks run first; an affordable combat-forbidden spell refunds both costs
and allows another selection. This order determines which error is shown.

The member prompt permits F1-F6 or portraits, within the current party size;
in combat it uses the combat-party array. It does not require a healthy
target. The effect itself decides whether a dead, stoned or eradicated
member is valid. Escape returns no target and calls the refund routine with
the ID supplied to the prompt. A completed cast that subsequently fails
(e.g. First Aid on a terminal member) does not automatically refund.

**DOS static (retained): Suppress Poison.** DOS passes global ID 68 to the
prompt, so cancellation refunds 4 SP. The pin incorrectly passes First Aid
(ID 26) and refunds 1. DOS offsets `0x2f62f`, `0x2f146`, `0x2f34b`,
`0x2f67b`, analysis 2026-10-11; [recorded discrepancy](../original-quirks.md).

**DOS static (2026-10-12), high confidence: Resurrection cancellation.**
The original dispatch entry at file `0x33598` reaches `0x30bee`; the target
prompt at `0x30bf3` supplies spell ID 58, so normal cancellation refunds the
full 125 SP/20 gems. The pin supplies Raise Dead ID 56 and refunds only
50 SP/10 gems. Calibration: First Aid supplies ID 26 at `0x2f3fa`, and the
previous Suppress Poison analysis establishes the same ID/refund convention.
See [the original-program evidence](evidence.md#spell-and-item-control-flow).

**DOS static (2026-10-12), high confidence; implemented in M61 Part C: First Aid item cancellation.**
Using power 4 and then Escape at its member prompt spends the charge and grants
the item user 1 SP without an initial SP debit or maximum-SP clamp. Its gem refund
is zero. The traced item caller, shared prompt and unconditional refund helper
are detailed in [items](items.md#item-use-and-cancellation) and
[evidence](evidence.md#spell-and-item-control-flow). **Unknown:** the complete
resource effect of every other item/dialog cancellation path; do not generalize
the amount or refund ID from this one route.

Exploration learned casts enter the normal step charge (1 minute indoors,
10 outdoors); item Use is a separate settlement path. **DOS play:** retained
2026-10-12 observations establish no time advance for item Use, a failed Jump
still spending its charge, and last-charge removal. **DOS static (2026-10-12):**
the traced Use path spends before the effect; admitted First Aid cancellation retains the
spent charge and normal monster response. This must not be conflated with
Escape before selecting an item. See [the item-use evidence](../original-quirks.md).

## Healing and conditions

**Evidence: Reference only**, except the marked DOS corrections.
Sources: [spell handlers][spells], [character rules][character]
`Character::addHitPoints`, `isDead`.

`addHitPoints(n)` does nothing to a member with Dead, Stoned or Eradicated.
Otherwise, when current HP is at most maximum HP, it adds `n` capped at the
maximum; already-overmaximum HP stays unchanged. Positive resulting HP
clears Unconscious. This means a cure implemented through `addHitPoints(0)`
can clear Unconscious without adding HP, but does not do so for a terminal
member. Spell handlers can still clear their explicit condition afterward.

| Spell | Exact effect after target selection |
| --- | --- |
| First Aid / Cure Wounds / Nature's Cure | Reject terminal target; heal 6 / 15 / 25 HP. Nature's Cure has no condition-clearing operation of its own in the pin. |
| Power Cure | Reject terminal target before RNG; heal `U(2,12) * L`, one draw for the whole amount. This is not a sum of `L` dice. |
| Revitalize | Call `addHitPoints(0)`, then clear Weak, including on a terminal target. |
| Cure Poison / Disease / Paralysis | Call `addHitPoints(0)`, then clear the named condition, including on a terminal target. |
| Stone to Flesh | Call `addHitPoints(0)` while Stoned is still set, then clear Stoned; this order can leave Unconscious untouched. |
| Awaken | Every active member: clear Asleep; clear Unconscious if current HP is positive. No terminal-status guard. |
| Suppress Disease | If count is nonzero, set counts 1-3 to 1 and subtract 3 from counts at least 4; then call `addHitPoints(0)`. Zero stays zero. |
| Suppress Poison | **DOS static (retained):** subtract 3 at counts at least 4; the pin subtracts 2. The retained evidence establishes the decrement and cancellation correction above; other presentation details remain reference only. |
| Raise Dead | Require Dead; clear Dead and Unconscious, set HP to 0, call `addHitPoints(1)`, then reduce permanent Endurance by 1 with minimum 1. Other terminal flags can prevent the heal. |
| Resurrection | Require Eradicated; call `addHitPoints(0)` before clearing it, then reduce permanent Endurance by 1 (minimum 1) and add 5 temporary age (cap 250). No explicit HP refill. DOS confirmation remains open. |
| Divine Intervention | Add 5 caster temporary age, cap 250; clear conditions Cursed through Stoned for all active members, leaving Eradicated. Refill HP only for non-eradicated members. It does not refill SP. |
| Moon Ray | Deliver attack first, then draw `U(1,30)` separately for every active member and call `addHitPoints` with it, even if terminal status makes the heal ineffective. |
| Create Food | Add active-party size to food, without filtering by condition. Original overflow behavior is not established here. |

**DOS static (2026-10-12), high confidence: Nature's Cure heals 25.**
DOS dispatch ID 49 at `0x33550` reaches `0x2fb9e`; target ID 49 is supplied
at `0x2fba3`. Terminal conditions 13-15 are refused at `0x2fbe0..0x2fbef`,
and the handler passes 25 at `0x2fbf9` to healing helper `0x2ee3a`.
Calibration: First Aid passes 6 at `0x2f450` to that same helper. There is
no separate condition cure; ordinary positive-HP Unconscious clearing is
part of healing. See [the original-program evidence](evidence.md#spell-and-item-control-flow).

Example: a living target at -3 HP receives First Aid, reaches 3 HP, and
loses Unconscious. A target at 55 HP with maximum 50 stays at 55. At level 7,
Power Cure's one draw of 5 gives 35 healing before the HP cap.

## Damage preparation and resolution

**Evidence: Reference only** for the preparation table, except Pain's DOS
correction. Sources: [handlers][spells], [combat][combat]
`rangedAttack`, `attack`, `attack2`, `getMonsterResistance`.
`Single`, `Group` and `All` describe the spell's range setting: ranged
resolution still calls `attack` using the spell/group path, even for a
single-target spell. Do not confuse that with the physical weapon
`RT_SINGLE` path, which bypasses several spell checks.

| Spell | Prepared damage | Type | Range |
| --- | --- | --- | --- |
| Acid Spray | 15 | Poison | All |
| Cold Ray | `U(2,4) * L` | Cold | All |
| Dancing Sword | `U(6,14) * L` | Physical | Group |
| Deadly Swarm | 40 | Physical | Group |
| Elemental Storm | 150; one type draw across Fire/Electrical/Cold/Poison | Drawn type | All |
| Energy Blast | `2L` | Energy | Single |
| Fantastic Freeze | 40 | Cold | Group |
| Fiery Flail | 100 | Fire | Single |
| Fireball | `U(3,7) * L` | Fire | Group |
| Flying Fist | 6 | Physical | Single |
| Frostbite | 35 | Cold | Single |
| Implosion | 1,000 | Energy | Single |
| Incinerate / Inferno | 250 | Fire | Single / Group |
| Lightning Bolt | `U(4,6) * L` | Electrical | Group |
| Magic Arrow | 8 in special dispatch | Magic Arrow | Single |
| Mega Volts | 150 | Electrical | Group |
| Moon Ray | 30 | Energy | All |
| Pain | **8 in DOS; 0 in pin** | Physical | Group |
| Prismatic Light | 80; one type draw from Physical through Energy | Drawn type | All |
| Shrapmetal | `2L` | Physical | Group |
| Sparks | `2L` | Electrical | Group |
| Star Burst | 500 | Fire | All |
| Sun Ray | 200 | Energy | All |
| Toxic Cloud | 10 | Poison | Group |

The scalar random draw in each variable-damage handler occurs once before
projectile resolution, not once per monster or once per caster level.
Single stops after the first attacked monster. Group stops after the first
occupied group, up to three positions. All continues through subsequent
reachable groups; walls still limit the projectile. Target ordering and
wall geometry live in `rangedAttack`, not the cost or damage table.

For ordinary nonzero damage spells the per-target order is:

1. If magic resistance `R` is nonzero, draw `U(1,100+L)`; reject when the
   draw is less than `R`. Zero resistance skips this draw. Equality passes.
2. Physical damage applies `floor(D * (100-P) / 100)` for physical
   resistance `P` under the normal spell weapon context.
3. The elemental contribution routine is added, then the monster saves:
   draw `U(1,50+I)`, where `I` is its resource/sprite ID, and succeed at
   `<= I`. Ordinary spell damage halves on success, truncating again.
4. Apply positive final damage; zero produces the miss outcome.

A resource ID is not monster level. For `I=10`, save probability is 10/60.
At level 7, Shrapmetal prepares 14; physical resistance 25 produces 10;
a successful save then produces 5. A monster with magic resistance 20 lets
through a gate roll of 20 and rejects 19.

**DOS static (retained):** spell elemental resistance contributes zero,
because the contribution starts at zero. It does not multiply down the
prepared damage. Calibrated DOS offsets `0x2c675`, `0x2c70c`, `0x2c873`,
`0x2bf8a` establish this behavior; it is not permission to remove the
separate magic-resistance gate. Magic Arrow still draws the monster save,
but ignores its result and retains 8 damage (`0x30fa8`, `0x2cde4`,
`0x2c037`, `0x2c59a`). Pain prepares 8 physical damage against a group
(`0x33558`, `0x2f5e9`, `0x2cd81`). See [original quirks](../original-quirks.md).

**Reference only, boundary cases:** zero-level Energy Blast/Shrapmetal
still draw a nonzero magic-resistance gate but do not enter damage handling
and do not draw a save. Nonzero prepared damage reduced to zero by physical
immunity does retain the save draw. Clouds resource ID 89 has a special
spell-damage refusal in `attack2`; this chapter does not certify the DOS
boss behavior. Poison Volley changes the shooting mode and uses the missile
branch, so the ordinary spell formula above must not be applied to it.

## Monster-specific magic

**Evidence: Reference only.** Source: [combat][combat] `Combat::attack` and
`attack2`. Every branch below first goes through the magic-resistance gate.
These are dispatch rules, not a complete account of status duration/AI.

| Spell | Type gate and effect |
| --- | --- |
| Sleep | Animal/Humanoid; draw `U(1,50+I)` and set Sleep only on `> I`. |
| Beast Master / Hypnotize | Animal/Humanoid; failed monster save sets the respective status. |
| Dragon Sleep | Dragon; failed monster save sets Dragon Sleep. |
| Finger of Death | Animal/Humanoid; first failed save allows `min(current HP,50)` into damage handling. A second successful save changes damage to 5. |
| Holy Word | Undead; send current HP to damage handling. Its ordinary save draw does not change damage. |
| Mass Distortion | Send `max(floor(current HP/2),1)` to damage handling; successful save changes damage to 5. |
| Golem Stopper | Golem; 100 damage; ordinary save draw does not halve it. |
| Insect Spray | Insect; 25 damage; ordinary save draw does not halve it. |
| Turn Undead | Undead receives 25; non-undead enters with zero. Ordinary save does not halve it. |

For Finger of Death, the first save and the damage-stage save are distinct
RNG draws. Do not replace this by a single kill chance inferred from its name.

## Party effects and movement

**Evidence: Reference only.** Source: [handlers][spells]. Durations and reset
ownership are detailed in [time and conditions](time-and-conditions.md).

| Spell | Assignment, not an inferred stacking rule |
| --- | --- |
| Bless / Heroism / Holy Bonus / Power Shield | Set corresponding party amount to caster `L`, replacing any larger prior amount. |
| Protection from Elements | Choose Fire/Electrical/Cold/Poison; set that party resistance to `min(2L+5,200)`. |
| Levitate | Set levitate count to 1. |
| Light | Increment light count; its interaction with darkness also marks a step outside complete blackness. |
| Wizard Eye | Set wizard eye and clear automap-on. It does not mark cells explored. |
| Clairvoyance / Walk on Water | Set their respective flags. |
| Day of Protection | Set Walk on Water; Heroism, Holy Bonus, Bless, four party elemental resistances and light count all become `L`. |
| Day of Sorcery | Set Power Shield and light count to `L`, Clairvoyance and Wizard Eye true, Levitate count 1, automap-on false. |

Jump moves two cells forward, retaining direction. Indoors both crossed
forward walls must be clear. Outdoors the first wall and destination wall
must differ from 1, and destination surface ID must be nonzero. A valid
jump marks the party stepped, so terrain arrival matters; it is not a
blanket safe traversal. Etherealize moves one cell ahead after its map
restriction and invalid-cell checks. Time Distortion moves to the map's run
location. Lloyd's Beacon, Teleport, Town Portal and Super Shelter each check
their own map restriction. Full destination-selection and map-transition
semantics require their dedicated dialogs; they are not settled by these
entry checks.

## Item power lookup

**Evidence: DOS static (2026-10-12), high confidence for the mapping;
Reference only for combat admission below.** Source: [casting][spells]
`Spells::castItemSpell`; original item dispatch at file `0x39e4b`, table
`[0x3a114,0x3a1a6)`. All 73 semantic destinations match the pin; power 33
uses the alternate Frostbite handler at `0x2fc77`. Incinerate power 64 reaches
`0x30b58`, which prepares the same 250 fire/single damage as learned Incinerate
at `0x30f0d` but uses different sound/projectile selectors. Thus matching effect
names do not prove identical presentation. Calibration uses known
First Aid, Cure Poison and learned-spell targets. See
[original-program evidence](evidence.md#spell-and-item-control-flow).
This maps power ID, not container/material ID.
Power 0 is invalid. Power 33 calls the special Frostbite animation handler;
it is not an ordinary global-spell entry. Learned books and SP/gem costs
are bypassed by item dispatch. See cancellation caveats above.

| Power range | Effects in ascending power order |
| --- | --- |
| 1-10 | Light; Awaken; Magic Arrow; First Aid; Flying Fist; Energy Blast; Sleep; Revitalize; Cure Wounds; Sparks |
| 11-20 | Shrapmetal; Insect Spray; Toxic Cloud; Protection from Elements; Pain; Jump; Beast Master; Clairvoyance; Turn Undead; Levitate |
| 21-30 | Wizard Eye; Bless; Identify Monster; Lightning Bolt; Holy Bonus; Power Cure; Nature's Cure; Lloyd's Beacon; Power Shield; Heroism |
| 31-40 | Hypnotize; Walk on Water; special Frostbite; Detect Monster; Fireball; Cold Ray; Cure Poison; Acid Spray; Time Distortion; Dragon Sleep |
| 41-50 | Cure Disease; Teleport; Finger of Death; Cure Paralysis; Golem Stopper; Poison Volley; Deadly Swarm; Super Shelter; Day of Protection; Day of Sorcery |
| 51-60 | Create Food; Fiery Flail; Recharge Item; Fantastic Freeze; Town Portal; Stone to Flesh; Raise Dead; Etherealize; Dancing Sword; Moon Ray |
| 61-70 | Mass Distortion; Prismatic Light; Enchant Item; Incinerate; Holy Word; Resurrection; Elemental Storm; Mega Volts; Inferno; Sun Ray |
| 71-73 | Implosion; Star Burst; Divine Intervention |

Item dispatch explicitly refuses powers 16, 21, 28, 42, 48, 55 and 58 in
combat. Unlike learned `castSpell`, that switch does not include Enchant
Item power 63. These are the reference's restrictions. In DOS, Town Portal's
item entry calls its notice and then reaches the handler even in combat
(`0x3a058..0x3a075`); the full downstream result remains unknown. Whether the
other admission-list differences match DOS is
**Unknown**; avoid silently making the two admission lists identical.

## Coverage and executable witnesses

The strongest retained DOS findings are the Pain, Suppress Poison,
elemental-damage and Magic Arrow corrections. Most of this chapter is a
carefully bounded reference inventory. Unknown high-impact areas include
non-First-Aid item-prompt refunds, Resurrection's actual recovery effect, Poison Volley's full missile
arithmetic, high-tier overflow, monster-control duration and full travel
spell dialogs. Names and spell descriptions are not sufficient evidence
for these rules.

MMModern cross-checks (implementation evidence, not DOS proof):
[XeenLearnedSpellRules.cpp](../../src/games/xeen/XeenLearnedSpellRules.cpp),
[XeenCombatRules.cpp](../../src/games/xeen/XeenCombatRules.cpp),
[XeenLearnedSpellRulesTests.cpp](../../tests/XeenLearnedSpellRulesTests.cpp).
The tests cover cost/book identities, supported healing and condition
semantics, and supported spell damage. Unsupported original spells in this
chapter do not become implemented merely because their rules are listed.

[spells]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/spells.cpp
[constants]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[dialogs]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_spells.cpp
[combat]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp
