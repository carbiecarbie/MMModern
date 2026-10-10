# MMModern - Roadmap

**M55 is the latest completed and accepted milestone.**
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
Quick Fight).

## Next milestones: closing Vertigo for a first public release

<a id="near-term"></a><a id="deferred-presentation-work"></a>

**Direction, not an approved plan.** A direction investigation (2026-10-10)
inventoried everything the original offers in Vertigo and on the map-23
mainland. The maintainer chose to close **Vertigo** (the city, logical map 28)
and then publish: Vertigo complete or nearly so, the mainland reachable at its
current progress and the rest of the world not yet started. The order, scope
and split below may change with DOSBox evidence and play-testing; each step
still needs its own plan and the maintainer's start.

1. **DOSBox evidence session** (maintainer): Joe's proof and Gunther's reward,
   trees, the well, display cases and jail, teacher prices, food and bank.
2. **Food and bank** (Tier B): buy food at the Tavern; deposit and withdraw.
3. **Generic Event effects and world state** (Tier A, probably split): moving
   and turning objects, rewards to the party (XP, gold, gems, items through the
   common treasure generation and RNG), durable awards and flags. Joe's proof
   and Gunther's reward are the first use.
4. **Skills and mapping** (Tier A): learning skills (Mylo, Rialdo), their
   effects from one character state, Cartography, the automap and the Map.
5. **Vertigo content** (Tier B): trees, chests, beds, rubbish, display cases
   and jail, the well, guild membership (Vern), signs and talk, through the
   systems above rather than per-cell handling.
6. **Magic** (Tier A): the original Cast dialog by mouse and keyboard, guild
   spell purchase with the original catalogue, and four new effects (Wizard
   Eye, Shrapmetal, Energy Blast, Jump); unimplemented spells say "not
   supported yet".
7. **Items needed for the loot** (reduced): Identify, Sell and Use outside
   combat for the items Vertigo actually yields.
8. **Closure and release**: the whole Vertigo route from New Game with
   save/load, then the release work (Windows binary, how to point at the GOG
   data, licence, an honest list of what is missing).

**Left for after the release:** the mainland's remaining content (fountains,
bottles, outpost, statues) and other areas, character creation and the Inn,
combat Equip/Use, Run inside Vertigo, Drink/Tip/Rumors, Donation/Uncurse, the
mirror, full Quests/Awards screens, the remaining spells, audio, animated
shopkeepers and the original treasure sequence.

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
