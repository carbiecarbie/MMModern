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

A new Clouds game starts in Vertigo, as in the original. You can play the
whole town and the F3 area around it.

In combat you can fight and run. Around town you can bash or unlock grates and
doors, search crates and trees, and rest, which uses food and restores the
party. Myra's quest, Joe's proof and Gunther's reward can all be finished. The
Tavern sells food, the Bank holds gold and gems, the Ironworks buys, sells,
identifies and fixes items, and Training and the Temple work too. Teachers
teach Cartography and Pathfinder, and the map fills in as you explore. You can
buy spells, cast the supported ones from the original Cast dialog and use
charged items.

The original title screen, main screen, character sheet, Items dialog and
service screens all work with mouse or keyboard, including strafing, Exchange,
Quick Reference, Info and Quick Fight. Saved games continue exactly where you
left off. Everything else is listed under [What is missing](#what-is-missing).

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
| Up / Down | Move forward/back outside combat; in combat they show "not supported yet" |
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
| I | Info; Items from the character sheet |
| Q | Quick Reference |
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

- Areas: only Vertigo and F3 are playable, and Run does not work indoors.
- Sound: there is no audio. Where the CD would play speech, a short notice
  appears. Speech, shop voices, the dream and the death laugh still take their
  original time, in silence.
- The intros and Mr Wizard's help are not available.
- Services: there is no Inn or recruiting. Temple Donation and Uncurse, and
  Tavern Drink, Tip, Rumors and Sign In, say "not supported yet". The Temple
  heals only the conditions modeled so far.
- Magic and items: most spells are missing, and monsters cannot yet be put to
  sleep, paralyzed or controlled. Light,
  Sleep, Insect Spray, Protection from Elements, Sparks, Toxic Cloud and
  Suppress Poison can be bought but say "not supported yet" when cast. Item
  powers that rely on missing spells say the same.
- World: terrain hazards on arrival (space, lava, sky, cloud, desert), the
  fountains in F3 and locked-chest locks and traps are not implemented.
- Combat: you cannot move, equip items or use the Use button during combat.
  The original allows equipping and Use.
- Skills: only Cartography and the skills existing rules use (Thievery,
  Navigator, Bodybuilder and others that affect HP or SP) do anything.
  Swimming, Mountaineer and the rest are recorded but have no effect yet.
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
caster and 1,000 party gems. Together the casters know the 22 M74 damage
spells implemented in Parts A and B, including Finger of Death, Holy Word,
Mass Distortion and Golem Stopper. Poison Volley remains unsupported until
Part C. Generate it with:

```powershell
.\build-rel\mmodern_test_save.exe monster-damage "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/monster-damage.mmsave"
```

```powershell
New-Item -ItemType Directory -Force D:/MModern-playtest
.\build-rel\mmodern_test_save.exe train-ready "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/train-ready.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/MModern-playtest/train-ready.mmsave"
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
