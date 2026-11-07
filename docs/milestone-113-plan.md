# Milestone 113 plan - Native Linux build

**Tier A. Status: completed and accepted by the maintainer (2026-11-06)**,
after a play-test under WSLg. Plan `537e826`, amended `885459d`; Part A
`54f9f17`, Part B `f8d3eec`, Part C `5e6441d`. Second half of
[later goal 3](roadmap.md#later-goals), after M112's Steam Deck through
Proton. No game behavior changes; save v26 unchanged, since its bytes were
already the same on both platforms.

## Scope

MMModern configures, builds, plays, saves and passes its complete suite
natively on Linux (x86-64, GCC, SDL2), and a Linux job in GitHub Actions
builds it and runs the tests that need no original data. The Windows build,
package, tests and behavior stay as they were. All platform code sits behind
one narrow layer under `src/platform/`, as the base for Linux without Steam
and a later Android port.

Out of scope by maintainer decision (2026-11-04), each for a later Tier B
milestone: a Linux launcher (game folder, Sound setup, Language), preferably
as a setup screen inside the game that can also serve Android, and a Linux
package (AppImage, Flatpak). Also out of scope: macOS, Android, other CPU
architectures, Clang, opt-in extras, the pinned ScummVM revision and the
Windows configure command.

## Key decisions

- **One platform layer.** `XeenUserFolders` gives the per-user base:
  `%LOCALAPPDATA%\MMModern` on Windows, `$XDG_DATA_HOME/MMModern` or
  `~/.local/share/MMModern` on POSIX, with the Windows layout. Every existing
  override keeps its scope. `settings.ini`, `window.ini`, the owned
  `XEEN.CFG` and log rotation keep their Windows write steps, each behind a
  platform function with a POSIX twin. One predicate classifies a full disk
  (Windows errors 112/39, POSIX `ENOSPC`/`EDQUOT`) for the four flows that
  show the original out-of-space text.
- **Save owner split, rules unchanged.** `XeenSaveFile` keeps encoding,
  validation, faults, slots and fingerprint, over a file layer whose Windows
  side moved without change. The POSIX side resolves with `realpath` and
  `lstat`, reads only regular files without blocking, serializes writers by
  `flock`, publishes by `rename` plus a directory `fsync`, and refuses saves
  on network file systems, naming `XDG_DATA_HOME` as the way out. A new
  `Publish` fault covers a failure after the new save is in place.
  Protected game and repository directories are compared by device and inode,
  not refused for being remote, so game data on a network share stays
  playable on Linux (approved by the maintainer, 2026-11-04).
- **Small fixes.** UTF-8 argv, `localtime_r`, `uname` in the diagnostics,
  POSIX path redaction, and POSIX paths kept as bytes. The `GAME1.INS` cue
  sheet's backslash `FILE` path is resolved component by component and
  case-insensitively: a real Linux defect the port found.
- **Build.** One `mmodern_platform_libs` target; on Linux the ScummVM define
  set comes from its `config.mk`, checked against
  `.github/ci/scummvm-defines-linux.txt`; on ELF `--gc-sections` replaces
  the PE section filter; Ogg and Vorbis from `pkg-config`, with ScummVM
  configured `--disable-ogg --disable-vorbis` so its `USE_` set matches
  Windows. The item-catalog generator and its test became CMake scripts, so
  the build needs no PowerShell.
- **Portable tests.** The child-process helper has a `posix_spawn` branch, a
  portable SHA-256 replaces BCrypt, and every mangled `--wrap` name is
  derived from the compiler and guarded by `--require-defined`. This also
  corrected the launcher witness's folder wrap on Windows, and Linux exposed
  a dangling reference in the M38 witness.
- **System Vorbis.** Linux decodes a few CD samples 1 LSB apart from the
  Windows static build, so `xeen_audio_golden` keeps its full-mix hash on
  Windows only; every other audio golden is exact on both (maintainer,
  2026-11-04).
- **CI and the friend's bundle.** The Linux job runs in an `ubuntu:26.04`
  container pinned by digest, with apt pinned to `snapshot.ubuntu.com` and
  exact versions, as an unprivileged user, and survives a snapshot outage
  with retries and a cache. A manual job builds a test bundle on Ubuntu
  22.04 with SDL2 2.32.10 built from the upstream tarball (maintainer,
  2026-11-04); it is not a release package.

## Results

- WSL Ubuntu 26.04 with GCC 15.2: every target builds and the complete suite
  with the original data, captures, ROMs and translation passes, 419 tests,
  without the Windows-only `xeen_launcher` and with the Linux-only
  `xeen_posix_paths_original`.
- Windows: the complete suite passes, 419 tests. The registration matches the
  416-test baseline except the added `xeen_platform`, `xeen_child_process`
  and `xeen_sha256` and the generator test's command. The M44 digest audits
  of Parts A and B found zero differences.
- Linux reaches the approved scenario digests. The cross-platform check
  writes 12 saves byte-identically on both builds, each reads the other's,
  and for the five Clouds game saves F9 after a load in the other build
  re-saves the same bytes. 401
  rendered frames are byte-identical, and the audio requests, registers,
  timing and gains are the same.
- Both CI jobs are green on `5e6441d`; a deliberately red commit on a work
  branch failed only the Linux job, and the bundle job is green.
- The WSLg play-test (title and intro sound, CD speech, keyboard, mouse,
  window and full screen, new game, save, quit and reload) passed. The
  native Linux check by a friend of the maintainer comes later and does not
  block closure; a problem found reopens the work as a maintenance task.
