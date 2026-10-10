# Milestone 54 plan - Original title menu and saves

**Tier A. Status: completed and accepted.** Plan `abcda8c`; Part A
`4ab4762`; Part B `11e8c81`; review fixes `a760e69`, `1941416`.

## Goal

Replace M52's temporary command-line entry with the original DOS interface of
the CD edition: the title screen, New with slot, name and difficulty, Load,
and the in-game control panel with Save, Save As, Load and Quit, by mouse and
keyboard. Save/restore stays exact.

## Final scope

- **Title (Part A).** Plain launch opens the CD title: animated art from
  DARK.CC (explicit archive selection, no fallback) with the reference fade
  transitions; Escape toggles the menu. Four-page credits (CD `credits.bin`)
  and Other Options, whose intro buttons say "not supported yet".
- **New and Load.** New: slot -> overwrite confirmation -> name ->
  difficulty, written to the slot before the first gameplay frame. The
  shared ten-slot chooser selects by number key or row click and confirms
  with Select; its arrows and bar do nothing, as in DOS. Load restores a
  validated snapshot and restarts the session through an outer loop; cancel
  or failure keeps the running game.
- **Control panel (Part B).** Opened by Tab or the gem, laid out from
  `WORLD/XEEN.DAT`: Save to the current slot (set by New, Load or the last
  Save As), Save As with the original overwrite, name and saved notices, Load
  and Quit with its confirmation; the original combat refusal and map save
  restriction. Efx, Music, Speech, Text and rescue refuse visibly; Mr Wizard
  confirms first. Typed text shows the original pulsing cursor.
- **Storage.** Ten slot files under Local AppData
  (`MMModern/Saves/<archive key>/`), written atomically with a final target
  check; protected (corrupt, foreign, newer) slots stay visible as unavailable
  rows. Developer modes (`--new-game`, `--journey-region`, `--load-game`,
  loose `--save-file`/F9) remain for tests; loose saves also refuse to
  overwrite a save made from different game data.

## Decisions

1. **DOS evidence over ScummVM**, whose panel and save chooser differ: the
   maintainer's DOSBox observations set the flows (ten slots, overwrite
   prompts, Escape paths, current-slot rule, name entry, Tab/gem).
2. **Approved:** unavailable rows for protected slots; with no current slot
   (developer modes) panel Save asks for a slot; title cadence and fades
   follow the reference unless play shows a visible difference.
3. **Following the reference:** each slot choice starts with an empty name,
   names cannot start with a space, and the cursor starts at its smallest
   square.
4. **Save v7** adds the slot name (printable ASCII, up to 20 bytes). The M44
   digests changed only by format bytes (version, length, CRC and an empty
   name byte); traces and decoded fields are identical.

## Results

- Tests for storage and writer faults, the v7 format, session restart and
  stale-input rejection, title, credits, chooser and panel flows; bare-launch
  tests moved to explicit developer modes.
- Full CTest 173/173 on the optimized and on the Debug build.
- Independent implementation review (REVISE): title animation jumped after
  modal screens, name and cursor details, DAT-derived hotkeys and missing
  tests; fixed. The Debug closure check then caught a test reading a retired
  input ticket (an empty optional), fixed in the test.
- Maintainer play-test against the DOSBox CD edition passed.
