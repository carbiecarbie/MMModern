# Milestone 114 plan - Linux setup screen and AppImage

**Tier B. Status: planned (2026-11-07).** Rest of
[later goal 3](roadmap.md#later-goals), after M112 (Steam Deck through
Proton) and M113 (native Linux build). Save v26, Journey schema and content
unchanged: nothing here is game state. No gameplay change.

## Goal

A Linux player downloads one `MMModern-<version>-x86_64.AppImage`, starts
it, and sees a setup screen inside MMModern's own window: the version, the
game folder and its data, the sound devices, the MT-32 ROMs, the language
and the save folder. From it the player chooses the game folder, makes the
original installer's sound choice, picks the language, copies a diagnostics
report and plays. The screen does what the Windows launcher of M100 and M110
does, built only on SDL, so that a later Android port can reuse it.

## Decisions (maintainer, 2026-11-07)

1. **A setup screen inside the game**, not a dialog script: `mmodern` started
   with no arguments opens it. Every other command line keeps its meaning.
2. **AppImage**, not Flatpak, built in CI like the M113 test bundle.
3. **One milestone, two parts**, one play-test after Part B.
4. **Windows keeps its WPF launcher and package unchanged.** The screen is
   portable and its tests run on both platforms, but only the AppImage
   starts it for now.

## Design

- **Its own owner, before the game.** The screen is a project UI outside
  the original game, like the Windows launcher, so it is a new owner
  (`XeenSetupScreen` in `src/app`) that runs before `Application` is built
  and returns a game folder or Quit. The game then starts as the player
  entry does today: same diagnostics in the log, same owned `XEEN.CFG`, same
  translation, same window placement. Reason for a new owner: neither the
  title flow nor `Application` exists until the folder, the CFG and the
  language are known. Sound and language choices therefore apply to the run
  they are made for, with no restart.
- **The same facts and writes as the Windows launcher.** The screen reads the
  `xeenDiagnostics` report in-process (`game.status`, devices, ROMs,
  language, saves, log) and changes the sound choice and the language
  through the code behind `--sound-setup` and `--language-setup`. The rules
  of M100 and M110 hold: the game folder's `WORLD/XEEN.CFG` is never written;
  devices MMModern plays come first, with Sound Blaster Pro music and Sound
  Blaster 16 speech recommended; the others are under "No sound in MMModern
  yet"; MT-32 without its ROMs gets the note; port, IRQ, DMA and the volume
  question sit under a collapsed Advanced section; Reset returns to the
  installed choice; a refused translation keeps English with the reason.
- **Game folder.** Kept in `game-folder.txt` in the user folder of
  `XeenUserFolders` (`MMODERN_PLAYER_DIR` overrides it, as for the Windows
  launcher). Order: the remembered folder, then known install locations of
  the GOG game on Linux, then the folder browser. A chosen `WORLD` folder
  resolves to its parent, as on Windows. The install locations are a short
  fixed list of roots, each documented by its tool: `~/Games/Heroic`
  (Heroic's default games folder), `~/GOG Games` (GOG's Linux default) and
  `drive_c/GOG Games` in the default Wine prefix (GOG's Windows default
  `C:\GOG Games`; Wine's `wine(1)` manual gives `$HOME/.wine` as the default
  prefix, kept only if its documentation confirms the prefix and `drive_c`).
  Each root's immediate subfolders are checked in sorted order, and a
  folder counts only when the diagnostics call it usable.
- **Folder browser.** Lists subfolders, sorted, with Parent, Home and the
  removable-media roots (`/media/$USER`, `/run/media/$USER`); hidden folders
  appear after a toggle, since Wine prefixes are hidden. A path line accepts
  typed or pasted text. An unreadable folder shows a message, never a crash.
- **Look and input.** A 320x200 indexed canvas, the game's own size, shown
  with the game's 4:3 rule and the remembered window placement, drawn with
  one of ScummVM's built-in BDF fonts from the pinned revision (GPL; no
  original font exists before a folder is chosen), chosen in Part A from
  rendered frames. `SdlWindow` presents any frame size, but its mouse path
  and `DialogInput` accept only 320x200 (maintainer, 2026-11-07: keep that
  shared input path unchanged). So the screen gives `SdlWindow` an ordinary
  dialog input context: clicks and keys reach it as dialog hits and keys,
  and `SdlWindow` and `DialogInput` stay as they are. Long paths show their
  end after an ellipsis; Sound setup may span pages. Mouse and keyboard: Tab and arrows
  move the focus, Enter activates, Escape goes back or quits; Play is
  focused, so Enter starts the game, as in M100. English project text;
  characters the font lacks show as its default glyph.
- **Actions.** Play; Change game folder; Sound setup; Language; Copy
  diagnostics (SDL clipboard; the notice says whether it worked); Open saves
  folder and Open log folder (`xdg-open`, started with `posix_spawn`, with
  the environment the AppImage received). After a failed start the screen
  comes back with the error and the log's last lines, as on Windows. A
  normal end of the game ends the process, as quitting the original does.
- **AppImage.** An AppDir with `usr/bin/mmodern`, its non-system libraries
  in `usr/lib` found through a `$ORIGIN/../lib` run path (no
  `LD_LIBRARY_PATH`, so `xdg-open` and its children see a clean
  environment), `AppRun`, a desktop file, an MMModern-owned icon and, under
  `usr/share/doc/mmodern`, `LICENSE.txt`, `SOURCE.txt` and the copyright
  files. The library selection, the pinned SDL2 build and the Ubuntu 22.04
  container come from M113's test bundle; the AppImage job replaces that
  bundle, whose `GUIDE.txt` and `mmodern.sh` leave. `appimagetool` and the
  type-2 runtime are pinned by version and SHA-256; their licenses (and
  those of what the runtime links statically) go into `SOURCE.txt`.
- **Icon.** A simple MMModern-owned icon drawn for the project, with no
  original art. The maintainer approves the image in Part B; the Windows
  executable keeps having none.
- **No new quirk or deviation.** The screen is outside the original game,
  like the Windows launcher; the owned CFG is the existing
  [approved deviation](original-quirks.md#dev-owned-sound-cfg).

## Part A - setup screen

- [ ] Confirm the Wine prefix root from Wine's documentation (drop it if
      unconfirmed) and choose the font from rendered 320x200 frames. Report
      before going on if any screen needs `SdlWindow` or `DialogInput` to
      change, or typed text cannot reach the screen through a dialog
      context.
- [ ] `XeenSetupScreen` with its model (focus, rows, dialogs, browser,
      notices) separate from drawing, and a no-argument entry in `main`.
- [ ] Folder logic: remembered file, candidate list, `WORLD` parent rule,
      browser listing, hidden toggle, typed path, unreadable folders.
- [ ] Sound setup and Language through the existing owners; Copy
      diagnostics; Open folders; failed-start return with the log's tail.
- [ ] Tests, on Windows and Linux: unit tests of the model and the folder
      logic; golden frames of the main screen, Sound setup with Advanced
      open, Language, the browser and a failed start, inspected by the agent
      before their digests are recorded; keyboard and mouse sequences that
      reach Play; one original-data test (`mmodern_data_test()`) in which a
      no-argument start with a remembered folder reaches the original title
      with the chosen CFG and language.

## Part B - AppImage, CI and documentation

- [ ] AppDir, run path, `AppRun`, desktop file (checked by
      `desktop-file-validate`), icon, licenses and `SOURCE.txt`.
- [ ] The manual CI workflow builds the AppImage in the pinned container,
      checks that `mmodern` has the run path and needs no `LD_LIBRARY_PATH`,
      runs `--version` and `--diagnostics` on an empty folder through
      `--appimage-extract-and-run`, and uploads the AppImage with its
      SHA-256. The M113 bundle workflow becomes this one.
- [ ] Agent checks under WSL: the AppImage through FUSE and through
      `--appimage-extract-and-run`; the screen, a new game, save, quit and
      reload with the original data; the log and `game-folder.txt` in
      `~/.local/share/MMModern`.
- [ ] README: a Linux section (download, `chmod +x`, start, the setup
      screen, where saves and the log are, FUSE note) next to the Windows and
      Steam Deck ones; `docs/dependencies.md` for `appimagetool`, the runtime
      and the job. Release notes say the save policy, as every release does.
- [ ] Play-test preparation as `docs/playtests.md` describes. No DOSBox
      comparison saves: no item compares behavior with DOS.

## Acceptance

- The complete CTest suite passes on Windows and on WSL Linux. The Windows
  launcher, its package layout and every existing command line behave as
  before; `xeen_launcher` passes unchanged.
- Both CI workflows are green, and the AppImage job's checks pass.
- Maintainer play-test, only what nothing cheaper settles: the screen's feel
  by mouse and keyboard on a real Linux desktop (WSLg is enough); the
  AppImage on the Steam Deck in Desktop Mode, and added to Game Mode as a
  non-Steam game, by touch and trackpad; a first run that finds the GOG
  folder, Sound setup to MT-32 and back, Russian and back, Copy diagnostics,
  Play.

## Out of scope

Flatpak and Flathub, a Windows switch to the screen, Android, controller
support, macOS, other CPU architectures, the GOG Windows registry lookup in
C++, and any change to the game itself.
