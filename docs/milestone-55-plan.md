# Milestone 55 plan - Interface gaps

**Tier A, in two parts. Status: completed and accepted.** Plan `3fec5ae`;
Part A `9d51e42`; Part B `9e4df22`; play-test fix `ef7ec84`.

## Goal

Make Strafe, Exchange, Quick Reference, Info and Quick Fight work through the
original CD interface, by mouse and keyboard, replacing their "not supported
yet" notices. Map, Quests, Dismiss, new spells, combat equipment/Use, party
recruitment and audio stayed out of scope.

## Final scope

- **Strafe (Part A).** Ctrl+Left/Right, keypad 4/6 and the lower corner
  buttons step sideways with the facing unchanged, through ordinary movement:
  walls and blocked steps, time, automatic Events and monster opportunities.
  A successful step flips the sky like a turn; combat ignores sideways
  movement and spends no turn.
- **Quick Reference and Info (Part A).** Q (main screen or character sheet)
  and the Quick Ref button show the party summary; I and the Info button show
  the date, 12-hour clock, weekday and active effects. Text comes from
  `WORLD/XEEN.DAT`; any key or click closes them, and neither changes the game.
- **Exchange (Part B).** E or the button on the character sheet asks whom,
  takes F1-F6 or a portrait, swaps the two members and follows the character;
  combat keeps the original refusal. Combat, Shoot, Rest, casting time,
  barrier selection and treasure delivery follow the current party order and
  write results back to the right character.
- **Quick Fight (Part B).** F or its combat button runs the acting member's
  configured action (Attack, Cast, Block, Run) and then advances normally;
  O or Options opens the original configuration dialog, whose highlight
  follows the selected member and returns to the actor on close.
- **Save v8 (Part B)** stores every character's quick action and remembered
  spell; schema 9/content 14 unchanged, v7 saves rejected clearly.

## Decisions

1. **DOS evidence over ScummVM** (maintainer DOSBox CD play, 2026-10-10):
   Exchange accepts only faces and the cancel button, and self-selection swaps
   nobody; Info shows the World of Xeen title; after a Run, Quick Fight
   Options keeps the old party indexing. That bug is reproduced on purpose and
   recorded in [original quirks](original-quirks.md), as is the Info title and
   Walk on Water format difference from the reference.
2. **Tier A** because Exchange replaced the fixed combat owner order with the
   party's current order (captured on combat entry), and Quick Fight needed
   persistent mutable settings.
3. **M44 digests.** Part A kept v7 and the digests. For v8, preserved Part A
   executables reproduced the three v7 hashes; traces and every common decoded
   field match, each save adds 60 bytes for the new settings, and only version,
   length and CRC also change. The maintainer approved the audit and the
   digest replacement.

## Results

- Tests for strafing, Exchange and reordered combat/Rest/Shoot/barrier/
  treasure owners, Quick Reference and Info rendering, Quick Fight actions,
  refusals and Options, input parity, and the v8 format and round trip.
- Full CTest 175/175 on the optimized build (runner, on the sources committed
  as `ef7ec84`), and Debug `ctest -L fast` 141/141 plus the three M44
  scenarios 3/3 on `ef7ec84`.
- Independent implementation review done (Codex).
- Maintainer play-test against the DOSBox CD edition passed. The strafe sky
  flip and a double highlight in Quick Fight Options were fixed in `ef7ec84`.
