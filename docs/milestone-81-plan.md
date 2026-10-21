# Milestone 81 plan - Monster treasure

**Tier A. Status: completed and accepted by the maintainer (2026-10-21)**
after the maintainer's play-test. Plan `29f5b30` (amendments `abeb6b7`,
`30c548a`, `d07dfdf`, `51daaf4`), Part A `cf256e1` and `be7e9bf`, Part B
`22bc04a`, Part C `b206193` and `831ec0c`, play-test presets `83856b0`.
Roadmap [step 11](roadmap.md#next-arc-finishing-the-games-systems), after
[M80](milestone-80-plan.md).

## Scope

Every Clouds monster drops treasure from its original record on the shared
[M71 generator](archive/milestone-71-plan.md), in every kill path (melee,
arrows, exploration Shoot, single and group spells), at drop levels 1-6 with
accessories, and delivers items, gold and gems through the original screens.
Event treasure shares the same staging as in DOS. Lord Xeen's fixed Scepter
(weapon 90) and quest item 8 wait for roadmap step 15 with the ending
(maintainer, 2026-10-20). Darkside gameplay and otherwise unsupported monster
abilities stayed out.

| Drop field | Original monster IDs | Chance and level |
| --- | --- | --- |
| 0 | the other 65 records, including 89 Lord Xeen | no draw |
| 1 | 5 Goblin, 6 Orc, 7 Mad Dwarf | 10%, level 1 |
| 2 | 10, 17, 18, 74 | 20%, level 2 |
| 3 | 24, 25, 29, 30, 31, 35, 38, 42, 75 | 50%, level 3 |
| 4 / 5 / 6 | 43, 45, 50, 76, 80 / 82, 84, 85 / 87 Darzog | 100%, one draw at that level |

## Key decisions

- **DOS evidence first** (Part 0, accepted 2026-10-20): calibrated static
  analysis of the CD program, no DOSBox play
  ([evidence](rules/evidence.md#m81-monster-treasure)). Each record's gold
  and gems stage before the chance draw; currency is 32-bit, modulo 2^32.
  Four ten-slot category arrays take items at the first empty slot; a full
  category loses the item with its draws spent. The Warzone (map 85) stages
  nothing after XP. Drop fields above 6, unused by Clouds, refuse the record
  as "not supported yet" ([quirk](original-quirks.md#sv-unused-drop-fields)).
- **Pending loot is running-program state** (maintainer, 2026-10-20), as in
  DOS: `XeenProgramState`, created by `main`, owns the staging next to M79's
  Temple prices, each with its own revision. Save writes no pending loot;
  Load and New Game clear pending currency and readiness and keep the staged
  items, which a later trigger can deliver; a new process starts empty
  ([quirk](original-quirks.md#sv-loot-load-process-state)). The party keeps
  only the purse (`XeenPurse`).
- **Save v17** (v16 -> v17; schema 9 and content 14 unchanged): the layout
  drops the staged arrays, pending currency and readiness. Older saves are
  rejected. The four M44 digests were replaced once, in Part A, after an
  audit and with maintainer approval; Parts B and C left them unchanged.
- **Event treasure shares the staging** (maintainer, 2026-10-20). Each Event
  run stashes pending currency, readiness and items, stages its own rewards
  in the same arrays, delivers after the Events and restores the stash. With
  pending monster gold, the monster items are delivered twice and the gold
  once: an original bug kept on purpose, a QoL candidate
  ([quirk](original-quirks.md#sv-event-loot-stash)). The Event run checks the
  staging against its entry snapshot; an aborted run leaves it unchanged.
- **Order**: the Part B trace showed that DOS runs a cell's automatic Event
  before ordinary treasure delivery, so MMModern does too.
- **WhoWill** (Part C trace): the preferred recipient persists until the next
  Event run and every delivery, monster loot included, prefers that member.
  MMModern clears it on arrival at each new position over a flagged cell and
  on Space not taken by a grate; the per-pass and post-combat Event calls
  stay an [approved deviation](original-quirks.md#dev-whowill-event-passes)
  (maintainer, 2026-10-21) until roadmap step 12.

## Parts

| Part | Result |
| --- | --- |
| 0 - evidence | Drop table, staging, currency, Arena, Save/Load/New Game and the Event stash from calibrated static analysis. |
| A - state, owner and save | `XeenProgramState`, `XeenPurse`, the staging value with revision-checked prepared work, guard observation, save v17 (`cf256e1`, review fixes `be7e9bf`). |
| B - Event treasure | Event rewards on the shared staging and the single delivery, Events before delivery, snapshot integrity checks (`22bc04a`). |
| C - production and presentation | Record-driven drops and currency in every kill path, the Arena exclusion, refused fields above 6, WhoWill as in DOS (`b206193`, review fixes `831ec0c`). |
| D - validation and closure | Census, a cross-part independent review and closure. |

## Results

- Census: monster random treasure levels 1-6 are supported for every Clouds
  record that uses them ([coverage](coverage.md)).
- Rules and original-data tests cover higher-level kills with accessories, guaranteed
  drops, gems, multi-target order, every kill path, the Arena exclusion, full
  packs, the Event duplicate after a live kill and after a full-party Run, and
  pending loot across Save, Load and New Game.
- Each part had an independent implementation review with its fixes applied.
  Part D's cross-part review found no code defects; its closure fixes removed
  leftovers (fixed Orc notice, queue-era names, an unused parameter) and stale
  docs. Complete suite 261/261.
- The maintainer play-tested drops, accessories, full packs, Event treasure
  with the kept duplicate and a plain new game with the `m81-drops`,
  `m81-fullpacks` and `m81-event` presets (`83856b0`); the Arena, covered by
  an original-data test, was judged unnecessary to play.
