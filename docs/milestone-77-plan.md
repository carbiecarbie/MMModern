# Milestone 77 plan - Combat items, Quests, Awards and skill indicators

**Tier B. Status: completed and accepted by the maintainer (2026-10-19)**
after a maintainer play-test. Plan `e63502b`, DOS check and plan amendment
`3397294`; implementation `2976c6e` (A), `672270b` (B), `c7c1828` (C);
play-test fixes `0c89957`. Step 7 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M76](milestone-76-plan.md).

## Scope

Item management and Use in combat, including Town Portal from an item; the
Quests and Awards screens; the three skill indicators on the border (Danger
Sense, Spot Secret Doors, Direction Sense). Out of scope: Dismiss (moved to
the Inn step), party and character creation, Choose123/HowMuch,
control-panel audio toggles, intros and endings, map admission, audio. The
save format stays v15: every source was already saved.

## Key decisions

- **DOS check first** (Part 0, static analysis of `WORLD/XEEN.DAT`): the
  rules match the pin except Discard's Xeen Slayer refusal, the Quests and
  Awards scroll keys (arrows and keypad, plus Home/End/Page keys on Quests;
  no U/D) and Danger Sense's animation phase. Evidence is in
  [items](rules/items.md#the-items-dialog-in-combat),
  [world Events](rules/world-events.md#quests-and-awards-screens),
  [magic](rules/magic.md#border-and-information-screens),
  [evidence](rules/evidence.md#m77-combat-items-quests-awards-and-skill-indicators)
  and [original quirks](original-quirks.md).
- **Combat items**: Equip, Remove, Discard, transfer and Quest are free
  actions; only a successful Use ends the member's turn. Town Portal from an
  item in combat continues into its dialog and loads the town; combat ends
  at the first attacker re-evaluation after the load, with the ordinary
  end-of-combat minute (maintainer decision, no DOSBox check).
- **Indicators**: Danger Sense comes from the existing monster-movement
  pass (any monster within three cells, whatever its state), Spot Secret
  Doors from the indoor scene setup (a plain wall ahead with open space
  behind it; outdoors it keeps its last indoor value) and Direction Sense
  hides under text and number prompts. Their state is presentation only,
  never saved; the two animated indicators use separate counters in phase.
- **Play-test fixes** (DOSBox screenshots): Town Portal's dialog is the
  original window 20, the combat Use button works by mouse, and item names
  use the original formatter: Cursed in yellow and Broken in red in item
  lists, and the Discard prompt's colored name.

## Results

- The census checklist marks the combat main screen, Items, Quests and
  Awards rows; Dismiss still refuses with "not supported yet".
- Developer test save `combat-items` (README) covers every combat item
  action, the Use button, Town Portal in combat, Quests, Awards and the
  three skills. No walkable Spot Secret Doors wall exists in the open areas
  (none in Vertigo); `--event-fixture` on Rivercity shows it.

## Validation

- Unit and original-data tests for each item action, refusal and turn
  cost, Use in combat for damage, healing and travel items, Quests and
  Awards contents and scrolling, each indicator on and off, the Town Portal
  window, combat button clicks and item-name colors; save/load round trips
  stay exact.
- Complete `build-rel` suite through the runner: 247/247 for Part C and for
  the play-test fixes.
- Maintainer play-test with `combat-items`.

## Open after closure

- The border counters are not zeroed when service screens redraw (DOS
  `0x1c402`, `0x1d4c4`); Levitate's counter never was either.
- The item prompts other than Discard follow the pin's color 15 without a
  DOSBox check.
