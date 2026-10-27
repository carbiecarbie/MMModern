# MMModern - Roadmap

**M93 is the latest completed and accepted milestone.**
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

Since then M65-M93 have made Clouds of Xeen playable from a new game to its
ending, with the original intros (see the arcs below). Step 19 is the only
step left in the current arc; the maintainer chooses the next milestone.

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
playing is the mechanics it uses. This arc implements them once each instead
of map by map. Each step is its own milestone and plan; the
[coverage report](coverage.md) is regenerated each time, so progress is
visible. Steps are direction, not approved plans, and the order may change
when the census or play shows a better one.

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
   [M72](archive/milestone-72-plan.md) (save v14).
6. **Spells**: [M73](milestone-73-plan.md) to [M76](milestone-76-plan.md),
   completed and accepted. All 76 Clouds spells work.
7. **Combat Equip and Use, and the missing screens**: completed and
   accepted in [M77](milestone-77-plan.md) (save v15 unchanged).

A gap investigation (2026-10-19) compared the census and the pinned reference
with the code. Nearly every remaining Clouds blocker comes from the fixed
admission of maps 23 and 28: about 64 of the 85 Clouds maps have no
unsupported mechanic, while Jump, Teleport, the mirror, Town Portal's other
towns, Lloyd's Beacon and falls from the cloud maps refuse only because their
destination is not admitted. The entrance scenes are the transitions to the
next areas, since they check a quest item and load the next map. The Inn,
recruiting and character creation do not block progress: the starting party
of six can play the game. The steps after M77 are reordered accordingly.

Before step 8, a maintenance fix (`bb22c52`) let combat Run use each map's
own run position and chance, indoors included, and added Mr Wizard's return
to Vertigo.

8. **Opening areas**: completed and accepted in
   [M78](milestone-78-plan.md) (save v16). Every Clouds map opens from the
   original data with every travel path; Mountaineer crosses mountains and
   Pathfinder outdoor wall types 3 and 6, each with two members, as DOS
   shows (the reference has no Pathfinder movement); dark maps and light
   drain follow DOS.
9. **Services of the other towns**: completed and accepted in
   [M79](milestone-79-plan.md) (save v16 unchanged). Rivercity, Shangri-La,
   Nightshadow, Asp, Winterkill and Newcastle work with each place's
   original values; Newcastle Level 2's Temple keeps the last Temple prices
   of the running program, as DOS does.
10. **Entrance scenes and the Arena**: completed and accepted in
    [M80](milestone-80-plan.md) (save v16 unchanged). The Reaper, Golem,
    Dwarf Mine and Sphinx lead to the Clouds dungeons they guard, and the
    Warzone Arena runs as in DOS. The Pyramid (action 12) and the four
    entrances whose keys come from Darkside wait for the Darkside arc.
11. **Monster treasure**: completed and accepted in
    [M81](milestone-81-plan.md) (save v17). Every Clouds monster drops the
    gold, gems and items of its original record, at treasure levels 1 to 6
    with accessories; pending treasure stays in the running program as in
    DOS, and Event treasure shares it, keeping the original's duplicate of
    monster items. Lord Xeen's Scepter came with step 15.
12. **Audio** (Tier A). Completed and accepted in [M82](milestone-82-plan.md)
    and [M83](milestone-83-plan.md) (maintainer, 2026-10-22):
    installed CD OPL3 music/effects, VOC voices, CD speech, Text off, fixed
    balance checked against DOSBox, and output recovery on the existing clock.
    Save v18 is unchanged; the small SDL resampler rate error remains a known
    limitation.
13. **Event dispatch as in DOS** (Tier A): completed and accepted in
    [M84](milestone-84-plan.md) and [M85](milestone-85-plan.md)
    (2026-10-23; save v18 unchanged). A flagged cell's Events run again
    after every key or click, Load and New Game start with a run, and the
    keyboard and mouse follow the original's tables; an Event a monster
    interrupted and a door label up at combat start run again after the
    fight, Run lands on every landing, and every Event run clears WhoWill
    ([rules](rules/world-events.md#event-dispatch)).
    [M86](milestone-86-plan.md) (Tier B, accepted 2026-10-23) then closed
    four Clouds presentation gaps: the open-door frame, the falling
    animation and its sounds, Moon Ray's per-member order and the border
    indicators at services.
14. **Inn, recruiting and character creation**, in two milestones
    (maintainer, 2026-10-23). [M87](milestone-87-plan.md) (Tier A, accepted
    2026-10-24, save v19): Sign In and the Inn with its saves, Dismiss, and a
    party of one to six everywhere, including the DOS monster target draws
    and Poison Volley for the active party size, with every roster record
    saved in full. [M88](milestone-88-plan.md) (Tier A, accepted
    2026-10-24, save v20): Create and Delete as in DOS, with the original
    creation screen and attribute rolls, saving an empty party inside the
    Inn as DOS does.

A completion audit (2026-10-24) found that the Clouds main quest could not
finish yet: Lord Xeen's death gave neither the Scepter nor the original's
death hook, and `CutsceneEndClouds` was unsupported (in DOS, `XEEN.DAT`
records the win and the score and hands off to `XEEN.EXE`, which plays the
ending). Three side quests waited for other death or map rules. Every other
quest used supported Event operations but had not been played outside
Vertigo. Steps 15-18 resolved all of this. A DOSBox run of the ending from
a late test save (maintainer, 2026-10-24) showed the rest. The Scepter
arrives through the ordinary treasure screen, credited to the first member
even when that member is dead. The Sixth Mirror shatters after a four-page
message. The game then saves the live party at Vertigo to the current slot,
before `XEEN.EXE` plays the ending; M92's repeat capture overturned an
earlier reading that the save kept the roster as it was before the fight.
The ending lasts about three minutes and closes with the score and the
World of Xeen title. A maintenance fix (`14fb64d`) also made the packed
flag banks read in DOS bit order. Steps 15-18 below finished Clouds; the
Pyramid and the Darkside and World endings wait for the Darkside arc.

15. **Clouds scripted kills and map rules**: completed and accepted in
    [M89](milestone-89-plan.md) (Tier B, 2026-10-25; save v20 unchanged).
    Only the Xeen Slayer Sword hurts Lord Xeen, with the original's +100
    per hit, and his death gives the Scepter through the ordinary treasure
    screen with his defeat flag and award. The Clan King, Count Draco and
    the lake's Water Dragons complete Gunther's, the Nightshadow well's and
    Medin's quests; Ice Trolls heal every round; the flag-25 caves, the
    quest-done objects and Newcastle's castle follow DOS. The audit's
    map-50 rule proved Darkside only. The Sixth Mirror shows its text, and
    its ending said "not supported yet" until step 18.
16. **Clouds verification sweep**: completed and accepted in
    [M90](milestone-90-plan.md) (Tier B, 2026-10-26; save format
    unchanged). Gate presets and presets on the watched Events, with
    original-data tests of the Megacredits chain, the main line and
    Winterkill, confirmed Clouds from Newcastle's Megacredits to Lord Xeen;
    the sweep found no content gap.
17. **Cutscene presenter and the Clouds intro**: completed and accepted in
    [M91](milestone-91-plan.md) (Tier B, 2026-10-25; save v20 unchanged).
    Other Options `C` plays the Clouds intro as `WORLD/XEEN.EXE` does, on
    one presenter that reads its tables from the installed EXE; only Escape
    skips it. The title lightning proved unreachable on the CD title. The
    startup path and the Darkside intro moved to step 20.
18. **Clouds ending and score**: completed and accepted in
    [M92](milestone-92-plan.md) (Tier A, 2026-10-26; save v21). The Sixth
    Mirror wins Clouds as in DOS: the live party is saved at Vertigo, the
    score counts experience and real play time, and the `WORLD/XEEN.EXE`
    ending plays with its score reveal before a fresh World title. The win
    is kept in `settings.ini` and unlocks Other Options `V`; `showtime`,
    `bloopers`, the vanished castle, the Beacon refusal and the panel
    clocks follow DOS. `showdown` and `genesis` wait for the Darkside arc.
19. **Other sound devices** (after step 12): the original's other drivers
    (AdLib and Sound Blaster FM, MT-32 with the player's ROMs, PC speaker and
    the rest; M82 plays the CD's Pro Audio Spectrum driver), chosen as the
    installer did; an opt-in switch while playing would be an approved
    deviation (maintainer, 2026-10-21).
20. **Startup and the Darkside intro**: completed and accepted in
    [M93](milestone-93-plan.md) (Tier B, 2026-10-27; save v21 unchanged).
    Every launch runs `WORLD/XEEN.EXE`'s Darkside path before the title:
    the full story until it has been watched to its end, then NWC, JVC and
    the World logo. Other Options `D` clears the watched state and plays
    the full story again. The watched state is kept in `settings.ini`,
    never in `XEEN.CFG`
    ([cutscene rules](rules/cutscenes.md#startup-dispatch)). Step 19
    remains in this arc.

Steps 15-18 and 20 are done; step 19 remains and is independent.

## Medium-term objective

The first objective, a **normal-start Clouds foothold** (a new game played
continuously with equipment, recovery, spells, Training, quests and saves),
was reached in Vertigo by M64 and extended to all of Clouds by M78-M81;
steps 15-18 and 20 then finished Clouds, and step 19 remains. A further area
should mostly need its resources and mechanic gaps, not a new coordinator or
save shape.

## Long-term direction

1. **Broader Clouds play** (reached): ordinary travel
   between regions, time/rest/food, full economy and services, skills,
   item/monster effects, indoor hazards and reusable quest/Event behavior.
2. **Complete Clouds** (steps 15-18, reached with M92 and confirmed by M90's
   play-through): all regions/dungeons, progression, quests
   and ending, including exceptional scripts/monsters, with faithful
   presentation and audio as maintainer priorities allow.
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
