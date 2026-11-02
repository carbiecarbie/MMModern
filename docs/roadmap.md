# MMModern - Roadmap

**M107 is the latest completed and accepted milestone.**
[Project status](project-status.md) describes what is playable now;
[project history](project-history.md) records each completed milestone.
Reference provenance belongs to [dependencies](dependencies.md).

The project's goal is a **faithful reimplementation** of the original games
(see [AGENTS.md](../AGENTS.md)). Every milestone below reproduces original
behavior; quality-of-life options are a later, opt-in decision.

## Where we are

<a id="current-planning-state"></a><a id="m28---durable-bounded-encounter-completion-and-revisit"></a><a id="approved-m33-m35-arc"></a>

M1-M43 built the foundations one narrow certified slice at a time: original
resource loading, rendering, navigation, Events, party and inventory, combat,
casting, treasure, services, economy and exact save/restore. M44 removed that
machinery; since then whole maps load from the original resources. M44-M64
made input, menus and presentation original, admitted the whole of Vertigo
with Rest, food, time, skills, magic, item services and the original clock,
moved to the CD edition's data (M53), and closed with the first public
releases.

Since then M65-M94 have made Clouds of Xeen playable from a new game to its
ending, with the original intros and the installer's sound devices, and
release 0.10.0 delivered them. The [next arc](#next-arc-release-quality-and-the-darkside-start)
makes Clouds sturdier and easier to test and report on, then starts Darkside.
Its first step is done: M95 closed the Clouds audit with tests for its
invariants. The DOS save investigation followed: it found that picked
chests stay open in DOS, which M96 added, and M97 added a tool that exports
MMModern saves to DOS. Its play-test led to M98, which makes a Cartographer
party mark its cell at every arrival. M99 then gave the picture the 4:3
shape of the original's monitors, and M100 added the launcher window, the
installer's sound setup, a log file and an issue template. M101 then started
Darkside: both sides' state, save v23 and the Pyramids between the sides.
The [Darkside arc](#darkside-arc) followed with its towns (M102), Events
(M103), the mixed World of Xeen maps (M104, save v24), their last three
Events (M105), and the Darkside and World wins with the Darkside ending
(M106, save v25). The World ending's presentation (M107) completed the arc:
every ending of the CD now plays. A Darkside audit followed; its
[follow-up steps](#after-the-darkside-audit) come next.

## Completed arc: closing Vertigo and the first release

<a id="near-term"></a><a id="deferred-presentation-work"></a><a id="next-milestones"></a>

A direction investigation (2026-10-10) chose to close Vertigo and then
publish. The arc ran from M56 to M64 and ended with the first public
releases, [v0.1.0 and v0.1.1](https://github.com/carbiecarbie/MMModern/releases);
the maintainer declined an opt-in faster movement option. After it,
[M65](archive/milestone-65-plan.md) measured every mechanic the original data
uses in [the coverage report](coverage.md), and
[M66](archive/milestone-66-plan.md) cut the complete test suite from 29
minutes to about 3.

## Completed arc: finishing the game's systems

<a id="next-arc-finishing-the-games-systems"></a>

The game loads whole maps from the original data; what keeps an area from
playing is the mechanics it uses. This arc implements them once each instead
of map by map. Each step is its own milestone and plan, and the
[coverage report](coverage.md) is regenerated each time. Steps are direction,
not approved plans, and the order may change when the census or play shows a
better one. [Project history](project-history.md) describes each completed
step.

| Step | Milestones | Result |
| --- | --- | --- |
| 1. Equipped item contributions | [M67](archive/milestone-67-plan.md) | Release v0.1.2 |
| 2. Monster abilities | [M68](archive/milestone-68-plan.md) | Release v0.1.3 |
| 3. Generic per-map state | [M69](archive/milestone-69-plan.md) | Per-map save state |
| Item generation | [M71](archive/milestone-71-plan.md) | Original makeItem draws for every source |
| 4. World Events, Clouds | [M70](archive/milestone-70-plan.md) | Darkside-only operations wait for the Darkside arc |
| 5. Terrain and simple services | [M72](archive/milestone-72-plan.md) | |
| 6. Spells | [M73](milestone-73-plan.md)-[M76](milestone-76-plan.md) | All 76 Clouds spells |
| 7. Combat Equip and Use, missing screens | [M77](milestone-77-plan.md) | |
| 8. Opening areas | [M78](milestone-78-plan.md) | Every Clouds map and travel path; Mountaineer and Pathfinder as DOS |
| 9. Services of the other towns | [M79](milestone-79-plan.md) | Each town with its original values |
| 10. Entrance scenes and the Arena | [M80](milestone-80-plan.md) | Pyramid and Darkside-key entrances wait for Darkside |
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

Two investigations shaped the order. A gap investigation (2026-10-19) found
that nearly every remaining Clouds blocker came from the fixed admission of
maps 23 and 28, so opening areas (step 8) came first and the Inn, which does
not block progress, later. A completion audit (2026-10-24) found that the
main quest could not finish: Lord Xeen's death gave no Scepter and the
ending was unsupported. A DOSBox run of the ending (maintainer, 2026-10-24)
showed the Scepter on the ordinary treasure screen, the Sixth Mirror's
message and the save of the live party at Vertigo before `XEEN.EXE` plays
the ending; steps 15-18 implemented it.

Step 19 left the Canvas/GS, PC speaker, Wave Blaster and TAP-10 music and
five digital drivers silent with a notice, their open questions recorded in
the [M94 plan](milestone-94-plan.md#scope). A switch between devices while
playing would be an opt-in approved deviation (maintainer, 2026-10-21).

The Darkside and World endings (`showdown`, `genesis`) wait for the
Darkside arc; M101 added the Pyramid.

## Next arc: release quality and the Darkside start

A next-steps analysis (2026-10-28), which merged two agent reports and a
community suggestion, chose to make the finished Clouds sturdier and easier
to test before Darkside begins. Each step is its own task or milestone, in
this order; steps are direction, not approved plans.

1. **Clouds fidelity and integration audit**, time-boxed, with findings
   turned into tests. It looks for defects between systems, of the kind the
   Spawn fix in release 0.10.0 found (58 containers that could be searched
   again and again, trap effects never drawn): invariant checks on the
   original data (a second use of every container, door and reward; Event
   changes reaching the frame drawn while the Event waits), Events during animation or combat, interrupted
   Events and keys pressed during waits, state after leaving, returning and
   loading, small or incapacitated parties and full packs, and differences
   from the pinned reference not yet recorded in
   [original quirks](original-quirks.md). Done: the audit ran on 2026-10-28.
   Release 0.10.1 fixed its save defect. [M95](milestone-95-plan.md) fixed
   its last finding and turned its invariants into tests.
2. **DOS save investigation**: whether original DOS saves can be read into
   MMModern and written from it, mainly so DOSBox and MMModern can start from
   the same state when collecting evidence, Darkside included. A converter
   follows only if the investigation shows it is worth it.
   Its first look found that DOS saves a picked chest as open;
   [M96](milestone-96-plan.md) added that, with save v22. The investigation
   finished on 2026-10-29: DOS loaded and re-saved a Clouds state that
   MMModern exported, so [M97](milestone-97-plan.md) added a developer
   exporter that also places the party on Darkside and prepares DOSBox.
   Its play-test showed that DOS marks a Cartographer party's cell when it
   loads a map; [M98](milestone-98-plan.md) made every arrival do that.
   Importing DOS saves waits for policy decisions.
3. **4:3 picture**: the 320x200 picture shown with the shape the original's
   4:3 monitors gave it. Done: [M99](milestone-99-plan.md) made 4:3 the
   default, with square pixels as an opt-in.
4. **Launcher and diagnostics**: the launcher shows the version, the detected
   data, the sound devices, the MT-32 ROMs and the save folder; the game
   writes a log file, and GitHub gets an issue template asking for the save
   and the log. Done: [M100](milestone-100-plan.md), which also lets the
   launcher make the original installer's sound choice through an
   MMModern-owned CFG, so that MT-32 can be chosen.
5. **Darkside start**: map loading, flags, quest items and save and load for
   both sides, with Darkside progress in the DOS save export as one of the
   first tasks, then Darkside's mechanics in order of
   [census](coverage.md#darkside-largest-gaps) impact.
   Started: [M101](milestone-101-plan.md) added both sides' state, save
   v23, the Darkside export and the original Pyramid between the sides, so
   that Darkside is reached in ordinary play. The [Darkside arc](#darkside-arc)
   continues it.

## Darkside arc

<a id="medium-term-darkside-and-world-of-xeen"></a>

A research pass (2026-10-31) compared every remaining Darkside gap in the
[census](coverage.md#darkside-largest-gaps) and the World of Xeen connections
with the pinned reference; its findings were checked against the code. Its
small defects were fixed at once (Temple Heal, Training, flag 118 falls, drop
field 7, missing Event lines, self-transfer). The rest is this order; steps
are direction, not approved plans. Each starts with a Part 0 of DOS program
analysis, because the reference was wrong in comparable Clouds cases (M80).

| Step | Scope | Tier | Notes |
| --- | --- | --- | --- |
| 0. Maintenance | Spawn onto Darkside outdoor maps and Skyroads (24 maps), refused by MMModern's own reachability check for slots parked off the map; outdoor wall 5 needs Mountaineer on Darkside, as DOS already showed; Ali Baba's chest lock recorded as a reference difference | B | Done (`5e4f685`), with DOS checks of the indoor ground beyond the map edge and the horizon, the Detect Monster picture and the Darkside mirror range and sound |
| 1. Darkside towns ([M102](milestone-102-plan.md)) | Bank, Smithy, Guild, Tavern and Inn, Temple and Training in the towns and Castle Kalindra 3 (map 43); the Tower, Dungeon and Dwarf town scenes (town actions 8, 9 and 13) | B | Done 2026-10-31 |
| 2. Darkside Events ([M103](milestone-103-plan.md)) | ExchObj, GotoRandom and GiveEnchanted for every item category; then ChooseNumeric and FallToMap; then ConfirmWord_2, MoveWallObj beyond the wall-item count and the map-29 IfMapFlag | B | Done 2026-11-01, with Event windows now closing at the end of every Event |
| 3. Mixed World of Xeen maps ([M104](milestone-104-plan.md)) | Darkside maps 113-128 (Darkstone Tower, Dragon Tower, Dungeon of Death, Southern Sphinx, Dragon Clouds), FlipWorld, the Darkside-key entrances and Town Portal from them | A | Done 2026-11-01, save v24 |
| 3a. Last mixed-map Events ([M105](milestone-105-plan.md)) | World flag 128 on map 116 and DisplayLarge on map 121 | B | Done 2026-11-01 |
| 4. Darkside and World wins, Darkside ending ([M106](milestone-106-plan.md)) | Opcodes 57 and 58, the ending mirror words on both sides, the Darkside and World completion words and won states, the title replay rows, and the Darkside presentation | A | Done 2026-11-02 |
| 5. World ending presentation ([M107](milestone-107-plan.md)) | The `XEEN.EXE` World ending after the win, `genesis` and the W replay | B | Done 2026-11-02 |

Three Tier A items wait for a fitting milestone. A runtime refusal after an
Event has shown a reward keeps that reward but drops the flags that stop a
repeat; DOS never stops mid-Event, so the whole Event should be discarded
(maintainer, 2026-10-31). A sweep of the original data found no Event that
reaches this today. And after a Xeen Slayer Sword attack, DOS lets the next
physical spell ignore physical resistance through the unsaved weapon
selector; MMModern does not yet. Finally, DOS draws the sky and the
Darkside entrance scenes from a night flag cached at map load and full time
advances, where MMModern reads the clock
([approved for M102](original-quirks.md#dev-current-clock-night)); the fix
starts by finding which time advances skip the refresh and whether play
reaches a scene or a sky redraw after one.

## After the Darkside audit

A Darkside fidelity and integration audit (2026-11-02) ran every Darkside
Event cell twice, with saves, loads, trips away and keys pressed during
waits. It found two hangs, fixed in `a6949b7` (a fall with a dead member and
a Space defeat on a flagged cell), and stale or missing documentation, fixed
in `8e52743`. The maintainer set this order for the rest (2026-11-02); steps
are direction, not approved plans.

1. **Maintenance round:** Darkside Create gives level 5, as DOS does (Tier B);
   a presentation error that cannot recover ends with a notice instead of
   blocking input (Tier A); the Event interpreter's instruction limit counts
   only since the last wait for the player, so the dials of maps 50, 76, 80
   and 122 turn as often as in DOS (Tier A); a check of the CD tracks that
   Darkside Events request, which the reference shifts by 30; and, in its
   play-test, a DOSBox check of the Quests screen's order when only Darkside
   items are held.
2. **Audit invariants as tests (M108, Tier B):** falls with a dead member, a
   defeat on a flagged cell, bounded repeats, exact restores, the crossing
   between the sides, loads after both wins, continuation keys after an
   Event and interrupted Event chains.
3. **Desert Paladin stone copies:** DOS program analysis of whether DOS
   hides the distant copies of a used stone on Darkside, which the reference
   does and MMModern does not ([details](original-quirks.md#sv-pin-data-patches)).
4. **Release** with all of the above; the maintainer sees no need for an
   earlier one.

## Later goals

Roughly in this order, each with its own decision and plan:

1. Automated builds and tests on GitHub: every push builds on a clean
   machine and runs the tests that need no original data, which also proves
   that the repository and [dependencies](dependencies.md) alone rebuild the
   project. Tests on the original data stay local, because that data cannot
   be published.
2. Optionally, a public announcement that World of Xeen plays from start to
   end, inviting players to report defects with the launcher's diagnostics
   and the issue template. The releases are already public.
3. Steam Deck through Proton, then a native Linux build and its distribution.
   A first attempt (2026-10-31) found that the build needs a POSIX platform
   layer (saves, settings, paths), a portable item-catalog generator and
   Linux object filtering for the ScummVM parts, not only narrow guards.
4. Saves that survive updates, when compatibility is revisited for 1.0.
5. A study of the community MM3 conversion
   ([mm345-conversion](https://github.com/hooby3dfx/mm345-conversion)), then
   mod support and modding tools.
6. The opt-in extras the README lists: game controller support, graphics
   filters and switching sound devices while playing.
7. Other language editions of the original.
8. **To evaluate:** whether to support Swords of Xeen, a standalone game on
   the same engine, made by the fan group Catware and later distributed by
   New World Computing. The maintainer decides whether it is included
   (2026-11-02). It keeps Darkside's formats in one archive (`swrd.cc`), and
   the pinned reference supports it as its own game type with about 70
   differences (item and quest tables, quest flags, the year, Town Portal,
   some spells and services, logo and credits). It would need a game profile
   and a save format change (Tier A), and its own analysis of `swords.exe`
   and its `xeen.dat`, which replace `WORLD/XEEN.EXE` and `XEEN.DAT` as DOS
   evidence.

## Review cadence

<a id="replanning-and-review-cadence"></a>

Revisit this roadmap after each milestone closes, using what play-testing showed.
Replan earlier if a milestone reveals that its scope is wrong, that whole-map
admission needs a missing foundation, or that maintainer priorities changed.
Roadmap approval does not authorize implementation; the maintainer starts each
milestone explicitly.
