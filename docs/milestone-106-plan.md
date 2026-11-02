# Milestone 106 plan - Darkside and World wins, Darkside ending

**Tier A. Status: completed and accepted by the maintainer (2026-11-02)**
after the play-test. Plan `aa09919`, amended `2cdba88` after the independent
plan review; Part 0 `dc76db6`, Part A `390a694`, Part B `77161a9`, each Part
independently reviewed. Step 4 of the [Darkside arc](roadmap.md#darkside-arc),
after [M105](milestone-105-plan.md), built on the Clouds ending of
[M92](milestone-92-plan.md) and the cutscene presenter of
[M91](milestone-91-plan.md) and [M93](milestone-93-plan.md). Save v24 -> v25;
Journey schema 9 and content 14 unchanged. The World ending's presentation
follows in step 5 (Tier B).

## Goal

Darkside of Xeen and World of Xeen can be won as in DOS. Bringing the Soul
Box with Corak inside to Alamar at map 51 `(2,13)` North reaches opcode 57,
which records the Darkside win, saves the party and plays the original
Darkside ending with the party's score, then returns to a fresh World title.
Opcode 58 (Darkside map 129, Clouds of the Ancients) records the World win and
saves as DOS does; its presentation shows "not supported yet" until step 5.
The ending mirror words and the title's replay rows work for every win DOS
offers them for.

Both wins went together (maintainer, 2026-11-01): they share the DAT tail,
one Part 0, one save bump and one Tier A cycle. The two long EXE presentations
were split so each Part stayed a normal size.

## Key decisions

- **DOS check first.** Part 0 settled both win tails, the mirror results, the
  title rows, the write boundary and the Darkside presentation from
  calibrated analysis of `WORLD/XEEN.DAT` and `WORLD/XEEN.EXE`, confirmed by
  an uninterrupted DOSBox capture
  ([evidence](rules/evidence.md#m106-part-0)). The pin was a guide only.
- **Reuse the Clouds ending owners.** `XeenEventFlow::startEnding` /
  `finishEnding`, `XeenEndingScore`, the forced Journey save, the settings
  store and the M91 presenter carry the new kinds; no new owner.
- **Won store as for Clouds** (maintainer, 2026-11-01): `darkside_won` and
  `world_won` join `won` and `darkside_intro_seen` in `settings.ini`,
  extending [`dev-clouds-won-store`](original-quirks.md#dev-clouds-won-store).
  Today's files still read, and every write keeps the other values.
- **Save v25** adds the Darkside and World completions; the DOS export writes
  PTY 606 and 608. The M44 digests changed once, after a byte audit the
  maintainer approved (two completion bytes plus envelope fields).
- **Approved approximations** (maintainer, 2026-11-01): the camera-only
  Darkside win save keeps map 51's song in Castleview until the next arrival
  ([`dev-darkside-load-music`](original-quirks.md#dev-darkside-load-music));
  the I do stage lasts about 0.65 s longer than DOS, since DOS's driver idle
  timing depends on sample placement
  ([`dev-m106-ido-boundary`](original-quirks.md#dev-m106-ido-boundary)).

## Results

- **Wins.** Opcode 57 sets quest 53, the won state and Darkside completion
  and moves the camera to Castleview `(25,21)` North; opcode 58 sets the won
  state and World completion without moving. Both make one forced save before
  the hand-off and show the M92 score
  ([win rules](rules/cutscenes.md#darkside-and-world-win-entry-and-save)).
  Nothing is written after the hand-off
  ([write boundary](rules/cutscenes.md#ending-write-boundary)).
- **Mirrors.** Darkside 33-36 and Clouds 60/61/40/62 dispatch Showdown,
  Genesis, Showtime and Bloopers as DOS does; the ending words save without
  completion or the move, and Bloopers only exits the Event
  ([mirror results](rules/cutscenes.md#ending-mirror-results)).
- **Title.** Other Options packs the E, V and W rows for every combination
  of wins; E replays the Darkside ending with the fixed score 1234567890 and
  writes nothing; W shows "not supported yet"
  ([rows](rules/cutscenes.md#other-options-ending-rows)).
- **Darkside ending.** The EXE ending plays with its stages, resources,
  songs, voices, captions and waits, unskippable animations, Ctrl-Q at the
  original polling points and mouth frames from a presentation random
  source; the score pages advance by keyboard only and match DOS frames
  pixel for pixel
  ([presentation](rules/cutscenes.md#darkside-ending-presentation)). The
  first-frame fade order is set per scene from the EXE, which also corrected
  the M92 Clouds throne reveal.
- **Tests.** Original-data runs of both wins from level-200 presets, exact
  reload and re-save, the score, completion propagation in every
  combination, every mirror result, failure retries, settings preservation,
  the eight title combinations, fake-clock presentation timing inside the
  DOSBox capture brackets and the real Alamar route through the whole
  ending. Complete suite 401/401.
- **Play-test** (2026-11-02, level-200 party, DOSBox comparison saves): the
  Alamar win and the full ending, the save at Castleview, the World win and
  its notice, a mirror word with Ctrl-Q, and the E replay all passed.

Left for later: the World ending presentation (roadmap step 5), and whether
the Clouds ending and the Darkside intro should also defer Ctrl-Q to the
EXE's keyboard polling points as the Darkside ending does.
