# Milestone 65 - Coverage census

**Tier B. Status: completed and accepted.** Plan `025079a`; implementation
`1248d83`. The maintainer accepted the report on 2026-10-14.

## Goal and final scope

Measure, from the original data, every mechanic World of Xeen uses and whether
MMModern supports it, so the next arc finishes the game's systems as a closed
list ordered by impact instead of discovering gaps map by map in play.

- `mmodern_census` reads every Clouds and Darkside map, monster, item and
  spell resource (read-only) and writes [the coverage report](../coverage.md):
  totals, the largest gaps first, every mechanic with its uses, maps and
  support status, and per-map readiness. Screens are the one manual checklist,
  taken from the pinned ScummVM dialog set.
- Support status comes from the engine's own checks, probed with a real New
  Game party, the loaded map and actors, the Journey RNG and the real
  Event-to-Service handoff. "Partial" means a genuine context dependency;
  behavior with no engine check stays "unclassified", never presumed supported.
- A guard test fails when the report is stale. A calibration test pins Vertigo
  to its known gaps (mirror, password doors, Tavern Drink/Tip/Rumors/Sign In,
  Temple Donation/Uncurse, audio-only partials and four unreferenced
  subroutines) and checks key F3 mechanics.

## Decisions

1. **Calibration against known play.** The first version probed with
   disposable owners and reported working Vertigo mechanics as partial; the
   review required a real context and the Vertigo gate before acceptance.
2. **Equipment in combat and Rest** is its own dimension: equipping uses the
   general equipment rules, while the old Journey validation, which refuses
   equipped material or enchanted items before combat, Shoot and Rest, is
   reported as a real gap.

## Results

- Clouds: 128 map resources and 15,919 Event instructions, 9,104 of them
  supported. Largest Clouds gaps: flying monsters (32 maps), Rest on surfaces
  10 and 15, `MoveWallObj`, `MakeNothingHere`, mirrors, password doors,
  `JumpRnd`, 65 of 76 spells and the equipment validation.
- Full CTest 195/195 on `build-rel`; M44 audit zero differences; the one
  production change shares the existing unsupported-service messages.

## Next

The test-suite cleanup (M66), then a systems arc ordered by the census.
