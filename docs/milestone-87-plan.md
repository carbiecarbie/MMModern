# Milestone 87 plan - The Inn and a party of one to six

**Tier A. Status: Part 0 done; Part A not started.**
Roadmap [step 14](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones, after [M86](milestone-86-plan.md). Baseline: `f77720f`,
equal to `origin/main`.

## Goal and scope

As in DOS, Sign In at the Vertigo, Rivercity or Shangri-La tavern lodges the
party, saves the game and opens the Inn: the original screen, with the Inn
song, lists the characters lodged in that town; members are added up to six
and removed, and Exit saves again and returns the party to the tavern door.
Loading a save made at Sign In reopens the Inn. Dismiss (D) removes a member
during exploration, and the next save lodges that character in Vertigo. The
game then plays with any one to six roster characters in any order: combat
(with the original's count-dependent monster target draws), spells (Poison
Volley shoots once per active member), Events, Rest, services, travel,
treasure and saves.

Delete and Create say "not supported yet" until M88, which manages roster
slots. Out of scope: Darkside lodging and its Inn song, and every other
roadmap step. Planning does not authorize implementation; independent plan
and implementation reviews, the complete suite and a maintainer play-test
are required.

## Current state

At `f77720f`:
- **Membership is fixed.** Journey validation requires exactly the original
  six roster IDs `kXeenCombatOwners` {0,18,14,11,1,6} and serialized counts
  of six (`XeenJourneyRules.cpp:32-34`); later checks and approach iterate
  that constant (`XeenJourneyRules.cpp:52, 75`,
  `XeenActorApproach.cpp:318, 333, 401`; 33 test files use it).
  `activeOrder()` throws for other sizes (`XeenParty.h:75-77`), restore
  forces both counts to six (`XeenSaveState.cpp:124`), and two Event
  operations refuse other sizes (`XeenEventInterpreter.cpp:1308, 1771`).
  The save layout already holds up to six IDs (`XeenSaveFormat.cpp:322`).
- **Six-lane consumers.** `XeenPartyOrder` and `XeenConsequenceCharacters`
  and `Inputs` are six-element arrays (`XeenCombatInputs.h:32-33`,
  `XeenCombatRules.h:59-60`); 84 `<6` loop lines run in combat, casting,
  consequences, Rest, barrier, approach and Events; approach and Event
  opportunity paths pass mask `0x3f` (`XeenActorApproach.cpp:614-615`,
  `XeenEventFlow.cpp:513, 813, 892-893`); dialogs expose six F-key and face
  targets (`XeenCharacterDialogFlow.cpp:79`, `XeenDialogView.cpp:31`).
  Monster targeting makes one `R(0,n-1)` draw for every count
  (`XeenCombatRules.cpp:427`).
- **Roster.** The 30 records and 30 supplements are saved (capture reads
  every supplement, `XeenSaveState.cpp:378`), but not
  `XeenCharacterOriginalDetails` (attribute and resistance pairs, awards,
  birth day, temporary AC, starting experience, side, saved map, spell
  choices, a 16-bit town field; `XeenCharacter.h:80-93`), which restore
  re-attaches from `maze.chr` by slot (`XeenSaveState.cpp:116-118`).
- **Saves.** The panel's Save writes the current slot only at an idle
  gameplay boundary, refusing services and pending presentations
  (`XeenGameplay.cpp:433-446`); the camera must name a valid map
  (`XeenSaveFormat.cpp:319-321`).
- **Interface.** Tavern Sign In (`XeenDialogView.cpp:367`) and D
  (`SdlWindow.cpp:154`) say "not supported yet"; `XeenAudio::requestInn`
  has no production caller (`XeenAudio.cpp:302`,
  [song rules](rules/audio.md#song-names-and-asynchronous-changes)); the
  party bar layout accepts fewer faces (`CloudsUiLayout.cpp:58, 86`).

## Maintainer decisions

- **Step 14 in two milestones** (2026-10-23). M87, this plan (Tier A), holds
  the core risk: membership, every six-lane consumer and the save layout.
  M88 manages roster slots: Create (rolls from the existing Journey
  generator, class requirements, attribute swaps, race and sex from the
  slot, name, starting spells, skills, resistances, HP, SP, birth year) and
  Delete. M88 is Tier B unless it changes the save format, an owner or the
  generator; ordinary draws from the existing generator are not an RNG
  change.
- **Delete moves to M88 with Create** (2026-10-23): without Create, a
  deletion could leave an empty Inn with Exit refused.
- **Sign In saves as in DOS** (2026-10-23, after the first Part 0 stop).
- **Monster target draws as in DOS, in Part A** (2026-10-23, after the
  second Part 0 stop), although six-member draw sequences change.
- Static program evidence first; DOSBox only where Part 0 cannot settle a
  rule.

## Part 0 - DOS evidence (done)

Calibrated static analysis of the CD `WORLD/XEEN.DAT` and resource
inspection, calibrated on Tavern Food; work copies in `D:/MModern-re/m87/`.
The rules are in [party rules](rules/party.md) and the
[target draw](rules/combat.md#monster-targeting-and-hit-resolution), the
offsets and limits in [M87 evidence](rules/evidence.md#m87-inn-and-party-size).
The results that shape the design:
- **Roster and taverns.** The CD roster names only the six starting
  characters, all lodged in Vertigo. The Inn lists roster IDs 0-23 with a
  name, the current side and the Inn's town, in ID order. Taverns stand on
  maps 28, 30 and 49, exits (18,4), (19,16) and (11,12), all facing West;
  Shangri-La's exit is an explicit DOS correction the reference lacks.
- **Sign In** never refuses for incapacitated members. It lodges each active
  member in the town, adds a day (calendar, merchant regeneration and
  interest when due; no Rest, food, recovery, condition ticks or dawn
  clears), sets the current map to 0 and saves to the current slot without a
  prompt, then ends the Tavern's Event continuation and opens the Inn.
- **Inn.** Loading a map-0 save opens the Inn without another day. Exit
  restores the prior map and door, saves again, reloads the map even when
  unchanged, rebuilds monsters and runs the ordinary flagged-cell Event
  pass. Add and Remove do not save; Remove may empty the party; Exit refuses
  zero. F1-F6 open the character sheet.
- **Dismiss** works in exploration only, never saves and costs no time. The
  departed member stays in the inactive tail with the old count, and the
  next save lodges it in Vertigo (map 28 plus side). This is the only
  inactive-lane read.
- **Targets.** One participant: index 0 without a draw; two to five:
  `R(0,n-1)`; six: `R(1,6)`, index 5 on a 6, otherwise `R(0,4)`.
- **Consumers** use the active or combat count; pooled gold, gems and food
  are untouched by membership changes; pending treasure does not survive
  the Inn round trip (gameplay entry clears it, as in M81).

## Design decisions

- **Owners.** `XeenPartyState` keeps the roster and membership. The Inn is a
  UI flow beside the service flows, not a state owner: it edits membership
  through the existing mutation and publication protections. Sign In ends
  the Tavern service with the shared Event completion tail. Dismiss is a
  main-screen dialog on the existing input path.
- **Membership.** One to six distinct recruitable IDs (0-23, named, Clouds
  side), in any order. Zero exists only inside the Inn, and M87's Exit
  refuses it, so M87 never saves or plays it (M88's Create and Delete saves
  can). Validation checks the actual members instead of
  `kXeenCombatOwners`; serialized counts derive from membership. Every
  change revalidates Journey authority and invalidates or remaps cached
  member selections (WhoWill, selected member, combat choices).
- **Six lanes and a count.** The six-element arrays stay, like the original
  party arrays, with an active count. Ordinary access goes through a checked
  accessor that rejects inactive lanes; loops run to the count; masks never
  exceed it. Dismiss's departed members form a named inactive tail, kept in
  the running program, read only by save capture to lodge them in Vertigo
  and cleared by Load and New Game. Part A migrates by category: rules and
  combat (including the count-dependent target draw); UI and input
  (F-keys, face hits, sheet, exchange and item targets, compacted combat
  selections); presentation (participant masks, highlights, portrait
  effects, projectile lanes, the `0x3f` sites); infrastructure (Journey
  capture and restore, fixed-ID test helpers, `mmodern_test_save`, census
  fixtures, M44 scenarios). Six-member baseline scenarios and unrelated
  six-element tables stay.
- **Roster persistence.** Every record field gets one authority. Part A
  inventories the original details: a field that already has a mutable
  owner (character or supplement) is read from that owner and leaves the
  details; the rest become saved roster fields. New Game still loads them
  from `maze.chr`; restore stops re-attaching resources. All 30 supplements
  stay present; an empty record keeps the values `maze.chr` gives it.
- **Save v19** (v18 -> v19), all in Part A: full roster records, any
  one-to-six membership, and an Inn state (inside the Inn, with the prior
  map and the door position and facing) that replaces DOS's map 0 so the
  camera keeps naming a valid map. Schema 9 and content 14 stay; membership
  validation changes are allowed, and an Inn state that needs the Journey
  section is a stop. Older saves are refused.
- **Inn saves** (Part B). Sign In and Exit save to the current slot through
  the panel Save's path without its prompt, refusals and acknowledgement,
  at the DOS points: Sign In after lodging and the day, with the pre-Inn
  membership; Exit after restoring the door and copying the party to the
  roster. Loading an Inn save opens the Inn without adding time. A write
  failure keeps the panel Save's existing handling; Part B compares it with
  DOS's termination and records the difference for the maintainer.
- **Format audit.** `tests/XeenSaveDump.cpp` and the equality helpers cover
  every new field. Three steps precede the maintainer's approval to replace
  the four M44 digests: a counterfactual build with the old target draw and
  the v18 layout reproduces the old digests byte for byte; the target draw
  alone explains the first divergence; the v19 layout then adds only the
  new fields, equal to `maze.chr`, with identical decoded existing fields
  and RNG state and byte-identical reload and re-save.
- **Presentation and time.** The original Inn screen, buttons, faces and
  resource text, with `inn.m` through `requestInn`; farewell and closing
  scroll reuse the Tavern exit presentation in the DOS order. Delete and
  Create show the standard "not supported yet" notice. The player cannot
  save from the Inn screen. Sign In adds its day through the existing clock
  owner, in the Tavern mode Part 0 describes.
- **RNG.** No new generator or speculative draws; membership operations draw
  only where DOS does. The target draw is the one intended change to
  six-member draw sequences; smaller-party sequences are tested.
- **Quirks.** Part A and B add, with the code, the rows and ScummVM
  differences Part 0 proposed: `sv-inn-town-list`, `sv-inn-shangri-la-exit`,
  `sv-inn-save-boundaries`, `sv-inn-return-reload`,
  `sv-dismiss-deferred-lodging`, `sv-monster-target-count-draws` and
  `inn-sign-in-dawn-mode`.
- **Decided now.** The rules in `rules/party.md` and the target draw are
  implemented without asking, including membership-validation changes, the
  Inn continuation that keeps M84's callers and lodging metadata the save
  can hold.
- **Stop rules.** Stop for: a Journey schema or wire-layout change; a new
  state owner; a digest difference the audit does not explain; state the
  save cannot represent; an inactive-lane read other than Dismiss's tail;
  contradictory or unverifiable evidence; scope beyond this plan.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS and resource evidence, `rules/party.md` (done) | Maintainer read the findings |
| A | Membership, six lanes and a count in every category, target draw, Dismiss and its tail, roster persistence, save v19 with the Inn state, format audit, tests | Independent implementation review, suite, digest approval |
| B | Sign In, the Inn (list, add, Remove, Exit, sheet, Delete and Create notices), Inn saves and Load into the Inn, return, music and presentation, presets | Independent implementation review, suite, play-test |
| C | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap | Maintainer acceptance |

## Acceptance

- Rules tests at sizes 1-6 with non-default IDs and order: combat order,
  target draws for one, two to five and six participants, XP division,
  Poison Volley shots, Rest and food, Event conditions, give and take, Run,
  defeat, smaller-party draw sequences; inactive-lane input (F5 with four
  members) ignored; Dismiss refusal and Vertigo lodging at the next save;
  Inn list by town and side, add, full party, Remove to zero, empty-party
  Exit.
- Exact save and load with one to six members and changed inactive
  characters, restore without resource re-attachment, malformed saves
  refused without partial publication, v18 refused clearly.
- Original data: Sign In at Vertigo's tavern saves; loading that slot
  reopens the Inn without another day; remove two, Exit saves, the map
  reloads with monsters rebuilt and the Event pass at the door; fight, save
  and load. Shangri-La's exit lands at (11,12) facing West.
- Complete suite passes; M44 digests change only through the approved audit.
- Maintainer play-test with `m87-inn` (Vertigo tavern) and `m87-three`
  (three members): recruit, remove and recruit again, reload the Sign In
  save, Dismiss with its last-member refusal and later Vertigo lodging, and
  Sign In at Shangri-La.
