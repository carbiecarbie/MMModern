# MMModern - Roadmap

The current stable baseline is **completed Milestone 23**. See
[project status](project-status.md) for present capabilities and
[project history](project-history.md) for completed milestones.

M23 completed the bounded static indoor-object and visible-interaction foundation;
its delivered scope, contracts and acceptance are recorded in the
[closed M23 plan](milestone-23-plan.md). No successor milestone is currently
promoted. The next planning activity is a post-M23 horizon reassessment, and
neither that review nor later roadmap direction authorizes implementation.

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

## Current planning state

M23 established static ordinary indoor-object composition and certified one
bounded original visible interaction. No next milestone has been selected or
approved. The post-M23 horizon reassessment will compare maintainer priorities
with the new stable architecture, original-data evidence, prerequisite boundaries
and remaining unsupported capabilities before promoting a concrete milestone.

Do not infer a successor number or implementation scope from this planning state.
Roadmap review, future milestone promotion and implementation authorization are
separate decisions.

## Dependencies and boundaries across future work

M23's static indoor-object composition does not establish indoor animation, wall
items, scripted object sequences, connected-map rendering or a general world
clock. Its [closed contract](milestone-23-plan.md) owns the bounded placement,
occlusion, interaction and persistence boundary. M22's
[runtime contract](milestone-22-plan.md#smallest-architecture-and-observable-runtime-policy)
continues to own ordinary outdoor and NPC timing distinctions.

Each new persistent category must deliberately extend save/load with a version
and validation policy. Transient visual phase needs an explicit reconstruction
policy; it must not become a second gameplay owner. Restoration must continue to
construct coherent owners and presentation without replaying prior effects.

This direction does not implicitly include combat, movement capabilities, full
inventory/equipment use, Darkside or certification of a normally playable region.
Promote a concrete prerequisite only when the selected checkpoint establishes it.

## Completed horizon direction

The post-M21 review selected M22 before M23. M22 delivered bounded ordinary
outdoor animation, and M23 subsequently delivered indoor static-object projection,
wall occlusion and the Nightshadow checkpoint. Their closed plans preserve the
durable rationale and contracts. Combat, route certification, inventory use and
other broad capabilities remain separate concerns; M23 closure promotes none of
them automatically.

## Replanning triggers

Revise direction when concrete evidence changes its premises:

- Original data or the chosen interaction exposes a substantial missing prerequisite.
- Normal travel exposes a movement, automatic-event or monster boundary that
  prevents the intended loop; record the exact blocker and bounded prerequisite.
- Save restoration exposes missing authoritative state or an ownership problem.
- A visual checkpoint needs scripted animation, alternate appearances or wall
  items outside the currently supported subsets.
- Work collapses entries together or grows enough to require a split, or
  maintainer priorities change the preferred direction.

Record evidence, affected dependencies and revised confidence here. The explicit
post-M23 reassessment is the current review point; later ordinary milestone
completion alone does not require rediscovering the whole roadmap.

## Review cadence

When a next entry exists, use it as the default planning successor and check it
against the then-stable baseline. The maintainer has chosen a broader post-M23
horizon reassessment before promoting any successor. After that review, reassess
the broader horizon approximately every three completed milestones or sooner
after a listed trigger.

Use the verified post-push SHA and handoff gate in [AGENTS.md](../AGENTS.md) for
external planning/review. Roadmap review, planning approval and explicit
implementation authorization remain separate; this roadmap authorizes no new
implementation.
