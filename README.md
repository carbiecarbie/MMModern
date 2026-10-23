# MMModern

[![Latest release](https://img.shields.io/github/v/release/carbiecarbie/MMModern)](https://github.com/carbiecarbie/MMModern/releases)
[![License: GPL-3.0-or-later](https://img.shields.io/github/license/carbiecarbie/MMModern)](LICENSE)

> [!IMPORTANT]
> MMModern is an unofficial fan project and is still incomplete. It ships no
> game data: you need your own English World of Xeen CD edition, sold by GOG as
> "Might and Magic 4-5".

MMModern is an open-source reimplementation of the engine behind
Might and Magic IV: Clouds of Xeen and Might and Magic V: Darkside of Xeen /
World of Xeen, running on your own copy of the game data. The goal is to match
the original DOS game in how it looks, plays and behaves. Quality-of-life
options may come later, and they will be off by default.

MMModern is not a port of ScummVM's Xeen engine. It reuses some ScummVM code,
but the reference is the original DOS game: where ScummVM behaves differently,
MMModern follows DOS. The
[ScummVM differences](https://github.com/carbiecarbie/MMModern/blob/main/docs/scummvm-differences.md)
page lists the known cases.

## What works today

The 0.x releases are the first public versions, and they do not replace the
original games yet.

A new Clouds of Xeen game starts in Vertigo with the original six characters,
and from there it plays like the original. Every Clouds map is open, and
combat, spells, items and town services follow the original rules. You cannot
change the starting party or create characters yet, and you cannot travel to
Darkside. So far only the Vertigo quests have been tested thoroughly.

The original interface is close to complete, with mouse and keyboard.
[What is missing](#what-is-missing) lists the rest.

## Download and play

You need Windows 10 or 11 (64-bit) and a legally obtained English
World of Xeen CD edition. MMModern never copies or modifies your game files.

1. Download the latest `MMModern-<version>-windows-x64.zip` from the
   [releases page](https://github.com/carbiecarbie/MMModern/releases).
2. Unpack it into a folder of your own, not inside the game folder.
3. Double-click `MMModern.bat`. The first time, it looks for the game where GOG
   installed it. If it cannot find it, a window asks you to choose your
   "Might and Magic 4-5" folder, the one that contains `WORLD\XEEN.DAT`. Your
   choice is remembered, so later the game starts directly.
4. If you move the game, double-click `Change game folder.bat` to choose the
   folder again. You can also run
   `mmodern.exe "C:\GOG Games\Might and Magic 4-5"` from a command prompt.

A console window stays open next to the game and shows diagnostics. If the game
cannot start, the console says why and waits for a key.

## Display and sound

The game window can be resized or maximized, and Alt+Enter switches between the
window and borderless fullscreen. The picture keeps its proportions and grows in
whole multiples of the original 320x200, with black borders around it, so every
pixel stays sharp and clicks land on the right buttons at any size. The first
window takes the largest multiple that fits your screen. The mode and window
size are remembered in `%LOCALAPPDATA%\MMModern\window.ini`, apart from the
saves; delete that file to start again from the defaults.

The original FM music, digital effects, shop and entrance voices, dream, death
laugh and CD speech play together. Text off hides the NPC text replaced by
speech; unavailable speech restores readable text. Audio follows the Windows
default output at its own rate, including changes during play. Output failures
silence audio with one notice and keep the game running; device removal
reopens the default output.

## Saving

Press Tab, or click the gem, to open the control panel with Save, Save As, Load
and Quit. There are ten slots, stored in
`%LOCALAPPDATA%\MMModern\Saves\<game data id>\slot-0.mmsave` to `slot-9.mmsave`.

During 0.x, each release reads only its own save format, so saves from an older
release show as "Unavailable". Choosing one tells you why, and New can
overwrite that slot after you confirm. Compatibility will be reconsidered for
1.0. A save only works with the game data it was made with, and neither the
original games nor ScummVM can read it.

## Controls

The mouse works on every button, portrait and dialog, as in the original. The
most used keys:

| Key | Action |
| --- | --- |
| Arrow keys | Move and turn |
| Ctrl+Left / Ctrl+Right | Strafe |
| Space | Interact; continue text |
| F1-F6 | Character sheet; pick a party member |
| A / B / R | Attack / Block / Run in combat |
| S / R / B | Shoot / Rest / Bash while exploring |
| C | Cast a spell |
| M / = | Map / automap on or off |
| Tab | Control panel: Save, Load, Quit |
| Escape | Back or cancel |
| Alt+Enter | Window or fullscreen |

The [full list of controls](https://github.com/carbiecarbie/MMModern/blob/main/docs/controls.md)
covers the character sheet, Items, Quick Fight and every service.

## What is missing

- The Inn. You cannot recruit, dismiss or create characters, and Sign In and
  Dismiss in the Tavern say "not supported yet".
- Darkside of Xeen cannot be played.
- The Pyramid entrance and the Clouds ending say "not supported yet". The
  Arms Master and Tracker skills do nothing.
- The intros.
- A few sounds: the Inn song, falling and landing, and the title-screen
  lightning.

The [project status](https://github.com/carbiecarbie/MMModern/blob/main/docs/project-status.md)
page has the full current state, the
[project history](https://github.com/carbiecarbie/MMModern/blob/main/docs/project-history.md)
lists finished milestones and the
[roadmap](https://github.com/carbiecarbie/MMModern/blob/main/docs/roadmap.md)
shows what comes next. The
[game rules reference](https://github.com/carbiecarbie/MMModern/blob/main/docs/rules/README.md)
documents mechanics, formulas, evidence from the original program and open
differences from the ScummVM reference.

## Planned extras

Later, MMModern may add a few things the original never had. None of them has
been studied yet, and each will be an option that is off by default, so the game
stays faithful unless you turn it on:

- Graphics filters for the scaled picture.
- A choice of which original sound device you hear, such as AdLib,
  Sound Blaster or MT-32 with your own ROMs. This one is already on the
  [roadmap](https://github.com/carbiecarbie/MMModern/blob/main/docs/roadmap.md).
- Game controller support.
- Possibly some interface improvements.

## Building

Development uses Windows x86-64 with MSYS2 UCRT64, CMake, SDL2, zlib,
libvorbis/libogg and a separate ScummVM source and build tree. MMModern reuses
selected ScummVM Xeen and support code, but it does not run ScummVM's engine or
SDL backend. [Dependencies](docs/dependencies.md) covers the setup, the pinned
ScummVM revision and release packaging.

[Development](docs/development.md) covers the command-line entry modes, which
game data is read, the test suite and the developer test saves used for
play-testing.

## Documentation

- [Project status](docs/project-status.md): what can be played, architecture, gaps.
- [Project history](docs/project-history.md): completed milestones.
- [Roadmap](docs/roadmap.md): next milestones and direction.
- [Controls](docs/controls.md): every key.
- [Development](docs/development.md): entry modes, tests and developer test saves.
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
`THIRD-PARTY.txt` lists the bundled libraries, including static libvorbisfile,
libvorbis and libogg, their licenses and corresponding source.
