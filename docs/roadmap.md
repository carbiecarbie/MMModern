# MMModern - Roadmap

**Milestone 23 remains the latest fully completed milestone; M24A is an accepted
stable catalog foundation.** M24 remains incomplete.
[Project status](project-status.md) owns current capabilities;
[project history](project-history.md) and the [closed M23 plan](milestone-23-plan.md)
own completed work. Dependency provenance belongs to [dependencies.md](dependencies.md).

## Current planning state

M24 was promoted and split into independently reviewable stages in the
[active M24 plan](milestone-24-plan.md). M24A's bounded catalog foundation is
accepted. M24B player-facing inspection and four-category transfer remain the
next stage, pending separate authorization and implementation. Acceptance of 24A
does not automatically authorize 24B. M25 remains provisional.

The preferred direction is to make existing party and item state useful to the
player, then bound an original encounter and its connected exploration needs.
[M21](milestone-21-plan.md) already established real item rewards and restart
persistence; [M22](milestone-22-plan.md) and M23 established substantial scene
presentation. Another storage framework or an automatic succession of visual
increments would miss the present opportunity for player agency.

The implementation agent investigates and implements authorized work; the
architecture/specification agent prepares contracts; the independent reviewer
audits implementation and evidence; the maintainer approves direction and
separately authorizes work. Create a detailed plan only when its milestone is
about to begin.

## Recommended near horizon

### M24B next - Usable party/item inspection and character-to-character transfer

**Outcome and boundary.** Show active characters' modeled condition and HP/SP,
inspect weapons, armor, accessories and miscellaneous items with bounded,
resource-driven names/descriptions, and transfer items in any of those four
categories between eligible active roster owners. Provide character/category/item
selection and original transfer restrictions with clear refusal feedback.
Ownership transfer is distinct from player-triggered equip/unequip.

**Original anchor.** The accepted controlled Myra exchange at Clouds map 23,
(9,11), west, yields five miscellaneous records `{10,37,1,0}` after the Phirna
Root/Q2 sequence. The pinned reference identifies these as charged antidote
potions. The new acceptance delta is to identify the resulting items, transfer
one from roster owner 0 to owner 18, save and actually restart with the ownership
and bytes preserved. This builds on the [M21 exchange contract](milestone-21-plan.md);
it does not recertify the route or require new reward semantics.
Myra is a primary acceptance anchor, not the definition of the transfer boundary.
The [M24 original anchors](milestone-24-plan.md#original-equipment-controls)
specify additional equipment-category transfer controls and the equipped-state
consequence, without adding player-triggered equip/unequip actions.

**Foundation and decisions.** Reuse [party ownership](../src/games/xeen/XeenParty.h),
[character/item records](../src/games/xeen/XeenCharacter.h),
[character rules](../src/games/xeen/XeenCharacterRules.cpp), the existing
[application flow](../src/app/XeenEventFlow.cpp) and
[save owner](../src/games/xeen/XeenSaveState.cpp). Selection is transient; roster
identity, active aliases, tail-slot capacity and compaction must remain coherent.
Item transfer has its own original restrictions; reward-recipient eligibility is
not automatically its policy.

The pinned reference's ordinary
[item dialog](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp)
uses category-generic transfer when changing active character with an item
selected: check cursed state and destination category capacity, move the item,
reset the destination item's equipped frame, then sort/compact source and
destination. The [M24 transfer contract](milestone-24-plan.md#transfer-rules-and-publication)
bounds those semantics, including active aliases and stable compaction. Preserve
exact item bytes except for deliberate original transfer semantics; resetting
the frame during transfer does not authorize an
equip/unequip command.

Accepted M24A supplies the reusable bounded read-only catalog. English names are
embedded at build time from the exact pinned ScummVM `CONSTANTS_7` Git blob;
optional commercial material names remain external in `DARK.CC/mae.xen`, read
through existing archive owners. Catalog generation compiles no ScummVM headers
and introduces no runtime `mm.dat` dependency. Provenance, schema, bounds and
fallbacks are settled in the [catalog contract](milestone-24-plan.md#catalog-source-and-delivery-decision)
and [dependencies](dependencies.md#build-generated-english-item-catalog).
Player presentation and transfer remain future 24B work.

**State and acceptance.** Existing save v2 represents the expected durable changes;
no new state category is presently identified. Implement and validate resource
compatibility, opaque-byte preservation and the idle save boundary according to
the [M24 persistence contract](milestone-24-plan.md#persistence-and-compatibility).
Automated checks should distinguish transfer from copying across all four
categories, cover capacity, cursed-item refusal, aliases, cancellation, stable
source/destination compaction and the equipped-frame reset, and use an independent
original-rule oracle. Original reward and equipment-category controls, a real
process restart and maintainer-operated UI acceptance must establish the visible
result and resulting ownership.

**Confidence and sequencing.** High confidence in state reuse; medium overall
because UI and transfer integration remain unimplemented. Inspection plus one
ownership action is a coherent first increment across the existing categories.
Exclude equip/unequip
actions, item use/activation, discard, shops, identification/repair, spells, quest
journal, recruitment, combat, full character statistics and normal-route
certification. The independent catalog contract is already accepted as 24A;
24B must deliver the player interaction and its connected acceptance together.

### Proposed M25 - Bounded equipment management and existing-rule feedback

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

M24 -> M25 is the proposed hard dependency for shared player-facing inspection,
selection and generic transfer; the equipment-rule investigation itself can
proceed independently.
Neither entry requires indoor animation, wall items, a world clock or combat.
Existing item ownership and persistence are foundations, not new prerequisites.

Two entries fit the present evidence: generic four-category ownership transfer
and equipment legality have distinct, reviewable acceptance contracts. Together they make
existing rewards and loadouts usable for management. They do not make potions
usable, certify a quest route or establish a playable region.

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

Before authorizing 24B, verify its existing specification against the stable 24A
foundation. Refresh the horizon after M24 and proposed M25, or earlier on a trigger.
Retain approximately three completed milestones as the ordinary broader review cadence;
it is not a quota for horizon length. Use the verified SHA and handoff gate in
[AGENTS.md](../AGENTS.md) for external review. Roadmap recommendation, direction
approval, detailed planning and implementation authorization remain separate.
