# Milestone 77 plan - Combat items, Quests, Awards and skill indicators

**Tier B. Status: planned.** Step 7 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M76](milestone-76-plan.md). Baseline `6f049f5`.

## Goal

The party can manage and use its items in combat, read its quests and
awards, and the three skill indicators on the border work, all as in the
original:

- **Combat items:** Equip, Remove, Discard and transfer in the combat Items
  dialog, the combat Use button and Items Use, with the original category
  limits, refusals and turn cost; charged items cast their spell in combat
  on the existing M73-M76 effects. Town Portal from an item in combat
  continues into its town dialog, as DOS does (M76 keeps a refusal).
- **Quests:** the Quests screen from the main-screen button and the Items
  Quest button: Quest Items, Current Quests and Auto Notes, scrolling, Exit.
- **Awards:** the Awards screen from the character sheet, with Up, Down,
  Exit and member selection.
- **Skill indicators:** Danger Sense (border `(107,9)`, monster nearby), Spot
  Secret Doors (`(194,91)`, secret passage ahead) and Direction Sense (facing
  letter below the view), today drawn inactive.

Out of scope: Dismiss (maintainer decision, 2026-10-18: it comes with the
Inn in step 8, since nothing can bring a dismissed member back yet), party
and character creation, Choose123/HowMuch, control-panel audio toggles,
intros and endings, map admission, audio. No save change expected: the
format stays v15 (items, quest items, quest and world flags and awards are
already saved).

## Tasks

- [ ] **Part 0 - DOS check (read-only, delegated).** Static analysis of
      `WORLD/XEEN.DAT` against the pin, with offsets: which item categories
      and actions combat allows (Equip, Remove, Discard, transfer, Use), what
      each refuses and whether it spends the member's turn or a round; the
      combat Use button's item mode; how Town Portal's item cast continues
      in combat; the Quests screen sources for each page (quest items, quest
      flags and the Auto Notes source) and their texts; Awards sources and
      layout; each indicator's condition (Danger Sense range and monster
      states, Spot Secret Doors' cell/wall test, Direction Sense's letter
      and when it shows) and their animation (the pin animates Danger Sense
      with Spot Secret Doors' frame counter). Record differences in
      `docs/original-quirks.md` (and `docs/scummvm-differences.md`) and in
      the matching `docs/rules/` chapter; unsettled cases go to the
      maintainer before Part A. If a page needs state the save lacks, stop.
- [ ] **Part A - combat items.** The Items dialog and the Use button in
      combat on the existing inventory and casting owners; Town Portal's
      combat item path.
- [ ] **Part B - Quests and Awards.** Both screens with the original
      presentation, mouse and keyboard.
- [ ] **Part C - skill indicators.** The three indicators on the border from
      the party's skills, live monsters and map data, in exploration and
      combat as Part 0 establishes.
- [ ] Developer test save `combat-items` (as `party-healing`): mainland near
      monsters, weapons and armor to swap, charged items including Town
      Portal, quest items, a few awards and quest flags, members with Danger
      Sense, Spot Secret Doors and Direction Sense; README entry.
- [ ] Update the manual screen checklist in `docs/coverage.md` (also the
      M76 dialogs still listed as unsupported) and regenerate the report.
- [ ] Project status: drop the closed known gaps at closure.

## Acceptance

- Unit tests: each combat item action and refusal, turn cost, Use in combat
  for a damage, a healing and a travel item, Quests page contents and
  scrolling, Awards listing, each indicator's condition on and off.
- Original-data tests: equip and Use in mainland combat, Town Portal from an
  item in combat reaching Vertigo, the Quests screen after Myra's request,
  Danger Sense near a mainland monster. A save/load round trip after a
  combat item use stays exact.
- Census checklist rows for Awards, Quests, Items and the combat main screen
  updated; Dismiss still refuses with "not supported yet".
- Complete `build-rel` suite through the runner (`tools/AGENTS.md`).
- Maintainer play-test with `combat-items`: every combat item action, the
  Use button, Town Portal in combat, Quests and Awards, the three
  indicators while walking.

## Risks

- If combat item actions need turn-scheduling changes beyond the existing
  combat owner, stop and report before widening (lesson from M75).
- Spot Secret Doors needs a reliable secret-passage test on the map data; if
  Part 0 cannot settle it statically, it goes to the maintainer, possibly
  for a DOSBox check.
- If Part 0 shows the scope is larger than medium, the maintainer decides
  whether M77 splits (for example, indicators into their own milestone).
