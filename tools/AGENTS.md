# Complete CTest suite protocol

Read this before running or delegating the complete CTest suite, before
rerunning its failed tests, and when acting as its test runner. The root
`AGENTS.md` still applies.

## When

- The complete suite runs once per required configuration after the relevant
  final changes, not once for the whole milestone: on `build-rel` after the
  final production build and, at milestone closure, also on `build-m44` as a
  check against optimization-dependent behavior.
- Do not run redundant complete suites. Run it again only when a later
  production-code change invalidates the previous result, or failures require
  it (see Failures and reruns).

## Delegation

- Delegate the complete suite to one lightweight test-runner subagent, so the
  main model does not spend its reasoning and usage allowance waiting: in Codex
  `gpt-6-luna`, low reasoning effort, `fork_turns: none`; in Claude Code Haiku,
  low effort, no conversation context.
- The runner runs `tools/run-full-ctest.ps1 -BuildDir <build>` with its default
  single job. Do not pass `-Jobs`: process tests have timeouts that parallel
  load breaks.
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
  (`ctest --test-dir <build> --rerun-failed --output-on-failure`, single job)
  and runs `ctest -L fast` itself, instead of another complete run. Such an
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
