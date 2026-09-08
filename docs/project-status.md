# MMModern - Project Status

## Current development state

**Milestone 20 is the current stable completed milestone**, committed on `main`.
Its final independent approval closes M20-A01 through M20-A16 with no known
blocker within scope. The accepted baseline includes local Windows save/resume
of supported Clouds progress across separate processes.

M21, Myra's return exchange and bounded item rewards, is the default next
planning entry in the [approved roadmap](roadmap.md). A
[M21 plan](milestone-21-plan.md) records independent acceptance of committed
**21A** and explicit authorization of **21B only**. Bounded delivery, receipt and
live inspection are implemented, corrected after the first independent review,
and independently re-reviewed with verdict **APPROVE 21B FOR COMMIT**. The 21B
working tree remains uncommitted. **21C and 21D remain unstarted and unauthorized;
M21 is not complete.** Other draft decisions
are not implicitly approved. Roadmap approval and M20 completion do not
authorize implementation; M21-M23 retain their recorded confidence and review
cadence.

## Current capability frontier

- **Original Clouds resources:** archive access, initial party/game-flag loading,
  maps, objects, event scripts and zero-based event-text lookup. Text preserves
  original bytes and valid empty entries; missing and malformed data have
  explicit outcomes.
- **Scenes and movement:** outdoor terrain and indoor geometry rendering,
  navigation/collision and supported teleports. Static outdoor Clouds objects
  share terrain ordering with direction, scale, clipping and occlusion. Clouds
  sprites come from `XEEN.CC`; the validated World of Xeen layout supplies their
  visual metadata through `DARK.CC/clouds.dat` without enabling Darkside gameplay.
- **Events:** decoding and bounded execution, automatic triggers, conditions,
  Call/Return, transfers, game flags and Remove. Space dispatches from the current
  cell/facing independently of the automatic-event bit; automatic dispatch
  retains that gate. Unsupported operations remain explicit boundaries.
- **Original text and interaction:** normal/reduced fonts in the native 320x200
  indexed framebuffer; sign/door labels, centered/main/bottom windows, supported
  formatting and pagination. Acknowledgment and Yes/No suspend and resume through
  the existing SDL loop, with gameplay blocked while a response is pending.
- **WhoWill and NPCs:** F1-F6 selects an eligible temporary active character;
  Action 9 reads that character's current SP. Escape cancels WhoWill. Clouds NPC
  mode 1 presents original FAC portraits, bounded speech/rest animation,
  positioned titles and paginated dialogue; Space/Enter/Escape advances or
  acknowledges its final page.
- **Party and quest state:** original roster and active membership, modeled
  character rule inputs, HP/SP and conditions; 35 Clouds quest-item counters
  (IDs 82..116), possession comparisons and bounded one-item grants; 30 Clouds
  quest flags with bounded immediate mode-104 set; 256 separate game flags.
  All 30 roster characters own four nine-slot item arrays with exact original
  material/ID/state/frame bytes. Existing equipment rules
  retain their previous modifier behavior and miscellaneous items add no effects.
- **Bounded rewards and inspection:** transient execution-owned production holds
  at most ten miscellaneous records. Successful termination warns if all active
  category tails are full, delivers synchronously to eligible live roster owners,
  then awaits a paginated numeric receipt. Production reward opcodes remain
  unsupported. Idle I prints actual live inventories, aliases, inactive owners,
  Root and Q2; setup emits the same observation before initial automatic dispatch.
- **World changes and persistence:** session-owned object/event removals survive
  map/cache reconstruction and immediately refresh the scene while retaining
  valid presentation layers. Versioned save/resume preserves supported durable
  categories across process restarts.

## Current architectural invariants

- Application owns the committed camera and game flags. `XeenPartyState` owns
  roster, active membership, quest-item counters and quest flags. `XeenWorld`
  owns the session's disabled object/event identity sets. Event/presentation
  coordinators and borrowed providers do not create parallel gameplay owners.
- Map identity includes side and numeric map ID. Object/event identities add
  the **original record index**, never a sprite ID, visible index or coordinate.
  Original order and metadata survive removal; disabled events become effective
  `None` without deleting records or changing first-match precedence.
- Physical camera position and logical script address remain distinct. Remove
  disables the selected object, when present, and events at the physical working
  cell, then restarts at line 0 of the current logical address. Selection uses
  original eligible-record order independently of visibility.
- Geometry/object, script, text and sprite caches are disposable. Rebuilding
  them consults the same authoritative session state and read-only resources;
  cache lifetime never defines gameplay lifetime. Fresh sessions load their own
  initial state; resume restores explicitly saved state.
- Event camera/game-flag changes commit on completion; errors or abandonment
  discard their working values, preserving movement committed before dispatch.
  WhoWill cancellation completes like Exit and commits working camera/flags.
  Successful party grants, quest-flag sets and world Remove are immediate and
  survive later errors, abandonment or cancellation. Saving captures these
  surviving values without changing the mutation policy.
- Quest flags, quest-item counters and game flags are independent categories.
  Neither possession nor a request flag implies any other flag or world removal.
- `XeenEventFlow` coordinates dispatch, continuations and recomposition through
  existing owners. Suspended presentation shows the committed camera; rebasing
  preserves pages, response generations and blocking state. Manual execution
  errors are recoverable; automatic errors remain fatal.
- Interpreter working state, call stacks, temporary character/object selection,
  pending responses, dialogue pages, portrait timing and retained layers are
  transient. The interpreter remains separate from SDL/drawing; no nested input
  loop or general animation scheduler is introduced by presentation.
- Flow is a noncopyable continuation owner. Execution values hold the ten-entry
  reward queue, independent preferred active index, finalization phase and typed
  receipt. Delivery mutates roster items immediately; only final receipt ACK
  completes execution and publishes working camera/game flags. Errors/abandonment
  discard undelivered records with accounting and preserve prior mutations.
  Warning/receipt pages and response generations are transient and never saved.
- `XeenSaveSnapshot` transfers values temporarily; it is not a parallel live
  owner. Restore validates unpublished candidates before constructing gameplay
  references and the first frame. It neither replays scripts nor infers effects.
  Derived rules/visuals are recomputed, while saved modeled values are preserved.
- New persistent state categories must deliberately extend save/load, with a
  version and validation policy. The 21A complete item arrays extend the existing
  roster owners; membership aliases never copy inventories. Only explicit stable
  category compaction clears empty-slot metadata. Loading and persistence retain
  holes, unknown byte values and ID-zero metadata exactly.

## Supported original-data checkpoints

These are local integration checkpoints, not certification of travel between
them or a generally playable region.

| Clouds checkpoint | Current supported result |
| --- | --- |
| Castle Basenji, map 1 `(8,8)` West | Original text and Yes/No; No stays, Yes teleports and recomposes the indoor destination. Also exercises suspended text/cache reconstruction. |
| Phirna, map 23 `(8,2)` North | Yes plus acknowledgment grants one Root and removes the plant while retaining success text. No/already-owned refusal grants nothing and leaves it present. Repeat cannot collect again. |
| Bone Whistle, map 20 `(5,14)` North | WhoWill plus acknowledgment grants one Whistle and removes the bones. Cancellation leaves collection state unchanged and permits retry; completed collection cannot repeat. |
| Myra, map 23 `(9,11)` West | No-Root request uses original NPC dialogue and sets quest flag 2 after final acknowledgment, including final Escape. Requests repeat on revisit. Root-owned return dialogue stops before unsupported consumption, without rewards or other mutation. |
| Air / Corner and Snake Oil | Original sign and reduced door-label presentation; Air / Corner also exercises static object/text layering. |

Phirna, Bone Whistle and Myra request state also pass separate-process resume,
cache reconstruction and fresh-session controls. A cumulative checkpoint keeps
both collections and Myra's request across maps; positioning is disclosed harness
setup. Myra's repeated dialogue alone does not prove quest flag 2 persistence.

## Save and resume

The current writer uses **MMModern Clouds binary v2** and reads **v1 and v2**,
using `.mmsave`, bounded
little-endian encoding and CRC32 corruption checks. Compatibility requires the
same `xeen.cc` contents and `dark.cc` presence/contents, checked by length/CRC32,
independently of installation path. This is not cryptographic authentication.
The unchanged envelope/payload contracts live in the
[Milestone 20 plan](milestone-20-plan.md); the exact item-block extension and
legacy policy are in the [Milestone 21 plan](milestone-21-plan.md#7-durability-exact-v2-wire-and-v1-policy).

Durable categories are the committed side/map/position/facing; ordered active
membership; all modeled fields of all 30 roster characters, including inactive
members, names, rule inputs, four complete item arrays, current HP/SP and conditions;
all 35 uint32 quest-item counters, 30 quest flags and 256 game flags; and complete,
independent disabled-object/event identity sets across every affected map.
Current values are not normalized to original defaults or recomputed maxima.

Each v2 character contains 144 item bytes in weapons/armor/accessories/miscellaneous,
slot, material/ID/state/frame order. The source CHR format remains 354 bytes per
character and 10,620 bytes per roster. V1 stores only equipment modifier triples;
its transient snapshot presence marker permits structural decoding/read/restore
but prevents direct v2 encoding. Before publication, restoration supplies only
missing equipment IDs and miscellaneous arrays from the matching initial roster
slots, preserving every saved value. V2 arrays, including explicit empties, are
authoritative. Read/startup never rewrites a v1 file; an explicit eligible F9 save
writes v2 through the existing protected replacement protocol.

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
```

F9 saves only after successful startup, between completed synchronous input
callbacks, with no active dispatch, pending execution/presentation, shutdown or
fatal presentation failure. Nonblocking retained labels are allowed but omitted.
Pending interactions refuse F9 without I/O, advancement or a queued save; press
F9 again after completion. Without a configured path, F9 writes nothing. Results
appear in the console and existing window title.

Use an existing parent directory outside the commercial installation. Relative
paths resolve against the working directory; spaces/Unicode are supported.
Saving safely replaces an existing supported valid MMModern save through a
sibling temporary file. Handled write/replacement failures preserve the old save;
unknown/unsupported files are not overwritten. The checked Windows directory
identity is retained through I/O, including alias containment checks.

Resume uses its load path for subsequent F9 saves. Validated owners precede the
first composed frame, with no retained UI or initial automatic dispatch. Later
navigation keeps ordinary automatic-event behavior. Explicit new sessions retain
original loading and normal initial dispatch. Invalid/incompatible saves fail
startup without falling back to a fresh session.

Excluded from durability: original resource payloads, loader diagnostics,
derived rules/frames/caches and all transient execution/presentation state.
There is no original Xeen/ScummVM save compatibility, suspended-dialog save,
in-session load, autosave, save-on-exit, slot menu or migration framework.
The file contract targets local Windows filesystems, excluding concurrent writers,
network filesystems, resource replacement while running and absolute power-loss
or arbitrary-crash guarantees.

## Current validation baseline

The **2026-09-08 21B implementation** passed a fresh pinned-dependency Debug
configure/build in `build/21b`, **31/31 focused tests**, **58/58 full CTest**,
and all **17 excluded smoke target builds**. Myra's original direct and SDL
matrices passed with the unchanged line-8/offset-255/three-instruction consumption
frontier. Phirna and WhoWill direct/SDL controls and the existing original
cross-process save/resume/CLI regression passed. Three maximum-size receipt
pages and two synthetic warning pages were visually inspected at 320x200.
Application/SDL tests cover pending/reentrant F9 refusal, final-ACK save, restored
insertions, read-only I/setup inspection and manual/fatal cleanup boundaries.
Commands, logs and limitations are in
[21B evidence](milestone-21-plan.md#13-21b-authorization-implementation-and-validation-evidence).
21A was independently accepted after its implementation commit; its original
loading/v1/v2 evidence remains in section 12. The former "independent 21B review
is pending" status is superseded by the [final independent approval for commit](milestone-21-plan.md#15-final-independent-21b-approval-for-commit).
The reviewer confirmed the P2 correction, no remaining findings, valid 21B
architecture/exactly-once lifecycle, Debug build, 13/13 focused tests, 58/58 CTest,
17 excluded builds, Myra/Phirna/WhoWill and save/resume direct/SDL controls, and
`git diff --check`. These are user-supplied independent review results, not new
runs in the final documentation step. Earlier pending-review and correction
evidence remains preserved in the milestone plan.
This is not full M21 or original completed-exchange acceptance.

The latest accepted complete baseline is the **2026-09-08 M20 closure**:

- Implementation validation: successful Debug build with the pinned dependency,
  **53/53 full CTest**, plus Windows identity/save and focused regressions.
- Original-data acceptance: Phirna, Bone Whistle, Myra and cumulative checkpoints
  passed separate producer/consumer/fresh processes directly and through SDL
  dummy/software; executable CLI resume and Application controls also passed.
  Native resumed/fresh frames were separately inspected. Detailed results are in
  the [20C evidence](milestone-20-plan.md#14-20c-cross-process-acceptance-and-stabilization-evidence).
- The supplied final independent review reported **53/53 full CTest**, required
  original-data direct/SDL acceptance and all restart checkpoints passing, and
  inspected native frame evidence; all M20 criteria are closed in the
  [final approval](milestone-20-plan.md#16-final-independent-approval-and-milestone-20-closure).
- Separately, the user supplied Gabriel's passing physical-window results for
  Phirna, Bone Whistle, Myra and fresh-session controls. These are manual
  observations, not reviewer-executed tests or a rerun of the entire matrix.
  Myra Q2 persistence is proven by automated live-state assertions. See the
  [manual evidence](milestone-20-plan.md#15-user-supplied-physical-window-validation-and-final-review-handoff).

These M20 acceptance results remain historical evidence, separate from the 21B
runs above. Ordinary CTest remains independent of commercial data.

## Known unsupported boundaries

- Myra return consumption, quest clearing and reward production remain unsupported.
  21B supplies reusable insertion, recipient selection, pending delivery, receipts
  and live inventory inspection; synthetic tests seed typed execution records.
  General inventory, equipment use, shops and generic TakeOrGive remain outside
  the supported subset. Quest-flag clear/check Action 104 is unsupported.
- Combat, monsters and certification of normal routes or a playable region;
  Swimming / Walk on Water and other unsupported movement capabilities.
- Broader outdoor animation, scripted appearance changes, indoor objects and
  wall items. Unsupported animated objects are not drawn as frozen substitutes;
  static outdoor objects use the current map's records, with bounded placements.
  Dark indoor maps are currently rendered illuminated for diagnostics.
- NPC modes/services beyond Clouds mode 1, a general game clock/animation system,
  and complete original UI fidelity. Existing parchment/choice/acknowledgment
  styling observations remain nonblocking.
- Darkside gameplay. Side-aware identities and reading Clouds metadata from
  `DARK.CC` do not establish it. Save/load exclusions are listed above.

## Architecture / dependencies

MMModern is a standalone SDL application, not a fork of the full ScummVM
application. The compatibility layer in `src/compat/scummvm/` reuses selected
Xeen archive/sprite code and Common, Graphics and Image libraries; it does not
instantiate ScummVM's engine or use its SDL backend. External source/build trees
and the authoritative pinned revision/configuration are documented in
[dependencies.md](dependencies.md); do not duplicate the full source tree here.

MMModern is GPL-3.0-or-later. Original commercial game data is external, never
included or modified by the project; development/runtime integration checks
require a legally obtained installation.

## Documentation map

- [project-status.md](project-status.md): authority for current implemented
  state, architecture, boundaries and latest validation; the routine bootstrap.
- [project-history.md](project-history.md): concise historical context and major
  completed results; not a current-state authority or required implementation
  reading. Consult it when evolution matters.
- [roadmap.md](roadmap.md): approved future direction, confidence, replanning
  triggers and review cadence; approval does not authorize implementation.
- `milestone-N-plan.md`: dedicated scope, decisions and detailed implementation/
  validation evidence where a plan exists. See [history](project-history.md) for
  links; historical pending-stage statements describe their original boundaries.
- [dependencies.md](dependencies.md): authoritative dependency pin, configuration
  and integration requirements.

## Updating this document

Update this snapshot when capabilities, architecture, validation baseline or
milestone/planning status materially changes. Replace superseded current claims;
do not append stage histories or old test matrices. A completed milestone should
normally need a small status update and 2-4 history bullets, with detailed
implementation and acceptance evidence only in its dedicated milestone plan.
