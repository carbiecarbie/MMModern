# MMModern - Roadmap

**M53 is the latest completed and accepted milestone.**
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
exact save/restore. The playable scope is still a **prepared Journey**: the
map-23 mainland and 49 certified Vertigo cells, with injected party levels.

Each milestone admitted a narrow, individually certified slice (exact cells,
actors, Event addresses, a new content/save contract and legacy isolation).
That proved the systems, but the cost per piece of content did not fall: M43
needed 55 files and ~2,400 added lines for one service. The process in
[AGENTS.md](../AGENTS.md) (playable-by-default admission, one current save
format, Tier A/B milestones) applies from M44 onward. M44 removed the legacy
machinery: the code now has one save format and one Journey configuration, and
`ctest -L fast` gives a quick iteration loop.

## Next milestones

<a id="near-term"></a><a id="deferred-presentation-work"></a>

| Milestone | Tier | Goal | Accepted when |
| --- | --- | --- | --- |
| **M54 - Original title menu and saves** | A | The original title screen (New, Load, Credits), the difficulty dialog and the original save/load dialogs with in-game loading, replacing the temporary command-line entry. | A new game and a saved game start from the original menus by mouse and keyboard; save/restore exact. |

The order matters. M44-M48 lowered the cost of change and made input, menus
and presentation original and generic, M49 corrected combat rules and M50
admitted the whole of Vertigo from resources, and M51 added Rest, food and
daily time. M52 started the original new game and M53 moved to the CD
edition's data, the single reference edition. M54 replaces the temporary
command-line entry with the original DOS menus, which come from that edition.

Deferred because they add no rework later: **audio** (sounds, music, voices;
a separate additive system), the original **event dialogs, casting dialog and
treasure sequence** (better redone once more events are playable), and
**animated location shopkeepers**. Combat-time equipping and the combat Use
button remain Tier A future work.

After M54, choose the next milestone from play-testing evidence. Likely candidates:

- **Leaving Vertigo:** connect the mainland to further areas,
  adding Event opcodes as they are reached. Map 22 (north of map 23) needs
  SetChar, GiveMulti and MakeNothingHere.
- **Missing town services:** Inn/party management, Tavern (food, tips), Guild
  spell purchase (Vertigo Guild at `(28,20,13)`; needs membership state), Sell
  (needs the Merchant skill input), Bank.
- **The deferred presentation work above**, and mechanic gaps found in play:
  item/weapon effects, more monster abilities, doors/locks/traps, lighting.

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
