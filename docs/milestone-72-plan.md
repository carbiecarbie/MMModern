# Milestone 72 - Terrain and simple services

**Tier B. Status: approved by the maintainer (2026-10-17); implementation not
started.** Baseline main
`191263a`. Step 5 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems).

## Goal

The party can rest on, and arrive on, every Clouds surface as the original
allows, and the last "not supported yet" options in the Vertigo Tavern and
Temple work. In the [coverage report](coverage.md) these are the four largest
Clouds gaps (Rest on surfaces 5, 10, 13 and 15, on 27 to 6 maps each), the
unclassified arrival rows, and all six Services rows.

Out of scope: the entrance scenes (`DoTownEvent` actions 8 to 12, roadmap
step 10), new areas, audio, Darkside-only surfaces and
banks. The arrival rules are shared by both sides and stay generic.

## Evidence

Calibrated static analysis of `WORLD/XEEN.DAT` (offsets cited, high
confidence) for every rule below, and pinned ScummVM `6814ee9b` where DOS was
not checked. The rest-surface and arrival code may live in different routines
for indoor and outdoor maps; surface numbers mean different things in each,
so the implementer confirms the meaning per map kind before writing a rule.

Maintainer DOSBox session in the Vertigo Tavern and Temple, before or during
implementation (cheap, reachable): the text and price of Drink, Tip, Rumors
and Sign In, what each changes (gold, food, conditions, a saved location),
whether Rumors repeats or varies, and the Donation and Uncurse prices and
results for a party with a cursed item.

## Tasks

1. **Rest on every surface.** Replace the current refusal with the original
   rule for each surface (lava, sky, cloud, space and any other the census
   lists): where Rest is refused, with the original message; where it runs,
   what happens after it (step 6 of the Rest sequence in
   [time and conditions](rules/time-and-conditions.md)).
2. **Arrival effects.** After each step, Jump, teleport or Rest, apply the
   original effect of the destination surface: damage and its type, death,
   falling to another map through M70's shared transition result (a refused
   destination leaks no state), extra time (desert, unless Navigator), and
   any protection from spells or skills (Levitate, Walk on Water, Swimmer,
   Mountaineer and similar) only as far as the original checks them here.
   One owner for arrival, called from every place the original calls it.
3. **Tavern.** Drink, Tip, Rumors and Sign In as in the original, with
   original text and prices, on the existing Tavern dialog.
4. **Temple.** Donation and Uncurse, using the per-temple surcharges from
   the Temple price fix (`xeenTempleSurcharges`). Uncurse removes the Cursed condition and cursed
   equipped items as the original does.
5. Record oddities in [original quirks](original-quirks.md) and, where
   MMModern then differs from ScummVM, in
   [ScummVM differences](scummvm-differences.md); update the rules chapters.
   Regenerate the census.

## Save and RNG

No save change is expected. If Sign In or Donation needs a value the save
does not keep (a saved location, a donation counter, a blessing), stop and
amend the plan: that would make this milestone Tier A. New random draws sit
where the original makes them. Run the M44 audit (Preserve before production
edits, Compare after); explain any difference and ask before replacing a
digest. F3 has surfaces from the census lists, so mainland routes may change.

## Acceptance

- Census: no unsupported Rest-surface or Services row in Clouds; arrival rows
  classified, each supported or with a named reason.
- Unit tests per surface and service with worked original values; one
  original-data test per new kind of effect (a damaging surface, a fall, a
  Tavern and a Temple option); exact v13 round trip; no net growth in process
  tests.
- M44 gate, complete `build-rel` suite through the runner.
- Maintainer play-test: the four Tavern options and Donation and Uncurse in
  Vertigo; Rest and arrival on a hazardous surface through the developer
  fixture; save and load after each.
