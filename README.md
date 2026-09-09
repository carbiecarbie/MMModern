# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Status

**Milestone 21 is the latest stable completed milestone.**

The engine supports a bounded Clouds quest loop: request a quest, collect an
item, return it for character-held rewards, and save/resume the resulting progress.
Original maps, text, portraits and supported objects appear through a standalone
SDL application.

MMModern remains incomplete and experimental. It is not yet a generally playable
replacement for the original games: combat, general inventory use and Darkside
gameplay remain unsupported, and travel between validated checkpoints is not certified.

See the [technical snapshot](docs/project-status.md),
[completed milestones](docs/project-history.md) and [future direction](docs/roadmap.md).

## Goals

- Reimplement the Xeen engine using legally obtained original resources.
- Preserve original behavior while keeping unsupported boundaries explicit.
- Build maintainable, testable gameplay and rendering with clear state ownership.

## Current capabilities

- Original Clouds resource loading, outdoor/indoor rendering, navigation and collision.
- Supported static outdoor objects and persistent removal after interactions.
- Bounded event execution, teleports, original text, choices, character selection
  and animated NPC dialogue portraits.
- Party/character state, quest items and flags, and deterministic item rewards.
- Local Windows save/resume and read-only live inventory diagnostics.

## Running and controls

Build/dependency setup is documented in [dependencies.md](docs/dependencies.md).
Run from a terminal to see diagnostics; quote paths containing spaces:

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
```

Use an existing save directory outside the original game installation. Relative
paths resolve against the working directory. Resume uses the loaded file as the
subsequent save target; invalid/incompatible saves fail without starting a new game.

| Key | Action |
| --- | --- |
| W/Up, S/Down | Move forward/backward |
| A/Left, D/Right | Turn left/right |
| Space, Enter | Interact (Space) or advance/acknowledge text |
| Y / N | Answer Yes/No |
| F1-F6 | Select an eligible active member during WhoWill |
| F9 | Save an idle session to its configured target |
| I | Print live inventory, owners and quest diagnostics while idle |
| Escape | Cancel WhoWill; advance/acknowledge NPC dialogue or reward pages; otherwise exit |

Movement and ordinary interaction are blocked while a response is required;
repeated keydown events are ignored. NPC dialogue and reward pages accept
Space/Enter/Escape, including final acknowledgment with Escape.

F9 refuses during an interaction without advancing it or scheduling a later save.
Press F9 again after completion. Without a configured path, it writes nothing.
Save results appear in the console and window title. Existing supported valid
MMModern saves can be replaced; there is no autosave, save-on-exit or in-session load.
Current saves write v2 and read v1/v2, require matching game archives, and are
not compatible with original Xeen or ScummVM saves. See the
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
- [Milestone 21 plan](docs/milestone-21-plan.md): closed exchange specification,
  architectural decisions and acceptance result.
- [Dependencies](docs/dependencies.md): supported toolchain and ScummVM setup.
- [Agent instructions](AGENTS.md): development, documentation and Git rules.

## Disclaimer and license

MMModern is unofficial and is not affiliated with or endorsed by Ubisoft,
New World Computing or the ScummVM project. Might and Magic names and assets
belong to their respective copyright holders.

MMModern is distributed under the GNU General Public License version 3 or, at
your option, any later version (GPL-3.0-or-later). Reused ScummVM code is copyright
its respective contributors and licensed under GPLv3 or later.
