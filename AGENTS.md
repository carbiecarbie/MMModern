# MMModern - Agent Instructions

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Source of truth

- Repository: https://github.com/carbiecarbie/MMModern
- With local access, actual Git state and the working tree establish the
  candidate; current code/tests establish implemented behavior. Inspect them
  before changing concrete interfaces or relying on earlier prose.
- Milestone plans specify intended scope. Historical planning prose does not
  override later accepted implementation. Identify conflicts among plans,
  code/tests and status before expanding scope or making architectural assumptions.
- Read relevant `docs/` files before architectural changes. Do not assume prior
  conversations describe the current checkout accurately.
- Architecture/specification agents without local access must use a verified
  commit SHA as their public baseline and prefer GitHub/raw/permalink retrieval
  pinned to that SHA. Cached `/main/` pages do not override a verified commit.
  If that commit cannot be retrieved, report the limitation; do not invent local state.

## Development workflow

Before implementing a task:

1. Read `docs/project-status.md` and the applicable milestone specification.
2. Inspect the implementation and identify relevant tests.
3. Preserve existing architecture unless there is a concrete reason to change it;
   reuse project abstractions before introducing parallel systems or owners.
4. Keep changes within the requested task or sub-stage.
5. Never modify or include original commercial Might and Magic game data.

After implementation, build, run relevant tests and run the complete CTest suite
when practical. Do not declare completion with failing tests. Update durable
technical documentation as appropriate and summarize changes and validation.
For documentation-only work, review the diff, links, factual consistency and
`git diff --check`; do not build or run CTest solely for prose changes.

When modifying a subsystem, add or update tests for changed behavior. Prefer the
smallest sufficient set during iteration; full CTest is required at milestone closure.

## Documentation responsibilities

Persistent documents describe durable truths, with one natural home per topic:

| Document | Responsibility |
| --- | --- |
| `README.md` | Concise public introduction, capabilities and runtime guidance |
| `docs/project-status.md` | Technical snapshot of the latest stable committed state |
| `docs/project-history.md` | Concise completed-milestone history; not current-state authority |
| `docs/roadmap.md` | Future direction, dependencies, confidence and review cadence |
| `docs/milestone-N-plan.md` | That milestone's specification, durable decisions and concise final record |
| `AGENTS.md` | Source-of-truth, workflow, documentation and Git rules |

Link to the natural home instead of duplicating narratives. History is optional
reading for ordinary implementation work; consult it when evolution matters.
Do not create another workflow/status document or a current-work-in-progress file.

Keep transient workflow state in task reports, chat context and temporary build
logs. README, status, roadmap and closed plans must not record a local candidate's
commit/push readiness, outstanding review, or just-finished implementation as
project state. These moments are not durable capabilities or acceptance results.

`project-status.md` describes only the latest **stable committed** state. Leave
it unchanged when a milestone starts, a local candidate exists, review is outstanding
or a correction is in progress. Update it during final closure only after required
acceptance/review establishes the durable state to be committed. It must remain
usable as the stable baseline throughout subsequent local implementation work.

Use roles for durable attribution: maintainer, implementation agent,
architecture/specification agent and independent reviewer. Distinguish
maintainer-performed physical acceptance from independent review and automated
or image-based evidence. Keep technically relevant tool names such as SDL,
ScummVM and CMake. Apply this rule to documents being maintained and future work;
do not mechanically rewrite unrelated historical files.

Active plans may contain investigation. At closure, condense them to final scope,
durable architectural decisions, original-data/behavior contracts,
persistence/compatibility policy, acceptance boundary and concise final results.
Keep a superseded decision only if a short explanation clarifies the final design.
Remove operational diaries: process IDs, ephemeral build/log/image paths, repeated
commands/test totals, review iterations, candidate states and duplicated matrices.
Git history preserves removed text; a closed plan is not a transcript archive.

## Milestones

- Create a dedicated plan only when that milestone is about to begin.
- Treat the applicable milestone/sub-stage specification as its intended scope.
- Use the next roadmap entry as the default successor for planning. Entries
  beyond the immediate next milestone remain provisional; revise them when
  evidence meets replanning triggers, not merely because work completed.
- Check roadmap approval and review cadence. Neither roadmap approval, completed
  work nor a push authorizes the next milestone or sub-stage implementation.
- Do not investigate future stages unless necessary to avoid an architectural
  mistake in the authorized stage.

At milestone closure:

1. Ensure the build, full CTest suite and required manual validation pass.
2. Obtain required acceptance/review before recording completion.
3. Update the stable status, add concise history, condense the closed plan and
   remove completed entries from future roadmap scope; link between them.
4. Update README only for public capability/interface changes.
5. Prepare a concise commit message; observe the Git authorization rules below.

## Git workflow and post-push baseline

- `main` is the stable development branch. Prefer focused commits with clear messages.
- Never force-push or rewrite history. Do not commit build directories or original data.
- Do not push or create/push milestone tags without explicit authorization.
- After an authorized commit and push closes a work unit, record in the handoff:
  branch, `git rev-parse HEAD`, `git rev-parse origin/main`,
  `git ls-remote origin refs/heads/main` and `git status --short`.
- Expected healthy handoff: branch `main`, HEAD == origin/main == direct remote
  main, with a clean working tree and empty staged state. Direct remote Git is
  the post-push authority; public HTML/raw/history views may refresh inconsistently.
- Before continuing from that handoff, verify the gate and intended committed
  closure. If dirty, divergent, on another branch, or unverifiable, stop and
  report the exact discrepancy; do not repair/synchronize the baseline implicitly.
- Use the exact resulting SHA for the next external planning/review task.
  Keep planning approval and implementation authorization separate.

## Dependencies and efficiency

Consult `docs/dependencies.md` for the authoritative pinned ScummVM revision and
configuration; source/build directory names are not fixed. Use that configuration
and never copy the full ScummVM source tree into this repository.
MMModern links selected ScummVM portions and is GPL-3.0-or-later.

Prefer targeted inspection. Do not rescan ScummVM or original resources when
existing documentation already establishes the required behavior.

For implementation-task preparation:

- The committed milestone/stage plan is the authoritative technical contract.
- Execution prompts should reference detailed formulas, traces, matrices,
  architectural decisions and acceptance requirements already recorded in this
  file or the plan instead of reproducing them.
- Prompts should primarily carry the verified baseline, authorized scope and
  objective, task-specific clarifications, validation requirements, Git
  restrictions and expected handoff.
- Investigate inherited milestones, code/tests, ScummVM and original resources
  selectively when the current contract already establishes needed behavior.
- Record durable new implementation decisions in the active milestone plan; do
  not create a second specification in the execution prompt.
- Investigation and implementation may remain one task when narrow investigation
  is necessary to execute an approved contract.
- Brevity must not weaken safety, ownership, scope or validation.

## Project language

English is canonical for identifiers, comments, diagnostics, command-line output,
tests, documentation and commit messages. Original game content stays
resource-driven; do not hard-code original text to satisfy this convention.
Localization is separate from development-language policy. Translate existing
non-English development strings opportunistically when touching relevant code,
without expanding unrelated work.
