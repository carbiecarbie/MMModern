# MMModern - Roadmap

**Milestone 39 is the latest completed milestone.**
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
two-region 8/8 continuation ([M37](milestone-37-plan.md)). M38 completed
bounded Ironworks Armor Repair, atomic carried-gold/item publication, one-day
departure and 8/9 continuation ([M38](milestone-38-plan.md)). M39 completed
combat-owned Magic Arrow, First Aid and Awaken, bounded native feedback and
exact 8/10 continuation with legacy isolation ([M39](milestone-39-plan.md)).
These contracts do not certify unrestricted map-23
or Clouds travel or normal original startup.

The approved M33-M35 arc is complete within its bounded map-23 mainland scope.
After the maintainer-reviewed post-M35 reassessment, the accepted direction is
to pause quest-driven vertical slices and develop reusable Clouds systems with
Vertigo progressively serving as their production hub. A bounded learned
exploration-casting step was completed first using original learned spells on
the admitted mainland. Vertigo admission and its first bounded town service
now complete that short arc.

## Near term

The approved **M36-M38 short arc is completed and accepted**: learned
exploration casting, bounded Vertigo travel, and Ironworks Armor Repair.
The repair slice gives earned gold a use without merchant stock or trading.

The maintainer accepted the following three-milestone near-term arc:

**M39 - Bounded already-learned combat casting -> M40 - Bounded service-day
continuation -> M41 - Vertigo Training and progression.**

These are accepted roadmap planning commitments, not milestone specifications
or implementation authorization. After each closure, the default is to plan
the next named milestone; a new broad reassessment is not required unless the
triggers below are met.

M39 is the **completed and accepted first milestone** of this arc. Its
[closed contract](milestone-39-plan.md) establishes useful finite-SP combat
consumers and their consequences on existing routes before M41 connects
progression and the original training refill. Exploration recovery remains
unchanged; no new service time or route was needed for acceptance.

**M40 is the immediate next roadmap unit; its planning/specification requires
separate authorization.** M41 remains the following accepted commitment.
No concrete replanning trigger was identified at M39 closure, so the accepted
sequence, rationale and post-M41 broad-review cadence remain unchanged.

### M40 - Bounded service-day continuation

Remove the day-10 obstacle to repeated admitted Ironworks visits by supporting
the mandatory consequences of daytime, script-driven service-day advances
within the current year. This is the prerequisite for M41's useful repeated
Training/repair loop, not a general calendar framework. Keep ordinary movement
and combat time admission bounded; exclude overnight adventuring, rest,
year rollover/aging and new service routes. Reserve any required departure
before admitting an operation that could exceed the supported date range.

The smallest useful production boundary is real Ironworks repair and subsequent
adventuring across day 11, a later visit, quiet save, process restart and further
repair/play. Include zero-transaction departures, but do not accept empty visits
alone as the gameplay witness. Date, retained actors/items, generated stock,
bank balances and subsequent world RNG must continue exactly, with no duplicate
departure or regeneration on failure/retry, revisit or restore. Require native
SDL acceptance alongside deterministic and process evidence. M38's route and
guarded publication/Flow ownership are the starting point; M39 remains usable
through the expanded service-day domain.

The minimum coherent foundation includes party-owned merchant wares and bank
gold/gems, explicit fresh-domain initialization, stock regeneration, interest,
and guarded publication with date and world-owned RNG. Reference regeneration
replaces the fixed wares for both sides' four shops, generating items even when
a category is already full. The reached stock-generation path covers item
levels 1-6, materials/enchantments and charges; it is substantially broader than
the existing bounded monster-drop generator. Preserve the complete reached
draw sequence, including the other shops and the second side's numeric tables,
without admitting Darkside gameplay. Reuse suitable checked item/RNG helpers
without changing legacy monster-drop semantics or adding a merchant RNG owner.

Original bank balances are zero in the inspected initial resources. Interest
still belongs to the operation and must preserve durable balances; bank UI,
deposits and withdrawals are unnecessary. New-game stock generation and later
regeneration are explicit lifecycle operations, never lazy menu or restore
work. Generated offers can remain unavailable to the player: exclude Buy,
stock depletion, Sell, new equipment effects and a general transaction/service
registry. This leaves a concrete foundation for later trading without making
trading a prerequisite for faithful service time.

Schema 8 has neither merchant wares nor bank balances, so M40 requires a new
persistence representation and explicit content admission. Specify fresh
initialization and its RNG position for the prepared Regional Journey domain;
do not pretend its day-8 start replays normal startup or silently populate old
saves. Existing domains, including 8/9 and 8/10, retain their original support limits.

The decisive time contract is reference `Party::addTime`: a day-changing call
regenerates stock and applies bank interest when its resulting day modulo ten
is one, or when the individual charge exceeds 1440 minutes. Preserve calls,
not just the final date. In the admitted daytime script-service path, the mode
suppresses the ordinary daily reset/Weak handling and clears the pending
new-day marker; this is not a replay of movement's condition ticks, actor
turns or `changeTime`. Prove that boundary in the specification. M40 must
support the separate one-day operations M41 needs; Temple's two-day operation
and other temporal modes remain later admissions.

Confidence is medium-high that this is an independent milestone. It separates
the shared stock/RNG/persistence change from Training's route, UI and character
publication, so M41 can use the same checked service-time consequence path
without revisiting a deliberately restricted first Training implementation.

## Medium term

### M41 - Vertigo Training and progression

Connect legitimately accumulated XP and carried gold to the original Vertigo
training service, permanent levels and existing derived HP/SP/combat rules.
Training is the preferred progression consumer: it uses both resources and
returns the trained member to full HP/SP, giving M39's spell users a connected
benefit. It follows M40 so repairs, different trainees and later visits need
not compete for the two days before the old stock boundary.

Admit the original training doorway/service route, its meaningful events,
additional Slime influence and resource-driven service presentation. The
examined route is a small extension of Vertigo admission, but full dynamic
actor closure and retained-city/revisit consequences must be proved during
specification; the existing static route evidence is not that certification.
Preserve party/roster progression ownership and Flow/modal/save authority.
Use M40's service-time operations and the existing purse, XP, character rules,
retained actors and M39 combat consumers rather than parallel progression or
payment systems.

The smallest useful acceptance boundary is earning the missing XP through
admitted play, reaching Training, paying for a level, observing derived stats
and the original refill, and using the result in further combat/casting after
quiet save and process restart. Also establish different-member training and
return visits across a stock boundary as connected service continuity, with
eligibility/refusal and departure behavior. Respect Vertigo's original level
cap and supported character/effect bounds; no synthetic grant of XP/gold is a
substitute for the production witness. Native SDL, deterministic and process
evidence remain distinct requirements.

The later specification must preserve class/XP eligibility, gold cost and XP
consumption, a day for each distinct newly trained member in a visit, and a
separate day on departure even without training. Additional levels for the
same member in that visit do not add another training day. Successful training
resets party temporary state and refills the selected member's HP/SP; it is
not general condition healing or free rest. Validate the complete reset inputs,
including currently unmodeled temporary resistances, as zero or explicitly
represented within the admitted domain rather than silently discarding them.

Levels, XP, HP/SP and modeled temporary state already fit schema 8; M41 is
expected to reuse M40's expanded representation with new content semantics.
Transient visit bookkeeping need not survive a quiet save. Recheck this during
specification if route consequences or reset inputs introduce durable state.
Exclude other trainers, class changes, skill purchases, guild/Temple work,
general replenishment, new quests and unrestricted city travel. This completes
a connected combat -> repeated services -> progression arc, leaving useful gold
expenditure and durable stock as foundations for a subsequent trading or spell
acquisition decision. Confidence in M41's position is medium; route closure
and the end-to-end earned-XP witness are its main remaining specification risks.

### Why service continuation precedes Training and Sell

A faithful first Training visit is possible under the old boundary: from day 8,
one distinct member can train on day 9 and depart on day 10. It can produce a
real level/refill and could later be extended without replacing its owners.
But one earlier smith departure already prevents that successful train-and-exit
sequence without day-11 consequences. Two distinct trainees also cross that
boundary. Such a slice would leave the intended repeatable progression loop
unavailable and force an immediate service-time follow-up. Combining all of
M40 with Training instead would couple stock generation and persistence to a
new route, modal UI and progression publication. The independent Ironworks
consumer makes the split preferable.

Sell is the strongest small alternative: it reuses the M38 route, modal work,
purse/item guards and removes carried items without adding them to stock.
However, the prepared party's carried gold already funds early Training;
earned XP and service continuity are the more immediate constraints. Sell
alone adds gold while preserving the repeated-visit limit. Advance it only if
real play establishes inventory congestion or a funding shortage that blocks
the chosen progression witness. Fewest changed lines does not by itself make
it the better next system.

### After M41 - provisional consumers

Review the next short arc after M41. Keep the deferred boundaries distinct:

- **Sell** is the strongest small economy candidate: reuse the admitted
  Ironworks route and publication guards to remove/compact carried items and
  credit gold, without merchant stock. Admit pricing, Merchant-skill inputs and
  equipment consequences explicitly. Prefer it when surplus loot or funding a
  useful gold consumer warrants it. Broader repair needs an actual unsupported
  damaged-item consumer; paid identification buys details, not a persistent
  identified bit. Neither is the automatic successor to repair.
- **Buy** would consume M40's generated, durable stock but still requires
  pricing, stock depletion, capacity/equipment eligibility and acceptance of
  purchased-item effects. Generation alone does not authorize those effects or
  establish a trading UI. Keep Buy separate from Sell unless a later consumer
  makes a combined scope independently acceptable.
- **Guild acquisition** should follow useful supported spell consumers. Books
  and payment owners are reusable; membership purchase/award persistence,
  class/town offers and the route with additional Slime influence are not yet
  admitted. Teaching First Aid/Awaken to other eligible members already has a
  consumer, but need not precede using already-known Magic Arrow in combat.
  Membership needs representation beyond schema 8. Departure costs one day
  even without a purchase.
- **Temple recovery** should follow a coherent service-time boundary. Original
  Heal includes temporary-stat/resistance clearing and resurrection costs; it
  does not refill SP. Successful Heal or Uncurse makes departure a single
  two-day charge, triggering stock regeneration/bank interest even from day 8.
  Admit the northward route and separate healing/resurrection, uncurse and
  donations rather than bundling a complete temple by default.

These are pinned-reference interpretations and planning conclusions, not
independent DOS observations. The reference pin belongs to
[dependencies](dependencies.md#pinned-scummvm-revision); M38's narrower accepted
[stock boundary](milestone-38-plan.md#stock-boundary-and-direct-repair-menu)
remains authoritative for implemented behavior.

The current day-10 entry refusal remains an MMModern support limit, not an
original opening rule. It remains outside completed M39 and is deliberately resolved
by M40 for the selected service loop. Do not combine Training's separate day
charges or split Temple's two-day charge: the reference consequences differ.

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
two-region transitions and 8/8 continuation, M38's bounded repair/departure
publications and explicit 8/9 pair, M39's combat-owned casting and 8/10
continuation, and M28's
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
- The planned units cannot be independently accepted despite a focused scope
  adjustment.
- Another original area or system demonstrates materially better leverage for
  a required foundation, or maintainer priorities explicitly change.

Ordinary implementation difficulty alone does not reopen the direction.
Use focused scope review before each specification, normally planning M40 after
M39 and M41 after M40. The next broad roadmap review follows M41, preserving the
approximately three-completed-milestone cadence. Replan earlier on concrete
evidence that:

- M40's reached stock/time path requires player access to generated items,
  other temporal modes or state outside the bounded same-year service domain;
  or repeated actual repair cannot independently demonstrate its value.
- M41's original route requires substantially wider actor/event/effect coverage,
  its reset inputs cannot be represented within the expected domain, or earned
  XP, survivability or replenishment prevents the connected progression witness.
- Inventory congestion or lack of gold makes Sell a demonstrated prerequisite,
  or a concrete recovery/acquisition prerequisite outranks the planned consumer.

Resolve these through the smallest targeted investigation or scope adjustment;
ordinary implementation detail is not a reason to repeat the candidate survey.
The current service-route evidence is not complete dynamic actor-closure
certification; require that proof when admitting the Training route.
Roadmap approval, milestone specification and
implementation authorization remain separate; completing one unit does not
authorize the next.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
