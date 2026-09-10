# MMModern - Roadmap

**Milestone 25 is the latest completed milestone.**
[Project status](project-status.md) owns current capabilities;
[project history](project-history.md) and the [closed M25 plan](milestone-25-plan.md)
own completed work. Reference provenance belongs to [dependencies](dependencies.md).

## Current planning state

The maintainer has prioritized progress toward meaningful, visible combat in
Clouds. The proposed sequence below requires roadmap approval; detailed
specification and implementation remain separately authorized. No next milestone
is yet specified or implementation-authorized.

Recommend an original outdoor Skeleton encounter, reached through an explicitly
bounded diagnostic entry, followed by combat actions and then durable completion.
Do not postpone this arc for inventory completion or normal-start navigation.
M25 equipment supplies useful agency, but does not implement attack, defense,
monster simulation, gameplay time or progression.

The implementation agent investigates and implements authorized work; the
architecture/specification agent prepares contracts; the independent reviewer
audits implementation and evidence; the maintainer approves direction and
separately authorizes work. Create a detailed plan only when its milestone is
about to begin.

## Preferred encounter and acceptance envelope

Use **Clouds map 20, original monster record 5 at `(13,2)`, Skeleton type 8**.
The initial Clouds `maze0020.mob` supplies placement and stable record identity;
`maze0020.dat` supplies terrain. In the validated World of Xeen installation,
`DARK.CC/xeen.mon` supplies statistics and `XEEN.CC/008.mon` and `008.att` supply
appearance. These commercial resources remain external and unmodified.
The pinned [map/resource lifecycle](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/map.cpp)
and [combat paths](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp)
are the reference anchors for subsequent specification.

The Skeleton has 20 HP, AC 5, speed 10, one physical attack with two six-sided
damage dice, hit parameter 4, Cleric preference and 50% physical resistance.
It has no ranged or special attack, gold, gems or item drop. Its base XP award
is 250. These properties eliminate random-loot and condition-inflicting attack
systems from this encounter, without eliminating mandatory retaliation, armor
breakage on severe injury, XP or gameplay-time consequences.

Recommend the **World of Xeen Clouds-side behavior profile**, matching the
accepted installation layout, with original initial Adventurer difficulty.
The pinned reference doubles per-recipient XP below level 15 outside standalone
Clouds: six eligible level-one recipients receive 82 each for this Skeleton,
after division and then doubling. Standalone Clouds would give 41. Resource
availability alone must not silently select a different rules profile; detailed
specification must declare the chosen profile and default reference options.
This does not authorize Darkside gameplay.

Start acceptance at map 20 `(13,1)` North, using the original six active roster
owners. The initial envelope is the four cells `x=13..14, y=1..2`, not a route
from Vertigo. Resource inspection finds ordinary surface type 1, no event records
or cell hazards there, and only record 5 inside the reference's seven-by-seven
movement scan around any of those cells. The target can activate in front of the
party and approach its cell; engagement follows original actor occupancy and
ordering, not an invented adjacent-cell combat rule. Preserve all original
records and consider offscreen actors; never obtain isolation by disabling them.

This envelope has real limits. South/west of the entry are blocked space cells;
farther north can bring Skeleton record 7 at `(10,6)` into movement range.
Map 20 also contains Zombies, other Skeletons and unsupported event tails.
Its Run destination `(0,3)` lies near two Skeletons and a Zombie. Run and combat
rotation/disengagement therefore remain explicit unsupported commands in this
first Attack/Block domain; neither means a harmless exit to the entry cell.
Leaving the supported envelope must report a diagnostic boundary before advancing
unsupported gameplay. It must not present an invisible wall as original geography.

This is a source-and-resource-supported choice, not exercised reference combat
or maintainer physical acceptance. M26 specification must confirm the exact
approach/input sequence, activation and delayed movement scheduling against the
pinned reference. If that narrow check disproves isolation or the entry sequence,
reconsider the envelope before implementation rather than freezing other actors.

## Preferred milestone sequence

### M26 - Visible original actor approach and engagement

**Objective and observable result:** the original Skeleton appears in the outdoor
scene, activates and approaches under supported player steps/waiting, and reaches
an explicit engagement boundary that prevents continuing ordinary exploration.
This is the first meaningful visible enemy interaction. It deliberately stops
before combat actions; it is not a completed fight.

**Scope and order:** add bounded monster-stat resource interpretation, original
record identity, authoritative actor position/lifecycle state, outdoor monster
composition and the approach/engagement rules needed by the anchor. Reuse map,
archive, scene, input and party infrastructure. Include the supported step/time
cost and pending monster-movement scheduling; do not derive enemy actions from
ordinary-object or portrait animation ticks. Gameplay time starts from original
party state and must stop explicitly before an unsupported calendar/condition
boundary. A general calendar, indoor monsters, attacks, rewards, Run, ranged
combat and connected travel are excluded.

Actor state and its composition/activation boundary belong before damage because
they determine whom a player can encounter, where that actor lives, and whether
reconstruction can duplicate or reset it. This is an independently observable
contract with substantial ownership risk, not a generic architecture-preparation
milestone. Additional character attack calculations belong inside M27, where
actions actually consume them; no separate inventory or character-sheet milestone
is justified.

**Acceptance:** deterministic identity, occupancy, movement, activation, boundary
and cache-reconstruction checks; original-data approach evidence including nearby
actors; scene ordering/clipping and unchanged ordinary-object behavior; one SDL
loop, explicit unsupported engagement feedback and no event/inventory bypass.
Maintainer physical acceptance must observe approach and engagement separately
from automated/image evidence. Record the new state categories and intended
persistence boundary before introducing new owners. Until M28 supports persistence, the diagnostic
encounter session is explicitly unsaveable, including after an interrupted
approach; existing non-encounter save behavior remains governed by
[M20](milestone-20-plan.md) and [M21](milestone-21-plan.md).

**Confidence:** medium. The actor and local resource envelope are established;
precise scheduling, monster projection and interaction ordering still need a
bounded specification. Additional actors entering this envelope or a required
new terrain/event contract trigger replanning.

### M27 - Playable Attack/Block encounter with original outcomes

**Objective and observable result:** play a sequence of original melee Attack and
Block actions against that Skeleton, with identifiable current actor/target,
misses, damage, mandatory enemy turns, party injury and either victory or party
defeat. This is the first playable exchange of combat actions. Victory removes
the original actor and awards the applicable XP once within the session; there
is no fabricated treasure reward.

**Scope and order:** build on M26's actor and engagement owner. Add only the
character inputs and calculations used by these paths: Might, Accuracy, Speed,
temporary AC, relevant equipped-item effects, age/level/difficulty contributions,
hit and damage rules, initiative, Block, eligible target selection and progression.
Reuse roster-owned HP/SP, conditions, item bytes and existing maximum-HP rules.
Honor the Skeleton's resistance and Cleric preference, negative HP, unconsciousness,
death and required equipped-armor breakage. Do not substitute arbitrary formulas
or drop a second damage application present in a reference critical-hit branch.
Random decisions need deterministic test control, separate from visual randomness.

The initial combat domain is the original unique six-member party, its original
item records and supported M24/M25 rearrangements, initially Good, plus conditions
and item breakage reachable through the supported fight. Effects of every admitted
loadout must be handled, including the initially unequipped attribute ring if
admitted after Equip. A bow can remain equipped without making Shoot supported.
Opaque items, arbitrary legacy loadouts, alias parties, active spell buffs and
other conditions require explicit combat admission decisions; preserve their
existing storage/inspection behavior rather than silently normalizing them.
Shops, repair, identification, consumables, spells, ranged attacks, combat-time
inventory mutations, Run and combat rotation remain outside this slice.

Original rounds/end transitions advance game time. Specify a bounded daytime
window from the initial day 1/year 610/minute 480 state, with explicit refusal
before unimplemented time effects; unlimited Block cannot silently freeze time.
A terminal defeat must stop gameplay without invented healing. Repair, rest or
resurrection are not prerequisites for demonstrating a single victory/defeat.

**Acceptance:** independent literal rule/RNG oracles and original party/item
comparisons; complete visible victory and loss evidence, forced retaliation,
critical hit, Block, injury/breakage, XP eligibility/rounding and no duplicate
outcome after failed drawing or repeated input. Tests must distinguish command
rejection from turn consumption and cosmetic waits from gameplay advancement.
Maintainer physical SDL acceptance observes a real exchange and readable outcomes.
This is session gameplay: F9 remains unavailable for the encounter session even
after victory until M28 can capture every new authoritative value.

**Confidence:** medium for the Attack/Block objective; lower for exact workload
until the bounded rule domain is specified. Split internal delivery only for
independently testable contracts; do not declare a damage calculator or an enemy
image to be this milestone's playable result.

### M28 - Durable bounded encounter completion and revisit

**Objective and observable result:** finish the supported encounter, save at an
eligible quiescent boundary, terminate the process, resume and revisit the
checkpoint with the enemy still defeated, XP awarded once, and exact party HP,
conditions and broken/equipped item state. M28 completes the supported encounter
**and its persistence boundary**, not a repeatable exploration/recovery loop.

**Scope and order:** extend existing capture, serialization, validation and
restore for the actor state, new character/progression inputs and gameplay
context introduced by M26/M27. Decide format version and legacy compatibility
explicitly; v2 cannot represent these values. Preserve existing saved fields and
resource checks, and distinguish missing legacy data from explicit new values.
Do not infer monster deaths from ordinary-object Remove or reconstruct earned
XP from the initial roster. The compatibility policy must be chosen before this
milestone's implementation, with the state requirements identified in M26.

Live partial HP and moved position must survive disposable cache rebuilds.
The pinned reference writes monster positions/removal into MOB state, but reloads
surviving monsters at their resource HP on a real map load, including restart.
Prefer that semantic reset at actual map entry/load; it must never occur merely
because MMModern discarded a rendering/resource cache. Combat, pending movement,
action publication and unresolved encounter outcomes remain unsaveable. Do not
silently turn a mid-fight process exit into a saved victory or a healed live foe.
The first saveable completion is victory with a viable party and all outcomes
published; defeat and unsupported-boundary stops are not successful completions.

True map re-entry may use disclosed diagnostic access for acceptance. This does
not certify travel across map 20, the Run landing region, Vertigo, or the Bone
Whistle route. Run, free disengagement, recovery and wider time effects are still
excluded. If later scope permits saving a surviving wounded actor outside combat,
the reference HP-reset policy must receive its own connected acceptance.

**Acceptance:** use the existing production save/restart harness and fresh owners;
compare full expected state, actual disk data and separate-process restoration;
prove no respawn/reward replay, correct legacy behavior, refused unsafe saves and
preservation on I/O failure. Independently distinguish cache reconstruction,
diagnostic real map re-entry, restart and fresh new game. Require maintainer
physical save/terminate/resume/revisit acceptance in addition to automated evidence.

**Confidence:** medium on the need for this boundary, provisional on exact format
and compatibility scope. New durable categories, broader save eligibility or an
unresolvable migration conflict require review before widening implementation.

## Architectural risks and shared boundaries

- Keep roster characters/items and party progression in the accepted party owner,
  and committed camera/game flags with their existing owner. Monster identity is
  side/map/original monster-record index, distinct from ordinary-object identity.
  Mutable monster HP/position/lifecycle needs genuine authoritative world state;
  the current disabled-object/event sets and immutable MOB cache are insufficient.
- Input/modal coordination must prevent navigation, events, inventory/equipment,
  saving and combat from advancing incompatible work simultaneously. Reuse the
  Application/Flow/SDL seams without making the event interpreter a combat engine
  or making Flow a second party/world owner. Preserve publication-before-feedback
  and once-only outcomes across fallible presentation.
- Initiative, turn progress, selected target and pending action coordination are
  transient at the proposed save boundary. HP, conditions, item breakage, XP and
  admitted gameplay-time/context values are authoritative. Statistics, occupancy
  queries, scene commands and sprite frames are derived; reconstruction cannot
  award XP or advance a turn. Reference rendering currently contributes to actor
  activation and delayed movement, so extracting that behavior requires explicit
  coordination rather than copying mutation into MMModern's compositor.
- Existing inventory/catalog/equipment acceptance is reusable infrastructure;
  it is not acceptance of combat effects for every storable item. Conversely,
  missing combat integration does not justify rebuilding the accepted inventory,
  event-reward or persistence systems. The selected Skeleton produces no item
  drops, so M21's bounded miscellaneous reward queue need not become a loot system.

## Alternatives and planning horizon

Vertigo's Slimes/Doom Bugs are credible early-game alternatives, but their attacks
use poison damage/resistance and saving throws even though they have no poison
condition special. Vertigo's original actors and party occupy a 32-coordinate
indoor layout across connected maps; accepting normal start adds navigation and
script work that this outdoor diagnostic fight does not need. Their lower HP is
not sufficient reason to select them first.

Map 23 remains valuable for the eventual Myra -> Phirna -> Myra loop. Its 19
original active actors include ranged/loot-bearing Orcs, poisoning Giant Snakes,
Giant Toads and undead. A terrain path between accepted event coordinates does
not establish a playable route. Neighboring map 24 has similar mixed threats.
Nightshadow's visible gravestone likewise does not make its Bat Queens and
Vampires a simple combat anchor. Map 20's selected isolated Skeleton avoids
those immediate dependencies while exercising reusable melee, defense, actor
lifecycle and progression rules.

Inventory completion, indoor animation and normal-start connectivity offer
independent value but delay the selected enemy interaction without satisfying a
prerequisite it actually has. Combining all three proposed milestones would hide
actor integration, combat-rule and persistence risks in one large acceptance gate.
Splitting out general statistics, loot, recovery or calendar foundations would
add scope before there is evidence that this bounded fight needs them.

Stop numbered planning at durable completion of this encounter. That is enough
to test whether the ownership, rules and persistence design supports real combat;
a longer sequence would speculate about recovery and route requirements before
there is playable evidence. The next conditional objective is a repeatable
recovery/exploration loop, selected after combat acceptance. Rest has food,
eight-hour time, interruption, condition and temporary-effect consequences;
item healing/antidotes and town services have their own action/target/economic
contracts. None should be treated as automatic HP reset or assigned a milestone
number now.

Broader direction remains connected original Clouds quests/exploration, selective
item effects/services, encounter-driven presentation/audio, and eventually wider
World of Xeen fidelity. Darkside progression, portability and localization remain
separate decisions. Preserve the distinction between diagnostic checkpoints,
normal original start, connected routes and generally playable regions.

## Replanning and review cadence

Review the next specification against the verified Git baseline and the pinned
reference. Replan earlier if:

- Exact approach/scheduling or offscreen actor tracing invalidates the bounded
  envelope, or a more faithful simpler encounter is demonstrated.
- Supported equipment/conditions, the chosen game/difficulty profile, combat time
  or a reference quirk introduces a required branch outside the admitted domain.
- Monster composition/activation conflicts with accepted cache or modal ownership,
  or repeatable input/presentation failure can duplicate turns or rewards.
- Save compatibility cannot preserve new authoritative values, or partial-state,
  Run, recovery or normal-start acceptance is requested.
- Evidence supports combining/splitting acceptance contracts or maintainer
  priorities change.

Retain approximately three completed milestones as the ordinary broader review
cadence, with focused scope review before each specification. The three-milestone
horizon here follows the actor, fight and durable-completion boundaries; the
cadence is not a quota. Use the verified SHA and handoff gate in
[AGENTS.md](../AGENTS.md) for external planning/review. Roadmap approval, detailed
planning and implementation authorization remain separate.
