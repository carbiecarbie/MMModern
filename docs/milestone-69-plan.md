# Milestone 69 plan - Generic per-map state

**Tier A, in three audited steps. Status: approved by the maintainer on
2026-10-15 after the independent plan review; implementation not started.**
Baseline `32bd9ec` (v0.1.3). Step 3 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems).

## Goal

Keep the live state of any Clouds or Darkside map in one generic form,
created, kept, written and restored on the original's lifetime rules, so that
the world Events of step 4 (`MoveWallObj`, `MakeNothingHere`, ...) and the
maps admitted in step 9 need no new save shape. Vertigo and F3 move onto it
with identical play. The save format changes (v13). No new Event opcode, map,
service or Darkside play is admitted here.

## What is saved today, and where it is special-cased

All of it lives in `XeenSessionWorldState` and `XeenSaveSnapshot`
(`XeenSaveFormat.cpp`, `XeenSaveState.cpp`):

- **F3 actors** `journey.actors`: a fixed table bound to map 23
  (`initializedMap`, `originalActorCount == MOB count`, ids `{23, i}`, 16x16
  positions, outdoor spawn closure), each with position, HP, activation,
  lifecycle, status and accounting.
- **Vertigo actors** `journey.vertigoActors`: an optional second table with
  `cityOriginalActorCount` and a `spawnedType` for script slots beyond the
  MOB (`applySpawn`, `stageVertigoActors`); the format hard-codes map 28,
  32x32 coordinates and MON type 0. Every reader branches on
  `camera.mapId == 28` (`regionalActors`, `XeenCombat`, `XeenBarrierFlow`,
  `XeenJourneyConsequences`).
- **Accounted monsters** and the treasure `pendingMask` and item `source`
  bits, validated as map-23 actor indices below 12.
- **Removed objects** and **disabled Events**: generic identities at the
  top level, but map 28 objects are rejected and map 28 Events require
  `vertigoActors`.
- **Moved or turned objects** (M57): top-level generic identities with
  position and effective flip; city entries require `vertigoActors`.
- **Barriers** (M50): top-level, per physical cell with the original
  word/attribute preimage, four faces limited to walls {1,3,6,9,13} and the
  unlocked bit; restore binds them to the town through `sampleCell(28, ..)`.
- **Explored tiles** (M58): top-level, per physical DAT tile, Clouds only, at
  most 512.
- **Head cache** (M59): primary map only, dropped on map load.
- Flags are party state; the camera may only be on map 23 or 28. Flips,
  spawn frames and activation views are presentation and are not saved.
- Both actor tables are kept live across the Vertigo transition
  (`publishTransition` swaps them unchanged); the "reset on exit" in
  [project status](project-status.md) is the exit script's own `Spawn`
  procedure, and the status line is corrected at closure.

## What the original persists per map

Reference: pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`
(`map.cpp`, `scripts.cpp`, `saves.cpp`, `files.cpp`, `shared/xeen/file.cpp`).
**DOS** = established from original data or program; **ref** = reference
only, not confirmed in DOS play.

1. **DOS.** A MOB record is x, y, id, direction (signed bytes), with no HP,
   wake or frame field (`XeenMapFormat::parseMob` on the original files).
2. **ref.** `Map::load` first calls `saveMaze()` for the map being left: EVT,
   the primary DAT and, indoors only, its eight neighbour tiles, and MOB are
   written into the in-memory save archive, which `File::open` reads before
   the original archive; `saveGameState` calls `saveMaze()` and writes the
   whole archive. Outdoors, changes to a neighbour tile's DAT are never
   written back (step 4 must respect this). Clouds map 85 (The Warzone) is
   never written back.
3. **ref.** MOB keeps object positions (Remove writes +128, which the signed
   MOB byte reads back as the disabled -128), monster positions and ids (dead
   monsters at -128,-128 by the same route, `combat.cpp` 1581; `cmdSpawn`
   may grow the table) and wall-item positions (`cmdMoveWallObj`). HP, attack
   state and frame are rebuilt from the MON record on every load, including
   `loadGameState`: a wounded survivor is at full HP after leaving and
   returning, and after loading a game.
4. **ref.** DAT keeps wall words (`cmdAlterMap`, the two-sided Bash and
   unlock changes, `openGrate` setting `0x80` on both cells) and cell
   surface and flag bytes (`cmdAlterCellFlag`) and the seen and stepped bits.
5. **ref.** EVT keeps opcode changes: `cmdMakeNothingHere` sets every record
   at the cell to opcode 0, `cmdAlterEvent` sets one line to any opcode,
   `cmdRemove` is object removal plus `MakeNothingHere`.
6. **ref.** HED is not written back; head changes last until the next map
   load ([quirk](original-quirks.md), AlterHed entry). Today's cache matches.
7. **ref, out of scope.** Load and save hooks: Clouds map 15 sets game flag
   56 when its first three monsters are gone, checked on load (all three) and
   on every exit and save (any of them); Darkside map 50 marks all tiles
   stepped; flag 25 clears the surfaces of maps 4/42/43. Recorded for step 9.

Only item 3's HP and wake rule changes today's behavior (decision 8).

## Design decisions

1. **One owner, `XeenMapState`, keyed by `XeenMapIdentity`** (side and
   number) in `XeenSessionWorldState`, replacing `_actors`, `_vertigoActors`,
   `_cityOriginalActorCount`, the disabled sets, `_barriers`,
   `_objectOverrides` and `_exploredTiles`. Per map: the monster table (every
   original or scripted slot: position, lifecycle, MON type for script slots;
   HP and wake state only until step A2 removes them); sparse object
   overrides (position, effective flip; -128 is removed); sparse wall-item
   overrides (position); sparse cell overrides keyed by physical tile and
   cell (effective word and attribute byte plus the original preimage,
   generalizing the barrier faces and unlocked bit); sparse Event opcode
   overrides (record index to opcode, 0 for `MakeNothingHere`); the explored
   bits. Wall-item and nonzero Event-opcode overrides have no producer in
   M69: they stay empty in play, are validated structurally and are covered
   by format tests only. The head cache stays a primary-map cache (item 6).
   Resources remain immutable and every override is validated against the
   loaded resource, as barriers are today.
2. **Lifetime follows the original.** A map's state is created when the
   party first enters it or a script first touches it (a neighbour tile's
   cell, an Event on another map), kept for the whole game, written in every
   save and restored before gameplay. Nothing is dropped on exit except the
   head cache. Map 85 is never retained (item 2).
3. **Generic access.** Combat, Shoot, Rest interruption, consequences,
   barriers, Events, scene composition and the automap read the camera map's
   state through one accessor; the `mapId == 28` branches go. Treasure
   sources become monster identities (decision 5). The Vertigo transition
   keeps its two destinations until step 9, but `publishTransition` and
   `XeenEventPublication` swap the whole per-map table, not named members.
4. **Validation stays as strict, map-agnostic.** Coordinates follow the
   map's geometry (16x16 outdoors, 32x32 indoors through the neighbour
   tiles); live actors stand on a sampled cell within base HP and occupancy
   three, outdoor actors keep the spawn closure; defeated and script slots keep
   the M50 canonical forms; cell overrides need a matching preimage; Event and
   object indices must exist; restore loads every map in the table and
   rejects contact at the camera, as today. Restore and map entry derive
   activation from the arrival view, as `prepareTransitionArrival` does, and
   the "quiet actor activation missing" check stays as the consistency proof.
5. **Save v13, schema 9 and content 14 retained** (maintainer decision,
   2026-10-15, as M61 Part C did; the envelope alone names the new layout and
   older envelopes get the existing "older build" rejection, no legacy
   reader). Complete v13 layout: header unchanged; party fields unchanged up
   to the game flags; the top-level `disabledObjects`, `disabledEvents`,
   `barriers`, `objectOverrides` and `exploredTiles` are removed; the Journey
   block keeps context, supplements, random, resistances, world flags,
   learned spells, poison, economy and awards, loses `initializedMap`,
   `originalActorCount`, `actors`, `vertigoActors` and
   `cityOriginalActorCount`, and its treasure block gains a `sources` list of
   up to 12 monster identities (side, number, record) that `pendingMask` bits
   and each item's `source` index into, each validated as a defeated,
   accounted actor of its own identity's map (today's restore only requires a
   source in view while the mask is nonzero, and that check stays); then the
   per-map table: map count, then per
   map its identity, monster table, object, wall-item, cell and Event
   overrides and explored bits, strictly ordered by key; then the head cache
   and the name as today. Bounds: 512 maps, 107 monsters per map, objects
   and wall items up to the MOB counts, 256 cells per tile, Event overrides
   up to the record count, the 4 MB cap unchanged; the fixed schema-9 size
   bound becomes a bound computed from the decoded counts. Restore stays
   exact: no replayed Events, rewards, time or random draws. A generic
   round-trip test (capture, encode, decode, restore, capture again, compare)
   covers every table with state on both maps.
6. **Darkside identities are representable, not loadable.** The side byte
   exists already; the Clouds-only loader rejects a Darkside entry at restore
   until Darkside resources load.
7. **Deltas, not whole-map copies** (maintainer decision, 2026-10-15). The
   original writes whole files per visited map; the save stores sparse
   overrides plus the full monster table, observably equivalent and small
   (about 1.5 KB per map with every monster moved). Item 6 and this
   difference go to [original quirks](original-quirks.md) in the same change.
8. **Monster HP and wake state follow the original's map load** (maintainer
   decision, 2026-10-15, on one condition). Item 3 is reference-only and
   changes existing behavior: MMModern keeps live HP and activation across
   the Vertigo transition and in the save today. Before any decision 8 code
   (step A2), a read-only static check of the map-load routine in
   `WORLD/XEEN.DAT` (calibrated as [evidence](rules/evidence.md) requires,
   citing file offsets; work copy outside the repository; no bytes or
   decompiled code committed) settles two facts: whether every monster's HP
   is refilled from the MON record on every map entry and on loading a save,
   and whether its wake state is cleared at the same points. If DOS matches
   the reference, HP and wake state are transient: rebuilt on every map load,
   absent from the save, and a wounded survivor is at full HP after leaving
   and returning, and after loading a game. If DOS differs, DOS is followed.
   Either way the confirmed rule, with its offsets, is recorded in
   [original quirks](original-quirks.md) and in the monster recovery section
   of [combat rules](rules/combat.md#initiative-block-run-and-monster-recovery)
   in the same change, and the maintainer's DOSBox check (wound a Slime,
   leave Vertigo, return and count the hits to kill it; the same across a DOS
   save and load) is the play-test corroboration. This is original game
   behavior on map load, not a replay: exact restore still forbids replaying
   Events, rewards, time or random draws.
9. **Checkpoint invariant under decision 8.** A run resumed from a save
   equals the uninterrupted run only when, at that save, every live monster
   is at base HP and not woken; otherwise the resumed run legitimately
   diverges (the reloaded monster is healed and asleep), and from the point
   where such a monster is met again its RNG draws and time differ. Every
   checkpoint a test resumes from must satisfy the invariant over the camera
   map's live monsters, asserted by the witness at the checkpoint from the
   live session state. Waking is sticky in the engine and in the reference
   (only `Spawn` and a map load clear it), so stepping out of sight never
   restores the invariant: routes are tuned by killing every woken monster
   of the camera map before the checkpoint. In A1, which still keeps
   activation across transitions, the assertion is a logged check; from A2
   it is fatal.

## Open questions for the maintainer

None at this revision: HP and wake state (decision 8), deltas (decision 7)
and the version numbers (decision 5) were settled on 2026-10-15. The
decision 8 static check may reopen the map-load rule only.

## Steps and order

Each step ends with its own complete suite and M44 audit, so every audit has
a single cause.

**A1 - generic owner (save stays v12, behavior unchanged).** Introduce
`XeenMapState`, move Vertigo and F3 onto it with HP and activation still
persisted, make every reader generic, and keep capture and restore projecting
the generic state into the v12 layout (MOB and EVT state on any map but 23
and 28 is a logic error; cell overrides and explored bits stay keyed by
physical DAT tile, such as Vertigo's 109-111, as v12 and the original's
per-tile DAT files do, maintainer decision 2026-10-15). Add the fourth M44 scenario and its witness extension (Tests
below). Extend `mmodern_save_dump`'s visitor and the audit projection so the
moved fields keep comparable paths. Acceptance: the three existing scenarios
show zero differences in traces, decoded fields and bytes; the fourth
scenario's digest is recorded at the end of A1 and that build is preserved as
the next baseline.

**A2 - decision 8 alone.** First the static check, recorded with its
offsets before any code. Then remove HP and activation from the live
projection (the v12 fields are written as base HP and false and ignored on
restore), rebuild them on every map load and game load, and derive activation
from the arrival view. Before changing tests, dump HP and activation at every
existing checkpoint with `mmodern_save_dump` on the preserved A1 build and
list which tests move a checkpoint or change an expectation under decision 9.
Acceptance: the audit shows exactly the enumerated differences (the projected
HP field only where a wounded survivor is alive at the checkpoint, the
projected activation field wherever any monster is woken, in view or not, and
in the fourth scenario the differences listed below), each explained, and
nothing else; the three existing scenarios show no trace difference unless
the dump proves a wounded or woken monster is met again after a checkpoint.
For the roundtrip scenario A2 expects: the B save's activation field for
Orc #11; #11's activation flag in the trace from the first return to F3 until
the second entrance view wakes it again; and full divergence from the second
return to F3 onward (the wounded Orc healed and the Orcs woken after B
asleep), including downstream RNG draws and time. Its A2 baseline is the
preserved A1 build `D:/M44-audits/m69a1-a2-baseline`.

**B - save v13.** Replace the projection with the generic encoding, delete
the 23/28 cases from `XeenSaveFormat::validate` and
`XeenSaveState::validateJourneyValues`, port the layout tests, run the
format-only audit of all four scenarios (traces identical, common decoded
fields equal apart from layout, every byte delta explained), obtain approval,
replace the digests, then the implementation review and the play-test.

## Tests

**Tests that depend on the save layout**, and how each is updated:

- **Literal wire bytes and offsets**, rewritten in B against the v13 layout
  of decision 5, each literal naming its field: `XeenSaveFormatTests` cases
  `wireContract`, `asymmetricBase`, `regionalCityWire`,
  `mainlandTopologyCounts`, `serviceEconomyWireContract` (offset -5043),
  `purchaseDepletedWireContract` (8467 + 4 + 21*46, day offset),
  `templeWireContract`, `skillExplorationWire`, `headFlagsWire`,
  `worldEffectsWire`; `legacyGolden` stays as a rejected v2 sample.
- **Recorded digests**, replaced only after each step's audit and approval:
  `XeenM44BaselineDigests.h` (header comment gains the fourth scenario),
  `xeen_m44_scenario_*`, the scenario table in `tools/m44_digest_audit.py`
  and its unit test, [tools/AGENTS.md](../tools/AGENTS.md) ("three" becomes
  four scenarios).
- **Decoded-field tooling**, with paths added for the per-map table and old
  paths mapped in the audit projection: `XeenSaveDump.cpp` and its self-test.
- **Snapshot builders and helpers**, a mechanical port in A1 with no
  behavior change: `XeenSaveTestSupport.h` (`currentWireSnapshot`),
  `XeenTrainingTestSupport.h`, `XeenRegionalJourneyTestSupport.h`,
  `XeenRegionalCombatTestSupport.h`, `XeenSaveGameplayTestSupport.h`,
  `XeenM40Evidence.h`, `XeenFreshStartControls.h`,
  `XeenResourceColdOriginalControls.h`, `XeenConsequencePhysicalControls.h`,
  `XeenConsequenceReviewControls.h`, `XeenDisengagementTestControls.h`, and
  the tests filling `journey.actors`, `vertigoActors`, `barriers`,
  `objectOverrides`, `exploredTiles` or the disabled sets:
  `XeenSaveStateTests`, `XeenSaveFlowTests`, `XeenEncounterSaveTests`,
  `XeenCompletedRestoreTests`, `XeenRegionalPersistenceTests`,
  `XeenJourneyPersistenceTests`, `XeenCombatPersistenceTests`,
  `XeenWorldEventTests`, `XeenWorldTests`, `XeenMappingGameplayTests`,
  `XeenExplorationTests`, `XeenM60ExplorationTests`, `XeenBarrierTests`,
  `XeenBarrierOriginalTests`, `XeenCityEventPublicationTests`,
  `XeenCdEventOriginalTests`, `XeenM61PartBTests`, `XeenTempleOriginalTests`,
  `XeenRestOriginalTests`, `XeenRegionalActionOriginal`,
  `XeenConsequenceOriginal`, `XeenDeathOriginalTests`,
  `XeenTrainingFlowTests`, `XeenTrainingOriginalTests`,
  `XeenIndoorComposerTests`, `XeenMapViewTests`, `XeenJumpRulesTests`,
  `XeenWholeMapFoundationTests`, `XeenPartyInterfaceTests`,
  `XeenInterfaceTests`, `XeenServiceDialogTests`,
  `XeenCombatCastingControlTests`, `XeenEquipmentOriginalTests`,
  `XeenExpeditionTests`, `XeenExpeditionGameplayTests`, `XeenJourneyTests`,
  `XeenJourneyEventTests`, `XeenMonsterAbilityGameplayTests`, and the process
  tests that read the actor fields: `XeenM37CliWitness`, `XeenM39CliWitness`,
  `XeenM40CliWitness`, `XeenSmithProcessTests`, `XeenServiceDayProcessTests`,
  `XeenCombatCastingProcessTests`.
- **Fixtures that wound or wake a monster through the snapshot**
  (`XeenExpeditionTests` HP 30, `XeenDisengagementTestControls` HP 16 and
  activated, `XeenCombatCastingProcessTests` saved HP 33 or 25, and any the
  A2 dump finds): under decision 8 the save cannot carry that. In A2 they
  wound or wake the monster after restore through the live session (the
  combat and approach rules, or the existing test-only world access), never
  through the snapshot, and the saved-HP expectation of the casting process
  test is restated as a live expectation.
- **Self-consistent byte comparisons**, no edit expected in A1 and B; a
  failure there is a restore defect, never a layout update: the checkpoint
  witnesses (`XeenM38`, `XeenM41`-`XeenM43CliWitness`,
  `XeenVertigoProcessTests` uninterrupted-versus-resumed bytes,
  `XeenM43ProcessTests`, `XeenM43DepletedProcessTests`,
  `XeenPurchaseProcessTests`, `XeenTrainingProcessTests`,
  `XeenCombatClockProcessTests`), `XeenSaveCliTests`, `XeenSaveSdlTests`,
  `XeenSaveFileTests`, and the developer presets `xeen_test_save_*`
  (`tools/XeenTestSave.cpp` edits party fields only). In A2 the checkpoints
  that resume (the M43 witness A..F, the Vertigo process routes, the
  disengagement controls) are checked against decision 9 from the dump; a
  checkpoint holding a wounded or woken monster is moved, or its resumed
  expectation restated, with the dump as the justification.

**Fourth M44 scenario `xeen_m44_scenario_roundtrip`** ("vertigo-f3-roundtrip",
seed 3626689381), added and recorded in A1 (`cfdcec34...`, maintainer
approval 2026-10-15). Route as implemented: prepared Journey at F3; kill the
Orc the start view wakes before any map change; walk to `(10,13)`, enter
Vertigo, kill a Slime and the two that come through the grate it bashes,
search a crate (object override and two disabled search records),
checkpoint A; exit to F3, checkpoint B; wound an F3 Orc and Run, enter Vertigo
and exit again, then kill that Orc; save the final file. The wounded monster
is an F3 Orc, not a Vertigo Slime (maintainer decision): the Vertigo exit
script respawns and heals every city slot while flag 9 is unset, so a city
wound could never show decision 8. At B the decision 9 check is violated in
A1 by Orc #11, woken by the unavoidable entrance view and unreachable; A2's
reload clears it. The witness gains
a checkpoint-resume mode like the M43 witness (environment stage plus resume
flag, remaining steps replayed from the checkpoint), and the scenario test runs
the uninterrupted route, a resume from A and a resume from B in separate
directories (`resume-A/`, `resume-B/`) so the audit's checkpoint glob sees
only the uninterrupted run; all three finals must be byte-identical, and the
final reloads and re-saves exactly. Its digest is recorded at the end of A1
with the maintainer's approval (a fresh baseline, no earlier build to compare)
and compared in A2 and B like the other three.

**Other acceptance tests:** rules tests for `XeenMapState` (creation on first
touch, retention across exits, map 85 exclusion, decision 8, bounds, preimage
validation, strict ordering); the round-trip test of decision 5; the existing
M50, M57 and M59 original-data suites unchanged; one original-data test that
a scripted Vertigo `Spawn` slot and an F3 defeat survive a Vertigo-F3-Vertigo
loop with a save at each stop; rejection of a v12 save and of a Darkside
entry; `xeen_coverage_original_guard` unchanged.

## Acceptance

- Complete `build-rel` suite through the runner in
  [tools/AGENTS.md](../tools/AGENTS.md) after each step; fast suite and the
  four M44 scenarios on `build-m44` at closure.
- An M44 audit with the maintainer's approval before any digest replacement:
  A1 zero differences, A2 the enumerated decision 8 differences, B format
  only.
- Independent plan re-review (now) and implementation review (after B).
- Maintainer play-test on `build-m44`: new game, fight and search in Vertigo,
  save, exit to F3, fight, save, load both saves, return to Vertigo; confirm
  objects, grates, dead monsters, explored cells, and the confirmed decision 8
  rule for a wounded monster.

## Risks

- **Digest churn.** Every save changes in B; the per-step audits and the
  dump projection are the only defence against unexplained failures.
- **Exact restore.** Moving actors behind one accessor can change publication
  order silently; A1's zero-difference audit and the untouched checkpoint
  witnesses are the proof.
- **Decision 8 is a behavior change** resting on reference evidence until
  the static check lands; it must be recorded as a settled rule with its
  evidence, and decision 9 moves checkpoints that today assume load
  transparency.
- **Size and time.** Visited maps grow the save linearly (deltas keep it
  small) and restore loads every map with state; a synthetic test with all 128
  Clouds maps populated keeps restore under a second.
- **Vertigo's reset order.** The exit script respawns slot 12 at `(4,12)`
  and each later slot at the next MOB record's position; it is reproduced
  from the data and is recorded in [original quirks](original-quirks.md) in
  step B.
- **Scope creep.** New opcodes (step 4), new maps (step 9) and Darkside play
  stay out; the Vertigo transition keeps its two destinations.

## Out of scope

`MoveWallObj`, `MakeNothingHere`, `AlterCellFlag`, `AlterMap` beyond the
barrier path, `JumpRnd`, mirrors and password doors (step 4); admitting any
map (step 9); the load and save hooks of item 7; Darkside gameplay; reading
original or ScummVM saves.
