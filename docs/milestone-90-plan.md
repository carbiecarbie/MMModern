# Milestone 90 plan - Clouds verification sweep

**Tier B. Status: completed and accepted by the maintainer (2026-10-26)**
after the Part D play-through. Plan `6ae5701`, implementation `b85f34a`,
Part D presets `6b18041` and the closure commit. Roadmap
[step 16](roadmap.md#next-arc-finishing-the-games-systems), after
[M89](milestone-89-plan.md). Save format unchanged (v20 when implemented,
v21 at closure); M44 digests unchanged.

## Scope and results

Clouds can be played from Newcastle's Megacredits to Lord Xeen and the
Sixth Mirror with what was already implemented; the sweep found no content
gap.

- **Gate presets.** `m90-newcastle`, `m90-golem`, `m90-illusion`,
  `m90-high-magic`, `m90-darzog`, `m90-clouds`, `m90-castle` and
  `m90-winterkill` put the level-30 `m78-playtest` party at each gate with
  only the items, flags and gold it needs
  ([development](development.md#developer-test-saves)).
- **Part D spots.** Twenty-seven more `m90-*` presets stand on the original
  Event cell with its facing, so the Event runs as the save loads, and
  carry the story state reached there: every Emerson payment and Taxman
  return, Artemus, the Laboratory password and the Slayer Sword, Mirabeth,
  the Golems and one credit cell per dungeon, Darzog's key, Crodo, Slick's
  games and trade, the Guard Making Machine, and Winterkill's gong and
  Randon after each wave. Event lines the original's AlterEvent has already
  disabled are applied from the original records.
- **Original-data tests** (`tests/XeenCloudsProgressionTests.cpp`): the
  Megacredits chain with a credit cell's persistence, the deed, the payment
  subroutine with too few and enough credits, every Taxman stage, Artemus,
  the password and the sword; the skull's sale, Crodo, the Cupie Doll trade
  and gate and the Castle Xeen teleports; Randon's script to its reward.
- **Coverage.** SelectElement, Combat, Items and Spells rows corrected and
  the census regenerated.
- **DOS static checks.** The Event entry never dispatches a fixed off-map
  position by itself, and sheet Use stays refused in combat
  ([original quirks](original-quirks.md)).

## Key decisions

- The gate-by-gate play-through was postponed (2026-10-25); an independent
  review, authorized in its place, approved the implementation after one
  round of test fixes.
- To keep the whole-quest play-through short, the maintainer asked for the
  Part D spots (2026-10-26) instead of walking every dungeon.
- Slick's games draw from the RNG, so the tests give the dolls directly.

## Results

The maintainer played every Part D spot and the M89/M92 presets to the
Sixth Mirror and the ending, and accepted the milestone. Two outcomes could
not be won with the preset party and rest on the original-data tests:
Winterkill's waves (their spell-only monsters need gems the party lacks) and
Slick's games (the party's attributes are too low to win). No defect was
found.
