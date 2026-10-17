# Milestone 75 plan - Party healing and conditions

**Tier B. Status: planned.** Third milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M74](milestone-74-plan.md). Baseline `b55b7c6`.

## Goal

Every remaining spell that heals the party or cures its conditions works as
in the original, from the spell book (combat and exploration), Quick Fight
and charged items, following the
[healing and conditions rules](rules/magic.md#healing-and-conditions):

- **Healing and cures (8):** Nature's Cure, Power Cure, Cure Poison, Cure
  Disease, Cure Paralysis, Stone to Flesh, Suppress Poison, Suppress Disease.
- **Death and the whole party (3):** Raise Dead, Resurrection, Divine
  Intervention.
- **Create Food (1):** it adds food for the party and belongs to no M76
  group, so it joins this milestone.

Out of scope: buffs, information, item and travel spells (M76), Temple
changes, combat inventory, map admission, audio. No save change: the format
stays v14 (conditions, HP, permanent Endurance, temporary age and food are
already saved).

## Tasks

- [ ] **Part 0 - DOS check (read-only, delegated).** Compare the 12 handlers
      in `WORLD/XEEN.DAT` with the rules table and the pin: target prompt ID
      and cancellation refund, terminal-condition guards, the order of
      `addHitPoints` and condition clearing, amounts and draws (Power Cure's
      one draw, Suppress Disease's decrement, which in DOS may be 3 like
      Suppress Poison's), Raise Dead's and Resurrection's HP, Endurance and
      age changes, Divine Intervention's member set, age and HP refill,
      and Create Food's member count and overflow. Also whether these
      spells are admitted in combat, and how a member raised in combat
      rejoins the turn order. Record differences with offsets in
      `docs/original-quirks.md` (and `docs/scummvm-differences.md`); raise
      evidence labels in `docs/rules/magic.md` only where DOS settles them.
      Unsettled cases go to the maintainer before Part A.
- [ ] **Part A - healing and cures.** The eight spells of the first group and
      Create Food, single-member prompt where the original has one, with
      their costs, gems and refunds. Suppress Poison uses the DOS prompt ID,
      refund and decrement (closing the M60 "not implemented" quirk row).
- [ ] **Part B - death and the whole party.** Raise Dead, Resurrection
      (refund of its own cost on cancel, as DOS) and Divine Intervention,
      in exploration and combat, including a member returning to combat
      after being raised, as Part 0 establishes.
- [ ] Item powers for the spells that have one (26, 27, 37, 41, 44, 51, 56,
      57, 66, 73) use the same effects and the M61 charge rules.
- [ ] Developer test save `party-healing` (as `monster-damage`): Vertigo, a
      Cleric and a Druid with all 12 spells, ample SP and gems, other members
      Poisoned, Diseased, Paralyzed, Stoned, Dead and Eradicated, and a few
      charged items with these powers; README entry.
- [ ] Regenerate `docs/coverage.md`.

## Acceptance

- Unit tests per spell: prompt, cancellation refund, terminal guards,
  condition order (Unconscious kept or cleared as the rules say), amounts,
  Power Cure's single draw, Suppress counts at 0, 1-3 and 4+, Raise Dead
  and Resurrection Endurance minimum and age cap 250, Divine Intervention
  leaving Eradicated, Create Food overflow as Part 0 settles it.
- Original-data tests: a cure in exploration, Raise Dead in mainland combat
  with the raised member acting, and one item-power cast. A save/load round
  trip after casting stays exact.
- Census: 54 of 76 spells supported; the rest still say "not supported yet".
- Complete `build-rel` suite through the runner (`tools/AGENTS.md`).
- Maintainer play-test with `party-healing`: each cure on its condition and
  on a member without it, Raise Dead and Resurrection with cancel, Divine
  Intervention, Create Food, combat casts, item use, save/load.

## Risks

- If returning a raised member to combat needs combat-state changes beyond
  the existing turn order, stop and report before widening.
- If Part 0 shows many DOS differences, the maintainer decides whether M75
  keeps its scope or splits.
