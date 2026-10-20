# Milestone 79 plan - Services of the other towns

**Tier B. Status: planned.** Step 9 of the
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

Per place: Training cap (pin 28 -> 10, 30 -> 15, others 20; cost `10*L*L`
everywhere); Ironworks stock shop (pin maps 28/30/73/49 -> shops 0-3, others
0); Guild catalogue (pin rows for 28-32, all class spells at 49) and
membership award (map-28, Shangri-La award 5); Tavern Food (28: 15 per member
for 10 gold, 30: 50/50, 49: 120/100, others 15/10) and its offer text; Temple
surcharges and Tavern Tip/Rumor groups are already per map and DOS-settled.
Bank interest and accounts are global. Out of scope, each keeping its "not
supported yet" refusal before any effect: entrance scenes, the Arena and the
Pyramid (`DoTownEvent` 6 and 8-12, including map 49 `(7,14)`), Sign In and
Dismiss, audio.

**Save:** no new durable value is expected. Stock for all four shops is
already generated, regenerated and saved; Tavern tip cursors are per map.
The current-economy validator accepts depletion only in shop 0; widening it
to shops 1-3 keeps the layout and meaning, so the save stays v16 (schema 9,
content 14) unless the maintainer decides otherwise.
**M44:** the services digest is expected unchanged; any change stops the
work for the maintainer.

## Maintainer decisions (2026-10-20)

- Map 74's Temple reproduces the DOS carry-over that Part 0 establishes. If
  that needs saved state, Part 0 stops for a Tier A decision.
- Depletion in shops 1-3 widens restore validation only; the save stays v16
  without an independent review.
- Scope adds Shangri-La and Newcastle to roadmap step 9's four towns.

## Tasks

- [ ] **Part 0 - DOS check (read-only, delegated, calibrated against the
      settled Temple and Tavern routines, cited offsets).** (1) Training cap
      by map and cost; (2) Tavern Food amount, price and offer text by map;
      (3) Guild catalogue source and membership award by map, including 49;
      (4) Ironworks shop by map and its fallback; (5) for map 74, which DOS
      leaves outside the surcharge switch: the surcharges' value at program
      start, and whether New, Load or Save change or store them. Record
      results in `docs/rules/`
      and `docs/original-quirks.md`, correcting the Temple entry that calls
      every Clouds temple tabled. If (5) shows saved state, stop: that piece
      becomes a Tier A decision.
- [ ] **Part A - rules by place.** Training cap, Ironworks shop selection
      and purchase offers for every shop, Guild rows and membership, Food
      offer, and Temple pricing at map 74 per Part 0, each as one function
      of the map identity in the existing rule owners. No per-town copies.
- [ ] **Part B - admission.** Remove the map-28 gates from the Event
      preflight and runtime, `xeenRegionalService`, `XeenEventPublication`,
      `beginSmith`, `beginTraining` and the service preflights, which bind
      the camera map's Events and text through the generic text admission.
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
  Shangri-La Ironworks from its own shop; the Temple or Bank at 73/74; a
  Guild purchase in Nightshadow, Asp or Winterkill.
- Sign In, Dismiss and actions 6 and 8-12 still refuse without changing
  state, time or random draws.
- The census lists every action 0-5 site as supported. One complete suite
  per `tools/AGENTS.md`; M44 digests unchanged.
- Maintainer play-test with `m79-playtest`: each town and each service kind
  through the original screens, with mouse and keyboard.
