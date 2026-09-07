# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The project is currently in early development.

## Current status

[Milestone 17](docs/milestone-17-plan.md) is complete and is the latest stable
milestone, including the completed Milestones 14-16. The recorded M17 validation
passed A01-A13 and full CTest **42/42**; these are historical results, not a new
test run. SDL validation used dummy/software rendering and native-frame
inspection, not physical-display hardware validation.

Supported static Clouds objects render with direction, scale, clipping and
terrain ordering, using validated World of Xeen metadata in `DARK.CC/clouds.dat`.
The party loads 35 Clouds quest-item counters; events support possession checks
and bounded TakeOrGive quest-item grants. The original Phirna interaction on
map 23 (Space -> Yes -> acknowledgment) grants exactly one root and removes the
plant immediately while preserving the success text. No and already-owned
refusal grant nothing and leave the plant present. Ownership and removal persist
through same-session map/cache changes; repeat interaction cannot grant again,
and a new session restores the initial state.

[Milestone 18](docs/milestone-18-plan.md) is approved/planned, with 18A as the
next implementation target; WhoWill and M18 functionality are not implemented.
Quest-item consumption, general inventory/TakeOrGive, Myra's exchange, disk
save/load and Darkside gameplay also remain unimplemented.

MMModern currently includes:

- Loading of original Xeen game resources
- Outdoor map rendering
- Supported static outdoor Clouds objects interleaved with terrain
- Immediate visual Remove with session persistence and presentation rebasing
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
