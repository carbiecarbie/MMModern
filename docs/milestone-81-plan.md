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
(weapon 90) and quest-item 8 increment wait for roadmap step 15 with the ending
(maintainer, 2026-10-20).

## Current state

- Only lethal hits on the Orc (resource 6) call `XeenMonsterDropCandidate`, from
  `XeenCombat::serviceConsequences`/`serviceCast` and `XeenEncounterFlow::serviceShoot`;
  no map-85 exclusion. [XeenMonsterTreasure](../src/games/xeen/XeenMonsterTreasure.cpp)
  stages 10 gold, refuses overflow, rolls 10% and generates at level 1, with ten
  weapon/armor/Misc slots, twelve source identities and twelve delivery records.
- Party `monsterTreasure` (optional, `XeenParty.h`) holds both the purse and the
  pending loot; every purse user (Ironworks, Training, Temple, casting, combat,
  Event values) goes through it. The party is created per session
  ([XeenGameplay.cpp](../src/app/XeenGameplay.cpp)); `XeenSaveState` saves pending
  loot and `XeenRestoreGuard` compares it; map transitions copy the whole value and
  publish only the purse (`XeenWorld.cpp`).
- Monster loot uses its own placeholder delivery (`XeenJourneyConsequences.cpp`);
  Event rewards use a per-run queue (`XeenEventInterpreter::pendingRewards`) with
  integrity checks in `XeenEventPublication.h`/`XeenEventContinuation.h`.
- M79's `XeenTemplePriceState` runs from `main.cpp` through `Application` to the
  sessions and flows, with one revision counter; test helpers declare their own.

## Original rules

DOS evidence is calibrated static analysis, high confidence, no DOSBox play
([evidence](rules/evidence.md#m81-monster-treasure)); the pin (`6814ee9`) is context only.

| Drop field | Original monster IDs (90 records) | Chance |
| --- | --- | --- |
| 0 | Other 65 records, including 89 Lord Xeen | No draw |
| 1 | 5 Goblin, 6 Orc, 7 Mad Dwarf | 10% |
| 2 | 10, 17, 18, 74 | 20% |
| 3 | 24, 25, 29, 30, 31, 35, 38, 42, 75 | 50% |
| 4 / 5 / 6 | 43, 45, 50, 76, 80 / 82, 84, 85 / 87 Darzog | 100%, still one draw |

| Rule | Decision |
| --- | --- |
| Inputs and level | Gold LE16 at record byte 42, gems at 44, drop field at 45, which gives chance and makeItem level. Fields 7-21 (unused by Clouds; Darkside's Doom Knight and Sandro use 7) refuse the record as not supported yet. |
| Draws | As in [monster drop draws](rules/items.md#monster-drop-draws) on the shared `XeenItemCandidate`. |
| Currency and Arena | Fixed gold/gems accumulate before the chance draw; staged sums and purse credit are 32-bit, modulo 2^32. Map 85 skips currency and every ordinary loot draw, after XP. |
| Staging | Four ten-slot arrays, first empty slot; a full category loses the item with its draws spent; readiness is set only on insertion. Keep the DOS Misc branch. |
| Delivery and screens | One delivery for monster and Event treasure, as DOS's `0x24128`: M61/M63 rules for order, recipients, full packs and screens. |
| Run, Save, Load, New Game | Run clears pending currency and readiness, keeping the arrays. Save writes no pending loot. Load and New Game in the running program clear currency and readiness and keep the arrays; a program start is empty. |
| Event processing | Each Event run stashes pending currency, readiness and slots when any is set, clears currency and readiness, stages Event items in the same arrays, delivers after the Events (never deferred: the Event loop runs in mode 9) and restores the stash. With stashed monster currency the monster items are delivered twice, an original bug kept on purpose ([quirk](original-quirks.md#sv-event-loot-stash)). |

## Design and save decision

- **Production.** `XeenMonsterRecord` exposes validated drop level and gems; one
  generic candidate serves every lethal path. World defeated/accounted state
  prevents duplicate rewards, and lethal publication keeps committing accounting
  and treasure together. No per-route gate or second generator.
- **Staging value.** Four ten-slot category arrays (able to hold Event quest
  items, raw IDs 82..116, in the weapons array), up to forty delivery records,
  pending gold and gems and readiness. No source mask, provenance or overflow refusal.
- **Purse.** The purse becomes `XeenPurse` (32-bit gold and gems) in the party,
  replacing `monsterTreasure` under its current installation rule; it stays saved.
- **Running-program owner** (maintainer, 2026-10-20). `XeenProgramState` replaces
  the Temple wiring and holds M79's `XeenTemplePriceState` unchanged, with its own
  revision, plus the staging value with a separate revision, so loot changes never
  invalidate a prepared Temple visit. The games layer (`XeenCombat`,
  `XeenEventInterpreter`, `XeenWorld`, `XeenSaveState`) receives the staging value,
  not the owner. A successful game entry (title New and Load, control-panel Load,
  `--new-game`, `--load-game`, `--journey-region`) clears currency and readiness
  and keeps the arrays; a failed or cancelled entry changes nothing; each process
  starts empty; Save and F9 omit the loot without changing it.
- **Prepared work.** Combat, Shoot, Run, Event runs, map transitions and delivery
  stage on a revision-checked copy of the staging value and publish it together
  with the party and world; abandoned work leaves the owner untouched. Map
  transitions keep the current retention. `XeenRestoreGuard`/`XeenMutation`
  observe the owner; cross-save carryover is intended and never rejected.
- **Save v17.** Layout changes (v16 -> v17; schema 9 and content 14 unchanged):
  no staged arrays, pending currency, readiness or contributors; purse, inventory,
  defeated/accounted actors, time and Journey RNG stay. Restore grants and draws
  nothing; recapture is byte-exact; v16/earlier are rejected as `Older`. Save stays
  refused during an Event run (`canSave`), so the stash never reaches a save.
  Capture checks that reject ready loot stay as scheduling invariants.
- **Event treasure.** Sources staging into the shared arrays: GiveEnchanted (Misc
  codes 70/71), Give mode 21 including quest items, mode 66, GiveMulti 34/35/66/100/101
  and the weapon-34 grant; readiness only on insertion, so the queue's overflow and
  invalid counters go. The Event run keeps a snapshot of the staging taken at
  entry; integrity checks (`validateRewardPrefix`, continuation equality, the
  mutation boundary) become "live staging = snapshot + dispatched effects, in
  first-empty-slot order". An aborted Event restores the snapshot and never clears
  monster loot.
- **Event and delivery order** (maintainer, 2026-10-20). If the Part B trace shows
  that the DOS main loop runs Events before ordinary delivery, MMModern follows it,
  replacing its current delivery of monster loot before the automatic Event.
- Dormant items deliver with a later currency/readiness trigger. Keep the original
  screens for accessories and Misc, interruptible pages, the initial full-pack
  warning and a separate final acknowledgment.

## Parts and acceptance

Each part starts in a fresh session and may continue in a second; none leaves a
failing check for a later part. Each M44 digest change (A for the format, B and C
for gameplay) is audited with `tools/m44-digest-audit.ps1` against the pre-A build
in `D:/M44-audits/m81-partA-pre-30c548a`, explained and replaced only with
maintainer approval, before that part's complete suite.

| Part | Work and acceptance |
| --- | --- |
| 0 - evidence | **Completed and accepted 2026-10-20.** Calibrated static analysis, no DOSBox play (maintainer decision). |
| A - state, owner and save | `XeenProgramState`, `XeenPurse`, the staging value and prepared-work publication, guard observation, v17, and every compile-required call-site change; production stays Orc-only and Events keep their queue. Acceptance: the state, lifetime, save and transition tests below. |
| B - Event treasure | First trace, statically, which mode writers an Event can reach before delivery and the DOS main-loop order of Events and ordinary delivery (`0xd0bd..0xd1b7`), recorded in `rules/evidence.md`; if a trace contradicts this plan, stop for the maintainer. Then move Event rewards onto the shared staging and the single delivery; `prepareVertigoPublication` and `prepareEventSequencePrefix` check the staging against the Event snapshot before publishing. Acceptance: the Event tests below. |
| C - production and presentation | Generic record-driven drops/currency in melee, arrows, exploration and group spells; map-85 exclusion; all four category screens. Acceptance: original-data kills produce levels 1-6, accessories and mixed currency; capacity loss preserves draws; drop fields above 6 refuse the record (with the drop-level accessor); a live kill while another monster holds an attack slot, then a treasure Event, delivers the monster items twice and the gold once, and the same after a full-party Run. Also: a static check of whether DOS resets the preferred recipient after Event processing (WhoWill before ordinary delivery), applied if it differs; tests drive the production `xeenBeginTreasureDelivery`/`xeenAdvanceTreasureDelivery` instead of the test-only `xeenDeliverTreasure`; drop `beginMonsterReward`'s unused catalog parameter. |
| D - validation and closure | Census admission and [coverage](coverage.md) regenerated with `mmodern_census`; independent implementation review; maintainer play-test of drops, accessories, full packs, Event rewards and Load/New Game carryover. Close only after acceptance. |

## Tests

- Rules tapes: zero/no draw, thresholds, levels 1-6, refused fields above 6, four
  categories, draw order/count, ten-slot overflow, more than twelve staged kills,
  40-item delivery, mixed currency, modulo-2^32 boundaries.
- Original-data gameplay: higher-level kills with accessories, guaranteed drops,
  gems and multi-target order; Arrow/spell/melee; Arena suppression; recipient
  fallback; stale input; full packs filling midway.
- Save: states differing only in pending loot encode to equal bytes; exact
  restore and guarded binding; old/newer versions rejected.
- One application-level lifecycle test: stage, Save, stage different items, Load:
  the later items survive, currency/readiness clear, nothing granted or drawn;
  New Game, failed Load, fresh start, and Temple prices still as in M79.
- Map transition with pending and dormant loot: retained, no extra draws;
  abandoned prepared work leaves the owner unchanged.
- Events: no Event reward leaves loot unchanged; an Event reward delivers dormant
  monster items with its own; Event capacity counts monster items; an aborted Event
  restores the snapshot. Original data: kill an Orc while another monster holds an
  attack slot, then step onto a treasure Event (items delivered twice, currency
  once, in the order Part B settles); the same with items left dormant by a Run.

## Decisions and risks

1. Lord Xeen's Scepter and quest item move to roadmap step 15 (2026-10-20).
2. Pending loot follows DOS: never saved, kept across Load and New Game (2026-10-20).
3. Event rewards share the staging; the duplicate is kept as an original bug,
   a QoL candidate (2026-10-20).
4. **Risks:** the purse split touches every purse user; `XeenProgramState`
   touches the Temple wiring and the test helpers; the Event snapshot model
   replaces existing integrity checks, which must keep rejecting genuine
   mismatches. Carryover between games is intended; never freeze old loot.
