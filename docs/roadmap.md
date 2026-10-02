# MMModern - Roadmap

**M43 is the latest completed and accepted milestone.**
[Project status](project-status.md) owns implemented capabilities and acceptance
boundaries; [project history](project-history.md) owns completed chronology.
Reference provenance belongs to [dependencies](dependencies.md).

## Current planning state

<a id="m28---durable-bounded-encounter-completion-and-revisit"></a>

The encounter/Journey line progressed from original actor approach and bounded
combat through completion/revisit ([M26](milestone-26-plan.md),
[M27](milestone-27-plan.md), [M28](milestone-28-plan.md)), mutable continuity and
the Bone Whistle expedition/collection ([M29](milestone-29-plan.md),
[M30](milestone-30-plan.md), [M31](milestone-31-plan.md)). The Regional Journey
then connected mainland navigation, combat/consequences, Run and Myra/Phirna
recovery ([M32](milestone-32-plan.md), [M33](milestone-33-plan.md),
[M34](milestone-34-plan.md), [M35](milestone-35-plan.md)).

<a id="approved-m33-m35-arc"></a>

The approved M33-M35 arc is complete. The subsequent approved M36-M38 arc added
learned exploration casting, bounded Vertigo entry/return and Armor Repair
([M36](milestone-36-plan.md), [M37](milestone-37-plan.md),
[M38](milestone-38-plan.md)). The approved **M39-M41 arc is complete and accepted**:
combat-owned learned casting, repeated service days with merchant/bank state,
and earned Training progression ([M39](milestone-39-plan.md),
[M40](milestone-40-plan.md), [M41](milestone-41-plan.md)).

The maintainer accepted the post-M41 roadmap direction. M42 equipment purchase
and M43 bounded Temple Heal/resurrection are completed and accepted. Their
durable boundaries and separate automated, independent-review and maintainer
acceptance belong in the [M42](milestone-42-plan.md) and
[M43](milestone-43-plan.md) closed plans; both are removed from future scope.

The prepared Journey covers the map-23 mainland and 49 Vertigo cells. It has
ordinary equipment purchase and selected paid recovery, including supported Dead
members with survivors, but Heal preserves SP. Normal original startup,
unrestricted services, recurring SP/food/rest and ordinary calendar processing
remain outside support. See [current boundaries](project-status.md#current-boundaries).

## Near term

### Provisional successor - test wider admission

The near-term objective is to improve the connected Journey's capacity to
venture further using established systems.

| Unit | Purpose and independently acceptable boundary | Dependency and confidence |
| --- | --- | --- |
| **Default provisional successor: connected Clouds content expansion** | Add a useful connected area with original encounters or an interaction, retained consequences, return to the established hub and exact restart. | Direction only; no M44/map/dungeon is selected or authorized. Reuse confidence is medium; a complete area boundary remains unproved. The broad review below must establish whether recovery, time, actor profiles and retained-region representation support the chosen witness. |

M43 establishes bounded HP/condition recovery and a certified approach; it does
not establish a sustainable SP loop. At the required review, prefer connected
content that composes existing systems. If repeatable SP/food/rest or ordinary
time processing is necessary for an honest exploration witness, select that
narrowly justified foundation first. This closure does not choose between them
or authorize another milestone.

### Deferred presentation work

The maintainer deferred the Temple's fully obscured static illustration: its
opaque text panel covers the art in `XeenSmithFlow.cpp`. Revisit the panel/art
composition with the broader service-character and scenery animation/art work.
This is a known presentation limitation, not an M43 acceptance prerequisite or
a reopened defect. Audio and animated service characters remain outside the
accepted M43 boundary; this item selects no successor milestone.

## Decision evidence and alternatives

Repository code/tests establish implemented behavior; the closed plans establish
accepted scope. Original-resource inspection confirms the service addresses,
not reachability or new gameplay acceptance. Reference interpretation uses only
ScummVM [6814ee9b](https://github.com/scummvm/scummvm/tree/6814ee9ba54582f5b5adcffab49efbbd8f589edd).
The post-M41 assessment below added no DOS observation or physical acceptance;
M42/M43 acceptance is recorded separately in their closed plans.

- **Buy:** [merchant generation](../src/games/xeen/XeenMerchantGeneration.cpp)
  and [initialization coverage](../tests/XeenServiceDayInitializationTests.cpp)
  establish real stock and original carried inventory.
  [Equipment](../src/games/xeen/XeenEquipment.cpp),
  [Journey contribution admission](../src/games/xeen/XeenJourneyRules.cpp) and
  [physical rules](../src/games/xeen/XeenCombatRules.cpp) distinguish useful plain
  equipment from unsupported modified records. Pinned
  [ItemsDialog::calcItemCost/doItemOptions](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp)
  establishes Buy price, capacity, unequipped transfer and stock removal.
  [Stock validation](../src/games/xeen/XeenServiceEconomy.cpp) now admits bounded
  purchase depletion only under contents 13/14, separately from strict generation.
  The [closed M42 contract](milestone-42-plan.md) owns that completed boundary.
- **Broader equipment/item use:** modified weapons/armor and miscellaneous
  effects would make more loot and stock useful. Physical item bytes, catalog,
  transfer/equipment and selected antidote consumption already exist; elemental
  attacks, weapon effectiveness, wider material contributions and item spells
  still require actual consumers. An independent boundary must acquire a real
  item, equip/use it, demonstrate its effect and retain charges/conditions or
  combat consequences through restart. Many effects can reuse current item
  storage, while newly consumed character/effect inputs may require schema work.
  Pursue effect families required by chosen content after the plain purchase
  loop, rather than make every generated record a prerequisite to the first shop.
- **Sell:** a useful independently acceptable unit would sell actual surplus
  loot, compact the owner's category, apply removal/equipment consequences,
  credit carried gold and spend it on an admitted consumer, with exact restart.
  It needs no stock addition or new route, but pricing uses the selected
  character's unmodeled Merchant-skill input. Cursed items and special weapons
  have original refusals. Clearing an equipped item changes derived consumers;
  purse overflow and atomic purse/item publication need explicit admission.
  New skill representation/persistence must be decided, not inferred from the
  four modeled maximum-stat skills. M41's accepted earned route reaches Training
  with 870 gold and leaves with 690; no inventory-relief or funding bottleneck
  is demonstrated there. Defer Sell unless purchases/recovery or wider loot
  make its funding/capacity value decisive. Buy-before-Sell is this evidence-backed
  choice, not a general ordering rule.
- **Guild acquisition:** Vertigo's original offers include supported Awaken,
  First Aid and Magic Arrow, so acquisition could expand useful learned consumers.
  However, class/town offer filtering, prices, original membership acquisition
  and per-character award state are additional work. Current learned books do
  not represent membership. The service at `(28,20,13)`, membership seller's
  complete Event path, actor closure and return are unadmitted. A valid boundary
  must acquire membership through supported play, buy a previously unknown useful
  spell, cast it and restart with award/book/payment exact. Existing books can
  hold learning; award representation likely needs a schema extension. Defer
  until repeatable SP supply or a needed exploration spell makes acquisition
  more valuable than the already-known consumers. Its original one-day departure
  remains owed even without buying a spell. See pinned
  [Character::guildMember](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp),
  [SpellsDialog::setSpellText](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_spells.cpp)
  and `LangConstants::CLOUDS_GUILD_SPELLS` at the same pin.
- **Temple versus Rest:** [M43's accepted recovery boundary](milestone-43-plan.md#selected-recovery-rules)
  has existing condition consumers and an explicit original service. General Rest/SP replenishment
  would solve a different bottleneck, but food, sleeping/rested state, ordinary
  dawn/daily processing and their condition consequences are not admitted.
  These can require new party state and schema coverage. Select them when
  practical continued play demonstrates the prerequisite; do not claim paid
  Heal completes recovery. The operation distinction follows pinned
  [BaseLocation::show/TempleLocation](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp),
  [Party::addTime](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp)
  and the [closed service-time contract](milestone-40-plan.md#service-time-and-admitted-domain).
- **Exploration, dungeons and reusable quests:** another outdoor area could reuse
  terrain/navigation, physical combat, rewards and existing Event operations;
  an indoor area could reuse checked geometry/object projection. As a concrete
  frontier, original map 23 connects north to map 22, whose geometry connects
  back to 23. Its two interaction chains require SetChar, GiveMulti and
  MakeNothingHere, outside the current Event decoder; geometry reuse alone does
  not make that original gameplay admissible. Neither expansion admits
  arbitrary monsters, map edges, doors/locks/traps, lighting, damage types or
  script operations. Current persistence has the mainland actor collection and
  an optional Vertigo collection, rather than a general collection of visited
  regions. A third retained region may need schema work. A useful expansion must
  enter from supported play, accomplish an original purpose and return with
  actors/flags/rewards exact, including initial/reset paths. Confidence in reuse
  is medium, in a complete area boundary low. It is the preferred subsequent
  proof of leverage, but no successor area has been certified. A quest alone
  is not the default successor; select it when it tests reusable semantics or
  supplies a necessary gameplay purpose.
- **Wider town gameplay:** extending Vertigo can reuse its retained region and
  transition/reset owners without adding a third region. Completing an original
  town objective with exploration/combat and useful return visits has more value
  than adding menus alone. It still requires full newly influencing actor
  profiles/closure, directional Event paths, object/door semantics and original
  quest/discovery consequences; M41 admits only one reachable Slime life per
  city form. Confidence in ownership reuse is higher than for a third region,
  but the wider gameplay boundary is unproved. Compare it with outdoor/indoor
  expansion at the next review instead of assuming Vertigo must remain central.
- **Normal startup/party preparation:** original level-1 party loading exists,
  while Journey deliberately substitutes prepared levels/XP/refill, fixed active
  owners and a selected starting context. Normal gameplay must begin from honest
  original initialization and support party preparation/recruitment and viable
  early encounters, recovery and progression. That is more than removing a flag.
  It likely needs broader skill/award/membership state and an independently
  playable opening. Prioritize it within the medium-term target, once the opening
  loop can survive without prepared advantages; an isolated startup screen is
  not an acceptable substitute.

## Generalization and architectural direction

**Reusable now:** resource loaders/identity, outdoor terrain navigation and
indoor projection, ordinary object ordering, world-owned actor scheduling and
lifecycle, supported combat/reward rules, original Event selection/resumption,
flags/quest counters, roster inventory/equipment, learned casting, derived
progression, service coordination, merchant generation and guarded exact restart.
The M39-M41 arc and M42/M43 demonstrate cross-system reuse, not unrestricted admission.

**Intentional gates:**
[XeenJourneyContent](../src/games/xeen/XeenJourneyContent.h) explicitly selects
capabilities and Vertigo cells; [regional rules](../src/games/xeen/XeenRegionalRules.cpp),
[route graphs](../src/games/xeen/XeenVertigoRoute.cpp) and
[city actor validation](../src/games/xeen/XeenVertigoWorld.cpp) bind original
resources and initial/reset influence. These restrictions protect honest
support. Their removal cannot supply missing effects, service rules or events.

**Missing mechanics/state:** modified weapon/armor and miscellaneous effects,
more damage/condition/monster behavior, meaningful lighting/doors/traps, broader
script consumers, recurring HP/SP/food recovery and ordinary calendar behavior,
character skills/awards/acquisition and normal preparation. Implement a mechanic
when selected content needs it, with its original semantics and existing owners;
do not treat accepting additional IDs/opcodes as completion.

**Content-specific work and validation:** special scripts, city resets and
exceptional monster behavior still need explicit treatment. Current exact
manifests, fixed region shapes, per-route actor certification and many capability
branches also make each new domain expensive to admit. Evolve these in the
chosen production slices: reusable depleted-stock validation, original-profile
admission, region retention and resource-driven actor/route closure should reduce
repeated implementation while retaining independent evidence. Existing parser,
original-resource, process-continuation and native-input tests are foundations;
repeatable content admission checks need to grow alongside new domains. A generic
framework rewrite or a separate validation-tooling milestone is not justified now.

The transition toward predominantly content admission occurs when an area's
terrain, actors, interactions, effects and retained state all fall within
established contracts. The medium-term expansion must demonstrate this with a
second useful area that mainly adds original resource/configuration admission
and validation, rather than another special Flow or state owner. Judge progress
by recurring mechanics reused and complete connected play, not admitted-ID counts
or reduced validation. Some original exceptions will continue to require code.
If consecutive areas still demand bespoke coordinators or persistence shapes,
reassess that bottleneck before admitting more content.

Preserve party/roster character, progression and inventory owners; world actor,
lifecycle and RNG owners; camera and flag owners; combat-owned actions; and
existing Event/Service/Presentation coordination. Preserve Quiet-only capture,
exact continuation, legacy isolation, full mutation/preimage observation and
accepted shared native-input scheduling. These are established contracts, not
generic prerequisite work. Commercial resources remain external and read-only.

## Medium-term objective

Target a **normal-start, connected Clouds foothold**, rather than an indefinitely
prepared Journey or a town with disconnected service menus. Vertigo is a useful
existing base, not a permanent restriction on which area has the best leverage.
After several arcs, observable completion should include:

- An original startup and supported party-preparation path into viable early
  play, without prepared levels, injected XP or substituted recovery.
- A connected town/outdoor/indoor loop with useful equipment acquisition and
  loot disposal/funding, repeatable HP/SP recovery, needed spell acquisition/use,
  earned Training and original quest progress. Useful recovery must remain
  available when no member has enough XP to train or has reached a trainer cap.
- Enough calendar, food, skills/awards, monster/effect and Event semantics for
  that loop to repeat without the current daylight/service-only assumptions.
  Cross-map actors, injuries, purchases, discoveries and return/reset behavior
  must remain exact across process restart.
- At least one subsequent useful area admitted mostly by established mechanics,
  resource/configuration policy and independent validation. Adding an area must
  not routinely introduce another party/world owner or special coordinator.

The foundational dependencies are the selected purchase consumer, honest
recurring recovery/time, acquisition/effect coverage driven by actual content,
normal party state and scalable retained-region/admission support. The next arcs
should close these dependencies through connected gameplay witnesses. Sell,
Guild, broader equipment and an indoor/dungeon slice are means to this target,
not an approved fixed sequence. A substantially functional production hub is a
step toward the foothold, not the whole medium-term objective.

## Long-term direction

1. **Bounded connected gameplay:** turn accepted systems into useful upgrade,
   recovery, exploration and progression loops, then prove reuse in another
   connected area. Keep honest boundaries and exact saves.
2. **Broader normal Clouds play:** support original initialization, party
   creation/recruitment/preparation, ordinary travel and time/rest/food, repeatable
   economy/recovery, skills and spell acquisition, needed equipment/monster
   effects, indoor hazards and reusable quest/Event behavior. Sustain multiple
   connected regions and return paths without prepared Journey assumptions.
3. **Substantially complete Clouds:** cover its regions/dungeons, progression,
   quests and ending, including exceptional scripts/monsters, comprehensive
   item/spell/service semantics and faithful resource-driven presentation.
   Completion requires whole-game progression and durable continuation, not a
   collection of isolated checkpoints. Prioritize audio/UI fidelity and wider
   delivery concerns according to demonstrated play and maintainer priorities.
4. **Mature shared foundations and Darkside:** reuse proven map, party, combat,
   economy, casting, script and save contracts; independently admit Darkside's
   content, balance, progression and exceptions. Numeric side-1 stock and access
   to `DARK.CC` metadata do not establish its gameplay readiness.
5. **World of Xeen integration:** connect both playable sides with original
   travel, shared/cross-side state, progression and integrated objectives/endings.
   Verify identity and compatibility across transitions; do not infer World
   integration from resource availability or shared numeric IDs.

Portability, localization, distribution and broader presentation/audio work
remain distinct decisions. At each review, ask whether recent arcs enable more
of the original game to be played continuously and more content to reuse the
same systems, rather than merely increasing the feature inventory.

## Replanning and review cadence

Review each proposed specification against verified Git state, current code/tests
and the pinned reference. **M43 closure reaches the required broad-review
boundary: perform that review before selecting any connected-area or recurring-SP
foundation.** This documentary closure records the trigger; it does not constitute
that architectural review or select M44. Evaluate the accepted longer route and
recovery loop against remaining SP/time limits, sustained funding, useful return
visits and the cost of adding another retained region. Keep the connected-content
entry as the provisional default successor until that evidence supports a choice.

Replan earlier when:

- A useful purchase/recovery witness needs materially broader item effects,
  route/actor/Event semantics, or hidden prerequisites than the proposed boundary.
- Depleted stock, service operation boundaries, new character inputs or another
  retained region require unexpected persistence/compatibility state.
- Practical play reveals inventory congestion, funding, SP, survival or normal
  startup constraints that contradict the assumed ordering.
- A chosen unit cannot be independently accepted, or an alternative area offers
  materially better reuse and a reachable original gameplay purpose.
- Consecutive content additions remain bespoke despite established mechanics,
  or maintainer priorities change.

Resolve a trigger with the smallest decision-changing investigation. Reconsider
the provisional successor and its necessary foundations first; revisit an
accepted boundary only when new evidence contradicts its own behavior or
persistence contract. Pricing details, menu design and exact acceptance matrices belong in an authorized specification,
not this roadmap. Ordinary implementation difficulty alone is not a replan.

Roadmap approval, milestone-specification authorization and implementation
authorization remain separate. Completion or push does not promote provisional
units into approved work. Use the verified baseline/handoff gate in
[AGENTS.md](../AGENTS.md) for subsequent planning and review.
