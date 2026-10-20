# Milestone 81 plan - Monster treasure

**Tier A. Status: Part 0 completed and accepted (2026-10-20)**
Roadmap [step 11](roadmap.md#next-arc-finishing-the-games-systems).
Baseline: clean `main`, `b137201651bce60b0b4b8bcd2edd57150558d8c1`, equal to `origin/main`.

## Goal and scope

Every Clouds monster uses its original drop field and shared [M71 generator](archive/milestone-71-plan.md),
including levels 2-6/accessories; deliver items, gold and gems through original screens.
This covers ordinary resource-defined rewards in every kill path, including
exploration Shoot and group spells, and Event treasure, which shares the monster
staging arrays as in DOS. It does not admit otherwise unsupported monster
abilities, Darkside gameplay or ending scenes. Lord Xeen's fixed Scepter
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
  stages 10 gold per Orc and refuses overflow; `service` rolls `R(1,100)<=10`, then
  `XeenItemCandidate(1)`. No monster gems; level 1 cannot generate accessories.
- Staging has ten weapons, armor and Misc slots, no accessories; validators admit
  only level-1 fields, twelve source identities and `pendingGold == 10*popcount(mask)`.
  Delivery has twelve records. These are implementation limits, not original limits.
- [XeenItemRewards](../src/games/xeen/XeenItemRewards.h) already handles all four
  categories. Monster delivery still uses armor/Misc booleans and publishes only
  weapons/armor/Misc inventories. `XeenEncounterFlow::beginMonsterReward`,
  `advanceMonsterReward`, `acknowledgeMonsterReward` and `XeenEventFlow::frameCopy`
  supply the existing phased screens; both currency fields must reach them.
- Party `monsterTreasure` is one value holding both the purse and pending loot;
  Ironworks, Training, Temple, casting, combat and Event values read and write its
  purse. The party is created per gameplay session ([XeenGameplay.cpp](../src/app/XeenGameplay.cpp)):
  control-panel Load and title New replace it. `XeenSaveState` saves and restores
  pending loot and validates its contributors; `XeenRestoreGuard` compares it.
  Map transitions copy the whole value into the candidate and write back only the
  purse (`XeenWorld.cpp`). M79's `XeenTemplePriceState` runs from `main.cpp`
  through `Application` to the sessions and flows; test support headers and the
  `XeenTestSave`/coverage tools declare their own instances.
- Event rewards use a separate queue per Event run (`XeenEventInterpreter::pendingRewards`),
  not the monster staging.

## Original rules and evidence

DOS evidence is calibrated static analysis, high confidence, no DOSBox play
([evidence](rules/evidence.md#m81-monster-treasure)); the pin
(`6814ee9`, [dependencies](dependencies.md)) is context only. Inventory of the
90 original `DARK.CC/xeen.mon` records (table: `build-rel/m81-clouds-monster-drops.csv`):

| Drop field | Original monster IDs | Chance |
| --- | --- | --- |
| 0 | Other 65 records, including 89 Lord Xeen | No draw |
| 1 | 5 Goblin, 6 Orc, 7 Mad Dwarf | 10% |
| 2 | 10, 17, 18, 74 | 20% |
| 3 | 24, 25, 29, 30, 31, 35, 38, 42, 75 | 50% |
| 4 | 43, 45, 50, 76, 80 | 100%, still one draw |
| 5 | 82, 84, 85 | 100%, still one draw |
| 6 | 87 Darzog (0 gold/50 gems) | 100%, still one draw |

| Rule | Decision |
| --- | --- |
| Inputs and level | Gold LE16 at record byte 42, gems at 44, drop field at 45; the field gives both chance and makeItem level. No Clouds record uses 7-21 (DOS's unbounded lookup reads 0 or stray bytes, the pin 0); MMModern refuses such a record as malformed data. |
| Currency and Arena | Fixed gold/gems accumulate before the chance draw in 32-bit sums, credited to the 32-bit purse at final acknowledgment, both modulo 2^32 without saturation. Map 85 skips currency and every ordinary loot draw, after XP. |
| Generation | Reuse `XeenItemCandidate` unchanged: four categories from level 2, no Misc at level 6, Misc charges 3-10 ([rules](rules/items.md#generation-and-reward-delivery)). |
| Staging | Four ten-slot arrays, first empty slot; generation precedes the capacity test, so a full category loses the item with its draws spent. Readiness is set only on insertion. Keep the DOS Misc branch, not the pin's accessory copy. |
| Delivery and screens | Unchanged M61/M63 rules: category/slot order, unrestricted recipient fallback, full-pack loss with currency still credited, original holds and a separate final acknowledgment. |
| Run, Save, Load, New Game | Full-party Run clears pending currency and readiness, keeping the arrays. Save has its ordinary guards only and writes no pending loot; an open receipt cannot reach Save. Load and New Game in the running program clear currency and readiness and keep the arrays; a program start is empty. |
| Event processing | Every Event run (`0x26774`, the pin's `checkEvents`) stashes pending currency, readiness and the forty slots when any of them is set (the pin tests `gold & gems`), clears currency and readiness only, stages Event items into the same arrays, delivers after the Events (deferred while any of the 26 attack slots is occupied), then restores the stash over the arrays. Arrival Events after a teleport run inside the same stash ([trace](rules/evidence.md#event-processing-stash)). |

**RNG contract (DOS-confirmed):** after a successful chance draw, category d100,
subcategory d100 (d80 at level 6), optional base-ID draw, enchantment d100,
then equipment selector d100/value, weapon effectiveness d20 and conditional d6;
Misc instead draws level-bounded power and `R(1,8)+2` charges. Fixed IDs skip
the base-ID draw; singleton random ranges still consume a draw. Level-1 equipment
costs 4 draws, Misc 6; levels 2-6 armor/accessories 5 or 6, weapons 7 or 8, Misc 6.
One chance draw per nonzero field, even at 100%; field zero none. Finish each
target's death/drop before the next; capacity loss never refunds RNG.

## Design and save decision

- Expose validated drop-level/gem accessors on `XeenMonsterRecord`; one generic candidate
  accepts the record and full identity in all lethal paths. World defeated/accounted
  state prevents duplicate rewards (`XeenJourneyProgression.h` rejects accounted or
  absent actors); lethal publication keeps committing accounting and treasure together.
  Combat keeps damage/death order and Flow the display. No per-route gate or second generator.
- Use a category value throughout staging, receipts and inventory publication:
  four ten-slot arrays, up to forty delivery records, pending gold and gems as
  32-bit sums. Remove the twelve-Orc source/mask and per-source provenance: the
  DOS arrays carry no contributor identity. Remove the overflow refusal for
  staged sums and their purse credit; both wrap modulo 2^32.
- **One running-program state owner** (maintainer, 2026-10-20). Extend M79's
  `XeenTemplePriceState` into a single owner of the running program's state,
  holding the last Temple prices and the pending loot (arrays, pending gold/gems,
  readiness); M79 behavior is unchanged. Split `monsterTreasure`: the purse stays
  in the party and the save, the pending loot moves to this owner, borrowed by
  every session through the existing Temple wiring. A successful game entry
  (title New and Load, control-panel Load, `--new-game`, `--load-game`,
  `--journey-region`) clears currency and readiness and keeps the arrays; a failed
  or cancelled entry leaves them untouched. Each process starts empty, which covers
  the developer fixtures. Save and F9 omit the loot without changing it.
  `XeenRestoreGuard`/`XeenMutation` observe this owner instead of party fields, so
  stale prepared work still cannot publish; cross-save carryover is intended and
  never rejected ([quirk](original-quirks.md#sv-loot-load-process-state)).
- **Layout changes: v16 -> v17; schema 9 and content 14 stay unchanged.** The save
  holds no staged arrays, pending currency, readiness or contributor fields; it keeps
  purse, inventory, defeated/accounted actors, time and Journey RNG. Restore performs
  no grant or draw; recapture is byte-exact. No legacy reader: v16/earlier return
  `XeenUnsupportedSave::Older`; protect newer/unknown files. Current capture checks
  that reject ready loot stay as scheduling invariants (ready loot delivers before
  input), not a loot guard.
- **Event treasure shares the staging** (maintainer, 2026-10-20). Event item and
  currency rewards stage into the owner's arrays instead of a per-run queue, and
  each Event run follows the DOS stash, delivery and restore above, including the
  OR test and the 26-slot deferral. Dormant monster items therefore deliver with an
  Event reward. **The original's duplicate-or-lose result is kept on purpose**
  ([quirk](original-quirks.md#sv-event-loot-stash)): with stashed monster currency,
  an Event reward that delivers hands the monster items over again after the
  restore, and a deferred one loses the Event's treasure. Before implementing it,
  a static trace of the arrival refresh (`0x10bc3`, attack-slot fill `0x14406`)
  settles which branch an arrival Event takes. Load and New Game clearing is never
  reused inside Event processing.
- Dormant items deliver with a later currency/readiness trigger; a failed insertion
  with no currency does not reactivate them. Keep original screens/names for
  accessories and Misc, interruptible timed pages, the initial full-pack warning
  and a separate final acknowledgment; currency-only/mixed rewards credit once. The
  DOS weapon-ID-34 full-pack exception is separate from Lord Xeen's weapon 90.

## Parts and acceptance

Each part starts in a fresh session and may continue in a second one. No part
leaves a failing check for a later part.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | **Completed and accepted 2026-10-20.** Calibrated static analysis of `WORLD/XEEN.DAT`, no DOSBox play (maintainer decision): all 90 records, chance table, staging, currency, Arena, Run, Save, Load, New Game and Event processing. |
| A - state, owner and save | Using the pre-A build preserved in `D:/M44-audits/m81-partA-pre-30c548a`: extend the Temple owner, split the purse from pending loot, four category arrays and receipts, modulo-2^32 sums, guard observation, v17 encoder/decoder/rejection, and every compile-required call-site change, keeping Orc-only production and the current Event queue. Audit the four M44 digests and obtain maintainer approval of their replacement before the complete suite. Acceptance: the state, lifetime and save tests below; complete suite green. |
| B - Event treasure | Trace `0x10bc3` first (static, recorded in `rules/evidence.md`); then move Event rewards onto the shared staging with the DOS stash, delivery and restore, the kept duplicate-or-lose result and the arrival branch the trace settles. Acceptance: the Event tests below; M44 digests re-audited with maintainer approval; complete suite green. |
| C - production and presentation | Generic record-driven drops/currency in melee, arrows, exploration and group spells; map-85 exclusion; all four category transfers/screens and page acknowledgments. Acceptance: original-data kills produce levels 1-6, accessories and mixed currency; capacity loss preserves draws; dormant items deliver with a later trigger; restore grants and draws nothing. Re-audit M44 digests with maintainer approval. |
| D - validation and closure | Update census admission and regenerate [coverage](coverage.md) with `mmodern_census`; independent implementation review and complete suite; maintainer play-test of higher-level drops, accessories, full packs, Event rewards, Load and New Game carryover. Update status/roadmap/history and condense this plan only after acceptance. |

## Tests and validation

- Rules tests with independent tapes (`XeenConsequenceRulesTests`, generation):
  zero/no draw, threshold boundaries, levels 1-6, refused fields above 6, four
  categories, exact draw order/count and suspended draws, ten-slot overflow,
  more than twelve staged kills, 40-item delivery, mixed currency and modulo-2^32
  sums and purse credit at the boundary.
- Original-data gameplay: extend the Orc witness with a few higher-level kills
  covering accessories, guaranteed drops, gems and multi-target order; exercise
  Arrow/spell/melee, Arena suppression, recipient fallback and stale input, and
  full packs filling midway (transferred prefix kept, rest lost, currency credited once).
- Save (`XeenSaveStateTests`): states differing only in pending loot encode to equal
  bytes; purse, inventory, accounting, time and RNG restore exactly and bind under
  the guards afterward; old/newer versions reject clearly. Remove obsolete
  contributor checks without weakening other mutation protection.
- One application-level lifecycle test: stage items, Save, stage different items,
  Load; the later items survive, currency/readiness clear, nothing is granted or
  drawn. Also New Game, a failed Load (no change), a fresh start (empty) and the
  Temple prices still carried as in M79.
- Event treasure: no Event reward leaves pending and dormant loot unchanged; an
  Event reward with dormant monster items delivers both; with stashed monster
  currency, a delivering Event reward hands the monster items over twice and a
  deferred one loses the Event's treasure; an arrival Event after a teleport follows
  the `0x10bc3` trace; Event capacity counts monster items; no extra draws.
- M44 digests: all four change in A (they hash raw saves) and may change again in
  B and C; audit with `tools/m44-digest-audit.ps1`, explain every delta and replace
  hashes only with maintainer approval.
- Each part: affected targets/tests, fast suite, then the complete `build-rel`
  CTest via [tools/AGENTS.md](../tools/AGENTS.md); no completion with failures.

## Decisions and risks

1. **Special boss scope (decided 2026-10-20):** Lord Xeen's fixed Scepter and
   quest-item 8 increment, a death hook in the pin outside the ordinary drop
   field, move to roadmap step 14 with the ending.
2. **Save fidelity (decided 2026-10-20, settled by Part 0):** pending loot follows
   DOS: never saved, kept across Load and New Game in the running program. This
   is not a deviation. Any other unverifiable original rule stops for the maintainer.
3. **Event treasure (decided 2026-10-20):** Event rewards share the monster
   staging within M81, and the duplicate-or-lose result is an original bug kept on
   purpose, a QoL candidate for a later opt-in correction.
4. **Technical risks:** splitting the purse from pending loot touches every purse
   user, restores, snapshots, guards and receipts; extending the Temple owner
   touches its whole wiring and the test helpers that declare it. Replacing the
   Event reward queue touches Event publication and continuation. Carryover between
   games is intended; tests and guards must not treat it as a leak. Explain
   downstream RNG/combat/M44 changes; never freeze old loot.
