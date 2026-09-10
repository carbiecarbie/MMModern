# MMModern - Roadmap

**Milestone 25 is the latest completed milestone.**
[Project status](project-status.md) owns current capabilities;
[project history](project-history.md), the [closed M25 plan](milestone-25-plan.md)
and earlier closed plans own completed work. Dependency provenance belongs to
[dependencies.md](dependencies.md).

## Current planning state

M25's bounded equipment management is complete. The existing inventory panel now
supports inspection, M24 character-to-character transfer, and contextual equip or
remove for the accepted Clouds domains. Equipment legality, transient action
safety, modeled-stat feedback and existing save/restart are available foundations;
they do not establish complete item effects, combat, or a playable route.

No next milestone is specified or implementation-authorized. The plausible next
direction remains investigation of one original encounter and the connected
navigation envelope required to reach, resolve and persist it. Detailed planning
must wait for evidence that identifies a bounded actor, statistics, turn, outcome
and persistence contract.

The implementation agent investigates and implements authorized work; the
architecture/specification agent prepares contracts; the independent reviewer
audits implementation and evidence; the maintainer approves direction and
separately authorizes work. Create a detailed plan only when its milestone is
about to begin.

## Connected gameplay objective and present blockers

The Myra -> Phirna -> Myra loop is the first existing original quest sequence to
reassess. Current terrain movement connects the two coordinates within map 23,
and the exchange, inventory, equipment and restart semantics exist. The map also
contains 19 active original monster records, including records near the geometric
path, which MMModern does not simulate. The pinned
[combat movement](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp)
requires more than checking whether a monster initially occupies a path cell. A
terrain-only path is not normal-route acceptance.

The original party resource starts at map 28, `(18,4)`, west. Existing indoor
movement is bounded to local coordinates 0..15; connected indoor maps and the
Vertigo exit's unsupported event operations also prevent treating this as a
working normal start. The application's diagnostic default is map 1, `(9,6)`,
south. These are different claims; see [startup](../src/main.cpp),
[movement](../src/games/xeen/XeenMovement.cpp) and
[world state](../src/games/xeen/XeenWorld.cpp).

Investigate a complete original encounter and navigation envelope. Promote a
faithful safe route if verified; otherwise identify the specific actor/combat or
connected-indoor prerequisite the route actually needs. Keep diagnostic
coordinates, controlled interaction sequences, normally traversable routes and
generally playable regions distinct. Do not ignore monsters, hazards or
unsupported script tails to claim integration.

## Provisional future directions

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
  before promoting it. Shops and other services need their actual currency,
  stock, eligibility and time contracts.
- **Connected original Clouds exploration.** Grow from an accepted encounter to
  a bounded quest/exploration loop with save/resume. Investigate connected indoor
  coordinates, special doors, party-dependent terrain, mutation, darkness and
  time only where the chosen route requires them. Generic event decoding does
  not establish execution support; new authoritative state needs a deliberate
  save-version policy.
- **Presentation and audio.** Ordinary indoor animation can reuse established
  visual timing, while wall items have a separate placement/facing contract.
  Scripted animation and alternate appearances are different concerns. Promote
  these or audio when original encounters or acceptance expose a concrete need;
  keep cosmetic phase separate from gameplay time.
- **World of Xeen and modernization.** Broader Clouds fidelity should establish
  reusable behavior before Darkside and cross-side progression are promised.
  Existing side-aware identities and archive access are necessary foundations,
  not proof of Darkside gameplay. Portability and localization remain separate
  decisions supported by resource-driven text and dependency boundaries.

The likely next integration objective is one complete original encounter with
navigation and save/resume, followed conditionally by the Myra loop or a better
substantiated route. The evidence may instead select recovery/item use, connected
indoor behavior or another concrete prerequisite.

## Alternatives, replanning and review cadence

Combat-first investigation addresses the largest obstacle to normal exploration,
but no sufficiently bounded original actor/rules/outcome contract has yet been
established. M25 equipment is useful player agency and a reusable foundation; it
is not a mandatory correctness prerequisite for a fixed-loadout combat slice.

Indoor animation remains independently plausible after M22/M23, but offers less
new agency. Wall items and broad event mutation have less complete acceptance
evidence. A map-23 bottle at `(14,4)` already completes its original
WhoWill/reward/acknowledgment/removal path through the current event system;
missing SDL/route/save certification there is an acceptance gap, not a reason to
implement another reward subsystem.

Replan when evidence changes the contracts:

- Encounter or route tracing establishes combat, world flags, time, doors,
  connected maps or another missing owner as an actual prerequisite.
- A faithful safe route becomes available without combat, or the preferred loop
  fails because of a specific movement/event boundary.
- Recovery or equipment use requires authoritative state, special effects or a
  save/restore change beyond existing item records.
- Indoor animation, wall items or presentation become necessary to identify or
  operate the selected encounter.
- Two directions share one coherent acceptance contract, a direction contains
  separable contracts, or maintainer priorities change.

Retain approximately three completed milestones as the ordinary broader review
cadence; it is not a quota for horizon length. Use the verified SHA and handoff
gate in [AGENTS.md](../AGENTS.md) for external review. Roadmap recommendation,
direction approval, detailed planning and implementation authorization remain
separate.
