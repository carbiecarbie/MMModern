# MMModern - Roadmap

**Milestone 31 is the latest completed milestone.**
[Project status](project-status.md) owns implemented capabilities and acceptance
boundaries; [project history](project-history.md) owns completed chronology.
Reference provenance belongs to [dependencies](dependencies.md).

## Current planning state

MMModern has connected ordinary navigation, bounded original interactions,
quest rewards, inventory/equipment management and save/restart within accepted
checkpoints. The encounter line now includes original actor approach
([M26](milestone-26-plan.md)), playable bounded combat
([M27](milestone-27-plan.md)), completed encounter persistence/revisit
([M28](milestone-28-plan.md)), mutable Journey continuity
([M29](milestone-29-plan.md)), and the production contract-2 Bone Whistle
expedition with grouped Skeleton/Zombie combat, accumulated consequences,
schema-2 restart ([M30](milestone-30-plan.md)), followed by connected original
Bone Whistle collection, return and restart ([M31](milestone-31-plan.md)).
These contracts do not certify travel between checkpoints, normal original
startup or a generally playable region.

The first connected Clouds vertical slice is accepted: a finite original
objective with encounter consequences, collection, return and durable
continuation. The next planning decision is which bounded expansion provides
the most useful original gameplay on that foundation.

## Near term

### Post-M31 roadmap investigation

M31 completion reaches the planned broader reassessment point. A focused
roadmap investigation is required before selecting the next concrete milestone;
the accepted slice does not make an immediate M32 scope unambiguous.

Compare the medium-term candidates against the accepted M31 state, original
route dependencies and maintainer priorities. Establish the next player-visible
objective, its minimum reusable capabilities, compatibility boundary and
acceptance route. Distinguish verified dependencies from provisional estimates
and record the resulting order and confidence here before a milestone plan is
created. No M32 specification or implementation is authorized by M31 closure.

## Medium term

Expand capabilities in response to playable-route evidence rather than assign
speculative milestone numbers:

- **Connected quests and rewards:** Myra -> Phirna -> Myra is a stronger later
  RPG slice, linking acquisition, return rewards and party management. Its route
  adds actor, ranged-combat, poison/sleep and possible treasure dependencies;
  accepted dialogue and reward checkpoints do not certify that journey.
- **Encounter breadth and recovery:** support additional actors, targeting,
  damage, conditions and failure outcomes required by selected routes. Add
  selective item effects, healing, rest or disengagement when those routes
  justify them. Rest carries food, time, interruption and temporary-effect
  consequences.
- **Original startup and connected regions:** establish Vertigo's logical indoor
  area and neighboring geometry semantics, then connect increasingly playable
  towns, outdoors and dungeons. Doors, hazards, lighting and movement abilities
  should follow route dependencies.
- **Sustained progression:** extend quest rewards, item/spell/combat support and
  services such as shops, temples, inns and training as coherent progression
  requires them. Economy, broader time behavior and persistence across visits
  must advance with their consumers.

These unnumbered directions are provisional inputs to the post-M31 investigation.
Myra/Phirna and normal startup remain candidates for different player-visible value, not a
fixed sequence following Bone Whistle. Persistence must grow with admitted state
without silently discarding consequences or changing legacy save meaning.

## Long term

Progress from bounded connected Clouds gameplay to increasingly playable
regions, normal Clouds progression, broad gameplay coverage and substantially
complete Clouds of Xeen. Expand shared foundations toward Darkside progression
and the eventual World of Xeen objective. Access to Clouds metadata in DARK.CC
does not establish Darkside gameplay.

Darkside content progression, portability, localization, distribution and wider
presentation/audio fidelity remain separate planning decisions. Their ordering
should follow demonstrated dependencies and maintainer priorities.

## Architectural direction

Preserve established owners: characters, items and progression belong to
party/roster; actors, lifecycle and contract-2 RNG belong to world; camera/game
flags retain their existing owners. Flow and SDL coordinate transient gameplay,
modal work, presentation and save authority. Reuse accepted systems before
introducing parallel frameworks.

Completed Diagnostic27 remains terminal and quiescent under its contract.
Mutable Journey uses M29's admitted domain, M30's content descriptor/group/
current-state extensions, M31's exclusive event/publication integration, and
M28's owner/preimage and presentation safeguards.
Commercial resources remain external and unmodified.

## Replanning and review cadence

Review each specification against verified repository state and the pinned
reference. Replan when:

- The objective requires unsupported travel, Run/disengagement, services,
  recovery or time semantics beyond its proposed boundary.
- Persistence or world-state generalization exceeds the bounded objective, or
  compatibility cannot preserve admitted consequences.
- Integration evidence supports splitting the objective or another slice becomes
  materially cheaper or more architecturally valuable.
- Maintainer priorities change.

Retain approximately three completed milestones as the ordinary broader-roadmap
review cadence, with focused scope review before each specification. Completion
of M31 triggers the focused roadmap investigation above. Subsequent roadmap
approval, milestone specification approval and implementation authorization
remain separate; completion does not authorize the next milestone.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
