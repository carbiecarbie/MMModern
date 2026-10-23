# Milestone 86 plan - Clouds presentation loose ends

**Tier B. Status: planned (not started).** A small milestone before roadmap
[step 14](roadmap.md#next-arc-finishing-the-games-systems), after
[M85](milestone-85-plan.md). Save v18, schema 9 and content 14 unchanged.

## Goal

Close four small presentation gaps left in Clouds, as the original shows
them: the open door seen for an instant when the party walks through it,
the falling animation with its scream and landing sounds, Moon Ray's healing
member by member with its sparkles, and the border indicators restarting
after a service screen. Mr Wizard in combat stays out: it changes combat's
exit (Tier A) and goes to its own maintenance fix with an independent
review. Planning and implementation may be one task; no independent review
is mandatory.

## Tasks

- [ ] **Open door.** On an admitted indoor forward move through a front door
  wall (types 4 and 13), draw the original one-frame open-door view with the
  door sound (FX 47, already played) before the step, as DOS does
  (`0x0cf93..0x0cfb7`; pin `checkMoveDirection`). Confirm statically which
  sprite frame is drawn and how long it stays before reproducing it.
- [ ] **Falling.** Every fall (sky and clouds surfaces, scripted falls)
  plays the original animation: falling FM 11 (`0x11692`), `scream.voc`
  (`0x116a0`) during the animation, the sample stopped (`0x116ec`),
  `unnh.voc` (`0x1170c`) and landing FM 31 (`0x11716`), around the existing
  terrain transition (`XeenEventFlow::prepareTerrainTransition`,
  `XeenEncounterFlow::beginTerrainTransition`). Confirm the frames, scroll
  and timing statically from the fall routine (pin `startFalling`,
  `handleFalling`) before implementing; damage, destination and RNG stay as
  today.
- [ ] **Moon Ray.** Interleave per member as DOS does
  (`0x30aa1..0x30adf`): cue 30, `U(1,30)`, healing and its sparkle
  (`0x2ee3a..0x2eed6`, `0x21087..0x21185`) for one member before the next.
  Update the quirk row
  [`sv-moon-ray-preparation`](original-quirks.md#sv-moon-ray-preparation).
- [ ] **Border indicators.** Zero the Levitate, Spot Secret Doors and Danger
  Sense counters with the statue frames where DOS does on service entry and
  exit (`0x1c402..0x1c410`, `0x1d4c4..0x1d4d2`), so their animation restarts.
- [ ] Tests: unit tests for each rule (the door frame and its order with the
  step, the fall sequence and its cues, Moon Ray's draw and heal order, the
  counter resets); an original-data fall from a cloud map and a door in
  Vertigo; the generic exact-restore test unchanged.
- [ ] Docs: [rules](rules/audio.md) and quirk rows for each settled rule;
  remove the four gaps from project status at closure.

## Rules for the implementer

- DOS evidence (static, calibrated, offsets) settles each rule; the pin is
  comparison only. A rule DOS cannot settle follows the pin and is recorded.
- Stop and report if a task needs a saved field, changes RNG order or the
  Journey clock beyond presentation, or touches combat exit.
- M44 digests: a changed digest needs its first divergence explained against
  a rebuilt baseline and the maintainer's approval in the implementing
  session.

## Acceptance

All tasks done; affected tests and `ctest -L fast` while iterating; one
complete `build-rel` suite passing as [tools/AGENTS.md](../tools/AGENTS.md)
requires; M44 digests unchanged or replaced with approval. The maintainer
play-tests a door in Vertigo, a fall from a cloud map, Moon Ray in a fight
and the border indicators after leaving a shop, from presets if needed, and
accepts the milestone.
