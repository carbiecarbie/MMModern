# Milestone 97 plan - DOS save export

**Tier A. Status: completed and accepted** by the maintainer on 2026-10-30
after the play-test. Plan `3a8b635`, Part A `85d5418`, Part B `73c1179` and
the closure commit. Follows roadmap
[step 2](roadmap.md#next-arc-release-quality-and-the-darkside-start), the DOS
save investigation, which finished on 2026-10-29 and recommended this
developer exporter. Save v22, Journey schema 9 and content 14 unchanged.

## Goal

A developer tool, `mmodern_dos_export`, writes a current MMModern Clouds save
(a `mmodern_test_save` preset or a game the maintainer saved) as an original
World of Xeen slot pair that DOS loads in DOSBox. It can place the party on
Clouds or Darkside, prepare a ready-to-run DOSBox folder, and compare two
slot pairs byte by byte. Its purpose is evidence: DOSBox and MMModern start
from the same state, and a DOS re-save shows exactly what DOS changed. The
[DOS save rules](rules/dos-saves.md) hold the format facts, the projection
and the refusals; the [development guide](development.md#dos-save-export)
gives the commands.

## Scope

- **Excluded:** importing DOS saves (the import RNG, two-name and Darkside
  history policies stay maintainer decisions); Darkside progress, which
  roadmap step 5 adds to the save and then to the export; any change to the
  game, its save format or its release package; the notes F1 (map 12's
  object 0 animates from sprite 85 in DOS), F2 (the pin skips Ali Baba's
  chest on Darkside) and F3 (ExchObj swaps only x and y).
- **Evidence:** the investigation's calibrated DOS static analysis and
  DOSBox runs, in `D:/Projetos/MModern/MModern-re/dos-save-20261028/` and
  `D:/Projetos/MModern/MModern-playtest/dos-save-20261028/`; the M97
  play-test folder is `D:/Projetos/MModern/MModern-playtest/m97/`.

## Key decisions

- **A projection, not a DOS history** (maintainer, 2026-10-29). Each DOS
  field with a v22 home gets the MMModern value; each field without one
  keeps its New Game value from the installation, such as the prior map
  (PTY 17), the seen bitmap, unmodeled counters and flags, unused party
  slots and the stale XEEN party replica. A state DOS cannot hold (an
  attribute above one byte, a quest counter above 255, award 73) is refused,
  not approximated.
- **A separate tool, built only on request.** Its code is under `tools/`.
  Production code gained only a DARK New Game reader in the bridge and DOS
  writers beside the existing CHR, PTY, DAT, MOB and EVT readers. The CC
  index reuses the pinned `BaseCCArchive::saveIndex`.
- **NAM.** Both NAMs carry the name, side 0 and pair marker 1, so DOS loads
  a native pair instead of converting a legacy save. The name limit is 15
  printable ASCII characters (maintainer, 2026-10-29); analysis did not prove
  the DOS prompt's own limit.
- **Placing** writes only NAM byte 29 and DARK PTY 10..13. A Clouds
  destination is restored through the existing owners; a Darkside one must
  be a main map with its DAT, MOB, EVT and load resources, and the tile
  holding the chosen cell. Auxiliary tiles are refused. Darkside arrives at
  its New Game state.
- **DOSBox folder.** `pristine/WORLD` holds the pair and New Game `XEEN.CUR`
  and `DARK.CUR`; `XEEN.CFG` only from `--cfg`. The DOSBox Staging profile
  mounts the CD installation and its two images; the launcher backs up a
  non-empty overlay before resetting it. Paths are guarded on resolved
  paths, and nothing is written to the installation or `cloud_saves`.
- **Compare.** `--diff` reports missing and added members, lengths and every
  differing offset, names members by ID and canonical name, and flags
  invalid pair markers; `--resave-check` accepts only the name and play
  time.

## Results

- **Part A** (`85d5418`): the exporter, the DARK reader, the writers, the
  refusals and the CLI. It reproduced the passing native pair of the
  investigation except three projected PTY bytes (8, 9 and 17).
- **Part B** (`73c1179`): `--place`, `--dosbox`, `--diff` and
  `--resave-check`, the `m97-open-chest` preset, the rules chapter and the
  quirks.
- **Reviews:** the plan review approved with nine changes, all applied. The
  coordinator's pass (not independent) led to canonical names in `--diff`
  without a DAT and wider CRLF tests. The independent implementation review
  found two issues: `--place` refused single-tile indoor maps, and the rules
  text misread the item-1 runs. Both were fixed, and its follow-up pass found
  nothing new.
- The complete suite passed 370/370 on the final state (run
  `20261030-002855-57c1fbfe`).

## Acceptance

The maintainer ran four DOSBox items and accepted the milestone:
1. An immediate Load and Save As passed `--resave-check`, and the projected
   PTY bytes 8, 9 and 17 survived. The first preset failed: DOS marked the
   party's cell as stepped on load, because a member has Cartographer. A
   state DOS had already loaded once, exported again, then passed.
2. The strong party and the opened chest appeared as expected.
3. A game the maintainer saved in MMModern loaded in DOS where it was saved.
4. The strong party placed on Darkside at Castleview `(14,4)` walked,
   talked and fought two goblins.
