# Milestone 61 plan - Items for the loot

**Tier A, in two parts. Status: approved; implementation not started.** Baseline
`c09dcdb6179755b16de647c936a418b0368f1d95` (`main = origin/main`, clean
before this plan). Authority: installed World of Xeen CD, original program
evidence below, pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
This is planning only; roadmap step 7 does not authorize implementation.

## Goal and boundary

Make Vertigo loot useful through the original Ironworks and character sheet:
Buy the exposed Accessories and Miscellaneous stock, Sell, Identify, Fix all
four categories, and Use and Discard from the sheet outside combat. Combat
Equip/Use, later regions' items and effects requiring systems outside the
Vertigo arc remain deferred. An unsupported effect says "not supported yet"
before any charge, item, time or RNG change.

Tier A applies to the whole milestone because item use can change time, RNG,
conditions and saved party effects, while service actions must preserve exact
transaction ownership. Split delivery: **Part A** finishes Smith Buy/Sell/
Identify/Fix and sheet Discard/Use admission; **Part B** routes item effects that already
exist as spells (M60) or as the antidote through shared casting rules; all other
powers stay "not supported yet" (maintainer decision, 2026-10-12). Schedule independent plan and
implementation reviews for each part. The maintainer's DOSBox play-test and
acceptance close the milestone.

## Reachable catalogue (exhaustive generation-table ranges)

Source: original `maze0028.evt`, `maze0023.evt`, maps' MOB and `xeen.mon`
records, initial Clouds merchant stock, and M57's shared `makeItem` tables in
`XeenMerchantTables.h`/`XeenItemGeneration.cpp`. These are symbolic branch
ranges, not random samples. L1 permits weapons 1-33, armor 1-7 and Misc
containers 1-9; no Accessories. L2 permits weapons 1-33, armor 1-13,
Accessories 1-10 and Misc containers 1-9. A generated Misc record stores the
container in `material`, the power in `id`, and 1-8 charges in `state`.

| Source | Reachable item outcome |
| --- | --- |
| Map 28 crates (9,28), (8,28), (9,26), (9,25), (3,25); beds (3,17), (28,7), (3,5), (28,3) | L1: weapons 1-33; armor 1-7; Misc container 1-9 with power 1-15. |
| Map 28 display cases | Scripted L1/L2 generation: the same L1 range plus L2 weapons 1-33, armor 1-13, Accessories 1-10 and Misc powers 16-30. The lock/trap/jail outcome can prevent delivery. |
| Map 28 rubbish (3,12) | L2: weapons 1-33; armor 1-13; Accessories 1-10; Misc container 1-9 with power 16-30. |
| Map 23 Myra return reward | Five fixed Potions of antidotes `{material=10,id=37,charges=1}`; its two scripted random item draws use the L1 generator. |
| Map 23 monsters | Orc is the only item-dropping kind: each of 12 Orcs has a 10% L1 generation chance. Weapons 1-33, armor 1-7 or Misc containers 1-9 with powers 1-15 and 1-8 charges. **Implemented (Part C):** Misc staging and shared delivery follow the [DOS evidence](rules/evidence.md#monster-treasure-staging), preserving generation draws; Part D implements the separately confirmed full-pack and recipient fallback rules. Giant Snake, Skeleton, Zombie and Giant Toad have no item drop. |
| Map 28 monsters | Slime, Doom Bug and Breeder Slime have no item drop. |
| Initial Ironworks stock | Fifteen L1 and five L2 generated entries. L2 is the source of its Accessories and higher powers; stock may include all four categories. |

L1 non-Misc material/state are zero. L2 non-Misc enchantment is exactly one of:
metal materials 37-40/46-49; elemental materials
1-3, 9-11, 16-17, 21-22, 26-28, 34; attribute materials
59-62, 69-71, 77-79, 85-87, 93-94, 99-100, 105-106, 110-111,
116-117, 121-122. L2 weapons also have an optional slaying/effectiveness
counter 1-6 (otherwise 0); armor and Accessories have state 0. These are
passive equipment properties, not sheet Use powers. Generation makes no
cursed or broken item. Existing starting equipment and combat can expose
broken armor; rules and service tests must cover broken/cursed records in
every category even when this loot generator does not produce them.

## Use effects and support boundary

Pinned `spells.cpp::castItemSpell` maps Misc `id` to the following spells;
`character.cpp` and `dialogs_items.cpp` gate sheet Use, and Myra's fixed
antidote is already implemented. **Existing** means an M60 spell or current
condition/antidote rule can be reused; **deferred** means the spell is not
implemented yet; **later** needs a system outside this arc. Deferred and later
powers refuse without spending. The pinned power-to-spell mapping is the rule;
verify it through REA before Part B if any mapping looks doubtful.

| Powers | Effect and disposition |
| --- | --- |
| 1-6 | Light **later** (darkness/drain); Awaken, Magic Arrow, First Aid, Flying Fist, Energy Blast **existing**. |
| 7-10 | Sleep **later** (monster status); Revitalize, Cure Wounds **existing**; Sparks **deferred** (shared projectile/damage). |
| 11-15 | Shrapmetal **existing**; Insect Spray and Toxic Cloud **deferred** (shared damage delivery); Protection from Elements **deferred** (saved party resistance field); Pain **existing**. |
| 16-21 | Jump **existing**, retaining M60 unsafe-terrain refusal; Beast Master **later** (monster control); Clairvoyance **deferred** (world information); Turn Undead **later** (monster type/turn); Levitate **deferred** (party/terrain effect); Wizard Eye **existing**. |
| 22-26 | Bless **deferred** (party combat modifier); Identify Monster **later** (monster information screen); Lightning Bolt **deferred** (damage delivery); Holy Bonus **deferred** (party modifier); Power Cure **deferred** (healing). |
| 27-30 | Nature Cure **deferred** (25 HP healing, [DOS-confirmed](rules/magic.md#healing-and-conditions)); Lloyd's Beacon **later** (saved travel anchors); Power Shield and Heroism **deferred** (party effects). |
| 37 | Cure Poison: Myra's Potion of antidotes **existing**; use its established target, cure and one-charge settlement. |

**Maintainer decision (2026-10-12): only existing effects.** Item Use supports
the powers marked **existing** and Myra's antidote. Powers marked **deferred**
are spells not implemented yet (Sparks, Insect Spray, Toxic Cloud, Protection
from Elements, Clairvoyance, Levitate, Bless, Lightning Bolt, Holy Bonus, Power
Cure, Nature Cure, Power Shield, Heroism), and **later** ones need systems
outside this arc; both refuse with "not supported yet" before any charge, time
or RNG change. Because item effects are the shared spell rules, implementing
those spells later enables the matching items without item-specific work. Item Use reuses the M60 effects (targets, conditions, projectiles, damage
and RNG) but has its own admission and settlement: no spellbook, SP or gem
requirement, **no time charge** (DOSBox evidence below; the existing antidote
also charges none), and exactly one normal monster response. Cast timing
(1 minute indoors, 10 outdoors) does not apply to items. Item cost/charge
logic stays with inventory. As in the reference, an admitted use decrements
the charge before the effect and removes an exhausted item; M61 preflights
support and admission first.

## Original program price evidence and confidence

The persistent fully analyzed Ghidra project is
`D:\MModern-re\ghidra-xeen-dat\WorldXeenDat.gpr`, outside the repository.
Headless Ghidra 12.1.4 imported CD `WORLD/XEEN.DAT` as MZ/x86 real mode at
load segment `0x1000` and completed autoanalysis. DAT SHA-256:
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
The outside-repo REA launcher sets a 20-minute Ghidra startup/tool timeout
and a 60-minute total call limit; the persistent headless import used no
short analysis timeout.
Ghidra linear address = file offset + `0xa940`. Because autoanalysis omitted
cross-references from some undecoded pricing code, targeted disassembly and
direct table-reference search found one common routine at file `0x3c67d`.
Its category dispatch is `0x3c6d5-0x3c6e7`; it reads weapon `0x4fa2e`
(`0x3c6fc`), armor `0x4fa74` (`0x3c880`), Accessories `0x4fa90`
(`0x3c9fa`), and Misc material/base `0x4faa6`/`0x4fb7c`
(`0x3cb7e`/`0x3cb9f`). The action callers are `0x3ab3e`, `0x3ac8d`,
`0x3add1`, `0x3aef5`; the routine's mode/skill dispatch is
`0x3c6ab-0x3c6d5`, category price branch `0x3c81f-0x3c865`, Misc price
branch `0x3cbbb-0x3cbf4`, and divisor bytes `{1,2,100,10}` at `0x4d880`.

**Calibration: passed (high confidence).** Original weapon table ID 1 = 50;
armor table ID 1 = 20 and ID 10 = 40. Buy gives 50/20/40, matching M47's
Buy rule/tests and the maintainer's Smith play-test; Fix gives
`max(1,20/10)=2` and, for armor ID 10 with material 38,
`max(1,(40/4)/10)=1`. These reproduce M38's DOSBox-checked Seymour repairs.
Table arithmetic and mode
dispatch, rather than the port's calling convention, establish:

| Verdict | Original file offsets | Confidence |
| --- | --- | --- |
| Buy uses the full item value, material adjustment/addends included; minimum 1 gold. | `0x3c6be`, `0x3c81f`, `0x3cbbb`, tables above | High, calibrated. |
| Sell uses half value without Merchant and full value when `Merchant & 0x7f` is nonzero (the routine masks the skill byte first, so 128 gets half value); minimum 1. Cursed items and slayer swords are refused provisionally per reference. | `0x3c6c2`, `0x3c81f`, `0x3cbbb`; service callers above | High for price/Merchant; refusal needs REA or DOS. |
| Identify costs `max(1,value/100)` and reveals details without a durable identified bit provisionally per reference. | `0x3c6cd`, `0x3c81f`, `0x3cbbb` | High for price; display/state needs REA or DOS. |
| Fix costs `max(1,value/10)` in all categories, independent of Merchant or the caller's `actionIndex`; broken clears on confirmed payment provisionally per reference. | `0x3c6d2`, `0x3c81f`, `0x3cbbb` | High for price; refusal/mutation needs REA or DOS. |

For Misc, the priced value is container/material cost plus special-power base
cost; the stored charge count does not enter this routine. For the other
categories, the metal multiplier/division is applied before elemental or
attribute addends and the action divisor. Those arithmetic branches are in
`0x3c6e7-0x3c81f`, `0x3c86b-0x3c9e5`, `0x3c9e5-0x3cb69`, and
`0x3cb69-0x3cbbb` (high confidence for table arithmetic, calibrated on the
plain/material-38 armor subset).

The pinned reference's Repair call supplies `actionIndex` where Sell supplies
Merchant. The DOS price routine overwrites that argument for Fix, so it is
price-inert; the service listing callers instead pass the character skill
byte at `0x3ab29-0x3ab32`, `0x3ac78-0x3ac81`, `0x3adbc-0x3adc5`,
`0x3aee0-0x3aee9`. [Original quirks](original-quirks.md) records the
reference discrepancy. No Use effect or service refusal was settled by this
price calibration; use the pinned reference provisionally and verify those
rules before implementing.

## Service and sheet design

Use the original Ironworks Items dialog: Weapons/Armor/Accessories/Misc tabs,
Buy/Sell/Identify/Fix actions, numbered rows 1-9, B/S/I/F and W/A/C/M
hotkeys, portraits or F1-F6 for owner, Escape to back out, mouse parity. Buy
refuses a full destination category; Sell refuses cursed items and the special
slayer sword; Fix refuses intact items; Identify accepts an occupied item and
shows its original detail window. Confirm before changing gold or inventory.
Identify has no persistent "known" flag in the pinned reference. Fix clears
broken and preserves curse, charges, material and other item bytes. Sell
removes and compacts once; Buy debits, transfers stock and compacts once.
Existing Ironworks departure costs one day even after cancellation; individual
transactions add no time in the reference. Verify the refusals, display,
state and time against the original before implementing.

On the original character sheet, Use is a Misc action, requires an actionable
owner, world map, non-bad item and positive charges. Supported use spends one
charge, removes/compacts on zero, applies its spell effect and normal original
world response exactly once. Discard asks for confirmation and refuses cursed
items; accepted discard clears/compacts without a price. The reference's
`discardItem` returns true even after refusal/cancel, a return-value oddity;
only the visible action and item state matter. Verify any uncertain admission
and post-use time/monster step with REA before implementation.

Keep inventory and item bytes in Party/roster, Smith transactions in
Service/Smith flow, random generation in M57's common item generator, and
spell-like effects in M60's casting rules. No new coordinator is justified.
Use one quote/admission preimage and one publication for each paid operation,
charge, discard or stock move; stale input does not replay. Cancellation
**before admission** (closing the item list, refusing "not supported yet",
No in a confirm) changes nothing. Cancellation **inside an admitted effect**
(for example Escape at the target prompt) keeps the spent charge, removes an
exhausted item and settles the world response, as the existing antidote does
(`tests/XeenRegionalActionOriginal.cpp`) and the reference's
decrement-before-casting flow implies; supported failures also spend it.

**Part C save decision (maintainer approved, 2026-10-12):** use envelope v12,
retaining schema 9 and content 14. Keep the pending weapon/armor records and
add a separate pending Misc array (ten slots, unique Orc provenance across
categories, at most twelve sources), including exact save/load and validation.
Reject v11 clearly; do not retain a legacy reader. Pending Misc survives Run
and later delivery through the existing treasure owner and shared recipient,
capacity and gold rules. Preserve generation draws and their order. Follow the
recorded DOS staging evidence and First Aid item Escape rule: spend the charge
and add 1 SP to the user without a maximum-SP clamp; no other item power gains
this refund. No new owner or coordinator is needed.

Run the M44 Preserve/Compare audit around the change. Format-related byte
differences are expected, but traces and every common decoded snapshot field
must remain identical. Explain every added field and envelope version, length
and CRC delta; stop for maintainer digest approval after Compare. Any trace
difference stops the task. Independent Tier A plan and implementation reviews,
the complete build-rel suite, and maintainer play-test remain required.

## Part D: original treasure delivery

Maintainer-authorized Tier A follow-up (2026-10-13): apply the high-confidence
original-program delivery findings to Event and monster loot, including Misc.
Use one helper in the existing reward owner: globally full packs lose remaining
items only; valid preferred recipients use eligible-first selection, while an
absent/invalid preference goes directly to unrestricted capacity fallback.
Category-only overflow replaces the first member's tail and retains its frame.
Generation, draw order and staged category order remain unchanged.

Retain envelope v12, schema 9, content 14: stored records and their interpretation
do not change. Add rule tests and a generic round-trip, update the quirks/rules
with DOS offsets and unreachable recovery/Break Weapon boundaries, preserve and
compare M44 digests, and stop for approval of any difference. Tier A plan and
implementation reviews, the complete build-rel runner suite and maintainer
play-test are required; this amendment does not record milestone acceptance.

## Acceptance and genuinely open DOS questions

- Pure rules tests: every category's Buy/Sell/Identify/Fix table prices,
  Merchant skill bytes 0, 1, 127, 128 and 255 (masked with `0x7f`), minimum price, material order, cursed/broken and
  full-pack refusals, preserved item bytes, charges, zero-charge removal,
  supported effects, target failures, time and RNG draw order. Unsupported
  effects, pre-admission cancellation and stale actions draw and mutate
  nothing; admitted target cancellation and supported failures spend the
  charge. Test successful and failed item uses indoors and outdoors (no time).
- Original mouse/keyboard parity for every Smith and sheet action; exactly
  once payment/credit/item/effect publication under retry; generic save/load
  round-trip. A few original-data runs must find loot in a Vertigo container,
  Identify and Use it where supported, then Sell it or another found item.
- Complete `build-rel` CTest suite, Debug closure check, independent Tier A
  plan/implementation reviews, and maintainer DOSBox play-test. This planning
  task performs none of those implementation checks.

Maintainer DOSBox CD evidence (2026-10-12) for the previously open questions:

1. **Broken items at the Ironworks.** Identify and Sell treat a broken item like
   any other (Sell at its normal price); Fix repairs it. While broken, the item
   name shows only "Broken" plus the base type (for example "Broken helm"
   instead of "Ebony helm"); Identify's detail window still shows the full
   properties (screenshot: "Broken cudgel" with "+5 Elec Damage"). No durable
   identified flag was observed: Identify only opens the detail window.
2. **Item Use.** Using an item does not advance time. A failed use still spends
   the charge (Jump into a wall gave "Spell failed"; Enchant Item on an invalid
   target); normal use and the last charge behave as expected, and the item
   disappears at zero charges.
3. **Discard.** The sheet asks "Discard which item?", then "Permanently discard
   <item>?" with the original Yes/No buttons; Yes removes the item, No keeps it.

Cursed items could not be tested (none reachable in a reasonable time); their
Sell/Discard refusals follow the pinned reference provisionally.
