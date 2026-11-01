# Milestone 106 plan - Darkside and World wins, Darkside ending

**Tier A. Status: planned (2026-11-01).** Step 4 of the
[Darkside arc](roadmap.md#darkside-arc), after [M105](milestone-105-plan.md),
built on the Clouds ending of [M92](milestone-92-plan.md) and the cutscene
presenter of [M91](milestone-91-plan.md) and [M93](milestone-93-plan.md).
Save v24 -> v25 expected (Part A); Journey schema 9 and content 14 unchanged
unless Part 0 shows otherwise. The World ending's presentation follows in
step 5 (Tier B).

## Goal

Darkside of Xeen and World of Xeen can be won as in DOS. Defeating Alamar in
Castle Alamar (Darkside map 51) reaches opcode 57 (CutsceneEndDarkside),
which records the Darkside win, saves the party and plays the original
Darkside ending with the party's score, then returns to a fresh World title.
Opcode 58 (CutsceneEndWorld, Darkside map 129, Clouds of the Ancients)
records the World win and saves as DOS does; its presentation shows "not
supported yet" until step 5. The ending mirror words and the title's replay
rows work for every win DOS offers them for.

## Why both wins together

Maintainer decision, 2026-11-01. Both entries sit side by side in the DAT,
the World win needs the Darkside win's state, and the World completion is
saved too: one Part 0, one save bump (v25) and one Tier A cycle cover both.
The two long EXE presentations are split so that each Part and play-test
stays a normal size; the World presentation is content work (Tier B).

## Scope

In scope:

1. **The win tails of opcodes 57 and 58**, in DOS order: completion words,
   won markers, quest items, flags, any move (the reference sends the
   Darkside win to Castleview, map 29 `(25,21)` North, and moves nothing for
   the World win), the forced save of the live party and roster, the sound
   switches, the score and the hand-off. Part 0 settles the order and values.
2. **Saved completion.** DOS keeps Darkside and World completion words in the
   party record (the pin's `_darkSideCompleted`, `_worldCompleted`);
   MMModern has only `cloudsCompleted`. Save v25 adds both, the DOS export
   writes them, and every DAT rule that reads them (found in Part 0) follows.
3. **Ending mirror results** on both sides as DOS dispatches them, including
   what each saves and skips: in the pin, Darkside results 33 `showdown`
   (Darkside ending), 34 `genesis` (World ending) and 35 (Clouds ending in
   World), and Clouds results 60 and 61, which MMModern refuses today. Today
   MMModern treats every Darkside result below 37 as a destination.
4. **Score** with the M92 helper, if DOS uses the same one for both wins.
5. **Won state and title replay.** The Darkside and World won states, and the
   Other Options rows and hotkeys that replay each ending with DOS's fixed
   score, in every combination of wins (the World replay row shows "not
   supported yet" until step 5).
6. **The Darkside presentation**: the `WORLD/XEEN.EXE` Darkside ending on the
   M91 presenter, its score page and the return to a fresh World title.

Out of scope: the World ending's presentation (step 5), the three waiting
Tier A roadmap items, DOS save import, and any other mechanic the route
meets, which keeps its census row and notice.

## Key decisions

- **DOS check first.** Part 0 is calibrated static analysis of
  `WORLD/XEEN.DAT` and `WORLD/XEEN.EXE`, then an uninterrupted DOSBox capture
  of the Darkside ending, as M92 did. The pin is a guide, not the authority;
  the Clouds ending differed from it in order and saved state.
- **Reuse the Clouds ending owners.** `XeenEventFlow::startEnding` /
  `finishEnding`, `XeenEndingScore`, the forced Journey save, the settings
  store and the M91 presenter grow the Darkside and World cases;
  `XeenEndingRequest` gains their kinds. No new owner unless Part 0 shows a
  flow the Clouds path cannot express.
- **Won store as for Clouds** (maintainer, 2026-11-01): each new won state is
  a boolean in `%LOCALAPPDATA%\MMModern\settings.ini`, next to `won` and
  `darkside_intro_seen`, extending the approved deviation
  [`dev-clouds-won-store`](original-quirks.md#dev-clouds-won-store); DOS's
  CFG is never written. A failed write behaves as in M92: a notice, the
  ending continues, the previous value is kept. The quirk entry is updated
  in Part A.
- **Save v25** carries both completions; v24 saves are rejected with the
  usual message. M44 digests change once, at Part A's format audit.
  Restore stays exact: no replayed Event, reward, time or RNG.
- **Playable by default.** Between Part A and Part B the Darkside hand-off,
  and until step 5 the World hand-off, show "not supported yet" after the
  save and return to the title; a missing or different `XEEN.EXE` does the
  same, as M92 does for Clouds.

## Parts

**Part 0 - DOS evidence** (no production code). Calibrated analysis with
file offsets, recorded in [cutscene rules](rules/cutscenes.md),
[party rules](rules/party.md), [evidence](rules/evidence.md) and, for any
difference from the pin, [original quirks](original-quirks.md) and
[ScummVM differences](scummvm-differences.md):

- the map 51 and map 129 Events around opcodes 57 and 58: what the party must
  have done to reach them and what runs before them;
- both DAT tails: every store and its order, the won markers (CFG bytes or
  other), quest items, flags, destination map, cell and facing, the forced
  save, sound switches, score and the `XEENCOMMAND` strings;
- every DAT reader of the completion words and won markers after a win (map
  or Beacon changes like Castle Xeen's after Clouds);
- the ending mirror results on both sides: which are reachable and what each
  stores, saves or skips;
- the title: Other Options rows, hotkeys and positions for every combination
  of wins, and the fixed replay scores;
- the EXE Darkside ending: sequencing, resources, songs, voices, captions,
  timing, input (Escape, keys, clicks, Ctrl-Q), random draws, score page and
  return, compared with the pin's `showDarkSideEnding1..4` and score; and
  where the World ending starts in the EXE, so step 5 can be sized;
- a DOSBox capture (maintainer, one session): the final fight and the whole
  Darkside ending uninterrupted, the slot and CFG before and after, the
  World win's save, `showdown`, and the title rows. The agent prepares the
  DOSBox saves for it (see Play-test).

Part 0 ends with a short findings list; a finding that changes this plan's
scope or the save decision amends the plan before Part A.

**Part A - the wins and the save** (Tier A core). Both win tails, both
completions in save v25 and in the DOS export, the ending mirror results, the
score, the won store, the title rows and the post-win rules. Original-data
tests: the map 51 and map 129 Events from real saves reach opcodes 57 and 58
and write the expected saves; the saves reload exactly and re-save
byte-identically; the score matches the DOSBox capture's party; the DOS
export of each post-win save matches DOS's slot in the completion words.
Independent review, complete suite.

**Part B - the Darkside presentation and its replay.** The EXE Darkside
ending on the M91 presenter with stage times inside the DOSBox capture's
brackets, the score page matching DOS frames pixel for pixel, the return to
a fresh World title, and the Darkside replay row. Tests on a fake clock as
M92 did, plus the real map 51 preset running through the ending in CTest.
Independent review, complete suite.

## Play-test

One session after Part B (Part 0's DOSBox capture is evidence, not a
play-test). Prepared as [play-tests](playtests.md) describes, with these
additions requested by the maintainer (2026-11-01):

- **A party of level 200 or more**, with full HP and SP and strong equipment
  inside DOS's one-byte attribute limits, because a level-30 party cannot
  win the final fights. New presets start it in Castle Alamar Level 3 before
  the fight (`m106-alamar`), before the World win on map 129
  (`m106-world`), at a mirror (`m106-mirror`), and after the wins for the
  title rows; each carries the progress its route needs.
- **DOSBox comparison saves for every item**: each preset exported with
  `mmodern_dos_export --dosbox` into its own folder, with its launcher, so
  the maintainer plays the same state in DOS side by side. If the export
  cannot represent a state an item needs, Part 0 or Part A says so before
  the session, and the guide says how to reach it in DOSBox instead.

Items, in route order: the Alamar fight and the Darkside win; the save
afterwards (load it at the cell and facing from Part 0); the Darkside ending
and its score page against DOSBox; the World win and its save; the ending
mirror words; the title rows. Each says what only play settles (input feel
during the ending, audible equivalence where no capture exists).

## Risks

- The endings may come from a different program or hand-off path than the
  Clouds ending; Part 0 decides before Part B is sized.
- Reaching opcodes 57 and 58 in DOSBox needs real endgame states; if the
  export cannot carry them (per-map state, Darkside progress), the capture
  needs the maintainer's own save or a narrower export extension, decided in
  Part 0.
- A level-200 party may exceed a DOS limit in HP, SP or derived values; the
  presets are checked with the exporter's refusals before the session.

## Acceptance

Tests for each changed rule; original-data tests for both wins, the saves,
the score and the Darkside ending; an independent review of this plan and of
each Part; a passing complete suite; and the maintainer's play-test.
