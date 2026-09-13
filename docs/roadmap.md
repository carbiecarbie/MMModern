# MMModern - Roadmap

**Milestone 30 is the latest completed milestone.**
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
schema-2 restart and Deferred objective presentation
([M30](milestone-30-plan.md)). These contracts do not certify travel between
checkpoints, normal original startup or a generally playable region.

The organizing goal remains a meaningful connected Clouds vertical slice: a
finite piece of original gameplay with an understandable objective, real
consequences and durable continuation. M30 supplies the accepted route, grouped
combat, presentation, persistence and event-disposition seam. M31 is the
immediate next planned direction and requires separate specification and
implementation authorization.

## Near term

### M31 - First connected Clouds vertical slice

Close the selected Bone Whistle objective loop on the accepted M30 expedition.
The player should enter through the declared prepared boundary, navigate and
resolve the influencing encounters, perform the original collection, return
with party/world consequences intact, and resume the result after a process
restart.

Integrate the existing ordinary WhoWill/grant/Remove continuation with M30's
same Journey owners, Deferred event address and presentation/save safeguards.
Retain grouped targets, automatic joining/multiattack, Disease/injuries,
inventory/equipment mutation, world RNG continuation and schema-2 current-state
persistence. A suspended objective interaction must hold exclusive coordination
and remain unsaveable until its acknowledged result and new frame establish a
quiet boundary.

This is a bounded entry into original content, not a claim of travel from
Vertigo, general map-20 exploration or normal original-game startup. M31
certifies the selected objective loop only. It does not imply full recovery,
general combat/item effects, services, original UI/audio fidelity or Darkside
gameplay. Optional First Aid is not required by M30 and should be admitted only
through a separately justified reusable spell/effect contract.

Do not obtain the slice through repeated relocation, state injection, suppressed
influencing actors or altered commercial data. Closure requires connected
automated evidence, independent implementation review and maintainer physical
acceptance appropriate to M31's actual scope.

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

Reassess these unnumbered directions after the first slice. Myra/Phirna and
normal startup remain candidates for different player-visible value, not a
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
current-state extensions, and M28's owner/preimage and presentation safeguards.
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
of M31 is a natural broader reassessment point. M30's production foundation is
accepted; M31 planning approval and implementation authorization remain separate.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
