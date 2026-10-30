# Milestone 100 plan - Launcher and diagnostics

**Tier B. Status: completed and accepted** by the maintainer on 2026-10-30
after the play-test. Plan `3a6d585`, implementation `e4703d7` and the closure
commit. Follows roadmap
[step 4](roadmap.md#next-arc-release-quality-and-the-darkside-start). Save
v22, Journey schema 9 and content 14 unchanged: nothing here is game state.

## Goal

A player double-clicks one file and sees a launcher window with the version,
the game folder and its data, the sound devices, the MT-32 ROMs and the save
folder. From it the player starts the game, chooses the sound devices as the
original installer let them, and copies a diagnostics report. The game runs
without a console and writes a log; a GitHub issue template asks for the
report, the log and the save.

## Key decisions (maintainer, 2026-10-30)

1. **Sound setup as the original installer.** The launcher asks the
   installer's questions and writes an MMModern-owned
   `%LOCALAPPDATA%\MMModern\XEEN.CFG`; the game folder's `WORLD/XEEN.CFG` is
   never changed. Every installer row can be chosen, and devices MMModern
   does not play yet are marked silent. This is an
   [approved deviation](original-quirks.md#dev-owned-sound-cfg) in where the
   choice is stored, not in what it means.
2. **A launcher window on every start**, Play focused so Enter starts the
   game.
3. **No console window; a log file** in `%LOCALAPPDATA%\MMModern\logs`.
   Developer modes keep printing to the terminal and write no log.
4. **The launcher stays PowerShell with WinForms**, over tested C++ commands
   in `mmodern.exe`, so a later Linux launcher only wraps the same commands.

Planning also decided that the player entry uses the owned CFG even when
started from a command prompt (`--xeen-cfg` overrides it, developer modes
ignore it), that the setup asks the installer's port, IRQ and DMA questions
because MMModern's card mixer and DMA digital use those fields, and that
profile paths appear as `%LOCALAPPDATA%` or `%USERPROFILE%` in the report and
the log.

## Results

- `XeenSoundSetup` holds the installer's two menus, ids, ports and questions
  from the M94 installer findings; applying a choice changes only the asked
  fields and keeps 29- and 30-byte bases. Played or silent comes from the
  existing deferred-device predicates.
- `mmodern --version`, `--diagnostics <game-dir>` (data, data id, CFG source
  and devices, MT-32 ROM state without opening a synth, save folder, setting
  and log paths) and `--sound-setup` (menus, atomic write, `--reset`).
- The player entry tees its output, one line at a time, to `mmodern.log`,
  keeps the previous run as `mmodern-previous.log`, logs each "not supported
  yet" notice once, and keeps running if the log cannot be written.
  `MMODERN_LOG_FILE` and `MMODERN_PLAYER_DIR` isolate tests.
- The launcher window (WPF, sharp at any display scale) summarizes the game,
  music, speech, MT-32 ROMs and saves, with Play, Sound setup, Change game
  folder, Copy diagnostics and folder links; it starts the game hidden from
  the console and brings it to the front, and after a failed start shows the
  log's last lines. `Change game folder.bat` left the package.
- Sound setup, revised after the play-test (maintainer, 2026-10-30): no
  installer menu keys, the devices MMModern plays first with Sound Blaster
  Pro music and Sound Blaster 16 speech recommended, the others under "No
  sound in MMModern yet", a note when MT-32 lacks its ROMs, and the port,
  IRQ, DMA and Yes/No volume questions under a collapsed Advanced section.
  IRQ and DMA do not affect MMModern's output.
- `.github/ISSUE_TEMPLATE` has a bug report form and `config.yml`.
- Play-test feedback: New and Load became very slow, because the log
  forwarded each character separately to the hidden console. The log now
  writes in blocks, with a regression test that counts writes.
- Tests: every installer row pair on both base lengths, the CLI and launcher
  functions from PowerShell 5.1, CFG selection per entry, the log, and ROM
  states. Complete suite 374/374.
- Play-test (2026-10-30): the window and zip flow, MT-32 music after setup
  and SB Pro after reset, the game in front with no console, the report and
  log, and normal speed after New and Load; all passed. A short re-check of
  the revised window and Sound setup (sharpness on a 4K display, the device
  lists, MT-32 and reset, Advanced) also passed. Complete suite after the
  revision: 374/374.
