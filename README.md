# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The project is currently in early development.

## Current status

Milestone 14 is complete. [Milestone 15](docs/milestone-15-plan.md) is approved;
15A is the next implementation target and has not yet been implemented.

MMModern currently includes:

- Loading of original Xeen game resources
- Outdoor map rendering
- Indoor map rendering
- Player navigation and collision
- Party data loading
- HP and SP handling
- Xeen event decoding and execution
- Automatic map events
- Conditions and event calls
- Teleport events
- Game flags
- Loading of Xeen event text resources
- Manual event dispatch from the current position and facing using Space
- Resumable event-text presentation and confirmation semantics
- Original Xeen normal and reduced font rendering in the indexed framebuffer
- In-game sign, door-label, main, bottom, and centered event-text presentation
- Runtime acknowledgment and Yes/No interaction without a nested input loop
- Automated test suite
- Manual rendering and gameplay validation

## Requirements

MMModern does not include any original Might and Magic game data.

A legally obtained installation of the original games is required.

The current development build also requires a local ScummVM source tree
and a compatible ScummVM build.

## ScummVM

MMModern currently integrates portions of the ScummVM Xeen implementation
and links against ScummVM components.

ScummVM is copyright its respective contributors and is distributed under
the GNU General Public License.

MMModern is distributed under the GNU General Public License version 3
or, at your option, any later version.

## Project status

MMModern is experimental software under active development.

Compatibility, build instructions and architecture may change substantially
while development continues.

## Disclaimer

MMModern is an unofficial project and is not affiliated with or endorsed by
Ubisoft, New World Computing, or the ScummVM project.

Might and Magic and related names and assets belong to their respective
copyright holders.
