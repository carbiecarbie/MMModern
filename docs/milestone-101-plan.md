# Milestone 101 plan - Darkside start

**Tier A. Status: planned (not started).**
Starts roadmap
[step 5](roadmap.md#next-arc-release-quality-and-the-darkside-start), the
Darkside start. Baseline: `d4c9e27` (release 0.12.0), equal to
`origin/main`.

## Goal and scope

A party walks into a Clouds Pyramid and arrives at Castleview on Darkside, as
in the World of Xeen CD. It can walk, talk and fight there with every
mechanic MMModern already has, save and load anywhere on Darkside, and
return through a Darkside Pyramid. Whatever Darkside uses that is not
implemented yet says "not supported yet" and keeps the game running, as on
Clouds. The engine holds both sides' state, and the DOS export carries
Darkside progress, so DOSBox and MMModern can start from the same Darkside
state for the content milestones that follow.

Out of scope, each kept as today or refused with the notice:
- Darkside mechanics from the [census](coverage.md#darkside-largest-gaps)
  (ChooseNumeric, ExchObj, FallToMap, GotoRandom, the Darkside GiveEnchanted
  items and the rest); they follow in census order, one milestone each.
  Darkside object pictures come in with the generic resource choice
  (maintainer, 2026-10-30, after the Part A review), each newly admitted
  sprite rendered and inspected;
- the services of the Darkside towns (Inn, Tavern, Temple, Bank, Smithy,
  Guild, Training, Dwarf town), with their original values; proposed as M102;
- the four Clouds entrances that need Darkside keys (maps 113-128, which mix
  Clouds and Darkside resources), the Sphinx and FlipWorld (opcode 59)
  (maintainer, 2026-10-30); they keep refusing;
- the Darkside and World endings (`showdown`, `genesis`) and Darkside and
  World completion (PTY 606..609);
- importing DOS saves.

## Current state

At `d4c9e27`:
- `XeenMapIdentity` already carries a side, and the save writes it, but the
  envelope accepts only side 0 (`XeenSaveFormat.cpp:480`), the restore
  refuses a Darkside camera or map state (`XeenSaveState.cpp:69-73`), and an
  active member must have side 0 (`XeenSaveFormat.cpp:342`).
- Game flags are one bank of 256 (`XeenGameFlags.h`); quest items and quest
  flags are Clouds-only types (`XeenCloudsQuestItems`,
  `XeenCloudsQuestFlags`, `XeenParty.h:102`, `:133`). World flags are 128
  (`XeenParty.h:152-154`), serialized inside the Journey block
  (`XeenSaveFormat.cpp:428`). Event values refuse every game flag, quest
  flag and quest item outside Clouds, and mode 107 (character flags)
  everywhere (`XeenEventValues.cpp:285-361`, `:530-574`, `:663-689`).
- The Journey rules profile is `WorldOfXeenClouds` only
  (`XeenGameplayContext.h:9`, `:23`), required by
  `XeenGameplayContext.cpp` and `XeenJourneyRules.cpp:30`, and written as
  profile byte 0 (`XeenSaveFormat.cpp:358`, `:406`, `:521`).
- Explicit and implicit Clouds assumptions block Darkside play: initial
  geometry, HED and MON bound to Clouds (`XeenMapLoader.cpp:41-60`,
  `CloudsMapComposer.cpp:82`); drawing and object visuals
  (`XeenAssetSource.cpp:143`, `XeenObjectVisual.cpp:15-34`); NPC
  conversations (`XeenEventInterpreter.cpp:866-868`); outdoor movement
  (`XeenMovement.cpp:133`, `:145`); town actions
  (`XeenRegionalRules.cpp:413-417`); Clouds towns in casting
  (`XeenCastingUi.cpp:17`, `XeenLearnedSpellRules.cpp:40`); the Clouds death
  presentation (`XeenDialogView.cpp:192-213`); Clouds-only map music
  (`XeenAudio.cpp:102`, `:347`, `:386`; `XeenGameplay.cpp:600`); and
  services, entrance scenes, combat spell attacks and the bridge's gameplay
  reads. A first list is in `build-rel/m101-inventory.txt` (not committed);
  Part A's inventory replaces it.
- Already two-sided and saved: shop wares (`XeenServiceEconomy.h:12`,
  `XeenSaveFormat.cpp:442-447`), the character side byte and Beacon side
  (`XeenSaveFormat.cpp:176-177`). The bridge already reads the DARK New Game
  archive (M97) and the Darkside scene archives (M93).
- The Clouds Pyramid (DoTownEvent action 12, maps 3, 10, 11, 16, 23 and 49)
  says "not supported yet"; it is the only Clouds census gap. Darkside has
  two Pyramids, maps 29 and 52. The Town handoff has preconditions a Pyramid
  must meet (`XeenEventInterpreter.cpp:780-800`).
- `mmodern_dos_export --place` already puts a party on Darkside; it refuses
  Darkside history and leaves Darkside fields at their New Game values. The
  party writer projects only the first quest-flag bank
  (`XeenCharacterFormat.cpp:272-273`).

## Evidence

- **Reference** (pin `6814ee9`).
  - `PyramidLocation::show` (`locations.cpp:2557-2600`): on Clouds, map 49
    leads to Darkside map 52 `(2,2)` and every other Pyramid to map 29
    `(25,21)`, both facing North; on Darkside, map 52 leads to Clouds map 49
    `(7,14)` South and every other Pyramid to map 23 `(8,10)` North. It
    loads the map before writing the position; `map.cpp:680-683` marks the
    arrival and clears input. It does not rewrite the character side.
  - `Map::load` (`map.cpp:690-756`) selects `dark.dat`, `dark.mon` and
    `darkpic.dat` for ordinary Darkside maps and derives the Town Portal
    side from the loaded side (`:685-691`; consumed at `spells.cpp:1214`).
  - Darkside map music draws `getRandomNumber(6)` indoors and outdoors
    (`map.cpp:902-911`, `:935-945`).
  - Party state (`party.h:232-236`, `party.cpp:331-337`): game flags 2 x 256
    and quest flags 2 x 30 are selected by side (`party.cpp:959`, `:1143`,
    `:1225`, `:1478`); the 85 quest counters are one shared array indexed by
    item ID minus 82 (`party.cpp:967-968`, `:1235`); character flags are
    30 x 32 by roster ID, shared; world flags allocate 129 slots but
    serialize 128. The yearly reset clears Clouds game flags 55, 155, 222 and
    231 whatever the side (`party.cpp:1624-1627`).
  - `cmdPlayCD` adds 30 to Darkside tracks of a flattened disc
    (`scripts.cpp:1503`); MMModern addresses the installed discs
    (`XeenEventFlow.cpp:1757`, `XeenCdAudio.cpp:62`).
- **DOS save layout** ([DOS save rules](rules/dos-saves.md)): Darkside game
  flags at PTY 691..722, world flags 723..738, quest flags 739..746 (the
  Darkside bank starts at bit 30, not on a byte boundary), quest items
  747..831, character flags 1408..1527, and DARK map history.
- **DOS play** (maintainer, 2026-10-29): a party placed at Castleview loaded
  there and could walk, talk and fight with only its position written.

## Maintainer decisions (2026-10-30)

- **Scope:** the two-sided foundation and the original Pyramid in both
  directions, so that Darkside is reached in ordinary play; developer
  presets as well.
- **Later:** the Darkside-key entrances, the mixed maps 113-128 and
  FlipWorld.
- **Tier A**, one plan, two parts, one play-test after Part B.
- **Independent plan review** (2026-10-30,
  `D:/Projetos/MModern/MModern-re/m101/plan-review.md`): approve with
  changes, all eleven applied here.
- **Darkside music on Load** (amendment, 2026-10-30, after Part A's evidence
  stopped on it): the save keeps the selected song; Load does not draw.
- **Part B** (amendment, 2026-10-30, after Part A): the Pyramid evidence is
  settled; music on the return from a location moves to M102.
- **No disc-change prompt** (maintainer, 2026-10-30, approved deviation): a
  DOSBox capture of a Clouds Pyramid crossing
  (`D:/Projetos/MModern/MModern-re/m101/dos-pyramid-disc-prompt/`) shows the
  "Please Wait..." box and then the installed CD program's prompt "Press
  CTRL+F4 and ENTER to continue...", which waits for Enter before the
  Darkside arrival. MMModern reads both discs, so it shows no prompt and
  waits for nothing there or at any other disc change (Load, travel spells);
  every other effect of the crossing is unchanged. Part B records it in
  [original quirks](original-quirks.md) beside `dev-no-please-wait`, with
  the capture and the program's disc check as evidence.

## Evidence to settle in Part A, before code that depends on it

Static evidence cites calibrated `WORLD/XEEN.DAT` offsets and a confidence;
DOSBox evidence uses paired M97 presets with captures and `--diff`. Notes stay
in `D:/Projetos/MModern/MModern-re/m101/`. Unsettled or contradictory
behavior stops before Part B. A difference from the reference is recorded in
[original quirks](original-quirks.md); DOS wins.
- **Music.** Settled (2026-10-30, notes `evidence-notes.md`): every Darkside
  map load, Load included and whether music is on, off or the same song,
  makes one gameplay-RNG draw `U(0,6)` (outdoors `0x102b1..0x102bd`, indoors
  `0x1048d..0x10499`, wrapper `0x45b9a`), from six tables. The return from a
  location replays the previous song without a draw (`0x1d75a..0x1d773`) and
  then reloads its map, which makes the ordinary map-load draw
  (`0x1d788`).
- **Resources.** The DOS resource choice for each ordinary Darkside map.
- **Pyramid.** Settled (2026-10-30, `evidence-notes.md`, all eight source
  sites in `pyramid-original-sites.json`): action 12 shows no
  scene and charges no time (DOS's map-load "Please Wait..." box and its
  disc-change prompt are approved deviations, above); it loads the destination map (marking it stepped
  and clearing input), then writes the position and facing, settles the source
  Event as handled and returns to the main loop, so the destination's Events
  run on the next ordinary pass. It writes no calendar or character side byte
  and makes no draw of its own.

## Design decisions

- **Owners.** No new owner. The existing `XeenGameFlags` owner widens to two
  banks rather than moving. The party/roster owns the quest and character
  state; the world owns Darkside map state beside Clouds map state, keyed by
  `XeenMapIdentity`.
- **State.**
  - Game flags: two banks of 256, selected by the camera's side.
  - Quest flags: two banks of 30, selected by the camera's side.
  - Quest items: the 85 shared counters, indexed by item ID minus 82 on
    either side; never selected by side. Megacredits stays item 26.
  - Character flags: 32 per roster record, shared, for Event mode 107 on
    both sides.
  - World flags: keep the 128 shared durable flags and the documented Auto
    Notes 128 -> Clouds quest flag 0 alias
    ([quirk](original-quirks.md)). The reference allocates 129 slots but
    serializes 128; M101 adds no 129th flag.
  - Cross-side rules stay explicit: the yearly reset clears Clouds flags 55,
    155, 222 and 231 even on Darkside.
  - Preserve the two-sided wares, the shared bank, calendar and effects, the
    character side and saved town, and the Beacon side. For admitted maps,
    derive the load, resource and Town Portal side from the map identity;
    keep these concepts separate so the deferred mixed maps cannot be
    misclassified.
  - The prior map keeps the M97 projection's New Game value while Darkside
    sign-in is deferred; the DOS save rules say so. The new state starts
    from the right DOS New Game layout, never by reading the short Clouds
    PTY as a full DARK PTY.
- **Journey profile.** Part A resolves profile 0 as the shared World of Xeen
  rules profile, audits every profile-dependent consumer (`XeenRestFlow.cpp:51`,
  `:182` among them) and uses the map identity for side-specific behavior.
- **Inventory.** It covers every `_ccNum` and `_loadCcNum` use in the pin and,
  in MMModern, every `XeenSide::Clouds` test, `WorldOfXeenClouds`,
  `XeenSceneArchive::Clouds`/`Current`, default-side numeric identities,
  `xeen`-prefixed file names, single-bank consumers and every existing flag
  consumer. Each site gets a recorded disposition, verified against the
  reference or DOS rule: a generic two-sided rule; a Darkside table from the
  original resources or program; or a "not supported yet" refusal when its
  Darkside values are later content. Ordinary loading, supported object
  pictures, NPC dialogue, movement and combat are admitted, not refused.
- **Loading.** A Darkside map loads from DARK.CC with the resources DOS
  selects, starting at its New Game state from the DARK New Game archive.
  A mixed map 113-128 is refused clearly.
- **Music.** A Darkside map arrival makes the DOS draw from the gameplay RNG
  and picks from the DOS tables, whether audio is on, off, unavailable or
  absent. Restoring a save makes no draw: the save holds the selected song,
  and Load plays it (maintainer, 2026-10-30). DOS draws again on Load, as
  part of its clock-mixed RNG that MMModern's exact restore already does not
  follow; the difference is recorded in
  [original quirks](original-quirks.md) with a `**ScummVM:**` tag if the pin
  also redraws.
- **CD tracks.** Keep addressing the installed discs; do not add 30 blindly.
- **Pyramid.** DoTownEvent action 12 moves the party between sides as DOS
  does, through the existing travel and arrival path, meeting the Town
  handoff's preconditions. It publishes once, settles the source Event once
  and runs the evidenced arrival work once.
- **Party side.** Characters keep their side byte; what changes it in DOS
  (Tavern sign-in, creation) waits for the Darkside services.
- **Spells and returns.** Town Portal, Lloyd's Beacon, Mr Wizard and the
  return after the party's death follow the side as DOS does; any whose
  Darkside tables belong to M102 is refused before it changes state.
- **Save v23** (v22 -> v23).
  - The envelope accepts both sides; its side byte equals the camera's side,
    and disagreement or an unknown side is refused before publication. The
    restore accepts a Darkside camera and Darkside map states.
  - The party block adds the second game-flag and quest-flag banks, the
    full 85 counters and the character flags.
  - The save adds the selected Darkside song (none on Clouds), outside the
    Journey block; restore checks it against the camera's map and table.
  - Journey schema 9 and content 14 stay only with an explicit audit that
    the Journey layout is unchanged; any added Journey field or different
    profile contract amends this plan before implementation.
  - Restore is exact: no Event, reward, time or draw is replayed. v22 saves
    are refused with the existing message; no v22 reader is kept.
- **Format audit.** An audit-only v22 writer reproduces all four current M44
  hashes; v23 is then compared field by field after removing only the
  documented inserted fields and framing. The continuation checks and RNG
  counts stay; the audit writer is deleted afterwards. Separately, nonzero
  first and last entries of every new array, mixed-side characters and
  Beacons, and same-number maps on opposite sides round-trip. The maintainer
  approves the new digests.
- **DOS export.** The exporter writes both game-flag banks, the quest flags
  at their DOS bits, all 85 counters, the character flags and DARK map
  history from v23 state, and stops refusing a Darkside input.
  `--resave-check` still accepts only the name and play time.
- **Census.** `mmodern_census` uses the real Darkside admission; the coverage
  report is regenerated in each part and its guards updated.
- **Stop rules.** Stop for a Journey schema or content change, a new owner,
  a draw not accounted for by the reviewed DOS timing and RNG design, a
  digest difference the audit does not explain, a site that needs Darkside
  content beyond a refusal to reach Castleview, evidence that contradicts
  this plan, or scope beyond it. If DOS evidence changes the music or RNG
  design, amend the plan first.

## Risks

- Renaming the quest and flag types touches many files and tests; a
  mechanical change with a clear diff, done first.
- A Clouds assumption missed by the inventory would silently use Clouds
  values on Darkside; the wide inventory and a walk of Castleview's cells and
  Events with distinct nonzero Clouds banks detect leaks.
- Treating shared state (quest items, character flags, the yearly reset) as
  side-local is as wrong as missing a side; tests cover both directions.
- Castleview runs into many partial mechanics at once; every one must refuse
  without corrupting state.
- Darkside music adds draws; a wrong timing would change gameplay RNG.
- Every v22 save stops loading, including the maintainer's play-test saves;
  presets regenerate as v23.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| A | Inventory and DOS evidence notes (outside the repository); two-sided state; profile; Darkside map loading and music; Event values on both sides; save v23 and format audit; census; preset `m101-castleview`. All inventory sites needed to render, walk, talk, fight and save Castleview are resolved; every deferred service and B-only transition (Pyramid, spells, defeat) is refused before it changes state, tested through live input | Independent implementation review, complete suite, digest approval |
| B | The Pyramid both ways; travel UI names and destinations, cross-side Beacon and defaults, Mr Wizard, defeat presentation and recovery; DOS export of Darkside progress; a list of the actions left refused for M102; quirks and rules docs; presets `m101-pyramid-clouds` and `m101-pyramid-darkside` | Independent implementation review, complete suite |
| Play-test | One session after Part B | Maintainer acceptance |
| Closure | Status, history, roadmap | Plan condensed |

M102 still owns the refused Darkside town actions 0 (Bank), 1 (Smithy),
2 (Guild), 3 (Tavern/Inn sign-in), 4 (Temple), 5 (Training) and 7 (Dwarf
Town), including their original values and location-return music. Refusals
precede service state changes. Mixed maps, key entrances, FlipWorld and
endings remain deferred as listed above.

## Acceptance

1. **State.** Game flags (2 x 256) and quest flags (2 x 30) are isolated by
   side. All 85 shared quest counters work through item IDs 82..166 on
   either side, and character flags by roster ID, preserved across side
   changes and party reordering. Boundary indices, untouched other members
   and Megacredits (item 26) are covered; shared state never follows the
   camera side. The yearly reset clears the Clouds flags on Darkside.
2. **Loading.** Every ordinary Darkside map loads from the original data
   with the DOS resources; maps 113-128 are refused clearly. Rendered
   Castleview frames indoors and outdoors are inspected. Indoor and outdoor
   music selection, Pyramid arrival, return and restore are tested with
   audio enabled, disabled and absent: arrival draws once and picks the DOS
   row; restore draws nothing and plays the saved song. Part A exercises
   arrival selection at the destination-load owner and restore through live
   Load; Part B covers arrival through live input (the Pyramid and the travel
   spells). The return from a location is unreachable on Darkside while its
   services refuse, so M102 tests it (amendment, 2026-10-30).
3. **Pyramid.** From each Clouds Pyramid the party arrives at its DOS
   destination on Darkside, and back from each Darkside Pyramid, with no
   scene, no time charge and no disc-change prompt (approved deviation);
   map 49 <-> 52 included. A destination or
   resource failure or an interruption leaves the committed source intact; a
   crossing publishes once, settles the source Event once and runs the
   arrival work once. Returning to a visited map keeps its history.
4. **Save.** A Darkside save restores the full state, and its next normal
   action matches an uninterrupted run, including RNG state and count,
   calendar, rewards and Event overrides; a load on an Event cell replays no
   entry work. The witness uses distinct nonzero values in both game banks,
   quest bits 29, 30 and 59, quest counters 34, 35 and 84, roster flags 0
   and 29, and same-number map edits on both sides; it re-saves
   byte-identically. v22, malformed and side-mismatched saves are refused
   without partial publication. This extends the generic round-trip test.
5. **Export.** The same witness exports a pair whose CHR, PTY and both
   archives hold those values at their DOS places, with padding untouched
   and unrepresentable counters refused. DOS loads it in DOSBox at the same
   place. The existing resave check passes on an already-stepped
   destination (or a party without Cartographer), with name and play time
   the only differences. The M97 checks still pass.
6. **Original data.** From preset `m101-pyramid-clouds`, enter the Pyramid,
   walk Castleview, trigger an Event that sets a Darkside flag, save, load,
   and return; a "not supported yet" mechanic keeps the game running.
7. **Suite.** The complete suite passes; coverage is regenerated.
8. **Play-test.** Before the single session after Part B, the agent inspects
   original-data renders and the available DOS and audio captures, and lists
   only the visible or audible equivalence and input-feel checks they leave
   unresolved, saying why for each. The session is prepared as
   [play-tests](playtests.md) describe, with coordinates. If everything is
   settled, it is one short Pyramid, Castleview and return check.
