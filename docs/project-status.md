# MMModern - Project Status

## Current development state

Current stable milestone: **Milestone 13**

Current development target: **Milestone 14C**

Milestone 14 is in progress. Stages 14A and 14B are complete.

Current automated test suite: **30/30 passing**

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

Text opcode support and graphical presentation remain stages 14C and 14D.

#### 14C - Text/display event opcodes

Investigate and implement the initial text-oriented Xeen event opcodes,
including relevant examples such as:

- `0x01 Display0x01`
- `0x02 DoorTextSml`
- `0x03 DoorTextLrg`
- `0x04 SignText`
- `0x29 DisplayBottom`
- `0x31 DisplayBottomTwoLines`
- `0x35 DisplayMain`

Exact scope should be determined from the original Xeen behavior and
the existing ScummVM implementation before coding.

The `NPC` opcode may require a later or separate stage if it introduces
dialogue, portraits, confirmation, branching, or other larger systems.

#### 14D - Gameplay UI integration and validation

Integrate text/message presentation into the current standalone runtime.

Validate manual interaction against real World of Xeen locations,
including signs, doors, and other simple directional interactions.

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
