# MMModern - Agent Instructions

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Source of truth

- Repository: https://github.com/carbiecarbie/MMModern
- The repository contents are the source of truth for the current state of the project.
- Do not assume that previous conversations or prompts describe the current code accurately.
- Inspect the current implementation before making changes.
- Read relevant files in `docs/` before making architectural changes.

## Development workflow

Before implementing a task:

1. Inspect the existing implementation.
2. Identify the relevant tests.
3. Preserve the existing architecture unless there is a concrete reason to change it.
4. Reuse existing project abstractions and infrastructure before introducing new parallel systems or architectural layers.
5. Keep changes focused on the requested task.
6. Do not modify or include original commercial Might and Magic game data.

After implementing a task:

1. Build the project.
2. Run the relevant tests.
3. Run the complete CTest suite when practical.
4. Do not consider the task complete while tests are failing.
5. Update documentation when behavior, architecture, dependencies, or project status changes.
6. Summarize what changed and what was tested.

## Git workflow

- `main` is the stable development branch.
- Do not force-push.
- Do not rewrite Git history.
- Do not commit build directories.
- Do not commit original Xeen game data.
- Do not create or push milestone tags unless explicitly requested.
- Do not push changes unless explicitly requested.
- Prefer focused commits with clear commit messages.

## ScummVM

MMModern currently integrates and links against portions of ScummVM.

Consult `docs/dependencies.md` for the authoritative pinned known-good ScummVM
revision and dependency configuration. Use that revision and configuration;
source and build directory names are not fixed.

Do not copy the full ScummVM source tree into the MMModern repository.

MMModern is distributed under GPL-3.0-or-later.

## Tests

Tests are an important part of the project.

When modifying an existing subsystem:

- run its relevant unit/integration tests;
- add or update tests when behavior changes;
- run the full CTest suite before declaring a milestone complete.

## Milestones

Use the planning documents according to their authority:

- `docs/project-status.md` records current implemented state and validation.
- `docs/project-history.md` is a concise historical summary, not a current-state
  authority or required reading for ordinary implementation tasks. Consult it
  only when historical context/evolution is relevant. Detailed historical
  implementation and validation evidence remains in dedicated milestone plans.
- `docs/roadmap.md` records approved future direction once reviewed and approved;
  entries marked proposed are recommendations, not implementation authorization.
- A dedicated `docs/milestone-N-plan.md` defines the detailed scope of an active
  milestone. Create that plan only when the milestone is about to begin.
- After completing a milestone, use the next roadmap entry as the default
  successor for planning, rather than selecting a successor from scratch.
- Entries beyond the immediate next milestone remain provisional. Revise them
  when concrete evidence meets the roadmap's replanning triggers; normal
  completion alone does not require rediscovering the roadmap.
- Read the roadmap's approval status and review cadence. Neither roadmap
  approval nor milestone completion authorizes starting the next implementation.

When a milestone or sub-stage has a dedicated plan/specification in `docs/`,
treat that document as the specification for its intended scope.

If the milestone plan, current code, tests, and project-status documentation
appear to conflict, identify the conflict before expanding scope or making
architectural assumptions.

When completing a milestone:

1. Ensure the project builds.
2. Ensure the complete test suite passes.
3. Perform any required manual validation.
4. Update project-status documentation.
5. Update the README if publicly visible capabilities changed.
6. Prepare a concise commit message.
7. Wait for explicit approval before creating a milestone tag or pushing changes.

## Current project status

Before starting any significant implementation task:

- Read `docs/project-status.md`.
- Treat it as the current record of milestone progress and implemented capabilities.
- Update `docs/project-status.md` when a milestone starts, materially changes, or is completed.
- Do not mark a milestone complete until its required build, automated tests, and manual validation have passed.

## Agent efficiency

- Prefer targeted inspection over broad repository exploration.
- Do not rescan ScummVM or original game resources when existing project
  documentation already records the required behavior.
- For milestone work, implement only the currently requested sub-stage.
- Do not begin the next milestone or sub-stage unless explicitly requested.
- Do not investigate future sub-stages unless required to avoid an
  architectural mistake in the current one.
- Prefer the smallest sufficient test set during iteration; run the full
  suite at completion.

## Project language

English is the canonical language for MMModern development.

Use English for:

- source-code identifiers and comments
- diagnostics, errors, and command-line output
- tests and test names
- repository documentation
- commit messages

Original Might and Magic game content must remain resource-driven. Do not convert
original game text into hard-coded engine strings just to satisfy this convention.
Localization remains separate from development-language policy.

Existing non-English development or diagnostic strings may be migrated
opportunistically when the relevant code is being touched; do not expand
unrelated tasks solely to translate existing strings.
