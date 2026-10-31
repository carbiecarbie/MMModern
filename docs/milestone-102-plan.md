# Milestone 102 plan - Darkside towns

**Tier B. Status: planned.** Step 1 of the [Darkside arc](roadmap.md#darkside-arc),
after arc step 0 (`5e4f685`). Baseline `5e4f685`.

## Goal

Every Darkside town service and entrance scene works as on the World of Xeen
CD, on the existing service and entrance-scene owners, with each side's
values, pictures, voices, captions and music. Today one gate,
`xeenTownServiceKnown`, admits town services on Clouds only, and the
entrance-scene site table has no Darkside rows. The original Events reach:

| Service (action) | Darkside maps |
| --- | --- |
| Bank (0) | 29 Castleview, 43 Castle Kalindra Level 3 |
| Smithy (1), Temple (4), Training (5) | 29, 31 Sandcaster, 37 Olympus, 43 |
| Guild (2) | 29, 31, 33 Lakeside, 35 Necropolis, 37 |
| Tavern (3), with Sign In (the Inn) | 29, 31, 37 |
| Tower scene (8) | 3, 13, 16, 23, 29 -> towers 57, 69, 61, 65, 53 with quest items 40-44 |
| Dungeon scene (9) | 12, 22 -> 73 Temple of Bark, 78 Lost Souls Dungeon with items 47, 48 |
| Dwarf town scene (13) | 4, 6, 19, 22, 98 -> towns 29, 35, 31, 33, 37 with keys 35-39 |

The reference values come from the research pass of 2026-10-31 (kept outside
the repository); they are leads for Part 0, not settled rules. Already
settled and kept: the Bank's accounts and interest, Smithy prices, Temple
prices for every Darkside temple, Tavern tips and rumors, and the location
return music ([audio](rules/audio.md): restore the previous song, then the
ordinary Darkside map reload draws once).

Out of scope, each keeping its refusal before any effect: the Clouds
entrances that need Darkside keys (maps 113-128, roadmap step 3), the
Darkside Events of step 2 and the endings.

**Save:** v23 unchanged expected. Darkside shop stock, quest items 35-48,
character awards and the side of each roster character are already saved.
Restore now accepts purchase depletion in Darkside shops, which widens
validation without changing layout or meaning, as M79 did for Clouds shops
1-3. If Part 0 or the work shows any new durable value, stop for a Tier A
decision. **M44:** digests expected unchanged; any change stops the work.

## Tasks

- [ ] **Part 0 - DOS check** (read-only static analysis of `WORLD/XEEN.DAT`,
      calibrated against the settled Clouds service and entrance-scene
      routines of M79/M80, cited offsets). For Darkside: Smithy shop by map;
      Guild catalogue, order and membership award by map (reference 83-87,
      Olympus every spell); Tavern Food amount, price and text by map;
      Training caps by map (reference 30/50/200/100); Inn doors for 29, 31
      and 37 and the Inn song on the World CD; greeting, action, refusal and
      farewell voices per service; animation frame counts, loop-backs,
      overlays and random frames (the reference looks broken for the Bank's
      frame -1 and the Guild after a refusal); archives for shop pictures,
      `tavern.bin`, `special.bin`, `spldesc.bin` and `box.vga`. For the
      scenes: the site table rows and whether the reference's dead rows
      exist; retreat on refusal per scene; pacing, pauses and whether a key
      skips the Dungeon wait; pictures and frames (resting `dung3` frame, day
      or night choice and when night is computed); mixer values and voices;
      caption records. Record results in `docs/rules/evidence.md` and the
      affected `docs/rules/` chapters, and each kept or rejected reference
      behavior in `docs/original-quirks.md` (and `scummvm-differences.md`).
      Ask the maintainer only for what program analysis cannot settle.
- [ ] **Part A - services.** One rule per value, chosen by side and map in
      the existing owners: Smithy shop index and Darkside purchase;
      Guild membership and catalogue; Food offer; Training cap; Inn doors,
      lodging characters with their side and the Inn song; side-aware
      archives, location music, animation tables and voice names (no
      hard-coded Clouds names in the flows). Remove the Clouds-only
      admission so every Darkside site above is admitted.
- [ ] **Part B - entrance scenes.** Darkside rows in the shared site table,
      keyed rows by quest item instead of the Clouds-only key rule, the
      Tower, Dungeon and Dwarf town scene steps with side-aware pictures,
      voices, captions and mixer values; admit action 13 in the decoder.
- [ ] Developer test save `m102-playtest` on Darkside (Castleview): members
      below each cap with the XP to train, partial spell books, guild awards
      for some members only, injuries, a cursed and a broken item, ample
      gold and gems, Town Portal, and the scene keys for some sites only;
      entry in `docs/development.md`.
- [ ] Regenerate the census.

## Acceptance

- Unit tests per rule and side: Smithy shop, Guild rows and membership,
  Food, Training cap, Inn door and lodging side, voices, animation tables,
  site rows with and without the key; Darkside shop depletion restores
  exactly.
- Original-data tests: a Smithy purchase and a Training visit in a Darkside
  town; Sign In at a Darkside Inn and the return; a Guild purchase at
  Olympus; one Tower, one Dungeon and one Dwarf town scene, refused and
  keyed, with a save round trip after arrival.
- Clouds services and scenes unchanged (M44 digests, existing tests).
- The census lists no Darkside `DoTownEvent` 0-5, 8, 9 or 13 site as
  refused. One complete suite per `tools/AGENTS.md`.
- One maintainer play-test after Part B with `m102-playtest`: each service
  kind and each scene through the original screens, plus only what Parts 0-B
  could not settle (for example audible pacing or an animation without a
  capture), each with its reason.
