# Milestone 102 plan - Darkside towns

**Tier B. Status: completed and accepted by the maintainer (2026-10-31)**
after the play-test. Plan `58ff690`, DOS check (Part 0) `b734d13`, town
services (Part A) `7e24b89`, entrance scenes (Part B) `c23dfe4`, and the
closure commit. Step 1 of the [Darkside arc](roadmap.md#darkside-arc).

## Scope

Every Darkside town service and entrance scene, on the existing service and
entrance-scene owners, with each side's values, pictures, voices, captions
and music:

| Service or scene (action) | Darkside maps |
| --- | --- |
| Bank (0) | 29 Castleview, 43 Castle Kalindra Level 3 |
| Smithy (1), Temple (4), Training (5) | 29, 31 Sandcaster, 37 Olympus, 43 |
| Guild (2) | 29, 31, 33 Lakeside, 35 Necropolis, 37 |
| Tavern (3), with Sign In (the Inn) | 29, 31, 37 |
| Tower scene (8) | 3, 13, 16, 23, 29 -> towers 57, 69, 61, 65, 53 with quest items 40-44 |
| Dungeon scene (9) | 12, 22 -> 73 Temple of Bark, 78 Lost Souls Dungeon with items 47, 48 |
| Dwarf town scene (13) | 4, 6, 19, 22, 98 -> towns 29, 35, 31, 33, 37 with keys 35-39 |

The Clouds entrances that need Darkside keys (maps 113-128), the Darkside
Events and the endings stayed out.

## Key decisions

- **DOS check first** (Part 0, calibrated static analysis of
  `WORLD/XEEN.DAT`; [evidence](rules/evidence.md#darkside-towns-m102-part-0)).
  The Smithy stock, Guild awards 83-87 and catalogues (Olympus sells every
  class spell), Food, Training caps 30/50/200/100, Inn doors, the World CD
  Inn song `newbrigh.m`, the resources from DARK.CC and the scene site
  tables agree with the reference. The DOS program differs from it in the
  Bank, Guild and Training animations, the night art of the Tower and Dwarf
  town scenes, the Tower's retreat and pace, the unskippable Dungeon pause
  and the Dungeon arrival facing East; each is recorded in
  [original quirks](original-quirks.md#darkside-town-rules-established-from-dos).
- **Night from the clock** (maintainer, 2026-10-31): DOS draws the sky and
  these scenes from a night flag cached at map load and full time advances.
  The scenes share the sky's current-clock rule instead
  ([approved deviation](original-quirks.md#dev-current-clock-night)), kept
  permanently by the maintainer on 2026-11-02.
- **Following the reference, unconfirmed in DOS:** the Darkside Inn omits
  the Clouds scroll, and a character created there lodges on Darkside.
- **Save v23 unchanged** (schema 9, content 14). Darkside shop stock was
  already saved; restore now accepts purchases from every shop on both sides.

## Results

- The census lists no Darkside `DoTownEvent` 0-5, 8, 9 or 13 site as
  refused. M44 digests unchanged in every part.
- Complete suite 377/378 after Part A, whose one failure was a save test that
  still required Darkside shops to be complete (test fixed, rerun passed),
  and 392/392 after Part B.
- The maintainer play-tested every service kind and every scene, refused
  and keyed, against DOSBox with the `m102` presets: all matched. The map
  song after a load or a reload can differ from DOS, as expected from the
  [Load music deviation](original-quirks.md#dev-darkside-load-music) and the
  clock-mixed DOS random draws.
