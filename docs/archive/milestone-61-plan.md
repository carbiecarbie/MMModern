# Milestone 61 plan - Items for the loot

**Tier A, in four parts. Status: completed and accepted.** Plan `7b942c6`;
Part A `5e0542c`; Part B `9cc4def`; Part C `dc056ae`; Part D `cbebb10`. The
maintainer accepted the Debug play-tests (2026-10-12/13). The rules reference
(`760cfe8`) and its findings (`d5067c4`) landed during this milestone.

## Goal and final scope

Make the items Vertigo's loot yields usable through the original interface
([roadmap](../roadmap.md) step 7). Authority: the World of Xeen CD resources,
calibrated analysis of the original program `WORLD/XEEN.DAT` (persistent Ghidra
project outside the repository), maintainer DOSBox evidence (2026-10-12) and
pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Catalogue.** Crates, beds and display cases give level-1 items; display
  cases and the rubbish at `(3,12)` give level-2 items (Accessories, enchanted
  equipment, Misc powers 16-30); only map-23 Orcs drop items; the Ironworks
  stocks 15 level-1 and 5 level-2 items; Myra gives five antidotes.
- **Part A - Ironworks and sheet.** Buy (now including Accessories and Misc),
  Sell, Identify and Fix for all four categories, with the original Items dialog,
  hotkeys, confirmations and mouse parity; sheet Discard with the original
  prompts. Prices from the original routine: Buy full value, Sell half (full when
  `Merchant & 0x7f` is nonzero), Identify value/100, Fix value/10, minimum 1.
  Broken items identify and sell normally, Fix repairs them, and their names hide
  material and suffix; Identify keeps no durable flag.
- **Part B - item Use.** Charged items use the M60 spell effects and the antidote
  with item-specific rules: no spellbook, SP, gems or time; the charge is spent
  before the effect, also on target cancellation and supported failures; the
  item disappears at zero; one monster response follows. Powers whose spells are
  not implemented refuse before spending anything.
- **Part C - original-program corrections.** Misc monster drops are delivered
  (the reference loses them); escaping a First Aid item's target prompt adds
  1 SP to the user without a maximum clamp. **Save v12** stores pending Misc
  treasure.
- **Part D - treasure delivery.** Full packs lose only the remaining items while
  gold and gems are credited; the final recipient search accepts asleep,
  disabled or dead characters; when only one category is full, the item replaces
  that category's last item on the first character, keeping its equipped mark.

## Decisions

1. **Only existing effects in Part B** (maintainer, 2026-10-12): Sparks, Insect
   Spray, Toxic Cloud, Protection from Elements, Clairvoyance, Levitate, Bless,
   Lightning Bolt, Holy Bonus, Power Cure, Nature Cure, Power Shield and Heroism
   stay "not supported yet"; implementing those spells later enables the items.
2. **Original program over reference** for prices, Misc drops, the First Aid
   item refund and delivery; recorded in [original quirks](../original-quirks.md)
   and the [rules reference](../rules/README.md). Cursed refusals follow the
   reference provisionally (no cursed item was reachable for DOSBox testing).
3. **Save v12** (schema 9, content 14) for Part C; M44 digests replaced after the
   maintainer-approved audit (traces identical). Parts A, B and D kept the
   format and the digests.

## Results

- Tests for prices per category and Merchant byte, refusals, broken naming,
  Discard, every supported power, charges, cancellation and failure, timing,
  delivery rules and exact save round-trips, plus original-data runs.
- Full CTest 187-188/188 on `build-rel` after each part; Debug closure check
  passed; independent Tier A plan and implementation reviews passed.
- Maintainer Debug play-tests accepted, including full packs, category-full
  replacement and the First Aid item Escape.

## Deferred

The thirteen spell-backed powers above and the "later" powers (Light, Sleep,
Beast Master, Turn Undead, Identify Monster, Lloyd's Beacon); combat Equip/Use;
cursed-item confirmation in DOS; the original treasure presentation sequence.
