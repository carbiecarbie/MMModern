# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The project is currently in early development.

## Current status

Milestones 14, [15](docs/milestone-15-plan.md) and
[16](docs/milestone-16-plan.md) are complete. Supported static
Clouds objects are visible in composed outdoor scenes with direction, scale,
clipping, terrain ordering, and resource validation. This requires the validated
World of Xeen metadata source in `DARK.CC/clouds.dat`.
The original Phirna Remove checkpoint on Clouds map 23 now updates the runtime
frame immediately without movement, preserving text presentations and other
objects. Removal persists across map changes and cache reconstruction in the
same session; a genuinely new session restores visibility.

[Milestone 17](docs/milestone-17-plan.md) is complete and is the latest stable
milestone. Stages 17A and 17B passed final independent review with verdict
**APPROVE MILESTONE 17**. All acceptance cases A01-A13 passed, and full CTest
passed **42/42**. Recorded SDL validation used dummy video/software rendering
and native-frame inspection, not physical-display hardware validation.
No Milestone 18 is approved or started. The party loads
35 Clouds quest-item counters. Events can query possession and execute the
bounded quest-item grant: normal Phirna Space -> Yes -> acknowledgment grants
exactly one root and removes the plant immediately, preserving the success
text. No and already-owned refusal grant nothing and leave the plant present.
Ownership and removal survive same-session cache reconstruction; a new session
restores the initial state. Re-interacting with the removed plant cannot grant
again. Quest-item consumption, general inventory/TakeOrGive, Myra's exchange,
disk save/load and Darkside gameplay remain unimplemented.

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
