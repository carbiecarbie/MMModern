# Milestone 45 plan - Reliable input and play stability

**Tier A. Status: completed and accepted.**

## Goal

Stop losing key presses in physical play, and stop integrity guards from ending
play when resources load on demand. No mouse/UI work (M46), no gameplay, RNG or
save-format change.

## Findings

- **Lost keys.** `SdlWindow` discarded any KEYDOWN stamped before the latest
  frame became ready (`readyAt`) and restamped every queued key as stale on each
  non-cosmetic frame (`retireQueuedKeys`). Keys pressed during movement redraws,
  actor work, combat animation or enemy turns vanished silently. Earlier fixes
  only narrowed the window for cosmetic redraws, so the problem kept returning.
  A synthetic human-paced run lost ~150 of 321 presses; a maintainer session
  lost 9 of 66 (mostly W during the previous step's redraw). Protective filters
  (key repeat, held keys, Escape/F9 on stale frames, Flow ticket checks)
  dropped nothing legitimate.
- **Original behavior.** The pinned ScummVM buffers up to 5 key presses
  (`MAX_PENDING_EVENTS`), queues repeated KEYDOWNs, consumes one per loop pass
  and flushes the queue on context changes (map load, after script Events,
  combat end, treasure/result windows, dialogs).
- **Guard false positives.** Integrity guards treated any map or object file
  newly loaded into a cache as tampering. Combat near the map-23 north edge
  loads neighbor map 22 on demand, so play ended with "Stale encounter frame"
  or "preparation owner preimage changed"; the maintainer hit both in
  physical play. Tests missed it because they ran with warm caches.
- **`xeen_save_sdl` flake.** It sent keys after fixed sleeps; a key landing
  before frame readiness was dropped under load.

## Decisions

- **Guard admission.** Maps and object files loaded from the original archives
  are admitted into every live guard snapshot through one shared path
  (`XeenRestoreGuard::admitLoadedResource`), validated with the guards'
  existing identity and content checks. Cache growth never fails integrity;
  a changed or reverted admitted resource, and any owner-state change, still do.
- **Input queue** (`SdlWindow`; Flow keeps authority through one
  `inputContext` callback returning `{contextId, acceptsQueuedInput,
  readyForAction}`):
  - FIFO of up to 5 actions, queued only in exploration and combat: movement
    and turns, Space, B, F, R, Wait, C, F1-F6 and combat targets 1-3.
  - Drained one action per ready frame, only when Flow is ready for a player
    action (combat `PlayerReady`, no animation, automatic or regional work,
    handoff, dispatch or save). Not-ready frames keep the entry; a refusal
    after readiness (e.g. a wall) consumes it.
  - Auto-repeat only for movement/turns, with at most one repeat pending, so
    releasing a held key adds at most one step. Physical presses queue up to 5.
  - Flushed on context change (map, panel, dialog, combat incarnation), on
    Escape and on focus loss. Escape and F9 are never queued.
  - Services, dialogs, result/reward panels, inventory and casting keep the
    previous strict rule until M46 replaces those menus.
- **Flake fix.** `xeen_save_sdl` waits for a presented, input-ready frame.

## Results

- Maintainer play-test: walking, holding W, Space during enemy turns and
  services felt fluid and faithful to the original; no lost keys and no
  ghost actions across combat end. Cold-cache play near the north edge, Run
  and Vertigo entry/exit showed no integrity failure.
- Tests: queue policy, readiness, flushing and W-during-redraw tests
  (`sdl_input`, `xeen_input_scheduling`); adapted training/purchase/native
  input tests keep duplicate/stale protection; cold-cache guard tests
  (`xeen_resource_admission`, `xeen_resource_cold_original`);
  `mmodern_consequence_original` passes without prewarming; `xeen_save_sdl`
  passed 20/20 under `ctest -j4`.
- Full CTest 138/138 (sequential, delegated to a lightweight runner); M44
  scenario digests unchanged. Independent review of the plan and of both
  implementation steps found no blocking issue.
