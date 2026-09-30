# MMModern - Roadmap

**M41 is the latest completed milestone.**
[Project status](project-status.md) owns implemented capabilities and acceptance
boundaries; [project history](project-history.md) owns completed chronology.
Reference provenance belongs to [dependencies](dependencies.md).

## Current planning state

<a id="m28---durable-bounded-encounter-completion-and-revisit"></a>

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
M40 completed bounded same-year service-day continuation, complete merchant
wares/shared bank state, stock regeneration/interest and exact 9/11 continuation
while retaining M39 casting ([M40](milestone-40-plan.md)).
M41 completed bounded original Training, permanent levels from earned XP/carried
gold, active temporary reset and selected refill, distinct-member service days,
expanded route/actor admission and exact 9/12 continuation
([M41](milestone-41-plan.md)).
These contracts do not certify unrestricted map-23
or Clouds travel or normal original startup.

<a id="approved-m33-m35-arc"></a>

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

The approved **M39-M41 short arc is completed and accepted**:

**M39 - Bounded already-learned combat casting -> M40 - Bounded service-day
continuation -> M41 - Vertigo Training and progression.**

M39's [closed contract](milestone-39-plan.md) established useful finite-SP combat
consumers. M40's [closed contract](milestone-40-plan.md) established complete
party-owned merchant wares/shared bank state, repeated same-year service days
and exact stock/interest/world-RNG continuation. These foundations enabled M41
without a parallel progression, payment or service-time system.

### M41 - Vertigo Training and progression

M41 is the **completed and accepted final milestone** of this arc. Its
[closed contract](milestone-41-plan.md) connects legitimately earned XP and
carried gold to original Training, permanent levels, active temporary reset and
selected-member HP/SP refill. The bounded twenty-eight-cell Vertigo route and
initial/reset actor closures, separate distinct-member Training days and
departure, stock-boundary consequences and exact envelope-v4/schema-9/content-12
restart passed acceptance. Automated testing, original-resource/process
continuation and independent technical review passed; separately, maintainer
physical native-SDL acceptance passed, including responsive input and continued
combat/casting after progression.

The next required activity is the scheduled **post-M41 broad roadmap
reassessment**, as a separate task. It must review the next short arc against
the accepted M41 state and maintainer priorities. No M42 or other implementation
successor is selected or authorized here. Sell, Buy, Guild and Temple remain
provisional candidates below; closure does not conduct that review.

## Medium term

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

### Post-M41 reassessment - provisional consumers

The scheduled broad review is now due. These candidates remain provisional until
that separate review selects and approves a direction. Keep their boundaries distinct:

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
  class/town offers and Guild-specific route/actor closure are not yet admitted.
  Teaching First Aid/Awaken to other eligible members already has a
  consumer, but need not precede using already-known Magic Arrow in combat.
  Membership is not represented in current schema 9. Departure costs one day
  even without a purchase.
- **Temple recovery** should follow a coherent service-time boundary. Original
  Heal includes temporary-stat/resistance clearing and resurrection costs; it
  does not refill SP. Successful Heal or Uncurse makes departure a single
  two-day charge, triggering stock regeneration/bank interest even from day 8.
  Admit the northward route and separate healing/resurrection, uncurse and
  donations rather than bundling a complete temple by default.

These are pinned-reference interpretations and planning conclusions, not
independent DOS observations. The reference pin belongs to
[dependencies](dependencies.md#pinned-scummvm-revision). The implemented stock/time
contract belongs to [M40](milestone-40-plan.md); M38's narrower accepted
[stock boundary](milestone-38-plan.md#stock-boundary-and-direct-repair-menu)
remains authoritative for legacy contents 9/10.

The legacy content-9/10 day-10 entry refusal is an MMModern support limit, not an
original opening rule. M40 resolves it in content 11, inherited by fresh content
12, for the selected same-year service loop. Do not combine Training's separate day
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
continuation, M40's party-owned economy/atomic service-day departure and 9/11
continuation, M41's existing-owner progression, chained day reservations,
expanded city admission and exact 9/12 continuation, and M28's owner/preimage
and presentation safeguards. Cosmetic input replacement retains acquired old
concrete authority until successor handoff, with exact-origin stale-input safety.
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
The scheduled post-M41 broad reassessment is now due, preserving the
approximately three-completed-milestone cadence. The M39-M41 arc met its bounded
acceptance requirements, including complete dynamic actor closure and the earned
progression witness; those are closed contracts, not outstanding planning risks.

The separate review should retain the established triggers for targeted
replanning: stock/time requirements outside the accepted same-year domain;
materially wider actor/Event/effect or persistence requirements; and demonstrated
inventory, funding, survival or acquisition prerequisites. Resolve concrete
contradictions through the smallest necessary investigation. Completion alone
does not promote a provisional consumer into an approved milestone.

Roadmap approval, milestone specification and implementation authorization
remain separate; completing the arc does not authorize its successor.
Use the verified SHA and handoff gate in [AGENTS.md](../AGENTS.md) for external
planning and review.
