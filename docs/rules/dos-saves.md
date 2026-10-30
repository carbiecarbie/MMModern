# DOS World of Xeen saves and export projection

Offsets below are zero-based within decoded members. The exporter targets the
CD World of Xeen pair, not ScummVM's save framing. It projects the current
MMModern Clouds state onto the installation's New Game members; it does not
reconstruct a DOS history.

## Container and pair

**DOS static and resource inspection.** A slot has `XEENnn.WOX` and
`DARKnn.WOX`; the live files use `XEEN.CUR` and `DARK.CUR`. Each is a CC
archive. The index codec was calibrated at `WORLD/XEEN.DAT` file offsets
`0x45d00..0x45d91`; all 24 investigated files re-encoded byte for byte.
Names hash to IDs with collisions, so map DAT names use the ID in DAT
bytes 768..769. A missing member is fatal (`0x45fa8 -> 0x45c8a`).

The live 30-record CHR and 1530-byte PTY come from DARK (`0xf013..0xf076`).
XEEN's CHR and 812-byte PTY are a stale replica. NAM byte 29 selects the
side (`0xe938..0xe93b`, `0xf06c..0xf072`). NAM byte 30 binds the pair:
matching 1..254 denotes a native pair, zero invokes legacy conversion and
255 marks a missing slot. Legacy conversion rebuilds the party from XEEN and
regenerates wares (`0xd6f7..0xd8bf`, `0xdaea..0xdb57`). The exporter uses
matching markers 1. The maintainer's DOS check loaded a native projected
Clouds pair and immediately saved it again: only eight name bytes and DARK
PTY byte 654 (elapsed time) changed among 666,345 payload bytes. The M97
play-test (2026-10-29) repeated this with a re-exported state: only the name
and play time changed, and the projected PTY bytes 8, 9 and 17 survived.

**DOS play: loading marks the party's cell as stepped.** In the same
play-test, the first item-1 export put a party with a Cartographer at map 9
`(14,11)`, North, on a cell whose stepped bit was clear. It was loaded and
saved at once without moving. Besides the name and play time, only XEEN's
`maze0009.dat` byte 883 changed, from 0 to 0x02. That is
`860 + 11*2 + 14/8` and `0x80 >> (14 & 7)`: the party's own cell in the
MSB-first stepped order the exporter writes (DARK PTY 10..13 = 0, 14, 11, 9
before and after). **Calibrated DOS static, high confidence (2026-10-30):**
map load sets stepped at `0xf58b`; shared Load/New Game entry loads at
`0xd08c` and redraws at `0xd0da`, before the entry Event pass. Scene lighting
`0x12012..0x12093` checks Cartographer at `0x1203c..0x12074` and marks the
current maze's bitmap through `0x1206f -> 1b98:3f73`; redraw clears the
pending step at `0x1184f`. This explains the observed changed bit; see
[arrival evidence and its window gates](evidence.md#travel-lighting-checks).
Pinned `Map::load` / `Party::handleLight` agree. So `--resave-check`
passes only when the party's cell is already stepped or nobody in the party
has Cartographer.

## Projection

**Resource inspection; field names reference only unless noted.** All 30
DARK CHR records are written from the current roster. Their names occupy
bytes 0..15; bytes after the first NUL retain New Game padding because the
save does not retain that padding. Attributes, skills, awards, spells, inventory,
resistances, conditions and character statistics occupy the rest of each
354-byte record. Award 9 uses the full byte 66, which overlaps award 73.

The live DARK PTY stores party counts and active roster IDs at 0..9, facing,
x, y and map at 10..13, sound settings at 14..15, effects and difficulty at
18..27, Clouds wares at 28..603, completion and calendar fields at 604..619,
light and resistances at 620..631, purse and bank at 638..653, play time at
654..657, rested state at 658, flags at 659..746, quest items at 747..831,
Darkside wares at 832..1407 and text/speech options at 1528..1529. Flags use
DOS MSB-first order (DOS static `0x24f07`, `0x24fb3`). Only the active ID
prefix, the first 30 quest flags and the first 35 quest-item counters have
current homes.

The following retain New Game values because v22 has no corresponding state:
the unused party-ID suffix, PTY 16, prior map PTY 17, Darkside and World
completion PTY 606..609, DOS counters PTY 632..637, Darkside flags PTY
691..722, the unused quest-flag bits, Darkside quest items PTY 782..831,
character flags PTY 1408..1527, the XEEN party replica and all DARK map
history. PTY 16's meaning is unknown. The independent XEEN and DARK New Game
members are retained; they are not historical copies of the exported party.

Clouds map changes project to DAT cell words 0..511, attributes 512..767,
Tavern tip byte 827 and stepped bitmap 860..891. DAT seen bitmap 828..859
stays New Game. Object positions and picture indices, monster and wall-item
positions are written to MOB; Event replacements change EVT opcodes. The
DAT stepped bitmap reverses the in-memory bit order. **DOS static:** the map
writer is at `0x26509..0x26772`; object picture index is written at
`0x26698..0x266ce`.

DOS's own save drops monster HP and status, spawned types within the original
count, spawned slots beyond that count and the cause of a monster removal.
A removed monster is written at `(-128,-128)`. MMModern-only object flips,
head cache, RNG and process state have no DOS representation. DOS uses its own
randomness after loading.

## Refusals

Export refuses a Darkside input or history, an Inn/map-0 party, an invalid
or older save or any state the existing restore rejects. It also refuses a
slot outside 1..10, a name outside 1..15 printable ASCII characters or with a
leading space, an output inside the resolved game installation, and an
existing slot without `--overwrite`. Character attributes exceeding one byte,
quest-item counters above 255 and award 73 cannot be represented and are
refused. A placement must have a main DAT, MOB and EVT, valid logical
geometry, its load resources and neighbours, and a cell inside the map;
auxiliary physical tiles are not destinations. Darkside progress is not
exported. **DOS play (maintainer, 2026-10-29):** a party placed on Darkside
at Castleview (map 29, `(14,4)`, North) loaded there and could walk, talk and
fight. Only the position was written; no other arrival state was needed.

The DOSBox preparation also refuses a missing executable, a non-CD profile or
missing `GAME1.INS`, `GAME2.INS`, `WORLD/XEEN.EXE` or selected CFG. Its
resolved `pristine`, `overlay` and `runs` paths must remain inside the output
folder and outside the installation. Existing output files require
`--overwrite` and must be ordinary files; the launcher refuses reparse points
in its working tree before backing up or resetting the overlay.
