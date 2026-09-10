# MMModern - Roadmap

**Milestone 24 is the latest completed milestone.**
[Project status](project-status.md) owns current capabilities;
[project history](project-history.md) and the [closed M24 plan](milestone-24-plan.md)
own completed work. Dependency provenance belongs to [dependencies.md](dependencies.md).

## Current planning state

M24's catalog, player-facing inspection and four-category transfer are complete;
the [closed M24 plan](milestone-24-plan.md) owns their contracts and acceptance.
M25 is the immediate proposed next direction, still provisional and not
implementation-authorized. M24 completion does not accept or authorize M25.

The preferred direction remains bounded equipment management, followed by an
original encounter and its connected exploration needs. Completed M24 supplies
the inventory/catalog/transfer foundation; existing rewards, persistence and
scene presentation remain reusable foundations.

The implementation agent investigates and implements authorized work; the
architecture/specification agent prepares contracts; the independent reviewer
audits implementation and evidence; the maintainer approves direction and
separately authorizes work. Create a detailed plan only when its milestone is
about to begin.

## Recommended near horizon

### Proposed M25 - Bounded equipment management and existing-rule feedback

The active [M25 specification](milestone-25-plan.md) owns the investigated rules,
bounded adaptation decisions and proposed acceptance/stage contracts. It is a
planning candidate for architecture review, not implementation authorization.

**Outcome and boundary.** Add player-triggered equip and unequip/remove for weapons,
armor and accessories with original class restrictions, category/slot/type and
cross-category conflicts, bounded ring/medal count rules and cursed-item removal
restrictions. Show equipped status and meaningful feedback through the attributes,
max-HP and max-SP effects already modeled by `XeenCharacterRules`. This does not
establish complete original equipment effects, weapon damage, armor-class or
combat statistics. Generic character-to-character transfer belongs to M24.

**Foundation and prerequisites.** M24 supplies the chosen UI/catalog and generic
four-category transfer path. Item frame/state bytes, character classes, roster
ownership and save storage already exist. The pinned reference's
[item rules](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp)
and [item interaction](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp)
provide the equipment-legality oracle. Keep authoritative legality with
character/item rules and presentation with the existing application owners.

**Original anchor.** Initial active roster owner 11 has weapon records
`{0,12,0,1}` and `{0,12,0,0}`: an equipped and an unequipped dagger. This supplies
an original conflict/refusal and unequip/equip checkpoint without inventing loot.
The complete category/class/curse acceptance selection remains provisional;
synthetic fixtures must discriminate modeled modifier behavior when initial
items do not provide the necessary contrast.

**State and acceptance.** Equipment changes appear representable by existing v2
item bytes. Preserve compaction and alias semantics, opaque fields and current
HP/SP according to the established rules; do not silently heal or normalize
loaded state. Settle any newly required derived rules before extending scope.
Acceptance should combine an independent restriction oracle, successful actions
and no-effect refusals, original loadout interaction, save/restart and physical
UI validation. New durable statistics would require an explicit compatibility
decision and horizon review.

**Confidence and sequencing.** Medium confidence: the original state is concrete,
but category restrictions and exceptional equipment need bounded verification.
This follows generic item inspection/transfer because equipment adds a distinct
legality contract. Exclude item activation, combat, spell systems, shops, random
loot generation and full stat simulation. Unexpected special-item effects are a
scope-growth trigger, not implicit authorization to implement them all.

### Dependencies and the end of this horizon

Completed M24 supplies M25's shared player-facing inspection, selection and generic
transfer foundation; the proposed equipment-rule investigation is a separate
contract. M25 requires no indoor animation, wall items, world clock or combat.
Existing item ownership and persistence are foundations, not new prerequisites.

The remaining entry keeps equipment legality distinct from completed generic
four-category transfer. Together they support management of existing rewards and
loadouts; they do not make potions usable, certify a quest route or establish a
playable region.

The numbered horizon ends here because a credible next encounter needs a bounded
actor, statistics, turn, outcome and persistence contract that this investigation
has not yet established. Do not fill that gap with an unverified combat milestone
or indefinitely defer connected gameplay with more isolated presentation work.
Equipment UI is a sequencing preference, not a correctness prerequisite for a
fixed-loadout combat foundation.

## Connected gameplay objective and present blockers

The Myra -> Phirna -> Myra loop is the first existing original quest sequence to
reassess. Current terrain movement connects the two coordinates within map 23,
and the complete exchange and restart semantics already exist. However, the map
contains 19 active original monster records, including records near the geometric
path. MMModern does not simulate them. The pinned
[combat movement](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp)
requires more than checking whether a monster initially occupies a path cell.
A terrain-only path is not normal-route acceptance.

The original party resource starts at map 28, (18,4), west. Existing indoor
movement is bounded to local coordinates 0..15; connected indoor maps and the
Vertigo exit's unsupported event operations also prevent treating this as a
working normal start. The application's diagnostic default is map 1, (9,6),
south. These are different claims; see [startup](../src/main.cpp),
[movement](../src/games/xeen/XeenMovement.cpp) and
[world state](../src/games/xeen/XeenWorld.cpp).

After the proposed horizon, prioritize an original encounter and navigation
envelope. Promote a faithful safe route if verified; otherwise promote the
specific actor/combat or connected-indoor prerequisite that the selected route
actually needs. Keep diagnostic coordinates, controlled interaction sequences,
normally traversable routes and generally playable regions distinct. Do not
ignore monsters, hazards or unsupported script tails to claim integration.

## Provisional direction beyond the horizon

- **Encounters and combat.** Enable meaningful original threats and outcomes.
  Usable loadouts and existing condition/item persistence help, but the current
  model lacks substantial original offensive/defensive statistics, actor state,
  turns and reward rules. Promote a foundation only after tracing a complete
  original encounter, including movement/aggression, retaliation, death/escape,
  rewards and save boundaries. A monster sprite or MOB record is insufficient.
- **Recovery, item effects and services.** Make acquired resources consumable and
  progression repeatable. Antidote activation is promising, but the reference's
  [spell path](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/spells.cpp)
  includes target selection, charge/cancellation ordering, HP/condition effects
  and monster-turn consequences. Verify an original condition-producing path
  before promoting it. Shops and other services additionally need their actual
  currency, stock, eligibility and time contracts.
- **Connected original Clouds exploration.** Grow from an accepted encounter to
  a bounded quest/exploration loop with save/resume. Investigate connected indoor
  coordinates, special doors, party-dependent terrain, mutation, darkness and
  time only where the chosen route requires them. Generic event decoding does
  not establish execution support; new authoritative state needs a deliberate
  save-version policy. Expand toward towns/regions after complete loops pass.
- **Presentation and audio.** Ordinary indoor animation can reuse established
  visual timing, while wall items have a separate placement/facing contract.
  Scripted animation and alternate appearances are different concerns. Promote
  these or audio when original encounters or acceptance expose a concrete need;
  keep cosmetic phase separate from gameplay time.
- **World of Xeen and modernization.** Broader Clouds fidelity should establish
  reusable behavior before Darkside and cross-side progression are promised.
  Existing side-aware identities and archive access are necessary foundations,
  not proof of Darkside gameplay. Promote that expansion after verified scripts,
  state domains, resources and cross-side saves support representative loops.
  Modern usability should expose faithful rules clearly. Portability and
  localization remain separate future decisions, supported by resource-driven
  text and dependency boundaries rather than an engine rewrite.

The likely next integration objective is one complete original encounter with
navigation and save/resume, followed conditionally by the Myra loop or a better
substantiated route. Neither objective is guaranteed by M24-M25.

## Alternatives, replanning and review cadence

**Strongest alternative: combat first.** It addresses the largest obstacle to
normal exploration sooner. It is not preferred yet because no sufficiently
bounded original actor/rules/outcome contract has been established, whereas
existing rewards and loadouts support immediate player agency. A verified small
encounter or a maintainer priority for exploration can reverse this order;
equipment UI must not be presented as a mandatory combat dependency.

Indoor animation is independently plausible and relatively well grounded after
M22/M23, but offers less new agency. Wall items and broad event mutation have
less complete acceptance evidence. A map-23 bottle at (14,4) already completes
its original WhoWill/reward/acknowledgment/removal path through the current event
system; missing SDL/route/save certification there is an acceptance gap, not a
reason to implement another reward subsystem.

Replan when evidence changes the contracts:

- New catalog requirements exceed the accepted 24A schema or resource boundary;
  review that change explicitly before expanding the foundation.
- Transfer/equipment exposes missing authoritative state, special effects or a
  save/restore incompatibility instead of a bounded change to existing records.
- Encounter or route tracing establishes combat, world flags, time, doors,
  connected maps or another missing owner as an actual prerequisite.
- A faithful safe route becomes available without combat, or the preferred loop
  fails because of a specific movement/event boundary.
- Two entries share one coherent acceptance contract, an entry contains separable
  contracts, or maintainer priorities change.

Before authorizing M25 implementation, verify its bounded specification against
the completed M24 foundation. Refresh the horizon after proposed M25, or earlier
on a trigger.
Retain approximately three completed milestones as the ordinary broader review cadence;
it is not a quota for horizon length. Use the verified SHA and handoff gate in
[AGENTS.md](../AGENTS.md) for external review. Roadmap recommendation, direction
approval, detailed planning and implementation authorization remain separate.
