# Milestone 46 plan - Clickable main screen

**Tier B. Status: approved by the maintainer on 2026-10-03.** Baseline: `main` at `8d0daa0`.

## Goal

Play the current Journey's exploration and combat with the mouse on the
original main screen, as in the original game, with keyboard shortcuts
unchanged. Replacing the project's inventory, Smith, Training and Temple menus
with the original dialogs is a separate follow-up milestone.

## Tasks

- [ ] Mouse input in `SdlWindow`: map window coordinates to the 320x200
      framebuffer (any window scale). Left button only; the right button is
      ignored on the main screen, as in the original.
- [ ] Hit areas from the pinned ScummVM `Interface::setMainButtons` and
      `addPartyButtons`: the nine action buttons, Tab area, the six movement
      buttons, the combat target rows 1-3, the six party portraits and the 3D
      viewport (left click = Space/Interact, ScummVM's wait bounds).
- [ ] Each hit area produces the same `PlayerAction` as its keyboard key, so
      clicks go through the M45 input path: buffered in exploration and
      combat under the same rules, strict elsewhere, flushed on context change.
      No new gameplay path or owner.
- [ ] Exploration: movement buttons (turn left/right, forward, back), Shoot,
      Cast and viewport interaction trigger existing actions.
- [ ] Portraits open the character sheet in the original (`CharacterInfo::show`
      for F1-F6, exploration and combat). MMModern has no sheet yet, so a
      portrait click shows the "not supported yet" notice; the sheet belongs
      with the original dialogs (M47).
- [ ] Combat: use the original combat button set (`ICONS_COMBAT`) and map its
      buttons to existing Attack, Block, Run, Cast and target selection.
- [ ] Buttons without a supported action (e.g. Rest, Bash, Dismiss, View
      Quests, Map, Info, Quick Ref, Tab, strafing) show a short visible
      "not supported yet" notice and change nothing.
- [ ] Optional, if cheap: pressed-button highlight using the original sprite
      frame (`main.icn` odd frames), as the original does.
- [ ] Inventory, services, dialogs and casting menus: keep keyboard only in
      this milestone; clicks there are ignored, not misrouted.

When this plan is unclear about a behavior, follow the original from the
pinned ScummVM reference without asking; ask only when the original itself is
ambiguous or reproducing it would leave this scope.

## Out of scope

Original item/service/spell dialogs, Rest and other new actions, mouse cursor
art beyond the system cursor, save-format or rule changes.

## Tests

- Coordinate mapping at several window scales, including edges of each hit
  area.
- Each hit area yields the expected action; clicks and keys share the queue
  and flush rules (click during enemy turn attacks once when ready).
- Unsupported buttons show the notice and leave state unchanged.
- Clicks in strict contexts do nothing.
- `ctest -L fast` while iterating; complete suite once at the end (AGENTS.md
  test-runner rule); M44 scenario digests unchanged.

## Acceptance

- Maintainer plays a mainland walk, a fight and a Vertigo visit using only the
  mouse on the main screen (keyboard for menus), and keyboard play still works.
- Complete CTest passes.
