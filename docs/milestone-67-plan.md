# Milestone 67 - Equipped item contributions

**Tier B. Status: completed and accepted.** Plan `a6631f8`; implementation
`11d3d72`; version 0.1.2 `3d08d2a`. The maintainer accepted the play-test on
2026-10-15; released as v0.1.2.

## Goal and final scope

Step 1 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems):
equipped items count in combat, Shoot and Rest as in the original. Before M67,
an equipped material or enchanted item, including Vertigo's level-2 loot, made
them say "not supported yet".

- **Weapons.** Metal materials add their hit and damage bonuses before
  Adventurer's triple. Elemental materials add their elemental damage once,
  after physical resistance, scaled by the monster's matching resistance.
  Effectiveness counters triple melee damage against their monster type. The
  Slayer Sword bypasses physical resistance. Ranged weapons follow the same
  rules in Shoot, where the monster save halves the combined damage.
- **Armor class.** One shared scan (armor strength, metal and the AC attribute
  bonus) serves the character sheet and combat, so metal armor now counts in
  combat too.
- **Attributes and resistances** from items were already computed; the old
  Journey validation that refused them keeps only the checks that protect
  state.
- **Presentation.** An elemental hit shows its second splat frame beside the
  physical one.

## Decisions

1. **Tier B confirmed.** No contribution adds or moves a random draw; the save
   format is unchanged.
2. **Cloud draft, finished locally.** The implementation was drafted in a
   cloud session without the game data and completed on Windows: the census was
   regenerated with the real tool, and the original-data test fixtures were
   corrected (existing armor kept, live actors within their base HP, a quiet
   Rest cell).
3. **Formulas follow the pinned reference** with the DOS table and gate
   offsets already recorded; the maintainer's play-test confirmed equipping and
   use, since damage and monster HP are not visible in the original.

## Results

- Census: Equipment combat/Rest 393 of 393 supported on both sides (was 7).
- `xeen_equipment_original` buys level-2 Ironworks items, then checks armor
  class, a metal-sword attack against an oracle on the same RNG, Shoot with a
  fire crossbow and Rest with an HP ring.
- Complete `build-rel` suite 210/210 (172 s); `ctest -L fast` 173/173; M44
  audit zero differences.

## Deferred

The Cursed condition's hit penalty and the party Blessed and Heroism bonuses
(they need conditions and spells that arrive later in the arc).
