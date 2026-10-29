# MMModern - Project History

One short entry per completed milestone, with its closing commit; Git has the
others. This is not the authority for current state: see
[project status](project-status.md) and the [roadmap](roadmap.md). Plans of
milestones 6 and 15-72 are kept in [docs/archive](archive/) as a historical
record; their limits and entry modes describe their time, not the present.

## M1-M43: certified slices

M1-M12 built the pre-public foundation: original-resource loading, outdoor
and indoor rendering, navigation, collision, party loading and the first
Event support; surviving history does not assign them to milestones. From
M13 each milestone admitted one narrow, certified slice.

| Milestone | Closing commit | Result |
| --- | --- | --- |
| M13 Public baseline | `c12d04d` | Event decoding and execution, automatic Events, conditions, teleports, game flags |
| M14 Manual interaction and original text | `275a0ca` | Space interaction, original text, fonts and windows, Yes/No, the SDL loop |
| [M15](archive/milestone-15-plan.md) Session-owned world mutations | `742b109` | Stable object and Event identity; Remove as session overlays |
| [M16](archive/milestone-16-plan.md) Outdoor objects and visual Remove | `484c8ce` | Static outdoor objects, refreshed on removal |
| [M17](archive/milestone-17-plan.md) Quest items and Phirna | `b59f39e` | Party quest-item counters; the Phirna Root collection |
| [M18](archive/milestone-18-plan.md) WhoWill and Bone Whistle | `32e6879` | Character selection with cancel and retry |
| [M19](archive/milestone-19-plan.md) NPC dialogue and Myra's request | `59d3016` | NPC portraits, paginated speech, quest flags |
| [M20](archive/milestone-20-plan.md) Save and resume | `a449bb7` | Versioned snapshots restored into the existing owners; F9 |
| [M21](archive/milestone-21-plan.md) Myra's exchange and item rewards | `bb524c0` | Full item records and the complete Myra loop |
| [M22](archive/milestone-22-plan.md) Outdoor object animation | `7f8a7d5` | Stationary outdoor objects animate |
| [M23](archive/milestone-23-plan.md) Indoor objects | `a6b0826` | Directional indoor art, wall occlusion, the Nightshadow gravestone |
| [M24](archive/milestone-24-plan.md) Inventory and transfer | `2c9ef4f` | Item catalog, inventory and transfer |
| [M25](archive/milestone-25-plan.md) Equipment | `b78398b` | Equip and remove with class, conflict and capacity rules |
| [M26](archive/milestone-26-plan.md) Actor approach | `49c2adf` | Outdoor monster wake-up, approach and engagement |
| [M27](archive/milestone-27-plan.md) Playable combat | `f63bc3d` | Attack and Block, injury, armor breakage, XP, defeat |
| [M28](archive/milestone-28-plan.md) Durable encounter completion | `d867b50` | A finished encounter saves and restores without replay |
| [M29](archive/milestone-29-plan.md) Mutable Journey | `45efc11` | Play continues after combat on the same owners |
| [M30](archive/milestone-30-plan.md) Expedition | `dfacac7` | Grouped combat, Disease, world-owned random continuation |
| [M31](archive/milestone-31-plan.md) First connected slice | `6faf45b` | Bone Whistle inside the expedition, saved across restart |
| [M32](archive/milestone-32-plan.md) Regional Journey | `5e15e1d` | Map-23 mainland with its 19 actors and the automatic sign |
| [M33](archive/milestone-33-plan.md) Mainland combat | `ac13999` | Ranged attacks, Shoot, Poison, Sleep, gold and equipment |
| [M34](archive/milestone-34-plan.md) Run and disengagement | `b200171` | Per-member Run, relocation and re-engagement |
| [M35](archive/milestone-35-plan.md) Myra quest and recovery | `58356a6` | The quest across mainland travel and the well |
| [M36](archive/milestone-36-plan.md) Exploration casting | `f8ba96e` | Spell books; First Aid and Awaken |
| [M37](archive/milestone-37-plan.md) Vertigo entry | `891b2a0` | Entrance, first cells, Slime combat, exit reset |
| [M38](archive/milestone-38-plan.md) Ironworks Armor Repair | `79c31a8` | Repair and the one-day departure |
| [M39](archive/milestone-39-plan.md) Combat casting | `0760a6b` | Magic Arrow, First Aid and Awaken in combat |
| [M40](archive/milestone-40-plan.md) Service days | `7d22ac0` | Stock generation, bank balances and interest |
| [M41](archive/milestone-41-plan.md) Training | `0c9877e` | Permanent levels from XP and gold |
| [M42](archive/milestone-42-plan.md) Equipment purchase | `b3b3679` | Weapons and Armor from generated stock |
| [M43](archive/milestone-43-plan.md) Temple Heal and resurrection | `c26a26b` | A 49-cell route to the Temple with original prices |

## M44-M64: whole maps and Vertigo

**M44 - Simplification** (`830af7e`). One save format and one Journey
configuration; legacy contracts and diagnostic modes removed; tests labelled
`fast`/`process` with digest-checked original-data scenarios.
[Plan](archive/milestone-44-plan.md).

**M45 - Reliable input** (`6794617`). Buffered keyboard input in exploration
and combat, and integrity guards that ended "Stale encounter frame" crashes
near map edges. [Plan](archive/milestone-45-plan.md).

**M46 - Clickable main screen** (`646ea1e`). Clicks on the main-screen and
combat buttons, targets and 3D view drive the existing actions.
[Plan](archive/milestone-46-plan.md).

**M47 - Original dialogs** (`d356466`). The original character sheet, items
dialog and Ironworks, Training and Temple screens, by mouse or key.
[Plan](archive/milestone-47-plan.md).

**M48 - Generic presentation** (`4d953c6`). Terrain, sky, water, wall item
and monster animation on every Clouds map; missiles, splats, portrait effects,
the combat strip, original keys and cursor. [Plan](archive/milestone-48-plan.md).

**M49 - Combat rules fidelity** (`804fc5a`). Monsters choose targets and
attack counts from their original data; damage applies on arrival; the party
can turn in combat. [Plan](archive/milestone-49-plan.md).

**M50 - Whole Vertigo** (`73739e7`). The whole town from the original
resources, all 46 monsters active, generic Event dispatch, Bash, locks and
traps, indoor Shoot; save v5. [Plan](archive/milestone-50-plan.md).

**M51 - Rest, food and daily time** (`cffb2c3`). Original Rest with food, the
dream and interruption; shared daily time with the eight-hour condition
schedule, dawn, night sky and year rollover; save v6.
[Plan](archive/milestone-51-plan.md).

**M52 - Normal start in Vertigo** (`a830248`). A new game starts from the
original initialization in Vertigo, for Adventurer and Warrior.
[Plan](archive/milestone-52-plan.md).

**M53 - CD edition data** (`37101c4`). The World of Xeen CD talkie edition
became the only reference data, with DOS text from `WORLD/XEEN.DAT` and CD
speech as deferred audio. [Plan](archive/milestone-53-plan.md).

**M54 - Original title menu and saves** (`a760e69`). The CD title screen,
New with slot, name and difficulty, and the original control panel with ten
slots under Local AppData; save v7. [Plan](archive/milestone-54-plan.md).

**M55 - Interface gaps** (`ef7ec84`). Strafe, Quick Reference, Info,
Exchange and Quick Fight with its Options dialog; save v8.
[Plan](archive/milestone-55-plan.md).

**M56 - Food and bank** (`cf4c9a9`). Tavern food and Bank deposits and
withdrawals on the original screens, with Event preludes before town
services. [Plan](archive/milestone-56-plan.md).

**M57 - Generic Event effects** (`d1f5459`). Events move objects and award
XP, awards, flags, currency and generated treasure; Joe's proof and
Gunther's reward work; save v9. [Plan](archive/milestone-57-plan.md).

**M58 - Skills and mapping** (`a39ec95`). One skill state per character,
teachers, Cartography, the automap and Map; save v10.
[Plan](archive/milestone-58-plan.md).

**M59 - Vertigo content** (`4d39d79`). Display cases and jail, the well,
city searches, Vern's membership, Guild entry and the portrait sparkle;
save v11. [Plan](archive/milestone-59-plan.md).

**M60 - Magic** (`6bc4779`). The original Cast dialog, Vertigo's Guild and
eight new spells; original program analysis settled spell resistance and
Pain. [Plan](archive/milestone-60-plan.md).

**M61 - Items for the loot** (`cbebb10`). Ironworks buy, sell, identify and
fix for every category, item Use, and treasure delivery as in DOS; the
`docs/rules` reference began; save v12. [Plan](archive/milestone-61-plan.md).

**M62 - Faithful screens** (`27f344a`). Event, NPC, WhoWill, combat-panel and
treasure screens on the original windows; animated shopkeepers; the title
and credits as in DOS. [Plan](archive/milestone-62-plan.md).

**M63 - Original clock and movement pace** (`5c40ffe`). One clock in BIOS
ticks drives every wait; the party walks about nine steps a second with no
step after release (approved deviation). [Plan](archive/milestone-63-plan.md).

**M64 - Vertigo closure and first release** (`5545471`). Play-through fixes,
then v0.1.0 with the 0.x save policy and v0.1.1 with the launcher.
[Plan](archive/milestone-64-plan.md).

## M65-M93: finishing Clouds

**M65 - Coverage census** (`1248d83`). A tool counts every mechanic the data
uses and its support status in the generated coverage report.
[Plan](archive/milestone-65-plan.md).

**M66 - Test-suite cleanup** (`613a566`). The complete suite went from 29
minutes to about 3, with 209 tests. [Plan](archive/milestone-66-plan.md).

**M67 - Equipped item contributions** (`11d3d72`). Metal, elemental and
slaying weapons and metal armor count in combat; release v0.1.2.
[Plan](archive/milestone-67-plan.md).

**M68 - Monster abilities** (`d32f9ce`). Every monster attack type, secondary
effect and movement kind as in DOS, with the original damage save sequence;
release v0.1.3. [Plan](archive/milestone-68-plan.md).

**M69 - Generic per-map state** (`f6074c2`). One owner keeps any map's
state, saved as deltas (save v13); monsters rebuild at full HP and asleep on
every load, as in DOS. [Plan](archive/milestone-69-plan.md).

**M70 - World Events, Clouds** (`c6c02ba`). Every Clouds Event operation
other than audio, town services and the ending runs as in DOS.
[Plan](archive/milestone-70-plan.md).

**M71 - Original item generation** (`b8ac99e`). Shops, Events, display cases
and monster drops share one generator with the original draws.
[Plan](archive/milestone-71-plan.md).

**M72 - Terrain and simple services** (`043586f`). Every surface's arrival
effect, Space passes time, and the Vertigo Tavern and Temple extras; save
v14. [Plan](archive/milestone-72-plan.md).

**M73 - Monster control spells** (`2f34569`). Monster statuses with the DOS
recovery draw, eight spells and the full item-power table.
[Plan](milestone-73-plan.md).

**M74 - Monster damage spells** (`ac4889b`). All 23 remaining damage spells,
checked first in DOS; Poison Volley shares the volley rule with Shoot.
[Plan](milestone-74-plan.md).

**M75 - Party healing and conditions** (`071d62f`). The 12 remaining
healing spells with their refunds; combat turn order as in DOS.
[Plan](milestone-75-plan.md).

**M76 - The last 22 spells** (`01c57e5`). Buffs, information, item and
travel spells, so all 76 Clouds spells work; Lloyd's Beacon saved (save v15).
[Plan](milestone-76-plan.md).

**M77 - Combat items, Quests, Awards** (`0c89957`). Items in combat as in
the original, the Quests and Awards screens and the skill indicators.
[Plan](milestone-77-plan.md).

**M78 - Opening areas** (`4d38594`). Every Clouds map opens with every
travel path; Mountaineer, Pathfinder and dark maps as DOS; save v16. A
maintenance fix (`bb22c52`) gave Run each map's own position and chance.
[Plan](milestone-78-plan.md).

**M79 - Services of the other towns** (`75a5829`). Every Clouds town service
with each place's DOS values. [Plan](milestone-79-plan.md).

**M80 - Entrance scenes and the Arena** (`041bc22`). The Reaper, Golem,
Dwarf Mine and Sphinx lead to their dungeons; the Warzone generates fights
as DOS does; "Please Wait..." omitted (approved deviation).
[Plan](milestone-80-plan.md).

**M81 - Monster treasure** (`f2bee4b`). Original monster drops in every kill
path; pending treasure lives in the running program, shared with Event
treasure, as in DOS; save v17. [Plan](milestone-81-plan.md).

**M82 - FM music and effects** (`3e9838a`). MMModern's reproduction of the
CD's Pro Audio Spectrum driver on an emulated OPL3, songs and 127 effect
calls, the sound buttons saved with the party (save v18).
[Plan](milestone-82-plan.md).

**M83 - Digital sound and CD speech** (`5b11b62`). Digital effects, voices
and CD speech mixed with FM, Text off, and output recovery; F8 replay
omitted by maintainer decision. [Plan](milestone-83-plan.md).

**M84 - Event dispatch on every input** (`fd5d8b5`). Every key or click runs
a flagged cell's Events again, as in DOS; the right button acts as the left
and keys follow the original table. [Plan](milestone-84-plan.md).

**M85 - Event calls after combat** (`60a8dc1`). An Event interrupted by a
monster runs after the fight, after Run too; "Nothing here" is acknowledged
first (approved deviation). [Plan](milestone-85-plan.md).

**M86 - Clouds presentation loose ends** (`b2bf700`). Open doors, the fall
animation and sounds, Moon Ray's order and service border indicators.
[Plan](milestone-86-plan.md).

**M87 - The Inn and a party of one to six** (`c45c112`). Sign In, the Inn
and Dismiss; any party size with DOS target draws; every roster record saved
(save v19); failed Inn saves retry (approved deviation).
[Plan](milestone-87-plan.md).

**M88 - Create and Delete at the Inn** (`fc6ae8a`). The original creation
screen and Delete, saving an empty party inside the Inn (save v20).
[Plan](milestone-88-plan.md).

**M89 - Clouds scripted kills and map rules** (`9d00723`). Lord Xeen and the
Scepter, the quest death hooks, Ice Trolls and the quest-done map changes.
Before it, a maintenance fix (`14fb64d`) read the packed flag banks in DOS
bit order.
[Plan](milestone-89-plan.md).

**M90 - Clouds verification sweep** (`87fe933`). Clouds plays from
Newcastle's Megacredits to Lord Xeen; the sweep found no content gap.
[Plan](milestone-90-plan.md).

**M91 - Cutscene presenter and the Clouds intro** (`a6b129c`). The Clouds
intro as `WORLD/XEEN.EXE` plays it, on a presenter that reads the installed
EXE. [Plan](milestone-91-plan.md).

**M92 - Clouds ending and score** (`2cdc98b`). Clouds can be finished: the
Sixth Mirror saves the live party, the score counts experience and play
time, and the EXE ending plays; save v21. [Plan](milestone-92-plan.md).

**M93 - Startup and the Darkside intro** (`3b0eb93`). Each launch plays the
Darkside story until watched, then the logos; Other Options `D` replays it.
[Plan](milestone-93-plan.md).

**M94 - Other sound devices** (`ea39c26`, `67d241f`, `6fc9a22`, `a12639e`).
`XEEN.CFG` selects the original drivers as the installer wrote them: AdLib,
Sound Blaster and PAS music on OPL2, MT-32 with the player's ROMs, Sound
Blaster digital; the other choices are silent with a notice.
[Plan](milestone-94-plan.md).

**M95 - Clouds audit follow-up** (`6647088`, `fcbc32d`). The Clouds audit
found one save defect, fixed in release 0.10.1. M95 makes the small map and
border show a waiting Event's changes, as in DOS. It also pins the audit's
invariants in original-data tests. [Plan](milestone-95-plan.md).
