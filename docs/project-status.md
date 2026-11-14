# MMModern - Project Status

The latest completed milestone is M117, accepted on 2026-11-14. It
replaces the eleven M116 scalers with two optional CRT profiles.
[Milestone plan](milestone-117-plan.md). Earlier milestones are in
[project history](project-history.md), direction is in the
[roadmap](roadmap.md), and build and dependency setup is in
[dependencies](dependencies.md).

MMModern aims to be a faithful reimplementation of the original games;
approved deviations are listed in
[original quirks](original-quirks.md#approved-deviations). It does not replace
them yet. It reads the World of Xeen two-CD talkie edition: every Clouds and
Darkside map is playable, the mixed World of Xeen maps 113-128 included, and
Clouds, Darkside and World of Xeen can each be won with their original
endings. The game runs on Windows, on Linux and on a Steam Deck, in English
or with the Russian fan translation.

## What can be played

`mmodern <game-dir>` first plays the Darkside intro, as the CD does: the full
story until it has been watched to its end, then only the logos and the World
of Xeen title. On the original title screen, Credits shows the four original
pages, and Other Options plays the Clouds intro, forgets that the Darkside
story was watched and, after each win, replays that ending. New asks for a
save slot, a name and the difficulty: Adventurer (weapons hit harder) or
Warrior. As in the original, the six level-1 Clouds characters then stand in
Vertigo at `(18,4)` facing West at 08:00 on day 1 of year 610, with 800
gold, 10 gems, 90 food and an empty bank. Load continues a saved game.

### The world

- Every Clouds map opens from the original resources, with its terrain,
  walls, objects, monsters and text. Outdoor borders load the next map, and
  larger dungeons span several original tiles. Surfaces work as in the
  original: lava burns, sky and clouds drop the party to the land below,
  space kills and the desert costs time. Water needs Swimming for the whole
  party or Walk on Water; mountains need Mountaineer and outdoor wall types
  3 and 6 need Pathfinder, each from two members. Dark maps stay black until
  Light.
- Monsters wake, approach and attack on the original rules, with every
  attack, secondary effect and movement kind in the original data. Every map
  load rebuilds them at full HP and asleep.
- Every Event operation the scripts of both sides use runs as in the
  original. As in DOS, a cell's Events run again after every key or click
  while the party stands on it, Space interacts and then waits, and an Event
  a monster interrupts runs once the fight ends
  ([Event dispatch](rules/world-events.md#event-dispatch)). While an Event
  waits, the view, the small map and the border already show what it has
  changed. "Nothing here" is acknowledged before an interrupting fight
  ([approved deviation](original-quirks.md#dev-skipped-window-first)).
- Darkside falls to another map keep the original fall, its "Oops..." box
  ([approved deviation](original-quirks.md#dev-fall-oops)) and the damage
  the DOS program reads
  ([approved deviation](original-quirks.md#dev-fall-damage-zero) for three
  falls).
- The Reaper, Golem, Dwarf Mine and Sphinx entrance scenes play their
  original animations and speech, with captions when Efx is off. The Dwarf
  Mine always admits the party; the others need the right quest item, as do
  Darkside's Tower, Dungeon and Dwarf town scenes. Night in the Darkside
  scenes follows the clock, as the sky does
  ([approved deviation](original-quirks.md#dev-current-clock-night)).
- The Pyramids join the two sides as on the CD, without its disc-change
  prompt ([approved deviation](original-quirks.md#dev-no-disc-prompt)). Game
  and quest flags are kept per side; quest items and character flags are
  shared. Town Portal, Lloyd's Beacon and Mr Wizard follow the side, Town
  Portal on Darkside with the CD's
  [fixed arrivals](original-quirks.md#sv-darkside-town-portal-stride), and a
  party defeated on Darkside hears Alamar laugh and returns to the title.
- The mixed World of Xeen maps 113-128 (Darkstone Tower, Dragon Tower, the
  Dungeon of Death, the Southern Sphinx and the Dragon Clouds) open from the
  four Clouds entrances whose keys only Darkside gives. They borrow Clouds
  monsters, objects, walls or scenery map by map, as the CD does, and their
  exit Events return the party to Clouds (FlipWorld). Saving while FlipWorld
  has chosen a side the party has not loaded yet says "not supported yet"
  ([approved deviation](original-quirks.md#dev-pending-side-save)).
- In the Warzone Arena the Battle Master offers fights of a chosen level and
  size. Each win adds to every member's Warzone award, up to 99, and the game
  cannot be saved in the Warzone.

### Combat

Combat starts on contact and is turn-based per member: Attack, Block, Run,
Cast, Use and Quick Fight. Shoot works in exploration, indoors too. Monsters
choose their targets and attacks from their original data, drawn as DOS
draws them, and the party can turn during combat. Equipped items count with
their metal, element and enchantment. Run is per member, with each map's
original run position and chance.

Every kill adds the monster's original gold and gems and may drop an item at
its original chance and level. The treasure appears on the original screens
once no monster is in view; full packs lose or replace items as in the
original, and Warzone kills give none ([treasure rules](rules/items.md)).

Only the Xeen Slayer Sword hurts Lord Xeen, and his death gives the Scepter.
The other scripted deaths work as in DOS too: the Clan King, Count Draco,
the Ice Trolls and the Water Dragons
([combat rules](rules/combat.md#lord-xeen-and-scripted-death-hooks)).

### The endings

Giving the Scepter to the Sixth Mirror wins Clouds of Xeen as in DOS: the
game moves the party to Vertigo's tavern door, saves it, computes the score
from experience and real play time and plays the original ending. Afterwards
Castle Xeen is gone
([ending rules](rules/cutscenes.md#clouds-ending-entry-and-save)). Bringing
the Soul Box with Corak inside to Alamar wins Darkside of Xeen: the party is
saved at Castleview and the original Darkside ending plays. Bringing Xeen's
Scepter to the altar of the Clouds of the Ancients, with the awards DOS
requires, wins World of Xeen, and the original World ending follows
([presentation](rules/cutscenes.md#world-ending-presentation),
[win rules](rules/cutscenes.md#darkside-and-world-win-entry-and-save)).

Each ending shows the party's score and returns to a fresh World title.
Animations ignore keys and clicks, the score pages advance by key only, and
Ctrl-Q quits. The mirror words `showtime`, `showdown` and `genesis` play an
ending without winning, as in DOS. The wins are remembered outside the save,
in `settings.ini`
([approved deviation](original-quirks.md#dev-clouds-won-store)).

### Magic

All 76 Clouds spells work from the learned book, Quick Fight and charged
items, in exploration and in combat where the original allows them, with the
original prompts, refusals, refunds and costs. Each town's Guild sells its
original selection. Party protections show their border indicators and end
at Rest or dawn; Wizard Eye, Detect Monster and Identify Monster open the
original screens; Enchant, Recharge and Item to Gold work through Items; and
Jump, Teleport, Lloyd's Beacon, Super Shelter and Town Portal move the
party. The [magic rules](rules/magic.md) give each spell's effect, cost and
evidence.

### Characters and items

The original character sheet and Items dialog show stats with their popups
and let the party equip, remove, transfer, discard and use items. Exchange
swaps two members, Quick Reference opens the party summary, Quests shows
quest items, quests and Auto Notes, and Awards lists a member's awards.

The party holds one to six roster characters in any order. D dismisses a
member, never the last. Sign In at the Vertigo, Rivercity or Shangri-La
tavern adds a day, saves and opens the original Inn, where characters are
added, removed, created and deleted on the original screens
([party rules](rules/party.md)).

Teachers and Events grant skills, and each does what it does in DOS: Arms
Master and Tracker do nothing there either. Thievery picks locks, and a
picked chest stays open for good. Cartography fills the 7x7 automap (`=`)
and the original 16x16 Map (M) with every cell the party arrives on.

### Towns and services

Services use the original location screens, by mouse or keyboard, with each
place's original values on both sides: the Ironworks buys, sells, identifies
and fixes with generated stock; Training grants levels up to the town's cap;
the Temple heals, resurrects, uncurses and blesses donations on the right
day; the Tavern sells food and drinks, takes tips, tells rumors and signs
in; the Bank keeps gold and gems with the original interest; and the Guild
sells spells to members. Each visit costs the original departure time, and
stock and interest follow the original schedule.

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
forbid it. Sign In, leaving the Inn, Delete, Create and the endings save
automatically, as in DOS; a failed write shows the error and retries
([approved deviation](original-quirks.md#dev-inn-save-recovery)). Sound and
text settings are saved with the party. Loading continues exactly, without
replaying any Event, reward, time or random draw.
[Save format](#save-format) has the rest.

### Sound

The game plays the original FM music and effects as the CD installation's own
setup does: the Sound Blaster Pro driver on an OPL3 chip, in stereo, with
each place's song and fades between songs. Digital effects, voices, entrance
speech and Event samples play, and CD speech reads the installed OGG tracks.
Text off restores text when speech cannot play
([audio rules](rules/audio.md)).

The sound devices follow `XEEN.CFG` as the original installer wrote it.
Music can also come from the AdLib, Sound Blaster and Pro Audio Spectrum
drivers on an OPL2 chip, or from the MT-32 driver on an emulated MT-32 with
the player's own MT-32 1.07 control and PCM ROMs. The other installer
choices are silent with one notice ([known gaps](#known-gaps)); the game's
timing is the same either way.

## Interface

[Controls](controls.md) lists every key. As in the CD original, either mouse
button makes the same click, and keys follow the original's translation
table. Animations, missiles, portrait effects, NPC speech, shopkeepers,
treasure, the dream, the title and party death follow the original sequences
and timing. Keys pressed while the game is busy are buffered in order, and
held keys repeat as the keyboard does. A held movement key steps about nine
times a second and adds no step after release, an approved deviation.

A game controller can play instead when its option is on: its buttons send
the original keys, the right stick moves the original pointer, RT clicks and
the D-pad types in text entry ([controls](controls.md#game-controller)). The
original has no controller support, so this is an
[approved deviation](original-quirks.md#dev-controller), off by default.

F11 cycles through Off (the default), CRT VGA and CRT Phosphor. CRT VGA
keeps small letters crisp with subtle scanlines. CRT Phosphor shows more
texture in flat colours. Both work with either picture shape and can be
selected in the Windows launcher or the Linux setup screen. On the Steam
Deck at 1280x800, the maintainer found both profiles as fluid as Off during
walking and combat. [Player guide](player-guide.md#picture-filters) and
[approved deviation](original-quirks.md#dev-picture-filter).

## Russian

With the identified old-games.ru World of Xeen fan translation in its own
folder, chosen in the launcher or on the setup screen, the game plays in
Russian ([player guide](player-guide.md#russian)). Texts, pictures, the font
and the program text come from the translation; maps, rules, timing, music
and the CD's English recorded speech stay those of the CD
([approved deviation](original-quirks.md#dev-russian-layer)). The keyboard
and hotkeys follow the Russian Darkside program, including its key defects
([Russian keyboard](original-quirks.md#ru-keyboard)). Any other translation
is refused, naming the file, and English plays. Project text (launcher,
diagnostics, notices) stays English.

## Entry modes

`mmodern <game-dir>` is the player's entry. On Windows the release launcher
opens it from a window; on Linux the release is an AppImage, and `mmodern`
with no arguments opens a setup screen in the game's own window. Both find
or browse for the game folder, make the original installer's sound choice
and the language choice, and copy the diagnostics for bug reports. The sound
choice is an MMModern-owned `XEEN.CFG`, which the player entry prefers to
the installed one
([approved deviation](original-quirks.md#dev-owned-sound-cfg)). On a Steam
Deck the AppImage runs natively and the Windows package runs through Proton
([player guide](player-guide.md#steam-deck)).

Saves, settings, the log and the MT-32 ROMs live in one user folder
([player guide](player-guide.md#where-mmmodern-keeps-its-files)); original
data is never copied or modified. Developer modes start a new game directly,
load a save file, explore or inspect any Clouds map and run isolated monster
or Event fixtures; the [development guide](development.md#entry-modes) gives
every option. All modes need a World of Xeen CD installation
([game data sources](development.md#game-data-sources)).

## Save format

There is one current save format, v26 (saves from older builds are
refused). A save holds the party with every roster record in full, both
sides' flags, its sound options, play time and Clouds completion, the
selected Darkside song
([approved deviation](original-quirks.md#dev-darkside-load-music)), whether
the party is inside the Inn, the world, and one state per visited map:
monster positions and lifecycle, moved and opened objects, changed cells,
disabled Events, explored cells and the Tavern tip index. Monster HP and
wake state are not saved, and neither is pending treasure, as in the
original. Russian and English games share the save folder.

During 0.x, older formats are rejected before anything is restored, and a
newer, damaged or foreign slot is never overwritten. Saves are tied to the
original archives by fingerprint, written atomically, refused on Linux
network file systems, and not compatible with the original games or ScummVM.
The game does not read DOS saves; the developer tool `mmodern_dos_export`
writes one for evidence ([DOS saves](rules/dos-saves.md)).

## Architecture overview

Source is under `src/`: `core` (input, frame and installation types),
`formats/xeen` (original resource and save formats), `games/xeen` (rules and
state), `app` (coordination and presentation), `platform` (SDL window, audio
output, the user folder, save file I/O, `settings.ini` and the system clock,
the only code with Windows and POSIX implementations besides `main.cpp`) and
`compat` (the linked ScummVM components).

- Party and roster (`XeenParty`, `XeenRoster`): all thirty roster
  characters, the active party, inventory, quest items, purse and books.
- World (`XeenWorld`, `XeenSessionWorldState`): map and object caches, live
  actors, per-map state, economy, calendar and the world random state.
- Running program (`XeenProgramState`): what the original keeps outside its
  saves, such as the last Temple prices, pending treasure and the play-time
  mark; the `settings.ini` values; and the audio owner `XeenAudio`, which
  never changes time, randomness or saves.
- Rules (pure, in `games/xeen`): combat, monsters, treasure, items, services,
  spells, terrain, Rest and time.
- Events: `XeenEventSystem` decodes and runs the original Event scripts;
  unsupported effects refuse visibly and discard the whole Event.
- Cutscenes: `XeenCutscene` and `XeenCutsceneFlow` play the intros and
  endings of `WORLD/XEEN.EXE`, read from it at run time.
- Flows (`app`): `XeenEventFlow` coordinates the gameplay session, input,
  dialogs, inventory and the Inn; `XeenEncounterFlow` hosts encounters,
  combat, services and casting; `XeenNavigationFlow` handles movement;
  `Application` wires resources, SDL and the entry modes.
- Save (`XeenSaveState`, `XeenSaveFormat`, `XeenSaveFile`): captures a
  snapshot at a quiet boundary and restores it into fresh owners;
  `XeenRestoreGuard` and `XeenMutationWatch` stop stale work from publishing.

Presentation draws into a 320x200 indexed frame. SDL shows it in the largest
4:3 area of the window, or in whole multiples with `aspect=square`; clicks
map back through the same rectangle. Each frame carries an identity, so
input from a stale frame is dropped. With the controller option on,
`SdlController` turns controller input into ordinary key and mouse events
before the input loop sees them.

## Tests

CTest covers rules and synthetic fixtures, and original-data integration and
process tests that need a legally obtained installation. Tests are labelled
`fast` or `process`: use `ctest -L fast` while iterating and the complete
suite before closing a milestone. The complete suite runs on Windows and on
Linux (in WSL), and both reach the same approved digests and save bytes. The
generated [coverage report](coverage.md) counts every mechanic the original
data uses. Every push also builds on GitHub Actions and runs the tests that
need no original data
([continuous integration](dependencies.md#continuous-integration)).

## Known gaps

- Sound: the Canvas/GS, PC speaker, Wave Blaster and TAP-10 music
  drivers, and the PC speaker, AdLib, Disney Sound Source, Pro Audio
  Spectrum and Covox digital drivers, are silent with a notice.
  There is no device switch while playing. F8 speech replay is
  omitted by maintainer decision. SDL's resampler plays FM and CD audio up
  to about 0.07% fast, which is inaudible
  ([audio rules](rules/audio.md#sdl-resampler-rate-limitation)).
- Darkside: the [census](coverage.md#darkside-largest-gaps) lists no
  Darkside gap.
- Russian: a reader of Russian has not yet checked the text in play.
- Linux: the AppImage is x86-64 only, and there is no Flatpak. Under Proton
  the Windows launcher's Sound setup and Language windows do not run; the
  native AppImage replaces that route on a Steam Deck.
- Game controller: buttons cannot be remapped inside MMModern (Steam Input
  or the system does that), and there is no rumble, gyro or focus highlight.
  Under Proton the option is set by editing `window.ini`.

Next steps are in the [roadmap](roadmap.md).
