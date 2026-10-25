# Milestone 90 plan - Clouds verification sweep

**Tier B. Status: Parts A-C implemented; Part D postponed, not accepted.**
The maintainer postponed the gate-by-gate play-through (2026-10-25) to a
later play-through of the whole Clouds quest and authorized an independent
review in its place, approved after one round of test fixes. Roadmap
[step 16](roadmap.md#next-arc-finishing-the-games-systems), after
[M89](milestone-89-plan.md). Save v20, schema 9 and content 14 unchanged;
work stops if an M44 digest, the save format or an RNG draw would change.

## Goal

Prove that Clouds can be played from Newcastle's Megacredits to Lord Xeen
with what is already implemented, and fix what the sweep finds. Every quest
on the main line and the Megacredits chain uses supported Event operations,
but none has been played outside Vertigo. Evidence for the chain and the
gates: the completion audit (`D:/MModern-re/clouds-completion/findings.md`
§3.1-3.3 and §5). The step-15 rules already have their original-data tests
(`tests/XeenScriptedOriginalTests.cpp`) and presets (`m89-*`); reuse them.

## Tasks

### Part A - Gate presets

- [ ] Add `m90-*` presets to `tools/XeenTestSave.cpp` on the level-30
  `m78-playtest` party, each placed on or beside its original Event and
  carrying only the items, flags and gold the gate needs:
  - `m90-newcastle`: Taxman Ebenezer, map 12 `(11,12)`, 50,000 gold for the
    deed, 15 Megacredits (quest item 26) and award 13 (Rescued Crodo) so the
    permit is reachable; the whole chain is then played from there.
  - `m90-golem`: the map-20 Golem entrance with the Yak, Thousand Terrors
    and Golem stones (counters 7, 5, 6) and no Megacredits, to collect them
    in maps 79, 80 and 81.
  - `m90-illusion`: the Cave of Illusion skull, map 45 `(7,14)`, 300 gems.
  - `m90-high-magic`: the map-12 Reaper with quest item 3, through map 55
    to Darzog's key at 58 `(7,12)`.
  - `m90-darzog`: the map-15 Reaper with quest item 2, through 59 to Crodo
    at 61 `(7,6)`.
  - `m90-clouds`: Clouds of Xeen, map 27 `(15,2)`, with quest flag 23,
    award 13 and gold for Slick's four games.
  - `m90-castle`: beside the gate at 27 `(16,29)` with the Cupie Doll (quest
    item 28) and the Slayer Sword, through 75-77 to `m89-lord-xeen`'s spot.
  - `m90-winterkill`: Randon, map 32 `(8,13)`, before the quest.
- [ ] Document them in `docs/development.md` like the M89 presets.

### Part B - Original-data end-to-end tests

One new test file through the Event interpreter, as the M89 original tests
do (a few cases, no process witness):
- [ ] **Megacredits chain:** one Megacredit cell (WhoWill, quest item 26,
  MoveWallObj, Remove) and its persistence across save and load; the deed
  (gold, flag 45, quest item 0); the payment subroutine 64 `(101,112)` both
  with fewer than five credits ("not enough") and with five (five taken);
  walls (flag 43), the Taxman's quest item 5 and teleport to 72 `(7,15)`;
  keep (flag 44), quest item 6 and 73 `(7,15)`; Artemus's permit (award 13,
  quest flag 22, quest item 27); dungeon (flag 54); the `Laboratory`
  password to 71 `(2,14)`; the sword at 71 `(7,4)`.
- [ ] **Main line:** the skull's sale (300 gems, quest item 3); Crodo
  (award 13, 1,000,000 XP, quest flag 23); the Cupie Doll trade at 27
  `(12,29)` and the gate to 75 `(6,0)`; the Castle Xeen teleports to 78.
  Slick's games draw from the RNG: give the dolls directly unless the game
  itself can be driven deterministically.
- [ ] **Winterkill:** Randon's 126-line script to its reward, including the
  `IfMapFlag 255` path.

### Part C - Stale coverage rows and small checks

- [ ] `tools/XeenCoverageScreens.h`: SelectElement is supported (M76); drop
  "effects limited by Spells dimension"; recheck the Combat Run note against
  `XeenEncounterFlow` after M85; regenerate `docs/coverage.md` with
  `mmodern_census`.
- [ ] ItemsDialog "sheet Use refused in combat": settle the DOS behaviour by
  calibrated static analysis of `WORLD/XEEN.DAT` if it is cheap; if DOS
  allows it, report before implementing (scope check).
- [ ] Check `docs/rules/combat.md` "Lord Xeen and scripted death hooks" for
  the pin's wrong map-41 index and an outdated evidence label.
- [ ] Read-only DOS check that the Event entry never runs a fixed off-map
  position such as `(102,102)` (audit §2, Unknown); record the result.

### Part D - Maintainer play-through

- [ ] The maintainer plays each gate from its preset to the next, then the
  M89 presets to the Sixth Mirror. Doubtful behaviour is compared in DOSBox
  with the late test saves (slots 04/05, map 78) or the mirror words from a
  new game. Postponed to a whole-quest play-through, which should watch
  most what the tests restore past: earning the stones and credits in order
  (maps 80 and 81, reloading emptied credit cells, every Taxman return),
  both towers and Darzog's key, Slick's games and doll delivery, every
  Winterkill wave with the bell rung before and after, and Castle Xeen
  walked continuously to the Sixth Mirror's notice.

## Defects found

A small defect (local, no save format, RNG or Tier A change) is fixed in
scope with a test. A larger one is reported and proposed as its own
milestone; the sweep goes on around it. An original oddity kept on purpose
goes to `docs/original-quirks.md` in the same change.

## Acceptance

- The `m90-*` presets load and reach their gates; the new original-data
  tests pass; coverage rows match the code and the census is regenerated.
- Complete CTest suite green on `build-rel`; M44 digests unchanged.
- The maintainer's play-through reaches the Sixth Mirror's notice gate by
  gate, and every defect found is fixed or has its own proposed milestone.
