# MMModern dependencies

MMModern is an independent SDL application. It does not use ScummVM's SDL
backend or instantiate its engine. The compatibility target reuses only the
Xeen archive/sprite code and the Common, Graphics, and Image libraries from a
separately configured ScummVM build.

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

The latter is compiled into a generated private object and filtered as
described below. The compatibility code includes the corresponding Xeen
archive, sprite, surface, and file headers as well as their transitive Common
and Graphics headers.

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

For this optional member only, the bridge uses the existing lazy Dark archive and
its decoded index entry, checks the indexed `mae.xen` extent against the opened
archive size, then performs a checked read and XOR decode. It therefore does not
invoke ScummVM's fatal member short-read path for `mae.xen`. With a valid loaded
CC index, a missing member is `Missing`, structurally invalid readable bytes are
`Malformed`, and an invalid extent or short read is `ReadError`. This narrow
recovery does not change `clouds.dat`, other required-resource behavior, or claim
recovery from a corrupt archive index that fails during ordinary CC construction.

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
The exact sequences and class/frame contracts belong in the
[M25 specification](milestone-25-plan.md#exact-class-restrictions-and-source-adaptation).

The predicates derive from `engines/mm/xeen/item.cpp`,
`InventoryItems::passRestrictions/removeItem` and
`WeaponItems/ArmorItems/AccessoryItems::equipItem` at that same revision.
This adapted numeric data and logic remain ScummVM GPLv3-or-later-derived
material, attributed to the ScummVM developers listed in upstream `COPYRIGHT`.
Preserve that attribution and license, and supply the corresponding adapted
source and pinned upstream source in distributions. Commercial resources are
not part of these tables.

## Bounded actor interpretation and approach provenance

The 26A parser and approach adaptation use ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`: Xeen `map.cpp`
(`MonsterStruct::synchronize`, `MonsterObjectData::synchronize`), `party.cpp`
(`Party::synchronize/changeTime`), `interface.cpp` (`chargeStep`, `stepTime`,
`perform`, `draw3d`), `combat.cpp` (`moveMonsters`, `canMonsterMove`,
`moveMonster`, movement grids), `combat.h` (107-record capacity), and
`interface_scene.cpp` (`setOutdoorsMonsters`). The classifier reuses the existing
position constants sourced from `devtools/create_mm/create_xeen/constants.cpp`.
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
[M26](milestone-26-plan.md#26a-concrete-interfaces-and-publication-contract).

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

Build and run the real-data integration targets explicitly:

```sh
cmake --build <mmodern-build> --parallel 4 --target \
  mmodern_graphics_smoke mmodern_party_smoke mmodern_indoor_map_smoke \
  mmodern_event_script_smoke mmodern_event_text_smoke \
  mmodern_game_flags_smoke mmodern_event_interpreter_smoke \
  mmodern_event_system_smoke mmodern_manual_event_smoke \
  mmodern_navigation_flow_smoke

<mmodern-build>/mmodern_party_smoke.exe <game-directory>
<mmodern-build>/mmodern_indoor_map_smoke.exe <game-directory>
<mmodern-build>/mmodern_event_script_smoke.exe <game-directory>
<mmodern-build>/mmodern_event_text_smoke.exe <game-directory>
<mmodern-build>/mmodern_game_flags_smoke.exe <game-directory>
<mmodern-build>/mmodern_event_interpreter_smoke.exe <game-directory>
<mmodern-build>/mmodern_event_system_smoke.exe <game-directory>
<mmodern-build>/mmodern_manual_event_smoke.exe <game-directory> \
  <visual-validation-directory>
<mmodern-build>/mmodern_navigation_flow_smoke.exe <game-directory>

SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  <mmodern-build>/mmodern_graphics_smoke.exe \
  <game-directory> <mode> escape
```

Run the SDL command for modes `ui`, `map`, `indoor`, `event`, `manual`,
`manual-no`, and `manual-yes`. Also run the `ui` mode with `quit` to validate
the SDL quit path.

Validation of the pinned commit completed with:

- full MMModern build: passed
- complete CTest suite: 32/32 passed
- all real-data smoke targets: built and passed
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

## Updating ScummVM

Changing the pinned ScummVM revision requires repeating the complete dependency
validation in fresh source, ScummVM build, and MMModern build directories. The
new revision may be documented only after provenance is recorded, the checkout
is clean, all four artifacts build, sprite filtering is reviewed, the full
MMModern build and CTest suite pass, and the relevant real-data and Milestone 14
runtime/manual checks pass.

ScummVM is licensed under GPLv3 or later. MMModern must remain compatible with
the license of the reused code.
