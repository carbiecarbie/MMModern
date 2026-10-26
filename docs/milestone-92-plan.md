# Milestone 92 plan - Clouds ending and score

**Tier A. Status: planned; implementation not started.**
**Part 0: completed and accepted by the maintainer (2026-10-25),** with
Beacon refusal established by calibrated static analysis; no suitable
Beacon was available for the DOSBox check. This accepts the evidence
phase only, not implementation or the whole milestone.
Roadmap [step 18](roadmap.md#next-arc-finishing-the-games-systems), after
[M91](milestone-91-plan.md); [M90](milestone-90-plan.md) still waits for its
whole-quest play-through. Evidence baseline: `1260911`, equal to `origin/main`.

## Goal and scope

As in DOS, Clouds of Xeen can be finished. Giving Lord Xeen's Scepter to
the Sixth Mirror records the win, computes the score from the party's
experience and the real play time, saves to the current slot as DOS
writes it, and plays the Clouds ending of `WORLD/XEEN.EXE` on the M91
presenter, with the score animation, then returns to a fresh World title.
The win is remembered outside the save, so Other Options offers the
ending. Also in scope: the real play-time counter (save v21) and the
control panel's clock and play time; the mirror words `showtime` and
`bloopers` as Part 0 item 5 shows; after the ending,
Castle Xeen gone from the Clouds map and Lloyd's Beacon refusing it.
Out of scope: `showdown` and `genesis` (Darkside and World endings), the
startup path and the Darkside intro (step 20), other sound devices
(step 19), and every other roadmap step.

## Current state

At `a6b129c`:
- **Ending entry.** Opcode 30 `CutsceneEndClouds` shows a terminal
  "Clouds ending: not supported yet" notice
  (`XeenEventInterpreter.cpp:729-736`). Mirror results 40, 60, 61 and 62
  are refused on Clouds (`XeenEventInterpreter.cpp:540-542`). Lord Xeen's
  death already gives the Scepter, flag 60, award 42 and clears quest
  flag 23 (M89).
- **Title.** Other Options plays only `C` (`XeenTitleFlow.cpp:281-286`);
  every other button says "not supported yet". Session entries are
  `XeenSession.h:9`; `CloudsIntro` is the one cutscene kind.
- **Presenter.** `XeenCutscene` and `XeenCutsceneFlow` have the intro's
  segment kinds only. The ending's kinds (compressed full-screen frames,
  horizontal merge and layered motion, monster parade, score reveal,
  acknowledgement holds) are listed in
  [cutscene rules](rules/cutscenes.md#darkside-and-ending-boundaries),
  not implemented.
- **Time.** No play-time counter exists anywhere. Save v20, schema 9,
  content 14 (`XeenSaveFormat.h:28-30`); the snapshot holds sound options,
  camera, roster, quest items, quest and game flags and maps
  (`XeenSaveSnapshot.h:158-174`). The control panel prints neither the
  clock nor the play time.
- **Configuration.** `WORLD/XEEN.CFG` is read only and never written
  (`XeenInstallationDetector.cpp:257-258`, `XeenAudio.cpp:277`,
  `XeenCutsceneData.cpp:39`). `%LOCALAPPDATA%\MMModern\window.ini` already
  holds the window placement (`SdlWindowPlacement.cpp:99-106`).
- **Post-ending world.** Neither the map-27 castle removal
  (`0xff1c..0xff4e`) nor the Beacon refusal (`0x2febf..0x2fee2`) exists.

## Evidence already in hand

- **DAT tail** (`0x28b98..0x28d3a`, audit
  `D:/MModern-re/clouds-completion/findings.md` §3.4): sets
  `cloudsCompleted` (DS `efe8`), flag 75, the party to map 28 `(18,4)`
  with its facing kept, Efx and Music on (`ed9a`/`ed9b`); score = party
  experience part + hours × 100 + minutes of real play time (C `time()`
  minus session start DS `98b0` plus the saved dword DS `f01a`); writes
  30 bytes of `xeen.cfg`; hands off with
  `XEENCOMMAND=ENDGAME,<score %010lu>,0`.
- **DOSBox run** (`ENDING.AVI`, notes in
  `D:/MModern-re/clouds-completion/dos-ending-observations.md`): Scepter
  via the ordinary treasure screen, the Sixth Mirror's four pages, an
  ending of about 3 min 15 s ending on score `0071001722` and "Press a
  Key", then the World title. The current slot was saved: Vertigo
  `(18,4)` West, Scepter kept, quest flag 23 cleared, flags 60 and 75 set,
  play time 62482 -> 62545. The repeat capture of slot 06 confirms that
  `DARK06.WOX` saves the live roster: four recipients gain 150,000 XP
  each, all six gain award 42, and Ella and Seymour retain their terminal
  conditions. Only map 78's DAT/MOB change in `XEEN06.WOX`; its unchanged
  `maze.chr` caused the earlier mistaken stale-roster claim. Both after
  slots equal their respective CUR archives. `XEEN.CFG` bytes 10-17 became
  `ereiamjh`,
  byte 18 went 1 -> 0.
- **Calibrated Part 0 rules:** the counter uses seconds; 62545 s is
  17:22:25 and contributes `1722`. Its start mark is taken at DAT entry
  and after a snapshot, not at Load. West is retained from using the
  Sixth Mirror facing West. Score XP includes the class/level allowance
  and the kill reward ([ending rules](rules/cutscenes.md#clouds-ending-score)).

## Maintainer decisions

- **Won state** (2026-10-24): in an MMModern settings file under
  `%LOCALAPPDATA%\MMModern`, never in `XEEN.CFG` and never in the save
  (approved deviation: location only).
- **Save v21** (2026-10-24) for the real play-time counter; Part 0
  confirms what DOS counts and where.
- **After the ending, follow DOS** (2026-10-24; roster correction
  confirmed 2026-10-25): save the live roster and party to the current
  slot, then the World title. No retained-roster snapshot or splice.
- **Mirror codes 60/61** stay "not supported yet" until Darkside.
- **Plan defaults** (2026-10-25): a failed ending save shows the error and
  retries, as [`dev-inn-save-recovery`](original-quirks.md#dev-inn-save-recovery)
  does for the Inn (an extension of that approved deviation); the won
  state goes to `%LOCALAPPDATA%\MMModern\settings.ini`; play time follows
  the system wall clock as DOS's `time()` does; Parts A-D as below.
- **Won-store failure** (2026-10-25): if writing `settings.ini` fails, a
  notice appears and the ending continues. A failed rewrite keeps the previous
  won value (maintainer, 2026-10-26).

## Part 0 - DOS evidence (read-only)

Calibrated static analysis of the CD `WORLD/XEEN.DAT` and
`WORLD/XEEN.EXE` (EXE MZ header `0x2e30`, M91 calibration reused), with
offsets, checked against `ENDING.AVI` and the slot-06 before/after pair
and both after CUR files in `D:/MModern-re/m92/dos-runs/`. Slot 05's
pre-fight data calibrates the recorded score; its old post-ending pair is
unavailable. The pin (`worldofxeen/clouds_cutscenes.cpp` ending, `Party::getScore`)
is a guide only. Work copies in `D:/MModern-re/m92/`. Results: a Clouds
ending chapter in `docs/rules/cutscenes.md`, play time in
`docs/rules/time-and-conditions.md`, the ending save in
`docs/rules/party.md`, `docs/rules/evidence.md`, quirk rows. No code.

1. **Play time.** Every writer and reader of DS `98b0` and `f01a`: when
   the session start is taken (program start, New Game, Load, Inn Load,
   return from an EXE hand-off), the unit, what Save adds, overflow, and
   whether title, menu and dialog time count. Confirm the seconds
   hypothesis against 62482 -> 62545 and the score. Clock edge cases: a
   clock moving backwards (negative elapsed under DOS's unsigned
   arithmetic) and daylight-saving changes.
2. **Score.** The exact integer arithmetic (which members, dead ones,
   division order, truncation, the `%010lu` field), recomputing
   `0071001722` from the slot-05 data as calibration.
3. **Ending tail and save.** The DAT tail in order: state changes, the
   facing, `cloudsCompleted` (is DS `efe8` inside the saved party block,
   and how is it initialized at startup, New Game and Load, possibly from
   `XEEN.CFG`?), Efx and Music, the save call and its slot, and whether the
   DAT loads map 28 before writing. A diff of **every** member of
   `XEEN06.WOX`/`DARK06.WOX` before and after (map 78's kill and objects,
   the Sixth Mirror's Event overrides, explored bits, map 28), not only
   `maze.chr`/`maze.pty`. Every point where DOS writes active characters
   into the working file's `maze.chr` (Save, the Inn, map change?), since
   slot 05's run had no map change and cannot tell "last Save" from "last
   flush"; what that file holds after New Game. The failure path of the
   write.
4. **XEEN.CFG.** The meaning of bytes 10-17 and 18 and who reads them
   (Other Options `V`, the EXE), so MMModern's settings file stores only
   what those readers need.
5. **Mirror words.** Result 40 (`showtime`): which part of the tail runs,
   its score, whether it saves or moves the party, and where play
   continues. Result 62 (`bloopers`): the empty `0x26773` and what the
   Event does next.
6. **Other Options `V`.** Its condition, the fixed `ENDGAME,1234567890`
   string, what the EXE shows for it, how the button and hotkey are drawn
   before and after a win, and that it leaves play time and the won state
   unchanged.
7. **Post-ending world.** The map-27 castle removal and the Beacon
   refusal: which state each tests (`cloudsCompleted` or flag 75), their
   texts and sounds.
8. **Control panel.** `0xf325` from `0x1efe7`, format DS `4be3`: clock and
   play-time layout, units, when it redraws, and the time-of-day
   conversion (Borland `localtime` without `TZ` against host local time).
9. **EXE ending.** The `ENDGAME` path stage by stage: resources (archive,
   names, sizes), the new segment kinds, timing per stage against the
   video, the score animation's digit generator and its random source
   (Darkside uses range `0x1cbdc` and time-mixed `0x1cc27`), texts, songs,
   VOC and FM cues, captions, input (Escape, keys, clicks), whether the
   path writes `XEEN.CFG` (Darkside's does at `0xd345..0xd374`), and the
   return to the title.
10. **DOSBox checks** for the maintainer, only where statics cannot settle
    them: `showtime` and `bloopers` at Vertigo's mirror (if `showtime`
    saves: gain XP, change map, `showtime`, compare the files); Other
    Options `V` after the win; keys during the ending; the panel clock
    after a few minutes; after the ending, Castle Xeen gone at map 27 and
    Beacon into the castle refused.

## Design decisions

- **Owners.** No new gameplay state owner. The saved play-time seconds
  belong to `XeenPartyState`, beside `purse` and `worldFlags`
  (`XeenSaveState.cpp:396`, restored at `:406`). The session start mark
  belongs to the application side (`XeenGameplayServices`), not to
  `XeenSessionState`, and is taken where Part 0 shows. The ending reuses
  `XeenCutsceneFlow` with a Clouds ending program and new segment kinds.
- **Wall clock.** `XeenWallClock` has no default. Only the player's title
  path binds the system clock, with DOS's local-date/time conversion
  and legacy DST rule
  ([clock rules](rules/time-and-conditions.md#real-play-time-and-panel-clock)).
  Every developer mode,
  `tools/XeenTestSave.cpp`, `tools/XeenCoverageContext.cpp` and the
  in-process fixtures (`tests/XeenSaveTestSupport.h:149`) use a fixed
  clock, which child processes can set with `--wall-clock <seconds>`; the
  M44 witnesses run `mmodern.exe` as a child
  (`tests/XeenM44ScenarioTests.cpp:54-67`), so they pass it there.
  Clock edge cases follow Part 0 item 1; a reproduced wrap gets a quirk
  row. The mark starts at DAT entry and resets after each snapshot;
  Load and Inn Load retain it. New Game zeros the saved counter before
  its first snapshot, so time spent at the title is included. Preserve
  the panel formatter's 16-bit truncation rather than normalizing it.
  The panel clocks are hidden by default; G, O, B, R enables them for
  that panel visit, and Exit clears the flag. Preserve that input gate
  in the existing panel flow rather than displaying clocks by default.
- **Won store.** `settings.ini` beside `window.ini`, resolved once by the
  application, with a developer path override that every child-process
  test uses; a test checks the player's real file is untouched. Written
  atomically as `window.ini` is (`SdlWindowPlacement.cpp:85-91`); missing
  or malformed means not won. DOS's saved party completion word is
  independent of the CFG marker: New Game starts incomplete and Load
  restores its own value. The settings store gates title `V` only; it
  must not make a fresh party complete. A failed write
  follows the won-store failure decision above.
- **Hand-off.** Opcode 30 and mirror result 40 end the Event with an
  ending request carrying only its kind; the interpreter runs on a
  detached candidate (`XeenEventInterpreter.cpp:713-717`) and has no wall
  clock. `XeenEventFlow` applies the tail and makes the forced ending
  write before computing the score, generalizing the Inn's
  `innSave` capability and site check (`XeenJourneyCapture.h:29-30`,
  `XeenInnFlow.cpp:117-149`) rather than adding an owner.
  `XeenSessionOutcome` (`XeenSession.h:17-22`) carries the new
  `CloudsEnding` entry and the score; `XeenTitle.cpp` gains the ending
  branch next to the intro's (`:106-127`) and then opens a fresh World
  title, as DOS restarts the DAT. Other Options `V` enters the same
  program with the score Part 0 settles.
  DOS writes the won marker, saves, then turns Efx/Music on and computes
  the score. The save therefore retains the preceding sound switches.
  Mirror 40 joins after the victory/movement stores, still saves, then
  plays the ending and returns to the title. Mirror 62 ends the Event
  without a scene or continuation to its next line.
- **Ending save.** To the current slot (New Game already writes one,
  `XeenTitle.cpp:132-151`), at the point and with the content Part 0's
  member diff shows; developer sessions without a save target behave as
  M87's no-target Sign In. Capture is live-only and needs the camera map's
  monster table (`XeenGameplay.cpp:41-43`, `XeenJourneyCapture.h:66-67`),
  so adapt that capture to save the relocated camera without loading
  map 28 before the write. DOS flushes the live map-78 state and saves
  the ordinary live roster and party; the repeat archive diff leaves
  map 28 untouched. Capture every per-owner section from live state,
  including experience, conditions and award 42. No file-held-roster
  owner, retained snapshot or save-path splice is needed. A failed write
  retries the
  same captured snapshot (score and play time frozen at the first
  attempt), as `dev-inn-save-recovery`; the won store and the other side
  effects follow DOS's tail order, so quitting during a retry leaves what
  DOS would.
- **Missing or different `XEEN.EXE`.** The tail, the save and the won
  store still happen, then the M91-style notice
  (`XeenTitle.cpp:117,122-124`), then a fresh World title. Part B's interim
  notice ends the session the same way.
- **Save v21** (v20 -> v21): the play-time dword and a boolean
  `cloudsCompleted` after the purse (DOS party offset 604 is a completion
  word, normalized to a one-byte boolean in MMModern's own format)
  (`XeenSaveFormat.cpp:423`); `kFixedJourneyBytes` 8083 -> 8088
  (`:508`); the version pin in `tests/XeenFoodBankFlowTests.cpp:439`.
  Schema 9 and content 14 stay, as the v13 precedent (`f6074c2`); v20
  saves are refused clearly. v21 also admits an omitted camera-map monster
  table for an unvisited camera map (maintainer, 2026-10-26; v21 was never released).
  Capture and preflight require the table whenever the camera map has retained state.
- **Format audit.** An audit-only build, never committed, writing v20
  with the fixed clock must reproduce the four baseline M44 digests; the
  v21 saves differ only by the version field and the new field(s), with
  unchanged decoded state and traces. The maintainer then approves the
  new digests.
- **RNG.** The score reveal uses the EXE's own range and time-mixed
  helpers as Part 0 item 9 confirms, seeded from the injected clock and
  fixed in tests; never the Journey generator. Reveal digits are never
  compared with the video.
- **Presentation.** Original resources read at run time from the
  installed EXE and archives, texts through the original data, nothing
  copied into the repository; the caption strip only without a digital
  device, as in M91.
  The parade reads all 73 DOS selectors. Extend the presenter with the
  compressed-frame reader, horizontal merge/rotation and key
  acknowledgement holds; animation polls do not abort on Escape or
  clicks. The calibrated Part C estimate is 8-10 developer days,
  including integration and validation; reassess the existing cost stop
  if the decoder or choreography needs more work.
- **Decided now.** Part 0 answers within items 1-9 are implemented without
  asking, including texts, tables, timings, which state the post-ending
  rules test, and the extensions they need within the owners above.
- **Stop rules.** Stop for: a Journey schema or wire-layout change beyond
  the field(s) above; any other persistent state; a new gameplay state
  owner; a Journey generator draw that Part 0 does not attribute to a
  DOS call site; a digest difference beyond the audit; writing `XEEN.CFG`
  or any original file; an ending stage that needs a resource reader
  MMModern lacks and cannot add within the presenter; contradictory or
  unverifiable gameplay or save evidence; scope beyond this plan. EXE
  presentation details the statics cannot settle follow `ENDING.AVI`
  within the timing tolerance, without stopping. A Part 0 cost estimate
  above about ten developer days for the EXE ending also stops, so the
  maintainer can split it.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| 0 | DOS evidence for items 1-10, rules chapters | Completed and accepted 2026-10-25; runtime results and Beacon limitation in [M92 evidence](rules/evidence.md#m92-clouds-ending-evidence) |
| A | Wall clock, play-time counter, save v21, format audit, gated control panel clock and play time | Independent implementation review, suite, digest approval |
| B | Ending tail, score, ordinary live-roster ending save, won store, mirror 40/62, map-27 removal, Beacon refusal; until C the ending entry shows a notice and opens the World title | Independent implementation review, suite |
| C | The EXE ending on the presenter, score animation, title return, Other Options `V`; presets | Independent implementation review, suite, play-test |
| D | Closure: census, quirks, ScummVM differences, rules, status, history, roadmap, README, controls | Maintainer acceptance |

## Acceptance

- Rules: play time from a fixed clock across New Game, Save, Load and the
  Inn as Part 0 shows; the score equals `0071001722` for slot 05's six
  raw experience, class, permanent level and play time, copied into a
  unit test as plain values (never read from `D:/MModern-re`); the tail's state changes,
  facing and sound switches; `showtime` and `bloopers` as Part 0 shows;
  castle removal and Beacon refusal only after the ending; clocks hidden
  on panel entry, G/O/B/R activation, live redraw and reset on panel exit.
- Saves: exact round trip with play time: on a fixed clock Load then Save
  is byte-identical when the application's mark is also T; Save at T+n
  adds n. Also test Load after time at the title or since the last Save:
  DOS retains that older mark. The ending save reproduces the slot as
  Part 0's member diff shows, and after Load the kill XP, terminal
  conditions and award 42 remain as in DOS. v20 and malformed saves
  refused; a failed ending write retries the same
  snapshot without repeating the tail.
- Won store: written once at the ending, read at the title, missing or
  malformed files treated as not won; the player's real file and
  `XEEN.CFG` untouched by tests.
- Missing or different `XEEN.EXE` at the ending: tail, save and won store
  happen, then the notice and the World title.
- Original data: from a map-78 preset, kill Lord Xeen, use the Sixth
  Mirror, see the ending's stages in order on a fake clock within about
  half a second of `ENDING.AVI`, land on the World title with `V`
  offered, load the saved game at Vertigo, find Castle Xeen gone and the
  Beacon refused.
- Complete suite passes; M44 digests change only through the format audit.
- Maintainer play-test: the whole ending and score against `ENDING.AVI`,
  the panel clock, `showtime`, `bloopers`, Other Options `V`, and the
  post-ending world.
