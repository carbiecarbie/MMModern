# MMModern - Project Status

This describes what can be played and done now. **M82 is the latest completed
and accepted milestone** ([plan](milestone-82-plan.md)): the game plays the
original FM music and sound effects, and the control panel's sound buttons
work. Every monster drops the treasure its original record gives (M81), the
entrance scenes and the Warzone Arena work (M80), every
Clouds town's services work (M79), and the whole of Clouds is open, with its
monsters, Events and every travel path (M78). Earlier
milestones are in [project history](project-history.md), direction is in the
[roadmap](roadmap.md), and build and dependency setup is in
[dependencies](dependencies.md).

MMModern aims to be a faithful reimplementation of the original games; any
approved deviation is noted where it applies. It is not yet a general
replacement for them. A **new game** of Might and Magic IV: Clouds of Xeen,
read from the World of Xeen two-CD talkie edition, starts as in the original.
Every Clouds map is playable. Darkside gameplay is not supported.

## What can be played

Launch `mmodern <game-dir>` (see [Entry modes](#entry-modes)) to reach the
original title screen. Escape shows or hides the menu; Credits shows the four
original pages, and Other Options lists the intros (not supported yet).
**New** asks for a save slot (confirming before replacing an occupied one), a
name and the difficulty: Adventurer (weapons hit harder) or Warrior. As in the
original, the six level-1 Clouds characters then stand in Vertigo at `(18,4)`
facing West at 08:00 on day 1 of year 610, with 800 gold, 10 gems, 90 food and
an empty bank. **Load** continues a saved game from the same slot list.

### The world

- **Maps.** Terrain, walls, objects, monsters and text come from the original
  resources. Every surface works as in the original: lava burns, sky and
  clouds drop the party to the land below, space kills and the desert costs
  time. Water needs Swimming for the whole party or Walk on Water. Mountains
  need Mountaineer and outdoor wall types 3 and 6 need Pathfinder, each from
  two members, as in DOS. Dark maps show a black view until Light or the
  party's remaining light clears it.
- **Areas.** Every Clouds map opens from the original data: walking across
  an outdoor border loads the next map, and larger dungeons span several
  original tiles as one map. Each map load rebuilds its monsters.
- **Monsters** wake, approach and attack on the original rules, with every
  attack, secondary effect and movement kind in the original data, including
  ranged attacks and flying. Every map load rebuilds its monsters at full HP
  and asleep, as the original does.
- **Events.** Every Event operation the Clouds scripts use runs as in the
  original, other than digital sound and CD speech, the Pyramid and the ending: signs, fountains, statues, NPC conversations, moved
  walls and objects, monster spawns, conditions, rewards, random Events,
  teleports, stairs, the magic mirror and password prompts. Those exceptions
  say "not supported yet" without changing anything.
- **Space** at an empty cell passes time and lets monsters act, as in DOS.
- **Entrance scenes.** The Reaper, Golem, Dwarf Mine and Sphinx play their
  original animations, with the speech silent for its original length.
  With the right quest item the party enters the dungeon behind them;
  without it the guardian refuses and the map reloads, as in the original.
  Four entrances lead to Darkside dungeons whose keys only Darkside gives,
  so they always refuse.
- **The Warzone Arena.** The Battle Master offers fights of a chosen level
  and size, generated as in the original; each win adds to every member's
  Warzone award, up to the original limit of 99. Leaving the Warzone
  discards an unfinished fight, and it cannot be saved there.

### Combat

Combat starts on contact and is turn-based per party member: Attack, Block,
Run, Cast, Use and Quick Fight. Shoot works in exploration, indoors
too. Monsters choose their targets and number of attacks from their original
data, and the party can turn during combat. Equipped weapons, armor, rings and
amulets count with their metal, element and enchantment. Wounds, conditions,
broken armor, XP, gold and generated items stay on the characters, the party
and the world.

**Treasure.** Every monster killed by melee, arrows, Shoot or a spell, alone
or in a group, adds the gold and gems of its original record and may drop an
item at its original chance and treasure level (1 to 6, accessories
included). Kills in the Warzone give no treasure. The treasure appears on the
original screens when the fight is over, or outside combat once no monster is
in view, and must be acknowledged page by page. Items go to the member last
chosen in a "who will" prompt when there is one. The original also forgets
that choice on every later key press on an Event cell and after some fights;
MMModern does not yet, an
[approved deviation](original-quirks.md#dev-whowill-event-passes) until
roadmap step 13 (Event dispatch). With full packs the original warning appears and items are
lost or replace the last one in their category, while gold and gems are still
credited. Event treasure shares the monster
treasure as in the original: an Event reward also hands over pending monster
items, and when monster gold is pending those items arrive twice, an original
bug kept on purpose. As in DOS, undelivered items are not saved and stay in
the running program across Load and New Game, where a later treasure can
deliver them.

**Run** is per member; the others keep fighting. Each map's original run
position and chance apply, indoors too. If the whole party leaves, it moves to
that position with its facing unchanged, and survivors keep their wounds.
Run says "not supported yet" when that cell has an automatic Event for the
party's facing.

In combat, equipping, removing, discarding and transferring items cost no
turn. The Use button (U, the potion) uses a charged item and ends the member's
turn; Town Portal from an item loads the town and ends the fight.

### Magic

All 76 Clouds spells work from the learned book, Quick Fight and charged
items, in exploration and in combat where the original allows them, with the
original prompts, refusals, refunds and costs. Each town's Guild sells its
original selection, and the original Cast dialog casts them. By theme:

- **Attack spells** fly forward to the first monster or group in reach, or up
  to four groups, with the original resistances and saving throws. A cast with
  no monster in reach still costs its SP and gems.
- **Monster control.** Sleep, Beast Master, Hypnotize and Dragon Sleep put
  monsters out of action until they recover; spells aimed at one kind of
  monster, such as Turn Undead or Insect Spray, leave the others untouched.
- **Healing and cures** restore HP, cure every condition and raise the dead,
  up to Resurrection and Divine Intervention; Create Food refills food.
- **Party protection.** Bless, Heroism, Holy Bonus, Power Shield and
  Protection from Elements strengthen the party and show their original
  border indicators. They, Levitate, Walk on Water, Clairvoyance and Light end
  at Rest or dawn.
- **Information.** Wizard Eye shows the surroundings on the automap,
  Clairvoyance lights its indicator, and Detect Monster and Identify Monster
  open the original screens.
- **Items.** Enchant Item, Recharge Item and Item to Gold work through the
  Items dialog.
- **Travel.** Jump, Etherealize, Time Distortion and Teleport move the party,
  crossing outdoor borders as walking does; each character keeps their own
  Lloyd's Beacon, Super Shelter sleeps where Rest is refused, and Town Portal
  reaches all five towns.

The [magic rules](rules/magic.md) give each spell's effect, cost and evidence.

### Characters and items

The original character sheet and Items dialog show stats with their popups and
let the party equip, remove, transfer, discard and use items. Charged items
cast their spell without spending time; each use costs a charge, even if the
target prompt is cancelled, and the item vanishes at zero. Exchange swaps two
members and Quick Reference opens the party summary. Quests shows quest items,
current quests and Auto Notes, and Awards lists a member's awards. Item lists
show Cursed in yellow and Broken in red, as the original does.

**Skills.** Teachers and Events grant skills, and the sheet lists them.
Thievery picks locks, Bodybuilder and the spell-casting skills raise HP or SP,
Merchant sells at full value, Swimming opens water and Navigator saves time in
the desert. Cartography records the cells the party steps on: `=` toggles the
7x7 automap, and M or the Map button opens the original 16x16 Map. The border
lights Danger Sense when a monster is within three cells and Spot Secret Doors
in front of a thin indoor wall, and shows the facing letter with Direction
Sense.

### Vertigo

Enter from mainland `(10,13)` facing North with Space and Yes; face South at
the entrance and use Space to leave. Leaving runs the town's original exit
script, which respawns and heals the city monsters.

- **Grates and doors.** Bash (B) breaks walls and grates; Space at a locked
  grate or door asks who will pick the lock, with the original trap, Thievery
  roll and XP. Opened barriers stay open across saves.
- **Quests.** Myra's request, with the Phirna Roots from the mainland and her
  antidote, Joe's proof box and Gunther's reward work end to end; rewards
  arrive once.
- **Searches and treasure.** Crates, trees, rubbish and beds give generated
  items, gold or a monster fight, as in the original; repeat searches find
  nothing. With full packs, items are lost or replace the last one in their
  category, as in the original.
- **Display cases and jail.** A member with Thievery takes the treasure; a
  member without it sends the party to jail with the Convicted Thief award
  and a one-year penalty.
- **The well, Vern's membership, the magic mirror** and the teachers (Mylo for
  Cartography, Rialdo for Pathfinder) follow their original scripts, including
  charging again for a repeat lesson or membership.

Services use the original location screens with the town art and text panel,
by mouse or keyboard:

- **Ironworks** `(8,4)`: Buy, Sell, Identify and Fix for every item category
  at the original prices; stock is generated and depleted as in the original.
- **Training** `(10,11)`: permanent levels from XP and gold, with the Vertigo
  level cap.
- **Temple** `(15,28)`: Heal and resurrection at original prices, Donation,
  which blesses the party when the count matches the day, and Uncurse.
- **Tavern** `(24,5)`: Food refills 15 food per active member for 10 gold;
  Drink, Tip and Rumors work as in the original. Sign In waits for the Inn.
- **Bank** `(26,17)`: deposit and withdraw gold or gems, typing up to ten
  digits; it pays the original interest.
- **Guild** `(20,13)`: members buy spells from Vertigo's original catalogue or
  read Spell Info; non-members are refused.

Each visit costs the original departure time, and merchant stock and bank
interest follow the original schedule.

### The other towns

The other towns' services use the same screens, with each place's original
values:

- **Rivercity** has all six: Training up to level 15, Food filling 60 rations
  per member for 50 gold, and its own Ironworks stock, Temple prices, Guild
  catalogue, tips and rumors.
- **Shangri-La** has a Temple with the highest prices, an Ironworks, a
  Tavern (120 rations per member for 100 gold) and a Guild that sells every
  spell of the class.
- **Nightshadow, Asp and Winterkill** each have a Guild with its own
  catalogue; each Guild admits only its own members.
- **Newcastle** has an Ironworks and Training up to level 20 on Level 1, and
  a Temple and the shared Bank on Level 2. As in DOS, Level 2's Temple uses
  the prices of the last Temple visited since the program started (none
  before any visit), across New Game and Load.

### Rest, food and time

- **Rest** (Rest button or R, outside combat): the party sleeps eight hours,
  eats one food per member who can recover and recovers HP and SP; temporary
  effects end. The original warning and dream appear, and without food time
  passes but nobody recovers. Rest is refused on the cells where the original
  refuses it.
- **Interruption.** Nearby monsters approach and attack while the party
  sleeps; the Rest stops without recovery.
- **Days and years.** Conditions change on the original eight-hour schedule,
  each dawn without rest can make members Weak, the sky darkens at night, and
  ages follow the year.

### Saving

Tab or the gem opens the original control panel when nothing else is open:
**Save** writes the current game's slot, **Save As** picks a slot and a new
name, **Load** replaces the running game with a saved one, and **Quit** asks
for confirmation. Saving is refused in combat, during service visits and on
maps that forbid it. **Mr Wizard** returns the party to Vertigo's run position
facing East and takes all its gems; in combat it says "not supported yet".
**Efx**, **Music**, **Speech** and **Text** switch as in the original and are
saved with the party; a new game starts with Text off. Slots live under Local AppData (`MMModern/Saves/`); a damaged or foreign
slot is shown as unavailable and never overwritten. Loading continues exactly,
without replaying any Event, reward, time or random draw.

### Sound

The game plays the original FM music and sound effects as the CD
installation's own sound setup does: the Pro Audio Spectrum driver on an OPL3
chip, in stereo, read from the installed data. Each map, town location, the
Inn and the title play their original song, fading between songs and
continuing when the next place uses the same one. Walking, doors, walls,
grates, combat, every spell, items, Events, services, Mr Wizard and death play
their original effects. Music off silences the music at once; Efx off silences
new effects, skips the wait for digital samples and shows the entrance-scene
captions instead; Speech off skips CD speech and turns Text on. Other sound
cards from the original installer are not supported yet. A missing
`WORLD/XEEN.CFG` refuses to start, as the original does.

## Interface

The [README](../README.md#controls) lists every key. Left clicks work as in
the original on the main-screen buttons, the combat buttons and targets, the
3D view, portraits and every dialog; the right button does nothing.

**Presentation.** Terrain, sky, water, wall items, torches and monsters
animate from their own data on every Clouds map. Missiles fly together, hits
show the original splats, portraits show damage and healing, and grants
sparkle on their recipients. NPC speech, shopkeepers, treasure, Shoot, the
dream, the title and party death follow the original sequences and timing.

**Input.** Keys pressed while the game is busy are buffered and applied in
order. A held movement key repeats at the original pace, about nine steps per
second, and releasing it adds no further step: an approved deviation from DOS,
which can add steps after release. Services, dialogs, inventory and casting
accept only fresh presses.

## Entry modes

All modes need a World of Xeen CD installation; the
[README](../README.md#game-data-sources) says which data is read. Floppy
archives are not used. Original data is never copied or modified, and saves
must not be written inside the game directories.

| Command | Purpose |
| --- | --- |
| `mmodern <game-dir>` | Open the original title screen (New, Load, Credits, Other Options). |
| `mmodern --new-game <game-dir> [--difficulty adventurer\|warrior] [--save-file <path>]` | Developer: start a new game directly (default Adventurer). |
| `mmodern --journey-region [--combat-seed <u32>] <game-dir> [--save-file <path>]` | Prepared Journey test mode: a prepared party at map 23 `(9,11)`; the automated scenarios use it. |
| `mmodern --load-game <game-dir> <save>` | Developer: continue a save file; it becomes the F9 target. |
| `mmodern --render-map <game-dir> [<map> <x> <y> <dir>]` | Explore any Clouds map for inspection. Cannot save. |
| `mmodern --monster-fixture <species> <game-dir> ...` | Developer: the prepared Journey with one F3 monster replaced by an original species. Cannot save. |
| `mmodern --event-fixture <game-dir> <map> <x> <y> <dir>` | Developer: an isolated Event session on any Clouds map. Cannot save. |
| `mmodern --inspect-map\|--inspect-party\|--inspect-events <game-dir> ...` | Developer inspection output. |

The [README](../README.md#entry-modes) gives every option.

## Save format

There is **one current save format**, v18. It keeps the party with its four
sound options, the world and
one generic state per visited map: monster positions and lifecycle, removed or
moved objects, changed cells, disabled Events, explored cells and the Tavern
tip index. Monster HP and wake state are not saved, because every map load
rebuilds them, and pending treasure is not saved, as in the original. During
0.x, older formats are not read: a save from an older build is rejected before
anything is restored, a save from a newer or unrecognized build is rejected and
left alone, and F9 overwrites only a recognizable older MMModern save. Saves are tied to the original archives by
fingerprint, are written atomically, and are not compatible with the original
games or ScummVM. There is no autosave.

## Architecture overview

Source is under `src/`: `formats/xeen` (original resource and save formats),
`games/xeen` (rules and state), `app` (coordination and presentation),
`platform` (SDL window, save file I/O) and `compat` (the selected ScummVM
components the project links).

Ownership, from state outward:

- **Party and roster** (`XeenParty`, `XeenRoster`): characters, inventory,
  conditions, quest items and flags, purse and the learned books. One owner
  for all thirty roster characters; everything else borrows it.
- **World** (`XeenWorld`, `XeenSessionWorldState`): map and object caches, the
  live actors, per-map state, merchant and bank economy, the calendar and the
  world random state.
- **Running program** (`XeenProgramState`, created by `main`): what the
  original keeps outside its saves, the last Temple prices and the pending
  treasure, each with its own revision; sessions borrow it. Next to it the
  audio owner (`XeenAudio`) holds the loaded music driver: a timeline
  `PromusDriver` answers the game on its clock, and a render driver in
  `XeenFmSource` follows the sound device through one request queue, so sound
  never changes time, randomness or saves.
- **Rules** (pure, in `games/xeen`): combat, monster approach and treasure,
  item generation, equipment, services, spells, terrain, Rest and time.
- **Events**: `XeenEventSystem` decodes and runs the original Event scripts;
  supported preludes publish once before terminal town services, and
  unsupported effects refuse visibly without publishing a partial prelude.
- **Flows** (`app`): `XeenEventFlow` is the gameplay session coordinator and
  presenter for input, dialogs and inventory. `XeenEncounterFlow` hosts
  encounters, combat, services, casting and consequences.
  `XeenNavigationFlow` handles movement. `Application` wires resources, SDL and
  the entry modes.
- **Save** (`XeenSaveState`, `XeenSaveFormat`, `XeenSaveFile`): captures a
  value snapshot at a quiet boundary and restores it into fresh owners before
  gameplay starts. `XeenRestoreGuard` and `XeenMutation` detect state that
  changed while work was prepared, so stale work cannot publish.
- **Journey content** (`XeenJourneyContent`): the immutable prepared start used
  by the test mode. Map admission comes from the original resources in
  `XeenWorld`, not from a list of maps.

Presentation draws into a 320x200 indexed frame that SDL scales. Presented
frames carry an identity so input from a stale frame is dropped.

## Tests

CTest covers rules and synthetic fixtures without original data, and
original-data integration and process tests that need a legally obtained
installation. Every test is labelled `fast` or `process`: use `ctest -L fast`
while iterating and the complete suite before closing a milestone. Four
original-data scenarios (mainland, Vertigo services, Temple and a Vertigo
round trip) compare their final saves with approved digests and re-save
byte-identically after a reload, guarding against unintended behavior
changes. The generated
[coverage report](coverage.md) counts every mechanic the original data uses
and its support status. Build and run instructions are in
[dependencies](dependencies.md).

## Known gaps

- **Inn and party.** There is no Inn, recruiting or character creation:
  Tavern Sign In and Dismiss say "not supported yet".
- **Town scenes.** The Pyramid entrance and the Clouds ending are not
  implemented; they say "not supported yet".
- **Skills.** Arms Master and Tracker
  are kept but do nothing; the reference gives them no effect either, and DOS
  has not been checked.
- **Sound.** There are no digital samples, voices or CD speech and tracks yet
  (M83): where the CD plays speech, a short notice appears, speech, shop voices,
  the dream and the death laugh keep their original duration silently, and
  Text off keeps text visible until speech plays. Only the CD installation's
  sound driver is supported; the original's other sound cards wait for roadmap
  step 16. Doors play their sound without the brief open-door picture, and
  falls and combat Mr Wizard are silent. If the Windows default output changes
  to another sample rate during play, the sound may crackle until restart.
- **Presentation.** There is no intro.
- **Darkside** gameplay is not supported.

Next steps are in the [roadmap](roadmap.md).
