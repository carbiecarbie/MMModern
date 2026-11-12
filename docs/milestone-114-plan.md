# Milestone 114 plan - Linux setup screen and AppImage

**Tier B. Status: completed and accepted by the maintainer (2026-11-07)**,
after a play-test under WSLg and on a Steam Deck. Plan `20c6ae3`, amended
`26b865f`; Part A `563006e`, Part B `32e26ba`, play-test fix `b5ec6da`.
Rest of [later goal 2](roadmap.md#later-goals), after M112 (Steam Deck
through Proton) and M113 (native Linux build). Save v26, Journey schema and
content unchanged: nothing here is game state. No gameplay change.

## Scope

A Linux player downloads one `MMModern-<version>-x86_64.AppImage`, starts
it, and sees a setup screen inside MMModern's own window. It does what the
Windows launcher of M100 and M110 does: choose the game folder, make the
original installer's sound choice, pick the language, copy diagnostics, open
the saves and log folders, and play. It is built only on SDL, so that a
later Android port can reuse it. Out of scope: Flatpak, a Windows switch to
the screen (Windows keeps its WPF launcher and package), Android, controller
support, macOS, other CPU architectures and any change to the game itself.

## Key decisions (maintainer, 2026-11-07)

- **A setup screen inside the game**, started by `mmodern` with no
  arguments; every other command line keeps its meaning. A new owner,
  `XeenSetupScreen` in `src/app`, runs before `Application`, because neither
  the title flow nor `Application` exists until the folder, CFG and language
  are known. It reads the `xeenDiagnostics` report in-process and writes
  through the code behind `--sound-setup` and `--language-setup`, so the
  rules of M100 and M110 hold, including the owned `XEEN.CFG`
  ([approved deviation](original-quirks.md#dev-owned-sound-cfg)).
- **320x200 canvas, shared input unchanged.** The screen draws on the game's
  own canvas with ScummVM's built-in 6x12 BDF font, shown with the 4:3 rule,
  and reaches `SdlWindow` through an ordinary dialog input context; mouse
  and touch hit rows, Tab and arrows move the focus, and typed or pasted
  text reaches the path line. Long lists page with Previous and More.
- **Game folder.** `game-folder.txt` in the user folder, then the immediate
  subfolders of `~/Games/Heroic`, `~/GOG Games` and the default Wine
  prefix's `drive_c/GOG Games`, then a folder browser. A `WORLD` folder
  resolves to its parent, as on Windows.
- **AppImage** built in CI in the pinned Ubuntu 22.04 container with SDL2
  2.32.10, a `$ORIGIN/../lib` run path and no `LD_LIBRARY_PATH`, a desktop
  file, an MMModern-owned icon (a shield with sword, scepter and "MM",
  approved by the maintainer) and the licenses; `appimagetool` and the
  type-2 runtime pinned by version and SHA-256. The manual workflow replaces
  M113's test bundle.

## Results

- The complete suite passes on Windows and on WSL Linux, 421 tests each;
  `xeen_launcher` passes unchanged. Golden frames of the main screen, Sound
  setup with Advanced open, paged Music, Language, the browser and a failed
  start were inspected before their digests were recorded. An original-data
  test starts with no arguments and a remembered folder and reaches the
  original title with the chosen CFG and language.
- The AppImage workflow is green; its checks cover the run path, `--version`
  and `--diagnostics` through `--appimage-extract-and-run`.
- Play-test: the first run, Sound setup to MT-32 and back, Russian and back,
  Copy diagnostics and keyboard paging under WSLg; the AppImage on a Steam
  Deck in Desktop Mode, and as a non-Steam game in Game Mode, by touch and
  trackpad. Game Mode found that SDL sends Steam an open- or close-keyboard
  request on every text-input start or stop, which `SdlWindow` made every
  loop cycle, so the folder browser's path line kept the Steam keyboard
  open. `SdlWindow` now changes text input only when it changes, and the
  browser opens on "Use this folder"; the re-test passed.
