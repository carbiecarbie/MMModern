# Milestone 66 - Test-suite cleanup

**Tier B. Status: approved; implementation not started.**

## Goal

Make the complete CTest suite cheap enough to run at the end of every task in
the systems arc, without losing coverage. Target: the complete `build-rel`
suite in under 10 minutes on the maintainer's machine (today about 29), with
`ctest -L fast` still under 3 minutes. No gameplay, save-format or timing
change.

## Where the time goes

The last complete run (M65, 195 tests, 1,723 s of test time) spent 86% in 15
tests over 20 s. Four process tests that drive the old M38-M43 CLI witnesses
took over half of it: `xeen_service_day_process` (390 s), `xeen_combat_clock_process`
(195 s), `xeen_training_process` (192 s) and `xeen_purchase_process` (128 s).
Then come `xeen_smith_process`, `xeen_temple_depleted_process`,
`xeen_temple_process`, `xeen_training_original`, the three M44 scenarios,
`xeen_combat_casting_controls` and `xeen_input_scheduling`. These witnesses
also break on legitimate changes: M63 Part B had to adapt six of them.

## Tasks

1. **Measure first.** For each test over 20 s, record where its time goes:
   process launches and CD loading, real-time waits, repeated validation, or
   the scenario itself. Record what each one asserts that no other test does.
2. **Retire or shrink the M38-M43 witnesses.** For each assertion, keep it
   only if no unit test, original-data test or M44 scenario already covers it;
   move what remains into a faster test (in-process, fewer launches, shorter
   routes). Every retirement cites the tests that cover it instead.
3. **Simulated time.** Replace real-time waits in tests with the M63
   presentation clock where the code allows injected time.
4. **Parallel runs.** Make timeouts independent of machine load, then let the
   runner use several jobs for tests that can share the machine; keep a
   serial group for the ones that cannot. Update `tools/AGENTS.md` and
   `tools/run-full-ctest.ps1` accordingly.
5. **Backlog.** Decide each excluded or broken witness (SaveResume, Phirna,
   Remove, WhoWill, IndoorObject integration tests, the legacy M35 probes):
   migrate it into CTest, or retire it with its replacement coverage cited.
   Check the 312 never-run cases in `mmodern_consequence_original` and the
   quadratic loops in the save-format tests.
6. **Shared loading.** Where many cases in one test reload the same CD data,
   load it once per process, if measurement shows it matters.

## Boundaries

- Test code, test tooling and build files only. A production change is
  allowed only for a measured per-frame cost that tests hit hard, must be
  behavior-identical, and needs the M44 audit at zero differences.
- The three M44 scenarios and their digests stay as they are. A fourth
  scenario for the systems arc is planned with that arc, not here.
- No assertion disappears without its replacement coverage named in the
  report.

## Acceptance

- A coverage ledger in the implementation report: every retired, merged or
  shortened test with the tests that now cover each of its assertions. The
  maintainer approves it before the retirements are committed.
- The complete `build-rel` suite passes through the runner in under 10
  minutes (or the report explains the measured floor), and passes twice in a
  row with the new parallel setting; `ctest -L fast` passes; the Debug closure
  check passes.
- `tools/AGENTS.md`, `docs/dependencies.md` and the README test section match
  the new runner settings.
