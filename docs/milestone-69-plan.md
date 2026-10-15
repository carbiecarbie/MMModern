# Milestone 69 plan - Generic per-map state

**Tier A, in three audited steps. Status: completed and accepted.** Plan
`00e84bf` (amended `a5ad56b`); A1 `ea201f2`; A2 `417a1c0`; B `f6074c2`.
Independent plan review, implementation review and focused re-review passed;
the maintainer accepted the play-test on 2026-10-15.

## Goal and final scope

Step 3 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems):
keep the live state of any Clouds or Darkside map in one generic form, on the
original's lifetime rules, so the world Events of step 4 and the maps opened
in step 9 need no new save shape. Vertigo and F3 moved onto it with identical
play, apart from the original monster recovery of A2.

- **A1 - one owner.** `XeenMapState`, keyed by map identity, replaced the F3
  and Vertigo actor tables, removed objects, disabled Events, barriers, object
  overrides and explored tiles. Every reader goes through the camera map; the
  `mapId == 28` branches are gone. A fourth M44 scenario,
  `vertigo-f3-roundtrip`, travels Vertigo, F3 and back with resumable
  checkpoints. M44: zero differences on the other three.
- **A2 - monster recovery on map load.** A calibrated static check of
  `WORLD/XEEN.DAT` confirmed the reference: map load (`0x0f542`) clears the
  monster array and refills each monster's HP from its MON record
  (`0x0f5be..0x0f5c7`, `0x0fa99..0x0faa8`), the save keeps positions only
  (`0x266e3..0x2671c`), and loading a game reaches the same map load. A wound
  persists only while the party stays on the map.
- **B - save v13.** One per-map table (monsters without HP or wake, object,
  wall-item, cell and Event overrides, explored bits), with cells and explored
  bits keyed by physical DAT tile, explicit treasure source identities, and
  bounds of 512 maps and 107 monsters per map; Darkside identities are
  representable and refused at restore; v12 and older saves are rejected.
  Clouds map 85 (The Warzone) is never retained, as in the original.

## Decisions

1. **Follow the original's map load** for monster HP and wake state, after the
   DOS check (maintainer, 2026-10-15). Exact restore still forbids replaying
   Events, rewards, time or random draws.
2. **Deltas over the original resources** instead of whole-file copies, and
   the envelope alone names the format (v13; schema 9, content 14 retained).
3. **The fourth scenario wounds an F3 Orc,** not a Vertigo Slime: Vertigo's
   exit script respawns and heals every city slot while flag 9 is unset.
4. **Decision 9:** resumed comparisons need every live monster of the camera
   map at full HP and asleep; one shared assertion guards every resume
   checkpoint. M41 was retuned; M39 B/C and M37 A stay explicit tests of the
   healing rule (maintainer decision).
5. **Quirks recorded:** deltas, HED not written back, Vertigo's reset order.

## Results

- M44 audits with one cause per step: A1 zero differences; A2 the enumerated
  recovery differences (approved); B format only (approved). Digests replaced
  twice with the maintainer's approval; unchanged by the review fixes.
- Complete `build-rel` suite 215/215; 128 synthetic maps restore well under a
  second.

## Deferred

Step 4's Event opcodes; the Clouds map 15 and Darkside map 50 load and save
hooks (step 9).
