# Milestone 64 - Vertigo closure and first public release

**Tier B, in three parts. Status: completed and accepted.** Plan `e7438a2`;
Part A `c080eb1`; Part B `080b05f` (release v0.1.0); Part C `5545471` (release
v0.1.1). The maintainer accepted the play-tests and the published downloads
(2026-10-14); other people also ran v0.1.1 successfully.

## Goal and final scope

Close [roadmap](../roadmap.md) step 10: fix what the maintainer's full Vertigo
play-through found, then publish the first public release for Windows. No new
content, mechanics or save-format change (v12, schema 9, content 14).

- **Part A - play-through fixes.** A grant sparkles each member at most once
  per Event run, as in DOS (the "already sparkled" latch lasts for the whole
  run), so Gunther's reward shows one pass and then "+ 5,000 experience", as in
  the maintainer's DOSBox capture. Equip refusals use the original base item
  names ("You must remove great axe to equip spear!"). Playing from the title
  menu shows only "MMModern" in the window title.
- **Part B - release v0.1.0.** A released binary no longer stops when the
  compiled-in repository path is missing on the player's machine. The packaging
  script builds the zip from the tested build: stripped executable, the UCRT64
  runtime DLLs, licence and third-party notices with the source tag. The README
  was rewritten for players. Tag `v0.1.0` and a GitHub release.
- **Part C - release v0.1.1.** People found editing `MMModern.bat` hard. A
  PowerShell launcher in the zip finds the game through GOG's registry entries
  or a folder picker, accepts only a folder with `WORLD\XEEN.DAT` and remembers
  it; `Change game folder.bat` asks again. The game itself is unchanged. Tag
  `v0.1.1`, now the latest release.

## Decisions

1. **Sparkle cause.** The plan blamed a game-flag grant; implementation found
   the per-Event latch in `WORLD/XEEN.DAT` and ScummVM and fixed that instead.
   DOS also sparkles for more grant types than ScummVM; no Vertigo or F3 script
   uses them (recorded in [original quirks](../original-quirks.md)).
2. **Save policy for 0.x** (maintainer): one current format; older saves show as
   "Unavailable"; compatibility is revisited at 1.0 (`AGENTS.md`).
3. **Player-facing names.** The README and release notes call the mainland
   area F3, its in-game name; map numbers stay in developer material.
4. **v0.1.1 instead of changing v0.1.0.** Published tags are never moved.

## Results

- Full CTest 192/192 on `build-rel` after Parts A and B, and 192/192 on
  `build-m44` before release (two Debug time limits raised); M44 audits found
  zero differences; Part C changed no game code.
- Both zips verified in a clean folder without MSYS2 on `PATH`, and again after
  download from GitHub (matching SHA-256).

## Deferred

A Windows installer and code signing. Release and development builds share one
save folder, which is acceptable for now.
