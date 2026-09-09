# MMModern - Project Status

## Stable baseline

**Milestone 21 is the latest stable completed milestone.** This file describes
the latest stable committed capabilities and architecture. Work in progress is
intentionally excluded; milestone acceptance belongs in the
[closed plan](milestone-21-plan.md#final-acceptance), and chronology in
[project history](project-history.md).

## Supported scope

### Runtime and world

MMModern is a standalone SDL application. Its compatibility layer reuses selected
ScummVM Xeen archive/sprite code and Common, Graphics and Image libraries, without
instantiating the ScummVM engine or using its SDL backend. The pinned revision,
external source/build requirements and configuration are in
[dependencies.md](dependencies.md).

Original Clouds archives supply party/game flags, maps, objects, events, text,
fonts and sprites. Event text uses zero-based lookup, preserves original bytes
and valid empty entries, and distinguishes missing/malformed data. Gameplay
renders in a native 320x200 indexed framebuffer.

Outdoor terrain and indoor geometry support navigation, collision and bounded
teleports. Supported static outdoor Clouds objects share terrain ordering,
direction, scale, clipping and occlusion. Sprites come from `XEEN.CC`; the
validated World of Xeen layout supplies Clouds visual metadata through
`DARK.CC/clouds.dat`. This does not establish Darkside gameplay.

### Events and interactions

- Bounded event decoding/execution supports conditions, Call/Return, transfers,
  game flags and Remove. Space dispatches from the current cell/facing without
  requiring the automatic-event bit; automatic dispatch retains that gate.
- Original normal/reduced fonts, sign/door labels, centered/main/bottom windows,
  supported formatting and pagination feed resumable acknowledgment and Yes/No
  through the existing SDL loop. Gameplay blocks while a response is required.
- WhoWill selects an eligible temporary active character with F1-F6; Action 9
  reads that character's current SP. Escape cancels selection.
- Clouds NPC mode 1 uses original FAC portraits, bounded speech/rest animation,
  positioned titles and paginated dialogue. Space/Enter/Escape advances or
  acknowledges the final page. Reward warning/receipt pages use the same keys.

### Party, items, quest state and rewards

- Party state includes all 30 roster characters, ordered active membership,
  modeled rule inputs, current HP/SP and conditions. Membership aliases refer
  to the same roster owners, including their inventories.
- Each character holds four nine-slot arrays of exact material/ID/state/frame
  bytes. Existing equipment modifier behavior is retained; miscellaneous records
  add no item effects. ID zero denotes an empty slot but its other bytes persist.
- The party owns 35 uint32 Clouds quest-item counters (IDs 82..116), possession
  comparisons and bounded one-item grants/checked consumption; 30 separate quest
  flags support bounded immediate mode-104 set/clear. Game flags are a separate
  256-value category. Conditional quest-flag Action 104 remains unsupported.
- GiveEnchanted supports codes 70/71, special IDs 1..73 and 2..4 operand bytes.
  The first two bytes determine material 10/11 and special ID; the remaining
  bytes are diagnostic only. Each produced record has state 1 and frame zero.
- Execution holds at most ten pending miscellaneous records. Successful terminal
  paths use an optional global-full warning, synchronous delivery to eligible
  live roster owners, and a paginated numeric receipt before completion.
  Eligibility, tail capacity, overflow and loss follow the
  [bounded reward contract](milestone-21-plan.md#21b-bounded-reward-lifecycle).
- Idle I prints live inventories, aliases, inactive owners, raw fields, Root and
  Q2. Setup emits the same observation before initial automatic dispatch; resume
  emits restored values. Inspection does not mutate state or dispatch events.

### Save and resume

Local Windows F9 saving and startup resume preserve supported durable state
across process restarts. [README](../README.md#running-and-controls) owns the
public CLI/control reference. The persistence contract below defines eligibility,
restoration and compatibility.

## Architectural ownership and invariants

- Application owns committed camera/game flags. `XeenPartyState` owns roster,
  membership, quest counters and quest flags. `XeenWorld` owns session-disabled
  object/event identity sets. Coordinators and borrowed providers create no
  parallel gameplay owners.
- Map identity includes side and numeric map ID; object/event identities add
  the original record index, never sprite ID, visible index or coordinate.
  Disabled events become effective `None` without deletion/reordering or changed
  first-match precedence. Original metadata remains intact.
- Physical camera position and logical script address are distinct. Remove
  disables the selected object, when present, and events at the physical working
  cell, then restarts at line 0 of the logical address. Selection follows original
  eligible-record order independently of visibility.
- Camera/game-flag changes publish on event completion. Errors/abandonment discard
  working values, preserving movement committed before dispatch. WhoWill
  cancellation completes like Exit and commits working camera/flags.
- Successful party grants/consumption, quest-flag set/clear, item insertion and
  world Remove are immediate and survive later error, abandonment or cancellation.
  No exchange-wide rollback/refund or inferred relationship joins these categories.
- `XeenEventFlow` is the noncopyable production continuation owner. Execution
  values hold the reward queue, separate preferred active index, finalization
  phase and typed receipt. Delivery clears queued production and mutates roster
  items once; only final receipt acknowledgment completes execution and permits
  camera/game-flag publication. Errors discard undelivered records with accounting
  and preserve earlier mutations. Stale/wrong-phase input cannot replay delivery.
- Flow coordinates recomposition through existing owners. Suspended presentation
  shows the committed camera; rebasing preserves pages, generations and blocking.
  Remove refreshes the scene while retaining valid layers. Clearing a retained
  label on blocked navigation still forces recomposition. Manual execution errors
  are recoverable; automatic errors remain fatal.
- Geometry/object, script, text and sprite caches are disposable. Reconstruction
  consults authoritative state and read-only resources; cache lifetime never
  defines gameplay lifetime. The interpreter remains separate from SDL/drawing,
  with no nested input loop or general animation scheduler.
- `XeenSaveSnapshot` is a temporary transfer value. Restore validates unpublished
  candidates before constructing gameplay references and the first frame; it
  does not replay scripts or infer effects. Derived rules/visuals are recomputed,
  while saved modeled values retain their exact values.

## Persistence model

**Persisted:** committed side/map/position/facing; ordered active membership;
all modeled fields of all 30 roster characters, including inactive owners,
names, rule inputs, four complete item arrays, current HP/SP and conditions;
all quest counters, quest flags and game flags; complete independent disabled
object/event identity sets across affected maps. Values are not healed, clamped
to maxima or normalized to original defaults. Slot holes, unknown item bytes
and ID-zero metadata round-trip; only explicit category compaction clears empty
metadata.

**Transient or reconstructed:** resource payloads, loader metadata/diagnostics,
derived rules/frames/caches, interpreter working state and call stacks, temporary
character/object selection, reward queue/preference/finalization, pending responses,
generations, dialogue/receipt pages, portrait timing and retained layers. Fresh
sessions load original initial state; resumed sessions reconstruct independent
owners and presentation from saved values plus compatible resources.

**Format and compatibility:** the writer emits MMModern Clouds binary **v2**;
the reader accepts **v1 and v2**. `.mmsave` uses bounded little-endian encoding,
a 4 MiB limit and CRC32 corruption checks. Compatibility requires matching
`xeen.cc` length/CRC32 and `dark.cc` presence/length/CRC32, independently of the
installation path. CRC32 is not cryptographic authentication. The envelope and
unchanged fields are specified in the [M20 format](milestone-20-plan.md#4-concrete-format-mmmodern-clouds-save-v1);
the exact item extension is in [M21](milestone-21-plan.md#save-v2-and-legacy-v1-compatibility).

V2 stores 144 item bytes per character in category/slot/material-ID-state-frame
order. V1 stores equipment modifier triples: restoration preserves all saved
values and supplies only missing equipment IDs and miscellaneous arrays from
the matching initial roster slots. V2 arrays, including explicit empties, win
in full. A transient presence marker permits v1 decoding/restoration but prevents
direct encoding of unresolved v1 snapshots. Reading never rewrites the file;
an explicit eligible F9 save writes v2 through protected replacement.

**Save boundary:** startup must succeed, and no dispatch, execution, presentation,
shutdown or fatal presentation failure may be active. Nonblocking retained labels
are allowed but omitted. Pending F9 performs no capture, I/O, advancement or
queued save; a new F9 after completion is required. No target means no write.

An existing parent outside the commercial installation is required. Saving uses
a sibling temporary file and preserves the old valid save on handled write or
replacement failure. Unknown/unsupported files are protected from overwrite.
The checked Windows directory identity is retained throughout I/O, including
alias containment. The contract targets local Windows filesystems; concurrent
writers, network filesystems, running-resource replacement and arbitrary-crash
or absolute power-loss guarantees are excluded.

Resume restores owners before the first frame, with no retained UI or initial
automatic dispatch; later navigation retains normal automatic behavior. New
sessions retain normal initial dispatch. Invalid/incompatible saves fail startup
without a fresh-session fallback. There is no original Xeen/ScummVM save
compatibility, suspended-dialog save, in-session load, autosave, save-on-exit,
slot menu or general migration framework.

## Supported original-data checkpoints

These are local integration contracts, not certification of travel between them
or of a generally playable region.

| Clouds checkpoint | Supported result |
| --- | --- |
| Castle Basenji, map 1 `(8,8)` West | Original text and Yes/No; No stays, Yes teleports and recomposes the indoor destination; suspended text survives cache reconstruction. |
| Phirna, map 23 `(8,2)` North | Yes/acknowledgment grants one Root and removes the plant while retaining success text. No/already-owned refusal grants nothing and leaves it present. Completed collection cannot repeat. |
| Bone Whistle, map 20 `(5,14)` North | WhoWill/acknowledgment grants one Whistle and removes the bones. Cancellation leaves collection state unchanged and permits retry; completed collection cannot repeat. |
| Myra, map 23 `(9,11)` West | No-Root request sets Q2 after final acknowledgment, including Escape. Root-owned return consumes one Root, clears Q2 and produces five `{10,37,1,0}` rewards subject to delivery capacity/eligibility; receipt acknowledgment completes nine instructions. Further Roots allow returns; exhaustion restores request behavior. |
| Air / Corner and Snake Oil | Original sign and reduced door-label presentation; Air / Corner also exercises static object/text layering. |

Separate-process resume covers the collections, Myra request, cumulative multi-map
progress and completed exchange, with cache reconstruction and fresh controls.
The exchange checkpoint saves five exact rewards with Root=0/Q2=false, restores
independent owners, rebuilds all relevant caches together and then runs a new
no-Root request without duplicate rewards. Q2 persistence is checked as state;
repeated dialogue alone cannot prove it. Exact acceptance boundaries and oracle
are owned by the [M21 restart contract](milestone-21-plan.md#21d-production-restart-and-acceptance-boundary).

## Current boundaries

- General inventory/equipment use, item effects, shops, random treasure, generic
  TakeOrGive and NPC modes/services beyond Clouds mode 1.
- Combat, monsters, normal-route/playable-region certification, Swimming /
  Walk on Water and other unsupported movement capabilities.
- Broader outdoor animation, scripted appearance changes, indoor objects and wall
  items. Unsupported animated objects are not drawn as frozen substitutes;
  static placements are bounded. Dark indoor maps are illuminated for diagnostics.
- A general game clock/animation system and full original UI fidelity; existing
  parchment/choice/acknowledgment styling limitations remain nonblocking.
- Darkside gameplay; side-aware identity and Clouds metadata access do not imply it.

MMModern is GPL-3.0-or-later. Commercial game data stays external and unmodified;
original-data integration checks require a legally obtained installation.
Ordinary CTest does not depend on commercial data.

## Next direction

[M22 bounded outdoor object animation](roadmap.md#m22---bounded-outdoor-object-animation)
is the next planning candidate. The roadmap retains M23 as a horizon entry and
requires a post-M21 horizon review. Neither this snapshot nor roadmap direction
authorizes M22 implementation.
