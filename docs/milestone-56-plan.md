# Milestone 56 plan - Food and bank

**Tier A (reclassified from Tier B). Status: approved; implementation not started.**

Buy food and use the bank through Vertigo's original CD screens ([roadmap step 2](roadmap.md#near-term)).
Planning only; implementation requires the maintainer's start. Clean baseline: `main = origin/main = 47f910d2c229fe92207670ea7062c62be4976cd4`.

- [ ] Approve Tier A: CD E28 `(24,5)` calls `(75,76,0)`, then `DoTownEvent(3)` on line 1.
  The subroutine conditionally sets flag 231 and clears 232-255, 57 and 58; it must run normally.
  `XeenRegionalRules::xeenRegionalService`, `XeenEventInterpreter` preflight/service admission and
  `XeenEventFlow::prepareVertigoResult` require a line-0, single-instruction terminal service.
  Existing CallEvent/Return decoding is insufficient: the publication/continuation boundary must change.
  Do not bypass the prelude or special-case this cell. Retaining Tier B requires a separate Tier A prerequisite.
- [ ] Independent Tier A plan review before implementation; the checklist below remains a proposal.

## Tasks and decisions

- [ ] Generalize the existing Event-to-Service handoff for supported preludes: publish effects once before
  service use, retain terminal DoTownEvent semantics and safely refuse unsupported dependent effects.
  Reuse Service, Event and Flow owners and the Smith/Training/Temple pattern.
- [ ] Tavern: original `tvrn*.twn`, `tavern.icn`, resource/DAT text, member selection, Food (F),
  Yes/No and Escape, with matching mouse actions. Drink (D), Tip (T), Rumors (R) and Sign In (S)
  stay visible and say "not supported yet"; recruitment, audio and shopkeeper animation stay deferred.
- [ ] Food: show `tavern.bin` entry 75's offer; Yes fills the party-wide counter to `15 * active members`
  for a flat 10 carried gold. At/above the limit, original full-packs message, no charge or reduction;
  otherwise insufficient gold gives the original refusal. No/cancel changes nothing; member condition
  does not gate Food. Other reference town limits/prices may be generic, but only Vertigo is reachable.
- [ ] Bank: E28 `(26,17)`, DoTownEvent(0) directly opens `bnkr*.twn`/`bank.icn` and both balances.
  D/W chooses deposit/withdrawal, then O/E gold/gems (`bank2.icn`); Escape backs out one level.
- [ ] Amount: window 35, ten digits, width 77, digits only, Backspace/Delete, Enter and Escape.
  Zero/empty/cancel transfers nothing; exact source balance is allowed, excess refuses without partial
  transfer. Empty source uses `NO_X_IN_THE_Y`; excess uses `NOT_ENOUGH_X_IN_THE_Y`.
  Return to the currency submenu after an attempt. Preserve the original balance arithmetic.
- [ ] Reuse saved balances and `XeenServiceDay.cpp`: no per-transaction time; either visit costs
  1440 minutes on departure, even without a purchase. Reconcile transfers with the precomputed departure:
  scheduled interest uses final bank balances once, without losing transfers or rerolling merchant stock.
- [ ] Extend bounded DAT fields for original templates/refusals, retaining DOS wording/layout
  ([original quirks](original-quirks.md)). Save stays **v8, schema 9, content 14**: food, balances and
  Event flags already persist; dialogs/continuations are transient. No new persistent state.

## Evidence and acceptance

Clean reference: `6814ee9ba54582f5b5adcffab49efbbd8f589edd` ([provenance](dependencies.md)),
`locations.cpp` Bank/Food/BaseLocation, `dialogs_input.cpp`, `party.cpp`, `scripts.cpp::cmdDoTownEvent`.
Read-only CD Events confirm both entries. DAT: Bank `[0x52d9f,0x52e2b)`, currency/amount/refresh
`[0x52ac9,0x52b79)`, full-packs `[0x52a0e,0x52a34)`; call `0x1c702` confirms ten digits/width 77.
Bank hotkeys agree. Maintainer DOSBox CD (2026-10-10): fixed town price/limit, refill to 90 (five displayed days for six), repeat purchase unchanged, bank interest.

- [ ] Ten-digit overflow (DOS scans `%lu`, `0x1e26f`, format `0x5311f`; ScummVM uses signed `atoi`):
  maintainer DOSBox CD, 2026-10-10: depositing `4294967296` and `4294967306` with more than 10 gold
  both give the original "not enough" refusal; no wrap. Treat such amounts as excess. The maintainer
  also confirmed that entering the Tavern or the Bank costs a day.
- [ ] Rules tests: party sizes, price/full/insufficient/no/cancel; both currencies/directions,
  zero/exact/excess, input and arithmetic bounds including resolved overflow.
- [ ] Original mouse/keyboard parity, text/screens; generic prelude publication, refusal and exactly-once
  settlement; transfers before interest-triggering departure, preserving stock/RNG sequencing.
- [ ] Save exclusion: negative save checks (F9 and control panel) during the generalized prelude
  handoff, Tavern/Bank menus, Food Yes/No, amount entry and departure/settlement retries; saving
  becomes available again only after terminal settlement.
- [ ] Original-data end-to-end: New Game -> Tavern Food -> Rest -> Bank -> save/load; check full packs,
  return after Rest for a paid refill, transfer both currencies, restore exact food/balances/flags/time/RNG.
- [ ] Independent Tier A implementation review; affected/fast tests, full `build-rel` build and CTest
  via [the runner protocol](../tools/AGENTS.md), required Debug closure checks, maintainer DOSBox play-test.
  Record acceptance only after the maintainer accepts the implemented milestone.
