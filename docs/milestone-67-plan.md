# Milestone 67 - Equipped item contributions

**Tier B. Status: approved; implementation not started.**

## Goal

Equipped items count in combat, Shoot and Rest as in the original, for every
material, attribute bonus, enchantment and accessory, and the old Journey
validation that refuses them is removed. This is step 1 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems): today an
equipped material or enchanted item makes combat, Shoot or Rest say "not
supported yet", which already affects Vertigo's level-2 loot.

Authority: [the items and combat rules](rules/items.md), the census
(`Equipment combat/Rest` in [the coverage report](coverage.md): 7 supported,
386 unsupported rows), calibrated analysis of `WORLD/XEEN.DAT` where it settles
a formula, and pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`
(`Combat::getWeaponDamage`, `getMonsterDamage`, `getDamageScale` and the
elemental branch, `Character::getStat`, `getArmorClass` and resistances).

## Tasks

1. **Inventory first.** List what each material range, attribute bonus,
   weapon state (effectiveness against monster types) and accessory changes,
   which of these the engine already computes and where, and what
   `xeenValidateJourneyMelee` refuses. Report this before changing rules.
2. **Weapons.** Metal materials add their hit and damage bonuses; elemental
   materials add their elemental damage; effectiveness multiplies damage
   against its monster type; ranged weapons follow the same rules in Shoot.
3. **Armor and accessories.** Material and enchantment contributions to armor
   class, attributes and resistances, with the existing broken and cursed
   rules.
4. **Attribute bonuses.** Equipped attribute materials change the effective
   attributes everywhere the engine uses them (hit chance, damage, HP/SP
   maximums, saves), as the original does.
5. **Remove the refusal.** `xeenValidateJourneyMelee` keeps only checks that
   still protect state; combat, Shoot and Rest accept every equipped item the
   original accepts.
6. **Census and docs.** Regenerate `docs/coverage.md`; the Equipment
   combat/Rest rows become supported. Update the rules chapters and their
   evidence labels; record any original or reference oddity kept on purpose in
   [original quirks](original-quirks.md).

## Boundaries

- No change to the random request order or count beyond what the original
  does; if an original contribution adds or moves a draw, stop and report it,
  since that would make this milestone Tier A.
- No save-format change. No combat Equip/Use (step 6 of the arc).
- Run the M44 audit; the scenarios' parties may already carry such items, so
  any difference is explained and approved before digests change.

## Acceptance

- Unit tests per material range, attribute bonus, effectiveness and accessory,
  with worked examples from the rules; an original-data test that fights,
  shoots and rests with level-2 equipment bought at the Ironworks.
- Complete `build-rel` suite through the runner; M44 audit gate.
- Maintainer play-test: buy level-2 weapons, armor and accessories, equip
  them, fight, shoot and rest, and compare damage and AC with DOSBox where
  practical.
