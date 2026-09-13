# MMModern - Roadmap

**Milestone 29 is the latest completed milestone.**
[Project status](project-status.md) owns implemented capabilities and their
acceptance boundaries; [project history](project-history.md) owns completed
chronology. Reference provenance belongs to [dependencies](dependencies.md).

## Current planning state

MMModern has connected ordinary navigation, bounded original interactions,
quest rewards, inventory/equipment management and save/restart within accepted
checkpoints. The encounter foundation adds original actor approach
([M26](milestone-26-plan.md)), playable bounded Attack/Block combat
([M27](milestone-27-plan.md)), completed encounter persistence and revisit
([M28](milestone-28-plan.md)), and a bounded production Journey with automatic
attachment, guarded mutable return and v4 restart
([M29](milestone-29-plan.md)). These contracts do not certify travel between
checkpoints, normal original startup or a generally playable region.

The organizing goal is a meaningful connected Clouds vertical slice: a finite
piece of original gameplay with an understandable objective, real consequences
and a durable continuation. The priority is integrating accepted systems into
coherent gameplay. Additional diagnostics are useful as validation controls,
but are not substitutes for that integration.

**Post-M29 route-evidence investigation is next.** It is an investigation and
replanning gate before detailed M30 planning, not authorization to implement
M30. M30 and M31 remain provisional, dependency-gated directions whose content
and separation may change from the evidence. Roadmap selection, detailed
planning and implementation authorization remain separate. Create a milestone
plan only when that milestone is about to begin.

## Near term

### Post-M29 gate - Route evidence before detailed M30 planning

After the M29 closure is committed and its post-push baseline is verified,
perform a small targeted investigation of the preferred expedition against
original resources and the pinned reference. Establish actor influence across
movement, facing and waiting; encounter grouping; Zombie/Disease rules; attrition with the
proposed starting party; and whether recovery or Run is necessary. Include
reachable event branches, a credible failure/exit policy and readable navigation,
threat and outcome feedback. Parsed resources and terrain passability alone are
insufficient evidence.

Review the M30/M31 assumptions against those findings before creating the detailed
M30 plan. If Bone Whistle requires disproportionate actor, rule, recovery,
disengagement or persistence work, adapt both milestones to a better original
slice. Their exact scope and separation may change without changing the connected
Clouds strategy.

### M30 - Bounded expedition encounter integration (provisional)

Apply M29's continuity foundation to the actual actor and encounter consequences
of the selected expedition. The player should be able to navigate its bounded
footprint, face all materially influencing actors and resolve successive
encounters while retaining accumulated injuries, XP, item/equipment state and
world consequences through supported save/restart boundaries.

Add only the monster rules, conditions and failure behavior that route evidence
requires. Necessary recovery or disengagement must be explicitly scoped or trigger
replanning; they cannot be omitted while claiming the route is playable. This is
route-specific integration, not general combat or whole-map certification.

Confidence is conditional on the route gate; M29 continuity is now an accepted
foundation. This boundary provides a connected navigation/combat acceptance
target before full objective-loop closure in M31. Reuse the production path intended for that slice;
do not build a separate diagnostic framework. Detailed actors, rules and acceptance
contracts remain for the M30 specification.

### M31 - First connected Clouds vertical slice (provisional)

Close a short original-game objective loop on the expedition behavior established
by M30. The player should understand the entry, objective and return/continuation,
manage party/items between encounters, complete the original interaction and
resume the resulting journey after a process restart. Navigation, combat,
interaction and persistence must operate coherently through production controls.

The preferred first slice is a bounded expedition in Clouds map 20: enter once
with a declared original party/resource state, navigate to the Whistle, resolve
the influencing encounters, perform the already-supported original collection,
and return with party/world consequences preserved through save/restart.
Success should connect navigation, combat, inventory/equipment and progression
in one session, including consequences across successive encounters.

This is a bounded entry into original content, not a claim of travel from
Vertigo or from the Skeleton diagnostic. Its attraction is a short route and a
small quest-script surface that reuse existing outdoor and interaction systems.
The [supported checkpoints](project-status.md#supported-original-data-checkpoints)
establish the collection behavior, not the expedition.

Confidence in the exact slice remains conditional on the route gate and M30
integration. Bone Whistle is the preferred candidate, not an irrevocable content
commitment. Do not obtain a successful slice through repeated relocation, state
injection, suppressed influencing actors or altered commercial data.

M31 certifies the selected loop, not all of map 20, general Clouds exploration,
normal Vertigo startup, full recovery, complete combat/item effects, services or
original UI/audio fidelity. Provide the controls and feedback required to play
and understand that loop. Closure requires connected automated evidence,
independent review and maintainer physical acceptance appropriate to its actual
scope. Split or combine M30/M31 if integration evidence supports a better boundary.

## Medium term

Expand capabilities in response to playable-route evidence rather than assign
speculative milestone numbers:

- **Connected quests and rewards:** Myra -> Phirna -> Myra is a stronger later
  RPG slice, linking acquisition, return rewards and party management. Its route
  adds actor, ranged-combat, poison/sleep and possible treasure dependencies;
  accepted dialogue and reward checkpoints do not certify that journey.
- **Encounter breadth and recovery:** support the additional actors, targeting,
  damage, conditions and failure outcomes required by selected routes. Add
  selective item effects, healing, rest or disengagement when those outcomes
  justify them. Inventory/catalog support does not imply item effects; rest
  carries food, time, interruption and temporary-effect consequences.
- **Original startup and connected regions:** establish Vertigo's logical indoor
  area and neighboring geometry semantics, then connect increasingly playable
  towns, outdoors and dungeons. Doors, hazards, lighting and movement abilities
  should follow the dependencies of those routes.
- **Sustained progression:** extend quest rewards, item/spell/combat support and
  services such as shops, temples, inns and training as coherent progression
  requires them. Economy, broader time behavior and persistence across visits
  must advance with their gameplay consumers.

Reassess these unnumbered directions after the first slice. Myra/Phirna and
normal startup remain candidates for different player-visible value, not a
fixed sequence following Bone Whistle. Persistence must grow with admitted
state without silently discarding consequences or changing legacy save meaning.

## Long term

Progress from bounded connected Clouds gameplay to increasingly playable
connected regions, normal Clouds progression, broad gameplay coverage and
substantially complete Clouds of Xeen. Expand engine and content support toward
Darkside progression and the eventual World of Xeen objective as the shared
foundations become reliable. Access to Clouds metadata in DARK.CC does not
establish Darkside gameplay.

Darkside content progression, portability, localization, distribution and wider
presentation/audio fidelity remain separate planning decisions. Their ordering
should follow demonstrated dependencies and maintainer priorities rather than
being inferred from a Clouds milestone closure.

## Architectural direction

Preserve the established owners: characters, items and progression belong to
party/roster; actors and lifecycle belong to world; camera/game flags retain
their existing owners. Flow and SDL coordinate transient gameplay and
presentation. Reuse accepted systems before introducing parallel frameworks.

Completed Diagnostic27 remains terminal and quiescent under its accepted
contract. Mutable Journey gameplay uses M29's separate admitted domain and
retains M28's reusable owner/preimage and presentation safeguards without
changing diagnostic semantics. Commercial resources remain external and
unmodified.

## Replanning and review cadence

Review each specification against verified repository state and the pinned
reference. Replan when:

- Route evidence reveals substantially more actor, combat-rule, condition or
  recovery work than expected.
- The objective requires unsupported travel, Run/disengagement, services or
  time/recovery semantics beyond its proposed boundary.
- Persistence or world-state generalization exceeds the bounded objective, or
  compatibility and lifecycle policies cannot preserve admitted consequences.
- Integration or acceptance evidence supports splitting or combining milestones.
- Another slice becomes materially cheaper or more architecturally valuable,
  or maintainer priorities change.

Retain approximately three completed milestones as the ordinary broader roadmap
review cadence, with focused scope review before each specification. This is
not a quota and does not delay an earlier evidence-driven review. Completion of
the provisional M30-M31 direction is a natural broader reassessment point; the
post-M29 route gate remains necessary before detailed M30 planning. Use the
verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external planning
and review. Roadmap approval, detailed planning and implementation authorization
remain separate.
