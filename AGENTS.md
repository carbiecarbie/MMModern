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
4. Keep changes focused on the requested task.
5. Do not modify or include original commercial Might and Magic game data.

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

The current local development setup expects:

- `../scummvm-master`
- `../build-xeen-probe-sdl`

Do not copy the full ScummVM source tree into the MMModern repository.

MMModern is distributed under GPL-3.0-or-later.

## Tests

Tests are an important part of the project.

When modifying an existing subsystem:

- run its relevant unit/integration tests;
- add or update tests when behavior changes;
- run the full CTest suite before declaring a milestone complete.

## Milestones

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
