# Maintainer play-tests

Read this before preparing a maintainer play-test. The root `AGENTS.md` says
when a play-test is worth holding and what to settle first. This file says how
to package a play-test and how to write its guide.

## The folder

- Use one folder per session, outside the repository and outside the game
  installation: `D:/Projetos/MModern/MModern-playtest/<milestone>/`, for
  example `m95`.
- Generate each item's save in that folder with the developer presets:
  `build-rel/mmodern_test_save.exe <preset> <game-dir> <output>`. The presets
  and their starting points are listed in [development](development.md). Add
  a preset when none of them starts close enough to the item.
- Whenever possible, give the preset an overleveled party, such as the
  level-30 `m78-playtest` party, so that fights and hazards do not slow the
  session. Use a weaker party only when its strength matters to what the
  item tests.
- Give each item a numbered launcher, `<n>-<short-name>.cmd`, that the
  maintainer double-clicks. The launcher copies the preset to a working file,
  so that every run starts fresh and the preset stays untouched, and then
  starts the game:

  ```bat
  @echo off
  set "PATH=C:\msys64\ucrt64\bin;%PATH%"
  copy /Y "<folder>\<preset>.mmsave" "<folder>\teste-<short-name>.mmsave" >nul
  "D:\Projetos\MModern\mmodern\build-rel\mmodern.exe" --load-game "F:\Games\gog\Might and Magic 4-5" "<folder>\teste-<short-name>.mmsave"
  ```

  Write `.cmd` files with CRLF line endings and literal backslashes.
- Point the launchers at the `build-rel` executable that contains the change
  under test.

## WSL sessions (Linux build)

A play-test of the Linux build runs it under WSLg from the
[WSL clone](dependencies.md#linux-build-under-wsl-build-linux-rel):

- Presets and working copies live on ext4 in WSL, in
  `~/mmodern-playtest/<milestone>/`, because `/mnt/d` is 9P and is refused as
  a save target. The Windows folder above keeps the launchers and the guide.
- Generate the presets with the Linux build's own
  `build-linux-rel/mmodern_test_save` and the ext4 copy of the game data.
- Each item gets its own `XDG_DATA_HOME` under the WSL folder, refreshed from
  a seed on every run, so the session never touches the maintainer's WSL
  user folder and settings such as the watched intro start the same each
  time.
- The `.cmd` launchers call `wsl.exe` with the Linux build:

  ```bat
  @echo off
  wsl.exe -e bash -lc "d=~/mmodern-playtest/<milestone>; rm -rf $d/xdg-<short-name> && cp -a $d/xdg-seed $d/xdg-<short-name> && cp -f $d/<preset>.mmsave $d/teste-<short-name>.mmsave && XDG_DATA_HOME=$d/xdg-<short-name> ~/mmodern-linux/build-linux-rel/mmodern --load-game <ext4-game-dir> $d/teste-<short-name>.mmsave"
  ```

- WSLg relays the window over RDP and the audio through PulseAudio, so it
  cannot settle input feel or native audio; mark such items for a native
  Linux check.
- For DOSBox comparisons, `--dosbox` is Windows-only: copy the Linux preset
  to the Windows folder and export it there with `build-rel`'s
  `mmodern_dos_export --dosbox`, as below.

## The guide

Write the guide in the chat report, in the maintainer's language, as a simple
numbered list.

- **Size.** About 10-15 items and at most about 30 minutes, in route order. A
  milestone with little left unsettled gets fewer items.
- **Each item** gives:
  - its launcher;
  - where the party starts: the map's name and number, the coordinates and
    the facing;
  - numbered steps, with the keys;
  - the expected result, in bold on the step where it shows: the concrete
    text, number, sound or frame, with its evidence;
  - why the item needs a play-test at all.
- **Assume the maintainer has not played that content**, Darkside included:
  say where to go and what triggers it.
- **Movement by coordinates.** Give every move as the cell to reach and the
  facing to end with, for example "walk to (9,5), facing South", and not only
  as a sequence of arrow keys. M shows the map's name, the party's coordinates
  and its facing; x grows to the East and y to the North. The arrow keys may
  follow as a hint.
- **Fights.** Mention any fight the route may bring, and say how to win or
  avoid it.
- **DOSBox.** Mark each DOSBox comparison and give its starting point in
  DOSBox.
- For a DOS comparison, generate the preset and run `mmodern_dos_export`
  with `--dosbox` into a separate folder for that item. Use the generated
  launcher so each run backs up the old overlay and starts from the seed.
  For an immediate re-save item, compare `pristine/WORLD` with
  `overlay/WORLD` using `--diff` and `--resave-check`; name the slots in the
  guide and explain what the check settles.
- **Answers.** The maintainer answers by item number ("3 fail: ..."). A
  re-test after a fix covers only the failed items and reuses the folder.
