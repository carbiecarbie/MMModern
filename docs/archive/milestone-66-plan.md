# Milestone 66 - Test-suite cleanup

**Tier B. Status: completed and accepted.** Plan `a5da56a`; implementation
`613a566`. The maintainer approved the coverage ledger on 2026-10-14.

## Goal and final scope

Make the complete CTest suite cheap enough to run at the end of every task in
the systems arc, without losing coverage. No gameplay, save-format or timing
change; no production code changed.

- **Real waits removed.** The M39-M43 CLI witnesses drove the SDL loop with a
  simulated clock but waited real time for presentation deadlines only that
  clock could reach. Their handlers now report the passed deadline, keeping
  the same idle sequence, clock values and assertions.
- **Shortened tests.** The casting controls are split into faults, effects and
  native tests; regional persistence keeps exhaustive truncation for each
  distinct save layout; `xeen_training_original` rasters its reset pass at one
  phase, with the other phases covered by its initial pass and the scene
  inventory test.
- **Retired and migrated programs.** Twelve programs outside CTest that failed
  on the CD data with pre-CD expectations were retired, each with replacement
  coverage named in the approved ledger. Eleven passing ones joined CTest.
- **Parallel runner.** `tools/run-full-ctest.ps1` runs half the logical
  processors (at most 8); real-window and real-clock tests run serially and
  shared paths use resource locks. Timeouts are hang guards only.

## Decisions

1. The three M44 scenarios and their witness stay untouched, as the plan
   required; the services scenario (about 99 s) is now the suite's critical
   path. The same wait fix there is an option for later.
2. `mmodern_consequence_original` keeps its impact-only CTest entry; its other
   cases stay behind their flag, covered by the consequence, resource and M44
   tests.

## Results

- Complete `build-rel` suite: 1,724 s with one job before; 183 s and 172 s
  with eight jobs after, 209 tests (was 195), twice in a row.
- `ctest -L fast`: 173 tests in about 1 minute; Debug closure check passed;
  M44 audit zero differences.
