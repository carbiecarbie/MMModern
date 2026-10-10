# Documentation rules

Read this before creating or editing `README.md`, any `AGENTS.md` or anything
under `docs/`, including milestone plans and milestone closure. The root `AGENTS.md` still
applies, including its documentation-only check and its ban on additional
workflow documents.

## Documents and their responsibilities

| Document | Responsibility | Target size |
| --- | --- | --- |
| `README.md` | Public introduction, how to build, run and play | Short |
| `docs/project-status.md` | What can be played and done now; architecture overview; known gaps | ~200-300 lines |
| `docs/project-history.md` | One short paragraph per completed milestone | Short |
| `docs/roadmap.md` | Next milestones and longer-term direction | ~100-150 lines |
| `docs/milestone-N-plan.md` | That milestone's plan; condensed at closure | Closed: ~30-100 lines |
| `docs/original-quirks.md` | Original DOS oddities kept on purpose, unconfirmed cases following the reference, and reference behavior not reproduced; QoL candidates | Grows by entry |
| `docs/dependencies.md` | Toolchain, pinned ScummVM revision and configuration, build setup (authoritative) | - |
| `docs/archive/` | Plans of milestones 6 and 15-43, kept as a historical record | Exempt |
| `AGENTS.md`, `docs/AGENTS.md`, `tools/AGENTS.md` | Agent instructions: every task; documentation; complete test suite | Root under 200 lines |

## Milestone plans

| | Tier A - core | Tier B - content, UI, services |
| --- | --- | --- |
| Plan | Short plan (~100-150 lines): goal, design decisions, risks, acceptance | Checklist: goal, tasks, acceptance criteria (~30-60 lines) |

Tier scope, reviews and acceptance are defined in the root `AGENTS.md`.

- Record the maintainer's formal acceptance in the plan's status line, as M50
  and M51 do ("Status: completed and accepted", with the relevant commits).
  Never record acceptance before the maintainer has accepted the milestone.
- At closure, condense the plan to scope, key decisions and results. Git keeps
  everything else; a closed plan is not a transcript.

## Writing rules

- Describe capabilities in player terms ("the Temple heals and resurrects"),
  not as lists of contract numbers or acceptance matrices.
- Update `project-status.md` only when a milestone closes, except for narrow
  factual corrections the maintainer explicitly authorizes. It is not a
  development diary.
- Do not record in-progress work, review transcripts, temporary review state or
  commit readiness in durable docs.
- Archived plans in `docs/archive/` are historical, not current rules or scope.
- `docs/original-quirks.md` is updated in the same change that keeps or
  rejects an original or reference oddity (see the root `AGENTS.md`); it is
  durable, not a work-in-progress list, and is not condensed at closure.
