# Milestone 74 plan - Monster damage spells

**Tier B. Status: planned.** Second milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M73](milestone-73-plan.md). Baseline `c185062`.

## Goal

Every remaining spell that hurts monsters works as in the original, from the
spell book (combat and exploration), Quick Fight and charged items, using the
M73 delivery, the monster save and the [magic rules](rules/magic.md).

- **Ordinary damage (18):** Acid Spray, Cold Ray, Dancing Sword, Deadly Swarm,
  Elemental Storm, Fantastic Freeze, Fiery Flail, Fireball, Frostbite,
  Implosion, Incinerate, Inferno, Lightning Bolt, Mega Volts, Moon Ray,
  Prismatic Light, Star Burst, Sun Ray.
- **Monster-specific (4):** Finger of Death, Holy Word, Mass Distortion,
  Golem Stopper.
- **Missile branch (1):** Poison Volley.

Out of scope: party healing and conditions (M75), buffs, information, item and
travel spells (M76), map admission, combat inventory, audio. No save change:
the format stays v14 (nothing new is persisted).

## Tasks

- [ ] **Part 0 - DOS check (read-only, delegated).** Compare each of the 23
      handlers' prepared damage, type, range, cost and save use in
      `WORLD/XEEN.DAT` with the pinned reference, as Pain was (M60). Record
      each difference with offsets in `docs/original-quirks.md` (and
      `docs/scummvm-differences.md`), and the resource ID 89 (Lord Xeen)
      spell refusal. Unsettled cases go to the maintainer before Part A.
- [ ] **Part A - ranges and ordinary damage.** "All" range: the projectile
      continues through every reachable group behind the first until a wall,
      not just the first three targets. One scalar draw per cast for
      variable damage (`U(a,b) * L`); one type draw for Elemental Storm and
      Prismatic Light. The 18 spells above with their costs, gems, icons and
      damage tints; the Frostbite item variant as the reference has it.
- [ ] **Part B - special spells.** Finger of Death (two distinct saves),
      Holy Word, Mass Distortion, Golem Stopper with their species gates;
      Poison Volley through the missile branch, not the ordinary formula.
- [ ] Developer test save `monster-damage` (as `monster-control`): Vertigo,
      casters with all 23 spells, ample SP and gems; README entry.
- [ ] Regenerate `docs/coverage.md`; update `docs/rules/magic.md` evidence
      labels only where Part 0 gives DOS evidence.

## Acceptance

- Unit tests per spell: preparation, range, gates and save use, including
  the All range past the first group, zero-level and immunity boundaries,
  and Finger of Death's two draws. Exact RNG order per target as in M73.
- Original-data tests: a group spell (Fireball) on mainland Orcs and an All
  spell reaching two groups; one item-power cast. A save/load round trip
  after casting stays exact.
- Census: 42 of 76 spells supported; the rest still say "not supported yet".
- Complete `build-rel` suite through the runner (`tools/AGENTS.md`).
- Maintainer play-test with `monster-damage`: single, group and All spells,
  a type-gated spell refused on the wrong species, item use, save/load.

## Risks

- If the All range needs combat-state changes beyond targeting (target lists
  longer than three, deaths mid-projectile), stop and report before widening.
- If Part 0 shows many DOS differences, the maintainer decides whether M74
  keeps its scope or splits.
