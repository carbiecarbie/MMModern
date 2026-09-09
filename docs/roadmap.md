# MMModern - Roadmap

The current stable baseline is **completed Milestone 21**. See
[project status](project-status.md) for present capabilities and
[project history](project-history.md) for completed milestones.

The future direction below retains the roadmap approved on 2026-09-08. M22 is
the next planning candidate; M23 remains a horizon candidate. Roadmap approval
is not implementation authorization. The scheduled broader review after M21
remains due before promoting this direction into detailed next-milestone work.

## Roadmap principles

Keep a rolling horizon with decreasing confidence; create a detailed milestone
plan only when that milestone is about to begin. Do not fill the horizon with
unsupported commitments.

Favor original Clouds gameplay and useful visible interactions. Preserve existing
state ownership, stable world identities, disposable caches, mutation policies
and resumable presentation. Extend existing infrastructure without parallel
systems or silently skipping unsupported original behavior.

Local checkpoints do not certify normal travel or a playable region. Keep route,
combat and other unsupported boundaries explicit. Dependency configuration is
owned by [dependencies.md](dependencies.md), not by the roadmap.

## Approved future direction

| Milestone | Direction | Confidence | Intended result |
| --- | --- | --- | --- |
| M22 | Bounded outdoor object animation | Directional; next planning candidate | Ordinary animated objects in explored Clouds scenes |
| M23 | Indoor objects and a first visible indoor interaction | Horizon candidate | Correctly occluded objects and a bounded original indoor encounter |

### M22 - Bounded outdoor object animation

- **Goal:** show ordinary original animated objects currently excluded by static
  rendering, improving explored places and broader scene coverage.
- **Dependencies:** existing object metadata, effective identities, sprite
  drawing, terrain ordering and presentation rebasing, extended with bounded
  visual timing. Refresh must preserve interaction state and removed objects.
  This direction does not depend on the item model.
- **Checkpoint selection:** immediate planning should verify an ordinary animated
  object in the explored map-1/map-23 area. No exact animated record is certified
  by this roadmap. Phirna, Air / Corner and resource 117 remain static controls;
  the new checkpoint should demonstrate stationary motion and presentation layering.
- **Non-goals:** monsters/combat, a general game clock, spell effects,
  quest-dependent appearance changes and scripted chest/door cycles.
- **Intended handoff:** bounded scene animation with an explicit distinction
  between transient visual phase and persistent gameplay state.

### M23 - Indoor objects and a first visible indoor interaction

- **Goal:** interact with an original indoor object rendered with correct wall
  occlusion. This horizon candidate is not a complete dungeon milestone.
- **Dependencies:** indoor geometry, effective object identities, sprite
  infrastructure and the interaction capabilities needed by the selected
  encounter. Indoor projection/occlusion need their own rules. M22 helps
  animated candidates but is not a prerequisite for static ones.
- **Checkpoint selection:** map 33 `(4,8)` North is a geometry control and Castle
  Basenji's indoor destination a transfer control; neither certifies an interactive
  object. Verify an original indoor encounter near the explored route before
  promoting the entry to a detailed plan.
- **Non-goals:** complete indoor content, lighting gameplay, doors/locks/traps,
  monsters/combat and Darkside. Wall items use a separate rendering path; include
  them only if the encounter requires reconsidering or splitting scope.
- **Intended handoff:** a validated indoor interaction and object-occlusion
  foundation for subsequent dungeon encounters.

## Dependencies and boundaries across future work

The visual direction extends existing static composition through bounded outdoor
animation and then indoor scene coverage. Existing bounded portrait timing may
be reused where suitable; it does not establish a general world clock.

Each new persistent category must deliberately extend save/load with a version
and validation policy. Transient visual phase needs an explicit reconstruction
policy; it must not become a second gameplay owner. Restoration must continue to
construct coherent owners and presentation without replaying prior effects.

Neither visual entry implicitly includes combat, movement capabilities, full
inventory/equipment use, Darkside or certification of a normally playable region.
Promote a concrete prerequisite only when the selected checkpoint establishes it.

## Replanning triggers

Revise direction when concrete evidence changes its premises:

- Original data or the chosen interaction exposes a substantial missing prerequisite.
- Normal travel exposes a movement, automatic-event or monster boundary that
  prevents the intended loop; record the exact blocker and bounded prerequisite.
- Save restoration exposes missing authoritative state or an ownership problem.
- A visual checkpoint needs scripted animation, alternate appearances or wall
  items outside the proposed subset.
- Work collapses entries together or grows enough to require a split, or
  maintainer priorities change the preferred direction.

Record evidence, affected dependencies and revised confidence here. Ordinary
milestone completion alone does not require rediscovering the whole roadmap.

## Review cadence

At closure, use the next entry as the default planning successor and check it
against the new stable baseline. Reassess the broader horizon after approximately
three completed milestones, or sooner after a major architectural discovery.
The post-M21 horizon review remains due before defining detailed M22 scope.

Use the verified post-push SHA and handoff gate in [AGENTS.md](../AGENTS.md) for
external planning/review. Planning approval and explicit authorization to implement
M22 remain separate; no M22 implementation is authorized by this roadmap.
