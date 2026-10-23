# Milestone 87 plan - The Inn and a party of one to six

**Tier A. Status: planned (not started).**
Roadmap [step 14](roadmap.md#next-arc-finishing-the-games-systems), first of
its two milestones, after [M86](milestone-86-plan.md). Baseline: `b2bf700`,
equal to `origin/main`.

## Goal and scope

As in DOS, Sign In at a Clouds tavern lodges the party and opens the Inn:
the original screen, with the Inn song, lists the lodged characters; members
are added up to six and removed, and Exit returns the party to the tavern
door. Dismiss (D) removes a member as DOS allows. The game then plays with
any one to six roster characters in any order: combat, spells (Poison
Volley shoots once per active member), Events, Rest, services, travel,
treasure and saves.

Delete and Create say "not supported yet" until M88, which manages roster
slots. Out of scope: Darkside
lodging and its Inn song, and every other roadmap step. Planning does not
authorize implementation; independent plan and implementation reviews, the
complete suite and a maintainer play-test are required.

## Current state

At `b2bf700`:
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
- **Roster.** The 30 records and 30 supplements are saved (capture reads
  every supplement, `XeenSaveState.cpp:378`), but not
  `XeenCharacterOriginalDetails` (attribute and resistance pairs, awards,
  birth day, temporary AC, starting experience, side, saved map, spell
  choices, a 16-bit town field; `XeenCharacter.h:80-93`), which restore
  re-attaches from `maze.chr` by slot (`XeenSaveState.cpp:116-118`).
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
- Static program evidence first; DOSBox only where Part 0 cannot settle a
  rule.

## Part 0 - DOS evidence (read-only)

Calibrated static analysis of the CD `WORLD/XEEN.DAT` with offsets, plus
resource inspection of `maze.chr` and the map data; work copies in
`D:/MModern-re/m87/`. The pinned reference (`locations.cpp:912-975`,
`dialogs_party.cpp`, `dialogs_dismiss.cpp`, `interface.cpp:651`) is a guide
only: it adds 1440 minutes at Sign In, its exit index `mazeId-28` misses map
49, it lists every named character of the side, copies the party to the
roster only in memory (`dialogs_party.cpp:217`) and reloads only a
different map on return (`locations.cpp:973`). Reuse settled evidence. The
consumer inventory is
bounded: the Part A consumer categories below and their original call sites.
A failed static search is not proof of absence; timing or presentation
questions statics cannot settle go to DOSBox or the maintainer. Results go
to a new chapter `docs/rules/party.md` (indexed in `docs/rules/README.md`;
M88 extends it) and `docs/rules/evidence.md`, with quirk rows. No code.

1. **Roster and list.** Filled `maze.chr` records (names, side byte
   encoding, saved map); records 24-29: eligibility and portraits; which
   characters the Inn lists (per town or side), order, member mark, an empty
   list; scroll limits; face-to-roster mapping.
2. **Sign In.** Which taverns offer it (all Clouds towns, Shangri-La,
   Newcastle) with exit position and facing; admission with Dead, Stoned or
   Eradicated members; the time added and its consequences in order (dawn
   clears, food, interest, conditions, losing the last viable member); when
   active changes become lodged roster state; farewell, song, scroll and
   fade order; how the Tavern service and its Event continuation end; the
   return (reload or not, monster rebuild, Event dispatch at the door,
   distinct from M84's Load/New Game pass); any disk write.
3. **Inn screen.** Key and mouse table (M84's method); add and Remove
   refusals with texts and sounds; Exit with an empty party.
4. **Dismiss.** Keys and dispatch in exploration and combat; refusals and
   the Clouds items that block it; the dismissed character's saved map and
   side; time and presentation.
5. **Consumers.** For each Part A category, whether DOS uses the active
   count: combat order, monster targets, XP division, Poison Volley shot
   flags, Rest and food, Event conditions, give, take, member prompts and
   WhoWill, treasure, Training, Temple, Town Portal, Mr Wizard, Lloyd's
   Beacon, the "everyone dead" check. A read at or beyond the count is
   recorded with a witness.
6. **Shared state.** Gold, gems, food and pending treasure across
   membership changes; which CHR fields Sign In, Dismiss and Remove change;
   that DOS keeps the roster in the saved game.

## Design decisions

- **Owners.** `XeenPartyState` keeps the roster and membership. The Inn is a
  UI flow beside the service flows, not a state owner: it edits membership
  through the existing mutation and publication protections, and Sign In
  ends the Tavern service as Part 0 shows. Dismiss is a main-screen dialog
  on the existing input path.
- **Membership.** One to six distinct IDs of the records Part 0 shows
  recruitable, in any order; zero exists only inside the Inn and is never
  captured, saved or played. Validation checks the actual members instead of
  `kXeenCombatOwners`; serialized counts derive from membership. Every
  change revalidates Journey authority and invalidates or remaps cached
  member selections (WhoWill, selected member, combat choices).
- **Six lanes and a count.** The six-element arrays stay, like the original
  party arrays, with an active count. Ordinary access goes through a checked
  accessor that rejects inactive lanes; loops run to the count; masks never
  exceed it. A verified original read of an inactive lane uses a separate,
  named compatibility read, bounded by the six physical lanes, sourced from
  saved state and recorded as a quirk. Part A
  migrates by category: rules and combat; UI and input (F-keys, face hits,
  sheet, exchange and item targets, compacted combat selections);
  presentation (participant masks, highlights, portrait effects, projectile
  lanes, the `0x3f` sites); infrastructure (Journey capture and restore,
  fixed-ID test helpers, `mmodern_test_save`, census fixtures, M44
  scenarios). Six-member baseline scenarios and unrelated six-element tables
  stay.
- **Roster persistence.** Every record field gets one authority. Part A
  inventories the original details: a field that already has a mutable
  owner (character or supplement) is read from that owner and leaves the
  details; the rest become saved roster fields. New Game still loads them
  from `maze.chr`; restore stops re-attaching resources. All 30 supplements
  stay present; an empty record keeps the values `maze.chr` gives it.
- **Save v19** (v18 -> v19): full roster records and any one-to-six
  membership. Schema 9 and content 14 stay: membership validation changes
  are allowed, a Journey wire-layout change is a stop. Older saves are
  refused.
- **Format audit.** `tests/XeenSaveDump.cpp` and the equality helpers cover
  every new field. The four M44 scenarios replayed on the new build must
  give identical decoded existing fields and RNG state, new fields equal to
  `maze.chr`, and byte-identical reload and re-save. A counterfactual build
  that writes the v18 layout must reproduce the four old digests byte for
  byte, so behavior is unchanged and only the layout differs; only then
  does the maintainer approve replacing the digests.
- **Presentation and time.** The original Inn screen, buttons, faces and
  resource text, with `inn.m` through `requestInn`; farewell and closing
  scroll reuse the Tavern exit presentation. Delete and Create show the
  standard "not supported yet" notice. The Inn cannot be saved. Sign In
  adds its time through the existing clock owner with the verified
  consequences.
- **RNG.** No new generator or speculative draws; membership operations draw
  only where DOS does. Six-member draw sequences stay unchanged, and
  smaller-party sequences are tested.
- **Decided now.** Part 0 answers within items 1-6 are implemented without
  asking, including membership-validation changes, an Inn continuation that
  keeps M84's callers, lodging metadata the save can hold, and an original
  read of an inactive lane within the six physical lanes that depends only
  on saved state (through the compatibility read).
- **Stop rules.** Stop for: a Journey schema or wire-layout change; a new
  state owner; a digest difference the audit does not explain; state the
  save cannot represent; contradictory or unverifiable evidence; scope
  beyond this plan; a disk write at Sign In or Exit; an original read that
  depends on unsaved memory or lies beyond the six physical lanes.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS and resource evidence, `rules/party.md` | Maintainer reads the findings |
| A | Membership, six lanes and a count in every category, roster persistence, save v19, Dismiss, format audit, tests | Independent implementation review, suite, digest approval |
| B | Sign In and the Inn (list, add, Remove, Exit, Delete and Create notices), music and presentation, presets | Independent implementation review, suite, play-test |
| C | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap | Maintainer acceptance |

## Acceptance

- Rules tests at sizes 1-6 with non-default IDs and order: combat order and
  targets, XP division, Poison Volley shots, Rest and food, Event
  conditions, give and take, Run, defeat, smaller-party draw sequences;
  inactive-lane input (F5 with four members) ignored; Dismiss refusals; Inn
  add, full party, Remove, empty-party Exit, Part 0's list rule.
- Exact save and load with one to six members and changed inactive
  characters, restore without resource re-attachment, malformed saves
  refused without partial publication, v18 refused clearly.
- Original data: Sign In at Vertigo's tavern, remove two, leave, fight, save
  and load; Sign In time and arrival as Part 0 shows.
- Complete suite passes; M44 digests change only through the approved audit.
- Maintainer play-test with `m87-inn` (Vertigo tavern) and `m87-three`
  (three members): recruit, remove and recruit again, Dismiss and its
  last-member refusal, and Sign In at a second tavern, through an
  exceptional return path if Part 0 finds one.
