# Milestone 113 plan - Native Linux build

**Tier A. Status: planned (not started).**
This is the native build of [later goal 3](roadmap.md#later-goals); M112
already plays the Windows package on a Steam Deck through Proton. Baseline:
`be8837e` (release 0.17.0), equal to `origin/main`. No game behavior
changes. The save stays v26: its bytes are already the same on both
platforms, so neither the envelope version nor the Journey schema or
content changes.

## Goal and scope

MMModern configures, builds, plays, saves and passes its complete suite
natively on Linux (x86-64, GCC, SDL2), and a Linux job in GitHub Actions
builds it and runs the tests that need no original data. The Windows build,
its package, its tests and its behavior stay as they are. The port is the
foundation for Linux without Steam, Linux CI and a later Android port, so
all platform code goes behind one narrow layer under `src/platform/`, and
nothing new depends on Windows-only or GNU-only tools.

Out of scope, both by maintainer decision (2026-11-04):
- **A Linux launcher** (game folder, Sound setup, Language) comes in a
  later Tier B milestone, preferably as a setup screen inside the game that
  can also serve Android. Until then, Linux players start
  `mmodern <game-dir>`, use `--russian <folder>`, and get the sound devices
  of the installed `WORLD/XEEN.CFG` or an owned one copied in by hand.
- **A Linux package** (AppImage, Flatpak) also comes in a later Tier B
  milestone. This narrows roadmap goal 3's "native Linux build and its
  distribution"; the roadmap records it at closure. The friend's test
  bundle in Acceptance is not a package.

Also out of scope: macOS, Android, other CPU architectures, Clang,
controller support and other opt-in extras, changing the pinned ScummVM
revision, and changing the Windows configure command.

## Current state (measured 2026-11-04)

The 2026-10-17 proof (`linux-port-proof-2026-10-17.md`, outside the
repository) ran 167 of 222 tests. A new measurement on this baseline used
WSL Ubuntu 26.04 on ext4, GCC 15.2, CMake 4.2.3, Ninja and SDL2 2.32.10,
with throwaway shims in a WSL clone (`~/m113-measure`, reference only):
- **ScummVM** at the pin configures with the documented
  `--backend=sdl --disable-all-engines --disable-detection-full` and builds
  its four artifacts in about 10 s. The Vorbis and Ogg development packages
  were absent, so neither was detected, as on Windows, where ScummVM is
  configured before they are installed (see
  [dependencies](dependencies.md)). Linux adds ALSA, sndio, curl, OpenGL and
  Linux CD to the `USE_` set.
- **Production code** compiles and links with GCC 15 once eight files are
  guarded:
  - `src/main.cpp` (wide argv);
  - `src/platform/XeenSaveFile.cpp`, `XeenWonStore.cpp`,
    `XeenSystemWallClock.cpp` and `sdl/SdlWindowPlacement.cpp`;
  - `src/compat/scummvm/XeenMt32.cpp`;
  - `src/app/XeenPlayerSupport.cpp`;
  - `src/formats/xeen/XeenCdAudio.cpp`.
  
  Five of these look up Local AppData through `SHGetKnownFolderPath`, each on
  its own. `mmodern --diagnostics` runs headless and finds the CD edition.
- **Build pieces:**
  - The item-catalog generator and its test need Windows PowerShell
    (`cmake/MMModernItemCatalog.cmake`).
  - `CMakeLists.txt:28-30` requires static `libvorbisfile.a`, `libvorbis.a`
    and `libogg.a`.
  - `shell32 ole32 uuid bcrypt` are linked without condition.
  - Five places hard-code `WIN32 UNICODE _UNICODE`:
    - the ScummVM bridge objects (`cmake/ScummVmXeen.cmake`);
    - `mmodern_mt32`;
    - `mmodern_dos_export_support`;
    - `mmodern_russian_layer_data_tests`;
    - `mmodern_object_visual_smoke`.
  - The sprite and Nuked OPL3 objects are filtered by PE section names. On
    ELF the Nuked filter fails on COMDAT groups, so the measurement linked
    with `--gc-sections` instead.
- **Tests:** 415 of Windows' 416 register (`xeen_regional_action_original`
  is `if(WIN32)`). 233 pass, 6 fail and 176 do not run, because 59 test and
  tool executables do not build:
  - 42 objects include `windows.h`: 19 through the Win32 child-process
    helper `tests/XeenChildProcessTestSupport.h`, 13 through BCrypt SHA-256
    in `tests/XeenM40Evidence.h`, and 10 directly, including
    `tools/XeenDosExport.cpp`;
  - three clash with POSIX `::pause`;
  - two mix `{-1LL,...}` with `std::int64_t`;
  - one uses `_wgetenv`.
  
  The six failures:
  - `xeen_launcher` runs `powershell.exe`;
  - `xeen_sound_setup` checks the Windows path redaction;
  - `xeen_cd_audio` and `xeen_audio_golden` open `music\xeen02.ogg`. The
    `FILE` line of the original `GAME1.INS` uses a backslash, which is a
    real Linux defect;
  - `xeen_completed_restore` and `xeen_m86_presentation` never reach their
    `--wrap` seams: `XeenRestoreReplayProbe.h` and the
    `XeenAudio::stopDigital(std::uint64_t)` wrap carry MinGW manglings, `y`
    for `size_t`/`uint64_t` where LP64 Linux has `m`. Of the 26 distinct
    wrapped symbols in `CMakeLists.txt` (39 options), six mangled ones
    carry such a code, and several C-named SDL ones resolve against
    `libSDL2.so` on Linux.
- **Data-free set:** with `MMODERN_ORIGINAL_DATA_TESTS=OFF`, 160 tests
  register and 252 are left out, as on Windows.
- **Timing:** reading the original data over WSL's 9p mount was too slow in
  the proof, so data tests read a copy on ext4. The partial suite took
  217 s at 8 jobs.

## Key decisions

- **One platform layer.** Windows and POSIX code live only under
  `src/platform/` and in `src/main.cpp`.
  - A new `XeenUserFolders` owner replaces only the per-user base folder.
    - On Windows it stays `%LOCALAPPDATA%\MMModern`.
    - On POSIX it is `$XDG_DATA_HOME/MMModern`, or
      `$HOME/.local/share/MMModern` when that variable is unset or
      relative, as the XDG specification says. A missing base is created
      with mode 0700.
    - The folder keeps the Windows layout: `Saves/`, `settings.ini`,
      `XEEN.CFG`, `logs/`, `roms/` and the window placement.
  - Every existing override keeps exactly its current scope:
    - `MMODERN_PLAYER_DIR` for the owned CFG and the log;
    - `MMODERN_LOG_FILE`;
    - `MMODERN_SETTINGS_FILE`;
    - `MMODERN_MT32_ROM_DIR`;
    - the save-directory test seam.
    
    Saves, `window.ini` and the ROMs keep following the base. A Windows
    test pins each scope.
  - Windows still calls `SHGetKnownFolderPath`, so the Windows tests keep
    their wraps of it and of `windowPlacementFile`. Only the POSIX tests
    override the new owner.
- **Small-file writers keep their own semantics.** Four places write small
  files outside the save owner, and each keeps its exact Windows body:
  - `settings.ini` (`XeenWonStore.cpp`): a fixed `<file>.tmp`, an
    `ofstream` flush and `MoveFileExW`;
  - `window.ini` (`SdlWindowPlacement.cpp`);
  - the owned `XEEN.CFG` replace and its `DeleteFileW` reset
    (`XeenPlayerSupport.cpp`);
  - log rotation, a plain rename of the existing log.
  
  Each body moves behind a platform function under `src/platform/`. Its
  POSIX twin does the same steps: the same temporary name, so the tests
  that check that no `<file>.tmp` is left keep their meaning; `rename`;
  `unlink`; and a directory `fsync` where Windows writes through.
- **Disk full is classified once.** Four app flows show the original
  out-of-space text by looking for "Windows error 112/39" in the message:
  `XeenTitle.cpp:234`, `XeenGameplay.cpp:424`, `XeenInnFlow.cpp:16` and
  `XeenCreateFlow.cpp:147`. The platform layer gains one predicate,
  `xeenIsOutOfSpace(const std::exception &)`, and the four flows call it.
  - On Windows it matches errors 112 and 39 exactly as today.
  - On POSIX it matches `ENOSPC` and `EDQUOT`. Those errors keep their
    `errno` in the message, as "(POSIX error N)" beside the Windows form.
  - A POSIX short write is retried until the kernel reports an error, so a
    full disk surfaces as `ENOSPC`. The injected `ShortWrite` fault still
    throws "Short save write".
- **Save owner split, rules unchanged.** `XeenSaveFile` keeps everything
  platform-neutral:
  - the encoding before any I/O;
  - the target validation and "different game data" refusals;
  - the `Fault` operations and their order;
  - slots, `dataId` and the fingerprint.
  
  It calls a narrow file layer with a Windows implementation, moved without
  change, and a POSIX one. The POSIX one uses narrow path strings only,
  never `wstring` or `towlower`:
  - **Resolution:** the parent is resolved with `realpath`, which follows
    links as `GetFinalPathNameByHandleW` does. The leaf is checked with
    `lstat` and refused unless it is absent or a regular file. The
    `.mmsave` extension is compared case-insensitively in ASCII, as on
    Windows. The device-name and alternate-stream checks stay Windows-only.
  - **Reads** (`read`, `inspectSlot` and the validation read inside
    `write`) open with `O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC` and
    require `S_ISREG` through `fstat`. A leaf swapped for a FIFO after the
    check then cannot hang the load menu.
  - **Remote targets are refused** when the save target's directory lies
    on NFS, SMB/SMB2/CIFS, AFS, Coda, Ceph, NCP, 9P (WSL's `/mnt` drives)
    or `vboxsf`. FUSE mounts are refused too when the
    `/proc/self/mountinfo` subtype is a known network one: `sshfs`,
    `rclone`, `gvfsd-fuse`, `s3fs`, `davfs` and `curlftpfs`.
    A user base on NFS is therefore refused too, as Windows would refuse
    one. The refusal message names `XDG_DATA_HOME` as the way to put saves
    on a local disk, and the README says so (maintainer, 2026-11-04).
  - **Protected directories differ from Windows on purpose** (approved by
    the maintainer, 2026-11-04). The
    installation, its archive folders and the repository are only resolved
    and compared by `(st_dev, st_ino)` along the target's resolved
    ancestors, never refused for being remote. The comparison by identity,
    not path text, catches case-insensitive volumes and bind mounts. On
    Windows a remote protected root still fails every save; on Linux, game
    data on a network share or on `/mnt/f` stays playable.
  - **Publication:**
    - Writers are serialized by `flock` on the resolved parent directory
      plus a process mutex. They are serialized per directory instead of
      per session as with the Windows named mutex; only writers to the same
      directory can race.
    - The temporary is created with `O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC`
      and mode 0666 under the umask.
    - Writes retry on `EINTR` and continue after a partial count until the
      kernel reports an error (see disk full above). The injected
      `ShortWrite` fault still throws "Short save write", as on Windows.
    - Then come `fsync`, `close`, the re-validation and `rename`.
    - After the `rename`, the temporary is no longer the save owner's to
      clean up. The parent directory is then `fsync`ed, as
      `MOVEFILE_WRITE_THROUGH` does on Windows; `EINVAL`, `ENOTSUP` and
      `ENOSYS` mean the file system has no directory durability and count
      as success. Any other failure reports the publish as failed even
      though the complete new save is already in place, so the existing
      retry is safe.
  - **A new `Fault` operation, `Publish`, stands for a failure after the
    new save is in place.**
    - On POSIX it fires where the directory `fsync` fails. On Windows it
      fires right after `MoveFileExW`.
    - Both platforms set a `published` flag after the replacement, and the
      catch block then skips the temporary cleanup.
    - The failure throws "Publish save file" with the injected text.
    - Its test expects the new bytes at the target and no temporary left;
      the other operations keep their old-bytes check.
    
    These are the whole Windows delta of the save owner: the `Publish`
    injection, the `published` flag, and the disk-full predicate's typed
    use. The implementation review checks every other moved Windows line
    with `git diff --color-moved`.
  - **Absent and unreadable roots.** As on Windows, a protected root whose
    `stat` fails with `ENOENT` or `ENOTDIR` is skipped, and any other error
    fails the save. An unreadable `/proc/self/mountinfo` fails the save
    with its error rather than letting a possibly remote target through.
- **Small platform fixes:**
  - POSIX argv is already UTF-8.
  - `XeenSystemWallClock` uses `localtime_r` and keeps DOS `time()`
    seconds.
  - `XeenCdAudio` opens by narrow path.
  - The diagnostics report `os: Linux <release> <machine>` from `uname`
    where Windows keeps its `windows:` line. Redaction is byte-exact and
    case-sensitive on POSIX; the user base becomes `$XDG_DATA_HOME` and
    `$HOME` becomes `~`. `xeen_sound_setup` gains these cases and keeps
    the Windows ones.
  - The cue sheet's `FILE` path is split on `\` and `/` and resolved
    component by component, case-insensitively, through the installation
    detector, which already matches names that way and refuses
    ambiguity. Part A checks every other literal original-data name in
    `src/` the same way, the Russian translation loader included.
- **Build:**
  - Platform libraries become one `mmodern_platform_libs` INTERFACE target,
    empty off Windows.
  - On Linux, the five `WIN32 UNICODE _UNICODE` sites take an allowlisted
    define set extracted from the ScummVM build's `config.mk`, which a
    configure check compares with `.github/ci/scummvm-defines-linux.txt`.
    Windows keeps its defines.
  - On ELF, the sprite and Nuked objects drop the PE section filter, and
    every target that links the bridge gets `--gc-sections` through an
    INTERFACE link option, in Debug too. A future reference to an
    engine-only symbol fails to link loudly, never silently. The PE filter
    stays as it is.
  - Ogg and Vorbis come from `pkg-config` (`vorbisfile`) on Linux and stay
    static `.a` on Windows.
  - The allowlisted ScummVM define set holds the `-D` entries of
    `config.mk`'s `DEFINES` that the four artifacts were built with
    (`POSIX` and `HAVE_CONFIG_H` among them). Part A records the list in
    `.github/ci/scummvm-defines-linux.txt`, and CMake refuses any other
    entry.
  - The stripping of test and tool debug information, today
    `if(MINGW ...)`, also applies to GCC on ELF. `mmodern` and
    `MMODERN_TEST_DEBUG_INFO` keep their rules.
  - On Linux, ScummVM's configure adds `--disable-ogg --disable-vorbis`
    (maintainer, 2026-11-04). With those libraries installed it would
    otherwise detect them; the measurement had configured ScummVM before
    they were installed. The Linux `USE_` set then matches Windows' Ogg and
    Vorbis state, whatever the machine has installed.
    - `.github/ci/scummvm.sh` adds the flags only on Linux, so the Windows
      configure command and its install order stay as they are.
    - MMModern still links the system's `vorbisfile` for CD speech.
- **One item-catalog generator.** `tools/GenerateXeenItemCatalog.ps1` and
  `tests/XeenItemCatalogGenerationTests.ps1` become CMake scripts run with
  `cmake -P` on every platform, still at configure time and on every build:
  - Every `GIT_*` variable, matched without regard to case, is removed:
    the script reads the environment from `cmake -E environment` and
    passes `cmake -E env --unset` for each one. `GIT_CONFIG_NOSYSTEM=1`,
    `GIT_CONFIG_GLOBAL` set to the platform's null device,
    `--no-replace-objects` and `safe.directory` are set as today.
  - The blob, which holds NUL bytes, is written with `OUTPUT_FILE`.
    `file(SHA256)` checks it, and `file(READ HEX)` decodes it.
  - An output whose content is unchanged is left untouched, so builds do
    not recompile. A changed one is published with
    `execute_process(COMMAND ${CMAKE_COMMAND} -E rename ...)`; a failing
    `RESULT_VARIABLE` removes the temporary and fails. The CMake minimum
    stays 3.20.
  - Both outputs are pinned by SHA-256: the item catalog by today's pin,
    the dialog include by a new one taken from today's output.
  - The test keeps every refusal case. The locked-destination case relies
    on Windows share modes, so it runs only on Windows, where a small
    PowerShell helper holds the file open without delete sharing.
  
  The build then needs no PowerShell; the launcher, packaging and
  `run-full-ctest.ps1` keep it on Windows. A second, Linux-only generator
  would drift, so it was rejected.
- **Portable test infrastructure:**
  - `XeenChildProcessTestSupport.h` keeps its interface with UTF-8
    arguments. Behind it are `CreateProcessW` and a `posix_spawn` branch
    with the same exclusive log, timeout (`SIGKILL`) and creation-time
    evidence. The creation time is read from `/proc/<pid>/stat` after
    `waitid(WEXITED|WNOWAIT)` and before the child is reaped. Children inherit the test
    environment, `SDL_VIDEODRIVER` and `SDL_AUDIODRIVER` included. The Win32
    native-window input branch stays Windows-only; Part B lists any test
    that uses it and keeps that test Windows-only.
  - A small portable SHA-256 in the tests (FIPS 180-4, checked against its
    published vectors) replaces BCrypt on both platforms.
  - **`--wrap` stays**, since GNU ld and lld (Android) both support it.
    - Every mangled wrapped name is written once with ABI placeholders.
      CMake derives the `size_t` and `uint64_t` manglings from the compiler
      and passes them to the link options and to the test sources.
    - Each mangled symbol defined in the project is also given to
      `--require-defined`, so a wrong name fails the link on every platform
      instead of leaving a seam silent. C symbols of shared libraries (the
      SDL wraps) are an explicit exemption list.
    - The wraps of functions this plan changes, such as
      `createSlotDirectory` and the folder lookups, follow the new
      signatures.
    - Moving these seams into production owners was rejected: it would
      change many Tier A call sites for test convenience.
  - The `pause` helper is renamed, and the initializer lists become
    explicit `std::int64_t`.
  - **POSIX paths are bytes.** POSIX code keeps a path's bytes as they are
    and never converts them to wide strings, so a non-UTF-8 save path, XDG
    base or `--russian` folder works. A test saves and loads through such a
    directory. Any text shown to the player or written to the log passes
    the path through the existing UTF-8 diagnostics escaping.
- **Linux CI from pinned sources.**
  - A second job runs in an `ubuntu:26.04` container pinned by digest. Its
    apt sources point to `snapshot.ubuntu.com` at a fixed timestamp, and
    `.github/ci/linux-packages.txt` lists the exact package versions the
    job checks, as the MSYS2 job does.
  - ScummVM is fetched at the pin and configured by `.github/ci/scummvm.sh`
    with the Linux flags. Its `USE_` set and defines are checked
    against their Linux files, and its build is cached the same way.
  - The job builds `RelWithDebInfo` `-O2 -g` without `NDEBUG`, including
    the EXCLUDE_FROM_ALL targets, with `MMODERN_ORIGINAL_DATA_TESTS=OFF`. It
    runs one `ctest`, where the test properties already give
    `SDL_VIDEODRIVER` and `SDL_AUDIODRIVER=dummy`.
  - The permissions, pinned actions, concurrency and log upload of M109
    apply. The Windows job's steps are untouched. Its ScummVM cache key
    hashes `scummvm.sh`, so if that script changes, one cold Windows
    rebuild is expected.
- **Local Linux build.** Locally, the Linux build runs from a WSL clone on
  ext4 with the build in `build-linux-rel`. The original data, captures,
  ROMs and translation are copied to ext4. `dependencies.md` documents the
  setup.
  - Before each suite the clone is synced to the Windows working tree. It
    fetches the Windows checkout's `HEAD` and applies its
    `git diff --binary HEAD`, untracked files included.
  - `summary.txt` records the synced `HEAD` and the SHA-256 of that diff.
  - The Windows and Linux complete suites never run at the same time, since
    their real-clock serial tests cannot protect each other across
    machines.
  - `tools/run-full-ctest.sh` writes the same `summary.txt` as the
    PowerShell runner and takes an explicit `-j`; otherwise it uses half of
    `nproc` (at least 1, at most 8). Its sandbox check and job preference
    file are Windows-only.

## Parts

Each part ends with both complete suites passing, the Windows one on
`build-rel` and the Linux one on `build-linux-rel`, and with an M44 digest
audit on Windows that finds zero differences. Each part then gets an
independent implementation review and its own commit after maintainer
authorization.
- Until Part B, the targets that cannot build on Linux are registered
  `if(WIN32)` behind one named list, so the Linux suite is complete for
  what Part A delivers. Part B empties that list down to the Windows-only
  tests.
- Part A changes no Windows test source except a named list, which the
  part's report gives: the new tests and the disk-full callers.

- **Part A - build and platform layer.** Make the CMake build portable and
  move to the CMake item-catalog generator. Add `XeenUserFolders`, the
  atomic replace, the save file layer with both implementations and the
  other production fixes. Add tests:
  - POSIX save failures matching the Windows ones: every `Fault`
    operation, `Publish` included, a symlinked leaf, parent and directory
    into the game data, a FIFO leaf, a dangling link, a protected root
    reached through a second link, and an unwritable directory;
  - the remote classifier, tested as a unit on `statfs` types and
    mountinfo lines;
  - the override scopes and each XDG case;
  - the backslash cue path;
  - the generator's byte identity.
  
  An unprivileged run cannot create a case-insensitive volume, so that case
  is covered only on Windows; on Linux the link alias stands for it. On
  Linux every production target builds, and every test that builds passes.
- **Part B - portable test infrastructure.** Port the child-process helper,
  SHA-256, every `--wrap` name and their guard, and the remaining test
  files and tools that include `windows.h`:
  - `XeenSaveFileTests`, where junctions and `subst` become their POSIX
    cases;
  - `XeenPlayerLogTests`, `XeenTitleFlowTests` and the ending tests;
  - `mmodern_test_save` and the DOS export tools.
  
  Register `xeen_regional_action_original` on Linux unless it proves
  Windows-bound. Result: Linux registers every test except a named
  Windows-only list, `xeen_launcher` at least, and all of them pass.
- **Part C - CI and documentation:**
  - the Linux job, its package, `USE_` and define files, and a deliberately
    red commit on a work branch, as M109 did;
  - `tools/run-full-ctest.sh`;
  - `dependencies.md`: the Linux toolchain, packages, ScummVM configure
    flags and WSL setup;
  - `development.md` and the README: build and run on Linux, and where saves
    and settings live;
  - `tools/AGENTS.md`: both complete suites;
  - the cross-platform save check;
  - preparing the play-test and the friend's bundle (see Acceptance);
  - amending `docs/playtests.md` for WSL sessions.

## Risks

- **Silent behavior drift between platforms.** This means `long` versus
  `long long`, the 32-bit Linux `wchar_t`, unordered container order and
  floating point.
  - The four original-data scenarios must reach the same approved digests
    on Linux; no new digest is approved for Linux.
  - In Part C a scripted check writes a save on each build and loads it on
    the other. It requires equal `mmodern_save_dump` output and
    byte-identical re-saves. The evidence stays local, because the saves
    come from the original data.
- **The save owner's new branch is Tier A I/O.** Windows code is moved
  without change, and the M44 digest audit and the Windows failure tests
  prove it. The POSIX semantics above are deliberate, including the
  protected-root difference from Windows.
- **WSL is not a real distribution.** The CI container covers a clean
  machine. Case sensitivity is covered by a Linux data test on a copy of
  the CD files renamed to lower case. WSLg relays the display over RDP and
  the audio through PulseAudio, so it cannot settle input feel or native
  audio. The friend's native check covers that, after closure (see
  Acceptance).
- **`--require-defined` and `--wrap` interaction.** Part B checks a
  deliberately misspelled name on both linkers. If the guard redirects
  through `--wrap`, the guard instead checks the linker's `--trace-symbol`
  output.
- **Prerequisite, done.** The maintainer installed `libvorbis-dev` and
  `libogg-dev` in WSL on 2026-11-04.

## Acceptance

- On WSL Ubuntu 26.04 with GCC 15.2, configuration needs no PowerShell,
  every target builds, and the Linux complete suite with the original data,
  captures, ROMs and translation passes, apart from the named Windows-only
  tests.
- On Windows:
  - the complete suite passes with every baseline test, now 416, plus the
    added tests;
  - the `ctest --show-only=json-v1` registration matches the baseline in
    names, labels and properties, except for the added test names, listed
    at closure, and the generator test's command;
  - the M44 digest audits of Parts A and B find zero differences;
  - both CI jobs are green on the closing commit.
- Linux reaches the same approved scenario digests, and the cross-platform
  save check passes.
- The documentation above is updated, and `project-status.md` is updated at
  closure.
- **Maintainer play-test under WSLg** (maintainer, 2026-11-04). Before it,
  the agent compares frames and audio with Windows output. The guide marks
  each item. Items marked **[friend]** are items where WSLg can mislead,
  because it relays the display over RDP and the audio through PulseAudio:
  - [friend] the title, intro sound and music;
  - [friend] CD speech;
  - [friend] keyboard and mouse feel, and the window and full screen;
  - a new game, a save, a quit and a reload.

  The session follows `docs/playtests.md`, which Part C amends for WSL:
  - The `.cmd` launchers call `wsl.exe` with the Linux build.
  - Presets and working copies live on ext4 in WSL, because `/mnt/d` is
    9P and is refused as a save target.
  - The presets come from the Linux build's own `mmodern_test_save`.
- **Native Linux check by a friend of the maintainer, not blocking.** The
  [friend] items also go to a friend of the maintainer who owns the CD
  edition of World of Xeen and plays on native Linux.
  - No original data and no saves are sent, so the friend's guide starts
    from a new game.
  - The guide is a short note of its own, outside `docs/playtests.md`.
  - Part C prepares a test bundle for this, outside the repository and not
    a release package. It is built by a manual CI job as a downloadable
    artifact, in an older pinned container (Ubuntu 22.04, glibc 2.35), so
    it runs on most current distributions. The bundle holds:
    - `mmodern` linked with `-static-libgcc -static-libstdc++`, as on
      Windows;
    - its non-system libraries;
    - a start script;
    - the GPL notice and the source commit;
    - the guide.
  - M113 closes on the WSLg play-test. The friend's results come later. A
    problem they find reopens the work as a maintenance task, or as a
    milestone if it is larger.
