# Milestone 6: Clouds Area A1

Historical record; not current rules or scope.

The mode below loads the original state of map 001 from `xeen.cc` and exits
after printing the report. It does not initialize SDL, create a framebuffer or open a window.

```bash
cd /d/Projetos/MModern/mmodern
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/mmodern.exe --inspect-map "F:/Games/gog/Might and Magic 4-5"
```

CMake regenerates the rules automatically. The format tests are controlled
by `BUILD_TESTING` (ON by default) and use only standard C++.

## Implemented flow

- `src/compat/scummvm/ScummVmXeenBridge.cpp`: opens `xeen.cc` with CCArchive;
  concatenates the present blocks `2a0c`, `2a1c`, `2a2c`, `2a3c`, `284c`, `2a5c`
  in that order. InitialCloudsArchive uses BaseCCArchive to read the internal index,
  checks bounds and returns the members in memory, without applying XOR again.
- `src/formats/xeen/XeenAssetSource.{h,cpp}`: provides bytes through a standard API;
  its constructor without dimensions allows access without a framebuffer.
- `src/formats/xeen/XeenMapFormat.{h,cpp}`: pure DAT, MOB and EVT parsers.
- `src/games/xeen/XeenMap.h`: geometry data, tables, entities and instructions,
  without ScummVM/SDL types. Original words and attributes are preserved.
- `src/games/xeen/XeenMapLoader.{h,cpp}`: loads only `maze0001.dat`,
  `maze0001.mob` and `maze0001.evt` and validates the internal ID 001.
- `src/app/Application.cpp`: prints the summary and grid, with Y=15 at the top.

No XEEN.CUR, save, mm.dat, HED, TXT or neighbouring map is loaded. The previous
graphical mode remains available using only the directory as the argument.

## Values verified on the real installation

| Data | Result |
|---|---|
| ID / type | 001 / outdoor |
| Dimensions / cells | 16x16 / 256 |
| Scenery objects | 7 records, 7 active |
| Monsters | 11 records, 11 active |
| Wall objects | 1 record, 0 active, 1 without resource |
| EVT instructions | 60 |
| Neighbours N/E/S/W | none / 005 / 002 / none |

Correction of the initial analysis: the wall record is `(99,99,0,0)`, but the
corresponding table contains only FF. The original reader ignores this reference
without a sprite. MMModern preserves the raw record and reports that it is not active.
None of these counts is hard-coded in the program.

Entities with X or Y equal to -128 stay in the original order and are counted
as disabled. "Active" means a resolved reference and the absence of that
marker; it does not mean visible or inside the 16x16 grid. For example,
an original Area A1 object has Y=-2, which must be preserved.

The grid uses local indices in hexadecimal. The local index -> surface type table
is printed below it. The 60 EVT entries are instructions; their opcodes and
parameters stay opaque, without script grouping or execution.

## Fidelity and references

The ScummVM paths below are relative to `D:/Projetos/MModern/scummvm-master`:

- `engines/mm/xeen/files.cpp`: SaveArchive::reset, order of the initial blocks;
  SaveArchive::createReadStreamForMember, internal payload without XOR.
- `engines/mm/shared/xeen/cc_archive.cpp`: original CCArchive and BaseCCArchive.
- `engines/mm/xeen/map.cpp`: MazeData::synchronize (892-byte DAT),
  MobStruct and MonsterObjectData::synchronize (MOB).
- `engines/mm/xeen/scripts.cpp`: MazeEvent::synchronize (EVT).
- `devtools/create_mm/create_xeen/constants.cpp`: WALL_SHIFTS.

Indoor maps use N/E/S/W walls in bits 12/8/4/0, following WALL_SHIFTS and
Map::setWall. We do not use the order of the C++ bitfields declared in map.h as
the binary specification. Outdoor maps use surface/intermediate/top/overlay
in bits 0/4/8/12; the parallel byte provides flags and the indoor surface.

The parsers reject sizes, lengths, indices outside the 16 slots and
incomplete terminators. They preserve holes in the object table and use the
compacted monster/wall object table, as the engine does. Unresolved references
are kept explicitly as resourceId=-1.

## Automated graphical regression

The test target below calls the same Application::run as the graphical mode, with
the real resources. The dummy driver exercises composition, SDL texture/presentation
and normal shutdown without opening a window. It does not replace a manual visual inspection.

```bash
cmake --build build --target mmodern_graphics_smoke --parallel 4
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/mmodern_graphics_smoke.exe "F:/Games/gog/Might and Magic 4-5" escape
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./build/mmodern_graphics_smoke.exe "F:/Games/gog/Might and Magic 4-5" quit
```

To test the executable visually:

```bash
./build/mmodern.exe "F:/Games/gog/Might and Magic 4-5"
```

The inspection was also run with a deliberately invalid SDL_VIDEODRIVER;
it exited with code 0. The synthetic tests cover the -128 markers, double empty
lists, placeholders, nibbles, endianness, bitmaps and truncated files.
