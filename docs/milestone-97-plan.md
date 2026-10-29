# Milestone 97 plan - DOS save export

**Tier A. Status: planned (not started).**
Follows roadmap
[step 2](roadmap.md#next-arc-release-quality-and-the-darkside-start), the DOS
save investigation, which finished on 2026-10-29 and recommended this
developer exporter. Baseline: `400ba66`, equal to `origin/main`. Save format
v22, Journey schema 9 and content 14 stay unchanged.

## Goal and scope

A developer tool writes any current MMModern Clouds save (a preset from
`mmodern_test_save`, or a game the maintainer saved while playing) as an
original DOS save slot pair that World of Xeen loads in DOSBox. It can also
place the party at a chosen place, on Clouds or on Darkside, so that DOS
evidence for Darkside can be collected before MMModern plays Darkside. It
prepares a ready-to-run DOSBox folder, and compares two DOS slot pairs byte
by byte. Its purpose is evidence: DOSBox and MMModern start from the same
state, and a DOS re-save after an action shows exactly what DOS changed.

**The export is a projection of the MMModern state onto the DOS save, not
a copy of a DOS history** (maintainer, 2026-10-29). Each DOS field with a v22
home gets the MMModern value. Each DOS field without one keeps its New Game
value from the installation, for example:
- the prior map (PTY 17);
- the seen bitmap (DAT 828..859);
- the counters and flags v22 does not model;
- the unused roster slots;
- PTY 16;
- the XEEN replica of the party.

The rules chapter lists every such field, and the tool's summary names those
that apply. Anything the tool cannot write faithfully is refused with a clear
message, not approximated.

Out of scope:
- importing DOS saves into MMModern (options C and D of the investigation):
  the import RNG policy, the two-name policy and the unsupported Darkside
  history stay maintainer decisions for later;
- Darkside progress. MMModern saves cannot hold Darkside state yet, so an
  export carries no Darkside flags, quest items or map changes, and a party
  placed on Darkside arrives with Darkside at its New Game state. Roadmap
  step 5 adds Darkside to the save; exporting that progress is one of its
  first tasks, and its plan sizes the work;
- any change to the game, its save format or its release package: the tool
  is built only on request, like `mmodern_test_save`;
- the later notes F1 (map 12's object 0 animates from sprite 85 in DOS), F2
  (the pin skips Ali Baba's chest on Darkside) and F3 (ExchObj swaps only x
  and y).

## Evidence

The investigation's results are outside the repository:
- `D:/Projetos/MModern/MModern-re/dos-save-20261028/`: `findings.md` (its
  "Resume 2026-10-29 (v22)" and check A sections), `ledger.md` with one row
  per DOS byte, `harness-v22.patch` and `scripts/`;
- the DOSBox package `D:/Projetos/MModern/MModern-playtest/dos-save-20261028/`.

They are evidence and a reference implementation, not code to copy: the
scripts are throwaway Python. Settled facts this plan relies on:
- **Container.** WOX slot files and CUR files are CC archives.
  - The index encoding is calibrated at `0x45d00..0x45d91`, and the
    inherited codec re-encodes all 24 real files byte for byte.
  - Member names hash with aliases (`mazex004.dat` and `mazex100.dat`
    collide), so a map member's name comes from the map ID embedded in its
    DAT, never from a hash guess.
  - A missing member makes DOS fail fatally (`0x45fa8 -> 0x45c8a`).
- **Pair and side.** A slot is a pair `XEENnn.WOX` + `DARKnn.WOX`, slots
  1..10. The live party (NAM, CHR, PTY 1530 bytes) is always read from
  DARK. The XEEN CHR and PTY (812 bytes) are a stale replica that a native
  pair keeps at its New Game value. NAM byte 29 selects the side.
- **Pair marker.** NAM byte 30 binds the pair.
  - Zero means a legacy save: at startup DOS converts it (`0xd6f7`),
    rebuilding the party from the Clouds replica and generating new shop
    wares.
  - A native pair holds the same nonzero value 1..254 in both NAMs (DOS
    draws it with `R(1,254)`).
  - 255 marks a missing slot.
  - Calibrated against two failed and one passing DOSBox run.
- **Fields.** `ledger.md` gives each PTY, CHR, NAM and map byte its v22
  home: flag banks in DOS MSB order, explored bytes bit-reversed, physical
  DAT tiles 1..85 and 100..142, object position and picture-table index
  (`0x26698..0x266ce`), monster and wall-item positions, Event opcodes.
- **Acceptance.** The final native check A passed: DOS loaded an exported
  earned Clouds state, and its immediate Save As changed 9 of 666,345 bytes
  (8 name bytes and 1 play-time byte). The real DOS re-save also imports
  into v22 and writes back exactly, except the NAM padding after the name's
  NUL, which the name abstraction zeroes.
- **Changing side, Reference only.** In the pin, the Pyramid sets the side,
  map, position and facing (`engines/mm/xeen/locations.cpp:2586`), FlipWorld
  sets only the side (`scripts.cpp:1492`), and the load rebuilds tables and
  music. No universal first-arrival flag was found. No DOS run has shown a
  Darkside arrival yet; the play-test does.

## Current state

At `400ba66`:
- MMModern reads the original CC archives through the linked ScummVM code
  (`src/compat/scummvm/ScummVmXeenBridge.cpp`).
- `XeenAssetSource::readInitialResource` gives the New Game members of
  **Clouds only**. The bridge rebuilds them from the six original blocks in
  `SaveArchive::reset` order (`ScummVmXeenBridge.cpp:252`); nothing reads
  DARK's New Game members.
- The pinned ScummVM already writes a CC index
  (`BaseCCArchive::saveIndex`, `engines/mm/shared/xeen/cc_archive.cpp:90`).
  `SaveArchive::save` adds ScummVM's own framing and does not fit. MMModern
  writes no CC archive.
- `XeenSaveFormat::decode` and the save restore owners read and validate a
  v22 save. The DOS format parsers (`XeenCharacterFormat`,
  `XeenGameplayContextFormat`, the flag, quest-item and map formats) read
  DOS bytes but have no writers.
- MMModern generates both shops' wares itself (`XeenActorApproach.cpp:341`).
  Save names are limited to 20 printable characters
  (`XeenSaveFormat.cpp:604`).
- `mmodern_test_save` (`tools/GenerateTestSave.cpp`, `tools/XeenTestSave.*`)
  generates the play-test presets. It and its support library are
  `EXCLUDE_FROM_ALL`.

## Key decisions

- **A separate developer tool**, `mmodern_dos_export`, beside
  `mmodern_test_save`, built only on request and never packaged in a
  release. Its code stays under `tools/`, with a small support library for
  the tests. Production code only gains additions, and only two:
  - the DARK New Game members reader in the bridge, built the same way as
    the Clouds one;
  - writers beside the existing DOS readers, where a reader exists.
- **The archive writer reuses `BaseCCArchive::saveIndex`** for the index.
- **Input is a current v22 save.** The tool decodes it and restores it
  through the existing owners against the installation, so it exports only
  states the game itself accepts. Older saves are refused by the existing
  reader.
- **The output is built on the installation's New Game members.** Every
  member DOS expects is present, in DOS's order. The projection above fills
  in the MMModern state, and DARK map members stay at New Game.
- **NAM.** Both NAMs carry the export name, side 0 and pair marker 1.
  - A fixed marker keeps exports deterministic. DOS treats any matching
    nonzero pair the same, as the final check A showed.
  - The name is printable ASCII and does not start with a space. Its limit
    is the DOS Save As prompt's, if calibrated program analysis settles it;
    otherwise it is 15 characters, the "MMModern Export" name DOS accepted
    and listed in check A (maintainer, 2026-10-29). Longer names are refused.
  - The other NAM bytes follow the passing native pair.
- **What DOS's own Save drops, the export drops too:** monster hit points
  and status, the Spawn type within the original count, Spawn slots beyond
  that count, and the cause of a removal (a removed monster is written at
  (-128,-128)).
- **MMModern-only state with no DOS home is not written:** object flips,
  the head cache, the RNG state and process state. DOS rolls its own random
  numbers, so random outcomes after loading are not expected to match
  MMModern's.
- **Placing the party.** `--place <side> <map> <x> <y> <facing>` replaces
  only the party's position: NAM byte 29 and the PTY map, x, y and facing.
  Nothing else changes, so no progress is invented.
  - On Clouds, the placed state is restored through the existing owners,
    like any other export.
  - On Darkside, which MMModern cannot restore yet, the destination must be
    a main map that DOS can load: the DAT, MOB and EVT members are present,
    its logical geometry is valid, and the resources the load needs exist,
    neighbours included. The cell must lie inside the map. An auxiliary
    physical tile, such as `mazex130.dat` with no MOB, is refused.
  - If DOS turns out to need more than the position to arrive on Darkside,
    the implementation stops and asks the maintainer instead of setting
    anything on a guess.
- **Refusals**, each with a clear message and no partial output:
  - an input save on Darkside, which v22 cannot hold;
  - a party inside an Inn (map 0, never exercised by the investigation);
  - a state the existing restore rejects;
  - a slot outside 1..10;
  - an invalid name;
  - an output folder inside the game installation, checked on resolved
    paths (junctions, links, case and short names);
  - existing slot files, unless `--overwrite` is given.
- **DOSBox folder.** `--dosbox <dosbox.exe>` also writes the folder below.
  The supported profile is the CD World of Xeen installation MMModern
  detects, with its two CD images `GAME1.INS` and `GAME2.INS`; any other
  profile is refused.
  - `pristine/WORLD/` holds the pair, plus New Game `XEEN.CUR` and
    `DARK.CUR` built from the same members, as the passing package had.
  - `XEEN.CFG` comes from `--cfg` (the maintainer's sound setup, only
    read). Without it, the seed holds no CFG and DOS reads the
    installation's own through the C: mount; the tool checks that file
    exists.
  - The profile is for DOSBox Staging. It mounts the installation root as
    C:, `overlay/` over it, and the two images as D:, all resolved from the
    installation path.
  - The launcher first copies a non-empty `overlay/WORLD` to
    `runs/<timestamp>/`, so a re-save is never lost. Then it resets
    `overlay/` from `pristine/` and starts DOSBox.
  - Nothing is ever written to the installation or `cloud_saves`.
- **Compare.** `mmodern_dos_export --diff <folderA> <slotA> <folderB>
  <slotB>` reads `XEENnn.WOX` and `DARKnn.WOX` from each folder; `cur` as
  the slot reads `XEEN.CUR` and `DARK.CUR`.
  - It reports missing and added members, different lengths, and each
    differing offset with both values, over each member's whole length.
  - Members are named by file ID and canonical name; for colliding hashes,
    the name comes from the DAT's embedded map ID.
  - It flags a pair whose markers are zero, unequal or 255.
  - `--resave-check` passes only when the differences are the name and the
    play time, so an immediate Load and Save As is audited without
    scripts.

## Parts

- **Part A - exporter.** The DARK New Game reader, the archive writer, the
  canonical member list, the NAM/CHR/PTY and map writers, the refusals and
  the CLI. Tests:
  - unit tests: archive write/read round trip on synthetic members, name
    hashing and the alias case, NAM bytes and the marker 1/1, flag bit
    order, explored bit reversal, each refusal;
  - original data: a New Game export equals the installation's New Game
    members byte for byte, except the NAMs and the fields MMModern
    initializes itself, such as both shops' wares. Each exception is listed
    with its reason, and its bytes are checked against the save's value,
    not skipped;
  - original data: exports of the earned route used by the investigation
    (Gunther and Joe, the crate, the doors) and of the `m96-chest` preset
    are parsed back with the existing DOS format parsers and compared with
    the save. The comparison covers camera, purse, calendar, roster and
    records, flags, quest items, options, both shops' wares, both bank
    balances, doors and cells on physical tiles, explored, disabled Events,
    objects with their picture index, monsters and wall items;
  - determinism: two exports of one save are byte-identical;
  - once, outside CTest: from `earned-export-v22.mmsave`, the exporter
    reproduces the passing native pair (`dosbox-check-a-03`). The report
    explains every byte that differs, such as the name or a projection rule
    the experimental pair did not follow.
- **Part B - placing, DOSBox folder, compare and docs.** The `--place`,
  `--dosbox` and `--diff` modes, with tests:
  - placing: a Clouds and a Darkside placement change only the position
    bytes; a missing map, an auxiliary tile and an outside cell are
    refused;
  - the DOSBox folder: its files, the CRLF launcher, the resolved-path
    guards and the backup before a reset;
  - the diff: equal pairs, a differing byte, a missing member, an added
    member, a length change, colliding names, the zero, unequal and 255
    markers, and `--resave-check` passing and failing.

  Docs:
  - `docs/development.md` gains a "DOS save export" section, with the
    reason for each seed file;
  - `docs/playtests.md` says how a DOSBox comparison item is prepared with
    the tool;
  - a new rules chapter, `docs/rules/dos-saves.md`, records the container,
    pair, side, marker and legacy-conversion facts and the projection's New
    Game fields, with offsets and evidence labels;
  - `docs/original-quirks.md` records each original oddity the tool
    deliberately reproduces, such as the stale XEEN replica.

  Then prepare the play-test.

## Risks

- **DOS fails on a member it cannot find, or reads a wrong layout.** The
  member-list test, the New Game equality test and the play-test cover it.
- **A silent legacy conversion** would hide every export error behind
  regenerated shops and a rebuilt party. The marker tests and the play-test's
  re-save check cover it.
- **A field with no v22 home is mistaken for preserved.** The projection
  lists each one, and the tests check exceptions against expected values.
- **A Darkside destination DOS cannot load,** or a party placed on Darkside
  behaving differently from one that walked there. The main-map check
  refuses auxiliary tiles. Places whose behavior depends on progress show
  the New Game Darkside, which the summary states. The play-test checks the
  arrival.
- **Writing into the installation or the real saves.** Path guards are
  tested on resolved paths. The tool never reads `cloud_saves` unless
  `--cfg` points there, and then only reads.
- **Original data in the repository.** Tests read the installation at run
  time, as the other original-data tests do; nothing derived from it is
  committed.

## Acceptance

- Independent reviews of this plan and of the implementation, as Tier A
  requires.
- The complete CTest suite passes.
- One maintainer play-test after Part B, in DOSBox only, about 15 minutes,
  with folders the tool generated. The tool audits the bytes; the
  maintainer only runs DOS and reports what DOS shows, which nothing but DOS
  execution settles.
  1. Re-save check, in its own run: load an exported preset and Save As at
     once. `--resave-check` must pass.
  2. A preset with a strong party and an opened chest loads at the expected
     place, with the party and the open chest.
  3. A game the maintainer saved while playing MMModern loads in DOS where
     it was saved.
  4. The same strong party, placed on Darkside at Castleview, loads there
     and can walk, talk and fight.
