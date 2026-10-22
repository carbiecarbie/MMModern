# Milestone 76 plan - Buffs, information, item and travel spells

**Tier A. Status: completed and accepted by the maintainer (2026-10-18)**
after an independent plan review, an independent implementation review and
a maintainer play-test. Plan `1bd6d5f`, DOS check and plan amendment
`b5748e5`; implementation `14e8324` (A1), `2566231` (A2), `6cd1383` (B),
`7f8b1e2` (C1), `7c7d550` (C2); Recharge quirk correction `57ef01d`, review
fixes `01c57e5`. Last milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M75](milestone-75-plan.md): all 76 Clouds spells now work.

## Scope

The 22 remaining spells, from the spell book in exploration and combat,
Quick Fight and charged items: the party buffs (Bless, Heroism, Holy
Bonus, Power Shield, Protection from Elements, Day of Protection, Day of
Sorcery), senses and movement (Light, Levitate, Walk on Water,
Clairvoyance), information (Detect Monster, Identify Monster), item spells
(Enchant Item, Recharge Item, Item to Gold) and travel (Etherealize, Time
Distortion, Super Shelter, Teleport, Lloyd's Beacon, Town Portal), with
their consumers in combat, movement, terrain, Rest and the border.

Out of scope: the dark view of dark maps and light-drain cells (Opening
areas step; the open maps have neither), combat Equip/Use and Town Portal's
combat item path (step 7), map admission, audio.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`): all
  22 handlers and their consumers settled with high confidence before any
  code. DOS differences kept: Holy Bonus adds to weapon damage (the pin
  never applies it), ordinary Rest is refused on cells with attribute bit
  `0x40` (42 mainland cells; the pin's check never fires), Enchant uses the
  caster's level and copies the generator's result, Recharge fills a broken
  item to 63, Item to Gold's DOS price, and the ID-34 exceptions. Evidence
  is in [magic](rules/magic.md), [evidence](rules/evidence.md#m76-spells)
  and [original quirks](original-quirks.md).
- **Party effect bytes** follow the DOS layout (`DS:ed9e..eda6`, also the
  pin's order) through one named index. A follow-up DOS check during A2
  found that Levitate/Clairvoyance and Bless/Heroism had been swapped, and
  that A1 had moved Temple Donation's Clairvoyance; all were fixed with
  cross-owner tests.
- **Recharge bits**: Part 0 had swapped the cursed (`0x40`) and broken
  (`0x80`) names; a DOS check corrected it, and an edited-save DOS play
  test confirmed broken fills to 63 and cursed recharges normally.
- **Travel**: Etherealize, Time Distortion, Teleport and same-map Beacon
  Return relocate without a reload, so live monsters keep their state;
  cross-map Return and Town Portal load through the existing transition.
  Destinations in maps not open yet refuse before any cost. Item travel
  follows item Jump; the exact timing of destination Events after an item
  cast is not confirmed in DOS (maintainer decision).
- **Save v15** (schema 9, content 14): each character's Lloyd's Beacon is
  mutable roster state, saved and protected by the restore guard. The four
  M44 digests were replaced after a format-only audit (traces and gameplay
  fields identical), with the maintainer's authorization.
- **Darkness to the Opening areas step** (maintainer decision): on maps that
  are not dark Light is complete as a counter.

## Results

- Census: 76 of 76 spells supported. Developer test save `m76-spells`
  (README) holds casters for all 22 spells, charged items for their
  powers, test items for the item spells, distinct beacons and documented
  mainland spots for water, the Rest refusal and travel.
- The admitted mainland area has one definition (water open, independent
  of the party's abilities) shared by movement, save restore, Event
  transitions and spell destinations.

## Validation

- Complete `build-rel` suite for each part: 234/234 (A1), 236/236 (A2),
  238/238 (B), 240/240 (C1), 240/240 (C2, one stale test fixed and
  rerun), 240/240 (review fixes). M44 audit with zero differences for
  every part except C2's approved format change.
- Independent reviews: plan (approve with changes, applied);
  implementation (accept with fixes: water admission for same-map travel,
  item travel dialogs over the inventory, documentation; applied).
- Maintainer play-test with `m76-spells`: buffs and indicators, senses,
  information screens, item spells, travel, the Rest refusal, Beacon and
  Town Portal, and save/load.
