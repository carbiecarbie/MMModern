# Milestone 100 plan - Launcher and diagnostics

**Tier B. Status: planned (not started).** Follows roadmap
[step 4](roadmap.md#next-arc-release-quality-and-the-darkside-start).
Baseline: `41a8ed4`, equal to `origin/main`. Save v22, Journey schema 9 and
content 14 unchanged: nothing here is game state.

## Goal

A player double-clicks one file and sees a small launcher window. It shows the
MMModern version, the game folder and the data found there, the sound devices
in use, the MT-32 ROMs and the save folder. From the window the player can
start the game, choose the sound devices as the original installer let them,
and copy a diagnostics report. The game runs without a console window and
writes a log file. A GitHub issue template asks for the report, the log and
the save.

## Key decisions (maintainer, 2026-10-30)

1. **Sound setup as the original installer.** The launcher asks the
   installer's questions and writes an MMModern-owned
   `%LOCALAPPDATA%\MMModern\XEEN.CFG`. The game folder's `WORLD/XEEN.CFG` is
   never changed. Every device the installer lists can be chosen. Devices
   MMModern does not play yet are marked as silent in MMModern, and the game
   shows its notice as today. MT-32 becomes selectable for GOG players. This
   is an approved deviation in where the choice is stored, not in what it
   means.
2. **A launcher window on every start**, with Play focused so that Enter
   starts the game.
3. **No console window; a log file.** The launcher starts the game without
   a console. Every diagnostic also goes to
   `%LOCALAPPDATA%\MMModern\logs\mmodern.log`. When the game fails, the
   launcher shows why and offers to open the log. Developer modes keep
   printing to the terminal and write no log.
4. **The launcher stays PowerShell with WinForms.** All facts and all CFG
   writing come from tested C++ in `mmodern.exe` (`--diagnostics`,
   `--sound-setup`), so detection is never duplicated in the script. This
   also suits the later Linux port: a Linux launcher only needs to wrap the
   same commands.

Further decisions made in planning:

- The player entry (`mmodern <game-dir>`) uses the MMModern-owned CFG when it
  exists, so the choice also applies when the game is started from a command
  prompt. `--xeen-cfg` still overrides it. Developer modes ignore the owned
  CFG, so tests and presets keep using the installed one. An owned CFG that
  is unreadable or malformed stops startup with a message that names it. The
  launcher offers to reset it, which deletes it.
- Paths under the user's profile appear as `%LOCALAPPDATA%\...` or
  `%USERPROFILE%\...` in the report and the log, so a pasted report does not
  reveal the Windows user name.

## Evidence already settled

The installer's menus, CFG ids, port tables, questions and write rules are
in the M94 Part 0 findings (`MModern-re/m94/findings-0a.md` section 2,
installer offsets in the unpacked `WORLD/INSTALL.EXE`). In summary:

- Speech first, then music. Speech rows 1-9, 0 write digital ids
  `0,2,1,6,7,8,5,9,10,6` (the row "Pro-Audio Spectrum Alt" writes 6).
  Music rows write `0,1,7,2,3,6,5,4,8,9` with music ports `retain, 0x388,
  retain, 0x388, 0x220, 0x388, 0x330, 0x330, 0x330`. Roland TAP-10 asks for
  its port from `0x220,0x230,0x240,0x250,0x320,0x330,0x340,0x350`.
- Digital port question: SB, SB Pro, SB16 and PAS Alt choose from
  `0x210..0x260`, and Covox from `0x22f,0x24f,0x28f,0x2cf`. IRQ question
  (`2,3,5,7,10`): SB, SB Pro, Covox, SB16 and PAS Alt. DMA question
  (`0,1,3`): SB Pro and SB16. SB, SB Pro, SB16 and PAS Alt then ask
  "Automatically adjust Sound Blaster volumes?", which sets the word at
  `+0x14`. A field without a question keeps its previous value.
- The installer writes 29 bytes. Other fields keep the values read from the
  existing CFG.

MMModern's output depends on these fields: the card mixer is addressed
through the music or digital port, DMA digital uses the DMA word, and the
mixer flag sets the initial levels. So the setup asks the port, IRQ and DMA
questions as the installer does, not only the device.

## Tasks

- [ ] **Version.** `mmodern --version` prints `MMModern <version>` from
      `project(MMModern VERSION ...)` through a generated header. It needs
      no game folder.
- [ ] **Sound setup rules** (pure C++, beside `XeenAudioConfig`): the two
      installer menus with their captions, ids, music ports and question
      lists as above. Applying a choice to the bytes of a base CFG writes
      ids and ports and asks only the installer's questions. Every other
      byte is kept, 29- and 30-byte bases included. The rules also say
      whether MMModern plays each device or stays silent with a notice,
      taken from the existing `deferredMusic` and `deferredDigital`
      predicates, not from a second list.
- [ ] **`mmodern --sound-setup <game-dir> ...`** lists the menus and the
      current answers in a line-based format the launcher reads. With
      answers, it writes the owned CFG atomically (temporary sibling, then
      replace), based on the existing owned CFG or else on the installed
      one. `--sound-setup <game-dir> --reset` deletes the owned CFG.
- [ ] **Owned CFG at the player entry**, as decided above. Every CFG reader
      (`XeenAudio`, cutscene data, caption gates) follows the selected file,
      as `--xeen-cfg` does since M94.
- [ ] **`mmodern --diagnostics <game-dir>`** prints a plain-text report
      with stable `key: value` lines:
  - version and Windows version;
  - game folder and the data found: edition, archives, `WORLD/XEEN.EXE`
    usable for the intros and ending, and the game data id used for saves;
  - which CFG is used (installed or owned) and the music and digital devices
    with their installer captions, ports, IRQ, DMA and automatic volume,
    each marked as played or silent;
  - MT-32 ROMs: folder, and found and identified (control 1.07, PCM), wrong
    or missing, using the same identification as `XeenMt32` without
    opening a synth;
  - the save folder with the number of used slots, and the paths of
    `settings.ini`, `window.ini` and the log.

  A missing or unusable game folder is reported, not thrown.
- [ ] **Log file.** At the player entry, `std::cerr` and `std::cout` also go,
      line-flushed, to `mmodern.log`. The previous run's log is kept as
      `mmodern-previous.log`. The log starts with a header: the date, the
      version, the game folder, the CFG source and the devices. Each "not
      supported yet" notice the game shows is also logged; add a line where
      a notice is not already written to `std::cerr`. An uncaught exception
      in `main` is logged before exit. `MMODERN_LOG_FILE` redirects the log
      for tests, as `MMODERN_SETTINGS_FILE` does. If the log cannot be
      written, the game still runs.
- [ ] **Launcher window** (`tools/release/MMModern-launcher.ps1`). It keeps
      the current folder search: remembered folder, then GOG registry, then
      the folder picker. The window shows the report's main facts and has
      these buttons:
  - **Play**, the default (Enter): starts `mmodern.exe` without a console
    window. The game window must come to the front.
  - **Sound setup**: a dialog with the two installer lists, then only the
    questions the installer would ask for that choice, then **Save** or
    **Reset to installed**.
  - **Change game folder**.
  - **Copy diagnostics**: the full report to the clipboard.
  - **Open saves folder** and **Open log folder**.

  The window stays open while the game runs and returns when it closes. If
  the game exits with an error, a message shows the log's last lines and
  offers to open the log. The script stays ASCII for Windows PowerShell 5.1.
  Its non-UI functions (report parsing, argument building) can be
  dot-sourced and tested without a window.
- [ ] **Package** (`tools/package-release.ps1`): `MMModern.bat` opens the
      launcher with a hidden PowerShell window. `Change game folder.bat`
      is removed, since the window has that button. The zip otherwise stays
      as today.
- [ ] **Issue template**: `.github/ISSUE_TEMPLATE/bug_report.yml`, plus
      `config.yml` with blank issues still allowed. It asks for the version,
      what happened and what was expected, steps with map and coordinates
      when known, and the pasted diagnostics report. It asks to attach
      `mmodern.log` (and `mmodern-previous.log` after a crash) and the save
      slot, and says where they are. It reminds reporters never to attach
      original game files.
- [ ] **Documentation in the same change:**
  - README: "Download and play", "Display and sound" and a short "Reporting
    a problem" section;
  - `docs/development.md`: the three new options, the owned CFG,
    `MMODERN_LOG_FILE`, and `--xeen-cfg` no longer described as play-test
    only;
  - `docs/original-quirks.md`: an approved-deviation entry for the owned
    CFG, with its anchor, the maintainer's date, the installer evidence, and
    a `**ScummVM:**` tag stating how the pin chooses the sound device, as
    read from its source;
  - the matching `docs/scummvm-differences.md` item;
  - `docs/rules/audio.md`: a sentence where the CFG is selected.

## Tests and evidence

- Unit: every menu row writes the installer's ids and ports. The PAS Alt
  row gives digital 6. TAP-10 asks for its port. A field without a question
  keeps the base value, for 29- and 30-byte bases. The automatic-volume
  word follows the answer. A malformed base is refused. The atomic write
  and reset are covered.
- Original data: `--diagnostics` on the installation reports the expected
  data id, matching the save folder, and the installed CFG's SB Pro music
  and SB16 digital. With an owned CFG that selects MT-32 it reports the
  owned CFG and MT-32. ROM states are checked with `MMODERN_MT32_ROM_DIR`
  (identified), an empty folder (missing) and a wrong file (wrong). A
  missing game folder is reported, not thrown.
- Player-entry CFG selection: the owned CFG is used by the player entry and
  ignored by developer modes, `--xeen-cfg` overrides it, and an unreadable
  owned CFG stops startup with its name.
- Log: a player-entry process test with `MMODERN_LOG_FILE` checks the
  header, a diagnostic line, the rotation to `mmodern-previous.log` and
  that developer modes write none.
- Launcher: a PowerShell test, run by CTest like
  `XeenItemCatalogGenerationTests.ps1`, dot-sources the non-UI functions
  and checks report parsing and the game command line, including paths
  with spaces.
- The agent runs the packaged launcher itself: the window, Play, Sound setup
  saving MT-32, then reset, Copy diagnostics, and a failing start. It takes
  screenshots and inspects them before the play-test.

## Acceptance

- One double-click shows the launcher. Enter starts the game in front, with
  no console window. A failure is explained, with the log one click away.
- Sound setup writes only the owned CFG, with the installer's ids, ports and
  questions. The installed `XEEN.CFG` is byte-identical before and after.
- The diagnostics report and the log contain the facts above, without the
  Windows user name.
- The issue template is in place. The complete CTest suite passes on
  `build-rel`.
- Maintainer play-test, one short session after the implementation, on what
  only real use settles:
  - how the window looks and the flow from the release zip;
  - hearing MT-32 music after choosing it, then SB Pro again after reset;
  - that the game comes to the front with no console window;
  - whether a pasted report and the log are useful in a draft issue.
