# Milestone 44 plan - Simplification

**Tier A. Status: completed and accepted.** Closing code commit `830af7e`.

## Goal

Remove the legacy machinery that made every change expensive, without changing
what the game does. Applies the pre-release policy in
[AGENTS.md](../AGENTS.md#save-compatibility-pre-release-policy).

## Final scope and decisions

- **One save format (D1).** Only envelope v4, schema 9, content 14 is read, with
  the wire layout unchanged, so M43-era saves still load. Older saves are
  rejected before anything is restored ("created by an older MMModern build");
  newer or unknown envelopes get a separate "newer or unsupported" message.
  F9 may overwrite a recognizable older MMModern save; unknown, corrupt and
  newer files stay protected. Schema 9 / content 14 are save-format constants,
  not gameplay configuration.
- **One configuration (D2).** `XeenJourneyContent` describes only the current
  Regional Journey (map 23 `(9,11)` West, 19 actors, 49 Vertigo cells). Numbered
  contracts, capability predicates, map-20 descriptors and `contract`
  parameters are gone.
- **Entry modes (D3).** Kept: `--journey-region [--combat-seed]`,
  `--load-game`, inspection tools, the static party screen and `--render-map`
  as an unsaveable explorer (revisit with the normal-start milestone, now M47). Removed: `--encounter-26/27`,
  `--journey-skeleton`, `--journey-expedition` and the Completed/Diagnostic27
  flow.
- **Tests (D4).** Synthetic regional fixtures replaced the map-20 ones. Legacy
  process tests were retargeted to the current game; assertions about
  legacy-only behavior were replaced by rejection tests. Every CTest entry is
  labelled `fast` or `process`. Three original-data scenarios (mainland combat
  with Myra/Phirna; Vertigo Buy, Repair and Training; Temple recovery) run in
  CTest against SHA-256 digests recorded before any M44 change, then reload and
  re-save byte-identically; this is also the generic round-trip test. Tests
  fail when an expected link-time probe never fires.
- **Docs (D5).** Plans M15-M43 and `milestone6.md` moved to `docs/archive/`;
  status, history and README rewritten in player terms.

Shared code with legacy names was kept because current play uses it
(`xeenValidateCompletedEquipment`, `party.publishCompleted`, `XeenRestoreGuard`).

## Evidence boundary

Behavior equivalence rests on the three Step-0 digests, which matched before
and after every stage, and on an M43-era save that loads and continues
identically. The remaining long process tests were evaluated for removal
(step 5b): they hold unique failure-injection, retry, mutation and native-input
coverage, so only two duplicates were removed. Speeding them up would require
in-process controls and is left as an optional future task.

## Results

| Measure | Before | After |
| --- | ---: | ---: |
| `src/` lines | 27,764 | 25,277 |
| CTest entries | 141 | 136 (107 fast, 29 process) |
| Full CTest, serial | ~47 min | ~47 min (~27 min with two parallel jobs) |
| `ctest -L fast` | - | ~5 min serial (~1.3 min parallel) |
| `project-status.md` / README | 1,107 / 489 lines | 202 / 108 lines |

Validation: full CTest 136/136; independent reviews of the plan (two), Stage A
(two) and step 6 (one), none blocking; maintainer play-test of a new Journey,
Vertigo service, F9/restart, M43-save continuation, older-save rejection and
overwrite, removed modes and the unsaveable explorer.

## Known issues carried forward

- **Run to an occupied destination:** the artificial control
  `mmodern_consequence_original` fails with "Stale encounter frame" on the
  pre-M44 baseline and after M44; likely a real current defect, not yet fixed.
- **`xeen_save_sdl` flake:** fails occasionally under heavy parallel load
  because it sends keys after fixed sleeps instead of waiting for frame
  readiness. Pre-existing.

Both are listed in the [roadmap](roadmap.md#next-milestones).
