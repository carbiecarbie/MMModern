# Milestone 95 plan - Clouds audit follow-up

**Tier B. Status: completed and accepted** by the maintainer on 2026-10-28
after the play-test. Plan `2a91e8b`, Part A `6647088`, Part B `fcbc32d` and
the closure commit. Roadmap
[step 1](roadmap.md#next-arc-release-quality-and-the-darkside-start).
Save v21, schema and content unchanged.

## Goal

While an Event waits, the small map and the border show what it has changed
so far, as in the original, and the Clouds audit's invariants become
permanent original-data tests.

## Scope

- **The audit** ran on 2026-10-28 in four sessions. Results and reproductions
  are in `D:/Projetos/MModern/MModern-re/clouds-audit-20261028/`.
  - It ran 8,034 Clouds Event runs, with second-use, save round-trip, frame,
    party-variant and interruption checks.
  - It found one defect, the hidden-object save failure, fixed in `3020bbe`
    (release 0.10.1).
  - It left one open question, Q01, which this milestone fixed.
- **Excluded.** These are the audit's recorded limits:
  - the five combat boundaries without a post-combat wait;
  - the gated reward paths it left uncovered;
  - full certification of overlays, lighting, audio and device timing;
  - temporary walls and Warzone lifetimes after leaving;
  - an exhaustive opcode comparison with the pin.

## Key decisions

- **One live state, as in DOS.**
  - The original redraws the 3D view, the border and the small map from its
    one live state during a wait
    ([evidence](rules/evidence.md#event-wait-drawing)).
  - MMModern still runs Events on a candidate and now draws those layers
    from it. Portraits, controls and dialogs stay live, and nothing is
    published early.
  - The fix only changed drawing, so the milestone stayed Tier B.
- **Tests over sweeps.** A few original-data cases pin each invariant, and
  the audit's exceptions are kept. Each new check was shown to fail under a
  temporary mutation.

## Results

- **Part A.**
  - At the Witch Tower Level 4 skull, the small map shows the passage that
    Rosebud opens during its confirmation, with Wizard Eye. It shows the
    passage closed by a wrong word during "Wrong. Begone!".
  - The border's Levitate, Clairvoyance, resistance, buff and Direction
    Sense elements also follow the candidate.
  - The `m95-witch-skull` preset serves the play-test.
- **Part B.** The `xeen_clouds_invariant_*` tests, plus extended dispatch,
  opening and value tests, pin:
  - second use (I1);
  - completion (I2);
  - exact restore (I3);
  - leaving and returning (I5);
  - small parties and full packs (I6);
  - interruptions and queued input (B1/B2);
  - the Misc material test, recorded as
    [a difference from the pin](original-quirks.md#sv-if-misc-material).
- The complete suite passed 359/359 (run `20261028-205752-a7d2cc56`). The
  M44 digests are unchanged.

## Acceptance

The maintainer play-tested three cases and accepted the milestone: the skull
with Wizard Eye, the wrong word, and an ordinary trap. The DOS rule rests on
calibrated static evidence, so there was no DOSBox item.
