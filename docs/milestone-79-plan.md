# Milestone 79 plan - Services of the other towns

**Tier B, except Part C (Tier A). Status: completed and accepted by the
maintainer (2026-10-20)** after maintainer play-tests and an independent
plan and implementation review of Part C. Plan `2632944`, DOS check (Part
0) `30d0580`, rules and admission (Parts A and B) `d3c607e`, Part C design
`5a965ef` and implementation `ae87434`, cleanup `75a5829`. Step 9 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M78](milestone-78-plan.md).

## Scope

Every Clouds town service outside Vertigo, from the original Events: the
Guilds of Nightshadow (29), Asp (31) and Winterkill (32); all six services
of Rivercity (30); Shangri-La's (49) Temple, Ironworks, Tavern and Guild;
Newcastle Level 1's (73) Ironworks and Training and Level 2's (74) Temple
and Bank. Entrance scenes, the Arena and the Pyramid (`DoTownEvent` 6 and
8-12), Sign In, Dismiss and audio stayed out with their "not supported yet"
refusal before any effect.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`):
  Training caps 10 in Vertigo, 15 in Rivercity and 20 elsewhere; Ironworks
  stock shops 0-3 for maps 28, 30, 73 and 49 with map-independent prices;
  Guild catalogues and membership awards per town, and every class spell at
  Shangri-La; Rivercity Food fills 60 rations per member for 50 gold, where
  the reference has 50 ([difference](original-quirks.md#sv-rivercity-food)).
  The Temple price table is at `0x1ce87`. Evidence is in
  [service evidence](rules/evidence.md#other-town-services-2026-10-20).
- **One owner per rule, chosen by map**: the existing Training, Smith,
  Temple, Tavern, Bank and Guild owners select their values from the camera
  map; Event admission asks the rule owners whether a service is known
  there, without a map list. Service signatures take the map explicitly.
- **Newcastle Level 2's Temple** is outside the DOS price table and uses
  the last Temple prices of the running program: zero at start, replaced by
  each tabled Temple, kept across New Game and Load, never saved
  ([quirk](original-quirks.md#sv-temple-map74-carry); the reference resets
  them). The maintainer chose to reproduce it (Part C): one
  `XeenTemplePriceState` created in `main` and borrowed by each session; a
  visit prepares its row, every Temple read uses it until Event settlement,
  and admission publishes it atomically.
- **Save v16 unchanged** (schema 9, content 14): stock for all four shops
  was already saved; restore now accepts purchases from shops 1-3.

## Results

- The census lists every Clouds `DoTownEvent` 0-5 site as supported (22
  sites after Parts A and B, all 23 after Part C).
- M44 digests, traces and save bytes unchanged in every part. Complete
  suite 251/251.
- The Part C plan review's findings (the redraw during preparation, the
  owner outside the title loop, per-test owners, the stale-work rationale,
  the publication order) were applied before code; implementation found
  and closed the departure-to-settlement redraw window. Implementation
  review clean.
- The maintainer play-tested every town and service kind with the
  `m79-playtest` developer save, including Newcastle Level 2's Temple
  carry-over across New Game, Load and a restart.
