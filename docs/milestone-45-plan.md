# Milestone 45 - Reliable input (Tier A)

Status: **approved by the maintainer on 2026-10-03; implementation not yet authorized.** Baseline: `main` at `83ef18a`.
Tier A: independent review of this plan and of the implementation is required.

## Goal

Keys pressed during redraws, animations, enemy turns or result screens are not
lost. They are buffered in order and applied one per valid frame, as the original
does, while duplicate-action and stale-screen protection stays. Also fix the
flaky `xeen_save_sdl` test and settle the Run "Stale encounter frame" defect.

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

**Run "Stale encounter frame".** Reproduced on `83ef18a`, and diagnosed in a
throwaway worktree (discarded). It fails in `disengagementFinishPresentation`
only for `occupied=true, attrition=false`. The message reads "Gameplay startup
failed" only because the test's `services.show` runs inside `playGameplay`'s try
block. Mechanism: the test injects a frame-copy fault
(`beforeEncounterFrameCopy`, test lines 268-273) into the first composition of
the destination frame; the **replacement combat** then fails with
`XeenCombatFailure::Integrity` (operation `End`, phase `Failed`) during the
guarded retry (`XeenCombat::guardCallback`, `XeenCombat.cpp:459-479`: the
post-throw `check()` fails and the combat is failed), so `renderEncounter`'s
retry sees a non-current ticket and throws at `XeenEventFlow.cpp:350`. This is
not an input defect. The normal first-attempt render of an occupied destination
passes (other controls pass), so it is reachable in normal play only if the
frame composition/copy itself throws. Which owner's snapshot the guard sees
changed is still to be pinned (step 1).

## Design decisions

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

### Flake and Run defect

- `xeen_save_sdl`: wait for a presented, input-ready frame before pushing each
  key instead of fixed sleeps; repeat 20 times under parallel load.
- Run defect, step 1 pins which owner snapshot makes the guarded retry fail, then:
  - **if reachable only through the injected frame-copy fault** (the retry path of
    a replacement incarnation after an artificial throw): fix the test/harness,
    stating that the production first-attempt path is already covered;
  - **if reachable in normal play** (a real composition/copy failure, or any path
    without the fault): fix the guarded retry for replacement incarnations in M45
    scope, generically, with no special case for `(10,12)`.

## Tests

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
- **Stop point:** after step 1, report measured drop counts and the Run owner. If
  the fix needs ownership changes beyond one context id and one callback, or the
  Run fix touches combat/save semantics, stop and re-plan with the maintainer.

## Work order

1. Temporary counters/probe in a separate worktree and build directory (discarded
   afterwards): measure drop reasons under human-paced input; pin the Run owner.
   **Stop and report.**
2. Queue, drain and context id in `SdlWindow`/handler; remove `readyAt` retirement.
3. Tests; adapt existing input tests.
4. Fix `xeen_save_sdl` and the Run defect or harness.
5. Full CTest, independent review, maintainer play-test.

## Acceptance

- Maintainer play-test: no lost keys in movement (including holding W), Space and
  combat keys, including presses during animations and enemy turns; nothing
  carries across combat end, dialogs or map changes.
- `xeen_save_sdl` stable over repeated parallel runs; `mmodern_consequence_original`
  passes (or its harness is corrected with justification); complete CTest passes.
