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

What can be played today is one prepared Journey in Clouds: the connected
mainland of map 23 and a bounded route through Vertigo. You can fight, run,
cast learned spells, finish Myra's quest, and use the Ironworks (Buy and Armor
Repair), Training and the Temple. Progress can be saved and continued exactly.
The original main screen works with mouse or keyboard. General exploration, a
normal new-game start, Rest, most spells and items, the original item and
service dialogs and Darkside gameplay are not available yet.

[Project status](docs/project-status.md) describes the current state and gaps,
[project history](docs/project-history.md) lists completed milestones and the
[roadmap](docs/roadmap.md) sets direction.

## Requirements and original game data

A legally obtained World of Xeen installation is required. No commercial game
data is included or modified. Reading Darkside metadata does not enable Darkside
gameplay.

The validated development setup is Windows x86-64 with MSYS2 UCRT64, CMake, SDL2,
zlib and a separate ScummVM source/build tree. MMModern reuses selected ScummVM
Xeen and support components; it does not run ScummVM's engine or SDL backend.
Build and dependency setup, including the pinned ScummVM revision, is in
[docs/dependencies.md](docs/dependencies.md).

## Running

Run from a terminal to see diagnostics, and quote paths containing spaces:

```text
mmodern <game-directory> [--difficulty adventurer|warrior] [--save-file <path.mmsave>]
mmodern --new-game <game-directory> [--difficulty adventurer|warrior] [--save-file <path.mmsave>]
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]
mmodern --inspect-map|--inspect-party|--inspect-events <game-directory> ...
```

- `mmodern <game-directory>` (or `--new-game`) starts a new game in Vertigo as
  in the original. The original title menu and difficulty dialog are not there
  yet: `--difficulty` chooses, and Adventurer is the default. Without
  `--save-file`, F9 writes nothing.
- `--journey-region` is a test mode with a prepared party at Clouds map 23
  `(9,11)`. `--combat-seed` fixes combat randomness.
- `--load-game` continues a save and keeps using that file for F9.
- `--render-map` explores a map for inspection. It cannot save.
- `--inspect-*` are developer tools.

Save outside the original installation. Relative paths use the working directory.
There is one current save format; saves from older builds are rejected with a
clear message (and may be overwritten by F9), and saves from newer builds are
protected. Saves need the same original archives and are not compatible with the
original games or ScummVM.

## Controls

| Key | Action |
| --- | --- |
| Up / Down | Move forward/back outside combat |
| Left / Right | Turn outside combat; combat movement shows "not supported yet" |
| Arrows in the sheet | Select a stat |
| Space | Interact outside combat; acknowledge text |
| A | Attack in combat |
| Enter | Open the selected stat; confirm a cast |
| Y / N | Yes / No |
| F1-F6 | Open the character sheet; choose a party member in dialogs |
| 1-9 | Select an inventory or stock slot; 1-3 pick a combat target |
| . | Wait |
| S | Shoot in mainland exploration |
| R / B | Run / Block in combat |
| C | Learned spells |
| I | Info (not supported yet); Items from the character sheet |
| W/A/C/M in Items | Weapons, Armor, Accessories, Misc |
| E / R / U in Items | Equip / Remove / Use; select a row then F1-F6 to move it |
| B in the Ironworks | Browse stock and inventory services |
| B / S / I / F in the Smith Items dialog | Buy / Sell / Identify / Fix; rows open Y/N Confirm |
| T in Training | Train the selected member immediately |
| H / D / U in the Temple | Heal / Donation / Uncurse |
| F9 | Save at a quiet moment |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

In Vertigo, enter from mainland `(10,13)` facing North with Space. The
[status](docs/project-status.md#vertigo) page lists the services and where they are.

Main-screen buttons and portraits also work by mouse. Combat portraits show the
remaining members after Run; click a portrait or press its corresponding F key
to view that member. The acting member and selected enemy use the original icons.
WASD movement and F Shoot shortcuts have been removed. Combat movement remains
unsupported until Milestone 48 Part C; the arrow buttons and keys show a notice.

## Tests

Every CTest entry is labelled `fast` or `process`. `process` tests run original-data
scenarios in child processes and take much longer; they need the original installation.

```text
ctest -L fast    # iteration: everything except process tests
ctest            # full suite, including process tests
```

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
