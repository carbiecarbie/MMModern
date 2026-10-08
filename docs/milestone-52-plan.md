# Milestone 52 plan - Normal start in Vertigo

**Tier A. Status: completed and accepted.** Plan `ff7b953`; Part A
`08178fe`; Part B `a830248`.

## Goal

A new Clouds of Xeen game starts from the original initialization and is
playable with the existing mechanics (Vertigo, the mainland, combat,
services, Rest, exact save/restore). The prepared Journey stays as the test
and developer mode used by the M44 scenarios.

## Final scope

- **Original start (Part A).** Party, roster and state come from the
  original data (`XEEN.CC` resources `2a0c..284c`): the six level-1
  characters Arturius, Tyro, Badger, Zippo, Rebecca and Seymour with 0 XP,
  at Vertigo `(18,4)` facing West, day 1 of year 610 at 08:00, 90 food,
  800 gold, 10 gems and an empty bank. Every byte of the 812-byte party
  record (including its truncated tail) and all 30 characters are checked
  against the data; merchant stock is generated once before publication.
  All 46 Vertigo actors start in their original state (the Slime at
  `(15,4)` is in view and activated), no mainland actor is activated, and
  an immediate F9 round-trips exactly.
- **Prepared Journey separation.** Its overrides (map 23 `(9,11)`, day 8,
  levels and XP, refilled HP/SP) apply only in that mode; the map-23
  assumptions of the start path were generalized in the existing owners.
- **Difficulty.** Both original difficulties: Adventurer adds +5 to hit
  and triples weapon damage for melee and Shoot (`Combat` reference
  `combat.cpp:1725`), Warrior adds neither. No other rule depends on it.
- **Entry (Part B).** `mmodern --new-game <game-dir>` and the bare
  `mmodern <game-dir>` start a new game (the static party screen is gone);
  `--difficulty adventurer|warrior`; `--save-file` sets the F9 target;
  `--combat-seed` stays with `--journey-region`.

## Decisions

1. **Temporary deviation (maintainer-approved).** The original title menu
   (New/Load/Credits) and difficulty dialog are replaced by the command
   line until the next milestone. The original dialog forces a choice; the
   command line defaults to Adventurer (the party record's byte 27 = 0)
   when `--difficulty` is omitted.
2. **Save v6 unchanged**: difficulty and the fresh state already fit it.
3. **M44 digests unchanged**: the prepared Journey stays Adventurer.
4. Map-load cosmetic draws keep the M48/M50 presentation-RNG policy, so
   merchant stock uses the first gameplay draws.

## Results

- Oracles over the original party record, characters, first-frame view and
  activation; Adventurer/Warrior melee, Shoot and unarmed attack tests; a
  new-game process test (walk, the East-facing start Event, combat, Rest, a
  purchase, a refusal, exit and re-entry, exact save continuation) in both
  difficulties.
- Full CTest 162/162 (Part A) and 163/163 (Part B) on the optimized build;
  closure run 163/163 on the Debug build.
- Independent implementation review: ACCEPT with no findings.
- Maintainer play-test from a new game in both difficulties passed.
