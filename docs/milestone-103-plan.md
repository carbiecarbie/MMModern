# Milestone 103 plan - Darkside Events

**Tier B. Status: planned; not started.** Step 2 of the
[Darkside arc](roadmap.md#darkside-arc).

## Goal

Every Event opcode that the ordinary Darkside maps (1-112) still refuse runs
as the original does, on the existing Event decoder, interpreter, world and
EventFlow owners, so that the census lists none of them as unsupported or
partial on those maps. The mixed World of Xeen maps 113-128, FlipWorld (59)
and the ending opcodes (57, 58) stay out (steps 3 and 4).

| Opcode | Darkside gap today (census) |
| --- | --- |
| 51 ExchObj | 68 uses on maps 30, 45-47, 53, 73-75, 77; outside the decoder subset |
| 56 GotoRandom | 9 uses on maps 3, 7, 33; outside the decoder subset |
| 44 GiveEnchanted | 92 rows: weapons, armor, accessories and quest items; only miscellaneous codes 70/71 are decoded |
| 48 ChooseNumeric | 61 uses on 31 maps; outside the decoder subset |
| 52 FallToMap | destinations 40, 49, 61, 62, 63 and 89 on 13 maps; outside the decoder subset |
| 55 ConfirmWord_2 | 1 use on map 72; outside the decoder subset |
| 34 MoveWallObj | maps 54, 55: the wall item named is beyond the map's wall-item count |
| 42 IfMapFlag | map 29, monster 31: the Event preflight refuses it |

## Tasks

- [ ] **Part 0 - DOS check.** Calibrated static analysis of
  `WORLD/XEEN.DAT` for each opcode above, against the pinned reference:
  ExchObj (the reference changes a copy and does nothing; DOS writes object
  x and y at `0x28a86..0x28ad6`, [evidence](rules/evidence.md)), the
  GotoRandom draw and its range, the GiveEnchanted category split and
  record fields, the ChooseNumeric prompt, keys and line table, FallToMap's
  destination, position, Event end and fall presentation, ConfirmWord_2
  against ConfirmWord, MoveWallObj on a slot beyond the count, and what
  IfMapFlag does on map 29. Record the findings in
  `docs/rules/evidence.md` and each difference from the reference in
  `docs/original-quirks.md` before Part A. Questions DOS cannot settle go
  to the maintainer, except condition-counter cases, which follow ScummVM.
- [ ] **Part A - ExchObj, GotoRandom, GiveEnchanted.** ExchObj exchanges two
  objects' positions as DOS does, through the object overrides MoveObj
  already saves. GotoRandom draws once through the Journey RNG and jumps.
  GiveEnchanted accepts every item category DOS accepts and writes the item
  through the existing treasure arrays.
- [ ] **Part B - ChooseNumeric and FallToMap.** ChooseNumeric shows the
  original choice with the existing blocking-presentation policy (mouse and
  keys) and jumps to the chosen line. FallToMap ends the Event and starts
  the fall through `XeenEventFlow::beginFall`, reusing the M86 presentation
  and arrival rules
  ([time and conditions](rules/time-and-conditions.md)).
- [ ] **Part C - ConfirmWord_2, MoveWallObj, map-29 IfMapFlag.** Each as
  Part 0 settles it, on the existing ConfirmWord, wall-item and IfMapFlag
  paths.
- [ ] Regenerate `docs/coverage.md` with `mmodern_census` after each part.

## Decisions and risks

- **Save v23 unchanged** (schema 9, content 14) is expected: object and
  wall-item positions, treasure and fall destinations already have saved
  owners, and no Event pauses across a save. If Part 0 finds state that must
  be saved, stop and amend the plan first (a format change is Tier A).
- GotoRandom adds one draw; the M44 digests must stay unchanged because no
  Clouds Event uses opcode 56. Any digest change is a defect to explain.
- A pick that leaves the plan's scope (a new presentation owner, a monster
  or map rule the mixed maps need) stops the part and goes to the maintainer.

## Tests and acceptance

- Unit tests for each opcode's rule as Part 0 settles it, including
  GotoRandom's range and draw count, every GiveEnchanted category, and
  ChooseNumeric's cancel and each choice.
- A few original-data tests on Darkside maps: an ExchObj, a ChooseNumeric
  choice, a FallToMap fall to its destination, and a GiveEnchanted weapon or
  armor reward, each surviving a save and load round trip.
- The census lists none of the opcodes above as refused on maps 1-112.
- Complete suite passes on `build-rel`.
- One maintainer play-test after Part C (`docs/playtests.md`), only for
  what nothing cheaper settles: the ChooseNumeric prompt and its input feel,
  and the FallToMap presentation against DOSBox.
