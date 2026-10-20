# Milestone 81 plan - Monster treasure

**Tier A. Status: Part 0 completed and accepted (2026-10-20); Part A not started**
Roadmap [step 11](roadmap.md#next-arc-finishing-the-games-systems).
Baseline: clean `main`, `b137201651bce60b0b4b8bcd2edd57150558d8c1`, equal to `origin/main`.

## Goal and scope

Every Clouds monster uses its original drop field and shared [M71 generator](archive/milestone-71-plan.md),
including levels 2-6/accessories; deliver items, gold and gems through original screens.
This covers ordinary resource-defined rewards in every kill path, including
exploration Shoot and group spells; it does not admit otherwise unsupported
monster abilities, Darkside gameplay or ending scenes. Lord Xeen's fixed Scepter
(weapon 90) and quest-item 8 increment wait for roadmap step 14 with the ending
(maintainer, 2026-10-20).
Planning does not authorize implementation. Independent plan/implementation reviews,
complete tests and maintainer play-test are required.

## Current state

- `XeenCombat::serviceConsequences`/`serviceCast` in [XeenCombat.cpp](../src/games/xeen/XeenCombat.cpp)
  and `XeenEncounterFlow::serviceShoot` in [XeenJourneyConsequences.cpp](../src/app/XeenJourneyConsequences.cpp)
  call `XeenMonsterDropCandidate` only for lethal hits on resource ID 6, Orc.
  Other species produce no ordinary monster items/currency; these gates also lack map-85 exclusion.
- [XeenMonsterTreasure.h/.cpp](../src/games/xeen/XeenMonsterTreasure.cpp): its constructor
  stages 10 gold per Orc; `service` rolls `R(1,100)<=10`, then `XeenItemCandidate(1)`.
  No monster gems are staged. Level 1 generates weapons 1-33, armor 1-7 or Misc
  containers 1-9/powers 1-15; it cannot generate accessories.
- Staging has ten weapons, armor and Misc slots, no accessories; validators admit
  only level-1 fields, twelve source identities and `pendingGold == 10*popcount(mask)`.
  Delivery has twelve records. These are implementation limits, not original limits.
- [XeenItemRewards](../src/games/xeen/XeenItemRewards.h) already handles all four
  categories. Monster delivery still uses armor/Misc booleans and publishes only
  weapons/armor/Misc inventories. `XeenEncounterFlow::beginMonsterReward`,
  `advanceMonsterReward`, `acknowledgeMonsterReward` and `XeenEventFlow::frameCopy`
  supply the existing phased screens; both currency fields must reach them.
- Party `monsterTreasure` owns purse/pending loot; `XeenSaveJourney::treasure`,
  `XeenSaveState` and `XeenSaveFormat` already save it.
  Quiet pending/dormant loot can survive a save; active reward presentation cannot.
  `XeenPartyInterfaceTests::originalMonsterDropReceipts` proves successful Orc melee/group-spell
  drops; the older rules' missing-witness note is stale.

## Original rules and evidence

Pin: `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, per [dependencies](dependencies.md).
Use `Combat::attack2`/`MONSTER_ITEM_RANGES`, `MonsterData::load`,
`Character::makeItem`, `Party::giveTreasure`/`arePacksFull`, `Interface` Run and
`SavesManager` in `engines/mm/xeen`. DOS overrides the pin where documented.
Original `DARK.CC/xeen.mon` inventory (90 records; full table: `build-rel/m81-clouds-monster-drops.csv`):

| Drop field | Original monster IDs (names/currency in scratch table) | Reference chance |
| --- | --- | --- |
| 0 | Other 65 records, including 89 Lord Xeen (0 gold/0 gems) | No roll |
| 1 | 5 Goblin, 6 Orc, 7 Mad Dwarf | 10% |
| 2 | 10, 17, 18, 74 | 20% |
| 3 | 24, 25, 29, 30, 31, 35, 38, 42, 75 | 50% |
| 4 | 43, 45, 50, 76, 80 | 100% |
| 5 | 82, 84, 85 | 100% |
| 6 | 87 Darzog (0 gold/50 gems) | 100% |

| Rule | Evidence and decision |
| --- | --- |
| Per-monster inputs | **DOS-confirmed, calibrated static:** 60-byte stride; gold LE16 at 42 (`0x2c2be`), gems at 44 (`0x2c2fc`), drop at 45 (`0x2c36c`). All 90 original records match the planning CSV; Orc is 10 gold/0 gems/field 1. Use the resource record. |
| Chance and level | **DOS-confirmed, calibrated static:** field 0 skips the draw; 1/2/3 have thresholds 10/20/50; 4/5/6 have 100 and still draw; Lookup at `0x4d650`, gate `0x2c373..0x2c39a`, same field passed as level at `0x2c3e2..0x2c3ec`. No Clouds record uses 7-21; the unbounded DOS lookup reads 0 for 7-17 and 5/4/5/5 for 18-21, so MMModern refuses such a record as malformed. [Boundary](rules/evidence.md#m81-monster-treasure). |
| Currency and Arena | **DOS-confirmed, calibrated static:** map 85 branches past ordinary currency/generation at `0x2c283..0x2c28a`, after XP. Otherwise fixed gold/gems accumulate before chance in two 32-bit fields (`0x2c2be..0x2c307`); purse credit is also 32-bit (`0x2486a..0x24884`). MMModern keeps these widths. |
| Categories and generation | **DOS-confirmed:** reuse `XeenItemCandidate` unchanged; all four categories from level 2, but ordinary level 6 cannot select Misc. Preserve material/effectiveness tables and Misc charges 3-10; see [generation rules](rules/items.md#generation-and-reward-delivery). |
| Staging | **DOS-confirmed, calibrated static:** four separate ten-slot arrays, first empty slot; all loop bounds checked at `0x2c44e/0x2c49d/0x2c4ed/0x2c53c`. Generate before capacity; a full category loses the new item with all draws consumed. Set hasItems only on insertion. Keep the DOS Misc branch, not the pin's accessory-copy bug. |
| Delivery | **DOS-confirmed:** category/slot order, no preferred monster recipient; unrestricted capacity fallback includes disabled/dead members. Category-only fullness replaces member zero's final slot, preserving frame. Global fullness discards remaining ordinary items but still credits currency. [Evidence](rules/evidence.md#treasure-delivery). |
| Presentation | **DOS-confirmed:** clear entry input; transfer before one-tick blank hold, sort before ten-tick found hold, then separate final acknowledgment credits currency. Keyboard/either mouse may interrupt a hold without acknowledging the final page. [Quirk](original-quirks.md#sv-treasure-waits). |
| Run, saving and deferred loot | **DOS-confirmed, calibrated static:** full-party Run clears currency/hasItems, retaining arrays (`0x2e928..0x2e949`). Normal Save checks combat and map restrictions, not loot (`0x1f1cb..0x1f1e6`, `0x1ebcf`); synchronous delivery cannot dispatch Save. The save excludes all pending loot. In-process Load clears currency/hasItems but retains the current live arrays as dormant items (`0xb819..0xb839`, `0xcfe3..0xcff7`); a fresh process cannot recover them. This differs from the pin's full reset. [Evidence and save consequence](rules/evidence.md#m81-monster-treasure). |

**DOS-confirmed RNG contract:** after a successful chance draw, category d100,
subcategory d100 (d80 at level 6), optional base-ID draw, enchantment d100,
then equipment selector d100/value, weapon effectiveness d20 and conditional d6;
Misc instead draws level-bounded power and `R(1,8)+2` charges. Fixed IDs skip
the base-ID draw; singleton random ranges still consume a draw. From the shared
generator: level-1 equipment costs 4 draws, Misc 6; levels 2-6 armor/accessories
cost 5 or 6, weapons 7 or 8, reachable Misc 6. Add one chance draw per nonzero field,
even at 100%; failure costs one, field zero none. Finish each target's death/drop
before the next; capacity loss never refunds RNG. Counts exclude attack/resistance/scene draws.

## Design and save decision

- Expose validated drop-level/gem accessors on `XeenMonsterRecord`; one generic candidate
  accepts the record and full identity in all lethal paths. World defeated/accounted
  state prevents duplicate rewards; combat retains damage/death order, party retains
  treasure, Flow retains display. No new owner, per-route gate or second generator.
- Use a category value throughout monster staging, receipts and inventory
  publication; four ten-slot arrays and up to forty delivery records, with pending
  gold and gems as 32-bit sums. Remove the twelve-Orc source/mask assumption and
  per-source provenance: the DOS arrays carry no contributor identity, and World
  defeated/accounted state already prevents duplicate rewards. Shared item
  validation covers higher materials/powers; drop fields above 6 refuse the record.
- **Layout changes: v16 -> v17; schema 9 and content 14 stay unchanged.**
  Pending loot leaves the save, as DOS writes none (Part 0): no staged arrays,
  pending gold/gems, readiness or contributor fields. The save keeps purse,
  inventory, defeated/accounted actors and Journey RNG. Restore never replays
  generation, rewards, time or RNG; recapture must be byte-exact. No legacy
  reader: v16/earlier return `XeenUnsupportedSave::Older`; protect newer/unknown files.
- **Pending loot is running-program state** (follow DOS, maintainer 2026-10-20;
  [quirk](original-quirks.md#sv-loot-load-process-state)). Save keeps its ordinary
  combat and map guards and no loot guard; an open receipt cannot reach Save, so
  there is no receipt cursor. Loading a game in the running program clears pending
  gold, gems and readiness but keeps the staged arrays as dormant items, which the
  next drop delivers with its own; a program start begins empty. New Game does the
same as Load: it clears currency and readiness and keeps the arrays.
  Like Newcastle Level 2's Temple prices (M79), this state lives with the party
  owner for the program's lifetime, outside the save.
- Keep original screens/names for accessories and Misc, automatic interruptible
  timed pages, initial full-pack warning and a separate final acknowledgment.
  Currency-only/mixed rewards credit once. The DOS weapon-ID-34 full-pack exception
  is separate from Lord Xeen's weapon 90 (step 14); ordinary generated weapons stop at ID 33.

## Parts and acceptance

Each part takes one fresh session; amend if scope changes. Start only after plan review/authorization.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | **Completed and accepted 2026-10-20.** Calibrated static analysis of `WORLD/XEEN.DAT`, no DOSBox play (maintainer decision): all 90 monster records, chance table, staging loops, currency widths, Arena, Run, Save, Load and New Game. Rules in [evidence](rules/evidence.md#m81-monster-treasure) and [quirks](original-quirks.md#sv-loot-load-process-state). |
| A - state and save | Generalize party treasure to four category arrays held as running-program state, category receipts and validation; remove the twelve-source mask and the pending-loot save fields; implement v17 encoder/decoder/rejection and the exact generic round-trip; implement Load, New Game and program start as decided above. Retain old production eligibility until B. Acceptance: four categories, higher item fields and more than twelve staged kills; the v17 file holds no pending loot; Load in the running program keeps the arrays and clears currency/readiness; New Game does the same; a fresh program starts empty; malformed records and old versions reject clearly; existing delivery remains coherent. |
| B - production and presentation | Generic record-driven drops/currency in melee, arrows, exploration and group spells; map-85 exclusion; all four category transfers/screens and page acknowledgments. Acceptance: original-data kills produce real levels 1-6, accessories and mixed currency; capacity loss preserves draws, Run and Load leave dormant items that the next drop delivers, repeated input/reload cannot duplicate rewards. Lord Xeen's death reward stays out (step 14). |
| C - validation and closure | Update census admission and regenerate [coverage](coverage.md) with `mmodern_census`; fix stale rules evidence labels/witness note. Audit M44 digests, complete scheduled independent implementation review and suite, then maintainer play-test across higher-level drops, accessories, full packs and save/load. Accept only after explicit maintainer acceptance; update status/roadmap/history and condense this plan at closure. |

## Tests and validation

- Extend `XeenConsequenceRulesTests`/generation tests with independent tapes: zero/no
  draw; threshold boundaries; levels 1-6/zero-chance fields; four categories;
  exact draw order/count and suspended draws;
  ten-slot overflow, >12 staged kills, 40-item delivery, mixed currency and
  refused drop fields above 6.
- Extend the existing Orc gameplay witness with a few original-data higher-level
  kills covering accessories, guaranteed drops, gems and multi-target order;
  table/unit tests cover all species/levels without a process witness per feature.
  Exercise Arrow/spell/melee, Arena suppression, recipient fallback and stale input;
  global/category-only full packs, including filling midway: preserve the transferred
  prefix, discard remaining items and credit currency once after final acknowledgment.
- Extend the generic `XeenSaveStateTests` round-trip and format rejection tests:
  a save taken with loot pending or dormant holds none of it and restores purse,
  inventory, accounting, time and RNG exactly. Add rules tests for the running
  program: Load and New Game keep dormant arrays and clear currency/readiness,
  and a fresh start is empty.
- All four pre-M44 baseline scenario **digests must change**: they hash raw saves,
  so v17 changes bytes even without gameplay changes. New non-Orc draws/rewards may
  change routes/items/currency. Preserve the baseline build before implementation;
  audit bytes/fields/traces with `tools/m44-digest-audit.ps1`.
  Explain every delta; obtain explicit maintainer approval before replacing hashes.
- Each production part: affected targets/tests, fast suite, then complete
  `build-rel` CTest via [tools/AGENTS.md](../tools/AGENTS.md); no completion with
  failures. Census checks cover guard/calibration, not probabilistic item identities.
  This planning change checks links and `git diff --check` only.

## Decisions and risks

1. **Special boss scope (decided 2026-10-20):** Lord Xeen's fixed Scepter and
   quest-item 8 increment, a death hook in the pin outside the ordinary drop
   field, move to roadmap step 14 with the ending.
2. **Save fidelity (decided 2026-10-20, settled by Part 0):** pending loot follows
   DOS: never saved, kept across Load in the running program. This is not a
   deviation. Any other unverifiable original rule still stops for the maintainer.
3. **Technical risks:** twelve-source assumptions affect restores, snapshots,
   receipts and tests. Items from one game reaching another after Load are
   intended, as in DOS; tests must not treat them as a leak, and restore guards
   must not reject them. Explain downstream RNG/combat/M44 changes; never freeze old loot.
