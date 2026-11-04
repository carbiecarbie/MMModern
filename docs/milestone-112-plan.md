# Milestone 112 plan - Steam Deck through Proton

**Tier B. Status: planned (2026-11-04).** First half of
[later goal 3](roadmap.md#later-goals): the native Linux build, its CI job
and its package follow as their own milestone (Tier A: platform layer and
save owner). Save v26 unchanged; no gameplay change.

## Goal

The Windows release package plays on a Steam Deck through Proton, started
from Game Mode: the original title, a new game, play by Steam Input and
trackpad or touch, sound and CD speech, saving, quitting and loading, and
sleep and resume. A README section tells players how to set it up.

## Known facts

- The 2026-10-17 proof ran `mmodern.exe` under Wine 10 with `SDL2.dll`,
  `zlib1.dll` and `libwinpthread-1.dll`: title, play and save worked. Saving
  at a volume root (`C:\x.mmsave`) failed in `GetFileInformationByHandle`;
  normal saves go under `%LOCALAPPDATA%\MMModern\Saves` in the Proton prefix.
- The launcher (`MMModern.bat`, WPF PowerShell) does not run under Wine. On
  the Deck, Steam starts `mmodern.exe "<game folder>"` directly: player mode
  writes the diagnostics and the player log under `%LOCALAPPDATA%\MMModern`
  and uses the installed `WORLD/XEEN.CFG` sound choice.
- The Deck screen is 1280x800, four times 320x200. The default 4:3 picture
  is 1066x800 with side borders; `aspect=square` in `window.ini` fills the
  screen. Both exist; the default stays 4:3.
- The game takes keyboard and mouse only. Native controller support is a
  later opt-in goal and out of scope: on the Deck, Steam Input maps buttons
  to keys, and the trackpads and touchscreen act as the mouse.
- WSL Ubuntu has Wine 10 and `xvfb-run` (checked 2026-11-04).

## Part A - agent checks and package (no Deck)

- [ ] Package the current `main` `build-rel` with
      `tools/package-release.ps1` into `D:/Projetos/MModern/MModern-playtest/m112/`
      (a test package, not published).
- [ ] Under Wine 10 in WSL (X server or `xvfb-run` at 1280x800, data copied
      to the Linux file system because `/mnt/f` is slow), run the unpacked
      package: `--version`; `--diagnostics` reports usable data; the player
      title starts with the game folder given both as `Z:\...` and as a Linux
      absolute path; a new game reaches Vertigo; an in-game save, quit and
      load restore the position; the player log is written; fullscreen
      frames at 1280x800 for both `aspect` settings are captured and
      inspected. Record each result in the report.
- [ ] Fix only small defects that block play under Proton (Tier B). A fix
      in the save owner, timing, the platform layer's file rules or the
      ScummVM integration stops Part A and goes to the maintainer as a
      Tier A maintenance fix with independent review. Any fix gets tests and
      the complete suite (`tools/AGENTS.md`).
- [ ] A suggested Steam Input layout derived from [controls](controls.md):
      movement on the D-pad and left stick, the mouse on the right trackpad
      with a trigger as left click, the most used commands (Enter, Esc,
      Space and the command letters) on the face and back buttons, 1-6 for
      the characters, and Steam+X for the on-screen keyboard in names.
- [ ] README section "Steam Deck (Proton)": copy the game folder (or install
      the GOG game with a launcher such as Heroic), unpack the zip, add
      `mmodern.exe` as a non-Steam game, the launch option with the game
      folder (the form verified under Wine), Proton in Compatibility, the
      Steam Input layout, where `window.ini` and the saves are in the prefix,
      and that the launcher's Sound setup and Language do not run there.
- [ ] Prepare the play-test (below) as [play-tests](playtests.md) describes,
      adapted to the Deck: the package, the preset saves and the guide with
      Steam launch options per item instead of `.cmd` launchers.

## Play-test (maintainer, on the Steam Deck)

Everything here needs the device: Proton and Gamescope, the Deck's audio
and sleep, Steam Input, the touchscreen and the 1280x800 panel cannot be
captured locally. No item compares behavior with DOS, so this session has
no DOSBox comparison saves. About 8 items, 20 minutes:

1. Start from Game Mode: the title fills the screen in 4:3 with music and
   speech.
2. New game, walk in Vertigo with the D-pad.
3. Interface buttons by trackpad click and by touch.
4. Text entry with the on-screen keyboard (a save name).
5. Save, quit to Game Mode, start again and load.
6. A fight from an overleveled preset, by buttons only.
7. Sleep with the power button during play and resume: the picture and
   sound return and play continues.
8. `aspect=square` fills the screen.

## Acceptance

- Part A results reported; blocking defects fixed or stopped as above.
- The maintainer's Deck play-test passes; failed items are re-tested after
  fixes.
- The README section matches what was verified. Publishing a release with
  it (0.17.0, which would also bring M111) is a separate decision.

## Out of scope

The native Linux build, a POSIX platform layer, a Linux CI job and a Linux
package (next milestone); controller support; a launcher that runs under
Proton; sound setup and the Russian language on the Deck beyond the
installed defaults; any save format change.
