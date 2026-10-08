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

## Specialized instruction files

This file holds the rules for every task. Not every agent loads the files
below automatically, so read each one whose trigger applies before acting; if
several apply, read all of them.

| File | Read it before |
| --- | --- |
| `docs/AGENTS.md` | Creating or editing `README.md`, any `AGENTS.md` or anything under `docs/`: milestone plans, milestone closure, project status, history, roadmap |
| `tools/AGENTS.md` | Running or delegating the complete CTest suite, rerunning its failed tests, or acting as its test runner |

## Source of truth

- Repository: https://github.com/carbiecarbie/MMModern
- Actual Git state, the working tree and current code/tests establish implemented
  behavior. Inspect them before changing interfaces or relying on prose.
- `docs/roadmap.md` sets direction; the active milestone plan sets scope. Older
  plans and history are background, not current authority. Ignore
  `docs/archive/` unless the maintainer explicitly asks for it.
- Agents without local access must use a verified commit SHA as their baseline
  and report when it cannot be retrieved; do not invent local state.

## Development principles

- **Fidelity first.** Observable behavior of the original DOS game takes
  precedence; evidence from normal DOSBox play overrides the reference. When a
  plan, prompt or code is unclear about a behavior and the pinned ScummVM
  reference shows what the original does, reproduce it without asking. Ask the
  maintainer when the original itself is ambiguous, when the evidence is
  contradictory or cannot be reliably verified (except the condition-counter
  cases below), or when reproducing it would leave the milestone scope. Never
  invent behavior the original does not have, including to get past
  uncertainty.
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
- **Implement mechanics generically.** Follow the original semantics and
  implement each mechanic once, for every place the original game uses it,
  instead of for one selected address or witness.
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
  legacy readers, frozen per-milestone behavior or per-milestone
  content/contract numbers.
- The envelope version `kJourneyVersion` and the Journey
  `kJourneySchema`/`kJourneyContent` pair (`src/formats/xeen/XeenSaveFormat.h`)
  legitimately name the current format; older values are recognized only to
  reject them clearly.
- When a save's layout or meaning changes, the milestone plan says so and the
  envelope version is incremented (M50: v5, M51: v6); schema and content change
  only when the plan requires it. Never change any of them otherwise; a plan
  that retains the format (M52 retains v6) must be amended before a change.
- Restoring must still be exact for the current format: no replayed events,
  rewards, time or RNG. A generic save/load round-trip test covers this.

## Milestone workflow

Each milestone is classified in its plan as Tier A or Tier B. Acceptance for
both is tests plus a maintainer play-test. Plan formats are in `docs/AGENTS.md`.
- **Tier A - core** (save format, RNG, timing/scheduling,
  ownership/architecture, ScummVM integration): an independent review of the
  plan and of the implementation is mandatory. These scheduled reviews are part
  of the workflow and need no separate authorization.
- **Tier B - content, UI, services** (areas, Events, monsters, items, spells,
  services, UI, presentation): no mandatory independent review. If a
  significant risk appears, ask the maintainer to authorize one.
- Any other, unplanned mid-task review needs the maintainer's authorization.

Steps:

1. Read `docs/project-status.md` and the milestone plan; inspect relevant code/tests.
2. Implement within the milestone scope. Planning and implementation may be
   one task for Tier B. If the scope proves wrong, stop and say so instead of
   silently widening or narrowing it.
3. Add or update tests for changed behavior: unit tests for rules, and a few
   original-data end-to-end tests where they prove real gameplay. Do not add a
   separate process-continuation witness per feature.
4. Build and run the complete CTest suite as `tools/AGENTS.md` requires. Do not
   declare completion with failing tests.
5. The maintainer plays the result and accepts it. Only after that acceptance,
   record it in the plan's status line (see `docs/AGENTS.md`).

Roadmap approval does not authorize implementation; the maintainer starts each
milestone explicitly.

**Maintenance tasks.** Small fixes (a defect, a flaky test, a doc correction)
may be done without a milestone plan: a scoped prompt, tests for a code or test
fix, the complete CTest suite if production code changes, and the mandatory
independent review when the fix touches Tier A areas. Documentation-only fixes
use the documentation check below. If a fix turns out to be larger or
recurring, stop and propose a milestone instead.

## Building and testing

- **Completion requirement.** A production-code change is never complete
  without a passing complete CTest suite, run through the test-runner protocol
  in `tools/AGENTS.md` (which also says when and on which builds).
- Tests run on `build-rel` (RelWithDebInfo, `-O2 -g` without `-DNDEBUG`).
  `build-m44` (Debug, `-O0`) stays for debugging, the maintainer's play-tests
  and the closure check (`tools/AGENTS.md`). Setup is in `docs/dependencies.md`.
- While iterating, run the affected tests and `ctest -L fast` on `build-rel`.
- Write long build output to a log and read the summary and failures after
  completion.
- **Documentation-only changes** (no production code or tests changed): check
  the diff and links and run `git diff --check`; do not build or run CTest.

## Documentation

`docs/AGENTS.md` lists each document's responsibility and the writing and
closure rules. Do not create additional workflow or work-in-progress documents
(progress reports, temporary workflow notes, redundant plans or duplicated
instructions). The only exception is the two specialized `AGENTS.md` files
listed above.

## Git

- `main` is the stable branch. Prefer focused commits with clear messages.
- Never force-push or rewrite history. Do not commit build directories or
  original data.
- Do not commit, push or tag without the maintainer's explicit authorization.
- When two agents work at the same time, give each its own `git worktree` and
  build directory; at most one agent writes to a given checkout. The one
  exception is the complete-suite test runner, which runs in the main agent's
  checkout (`tools/AGENTS.md`).
- After an authorized push, report branch, `git rev-parse HEAD`,
  `git rev-parse origin/main` and `git status --short`. If the tree is dirty or
  diverged before new work, stop and report it; do not repair it silently.

## Efficiency

Priorities, in order: correctness, then credit/resource efficiency, then speed.
- Prefer targeted inspection; do not rescan ScummVM or original resources when
  the docs already establish the behavior.
- Keep prompts and plans short: reference the plan instead of restating it.
- The main agent may launch subagents or parallel work when they materially
  improve correctness, investigation or independent verification, except while
  the complete suite runs (`tools/AGENTS.md`).

## Dependencies

`docs/dependencies.md` is authoritative for the pinned ScummVM revision and
configuration. Never copy the ScummVM source tree into this repository.
MMModern links selected ScummVM portions and is GPL-3.0-or-later.

## Project language

English is canonical for identifiers, comments, diagnostics, command-line
output, tests, documentation and commit messages. Original game text stays
resource-driven. Translate existing non-English development strings when
touching that code.
