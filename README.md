# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

The project is currently in early development.

## Current status

[Milestone 20](docs/milestone-20-plan.md#16-final-independent-approval-and-milestone-20-closure)
is complete and independently approved, and is the current stable milestone
recorded in the reviewed candidate pending commit. M20-A01 through M20-A16
are closed. The final reviewer reported **53/53 full CTest**, original-data
direct/SDL dummy/software acceptance and all restart checkpoints passing,
and inspected native resumed/fresh frame evidence. Gabriel's passing physical-window
results remain separately attributed user-supplied manual evidence.

Clouds NPC mode-1 dialogue presents original animated portraits, positioned titles
and paginated text through the resumable event UI. Myra's original local request
at map 23 `(9,11)` West now records Clouds quest flag 2 after final acknowledgment,
including Escape. Revisits repeat her request; return dialogue with a Root still
stops before unsupported consumption. The party loads 30 Clouds quest flags from
original data and retains request state across same-session owner/cache changes.
A fresh session loads its own original state. This acceptance does not certify
ordinary travel to Myra or her completed exchange.

Supported static Clouds objects render with direction, scale, clipping and
terrain ordering, using validated World of Xeen metadata in `DARK.CC/clouds.dat`.
The party loads 35 Clouds quest-item counters; events support possession checks
and bounded TakeOrGive quest-item grants. The original Phirna interaction on
map 23 (Space -> Yes -> acknowledgment) grants exactly one root and removes the
plant immediately while preserving the success text. No and already-owned
refusal grant nothing and leave the plant present. Ownership and removal persist
through same-session map/cache changes; repeat interaction cannot grant again,
and a new session restores the initial state.

WhoWill selects a temporary active character through the gameplay UI, and Action 9
reads that character's current SP. The original local Bone Whistle interaction on
Clouds map 20 `(5,14)` North (Space -> F1-F6 -> acknowledgment) grants exactly one
item 100 and removes the bones immediately while retaining the success text.
Cancellation leaves the item/object/events unchanged and permits retry. Repeat
interaction cannot grant again; removal survives cache reconstruction and a new
session restores the initial resources. This validates the local interaction,
not normal travel to the checkpoint, combat or Orothin's quest completion.

Quest-item consumption, general inventory/TakeOrGive, Myra's exchange and
Darkside gameplay remain unimplemented.

Milestones 20A, 20B and 20C are independently approved. Local Windows saving and
startup resume preserve supported Phirna, Bone Whistle, Myra request and cumulative
progress across separate processes. Automated, original-data, SDL, native-frame
and supplied physical-window evidence is recorded in the
[Milestone 20 evidence](docs/milestone-20-plan.md#16-final-independent-approval-and-milestone-20-closure).
Use an existing output directory outside the commercial installation:

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
```

F9 saves an idle session to the configured path, replacing an existing valid
MMModern save. Pending interactions refuse the request without advancing it;
press F9 again after completing the interaction. Without `--save-file`, F9 writes
nothing. Relative paths resolve against the working directory; spaces and Unicode
paths are supported. The console and existing window title report the result.
Resume uses its load path for future F9 saves, restores the saved camera/state
and skips initial automatic dispatch. New sessions retain normal initialization.
An invalid/incompatible save fails startup without starting a fresh session.
There is no autosave, save-on-exit or in-session load. Version 1 requires matching
xeen.cc/dark.cc contents; it is not compatible with original Xeen/ScummVM saves.

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
- Clouds quest-request flags: original loading and bounded in-memory set
- Loading of Xeen event text resources
- Manual event dispatch from the current position and facing using Space
- Resumable event-text presentation and confirmation semantics
- Original Xeen normal and reduced font rendering in the indexed framebuffer
- In-game sign, door-label, main, bottom, and centered event-text presentation
- Runtime acknowledgment and Yes/No interaction without a nested input loop
- WhoWill character selection, eligibility feedback and explicit cancellation
- Clouds NPC mode-1 acknowledgment, original FAC portraits and bounded idle animation
- Automated test suite
- Manual rendering and gameplay validation

## Controls

- W/Up and S/Down move; A/Left and D/Right turn.
- Space interacts or acknowledges; Enter acknowledges; Y/N answers Yes/No.
- During WhoWill, F1-F6 selects the corresponding active party member. An
  incapacitated member produces feedback and allows another attempt.
- During NPC mode-1 dialogue, Space/Enter/Escape advances a page or acknowledges
  the final page. Y/N and F1-F6 do not acknowledge it.
- Escape cancels WhoWill, terminating the current event. Outside WhoWill/NPC
  dialogue, Escape exits the application. Repeated keydown events are ignored.
- Navigation and ordinary interaction are blocked while a response is pending.

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
