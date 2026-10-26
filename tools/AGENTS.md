# Complete CTest suite protocol

Read this before running or delegating the complete CTest suite, before
rerunning its failed tests, and when acting as its test runner. The root
`AGENTS.md` still applies.

## When

- The complete suite runs on `build-rel` once after the final production
  build of each task, not once for the whole milestone. It takes a few minutes
  (M66 target: under 10); a run over 15 minutes means something hangs or the
  machine is otherwise loaded, so check that before anything else.
- `build-rel` is also the build that releases package and the maintainer
  play-tests, so no separate Debug (`build-m44`) run is required at milestone
  closure or before a release (maintainer, 2026-10-16). Use `build-m44` only to
  debug, for example when a test behaves differently between the two builds.
- Do not run redundant complete suites. Run it again only when a later
  production-code change invalidates the previous result, or failures require
  it (see Failures and reruns).
- Batch fixes: collect the review and play-test fixes of one round and run one
  complete suite for the whole batch, not one per fix. Each fix still gets its
  affected tests and `ctest -L fast` while iterating; no fix of the batch is
  complete until the batch suite passes.

## How it runs

- The main model never spends reasoning or usage allowance waiting for the
  suite. In Codex, delegate it to one lightweight test-runner subagent
  (`gpt-6-luna`, low reasoning effort, `fork_turns: none`). In Claude Code, the
  main agent starts the script itself as a background shell command
  (`run_in_background`) and is notified when it exits; no subagent is needed.
- The suite runs `tools/run-full-ctest.ps1 -BuildDir <build>` with its default
  parallel setting (half the logical processors, at most 8 jobs; `-Jobs` only
  to reproduce a problem). CMake runs the real-clock and native-window tests
  alone (`RUN_SERIAL`), locks the tests that share a fixed path, and gives every
  test a generous hang-guard timeout (1800 s process, 300 s fast) that no
  machine load can break; a timeout is therefore a hang, never load.
- The complete suite runs outside any agent sandbox: in Codex, request
  escalation once for the validation phase and give it to the runner. The
  script refuses to start when `TEMP` points into an app sandbox and puts the
  configured compiler's directory on `PATH` itself. A sandboxed run is
  invalid, not a test result: process tests fail with Windows
  error 5 (save directory, save publication, junction creation), temporary
  paths point into `AppData\Local\Packages\sandbox.{...}\AC\Temp`, and
  `xeen_item_catalog_generation` hangs to its 300 s timeout. Do not diagnose or
  change code for such failures; rerun the complete suite unsandboxed.
- It runs on the current checkout and build directory, including uncommitted
  changes. This is an exception to the separate-worktree rule. While tests
  run, the main agent does not modify either, and launches no other subagents,
  unrelated reviews or parallel work unless the maintainer authorizes it.
- The Codex runner waits on the process using the longest wait per call its
  tools allow (for example 300 s), not short polls. It returns only exit code,
  duration, summary, failed test names and result paths. It never diagnoses,
  edits or fixes anything and never starts a rerun on its own initiative.
- The main agent waits through the native completion notification (the
  runner's completion in Codex, the background command's exit in Claude Code)
  without reading logs, polling status or sending progress messages, then
  reads `summary.txt` once.

## Failures and reruns

- If the complete suite fails only because of stale test expectations, the
  main agent investigates and fixes the tests (test code only). It then
  reruns just the failed tests
  (`ctest --test-dir <build> --rerun-failed --output-on-failure`; in Codex by
  explicitly instructing the same runner, in Claude Code as a background
  command) and runs `ctest -L fast` itself, instead of another complete run.
  Such an explicitly requested rerun is the only rerun the Codex runner
  performs.
- Any production-code change requires another complete run.
- A passing process test deletes its working directory; a failing one keeps
  it for diagnosis, under the build directory or `%TEMP%` and named after the
  test (for example `mmodern-m38-process-*` or `m42-process-*`).

## When the runner is unavailable

If the Codex runner model, delegation, background commands or the script are
unavailable, report it and leave full-suite validation pending; never make the
main model wait on the complete suite in the foreground instead. A passing
complete suite is still required before declaring production changes
complete.

## Handoff

State how the complete suite was run: runner model and effort or background
command, the command, and how the main agent waited (including any
intermediate checks).

## M44 digest audits

Use `m44-digest-audit.ps1` before and after a change that could alter the M44
hashes. Requires Windows, Python 3 and UCRT64 `objdump` on PATH (or `-Python`
and `-Objdump`). First build the helpers with the **old source still present**:

```powershell
cmake --build build-rel --target mmodern_save_dump mmodern_m44_baseline_witness mmodern_m43_cli_witness mmodern_m44_baseline_restart
$game = 'F:/Games/gog/Might and Magic 4-5'
$preserved = 'D:/Projetos/MModern/M44-audits/baseline' # new directory outside the repository
./tools/m44-digest-audit.ps1 -Step Preserve -BuildDir build-rel -PreservedDir $preserved -GameDir $game
# After changing/rebuilding the same four targets:
./tools/m44-digest-audit.ps1 -Step Compare -BuildDir build-rel -PreservedDir $preserved -GameDir $game -OutputDir build-rel/digest-comparison
```

Both destinations must be new directories; failed runs retain their evidence.
Preserve copies the four executables and recursively resolved non-system DLLs,
records their hashes and source Git state, and verifies every recorded header
hash; a scenario whose digest still awaits approval is generated but not verified.
Windows system DLLs remain OS-provided. Compare verifies the preserved binaries,
reruns the scenarios the preserved build knows with the original seeds/inputs
(a newer scenario is reported with its candidate digest, not compared), compares full traces
and every checkpoint, and checks native final reload/re-save equality. Each
build's `mmodern_save_dump` uses its own reader, checks CRC and canonical
re-encoding, and dumps all snapshot fields. Maintain its stable field paths and
visitor when persisted fields change; no legacy reader belongs in the new build.
Since M69 it also writes a per-map view (`maps[side:number]...`, marked by
`view.perMap`) keyed by map identity and record, independent of the save
layout; the audit compares it only when both builds write it. The roundtrip
scenario also resumes from checkpoints A and B in `resume-A/` and `resume-B/`
(outside the checkpoint glob) and requires both finals to equal the
uninterrupted one.

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
