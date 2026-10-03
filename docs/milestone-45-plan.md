# Milestone 45 - Reliable input and play stability (Tier A)

Status: **approved by the maintainer on 2026-10-03, amended after step 1 the same day; step 1 done, later steps not yet authorized.** Baseline: `main` at `83ef18a`.
Tier A: independent review of this plan and of the implementation is required.

## Goal

First, stop integrity guards from ending play when a map or other immutable
resource is loaded into a cache on demand (the step-1 finding behind "Stale
encounter frame" and "preparation owner preimage changed"). Then, keys pressed
during redraws, animations and enemy turns in exploration and combat are not
lost: they are buffered in order and applied one per ready frame, as the
original does, while duplicate-action and stale-screen protection stays.
Result/reward panels, services, dialogs, inventory and casting keep today's
strict rule (M46 replaces those menus). Also fix the flaky `xeen_save_sdl` test.

Non-goals: mouse/UI work (M46), new gameplay, save-format change, a second input
system beside SdlWindow/Flow.

## Findings

Path: SDL event -> `dispatchEvent` (`SdlWindow.cpp:225`) -> `playerAction` (`:40`)
-> `withPresentedInput` -> `dispatch` (`XeenGameplay.cpp:236`) ->
`XeenEventFlow::handle` (`XeenEventFlow.cpp:982`). Frames are acknowledged by
`framePresented`/`completeInputHandoff` (`XeenEventFlow.cpp:127,155`).

| Where | Condition | Kind |
| --- | --- | --- |
| `SdlWindow.cpp:237` | `repeat != 0` | design (changes in M45, see below) |
| `:241-242`, `:245-248` | key already marked held | protective |
| `:242,248,250` | `timestamp < readyAt` | **lost input** |
| `:206-219`, `:357` | `retireQueuedKeys` restamps every queued KEYDOWN stale on each non-cosmetic frame/semantic change | **lost input** |
| `:252-254` | Escape on a non-current batch frame | protective |
| `:260-261`, `XeenEventFlow.cpp:988-991,1000-1001` | frame/generation/combat ticket not current, handoff pending, dispatching, saving; `handle()` returns `frameCopy()` silently | protective, but silent |
| `XeenGameplay.cpp:241-255` | F9 during service/handoff/stale frame | protective |
| `XeenEventFlow.cpp:1002-1004,1013` | action meaningless in this mode | legitimate refusal |

**Root cause of lost keys.** `readyAt` is reset to "now" whenever a non-cosmetic
frame is acquired (`:357`), and every key already queued is restamped older. A
key pressed while the game composes, uploads or animates the previous step is
discarded, though it was meant for the next ready state. Humans do this during
movement redraws, each combat enemy-turn step (`++_inputGeneration`,
`XeenEventFlow.cpp:370`), ATT/projectile animation, result panels and service
frames; the 16 ms loop plus `idle()` work lengthens the window. The M33/M41
rule ("a later action needs a newly acquired frame and a key edge") is right for
authority but implemented "not ready" as "discard". Earlier fixes shrank the
window for cosmetic redraws only, so the problem returned.

**Held keys.** KEYUP is never gated and the cosmetic path replays queued KEYUPs
in order (`:343-345`), so ordinary gating cannot strand a key. The only gap is
focus loss (SDL2 synthesizes KEYUP; not verified on every platform): the plan
clears held flags and the queue on focus loss.

**Original (pinned ScummVM `engines/mm/xeen/events.cpp`).** Keys are collected
inside every engine wait/animation loop (`pollEvents`), so keys pressed during
animation are queued, not ignored. `Common::Queue`, `MAX_PENDING_EVENTS 5`
(`events.h:36`); a press is silently ignored when 5 are queued (`:129-137`);
repeated KEYDOWNs are queued like any other; one event is consumed per loop
pass. `clearEvents` flushes the queue at map load (`map.cpp:683`), after a script
Event (`scripts.cpp:252,271,545`), at combat end (`interface.cpp:1885`), after
treasure/result windows (`party.cpp:722,789,797`), around `waitForPress`, and
after service/location dialogs (`locations.cpp`). Keys also interrupt `wait()`.
So the original buffers within a context and flushes across context changes.
DOSBox adds its own buffer and is not modeled.

**`xeen_save_sdl` flake.** Same mechanism: it pushes keys after fixed sleeps
(`XeenSaveSdlTests.cpp:19-20`); `SDL_PushEvent` stamps `SDL_GetTicks()`, so a key
landing before the next `readyAt` is dropped under load. Not a product bug.

**Step-1 results (measured).**
- Lost keys: of 321 human-paced presses (180 s walking, two fights, Shoot), about
  150 were dropped: 86 by the `readyAt` discard in exploration, 34 by
  encounter-busy refusals (25 in combat animation/enemy turns), 20 by a
  non-current batch frame, 3 on result panels. Protective filters (repeat,
  held, Escape, F9, Flow ticket/handoff/save gates) dropped nothing. The policy
  below addresses the dominant causes.
- **Guard false positives on lazy cache growth (product defect).** With a cold
  cache, combat near the north edge loads neighbor map 22 during composition.
  `XeenCombat::Impl::lifetime`'s retained resource snapshot lacks it; the
  temporary guard admits it, but `current()` compares against the older
  lifetime snapshot and fails `Integrity`, ending the combat and the session
  ("Stale encounter frame"). The failure occurs before the test's injected
  fault and also with it disabled; prewarming map 22 makes all four
  occupied/attrition cases pass; a normal Journey run reproduced it.
  `XeenRestoreGuard::cachesCurrent()` (`XeenRestoreGuard.h:86-98`) similarly
  rejects any map/object cache entry absent from its snapshot, throwing
  "preparation owner preimage changed" (`:108`). The maintainer hit both
  messages in physical play (terminal captures, 2026-10-03), so this is the
  same class of defect, not a harness issue.

## Design decisions

### Guards and lazily loaded immutable resources

- A map, object file or other immutable resource that enters a cache during
  play, loaded from the original archives, is **admitted** into every retained
  guard snapshot that checks caches (`XeenRestoreGuard`, the combat lifetime
  snapshot and any other owner-preimage guard), using the same identity and
  content checks the guards already apply to admitted entries. Admission is
  monotonic: an admitted entry whose content later differs still fails
  integrity, and gameplay-state changes are still detected exactly as today.
- Cache growth alone never fails integrity; cache *mutation* or *reversion* of
  an admitted entry still does. Prefer one shared admission helper over
  per-guard special cases; no special case for map 22 or `(10,12)`.
- Stop and re-plan if admission requires changing what the guards protect
  (party/world/combat owners, RNG, save state) rather than how they treat
  immutable cache entries.

### Policy: bounded pending-action queue (owner: `showLoop`; authority: Flow)

1. Replace discard with a **FIFO of `PlayerAction`s** in `showLoop`. Bound **5**
   (as `MAX_PENDING_EVENTS`); presses while full are ignored.
2. A KEYDOWN is enqueued if it is a fresh edge. Held tracking stays but only
   prevents duplicates; it no longer decides readiness. In queueable contexts
   the `readyAt` timestamp test and `retireQueuedKeys` no longer discard keys;
   strict contexts (item 4) keep them unchanged.
3. **Auto-repeat:** SDL key repeat is accepted for movement and turn keys only
   (WASD/arrows), so holding W walks. It is accepted only if **no auto-repeat
   entry is already pending**, so holding a key never builds a backlog of extra
   steps after release (at most one repeat entry waits at any time). Physical
   presses (repeat = 0) of any queueable key queue normally, up to 5. Space, B, F,
   R, `.` and every other key ignore repeat entirely.
4. **Queueable contexts only.** Queueing applies in the **exploration** (Quiet
   Journey or map) and **combat** contexts. In services (Smith, Training, Temple),
   dialogs, result/reward panels, inventory, casting UI and any other panel, every
   key keeps today's strict rule: fresh frame, fresh key edge, no queue (Enter/Y/N,
   paid or irreversible confirmations, inventory keys - M46 replaces those menus).
   The context callback reports whether the current context accepts queued input;
   outside such a context a press goes straight to today's path and nothing is
   enqueued.
5. **Drain gating.** An entry is popped only when Flow reports it is **ready to
   accept a player action in the current context**: the context accepts queued
   input, the presented frame is the actionable frame, and Flow is not
   dispatching, saving, handoff-pending, running automatic work, animating, or in
   an enemy turn. Otherwise the entry **stays queued** across any number of
   intermediate frames. At most **one action per ready frame**, so each action sees
   the screen produced by the previous one. A refusal by `handle()` **after
   readiness** (walking into a wall, an action meaningless in this mode) consumes
   the entry and is not retried. Hence a Space pressed during enemy turns is
   delivered when the player is next ready in the same combat.
6. **Who may queue (when the context allows):** movement/turn, Space
   (Interaction/Attack), B, F, R, `.`, C, F1-F6, combat target digits. **Never
   queued:** Escape and quit (immediate; Escape also clears the queue) and F9
   (immediate; allowed only at a current idle boundary, otherwise refused with the
   existing status, never replayed).
7. **Flush on context change** (the original's `clearEvents` points). Flow exposes
   one *context id* plus the "accepts queued input" flag. The id changes when
   combat starts or ends (new incarnation), a map transition occurs, a dialog,
   service, result/reward panel, inventory or casting UI opens or closes, or a
   handoff/save fails. Each entry records the id at enqueue; on drain a mismatch
   **discards the whole queue**. Within a context (several walking steps, enemy
   turns then player ready in the same combat) entries survive.
   `_inputGeneration` is not the context id: it changes every step. Combat ends,
   panel/dialog opens, map transition -> flush.
8. **Stale/duplicate protection:** the queue holds actions, never frame
   identities; every drained action passes the unchanged `handle()` checks, so a
   queued key cannot authorize an old frame, and one action per ready frame bounds
   bursts.
9. Focus loss clears held flags and the queue.

Ownership: `SdlWindow` keeps SDL events and the queue; Flow remains the only
authority on readiness and whether an action runs; `FrameUpdateHandler` gains
one callback returning `{contextId, acceptsQueuedInput, readyForAction}`. No
change to save format, RNG or time.

### Flake

- `xeen_save_sdl`: wait for a presented, input-ready frame before pushing each
  key instead of fixed sleeps; repeat 20 times under parallel load.

## Tests

- Cold-cache stability (original data, no prewarming): walk and fight near the
  map-23 north edge so map 22 loads during combat composition; enter and leave
  Vertigo; Run to an occupied destination. No integrity failure, and the M44
  scenario digests still match. Unit tests: a guard admits a newly loaded
  immutable entry, still fails when an admitted entry changes or reverts, and
  still detects owner-state changes. `mmodern_consequence_original` passes
  without prewarming.

- Queue policy (headless, `SdlInputTests`): bound 5 and overflow ignored; FIFO;
  one action per ready frame; Escape/F9 never queued and Escape clears; repeat
  accepted for movement only, ignored for Space/B/F/R; only one auto-repeat entry
  pending (hold W for N frames, release: at most one extra step); physical
  presses still queue to 5; focus loss clears flags and queue; held/release
  sequences.
- Drain gating: a queued Space survives several enemy-turn frames (not ready) and
  attacks exactly once when the player is ready; an entry is not consumed by
  frames during animation, automatic work or handoff; a refusal after readiness
  (wall) consumes it.
- Context acceptance: in a service, dialog, result/reward panel, inventory and
  casting UI nothing is queued and the strict rule applies unchanged.
- Deterministic scenarios with a fake clock/handler: keys pressed during a
  movement redraw, an enemy-turn step, ATT/projectile animation and a
  same-generation frame change; each applied once, in order, after readiness.
- Context flush: queued Space then combat ends, a dialog opens, a map transition:
  nothing executes in the new context.
- Stale protection: a queued key never reaches `handle()` with an old frame; no
  duplicate attack or purchase; strict confirmations unaffected.
- Keep and adapt `XeenInputSchedulingTests`, `SdlInputTests`,
  `XeenTrainingInputTests`, `XeenPurchaseInputTests` and the M34 native controls,
  changing only assertions that expected "dropped" where the rule is now
  "queued". Full CTest at the end.

## Risks and stop point

- Replayed keys can feel like "ghost" moves; the original behaves the same,
  bounded by 5. Held movement fills the queue; drain rate is one per frame.
- A missed flush point could apply a key in the wrong context; mitigated by
  enumerating the original flush sites, tests per class, and `handle()` revalidation.
- Existing tests that encode drop semantics need rewording.
- Guard admission could weaken integrity checks; mitigated by admitting only
  entries loaded from the original archives with existing identity/content
  checks, keeping mutation/reversion detection, and independent review.
- **Stop points:** step 1 is done. Stop and re-plan if the guard fix needs to
  change what guards protect, or the input fix needs ownership changes beyond
  one context callback.

## Work order

1. Done: measurement and diagnosis (results above).
2. Guard admission of lazily loaded immutable resources, with cold-cache tests.
   Commit separately after its own review (Tier A).
3. Queue, drain and context callback in `SdlWindow`/handler; readyAt/restamp no
   longer discard keys in queueable contexts.
4. Input tests; adapt existing input tests; fix `xeen_save_sdl`.
5. Full CTest, independent review, maintainer play-test.

## Acceptance

- Maintainer play-test: no lost keys in movement (including holding W), Space and
  combat keys, including presses during animations and enemy turns; nothing
  carries across combat end, dialogs or map changes.
- No integrity failure from on-demand map/resource loading in cold-cache play
  (near the north edge, Run to an occupied destination, Vertigo entry/exit);
  `mmodern_consequence_original` passes without prewarming.
- `xeen_save_sdl` stable over repeated parallel runs; complete CTest passes.
