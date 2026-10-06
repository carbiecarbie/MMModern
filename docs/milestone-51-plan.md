# Milestone 51 plan - Rest, food and daily time

**Tier A. Status: completed and accepted.** Plan `0f09473`; Part A `b8856c1`;
Part B `55633c1`; review fixes `cffb2c3`.

## Goal

The prepared party can Rest as in the original, repeatedly, from the
exploration button or R, with food, HP/SP recovery, time advance, monster
interruption and the daily processing that resting crosses, and play
continues normally across days and years.

## Final scope

- **Daily time (Part A).** Shared `changeTime`/`addTime` processing
  (`Party::changeTime/addTime`): the eight-hour condition block runs once per
  call; dawn and `newDay` with the needs-rest message and Weak; Confused and
  Paralyzed checks; day 100 rolls into the next year; merchant restock and
  bank interest on the original conditions; night sky. The old day-8..99 /
  year-610 limits were replaced by this processing in travel, combat, Shoot,
  casting, Bash/unlock and services. Age follows the current year.
- **Food and state.** Food is read from the correct party offset (618..619;
  the earlier loader read the light byte) and starts at the original 90.
  Food and fire/energy/magic resistances became saved live state; `resetTemps`
  clears the original temporary fields.
- **Rest (Part B).** Original entry (button and R; R stays Run in combat and
  Repair in the Ironworks lobby), primary-map rest restriction, the "some
  characters may die" confirmation, ten charge opportunities and 480 minutes,
  the dream roll and visual sequence, `resetTemps`, active-order food and
  recovery (starving per member), the original Weak/Drunk order, terrain
  consequences (desert +170 minutes unless indoors or with a Navigator) and
  the completion message. Nearby monsters move and attack while the party
  sleeps; an interrupted Rest keeps its elapsed time and skips completion,
  food and recovery, and members who were not hit stay asleep.

## Decisions

1. **Weak/Drunk** follows the original (replacement, no post-refill cure).
2. **Condition counters** are unsigned bytes with explicit wrap and
   saturation. For byte `0xFF` the pinned ScummVM `-1` sentinel is followed
   in comparisons, dawn increments and stat modifications; this is not
   confirmed in DOS and is covered by the AGENTS.md rule for condition
   counter edge cases (added during this milestone).
3. **Poison/Disease** eight-hour branch follows the pin, which only draws
   for members without the condition; a DOSBox check remains open.
4. **Save v6** adds food and the three resistance pairs (182 bytes); the
   M44 digests changed by these format bytes only, with complete traces
   identical.
5. Unsupported terrain (lava, sky, cloud fall, space) refuses Rest before
   any change; none is reachable in the current mainland or Vertigo.

## Results

- Rules oracles for food, recovery, time, daily effects, byte counters and
  calendar boundaries; fixed-seed interruption tests (interrupting charge,
  actor positions, RNG count, wake reasons); original-data Rest in Vertigo
  and on the mainland; mid-sequence save round-trips.
- Full CTest 160/160 (Part A) and 162/162 (Part B and review fixes), single
  job, lightweight runner; fast suite 129/129.
- Independent implementation review (REVISE): terrain checked only after
  time was published, `0xFF` subtracted as 255 in stat modifications, weak
  interruption tests, dream fade rounding and background timing, and
  Training's partial reset; all fixed. Physical saving throws now use
  effective Luck including condition penalties, as in the pin.
- Maintainer play-tests of Part A (days, dawn, services) and of Rest.
