# Player guide

The [README](../README.md) covers downloading and starting the game, and
[controls](controls.md) lists every key and the controller layout. This page
has the rest: the Steam Deck, the Russian translation, display and sound
options, saves, and where MMModern keeps its files.

## Steam Deck

There are two ways to play on a Deck. The Linux AppImage has the setup screen
for the game folder, sound, language and controller. The Windows package also
runs through Proton, but its launcher does not, so the game folder goes into
the shortcut's launch options.

Either way, first copy your World of Xeen game folder to the Deck, or install
the GOG game with a launcher such as Heroic.

### With the Linux AppImage

Download the AppImage and make it executable, as the
[README](../README.md#linux) describes. You can start it in Desktop Mode, or
add it to Steam as a non-Steam game and start it in Game Mode. Leave
Compatibility forcing off: this is the native Linux version.

Touch and the right trackpad click the setup screen's buttons, and Steam+X
opens the on-screen keyboard when you choose a folder. To play with the
Deck's controls, turn on the setup screen's **Game controller** row and use
Steam's **Gamepad** template, or keep the option off and use the
[keyboard layout](#keyboard-layout-for-the-deck) below.

### With the Windows package through Proton

Unpack the Windows zip outside the game folder. In Steam, add the unpacked
`mmodern.exe` as a non-Steam game and select Proton under **Compatibility**
in its Properties. Set the launch options to your game folder, quotes
included. For example:

```text
"Z:\home\deck\Games\Might and Magic 4-5"
```

Start the shortcut from Game Mode. The launcher's **Sound setup** and
**Language** windows do not run under Proton, so the game uses the sound
devices of the installed game.

The shortcut's files are inside its Proton prefix, in
`~/.local/share/Steam/steamapps/compatdata/<shortcut-id>/pfx/drive_c/users/steamuser/AppData/Local/MMModern/`.
Edit `window.ini` there while the game is closed. `aspect=square` fills the
Deck's 1280x800 screen, where the default `aspect=4:3` leaves side borders,
and `controller=on` turns on the game controller support. With the controller
on, choose Steam's **Gamepad** template; with it off, use the keyboard layout
below.

### Keyboard layout for the Deck

With the game controller option off, MMModern reads only the keyboard and
the mouse. In the shortcut's controller settings, start from the **Keyboard
(WASD) and Mouse** template and change it to this layout, which was played on
a Steam Deck:

| Deck control | Keyboard or mouse action |
| --- | --- |
| A / B / X / Y | Enter / Escape / Space / Tab (control panel) |
| D-pad and left stick | Arrow keys: forward, back, turn left and right |
| Left stick click / R1 | W (wait) / B (bash or block) |
| L4 / L5 / R4 / R5 | A (attack) / C (cast) / S (shoot) / R (rest or run) |
| Left trackpad, pressed at an edge | Up M (map), down I (info), right Q (quick reference), left F (quick fight) |
| Right trackpad or right stick / triggers | Mouse / left mouse click |
| L1 | Alt+Enter: window or fullscreen |
| Menu / View | Escape / Tab |

Click a portrait in place of F1-F6; touching the screen also clicks. Steam+X
opens the on-screen keyboard for names and number keys.

Do not bind one physical button to both a key and a controller button, or it
will act twice.

## Game controller

The original game has no controller support, so MMModern leaves it off until
you turn it on:

- Windows: tick **Play with a game controller** in the launcher.
- Linux: turn on the **Game controller** row of the setup screen.
- Proton, or by hand anywhere: set `controller=on` in `window.ini` while the
  game is closed.

A controller then plays the whole game, including the title, shops and
typing names. The [controller layout](controls.md#game-controller) lists
every button.

## Russian

MMModern can show the old-games.ru Russian fan translation (Clouds of Xeen
RUS and Darkside of Xeen RUS 1.2) over your English CD game. Put these five
files from the translation in one folder:

- `xeen.cc` from Clouds of Xeen RUS;
- `dark.cc`, `intro.cc`, `xeen.dat` and `xeen.exe` from Darkside of Xeen RUS
  1.2.

In the launcher or the Linux setup screen, open **Language**, choose Russian
and pick that folder. MMModern reads the files and never changes them. Other
versions of the translation are refused, and the game then plays in English.

Texts, pictures and the font come from the translation. Maps, rules, music
and the recorded speech come from the CD, so the speech stays English. The
keyboard follows the Russian program: letters type Cyrillic by their
position on a Russian keyboard, whatever your system layout, and Shift gives
capitals. The Russian program's own key oddities are kept, and
[controls](controls.md) lists them. Russian and English games share the same
saves.

A Russian reader hasn't checked the translation's text in play yet, so
reports are welcome.

## Display

The game window can be resized or maximized, and Alt+Enter switches between
the window and borderless fullscreen. The picture fills the largest 4:3 area
and leaves black borders around it, which is the shape of the original
display. MMModern remembers the window mode and size in `window.ini`.

Set `aspect=square` in that file to use square pixels instead, scaled in
whole multiples of 320x200. Set `aspect=4:3` to go back to the default.

### Picture filters

The original draws its 320x200 picture with plain pixels. MMModern can soften
the enlarged picture with one of the classic pixel-art filters that ScummVM
and DOSBox offer. It is off by default, so the picture stays sharp unless you
choose one, and it works with both picture shapes. A filter changes only how
the picture looks, never the game.

Choose it in the Windows launcher's **Picture filter** list, in the Linux
setup screen's **Picture filter** row (each click picks the next one), or set
`filter=` in `window.ini`. While playing, F11 switches to the next filter,
and to off after the last; MMModern remembers the choice. The names are `off`,
`advmame2x`, `advmame3x`, `advmame4x`, `hq2x`, `hq3x`, `edge2x`, `edge3x`,
`sai2x`, `supersai2x`, `supereagle2x` and `pm2x`; an unknown name means off.

Edge is the heaviest filter. On a slow machine, such as a Steam Deck, it may
draw the picture late, but the game's own timing is not affected. If it feels
slow, pick another filter.

## Sound

The original FM music, digital effects, shop and entrance voices and CD
speech all play. With Text off in the control panel, the NPC text that speech
replaces is hidden; where speech cannot play, the text comes back. Sound
follows the system's default output, including a change while you play. If
the output fails, the game shows one notice and goes on silently; if the
device is removed, MMModern reopens the default one.

Use **Sound setup** in the launcher or the Linux setup screen to choose the
music and speech devices from the original installer's lists. The devices
MMModern can play come first, with Sound Blaster Pro music and Sound Blaster
16 speech recommended, as the CD installs them. The others are listed under
"No sound in MMModern yet": you can still choose one, and the game then
stays silent and shows a notice. AdLib, Sound Blaster, Pro Audio Spectrum
and MT-32 music play.

The installer's port, IRQ, DMA and volume questions are under **Advanced**.
They described the original DOS sound cards and do not change how MMModern
sounds. Your choice is saved in MMModern's own `XEEN.CFG`, and the installed
`WORLD/XEEN.CFG` is never changed. **Reset to installed** removes your choice
and goes back to the installed devices, which are Sound Blaster Pro music and
Sound Blaster 16 speech in GOG's installation. Starting the game from a
command prompt uses the same choice.

For MT-32 music, put your MT-32 1.07 control ROM and MT-32 PCM ROM, under
any file names, in the `roms` folder of
[MMModern's user folder](#where-mmmodern-keeps-its-files). MMModern includes
no ROMs. The launcher reports whether it identified them.

## Saves

Tab, or a click on the gem, opens the control panel with Save, Save As, Load
and Quit, as in the original. There are ten slots.

While MMModern is at 0.x there is one save format, and a new release may
change it. A save from an older release then shows as "Unavailable".
Choosing it tells you why, and New can overwrite that slot after you
confirm. Compatibility will be reconsidered for 1.0.

A save only works with the game data it was made with. The original games
and ScummVM cannot read it.

## Where MMModern keeps its files

MMModern never writes into the game folder. Everything goes into one user
folder:

| System | User folder |
| --- | --- |
| Windows | `%LOCALAPPDATA%\MMModern` |
| Linux | `$XDG_DATA_HOME/MMModern` when that variable is set, otherwise `~/.local/share/MMModern` |
| Proton | `AppData/Local/MMModern` inside the shortcut's prefix ([above](#with-the-windows-package-through-proton)) |

| Inside it | Holds |
| --- | --- |
| `Saves/<game-data-id>/` | The ten slots, `slot-0.mmsave` to `slot-9.mmsave` |
| `window.ini` | Window mode and size, `aspect`, `controller` and `filter` |
| `settings.ini` | The language choice, whether the Darkside intro was watched, and the wins |
| `XEEN.CFG` | Your **Sound setup** choice |
| `roms/` | Your MT-32 ROMs |
| `logs/` | `mmodern.log`, the log of the latest run, and `mmodern-previous.log`, the run before it |
| `game-folder.txt` | The game folder the launcher or setup screen remembered |

The Linux setup screen can open the saves and log folders for you.

On Linux, MMModern refuses to save on a network drive (NFS, SMB, sshfs and
similar). If your home folder is on one, set `XDG_DATA_HOME` to a folder on
a local disk.

### Starting without the launcher

On Windows you can also start the game from a command prompt with the game
folder as its argument:

```text
mmodern.exe "C:\GOG Games\Might and Magic 4-5"
```

On Linux, if the AppImage cannot mount itself because FUSE is unavailable,
run it with `--appimage-extract-and-run`:

```text
./MMModern-<version>-x86_64.AppImage --appimage-extract-and-run
```

The AppImage bundles its own FUSE library, so it does not need `libfuse2`
installed.
