# Milestone 74 plan - Monster damage spells

**Tier B. Status: completed and accepted by the maintainer (2026-10-17)**
after a maintainer play-test. Plan `3ac560a`, DOS check `fe48d81`, plan
split `2361596`; implementation `99a7518` (Part A), `746acbd` (Part B) and
`ac4889b` (Part C). Second milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M73](milestone-73-plan.md).

## Scope

The 23 remaining spells that hurt monsters, from the spell book in combat
and exploration, Quick Fight and charged items: 18 ordinary damage spells
(Acid Spray, Cold Ray, Dancing Sword, Deadly Swarm, Elemental Storm,
Fantastic Freeze, Fiery Flail, Fireball, Frostbite, Implosion, Incinerate,
Inferno, Lightning Bolt, Mega Volts, Moon Ray, Prismatic Light, Star Burst,
Sun Ray), four monster-specific ones (Finger of Death, Holy Word, Mass
Distortion, Golem Stopper) and Poison Volley.

Out of scope: party healing and conditions (M75), buffs, information, item
and travel spells (M76), map admission, combat inventory, audio.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`): all 23
  handlers match the pin in damage, type, range and cost. Kept DOS
  differences: learned Frostbite has no initial FX8, item Incinerate uses
  its own projectile, wrong-species Holy Word takes the miss path, and a
  negative player hit total hits (unsigned comparison). Evidence is in
  [magic](rules/magic.md#damage-preparation-and-resolution) and
  [original quirks](original-quirks.md).
- **All range**: up to four groups (twelve positions), stopped by walls,
  each target settled before the next. Variable damage and the Elemental
  Storm and Prismatic Light type draws happen once per cast, before
  targeting, even with no target; a cast with no target keeps its cost.
- **Unsigned hit total** (maintainer decision): fixed generically for every
  party attack, melee, Shoot and Poison Volley, not only for the spell.
- **Poison Volley split into Part C** (maintainer decision): combat had no
  party volley, so the Shoot volley progression became one shared rule used
  by exploration and combat, keeping both owners.
- **Save v14 kept**: nothing new is persisted.

## Results

- Moon Ray heals each member by 1-30 after its projectile; Finger of Death
  draws two distinct saves, and Finger and Mass Distortion set exactly 5 on
  a successful damage save; resource ID 89 refuses spell damage before
  waking.
- Poison Volley gives each active member one shot, without a missile weapon,
  using weapon accuracy; a miss keeps the shot for later targets.
- Census: 42 of 76 spells supported. Developer test save `monster-damage`
  (README) starts in Vertigo with all 23 spells.
- Known limit: Poison Volley assumes a full party of six shooters; the DOS
  loop follows the active party size, which matters once the party can
  shrink (Inn step).

## Validation

- Complete `build-rel` suite for each part, unsandboxed: 225/225 (Part A),
  226/226 (Part B), 226/226 (Part C); original-data digests unchanged.
- Maintainer play-test with `monster-damage`: single, group and All spells,
  random types, Moon Ray healing, casts without a target, special spells on
  compatible and incompatible species, Poison Volley in exploration and
  combat, Quick Fight, melee and save/load.
