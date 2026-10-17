# Milestone 72 - Terrain and simple services

**Tier A. Status: implementation authorized by the maintainer (2026-10-17),
including the save-format and manual-Space scheduling amendments below;
acceptance pending.** Baseline main
`e21f398`. Step 5 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems).

## Goal

The party can rest on, and arrive on, every Clouds surface as the original
allows, and Drink, Tip, Rumors, Donation and Uncurse work in the Vertigo
Tavern and Temple. In the [coverage report](coverage.md) these are the four largest
Clouds gaps (Rest on surfaces 5, 10, 13 and 15, on 27 to 6 maps each), the
unclassified arrival rows, and five Services rows. Sign In remains explicitly
unsupported until the inn and roster-management milestone.

Out of scope: the entrance scenes (`DoTownEvent` actions 8 to 12, roadmap
step 10), new areas, audio, Darkside-only surfaces and
banks. Sign In and its original inn party manager are also deferred by the
maintainer's explicit scope decision (2026-10-17); merely storing its town
would omit its central behavior. The arrival rules are shared by both sides
and stay generic.

## Evidence

Calibrated static analysis of `WORLD/XEEN.DAT` (offsets cited, high
confidence) for every rule below, and pinned ScummVM `6814ee9b` where DOS was
not checked. Indoor and outdoor cells encode their surface slots differently;
the primary map resolves both through its surface-type table. The calibrated
Rest and shared arrival routines establish the meaning of each resolved type.

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
   The maintainer approved the missing manual-Space scheduling prerequisites
   on 2026-10-17: after the complete manual Event chain, charge 1 minute
   indoors or 10 outdoors, run the original monster opportunity, then arrival
   (`0x0d468..0x0d482`, shared call `0x0d411`). Event-free Space uses that
   same suffix; a blocked Bash does not (`0x0d547..0x0d54f`). Handled Space
   doors/grates clear the key (`0x0c911`) and skip that suffix, like terminal
   services (`0x1d7c8`); Bash pass-through still reaches movement arrival. Reuse the
   existing Event and regional action/opportunity continuations, retaining
   their presentation holds. Stage the completed suffix and validate any
   resulting fall before publishing the manual action, so a refusal leaks
   no Event effects, time, damage or RNG. Retries resume retained work without
   repeating Events or opportunities. Add no coordinator or area admission.
3. **Tavern.** Drink, Tip and Rumors as in the original, with original text
   and prices, on the existing Tavern dialog. Retain Sign In's unsupported
   notice; its inn party manager belongs to roadmap step 8.
4. **Temple.** Donation and Uncurse, using the per-temple surcharges from
   the Temple price fix (`xeenTempleSurcharges`). Uncurse removes the Cursed condition and cursed
   item-state flags in all four carried categories, preserving the items and
   their other bytes (`0x1afa2..0x1b01b`). Each successful Heal or Uncurse adds
   1,440 minutes to the visit's farewell accumulator (`0x1ae68`); the common
   exit adds another 1,440 (`0x1d78e..0x1d798`). Keep this cumulative DOS rule,
   including its word-width arithmetic, rather than the pin's one-time
   two-day upgrade. Donation adds no separate farewell charge.
5. Record oddities in [original quirks](original-quirks.md) and, where
   MMModern then differs from ScummVM, in
   [ScummVM differences](scummvm-differences.md); update the rules chapters.
   Regenerate the census.

## Save and RNG

The maintainer approved reclassifying M72 as Tier A on 2026-10-17 after the
pre-production investigation found that Tip changes the map's tip index,
which v13 does not retain. DOS increments and takes the remainder modulo 15
on Clouds (`WORLD/XEEN.DAT` `0x1b501..0x1b521`); the pinned reference instead
divides. Repeated visits and exact save/load must retain the DOS value.

- Increment `kJourneyVersion` from 13 to **14**. Retain schema 9 and content
  14. Reject v13 and earlier clearly; add no legacy reader.
- Add the mutable tip index to the existing per-map state and its saved
  projection. Its initial value comes from the original DAT. Preserve its
  map identity and original map-retention lifetime; do not store it in a
  quest flag, item or unrelated reserved field.
- Confirm every Donation write against DOS before production edits. Reuse
  the existing saved party effects wherever their meanings match. Dialog-local
  counters and latches remain transient; never replay donations on load.
- Keep `mmodern_save_dump`'s existing field paths and per-map view stable;
  expose new persisted values there. Extend exact current-format round-trip
  tests to the new state, including a second map.
- Independent reviews of this amended plan and the implementation are
  mandatory. Resolve plan findings before production edits. Any further
  expansion still requires the maintainer's scope decision.

Sign In's original transition sets maze 0 and mode 2
(`0x1b78f..0x1b794`), returns through gameplay (`0x0d617`), and reaches the inn
party manager (`0x0b819..0x0b839`, `0x394b7..0x395d5`). This is the evidence
for the maintainer-approved deferral; no character town/side fields are added
in M72 solely for that deferred option.

New random draws sit where the original makes them. Run the M44 audit
(Preserve before production edits, Compare after); explain every difference
and ask before replacing a digest. The preserved v13 build is the baseline;
the new build reads only v14. Each audit binary reads its own format. F3 has
surfaces from the census lists, so mainland routes may change.

## Acceptance

- Census: no unsupported Rest-surface row in Clouds; Drink, Tip, Rumors,
  Donation and Uncurse supported; Sign In explicitly deferred to the inn
  milestone; arrival rows classified, each supported or with a named reason.
- Unit tests per surface and service with worked original values; one
  original-data test per new kind of effect (a damaging surface, a fall, a
  Tavern and a Temple option); exact v14 round trip (the baseline v13 round
  trip is retained by M44 Preserve); no net growth in process
  tests.
- Arrival integration tests cover movement, turns/waits, successful ordinary
  casts, Shoot, Jump, completed Rest and original Event travel/fall paths.
  Cover both mainland and indoor manual Events, Event-free Space, and the
  order of charge, monster opportunity and final arrival at dawn or protection
  expiry. A terminal service retains its existing service farewell path;
  verify its original return path before applying a manual-Space suffix.
  Check original order, destination refusal without state leaks, no extra
  arrival for blocked/cancelled actions, and no arrival replay on restore.
- M44 gate, complete `build-rel` suite through the runner.
- Maintainer play-test: Drink, Tip and Rumors, and Donation and Uncurse in
  Vertigo, saving and loading after each service visit. Observe Rest and
  arrival on hazardous surfaces through the existing developer Event fixture;
  its save/load prohibition and map admission remain unchanged. Automated
  exact-v14 round trips prove persisted terrain/service state without
  replay. This validation correction was approved by the maintainer on
  2026-10-17 after independent plan review.
