# Milestone 81 plan - Monster treasure

**Tier A. Status: planned (not started)**
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
| Per-monster inputs | **Reference only** interpretation, verified original-resource inventory above: 60-byte record, gold LE16 at 42, gems at 44, drop at 45. Orc reads 10 gold/0 gems/level 1. Use the record, never a species-specific runtime table; Part 0 calibrates these fields in DOS. |
| Chance and level | **Reference only** thresholds: field 0 skips the chance draw; 1/2/3 give 10/20/50%; 4/5/6 give 100%; 7-21 consume a chance draw but never generate. **DOS-confirmed** field `+0x2d` supplies both chance selection and makeItem level (`0x2c36c..0x2c3ec`); thresholds need Part 0 calibration. |
| Currency and Arena | **Reference only:** outside map 85 stage each killed monster's fixed gold/gems before item generation; inside 85 skip ordinary treasure and its draws. No random currency roll. Check DOS accumulator widths and overflow in Part 0. |
| Categories and generation | **DOS-confirmed:** reuse `XeenItemCandidate` unchanged; all four categories from level 2, but ordinary level 6 cannot select Misc. Preserve material/effectiveness tables and Misc charges 3-10; see [generation rules](rules/items.md#generation-and-reward-delivery). |
| Staging | **DOS-confirmed** category dispatch at `0x2c402/0x2c457/0x2c4a6/0x2c4f5`: separate arrays, first empty category slot; generate before testing capacity. **Reference only** complete four-by-ten bound pending Part 0 loop checks. Keep the DOS Misc branch, not the pin's accessory-copy bug. |
| Delivery | **DOS-confirmed:** category/slot order, no preferred monster recipient; unrestricted capacity fallback includes disabled/dead members. Category-only fullness replaces member zero's final slot, preserving frame. Global fullness discards remaining ordinary items but still credits currency. [Evidence](rules/evidence.md#treasure-delivery). |
| Presentation | **DOS-confirmed:** clear entry input; transfer before one-tick blank hold, sort before ten-tick found hold, then separate final acknowledgment credits currency. Keyboard/either mouse may interrupt a hold without acknowledging the final page. [Quirk](original-quirks.md#sv-treasure-waits). |
| Run, saving and deferred loot | **Reference only:** full-party Run clears currency/hasItems but leaves item arrays; reference load resets treasure. **Unknown:** DOS save availability and reload result with deferred/dormant loot or an open receipt; Part 0 settles them and MMModern follows DOS (maintainer, 2026-10-20). |

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
  publication; four ten-slot arrays and up to forty delivery records. Remove
  the twelve-Orc source/mask assumption. Canonical contributions keyed by full
  map/record identity retain owed currency and item provenance, bounded by retained
  map records rather than twelve kills. Shared item validation covers higher materials/powers.
- **Layout really changes: v16 -> v17; schema 9 and content 14 stay unchanged.**
  Save four category arrays, pending gems as well as gold, contribution identities
  and Part 0's readiness/dormancy state, with purse, items, defeated/accounted actors
  and Journey RNG. Remove mask/popcount validation; canonicalize identities and
  validate totals, extents and references. Per-source contribution identities go
  beyond the original's shape (four arrays, gold, gems): Part A keeps them only if
  defeated/accounted actor state cannot prevent duplicates or prune sources, and
  states why; otherwise the save holds the original shape.
  Update source pruning and beyond-MOB-count cleanup for actual contributions.
- Part 0 establishes legal save boundaries and DOS pending-loot persistence.
  **Follow DOS** (maintainer, 2026-10-20): if a DOS reload discards deferred or
  dormant loot, MMModern does not save it either; exact restoration covers what
  the save holds, not more than DOS keeps. Part 0's result may therefore remove
  pending loot from the v17 layout; amend this plan before Part A. Add no receipt cursor speculatively: if DOS permits
  saving during delivery, amend this plan for exact transferred-prefix/cursor state.
  Restore never replays generation, rewards, time or RNG; recapture must be byte-exact.
  No legacy reader: v16/earlier return `XeenUnsupportedSave::Older`; protect newer/unknown files.
- Keep original screens/names for accessories and Misc, automatic interruptible
  timed pages, initial full-pack warning and a separate final acknowledgment.
  Currency-only/mixed rewards credit once. The DOS weapon-ID-34 full-pack exception
  is separate from Lord Xeen's weapon 90 (step 14); ordinary generated weapons stop at ID 33.

## Parts and acceptance

Each part takes one fresh session; amend if scope changes. Start only after plan review/authorization.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | Read-only inventory of every Clouds monster's ID/name/gold/gems/drop field; validate against Orc and guaranteed-drop records. In DOSBox check a higher-level/accessory drop, mixed currency, full packs, Run then another fight, and saving/reloading deferred loot. Calibrate `WORLD/XEEN.DAT` analysis against the established Orc/generator and delivery offsets; trace chance table, all staging loops, accumulator widths, Arena exclusion and Run/save/load. Record confidence/file offsets and settled rules in `rules/`/`original-quirks.md`; add tagged differences where needed. Accept only with an explicit rule table and resolved blockers; tables/logs stay under build/scratch, original bytes/decompilation outside the repository. |
| A - state and save | Generalize party treasure, category receipts, canonical contributors and validation; implement v17 encoder/decoder/rejection and exact generic round-trip. Retain old production eligibility until B. Acceptance: four categories, >12 sources, higher item fields, pending gold/gems and dormant state round-trip; malformed references/counts/old versions reject clearly; existing delivery remains coherent. |
| B - production and presentation | Generic record-driven drops/currency in melee, arrows, exploration and group spells; map-85 exclusion; all four category transfers/screens and page acknowledgments. Acceptance: original-data kills produce real levels 1-6, accessories and mixed currency; capacity loss preserves draws, Run follows Part 0, repeated input/reload cannot duplicate rewards. Lord Xeen's death reward stays out (step 14). |
| C - validation and closure | Update census admission and regenerate [coverage](coverage.md) with `mmodern_census`; fix stale rules evidence labels/witness note. Audit M44 digests, complete scheduled independent implementation review and suite, then maintainer play-test across higher-level drops, accessories, full packs and save/load. Accept only after explicit maintainer acceptance; update status/roadmap/history and condense this plan at closure. |

## Tests and validation

- Extend `XeenConsequenceRulesTests`/generation tests with independent tapes: zero/no
  draw; threshold boundaries; levels 1-6/zero-chance fields; four categories;
  exact draw order/count and suspended draws;
  ten-slot overflow, >12 contributors, 40-item delivery and mixed currency.
- Extend the existing Orc gameplay witness with a few original-data higher-level
  kills covering accessories, guaranteed drops, gems and multi-target order;
  table/unit tests cover all species/levels without a process witness per feature.
  Exercise Arrow/spell/melee, Arena suppression, recipient fallback and stale input;
  global/category-only full packs, including filling midway: preserve the transferred
  prefix, discard remaining items and credit currency once after final acknowledgment.
- Extend the generic `XeenSaveStateTests` round-trip and format rejection tests;
  compare items, provenance, currency, accounting, time/RNG at legal pending/dormant boundaries.
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
2. **Save fidelity (decided 2026-10-20):** pending loot across save/load follows
   DOS, as Part 0 establishes it; this is not a deviation. Do not silently widen
   save eligibility. Any other unverifiable original rule still stops for the maintainer.
3. **Technical risks:** twelve-source assumptions affect restores, snapshots, receipts/tests;
   uint32 purse totals do not prove DOS accumulator width. Audit those boundaries and
   ghost-source cleanup. Explain downstream RNG/combat/M44 changes; never freeze old loot.
