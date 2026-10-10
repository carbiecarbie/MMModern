# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The aim is a **faithful reimplementation**: the game should play, look and
behave like the original, using your own copy of the original game data.
Optional quality-of-life features may come later, always off by default.

## Status

MMModern is incomplete and experimental. It is not yet a replacement for the
original games.

A new Clouds game starts as in the original, in Vertigo, and the playable area
is the whole town and the connected mainland of map 23. You can fight, run, bash and
unlock grates and doors, cast learned spells, finish Myra's quest, rest with
food and recovery, and use the Ironworks (Buy and Armor Repair), Training and
the Temple. Progress can be saved and continued exactly. The original main
screen, character sheet, Items dialog and service screens work with mouse or
keyboard, and the game starts on the original title screen with New, Load and
the in-game Save, Save As, Load and Quit. The party can strafe, exchange
members and use Quick Reference, Info and Quick Fight. Other areas, buying
food, most spells and items, audio and Darkside gameplay are not available yet.

[Project status](docs/project-status.md) describes the current state and gaps,
[project history](docs/project-history.md) lists completed milestones and the
[roadmap](docs/roadmap.md) sets direction.

## Requirements and original game data

A legally obtained English World of Xeen two-CD installation is required. No commercial game
data is included, copied or modified. Reading Darkside metadata does not enable Darkside
gameplay.

At a GOG root, MMModern reads the cue-located ISO archives and the installed
`WORLD/XEEN.DAT`, ahead of any co-located floppy archives. One complete disc
suffices. A pre-existing CD `GAME` folder (or its parent) is also accepted with
all three archives and an uncompressed English DOS UI module: supply
`--ui-data <path-to-XEEN.DAT>` or an unambiguous `WORLD` sibling. A bare `GAME`
copy has no UI text. Conflicting or ambiguous sources and packed DATs fail clearly.

The validated development setup is Windows x86-64 with MSYS2 UCRT64, CMake, SDL2,
zlib and a separate ScummVM source/build tree. MMModern reuses selected ScummVM
Xeen and support components; it does not run ScummVM's engine or SDL backend.
Build and dependency setup, including the pinned ScummVM revision, is in
[docs/dependencies.md](docs/dependencies.md).

## Running

Run from a terminal to see diagnostics, and quote paths containing spaces:
`--ui-data <path>` can be supplied with every entry mode.

```text
mmodern <game-directory>
mmodern --new-game <game-directory> [--difficulty adventurer|warrior] [--save-file <path.mmsave>]
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]
mmodern --inspect-map|--inspect-party|--inspect-events <game-directory> ...
```

- `mmodern <game-directory>` opens the original title screen. New asks for a
  slot, a name and the difficulty; Load continues a slot; Tab or the gem opens
  the control panel in the game. Slots are kept under Local AppData.
- `--new-game` is a developer shortcut that starts directly in Vertigo
  (`--difficulty`, default Adventurer). Without `--save-file`, F9 writes
  nothing.
- `--journey-region` is a test mode with a prepared party at Clouds map 23
  `(9,11)`. `--combat-seed` fixes combat randomness.
- `--load-game` continues a save file and keeps using it for F9 (developer).
- `--render-map` explores a map for inspection. It cannot save.
- `--inspect-*` are developer tools.

Save outside all original source directories, including separately supplied
disc-image and DAT directories. Relative paths use the working directory.
There is one current save format; saves from older builds are rejected with a
clear message (and may be overwritten by F9), and saves from newer builds are
protected. Saves need the same original archives and are not compatible with the
original games or ScummVM.
Floppy-bound saves are rejected before restoration with a data-edition diagnostic.

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
| R | Rest in exploration; Run in mainland combat |
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
| Tab | Control panel: Save, Save As, Load, Quit |
| F9 | Save to the developer `--save-file` target |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

In Vertigo, enter from mainland `(10,13)` facing North with Space. The
[status](docs/project-status.md#vertigo) page lists the services and where they are.

Main-screen buttons and portraits also work by mouse. Combat portraits show the
remaining members after Run; click a portrait or press its corresponding F key
to view that member. The acting member and selected enemy use the original icons.
WASD movement and F Shoot shortcuts have been removed. In combat the party can
turn; moving forward or back shows a "not supported yet" notice.

## Tests

Every CTest entry is labelled `fast` or `process`. `process` tests run original-data
scenarios in child processes and take much longer; they need the original installation.

```text
ctest -L fast    # iteration: everything except process tests
ctest            # full suite, including process tests
```

Tests run on the optimized `build-rel` build (assertions kept); its setup is in
[docs/dependencies.md](docs/dependencies.md#optimized-test-build-build-rel).
Configure `MMODERN_XEEN_DATA_DIR` explicitly for required CD validation and,
when needed, `MMODERN_XEEN_UI_DATA` for its installed UI module.

## Developer test saves

Build the opt-in tool with `cmake --build build-m44 --target mmodern_test_save`.
It starts from the normal fresh Regional Journey and writes a validated current
save for `broken-armor`, `train-ready`, `injured-dead` or `poisoned`. The party
stays at map 23 `(9,11)` facing West; world, calendar and RNG retain fresh
Journey values (explicit seed 7). The presets provide equipped broken armor,
enough XP/gold for one Training, an Unconscious member and a Dead member with
four survivors, or Poison and a usable Misc antidote, respectively.

```powershell
.\build-m44\mmodern_test_save.exe train-ready "F:/Games/gog/Might and Magic 4-5" "C:/Playtest/train-ready.mmsave"
.\build-m44\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "C:/Playtest/train-ready.mmsave"
```

Choose your own output path in an existing directory outside both this repository
and the game installation. The tool refuses existing files. Generated `.mmsave`
files are local play-test artifacts; never commit them. The game and save format
are unchanged.

## Documentation

- [Project status](docs/project-status.md): what can be played, architecture, gaps.
- [Project history](docs/project-history.md): completed milestones.
- [Roadmap](docs/roadmap.md): next milestones and direction.
- [Dependencies](docs/dependencies.md): toolchain and ScummVM setup.
- [Agent instructions](AGENTS.md): development, documentation and Git rules.

## Disclaimer and license

MMModern is unofficial and is not affiliated with or endorsed by Ubisoft,
New World Computing or the ScummVM project. Might and Magic names and assets
belong to their respective copyright holders.

MMModern is distributed under the GNU General Public License version 3 or, at
your option, any later version (GPL-3.0-or-later). Reused ScummVM code is copyright
its respective contributors and licensed under GPLv3 or later.
