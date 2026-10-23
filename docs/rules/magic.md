# Magic

This chapter describes original-game rules, not MMModern's current support
list. The reference is ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
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

## Clouds Guild catalogues and membership

**DOS static (2026-10-20), high confidence.** The 100 catalogue entries
match pinned `CLOUDS_GUILD_SPELLS`; the three 40-entry class tables match
`SPELLS_ALLOWED`. These establish catalogue identity and ordering, not
every Guild pricing or book-storage rule.

| Map | Catalogue source | Membership award |
| --- | --- | --- |
| 28 Vertigo | `CLOUDS_GUILD_SPELLS[0]` | 0 |
| 29 Nightshadow | Row 1 | 1 |
| 30 Rivercity | Row 2 | 2 |
| 31 Asp | Row 3 | 3 |
| 32 Winterkill | Row 4 | 4 |
| 49 Shangri-La | All global spell IDs 0..75 allowed by the selected class | 5 |

For maps 28..32, visit the row's 20 entries in stored order and keep only
spells in that class book; unmatched padding is skipped. Shangri-La scans
global IDs in ascending order and keeps the class's spells. Buy omits
already learned spells. Spell Info uses the same source and order, including
learned spells (mode `0x80`); it does not list only the member's learned book.
Both actions require the selected member's membership award. Other Clouds
maps have no original Guild; DOS/pin membership arithmetic remains `map-28`,
but the catalogue is not a verified fallback outside the rows above.

Pinned `dialogs/dialogs_spells.cpp::setSpellText`,
`character.cpp::guildMember` and `locations.cpp::GuildLocation::doOptions`
agree. [Offsets and table comparison](evidence.md#other-town-services-2026-10-20).

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
[evidence](evidence.md#spell-and-item-control-flow). The M75 item paths below
are also settled; other item/dialog cancellation paths remain unknown.

**DOS static (2026-10-17), high confidence:** all twelve M75 spells are
admitted in combat. The ten member-targeted handlers supply their own global
spell ID to the shared prompt and refund their own table cost on Escape;
Create Food and Divine Intervention have no member prompt. Item powers 26,
27, 37, 41, 44, 56, 57 and 66 reach those same prompts: cancellation spends
the charge and adds the corresponding spell's SP and gems to the item user
and party, without an initial SP/gem debit or maximum-SP clamp. Powers 51
and 73 have no target cancellation. Power Cure's refund is `2L` SP/3 gems,
using the item user's level. See [offsets and limits](evidence.md#party-healing-spells).

**DOS static, high confidence:** re-enabled members retain their combat-party
slots. Before their slot arrives they can act this round; after a skipped
slot they wait until next round's flag reset. This applies to Awaken,
Cure Paralysis, Stone to Flesh and revival, with other disabling conditions
still respected. The pin agrees. Implemented in M75 Part A: skipped disabled party slots
are marked done even when the speed order is re-sorted. See [turn-order evidence](combat.md#initiative-block-run-and-monster-recovery)
and [admission evidence](evidence.md#party-healing-spells). Zero/negative HP
alone does not prevent a turn or ordinary command admission when disabling
conditions are absent; Resurrection need not refill HP to restore eligibility.

Exploration learned casts enter the normal step charge (1 minute indoors,
10 outdoors); item Use is a separate settlement path. **DOS play:** retained
2026-10-12 observations establish no time advance for item Use, a failed Jump
still spending its charge, and last-charge removal. **DOS static (2026-10-12):**
the traced Use path spends before the effect; admitted First Aid cancellation retains the
spent charge and normal monster response. This must not be conflated with
Escape before selecting an item. See [the item-use evidence](../original-quirks.md).

## Healing and conditions

**Implemented:** First Aid, Cure Wounds, Revitalize, Awaken and Moon Ray;
M75 Part A adds Nature's Cure, Power Cure, Cure Poison, Cure Disease,
Cure Paralysis, Stone to Flesh, Suppress Poison, Suppress Disease and
Create Food. M75 Part B adds Raise Dead, Resurrection and Divine
Intervention. All use the book, Quick Fight and their item powers, with
the M75 own-ID target prompts and refunds above, including charged-item
cancellation.

**Evidence: DOS static (2026-10-17), high confidence** for the twelve M75
spells and the shared healing helper's terminal guards and condition order;
other rows remain Reference only unless marked. See the
[bounded handler checks](evidence.md#party-healing-spells).
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
| Suppress Disease | Set counts 1-3 to 1 and subtract 3 from counts at least 4; zero stays zero. Always call `addHitPoints(0)` afterward, including at zero. The pin also subtracts 3, but skips the helper at zero. |
| Suppress Poison | DOS branches `0x2f663..0x2f685` set counts 1-3 to 1, subtract 3 at counts at least 4, and leave zero unchanged; all selected-target paths call `addHitPoints(0)` at `0x2f69b`. The pin agrees at zero and 1-3, but subtracts 2 and uses the wrong cancellation ID. |
| Raise Dead | Require Dead; clear Dead and Unconscious, set HP to 0, call `addHitPoints(1)`, then decrement permanent Endurance as a byte, replacing a zero result with 1. Input 0 wraps to 255; inputs 1/2 become 1. No age change. Other terminal flags can prevent the heal, leaving HP 0. |
| Resurrection | Require Eradicated; call `addHitPoints(0)` before clearing Eradicated alone, leaving HP and other conditions unchanged. Decrement permanent Endurance with the same byte rule as Raise Dead; add 5 temporary age, capped at 250. |
| Divine Intervention | Add 5 caster temporary age, cap 250; clear conditions Cursed through Stoned for all active members, leaving Eradicated. Refill HP only for non-eradicated members. It does not refill SP. |
| Moon Ray | Deliver attack first, then draw `U(1,30)` separately for every active member and call `addHitPoints` with it, even if terminal status makes the heal ineffective. |
| Create Food | Add active-party size to the unsigned 16-bit food field, without filtering by condition; wrap modulo 65536, with no cap. The pin's in-memory field is wider. |

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

**Evidence: DOS static (2026-10-17), high confidence** for the 18 ordinary
M74 spells' preparation, ranges, draws and costs; other rows remain
**Reference only**, except Pain's retained DOS correction and Energy Blast's
calibrated DOS static correction below. Sources: [handlers][spells], [combat][combat]
`rangedAttack`, `attack`, `attack2`, `getMonsterResistance`.
The checked image and mapping are in [evidence](evidence.md#exact-baseline).
Pain, the zero elemental contribution and item dispatch were recalibrated
before this pass. Bounded instructions and per-spell offsets are retained in
`D:/MModern-re/m74-damage-spells/findings.md`; no game execution was used.
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
| Energy Blast | `U(2,6) * L` | Energy | Single |
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

**DOS static, high confidence:** the scalar random draw in each variable-damage handler occurs once before
projectile resolution, not once per monster or once per caster level.
**Energy Blast: calibrated DOS static (2026-10-22), high confidence.**
The learned ID-20 dispatcher entry at `0x33853` reaches `0x333ab`, then
handler `0x2f584`: draw `U(2,6)` at `0x2f588`, read current caster level at
`0x2f599`, multiply at `0x2f5a4`, then request effect 16 at `0x2f5b7` and
projectile 13 at `0x2f5bf`. The RNG target is calibrated against Power Cure
(`0x2fb6f`) and Recharge Item (`0x31bf0`); the level helper matches Sparks
(`0x2f55a`). This preparation draw also occurs at level zero and without a
target. The pin instead prepares fixed `2L`; MMModern uses the DOS draw in
the shared learned/item, combat/exploration preparation owner.
Cold Ray, Dancing Sword, Fireball and Lightning Bolt draw at `0x30263`,
`0x30a14`, `0x30043` and `0x2fc0f`, then multiply by current level.
Elemental Storm stores 150 before its one type draw `U(2,5)` (`0x30b7b..0x30b8d`);
Prismatic Light stores 80 before `U(0,6)` (`0x30a52..0x30a64`). Neither has
a damage draw. Prismatic's interval includes Magical (1). These draws precede
every target's magic-resistance gate and saves, even with no targets.
Single stops after the first attacked monster. Group stops after the first
occupied group, up to three positions. All continues through subsequent
reachable groups, **at most four groups / twelve positions**, while the
projectile remains active. DOS iterates slots `0x98b4..0x98bf` at
`0x1d98a..0x1de91`. Between groups, outdoors wall values 1, 3, 6, 7, 9,
10 and 12 block; indoors a wall at least the no-pass threshold blocks
(`0x1db3e..0x1db84`, `0x1dc65..0x1dca4`, `0x1dd85..0x1ddc4`).
Each target finishes damage, death and treasure generation/staging before
the next target's gate; party treasure delivery happens after the projectile
(`0x2cf94..0x2cf9f`, `0x2c112..0x2c591`, `0x1debf`). This matches the pin.

For ordinary nonzero damage spells the per-target order is **DOS static,
high confidence** (`0x2cd12..0x2cd93`, `0x2beab..0x2bf98`,
`0x2c020..0x2c06e`; save helper `0x2e19b..0x2e1bf`):

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

**Reference only, boundary cases:** zero-level Energy Blast/Shrapmetal/Sparks
still draw a nonzero magic-resistance gate but do not enter damage handling
and do not draw a save. Energy Blast's earlier `U(2,6)` preparation draw is
separately established by the DOS handler above. **DOS static, high confidence:** nonzero prepared
damage reduced to zero by physical immunity does retain the save draw.
Clouds resource ID 89 refuses nonzero damage supplied with a nonzero
attack range argument (`0x2bdf8..0x2be33`). All spell projectiles, including
Single spells and Poison Volley, use such arguments. The refusal precedes
waking, damage-stage saves and HP changes, but follows earlier preparation,
magic-resistance and Finger of Death's first save when applicable. The
corresponding Darkside test does not refuse. This matches the pin's spell
refusal; it does not settle the separate melee/Slayer Sword path.

**DOS static, high confidence:** all 23 M74 spells can be cast in exploration
and spend their normal learned costs with no target. Cost debit precedes
direct dispatch (`0x3327b..0x332c7`); empty target slots are skipped without
a refund (`0x1d98a..0x1dec7`). Preparation draws still occur, but no monster
gate/save is drawn. Moon Ray still draws `U(1,30)` per active member after
delivery and calls healing (`0x30aa1..0x30ad4`). This confirms that ordering
and empty-target behavior, not all surrounding exploration time/AI rules.

**Poison Volley: DOS static, high confidence.** Entry `0x30fc3` prepares
10/type 17/All; `0x1d899..0x1d8be` changes type to Poison and enables six
shot flags. The active-party loop supplies one successful shot per member,
without requiring a missile weapon. Each target resets miss flags; a miss
retains the shot for later targets, a hit consumes it (`0x2cb46..0x2ccd9`).
It uses weapon/accuracy rolls, bypassing the spell magic-resistance gate.
On a hit the base is 10, regardless of the weapon dice already rolled;
RT_HIT adds that shooter's missile-material elemental contribution, reduced
by its corresponding resistance, and the monster save halves the combined
amount. Physical resistance does not reduce the Poison base
(`0x2cb89..0x2cbbc`, `0x2c675..0x2c70c`, `0x2beab..0x2beb4`, `0x2c037`).
Without an elemental missile the hit is therefore 10 or 5. The same four
tiers, walls and target settlement apply. The pin agrees with this branch,
but its signed hit-total comparison differs from DOS's unsigned word
comparison: [negative hit totals](../original-quirks.md#sv-hit-total-unsigned).

## Monster-specific magic

**Evidence: Reference only**, except Finger of Death, Holy Word, Mass
Distortion and Golem Stopper, whose species gates, amounts and saves are
**DOS static (2026-10-17), high confidence**. Source: [combat][combat] `Combat::attack` and
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

DOS special dispatch table `0x2cfab` reaches Finger `0x2ced1..0x2cf2c`,
Holy Word `0x2cf4c..0x2cf68`, Mass Distortion `0x2cf6c..0x2cf91` and
Golem Stopper `0x2cdf1..0x2ce04`. Finger's first save is at `0x2cef3`;
damage-stage save `0x2c037` uses table `0x2c59a`. Finger/Mass success
sets exactly 5, even if that increases a smaller amount; Holy Word/Golem
consume the save and ignore it. Prepared ranges are Group for Finger,
Holy Word and Mass Distortion, Single for Golem Stopper.
Wrong-species Holy Word enters zero-damage/miss handling in DOS, whereas
the pin skips it; neither harms the target or draws a damage save.
The [miss-call difference](../original-quirks.md#sv-holy-word-miss) is
confirmed statically; its audible presentation has not been played.

Sleep, Beast Master and Hypnotize affect the first reachable group (up to
three targets); Dragon Sleep affects one target. All prepare zero damage,
use Magic Orb (`pow7.icn`) and sound FX18. After the nonzero-resistance gate,
check species, then draw `U(1,50+I)` only for compatible targets. Success
(`> I`) replaces the single status; there is no damage-stage save, damage
or allegiance change. **Evidence: Reference only** for application and
presentation (`Combat::attack`, the four spell handlers). Recovery and
waking are documented in [combat](combat.md#initiative-block-run-and-monster-recovery), with
their separate evidence labels; application tests do not establish DOS play.

Insect Spray and Turn Undead prepare 25 for Insect and Undead respectively,
then consume the ordinary save without halving. Wrong-type Spray skips
`attack2`; wrong-type Turn Undead enters it with zero, so draws no save and
does not wake Sleep. **Evidence: Reference only** (`attack`, `attack2`).
Both use group range; Insect Spray uses Spray (`pow10.icn`), sound FX17, while
Turn Undead uses Energy Blast (`pow13.icn`), sound FX18.

Sparks prepares `2L` electrical damage, costs `L` SP and one gem, and uses
Sparks (`pow5.icn`), sound FX14. Toxic Cloud prepares 10 poison damage, costs four
SP and one gem, and uses Spray, sound FX17. Both use group range and the ordinary
halving save. **Evidence: Reference only** for preparation/presentation;
**DOS static (retained)** for the zero elemental-resistance contribution.
Visual monster tints follow `Combat::DAMAGE_TYPE_EFFECTS`: Insect Spray and
Toxic Cloud use 2, Sparks 11, Turn Undead 14. Controls do not enter `attack2`
and do not apply a damage tint. These are distinct from the sound FX IDs.
A level-zero Sparks still gates against nonzero magic resistance, then skips
the damage save. Each target's damage, death and treasure draws finish
before the next target's resistance gate.

All eight spells are delivered through learned Cast and item Use (combat or
exploration) and Quick Fight. Costs/books retain the tables above;
item powers 7/17/31/40 and 12/19/10/13 deliver Sleep/Beast Master/Hypnotize/
Dragon Sleep and Insect Spray/Turn Undead/Sparks/Toxic Cloud respectively.
These are implementation witnesses, not additional DOS evidence.

Finger of Death, Holy Word, Mass Distortion and Golem Stopper are implemented
(M74 Part B) through the same damage candidate in combat Cast, Quick Fight,
exploration Cast and item Use (powers 43, 65, 61 and 45 respectively). The
candidate uses the target's current HP and retains both Finger saves across
bounded execution. Clouds ID89 refuses nonzero spell damage after dispatch
and Finger's first save, before waking or the damage-stage save. Holy Word's
wrong-species path records a miss without waking or drawing a damage save.
Melee and Shoot compare the final hit total as a
16-bit unsigned word, also used by Poison Volley (M74 Part C).
Poison Volley uses the shared party volley in Cast and item Use (power 46),
in exploration or combat, and in Quick Fight: one successful shot per active
member, with misses retaining their shot for later targets. Its missile
candidate preserves weapon dice and accuracy draws, substitutes the 10
Poison base, adds resisted missile elemental damage and halves the total
on a save. No missile weapon is required; neither the magic-resistance gate
nor physical resistance reduces the base. Target settlement remains inside
the existing exploration and combat owners.
These implementation checks add no DOS evidence.

For Finger of Death, the first save and the damage-stage save are distinct
RNG draws. Do not replace this by a single kill chance inferred from its name.

## Party effects and movement

**Evidence: DOS static, high confidence (2026-10-18)** for the M76 rules below.
The earlier Jump evidence is retained. Source comparison: [handlers][spells].
Offsets, calibration and limits are in [M76 spells](evidence.md#m76-spells).
The saved byte order is Levitate, automap on, Wizard Eye, Clairvoyance,
Walk on Water, Bless, Power Shield, Holy Bonus, Heroism; see
[Party effect bytes](evidence.md#party-effect-bytes). Temple Donation grants
Clairvoyance (byte 3), not Levitate (byte 0).
Durations and reset ownership are detailed in
[time and conditions](time-and-conditions.md).

| Spell | Assignment, not an inferred stacking rule |
| --- | --- |
| Bless / Heroism / Holy Bonus / Power Shield | Set corresponding party byte to current caster level `L` modulo 256, replacing any larger prior amount. |
| Protection from Elements | Choose Fire/Electrical/Cold/Poison; set that party resistance to `min(2L+5,200)`. |
| Levitate | Set levitate count to 1. |
| Light | Increment the light word modulo 65536; mark stepped when the obscurity value is nonzero (outside complete blackness). This includes maps that are not dark. |
| Wizard Eye | Set wizard eye and clear automap-on. It does not mark cells explored. |
| Clairvoyance / Walk on Water | Set their respective flags. |
| Day of Protection | Walk on Water, Heroism, Holy Bonus, Bless, four party elemental resistances and light count all receive the low byte of `L`. |
| Day of Sorcery | Power Shield, Clairvoyance, Wizard Eye, Levitate and light count all receive the low byte of `L`; automap-on becomes zero. |

`L` includes temporary level (`0x21fc9..0x21fed`). The Day spells copy that
level byte even into their flag/count fields; they do not substitute 1.
The pin substitutes true/1 for those fields. See
[Day-spell counters](../original-quirks.md#sv-day-spell-counters).
Its in-memory buff/light integers also retain values beyond the DOS byte/
word limits; see [effect widths](../original-quirks.md#sv-spell-effect-widths).
Wizard Eye's standalone flag assignment is also confirmed by the calibrated
[party byte check](evidence.md#party-effect-bytes); its established item entry
is retained.

Bless adds to Speed's stat bonus and equipment AC, together with temporary
AC, unless the caller requests base-only AC (`0x3bccf..0x3bd25`). There is
no condition test in that calculation. A disabling condition can separately
change a monster's attack path; it does not erase the Bless amount.

Heroism adds to the shared weapon hit bonus for each matching equipped
melee/missile weapon (`0x2e0c8..0x2e0d2`). It therefore affects melee, Shoot
and the weapon-accuracy path used by Poison Volley. It does not add to the
magic-resistance gate used by ordinary damage spells. Holy Bonus adds to
weapon damage at `0x2e0d6..0x2e0db`, before Adventurer's multiplication by
3 (`0x2e15d..0x2e165`); Shoot uses it too. A spell's nonzero prepared
damage replaces weapon damage, so Poison Volley's base remains 10.
The pin has the Heroism contribution but omits Holy Bonus's damage addition.
See [Holy Bonus](../original-quirks.md#sv-holy-bonus-damage).

Power Shield and the four party protections are flat damage subtractions,
not percentages or additions to personal resistance. Monster damage uses
the [retained combat sequence](combat.md#monster-damage-saving-throws-and-secondary-attacks):
initial Physical save for a nonphysical attack, party elemental subtraction,
personal elemental save loop, then Power Shield and a zero clamp before
the secondary-effect Physical save. Generic party damage (including traps,
Events and monster spell damage using that path) subtracts party protection,
runs the personal save loop, then subtracts Power Shield and clamps at zero
(`0x23e27..0x23f10`). Physical damage skips elemental protection/saves but
still subtracts Power Shield. The special death-type suffix replaces the
damage with current HP after that clamp (`0x23f10..0x23f25`). The existing
[terrain](time-and-conditions.md#terrain-arrival) and
[monster evidence](evidence.md#monster-damage-and-secondary-attacks) retain
their DOS differences from the pin; they are not new M76 discoveries.

Protection from Elements opens Select Element: Fire, Electricity, Cold,
Acid, in that screen order, using F/E/C/A or their buttons. Acid selects
party Poison protection. Escape refunds its own `L` SP/1 gem and makes no
assignment. Item power 14 uses the same dialog: cancellation retains the
spent charge and adds those SP/gems without an initial debit, as with the
previously established shared refund convention.

Light drain is attribute-byte bit `0x20`, latched by cell lookup in
DS:`f486` (`0x11fef..0x11ff4`). A stepped lighting check decrements a
nonzero light count once before checking map darkness (`0x12012..0x12084`).
The original CD mainland 23 and Vertigo maps 28/109/110/111 contain **no**
such cells; all five are not dark. Levitate is tested as nonzero, not
consumed per movement. It protects cloud arrival, not sky or an unconditional
scripted fall/pit. Walk on Water permits shallow and deep outdoor water;
Swimming alone permits shallow water. Indoor movement's separate raw-slot-4
check accepts either Swimming or Walk on Water (`0x0cf52..0x0cf8d`). Neither
provides extra terrain-arrival immunity. Rest/dawn temporary reset ends
both effects; script changes can also alter them.

The consumers are `XeenMovement` (Swimming/Walk on Water) and
`XeenWorld::prepareTerrainArrival`/`XeenTerrainRules` (Levitate). The existing
party skill test requires Swimming on all active members and does not filter
conditions (**DOS static, high confidence**, [shared skill evidence](evidence.md#outdoor-movement-skills)). Saved positions remain
valid on water when the temporary effect expires; the next attempted move
still checks the current abilities.
Light's stepped mark cannot drain light on the currently open maps, because
they contain no drain cells; the successful learned-cast suffix still performs
its ordinary arrival.

### Darkness and light timing

**DOS static, high confidence (2026-10-19).** With zero light, primary-map
flags2 bit `0x4000` makes the 3D scene completely black. Any nonzero light
clears that darkness. The ordinary rule selects only black/clear; the original
renderer also supports three partial-shading levels used by a separate UI fade.
The obscurer affects `[8,224) x [8,140)`, including monsters already drawn.
Minimap, full Map, exploration recording and border indicators remain available.
The flag test, independent pixel-renderer/Light checks and layer order are in
[the DOS evidence](evidence.md#darkness-and-lighting-continuations).

Turns and waits mark stepped; blocked movement does not. Scene preparation
checks light with that pending state and completed redraw clears it. A further
redraw with stepped clear cannot spend light. Travel can create multiple
check opportunities, especially Event Continue; learned casts, item wrappers,
falls and Run retain their own continuations. See [travel lighting checks](evidence.md#travel-lighting-checks).
Exact MMModern snapshot restore derives darkness without inventing a step;
the [native-load evidence limit](evidence.md#restore-boundary) is distinct.

`XeenWorld::sceneIsBlack` (`XeenWorldPresentation.cpp`) derives blackness from
the primary map and saved light word; `CloudsMapComposer` obscures the scene
before assembling the border.
The existing Encounter and combat owners publish a pending drain check through
their mutation guards. Stepped is transient presentation state, cleared by a
completed redraw and by exact restore; the save format is unchanged.

The initial Clouds resources contain 36 dark physical DAT maps and no drain
cells (128 DAT maps inspected). See [the complete ID list and traversal
witnesses](evidence.md#clouds-darkness-and-traversal-resource-inventory).

### Border and information screens

**DOS static, high confidence.** The border assembler is `0x1b9c6..0x1be1b`.
Animation advances once per assembly call, not per game minute; this static
check does not establish a universal wall-clock frame rate.

| Indicator | Resource, frames and position |
| --- | --- |
| Bless | `bless.icn`, active 0..3 at (33,137), with base frame 16 drawn first |
| Power Shield | `bless.icn`, active 4..7 at (55,137) |
| Holy Bonus | `bless.icn`, active 8..11 at (160,137) |
| Heroism | `bless.icn`, active 12..15 at (182,137) |
| Levitate | `border.icn`, active 16..27 at (0,82); inactive frame 16; counter advances modulo 12 even while inactive |
| Spot Secret Doors | `border.icn`, active 28..39 at (194,91); inactive frame 28; own counter `0x974e`, modulo 12, advances even while inactive (`0x1ba1e..0x1ba63`) |
| Danger Sense | `border.icn`, active 40..51 at (107,9); inactive frame 40; own counter `0x974f`, modulo 12, advances even while inactive (`0x1ba67..0x1baab`) |
| Direction Sense | The facing letter N/E/S/W, font color 8, centered at column 116, row 139 of window 0 (`0x1bdb8..0x1bdfd`, format `0x52a8a`); no animation |
| Fire/Electrical/Cold/Poison | `fecp.brd`, inactive/active pairs 0/1, 2/3, 4/5, 6/7 at (2,2), (219,2), (2,134), (219,134); alternate window layout uses 8/9, 10/11, 12/13, 14/15 at (8,8), (219,8), (8,134), (219,134) |

**Reference only** for character-sheet presentation: the Good condition row
adds one original `PLUS_14` marker for each active Bless, Power Shield, Holy
Bonus and Heroism, in that order. The condition expansion lists their names
and signed amounts after the character conditions, even for an ill member.
`XeenDialogView` reads the party context for these lines and the sheet AC;
this presentation has not received an additional DOS confirmation.

**Skill indicators (DOS static, 2026-10-18, high confidence;
[M77 evidence](evidence.md#m77-combat-items-quests-awards-and-skill-indicators)).**
Each of the three indicators needs its party skill through the shared skill
check (`0x215b9`): one active member with Spot Secret Doors (16), Danger
Sense (17) or Direction Sense (6) suffices; only Crusader and Swimming need
the whole party and Mountaineer and Pathfinder two members (`0x2160a`).
The complete helper has no condition filter, so dead and unconscious active
members count too (**DOS static, 2026-10-19, high confidence**;
[calibration and movement consumers](evidence.md#outdoor-movement-skills)).
Beyond the skill:

- **Danger Sense** lights while the monster-movement flag `0x974c` is set.
  The movement routine (`0x1fdc1`) clears it, then sets it for any monster
  whose position equals one of the 49 cells within three steps of the party
  on both axes (`0x1fe49`, `0x1fefb`), before the attacking, asleep or
  line-of-sight tests, so sleeping and unaware monsters count and walls do
  not matter. It is evaluated only when that routine runs: after each step
  and turn in exploration, at each combat round start and after an item Use
  in combat; it keeps its value in between, and keeps it when the routine
  returns early because monster movement is disabled or shots are in flight
  (`0x1fdc7..0x1fde4`).
- **Spot Secret Doors** lights while the thin-wall flag `0xf496` is set. The
  indoor scene setup (`0x12094`, the pin's `setMazeBits`) sets it when the
  cell ahead exists (its wall word is not `0x8888`) and the party's own cell
  carries wall value 5 or 8 on its facing side (`0x12303..0x1231e`, the
  pin's `_wo[27]` for cell index 2). The test looks one wall ahead only. The
  outdoor setup never writes the flag, so outdoors it keeps its last indoor
  value in DOS and in the pin alike; not observed in DOSBox play.
- **Direction Sense** writes the letter whenever the skill is present and no
  text or number prompt is open: the prompt routines set `0x8c52` on entry
  and clear it on return (`0x1e015`, `0x1e19f`, `0x1e16c`, `0x1e26c`).

All three are assembled with the border on every scene redraw, in
exploration and combat. The two animated indicators use separate counters
that advance together once per assembly and are zeroed together with the
Levitate counter at `0x1c402..0x1c410` and `0x1d4c4..0x1d4d2`, so they stay
in phase; the pin draws Danger Sense from the Spot Secret Doors counter after
incrementing it, one frame ahead
([quirk](../original-quirks.md#sv-danger-sense-frame)).

In the CD data, value 8 is the ordinary wall and `0x8888` a solid block, so
Spot Secret Doors lights only in front of a plain wall with open space behind
it: 43 walkable cell sides on the Clouds indoor maps, none in Vertigo (for
example Rivercity (6,7) facing North).

MMModern implements the three indicators in M77 Part C on these rules
(`CloudsMapComposer.cpp`; Danger Sense from `XeenActorApproach::dangerSense`
in the movement candidate, Spot Secret Doors from `XeenIndoorScene::build`).
DOS keeps the two sources in memory and never saves them; MMModern keeps
them in the scene presentation, which a load starts afresh, so after a load
Danger Sense stays off until the first step, turn or round and Spot Secret
Doors until the first indoor draw. **Unknown:** whether a DOS load inside a
running game shows the previous in-memory value until then. The Levitate, Spot Secret Doors and Danger Sense counters
start together and are not yet zeroed at the service screens' redraws
(`0x1c402`, `0x1d4c4`, the pin's `BaseLocation::drawBackground`).

The four buff counters advance modulo 4 while active; elemental indicators
are static presence tests. Clairvoyance reads each cell's two HED hint bytes
when the corresponding Yes/No, Who Will or location prompt installs them;
they are not a prediction calculated by the spell. Hint 2 is neutral, with
frames 0/8; hint 1 uses left 0..3/right 12..15; hint 0 uses left 4..7/right
8..11. The statues are at (0,32)/(215,32), in `border.icn`; an overlapping
window can replace the right statue with frame 52. Without Clairvoyance
they remain neutral. The pin's right hint-0 frames are 9..12, a
[one-frame discrepancy](../original-quirks.md#sv-clairvoyance-right-frames).
The HED bytes encode the good/bad hints; static control flow does not
independently establish the emotional meaning of each drawn frame.

Detect Monster opens window 19 with `detctmon.icn` on Clouds, showing the
7x7 square within three cells on each axis, north upward, without a wall or
line-of-sight test. Matching monster positions increment each marker to a
maximum of three; the party arrow shows facing. It does not list species or
HP. Any key/mouse dismissal closes it; an empty area still costs 6 SP.
Marker positions are (244+9*column,81+7*row), matching the pin
(`0x3017d..0x301a4`).

Identify Monster tests the three current attack slots, rather than combat
mode. With none it says Spell Failed and retains 5 SP cost. Otherwise window
17 lists each occupied slot in order, with name, current HP, AC, number of
attacks and special-attack description; a key/mouse dismissal closes it
(`0x2f8f8..0x2faf3`). Both information spells are admitted from the book and
their items in combat and exploration. These information-screen rules match
the pin.

### Item spell actions

**DOS static, high confidence; implemented (M76 Part B).** Enchant Item, Recharge Item and Item to Gold
first use the member prompt with their own IDs 19/57/39. Escape there refunds
30 SP/20 gems, 15 SP/10 gems, or 20 SP/10 gems respectively. Enchant and
Recharge items reach those same prompts, retaining their spent charge and
adding the refund. Later Escape from Items/selection has no spell refund.
Failed eligibility also retains the cost. Items modes are 6/4/11 respectively.

Enchant draws `U(1,floor(caster current level/5)+1)`, capped at 5, **before**
category/eligibility refusal (`0x31c91..0x31cca`). Only weapons and armor
with material 0 and state byte 0 qualify; broken, cursed and already
enchanted records refuse. Equipped frame alone does not refuse. The weapon
exception is ID **equal to 34**, not all higher IDs. The handler calls the
existing DOS generator in forced weapon/armor mode and copies only its
material/state, retaining the selected base item and frame. Generation uses
the [already settled draw order](items.md#generation-and-reward-delivery),
not a new enchantment formula. A zero-property result is copied and receives
the success sound selector too. The pin instead uses the initially selected
recipient's level, rejects every weapon ID at least 34 and adds an error for
a zero-property result. See [Enchant](../original-quirks.md#sv-enchant-caster-and-copy)
and [special item IDs](../original-quirks.md#sv-item-spell-special-id).

Recharge admits nonempty Misc containers with material at most 9, excluding
power 53. It draws `U(1,6)` once and normally adds charges up to 63, preserving
broken/cursed flags; there is no break or destruction draw. Broken items
are not refused. The DOS cap comparison masks the state with `0xbf`, which
drops the cursed bit (`0x40`) but keeps the broken bit (`0x80`): an
eligible broken item therefore always fills to 63 charges, while a cursed
one recharges normally (`0x31bae..0x31c3f`; bits confirmed by Curse Item
`0x2d685` and Break Weapon `0x2d52c`; confirmed in DOS play on 2026-10-18 with edited item states,
see the quirk). The pin always compares just the six-bit charge count. See
[broken Recharge](../original-quirks.md#sv-recharge-broken).

Item to Gold converts a selected nonempty item immediately, with **no extra
Yes/No confirmation**. Equipped, broken or cursed status does not refuse it;
the weapon exception is ID exactly 34. Quest counters are not inventory slots
and cannot be selected here. Gold is `max(1,full item value)`, using the
[DOS price formula](items.md#smith-prices), ignoring charges and condition,
then the item is cleared and its category compacted (`0x31ddd..0x31fad`).
The pin rejects all weapon IDs at least 34 and retains its previously
documented attribute-material pricing difference.

### Travel handlers and callers

**DOS static, high confidence** for the following mutations and admission.
Learned Enchant Item, Etherealize, Lloyd's Beacon, Super Shelter, Teleport
and Town Portal refuse combat with a full refund before calling the handler.
The other sixteen M76 spells are admitted. Items refuse powers 16, 21, 28,
42, 48 and 58 in combat. Power 55 shows the refusal notice but continues
into Town Portal: if its map flag allows it, the dialog and map load really
remain reachable. This is a
[DOS difference](../original-quirks.md#sv-town-portal-item-combat), not a claim
that every resulting combat state was observed in DOSBox.

Guards tested inside an admitted handler say Spell Failed and **retain**
SP/gems or the item charge. Escape from the travel dialogs does not refund.
Super Shelter's dying-risk confirmation likewise has no spell-cost refund
on decline; it cancels sleeping/time/recovery before those begin.

MMModern implements these six travel handlers in `XeenLearnedSpellRules::travel`;
the existing casting, combat and item owners publish the movement. Outdoors, a
Teleport or Etherealize destination beyond the border enters the neighbouring
map as walking does. Beacon stores map/X/Y/side on the caster's mutable roster record.
Cross-map Return and every Town Portal selection load through the existing
world transition, retaining per-map changes and rebuilding live monsters.
Every Clouds map can be a destination; Darkside and missing resources
refuse before payment. M77 Part A admits Town Portal items in combat
after the original notice; combat ends at the first attacker re-evaluation after
the load, with its ordinary minute (maintainer decision, 2026-10-19).
Travel dialogs hold payment until the chosen
destination is admitted, so that refusal spends nothing.
Empty/zero/Escape still commits the paid cancellation. Original-data automated
checks in `XeenTravelSpellOriginalTests.cpp` cover same-map movement, map loads,
per-character beacons and exact restore of the current save format. They are implementation evidence,
not an additional DOS observation. Destination Event timing after item travel
remains unconfirmed in DOS; see [the retained limit](../original-quirks.md).

| Spell | Map flag, mutation and facing |
| --- | --- |
| Etherealize | `0x40`; one cell forward, requiring full-word lookup different from `0x8888`; ignore intervening wall checks; preserve facing; no direct map load |
| Time Distortion | `0x400`; assign primary map run X/Y, preserve facing, redraw; no direct map load; works out of combat too |
| Teleport | `0x1000`; one-digit distance prompt in current facing; positive input 1..9; walls between start and destination are ignored; preserve facing; no direct map load |
| Lloyd's Beacon | `0x800`; Set stores the caster's map/X/Y/side; Return loads only if map or side changes, then assigns X/Y; preserve facing |
| Town Portal | `0x100`; select town 1..5; always load selected town, even the current one; Clouds uses that town's run X/Y; preserve facing |

Teleport Escape/empty/0 closes without moving. Indoors a destination full
word `0x8888` fails; outdoors raw destination surface 0 or 8 fails. Shared
lookup resolves loaded neighboring cells; a missing neighbor returns
`0x8888` indoors or 15 outdoors, while an excessive positive coordinate
returns `0x8888`. Do not replace these exact comparisons with a generic wall
or water test (`0x303e3..0x30541`, `0x11d40..0x11f03`). A later ordinary
outdoor boundary transition is distinct from a handler explicitly reloading.

Beacon's S/T Set and R Return buttons/keys or Escape use window 10. Before
display, map 0 initializes Clouds beacon to map 28, (18,4), side 0; even
Escape retains that initialization. Stored fields are character offsets
`0xa0`, `0xa1`, `0xa2`, `0x136`. Return to Clouds maps 75..78 after castle
completion fails. There is no separate direction in a beacon.

Town Portal's window 20 accepts 1..5, retries higher digits and treats
Escape/0 as cancel. Clouds destinations, from the CD initial map records:
Vertigo 28 (18,4), Nightshadow 29 (14,1), Rivercity 30 (17,1), Asp 31 (8,1),
Winterkill 32 (7,14). Restrictions precede dialog/default-beacon writes.

The handlers do not run Events or terrain directly. Exploration learned
casting reaches the existing monster/step-time/terrain suffix at `0x0d5ba`;
normal outer Event processing then sees the destination. Item use returns
through its existing monster response, without an immediate terrain call,
as already established for Jump. Combat Time Distortion moves/redraws and
returns to ordinary cast/turn bookkeeping; it does not directly issue Run.
The combat loop rechecks attack slots (`0x2ea99..0x2ead1`). Town Portal's
combat item route similarly returns to item bookkeeping after loading;
the notice is not a downstream admission gate. Exact DOSBox presentation
of that anomalous route remains unobserved. Super Shelter's separate Rest
entry and arrival are in [Rest](time-and-conditions.md#rest).

Jump moves two cells forward, retaining direction. Indoors both crossed
forward walls must be clear. Outdoors the first wall and destination wall
must differ from 1, and destination surface ID must be nonzero. A valid
jump marks the party stepped, so terrain arrival matters; it is not a
blanket safe traversal. Learned Jump reaches the successful-cast arrival
suffix, including damage or a fall; item Jump returns through the item-use
monster response, which does not call arrival. This distinction follows the
calibrated DOS call sites `0x0d411`, `0x0d5ba`, `0x1e7df` and item dispatch
`0x39e4b` (2026-10-17). MMModern enters a destination in a neighbouring
outdoor map as walking does and refuses Darkside or missing resources before
charge. See [terrain arrival](time-and-conditions.md#terrain-arrival).

## Item power lookup

**Evidence: DOS static (2026-10-12), high confidence for the mapping;
DOS static (2026-10-18), high confidence for the combat branches below.** Source: [casting][spells]
`Spells::castItemSpell`; original item dispatch at file `0x39e4b`, table
`[0x3a114,0x3a1a6)`. All 73 semantic destinations match the pin; power 33
uses the alternate Frostbite handler at `0x2fc77`. Incinerate power 64 reaches
`0x30b58`, which prepares the same 250 fire/single damage as learned Incinerate
at `0x30f0d` but uses different sound/projectile selectors. Thus matching effect
names do not prove identical presentation. Calibration uses known
First Aid, Cure Poison and learned-spell targets. See
[original-program evidence](evidence.md#spell-and-item-control-flow).
This maps power ID, not container/material ID.
**DOS static (2026-10-17), high confidence:** Frostbite power 33 prepares
35 Cold/Single and calls sound/projectile selectors 15/9
(`0x2fc77..0x2fc99`), matching `frostbite2`. Learned Frostbite retains
35 Cold/Single/projectile 8 but omits the initial FX8 call present in the
pin (`0x30eac..0x30ec6`); this does not mean the whole cast is silent.
See [Frostbite](../original-quirks.md#sv-frostbite-initial-fx) and
[item Incinerate](../original-quirks.md#sv-item-incinerate-selectors).
Selector calls are established; their perceived sound/animation remains
unverified by play.
Power 0 is invalid. Power 33 calls the special Frostbite animation handler;
its shared Frostbite damage uses projectile 9 instead of learned projectile 8.
Item Incinerate similarly selects projectile 2 instead of learned projectile 1.
FM requests follow the [DOS site inventory](audio.md#dos-fm-effect-call-sites).
Learned books and SP/gem costs
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

DOS item dispatch refuses powers 16, 21, 28, 42, 48 and 58 in combat.
Town Portal power 55 shows the notice but reaches its dialog and map load
when its map restriction is clear (`0x3a058..0x3a075`, `0x30654..0x30819`).
The pin returns after that notice. Both admit Enchant Item power 63,
unlike learned combat casting. See [M76 spells](evidence.md#m76-spells).

## Coverage and executable witnesses

The strongest retained DOS findings are the Pain, Suppress Poison,
elemental-damage and Magic Arrow corrections. Most of this chapter is a
carefully bounded reference inventory. Unknown high-impact areas include
high-tier overflow, exact post-item automatic-Event timing and the full DOSBox
presentation of Town Portal's combat item path. The M76 static traces settle
the ordinary travel handlers and dialogs above. Names and spell descriptions are not sufficient evidence
for these rules.

MMModern cross-checks (implementation evidence, not DOS proof):
[XeenLearnedSpellRules.cpp](../../src/games/xeen/XeenLearnedSpellRules.cpp),
[XeenCombatRules.cpp](../../src/games/xeen/XeenCombatRules.cpp),
[XeenLearnedSpellRulesTests.cpp](../../tests/XeenLearnedSpellRulesTests.cpp).
The tests cover cost/book identities, supported healing and condition
semantics, and supported spell damage.

[spells]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/spells.cpp
[constants]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[dialogs]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_spells.cpp
[combat]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp

## Moon Ray healing presentation

**DOS static, high confidence (2026-10-23):** after ranged damage, Moon Ray
visits every active party member in order, including dead, petrified and
eradicated members. For each it requests cue 30, draws U(1,30), applies the
shared healing rule and completes its sparkle before visiting the next
(`0x30aa1..0x30adf`). Terminal conditions reject healing/sparkle but retain
the cue and draw. Exact max HP still sparkles; already-above-max HP does not
(`0x2ee3a..0x2eed6`). The existing portrait owner supplies cue 20 and four
frames (`0x21087..0x21185`). Combat and exploration use the same retained
per-member preparation; RNG membership, count and order stay unchanged.
[Quirk](../original-quirks.md#sv-moon-ray-preparation).
