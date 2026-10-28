# MMModern - Project Status

This describes what can be played and done now. M94 is the latest completed
and accepted milestone (2026-10-28; [plan](milestone-94-plan.md)): the
original AdLib, Sound Blaster and MT-32 drivers play when `XEEN.CFG`
selects them. M93 before it opened each launch with the original Darkside
intro, M90 confirmed that Clouds of Xeen can be played from Newcastle's
Megacredits to Lord Xeen, M92 made Clouds finishable, with the original
ending and score, and M91 added the Clouds intro.
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

`mmodern <game-dir>` first plays the Darkside intro, as the CD does: the full
story until it has been watched to its end, then only the logos and the World
of Xeen title. Escape skips the story. The original title screen follows:
Escape shows or hides the menu, Credits shows the four original pages, and
Other Options plays the Clouds intro with its music and Crodo's narration,
`D` forgets that the Darkside story was watched, and after a win `V` replays
the Clouds ending. The watched state and the win are kept in `settings.ini`.
New asks for a save slot, a name and the difficulty: Adventurer (weapons hit
harder) or Warrior. As in the original, the six level-1 Clouds characters
then stand in Vertigo at `(18,4)` facing West at 08:00 on day 1 of year 610,
with 800 gold, 10 gems, 90 food and an empty bank. Load continues a saved
game from the same slot list.

### The world

- Every Clouds map opens from the original resources, with its terrain,
  walls, objects, monsters and text. Outdoor borders load the next map, and
  larger dungeons span several original tiles. Every surface works as in the
  original: lava burns, sky and clouds drop the party to the land below, space
  kills and the desert costs time. Water needs Swimming for the whole party
  or Walk on Water; mountains need Mountaineer and outdoor wall types 3 and 6
  need Pathfinder, each from two members. Dark maps stay black until Light.
- Monsters wake, approach and attack on the original rules, with every
  attack, secondary effect and movement kind in the original data, ranged and
  flying included. Every map load rebuilds them at full HP and asleep.
- Every Event operation the Clouds scripts use runs as in the original,
  except the Pyramid, which says "not supported yet". As in DOS, a cell's
  Events run again after every key or click while the party stands on it,
  Space interacts and then waits, and an Event a monster interrupts runs once
  the fight ends ([Event dispatch](rules/world-events.md#event-dispatch)).
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
choose their targets and attacks from their original data, drawn as DOS
draws them, and the party can turn during combat. Equipped items count with
their metal, element and enchantment; wounds, conditions, broken armor, XP,
gold and items stay on the party and the world. Equipping and transferring
cost no turn; Use (U) uses a charged item and ends the member's turn. Run is
per member, with each map's original run position and chance.

Every kill adds the monster's original gold and gems and may drop an item at
its original chance and level. The treasure appears on the original screens
once no monster is in view and goes to the member last chosen in a "who will"
prompt; full packs lose or replace items as in the original. Warzone kills
give none. Event treasure shares the monster treasure, including the
original's duplicate of pending items ([treasure rules](rules/items.md)).

Only the Xeen Slayer Sword hurts Lord Xeen, with the original's +100 per hit.
His death gives the Scepter and every active member the "Defeated Lord Xeen"
award. Scripted deaths work as in DOS: the Clan King completes Gunther's
quest, Count Draco's death makes the Nightshadow well grant ten levels, Ice
Trolls heal at each new round, and the three dead Water Dragons complete
Medin's quest. Completed quests hide their neighbouring-map objects, and
Newcastle shows its castle once the walls are bought
([combat rules](rules/combat.md#lord-xeen-and-scripted-death-hooks)).

### The ending

Giving the Scepter to the Sixth Mirror wins Clouds of Xeen as in DOS. The
game moves the party to Vertigo's tavern door, saves it, and computes the
score from experience and real play time. Then the original ending plays:
the castle's fall, Castle Burlock and the king, the parade of monsters and
the score, after which a fresh World title appears. Animations ignore keys
and clicks, the two score pages advance by key only, and Ctrl-Q quits. The
win is remembered outside the save
([approved deviation](original-quirks.md#dev-clouds-won-store)). Afterwards
Castle Xeen is gone and Lloyd's Beacon cannot return there. The mirror word
`showtime` plays the ending without winning, and `bloopers` does nothing, as
in DOS ([ending rules](rules/cutscenes.md#clouds-ending-entry-and-save)).

### Magic

All 76 Clouds spells work from the learned book, Quick Fight and charged
items, in exploration and in combat where the original allows them, with the
original prompts, refusals, refunds and costs. Each town's Guild sells its
original selection. Attack spells fly to the first monster or group in
reach; Sleep, Hypnotize and the spells aimed at one kind of monster work as
in the original, and healing goes up to Resurrection and Divine Intervention.
Party protections show their border indicators and end at Rest or dawn.
Wizard Eye, Detect Monster and Identify Monster open the original screens,
Enchant, Recharge and Item to Gold work through Items, and Jump, Teleport,
Lloyd's Beacon, Super Shelter and Town Portal move the party. The
[magic rules](rules/magic.md) give each spell's effect, cost and evidence.

### Characters and items

The original character sheet and Items dialog show stats with their popups
and let the party equip, remove, transfer, discard and use items; a charged
item vanishes at zero charges. Exchange swaps two members, Quick Reference
opens the party summary, Quests shows quest items, quests and Auto Notes, and
Awards lists a member's awards.

The party holds one to six roster characters in any order, and every rule
counts the members it has. D dismisses a member, never the last; the next
save lodges them in Vertigo, as in DOS. Sign In at the Vertigo, Rivercity or
Shangri-La tavern adds a day, saves and opens the original Inn with its song.
There 1-4 add lodged characters, R removes members, F1-F6 open the sheet and
Exit saves and returns to the tavern door. Create rolls the seven attributes
on the original screen, lights the classes they allow, exchanges two
attributes, picks a free roster slot and lodges the named level-1 character.
Delete removes a character after confirmation, except one carrying the Xeen
Slayer Sword. Loading an Inn save reopens the Inn, without music when loaded
from the title, as in DOS ([party rules](rules/party.md)).

Teachers and Events grant skills. Thievery picks locks, Bodybuilder and the
spell-casting skills raise HP or SP, Merchant sells at full value, Swimming
opens water and Navigator saves time in the desert; Arms Master and Tracker
do nothing, as in DOS. Cartography fills the 7x7 automap (`=`) and the
original 16x16 Map (M), and the border lights Danger Sense, Spot Secret Doors
and Direction Sense.

### Towns and services

Enter Vertigo from mainland `(10,13)` facing North with Space and Yes; leave
facing South at the entrance, which respawns and heals the city monsters.
Grates and doors break with Bash or open with a lock pick and stay open.
Myra's request, Joe's proof box, Gunther's reward, the crates, trees, beds,
display cases, well, mirror and teachers follow their original scripts.

Services use the original location screens, by mouse or keyboard, with each
place's original values: the Ironworks buys, sells, identifies and fixes with
generated stock; Training grants levels up to the town's cap; the Temple
heals, resurrects, uncurses and blesses donations on the right day; the
Tavern sells food and drinks, takes tips, tells rumors and signs in; the Bank
keeps gold and gems with the original interest; and the Guild sells spells
to members. Vertigo and Rivercity have all six; Shangri-La has a Temple,
Ironworks, Tavern and a Guild selling every spell of the class; Nightshadow,
Asp and Winterkill each have a Guild; Newcastle has an Ironworks and Training
to level 20 on Level 1 and a Temple and the shared Bank on Level 2. As in
DOS, Newcastle's Temple uses the last Temple prices of the running program.
Each visit costs the original departure time, and stock and interest follow
the original schedule.

### Rest, food and time

Rest (R) sleeps eight hours, eats one food per member who can recover and
recovers HP and SP. Temporary effects end, the original warning and dream
appear, and without food nobody recovers. Rest is refused where the original
refuses it, and nearby monsters interrupt it. Conditions change on the
original eight-hour schedule, each dawn without rest can make members Weak,
the sky darkens at night and ages follow the year.

### Saving

Tab or the gem opens the original control panel: Save, Save As, Load and
Quit. Saving is refused in combat, during service visits and on maps that
forbid it. Sign In, leaving the Inn, Delete, Create and the ending save
automatically, as in DOS; a failed write shows the error and retries
([approved deviation](original-quirks.md#dev-inn-save-recovery)). Play time
follows the system clock, as in DOS; typing G, O, B, R in the panel shows it.
Mr Wizard returns the party to Vertigo and takes all its gems. Sound and text
settings are saved with the party. Slots live under Local AppData
(`MMModern/Saves/`), and a damaged or foreign slot is never overwritten.
Loading continues exactly, without replaying any Event, reward, time or
random draw.

### Sound

The game plays the original FM music and effects as the CD installation's own
setup does: the Sound Blaster Pro driver on an OPL3 chip, in stereo, with
each place's song and fades between songs. Digital effects, voices, entrance
speech and Event samples play, and CD speech reads the installed OGG tracks.
The panel's sound buttons work as in the original, and Text off restores text
when speech cannot play. An output failure silences audio with one notice,
and a removed device reopens the default. A missing `WORLD/XEEN.CFG` refuses
to start, as in the original ([audio rules](rules/audio.md)).

The sound devices follow `WORLD/XEEN.CFG` as the original installer wrote
it, in both the game and the intros. Music can also come from the AdLib,
Sound Blaster and Pro Audio Spectrum drivers on an OPL2 chip, or from the
MT-32 driver on an emulated MT-32 with the player's MT-32 1.07 control and
PCM ROMs in `%LOCALAPPDATA%\MMModern\roms`. Digital sound can also come from
the Sound Blaster driver. The other installer choices play silently with one
"not supported yet" notice, and missing MT-32 ROMs give one notice and
silence; the game's timing is the same either way.

## Interface

[Controls](controls.md) lists every key. As in the CD original, either mouse
button makes the same click, and keys follow the original's translation
table. Terrain, sky, water, wall items, torches and monsters animate from
their own data. Missiles, splats, portrait effects, NPC speech, shopkeepers,
treasure, the dream, the title and party death follow the original sequences
and timing. Keys pressed while the game is busy are buffered in order, and
held keys repeat as the keyboard does. A held movement key steps about nine
times a second and adds no step after release, an approved deviation.
Services, dialogs, inventory and casting accept only fresh presses.

## Entry modes

`mmodern <game-dir>` is the player's entry. Developer modes start a new game
directly, open the prepared test region at map 23 `(9,11)` used by the
automated scenarios, load a save file, explore or inspect any Clouds map, and
run isolated monster or Event fixtures; the
[development guide](development.md#entry-modes) gives every option. All modes
need a World of Xeen CD installation
([game data sources](development.md#game-data-sources)); original data is
never copied or modified, and saves are never written inside the game
directories.

## Save format

There is one current save format, v21. It holds the party with every roster
record in full, its sound options, play time and Clouds completion, whether
the party is inside the Inn (where it may be empty), the world, and one state
per visited map: monster positions and lifecycle, moved objects, changed
cells, disabled Events, explored cells and the Tavern tip index. Monster HP
and wake state are not saved, and neither is pending treasure, as in the
original. During 0.x, older formats are rejected before anything is
restored, newer or unrecognized saves are left alone, and F9 overwrites only
a recognizable older MMModern save. Saves are tied to the original archives
by fingerprint, written atomically, and not compatible with the original
games or ScummVM.

## Architecture overview

Source is under `src/`: `core` (input, frame and installation types),
`formats/xeen` (original resource and save formats), `games/xeen` (rules and
state), `app` (coordination and presentation), `platform` (SDL window, audio
output, save file I/O, `settings.ini` and the system clock) and `compat` (the
linked ScummVM components).

- Party and roster (`XeenParty`, `XeenRoster`): all thirty roster
  characters, the active party, inventory, quest items, purse and books.
- World (`XeenWorld`, `XeenSessionWorldState`): map and object caches, live
  actors, per-map state, economy, calendar and the world random state.
- Running program (`XeenProgramState`): what the original keeps outside its
  saves, such as the last Temple prices, pending treasure, the play-time mark
  and the skipped-Event flag; the `settings.ini` values; and the audio owner
  `XeenAudio`, which never changes time, randomness or saves.
  `XeenWallClock` gives DOS `time()` seconds; only the player's title path
  binds the system clock.
- Rules (pure, in `games/xeen`): combat, monsters, treasure, items, services,
  spells, terrain, Rest and time.
- Events: `XeenEventSystem` decodes and runs the original Event scripts;
  unsupported effects refuse visibly without publishing anything partial.
- Cutscenes: `XeenCutscene` and `XeenCutsceneFlow` play the Darkside intro
  and the Clouds intro and ending of `WORLD/XEEN.EXE`, with tables, frames
  and a presentation random source read from it at run time.
- Flows (`app`): `XeenEventFlow` coordinates the gameplay session, input,
  dialogs, inventory and the Inn; `XeenEncounterFlow` hosts encounters,
  combat, services, casting and consequences; `XeenNavigationFlow` handles
  movement; `Application` wires resources, SDL and the entry modes.
- Save (`XeenSaveState`, `XeenSaveFormat`, `XeenSaveFile`): captures a
  snapshot at a quiet boundary and restores it into fresh owners;
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
- Sound: the Canvas/GS, PC speaker, Wave Blaster and TAP-10 music
  drivers, and the PC speaker, AdLib, Disney Sound Source, Pro Audio
  Spectrum and Covox digital drivers, are silent with a notice.
  There is no device switch while playing. F8 speech replay is
  omitted by maintainer decision. SDL's resampler plays FM and CD audio up
  to about 0.07% fast, which is inaudible
  ([audio rules](rules/audio.md#sdl-resampler-rate-limitation)).
- Darkside gameplay is not supported.

Next steps are in the [roadmap](roadmap.md).
