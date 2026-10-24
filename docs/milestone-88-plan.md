# Milestone 88 plan - Create and Delete at the Inn

**Tier A. Status: Part 0 done, with its DOSBox checks; Part A not started.**
Roadmap [step 14](roadmap.md#next-arc-finishing-the-games-systems), second of
its two milestones, after [M87](milestone-87-plan.md). Baseline: `d3eec99`,
equal to `origin/main`.

## Goal and scope

As in DOS, the Inn's Create opens the original creation screen: the dice
roll the seven attributes, the classes they allow light up, attributes can
be exchanged, Up and Down choose a free roster slot and with it the
portrait, race and sex, and Create asks for a name and writes the new
character into that slot, lodged in the Inn's town. Delete removes a listed
character from the roster after confirmation. Both save the game where DOS
does. Step 14 is then complete, and the Inn has no "not supported yet"
notice left. Out of scope: Darkside creation and lodging, other Inn changes
and every other roadmap step.

## Current state

At `d3eec99`:
- **Inn.** C and D say "not supported yet" (`XeenInnFlow.cpp:255`).
  `forcedInnSave` admits only Sign In and Exit (`XeenInnFlow.cpp:95-99`),
  with the approved failure recovery
  ([`dev-inn-save-recovery`](original-quirks.md#dev-inn-save-recovery)).
  Opening the Inn always requests `inn.m` (`XeenGameplay.cpp:552`,
  `XeenInnFlow.cpp:202`).
  `innMembership` validates a detached successor but changes only
  membership and counts (`XeenInnFlow.cpp:49-60`).
- **Zero members.** Journey rules accept zero inside the Inn
  (`XeenJourneyRules.cpp:33`), but Inn-save capture refuses it
  (`XeenJourneyCapture.h:46`), and so does the save validator
  (`XeenSaveFormat.cpp:326`). `fromRosterIds`, snapshot capture, restore,
  the restore guard and the dump and equality helpers accept zero.
  `kJourneyVersion` is 19 (`XeenSaveFormat.h:27`).
- **Roster records.** Thirty records are saved in full, each split between
  `XeenCharacter` and its private `XeenCombatInputs` supplement (Might,
  Speed, Accuracy, Luck, resistances, experience, temporary AC;
  `XeenCombatInputs.h:22-31`, `XeenParty.h:40`). Journey requires every
  supplement with Luck, resistances and poison resistance
  (`XeenJourneyRules.cpp:38`), and the restore guard compares both halves.
  `XeenCharacterRules` has `maxHp` and `maxSp` (`XeenCharacterRules.h:60-62`).
- **Elsewhere.** The Journey generator (`XeenWorld::journeyRandom`) is
  present inside the Inn, including after Load into it. `XeenTextInput`
  serves the title's name and Save As prompts. The generic preparation of
  `mmodern_test_save` reads two members (`tools/XeenTestSave.cpp:127`).

## Maintainer decisions

- **Step 14 in two milestones** (2026-10-23): M88 manages roster slots,
  Create (rolls from the existing Journey generator, class requirements,
  attribute swaps, race and sex from the slot, name, starting spells,
  skills, resistances, HP, SP, birth year) and Delete. Ordinary draws from
  the existing generator are not an RNG change.
- **Tier A with save v20** (2026-10-24). DOS saves after a confirmed Delete
  and every return from Create, also with the empty party Remove can leave
  ([party rules](rules/party.md#saves-load-and-return)); save v19 refuses
  it. The independent plan review's changes are applied.
- **The Inn song after a title Load joins Part A** (2026-10-24): DOS
  plays no Inn song when a save is loaded into the Inn from the title, and
  M87 does not reproduce it.
- Static program evidence first; DOSBox only where Part 0 cannot settle a
  rule.

## Part 0 - DOS evidence (done)

Part 0 (2026-10-24) settled items 1-7 of its question list by calibrated
static analysis of the CD `WORLD/XEEN.DAT`, with no stop rule hit. The rules
are in [Create and Delete](rules/party.md#create-and-delete), the evidence in
[M88 evidence](rules/evidence.md#m88-create-and-delete). The implementing
parts follow those rules, not the pinned reference, notably:

- **Create entry.** Six members can enter; only a displayed list of 24
  refuses. Free slots are IDs 0-23 with an empty name, in order, with no
  side test; race is `floor(id/4)%5` and sex `id&1`. Without a free slot,
  Create returns at once, and filling the last free slot returns too.
- **Rolls.** Each attempt makes 21 draws (three passes of
  `floor(R(10,79)/10)` over the seven attributes) and repeats while no class
  qualifies. Nothing else draws: not the dice, the name prompt or Delete.
- **Classes.** The arrow starts on the first eligible class; Left and Right
  move it, and Enter or a click commits a class. A reroll or a completed
  exchange clears the committed class. Unavailable classes use color 2.
- **The record.** All 354 bytes are cleared, then initialized as the rules
  list: level 1 on Clouds, the first starting spell current (255 without
  spells), every Beacon field cleared.
- **Delete.** Only a listed face selects; carrying the Xeen Slayer Sword
  (weapon 34) refuses; Y confirms. The whole record is cleared and
  membership compacts, with no last-member rule. The list is rebuilt by town
  without the side test, keeping its page.
- **Saves.** Every return from Create and every confirmed Delete copies the
  active records and snapshots, with zero members too, replaying neither
  Sign In, rolls nor Dismiss lodging. Return from Create resets the page
  and fades into the Inn without its opening scroll; the Inn song continues.
- No CHR field lies outside the save representation; v20, schema 9,
  content 14 and the owners stay as planned.

The maintainer ran the [runtime checks](rules/evidence.md#m88-runtime-checks)
in DOSBox on 2026-10-24, except the globally full roster, which rests on
the static rule; they agree with the static rules. They also showed that
the empty party bar keeps the yellow selection frame on its first slot,
and that a save loaded into the Inn from the title opens it in silence
until Exit. Static analysis traced that to DOS's Inn song flag, checked
against two more observations.

## Design decisions

- **Owners.** No new state owner. Create is a dialog of the Inn flow and
  Delete an Inn action. Pure functions in `games/xeen` hold the record
  rules: rolls, class requirements, the created and the deleted record.
  Beside `innMembership`, the Encounter/Inn owner gains ticket-checked
  operations that prepare and validate complete character and supplement
  successors, membership and counts before publishing them. They update the
  retained preimage and mutation boundary consistently and reject stale or
  reentrant calls without partial publication.
- **Records.** All 30 records keep present supplements, Luck, resistances,
  poison resistance and learned-spell storage, deleted records included.
  Created values follow the rolls and DOS initialization; a deleted record
  is all zero, like the original empty records. Values are stored
  explicitly, never as absent optionals.
- **RNG.** The existing Journey generator stays, and DOS clock mixing remains
  deferred. Creation reproduces the verified formula, order and retries of
  the attribute rolls; the dice animation is presentation only once Part 0
  confirms it draws nothing. Rerolls advance the saved state, so Load
  continues exactly.
- **Save v20** (v19 -> v20), the only format change: inside a validated Inn
  an empty active party is valid; elsewhere one to six members remain
  required. Inn-save capture and forced-save admission extend to a
  confirmed Delete and every return from Create, keeping the ticket,
  boundary, busy-state and preimage checks and all three Inn-door
  validations. The layout, schema 9 and content 14 stay; v19 saves are
  refused clearly. Loading an empty-party Inn save reopens the Inn.
- **Inn song.** The running program (`XeenProgramState`) keeps DOS's Inn
  song flag, never saved: the title menu sets it, the Inn requests `inn.m`
  only while it is clear and then sets it, and leaving the Inn or entering
  gameplay outside it clears it. A title Load into the Inn stays silent
  until Exit.
- **Saves.** Both new sites use the existing forced Inn save and its
  recovery; a retried failed write repeats neither the record change nor a
  draw.
- **Format audit.** The four M44 scenarios never enter the Inn. An
  audit-only build writing version 19 must reproduce their baseline
  digests; the v20 saves must have the same length and bytes except the
  envelope version at offsets 8-9, with unchanged decoded state and traces.
  No production v19 writer is kept. The maintainer then approves the new
  digests.
- **Presentation.** The original screens, sprites, text through
  `XeenDosText` and the program's tables, by mouse and keyboard, with the
  existing name input. Created characters take the Clouds side.
- **Decided now.** The Part 0 rules are implemented without asking, including texts, tables, ordinary created and deleted values, and
  the extensions they need within existing owners and capture and
  publication protections.
- **Stop rules.** Stop for: a Journey schema or wire-layout change; any
  other persistent representation or interpretation change beyond
  admitting empty Inn parties; a new state owner; generator calls beyond
  the attribute rolls, or call counts that depend on timing; a digest
  difference beyond the version field; a CHR field the save cannot hold;
  contradictory or unverifiable evidence; scope beyond this plan.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS and resource evidence, `rules/party.md` | Done (2026-10-24), with the DOSBox checks |
| A | Save v20 with the empty Inn party (capture, admission, validator), record operations, Delete and its save, the Inn song flag, format audit, quirk rows for Delete, the empty snapshot and the song, tests | Independent implementation review, suite, digest approval |
| B | Create: record rules, screen, rolls, exchange, name, return save, Create quirk rows; presets | Independent implementation review, suite, play-test |
| C | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap, README, controls | Maintainer acceptance |

## Acceptance

- Rules: rolls in DOS order from a known generator state, the retry, class
  requirements, the default class, exchange; the created record for every
  class and every race (spells, skills, resistances, HP, SP, age);
  free-slot order with race and sex; the arrow apart from the committed
  class; the first starting spell current; roster full, no-class refusal,
  name abort; Delete of a lodged character and of an active member, the
  Slayer Sword refusal, cancel, deleting the last face of the last page
  (the page is kept), and delete then recreate in the same slot.
- Publication: stale tickets and callback mutations rejected without
  partial publication; Create cancelled after rerolls saves the advanced
  generator; a retried failed write repeats no record change or draw; the
  Dismiss tail settles as Part 0 shows; sessions without a save target
  behave as in M87.
- Saves: exact round trip after Create and after Delete, and of an empty
  Inn party after a Create return and after deleting the last member, with
  no replayed Sign In, time, rolls, rewards, Events or Dismiss lodging; the
  empty party bar keeps its yellow frame and the Load menu shows level 0. Refused without partial publication:
  zero members outside the Inn, malformed Inn doors, missing supplement
  fields, v19 and other malformed saves.
- Inn song: a title Load into an Inn save opens the Inn without music until
  Exit; Sign In after New Game or a Load, and a panel Load into the Inn,
  play `inn.m`.
- Original data: Sign In at Vertigo, create a character, add it, Exit,
  fight, save and load; delete a lodged character; remove everyone, return
  from Create, load, and find the Inn empty with Exit refused.
- Complete suite passes; M44 digests change only through the version audit.
- Maintainer play-test with `m87-inn` and a new preset with a nearly full
  roster (an empty-party preset must bypass the generic two-member
  preparation): create, exchange, reroll, name, add, delete, roster full.
