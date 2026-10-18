# Milestone 49 plan - Combat rules fidelity

**Tier A. Status: completed and accepted.** Plan `cf62bf6`; implementation
`804fc5a`.

## Goal

Correct three combat rules that differed from the original, generically and
through the existing combat authority: monster attack targeting and count,
damage published at projectile impact, and turning the party during combat.
No new spells, monster abilities, areas, combat equipment use or audio; whole
Vertigo admission stays in M50.

## Final scope

- **Monster attacks.** Root cause: hatred 16 (HATES_NOBODY) was read as a
  whole-party attack, and true hatred 15 (HATES_PARTY) was mishandled, so the
  Slime's two attacks became two passes over the whole party. Now, per attack
  and for every monster record (`Combat::monstersAttack`/`doMonsterTurn`,
  `combat.cpp:852-930`): hatred 15 hits every member in order, including
  disabled and dead ones; class hatred picks the first eligible match (12 =
  dwarf); otherwise a random member, redrawn among eligible members if the
  first pick is disabled. Melee and ranged share one attack-count loop.
  Physical roll 20 can injure the same target twice, as in the reference.
- **Impact timing.** Shoot, Magic Arrow and enemy volleys prepare their
  outcomes privately, keep targets present through travel and the hit frame,
  then publish HP, show the post-HP frame, and only then publish death, XP,
  drops and removal, exactly once (`attack2`, `rangedAttack`). Fallible
  preparation (including lethal accounting) happens before any store. Shoot
  lanes advance on every reference scene draw while `_charsShooting` holds.
  Timing adds or removes no RNG draw.
- **Turning in combat.** Left/Right (keys and buttons) rotate the party when
  ready, flip the sky, set the party-stepped state and, if movement is owed,
  let monsters move once; the acting member, initiative and Block are kept,
  and no time is charged (`Interface::doCombat`). Camera guards still reject
  any unauthorized change.

## Decisions

1. **Scenario amendment (maintainer-approved, 2026-10-04).** The services and
   Temple M44 scenarios relied on the old defect (the Slime hitting unconscious
   members) to break armor and kill a member. With their old inputs, both
   traces matched the baseline exactly up to the first corrected target
   choice. They were then re-routed to earn broken armor and death in
   ordinary Ogre combat, keeping paid Fix, Training, Heal and resurrection,
   without injected state. The mainland scenario kept its inputs; all 47
   changed bytes were explained (HP, armor, conditions, XP, time, RNG,
   positions, checksum). The three digests were regenerated once, after
   item 1; items 2 and 3 left them unchanged.
2. **Movement countdown.** A positive countdown at an ordinary combat command
   is unreachable in current content: combat entry flushes it
   (`interface.cpp:1637-1639`, `combat.cpp:472`) and only `chargeStep` re-arms
   it. Positive-countdown tests are explicitly artificial; ordinary input
   covers zero.
3. **No save-format change.** Impact and rotation work is transient and never
   saved.

## Results

- Selector/count oracle over every original Clouds monster record and all 256
  attack-count values; ordered frame oracles for nonlethal, lethal and
  multi-shooter Shoot; failure controls proving a failed lethal preparation
  leaves HP and RNG unchanged; rotation in every facing.
- New digests: mainland `d70e358e…`, services `6cf28764…`, temple `b621a290…`,
  with exact reload/re-save. Other changed test expectations (for example the
  service-day trace, now 17 draws with seed 1114) were audited one by one.
- Full CTest 153/153 (single job, lightweight runner); `ctest -L fast` 121/121.
- Independent implementation review (REVISE) found Shoot storing HP before
  preparing lethal accounting and frozen arrow lanes during hits; both fixed,
  then approved by a second context-free review.
- Maintainer DOSBox comparison passed: the Slime hits two members, arrows
  arrive before damage and a killed monster stays visible until impact, and
  the party turns in combat.
