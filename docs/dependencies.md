# MMModern dependencies

MMModern is an independent SDL application. It does not use ScummVM's SDL
backend or instantiate its engine. The compatibility target reuses only the
Xeen archive/sprite code, the plain Nuked OPL3 core, and the Common, Graphics,
and Image libraries from a separately configured ScummVM build. Audio never
instantiates ScummVM's audio classes or OSystem; `g_system` remains null.

## Pinned ScummVM revision

The validated known-good ScummVM dependency is:

- upstream: `https://github.com/scummvm/scummvm.git`
- commit: `6814ee9ba54582f5b5adcffab49efbbd8f589edd`
- exact tag: none
- validation state: detached HEAD with an empty `git status --porcelain`
- validation date: 2026-09-07
- validation platform: Windows x86-64, MSYS2 UCRT64

Always check out the full commit SHA. A moving branch name such as `master` is
not a dependency version.

Use separate source and build directories. Their names and locations are not
fixed; pass them to MMModern through `SCUMMVM_SOURCE_DIR` and
`SCUMMVM_BUILD_DIR`. Do not build inside the ScummVM source checkout.

For example:

```sh
git clone https://github.com/scummvm/scummvm.git <scummvm-source>
git -C <scummvm-source> checkout --detach 6814ee9ba54582f5b5adcffab49efbbd8f589edd
git -C <scummvm-source> status --porcelain
mkdir <scummvm-build>
cd <scummvm-build>
```

The final status command must produce no output before validation begins.

## Toolchain and library versions

The pinned revision was validated with:

- MSYS2 UCRT64 on Windows x86-64
- GCC/G++ 16.2.0, MSYS2 Rev3
- GNU Make 4.4.1
- CMake 4.4.2 with the `MSYS Makefiles` generator
- SDL2 2.32.10, linked through the shared `SDL2::SDL2` CMake target and
  `libSDL2.dll.a`; `SDL2.dll` is required at runtime
- zlib 1.3.2, linked through `ZLIB::ZLIB` and `libz.dll.a`

The ScummVM build uses GNU++11. MMModern uses GNU++17 and must use an
ABI-compatible compiler/toolchain when compiling ScummVM source directly and
linking the prebuilt ScummVM archives.

## ScummVM configuration and required artifacts

From an MSYS2 UCRT64 shell in the separate ScummVM build directory, configure
with:

```sh
SDL_CONFIG=/ucrt64/bin/sdl2-config \
  <scummvm-source>/configure \
  --backend=sdl \
  --disable-all-engines \
  --disable-detection-full
```

Only four ScummVM build artifacts are required. Build them directly instead of
building ScummVM or its engines:

```sh
make -j4 \
  image/libimage.a \
  graphics/libgraphics.a \
  common/formats/po_parser.o \
  common/libcommon.a
```

MMModern checks for and links exactly these files from `SCUMMVM_BUILD_DIR`:

- `image/libimage.a`
- `graphics/libgraphics.a`
- `common/formats/po_parser.o`
- `common/libcommon.a`

`SCUMMVM_BUILD_DIR/config.h` is also required as evidence that the external
tree was configured.

MMModern compiles these ScummVM source files directly from
`SCUMMVM_SOURCE_DIR`:

- `engines/mm/shared/xeen/cc_archive.cpp`
- `engines/mm/shared/xeen/sprites.cpp`
- `audio/softsynth/opl/nuked.cpp`

The sprite source is compiled into a generated private object and filtered as
described below. The compatibility code includes the corresponding Xeen
archive, sprite, surface, and file headers as well as their transitive Common
and Graphics headers.

### Nuked OPL3 core

`cmake/ScummVmXeen.cmake` compiles the pinned `nuked.cpp` with GNU++11,
`-O2 -ffunction-sections -fdata-sections`, then objcopy removes sections
matching `*3OPL5NUKED3OPL*` (the class adapter and thunks), `*5Audio*`,
`*_ZT*` (RTTI/vtables) and `*refptr*`. The generated `nuked.core.o` retains
the plain core functions and tables. No source patch, ScummVM mixer, backend
or additional audio translation unit is required. The unused undefined symbol
names left in the object have no relocations from retained sections.

`XeenOpl3` owns only the plain chip struct. It calls `OPL3_Reset` at native
49716 Hz, unbuffered `OPL3_WriteReg`, and `OPL3_Generate`. No stereo-extension
define is supplied; the header's default is zero. MMModern's promus interpreter
places interrupts using the exact ratio `49716 * 16390 / 1193182`. SDL owns one
resampler for the FM source, and MMModern mixes its output at the device rate.
One interpreter advances by an exact integer native-sample accumulator on the
presentation clock. Its timestamped register writes and completed sample count
cross one queue to the chip consumer. A pull ahead of published time emits
silence without advancing the chip; callback size and device prebuffering
cannot move commands onto a different driver tick. Output latency never delays
the game or its fade waits.
Title/SDL and gameplay/injected clocks may have different epochs. Explicit
session handoffs consume the outgoing clock's final interval, then rebase the
source counter while retaining elapsed driver time and fractional phase;
ordinary backwards movement within a source is rejected.

The retained core is Nuked-OPL3-fast 1.8-fast.2 (upstream 1.8, cfedb09),
copyright Nuke.YKT (2013-2020) and Tony Gies (2026), licensed LGPL-2.1-or-later.
Preserve these notices and the pinned corresponding source; releases include
the pin's `LICENSES/COPYING.LGPL` alongside the GPL and runtime-library notices.
MMModern's existing ScummVM adaptations remain GPL-3.0-or-later.

The external DRO v2 validation data is configured with
`-DMMODERN_PROMUS_CAPTURES_DIR=<capture-directory>`, alongside
`MMODERN_XEEN_DATA_DIR`. Captures and derived register streams are never bundled.
The comparison removes DOSBox's initial register snapshot and unchanged writes,
aligns the sustained song/effect suffix and PIT phase, and compares both banks
through capture end. The startup's EXE intro is outside M82. The supplied
`promus-inn` records the Tavern song; `promus-wall-bump` records effect 46.

## `xeen_probe` provenance and replacement

The official pinned revision does not contain `devtools/xeen_probe`.

The earlier unversioned local ScummVM snapshot contained exactly two additions
under that directory:

- `module.mk` (SHA-256
  `b7e15826f0b64018ba67271b840e4498b4871a660e4505bdb3afa77fff3f9f4c`)
- `xeen_probe.cpp` (SHA-256
  `4bac641530fd016b768408d8227ea7236257726f1a01c27fbbeeaa68da00b5c4`)

Their upstream provenance could not be established, so they are not part of
the pinned dependency and must not be copied silently into a clean ScummVM
checkout. No source patch is required to prepare MMModern's dependency: the
direct four-target `make` command above is the MMModern-specific build helper
procedure and produces everything the integration consumes while leaving the
pinned checkout clean.

## Revision-sensitive sprite filtering

`cmake/ScummVmXeen.cmake` compiles `sprites.cpp` with
`-ffunction-sections -fdata-sections`, then uses GNU `objcopy` to remove only
the unused engine-specific `Common::Path` constructor/loader and associated
`MM::Shared::Xeen::File` destructor, thunk, VTT, and vtable-reference sections.
The stream decoder and sprite drawing implementation remain intact.

This filtering depends on ScummVM class/signature layout, GNU C++ mangled
names, the MinGW ABI, and PE/COFF section naming (`.text$`, `.xdata$`,
`.pdata$`, `.debug_frame$`, and `.rdata$.refptr`). It is therefore explicitly
revision-sensitive. A ScummVM revision change must verify that every named
section still has the intended meaning and that no engine-only unresolved
references remain; successful compilation alone is not sufficient.

No file in the pinned ScummVM source checkout is modified by this process.

## Read-only CD data and DOS dialogs

The English World of Xeen two-CD edition supplies the runtime data. Set
`MMODERN_XEEN_DATA_DIR` to its GOG installation root, a CD `GAME` directory,
or a parent containing `GAME`. Original-data tests are required when
`BUILD_TESTING` is enabled: a missing configured source fails configuration,
and an incomplete or malformed source fails validation rather than removing tests.
`MMODERN_XEEN_UI_DATA` optionally supplies the installed uncompressed English
`XEEN.DAT` for tests using a CD copy. The application receives that path through
`--ui-data <path>`; build settings do not supply runtime paths.

At a GOG root, cue metadata in `.INS` files locates the MODE1/2352 data track.
MMModern reads ISO9660 `/GAME/XEEN.CC`, `DARK.CC` and `INTRO.CC` through bounded
logical streams, with the unchanged pinned CC index/name decoding. One complete
disc suffices; byte-identical complete candidates are selected deterministically,
and conflicting or ambiguous candidates refuse startup. Co-located root floppy
archives are never used. A pre-existing plain `GAME` copy must contain all three
archives; MMModern never creates a copy, mounts an image or executes a game module.

The ISO does not contain the UI module. GOG discovery uses `WORLD/XEEN.DAT`;
a plain CD copy needs an unambiguous `WORLD` sibling or explicit `--ui-data`.
The bounded DOS reader validates the MZ load image, relocations, named field
bounds, far-pointer tables, terminators, format arguments and control streams.
Packed root DATs and unsupported layouts fail clearly. Dialogs, item/service
screens and Rest retain the DOS bytes, including the single alignment `r` in
Charges. Title/credits and save/load fields are exposed for later screens only.

`generated/XeenDialogDrawing.inc` retains only the pinned numeric window
symbols and four-shade text palettes. `-DialogDrawing` generates this private
build include from the verified constants blob; text is read at runtime from DAT.
The separate item catalog keeps its existing pinned provenance below. No DOS
strings or original-data fixtures are distributed.

Save files remain envelope v6, schema 9, content 14. Archive fingerprints hash
size and CRC32 over logical encrypted CC bytes, so both discs and equal plain
copies have identical signatures. Floppy-bound saves fail the data-edition check
before restoration; signatures and resource-relative overlays are never translated.
Save paths must stay outside every resolved commercial source directory, including
separately supplied image and UI-module directories.

## Build-generated English item catalog

Milestone 24A derives the embedded English catalog directly from one immutable
object in the pinned ScummVM repository:

- path `devtools/create_mm/files/xeen/CONSTANTS_7`;
- Git blob `455b2eb3900a60be910e4d045d103a73586e73b0`;
- 35,065 bytes; SHA-256
  `a3022d378e7570a56332f30128942afe02ae2bfdef70c307b2de997eb07a9e77`.

ScummVM's `LangConstants::writeConstants` produces this tracked artifact from
the language getters. At the pinned revision it writes `ITEM_BROKEN`,
`ITEM_CURSED`, `ITEM_OF`, then the six required arrays with counts
7/41/14/11/22/74. MMModern parses only that bounded block, byte range
`[20680,22438)`, rather than the complete unversioned constants stream.

`tools/GenerateXeenItemCatalog.ps1` requires exact checkout HEAD, exact tree
entry mode/path/blob ID, size and independent SHA-256. It then reads the blob
through `git cat-file`, never through the ScummVM worktree. Inherited `GIT_*`
state, replacement objects and global/system Git configuration are removed from
the plumbing process. Only the verified blob supplies catalog bytes; mutable
worktree files and compiler inputs are outside the generation boundary.

No ScummVM header is compiled and no compiler preprocessing participates in
catalog generation.

The generator validates the three nonempty scalar strings, exact array tags and
counts, reserved/required entries, the fixed block end, a 63-byte token limit and
the 16,384-byte aggregate output limit. It emits deterministic fixed-width octal
escapes with schema 1, English language ID 7 and the exact revision marker.
Publication uses a flushed sibling temporary file and atomic replacement;
Windows PowerShell passes `NullString.Value` as the CLR-null backup path to
`File.Replace`. Failed replacement preserves the previous destination and cleans
the temporary file; unchanged output is not rewritten. The private include remains
ignored, build-tree-only and unavailable as runtime or installed companion data.

Generation runs during configuration and on every build, so a wrong repository
revision or object identity fails immediately and stale generated output cannot
bypass the gate. The build-only generator uses Windows' inbox PowerShell/.NET and
Git and adds no application runtime dependency. Cross-compilation remains
unsupported.

The embedded source-derived names are distinct from commercial material text.
At runtime MMModern reads only `mae.xen` through the selected installation's
`DARK.CC`; it does not load ScummVM's `mm.dat`, the generated include, the ScummVM
source tree, a working-directory catalog or an environment-selected companion.
The parser accepts exactly 131 bounded NUL-terminated entries and publishes no
partial table on missing, malformed or failed reads. Such failures retain bounded
base-name/numeric fallback behavior and do not prevent startup.

All archive roles use the same bounded read-only source and pinned CC decoder.
The bridge validates the complete index and member extents before upstream
short-read paths, rechecks the retained index on reopening, then reads and XOR
decodes the requested outer payload exactly once. A missing optional member is
`Missing`, structurally invalid readable bytes are `Malformed`, and invalid
extents or read failures are `ReadError`. Sprite reconstruction compares admitted
bytes under physical source, role and resource ID, including DARK and INTRO.

The normal configure/build commands below reproduce the adapter. Its focused
validation is:

```sh
cmake --build <mmodern-build> --parallel 4 --target \
  mmodern_item_catalog_data mmodern_item_catalog_smoke
ctest --test-dir <mmodern-build> --output-on-failure \
  -R '^xeen_item_catalog($|_)'
<mmodern-build>/mmodern_item_catalog_smoke.exe <game-directory>
<mmodern-build>/mmodern_item_catalog_smoke.exe --verify-reference \
  <game-directory> <scummvm-source>
```

The pinned blob and generated text remain ScummVM GPLv3-or-later-derived material.
Preserve the upstream license notices, contributor attribution, exact revision,
blob/path identity and corresponding source availability in distributions.
Commercial `DARK.CC/mae.xen`, extracted `mae.cld`, original archives and
original-data fixtures remain external and must not be bundled.

## Bounded equipment restriction data

`XeenEquipment.cpp` adapts 34 weapon and eight armor decimal masks from pinned
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`,
`devtools/create_mm/create_xeen/constants.cpp`,
`LangConstants::ITEM_RESTRICTIONS` and `RESTRICTION_OFFSETS`. Private constexpr
uint8 arrays use validated ID minus one; their entries correspond to upstream
index `id` for Weapons 1..34 and `id+35` for Armor 1..8. No other masks,
generated catalog changes or runtime reference-table dependency are introduced.
The bounded equipment and frame contracts belong in the
[M25 specification](archive/milestone-25-plan.md#equipment-domains-and-frame-contract).

The predicates derive from `engines/mm/xeen/item.cpp`,
`InventoryItems::passRestrictions/removeItem` and
`WeaponItems/ArmorItems/AccessoryItems::equipItem` at that same revision.
This adapted numeric data and logic remain ScummVM GPLv3-or-later-derived
material, attributed to the ScummVM developers listed in upstream `COPYRIGHT`.
Preserve that attribution and license, and supply the corresponding adapted
source and pinned upstream source in distributions. Commercial resources are
not part of these tables.

## Bounded armor repair provenance

`XeenArmorRepair.h` adapts the thirteen armor base costs in
`LangConstants::ARMOR_BASE_COSTS` and repair divisor 10 from
`devtools/create_mm/create_xeen/constants.cpp`, together with material-38
integer division, minimum-price and broken-bit behavior in
`engines/mm/xeen/dialogs/dialogs_items.cpp` (`ItemsDialog::calcItemCost`,
repair handling), at pinned revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The adapted data and rules are
GPL-3.0-or-later, attributed to the ScummVM developers in upstream `COPYRIGHT`,
without warranty; preserve the corresponding source and attribution in
redistribution. No merchant stock tables or commercial assets are embedded.
The exact bounded dispatch, prices and separate departure operation belong in
[M38's contract](archive/milestone-38-plan.md).

`XeenEquipmentPurchase.cpp` additionally adapts the thirty-three ordinary
weapon base costs from `LangConstants::WEAPON_BASE_COSTS` and the Buy divisor-0
rule in `ItemsDialog::calcItemCost` at the same pinned revision. Armor Buy uses
the shared thirteen-entry armor base-cost table, without Repair's divisor or
material adjustment. These private numeric adaptations and rules remain
ScummVM GPL-3.0-or-later-derived material attributed to the ScummVM developers
in upstream `COPYRIGHT`; preserve their notices and corresponding source in
redistributions. They embed no commercial resource or modifier/effect table.
The plain-offer domain, immutable browsing adaptation, physical movement and
content-specific stock admission belong in [M42's contract](archive/milestone-42-plan.md).

## Bounded actor interpretation and approach provenance

The 26A parser and approach adaptation use ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`: Xeen `map.cpp`
(`MonsterStruct::synchronize`, `MonsterObjectData::synchronize`), `party.cpp`
(`Party::synchronize/changeTime`), `interface.cpp` (`chargeStep`, `stepTime`,
`perform`, `draw3d`), `combat.cpp` (`moveMonsters`, `canMonsterMove`,
`moveMonster`, movement grids), `combat.h` (107-record capacity), and
`interface_scene.cpp` (`setOutdoorsMonsters`). The classifier reuses the existing
position constants sourced from `devtools/create_mm/create_xeen/constants.cpp`.
M26B's four bounded outdoor actor placements adapt selected-slot/order, anchor,
scale and clipping values from `interface_scene.cpp` (`OutdoorDrawList` and
`setOutdoorsMonsters`) at the same pin. `XeenOutdoorScene::actorCommands` emits
them through the existing ordered scene stream and sprite drawer.
This adapted logic remains GPL-3.0-or-later material attributed to the ScummVM
developers in upstream `COPYRIGHT`; distributions must preserve corresponding
source/provenance. It adds no engine linkage or copied commercial resource.

`readCloudsMonsterStatisticsFromDarkArchive` extends the existing lazy Dark
archive owner with a checked, explicitly named `xeen.mon` read. Its private
extent/read helper is shared only with `mae.xen`; the material size limit and
availability semantics remain unchanged. The 60-byte parser enforces the CC
65535-byte member bound separately. Missing data, malformed bytes and I/O failures
remain distinct. The existing archive-index-construction limitation documented
above still applies. Actor admission/context/persistence contracts belong in
[M26](archive/milestone-26-plan.md#final-ownership-and-lifecycle).

## MMModern configuration and validation

Configure MMModern in its own new build directory with explicit dependency
paths:

```sh
cmake -S <mmodern-source> -B <mmodern-build> \
  -G "MSYS Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSCUMMVM_SOURCE_DIR=<scummvm-source> \
  -DSCUMMVM_BUILD_DIR=<scummvm-build>
cmake --build <mmodern-build> --parallel 4
ctest --test-dir <mmodern-build> --output-on-failure
```

### Optimized test build (`build-rel`)

`AGENTS.md` runs iteration (`ctest -L fast`) and the complete suite on
`build-rel`, an optimized build with debug information and assertions kept.
CMake's default RelWithDebInfo flags add `-DNDEBUG`, so override them. From the
MMModern source directory, with `C:\msys64\ucrt64\bin` on `PATH` (it provides
the compiler, Ninja, `objcopy` and `SDL2.dll`):

```sh
cmake -S . -B build-rel -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -g" \
  -DSCUMMVM_SOURCE_DIR=<scummvm-source> \
  -DSCUMMVM_BUILD_DIR=<scummvm-build>
cmake --build build-rel
cmake --build build-rel --target mmodern_test_save   # optional tool, not built by default
ctest --test-dir build-rel -L fast -j 8 --output-on-failure
```

`build-rel/build.ninja` must not contain `NDEBUG`. Outside Debug builds, test
and tool executables are linked with `-Wl,--strip-debug`: debug information is
about 97% of each one, and writing it dominated incremental builds (one source
change relinked 183 executables in 505 s). The generated code is unchanged and
`mmodern.exe` keeps its debug information. Configure with
`-DMMODERN_TEST_DEBUG_INFO=ON` to debug a test or tool in `build-rel`. The complete suite runs
through `tools/run-full-ctest.ps1 -BuildDir build-rel`, as described in
`tools/AGENTS.md`: by default in parallel (half the logical processors, at
most 8 jobs); CMake runs the real-clock and native-window tests alone and gives
every test a load-independent hang-guard timeout. Play-tests and release
packages use `build-rel` as well; the Debug build above (`build-m44`) remains
for debugging only.

The original-data checks that used to be opt-in smoke targets run with the
suite since M66 (`xeen_regional_original`, `xeen_party_original`,
`xeen_item_catalog_original`, `xeen_item_catalog_reference`,
`xeen_object_visual_original` and the `xeen_*_original` event, flag and
navigation checks); the item catalog's reference verification needs the pinned
ScummVM checkout, as the catalog section above describes. The floppy-era
integration programs that no longer matched the CD data (indoor map, indoor and
outdoor objects, Remove, WhoWill, Phirna, save/resume, the graphics smoke and
the M32, M35 and M36 witness drivers) were retired in M66; the registered unit,
original-data and process tests cover their subjects.

Validation of the pinned commit completed with:

- full MMModern build: passed
- complete CTest suite of that time: 32/32 passed
- all real-data smoke targets of that time: built and passed
- real archive text, party, flags, indoor map, navigation, event script,
  interpreter, and event-system checks: passed
- SDL dummy/software runtime checks for UI, outdoor map, indoor map, automatic
  event, manual interaction, Castle Basenji No, and Castle Basenji Yes: passed
- Milestone 14 manual presentation checks for `Air / Corner`, `Snake Oil`, and
  Castle Basenji text 19: passed
- Castle Basenji Action 44 No path without teleport and Yes path with teleport:
  passed
- generated 320x200 visual-validation frames for the sign, reduced door text,
  Castle question, No state, and Yes destination: visually inspected and passed

Real-data smoke tests require a legally obtained World of Xeen installation;
original game data is never part of MMModern or its dependency checkout.

## Release packaging

A release zip is built from the tested `build-rel` executable after the complete
suites pass:

```powershell
./tools/package-release.ps1 -BuildDir build-rel -OutputDir D:/MModern-releases
```

The script strips debug information into a staging copy (`build-rel` keeps its
unstripped binary), copies exactly the non-system DLLs that `ldd` resolves, and
only from `C:\msys64\ucrt64\bin`, and adds `LICENSE.txt`, `README.md`,
`THIRD-PARTY.txt` (bundled libraries, the GCC runtime exception, the `v<version>`
tag and the pinned ScummVM revision), the MSYS2 license texts under `licenses\`
and the launcher: `tools/release/MMModern-launcher.ps1` with two CRLF batch
files that start it, `MMModern.bat` and `Change game folder.bat`. The launcher
finds the game through GOG's registry entries or a folder picker, accepts only
a folder with `WORLD\XEEN.DAT` and remembers it in
`%LOCALAPPDATA%\MMModern\game-folder.txt`. The output directory must be outside the
repository and the zip and staging folder must not exist yet. No game data, saves,
logs or build trees are packaged. Before publishing, unpack the zip into an empty
folder and start it with a `PATH` without MSYS2.

## Updating ScummVM

Changing the pinned ScummVM revision requires repeating the complete dependency
validation in fresh source, ScummVM build, and MMModern build directories. The
new revision may be documented only after provenance is recorded, the checkout
is clean, all four artifacts build, sprite filtering is reviewed, the full
MMModern build and CTest suite pass, and the relevant real-data and Milestone 14
runtime/manual checks pass.

ScummVM is licensed under GPLv3 or later. MMModern must remain compatible with
the license of the reused code.
