# Milestone 53 plan - CD edition data

**Tier A. Status: completed and accepted.** Plan `340587d`; Part A
`69b82d0`, `52b8760`; Part B `2bbe975`; review fix `37101c4`.

## Goal

Make the World of Xeen two-CD talkie edition the only reference data, as the
maintainer decided (it is the edition GOG sells and the one played in
DOSBox), and keep everything accepted in M44-M52 faithful on it. Floppy
support is not needed; the loader stays generic.

## Final scope

- **Data source (Part A).** One read-only source for `XEEN.CC`, `DARK.CC`
  and `INTRO.CC`: from the GOG disc images (ISO9660 located through the
  `.INS` cue files) or from a copy of the CD's `GAME` folder. CD data is
  chosen ahead of the floppy archives at a GOG root; one complete disc is
  enough; mixed or ambiguous sources are rejected. CC decoding is
  unchanged and fingerprints cover the logical archive bytes. Saves are
  never written inside any resolved source directory.
- **DOS interface text.** A bounded reader of the installed
  `WORLD/XEEN.DAT` (87 named fields) replaces the text generated from the
  pinned ScummVM constants; the Charges format now comes from the DOS
  bytes. A plain CD copy needs `--ui-data <XEEN.DAT>`.
- **Floppy assumptions removed.** Fixed sizes and checksums of floppy
  resources (map 23 data, monsters, the initial party record, service art
  and others) became structural checks.
- **Event differences (Part B).** All 26 changed Clouds Event scripts and
  the 20 changed text files are covered: CD speech (`PlayCD`, `0x28`) shows
  the deferred-audio notice and the script continues without time or RNG;
  the copy-protection steps on maps 14 and 28 are gone; map 33's prompt
  changed; relocated lines keep their meaning.

## Decisions

1. **Save v6 unchanged** in layout and meaning; saves bound to the floppy
   archives are rejected with a data-edition message.
2. **M44 digests** were revalidated on CD data: the complete traces of all
   three routes are identical to the floppy ones; the save differences are
   the archive signatures, the Phirna Event indices relocated by six CD
   speech records (125..135 -> 131..141), the removed Vertigo protection
   overlay `{28,764}` and the derived length and CRC. New digests were
   recorded after maintainer approval.
3. The M52 initial party and characters are byte-identical on both
   editions.

## Results

- Tests for archive access from images and plain files, fingerprints, the
  DAT reader and every changed Event script; Myra's integration witness was
  migrated to CD data and now runs in CTest. Older excluded witnesses broken
  since M47-M48 (SaveResume, Phirna, Remove, WhoWill, IndoorObject) are left
  for the test-cleanup milestone.
- Full CTest 169/169 on the optimized build; closure check on the Debug
  build: `ctest -L fast` 137/137 and the three M44 scenarios passed.
- Independent implementation review (REVISE): two excluded witnesses still
  expected floppy Event counts; fixed.
- Maintainer play-test against the DOSBox CD edition passed.
