# Milestone 60 plan - Magic

**Tier A, in two parts. Status: completed and accepted.** Plan `a600364`;
Part A `e7e7a62`; Part B `6bc4779`. The maintainer accepted the Debug play-tests
of both parts (2026-10-12). Prerequisite: the condition-rule correction `b7a870a`.

## Goal and final scope

Replace the temporary casting screens with the original Cast dialog, buy and
inspect spells at Vertigo's Guild, and implement eight new effects
([roadmap](../roadmap.md) step 6). The maintainer chose option "(a+)": Wizard Eye,
Shrapmetal, Energy Blast and Jump plus Cure Wounds, Flying Fist, Revitalize and
Pain. Authority: the World of Xeen CD resources, original program analysis of
`WORLD/XEEN.DAT` (2026-10-11) and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Part A - Cast and Guild.** The original Cast dialog (C/N/Escape, caster
  switching by F1-F6 or portraits in exploration, fixed caster in combat) and the
  ten-row New list replace the project screens; `currentSpell` stays the single
  saved selection shared with Quick Fight. Guild Buy Spells and Spell Info offer
  Vertigo's original 18 spells, filtered by class book, to members only; price is
  100 x fixed SP cost or 500 x |level-scaled cost|, doubled for Paladin, Archer
  and Ranger, debited once with the learned slot. Spell costs, contexts, support
  and targets are centralized. Exploration casts cost 1 minute indoors and 10
  outdoors.
- **Part B - effects.** Shared projectile delivery, target order and
  impact/death/reward settlement; Magic Arrow now uses the same forward path in
  exploration. Shrapmetal (2 x level) and Pain (8) hit up to three monsters of
  the first reachable group; Energy Blast (2 x level energy) and Flying Fist (6)
  one target, with the original magic gate, saving throw and draw order. Cure
  Wounds heals 15 and Revitalize clears Weak, through First Aid's rules. Wizard
  Eye reveals the minimap without writing exploration and lasts until the
  temporary-effect reset. Jump moves two cells with the original wall checks and
  ordinary arrival.

## Decisions

1. **Option (a+)** (maintainer, 2026-10-11). Still purchasable but "not supported
   yet" when cast, with no charge, time, turn or draws: Light (darkness/drain),
   Sleep (monster status, save change), Insect Spray, Protection from Elements,
   Sparks, Toxic Cloud and Suppress Poison.
2. **Original program evidence** settled monster elemental resistance (no
   reduction of spell damage), Pain (8 physical, not zero), Suppress Poison
   (refund 4, subtract 3) and the Magic Arrow saving-throw draw (kept); see
   [original quirks](../original-quirks.md).
3. **Jump restriction** (maintainer, 2026-10-12): the original lets Jump land on
   space, lava, sky, cloud and desert with death, damage, falling or extra time.
   Those destinations, and cells outside the current walking-reachable save
   area, refuse before charge until terrain arrival is implemented; save
   validation is unchanged.
4. **Save v11 retained** (schema 9, content 14): Wizard Eye already lives in the
   saved party effects. All M44 digests unchanged in both parts.

## Results

- Rules, dialog, Guild, draw-tape, exactly-once, Quick Fight parity, Jump and
  Eye tests, plus original-data runs.
- Full CTest 183/183 (Part A) and 186/186 (Part B) on `build-rel`; Debug closure
  check passed; independent Tier A plan and implementation reviews passed.
- Maintainer Debug play-tests accepted, including Guild member-selection and
  Spell Info layout corrections in Part A.

## Deferred

Terrain arrival (space, lava, sky, cloud, desert) and Jump beyond the walking
area; the seven spells above; darkness and light drain; monster status effects.
