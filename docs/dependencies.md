# MMModern dependencies

MMModern is an independent SDL application. It does not use ScummVM's SDL
backend or instantiate its engine. The compatibility target reuses only the
Xeen archive/sprite code, the plain Nuked OPL3 and mt32emu cores, and the
Common, Graphics, and Image libraries from a separately configured ScummVM
build. Audio does not instantiate ScummVM's mixer or OSystem; `g_system`
remains null.

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
- libvorbis 1.3.7 and libogg 1.3.6 from MSYS2 UCRT64. CMake links
  `libvorbisfile.a`, `libvorbis.a`, then `libogg.a` explicitly for CD cue
  decoding. No ScummVM audio backend is linked and `g_system` stays null.
  `MMODERN_VORBIS_INCLUDE_DIR` and the three `MMODERN_*_LIBRARY` cache
  paths select these static artifacts through `find_library`; normal toolchain
  search paths apply, with no hard-coded MSYS2 installation directory.

CD audio tests require `MMODERN_XEEN_AUDIO_DIR` (defaulting to
`MMODERN_XEEN_DATA_DIR`) with `GAME1.INS`, `GAME2.INS` and `MUSIC/`.
Configuration rejects an absent source. Validation decodes every audio track;
missing or malformed tracks fail rather than skip. Runtime still accepts a
plain CD `GAME` copy, with timed speech and visible text when tracks are absent.

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
- the mt32emu 2.7.0 core under `audio/softsynth/mt32`, listed below

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
49716 Hz, `OPL3_WriteRegBuffered` (the pin's two-sample spacing between
writes, as its `OPL::writeReg` uses; see
[original quirks](original-quirks.md#promus-write-spacing)), and
`OPL3_Generate`. No stereo-extension
define is supplied; the header's default is zero. MMModern's promus interpreter
places interrupts using the exact ratio `49716 * 16390 / 1193182`. The SDL
device opens at the default output's mix rate (`SDL_GetDefaultAudioInfo`,
48000 when unreported) with no allowed changes. `XeenAudio` mixes FM through
an `SDL_AudioStream` from 49716 Hz and unsigned mono VOC through an independent
exact rational resampler from `1000000/68` Hz; letting SDL accept 49716 made its
WASAPI path resample internally and drop ~9 ms blocks. During play the main
thread probes SDL's current default name/rate every 250 ms and reopens when
either changes, even when WASAPI followed the default internally. Removal
events for the opened output also reopen the default. Close joins the old
callback before resampler recreation; source state and VOC phase survive rate
changes. Recreating FM/CD conversion discards its small queued tail; one small
discontinuity is accepted (maintainer, 2026-10-22). Physical removal leaves
digital/CD silent until their next start while continuing the current song.
Callback/conversion errors, a bare backend disconnection and failed recovery
silence output with one notice; gameplay continues.

FM/CD conversion uses M82's 512-native-frame refills, with fixed refill
buffers and SDL conversion storage prepared before callbacks. It uses only
the public stream API; no private padding constant, history replay or whole
rate-ratio buffering is required. Fixed FM/CD gain precedes resampling and
its S16 clamp. Dependency changes must revalidate callback allocation,
effect latency, rate transitions and callback-size independence with
`sdl_audio_device`, `xeen_digital_audio` and `xeen_cd_audio`.
The maintainer's M83 Part C Windows play-test passed on 2026-10-22.

A timeline interpreter advances by an exact integer
native-sample accumulator on the presentation clock and alone answers the game;
a render interpreter in the FM source ticks from the device's own rendered
samples and receives the timeline's operations through one queue (M82 C3), so
output never waits for the game loop and never delays the game or its fade
waits.
Title/SDL and gameplay/injected clocks may have different epochs. Explicit
session handoffs consume the outgoing clock's final interval, then rebase the
source counter while retaining elapsed driver time and fractional phase;
ordinary backwards movement within a source is rejected.

The retained core is Nuked-OPL3-fast 1.8-fast.2 (upstream 1.8, cfedb09),
copyright Nuke.YKT (2013-2020) and Tony Gies (2026), licensed LGPL-2.1-or-later.
Preserve these notices and the pinned corresponding source; releases include
the pin's `LICENSES/COPYING.LGPL` alongside the GPL and runtime-library notices.
MMModern's existing ScummVM adaptations remain GPL-3.0-or-later.

### MT-32 core and player ROMs

Music device 4 uses the pinned `audio/softsynth/mt32` core directly. CMake
compiles `Analog`, `BReverbModel`, `Display`, `File`, `LA32FloatWaveGenerator`,
`LA32Ramp`, `LA32WaveGenerator`, `Part`, `Partial`, `PartialManager`, `Poly`,
`ROMInfo`, `Synth`, `Tables`, `TVA`, `TVF`, `TVP`, and `sha1/sha1.cpp` from
that directory. The sole ScummVM utility dependency is `common/util.h` for
`ARRAYSIZE`; no ScummVM MIDI backend, OSystem, configuration manager or host
MIDI device is used. The mt32emu 2.7.0 core is copyright Dean Beeler, Jerome
Fisher and Sergey V. Mikayev, LGPL-2.1-or-later. Its SHA-1 implementation is
copyright Micael Hildenborg, BSD-3-Clause; releases retain its full notice and
the LGPL license alongside the pinned corresponding source reference.
The adapter observes the pinned core's MIDI queue read cursor to account for
events actually consumed, including notes delayed by partial aborts. It uses
explicit-instantiation member access limited to the synth's queue pointer and
the queue's read cursor and mask; the public API exposes neither the cursor
nor the pending count.
The renderer uses 48 kHz accurate analogue output, the 16-bit renderer,
immediate MIDI mode and the reference capture's nice DAC, ramp, panning and
partial-mixing settings.

Players supply the MT-32 control ROM 1.07 and MT-32 PCM ROM in
`%LOCALAPPDATA%\MMModern\roms`. mt32emu identifies their contents, so file
names are unrestricted; matching split ROM halves are also accepted. MMModern
does not bundle or copy ROM files. A missing, incomplete or different set
produces one not-supported notice and silent MT-32 output without changing
logical timing. For original-data tests, set `MMODERN_MT32_ROM_DIR` to an
external ROM directory and `MMODERN_M94_CAPTURES_DIR` to the external M94
capture root; neither belongs in the repository or a release package.

The external DRO v2 validation data is configured with
`-DMMODERN_PROMUS_CAPTURES_DIR=<capture-directory>`, alongside
`MMODERN_XEEN_DATA_DIR`. Configuration does not check it, but `xeen_promus`, and
so the complete suite, needs six captures there: `promus-startup.dro`,
`promus-inn.dro` (the Tavern song), `promus-outdoors.dro`,
`promus-fx20-alone.dro`, `promus-wall-bump.dro` (effect 46) and
`promus-fx20-over-music.dro` (effect 20 over town music). Captures and derived
register streams are never bundled.
The M94 tests also require `-DMMODERN_M94_CAPTURES_DIR=<directory>`, containing
`admus`, `blastmus` and `specmus` subdirectories, each with `rawopl0001.dro`,
and `rolmus/midi0001.mid`. These captures remain external to the repository.
On the development machine the directory is
`D:/Projetos/MModern/MModern-re/m94/captures-0c`.
The comparison removes DOSBox's initial register snapshot and unchanged writes,
aligns the sustained song/effect suffix and PIT phase, and compares both banks
through capture end. The startup's EXE intro is outside M82.

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
The installed `XEEN.CFG` must sit beside the UI module, or startup fails;
MMModern reads it and never creates or writes it. `XEEN.EXE` in the same folder
is read, never executed, for the Clouds and Darkside intros and the Clouds
ending; when it is missing or different, those do not play and the game
continues.
The bounded DOS reader validates the MZ load image, relocations, named field
bounds, far-pointer tables, terminators, format arguments and control streams.
Packed root DATs and unsupported layouts fail clearly. Dialogs, item/service
screens and Rest retain the DOS bytes, including the single alignment `r` in
Charges. The title, credits, control panel and save/load chooser also read
their fields from the module.

`generated/XeenDialogDrawing.inc` retains only the pinned numeric window
symbols and four-shade text palettes. `-DialogDrawing` generates this private
build include from the verified constants blob; text is read at runtime from DAT.
The separate item catalog keeps its existing pinned provenance below. No DOS
strings or original-data fixtures are distributed.

The current save format is named by `kJourneyVersion`, `kJourneySchema` and
`kJourneyContent` in `src/formats/xeen/XeenSaveFormat.h`. Archive fingerprints hash
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

## Bounded item table provenance

`src/games/xeen/detail/XeenItemTables.h` adapts numeric tables from
`devtools/create_mm/create_xeen/constants.cpp` at pinned revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`: the weapon, armor, accessory and
misc base costs, the metal, elemental, attribute and resistance material
tables, and the weapon damage, armor class, misc spell and category tables that
combat and the item dialogs also read. `xeenItemPrice` in
`XeenItemServiceRules.cpp` prices Buy, Sell, Identify and Fix for all four
categories from them, adapting the material adjustments, action divisors,
integer division and minimum price of `ItemsDialog::calcItemCost` in
`engines/mm/xeen/dialogs/dialogs_items.cpp` at the same pin; attribute
materials follow the original program instead
([original quirks](original-quirks.md#sv-attribute-material-price)).
`XeenArmorRepair.h` applies those prices with Fix's broken-bit rule and also
carries the thirteen armor base costs; `XeenMerchantTables.h` adapts the
stock-generation tables of `Character::makeItem` at the same pin. These
adaptations are GPL-3.0-or-later, attributed to the ScummVM developers in
upstream `COPYRIGHT`, without warranty; preserve their notices and
corresponding source in redistribution. No commercial resource is embedded.
The Smith dispatch and browsing contracts are in
[M38](archive/milestone-38-plan.md) and [M42](archive/milestone-42-plan.md).

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
remain distinct. Actor admission/context/persistence contracts belong in
[M26](archive/milestone-26-plan.md#final-ownership-and-lifecycle).

## MMModern configuration and validation

Configure MMModern in its own new build directory with explicit dependency
paths:

```sh
cmake -S <mmodern-source> -B <mmodern-build> \
  -G "MSYS Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSCUMMVM_SOURCE_DIR=<scummvm-source> \
  -DSCUMMVM_BUILD_DIR=<scummvm-build> \
  -DMMODERN_XEEN_DATA_DIR=<game-directory> \
  -DMMODERN_PROMUS_CAPTURES_DIR=<capture-directory> \
  -DMMODERN_M94_CAPTURES_DIR=<m94-capture-directory> \
  -DMMODERN_MT32_ROM_DIR=<mt32-rom-directory>
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
  -DSCUMMVM_BUILD_DIR=<scummvm-build> \
  -DMMODERN_XEEN_DATA_DIR=<game-directory> \
  -DMMODERN_PROMUS_CAPTURES_DIR=<capture-directory> \
  -DMMODERN_M94_CAPTURES_DIR=<m94-capture-directory> \
  -DMMODERN_MT32_ROM_DIR=<mt32-rom-directory>
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
./tools/package-release.ps1 -BuildDir build-rel -OutputDir D:/Projetos/MModern/releases
```

The script strips debug information into a staging copy (`build-rel` keeps its
unstripped binary), copies exactly the non-system DLLs that `ldd` resolves, and
only from `C:\msys64\ucrt64\bin`, and adds `LICENSE.txt`, `README.md`,
`THIRD-PARTY.txt` (bundled libraries, the GCC runtime exception, the `v<version>`
tag and the pinned ScummVM revision), static Vorbis/Ogg notices and package
versions, license texts under `licenses\` (Vorbis COPYING is retained from
the upstream v1.3.7 release because the MSYS2 package omits it), and the
launcher: `tools/release/MMModern-launcher.ps1` with two CRLF batch files that
start it, `MMModern.bat` and `Change game folder.bat`. The launcher
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
On an mt32emu change, re-verify that queue retirement is index-based and
that abort-delayed notes stay queued.

ScummVM is licensed under GPLv3 or later. MMModern must remain compatible with
the license of the reused code.
