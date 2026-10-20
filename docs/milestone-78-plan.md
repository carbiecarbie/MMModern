# Milestone 78 plan - Opening areas

**Tier A. Status: plan approved after independent review; Part 0 authorized.**
Parts A and B start only when the maintainer authorizes them.
Step 8 of the [roadmap](roadmap.md#next-arc-finishing-the-games-systems), after M77.
Baseline: clean `main`/local `origin/main`, `58d8c9a9992953239dcff03b4a7086e83d74acf5`.

## Goal and scope

Open whole Clouds areas beyond 23/28, with actors, Events and every travel path.
Implement Mountaineer traversal, dark view and light drain; Pathfinder movement
requires a consumer established by calibrated DOS evidence, never invention.
Keep original travel costs, facing, timing, actor reloads and arrival effects.

Other towns' services (step 9), scenes/Arena (10), expanded treasure (11), audio,
Inn and Darkside stay out. Unsupported mechanics give a clear notice and preserve
safe play, without substituting Vertigo parameters or bypassing progression checks.

## Design decisions

- Derive admission from original Clouds resources/topology through existing
  loaders and `XeenWorld`, without a maintained map list, component whitelist
  or census threshold. Distinguish primary gameplay maps from neighboring indoor
  tiles. Validate resources/coordinates before publication; respect optional files.
- Share destination validation across movement, Events, spells and restore.
  Admission is independent of skills/buffs; walking, Jump and Teleport retain
  distinct terrain checks. Buff expiry cannot invalidate a saved location.
- Reuse Party/roster, World/actors, Event, Service and Flow owners. Keep immutable
  Journey setup and New Game/developer starts separate from area admission.
  Outdoor crossings use detached transitions: camera, actors, Events, terrain,
  time and RNG publish together. Preserve logical indoor identity across tiles.
- Cover Event teleports (both forms, including stairs), mirror, Town Portal's
  five towns, Beacon Set/Return, Jump, Teleport, Etherealize and Time Distortion;
  also combat Run, Mr Wizard from other maps, sky/cloud surface falls and Event
  fall/pit operations. Keep learned/item paths, including combat item Portal,
  same-map relocation versus reload, original edge normalization and refusals.
- Use `XeenSkills`' two-holder queries; Mountaineer is 9, Pathfinder 11 and
  Navigator is 10. If DOS establishes no Pathfinder movement consumer, record no effect.
  If DOS has one absent from ScummVM, implement DOS and index the difference.
  Ambiguity goes to the maintainer before Part A; no invented forest restriction.
- Lighting uses existing rules/composer, original flags and saved light counter.
  Reproduce obscurity/layers; rendering never spends light. Part 0 settles timing.

## Save decision and M44 audit

Increment `kJourneyVersion` from 15 to 16 in Part B; retain schema 9/content 14
and the planned byte layout. The admitted Journey and restore contract change:
remove the required map-23 state/regional manifest as universal restore gates.
A v15 build must reject new-area saves clearly as newer. Current builds reject
v15 under the [0.x policy](../AGENTS.md); no legacy reader or milestone counters.

Existing per-map state, beacons, skills and light counter carry the new areas.
Part B must confirm that logical-map versus physical-tile state keys do not
change. Restore derives darkness without replaying arrivals or draining light.
If Part 0 or B reveals a durable-field or layout change, amend this decision
before implementing it and maintain the save-dump visitor as needed.

Follow [the M44 audit protocol](../tools/AGENTS.md#m44-digest-audits): Preserve
at the start of each production part, with its old helpers, in a new external
directory; Compare after its changes and each later hash-affecting task.
Part A expects mainland, Temple and roundtrip to remain unchanged. Services
changes because the DOS two-holder Pathfinder requirement for outdoor walls 3/6
changes its map-23 route: `(4,2)` remains passable, but the former continuation
through `(3,2)` is blocked. The detour changes exploration, timing and merchant
regeneration. Audit every changed field/checkpoint and native reload/re-save
equality; differences outside these consequences remain defects. Save v15/schema
9/content 14 remain unchanged. Replacement digests require explicit maintainer
approval.

Part B expects only version/CRC differences plus explained traces from the
unified Vertigo transition.

## Part 0 - Read-only rules check

Use [the pinned reference](dependencies.md), revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, and calibrated static analysis of
`WORLD/XEEN.DAT` where needed. Reuse [magic](rules/magic.md),
[evidence](rules/evidence.md) and [original quirks](original-quirks.md): settled
anchors include drain bit `0x20`, its latch/decrement before darkness, Light's
stepped mark, two holders (`0x2160a`), Levitate/cloud behavior and DOS fall tables.
Investigate only remaining questions; do not rescan settled rules:

- Dark flag (reference `mazeFlags2` bit `0x4000`) and a second DOS consumer;
  whether DOS uses partial obscurity; affected scene/monster, map/minimap and
  border layers (the pin uses BLACK/NONE on the 3D view rectangle).
- Whether turns/blocked moves set stepped, and lighting checks after arrival,
  travel and Light; confirm restore does not replay the drain consumer.
- Skill condition handling (`0x215b9`/`0x2160a`), Mountaineer outdoor wall types
  (reference 1/7/9/10/12) and whether DOS has Pathfinder movement consumers.
- Reconcile cloud fall destinations/coordinates/facing with the settled table;
  trace only unresolved integration, including map-edge normalization.

Calibrate on known rules; verify new bit meanings at a second independent DOS site.
Cite offsets/confidence; keep work copies/projects outside the repo, with no original
bytes/disassembly copied into it. Update rule/evidence docs, quirks and differences.
Conflicting/ambiguous evidence goes to the maintainer before dependent work.
Only condition-counter values unreachable in ordinary play may follow the pinned
reference, marked as the root AGENTS.md requires; amend the plan for scope changes.

## Implementation parts

The maintainer selected sequential fresh tasks: **0, A, then B**.

- **A - Rules:** implement settled traversal/lighting rules with unit/render
  tests and census probes. Retain fixed admission until B; use original-data
  fixtures for new rules. Pathfinder follows Part 0's evidence decision.
- **B - Opening, travel and save:** begin with a complete fixed-guard inventory;
  unify admission, staging, providers and restore; enable all travel, v16 and tests/census.

Inventory `XeenMapIdentity(23|28)`, `regionalJourney()`, `developerEventMap()`,
`kXeenAdmittedAreaTraversal`, regional/city providers, manifests and text guards.
Include JourneyContent/Flow, VertigoWorld, ActorApproach, Jump/LearnedSpellRules,
SaveState/RestoreGuard, BarrierFlow, EventInterpreter (`city`/ProtectionCheck),
RegionalRules/EventFlow (VertigoEntrance/Exit), `xeenRunDestination` and
World's outdoor-neighbor AlterCellFlag path. Classify admission restrictions
(remove), original identity/service rules (retain with justification) and developer
fixtures; the implementation report lists remaining sites and reasons.
Fold dedicated Vertigo entrance/exit into generic Event transitions unless DOS
requires otherwise, preserving script effects, text and actor reloads. Unverified
operands such as neighbor AlterCellFlag must surface a clear unsupported notice,
never an unhandled exception or a guessed write. Keep generic Bash/locks usable.

## Risks and acceptance

Risks: missed guards, tile/map confusion, partial transitions, double drain and scope creep.

- Unit tests for 0/1/2 skill holders and conditions, terrain, darkness/drain/zero
  boundary, destination resolution and atomic refusals with unchanged state/RNG.
  Test redraw/restore without drain and original travel/cancellation costs.
  Part 0 found no drain cells in the initial Clouds data, so drain is proven by
  synthetic fixtures (plus any Clouds Event that sets the drain bit, if found).
- A few original-data end-to-end tests combine outdoor crossing, Event/stair/mirror
  travel and sky/cloud/scripted falls. Focused tests cover every travel path above,
  all five towns, combat item Portal, generic Bash/locks and text in new maps.
  Include multi-tile interiors and actor reloads, plus one generic multi-map
  save/load round trip with no replayed Event, reward, time or RNG; no separate
  process-continuation witness per feature. Verify clear v15/newer-save rejection.
- Another town's service and `DoTownEvent` 8-11 show "not supported yet" without
  publishing a prelude or changing time/RNG; test safe unverified-operand refusal.
- Add dark maps/light-drain cells with engine probes to the census; update travel
  readiness/screens and regenerate whenever support changes. Keep calibration
  meaningful; census coverage is neither an admission gate nor playthrough proof.
- Follow [tools/AGENTS.md](../tools/AGENTS.md): affected targets/tests and
  `ctest --test-dir build-rel -L fast -j 8` during iteration; one passing complete
  suite per final production task and per batch of review/play-test fixes, using
  the Codex lightweight runner or Claude Code background command. Apply the
  protocol's stale-test rerun exception; no redundant suites or sandboxed runs.
- Independent plan review in another conversation precedes execution; independent
  implementation review precedes acceptance. Doc-only planning/Part 0 uses doc
  checks, no build/CTest. Review findings must be resolved before dependent work.
- Maintainer play-test samples terrain/borders, towns, a multi-tile dungeon and
  falls, with original keyboard/mouse UI. The handoff names a dark map with Light,
  the Rivercity Mountaineer teacher and the Part 0 mountain and wall-3/6
  Pathfinder crossings (Rialdo teaches Pathfinder). Save/load in a new
  area and check clear v15 rejection; cover travel families across these samples.
  Include pending `bb22c52`: Run's map-specific position/chance indoors/outdoors
  and its automatic-Event destination refusal; Mr Wizard from outside Vertigo
  returns there facing East, zeroes gems and says "not supported yet" in combat.
- Close after tests, review and formal play-test acceptance; update status/history/
  README, align roadmap Pathfinder wording with Part 0 and condense this plan.
  No implementation/commit/push without authorization; show the commit message first.
