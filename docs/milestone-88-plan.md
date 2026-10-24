# Milestone 88 plan - Create and Delete at the Inn

**Tier A. Status: planned (not started).**
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
- Static program evidence first; DOSBox only where Part 0 cannot settle a
  rule.

## Part 0 - DOS evidence (read-only)

Calibrated static analysis of the CD `WORLD/XEEN.DAT` with offsets, plus
resource inspection; work copies in `D:/MModern-re/m88/`. Reuse M87's
evidence. The pinned reference (`dialogs_create_char.cpp:91-704`,
`dialogs_party.cpp:222-282`) is a guide only: it offers all 30 records as
free slots (Clouds has faces for 24), compares the displayed list with 30
for a full roster, gives level 5 on Darkside, maps PageUp and PageDown,
slices skill names in the stats text, never saves after Create or Delete,
and checks Delete for the Slayer Sword. Results extend `docs/rules/party.md`
with a Create and Delete chapter and `docs/rules/evidence.md`, with quirk
rows. No code.

1. **Create entry and exit.** The C dispatch, with six members too; the
   roster-full condition, text and sound; fades, music, the Inn screen on
   return, and the return snapshot (`0x39a96..0x39a9a`) with and without a
   new character.
2. **Free slots.** Which roster IDs qualify (range, empty name, side), their
   order, Up and Down limits, race and sex from the slot, the portrait, and
   zero free slots.
3. **Rolls.** The draw formula, order and retry condition; the class
   requirements; the default and highlighted class; what Roll and Space
   redo; whether a chosen class survives a reroll or exchange. Every draw
   on the screen, including whether the dice animation draws.
4. **Screen and input.** `create.raw`, `create.icn`, `create2.icn`,
   `dice.vga`; layout and redraws; the stats text, its colors and the skill
   name substrings (offsets and lengths); the key and mouse table (M84's
   method); the attribute exchange (window, Escape, same attribute); class
   choice by mouse and keys; the dice cadence on the M63 clock.
5. **Writing the character.** The no-class refusal; the name prompt
   (window, length, case, Escape, empty name). Each CHR field as written,
   cleared or preserved: level, attributes and temporaries, resistances,
   birth year and day, CHR byte 339, the lodging word and side, every
   Beacon field, quick option and current spell, each class's starting
   spellbook, class and race skills, awards, inventory, conditions,
   experience, HP and SP (checked against the existing level-1 formulas).
   Then whether the screen restarts with a new roll and slot list.
6. **Delete.** Prompt, keys and Escape button; who can be deleted (active
   members, characters lodged in other towns, carried protected items as
   opposed to pooled quest items); confirmation text and input; membership
   change; each field cleared or preserved, compared with the empty records
   of the original `maze.chr`; list and page reset; sound; the snapshot
   (`0x39a2e..0x39a32`).
7. **Empty party and order.** That both snapshots save and Load restores
   zero members, that the Inn then opens with Exit refused until one is
   added, and where the Dismiss tail settles relative to these saves.

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
  Created values follow the rolls and DOS initialization; deleted values
  follow DOS clearing and preservation, stored explicitly, never as absent
  optionals.
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
- **Decided now.** Part 0 answers within items 1-7 are implemented without
  asking, including texts, tables, ordinary created and deleted values, and
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
| 0 | DOS and resource evidence, `rules/party.md` | Evidence recorded, open runtime questions checked in DOSBox, stop-rule decisions settled before dependent work |
| A | Save v20 with the empty Inn party (capture, admission, validator), record operations, Delete and its save, format audit, tests | Independent implementation review, suite, digest approval |
| B | Create: record rules, screen, rolls, exchange, name, return save; presets | Independent implementation review, suite, play-test |
| C | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap, README, controls | Maintainer acceptance |

## Acceptance

- Rules: rolls in DOS order from a known generator state, the retry, class
  requirements, the default class, exchange; the created record for every
  class and every race (spells, skills, resistances, HP, SP, age);
  free-slot order with race and sex; roster full, no-class refusal, name
  abort; Delete of a lodged character and of an active member, cancel, and
  delete then recreate in the same slot.
- Publication: stale tickets and callback mutations rejected without
  partial publication; Create cancelled after rerolls saves the advanced
  generator; a retried failed write repeats no record change or draw; the
  Dismiss tail settles as Part 0 shows; sessions without a save target
  behave as in M87.
- Saves: exact round trip after Create and after Delete, and of an empty
  Inn party after a Create return and after deleting the last member, with
  no replayed time, rewards or Events. Refused without partial publication:
  zero members outside the Inn, malformed Inn doors, missing supplement
  fields, v19 and other malformed saves.
- Original data: Sign In at Vertigo, create a character, add it, Exit,
  fight, save and load; delete a lodged character; remove everyone, return
  from Create, load, and find the Inn empty with Exit refused.
- Complete suite passes; M44 digests change only through the version audit.
- Maintainer play-test with `m87-inn` and a new preset with a nearly full
  roster (an empty-party preset must bypass the generic two-member
  preparation): create, exchange, reroll, name, add, delete, roster full.
