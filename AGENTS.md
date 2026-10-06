# MMModern - Agent Instructions

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

**The goal is a faithful reimplementation**: the game must behave, look and
feel like the original - rules, formulas, timing, input, interface, text and
presentation - using the original resources and the pinned ScummVM reference
as the authority. Within that goal, judge every task by whether more of the
original game becomes continuously playable, or whether the next piece of
content becomes cheaper to add.

Deviations from the original are allowed only when the maintainer explicitly
approves them, and must be documented where they live. Quality-of-life options
may come later, but only as opt-in additions that leave faithful behavior as
the default; they are not part of current work.

## Source of truth

- Repository: https://github.com/carbiecarbie/MMModern
- Actual Git state, the working tree and current code/tests establish implemented
  behavior. Inspect them before changing interfaces or relying on prose.
- `docs/roadmap.md` sets direction; the active milestone plan sets scope. Older
  plans and history are background, not current authority.
- Agents without local access must use a verified commit SHA as their baseline
  and report when it cannot be retrieved; do not invent local state.

## Development principles

- **Fidelity first.** When a plan, prompt or code is unclear about a
  behavior, reproduce the original from the pinned ScummVM reference without
  asking. Ask the maintainer only when the original itself is ambiguous or
  reproducing it would leave the milestone scope. Never invent behavior the
  original does not have.
- **Condition counter edge cases follow ScummVM.** For status-effect
  (condition) counters only, when the original's behavior cannot be observed
  in normal play and could only be checked by inspecting or editing DOSBox
  memory (for example a byte 0xFF read as the -1 sentinel, or values
  unreachable in ordinary play), follow the pinned ScummVM reference without
  asking and mark the site "follows ScummVM; not confirmed in DOS". Evidence
  from normal DOSBox play still overrides ScummVM. Any other unverifiable
  case goes to the maintainer.
- **Playable by default.** Load whole original maps, actors and events from the
  original resources. When play reaches something not yet implemented (an Event
  opcode, monster ability, service, spell or item effect), show a clear
  "not supported yet" notice and keep the game running when that is safe.
  Do not certify content cell by cell or gate areas behind per-route manifests.
- **Implement mechanics generically.** Follow the original semantics from the
  pinned ScummVM reference and implement each mechanic once, for every place the
  original game uses it, instead of for one selected address or witness.
- **Reuse existing owners.** Preserve the party/roster, world/actor, combat,
  Event, Service and Flow ownership already in place. Introduce a new owner or
  coordinator only for a concrete reason, and say why.
- **Faithful UI.** Use the original interface: main screen buttons, portraits
  and original dialogs, operable by mouse and keyboard. Avoid project-specific
  menus except as a temporary stopgap.
- Never modify or include original commercial Might and Magic game data.

## Save compatibility (pre-release policy)

Until a public release is declared, there is **one current save format**.
- Saves from older builds may be rejected with a clear message; do not keep
  legacy readers, frozen per-milestone behavior or content/contract numbers.
- Change the format version whenever its layout or meaning changes.
- Restoring must still be exact for the current format: no replayed events,
  rewards, time or RNG. A generic save/load round-trip test covers this.

## Milestone workflow

Each milestone is classified in its plan as Tier A or Tier B.

| | Tier A - core | Tier B - content, UI, services |
| --- | --- | --- |
| Applies to | Save format, RNG, timing/scheduling, ownership/architecture, ScummVM integration | Areas, Events, monsters, items, spells, services, UI, presentation |
| Plan | Short plan (~100-150 lines): goal, design decisions, risks, acceptance | Checklist: goal, tasks, acceptance criteria (~30-60 lines) |
| Review | Independent review of the plan and of the implementation | No mandatory independent review; request one if risk appears |
| Acceptance | Tests plus maintainer play-test | Tests plus maintainer play-test |

Steps:

1. Read `docs/project-status.md` and the milestone plan; inspect relevant code/tests.
2. Implement within the milestone scope. Planning and implementation may be
   one task for Tier B. If the scope proves wrong, stop and say so instead of
   silently widening or narrowing it.
3. Add or update tests for changed behavior: unit tests for rules, and a few
   original-data end-to-end tests where they prove real gameplay. Do not add a
   separate process-continuation witness per feature.
4. Build and run the complete CTest suite before declaring completion. Do not
   declare completion with failing tests.
5. The maintainer plays the result and accepts it. Record acceptance briefly.

Roadmap approval does not authorize implementation; the maintainer starts each
milestone explicitly.

**Maintenance tasks.** Small fixes (a defect, a flaky test, a doc correction)
may be done without a milestone plan: a scoped prompt, tests for the fix, the
complete CTest suite if production code changes, and an independent review
only when the fix touches Tier A areas. If a fix turns out to be larger or
recurring, stop and propose a milestone instead.

## Documentation

| Document | Responsibility | Target size |
| --- | --- | --- |
| `README.md` | Public introduction, how to build, run and play | Short |
| `docs/project-status.md` | What can be played and done now; architecture overview; known gaps | ~200-300 lines |
| `docs/project-history.md` | One short paragraph per completed milestone | Short |
| `docs/roadmap.md` | Next milestones and longer-term direction | ~100-150 lines |
| `docs/milestone-N-plan.md` | That milestone's plan; condensed at closure | Closed: ~30-100 lines |
| `docs/archive/` | Plans of milestones 6 and 15-43, kept as a historical record | Exempt |
| `AGENTS.md` | These rules | This file |

- Describe capabilities in player terms ("the Temple heals and resurrects"),
  not as lists of contract numbers or acceptance matrices.
- Update `project-status.md` only when a milestone closes. Do not record
  in-progress work, review state or commit readiness in durable docs.
- At closure, condense the plan to scope, key decisions and results. Git keeps
  everything else; a closed plan is not a transcript.
- Agents ignore `docs/archive/` unless the maintainer explicitly asks for it.
  Archived plans are historical, not current rules or scope, and are exempt
  from the closed-plan size target.
- Do not create additional workflow or work-in-progress documents.
- For documentation-only changes, check the diff, links and `git diff --check`;
  do not build or run CTest.

## Git

- `main` is the stable branch. Prefer focused commits with clear messages.
- Never force-push or rewrite history. Do not commit build directories or
  original data.
- Do not commit, push or tag without the maintainer's explicit authorization.
- When two agents work at the same time, give each its own `git worktree` and
  build directory; at most one agent writes to a given checkout.
- After an authorized push, report branch, `git rev-parse HEAD`,
  `git rev-parse origin/main` and `git status --short`. If the tree is dirty or
  diverged before new work, stop and report it; do not repair it silently.

## Efficiency

Priorities, in order: correctness, then credit/resource efficiency, then speed.
- Prefer targeted inspection; do not rescan ScummVM or original resources when
  the docs already establish the behavior.
- Keep prompts and plans short: reference the plan instead of restating it.
- Use subagents or parallel work only when they improve correctness or
  independent verification.

### Long-running commands

- Tests run on `build-rel` (RelWithDebInfo, `-O2 -g` without `-DNDEBUG`):
  iteration (`ctest -L fast`) and the complete suite. `build-m44` (Debug,
  `-O0`) stays for debugging and the maintainer's play-tests. At milestone
  closure, the complete suite also runs once on `build-m44` through the same
  runner, as a check against optimization-dependent behavior.
- While iterating, run affected tests and `ctest -L fast`. Run the complete
  suite once after the final build, unless failures require a rerun.
- Delegate the complete suite to one test-runner subagent (in Codex:
  `gpt-6-luna`, low reasoning effort, `fork_turns: none`; in Claude Code:
  Haiku, low effort, no conversation context). It runs
  `tools/run-full-ctest.ps1` with its default single job (do not pass
  `-Jobs`; process tests have timeouts that parallel load breaks) on the
  current checkout and build directory,
  including uncommitted changes; this is an exception to the
  separate-worktree rule, and the main agent must not modify either while
  tests run. The runner waits on the process using the longest wait per call
  its tools allow (for example 300 s), not short polls. It does not edit,
  diagnose, fix or rerun anything, and returns only exit code, duration,
  summary, failed test names and result paths.
- The main agent waits through the native agent-completion mechanism with
  the longest wait allowed, without reading logs, polling status or sending
  progress messages, then reads the result once.
- If the complete suite fails only because of stale test expectations and the
  fix changes test code only, the same runner reruns just the failed tests
  (`ctest --test-dir <build> --rerun-failed --output-on-failure`, single
  job) and the main agent runs `ctest -L fast`, instead of another complete
  run. Any production-code change requires another complete run.
- If that model, the script or delegation is unavailable, report it and leave
  full-suite validation pending; never run the complete suite on the main
  model instead. A passing complete suite is still required before declaring
  production changes complete.
- In the handoff, state how the complete suite was run: runner model and
  effort, the command, and how the main agent waited (including any
  intermediate checks).
- Write long build output to a log and read the summary and failures after
  completion. Do not start other subagents, mid-task reviews or parallel
  worktrees unless the maintainer asks.

## Dependencies

`docs/dependencies.md` is authoritative for the pinned ScummVM revision and
configuration. Never copy the ScummVM source tree into this repository.
MMModern links selected ScummVM portions and is GPL-3.0-or-later.

## Project language

English is canonical for identifiers, comments, diagnostics, command-line
output, tests, documentation and commit messages. Original game text stays
resource-driven. Translate existing non-English development strings when
touching that code.
