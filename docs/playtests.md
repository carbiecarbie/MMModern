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
- **Answers.** The maintainer answers by item number ("3 fail: ..."). A
  re-test after a fix covers only the failed items and reuses the folder.
