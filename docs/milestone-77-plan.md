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
      *Part 0 findings (2026-10-18, [rules](rules/items.md#the-items-dialog-in-combat),
      [evidence](rules/evidence.md#m77-combat-items-quests-awards-and-skill-indicators)):
      the DOS rules match the pin except Discard's Xeen Slayer refusal, the
      Quests/Awards scroll keys and Danger Sense's frame phase; every source
      is already saved; one open question (below).*
- [ ] **Part A - combat items.** The Items dialog and the Use button in
      combat on the existing inventory and casting owners; Town Portal's
      combat item path. Part 0 adds: Equip, Remove, Discard, transfer and
      Quest are free actions in combat and only a successful Use ends the
      turn; the main-screen Use dialog ignores F1-F6 while the sheet's
      Items dialog transfers within the combat party; Use from the sheet in
      combat shows the original refusal; Discard also refuses the Xeen
      Slayer Sword ([quirk](original-quirks.md#sv-discard-xeen-slayer)).
      The combat state after Town Portal's arrival follows the maintainer's
      answer to the open question.
- [ ] **Part B - Quests and Awards.** Both screens with the original
      presentation, mouse and keyboard. Part 0 adds: Up/Down arrows and
      keypad 8/2, Home, End, Page Up and Page Down scroll; no U/D letters
      ([quirk](original-quirks.md#sv-quest-award-scroll-keys)); Awards has
      no member selection of its own; `qnotes.bin` and `award.bin` come from
      the Darkside archive ([rules](rules/world-events.md#quests-and-awards-screens)).
- [ ] **Part C - skill indicators.** The three indicators on the border from
      the party's skills, live monsters and map data, in exploration and
      combat as Part 0 establishes: Danger Sense from the monster-movement
      pass (any monster within three cells on both axes, whatever its
      state), Spot Secret Doors from the indoor scene setup (facing wall
      value 5 or 8 of the party's cell, one cell ahead), Direction Sense
      hidden while a text prompt is open; separate 12-frame counters in
      phase ([rules](rules/magic.md#border-and-information-screens),
      [quirk](original-quirks.md#sv-danger-sense-frame)). No DOSBox check is
      needed for the secret-passage test.
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
- Spot Secret Doors' secret-passage test is settled statically (Part 0);
  the indicator keeps its last indoor value outdoors in DOS and the pin,
  which only a DOSBox observation could contradict.
- If Part 0 shows the scope is larger than medium, the maintainer decides
  whether M77 splits (for example, indicators into their own milestone).
  Part 0 found no reason to split.

## Maintainer decision (Part 0, 2026-10-19)

**Town Portal from an item in combat.** DOS continues into the town dialog,
loads the town and finishes the Use like any other (charge spent, turn
ended, dialog closed). Combat then ends when the combat loop finds no
attackers, which the scene setup decides after the load; whether one more
combat turn is shown in town cannot be settled statically. Decision: Part A
ends combat at the first attacker re-evaluation after the town load, with
the ordinary end-of-combat minute; no DOSBox check first. See
[the evidence](rules/evidence.md#open-questions-from-this-check).
