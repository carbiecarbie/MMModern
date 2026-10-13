# Milestone 63 - Original presentation clock and movement pace

**Tier A, in two parts. Status: approved; implementation not started.**

## Goal, authority and boundary

Reproduce the original presentation timing and roughly nine movement steps per
second on [roadmap](roadmap.md) step 9, including the approved no-phantom-steps
deviation. Tier A is required: clock/input scheduling, actor opportunities,
RNG order and publication boundaries change. Independent plan and implementation
reviews are mandatory; plan approval alone does not start implementation.

Baseline: clean `main = origin/main = 3de377139251a25c9f9bf172d065b23de7e21107`.
Read this with [M62's boundary](milestone-62-plan.md), [original quirks](original-quirks.md),
[rules and evidence labels](rules/README.md) and [dependency provenance](dependencies.md).
External evidence: [UI findings](D:/MModern-re/ui-presentation-20261013/findings.md)
and [movement findings](D:/MModern-re/movement-fluidity-20261013/findings.md).
Their older MMModern baselines are historical; current code supplies ownership.
Calibrated DOS static evidence uses DAT SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`, file offsets
and the six-phase cursor calibration (`0x43f08`, table `0x3f3be`); it overrides
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Static order is not measured duration.

M62 already composes shop speaking frames and death scenes. Reuse them. Exclude
audio playback, new content, interpolation and an opt-in faster-movement mode
(maintainer decision after M63). Guard/rule-scan optimization, sprite validation
caching and ScummVM compile flags are excluded unless measurements prove a
specific change necessary to reach the pace; amend scope before that work.

## Part A - shared clock, movement admission and input queue

- Add a small shared presentation-clock value/helper, injected from the SDL
  monotonic clock into existing Flows. Its concrete purpose is to replace
  inconsistent 50/100/150/200 ms deadlines with one tested counter contract;
  it owns neither gameplay state nor a second scheduler/coordinator.
- Map BIOS counter increments to standard 18.2065 Hz (about 54.9254 ms), subject
  to the calibration gate below. Use an absolute high-resolution epoch and
  fractional/rational conversion, not rounded milliseconds accumulated per wait.
  Reset samples the current integer tick; elapsed >= N can first occur after
  roughly N-1..N periods. Preserve independent reset points on that common clock.
- Model a semantic scene draw as original draw3d: reset, compose/publish in the
  original order, finish at elapsed >= 2 (`0x10bcb`, `0x11873..0x1187b`). Outer
  one-tick waits overlap that draw rather than adding another mandatory tick.
  Advance scenery, poses, splats and projectiles once per original draw call,
  never per host refresh; respect window-11 suppression. Repainting a frame or
  moving the cursor must not advance a sequence, actor work or RNG.
- SDL waits until the next deadline or input, services release/focus/close events
  promptly and drains ready input without another fixed 16 ms sleep. Host stalls
  must not create catch-up moves, random draws or skipped semantic frames. Keep
  the original reset-after-work semantics where applicable, not a global FPS cap.
  Fades use a separate calibrated VGA-retrace timeline: publish every clamped
  palette level once per emulated retrace (step 4: 33 writes; step 2: 65), with
  host presentation/vsync as transport, not the host monitor's Hz as DOS time.
  Do not substitute BIOS ticks or the pin's 10 ms fade polling for retrace.
- Reuse `XeenEventFlow::inputContext`, `XeenEncounterFlow`, regional action
  candidates and `XeenNavigationFlow`. Admit ordinary navigation while only the
  simple actor countdown remains; do not globally loosen `journeyMutable` or
  save/modal admission. Retain Event, combat, busy/retained-work, projectile,
  stale-context and unpresented-frame fences. Combat turns retain PlayerReady.
- Follow chargeStep at the newly reached position: successful movement changes
  position, advances time, services the prior pending monster opportunity if
  any, resets the countdown to three, then performs stepTime/arrival processing
  and the next ordinary draw/input poll. Preserve condition/economy draws before
  actor draws and original actor iteration order. Turns use stepTime without
  inventing a chargeStep; blocked moves do not acquire a successful-step charge.
  Idle countdown completion still services one opportunity; never service it
  again after a rapid step consumed it. Retained ranged/combat/Event work takes
  precedence over subsequent queued navigation until its frames are acquired.
- Remove keyboard's uninterruptible ~100 ms pressed-button hold: pressed and
  restored frames have no explicit tick dwell (`0x0e1ca..0x0e2b1`). Mouse feedback
  waits for physical release (`0x0e41e..0x0e430`), while the platform stays responsive.
- Implement approved option b for movement, strafe and turns only: each distinct
  keydown/up tap contributes one action; held state supplies repeats at the
  original draw pace, independent of OS repeat bursts. Track physical key/press
  identity separately from the resolved action and from fresh taps. Keyup removes
  that press's queued repeats, including repeats in an SDL batch, but preserves
  its fresh tap and other distinct taps. Only an already dispatched held-repeat
  step may finish after release; undispatched distinct taps remain obligations.
  Keep fresh navigation taps in an ordered, dynamically sized FIFO, with held
  repeat demand separate: the existing five-entry limit must not silently lose
  taps. Preserve order and existing context/focus clears (which cancel those
  obligations), handle modifier changes, and prevent stuck holds. Other keys
  keep current queueing;
  do not impose DOS's ten-make/release-preserved FIFO on this approved exception.

## Part B - original sequences on the clock

Reuse Event/Encounter/Rest/Title Flow continuations, `XeenScenePresentation`,
`XeenServiceAnimation`, existing scene/dialog composition and shared reward rules.
Flow owns sequence progress and gameplay commits; views only compose retained
frames. The SDL loop reports actual frame acquisition and supplies time/input.

| Sequence | Required behavior and evidence (DAT file offsets) |
| --- | --- |
| Dialog waits and NPC speech | World YesNo/WhoWill outer waits: 1 tick; location: 3, interruptible. NPC: wait 3 ticks, draw, reset, then poll; do not make this inner wait interruptible. Preserve page/final-response semantics (`0x24bd7..0x24c16`, `0x251a3..0x25262`, `0x25b8f..0x25bad`). Replace word-count talking with the sampled CD-busy visual gate. Decode MMSSFF to 75-Hz frames; normal requested seconds = `(end - start + 25) / 75`, not centiseconds or whole-track duration (`0x3daec..0x3dbbf`). |
| Silent sample intervals and shops | Derive VOC duration from sample count/header rate (these resources use `1000000/68` Hz); support start, replacement, stop and sampled completion without playback. Shops poll input before their 3-tick draw wait (`0x1c520`), so input can end it early. Activate original random speaking choices: Bank 7..11, Smith 5..7 replacing base, Temple 2..4, Training 8..12 over base; Smith logical index advances while speaking. Guild/Tavern have no Clouds speaking overlay. Include greeting/action/refusal replacement and Tavern goodbye's last-screen hold; no uninterruptible greeting delay. |
| Treasure | Clear input on entry; preserve full-pack modal 6 and shared DOS capacity rules. Publish each ordinary/quest item transfer before its blank 1-tick hold, then its found line and inventory sort before the 10-tick hold; both holds are interruptible (`0x24128..0x248f2`, helper `0x264c5`). Admit quest grants into the common sequence, without re-granting them. Final acknowledgment alone credits currency and clears remaining staged treasure. A skip is not an extra reward or final acknowledgment by fiat. |
| Sparkles and portrait damage | Complete each recipient's four sparkle frames before invoking the next recipient; each frame has outer 2 ticks with window 11, otherwise 1 plus nested draw3d, preserving countdown suspension/restoration (`0x21087..0x21186`). Monster and Event/trap damage both hold 2 ticks, uninterruptibly; generic multi-recipient damage publishes each overlay/injury in order and then shares the final hold, rather than adding a separate hold per victim (`0x23f73`, `0x2d6c4`, helper `0x264ed`). |
| Shoot, hits and lethal removal | Keep simultaneous lanes and depth frames 0,1,2,0. Every travel/impact draw advances remaining arrows; consume only the impacting lane. Install pose/splat/tint, draw pre-HP, publish damage, draw post-HP with the victim still visible, then award/drop/remove and draw again if lethal (`0x1d867..0x1dec7`, `0x2bdf8..0x2c591`). Add arrow splats; decrement splat/pose counters per draw. Monster attack has two pose draws before damage. No local Escape skip or sound-length hold. |
| Rest/dream | Preserve Rest rules and recovery exactly once after the dream: fade out 4, scene1, fade in 4, 7 ticks, dreams2 completion, laff1 completion, 7 ticks, fade out 4, restore scene/cursor, fade in 4 (`0x1e605..0x1e72a`). Sample holds are sequential (1.664504 + 1.966764 seconds), not omitted or overlapped; no local input skip. Keep summary acknowledgment and interruption/death rules. |
| Title and party death | Title: 13 phases, background alone at phase 0, interruptible 4-tick idle wait and first CD fade step 4 (`0x0be9a..0x0bf8d`). Death replaces DEFEAT on every party-death route, using M62 fire/rising/final/laugh composition (`0x0b02f..0x0b39a`). Fire has no explicit 1-tick hold; retain publication/retrace cost. Clouds laugh uses 2-tick iterations bounded by the sample interval and original restart/index rules, with no added final sound tail. Return to the capture-established usable UI, never a guessed title/load target. |

## RNG, publication, saves and trace gate

Preserve each original random request's bounds, count and causal order, including
new shop/NPC speaking draws only at eligible original animation calls. Use the
existing world/Journey RNG continuation for original shared-generator calls;
do not hide them in a new cosmetic stream or extend the existing M48 cosmetic
initialization exception. Retain a chosen frame/result across redraw or retry.
Candidate work must survive yield/failure without resampling, double actor
opportunities, repeated HP/rewards/time or duplicate frame acquisition. Reentrant
callbacks, changed-and-restored owners and stale input must still fail closed.

**Retain envelope v12, schema 9, content 14.** Clock epochs, held keys and visual
continuations are transient; save only at an existing quiet boundary after all
sequence commits. Preserve already-saveable staged monster treasure (including
Run) and exact current-format restore with no event/time/RNG replay. If a newly
required durable state cannot fit the existing meaning, stop and amend this
plan before changing the format/version; no legacy readers.

Before production edits, build the four audit helpers and run
`tools/m44-digest-audit.ps1 -Step Preserve` under [the audit protocol](../tools/AGENTS.md#m44-digest-audits),
keeping baseline binaries/DLLs and evidence outside the repository. After each
relevant change, rebuild and run `-Step Compare` for the same three seeds/inputs.
Compare complete traces, every checkpoint, decoded fields and final reload/re-save
bytes. Actor work moving relative to input can legitimately change a trace or
final RNG/state; neither blanket trace identity nor automatic rebaselining is valid.
Explain every difference from its first divergence: old/new input and draw order,
clock phase, prior countdown, time/actor/animation random requests and resulting
state, tied to DOS offsets or the approved queue deviation. Presentation-only
observations must be distinguished from semantic changes. Unexplained differences
block acceptance; independent review precedes maintainer approval of replacements.
Exit 2 is a review gate, not a pass; never replace hashes merely to make tests pass.

## Blocking captures and acceptance

Only these unresolved DOSBox captures block the named behavior; record timestamps
and machine/cycles/video/audio settings. Established static rules can proceed
after implementation authorization; do not guess across a blocked boundary.

| Capture | Gate |
| --- | --- |
| One complete title idle cycle/initial fade and one full dream with sound; include Escape during a voice | Part A final BIOS/retrace host calibration; Part B exact title/dream holds and deferred input. Use standard 18.2065 Hz unless evidence establishes otherwise; establish retrace cadence and driver timing before finalizing conversion. |
| One Clouds party death with sound, uninterrupted through the next usable UI | Part B death fire publication pace, laugh stop and return target. |
| One known short NPC CD sentence through silence, with its Event cue and carrier identified | Part B CD-speaking gate: track/start mapping, actual start latency and busy-to-idle boundary. Formula is already established; do not replace it with a guessed carrier duration. |

Acceptance for implementation (none of these runs belongs to this planning task):
- Rules tests: phase-dependent tick resets, nested/interruptible/uninterruptible
  waits, retrace levels, late wakes, no catch-up and one semantic advance per draw.
- Input tests: more than five distinct taps queued behind an unpresented-frame
  fence, holds/release with queued SDL repeats, strafe/
  modifier changes, turns, focus/context clears, capacity, mouse release and all
  retained fences; actor opportunity/time/RNG order for rapid steps and idle drain.
- Sequence tests for every row above, silence/replacement boundaries, full packs,
  quest grants, survivor/lethal hits and sequential effects; failure/retry and
  stale-frame injection prove exactly-once publication. Generic save round-trip.
- A few original-data runs join navigation, Events, combat, treasure and services;
  cover all death entry routes and return UI. Reuse existing tests, without a
  separate process-continuation witness per feature. Complete the M44 audit gate.
- Measure optimized `build-rel` (`-O2 -g`, assertions enabled) like the fluidity
  investigation: real SDL, the disclosed cleared-town `(15,28,South)` gate route,
  timestamped enqueue/delivery/present, isolated steps, sustained holds, release
  tails and separate held turns. Use three runs plus capture-free confirmation;
  report successful-step mean/SD/range/count, enqueue-to-present latency and tails,
  excluding blocked attempts, startup and door transition. Target near 9 steps/s
  (about 109.85 ms under standard BIOS), with no excess release-repeat actions.
  Compare DOSBox's 7.85/8.77 moving views/s honestly; do not call graphics FPS or
  SDL present-return timing physical input-to-photon latency.
- Independent plan review before implementation; independent implementation review
  of both parts, especially RNG, ownership and input fences. Build and run the
  complete `build-rel` suite after each final production task via the runner in
  [tools/AGENTS.md](../tools/AGENTS.md); at closure run Debug `build-m44` fast tests
  and all three M44 scenarios. No completion with failing tests.
- Maintainer play-test against DOSBox: original pace and all sequence rows,
  specifically Smith/Training speaking gates, skippable treasure and lethal order.
  Update affected rules/quirk entries with evidence when implemented; update
  project status/history and record acceptance only after maintainer acceptance.
