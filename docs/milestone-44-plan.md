# Milestone 44 plan - Simplification

**Tier A. Status: approved by the maintainer on 2026-10-02; implementation not yet authorized.**
Baseline: `main` at `6d07af86a074711cb8adc8cd78588865f67b34ef` (clean).
Revised after two independent plan reviews (both REVISE); their findings are
addressed below.

## Goal

Remove the legacy machinery that makes every change expensive, without changing
what the current game does. After M44 there is one Journey configuration, one
accepted save format, fewer entry modes, proportionate tests and short docs.
This applies the pre-release policy in [AGENTS.md](../AGENTS.md#save-compatibility-pre-release-policy)
and prepares M45/M46 ([roadmap](roadmap.md#next-milestones)). The roadmap's
"capability flags" wording is superseded here: with one configuration the
predicates are simply removed.

Baseline size, for comparison at closure: `src/` 27,764 lines; `tests/` 36,791
lines of `.cpp` (39,895 with headers); 134 `add_test` declarations (141 tests
after loop expansion); plans M15-M43 plus `milestone6.md` 19,094 lines;
`project-status.md` 1,107; `project-history.md` 410; `README.md` 489.

## Non-goals

- No gameplay, rule, route, actor, RNG or presentation change. The current
  Regional Journey (content 14) must play exactly as after M43.
- No whole-map admission, mouse/UI or Rest work (M45/M46).
- No owner/coordination redesign. If a removal forces one, stop and report.

## Decisions

**D1 - One save format.** Only envelope v4, schema 9, content 14 is accepted,
with its wire layout unchanged, so M43-era saves still load.
- Every other envelope or pair (ordinary v1/v2, completed v3, Journey
  1/1-9/13) is rejected during decoding/validation, before any restore or
  gameplay publication: "This save was created by an older MMModern build and
  is no longer supported." (Schema/content are read after the common base,
  `XeenSaveFormat.cpp:457/508`; that is acceptable because nothing is published.)
- **Overwrite:** `XeenSaveFile::write` decodes an existing destination before
  replacing it (`XeenSaveFile.cpp:134`). F9 may overwrite a file that is a
  recognizable MMModern save of an older version (valid envelope/CRC); unknown
  or corrupt files stay protected. Test both outcomes.
- Keep the common v4 base reader (`readCharacter(in, version)`,
  `XeenSaveFormat.cpp:143-146`) fixed to the current layout; remove
  `LegacyV1MissingFields` (`XeenSaveSnapshot.h:38`, `XeenSaveState.cpp:482-500`)
  and ordinary/completed capture and restore.
- Keep schema 9's legal variants: Vertigo actor block absent/46/52 and the
  variable suffix length (`XeenSaveFormat.cpp:589-604`). Optional Journey fields
  stay optional in the types; content 14 requires them as today.
- Future layout or meaning changes bump the version and reject older saves.

**D2 - One Journey configuration.** `XeenJourneyContent` collapses to the
current Regional Journey (map 23 `(9,11)` West, 19 actors, 49 Vertigo cells).
- Remove the `contract` field, `xeenSupportedJourneyPair`, the capability
  predicates and every branch that runs only when a predicate is false.
- Remove the map-20 descriptors (`contains`, `actor`, `movementContains`,
  `blockedTerrain`). Content 14 does not reach them: combat returns through
  `serviceConsequences` (`XeenCombat.cpp:781`) and uses the regional movement
  candidate (`XeenCombat.cpp:1273`).
- Replace explicit references to descriptor 3 in live regional code (for
  example `XeenActorApproach.cpp:225`) and `contract != 3` paths content 14 takes
  (`XeenGameplay.cpp:230/235`, `CloudsMapComposer.cpp:85-87`) with the single
  configuration.
- Remove default `contract` arguments (`XeenMonsterTreasure.h` default 4;
  `XeenJourneyRules.h`, `XeenActorApproach.h`, `XeenEncounterFlow.h` default 1)
  and fix callers explicitly.

**D3 - Entry modes.**

| Mode | Decision |
| --- | --- |
| `--journey-region [--combat-seed]` | Keep; `--combat-seed` stays for this mode only (tests depend on it). |
| `--load-game` | Keep; loads only the D1 format. |
| `--inspect-map/-party/-events`, default static UI run | Keep (developer tools). |
| `--render-map` | Keep as an **unsaveable** map explorer: remove `--save-file` (`main.cpp:185-199`) and the F9 target path (`XeenGameplay.cpp:312`), keeping its initial-owner loading (`XeenGameplay.cpp:153`). Revisit in M46. |
| `--encounter-26`, `--encounter-27`, `--journey-skeleton`, `--journey-expedition` | Remove, with the Completed/Diagnostic27 flow, revisit and v3 saves. |

Shared code with legacy names that current play uses and must survive (rename
allowed): `xeenValidateCompletedEquipment` (Smith, Purchase, Journey rules);
`party.publishCompleted` used by Journey restore (`XeenSaveState.cpp:249`,
`XeenParty.cpp:53`); `XeenRestoreGuard` (casting, services). `XeenEncounterFlow`
hosts the Journey; only its Completed members go, with the `completed()` uses in
`XeenEventFlow.cpp`, `XeenGameplay.cpp` and `_completedMonster` in
`XeenRestoreGuard.h`.

**D4 - Tests.** Preserve assertions before removing their harnesses.
- **Synthetic regional fixture (test-only)** supplying actor admission without
  original data. Retarget the synthetic suites built on map-20/Diagnostic27
  fixtures (`xeen_combat`, `xeen_combat_authority`, `xeen_combat_persistence`,
  `xeen_encounter_flow`, `xeen_actor_approach`, `xeen_journey`) and the
  `XeenJourneyEventTests` assertions (grant survives failed Remove, recovery and
  save checks; regional Events still use `journeyEventWork`,
  `XeenEventFlow.cpp:796`).
- **Ordinary-save oracles** (`XeenSaveStateTests`, `XeenSaveFileTests`,
  `XeenSaveFlowTests`, `XeenSaveCliTests`, `XeenSessionPersistenceTests`,
  `SaveResumeIntegrationTest`, persistence part of `IndoorObjectIntegrationTest`)
  move to a synthetic schema-9/content-14 snapshot builder in test support.
- **Process scenarios:** replace the per-milestone M35-M43 CLI/process
  witnesses, `content10/12` variants and non-CTest executables/scripts
  (`*_cli_witness`, `*_smoke`, `*_original`, `RunConsequenceWitnesses.py`,
  `RunRegionalWitnesses.ps1`) with **three representative original-data
  process scenarios** reusing `XeenChildProcessTestSupport.h` and the M43 harness:
  (1) mainland combat plus Myra/Phirna; (2) Vertigo entry, Repair, Buy and
  Training; (3) Temple recovery. Each ends in save, restart and exact compare.
- **Generic round-trip test**, parameterized over quiet states from those
  scenarios: capture -> encode -> decode -> restore -> bind/present Flow ->
  capture is byte-identical (see `XeenJourneyPersistenceTests.cpp:76`).
- Focused rule and failure-path tests stay as they are.
- **Link-time probes:** update every `--wrap` symbol (`CMakeLists.txt:199-200`,
  `XeenRestoreReplayProbe.h:8-26`) and assert that each probe fired.

**D5 - Documentation.**
- Move `milestone-15..43-plan.md` and `milestone6.md` to `docs/archive/` with a
  header line: "Historical record; not current rules or scope." Rewrite their
  relative links mechanically for the new location. Archived plans are exempt
  from the closed-plan size target; add to `AGENTS.md` that agents ignore
  `docs/archive/` unless explicitly asked, and that this exemption applies.
- Rewrite `project-status.md` in player terms (<=300 lines), trim
  `project-history.md` to one short paragraph per milestone, trim `README.md`
  run/controls to the D3 modes. Update links from current docs (README,
  dependencies, status, history) to the archive or remove them.
- No new code comments beyond rules whose only description was a removed link
  that current code needs.

## Stages and work order

**Step 0 - Baseline evidence.** In a separate worktree at the baseline, run the
three D4 process scenarios (retargeted to content 14 first where an existing
witness forces a legacy configuration, e.g. `XeenM39CliWitness.cpp:97`) with
fixed seeds, and **commit their SHA-256 digests** of final saved state as test
expectations. Never commit `.mmsave` files: they contain bytes derived from
original data.

**Stage A - test migration, modes and save formats:**
1. D4 fixtures first: synthetic regional fixture, retargeted synthetic suites
   and `XeenJourneyEventTests` assertions, synthetic snapshot builder. All
   existing tests still pass.
2. D3: remove modes and the Completed/Diagnostic27 flow with their now-redundant
   tests.
3. D1: restrict reader/writer, overwrite policy, retarget ordinary-save oracles.
   Process tests and witnesses that save/restore older Journey contents
   (e.g. Smith, combat-casting, service-day, Training, M35-M42 witnesses) are
   migrated in this step, pulling that part of step 5 forward:
   - assertions about legacy-only behavior (for example "combat casting is
     unavailable in content 8/9") are obsolete, not lost coverage; delete them
     and add D1 rejection tests for those versions;
   - assertions about behavior content 14 still has (repair, casting, service
     days, Training, purchase) are retargeted to content 14, or deleted when an
     existing content-14 process test or the Step-0 scenarios already cover it;
     record which in the handoff.
   The "preserve assertions" rule applies to content-14 behavior only.

**Stage B - docs, consolidation and configuration** (one commit per step):
4. D5: documentation first. After Stage A, README and `project-status.md`
   describe removed modes, contracts and save versions, so they mislead agents.
   Status and README describe the post-Stage-A state; step 6 updates any line
   it invalidates.
5. D4 remainder, split by maintainer decision. **5a:** `fast`/`process` labels on
   every CTest entry (`ctest -L fast` for iteration, plain `ctest` for full
   runs, both in README), the three scenarios as CTest tests checked against the
   Step-0 digests with reload/re-save (this is also the generic round-trip
   test), and probe-fired assertions. **5b** (authorized after 5a; runs before
   step 6 so step 6 does not adapt tests it would delete): remove the old
   M35-M43 witnesses and legacy process tests whose content-14 behavior is
   evidently covered by the scenarios or an existing current test; report,
   without deleting, any case where coverage is unclear.
6. D2: collapse the configuration, protected by the step-5 scenarios.

Every step leaves the build and remaining tests green. The maintainer may accept
Stage A before Stage B starts; if Stage B grows beyond plan, it closes as a
separate milestone. If any assertion cannot be preserved without disproportionate
work, stop and report rather than silently dropping coverage.

## Risks

- **Lost coverage:** mitigated by migrating fixtures and assertions before any
  deletion, and by checking which source paths a test uniquely covers.
- **Silent behavior change in content 14:** mitigated by Step-0 digests and by
  loading an M43-era save before and after.
- **Silently inert test probes:** mitigated by fired-probe assertions.

## Acceptance

- Build succeeds; full CTest passes after each stage.
- The three process scenarios match their Step-0 digests; an M43-era save loads
  and continues identically; older saves are rejected with the D1 message and
  can be overwritten by F9; unknown files stay protected.
- Removed modes print usage instead of starting.
- Size comparison against the baseline is reported.
- Independent implementation review (Tier A).
- Maintainer play-test: mainland fight, Vertigo services (Repair, Buy,
  Training, Temple), F9, restart.
