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

- [x] **Part 0 - DOS check (read-only, delegated).** Done 2026-10-17: all
      23 handlers match the pin in damage, type, range and cost. DOS
      differences are recorded in `docs/original-quirks.md`: learned
      Frostbite has no initial FX8, item Incinerate uses its own selectors,
      wrong-species Holy Word enters the miss path, and a negative player hit
      total hits (unsigned comparison). Rules and offsets are in
      [magic](rules/magic.md#damage-preparation-and-resolution).
- [x] **Part A - ranges and ordinary damage.** "All" range: the projectile
      continues through up to four groups (twelve positions), stopped by
      walls, each target settled before the next. One scalar draw per cast
      for variable damage (`U(a,b) * L`); one type draw for Elemental Storm
      and Prismatic Light; draws happen even with no target. The 18 spells
      above with their costs, gems, projectiles and damage tints, the DOS
      item selectors (Frostbite power 33, item Incinerate) and Moon Ray's
      party healing. Casting with no target spends the cost, as in DOS.
- [ ] **Part B - special spells and the hit total.** Finger of Death (two
      distinct saves), Holy Word (wrong species takes the miss path), Mass
      Distortion and Golem Stopper with their species gates; resource ID 89
      refuses spell damage. The player hit comparison becomes unsigned as in
      DOS, generically for every party attack (melee and Shoot).
- [ ] **Part C - Poison Volley and the shared volley.** Combat has no party
      volley, so the Shoot volley progression (one shot per member, a miss
      keeps the shot for later targets) moves from exploration's
      `serviceShoot` into one shared rule used by exploration and combat,
      keeping both owners and save v14. Poison Volley then works through
      the missile branch everywhere: weapon accuracy, 10 Poison plus the
      shooter's missile contribution, halved by the save.
- [ ] Developer test save `monster-damage` (as `monster-control`): Vertigo,
      casters with all 23 spells, ample SP and gems; README entry. Part B
      creates it; Part C adds Poison Volley.
- [ ] Regenerate `docs/coverage.md` in each part (41 of 76 spells after
      Part B, 42 after Part C); update `docs/rules/magic.md` where an
      implementation changes what it says.

## Acceptance

- Unit tests per spell: preparation, range, gates and save use, including
  the All range past the first group, zero-level and immunity boundaries,
  and Finger of Death's two draws. Exact RNG order per target as in M73.
- A negative hit total hits in melee, Shoot and Poison Volley; the
  original-data digests change only if a scenario reaches one.
- Original-data tests: a group spell (Fireball) on mainland Orcs and an All
  spell reaching two groups; one item-power cast. A save/load round trip
  after casting stays exact.
- Census: 42 of 76 spells supported; the rest still say "not supported yet".
- Complete `build-rel` suite through the runner (`tools/AGENTS.md`).
- Maintainer play-test with `monster-damage`: single, group and All spells,
  a type-gated spell refused on the wrong species, item use, save/load.

## Risks

- The current spell targeting holds at most three targets; All needs up to
  twelve. If that needs combat-state or ownership changes beyond targeting
  and resolution, stop and report before widening.
- Part C changes exploration Shoot, which already plays and has
  original-data digests: those must stay unchanged unless the hit-total
  fix explains a difference.
- Perceived sound and animation of the DOS selector differences and of the
  Holy Word miss are unplayed; audio stays deferred.
