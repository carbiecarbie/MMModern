# Milestone 86 plan - Clouds presentation loose ends

**Tier B. Status: completed and accepted by the maintainer (2026-10-23)**
after the play-test. Plan `4542db9`, implementation `661fdcc` and the
closure commit. A small milestone before roadmap
[step 14](roadmap.md#next-arc-finishing-the-games-systems), after
[M85](milestone-85-plan.md). Save v18, schema 9 and content 14 unchanged.

## Scope and results

Four presentation gaps left in Clouds now look and sound as in DOS:

- **Doors.** Walking forward through an indoor door plays the door sound and
  redraws the view with the door open, held for at least two BIOS ticks,
  before the step. Blocked, backward, sideways and outdoor moves do not.
- **Falling.** A fall from the sky or a cloud surface scrolls 15
  accelerating images and four impact images, one per retrace, with falling
  FM 11 and the scream, the scream cut, `unnh` and landing FM 31. The fall
  damage and its sound come after the last image; destination, damage and
  RNG are unchanged.
- **Moon Ray.** Each active member, dead, petrified and eradicated ones
  included, gets cue 30, a draw, the healing and its sparkle before the next
  member. The RNG sequence is unchanged.
- **Border indicators.** Levitate, Spot Secret Doors and Danger Sense
  restart at every service entry and at each full redraw of the service
  art, then carry on from there after the party leaves.

## Key decisions

- **Static DOS evidence** (high confidence, calibrated on the installed CD
  program) settled each rule: door `0x0cf93..0x0cfb7` and the redraw's hold
  `0x11873..0x11880`; fall `0x11692..0x11782`; Moon Ray `0x30aa1..0x30adf`
  with healing `0x2ee3a..0x2eed6` and sparkle `0x21087..0x21185`; service
  resets `0x1d4c4..0x1d4d2` and `0x1c3fc..0x1c414`.
- **Corrections to the planned scope.** DOS has no reset when a service
  closes, only at entry and at the art's full redraws. Scripted falls were
  dropped: no Clouds Event uses opcode 52 (FallToMap, used only in
  Darkside), so it stays unsupported, and the fall presentation is one
  shared entry a future FallToMap can call (maintainer, 2026-10-23).
- **Follows the pin** where the DOS branch was not isolated: the door frame
  (`fwl2` frame 10 in towns, 1 elsewhere) and the service-window mapping.
  The pin's linear, shaking fall is a recorded
  [ScummVM difference](original-quirks.md#sv-fall-presentation).

The rules are in [world Events](rules/world-events.md#opening-doors-and-service-borders),
[time and conditions](rules/time-and-conditions.md#falling-presentation),
[magic](rules/magic.md#moon-ray-healing-presentation) and
[audio](rules/audio.md); the quirk rows, including
[Moon Ray](original-quirks.md#sv-moon-ray-preparation), are in
[original quirks](original-quirks.md).

## Validation and acceptance

No independent review was required (Tier B). The complete `build-rel` suite
passed 283/283 as [tools/AGENTS.md](../tools/AGENTS.md) requires, rerun on
the final build after a Rest failure in the first run was fixed. The M44
audit left all four digests and checkpoints unchanged; the services and
temple traces only gain repeated held-door frames. Presets
`m86-playtest-fall`, `-moon-ray` and `-border` are in
[development](development.md). The maintainer play-tested a Vertigo door, a
fall from Clouds map 26, Moon Ray in a fight and the indicators at the
Vertigo Training, and accepted the milestone on 2026-10-23.
