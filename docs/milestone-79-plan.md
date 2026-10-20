# Milestone 79 plan - Services of the other towns

**Tier B, except Part C (Tier A). Status: planned.** Step 9 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M78](milestone-78-plan.md). Baseline `093d517`.

## Goal

Every Clouds service outside Vertigo works as in the original, on the
existing Smith, Training, Temple, Tavern, Bank and Guild owners, chosen by
the camera map rather than by map 28. The original Events reach them only
through unconditional `DoTownEvent` records at line 0, facing All:

| Map | Services (action) |
| --- | --- |
| 29 Nightshadow, 31 Asp, 32 Winterkill | Guild (2) at `(12,4)`, `(2,1)`, `(7,1)` |
| 30 Rivercity | Ironworks `(24,9)`, Tavern `(24,16)`, Training `(12,12)`, Guild `(6,30)`, Bank `(25,3)`, Temple `(6,21)` |
| 49 Shangri-La | Temple `(15,3)`, Ironworks `(14,8)`, Tavern `(14,13)`, Guild `(0,7)` |
| 73 Newcastle Level 1 | Ironworks `(4,7)`, Training `(11,7)` |
| 74 Newcastle Level 2 | Temple `(4,4)`, Bank `(5,8)` |

Per place (DOS, Part 0): Training cap 10 at 28, 15 at 30, 20 elsewhere,
cost `10*L*L`; Ironworks shop 0-3 for maps 28/30/73/49, otherwise 0, with
map-independent prices; Guild rows and awards 0-4 for maps 28-32, every
class spell and award 5 at Shangri-La, Spell Info including learned spells;
Tavern Food 15 per member for 10 gold at 28, 60/50 at 30 (the pin has 50),
120/100 at 49, with offer texts 75-77. Temple surcharges and Tavern Tip/Rumor
groups were already per map. Map 74's Temple uses the last Temple prices of
the running program: zero at program start, replaced at each tabled Temple,
kept across New Game and Load, never saved. Bank interest and accounts are
global. Out of scope, each keeping its "not supported yet" refusal before
any effect: entrance scenes, the Arena and the Pyramid (`DoTownEvent` 6 and
8-12, including map 49 `(7,14)`), Sign In and Dismiss, audio.

**Save:** stays v16 (schema 9, content 14). Stock for all four shops is
already generated, regenerated and saved; Tavern tip cursors are per map.
Accepting depletion in shops 1-3 widens restore validation only. The map 74
prices are process state outside the save.
**M44:** the services digest is expected unchanged; any change stops the
work for the maintainer.

## Maintainer decisions (2026-10-20)

- Map 74's Temple reproduces DOS (option A): the prices live for the
  running program, outside the game session and the save. This new owner is
  Part C, Tier A, with independent plan and implementation reviews of that
  part only.
- Depletion in shops 1-3 widens restore validation only; the save stays v16
  without an independent review.
- Scope adds Shangri-La and Newcastle to roadmap step 9's four towns.

## Tasks

- [x] **Part 0 - DOS check.** Calibrated static analysis of
      `WORLD/XEEN.DAT` settled every question; see
      [service evidence](rules/evidence.md#other-town-services-2026-10-20).
- [ ] **Part A - rules by place.** Training cap, Ironworks shop selection
      and purchase offers for every shop, Guild rows and membership, Food
      offer, each in the existing rule owners. No per-town copies. Correct
      the Temple table offset cited in `XeenTempleHeal.cpp` (`0x1ce87`).
- [ ] **Part B - admission.** Remove the map-28 gates from the Event
      preflight and runtime, `xeenRegionalService`, `XeenEventPublication`,
      `beginSmith`, `beginTraining` and the service preflights, which bind
      the camera map's Events and text through the generic text admission.
- [ ] **Part C - Newcastle Level 2 Temple (Tier A).** A short design note
      in this plan, reviewed before code: the owner of the program-lifetime
      Temple prices, how the Temple Flow reads and replaces them, and why New
      Game, Load and restore leave them alone. Then implement it; map 74
      refuses until Part C lands.
- [ ] Developer test save `m79-playtest`, starting in Rivercity: members
      below each cap with the XP to train, partial spell books, guild
      awards for some members only, one cursed item, one broken item and
      injuries, 100,000 gold and 5,000 gems, Town Portal and Lloyd's Beacon
      for travel to 49, 73 and 74; README entry.
- [ ] Regenerate the census; update its calibration if needed.

## Acceptance

- Unit tests: each per-place rule, the fallbacks, Guild membership
  refusal, depleted stock in shops 1-3 restoring exactly.
- Original-data tests: Rivercity Training to the cap; a purchase in the
  Shangri-La Ironworks from its own shop; the Bank at 74; a
  Guild purchase in Nightshadow, Asp or Winterkill.
- Map 74's Temple: zero prices in a fresh program, the last tabled Temple's
  prices after a visit, kept across New Game and Load, absent from the save.
- Sign In, Dismiss and actions 6 and 8-12 still refuse without changing
  state, time or random draws.
- The census lists every action 0-5 site as supported. One complete suite
  per `tools/AGENTS.md`; M44 digests unchanged.
- Maintainer play-test with `m79-playtest`: each town and each service kind
  through the original screens, with mouse and keyboard.
