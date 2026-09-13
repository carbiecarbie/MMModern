# Milestone 29 - Mutable encounter continuity and durable journey state

**Status: completed and accepted.** M29A, M29B and M29C are implemented.
Automated and original-resource/process validation passed, the independent
implementation reviewer accepted M29C, and maintainer-performed physical
acceptance of the connected seed-56 Journey and fresh control passed.

This closed plan is the authoritative M29 contract. [Project status](project-status.md)
owns the current stable snapshot, [project history](project-history.md) owns the
concise chronology, and [roadmap](roadmap.md) owns future direction.

## Final objective and bounded scope

M29 connects one original encounter to continuing mutable production gameplay:

```text
mutable gameplay -> approach -> combat -> successful End -> mutable gameplay
-> navigation and item management -> save -> process restart -> further mutation
```

The accepted content contract is the Clouds map-20 Skeleton footprint: cells
`(13,1)`, `(14,1)`, `(13,2)` and `(14,2)`, all four facings, original monster
record 5/type 8, original spawn `(13,2)`, and initial camera `(13,1)` North. All
27 original actors remain present in original order. The public fresh entry is:

```text
mmodern --journey-skeleton [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>]
```

This is a bounded production Journey, separate from Diagnostic26/27. It supports
mutable inventory/equipment before combat, automatic same-cell attachment,
Attack/Block combat through genuine End, guarded return to the same mutable
owners, continued four-cell navigation/item management and v4 save/restart.
It does not certify general map-20 exploration, normal Vertigo startup or a
generally playable game.

## Inherited foundations

M29 retains the detailed accepted contracts of:

- [M26](milestone-26-plan.md) for actor identity, original-order
  movement/classification, source rendering and explicit action/pulse timing;
- [M27](milestone-27-plan.md) for combat arithmetic, deterministic random draws,
  damage, initiative, Attack/Block and mandatory automatic work;
- [M28](milestone-28-plan.md) for completed Diagnostic27 semantics, v3,
  guarded restore/save, presentation handoff and seed-1/seed-56 controls;
- [M20](milestone-20-plan.md) and [M21](milestone-21-plan.md) for the base save
  envelope, archive identity, v1/v2 compatibility, events and rewards; and
- [M24](milestone-24-plan.md) and [M25](milestone-25-plan.md) for exact item
  storage, transfer and equipment operations.

## Production entry and controls

Fresh entry loads the original 30-character roster, active owners
`[0,18,14,11,1,6]`, quest/game state, complete initial context and all 30
owner-keyed combat supplements once. It initializes all 27 actors once,
classifies the initial North view once, activates record 5 without moving it,
and binds pending-zero Journey coordination. No combat coordinator exists during
initial inventory preparation.

Without `--save-file`, fresh entry has no save destination. Duplicate,
reordered, malformed and conflicting options reject before path use. The optional
seed is a strict nonzero u32. An explicit seed bypasses sampling; otherwise one
sample is taken and zero maps to one. Normal `--load-game` selects the restored
domain from the file; it accepts no seed or Journey-entry override.

Journey controls reuse the production SDL loop and existing UI. Arrows/WASD move
or turn; period is Wait; I opens eligible inventory; F1-F6 select owners; arrows
browse categories/slots; 1-9 select physical slots; T transfers; Enter confirms;
E equips/removes; and F9 saves at an eligible quiet boundary. Space interacts
outside combat and attacks during combat; B blocks. Enter never begins Journey
combat, and R remains exclusive to completed Diagnostic27.

## Durable ownership and authority

Three lifetimes remain distinct:

| Lifetime | Authoritative state |
| --- | --- |
| Durable Journey | Party/roster character fields, exact items, quests, all 30 supplements/XP and gameplay context; world content identity, complete live actors, per-identity accounting and the single Skeleton seed; existing camera/game-flag owners. |
| Active encounter | One noncopyable `XeenCombat` borrowing those same owners and the shared boundary from automatic attachment through successful End/retirement or terminal failure. |
| Transient authority | Incarnations, revisions, retained preimages, tickets, callback guards, leases, input generations, pending work and presentation handoffs. None is serialized. |

Journey is positively admitted. `hasEncounterState` remains a conservative
ordinary-owner protection and is not treated as synonymous with an active combat
coordinator. Marked roster copy/move/replacement protections remain intact.
Temporary candidate or observation values never become a second gameplay graph.

`XeenEncounterState` owns the transient approach phase and pending countdown.
World-visible Journey activity and generation block capture during approach,
attachment, combat, End, retirement, presentation, modal work and save work.
Destroying a coordinator, Flow or presentation path cannot manufacture a quiet
boundary. Geometry, object, script, sprite and composition caches remain
disposable and never define gameplay lifetime.

Lifetime/ABA checks cover world, party, roster, camera and game flags. Callback
results are detached, owner/resource state is checked before and after fallible
operations, and stale work cannot publish over, stop or release newer authority.
An observed integrity violation is monotonic: restoring equal bytes does not
revive an old ticket or retained preimage.

## Journey validity, melee admission and items

**Journey-valid** means the durable graph can be preserved, inspected and used
by supported item operations. **Melee-ready** additionally means every currently
consumed combat contribution is implemented and safe. Fresh startup is both.
A supported item operation may leave the Journey valid but melee-unready; the
specific contribution is reported, saving remains possible at an otherwise
quiet boundary, and the player may remove or rearrange it. Admission is checked
before a live-enemy action or attachment without publishing the attempted action.

The concrete Journey-valid admission envelope is:

| Category | Admitted values |
| --- | --- |
| Membership and presence | Active owners are exactly `[0,18,14,11,1,6]`, distinct, with first/effective serialized counts 6. All roster IDs 0..29 and all 30 supplements are present; inactive owner fields remain preserved rather than initial-state constrained. |
| Context | World of Xeen Clouds, Adventurer, day 1/year 610, minutes `[480,960)`, ctr24 `[0,24)`, nine effects and six light/resistance values all zero, and rested/newDay false. The camera remains in the four admitted cells with any facing. |
| Active rule inputs | Supported class/race/sex enums; permanent level 1..255; temporary level/age and permanent/temporary attributes 0..255; birth year remains u16. Physical supplement inputs are 0..255 and XP is current u32 progression, without an initial-state formula. At least one active owner must be able to act. |
| HP, SP and conditions | HP/SP are exact signed i16 values and are never healed or clamped. Only condition indexes 12/13 (Unconscious/Dead) may be set, each to 0 or 1; all others are zero. Good requires positive HP, either injury flag requires nonpositive HP, and simultaneous Unconscious/Dead remains valid. Historical injury is not reinterpreted against current maximum HP. |

All four nine-slot item arrays retain exact material/ID/state/frame bytes,
including holes, unknown unequipped records and ID-zero metadata. Transfer keeps
the M24 tail-capacity, curse, frame-reset and stable-compaction rules. Equipment
keeps the M25 legality, conflict, capacity and one-frame publication rules.
Journey item mutation uses the Journey boundary authority directly; it never
mutates a marked owner through an ordinary helper and then adopts a new preimage.
Selections/certificates are consumed before publication, fixed results are
adopted before fallible reporting, and successful mutation invalidates stale
input without consuming gameplay time or RNG.

Melee-ready applies the actual consumed-record predicates:

| Contribution | Admitted consumed record |
| --- | --- |
| Weapon | Frames 1/13 require ID `{2,6,7,8,12,15}`, material 0 and state 0. Bare melee is allowed. The original bow ID 30/material 0/state 0/frame 4 is admitted without melee dice. |
| Armor | Equipped IDs 1..13 use material 0 or 38 and state 0 or 128, with M25 arrangement rules and broken-armor suppression. |
| Accessory | Equipped IDs 1..10 use material 0, 38, 42 or 86 and state 0 or 128, with M25 arrangement/capacity rules. The exact legacy exception is roster owner 1's Accessories material 42/ID 5/state 0/frame 8 record until an explicit item operation changes it. |
| ID-zero metadata | Frame-zero records and miscellaneous items are not consumed and do not affect readiness. A nonzero-frame ID-zero record is still validated when a consumer sees it: melee weapon frames refuse; armor may contribute strength zero and accessories may contribute an admitted attribute bonus when their material/state are admitted. The raw bytes remain preserved. |

Unsupported equipped contributions remain preservable Journey data but must be
removed or rearranged before live-enemy work. Post-victory navigation does not
apply melee readiness when no relevant live enemy remains.

## Encounter, progression and mutable return

Approach actions retain M26 publication and timing. Engagement automatically
prepares immutable environment, MON and ATT resources under retained authority,
revalidates current melee inputs and constructs combat against the existing
Journey owners. No Enter/Begin action, replacement party, alternate combat driver
or diagnostic Journey initialization exists.

The active combat uses the stored Skeleton seed and M27 scheduling. Lethal
publication atomically defeats/removes the selected actor and adds checked XP to
eligible owners once. Accounting is keyed by original monster identity; the
Diagnostic27 session-wide accounting boolean is not reused. Unconscious owners
remain XP-eligible and Dead owners do not. Overflow rejects the complete lethal
award/removal publication rather than partially applying it.

Successful End is separate runtime authority. Retirement requires that genuine
End, no remaining combat candidate/work and a quiet external boundary. It runs
once after service busy work unwinds, removes the combat coordinator, consumes
End authority and enters presentation without changing HP, SP, conditions, XP,
items, camera, actor values or time. It does not construct M28 completed-domain
authority. Only a matching successfully presented return frame opens a new
Journey input generation; old combat tickets and queued input cannot act in the
returned gameplay activity.

Defeat, support stop, failed End, integrity failure, fatal presentation failure
or shutdown leave the Journey unavailable for saving or recovery. Already
published damage, breakage, lethal/accounting and XP facts remain authoritative.
For unchanged-authority composition failure, one guarded rebuild may present
the current facts; the operation is never replayed.

## Navigation and timing

The Journey supports Forward/Backward, turns, Wait and no-event Space interaction
only within the four admitted cells. Terrain and content-edge refusal publish no
movement. No map change or true Journey re-entry is available.

Successful Forward/Backward and Wait add ten minutes and increment ctr24 modulo
24; turns increment ctr24 without minutes; terrain blocks do neither. A charged
step arms three opportunities and the distinct supplied pulse normally leaves
two. Due 100 ms idle pulses service pending work once without elapsed backlog.
Redraw, cosmetic MON/ATT or ordinary-object animation, inventory, capture and
restore advance no gameplay time or RNG. Inventory, interaction and F9 refuse
while approach work remains and do not drain it.

Round and End each add one minute without changing ctr24. A ten-minute charge
from minute 950 or later, or Round/End from 959, stops before the attempted charge
and dependent work. M29 adds no 960-minute condition processing, food, rest,
healing or day rollover.

The one nonzero-u32 seed belongs to content contract 1. Attachment initializes
the combat cursor from that retained value without changing it. Active combat
cursor position is transient because combat is unsaveable. Inventory, rendering,
capture, restore and post-victory navigation do not consume it.

## Input, presentation and save boundary

Displayed-input generations span initial entry, engagement, combat and mutable
return. SDL retains the displayed generation for a fixed poll batch, rejects held
or repeat input across generation changes, and distinguishes prepared/copied/
uploaded frames from a successfully presented frame. Application validates the
Journey generation before its intercepted F9 path, so obsolete F9 cannot reach
capture, providers, target resolution or file I/O and cannot queue a later save.

An eligible F9 requires a Journey-valid graph, pending zero, quiet approach and
combat boundaries, no modal selection/confirmation, no combat/attachment/End/
retirement/save work, no unresolved presentation lease, no integrity/fatal state,
and a current presented frame. Before victory the actor must be a coherent
activated Present target separated from the camera; afterward it must be the
coherent Defeated/accounted record produced by successful End and retirement or
restored from an eligible v4. Melee readiness is not required merely to preserve
a quiet rearrangeable inventory.

Capture retains one source authorization across callback-free capture, the
exclusive save lease, detached preflight, observers and final write. Refusal is
early and performs no provider or destination work. Existing checked target,
alias/installation-containment, sibling temporary, flush/close/replacement and
old-file protection remain unchanged.

## V4 Journey format

V4 is exclusive to Journey. It retains the complete v2 base payload and appends
a separately discriminated schema-1 Journey extension. V1/v2 remain ordinary;
v3 remains completed Diagnostic27. A snapshot cannot contain both completed and
Journey extensions.

The existing magic/envelope, little-endian fields, 4 MiB bound, payload length,
CRC32 and archive fingerprint compatibility apply to the complete v4 payload.
No installation path or commercial resource bytes are serialized.

The exact extension begins immediately after the v2 disabled-event list:

| Offset | Field | Encoding |
| ---: | --- | --- |
| 0 | Domain | u8 = 3 (Journey) |
| 1 | Schema | u16 = 1 |
| 3 | Content contract | u16 = 1 |
| 5 | Context presence | u8 = 1 |
| 6 | Context | 33 bytes |
| 39 | Supplement count | u8 = 30 |
| 40 | Thirty owner-ordered supplements | 30 x 33 bytes |
| 1030 | Skeleton combat seed | nonzero u32 |
| 1034 | Initialized map | u8 side=Clouds, u16 map=20 |
| 1037 | Original actor count | u16 = 27 |
| 1039 | Live-record count | u16 = 1 |
| 1041 | Live record for Clouds/20/5 | 19 bytes |

The suffix is exactly 1060 bytes with exact EOF. Context is, in order, u8 profile,
u8 difficulty, u16 ctr24/day/year/minutes, nine u8 effects, six u16
light/resistance values, then u8 rested and u8 newDay (33 bytes). Profile maps
WorldOfXeenClouds=0; difficulty maps Adventurer=0 and Warrior=1, while this
content contract admits Adventurer. Each owner-ordered supplement (owners 0..29)
is u8 owner; seven LE i32 values in permanent/temporary Might,
permanent/temporary Speed, permanent/temporary Accuracy and temporary AC order;
then u32 XP (33 bytes). The live record is u8 side, u16 map, u32 original record
index, i16 x, i16 y, i32 HP, then u8 activated/lifecycle/status/accounted
(19 bytes). Lifecycle mappings are Present=0, Disabled=1, Unresolved=2 and
Defeated=3; status mappings are Physical=0 and Unsupported=1. Every boolean is
encoded as u8 0 or 1. All multibyte fields are little-endian.

Structural checks cover exact counts/length, bounded allocation, ordered unique
identities, enum/boolean encoding, coordinates `[-128,31]`, HP `[0,65535]`,
supplement inputs `[0,255]`, complete item state and mutually exclusive domains.
Resource/domain validation requires World of Xeen archive compatibility, the
declared map and 27-record topology, original record-5 metadata/statistics,
four-cell event/actor isolation, a complete Journey-valid party and coherent
independent overlays.

The live target record is one of:

- Present: HP 20, activated, Physical, unaccounted, inside the footprint and not
  at the saved camera cell; or
- Defeated: HP 0, `(-128,-128)`, inactive, Physical and accounted.

All other actors must match their immutable/resource-initialized state at
capture and are reconstructed from compatible resources. Defeated/accounted v4
represents quiescent post-retirement state, never lethal-before-End work. No End
bit, countdown, combat cursor, UI state or runtime authority is encoded.

## Restore and restore-only binding

V4 restoration is startup-only into fresh, unborrowed destinations:

1. Decode and validate structure, archive identity and required providers.
2. Build unpublished party/world/camera/flag candidates containing every saved
   mutable field, context, all supplements, seed and live actor record.
3. Reconstruct immutable map/MOB/MON/EVT metadata and all 27 actors; apply the
   saved live record and validate domain, resources and overlays.
4. Compose the saved first frame under retained candidate/destination guards,
   without activation, action, pulse, event dispatch, combat or item work.
5. Publish once into fresh final owners as Unbound Journey, consume the
   restore-only EncounterFlow binding, and keep presentation/input closed until
   the matching real frame handoff succeeds.

Mutable fields are never replenished from CHR/PTY during restore. No old pointer,
candidate incarnation, ticket or modal generation crosses publication. Invalid
v4 fails startup without fresh fallback.

At the production boundary, EventFlow must consume the restore-only handoff
before acquiring its gameplay borrow. The retained guard is checked immediately
before that acquisition, then admits exactly the borrow's one revision increment
and rechecks all retained values and identities. Acquiring the borrow first would
invalidate the prepared destination guard; replacing the guard afterward would
discard the restoration authority. Arbitrary borrow, replacement or revision
changes remain rejected.

Plain v1/v2 load remains ordinary and explicit save writes v2. V1 retains its
missing-ID/miscellaneous policy; v2 explicit arrays win. V3 restores terminal
completed Diagnostic27 with its six supplements, canonical defeated overlay,
read-only inspection and bounded R. Only fresh `--journey-skeleton` and direct
v4 restore create Journey. There is no conversion, automatic rewrite, in-session
load or suspended-combat save.

## Accepted original-data contracts

The connected seed-56 witness preserves M28's combat prestate while changing
the attachment and continuation lifetime:

1. At `(13,1)` North, minute 480/ctr24 0, Seymour's Accessories slot 1
   `{86,1,0,0}` transfers to Arturius and equips as `{86,1,0,8}` with no combat,
   time or RNG advancement.
2. One Wait reaches minute 490/ctr24 1 and automatically attaches combat. Six
   displayed Blocks followed by Arturius and Tyro attacks produce successful End.
3. Mutable return is minute 492/ctr24 1 with active HP
   `[12,16,12,10,-17,5]`, SP `[2,0,2,0,7,9]` and XP
   `[100,100,100,100,0,100]`. Rebecca retains Unconscious=1 and Dead=1; armor
   slots 0/1 are `{0,2,128,3}` and `{38,10,128,9}`. Record 5 is defeated,
   inactive, HP 0 at `(-128,-128)` and accounted.
4. Post-return movement reaches `(14,2)` North at minute 512/ctr24 5. The ring
   returns to Seymour and equips; F9 writes v4.
5. Separate `--load-game` preserves those facts without replay. Removing the
   ring and moving reaches `(13,2)` West at minute 522/ctr24 7; another F9 and
   separate load retain the exact result.
6. An independent fresh Journey retains original party/items/HP/SP/conditions,
   zero XP, minute 480 and original live actors/accounting.

The distinct moved-anchor oracle saves camera `(14,1)` East and actor `(13,1)`
at minute 490/ctr24 2, restores those exact values, then Wait engages at minute
500/ctr24 3. Its prestate and timing remain independent of the connected
seed-56 combat oracle.

## Final acceptance

M29A established the Journey domain, durable owner model, current-value admission,
automatic encounter lifetime, progression/accounting and guarded mutable return.
M29B added the exact Journey-only v4 capture/restoration and source/destination
authority contracts. M29C connected fresh entry, v4 load, inventory/navigation/
events, combat attachment/retirement, SDL generations and F9 through production.

The configured build and full 87-test CTest suite passed. Focused Journey,
Application, SDL, CLI, persistence and legacy regression tests passed. Original
resource validation covered the exact actor collection/isolation, seed-56 and
moved-anchor oracles, repeated restoration and separate producer/consumer
processes. The independent implementation reviewer returned **ACCEPTED FOR
PHYSICAL ACCEPTANCE** with no required correction. The maintainer then completed
and passed physical acceptance of the connected seed-56 production sequence and
independent fresh Journey control. M29 is complete.

## Exclusions and M30 handoff

M29 excludes arbitrary map-20 exploration, map transitions/true Journey re-entry,
normal Vertigo startup, Bone Whistle travel/collection as a connected route,
additional encounters/species/grouping, Zombie/Disease, ranged/spell combat,
Run/disengagement, recovery/rest/food/healing, loot expansion, item use,
shops/services, leveling/training, recruitment/reordering, general world-clock
or RNG architecture, mid-combat save, in-session load and Darkside gameplay.

The reusable successor seam is current-owner admission -> identity-bound combat
borrow -> per-identity lethal accounting -> successful End -> guarded mutable
return, with context/progression and keyed live actor records preserved across
v4 saves. Content contract 1 still admits exactly record 5; a later contract
requires explicit compatibility, validation and evidence.

The next activity is the roadmap's route-evidence gate after the committed M29
baseline is verified. It must investigate actor influence, encounter grouping,
Zombie/Disease, attrition, recovery/Run needs and route feedback before detailed
M30 planning. M30 and M31 remain provisional; M29 completion does not authorize
their planning details or implementation.
