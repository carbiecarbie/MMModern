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
combat, spells, items and town services follow the original rules. The Inn
lets you sign in, recruit and remove members, and create and delete
characters, but you cannot travel to Darkside yet. The Clouds story has been
verified through Lord Xeen and the Sixth Mirror, and the original ending and
score are playable.
The original Clouds and Darkside intros can also be watched.

The original interface is close to complete, with mouse and keyboard.
[What is missing](#what-is-missing) lists the rest.

## Download and play

You need Windows 10 or 11 (64-bit) and a legally obtained English
World of Xeen CD edition. MMModern never copies or modifies your game files.

1. Download the latest `MMModern-<version>-windows-x64.zip` from the
   [releases page](https://github.com/carbiecarbie/MMModern/releases).
2. Unpack it into a folder of your own, not inside the game folder.
3. Double-click `MMModern.bat` to open the launcher. It looks for your
   remembered game folder, then GOG's installation; if needed, choose the
   folder containing `WORLD\XEEN.DAT`. The window shows the data and sound
   devices found. Press Enter or click **Play** to start the game.
4. Use **Change game folder** if you move the installation. You can also run
   `mmodern.exe "C:\GOG Games\Might and Magic 4-5"` from a command prompt.

The launcher stays open while you play. The game opens in front without a
console window. If it fails, the launcher shows the error and offers to open
its log.

## Display and sound

The game window can be resized or maximized, and Alt+Enter switches between the
window and borderless fullscreen. By default the picture fills the largest 4:3
area with black borders, matching the shape of the original display. Window
mode and size are remembered in `%LOCALAPPDATA%\MMModern\window.ini`, apart
from saves. Set `aspect=square` in that file to use the former square-pixel
presentation, scaled in whole multiples of 320x200. Set `aspect=4:3` or delete
the file to restore the default.

The original FM music, digital effects, shop and entrance voices, dream, death
laugh and CD speech play together. Text off hides the NPC text replaced by
speech; unavailable speech restores readable text. Audio follows the Windows
default output at its own rate, including changes during play. Output failures
silence audio with one notice and keep the game running; device removal
reopens the default output.

Use **Sound setup** in the launcher to choose the music and the speech device
from the original installer's lists. The devices MMModern plays come first,
with Sound Blaster Pro music and Sound Blaster 16 speech recommended, as the
CD installs them; the others are listed under "No sound in MMModern yet". The
installer's port, IRQ, DMA and volume questions are under **Advanced**; they
described the original DOS sound cards and do not change the sound quality in
MMModern. The choice is saved in `%LOCALAPPDATA%\MMModern\XEEN.CFG`; the installed
`WORLD/XEEN.CFG` is never changed. **Reset to installed** removes the owned
choice and restores the installed devices (SB Pro music and SB16 speech in
GOG's installation). Command-prompt play uses the same choice.

AdLib, Sound Blaster, Pro Audio Spectrum and MT-32 music play. For MT-32,
put your MT-32 1.07 control ROM and MT-32 PCM ROM, under any file names, in
`%LOCALAPPDATA%\MMModern\roms`; MMModern includes no ROMs. The launcher
reports whether they are identified. A device awaiting support can still be
chosen; the game stays silent and shows a notice.

## Reporting a problem

Use **Copy diagnostics** in the launcher and paste the report into a
[bug report](https://github.com/carbiecarbie/MMModern/issues/new?template=bug_report.yml).
Describe what happened, what you expected, and the steps, with map and
coordinates when known. Attach `%LOCALAPPDATA%\MMModern\logs\mmodern.log`
(and `mmodern-previous.log` after a crash) and the affected save slot from the
folder shown in the report. Profile paths in the report and log are abbreviated.
Never attach original game files or MT-32 ROMs.

## Saving

Press Tab, or click the gem, to open the control panel with Save, Save As, Load
and Quit. There are ten slots, stored in
`%LOCALAPPDATA%\MMModern\Saves\<game data id>\slot-0.mmsave` to `slot-9.mmsave`.

During 0.x, saves from an older release may show as "Unavailable" if the save
format has changed. Choosing one tells you why, and New can
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

- Darkside of Xeen cannot be played.
- The Pyramid entrance and the mirror words `showdown` and `genesis` (the
  Darkside and World endings) say "not supported yet".

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
- Switching between the original sound devices from inside MMModern,
  instead of through `XEEN.CFG`.
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
