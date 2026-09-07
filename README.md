# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The project is currently in early development.

## Current status

Milestones 14 and [15](docs/milestone-15-plan.md) are complete.
Milestones [16A and 16B](docs/milestone-16-plan.md) are complete: supported static
Clouds objects are visible in composed outdoor scenes with direction, scale,
clipping, terrain ordering, and resource validation. This requires the validated
World of Xeen metadata source in `DARK.CC/clouds.dat`.
The next implementation target is **16C - Visual Remove and runtime lifecycle**.
The Remove mutation checkpoint on Clouds map 23 works with session-owned
object/event disabling that survives map changes and disposable-cache rebuilds;
a new session restores the original resource state. Complete Phirna harvesting,
quest-item granting, immediate visual removal after Remove, disk save/load, and
Darkside gameplay are not implemented. Disabled objects are omitted on the next
explicit scene reconstruction; mutation does not yet trigger that reconstruction.

MMModern currently includes:

- Loading of original Xeen game resources
- Outdoor map rendering
- Supported static outdoor Clouds objects interleaved with terrain
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
