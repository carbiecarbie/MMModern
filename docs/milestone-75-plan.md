# Milestone 75 plan - Party healing and conditions

**Tier B. Status: completed and accepted by the maintainer (2026-10-18)**
after a maintainer play-test. Plan `493f6f6`, DOS check and plan update
`821ed91`; implementation `c12e551` (Part A) and `071d62f` (Part B). Third
milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M74](milestone-74-plan.md).

## Scope

The 12 remaining spells that heal the party or cure its conditions, from
the spell book in combat and exploration, Quick Fight and charged items:
Nature's Cure, Power Cure, Cure Poison, Cure Disease, Cure Paralysis, Stone
to Flesh, Suppress Poison, Suppress Disease, Raise Dead, Resurrection,
Divine Intervention and Create Food, with item powers 26, 27, 37, 41, 44,
51, 56, 57, 66 and 73.

Out of scope: buffs, information, item and travel spells (M76), Temple
changes, combat inventory, map admission, audio.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`, with a
  follow-up): every handler, prompt, refund, guard, amount and item path
  settled. Kept DOS differences from the pin: Suppress Poison's and
  Resurrection's own refunds, Suppress Disease healing at count zero, the
  revival Endurance byte wrap and Create Food's 16-bit wrap. Evidence is in
  [magic](rules/magic.md#healing-and-conditions),
  [evidence](rules/evidence.md#party-healing-spells) and
  [original quirks](original-quirks.md).
- **Combat turn order** (maintainer decision): DOS and the pin spend the
  turn of a member skipped while disabled, so one re-enabled after their
  slot waits for the next round. MMModern let them act late in the same
  round; Part A fixed this generically for Awaken, the cures and revival
  ([combat rules](rules/combat.md)).
- **Witnesses adapted** (maintainer decision): the services, M38 and M40
  witnesses relied on the old order; their action policies and dependent
  expectations were updated and the services digest re-recorded. Mainland
  and Temple digests are unchanged.
- **HP and Unconscious are independent:** the Journey check that tied them
  was dropped, since DOS cures can separate them (Resurrection keeps HP).
- **Save v14 kept**: nothing new is persisted.

## Results

- Census: 54 of 76 spells supported. Developer test save `party-healing`
  ([development guide](development.md#developer-test-saves)) starts in
  Vertigo with a Cleric and a Druid who know all 12 spells, four patients
  with every condition, and charged items for the ten powers.
- Cancelling a member prompt refunds the spell's own SP and gems, from the
  book and from items.

## Validation

- Complete `build-rel` suite, unsandboxed: 230/230 for Part A and Part B.
- Maintainer play-test with `party-healing`: each cure, Raise Dead and
  Resurrection with cancel, Divine Intervention, Create Food, item use,
  combat casts and save/load.
