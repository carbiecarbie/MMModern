# Milestone 103 plan - Darkside Events

**Tier B. Status: completed and accepted by the maintainer (2026-11-01)**
after the play-test. Plan `c78ed94`, DOS check (Part 0) `324c41b` and its
fall-damage follow-up `cb64313`, exchanges, random jumps and enchanted
rewards (Part A) `a3119e5`, numbered choices and scripted falls (Part B)
`f136b72`, second password, unused wall slots and signed map-flag jumps
(Part C) `db9c2c8`, play-test fixes `27eefe2`, and the closure commit. Step 2
of the [Darkside arc](roadmap.md#darkside-arc).

## Scope

Every Event opcode the ordinary Darkside maps (1-112) still refused, on the
existing Event decoder, interpreter, world and EventFlow owners:

| Opcode | Darkside uses |
| --- | --- |
| 51 ExchObj | 48 on nine maps |
| 56 GotoRandom | 9 on maps 3, 7, 33 |
| 44 GiveEnchanted | weapons, armor, accessories, Misc and quest codes |
| 48 ChooseNumeric | 61 on 31 maps |
| 52 FallToMap | 220 on 21 maps |
| 55 ConfirmWord_2 | 1 on map 72 |
| 34 MoveWallObj | 7 writes beyond the wall-item count on maps 54, 55 |
| 42 IfMapFlag | map 29's signed jump lines |

The mixed World of Xeen maps 113-128, FlipWorld and the ending opcodes 57
and 58 stayed out (steps 3 and 4).

## Key decisions

- **DOS check first** (Part 0, calibrated static analysis of
  `WORLD/XEEN.DAT`; [evidence](rules/evidence.md#darkside-events-m103-part-0)).
  DOS differs from the reference in ExchObj (DOS moves the live objects),
  GiveEnchanted's Misc test and high codes, ChooseNumeric (Escape ignored,
  last number cancels, compact Text-off layout), FallToMap damage, unused
  wall slots and signed jump lines; each is in
  [original quirks](original-quirks.md#darkside-event-rules-established-from-dos).
- **Fall damage** is the byte DOS reads after the three-parameter record,
  usually the next record's length (217 of 220 falls). The three records at
  the end of their Event resource use 0
  ([approved deviation](original-quirks.md#dev-fall-damage-zero)).
- **Event windows close when the run ends** (play-test, 2026-11-01): DOS's
  CloseAll removes every Event window, not only NPC windows; door and sign
  labels stay until the party moves. This replaces the text retention
  accepted in M18, for Clouds too
  ([rule](rules/world-events.md#event-dispatch)).
- **"Oops..." while falling** (maintainer, 2026-11-01): the falling variant
  of the loading box is kept for four retraces, in terrain and Event falls
  ([approved deviation](original-quirks.md#dev-fall-oops)).
- **Save v23 unchanged** (schema 9, content 14): exchanged objects use the
  saved object overrides, and unused wall slots need no saved state.

## Results

- The census lists none of the eight opcodes as unsupported or partial on
  maps 1-112. M44 digests unchanged in every part and after the fixes.
- Complete suite 392/392 after each part and after the play-test fixes.
- The maintainer play-tested a numbered choice with Text on and off and the
  map 8 fall against DOSBox. The choice background (global frame 7 hides the
  command icons), the window close and the "Oops..." box were fixed; the
  retest and three Clouds window-close comparisons passed.
