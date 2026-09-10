# MMModern - Project Status

## Stable baseline

**Milestone 25 is the latest completed milestone; both 25A and 25B are accepted.**
This file describes stable capabilities and architecture. Acceptance belongs in
the [M25 closed plan](milestone-25-plan.md#final-acceptance); completed
milestone chronology belongs in [project history](project-history.md).

## Supported scope

### Runtime and world

MMModern is a standalone SDL application. Its compatibility layer reuses selected
ScummVM Xeen archive/sprite code and Common, Graphics and Image libraries, without
instantiating the ScummVM engine or using its SDL backend. The pinned revision,
external source/build requirements and configuration are in
[dependencies.md](dependencies.md).

The read-only English item catalog is embedded reproducibly at build time from
the exact pinned ScummVM `CONSTANTS_7` Git blob. Catalog generation compiles no
ScummVM headers and adds no runtime `mm.dat` or source-tree dependency. Optional
commercial material names come from `DARK.CC/mae.xen` through the existing
`XeenAssetSource`/`ScummVmXeenBridge` archive ownership. See the
[catalog dependency contract](dependencies.md#build-generated-english-item-catalog).

Original Clouds archives supply party/game flags, maps, objects, events, text,
fonts and sprites. Event text uses zero-based lookup, preserves original bytes
and valid empty entries, and distinguishes missing/malformed data. Gameplay
renders in a native 320x200 indexed framebuffer.

Outdoor terrain and indoor geometry support navigation, collision and bounded
teleports. Supported static and ordinary animated outdoor Clouds objects share
terrain ordering, direction, scale, clipping and occlusion. Static ordinary
indoor objects use twelve bounded placements with original directional
appearances, anchors and scale masks. Exact wall-predicate admission and one
wall/object order stream provide indoor occlusion through the existing checked
object sprite path; the [M23 contract](milestone-23-plan.md#projection-and-occlusion-contract)
owns the exact tables and predicates. Geometry and objects remain one indoor
composition, not separate rendering or state systems.

Sprites come from `XEEN.CC`; the validated World of Xeen layout supplies Clouds
visual metadata through `DARK.CC/clouds.dat`. This does not establish Darkside
gameplay. Ordinary objects retain original record identities and consult
session-disabled overlays before visual resolution. Indoor commands, placements,
wall samples and raster results are derived, cache-reconstructible values.

Ordinary outdoor objects animate while stationary through the existing
Application/Flow/SDL idle path, with a 100 ms cadence independent of the NPC
portrait's 150 ms timing. Dialogue, reward and inventory presentation retain
their semantic state while the underlying scene animates. Indoor object animation is outside
this capability.

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
- At the bounded Nightshadow checkpoint, the original static gravestone at
  Clouds map 29 `(4,6)` West is visible and presents its original bottom-window
  clue through the existing manual-event, Flow and Presenter path. Acknowledgment
  blocks gameplay as usual; the interaction is repeatable and creates no durable
  mutation. M23 introduced no new event system or presentation mode.

### Party, items, quest state and rewards

- Party state includes all 30 roster characters, ordered active membership,
  modeled rule inputs, current HP/SP and conditions. Membership aliases refer
  to the same roster owners, including their inventories.
- Each character holds four nine-slot arrays of exact material/ID/state/frame
  bytes. Existing equipment modifier behavior is retained; miscellaneous records
  add no item effects. ID zero denotes an empty slot but its other bytes persist.
- A bounded read-only catalog describes supported weapons, armor, accessories
  and miscellaneous records without mutating stored bytes. It exposes raw fields,
  status and counters/charges, with explicit unknown-field and missing/malformed
  material fallbacks. List rows may be elided; selected descriptions wrap in a
  bounded two-line area, with final-line elision only for overflow.
  The [M24 catalog contract](milestone-24-plan.md#naming-and-bounds) defines its limits.
- The active-party inventory panel shows modeled condition, signed current and
  complete maximum HP/SP, all nine physical slots in each of Weapons, Armor,
  Accessories and Miscellaneous, selected status/counter or charges, and raw
  M/ID/S/F. Active indexes resolve to authoritative roster owners, including aliases.
- Manual transfer covers all four categories. Same-owner aliases and cursed items
  refuse; destination capacity uses the tail slot. Success resets the moved frame
  and stable-compacts both touched arrays, preserving occupied order and all other
  moved bytes. Current HP/SP remain exact. No class/equip-legality, canAct or
  reward-recipient restriction applies. See the
  [transfer contract](milestone-24-plan.md#transfer-rules-and-publication).
- Players can contextually equip or remove supported Weapons, Armor and
  Accessories from the same panel. E equips a selected raw-frame-zero item and
  removes a selected raw-nonzero item. Supported equip IDs are Weapons 1..34,
  Armor 1..13 and Accessories 1..10; occupied unknown equipment IDs remain
  removable. Miscellaneous stays outside equipment actions.
- Equipment preserves every physical slot and item byte except the selected frame;
  it never compacts. Bounded Clouds class/proficiency restrictions, subtype and
  two-handed/shield conflicts, ring/medal capacity, and cursed Remove refusal are
  enforced. Successful actions recompute existing modeled INT/PER/max HP/max SP
  presentation, while current HP/SP remain exact and are never healed or clamped.
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
- Opening inventory with idle I also prints live inventories, aliases, inactive
  owners, raw fields, Root and Q2. Setup emits the same observation before initial
  automatic dispatch; resume emits restored values. Inspection does not mutate
  state or dispatch events.

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
- Flow owns transient Browse / ChooseDestination / Confirm inventory state,
  mutually exclusive with event presentation. Explicit confirmation is consumed
  before transfer; generation, membership and selected-record checks prevent stale
  replay. Reconstruction invalidates confirmation; ordinary timed rebasing retains
  it. Publication precedes fallible feedback/drawing, so presentation failure cannot
  undo or repeat a move. There is no second gameplay owner or nested SDL loop.
- Each explicit occupied equipment selection arms a transient certificate over
  the active membership, authoritative roster owner, category, physical slot and
  exact item record. E consumes it before synchronous publication; stale or
  replaced records clear the selection and cannot authorize another item. The
  typed `XeenEquipmentResult` owns fixed result facts. `xeenSetEquipment` validates
  against authoritative party state and publishes exactly the selected frame byte
  only after all fallible preparation succeeds. Fallible reporting, drawing and
  recovery occur after publication and cannot repeat or roll back the action.
- Flow owns one transient shared ordinary outdoor phase and deadline; rendering
  consumes an explicit phase. Rebased presentation preserves NPC timing and
  reveals the current animated base on dismissal. Remove remains authoritative
  by object identity through animation and cache reconstruction. No general
  world clock or per-object/per-map timing store is introduced.
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
derived rules/frames/caches, indoor placement/wall/command/raster values,
interpreter working state and call stacks, temporary
character/object selection, reward queue/preference/finalization, pending responses,
generations, inventory selection/confirmation/feedback, dialogue/receipt pages,
equipment certificates/results, ordinary outdoor phase/deadline, portrait timing
and retained layers.
Fresh sessions load original initial state;
resumed sessions reconstruct independent
owners and presentation from saved values plus compatible resources.
Fresh and restored gameplay start ordinary animation at phase zero with a new
100 ms deadline. F9 preflight uses independent phase zero; saving and inventory
inspection do not advance or rearm live animation. The save format is unchanged.

M23 introduced no persistent category and no save-version change. Existing
disabled object/event identities are sufficient to reconstruct indoor visibility
after save/restore into fresh owners. Visual placement, occlusion, commands and
pixels are never serialized.

M24 adds no persistent category or save-version change. Catalog strings and
availability are resource-derived; missing or malformed optional material names
degrade descriptions without becoming save/gameplay compatibility state. Existing
archive fingerprints remain unchanged. Transferred ownership is captured in the
existing v2 roster item arrays. Inventory state is not serialized: restart begins
with inventory closed, and reopening reads actual restored owners and slots
without reconstructing items from labels or replaying transfers.

M25 also adds no persistent category or save-version change. Existing v2 item
frame bytes preserve equipped state. Resume rebuilds presentation and modeled
derived values from those saved bytes with inventory closed; it does not rerun an
Equip operation, infer or normalize a loadout, or restore a transient selection
certificate. M24 transfer remains independent: it resets the moved frame and
compacts touched categories, whereas equipment changes one frame in place.

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
open inventory, synchronous transfer, shutdown or fatal presentation failure may
be active. Nonblocking retained labels are allowed but omitted. Pending F9 performs
no capture, I/O, advancement or queued save; close inventory or complete the
interaction and issue a new F9. No target means no write.

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
| Nightshadow, map 29 `(4,6)` West | Original RIP gravestone rendered as a static ordinary indoor object with its original bottom-window clue and acknowledgment. The interaction is repeatable and has no durable state effect. |

Myra's ordinary tent-flag cycle runs without input and continues underneath
dialogue, independently of the portrait. The [M22 checkpoint contract](milestone-22-plan.md#certified-original-data-checkpoint)
owns its directional frame oracle and acceptance boundary.

Nightshadow validation covers the selected gravestone at four centered distances,
partial wall occlusion and a fully blocked control. The
[M23 checkpoint contract](milestone-23-plan.md#selected-original-checkpoint) owns
the exact bounded views and interaction boundary; it does not certify broader
Nightshadow traversal or gameplay.

Separate-process resume covers the collections, Myra request, cumulative multi-map
progress and completed exchange, with cache reconstruction and fresh controls.
The exchange checkpoint saves five exact rewards with Root=0/Q2=false, restores
independent owners, rebuilds all relevant caches together and then runs a new
no-Root request without duplicate rewards. Q2 persistence is checked as state;
repeated dialogue alone cannot prove it. Exact acceptance boundaries and oracle
are owned by the [M21 restart contract](milestone-21-plan.md#21d-production-restart-and-acceptance-boundary).

M24 extends the genuine exchange checkpoint through a manual transfer and
production save/separate-process restart with four potions on owner 0 and one on
owner 18. Independent original Dagger, Leather boots and Silver ring controls
establish equipment transfer and destination frame reset. Detailed outcomes and
the automated/physical acceptance distinction belong in the
[M24 acceptance record](milestone-24-plan.md#final-acceptance).

M25 adds original Dagger conflict/remove/equip behavior for Zippo, reversible
Leather boots and Silver ring removal/equip behavior in their original physical
slots, bounded proficiency and ring-capacity controls, and save/restart of the
resulting frames. Distinct producer, consumer, fresh-session and actual CLI
processes verify that resume preserves exact slots and performs no modal or action
replay. The [M25 acceptance record](milestone-25-plan.md#final-acceptance) owns the
complete automated, original-data, independent-review and physical boundary.

## Current boundaries

- Item use/consumption, discard, repair, paid identification, shops/trading,
  item spells/effects, complete equipment effects, damage/attack/armor-class/
  resistance modeling, combat inventory/statistics and
  recruitment/reordering. Random treasure, generic TakeOrGive and NPC
  modes/services beyond Clouds mode 1 also remain unsupported.
- Combat, monsters, normal-route/playable-region certification, Swimming /
  Walk on Water and other unsupported movement capabilities. General indoor
  traversal, connected-map behavior and playable-region certification remain
  outside the accepted checkpoints.
- Scripted object animation/appearance changes, terrain animation, ordinary
  indoor animation and wall items. Static ordinary indoor objects are supported
  only within M23's bounded placements and wall predicates; this does not cover
  doors, locks, grates, traps or scripted movement. Dark indoor maps are
  illuminated for diagnostics; lighting gameplay remains unsupported.
- A general game clock/animation system and full original UI fidelity; existing
  parchment/choice/acknowledgment styling limitations remain nonblocking.
- Darkside gameplay; side-aware identity and Clouds metadata access do not imply it.

MMModern is GPL-3.0-or-later. Commercial game data stays external and unmodified;
original-data integration checks require a legally obtained installation.
Ordinary CTest does not depend on commercial data.

## Next direction

M25's bounded equipment capability is now an available foundation. The
[roadmap](roadmap.md#current-planning-state) retains provisional investigation of
an original encounter/navigation envelope and its actual combat, recovery,
connected-exploration and persistence prerequisites; no next milestone is yet
specified or authorized.
