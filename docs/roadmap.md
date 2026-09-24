# MMModern - Roadmap

**Milestone 37 is the latest completed milestone.**
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
([M33](milestone-33-plan.md)). M34 completed individual Run, partial-party combat,
non-victory fixed relocation and exact survivor/treasure continuation in 5/5
([M34](milestone-34-plan.md)). M35 connected the Myra/Phirna quest, selected
well recovery and bounded antidote use with exact 6/6 continuation
([M35](milestone-35-plan.md)). M36 added learned First Aid and Awaken with exact
7/7 continuation on the same mainland ([M36](milestone-36-plan.md)). M37
completed original Vertigo entry, bounded traversal, return/revisit and retained
two-region 8/8 continuation ([M37](milestone-37-plan.md)).
These contracts do not certify unrestricted map-23
or Clouds travel or normal original startup.

The approved M33-M35 arc is complete within its bounded map-23 mainland scope.
After the maintainer-reviewed post-M35 reassessment, the accepted direction is
to pause quest-driven vertical slices and develop reusable Clouds systems with
Vertigo progressively serving as their production hub. A bounded learned
exploration-casting step was completed first using original learned spells on
the admitted mainland. Completed Vertigo admission establishes the city/world
boundary for the next bounded town service.

## Near term

The accepted short arc is M36-M38. M36 and M37 are completed; **M38 - Vertigo
blacksmith and economy** is the immediate provisional successor. Its purpose
below is a roadmap direction, not a technical specification or implementation
authorization. M38 still requires scope review, specification and explicit
implementation authorization; M37 completion does not authorize that work.
Change this decomposition only when concrete implementation evidence shows a
materially better acceptance boundary.

### M38 - Vertigo blacksmith and economy

Use the admitted city to establish a first meaningful town service and reusable
economy transaction path. A supported blacksmith transaction should give
accumulated gold a gameplay use and connect the purse, items, inventory and
equipment, merchant state and quiet persistence in an original location.
Broader trading, complete shop/item functionality and other services remain
outside this unit.

## Medium term

After the initial Vertigo foundation/economy arc, conduct a focused
reassessment **within the accepted Vertigo-centered systems direction**:
given the infrastructure then actually present, which reusable system has the
best next production and acceptance consumer? Candidates, not a committed
sequence, include training/progression, broader magic and combat casting,
guild/spell acquisition, temple and recovery services, and further economy/item
breadth. Route evidence may identify another prerequisite system. Training
deserves particular attention as a consumer of both XP and gold, but its
original time/day consequences should be evaluated against the city and time
foundations then available.

Further city content, regions, quests and encounters should follow demonstrated
system or route value. Another quest chain is not the default successor. Grow
persistence with admitted state without discarding consequences or changing
legacy save meanings.

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
consequences and party-owned monster treasure, M34's participation/non-victory
lifecycle and dormant-item semantics, M36's learned casting, M37's retained
two-region transitions and 8/8 continuation, and M28's
owner/preimage and presentation safeguards.
Commercial resources remain external and unmodified.

## Replanning and review cadence

Review each specification against verified repository state and the pinned
reference. Refine a milestone boundary when its route, service, time or
persistence dependencies prevent independent acceptance. Reopen the accepted
Vertigo-centered strategy only if new evidence shows a material contradiction:

- Vertigo admission requires substantially broader architecture than the
  current evidence indicates, or actor/event/persistence ownership requires a
  fundamental redesign.
- A major system outside the short arc becomes a proven prerequisite.
- The proposed units cannot be independently accepted despite a focused scope
  adjustment.
- Another original area or system demonstrates materially better leverage for
  a required foundation, or maintainer priorities explicitly change.

Ordinary implementation difficulty alone does not reopen the direction.
Retain approximately three completed milestones as the ordinary broader-roadmap
review cadence, with focused scope review before each specification and the
focused system choice after M38. Roadmap approval, milestone specification and
implementation authorization remain separate; completing one unit does not
authorize the next.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
