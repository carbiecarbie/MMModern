# Milestone 64 - Vertigo closure and first public release

**Tier B, in three parts. Status: in progress.** Plan `e7438a2`; Part A
`c080eb1`; Part B `080b05f` (v0.1.0 published 2026-10-14); Part C approved.

## Goal and boundary

Close [roadmap](roadmap.md) step 10: fix the defects the maintainer found in a
full Vertigo play-through (New Game with save/load, 2026-10-14), then publish
the first public release for Windows. Baseline: clean
`main = origin/main = 24d6ce1acbc2268c6f0b011920128eb8be73bc83`. No new
content, mechanics or save-format change (v12, schema 9, content 14). Authority
is as in [M63](milestone-63-plan.md): calibrated `WORLD/XEEN.DAT` analysis,
maintainer DOSBox captures and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

## Part A - play-through fixes

1. **Sparkle only on the original grant types.** The maintainer's DOSBox
   capture of Gunther's reward (`recompensa gunther.mkv`) shows one sparkle
   pass over the six portraits, then "+5,000 experience", then The Party Found.
   Gunther's script (map 28 `(14,5,West)` lines 13-17) gives XP (type 16) and
   then sets game flag 10 (type 20); MMModern sparkles twice. The pin's
   `Party::giveTake` sparkles only for HP (8), skills (13), awards (15, except
   award 8), experience (16) and spells (19). Confirm the original give dispatch
   with REA where cheap and restrict sparkles to those types for every
   Give/Take/GiveMulti path, keeping M63's per-recipient sequence. Check that the
   "+N experience" side-panel text appears when and where the capture shows it.
2. **Equip conflict message.** "You must remove Brass great axeto equip Spear!"
   glues words: the original template `REMOVE_X_TO_EQUIP_Y` expects the base
   item names from the original name tables, which end in a space (`\b` removes
   it before "!"). The pin's `InventoryItems::equipError` passes those base
   names, conflicting item first. Fix `XeenInventoryFlow` and check the other
   item messages built from the same templates (Not Proficient and similar).
3. **Window title.** Normal play shows developer state in the title bar ("Map 28
   - Inventory: 6 active references, 30 owners; Root=0 Q2=0"). The player sees
   only "MMModern"; developer entry modes may keep their diagnostics.

Unit tests for each fix with the exact rendered text or sparkle sequence. The
M44 audit runs as usual: well and other XP/HP sparkles must keep their traces;
any difference is explained and approved before digests change.

## Part B - release

1. **Validation.** Complete suite through the runner on `build-rel` after Part
   A, and the complete suite on `build-m44` as `tools/AGENTS.md` requires before
   a public release.
2. **Package.** The tested `build-rel` configuration, with debug information
   stripped, plus exactly the runtime DLLs it needs from UCRT64 (today SDL2,
   zlib and winpthread; resolve with `ldd`, never from another toolchain's
   folder). Include `LICENSE`, a short third-party notice (SDL2, zlib,
   winpthread and the GCC runtime exception; ScummVM portions under GPLv3+) and
   where the corresponding source is (repository tag and pinned ScummVM
   revision). Verify the zip on a clean folder and on a machine path without
   MSYS2 in `PATH`. No original game data, saves or build trees.
3. **Player documentation.** README: what the release is (Vertigo and the
   map-23 mainland of Clouds, World of Xeen CD data from GOG required), how to
   download, unpack and point it at the GOG folder, controls, where saves go,
   and an honest list of what is missing, taken from
   [project status](project-status.md) known gaps.
4. **Version and publication.** Version `0.1.0` (already the CMake project
   version), tag `v0.1.0` on `main`, GitHub release with the zip and release
   notes. Tagging and publishing need the maintainer's explicit authorization at
   that moment.

## Part C - launcher without editing (v0.1.1)

Approved by the maintainer on 2026-10-14 after people trying v0.1.0 struggled to
edit `MMModern.bat`. The launcher is packaging only: the game, its rules and
the save format do not change.

1. **First run.** `MMModern.bat` starts a small PowerShell launcher shipped in
   the zip (the PowerShell built into Windows, no extra install). It first
   looks for the GOG installation in the registry (`GOG.com\Games` entries,
   32- and 64-bit views) and accepts a folder only if it contains
   `WORLD\XEEN.DAT`. If none is found, it opens the standard Windows folder
   picker with a one-line explanation, validates the choice and explains and
   asks again on a wrong folder; cancelling exits cleanly.
2. **Later runs.** The chosen folder is stored per user under
   `%LOCALAPPDATA%\MMModern\` and reused; if it no longer contains
   `XEEN.DAT`, the first-run steps repeat. `Change game folder.bat` forgets it
   and asks again.
3. **Errors.** If the game exits with an error, the console stays open with
   the message, as today. Paths with spaces and accented characters work.
4. **Package and docs.** `tools/package-release.ps1` writes the launcher files
   with CRLF line endings; README and release notes stop asking players to edit
   anything. Version `0.1.1` (CMake project version, `THIRD-PARTY.txt` tag and
   zip name). If the version is compiled into the executable, run the complete
   `build-rel` suite through the runner; otherwise `ctest -L fast` suffices.
5. **Publication.** Tag `v0.1.1` and a GitHub release that becomes "Latest";
   v0.1.0 stays as published. Each step needs the maintainer's authorization.

## Save policy after release

`AGENTS.md` keeps one current save format "until a public release is declared".
Approved (maintainer, 2026-10-14): during `0.x`, keep one current format and
reject older saves with a clear message, stated in the README and release
notes; revisit compatibility at `1.0`. Part B updates `AGENTS.md` accordingly.

## Acceptance

- Part A: tests, complete `build-rel` suite, M44 audit gate and the maintainer's
  play-test of Gunther's reward, the equip message and the title bar.
- Part B: both complete suites pass; the maintainer downloads the published zip
  on a clean folder, starts a new game from the GOG data and plays briefly.
- Part C: the maintainer tests the new zip three ways (GOG found automatically,
  folder chosen by hand, wrong folder chosen), then the published v0.1.1 zip.
- Docs at closure: plan condensed, status, history and roadmap (step 10 done),
  README for players.
