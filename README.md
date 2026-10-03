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
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]
mmodern --inspect-map|--inspect-party|--inspect-events <game-directory> ...
mmodern <game-directory>
```

- `--journey-region` starts the playable Journey at Clouds map 23 `(9,11)` facing
  West. `--combat-seed` fixes combat randomness. Without `--save-file`, F9 writes
  nothing.
- `--load-game` continues a save and keeps using that file for F9.
- `--render-map` explores a map for inspection. It cannot save.
- `--inspect-*` and the plain run are developer tools.

Save outside the original installation. Relative paths use the working directory.
There is one current save format; saves from older builds are rejected with a
clear message (and may be overwritten by F9), and saves from newer builds are
protected. Saves need the same original archives and are not compatible with the
original games or ScummVM.

## Controls

| Key | Action |
| --- | --- |
| W/Up, S/Down | Move forward/back; browse slots |
| A/Left, D/Right | Turn; browse categories |
| Space | Interact; attack in combat; acknowledge text |
| Enter | Confirm a transfer, cast, purchase, Training or Temple step |
| Y / N | Yes / No |
| F1-F6 | Choose a party member |
| 1-9 | Select an inventory or stock slot; 1-3 pick a combat target |
| . | Wait |
| F | Shoot in mainland exploration |
| R / B | Run / Block in combat; Repair / Buy in the Ironworks lobby |
| C | Learned spells |
| I, T, E, U | Inventory, transfer, equip/remove, use item |
| F9 | Save at a quiet moment |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

In Vertigo, enter from mainland `(10,13)` facing North with Space. The
[status](docs/project-status.md#vertigo) page lists the services and where they are.

## Tests

Every CTest entry is labelled `fast` or `process`. `process` tests run original-data
scenarios in child processes and take much longer; they need the original installation.

```text
ctest -L fast    # iteration: everything except process tests
ctest            # full suite, including process tests
```

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
