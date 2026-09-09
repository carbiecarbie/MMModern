# MMModern - Roadmap

The current stable baseline is **completed Milestone 22**. See
[project status](project-status.md) for present capabilities and
[project history](project-history.md) for completed milestones.

M22 completed bounded ordinary outdoor object animation; its delivered scope,
contracts and acceptance are recorded in the [closed M22 plan](milestone-22-plan.md).
The post-M21 horizon review retained the direction approved on 2026-09-08.
M23 is now the next planning candidate, retaining its provisional scope and
requirement to certify an original indoor encounter before detailed planning.
Neither roadmap direction nor detailed planning authorizes implementation.

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
| M23 | Indoor objects and a first visible indoor interaction | Next planning candidate; provisional encounter/scope | Correctly occluded objects and a bounded original indoor encounter |

### M23 - Indoor objects and a first visible indoor interaction

- **Goal:** interact with an original indoor object rendered with correct wall
  occlusion. This planning candidate is not a complete dungeon milestone.
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

The visual direction continues from delivered ordinary outdoor animation to
indoor scene coverage. M22's transient phase and existing idle integration do
not establish indoor projection/occlusion, indoor animation, scripted object
sequences or a general world clock. Its [runtime contract](milestone-22-plan.md#smallest-architecture-and-observable-runtime-policy)
owns the ordinary/NPC timing distinction.

Each new persistent category must deliberately extend save/load with a version
and validation policy. Transient visual phase needs an explicit reconstruction
policy; it must not become a second gameplay owner. Restoration must continue to
construct coherent owners and presentation without replaying prior effects.

This direction does not implicitly include combat, movement capabilities, full
inventory/equipment use, Darkside or certification of a normally playable region.
Promote a concrete prerequisite only when the selected checkpoint establishes it.

## Post-M21 horizon review

The post-M21 review selected M22 before M23 because original outdoor animation
had a verified Myra checkpoint and could reuse existing rendering and presentation
without a new persistent category. M22 is now complete. The
[M22 rationale](milestone-22-plan.md#baseline-and-post-m21-horizon-review) retains
that decision and the pinned-reference findings.

M23 still needs indoor projection/occlusion and an original encounter certified.
Static indoor objects do not depend on outdoor animation. Combat, route
certification and inventory use remain separate concerns; no additional future
milestones are promoted by M22 closure.

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
The required post-M21 horizon review is recorded above. Reassess sooner if the
listed triggers occur; completing M22 alone does not require speculative horizon
expansion or authorize M23 implementation.

Use the verified post-push SHA and handoff gate in [AGENTS.md](../AGENTS.md) for
external planning/review. Planning approval and explicit authorization to implement
M23 remain separate; no M23 implementation is authorized by this roadmap.
