# Milestone 101 plan - Darkside start

**Tier A. Status: completed and accepted** by the maintainer on 2026-10-30
after the play-test. Plan `6631e31`, amended in `07f28c0` and `682c2fa`;
Part A `86821a6`, Part B `21cd4ec` and the closure commit. Starts roadmap
[step 5](roadmap.md#next-arc-release-quality-and-the-darkside-start).
Save v22 -> v23; Journey schema 9 and content 14 unchanged.

## Goal

A party walks into a Clouds Pyramid and arrives on Darkside, as in the World
of Xeen CD. It walks, talks and fights there with every mechanic MMModern
already has, saves and loads anywhere, and returns through a Darkside
Pyramid. Whatever Darkside uses that is not implemented yet says "not
supported yet" and keeps the game running. The DOS export carries Darkside
progress, so DOSBox and MMModern can start from the same Darkside state.

Out of scope: the Darkside mechanics from the
[census](coverage.md#darkside-largest-gaps), one milestone each; the
Darkside town services (Bank, Smithy, Guild, Tavern and Inn, Temple,
Training, Dwarf town), proposed as M102 with the music on the return from a
location; the Clouds entrances that need Darkside keys (the mixed maps
113-128), the Sphinx and FlipWorld; the Darkside and World endings and
completion; importing DOS saves.

## Key decisions (maintainer, 2026-10-30)

1. **Scope:** the two-sided foundation and the original Pyramid both ways,
   so Darkside is reached in ordinary play. Tier A, one plan, two parts and
   one play-test; the independent plan review's eleven changes were applied
   before Part A.
2. **State as in DOS:** game flags 2 x 256 and quest flags 2 x 30 selected
   by the map's side; the 85 quest counters and 32 character flags per roster
   record shared; 128 world flags; the yearly reset clears the Clouds flags
   on either side. No new owner, and the Journey profile stays the shared
   World of Xeen profile.
3. **Darkside music on Load:** DOS draws a song on every Darkside map load,
   Load included. The save keeps the selected song and Load replays it
   without a draw ([approved deviation](original-quirks.md#dev-darkside-load-music)).
4. **Object pictures** come with the generic Darkside resource choice, each
   newly admitted sprite rendered and inspected.
5. **No disc-change prompt:** a DOSBox capture showed the CD program's
   "Press CTRL+F4 and ENTER to continue..." at the Pyramid. MMModern reads
   both discs and shows no prompt
   ([approved deviation](original-quirks.md#dev-no-disc-prompt)).

## Results

- **Part A.** Both sides' state, Event values (mode 107 included) on both
  sides, the 113 ordinary Darkside maps loaded from DARK.CC with the DOS
  resources (Clouds monster art for species 91), the mixed maps refused, one
  `U(0,6)` music draw per Darkside map arrival from the DOS tables, and save
  v23 (1447 bytes more, envelope side equal to the camera side, v22
  refused). An audit-only v22 writer reproduced the four M44 hashes before
  the maintainer approved the v23 digests. Its independent review found a
  quest-journal out-of-bounds write and Event mode 107 still refused; both
  were fixed with the rest of its findings.
- **Part B.** The Pyramid uses the four DOS destination classes from all
  eight original sites, publishes once, settles its Event once and charges
  no time. Town Portal on Darkside shows the Darkside town names and goes
  to five [fixed outdoor arrivals](original-quirks.md#sv-darkside-town-portal-stride);
  a DOS recording confirmed Castleview -> Area A4 `(14,11)` South. Lloyd's
  Beacon keeps its side, World [Mr Wizard](original-quirks.md#dos-world-wizard-darkside)
  returns to Vertigo, and a [Darkside defeat](original-quirks.md#dos-darkside-defeat)
  plays Alamar's laugh and returns to the title. The DOS export writes both
  banks, the quest bits, the 85 counters, the roster flags and DARK map
  history; DOS loaded and re-saved the full witness with only name and play
  time changed.
- **Fixes from play and review.** Darkside monster voices come from the
  monster's map archive (the goblin fight at Castleview stopped the game);
  a Town Portal item used in combat on Darkside keeps the destination's
  music draw. A second independent review found it before commit.
- **Tests.** Live-input Pyramid crossings at every site with audio enabled,
  muted and absent; travel spells across sides; Mr Wizard and defeat; a
  save witness with nonzero values in every new field that re-saves
  byte-identically and exports to DOS; the original route from the
  `m101-pyramid-clouds` preset. Complete suite 375/375.
- **Play-test** (2026-10-30): Pyramid both ways, travel dialogs, Mr Wizard,
  defeat and the DOS witness; all passed after the goblin-voice fix.
