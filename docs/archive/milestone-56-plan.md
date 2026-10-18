# Milestone 56 plan - Food and bank

**Tier A. Status: completed and accepted.** Plan `61946c0`; implementation
`cf4c9a9`. The maintainer approved the Debug play-test and authorized closure.
Implementation baseline: `61946c0c6841d2e86b859fb1acfdf301b01e548b`.

## Goal and final scope

Buy food and use the Bank through Vertigo's original CD screens, by mouse and
keyboard. Reuse the Event, Service and Flow owners; no new persistent state.

- **Event handoff.** Supported preludes execute normally and publish their
  effects once before service use. Tavern E28 `(24,5)` calls `(75,76,0)` before
  `DoTownEvent(3)`: its flag changes are not bypassed or handled per cell.
  `DoTownEvent` terminates the entire Event, including called subroutines;
  unsupported dependent effects refuse safely without publishing a prefix.
- **Food.** Original Tavern art, buttons and `tavern.bin` entry 75 offer a
  refill to `15 * active members` for a flat 10 carried gold. Full or overfull
  packs cost nothing and are not reduced. Insufficient gold gives the original
  refusal; No/Escape changes nothing. Member condition does not gate Food.
- **Bank.** E28 `(26,17)`, `DoTownEvent(0)`, opens the original Bank screen.
  D/W chooses deposit/withdrawal, then O/E gold/gems. Window 35 accepts ten
  digits at width 77, Backspace/Delete, Enter and Escape. Zero, empty or
  cancelled input transfers nothing; exact source balances work and excess
  refuses without partial transfer. Escape backs out one level.
- **Time and economy.** Either visit costs 1440 minutes on departure, even
  without a transaction. Transactions add no time. Scheduled interest uses
  final bank balances once, after stock generation, preserving the prepared
  merchant stock and RNG continuation. Original unsigned balance arithmetic
  is retained; entered amounts above `UINT32_MAX` are refused as excess.
- **Saving.** Save remains **v8, schema 9, content 14**. Food, balances and
  Event flags already persist. Saving is unavailable during handoff, menus,
  Yes/No, amount entry and departure/settlement retries, and becomes available
  again only after terminal settlement.
- **Deferred.** Drink, Tip, Rumors and Sign In stay visible and say "not
  supported yet". Recruitment, audio and shopkeeper animation remain deferred.

## Evidence

Pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`
([provenance](../dependencies.md)): `locations.cpp` Bank/Food/BaseLocation,
`dialogs_input.cpp`, `party.cpp` and `scripts.cpp::cmdDoTownEvent`.
Read-only CD Events confirm both entrances. DAT fields: Bank
`[0x52d9f,0x52e2b)`, currency/amount/refresh `[0x52ac9,0x52b79)`, full packs
`[0x52a0e,0x52a34)`; call `0x1c702` confirms ten digits and width 77.

Maintainer DOSBox CD evidence (2026-10-10) confirmed the fixed food price and
limit, unchanged full packs, bank interest and one-day visits. Depositing
`4294967296` or `4294967306` with more than 10 gold gives the original excess
refusal without wrapping. DOS scans `%lu` (`0x1e26f`, format `0x5311f`), unlike
the reference's signed `atoi`. Retained quirks and DAT differences are recorded
in [original quirks](../original-quirks.md).

## Acceptance

- Independent Tier A plan and implementation reviews completed; findings fixed.
- Rules, original dialogs and mouse/keyboard parity, generic/called preludes,
  unsupported-effect refusal, transaction and settlement retries, stock/RNG
  ordering and final-balance interest tested. Application-level F9/control-panel
  checks cover all transient service states without reaching persistence.
- Original-data New Game -> Tavern -> Rest -> paid refill -> Bank -> save/load
  passes with exact food, balances, flags, time and RNG restoration.
- Full builds passed. `build-rel`: 143/143 fast and 177/177 complete tests;
  complete suite ran through the single-job `gpt-6-luna` low-effort runner
  ([protocol](../../tools/AGENTS.md)). `build-m44`: 143/143 fast and 3/3 M44 scenarios.
  All three M44 digests are unchanged on both builds.
- Maintainer Debug play-test passed and was accepted. No scope deviations.
