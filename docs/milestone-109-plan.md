# Milestone 109 plan - Automated builds and tests on GitHub

**Tier A. Status: completed and accepted by the maintainer (2026-11-03)**
after the maintainer saw the green run of `main` with its test count and the
README badge. Plan `891be9d` (amended `0494175`), Part A `903ef36`, Part B
`6389ea1`, `bf7f539`, `3ff2554`, `7b2c816` and `16e0902`, M110 merged in
`ec0fa8d`; `main` at `cf3fd07`.
[Later goal 1](roadmap.md#later-goals) of the roadmap. No game behavior
changes; the save is v26 since M110.

## Goal and scope

Every push to any branch of `carbiecarbie/MMModern`, every pull request from
a fork and a manual dispatch start a GitHub Actions run on a clean hosted
Windows machine. The run installs the documented toolchain, builds the
pinned ScummVM revision and MMModern as `build-rel` is built, and runs every
test that needs nothing outside the repository. It is the executable form of
[dependencies](dependencies.md): when the repository and that document no
longer rebuild the project, the run fails. Tests that need original data,
DOSBox captures, MT-32 ROMs, CD audio or the Russian translation stay local,
and the complete local suite stays the completion requirement of every
change. Release packaging, Linux, a different toolchain or ScummVM
configuration, and repository settings such as required checks are out of
scope.

## Key decisions

- **One switch, local default unchanged.** `MMODERN_ORIGINAL_DATA_TESTS`
  (default ON) keeps configuration, its refusals and every registered test
  exactly as before; ON was proven identical through
  `ctest --show-only=json-v1` before and after. OFF needs no data path:
  tests that need material register through `mmodern_data_test()` and their
  named property calls through `mmodern_test_properties()`, so both are left
  out together, and configuration prints both counts. Both settings build
  the same targets.
- **The data-free set is measured.** Part A ran it in a fresh OFF build with
  an empty user profile; the runner is the final authority.
- **The documented toolchain, pinned.** `windows-2025` with MSYS2 UCRT64;
  the 54 package files of the maintainer's `pacman -Q` come from the MSYS2
  archive with checksums (`.github/ci/msys2-packages.txt`), and the run
  checks that `dependencies.md` lists exactly those packages.
- **ScummVM from the pinned commit.** Fetched at the pinned commit with a
  clean status, configured before libvorbis and libogg are installed as on
  the maintainer's machine, its `USE_*` set checked against
  `.github/ci/scummvm-use.txt`, and its build cached under a key of the
  commit, the pin and `.github/ci/scummvm.sh`.
- **Built and tested as locally.** `RelWithDebInfo` with `-O2 -g`, no
  `NDEBUG`, every EXCLUDE_FROM_ALL target, the existing labels, serial
  properties and timeouts, one `ctest` without retries; a failed run uploads
  its logs for seven days. `xeen_save_state` joined the serial group, since
  its one-second restore bound failed beside other tests on the
  4-processor runner.
- **Safety.** A read-only token, no secret, actions pinned by full commit
  SHA; a newer push cancels an older run of the same branch except on
  `main`, where every commit keeps its result.

## Results

- The run is green on `m109-ci` from a cold and from a warm cache, and the
  deliberately failing commit on `m109-ci-red` turned its run red. After M110
  was merged in, `main` at `cf3fd07` registers 160 tests, leaves 253 data
  tests out and passes 160/160. Cached runs take about 20 minutes, cold ones
  about 30 to 60.
- `dependencies.md` documents the full package set, the configure order and
  the run; `development.md` names the switch; the root and tools `AGENTS.md`
  say the run never replaces the complete local suite, that a red `main` is
  a maintenance defect and how data tests register; the README shows the
  badge.
- Locally the complete suite passes on `build-rel`: 416/416 with the Russian
  translation, 413/413 without it, after the M110 merge.
- Independent reviews of the plan and both parts; their findings are
  applied.
- **Maintainer check** (2026-11-03): the green run of `main` on the Actions
  page, with its test count, and the README badge.
