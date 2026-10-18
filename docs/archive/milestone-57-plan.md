# Milestone 57 plan - Generic Event effects and world state

**Tier A, in two parts. Status: completed and accepted.** Plan `614d859`;
Part A `94007d0`; Part B and two play-test fixes `d1f5459`.
The maintainer's Debug play-test against the DOSBox CD evidence was accepted.
Baseline: `454eea60fa1a49f5432784007c3e5e2e35f90374`.

## Goal and final scope

Complete Joe's proof and Gunther's reward through generic Event mechanics, then
deliver Event treasure through shared item rules and RNG ([roadmap](../roadmap.md)
step 3). Addresses identified evidence only, never runtime admission. Authority:
the installed World of Xeen CD resources and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with maintainer DOSBox CD evidence
of 2026-10-10.

## Part A - Joe, Gunther and durable world effects

- `MoveObj` moves and turns objects by original record identity with signed
  coordinates and the same-position flip. Changes are durable session
  overrides (commercial MOBs untouched); drawing, selection and interaction use
  one effective state, kept across travel and save/load.
- `SetChar` for the whole active party (retaining WhoWill's selection and
  cancellation), TakeOrGive modes 15 (award), 16 (XP), 20 (game flags), 34/35
  (currency) and 104 (quest flags), and GiveMulti's fixed currency rewards.
  Other forms still say "not supported yet".
- Joe's proof box and Gunther's reward run from the original scripts: quest
  flags, XP to every member, 4,000 gold, 50 gems, award 6 and the Mad Dwarf
  Clan hook, each granted once. Flag 9 closes Joe's shop and skips monster
  respawn through his own script; no separate removal mechanism.
- The well stays deferred to step 5: both branches refuse visibly before any
  head, HP, flag or RNG change.
- Awards are mutable for all 30 roster owners; **save v9** (schema 9, content 14)
  stores them and the object overrides. M44 digests were replaced after the
  maintainer-approved audit; traces unchanged.

## Part B - Shared treasure generation and delivery

- Merchant `makeItem` generation became common item generation used by
  merchants and Event mode 66, with the same tables, levels and conditional
  draw sequence on the retained Journey RNG.
- GiveMulti runs its ordered pairs: generated items (66), random gold (100) and
  fixed currency. Delivery shares the recipient, order, capacity, overflow and
  full-pack rules for weapons, armor, accessories and miscellaneous items.
- Original-data proofs: the crate family (searching opens the crate, repeats say
  "Nothing here.") and the trees, which pay 1-10 gold once per initialized cycle
  across revisit, Tavern entry, departure and reload.
- Candidate effects are staged and published once with XP, purse, awards, flags,
  object edits, inventory and RNG; cancellation changes nothing, and retries or
  stale input cannot reroll or regrant. Saving is available only when settled.
  Part B keeps save v9.
- Play-test fixes: a left side wall beside a distant front wall was wrongly
  culled; the proof text no longer lets the party panel overdraw its window.
- Quirks: the full-pack loss of staged treasure, unsigned XP/currency wrap and
  award slots are in [quirks](../original-quirks.md).

## Deferred

The well's effects, display cases and jail, the locked-chest GiveMulti path,
broader step-5 content, mines exploration, full Quests/Awards screens and the
original treasure UI.
