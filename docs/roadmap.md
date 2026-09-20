# MMModern - Roadmap

**Milestone 33 is the latest completed milestone.**
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
M32 then established the Regional Journey foundation: resource-derived map-23
mainland navigation, complete 19-actor ownership/scheduling, exact sign admission,
truthful support boundaries and schema-3 continuation
([M32](milestone-32-plan.md)). M33 added admitted mainland combat, ranged/Shoot,
conditions, rewards and exact consequence-aware 4/4 restart
([M33](milestone-33-plan.md)). These contracts do not certify unrestricted map-23
or Clouds travel or normal original startup.

The first connected Clouds vertical slice and its Regional Journey foundation
are accepted. The approved Myra -> Phirna -> Myra arc continues across the
connected mainland region of Clouds map 23.

## Near term

<a id="approved-m33-m35-arc"></a>

### Approved M33-M35 arc

This sequence records the approved strategic direction, not detailed milestone
specifications. Each milestone requires its own specification and explicit
implementation authorization; roadmap approval does not authorize implementation.

M33 is the completed combat/consequence foundation of this arc. **M34 is the
immediate next milestone**; its implementation requires separate authorization.

- **M34 - Original disengagement and encounter lifecycle:** add per-character
  Run, partial-party combat participation, escape/failure/casualty and treasure
  outcomes, fixed original relocation and non-victory retirement. Preserve
  surviving damaged enemies through return, re-engagement and save/restart
  without healing or replay.
- **M35 - Connected Myra quest and local recovery:** integrate the existing
  [M21 endpoints](milestone-21-plan.md) through the expanded Journey and selected
  route-justified mainland interactions/recovery, including a narrow reusable
  antidote-item-use path. Close the arc with Myra request -> mainland travel and
  encounters -> Phirna collection -> connected return -> Myra exchange/reward
  -> save/restart -> continued gameplay, preserving accumulated consequences.

Unless evidence triggers replanning, this arc excludes general magic/spellcasting,
full Rest, shops/temples/inns and broader services/economy, training/permanent
level progression, Swimming/Walk on Water/Mountaineer and disconnected map-23
areas, Vertigo/mines/adjacent maps/Darkside transfer, unrestricted higher-tier
loot/item effects and unrelated side-quest completion. Gold and XP are durable
consequences; their spending and progression consumers remain outside the
accepted scope.

## Medium term

Beyond M35, expand capabilities in response to playable-route evidence without
assigning speculative milestone numbers or ordering:

- **Connected quests and rewards:** extend the planned Myra/Phirna foundation
  toward further original objectives as their routes and dependencies justify.
- **Encounter breadth and recovery:** M33-M35 cover the selected mainland
  combat, disengagement and local recovery. Broader actors, conditions, item
  effects, healing and rest remain future route-driven work. Rest carries food,
  time, interruption and temporary-effect consequences.
- **Original startup and connected regions:** establish Vertigo's logical indoor
  area and neighboring geometry semantics, then connect increasingly playable
  towns, outdoors and dungeons. Doors, hazards, lighting and movement abilities
  should follow route dependencies.
- **Sustained progression:** extend quest rewards, item/spell/combat support and
  services such as shops, temples, inns and training as coherent progression
  requires them. Economy, broader time behavior and persistence across visits
  must advance with their consumers.

These unnumbered post-M35 directions remain provisional. Persistence must grow
with admitted state without silently discarding consequences or changing legacy
save meaning.

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
party/roster; actors, lifecycle and Journey RNG belong to world; camera/game
flags retain their existing owners. Flow and SDL coordinate transient gameplay,
modal work, presentation and save authority. Reuse accepted systems before
introducing parallel frameworks.

Completed Diagnostic27 remains terminal and quiescent under its contract.
Mutable Journey uses M29's admitted domain, M30's content descriptor/group/
current-state extensions, M31's exclusive event/publication integration, M32's
resource-derived regional/complete-actor/context foundation, M33's shared physical
consequences and party-owned monster treasure, and M28's
owner/preimage and presentation safeguards.
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
review cadence, with focused scope review before each specification. The post-M31
investigation is complete; M35 arc closure is the next natural broader roadmap
reassessment point, with the evidence-based triggers above applying throughout
M33-M35. Subsequent roadmap approval, milestone specification approval and
implementation authorization
remain separate; completion does not authorize the next milestone.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
