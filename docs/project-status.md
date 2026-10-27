# MMModern - Project Status

This describes what can be played and done now. M93 is the latest completed
and accepted milestone (2026-10-27; [plan](milestone-93-plan.md)): each
launch opens with the original Darkside intro. M90 before it confirmed that
Clouds of Xeen can be played from Newcastle's Megacredits to Lord Xeen, M92
made Clouds finishable, with the original ending and score, and M91 added
the Clouds intro.
Earlier milestones are in [project history](project-history.md), direction is
in the [roadmap](roadmap.md), and build and dependency setup is in
[dependencies](dependencies.md).

MMModern aims to be a faithful reimplementation of the original games;
approved deviations are listed in
[original quirks](original-quirks.md#approved-deviations). It does not replace
them yet. A new game of Might and Magic IV: Clouds of Xeen, read from the
World of Xeen two-CD talkie edition, starts as in the original, and every
Clouds map is playable. Darkside gameplay is not supported.

## What can be played

Launch `mmodern <game-dir>` (see [Entry modes](#entry-modes)) to reach the
original title screen. Escape shows or hides the menu; Credits shows the four
original pages, and Other Options plays the Clouds intro with its music and
Crodo's narration (Escape skips it); after a win it also offers `V`, which
replays the Clouds ending. Each launch first plays the Darkside intro, as
the CD does: the full story until it has been watched to its end, then only
the logos and the World of Xeen title. Escape skips the story, and Other
Options `D` forgets that it was watched and plays it in full again. Whether
it was watched is kept in `settings.ini` beside the Clouds win.
Choosing New asks for a save slot, a name and the difficulty: Adventurer
(weapons hit harder) or Warrior. As in the original, the six level-1 Clouds
characters then stand in Vertigo at `(18,4)` facing West at 08:00 on day 1 of
year 610, with 800 gold, 10 gems, 90 food and an empty bank. Load continues a
saved game from the same slot list.

### The world

- Every Clouds map opens from the original resources, with its terrain,
  walls, objects, monsters and text. Walking across an outdoor border loads
  the next map, and larger dungeons span several original tiles. Every surface
  works as in the original: lava burns, sky and clouds drop the party to the
  land below with the original fall, space kills and the desert costs time.
  Doors show open for an instant as the party walks through. Water needs
  Swimming for the whole party or Walk on Water; mountains need Mountaineer
  and outdoor wall types 3 and 6 need Pathfinder, each from two members. Dark
  maps show a black view until Light clears it.
- Monsters wake, approach and attack on the original rules, with every
  attack, secondary effect and movement kind in the original data, ranged and
  flying included. Every map load rebuilds them at full HP and asleep.
- Every Event operation the Clouds scripts use runs as in the original,
  except the Pyramid, which says "not supported yet". As in DOS,
  a cell's Events run again after every key or click while the party stands
  on it, Space interacts and then waits, and an Event a monster interrupts
  runs once the fight ends ([Event dispatch](rules/world-events.md#event-dispatch)).
  "Nothing here" is acknowledged before such a fight
  ([approved deviation](original-quirks.md#dev-skipped-window-first)).
- The Reaper, Golem, Dwarf Mine and Sphinx entrance scenes play their
  original animations and speech, with captions when Efx is off. The Dwarf
  Mine always admits the party, the others need the right quest item, and the
  four entrances whose keys only Darkside gives always refuse.
- In the Warzone Arena the Battle Master offers fights of a chosen level and
  size. Each win adds to every member's Warzone award, up to 99, and the game
  cannot be saved in the Warzone.

### Combat

Combat starts on contact and is turn-based per member: Attack, Block, Run,
Cast, Use and Quick Fight. Shoot works in exploration, indoors too. Monsters
choose their targets and number of attacks from their original data, drawn as
DOS draws them, and the party can turn during combat. Equipped items count
with their metal, element and enchantment, and wounds, conditions, broken
armor, XP, gold and items stay on the party and the world. Equipping and
transferring items cost no turn; Use (U) uses a charged item and ends the
member's turn.

Every kill adds the gold and gems of the monster's original record and may
drop an item at its original chance and level (1 to 6). The treasure appears
on the original screens once no monster is in view and goes to the member
last chosen in a "who will" prompt; full packs lose or replace items as in
the original. Kills in the Warzone give none. Event treasure shares the
monster treasure, including the original's duplicate of pending items, and
undelivered items stay in the running program, as in DOS
([treasure rules](rules/items.md)).

Run is per member, with each map's original run position and chance. If the
whole party leaves, it lands there with its facing unchanged.

Only the Xeen Slayer Sword hurts Lord Xeen, each of its hits gets the
original's +100, and spells, Poison Volley and Shoot do nothing. His death
gives the Scepter on the ordinary treasure screen, ends his quest and gives
every active member, dead ones included, the "Defeated Lord Xeen" award. As
in DOS, killing the Clan King completes Gunther's dwarf quest, killing Count
Draco makes the Nightshadow well grant ten levels, Ice Trolls heal fully at
each new round, and leaving the lake with its three Water Dragons dead
completes Medin's quest. Completed quests hide their neighbouring-map
objects, Newcastle shows its castle once the walls are bought, and flag 25
drains the three caves ([combat rules](rules/combat.md#lord-xeen-and-scripted-death-hooks)).

### The ending

Giving the Scepter to the Sixth Mirror wins Clouds of Xeen as in DOS. The
game marks the party complete, moves it to Vertigo's tavern door, saves it
with its experience, deaths and awards, and computes the score from the
party's experience and the real play time. Then the original ending plays:
the castle's fall, Castle Burlock and the king, the parade of monsters and
the score, after which a fresh World title appears. Animations ignore keys
and clicks, the two score pages advance by key only, and Ctrl-Q quits. The
win is remembered in `%LOCALAPPDATA%\MMModern\settings.ini`, outside the
save ([approved deviation](original-quirks.md#dev-clouds-won-store)), and
unlocks Other Options `V`. After the win Castle Xeen is gone from the
Clouds map and Lloyd's Beacon cannot return there. The mirror word
`showtime` plays the ending without winning, and `bloopers` does nothing,
as in DOS ([ending rules](rules/cutscenes.md#clouds-ending-entry-and-save)).

### Magic

All 76 Clouds spells work from the learned book, Quick Fight and charged
items, in exploration and in combat where the original allows them, with the
original prompts, refusals, refunds and costs. Each town's Guild sells its
original selection. Attack spells fly to the first monster or group in
reach. Sleep, Hypnotize and the spells aimed at one kind of monster work as
in the original, and healing goes up to Resurrection and Divine Intervention.
Party protections show their border indicators and end at Rest or dawn.
Wizard Eye, Detect Monster and Identify Monster open the original screens,
Enchant, Recharge and Item to Gold work through Items, and Jump, Teleport,
Lloyd's Beacon, Super Shelter and Town Portal (all five towns) move the
party. The [magic rules](rules/magic.md) give each spell's effect, cost and
evidence.

### Characters and items

The original character sheet and Items dialog show stats with their popups
and let the party equip, remove, transfer, discard and use items. Each use of
a charged item costs a charge, and the item vanishes at zero. Exchange swaps
two members, Quick Reference opens the party summary, Quests shows quest
items, quests and Auto Notes, and Awards lists a member's awards.

The party holds any one to six roster characters in any order, and every rule
counts the members it has. D dismisses a member during exploration, never the
last; the next save lodges them in Vertigo, as in DOS. Sign In at the
Vertigo, Rivercity or Shangri-La tavern lodges the party there, adds a day,
saves the game and opens the original Inn with its song. The Inn lists the
characters lodged in that town: 1-4 add them up to six, R removes members and
F1-F6 open the sheet. Exit saves again and returns the party to the tavern
door, and loading a Sign In save reopens the Inn. Create opens the original
creation screen: the dice roll the seven attributes, the classes they allow
light up, two attributes can be exchanged, and Up and Down choose a free
roster slot with its portrait, race and sex. Create then asks for a name and
lodges the new level-1 character in that town. Delete removes a listed
character after confirmation, except one carrying the Xeen Slayer Sword.
Both save the game, also with nobody in the party, and loading such a save
reopens the Inn. As in DOS, a save loaded into the Inn from the title opens
it without music until Exit. Any key or click skips the Inn's scrolls and
the death scroll ([party rules](rules/party.md)).

Teachers and Events grant skills. Thievery picks locks, Bodybuilder and the
spell-casting skills raise HP or SP, Merchant sells at full value, Swimming
opens water and Navigator saves time in the desert; Arms Master and Tracker
do nothing, as in DOS. Cartography fills the 7x7 automap (`=`) and the
original 16x16 Map (M), and the border lights Danger Sense, Spot Secret Doors
and Direction Sense.

### Vertigo

Enter from mainland `(10,13)` facing North with Space and Yes. To leave, face
South at the entrance and use Space, which respawns and heals the city
monsters. Grates and doors break with Bash or open with a lock pick (the
original trap, Thievery roll and XP) and stay open. Myra's request, Joe's
proof box and Gunther's reward work end to end. Crates, trees, rubbish and
beds give items, gold or a fight once, and display cases reward Thievery or
send the party to jail. The well, Vern's membership, the magic mirror and the
teachers follow their original scripts.

Services use the original location screens, by mouse or keyboard:

- Ironworks `(8,4)`: Buy, Sell, Identify and Fix at the original prices,
  with stock generated and depleted as in the original.
- Training `(10,11)`: permanent levels from XP and gold, with the Vertigo
  level cap.
- Temple `(15,28)`: Heal and resurrection, Donation (which blesses the party
  when the count matches the day) and Uncurse.
- Tavern `(24,5)`: Food refills 15 food per active member for 10 gold;
  Drink, Tip and Rumors work, and Sign In opens the Inn.
- Bank `(26,17)`: deposit and withdraw gold or gems, with the original
  interest.
- Guild `(20,13)`: members buy spells or read Spell Info.

Each visit costs the original departure time, and merchant stock and bank
interest follow the original schedule.

### The other towns

The other towns' services use the same screens with each place's original
values. Rivercity has all six, with Training to level 15 and its own Food
price, stock, Temple prices, Guild catalogue, tips and rumors. Shangri-La has
a Temple, an Ironworks, a Tavern and a Guild selling every spell of the
class. Nightshadow, Asp and Winterkill each have a Guild. Newcastle has an
Ironworks and Training to level 20 on Level 1, and a Temple and the shared
Bank on Level 2; as in DOS, that Temple uses the last Temple prices of the
running program.

### Rest, food and time

Rest (R) sleeps eight hours, eats one food per member who can recover and
recovers HP and SP. Temporary effects end, the original warning and dream
appear, and without food nobody recovers. Rest is refused where the original
refuses it, and nearby monsters interrupt it. Conditions change on the
original eight-hour schedule, each dawn without rest can make members Weak,
the sky darkens at night and ages follow the year.

### Saving

Tab or the gem opens the original control panel. Save writes the current
slot, Save As picks a slot and name, Load replaces the running game and Quit
asks for confirmation. Saving is refused in combat, during service visits and
on maps that forbid it. Sign In, leaving the Inn, Delete and returning from
Create save automatically, as in DOS, and so does the ending; a failed
write shows the error and retries
([approved deviation](original-quirks.md#dev-inn-save-recovery)). The game
counts real play time from the system clock, as DOS does; typing G, O, B, R
in the panel shows the clock and the play time for that visit.
Mr Wizard returns the party to Vertigo and takes all its gems. The Efx,
Music, Speech and Text settings are saved with the party. Slots live under Local AppData
(`MMModern/Saves/`), and a damaged or foreign slot is never overwritten.
Loading continues exactly, without replaying any Event, reward, time or
random draw.

### Sound

The game plays the original FM music and effects as the CD installation's own
setup does: the Pro Audio Spectrum driver on an OPL3 chip, in stereo, with
each place's song and fades between songs. Digital effects, voices, entrance
speech and Event samples play, and CD speech reads the installed OGG tracks.
The panel's sound buttons work as in the original. Text off hides the text
that speech replaces and restores it when speech cannot play. An output
failure silences audio with one notice, and a removed device reopens the
default. A missing `WORLD/XEEN.CFG` refuses to start, as in the original
([audio rules](rules/audio.md)).

## Interface

[Controls](controls.md) lists every key. As in the CD original, either mouse
button makes the same click everywhere, and keys follow the original's
translation table. Terrain, sky, water, wall items, torches and monsters
animate from their own data. Missiles, splats, portrait damage and healing,
NPC speech, shopkeepers, treasure, the dream, the title and party death follow
the original sequences and timing. Keys pressed while the game is busy are
buffered in order, and held keys repeat as the keyboard does. A held movement
key steps about nine times a second and adds no step after release, an
approved deviation. Services, dialogs, inventory and casting accept only
fresh presses.

## Entry modes

All modes need a World of Xeen CD installation; the
[development guide](development.md#game-data-sources) says which data is read.
Original data is never copied or modified, and saves must not be written
inside the game directories.

| Command | Purpose |
| --- | --- |
| `mmodern <game-dir>` | Play the Darkside intro, then open the original title screen (New, Load, Credits, Other Options). |
| `mmodern --new-game <game-dir> [--difficulty adventurer\|warrior] [--save-file <path>]` | Developer: start a new game directly (default Adventurer). |
| `mmodern --journey-region [--combat-seed <u32>] <game-dir> [--save-file <path>]` | Prepared test mode at map 23 `(9,11)`; the automated scenarios use it. |
| `mmodern --load-game <game-dir> <save>` | Developer: continue a save file; it becomes the F9 target. |
| `mmodern --render-map <game-dir> [<map> <x> <y> <dir>]` | Explore any Clouds map for inspection. Cannot save. |
| `mmodern --monster-fixture <species> <game-dir> ...` | Developer: the prepared start with one monster replaced. Cannot save. |
| `mmodern --event-fixture <game-dir> <map> <x> <y> <dir>` | Developer: an isolated Event session on any Clouds map. Cannot save. |
| `mmodern --inspect-map\|--inspect-party\|--inspect-events <game-dir> ...` | Developer inspection output. |

The [development guide](development.md#entry-modes) gives every option.

## Save format

There is one current save format, v21. It holds the party with every roster
record in full, its sound options, play time and Clouds completion, whether
the party is inside the Inn (where it may be empty),
the world, and one state per visited map: monster positions and lifecycle,
moved objects, changed cells, disabled Events, explored cells and the Tavern
tip index. Monster HP and wake state are not saved, and neither is pending
treasure, as in the original. During 0.x, older formats are rejected before
anything is restored, newer or unrecognized saves are left alone, and F9
overwrites only a recognizable older MMModern save. Saves are tied to the
original archives by fingerprint, written atomically, and not compatible with
the original games or ScummVM. The Inn's saves and the Clouds ending's save
are the only autosaves.

## Architecture overview

Source is under `src/`: `core` (input, frame and installation types),
`formats/xeen` (original resource and save formats), `games/xeen` (rules and
state), `app` (coordination and presentation), `platform` (SDL window, audio
output, save file I/O, the `settings.ini` store and the system clock) and
`compat` (the linked ScummVM components).

- Party and roster (`XeenParty`, `XeenRoster`): all thirty roster
  characters, the active party of one to six, inventory, quest items, purse
  and books. Everything else borrows them.
- World (`XeenWorld`, `XeenSessionWorldState`): map and object caches, live
  actors, per-map state, economy, calendar and the world random state.
- Running program (`XeenProgramState`, created by `main`): what the original
  keeps outside its saves: the last Temple prices, pending treasure, the
  play-time start mark, the skipped-Event flag, whether the Inn song has
  started, and the current weapon and footstep sounds. It also holds the
  Clouds win and the Darkside intro's seen state read from `settings.ini`,
  and the audio owner `XeenAudio`, whose music, VOC and CD paths never
  change time, randomness or saves.
  `XeenWallClock` gives DOS `time()` seconds; only the player's title path
  binds the system clock, and every test and developer mode uses a fixed
  one.
- Rules (pure, in `games/xeen`): combat, monster approach and treasure,
  items, services, spells, terrain, Rest and time.
- Events: `XeenEventSystem` decodes and runs the original Event scripts.
  Unsupported effects refuse visibly without publishing anything partial.
- Cutscenes: `XeenCutscene` (pure segments) and `XeenCutsceneFlow` play
  the Darkside intro and the Clouds intro and ending of the CD's second
  program, `WORLD/XEEN.EXE`, with tables read from it at run time, the
  EXE's compressed frames and its own presentation random source. At launch
  `Application` chooses the Darkside intro's full or short path from the
  seen state.
- Flows (`app`): `XeenEventFlow` coordinates the gameplay session, input,
  dialogs, inventory and the Inn. `XeenEncounterFlow` hosts encounters,
  combat, services, casting and consequences, `XeenNavigationFlow` handles
  movement, and `Application` wires resources, SDL and the entry modes.
- Save (`XeenSaveState`, `XeenSaveFormat`, `XeenSaveFile`): captures a
  snapshot at a quiet boundary and restores it into fresh owners.
  `XeenRestoreGuard` and `XeenMutationWatch` stop stale work from publishing.

Presentation draws into a 320x200 indexed frame that SDL scales. Each frame
carries an identity, so input from a stale frame is dropped.

## Tests

CTest covers rules and synthetic fixtures, and original-data integration and
process tests that need a legally obtained installation. Tests are labelled
`fast` or `process`: use `ctest -L fast` while iterating and the complete
suite before closing a milestone. Four original-data scenarios compare their
final saves with approved digests and re-save byte-identically after a
reload. The generated [coverage report](coverage.md) counts every mechanic
the original data uses and its support status.

## Known gaps

- Scenes and commands: the Pyramid entrance and the mirror words
  `showdown` and `genesis` (the Darkside and World endings) say "not
  supported yet".
- Sound: other sound devices belong to step 19. F8 speech replay is
  omitted by maintainer decision. SDL's resampler plays FM and CD audio up
  to about 0.07% fast, which is inaudible
  ([audio rules](rules/audio.md#sdl-resampler-rate-limitation)).
- Darkside gameplay is not supported.

Next steps are in the [roadmap](roadmap.md).
