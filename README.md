# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The goal is a faithful reimplementation. The game should play, look and behave
like the original, using your own copy of the original game data. Optional
quality-of-life features may come later, and they will be off by default.

## What you can play

MMModern is incomplete and experimental. The 0.1 releases are its first
public versions, and they do not replace the original games yet.

A new Clouds game starts in Vertigo, as in the original. Every Clouds map is
open: walk across the outdoor borders, take stairs, teleports and the magic
mirror, or travel by Town Portal, Lloyd's Beacon and the other travel spells.
Mountaineer and Pathfinder cross mountains and outdoor walls, and dark
dungeons need light.

Combat follows the original rules: attack, block, shoot, run, cast
spells, change equipment and use charged items with the Use button. Monsters
use every attack and ability the original data gives them. Around town you can
bash or unlock grates and doors, search crates and trees, and rest, which uses
food and restores the party. Myra's quest, Joe's proof and Gunther's reward can
all be finished.

Every Vertigo service except the Inn works: the Tavern sells food and drinks
and gives tips and rumors, the Bank holds gold and gems, the Ironworks buys,
sells, identifies and fixes items, Training raises levels, the Temple heals,
blesses and uncurses, and the Guild sells spells. Teachers teach Cartography
and Pathfinder, and the map fills in as you explore. All 76 Clouds spells can
be cast from the original Cast dialog, and charged items cast their powers.

The original title screen, main screen, character sheet, Items dialog, Quests
and Awards screens and service screens all work with mouse or keyboard,
including strafing, Exchange, Quick Reference, Info and Quick Fight. The
border lights Danger Sense, Spot Secret Doors and Direction Sense as in the
original. Saved games continue exactly where you left off. Everything else is
listed under [What is missing](#what-is-missing).

## Download and play

You need Windows 10 or 11 (64-bit) and a legally obtained English
World of Xeen CD edition, as sold by GOG ("Might and Magic 4-5"). MMModern
includes no game data and never copies or modifies yours.

1. Download the latest `MMModern-<version>-windows-x64.zip` from the
   [GitHub releases](https://github.com/carbiecarbie/MMModern/releases) page.
2. Unpack it into a folder of your own, not inside the game folder.
3. Double-click `MMModern.bat`. The first time, it looks for the game where GOG
   installed it. If it cannot find it, a window asks you to choose your
   "Might and Magic 4-5" folder, the one that contains `WORLD\XEEN.DAT`. Your
   choice is remembered, so later the game starts directly.
4. If you move the game, double-click `Change game folder.bat` to choose the
   folder again. You can also run
   `mmodern.exe "C:\GOG Games\Might and Magic 4-5"` from a command prompt.

The game opens on the original title screen. New asks for a slot, a name and
the difficulty, and Load continues a saved slot. The
[status page](https://github.com/carbiecarbie/MMModern/blob/main/docs/project-status.md#vertigo)
lists Vertigo's services and where to find them. If you leave town, you can
walk back in from `(10,13)` in F3 by facing North and pressing Space.

A console window stays open next to the game and shows diagnostics. If the game
cannot start, the console says why and waits for a key.

## Saving

Press Tab, or click the gem, to open the control panel with Save, Save As, Load
and Quit. There are ten slots, stored in
`%LOCALAPPDATA%\MMModern\Saves\<game data id>\slot-0.mmsave` to `slot-9.mmsave`.

During 0.x, each release reads only its own save format. Saves from an older
release show as "Unavailable" and cannot be loaded. Choosing one tells you why,
and New can overwrite that slot after you confirm. Compatibility will be
reconsidered for 1.0. A save needs the same original game data it was made
with, and it does not work with the original games or ScummVM.

## Controls

| Key | Action |
| --- | --- |
| Up / Down | Move forward/back outside combat |
| Left / Right | Turn, also during combat |
| Ctrl+Left / Ctrl+Right, keypad 4 / 6 | Strafe outside combat |
| Arrows in the sheet | Select a stat |
| Space | Interact outside combat; acknowledge text |
| A | Attack in combat |
| Enter | Open the selected stat; confirm a cast |
| Y / N | Yes / No |
| F1-F6 | Open the character sheet; choose a party member in dialogs |
| 1-9 | Select an inventory or stock slot; 1-3 pick a combat target |
| . | Wait |
| S | Shoot in exploration |
| R | Rest in exploration; Run in combat outdoors |
| B | Bash in exploration; Block in combat |
| C | Learned spells |
| U in combat | Use a charged item |
| I | Info; Items from the character sheet |
| Q | Quick Reference |
| V | Quests; Q in Items |
| M | Map |
| = | Toggle the automap |
| E in the character sheet | Exchange with another member |
| F / O in combat | Quick Fight / Quick Fight Options |
| W/A/C/M in Items | Weapons, Armor, Accessories, Misc |
| E / R / U in Items | Equip / Remove / Use; select a row then F1-F6 to move it |
| B / R in the Ironworks | Buy (browse stock and inventory services) / Armor Repair |
| B / S / I / F in the Smith Items dialog | Buy / Sell / Identify / Fix; rows open Y/N Confirm |
| T in Training | Train the selected member immediately |
| H / D / U in the Temple | Heal / Donation / Uncurse |
| F in the Tavern | Buy food; Y/N confirms or cancels |
| D / W in the Bank, then O / E | Deposit / Withdraw, then Gold / Gems |
| Digits, Backspace/Delete, Enter in Bank amount entry | Enter up to ten digits, edit, confirm; Escape cancels |
| Tab | Control panel: Save, Save As, Load, Quit |
| F9 | Save to the developer `--save-file` target |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

Main-screen buttons and portraits also work by mouse. Combat portraits show the
remaining members after Run; click a portrait or press its corresponding F key
to view that member. The acting member and selected enemy use the original icons.

## What is missing

- Other towns: Nightshadow, Rivercity, Asp and Winterkill have no services
  yet; they say "not supported yet".
- Sound: there is no audio. Where the CD would play speech, a short notice
  appears. Speech, shop voices, the dream and the death laugh still take their
  original time, in silence.
- The intros and the control panel's sound and text options are not
  available.
- Inn and party: there is no Inn, recruiting or character creation. Tavern
  Sign In and Dismiss say "not supported yet".
- Entrance scenes and the Arena: the Reaper, Golem, Dwarf Mine, Sphinx and
  Pyramid entrances and the Arena say "not supported yet".
- Monster treasure: not every monster drops items yet, and drops of higher
  treasure levels and accessories are missing.
- Darkside gameplay is not supported.

The [project status](https://github.com/carbiecarbie/MMModern/blob/main/docs/project-status.md)
page has the full current state, the
[project history](https://github.com/carbiecarbie/MMModern/blob/main/docs/project-history.md)
lists completed milestones and the
[roadmap](https://github.com/carbiecarbie/MMModern/blob/main/docs/roadmap.md)
shows what comes next. The
[game rules reference](https://github.com/carbiecarbie/MMModern/blob/main/docs/rules/README.md)
documents mechanics, formulas, evidence from the original program and open
differences from the ScummVM reference.

## For developers

### Game data sources

At a GOG root, MMModern reads the cue-located ISO archives and the installed
`WORLD/XEEN.DAT`, ahead of any co-located floppy archives. One complete disc
suffices. A pre-existing CD `GAME` folder (or its parent) is also accepted with
all three archives and an uncompressed English DOS UI module: supply
`--ui-data <path-to-XEEN.DAT>` or an unambiguous `WORLD` sibling. A bare `GAME`
copy has no UI text. Conflicting or ambiguous sources and packed DATs fail clearly.
Reading Darkside metadata does not enable Darkside gameplay.

### Building

The validated development setup is Windows x86-64 with MSYS2 UCRT64, CMake, SDL2,
zlib and a separate ScummVM source/build tree. MMModern reuses selected ScummVM
Xeen and support components; it does not run ScummVM's engine or SDL backend.
Build and dependency setup, including the pinned ScummVM revision and release
packaging, is in [docs/dependencies.md](docs/dependencies.md).

### Entry modes

Run from a terminal to see diagnostics, and quote paths containing spaces:
`--ui-data <path>` can be supplied with every entry mode.

```text
mmodern <game-directory>
mmodern --new-game <game-directory> [--difficulty adventurer|warrior] [--save-file <path.mmsave>]
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]
mmodern --monster-fixture <species> <game-directory> [--record <n>] [--start <x>,<y>,<facing> | --water-edge] [--combat-seed <nonzero-u32>]
mmodern --event-fixture <game-directory> <map> <x> <y> <north|east|south|west> [--combat-seed <nonzero-u32>]
mmodern --inspect-map|--inspect-party|--inspect-events <game-directory> ...
```

- `mmodern <game-directory>` opens the original title screen (normal play).
- `--new-game` is a developer shortcut that starts directly in Vertigo
  (`--difficulty`, default Adventurer). Without `--save-file`, F9 writes
  nothing.
- `--journey-region` is a test mode with a prepared party at Clouds map 23
  `(9,11)`. `--combat-seed` fixes combat randomness.
- `--load-game` continues a save file and keeps using it for F9 (developer).
- `--render-map` explores a map for inspection. It cannot save.
- `--monster-fixture` starts the prepared Journey on F3 with one placed monster
  replaced by the chosen original `xeen.mon` species, through an in-memory copy
  of the map's monster list. By default it picks the placement nearest the
  Journey entry that has a walkable start cell with the monster 2-3 cells
  straight ahead on a clear line (no tent, tree or other monster between);
  `--record` fixes the placement and
  `--start <x>,<y>,<north|east|south|west>` fixes the start, which must be a
  walkable F3 cell without an automatic Event or a monster spawn (refused with
  the reason otherwise). `--water-edge` instead starts the party facing water
  and moves the placement (or `--record`) to free land 2-3 cells beyond it, so
  a flyer crosses the water while a walker stays on its bank; for example
  `--monster-fixture 1 <game-directory> --water-edge` for a Giant Bat. Walking
  and flying species are both accepted if they pass the normal mainland checks;
  the terminal prints the species, its cell, the start and the facing. Saving
  and the control panel's Save, Save As and Load are disabled. Not in the
  title menu.
- `--event-fixture` starts an isolated Event session on the chosen Clouds map,
  position and facing, using the original loader, interpreter and Journey
  publication owners. The default seed is 7; `--combat-seed` makes another run
  reproducible. Party prerequisites are the prepared Journey's six original
  members, at levels `3,3,3,4,3,3` with XP `1000,2000,1000,1000,2000,1000`,
  full HP/SP, cleared conditions and temporary bonuses, on day 8 of year 610
  at 08:00 in Adventurer mode. Original food, purse, flags, quest counters,
  inventories and skills are loaded normally; merchant stock uses that seed.
  For example, `mmodern --event-fixture "F:/Games/gog/Might and Magic 4-5" 27 10 10 north`.
  The position must resolve to original geometry without immediate monster
  contact. Resource identity, actor binding, ownership and mutation checks still
  run. Unsupported operations refuse through the normal Event flow. Travel to
  an unavailable map discards unpublished changes; already displayed and
  published script rewards or damage remain. Saving, Save As and Load are
  disabled, including direct snapshot capture. This entry is absent from the
  title menu and does not admit maps to ordinary play.
- `--inspect-*` are developer tools.

Developer save files must be outside all original source directories, including
separately supplied disc-image and DAT directories. Relative paths use the
working directory. Saves from older builds are rejected with a clear message
(and may be overwritten by F9), and saves from newer builds are protected.
Floppy-bound saves are rejected before restoration with a data-edition diagnostic.

### Tests

Every CTest entry is labelled `fast` or `process`. `process` tests run original-data
scenarios in child processes and take longer; they need the original installation.
Tests are independent, so run them in parallel; the few that assert real-clock
behavior or open a native window are marked to run alone.

```text
ctest -L fast -j 8    # iteration: everything except process tests
ctest -j 8            # full suite, including process tests
```

The complete suite is normally run through `tools/run-full-ctest.ps1`, which
picks the job count and keeps a log and summary per run. Tests run on the
optimized `build-rel` build (assertions kept); its setup is in
[docs/dependencies.md](docs/dependencies.md#optimized-test-build-build-rel).
Configure `MMODERN_XEEN_DATA_DIR` explicitly for required CD validation and,
when needed, `MMODERN_XEEN_UI_DATA` for its installed UI module.

### Developer test saves

Build the opt-in tool with `cmake --build build-rel --target mmodern_test_save`.
It starts from the normal fresh Regional Journey and writes a validated current
save for `broken-armor`, `train-ready`, `injured-dead` or `poisoned`. The party
stays at map 23 `(9,11)` facing West; world, calendar and RNG retain fresh
Journey values (explicit seed 7). The presets provide equipped broken armor,
enough XP/gold for one Training, an Unconscious member and a Dead member with
four survivors, or Poison and a usable Misc antidote, respectively.
The `monster-control` preset stands in Vertigo `(18,4)` facing West: each
caster knows the M73 spells available in their book (Sleep, Beast Master,
Hypnotize, Dragon Sleep, Insect Spray, Turn Undead, Sparks and Toxic Cloud),
with 1,000 SP each and 1,000 party gems. Generate it with:

```powershell
.\build-rel\mmodern_test_save.exe monster-control "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/monster-control.mmsave"
```

The `monster-damage` preset uses the same Vertigo position, 1,000 SP per
caster and 1,000 party gems. Together the casters know all 23 M74 damage
spells, including Finger of Death, Holy Word, Mass Distortion, Golem Stopper
and Poison Volley. Generate it with:

```powershell
.\build-rel\mmodern_test_save.exe monster-damage "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/monster-damage.mmsave"
```

The `m76-spells` preset starts at Vertigo `(18,4)` facing West. A level-7
Cleric and Sorcerer know the seven M76 party buffs and Light, Levitate,
Walk on Water, Clairvoyance, Detect Monster and Identify Monster between them;
the Sorcerer also knows Enchant Item, Recharge Item, Item to Gold,
Super Shelter, Etherealize, Time Distortion, Teleport, Lloyd's Beacon and Town Portal,
with 1,000 SP each and 1,000 party gems. Their miscellaneous packs hold
five-charge items for the first fifteen powers: the Cleric has the seven buff
powers, Light and Levitate; the Sorcerer has Walk on Water, Clairvoyance,
Detect Monster, Identify Monster, Enchant and Recharge. The Sorcerer's last
three Misc slots hold ordinary, cursed and broken rechargeable items: broken
fills to 63 charges; cursed adds the normal draw. His Recharge-power item is
itself ineligible for Recharge. The third member carries a plain weapon and
armor, an enchanted weapon, broken and cursed weapons, and weapon ID 34.
Enchant refuses the latter four weapons; Item to Gold refuses only ID 34
and converts immediately. Escape at the member prompt refunds SP/gems,
including for an item cast whose charge remains spent; later Escape keeps
the cost. An original Vertigo monster stands one
step west, at `(17,4)`, for combat casting, Quick Fight and item Use.
On mainland map 23, stand at `(12,9)` facing West to try
Walk on Water onto the shallow water at `(11,9)`.

The third member's first six Misc slots hold five-charge Super Shelter,
Etherealize, Time Distortion, Teleport, Lloyd's Beacon and Town Portal items. On mainland map 23,
`(12,10)` has cell attribute bit `0x40`: ordinary Rest refuses there, while
Super Shelter sleeps eight hours without monster interruption opportunities.
Time Distortion returns to `(10,12)` without changing facing. Teleport asks
for one digit (1-9); empty, zero or Escape keeps the cost and cancels.
Etherealize
moves one cell forward, ignoring walls. Only Time Distortion is admitted
from these six travel spells in the learned book or Quick Fight during combat.

The Cleric's beacon starts in Vertigo `(18,4)`, the Sorcerer's on mainland 23
`(9,11)`, and the third member's in Nightshadow (map 29). The third member also
knows Lloyd's Beacon. Transfer the Beacon item to the Cleric to use his stored
Vertigo destination. Set uses S/T or the original Set button; Return uses R
or its button and keeps facing. Same-map Return keeps live monster HP and
status; cross-map Return reloads. Town Portal accepts 1-5 followed by Enter;
Vertigo always reloads, including from Vertigo. Darkside beacons refuse
without spending resources. Escape keeps the
spell or item cost; an unset beacon defaults to Vertigo even on cancel.

```powershell
.\build-rel\mmodern_test_save.exe m76-spells "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/m76-spells.mmsave"
```

The `m79-playtest` preset starts at Rivercity Training, map 30 `(12,12)`
facing North, with 100,000 gold, 5,000 gems and one ration. Permanent levels
repeat 9, 14 and 19, with enough XP for the three Training caps. Members 1,
3 and 5 have all six Clouds Guild memberships; the others have none. Books
are partial, members are injured, member 1 carries a cursed weapon and member
2 a broken armor item. Members 5 and 6 are Sorcerers with Town Portal and
Lloyd's Beacon: their beacons reach Shangri-La Ironworks `(14,8)` on map 49
and Newcastle Training `(11,7)` on map 73. Use Town Portal for the five towns;
use the two beacons for Shangri-La and Newcastle, then the original stairs to
Newcastle Level 2's Bank. Its Temple still says "not supported yet" until M79
Part C. Interact with Space at a service, and use its original buttons or keys.

```powershell
.\build-rel\mmodern_test_save.exe m79-playtest "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/m79-playtest.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/m79-playtest.mmsave"
```

The `m78-playtest` preset is a travel party at map 23 `(9,11)` facing West:
the original six at level 30 with full HP and SP, Obsidian weapons and armor
equipped as each class allows, every spell of each caster's list, 100,000
gold and 5,000 gems. Arturius and Tyro hold Mountaineer and Pathfinder,
everyone can swim, and Badger has the information skills. `m78-one-holder`
is the same party with Mountaineer and Pathfinder on Arturius only, so the
mountains and outdoor wall types 3 and 6 stay closed until a teacher adds a
second holder.

```powershell
.\build-rel\mmodern_test_save.exe m78-playtest "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/m78-playtest.mmsave"
```

The `party-healing` preset also starts at Vertigo (18,4). A Cleric and a
Druid know the M75 spells allowed by their classes, with 1,000 SP per caster
and 1,000 party gems.
Four patients carry Poison, Disease, Paralysis, Stone, Dead and Eradicated;
the casters carry five-charge items for all ten M75 powers.

```powershell
.\build-rel\mmodern_test_save.exe party-healing "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/party-healing.mmsave"
```

For example, to create the play-test folder, generate the `train-ready` save
and load it:

```powershell
New-Item -ItemType Directory -Force D:/MModern-playtest
.\build-rel\mmodern_test_save.exe train-ready "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/train-ready.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/train-ready.mmsave"
```

The `combat-items` preset starts on the mainland at (9,11), facing West,
near monsters. The party carries weapons and armor to swap, a cursed weapon,
the Xeen Slayer Sword, and charged damage, healing and Town Portal items.
The last member has full packs. Enter combat with Wait, manage equipment
through a character's Items dialog, and use charged items through the combat
Use button (U). The Quests button (V, or Q in Items) shows quest items,
including three Phirna Roots, current quests and Auto Notes. The first member
has more than nine awards, including Warzone wins and a race award: open
Awards on that character's sheet and scroll with Up/Down. The party has
Direction Sense, Spot Secret Doors and Danger Sense: the facing letter shows
below the view (hidden while a text or number prompt is open), and the top
border gem lights after the first step or turn while a monster is within
three cells. Spot Secret Doors lights only indoors, facing a plain wall with
open space behind it; Vertigo has none, so try it in an Event developer
session, whose original party also has the skill:
`.\build-rel\mmodern.exe --event-fixture "F:/Games/gog/Might and Magic 4-5" 30 6 7 north`
(Rivercity).

```powershell
.\build-rel\mmodern_test_save.exe combat-items "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/combat-items.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/combat-items.mmsave"
```

Choose your own output path in an existing directory outside both this repository
and the game installation. The tool refuses existing files. Generated `.mmsave`
files are local play-test artifacts; never commit them. The game and save format
are unchanged.

### Documentation

- [Project status](docs/project-status.md): what can be played, architecture, gaps.
- [Project history](docs/project-history.md): completed milestones.
- [Roadmap](docs/roadmap.md): next milestones and direction.
- [Dependencies](docs/dependencies.md): toolchain, ScummVM setup and release packaging.
- [ScummVM differences](docs/scummvm-differences.md): where MMModern behaves differently from ScummVM, and why.
- [Agent instructions](AGENTS.md): development, documentation and Git rules.

## Disclaimer and license

MMModern is unofficial and is not affiliated with or endorsed by Ubisoft,
New World Computing or the ScummVM project. Might and Magic names and assets
belong to their respective copyright holders.

MMModern is distributed under the GNU General Public License version 3 or, at
your option, any later version (GPL-3.0-or-later). Reused ScummVM code is copyright
its respective contributors and licensed under GPLv3 or later. The release zip's
`THIRD-PARTY.txt` lists the bundled libraries, their licenses and the
corresponding source.
