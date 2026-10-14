# Complete CTest suite protocol

Read this before running or delegating the complete CTest suite, before
rerunning its failed tests, and when acting as its test runner. The root
`AGENTS.md` still applies.

## When

- The complete suite runs on `build-rel` once after the final production
  build of each task, not once for the whole milestone. It takes a few minutes
  (M66 target: under 10); a run over 15 minutes means something hangs or the
  machine is otherwise loaded, so check that before anything else.
- At milestone closure, also run `ctest -L fast` and the three M44 scenarios
  on `build-m44`, as a cheap check against optimization-dependent behavior.
  Run the complete suite on `build-m44` (through the runner) only before a
  public release or when a test behaves differently between the two builds.
- Do not run redundant complete suites. Run it again only when a later
  production-code change invalidates the previous result, or failures require
  it (see Failures and reruns).

## Delegation

- Delegate the complete suite to one lightweight test-runner subagent, so the
  main model does not spend its reasoning and usage allowance waiting: in Codex
  `gpt-6-luna`, low reasoning effort, `fork_turns: none`; in Claude Code Haiku,
  low effort, no conversation context.
- The runner runs `tools/run-full-ctest.ps1 -BuildDir <build>` with its default
  parallel setting (half the logical processors, at most 8 jobs; `-Jobs` only
  to reproduce a problem). CMake runs the real-clock and native-window tests
  alone (`RUN_SERIAL`), locks the tests that share a fixed path, and gives every
  test a generous hang-guard timeout (1800 s process, 300 s fast) that no
  machine load can break; a timeout is therefore a hang, never load.
- It runs on the current checkout and build directory, including uncommitted
  changes. This is an exception to the separate-worktree rule. While tests
  run, the main agent does not modify either, and launches no other subagents,
  unrelated reviews or parallel work unless the maintainer authorizes it.
- The runner waits on the process using the longest wait per call its tools
  allow (for example 300 s), not short polls. It returns only exit code,
  duration, summary, failed test names and result paths.
- The runner never diagnoses, edits or fixes anything and never starts a rerun
  on its own initiative.
- The main agent waits through the native agent-completion mechanism with the
  longest wait allowed, without reading logs, polling status or sending
  progress messages, then reads the result once.

## Failures and reruns

- If the complete suite fails only because of stale test expectations, the
  main agent investigates and fixes the tests (test code only). It then
  explicitly instructs the same runner to rerun just the failed tests
  (`ctest --test-dir <build> --rerun-failed --output-on-failure`) and runs
  `ctest -L fast` itself, instead of another complete run. Such an
  explicitly requested rerun is the only rerun the runner performs.
- Any production-code change requires another complete run.

## When delegation is unavailable

If that model, the script or delegation is unavailable, report it and leave
full-suite validation pending; never run the complete suite on the main model
instead. A passing complete suite is still required before declaring
production changes complete.

## Handoff

State how the complete suite was run: runner model and effort, the command,
and how the main agent waited (including any intermediate checks).

## M44 digest audits

Use `m44-digest-audit.ps1` before and after a change that could alter the M44
hashes. Requires Windows, Python 3 and UCRT64 `objdump` on PATH (or `-Python`
and `-Objdump`). First build the helpers with the **old source still present**:

```powershell
cmake --build build-rel --target mmodern_save_dump mmodern_m44_baseline_witness mmodern_m43_cli_witness mmodern_m44_baseline_restart
$game = 'F:/Games/gog/Might and Magic 4-5'
$preserved = 'D:/M44-audits/baseline' # new directory outside the repository
./tools/m44-digest-audit.ps1 -Step Preserve -BuildDir build-rel -PreservedDir $preserved -GameDir $game
# After changing/rebuilding the same four targets:
./tools/m44-digest-audit.ps1 -Step Compare -BuildDir build-rel -PreservedDir $preserved -GameDir $game -OutputDir build-rel/digest-comparison
```

Both destinations must be new directories; failed runs retain their evidence.
Preserve copies the four executables and recursively resolved non-system DLLs,
records their hashes and source Git state, and verifies the three header hashes.
Windows system DLLs remain OS-provided. Compare verifies the preserved binaries,
reruns old and new scenarios with the original seeds/inputs, compares full traces
and every checkpoint, and checks native final reload/re-save equality. Each
build's `mmodern_save_dump` uses its own reader, checks CRC and canonical
re-encoding, and dumps all snapshot fields. Maintain its stable field paths and
visitor when persisted fields change; no legacy reader belongs in the new build.

`digest-audit.json` (report schema 1) and `summary.txt` contain results and
candidate hashes. Byte ranges are zero-based, half-open sequence alignments;
repeated bytes can have ambiguous alignments, so ranges are evidence rather than
an automatic explanation of format changes. Common decoded fields are compared;
added/removed fields and checkpoints are reported separately. Exit 0 means the
baseline was verified or Compare found zero differences; 2 means differences
need review; 1 means the audit failed. Digests are never edited. Approval of a
replacement remains the maintainer's decision. Evidence contains original-data
save bytes: keep it local and never commit it. Test the comparison helpers with
`python -B -m unittest discover -s tools -p test_m44_digest_audit.py`; the C++ dump
self-test is also part of `ctest -L fast`.
