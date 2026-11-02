# Milestone 109 plan - Automated builds and tests on GitHub

**Tier A. Status: planned (not started).**
[Later goal 1](roadmap.md#later-goals) of the roadmap. Baseline: `58352f9`,
equal to `origin/main`. No game behavior changes; save v25, Journey schema
and content unchanged.

## Goal and scope

Every push to any branch of `carbiecarbie/MMModern`, and every pull request
from a fork, starts a GitHub Actions run on a clean hosted Windows machine.
The run installs the toolchain of [dependencies](dependencies.md), checks out
the pinned ScummVM revision, builds its four artifacts, builds MMModern as
`build-rel` is built, and runs every CTest test that needs no material from
outside the repository. The run is the executable form of the dependencies
document: when the repository and that document no longer rebuild the
project, the run fails.

Out of scope:
- tests that need the original data, DOSBox captures, MT-32 ROMs or the CD
  audio tracks; they stay local, and the complete local suite stays the
  completion requirement of every change;
- release packaging and publishing binaries (`tools/package-release.ps1`
  stays local);
- the Linux and Steam Deck build ([later goal 3](roadmap.md#later-goals));
- changing the pinned ScummVM revision, its configure command or the
  documented toolchain;
- GitHub repository settings such as branch protection or required checks;
  the maintainer decides those after closure.

## Current state

At `58352f9`:
- `.github/` holds only the issue template (M100). Actions are enabled and
  the repository is public, so hosted runners cost nothing.
- `include(CTest)` (`CMakeLists.txt:218`) turns `BUILD_TESTING` on, and
  configuration then refuses to start without the CD audio files
  (`:232-234`) or `MMODERN_XEEN_DATA_DIR` (`:273-275`). The guard is
  deliberate: a forgotten path fails instead of silently dropping tests
  ([dependencies](dependencies.md#read-only-cd-data-and-dos-dialogs)).
- `build-rel` registers 411 tests; 158 of them have no data, capture, ROM
  or ScummVM path in their command or environment. Some of those may still
  reach original data another way (the `MMODERN_TEST_UI_DATA` definition at
  `:279`, GOG registry lookups, `%LOCALAPPDATA%\MMModern`). Part A settles
  the real set; 158 is only a first estimate.
- Several blocks name tests that need data next to tests that do not: the
  process-label list and its existence check (`:1448-1452`), the RUN_SERIAL,
  COST and RESOURCE_LOCK lists (`:1468-1497`), mixed `set_tests_properties`
  pairs such as `:725` and `:1176`, and the `coverage_current` fixture, whose
  data-free `xeen_coverage_calibration` needs the data test
  `xeen_coverage_original_guard` (`:244-245`).
- No data-free test uses a real device: the SDL tests set
  `SDL_VIDEODRIVER=dummy`, `sdl_audio_device` wraps the audio device, and
  `:1522-1527` gives every test `SDL_AUDIODRIVER=dummy`. The three tests
  that open a real window (`xeen_input_scheduling`, `xeen_main_screen`,
  `xeen_native_responsiveness`) need data.
- The documented toolchain matches the maintainer's `pacman -Q`. Undocumented
  but relevant: the msys `make` 4.4.1-3 that `make -j4` runs, CMake's
  package release (4.4.2-2), Ninja 1.13.2-1, binutils 2.47-3 (its `objcopy`
  does the [sprite filtering](dependencies.md#revision-sensitive-sprite-filtering))
  and the `crt`, `headers` and `winpthreads` packages that GCC 16.2.0-3 is
  built against.
- The pinned ScummVM build's `config.h` defines `USE_LIBCURL`, `USE_CURL`,
  `USE_HTTP` and `USE_CLOUD`: `configure` picks up whatever libraries are
  installed, and MMModern compiles ScummVM headers against that `config.h`.
- The item catalog generator needs Git for Windows (`find_package(Git)` in
  `cmake/MMModernItemCatalog.cmake:20`; Windows paths and
  `GIT_CONFIG_GLOBAL=NUL` in `tools/GenerateXeenItemCatalog.ps1:41-47`) and
  reads only `rev-parse HEAD`, `ls-tree` and `cat-file blob` (`:267-273`).
- Foreign MinGW DLLs earlier on `PATH` crash the tests at load
  (`tools/run-full-ctest.ps1:62-68`); the hosted image ships `C:\mingw64`
  and Strawberry Perl's MinGW.
- Four targets are EXCLUDE_FROM_ALL: `mmodern_test_save`,
  `mmodern_dos_export`, `mmodern_m44_baseline_witness` and
  `mmodern_m44_baseline_restart`.

## Design decisions

1. **One explicit switch; the local default is unchanged.** A new cache
   option `MMODERN_ORIGINAL_DATA_TESTS`, ON by default. ON keeps
   configuration, its refusals and every registered test exactly as today.
   OFF needs no data path and registers only the tests that need nothing
   outside the repository, the build tree, the pinned ScummVM checkout and
   temporary folders. Data tests are registered through one wrapper (for
   example `mmodern_data_test()`) that records their names; under OFF it
   registers nothing, and every named list above (process labels and their
   check, RUN_SERIAL, COST, RESOURCE_LOCK, mixed property pairs, the
   coverage fixture) is filtered to the registered tests. The filtering
   happens only under OFF, so ON is untouched. Configuration prints how many
   tests it registered and how many the wrapper left out. An empty data path
   with the option ON still fails, as now. Both settings build the same
   targets, so the run proves the whole build, not only the tests it runs.
2. **The data-free set is measured, not guessed.** Part A configures OFF in
   a fresh build directory with no data paths and runs the candidates with
   `LOCALAPPDATA`, `APPDATA`, `USERPROFILE` and `TEMP` pointing at empty
   folders. A candidate that fails because it found no data joins the data
   set. The runner, which has no data and no GOG registry entries, is the
   final authority. A failure there that comes from the runner's paths (8.3
   short `TEMP`, another user or drive) is a defect to fix, not a reason to
   leave a test out. Tests get no new skip logic, because tests never skip
   silently ([dependencies](dependencies.md#read-only-cd-data-and-dos-dialogs));
   a test that mixes data-free and data cases stays in the data set unless
   an existing command-line argument already separates them. Each left-out
   group (original data, captures, ROMs, CD audio) is named in one comment
   at the wrapper.
3. **Hosted Windows runner with MSYS2 UCRT64.** A named image
   (`windows-2025`) rather than `windows-latest`, so the image changes only
   on purpose. `msys2/setup-msys2` provides the shell with `update: false`
   and `path-type: minimal`, and CMake gets `-DGIT_EXECUTABLE` pointing at
   the image's Git for Windows. Before `ctest` the run logs which
   `libstdc++-6.dll` and `SDL2.dll` are found first.
4. **The documented toolchain, pinned** (maintainer, 2026-11-02). The run
   installs the whole UCRT64 package set the build uses, plus the msys
   `make`, at the name, version and release of the maintainer's
   `pacman -Q`, from the MSYS2 package archive with checksums; the
   downloaded files are cached under their checksums. It records
   `msys2-runtime` and prints each tool's version. This includes curl and
   whatever else ScummVM's `configure` detects. The full set, with
   binutils, Ninja and the msys `make`, goes into `dependencies.md`. A rolling
   install would only prove that today's MSYS2 builds the project; the pin
   proves that the documented one does. The cost: a toolchain update changes
   `dependencies.md` and the workflow together, and the run fails until both
   agree. If the pinned packages cannot be installed together on a clean
   machine, or the archive no longer serves one of them, Part B stops and
   reports; the maintainer chooses between updating the documented set and
   a rolling install.
5. **ScummVM from the pinned commit, cached.** The run fetches commit
   `6814ee9b...` at depth 1 without a blob filter, so its whole tree is
   present for the item catalog generator. It checks the commit out
   detached, requires an empty `git status --porcelain`, and configures and
   builds the four artifacts exactly as documented. After `configure` it
   compares the `USE_*` lines of `config.h` and `config.mk` with a set
   recorded from the maintainer's build and fails on any difference. The
   ScummVM build directory is cached under a key of the commit and the
   toolchain pin. A cache hit skips only the ScummVM build; the checkout and
   its checks always run.
6. **Built as `build-rel`.** Ninja, `RelWithDebInfo` with `-O2 -g`, a check
   that `build.ninja` holds no `NDEBUG`, and every EXCLUDE_FROM_ALL target,
   so every documented target compiles. The run logs free disk space before
   and after the build.
7. **Tests run once, as locally.** `ctest` with the existing labels, serial
   properties and timeouts and `--output-on-failure`, at the runner's
   processor count. No retries: a failure on the runner is a defect to
   explain, as it is locally. A failed run uploads the CTest logs, which
   contain no original data, for seven days.
8. **Triggers and safety.** `push` on every branch, `pull_request` only for
   pull requests from forks (a branch of this repository already runs on
   push), and manual dispatch. A newer push cancels the older run of the
   same branch, except on `main`, where every commit keeps its own result.
   The token is read-only (`contents: read`), no secret is used, and each
   third-party action is pinned by full commit SHA with its version in a
   comment. Caches made on other branches are not visible to `main`, so its
   first run is cold.
9. **Documentation.** `dependencies.md` gains a continuous-integration
   section: what the run does, the switch, the recorded `USE_*` set, and
   that the workflow and the document change together. It also documents the
   full pinned package set. `development.md`'s Tests section names the
   switch. `tools/AGENTS.md` says the run never replaces the complete local
   suite and that a red run on `main` is a defect handled as a maintenance
   task. The root `AGENTS.md` gets one line to the same effect (it is at 229
   of 240 lines), and `README.md` gets the run's status badge.
10. **Pushes during implementation.** The workflow can only be proven on
    GitHub. Part B works on branch `m109-ci`; when starting the milestone the
    maintainer is asked to authorize pushes of that branch and of a
    throwaway branch `m109-ci-red`, never a force-push. `m109-ci-red` holds
    only the deliberately failing commit, is never merged, and is deleted
    remotely only with the maintainer's OK. `main` receives `m109-ci` only
    with the maintainer's own authorization at closure.

Tier A because it changes the configuration that guards the original-data
suite and rebuilds the ScummVM integration from scratch: a mistake would
drop local tests without any failure. Plan and implementation get the
mandatory independent review.

## Risks

- **Silent loss of local tests.** Part A compares the `tests` array of
  `ctest --show-only=json-v1` from a fresh ON configuration before and after
  the change. It uses the same build-directory path and cache both times,
  removes the `backtrace` fields (CMake line numbers move), and compares
  name, command and every property, ENVIRONMENT order included, both in
  registration order and sorted by name. Both must be identical, and the
  complete local suite must pass.
- **A data-free test reaching data another way.** Decision 2's redirected
  local run and the runner itself catch it.
- **A different ScummVM `config.h` on a clean machine.** The `USE_*` check
  of decision 5 catches it. Changing the documented configure command
  instead, for example with explicit `--disable-...` flags, is out of scope.
- **Real-clock tests on a slower 4-processor runner.** A test that cannot
  pass reliably there is investigated; leaving it out of the run needs the
  maintainer's decision.
- **Pinned packages that no longer install together, or leave the archive**
  (decision 4).
- **Run time and disk.** Cold and cached times and free disk are measured
  and reported. If the cached run exceeds 45 minutes, the implementer
  reports before adding a compiler cache or other speedups.

## Parts

- **Part A - the switch (local).** The wrapper and filtered lists, the
  classification, the ON identity check, an OFF configuration in a fresh
  build directory with no data paths and redirected user folders that builds
  and passes, and the complete local suite on `build-rel`.
- **Part B - the workflow.** The workflow on `m109-ci`, green from a cold
  and from a warm cache; one deliberately failing data-free test on
  `m109-ci-red` turns its run red; the documentation.

One maintainer check after Part B.

## Acceptance

- With the option ON, the registered tests are identical before and after
  by the comparison under Risks, and the complete local suite passes on
  `build-rel`.
- A fresh clone, configured OFF by following `dependencies.md` alone and
  without data, builds and passes every registered test locally with
  redirected user folders.
- The run on `m109-ci` is green from a cold and from a warm cache. Its log
  shows the pinned package versions, the ScummVM HEAD equal to the pinned
  commit with a clean status, the expected `USE_*` set, no `NDEBUG`, the
  DLLs found first on `PATH`, and the counts of tests run and left out. The
  run on `m109-ci-red` is red.
- The report lists the tests left out by group and reason, the measured run
  times and the free disk.
- **Maintainer check** (nothing the player sees changes, so no game
  play-test): after the authorized push to `main`, the maintainer opens the
  repository's Actions page, sees the green run of `main` with its test
  count, and sees the badge in the README.
