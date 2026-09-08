# MMModern rolling roadmap: M19-M23

**Status: approved, 2026-09-08.**

This is the approved rolling direction from the completed M18 baseline.
[project-status.md](project-status.md) is authoritative for current implemented
state and validation; [dependencies.md](dependencies.md) defines the pinned
ScummVM revision and dependency configuration. Detailed scope belongs in the
active milestone's `docs/milestone-N-plan.md`.

## Roadmap principles

Plan approximately five milestones ahead, with decreasing confidence. Only the
milestone about to begin receives a detailed plan. Approval of this direction
does not authorize implementation or make distant entries fixed specifications.

Favor original Clouds gameplay that connects into a useful loop: meet a quest
giver, collect an item, retain progress across sessions, return it, and receive
the actual reward. Preserve existing state ownership, stable world identities,
disposable caches and resumable presentation. Extend those boundaries as needed
without introducing parallel systems or skipping unsupported original behavior.

Local interaction checkpoints do not certify normal travel or a playable region.
Keep route, combat and other unsupported boundaries explicit.

## Starting point and ordering rationale

M18 supplies the starting point: local Phirna and Bone Whistle collection,
session-persistent ownership/removal, static outdoor objects, and resumable
interaction with temporary character selection. Consult project status for the
full capability frontier and validation record.

The roadmap review identified **Myra's NPC dialogue as the first relevant
unsupported boundary**, followed by quest-state handling needed to finish her
request. Existing event and presentation infrastructure supplies most of the
prerequisites. Outdoor animation does not block that script path.

Completing Myra's return exchange is materially broader: it requires root
consumption, quest-state changes, and delivery of five original miscellaneous
rewards into character inventory. Current quest counters and equipment modifier
fields cannot represent that reward flow. This supports separating the request
from the exchange, with persistence between them. Detailed opcode, portrait,
resource and inventory semantics belong in the respective milestone plans.

## Approved sequence

| Milestone | Direction | Confidence | Main result |
|---|---|---|---|
| M19 | NPC dialogue and Myra's quest request | High confidence | Meet Myra through the original script and record the request |
| M20 | Save and resume supported Clouds progress | Strong provisional | Keep collected items, quest request and world changes across restarts |
| M21 | Myra's return exchange and bounded item rewards | Provisional | Turn a collected root into the original character-held rewards |
| M22 | Bounded outdoor object animation | Directional | Show ordinary animated objects in explored Clouds scenes |
| M23 | Indoor objects and a first visible indoor interaction | Horizon candidate | Extend interactions into correctly occluded indoor scenes |

### M19 — NPC dialogue and Myra's quest request

- **Goal and placement:** complete Myra's original request path, including its
  quest-state change. This connects working collection to a quest giver without
  taking on the substantially larger reward system immediately.
- **Dependencies:** existing event execution and resumable presentation, portrait
  resources, and explicitly owned quest state with defined loading and mutation
  behavior. Reuse the current interaction path; quest flags remain distinct from
  game flags. Resolve dialogue and any bounded portrait timing in the M19 plan.
- **Checkpoints:** Myra on map 23 `(9,11)` West, starting the original request
  without a root. Phirna and Bone Whistle remain regression controls. The
  root-owned return branch still stops at unsupported consumption; it does not
  constitute a completed exchange.
- **Non-goals:** root consumption, inventory/rewards, generic NPC services,
  outdoor animation, save/load, combat and certification of travel to Myra.
- **Handoff:** reusable bounded NPC presentation and a recorded quest request
  for M20 to persist and M21 to complete.

### M20 — Save and resume supported Clouds progress

- **Goal and placement:** preserve the supported adventure across process
  restarts. Collections and a quest request provide useful progress to retain;
  existing session ownership and stable identities support this step.
- **Dependencies:** authoritative world, party and quest state from preceding
  milestones, plus a versioned MMModern save format and coherent restoration.
  Cover state actually supported at implementation time, rebuilding disposable
  caches and presentation through existing owners.
- **Checkpoints:** save after Phirna or Bone Whistle collection, restart and
  preserve ownership, removal and repeat prevention; also resume Myra's request.
- **Non-goals:** original Xeen/ScummVM save compatibility, suspended scripts or
  dialogs, commercial resource serialization, future inventory design and
  Darkside. Prefer saving at a stable gameplay boundary, defined in the plan.
- **Handoff:** durable progress and a save-version policy that M21 can extend
  when character-held rewards arrive.

### M21 — Myra's return exchange and bounded item rewards

- **Goal and placement:** complete the collection-to-return loop with actual
  consumption, quest-state changes and the original rewards. Its broader
  inventory and reward dependencies make this entry provisional.
- **Dependencies:** M19 dialogue/quest state, existing counted possession, bounded
  character item storage and reward distribution, and M20 save extensions.
  Item identity, initial inventory, pack capacity, recipients and reward
  presentation must form a coherent slice rather than a counter-only shortcut.
- **Checkpoints:** collect Phirna on map 23 `(8,2)`, return to Myra at `(9,11)`
  West through the original script, inspect received items and resume the result
  from disk. Preserve original no-root and revisit behavior. Validate the
  connecting route separately before claiming normal travel.
- **Non-goals:** full equipment use, shops/trading, all reward modes, generic
  random loot, combat and a complete quest journal. Delivered, inspectable
  rewards are part of the goal; a success message alone is insufficient.
- **Handoff:** a persistent local quest exchange and character item ownership
  useful to later encounters. Split this milestone if reward work requires a
  substantial inventory prerequisite rather than silently broadening it.

### M22 — Bounded outdoor object animation

- **Goal and placement:** show ordinary original animated objects currently
  excluded by static rendering. Dialogue and rewards take gameplay priority;
  animation then improves explored places and supports broader scene coverage.
- **Dependencies:** existing object metadata, effective identities, sprite
  drawing, terrain ordering and presentation rebasing, extended with bounded
  visual timing. Refresh must preserve interaction state and removed objects.
  This work does not depend on M21's item model.
- **Checkpoints:** choose an ordinary animated object in the explored map-1/map-23
  area during immediate planning. No exact animated record is certified by this
  roadmap. Phirna, Air / Corner and resource 117 remain static controls; the new
  checkpoint should demonstrate stationary motion and presentation layering.
- **Non-goals:** monsters/combat, a general game clock, spell effects,
  quest-dependent appearance changes and scripted chest/door cycles.
- **Handoff:** bounded scene animation with an explicit distinction between
  transient visual phase and persistent gameplay state.

### M23 — Indoor objects and a first visible indoor interaction

- **Goal and placement:** interact with an original indoor object rendered with
  correct wall occlusion, extending the established loop into another scene
  type. This is a horizon candidate, not a complete dungeon milestone.
- **Dependencies:** existing indoor geometry, effective object identities,
  sprite infrastructure and the interaction capabilities needed by the chosen
  encounter. Indoor projection and occlusion need their own rules. M22 helps
  animated candidates but is not a prerequisite for static ones.
- **Checkpoints:** map 33 `(4,8)` North is an existing geometry control, and
  Castle Basenji's indoor destination is a transfer control. Neither certifies
  an interactive object. Choose and verify an original indoor encounter near
  the explored route before promoting this entry to a detailed plan.
- **Non-goals:** complete indoor content, lighting gameplay, doors/locks/traps,
  monsters/combat and Darkside. Wall items have a separate rendering path;
  include them only if the encounter requires reconsidering or splitting scope.
- **Handoff:** a validated indoor interaction and object-occlusion foundation
  for subsequent dungeon encounters.

## Dependency narrative

The gameplay chain is **existing collection -> M19 quest request -> M21
return/reward**. M20 sits between request and exchange to make progress durable
before inventory expands. Saving is not technically required to execute the
exchange in one session; its position prioritizes retaining useful progress.
After M20, each new persistent state category must extend save/load deliberately.

The visual branch is **existing static composition -> M22 animation -> M23
broader scene coverage**. Animation does not unlock NPC scripts or persistence.
Any portrait timing needed by M19 belongs to its bounded presentation work and
does not require outdoor animation first. Reuse that work later where suitable.

Across both branches, retain authoritative state ownership, existing mutation
policies and resumable presentation. New state needs an explicit policy, and
restoring a session must replace transient execution/presentation coherently.

## Alternative orderings considered

| Alternative | Reason for the approved ordering |
|---|---|
| Prior M19 animation / M20 save-load | Partly retained: saving remains M20; animation moves to M22 because NPC dialogue is the actual first blocker for Myra. |
| Save/load first | Technically viable, but dialogue connects collection to a quest giver and settles quest-state ownership before the first save format. Revisit if restart loss becomes the dominant priority. |
| Full Myra exchange at M19 | Dialogue, quest state, consumption, inventory and reward distribution introduce too many boundaries for the high-confidence next slice. |
| Quest consumption alone next | Myra stops at NPC first; consumption without rewards does not complete the exchange. |
| Finish Myra before saving | Could include inventory in the first save format, but delays persistence for the larger reward investigation. A versioned format can evolve instead. |
| Indoor objects or wall items first | No demonstrated prerequisite for the known outdoor quest loop; revisit if a selected encounter needs them. |
| Swimming/Walk on Water or combat first | May be essential to normal travel, but local checkpoints do not establish the first route blocker. Promote a bounded prerequisite when concrete route evidence identifies it. |
| Darkside early | Side-aware identity alone does not supply gameplay support. Concentrating on Clouds advances a coherent local loop first. |

The intended outcome is a persistent local Clouds quest loop and broader visible
interactions, without claiming a normally playable region or completed combat.

## Replanning triggers

Revise the sequence when concrete evidence changes its premises:

- NPC presentation or quest-state behavior requires a substantial unrepresented
  prerequisite, or original data contradicts the assumed boundary.
- Myra's inventory, capacity or reward-delivery requirements justify splitting
  M21 into separate coherent milestones.
- Normal travel between checkpoints exposes an unsupported movement, automatic
  event or monster boundary that prevents the intended loop. Record the exact
  blocker and promote its bounded prerequisite.
- Save restoration exposes missing authoritative state or a material ownership
  problem beyond the expected slice.
- A visual checkpoint requires scripted animation, alternate appearances or
  wall items outside the proposed object subset.
- Work naturally collapses two entries into one, or grows enough to warrant a
  split; new user priorities may also change the preferred gameplay direction.

Record the evidence, affected dependencies and revised confidence here. Normal
milestone completion alone does not trigger full roadmap rediscovery.

## Review cadence and next action

At each completion, make a short handoff: use the next roadmap entry as the
default successor, check it against the new state and note concrete changes.
Reassess the broader rolling horizon after approximately **three completed
milestones (after M21)**, or earlier following a major architectural discovery.

The [Milestone 20 plan](milestone-20-plan.md) is complete, with final independent
**APPROVE MILESTONE 20C / MILESTONE 20** and all A01-A16 criteria closed.
M20 is committed and is the current stable completed milestone.
M21 remains the default next planning entry; it has not begun.
Gabriel's physical-window results
remain user-supplied manual evidence, separate from reviewer-executed validation.
The approved sequence and review cadence remain unchanged; no later milestone
is authorized by this closure or by roadmap approval.
