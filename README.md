# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Status

**Milestone 28 is the latest completed milestone.**

The engine supports a bounded Clouds quest loop: request a quest, collect an
item, return it for character-held rewards, and save/resume the resulting progress.
Active-party inventory inspection shows resource-driven item descriptions and
supports transfer between active roster owners and contextual equip/remove for
bounded Clouds weapons, armor and accessories, preserving frames on restart.
Original maps, text, portraits and supported objects appear through a standalone
SDL application, with ordinary outdoor objects animating while stationary and
during dialogue. Static ordinary indoor objects use original directional
appearances, placement and wall occlusion; the bounded Nightshadow gravestone
interaction displays its original clue through the existing event flow.

The Diagnostic26 entry presents an original outdoor Skeleton, supports its
activation and approach, and stops at terminal same-cell engagement. Diagnostic27
continues that bounded encounter through playable Attack/Block combat with
original MON/ATT appearance, injury, armor breakage, victory/defeat and once-only
XP. A successfully ended victory becomes a saveable completed checkpoint that
can resume in a new process, expose read-only inspection and perform a bounded
true revisit with the Skeleton still defeated. Neither diagnostic enables
normal-start gameplay.

MMModern remains incomplete and experimental. It is not yet a generally playable
replacement for the original games: general combat, item use, complete item effects and Darkside
gameplay remain unsupported, and travel between validated checkpoints is not certified.

See the [technical snapshot](docs/project-status.md),
[completed milestones](docs/project-history.md) and [future direction](docs/roadmap.md).

## Goals

- Reimplement the Xeen engine using legally obtained original resources.
- Preserve original behavior while keeping unsupported boundaries explicit.
- Build maintainable, testable gameplay and rendering with clear state ownership.

## Current capabilities

- Original Clouds resource loading, outdoor/indoor rendering, navigation and collision.
- Resource-derived outdoor monster state, normal sprite and delayed approach at
  one bounded Skeleton checkpoint, plus playable Attack/Block combat with original
  attack sprites and outcomes in Diagnostic27.
- Supported static and ordinary animated outdoor objects and static ordinary
  indoor objects, with persistent removal after interactions. Indoor ordinary
  animation remains unsupported.
- Bounded event execution, teleports, original text, choices, character selection
  and animated NPC dialogue portraits.
- Party/character state, quest items and flags, and deterministic item rewards.
- Active-character condition and current/max HP/SP, four-category inventory
  inspection and character-to-character transfer, with nine slots per category.
- Contextual equip/remove for bounded Clouds weapons, armor and accessories,
  including class, conflict, capacity and curse feedback for modeled rules.
- Local Windows save/resume for ordinary progress and completed Diagnostic27,
  including transferred ownership, exact combat outcomes and live diagnostics.

## Running and controls

Build/dependency setup is documented in [dependencies.md](docs/dependencies.md).
Run from a terminal to see diagnostics; quote paths containing spaces:

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --encounter-26 <game-directory>
mmodern --encounter-27 [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
```

Use an existing save directory outside the original game installation. Relative
paths resolve against the working directory. Resume uses the loaded file as the
subsequent save target; invalid/incompatible saves fail without starting a new game.

`--encounter-26` opens the bounded World of Xeen Clouds map-20 diagnostic at
`(13,1)` North. Period (`.`) is Wait; the initial Skeleton may be almost completely
hidden by original forest occlusion, becoming identifiable at engagement. The
entire session is unsaveable: F9 is refused, as are ordinary events and inventory.
Engagement or a support stop terminates supported exploration before combat;
Escape/window close exits. This mode accepts no save options or camera overrides.
Ordinary `--render-map` behavior remains unchanged.

`--encounter-27` adds the bounded Attack/Block diagnostic. Preparation starts
without live actors: I opens inventory for normal transfer/equipment operations,
and Enter begins only with inventory closed. In transfer selection/confirmation,
I cancels to Browse; another I closes. N also cancels confirmation. Escape always
exits the session. After approach engagement, Space attacks and B blocks for the
displayed character; enemy and round work continues automatically. Preparation,
approach, combat, incomplete victory, defeat and failure remain unsaveable. After
a successful End, the completed checkpoint permits F9 saving to the configured
target, read-only I inspection and bounded R re-entry at `(13,1)` North. Resume
that checkpoint with `--load-game`; it cannot resume combat or exploration, and R
does not mean Run or general navigation. The optional nonzero 32-bit seed
reproduces diagnostic RNG. Camera overrides and other entry modes cannot be
combined with this entry.
Combat uses the original normal and attack sprites with bounded source-derived
sequences, while the live roster panel retains injuries and terminal XP results.

| Key | Action |
| --- | --- |
| W/Up, S/Down | Move forward/backward; browse physical slots in inventory |
| A/Left, D/Right | Turn left/right; browse categories in inventory |
| Space, Enter | Interact (Space) or advance/acknowledge text; Enter confirms an armed transfer |
| Y / N | Answer Yes/No; N cancels a transfer confirmation |
| F1-F6 | Select inventory owner or transfer recipient; outside inventory, select an eligible member during WhoWill |
| 1-9 | Select a physical inventory slot while browsing |
| T | Begin transfer of the selected occupied slot |
| E | Equip or remove the explicitly selected occupied weapon, armor or accessory |
| . | Wait in the diagnostic encounter; no action elsewhere |
| F9 | Save an eligible idle ordinary session or completed Diagnostic27; refused before completion or while inspection/inventory is open |
| I | Open inventory while idle; in completed Diagnostic27 open read-only inspection; close while browsing and print live diagnostics on opening |
| R | Revisit the completed Diagnostic27 checkpoint through its bounded true re-entry |
| Escape | Exit either diagnostic session; otherwise back/cancel transfer or close inventory, cancel WhoWill, acknowledge NPC/reward pages, or exit |

Movement and ordinary interaction are blocked while a response is required;
repeated keydown events are ignored. NPC dialogue and reward pages accept
Space/Enter/Escape, including final acknowledgment with Escape.
Inventory navigation applies while browsing; during transfer selection/confirmation,
Escape returns to browsing before changing category or slot. Each equipment
attempt consumes its selection; select the slot again before another E action.
Misc item use and general item effects are not provided by this panel.

F9 refuses during an interaction, while inventory or completed inspection is
open, or before Diagnostic27 reaches its completed boundary, without advancing
work or scheduling a later save. Close the blocking UI or finish the interaction,
then issue a new F9. Without a configured path, it writes nothing. Save results
appear in the console and window title. Existing supported valid MMModern saves
can be replaced; there is no autosave, save-on-exit or in-session load. Ordinary
eligible saves write v2, completed Diagnostic27 writes v3, and the reader accepts
supported v1/v2/v3 under their respective compatibility policies. Saves require
matching game archives and are not compatible with original Xeen or ScummVM
saves. See the
[persistence model](docs/project-status.md#persistence-model) for details.

## Requirements and original game data

A legally obtained original installation is required; no commercial game data is
included or modified. Current original-data validation uses World of Xeen resources
for Clouds gameplay. Reading Clouds visual metadata from `DARK.CC` does not enable
Darkside gameplay.

The validated development setup is Windows x86-64 with MSYS2 UCRT64, CMake, SDL2,
zlib and separate ScummVM source/build trees. MMModern reuses selected ScummVM
Xeen and support components; it does not instantiate ScummVM's engine or use its
SDL backend. The exact pin and configuration live in
[dependencies.md](docs/dependencies.md).

## Documentation

- [Project status](docs/project-status.md): current stable technical capabilities,
  ownership, persistence and boundaries.
- [Project history](docs/project-history.md): concise completed milestones and plan links.
- [Roadmap](docs/roadmap.md): future direction and planning review cadence.
- [Milestone 28 plan](docs/milestone-28-plan.md): closed completed-encounter
  authority, persistence, restoration, revisit and acceptance contract.
- [Milestone 27 plan](docs/milestone-27-plan.md): closed bounded combat rules,
  ownership, original appearance and acceptance results.
- [Milestone 26 plan](docs/milestone-26-plan.md): closed actor/approach, timing,
  engagement and unsaveable-session contracts, with acceptance results.
- [Milestone 25 plan](docs/milestone-25-plan.md): closed bounded equipment
  contracts, architectural decisions and acceptance results.
- [Milestone 24 plan](docs/milestone-24-plan.md): closed item catalog, inspection
  and transfer contracts.
- [Dependencies](docs/dependencies.md): supported toolchain and ScummVM setup.
- [Agent instructions](AGENTS.md): development, documentation and Git rules.

## Disclaimer and license

MMModern is unofficial and is not affiliated with or endorsed by Ubisoft,
New World Computing or the ScummVM project. Might and Magic names and assets
belong to their respective copyright holders.

MMModern is distributed under the GNU General Public License version 3 or, at
your option, any later version (GPL-3.0-or-later). Reused ScummVM code is copyright
its respective contributors and licensed under GPLv3 or later.
