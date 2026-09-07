# MMModern - Project Status

## Current development state

Current stable milestone: **Milestone 14**

Current development target: **Milestone 15B - pending implementation authorization**

Milestone 14 is complete. Stages 14A, 14B, 14C, and 14D are complete.

Milestone 15 is approved and in progress. 15A is complete.
15B and 15C have not started; no 15B implementation is authorized by completing
15A. The approved specification is
[Milestone 15 plan](milestone-15-plan.md).

Current automated test suite: **33/33 passing**

## Milestone 13

Milestone 13 established the first functional Xeen event execution foundation.

Current implemented capabilities include:

- Loading original Xeen game resources
- Outdoor map rendering
- Indoor map rendering
- Player navigation
- Collision handling
- Loading real party data
- HP and SP state
- Xeen event decoding
- Xeen event script execution
- Automatic map events
- Event conditions
- Event calls and returns
- Teleport events
- Game flags
- Automated tests
- Manual runtime validation

At the completion of Milestone 13:

- Full automated test suite: **27/27 passing**
- Manual validation: **passing**
- First public Git repository created
- Milestone 13 is the initial public MMModern codebase

## Milestone 14 - Manual world interaction and text events

The main goal of Milestone 14 is to allow the player to manually
interact with the Xeen world.

The intended gameplay flow is:

Player faces an object or direction
-> presses Space
-> MMModern resolves the event using the current position and direction
-> the appropriate event script executes
-> text or other basic interaction feedback is displayed

Unlike automatic events, manual interaction should not require the
automatic-event bit (`0x10`).

### Planned investigation / stages

#### 14A - Xeen event text resources

**Status: complete.**

Investigate and implement loading of the Xeen `aazeXXXX.txt` resources.

Provide a reliable mapping between event text indices and the original
game strings.

Implemented behavior includes:

- read-only access to event text resources in the outer `xeen.cc` archive;
- NUL-delimited parsing with raw bytes and empty entries preserved;
- zero-based lookup;
- `aazeXXXX.txt` / `aazexXXX.txt` resource naming;
- distinct missing-resource, empty-resource, and invalid-index states;
- synthetic tests and validation against real map 1 data.

#### 14B - Manual interaction input

**Status: complete.**

Add manual interaction, initially through the Space key.

Resolve event execution using:

- current map
- current player position
- current facing direction

Manual interaction must use the appropriate Xeen event rules rather than
requiring the automatic-event flag.

Implemented behavior includes:

- Space dispatches an interaction action distinct from navigation;
- repeated keydown events are ignored;
- lookup starts at line 0 on the current cell and uses the current facing;
- original first-match ordering and the all-directions entry are preserved;
- manual lookup is independent of the automatic-event bit (`0x10`), while
  automatic events remain gated by it;
- no-event, completion, execution-error, and unsupported special-interaction
  outcomes are distinct;
- supported scripts retain the existing transactional camera/flag behavior;
- applicable Clouds grate/door handling is recognized before ordinary event
  lookup and reported as unsupported without changing the world.

Validation completed for 14B:

- full automated test suite: **30/30 passing**;
- synthetic SDL validation covers Space dispatch, repeat suppression, release
  and re-press, and continued navigation input;
- real-data validation at map 1 `(8,8)`, facing West, reaches the expected
  `Display0x01` instruction at offset 461 without changing camera or flags;
- headless runtime validation confirms that the unsupported text opcode is
  reported and the application continues accepting interaction and navigation.

Text opcode semantics are implemented by 14C; graphical presentation remains 14D.

#### 14C - Text/display event opcodes

**Status: complete.**

Investigate and implement the initial text-oriented Xeen event opcodes,
including relevant examples such as:

- `0x01 Display0x01`
- `0x02 DoorTextSml`
- `0x03 DoorTextLrg`
- `0x04 SignText`
- `0x29 DisplayBottom`
- `0x31 DisplayBottomTwoLines`
- `0x35 DisplayMain`

Implemented behavior includes:

- strict decoding for all seven planned display opcodes, including the
  two-byte `DisplayBottomTwoLines` layout;
- distinct centered-message, reduced/normal scene-label, sign-label,
  bottom-window, two-line bottom-window, and main-window semantics;
- zero-based event-text resolution against the currently executing map while
  preserving raw string bytes and valid empty strings;
- distinct missing-resource and invalid-index diagnostics;
- resumable execution state retaining the logical address, current script,
  working camera and flags, call stack, instruction count, transfer state, and
  pending presentation request;
- response validation for presentation completion, acknowledgment, and Yes/No;
- `DisplayBottomTwoLines` acknowledgment followed by immediate event
  termination without executing the next script line;
- restricted condition Action 44 support: value 0 requests Yes/No with original
  comparison values Yes=`0` and No=`2`; value 1 requests acknowledgment and
  compares as `1`;
- transactional camera and flag commit/rollback across any number of
  presentation suspensions.

Validation completed for 14C:

- full automated test suite: **31/31 passing**;
- focused synthetic coverage for all seven opcodes, malformed operands, text
  errors and empty text, semantic presentation kinds, repeated suspension,
  instruction limits, calls/returns, cross-map text, rollback, and Action 44;
- all existing interpreter, event-system, manual-event, navigation, and SDL
  input regressions remain passing;
- real Castle Basenji map 1 `(8,8)`, facing West, resolves text index 19 from
  `Display0x01` at offset 461, then requests the Action 44 Yes/No response;
- the real-data No path completes without teleport, while Yes follows the
  existing supported teleport implementation.

14C remains headless at the presentation boundary. It does not parse fonts,
interpret text formatting bytes, paginate by pixel metrics, or render windows.

The `NPC` opcode requires a later or separate stage if it introduces
dialogue, portraits, confirmation, branching, or other larger systems.

#### 14D - Gameplay UI integration and validation

**Status: complete.**

Implemented behavior includes:

- independent decoding of the original English `fnt` resource, including its
  normal and reduced 8x8 glyph banks and per-glyph advance tables;
- indexed-color glyph rasterization with clipping, metric wrapping, centered
  alignment, supported formatting controls, explicit diagnostics for
  unsupported controls, and page splitting for the supported windows;
- distinct scene-label placement for reduced door text, normal door text, and
  signs, plus the original main, bottom, two-line bottom, and centered-message
  window regions;
- presentation composition directly into the existing 320x200 `IndexedFrame`;
- a runtime presentation controller that uses the 14C response protocol and
  keeps SDL and drawing concerns out of the event interpreter;
- presentation handling in the existing SDL loop without nested event loops;
- blocked gameplay input while an acknowledgment or choice is pending, with
  navigation restored afterward;
- Space or Enter for acknowledgment and Y/N for the standalone Yes/No mapping;
- removal of scene labels on the next gameplay action and recomposition after
  event-driven camera changes.

Validation completed for 14D:

- full automated test suite: **32/32 passing**;
- synthetic coverage for deterministic font parsing, normal/reduced glyphs,
  glyph metrics, clipping, formatting diagnostics, metric wrapping, semantic
  composition, pagination, response mapping, and gameplay blocking/resumption;
- SDL input coverage confirms repeat suppression and the Space, Enter, Y, and
  N mappings while Escape retains application-exit behavior;
- the real map 1 `(1,14)` West SignText renders the two-line `Air` / `Corner`
  label over the scene;
- the real map 31 `(5,1)` West DoorTextSml renders `Snake Oil` with the reduced
  font over the doorway;
- Castle Basenji at map 1 `(8,8)` West visibly renders text index 19 and its
  Yes/No choice; No completes without teleport and Yes recomposes at the
  existing teleport destination;
- headless SDL runtime checks exercise blocked navigation, both Yes and No
  response paths, resumed navigation, and clean Escape shutdown;
- captured indexed frames for all selected real cases were inspected at native
  320x200 resolution, including the distinct reduced font and the post-teleport
  frame.

Milestone 14 is complete. Milestone 15 is the approved next milestone;
15A is complete; 15B awaits implementation authorization.

## Out of scope for Milestone 14

Unless required by investigation, Milestone 14 is not intended to
implement complete:

- NPC interaction
- shops
- inventory
- combat
- Swimming / Walk on Water capabilities

Swimming / Walk on Water remains navigation/capability work and should
not redefine the primary goal of Milestone 14.

## Milestone 15 - Mutable session-world state and Remove

**Status: in progress; 15A complete, 15B and 15C not started.**

The [dedicated plan](milestone-15-plan.md) is the specification for this milestone.
It defines the minimum session-owned world mutations needed by the original
`Remove` operation, independently of disposable map and script caches.

Approved stages:

- **15A - Session ownership and side-aware map identity:** complete.
- **15B - Stable object/event identities and Remove execution:** planned;
  not implemented or complete.
- **15C - Same-session persistence and integration validation:** planned;
  not implemented or complete.

The planned real-data checkpoint is Clouds map 23 at `(8,2)`, starting at the
original `Remove` boundary. Confirmation of `.mob` object record 13 / resource
111 is required during 15B. This does not include quest-item grants, visible
entity rendering, disk save/load, or Darkside gameplay.

Implemented and validated in 15A:

- `XeenMapIdentity` carries side + numeric map ID through camera, world/cell
  queries, scene command provenance, event/text providers and caches, logical
  addresses, call stacks, presentation requests, and diagnostics;
- numeric resource neighbor/teleport operands are resolved in the current side;
- `XeenWorld` owns a separate, deliberately empty session-state boundary and
  cannot be copied; no artificial mutation has been added;
- explicit map, script, and text cache discard forces subsequent loading while
  preserving session ownership and suspended execution values;
- map/script/text identity mismatches are rejected, and real Clouds resource
  adapters reject Darkside requests; both sides are exercised only synthetically;
- existing physical/logical location separation and camera/flag transactions
  are preserved.

15A validation completed on 2026-09-07:

- Debug build in `build/15a`, MSYS Makefiles, MSYS2 UCRT64 GCC 16.2.0;
- source dependency `../scummvm-known-good-candidate`, verified clean at
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd`;
- dependency artifacts from `../build-scummvm-6814ee9b-ucrt64`;
- focused world/navigation/event/presentation suite: 16/16 passed;
- final full CTest suite: 33/33 passed, including the new session identity test;
- all nine existing real-data smoke executables passed: party, indoor map,
  event script, event text, game flags, interpreter, event system, manual event,
  and navigation flow;
- SDL dummy/software runtime passed for ui, map, indoor, event, manual,
  manual-no, and manual-yes with Escape, plus ui with SDL quit;
- generated 320x200 frames for Air / Corner, Snake Oil, Castle Basenji question,
  No, and Yes destination were visually inspected;
- validation logs and frames are local ignored outputs under `build/15a`.

No Remove decoding/execution, effective mutable events, object selection or
new object-loading path, quest-item grant, disk persistence, or Darkside gameplay
was implemented. The full M15 remains incomplete.

## Architecture notes

MMModern is a standalone application and is not a fork of the complete
ScummVM application.

The current implementation uses selected ScummVM Xeen components and
ScummVM libraries through the compatibility layer under:

`src/compat/scummvm/`

The local development setup currently expects ScummVM source and build
trees outside the MMModern repository.

See `docs/dependencies.md` for additional dependency information.

## Game data

Original Might and Magic / World of Xeen game data is not part of this
repository.

Development and runtime testing require legally obtained original game data.

## Updating this document

Update this file whenever:

- a milestone begins or ends;
- a major subsystem becomes functional;
- test status changes significantly;
- architecture or major dependencies change.

This document should describe the current repository state rather than
outdated plans or assumptions.
