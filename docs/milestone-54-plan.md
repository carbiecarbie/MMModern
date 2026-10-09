# Milestone 54 plan - Original title menu and saves

**Tier A. Status: approved; not started (2026-10-09).** Implementation requires the maintainer's explicit start.

## Goal and baseline

Plain launch opens the CD World of Xeen title. New chooses a slot, name and difficulty; title Load and the in-game panel use
original DOS dialogs, by mouse and keyboard. Save/restore stays exact. Implementation requires the maintainer's explicit start.
Baseline: `main`, HEAD, origin/main and remote main = `60743843abf61b56d3e4c5ff499ded30049fa77b`; only this draft was untracked.
Reference `R/` means `engines/mm/xeen/` in `D:/Projetos/MModern/scummvm-known-good-candidate`, pinned at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Direction: [roadmap](roadmap.md), [status](project-status.md),
[M53 CD source](milestone-53-plan.md), [M52 initialization](milestone-52-plan.md), [dependencies](dependencies.md).
Original data at `F:/Games/gog/Might and Magic 4-5` remains read-only; never extract archives or commit commercial bytes/text.

## CD resources and presentation

- Reuse M53's `GameInstallation`/read-only source and `XeenAssetSource`/`ScummVmXeenBridge`; no new archive owner, copied data,
  floppy fallback or ScummVM engine instance. GOG images and a CD GAME directory with explicit UI data remain supported.
- Title art comes from CD DARK.CC: `world0.int`, `world1.int`, `world2.int`, CD `world.raw`, `dark.pal`, `start.icn` and
  `special.icn`. Discard the draft's root-floppy `world.int` mapping. Validate required resources/frames through the CD source.
- Credits come from CD DARK.CC `credits.bin`, decoded with XOR `0x35`, retaining four ordered pages and original controls;
  use the original background/palette (reference `marb.raw`), validating the CD presentation. Do not substitute ScummVM credits.
- Title and credits resources are read with an explicit DARK.CC archive selection through `XeenAssetSource`/the bridge (today
  its palette/RAW/byte readers use XEEN.CC, `ScummVmXeenBridge.cpp:432,450`, and the scene lookup can fall back to INTRO.CC,
  `:272`). Verify DARK.CC provenance; a missing title or credits member is an error, never satisfied by another archive.
- `WORLD/XEEN.DAT` supplies title/menu text and resource selectors, difficulty text, DOS panel script coordinates/hotkeys,
  chooser rows and headings, file field, empty-row text, overwrite/current-game/name/quit prompts, combat refusal,
  no-saves notice, saved notice, free-space errors and DOS filename patterns. These are resource roles, not copied strings.
- Reuse `XeenDosText::scalar/table` and its bounded layout/control validation. M53 already exposes `WORLD_MENU`,
  `WORLD_MENU_TITLE`, `CREDITS_RESOURCE`, `MENU_SPRITE`, `LOAD_LABEL`, `SAVE_LABEL` and `OVERWRITE_CONFIRM`;
  add verified fields for difficulty and remaining panel/chooser material in that reader, not a second parser or literals.
- Reuse original font/window rendering and CD sprites (`cpanel.icn`, `choice.icn`, chooser/confirmation art). Derive text
  positions/hotkeys from DAT scripts and verify sprite bounds/hit areas against DOS; ScummVM's rearranged panel is not authority.
  `R/dialogs/dialogs_control_panel.cpp` omits Save As; `R/saves.cpp` uses a replacement GUI chooser. Copy neither UI.
- Stop before implementing an affected screen if CD members, page boundaries, palette/frame mapping or required DAT fields
  are missing, malformed or unverified. Report the gap; never substitute floppy data, hardcoded commercial text or guessed UI.

## Confirmed flows and remaining evidence

Maintainer DOSBox CD observations of 2026-10-08/09 override ScummVM; the installed manual (printed p.31/PDF p.35)
describes the panel and Save As but not Text.

- Title Escape toggles animated background alone versus background with the main menu; it never quits. New, Load, Credits
  and Other Options use original buttons/hotkeys. Intro/logo movies and all audio are deferred; M54 starts at the title.
- **Approved (maintainer):** title animation cadence (four 50-ms ticks, `R/worldofxeen/worldofxeen_menu.cpp`, `R/events.h`)
  and fade transitions (`R/screen.cpp`) follow the pinned reference unless the final DOSBox comparison shows a visible
  difference. Preserve animation phase and input responsiveness.
- Credits advance on click/any key, with Escape returning immediately to the menu; after page four return to the menu,
  never automatically advance. Other Options shows the two intro buttons; deferred movies report "not supported yet".
  Endings remain locked; no new unlock state or cinematic implementation.
- Ten slots, numbered keys 1-9 then 0: green selection, empty rows, file field, Select/Exit and arrows. Copying DOS slot pairs
  to index 11 exposed no extra row. A slot is selected only by its number key or by clicking its row; Select confirms the
  selection, like the keyboard confirmation. The arrows show their pressed frame but do nothing. Clicking the bar between
  them does nothing (maintainer DOS clarification, 2026-10-09); it has no pressed feedback.
  Title and panel Load share this chooser and load without confirmation; Escape in title Load returns to the main menu. DOS paired filenames
  are display-only patterns from DAT; MMModern never reads/writes native saves. Derive difficulty/highest party level from payload.
- New: slot -> occupied-slot overwrite confirmation with thumbs -> name -> difficulty. Before selecting a slot, Escape returns
  to the main menu; from any later New screen, Escape returns to slot selection. Selection/name remain provisional until
  difficulty is chosen. No cancellation writes a file; only successful publication makes the slot occupied/current.
- Names: the DOS entry accepted every printable key and preserved case (observed). Store names as single-byte codes in the
  printable ASCII range 0x20-0x7E, exactly as entered, at most 20 bytes (one byte per character); reject other codes and any
  code the original font cannot display, at entry and at save decode. Backspace deletes, Enter confirms only nonempty input,
  Escape cancels. Test the round trip of every accepted code. Never use a display name as a filesystem path.
- Save As: occupied-slot confirmation -> name -> write -> success notice; dismissing success returns to the list. Escape during
  name entry or the overwrite confirmation returns to slot selection; Escape on slot selection returns to the control panel.
  Direct Save targets the current slot: the one chosen by New, the last Load or the last Save As. Observed: after New in slot 1
  and Save As into slot 3, a direct Save updated slot 3 only (DOS also rewrites its current-game files).
- Quit confirms with the original screen, then exits the application; cancellation retains the session. Panel Escape dismisses.
- Tab and clicking the gem both open the panel (observed). DAT drives the full panel including Text/Speech. Text remains unknown after the manual check: show it and report
  "not supported yet", as authorized. Audio controls likewise refuse visibly without inventing toggles.
- Mr Wizard first shows the DOS confirmation: decline/cancel returns; confirmation then reports "not supported yet".
  Neither branch changes gems, camera, time, RNG or any other gameplay state. Rescue itself stays deferred.
- No Darkside gameplay, Tavern autosave, intros/logo movies, audio, rescue or unrelated content in M54.

## Storage and save format

1. Resolve OS Local AppData and explicitly create `MMModern/Saves/<archive-key>/` using the existing logical archive fingerprints
   as a deterministic key. Fail visibly on resolution/creation errors; no working-directory or installation fallback.
   Ten fixed internal slot IDs map to ten `.mmsave` files. No catalog, generations, thumbnails or timestamps.
2. Each atomic file contains its validated name and snapshot. Save v7 adds bounded name metadata covered by the envelope CRC;
   retain schema 9/content 14. Managed names must be nonempty and <=20 characters; developer loose saves may use an explicit
   absent-name encoding, never an invented commercial name. Preserve metadata across exact reload/re-save.
   One current reader: clearly reject v6/older, protect newer/unknown formats; no migration or legacy reader.
3. Reuse `XeenSaveFile::write` temporary-file, flush and atomic-replace path; failure leaves the previous slot intact.
   Validate CRC, resource signature and name before offering Load or overwrite. Reject corrupt, foreign-data, unknown and newer
   targets without mutation; recognizable older MMModern files retain the existing explicit replacement policy.
   Serialize MMModern's own managed writes and re-validate the target immediately before the atomic replace. Guarantee scope:
   a foreign, corrupt or newer target present when Save starts, or swapped in before that final re-validation, is refused; an
   external replacement in the instant between re-validation and the OS rename is not defended (the slots live in an
   application-owned directory). Tests swap a foreign target in before the final re-validation and expect refusal.
4. Resolve and validate directories before creation/writes; protect every M53 source directory and repository, including aliases,
   links/reparse points and traversal. Slot IDs, not names or DOS file labels, determine paths. Extend the existing path owner.
   Map applicable storage errors to DAT messages; retain English technical diagnostics for unsupported OS failure details.
5. **Approved deviation (maintainer, 2026-10-09):** a protected slot occupies its original row with a short MMModern-owned
   unavailable marker; selection explains the reason, and Load/overwrite are disabled. Never disguise it as empty, delete it,
   expose unvalidated names or add rows. There is no DOS evidence for these MMModern-specific files or their row presentation.
   The single-file v7 storage is the requested format deviation; DOS-style labels remain presentation only.

## Session lifetime and publication

- `Application` owns an outer title/New/Load/gameplay loop. Keep existing party/roster, world, Event, Service and Flow owners;
  add entry/outcome values to coordinate their lifetime, not a parallel gameplay coordinator or in-place owner swapping.
- Ordinary Save and in-game Load begin at an idle, presented Journey exploration boundary, using current frame/ticket and
  `XeenRestoreGuard` checks. Exclude combat/projectiles, Rest, rewards, casting, inventory, service, Event, transition, pending
  arrival/dispatch/save work. The panel/chooser holds that boundary without time/RNG. Combat uses the original refusals;
  honor the map save restriction for Save, not Load. Stale requests refuse before providers or file I/O.
- In-game Load reads once into an immutable candidate; decode/name/CRC/fingerprint checks and `restoreBeforeGameplay` on scratch
  owners, including existing first-frame preflight, all succeed before ending gameplay. Guard the retained session across
  every provider/handoff. Cancellation or validation failure leaves its owners and current slot intact; integrity failure
  remains fatal. Then return a Load outcome carrying the validated snapshot/slot to the outer Application loop.
- Destroy the old Flow before its borrowed owners; invalidate input, frame/ticket generations and callbacks. Start a fresh
  session from the retained validated snapshot, never reread the path or run New initialization. Title Load uses the same path.
  The fresh session restores exactly without Event/reward replay, time advance or RNG draws. Preflight failures stay before
  teardown; an unexpected post-teardown runtime failure is reported, never described as a recoverable chooser failure.
- New needs a separate initialization-only publication boundary: current `xeenSaveGameplay` requires a presented idle frame.
  After difficulty, finish M52 initialization once (original roster/party/camera/actors/economy, stock once, prepared=false),
  before first gameplay presentation/input or arrival dispatch. Capture under owner guards, validate by restoring to scratch
  owners and composing off-screen, then atomically write the named slot. Do not fake a presented frame or weaken normal Save.
  Only write success sets the current slot and exposes gameplay, using that exact initialized state without a second seed,
  stock generation or initialization. Failure retains the old slot and stays in New with visible feedback; retries use the
  retained candidate, cancellation discards it. Test equality of the initial save and the state first presented.

## Developer entry and implementation sequence

- Bare `<game-dir>`/`Application::run` now opens title. Keep explicit `--new-game` (default Adventurer), `--difficulty`,
  `--journey-region`/seed, `--load-game`, `--render-map`, `--inspect-*` and M53 `--ui-data`. Difficulty/save-file overrides
  require an explicit developer mode. Loose `--save-file`/F9 remains a developer path, with no implicit managed-slot import;
  with no current managed slot, panel Save routes to slot selection (approved developer-only policy). Render-map stays unsaveable.
- Replace unconditional SDL Escape exit in title/modal contexts with the confirmed routes. Translate Portuguese diagnostics
  in `Application.cpp`/`main.cpp` where touched. Keep 320x200 composition, scaled hit testing and fresh-press modal input.
- After independent plan review and explicit start: storage/v7 and initialization/session boundaries; shared chooser/title/
  Credits/Other Options/New/title Load; then panel Save/Save As/Load/Quit and CLI test migration. Neither part alone closes M54.
- Retarget gameplay tests using bare paths or `Application::run`, specifically `XeenSaveCliTests`,
  `XeenFreshStartProcessControls` and `GraphicsSmokeTest`, to explicit entry. Keep separate bare-launch title tests and CLI
  rejection tests; audit other launch sites so witnesses do not silently stop at title or change their gameplay inputs.

## Validation, digest protocol and acceptance

- Cover mouse/keyboard parity, hit edges/pressed frames, title toggle/animation, four credits pages, modal input flushing,
  ten slots, validation and cancel routes, overwrite-before-name, success-to-list, current-slot targeting and deferred actions.
  Test New's pre-presentation write, no premature writes, write failures, protected targets, directory creation, path aliases,
  concurrent replacement and writer fault points. Use synthetic fixtures plus a few original-data UI paths, not one witness/button.
- Exercise canceled/failed/successful in-game Load, scratch validation and stale providers/callbacks/frames, destruction order,
  exact immediate re-save and continuation with unchanged RNG/time/rewards. Reuse save/restore/file tests and their guards.
- M44 protocol for v7: reproduce all three current CD baseline digests with the baseline executable and unchanged inputs/seeds;
  compare complete old/new traces and decoded snapshots. Isolate and explain only format bytes: version, bounded name metadata
  (absent for existing loose witnesses), payload length and CRC. All common gameplay fields and traces must match exactly.
  Show per-byte mapping, first divergence if any, and v7 reload/re-save equality; obtain maintainer approval before regenerating
  `tests/XeenM44BaselineDigests.h`. Any unexplained gameplay/RNG/time difference stops work; never bless it as format churn.
- During implementation run affected tests plus build-rel fast; final production builds require the complete suite via
  [tools/AGENTS.md](../tools/AGENTS.md). Closure uses build-m44 fast plus the three M44 scenarios under that protocol.
  This planning-only task checks diff/links and `git diff --check`; no builds or CTest.
- Gates: missing or unverified CD members and DAT fields. Stop affected work on contradictions with the DOS evidence, unsafe
  publication or scope growth.
- Acceptance: independent plan and implementation reviews, passing tests, then maintainer CD DOSBox comparison of title,
  New/both difficulties/cancel, Credits/Other Options, panel Save/Save As/Load/Quit and title Load by mouse and keyboard.
  Only acceptance permits completed status and closure updates to README/status/roadmap/history, retiring M52's entry deviation
  without rewriting its historical result. No implementation, commit or push is authorized by this proposed plan.
