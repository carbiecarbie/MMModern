# MMModern - Project History

One short paragraph per completed milestone, with the closing commit. This is
not the authority for current state: see [project status](project-status.md) and
the [roadmap](roadmap.md). Plans of milestones 6 and 15-72 are kept in
[docs/archive](archive/) as a historical record; their limits and entry modes
describe their time, not the present.

**M1-M12 - Pre-public foundation.** Original-resource loading, outdoor and indoor
rendering, navigation, collision, party loading and the first Event support.
Surviving history does not assign these to individual milestones.

**M13 - Public baseline** (`c12d04d`). First public codebase: Event decoding and
execution, automatic Events, conditions, teleports and game flags.

**M14 - Manual interaction and original text** (`275a0ca`). Space interaction,
original Event text, resumable dialogs and Yes/No, original fonts and windows,
and the SDL gameplay loop; Castle Basenji connected dialogue to teleport.

**M15 - Session-owned world mutations** (`742b109`). Stable object and Event
identity and Remove as session overlays that survive cache reconstruction.
[Plan](archive/milestone-15-plan.md).

**M16 - Outdoor objects and visual Remove** (`484c8ce`). Static outdoor objects
in the scene with immediate refresh when an object is removed.
[Plan](archive/milestone-16-plan.md).

**M17 - Quest items and Phirna** (`b59f39e`). Party-owned quest-item counters and
the original Phirna Root collection. [Plan](archive/milestone-17-plan.md).

**M18 - WhoWill and Bone Whistle** (`32e6879`). Character selection and the
Bone Whistle collection with cancel and retry. [Plan](archive/milestone-18-plan.md).

**M19 - NPC dialogue and Myra's request** (`59d3016`). Original NPC portraits and
paginated speech, quest flags and Myra's request. [Plan](archive/milestone-19-plan.md).

**M20 - Save and resume** (`a449bb7`). Versioned snapshots restored into the
existing owners, F9 on Windows and resume at startup, with resource checks.
[Plan](archive/milestone-20-plan.md).

**M21 - Myra's exchange and item rewards** (`bb524c0`). Full item records,
deterministic rewards and the complete Myra request, Phirna and return loop with
exact restart. [Plan](archive/milestone-21-plan.md).

**M22 - Outdoor object animation** (`7f8a7d5`). Ordinary outdoor objects animate
while stationary, with independent cosmetic timing. [Plan](archive/milestone-22-plan.md).

**M23 - Indoor objects** (`a6b0826`). Static indoor objects with original
directional art, wall occlusion and the Nightshadow gravestone interaction.
[Plan](archive/milestone-23-plan.md).

**M24 - Inventory and transfer** (`2c9ef4f`). Item catalog from original names,
inventory inspection and character-to-character transfer.
[Plan](archive/milestone-24-plan.md).

**M25 - Equipment** (`b78398b`). Equip and remove with class, conflict and
capacity rules and truthful feedback. [Plan](archive/milestone-25-plan.md).

**M26 - Actor approach** (`49c2adf`). Original outdoor monster identities, wake-up,
approach and engagement through the Flow and SDL path.
[Plan](archive/milestone-26-plan.md).

**M27 - Playable combat** (`f63bc3d`). Attack/Block combat with original
appearance, injury, armor breakage, XP and once-only defeat.
[Plan](archive/milestone-27-plan.md).

**M28 - Durable encounter completion** (`d867b50`). A finished encounter becomes
saveable and restores in a new process without replay.
[Plan](archive/milestone-28-plan.md).

**M29 - Mutable Journey** (`45efc11`). The first Journey: play continues after
combat on the same owners, with save and restart. [Plan](archive/milestone-29-plan.md).

**M30 - Expedition** (`dfacac7`). Grouped Skeleton and Zombie combat, Disease and
world-owned random continuation. [Plan](archive/milestone-30-plan.md).

**M31 - First connected slice** (`6faf45b`). Bone Whistle collection inside the
expedition with accumulated consequences saved across restart.
[Plan](archive/milestone-31-plan.md).

**M32 - Regional Journey** (`5e15e1d`). Map-23 mainland exploration with all 19
original actors and the automatic sign. [Plan](archive/milestone-32-plan.md).

**M33 - Mainland combat** (`ac13999`). Orc, Snake and Toad combat, enemy ranged
attacks, player Shoot, Poison, Sleep, XP, gold and generated equipment.
[Plan](archive/milestone-33-plan.md).

**M34 - Run and disengagement** (`b200171`). Per-member Run, casualties,
relocation to `(10,12)` and re-engagement with wounded survivors.
[Plan](archive/milestone-34-plan.md).

**M35 - Myra quest and recovery** (`58356a6`). The quest loop across mainland
travel, the selected well and the delivered antidote.
[Plan](archive/milestone-35-plan.md).

**M36 - Exploration casting** (`f8ba96e`). Learned spell books and First Aid and
Awaken in exploration. [Plan](archive/milestone-36-plan.md).

**M37 - Vertigo entry** (`891b2a0`). Entrance, first town cells, Slime combat and
the exit with the original monster reset. [Plan](archive/milestone-37-plan.md).

**M38 - Ironworks Armor Repair** (`79c31a8`). Repair for carried gold and the
one-day visit departure. [Plan](archive/milestone-38-plan.md).

**M39 - Combat casting** (`0760a6b`). Magic Arrow, First Aid and Awaken in combat.
[Plan](archive/milestone-39-plan.md).

**M40 - Service days** (`7d22ac0`). Repeated visits across days, merchant stock
generation, bank balances and interest. [Plan](archive/milestone-40-plan.md).

**M41 - Training** (`0c9877e`). Permanent levels from earned XP and gold, with
per-member service days. [Plan](archive/milestone-41-plan.md).

**M42 - Equipment purchase** (`b3b3679`). Buying plain Weapons and Armor from
generated stock with depletion. [Plan](archive/milestone-42-plan.md).

**M43 - Temple Heal and resurrection** (`c26a26b`). A 49-cell Vertigo route to the
Temple with original prices and a two-day paid departure.
[Plan](archive/milestone-43-plan.md).

**M44 - Simplification** (`830af7e`). One save format and one Journey
configuration; legacy contracts, save versions and diagnostic modes removed;
tests labelled `fast`/`process` with digest-checked original-data scenarios;
closed plans archived and docs rewritten. Game behavior unchanged.
[Plan](archive/milestone-44-plan.md).

**M45 - Reliable input and play stability** (`3ff28ec`, `6794617`). Buffered
keyboard input in exploration and combat (up to five presses, applied when the
game is ready, flushed on context changes), replacing silent dropping;
integrity guards admit maps and object files loaded on demand, ending "Stale
encounter frame" crashes near map edges. [Plan](archive/milestone-45-plan.md).

**M46 - Clickable main screen** (`646ea1e`). Left clicks on the original
main-screen buttons, combat buttons, targets and 3D view drive the existing
actions through the M45 input path; unimplemented buttons say so.
[Plan](archive/milestone-46-plan.md).

**M47 - Original dialogs** (`02214e1`, `d356466`). The original
character sheet and items dialog replace the project inventory, and the
Ironworks, Training and Temple use the original location screens and
dialogs, by mouse or key; dialog text comes from a pinned build-time
generator. [Plan](archive/milestone-47-plan.md).

**M48 - Generic presentation systems** (`6387a0e`, `b0e4919`, `4d953c6`).
Terrain, sky, water, wall items and monster animation from original data on
every Clouds map; simultaneous missiles, splats, portrait damage/healing
effects, Magic Arrow travel, the combat strip, original keys and cursor.
Presentation only; combat impact timing and rotation moved to M49.
[Plan](archive/milestone-48-plan.md).

**M49 - Combat rules fidelity** (`804fc5a`). Monsters choose targets and attack counts
from their original data for every record (the Slime now hits two members,
not the whole party); missile damage, death and rewards apply on arrival;
the party can turn during combat. Two M44 scenarios were re-routed off the
old defect and all three digests regenerated with every changed byte
explained. [Plan](archive/milestone-49-plan.md).

**M50 - Whole Vertigo** (`2cabd9a`, `ff91d0a`, `73739e7`). The whole town is
loaded from the original resources: certification and actor freezes removed,
all 46 monsters active, Events dispatched generically through guarded
publication, original Bash and grate/door unlocking with typed trap damage
and Thievery, indoor Shoot and enemy ranged attacks; save v5.
[Plan](archive/milestone-50-plan.md).

**M51 - Rest, food and daily time** (`b8856c1`, `55633c1`, `cffb2c3`).
Original Rest from the button or R with food, recovery, the dream and monster
interruption; shared daily time with the original eight-hour condition
schedule, dawn, night sky and year rollover replaced the old calendar limits;
food read from the correct offset; save v6. [Plan](archive/milestone-51-plan.md).

**M52 - Normal start in Vertigo** (`08178fe`, `a830248`). A new game starts
from the original initialization: the six level-1 characters in Vertigo at
`(18,4)`, day 1, 800 gold and 90 food, with all town actors in their original
state. Adventurer and Warrior both work. The command line temporarily replaces
the original title menu and difficulty dialog; the prepared Journey remains a
test mode. [Plan](archive/milestone-52-plan.md).

**M53 - CD edition data** (`69b82d0`, `2bbe975`, `37101c4`). The World of
Xeen CD talkie edition became the only reference data: archives read from the
GOG disc images or a CD copy, DOS interface text from `WORLD/XEEN.DAT`, floppy
fixed checks replaced by structural ones, and CD speech handled as deferred
audio. All accepted behavior was revalidated: the three M44 routes play
identically on CD data. [Plan](archive/milestone-53-plan.md).

**M54 - Original title menu and saves** (`4ab4762`, `11e8c81`, `a760e69`).
The game opens on the original CD title screen with Credits and Other Options;
New picks a slot, name and difficulty; the in-game control panel saves, saves
as, loads and quits as in DOS, with ten slots under Local AppData and the
original pulsing text cursor. Save v7 adds the slot name. M52's temporary
command-line entry is retired. [Plan](archive/milestone-54-plan.md).

**M55 - Interface gaps** (`9d51e42`, `9e4df22`, `ef7ec84`). The party can
strafe, Quick Reference shows the party summary and Info the date, time and
active effects, Exchange swaps two members and combat, Shoot, Rest, casting,
barriers and treasure follow the new order, and Quick Fight runs each
member's configured action with the original Options dialog, all by mouse and
keyboard. Save v8 stores each character's quick action and remembered spell.
[Plan](archive/milestone-55-plan.md).

**M56 - Food and bank** (`cf4c9a9`). The Tavern refills party food for the
original flat price, with full packs free; the Bank deposits and withdraws
gold or gems through the original screens and ten-digit amount entry.
Supported Event preludes run and publish once before terminal town services;
one-day departures apply scheduled interest to final balances without
rerolling stock. Save v8 and all M44 digests are unchanged. Maintainer play-test
accepted. [Plan](archive/milestone-56-plan.md).

**M57 - Generic Event effects** (`94007d0` and `d1f5459`). Events
can move and turn objects, award XP, awards, flags, gold and gems, and give
treasure through the same item generation and random draws as merchants. Joe's
proof and Gunther's reward work from the original scripts, searched crates
open and each tree pays once; the well refuses safely until it is implemented.
Save v9 stores all characters' awards and moved objects. Two display bugs
found in play-testing were fixed. Maintainer play-test accepted.
[Plan](archive/milestone-57-plan.md).

**M58 - Skills and mapping** (`a39ec95`). Each character has one skill state
from the original data, used by the sheet and the rules. Mylo and Rialdo teach
Cartography and Pathfinder through the original scripts, including Rialdo's
lower actual price and paid repeat lessons. Cartography records explored cells
for the original automap and Map. Save v10 stores skills and exploration; a
reusable digest audit tool now checks save-format changes.
[Plan](archive/milestone-58-plan.md).

**M59 - Vertigo content** (Part A `384eb13`, `5166cab`; Part B `4d39d79`). Display cases use
Thievery for treasure or a one-year jail penalty with ordinary walk-out; the
well damages or heals only the selected member. Generic city searches preserve
crate/bed appearance, actor identity and repeats. Vern charges membership per
character, and Guild entry uses that character's membership with original
art/text and deferred spell actions. Skill, award and XP grants show the
original portrait sparkle. Save v11 stores generic world flags and head-cache
overrides; Part A's explained M44 digest update was approved, and Part B and the
sparkle correction left the digests unchanged. Both parts and the corrected
feedback were accepted after maintainer play-testing on 2026-10-11.
[Plan](archive/milestone-59-plan.md).

**M60 - Magic** (`e7e7a62`, `6bc4779`). The original Cast dialog and New list
replace the project screens, and the Guild sells and describes Vertigo's 18
spells to members at the original prices. Eight new spells work through shared
projectile and casting rules: Wizard Eye, Shrapmetal, Energy Blast, Jump, Cure
Wounds, Flying Fist, Revitalize and Pain. Original program analysis settled
spell resistance and Pain. Jump refuses unimplemented terrain hazards. Save v11
unchanged. [Plan](archive/milestone-60-plan.md).

**M61 - Items for the loot** (`5e0542c`, `9cc4def`, `dc056ae`, `cbebb10`). The
Ironworks buys, sells, identifies and fixes every item category at prices taken
from the original program, and charged items cast their supported effects from
the character sheet without spending time. Analysis of the original program
corrected the reference: monsters' miscellaneous drops are delivered, full packs
keep gold and gems, and treasure delivery follows the original recipient and
replacement rules. A game rules reference (`docs/rules`) records the evidence.
Save v12 stores pending miscellaneous treasure. [Plan](archive/milestone-61-plan.md).

**M62 - Faithful screens and animation** (`5cc0079`, `41237e1`, `27f344a`). The
developer overlay and log-style combat text left normal play. Event, NPC,
WhoWill, combat-panel and treasure screens use the original windows, text and
input, the six shopkeepers animate, scenery animations include their final
frame, and the title and credits follow DOS. Timing-dependent parts are left
for M63. [Plan](archive/milestone-62-plan.md).

**M63 - Original presentation clock and movement pace** (`4f20b52`,
`5c40ffe`). One clock in original BIOS ticks drives every wait, and the party
walks about nine steps per second as in DOS, keeping every tap but adding no
step after a held key is released (approved deviation). NPC speech, shop
voices, treasure, sparkles, Shoot, the dream, the title and party death follow
the original sequences and timing; death returns to the title instead of
DEFEAT. Myra's original speaking draws changed the Mainland M44 digest
(approved). [Plan](archive/milestone-63-plan.md).

**M64 - Vertigo closure and first public release** (`c080eb1`, `080b05f`,
`5545471`). The maintainer's full play-through fixes: one sparkle per member
per Event run, as in DOS, original item names in equip refusals and a plain
window title. The first public releases followed: v0.1.0, a Windows zip with
a player README and the 0.x save policy, and v0.1.1, whose launcher finds and
remembers the GOG folder. [Plan](archive/milestone-64-plan.md).

**M65 - Coverage census** (`1248d83`). A read-only tool counts every mechanic
the World of Xeen data uses and asks the engine whether it is supported, with
a real game context; the generated coverage report lists totals, the largest
gaps and per-map readiness, and a calibration test pins Vertigo to its known
gaps. It found a real gap: equipped material or enchanted items are refused
before combat and Rest. [Plan](archive/milestone-65-plan.md).

**M66 - Test-suite cleanup** (`613a566`). The old M39-M43 witnesses stopped
waiting real time for deadlines of their simulated clock, slow tests were
shortened or split, twelve broken programs outside CTest were retired and
eleven passing ones joined it, and the runner went parallel. The complete
suite went from 29 minutes to about 3, with 209 tests instead of 195.
[Plan](archive/milestone-66-plan.md).

**M67 - Equipped item contributions** (`11d3d72`). Metal weapons add hit and
damage, elemental weapons add their element's damage against the monster's
resistance, slaying weapons triple damage against their kind, and metal armor
counts in combat; the old validation that refused these items before combat,
Shoot and Rest is gone. Drafted in a cloud session and finished on Windows
with the game data; released as v0.1.2. [Plan](archive/milestone-67-plan.md).

**M68 - Monster abilities** (`ff38fb2`, `d32f9ce`). Every monster attack type,
secondary effect and movement kind works as in DOS: the original damage save
sequence (which also corrected Poison), Break Weapon, Drain SP, Curse Item,
Eradicate and the rest, ranged elemental attacks, and flying monsters and the
Water Dragon crossing water by the original terrain rule. A developer fixture
places any original species on F3 for testing. The Shoot refusal and the
death scene were fixed on the way; released as v0.1.3.
[Plan](archive/milestone-68-plan.md).

**M69 - Generic per-map state** (`ea201f2`, `417a1c0`, `f6074c2`). One owner
keeps the state of any map, Vertigo and F3 included, and save v13 stores it as
deltas over the original resources. As in DOS, confirmed in `WORLD/XEEN.DAT`,
monsters are rebuilt at full HP and asleep on every map or game load. A fourth
M44 scenario travels Vertigo, F3 and back. Saves from 0.1.x are no longer
read. [Plan](archive/milestone-69-plan.md).

**M70 - World Events, Clouds** (`bf49eb3`, `a590e30`, `c8f1a19`, `c6c02ba`).
Every Event operation the Clouds scripts use, other than audio, town services
and the ending, runs as in DOS: moved walls and objects, cleared and altered
cells, monster spawns anywhere, conditions, values, rewards and random draws,
Gold Vein tiers, teleports, the magic mirror and password prompts. A developer
fixture starts an Event session on any Clouds cell. Save v13 is unchanged.
[Plan](archive/milestone-70-plan.md).

**M71 - Original item generation** (`697385f` plan, `b8ac99e`).
Shops, Event treasure, display cases and monster drops share one item
generator with the original draws: categories from 1, the level-7 selector,
weapon effectiveness and Misc charges 3 to 10. Monster drops lost their
duplicate generator. Tests that remembered one stock or one combat were
rewritten to check rules, with no growth in the suite; save v13 and v0.2.0
saves are unchanged. [Plan](archive/milestone-71-plan.md).

**M72 - Terrain and simple services** (`e21f398` plan, `043586f`).
Every surface has its original arrival effect (lava, falls from sky and
cloud, space, desert time) from every place the original applies it, and Rest
runs on any surface. Pressing Space now passes time and gives monsters their
turn, a gap since before M44. The Vertigo Tavern and Temple gained Drink, Tip,
Rumors, Donation and Uncurse; Sign In moved to the Inn step. Save v14 keeps
the Tavern tip index. [Plan](archive/milestone-72-plan.md).

**M73 - Monster control and first spell expansion** (`d58d097` plan,
`4792a3d`, `972256b`, `6724171`, `2f34569`). Monsters can hold one status:
Sleep, Beast Master, Hypnotize or Dragon Sleep stop them moving and
attacking, and they recover with the original DOS draw. Eight spells joined
(the four controls plus Insect Spray, Turn Undead, Sparks and Toxic Cloud),
from books, Quick Fight and items; the item-power table now has all 73
original entries. The play-test found a treasure-receipt crash present since
M63, now fixed with the first gameplay tests of monster item drops. Save v14
is unchanged. [Plan](milestone-73-plan.md).

**M74 - Monster damage spells** (`3ac560a` plan, `fe48d81`, `99a7518`,
`746acbd`, `ac4889b`). A static check of the DOS program first confirmed
all 23 remaining damage spells and kept its few differences. Every spell
that hurts monsters now works from books, Quick Fight and items: the All
range reaches up to four groups, Elemental Storm and Prismatic Light draw
their type once per cast, Moon Ray also heals, Finger of Death and Mass
Distortion follow their two saves, and Poison Volley fires one arrow per
member through a volley rule now shared by Shoot and combat. Party attacks
compare the hit total unsigned, as in DOS. Save v14 is unchanged.
[Plan](milestone-74-plan.md).

**M75 - Party healing and conditions** (`493f6f6` plan, `821ed91`,
`c12e551`, `071d62f`). A static check of the DOS program settled all 12
remaining healing spells before any code. Nature's Cure, Power Cure, the
four cures, the two suppressions, Raise Dead, Resurrection, Divine
Intervention and Create Food now work from books, Quick Fight and items,
with each spell's own refund on cancel and the DOS differences kept.
Combat turn order now follows DOS: a member woken or cured after their
turn was skipped waits for the next round. Save v14 is unchanged.
[Plan](milestone-75-plan.md).

**M76 - Buffs, information, item and travel spells** (`1bd6d5f` plan,
`b5748e5`, `14e8324`, `2566231`, `6cd1383`, `7f8b1e2`, `7c7d550`,
`01c57e5`). The last 22 spells work, so all 76 Clouds spells do: party
buffs that change combat and show on the border, Light, Levitate, Walk on
Water, Clairvoyance, Detect and Identify Monster, Enchant, Recharge and
Item to Gold, Super Shelter and the travel spells. A DOS check first
settled every rule; later checks fixed the party effect byte layout and
the Recharge bits. Ordinary Rest is refused on the cells where DOS refuses
it, and each character's Lloyd's Beacon is saved (save v15).
[Plan](milestone-76-plan.md).

**M77 - Combat items, Quests, Awards and skill indicators** (`e63502b`
plan, `3397294`, `2976c6e`, `672270b`, `c7c1828`, `0c89957`). A DOS check
first settled every rule. The party equips, removes, discards, transfers and
uses items in combat as in the original, and Town Portal from an item in
combat loads the town and ends the fight. The Quests and Awards screens
work, and the Danger Sense, Spot Secret Doors and Direction Sense indicators
light on the border. Play-test fixes followed DOSBox screenshots: the Town
Portal window, the combat Use click and the Cursed and Broken colors. Save
v15 is unchanged. [Plan](milestone-77-plan.md).

**M78 - Opening areas** (`612a4ac` plan, `a0872f1`, `48a2784`,
`4d38594`). A DOS check first settled Mountaineer, Pathfinder and the
dark view: each skill needs two members, Pathfinder crosses outdoor wall
types 3 and 6 (the reference has no such rule), and dark maps stay black
until the party has light. Every Clouds map now opens from the original data,
with outdoor borders, multi-tile dungeons, Event teleports and stairs, the
mirror, Town Portal's five towns, Lloyd's Beacon, the travel spells, Run, Mr
Wizard and falls. Other towns' services and the entrance scenes refuse
without changing anything. Save v16 changes only the version.
[Plan](milestone-78-plan.md).

**M79 - Services of the other towns** (`2632944` plan, `30d0580`,
`d3c607e`, `5a965ef`, `ae87434`, `75a5829`). A DOS check first settled each
place's values: Training caps 10, 15 and 20, the Ironworks stock shops, the
Guild catalogues and memberships, and Rivercity's Food, which fills 60
rations per member where the reference has 50. Every Clouds town service now
works: Rivercity's six, Shangri-La's four, the Guilds of Nightshadow, Asp and
Winterkill, and Newcastle's two levels. Newcastle Level 2's Temple keeps the
last Temple prices of the running program, as DOS does. Save v16 is
unchanged. [Plan](milestone-79-plan.md).

**M80 - Entrance scenes and the Arena** (`a7b9bc3` plan, `3d6b6ee`,
`b09dec7`, `4126973`, `81bc644`, `041bc22`). Two DOS checks and the
maintainer's DOSBox captures settled the site table, the timing and the
Arena rules first. The Reaper, Golem, Dwarf Mine and Sphinx now lead to the
Clouds dungeons they guard, refusing and reloading the map without their
key; the four whose keys come from Darkside always refuse. The Warzone
Battle Master generates fights as DOS does, which differs from the
reference, and awards each win. The "Please Wait..." box is omitted as an
approved deviation. Save v16 is unchanged. [Plan](milestone-80-plan.md).

**M81 - Monster treasure** (`29f5b30` plan, `cf256e1`, `be7e9bf`,
`22bc04a`, `b206193`, `831ec0c`, `83856b0`). A DOS check first settled the
drop table, the staging and what the original keeps outside its saves. Every
Clouds monster now drops the gold, gems and items of its original record, at
treasure levels 1 to 6 with accessories, in every kill path; the Warzone
gives none. Pending treasure lives in the running program, as in DOS: it is
not saved, and undelivered items survive Load and New Game. Event treasure shares it, keeping
the original's duplicate of monster items on purpose, and runs before
ordinary delivery; the "who will" recipient follows DOS, with an approved
deviation until Event dispatch is reworked. Save v17.
[Plan](milestone-81-plan.md).

**M82 - Audio, FM music and effects** (`61112d9` plan, `d5bba59`, `6792341`,
`a812944`, `946ac81`, `bcabfd6`, `3c4d7d0`, `6d71e82`, `088cfb0`, `57e161f`,
`82eee2b`, `861d607`, `3e9838a`). Static analysis of the CD program and its
sound driver, checked against the maintainer's DOSBox captures, came first.
The game now plays the original FM music and sound effects as the installed CD
setup does: MMModern's own reproduction of the Pro Audio Spectrum driver on an
emulated OPL3 chip, in stereo, without ScummVM's system layer. Songs follow
maps, locations and the title; 127 original effect calls play. Sound follows
the device while the game keeps its own clock, so it never changes time,
randomness or saves; listening found late effects and crackle, fixed by that
split and by opening the device at its own rate. The Efx, Music, Speech and
Text buttons work and are saved with the party (save v18), and Efx off shows
the entrance captions. Energy Blast now deals 2-6 times the caster's level, as
in DOS. [Plan](milestone-82-plan.md).

**M83 - Digital sound and CD speech** (`9affd5f`, `69816ac`, `af987a0`,
`aab321f`, `c1d0e83`; documentation audit `bc2536a` and the closure commit).
Accepted by the maintainer on 2026-10-22 after play-testing A2, B and C;
Part D changes nothing audible. Digital effects, service and entrance voices, the dream, death laugh and CD
speech now mix with FM using the installed digital driver's fixed transfer
layout. GOG cue sheets retain disc addresses and cross-file playback; Text off
hides the NPC body replaced by speech, with readable text when delivery fails.
Fixed source gains match the maintainer's DOSBox captures. Output failures
silence audio with one notice, and removal/default changes recover at the new
output rate without changing gameplay deadlines, RNG or saves (v18 unchanged).
F8 replay and resource-read CD restarts are omitted by maintainer decision.
[Plan](milestone-83-plan.md).

**M84 - Event dispatch on every input** (plan `d13ced5` with amendments
`10260af`, `2bbd262`; Part 0 `ee9fe24`, A1 `7a6ed85`, A2 `9e6fdd5` and the
closure commit). Accepted by the maintainer on 2026-10-23 after play-testing
Part A. As in DOS, every key or click in exploration ends with one pass that
runs a flagged cell's Events again, even with no matching record: traps hurt
again, labels return and WhoWill clears; Space and a 3D-view click run them
twice, and Load and New Game start with one pass. The key an Event leaves
decides what follows, so an NPC or WhoWill answer skips Space's Wait; this
replaced all four M44 digests with approval. Part 0 also showed that the CD
game treats the right mouse button as the left, confirmed in DOSBox, and the
keyboard now follows the original's translation table (W waits, keypad 4/6
turn). Save v18 is unchanged; the post-combat calls wait for the second
milestone. [Plan](milestone-84-plan.md).

**M85 - Event calls after combat** (plan `1d2b92d` with amendment
`70e152c`; Part 0 `15ea744`, Part A `e3066f0` and the closure commit).
Accepted by the maintainer on 2026-10-23 after play-testing Part A. As in
DOS, an Event call that finds a monster at the party before one of its lines
stops and is remembered; when the fight ends, a door label that was up comes
back and the interrupted Event runs at the party's position, after Run too.
Run therefore no longer refuses landings with an Event, and every Event run
clears the "who will" recipient. "Nothing here" is acknowledged before such a
fight, an approved deviation from DOS, which starts it under the window.
Treasure after a win still waits while monsters are in view. Save v18 and
all M44 digests are unchanged. [Plan](milestone-85-plan.md).

**M86 - Clouds presentation loose ends** (plan `4542db9`, implementation
`661fdcc` and the closure commit). Accepted by the maintainer on 2026-10-23
after the play-test. As in DOS, walking through a door shows it open for an
instant, a fall from the sky or the clouds plays the original drop with the
scream and the landing sounds before its damage, Moon Ray heals and sparkles
one member at a time, and the border indicators restart when a service
screen opens. FallToMap, used only in Darkside, stays unsupported. Save v18
and all M44 digests are unchanged. [Plan](milestone-86-plan.md).

**M87 - The Inn and a party of one to six** (plan `f77720f`, Part 0
`4ed3a15`, Part A `dfa3788`, Part B `4182202` and the closure commit).
Accepted by the maintainer on 2026-10-24 after the play-test. As in DOS,
Sign In at the Vertigo, Rivercity or Shangri-La tavern lodges the party,
adds a day, saves the game and opens the original Inn, where lodged
characters join and leave; Exit saves again and reloads the map at the
tavern door, and a save made at Sign In reopens the Inn. Dismiss works in
exploration, and the next save lodges the dismissed member in Vertigo. The
game plays with any one to six characters, and monster targets follow the
DOS count-dependent draws, which replaced the four M44 digests after an
approved audit. Every roster record is saved in full (save v19). A failed
Inn save retries instead of quitting (approved deviation), and the death
scroll gained the DOS skip. Create and Delete wait for M88.
[Plan](milestone-87-plan.md).

**M88 - Create and Delete at the Inn** (plan `e862107`, Part 0 `b54f893`,
Part A `eba589d`, Part B `d8168c4` and the closure commit). Accepted by the
maintainer on 2026-10-24 after the play-test. As in DOS, the Inn's Create
opens the original creation screen: the dice roll the attributes, the
classes they allow light up, attributes can be exchanged, and each free
roster slot brings its portrait, race and sex; the new character is named
and lodged in that town. Delete removes a listed character after
confirmation. Both save the game, also with an empty party, which save v20
admits inside the Inn; the four M44 digests changed only in the version
byte, after an approved audit. A save loaded into the Inn from the title
stays silent until Exit, following DOS's Inn song flag, and the rolls use
the Event interpreter's range draw (maintainer decision).
[Plan](milestone-88-plan.md).
