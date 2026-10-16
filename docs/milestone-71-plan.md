# Milestone 71 - Original item generation

**Tier A. Status: approved after the independent plan review (maintainer,
2026-10-16); implementation not started.** Baseline main `e035c5c`. The makeItem correction
changes the game's random sequence and requires downstream stock and combat
witness repairs, beyond a small maintenance fix.

## Goal

Use the original makeItem rules once for all currently supported generation
sources, including level-1 monster drops. Repair affected tests while preserving
their behavioral and exact RNG-continuation guarantees.

Out of scope: higher-level monster drops, generic monster reward admission,
accessory treasure staging, other random rules, save-layout changes, new gameplay
and opening areas. Higher-level drops require a separate plan covering staging,
delivery, resource-driven chance and rewards, and the save-format decision.

## Evidence already settled

Static analysis of `WORLD/XEEN.DAT` (makeItem `0x25ca8..0x26382`, random helper
`0x45b9a..0x45bb4` inclusive min..max, calibrated on the Event gold draw
`0x23770..0x23791`), high confidence. Four differences from pinned ScummVM
`6814ee9b` are addressed by the existing uncommitted `makeitem-draws` change:

1. Category `R(1,100)`, subcategory `R(1,100)` or `R(1,80)` at levels 6-7;
   the pin draws from 0. Both rolls happen even with an Event ItemType override.
2. Level 7 consumes a selector `R(1,100)` before overriding the material row.
3. Weapon effectiveness tests `R(1,20)==10`; the pin uses `R(0,20)`.
4. Misc charges are `R(1,8)+2` (3..10); the pin stores `R(1,8)` (1..8).

Category thresholds, ID bands, enchantment comparisons and tables agree with
the existing generator. Charges are visible to players in stock and treasure.

## Design

### 1. Share generation within existing drop support

Shops (initial stock and restock), Event Give/GiveMulti mode 66, display cases
and crates already use `XeenItemCandidate`. Replace the duplicated level-1 item
generation in `XeenMonsterDropCandidate` with that shared candidate.

- First confirm in `XEEN.DAT` that the combat drop routine calls makeItem with
  the item-drop field as its level, and cite the offsets. The pin does so in
  `Combat::attack2`. If DOS contradicts this or the trace is inconclusive, stop.
- Keep existing supported callers and kill paths, level 1, the 10% chance draw,
  currency accounting, source identities and publication order. Do not admit
  other monsters or drop levels as part of this refactor.
- Keep generation before capacity checks, including all draws for lost items,
  and the separate Misc staging array (`0x2c4f5..0x2c535`). Preserve delivery,
  Run retention and retry behavior through the existing owners.
- Update pending-loot validation for Misc charges 1..10, retaining old 1..2
  records and admitting newly generated 9..10. Keep all other structural and
  source-accounting checks. Add no category, owner or persisted field.
- Exercise existing physical, spell and exploration kill integrations that use
  this candidate. Cover failed chance (no generation), successful generation,
  full-category loss with all draws consumed, and pending loot across save/load.

### 2. Repair tests without weakening their contracts

Classify failures using the retained run and the rerun protocol in
[tools/AGENTS.md](../tools/AGENTS.md); separate Windows error 5 from assertions.
Run required validation outside the Codex sandbox. Limit rewrites to affected
checks and directly duplicated coverage; do not launch a suite-wide cleanup.

- Select stock offers by the property needed for the transaction, asserting
  that an eligible offer exists. Compute expected prices/transfers independently;
  do not adopt the operation's own result as its expected value.
- Re-route broken combat witnesses through ordinary gameplay to retain their
  original service obligations. Do not inject state or search for a lucky seed.
- Keep fixed generation vectors and independent draw tapes. At integration
  boundaries derive expected stock and RNG continuation from the operation's
  starting cursor using `XeenM42Evidence.h`; counts/categories/bounds alone are
  insufficient. Preserve exact uninterrupted-versus-restored comparisons.
- Each rewritten test names its protected behavior and a mutation it catches.
  Every removed behavioral assertion, including one in a retired witness, must
  map to a surviving test and mutation. Preserve detection of extra draws,
  duplicate restock and incorrect RNG publication, as well as item/price errors.
- Replace existing cases rather than adding parallel witnesses or CTest entries.
  Fold missing boundary cases into existing tests; no net suite-target growth.

### 3. Retain v13 saves

Keep envelope v13 / schema 9 / content 14 and their layout and meaning. Merchant
stock and pending monster Misc validators accept charges 1..10, the union of
old and corrected generated values; generation itself produces only 3..10.
Retain the other category, power, container, frame and source constraints.
This does not widen party-inventory charge rules or add a legacy reader.

Old stock remains unchanged until normal restock; pending loot remains unchanged
until normal delivery. Restore performs no regeneration, reward replay, time
advance or RNG draw. Extend existing round-trip tests with stock and pending
Misc at 1, 2, 9 and 10 charges and invalid boundary values. Valid v0.2.0 saves
must load and re-save exactly. Any required layout or meaning change stops M71
for replanning; it must not be hidden behind the unchanged envelope.

### 4. M44 gate

Use the verified Preserve at `D:/M44-audits/makeitem-e035c5c`. Rebuild the audit
helpers and run the standard Compare from [tools/AGENTS.md](../tools/AGENTS.md)
after route repairs. All four scenarios must complete; an audit error or an
independently generated candidate is not a substitute. Exit 2 means differences
requiring review, not an audit failure.

For each scenario record the first differing draw and item, all deliberate
input/route changes, and explain every changed checkpoint/final field and digest
by the corrected rules or those route changes. Account for added/removed
checkpoints; the first divergence alone is insufficient. Retain paid Fix,
Training, Heal and resurrection wherever the original scenarios cover them,
with ordinary gameplay preconditions and independently checked consequences.

Require canonical decoding, native final reload/re-save equality, and Roundtrip
resumes from both A and B equal to the uninterrupted final. Then stop for separate
maintainer approval before replacing any `tests/XeenM44BaselineDigests.h` digest.
Plan approval does not approve replacement digests.

## Outside judges and risks

- DOS offsets establish the corrected rules and drop call. Update
  [original quirks](original-quirks.md), [items rules](rules/items.md) and the
  affected [combat treasure rules](rules/combat.md) in the implementation change;
  preserve evidence labels and explicitly retain higher-level support limits.
- Exhaust legal category/subcategory pairs at levels 1..7: category 1..100,
  subcategory 1..100 at levels 1..5 and 1..80 at levels 6..7. Test rejection of
  zero/out-of-range tape values separately from mapping these legal draws.
- The independent oracle must use corrected rules and independently transcribed
  tables, not production outputs/data. Extend its level-2..6 tables explicitly
  before using it at level 7; retain level-7 selector, effectiveness, charge and
  ItemType-override draw-order cases in existing generation tests.
- Route repair can hide regressions: retain each scenario's behavioral purpose
  and explain expectation changes. No repair may expand gameplay support.

## Acceptance

- All currently supported item-generation sources use the corrected shared
  generator; level-1 drops retain their chance, accounting, staging and delivery.
  Higher-level drops and accessory staging remain deferred.
- Generation boundaries and draw order pass independent oracles. Rewritten and
  retired assertions have surviving mutation coverage; exact integration RNG
  and restore checks remain, with no net CTest-target growth.
- v13/schema 9/content 14 are unchanged; valid v0.2.0 saves and old/new charge
  boundaries round-trip exactly without replay, regeneration or RNG movement.
- All four M44 comparisons and restart checks complete with explained differences;
  digest replacements have separate maintainer approval.
- Required affected/fast tests and the complete `build-rel` suite pass through
  the runner protocol, outside the sandbox; independent implementation review.
- Maintainer play-test: new-game shop stock, Misc charges in stock and treasure,
  a supported level-1 monster drop, and loading a v0.2.0 save. Record completion
  and acceptance only after the maintainer accepts the result.
