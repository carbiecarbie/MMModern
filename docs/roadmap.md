# MMModern - Roadmap

**M117 is the latest completed and accepted milestone**
([plan](milestone-117-plan.md)), after M116
([plan](milestone-116-plan.md)).
[Project status](project-status.md) describes what is playable now;
[project history](project-history.md) records each completed milestone.
Reference provenance belongs to [dependencies](dependencies.md).

The project's goal is a **faithful reimplementation** of the original games
(see [AGENTS.md](../AGENTS.md)). Every milestone below reproduces original
behavior; quality-of-life options are a later, opt-in decision.

## Where we are

<a id="current-planning-state"></a><a id="m28---durable-bounded-encounter-completion-and-revisit"></a><a id="approved-m33-m35-arc"></a>

Clouds of Xeen, Darkside of Xeen and World of Xeen play from a new game to
their endings on the CD edition's data, on Windows, Linux and the Steam
Deck, in English or with the Russian fan translation. Release 0.19.0 has all
of that, plus the opt-in game controller support (M115). The optional CRT
profiles (M117) are available in the development build. What is still open is under
[later goals](#later-goals).

## Completed arcs

M1-M43 built the foundations one narrow certified slice at a time; M44
removed that machinery, and since then whole maps load from the original
resources. The arcs stay here for their step numbers, which plans cite.

### Closing Vertigo and the first release (M44-M66)

<a id="near-term"></a><a id="deferred-presentation-work"></a><a id="next-milestones"></a>

M44-M64 made input, menus and presentation original, admitted the whole of
Vertigo, moved to the CD edition's data (M53) and ended with the first
public releases,
[v0.1.0 and v0.1.1](https://github.com/carbiecarbie/MMModern/releases). The
maintainer declined an opt-in faster movement option.
[M65](archive/milestone-65-plan.md) then added
[the coverage report](coverage.md), and [M66](archive/milestone-66-plan.md)
cut the complete test suite from 29 minutes to about 3.

### Finishing the game's systems (M67-M94)

<a id="next-arc-finishing-the-games-systems"></a>

This arc implemented each mechanic once instead of map by map, and release
0.10.0 delivered Clouds of Xeen from a new game to its ending.

| Step | Milestones | Result |
| --- | --- | --- |
| 1. Equipped item contributions | [M67](archive/milestone-67-plan.md) | Release v0.1.2 |
| 2. Monster abilities | [M68](archive/milestone-68-plan.md) | Release v0.1.3 |
| 3. Generic per-map state | [M69](archive/milestone-69-plan.md) | Per-map save state |
| Item generation | [M71](archive/milestone-71-plan.md) | Original makeItem draws for every source |
| 4. World Events, Clouds | [M70](archive/milestone-70-plan.md) | Darkside-only operations came with the Darkside arc |
| 5. Terrain and simple services | [M72](archive/milestone-72-plan.md) | |
| 6. Spells | [M73](milestone-73-plan.md)-[M76](milestone-76-plan.md) | All 76 Clouds spells |
| 7. Combat Equip and Use, missing screens | [M77](milestone-77-plan.md) | |
| 8. Opening areas | [M78](milestone-78-plan.md) | Every Clouds map and travel path; Mountaineer and Pathfinder as DOS |
| 9. Services of the other towns | [M79](milestone-79-plan.md) | Each town with its original values |
| 10. Entrance scenes and the Arena | [M80](milestone-80-plan.md) | Pyramid and Darkside-key entrances came with Darkside |
| 11. Monster treasure | [M81](milestone-81-plan.md) | Original drops; pending treasure in the running program |
| 12. Audio (Tier A) | [M82](milestone-82-plan.md), [M83](milestone-83-plan.md) | CD OPL3 music and effects, voices, CD speech, Text off |
| 13. Event dispatch as in DOS (Tier A) | [M84](milestone-84-plan.md), [M85](milestone-85-plan.md), [M86](milestone-86-plan.md) | Events rerun after every input; four presentation gaps |
| 14. Inn, recruiting and creation | [M87](milestone-87-plan.md), [M88](milestone-88-plan.md) | Party of one to six; Create and Delete |
| 15. Clouds scripted kills and map rules | [M89](milestone-89-plan.md) | Lord Xeen, the Scepter and the quest death hooks |
| 16. Clouds verification sweep | [M90](milestone-90-plan.md) | Megacredits to Lord Xeen confirmed; no content gap |
| 17. Cutscene presenter and Clouds intro | [M91](milestone-91-plan.md) | Read from the installed `WORLD/XEEN.EXE` |
| 18. Clouds ending and score | [M92](milestone-92-plan.md) | Clouds finishable; win kept in `settings.ini` |
| 19. Other sound devices (Tier A) | [M94](milestone-94-plan.md) | AdLib, Sound Blaster and MT-32 from `XEEN.CFG`; the rest silent with a notice |
| 20. Startup and the Darkside intro | [M93](milestone-93-plan.md) | Full story until watched, then the logos |

Step 19 left the Canvas/GS, PC speaker, Wave Blaster and TAP-10 music and
five digital drivers silent with a notice, their open questions recorded in
the [M94 plan](milestone-94-plan.md#scope). A switch between devices while
playing would be an opt-in approved deviation (maintainer, 2026-10-21).

### Release quality and the Darkside start (M95-M101)

<a id="next-arc-release-quality-and-the-darkside-start"></a>

A next-steps analysis (2026-10-28) chose to make the finished Clouds
sturdier and easier to test before Darkside began, in five steps:

1. Clouds audit: release 0.10.1 fixed its save defect, and
   [M95](milestone-95-plan.md) turned its invariants into tests.
2. DOS save investigation: picked chests stay open
   ([M96](milestone-96-plan.md), save v22), a developer tool exports DOS
   saves ([M97](milestone-97-plan.md)) and a Cartographer party marks its
   cell at every arrival ([M98](milestone-98-plan.md)). Importing DOS saves
   waits for policy decisions.
3. 4:3 picture ([M99](milestone-99-plan.md)), square pixels as an opt-in.
4. Launcher, sound setup, log and issue template
   ([M100](milestone-100-plan.md)).
5. Darkside start ([M101](milestone-101-plan.md)): both sides' state, save
   v23 and the Pyramid between the sides.

### Darkside arc

<a id="medium-term-darkside-and-world-of-xeen"></a>

A research pass (2026-10-31) compared every remaining Darkside gap with the
pinned reference. Each step started with a Part 0 of DOS program analysis,
because the reference was wrong in comparable Clouds cases (M80).

| Step | Tier | Result |
| --- | --- | --- |
| 0. Maintenance (`5e4f685`) | B | Spawn onto Darkside outdoor maps and Skyroads; outdoor wall 5 needs Mountaineer on Darkside; Ali Baba's chest lock recorded as a reference difference |
| 1. Darkside towns ([M102](milestone-102-plan.md)) | B | Every town service, and the Tower, Dungeon and Dwarf town scenes. Done 2026-10-31 |
| 2. Darkside Events ([M103](milestone-103-plan.md)) | B | Every Event instruction of the ordinary Darkside maps. Done 2026-11-01 |
| 3. Mixed World of Xeen maps ([M104](milestone-104-plan.md)) | A | Darkside maps 113-128, FlipWorld, the Darkside-key entrances and Town Portal from them. Done 2026-11-01, save v24 |
| 3a. Last mixed-map Events ([M105](milestone-105-plan.md)) | B | World flag 128 on map 116 and DisplayLarge on map 121. Done 2026-11-01 |
| 4. Darkside and World wins, Darkside ending ([M106](milestone-106-plan.md)) | A | Opcodes 57 and 58, the ending mirror words, the won states, the title replay rows and the Darkside presentation. Done 2026-11-02, save v25 |
| 5. World ending presentation ([M107](milestone-107-plan.md)) | B | The `XEEN.EXE` World ending after the win, `genesis` and the W replay. Done 2026-11-02 |

[M111](milestone-111-plan.md) (Tier A, accepted 2026-11-04) then made a
refused Event discard the whole Event
([approved deviation](original-quirks.md#dev-event-presentation-abandon))
and made physical spells after a Xeen Slayer Sword attack ignore physical
resistance, as in DOS.

### After the Darkside audit

A Darkside fidelity and integration audit (2026-11-02) ran every Darkside
Event cell twice, with saves, loads, trips away and keys pressed during
waits. It found two hangs, fixed in `a6949b7`, and stale or missing
documentation, fixed in `8e52743`. Its follow-up is done: a maintenance
round (`b75e050`: Darkside Create at level 5, a notice where a presentation
error blocked input, the Event instruction limit, the Darkside CD tracks and
the Quest Items title order), the audit invariants as tests
(M108, [plan](milestone-108-plan.md)), the Desert Paladin stone copies kept
as an original bug ([details](original-quirks.md#sv-pin-data-patches)) and
release 0.15.1 (2026-11-02).

## Later goals

Roughly in this order, each with its own decision and plan:

1. Automated builds and tests on GitHub. Done: M109 (Tier A,
   [plan](milestone-109-plan.md), accepted 2026-11-03). Tests on the
   original data stay local, because that data cannot be published.
2. Other systems. Done: the Steam Deck through Proton
   (M112, [plan](milestone-112-plan.md)), the native Linux build and its CI
   job (M113, [plan](milestone-113-plan.md)), and a setup screen inside the
   game that can also serve Android, with an AppImage
   (M114, [plan](milestone-114-plan.md)).
3. Saves that survive updates, when compatibility is revisited for 1.0.
4. A study of the community MM3 conversion
   ([mm345-conversion](https://github.com/hooby3dfx/mm345-conversion)), then
   mod support and modding tools.
5. Opt-in extras: things the original never had, each one an option that is
   off by default, so the game stays faithful unless the player turns it on.
   Game controller support is done: M115 (Tier A,
   [plan](milestone-115-plan.md), accepted 2026-11-10), and so are the
   picture filters: M116 (Tier B, [plan](milestone-116-plan.md), accepted
   2026-11-11). [M117](milestone-117-plan.md) (Tier B, accepted 2026-11-14) replaced
   the classic filters with two optional OpenGL CRT profiles. Still to
   study: switching between the original sound
   devices while playing, and possibly some interface improvements.
6. Other language editions of the original. Russian is done: M110 (Tier A,
   [plan](milestone-110-plan.md), accepted 2026-11-03) lays the old-games.ru
   fan translation over the CD edition. A reader of Russian has not yet
   checked the text in play.
7. **To evaluate:** whether to support Swords of Xeen, a standalone game on
   the same engine by the fan group Catware; the maintainer decides
   (2026-11-02). It keeps Darkside's formats in one archive (`swrd.cc`), and
   the pinned reference supports it as its own game type with about 70
   differences. It would need a game profile and a save format change
   (Tier A), and its own analysis of `swords.exe` and its `xeen.dat` as DOS
   evidence.

## Review cadence

<a id="replanning-and-review-cadence"></a>

Revisit this roadmap after each milestone closes, using what play-testing showed.
Replan earlier if a milestone reveals that its scope is wrong, that whole-map
admission needs a missing foundation, or that maintainer priorities changed.
Roadmap approval does not authorize implementation; the maintainer starts each
milestone explicitly.
