# MMModern - Roadmap

**M77 is the latest completed and accepted milestone.**
[Project status](project-status.md) describes what is playable now;
[project history](project-history.md) records completed milestones. Reference
provenance belongs to [dependencies](dependencies.md).

The project's goal is a **faithful reimplementation** of the original games
(see [AGENTS.md](../AGENTS.md)). Every milestone below reproduces original
behavior; quality-of-life options are a later, opt-in decision.

## Where we are

<a id="current-planning-state"></a><a id="m28---durable-bounded-encounter-completion-and-revisit"></a><a id="approved-m33-m35-arc"></a>

M1-M43 built the foundations: original resource loading, outdoor/indoor
rendering, navigation, Events, party/inventory/equipment, combat, casting,
treasure, services (Armor Repair, Buy, Training, Temple), economy/calendar and
exact save/restore, one narrow certified slice at a time. M44 removed that
machinery; since then whole maps load from the original resources. M44-M48
made input, menus and presentation original and generic, M49 corrected combat
rules, M50 admitted the whole of Vertigo, M51 added Rest, food and daily time,
M52 started the original new game, M53 moved to the CD edition's data (the
single reference edition), M54 brought the original title screen and saves,
and M55 closed small interface gaps (Strafe, Exchange, Quick Reference, Info,
Quick Fight). M56 added Tavern food and Bank transfers, with generic terminal
Event-to-Service handoff for supported preludes. M57 added generic Event effects
(moving objects, XP, awards, flags, currency and generated treasure), and M58
skills and mapping (teachers, Cartography, automap and Map). M59 completed city
searches, display cases and jail, the well, Vern's membership and Guild entry,
and M60 the original Cast dialog, Guild spells and eight new spell effects.
M61 completed the Ironworks item services and item Use, M62 the original
screens and shopkeeper animation, M63 the original clock, movement pace
and timed sequences, and M64 the last Vertigo fixes and the first public
releases.

## Completed arc: closing Vertigo and the first release

<a id="near-term"></a><a id="deferred-presentation-work"></a><a id="next-milestones"></a>

A direction investigation (2026-10-10) chose to close Vertigo, the city, and
then publish. The arc ran from M56 to M64: food and bank, generic Event
effects, skills and mapping, Vertigo content, magic, items for the loot,
faithful screens, the original clock and movement pace (the maintainer
declined an opt-in faster movement option), and closure with the first public
releases, [v0.1.0 and v0.1.1](https://github.com/carbiecarbie/MMModern/releases).
[Project history](project-history.md) has each milestone. After it,
[M65](archive/milestone-65-plan.md) measured every mechanic the original data uses in
[the coverage report](coverage.md), and [M66](archive/milestone-66-plan.md) cut the
complete test suite from 29 minutes to about 3.

## Next arc: finishing the game's systems

The game loads whole maps from the original data; what keeps an area from
playing is the mechanics it uses. This arc implements them once each, in the
order [the coverage report](coverage.md) gives (most maps unblocked first),
instead of map by map. Each step is its own milestone and plan; the census is
regenerated each time, so progress is visible. Steps are direction, not
approved plans, and the order may change when the census or play shows a
better one.

Between steps 4 and 5, [M71](archive/milestone-71-plan.md) made item generation
follow the original makeItem draws for every supported source (completed and
accepted).

1. **Equipped item contributions**: completed and accepted in
   [M67](archive/milestone-67-plan.md) (release v0.1.2).
2. **Monster abilities**: completed and accepted in
   [M68](archive/milestone-68-plan.md) (release v0.1.3).
3. **Generic per-map state**: completed and accepted in
   [M69](archive/milestone-69-plan.md) (save v13).
4. **World Events, Clouds**: completed and accepted in
   [M70](archive/milestone-70-plan.md). Darkside-only operations and Darkside durable
   state wait for a later Darkside arc; shared operations stay generic.
5. **Terrain and simple services**: completed and accepted in
   [M72](archive/milestone-72-plan.md) (save v14; Sign In moved to step 8).
6. **Spells**: [M73](milestone-73-plan.md) monster control and four group
   spells (Tier A, completed and accepted); [M74](milestone-74-plan.md)
   monster damage spells (Tier B, completed and accepted);
   [M75](milestone-75-plan.md) party healing and conditions, including Raise
   Dead, Resurrection, Divine Intervention and Create Food (Tier B,
   completed and accepted); [M76](milestone-76-plan.md) buffs,
   information, item and travel spells (Tier A, completed and accepted).
   All 76 Clouds spells work.
7. **Combat Equip and Use, and the missing screens**: completed and
   accepted in [M77](milestone-77-plan.md) (combat items and Town Portal's
   combat item path, Quests, Awards and the three skill indicators; save v15
   unchanged).
8. **Inn, recruiting and character creation** (Tier A, high): Tavern Sign In,
   Dismiss (moved from step 7: nothing brings a dismissed member back before the Inn),
   attribute rolls, the creation screens and changes to the saved party,
   including Poison Volley shooting for the active party size (M74 assumes six).
9. **Monster treasure** (Tier A, changes the save): item drops for every
   monster and drop levels 2 to 6, including accessories in staged treasure,
   on the shared generator from [M71](archive/milestone-71-plan.md).
10. **Entrance scenes** (likely Tier B): the scenes shown at some entrances,
    without interaction (`DoTownEvent` 8 to 12: Reaper, Golem, Dwarf Mine,
    Sphinx, Pyramid), on up to 6 Clouds maps each, F3 included. The Dwarf
    Mine scene leads toward the next area after Vertigo.
11. **Opening areas**: admit every Clouds map whose census readiness has no
    unsupported mechanic, then play-test by sampling rather than map by map.
    The original dark view of dark maps moved here from M76, and so did
    light-drain cells unless M76 finds some on the open maps (the census
    counts neither yet).

Audio comes after this arc (Tier A: music, sounds and CD tracks on the
existing clock); the original durations are already kept.

Also left from the release list: Run inside Vertigo; it joins the step whose
mechanic it needs.

## Medium-term objective

A **normal-start Clouds foothold**: start a new game, play Vertigo and its
surroundings continuously, with equipment buy/sell, recurring HP/SP recovery,
spell acquisition, Training and original quest progress, and save/restore
wherever it is safe. Adding a further area should mostly mean loading its
resources and filling mechanic gaps, not writing a new coordinator or save shape.

## Long-term direction

1. **Broader Clouds play:** ordinary travel between regions, time/rest/food,
   full economy and services, skills, item/monster effects, indoor hazards and
   reusable quest/Event behavior.
2. **Complete Clouds:** all regions/dungeons, progression, quests and ending,
   including exceptional scripts/monsters, with faithful presentation and audio
   as maintainer priorities allow.
3. **Darkside:** reuse the shared engine and admit Darkside's own content,
   balance and exceptions. Resource access alone does not make it playable.
4. **World of Xeen:** connect both sides with original travel, shared state and
   integrated objectives/endings.

Portability, localization and distribution are separate decisions.

## Review cadence

<a id="replanning-and-review-cadence"></a>

Revisit this roadmap after each milestone closes, using what play-testing showed.
Replan earlier if a milestone reveals that its scope is wrong, that whole-map
admission needs a missing foundation, or that maintainer priorities changed.
Roadmap approval does not authorize implementation; the maintainer starts each
milestone explicitly.
