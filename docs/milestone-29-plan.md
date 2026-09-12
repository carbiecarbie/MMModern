# Milestone 29 - Mutable encounter continuity and durable journey state

**Status: technical plan candidate; not approved or authorized for implementation.**

Baseline: `037e17c1486d3579a04016a68bb15f8d2741263f` (`main`). This proposal
requires architecture/specification review and maintainer approval before work.
It records proposed behavior, not implemented capabilities or completed acceptance.

## Objective and scope decision

Connect one original encounter to a continuing mutable journey through the
existing Application, Flow, SDL, party, world and save paths:

`mutable gameplay -> approach -> combat -> successful End -> mutable gameplay`
`-> navigation and item management -> save -> process restart -> further mutation`.

Use the isolated Clouds map-20 Skeleton footprint: cells `(13,1)`, `(14,1)`,
`(13,2)`, `(14,2)`, all four facings, original monster record 5/type 8, original
spawn `(13,2)`, and initial camera `(13,1)` North. Keep all 27 original actors.
This is a bounded production entry into original content, not another diagnostic
or an exploration unlock on a completed Diagnostic27 graph. There is no Begin
button or combat coordinator during initial item management. Same-cell engagement
automatically attaches combat to the current owners and their current values.

One physical encounter is enough for this milestone. Navigation after victory
remains inside the four cells. There is no recovery dependency for the selected
surviving witness. Defeat and support failure terminate the session without a
save; the player may separately restart from an earlier eligible save. The
recommendation deliberately accepts a small navigable footprint to establish
ownership, admission and persistence before route expansion.

## Evidence and inherited contracts

The local gate independently matched the required repository and SHA for HEAD,
`origin/main` and direct remote main, on `main`, with clean tree and empty index.
The complete [roadmap](roadmap.md) selects M29 and leaves M30/M31 provisional.
[Project status](project-status.md) describes stable implemented capabilities.
Its final Next direction paragraph incorrectly says no successor is selected;
that documentation discrepancy does not reverse the roadmap and is not corrected
by this candidate.

Inherit rather than restate the detailed contracts for:

- [M26 resource isolation and timing](milestone-26-plan.md): actor identity,
  original-order movement/classification, source rendering and explicit pulses.
- [M27](milestone-27-plan.md): physical combat arithmetic, damage, initiative,
  deterministic random draws, publication, Attack/Block and automatic work.
- [M28](milestone-28-plan.md): unchanged Diagnostic27 retirement, v3 layout,
  completed preimage, guarded restore/save, presentation handoff and original
  seed-1/seed-56 controls.
- [M20](milestone-20-plan.md), [M21](milestone-21-plan.md): existing base save
  fields, archive identity, v1 missing-field policy, v2 items and event/reward
  publication. [M24](milestone-24-plan.md) and [M25](milestone-25-plan.md) own
  transfer/equipment operations, exact slots and current-stat preservation.

### Implementation findings that determine this design

| Actual baseline implementation | Consequence for M29 |
| --- | --- |
| `XeenWorld::hasEncounterState` includes irreversible marking, initialization and actors; `restoreSessionState` refuses such worlds. | A journey must be a positively admitted domain, never an ordinary graph obtained by clearing markers. |
| `XeenCombat` constructs `Impl::expected` as a party copy, marks world/roster, calls `xeenValidateInitialCombatParty`, and installs six supplements before Preparation. | Current-state admission and attachment must be separate from resource initialization; marked journey owners cannot use that copy constructor. |
| `XeenRoster::requireOrdinary` rejects copy/move/replacement of marked rosters; ordinary copy/swap copies characters but not supplements. | Preserve protections and explicitly separate detached values from owners. Merely permitting copies would lose progression or transfer authority. |
| `XeenActorApproach::validateDomain` requires all-Good positive-HP original membership; `validateEnvironment` requires a Present anchor. | Split immutable content validation from mutable party/actor validation, including the defeated-anchor case. |
| `xeenValidateCompletedParty` enforces an original item multiset, original inactive owners, initial XP plus one award, HP >= -23 and current-max-based injury consistency. | It remains exclusively Diagnostic27 validation. These are not journey invariants. |
| `_combatEntered`, `_combatAccounted`, terminal approach and completion are session-wide diagnostic facts. | Journey encounter activity is per engagement; defeated-identity accounting survives retirement independently. |
| `Application::playGameplay` in `XeenGameplay.cpp` captures an irreversible `encounter` boolean for save refusal and routes every nonordinary entry through encounter setup. | Replace routing decisions with explicit domain/activity predicates, preserving early refusal for diagnostics. |
| `XeenEventFlow` gates inventory/events/navigation on encounter presence; `canSave` also checks modal work and frame handoff. | Journey existence must not mean active combat or blocked ordinary interaction. |
| `XeenSaveState` selects ordinary or completed capture/restore; v3 has an exact 243-byte suffix. | Add a distinct journey representation and restore branch; do not reinterpret v3. |

Other inspected seams include `XeenPartyLoader`, `XeenCharacterFormat`,
`XeenGameplayContextFormat`, `XeenCharacterRules`, `XeenMovement`, equipment and
transfer, `XeenEncounterFlow`, `XeenRestoreGuard`, `XeenGameplayBorrow`,
`XeenSaveFile`, CLI parsing and `SdlWindow` displayed-input/poll-batch handling.
Relevant test evidence includes `XeenCombatAuthorityTests`,
`XeenCombatPersistenceTests`, `XeenCompletedRestoreTests`,
`XeenCompletedGameplayOracle`, `XeenRestoreReplayProbe`, actor approach,
encounter Flow, inventory/equipment gameplay and save tests located through CMake.

### Targeted original/reference evidence

[Dependencies](dependencies.md) selects ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The configured source checkout was
verified at that exact SHA, detached and clean. Reference inspection was limited
to the relevant `engines/mm/xeen/character.cpp`, `party.cpp` and `interface.cpp`
consumers and the inherited M26/M27 source contracts.

- `Character::subtractHitPoints` evaluates max HP when applying damage and sets
  condition bytes then. It does not establish a requirement to reinterpret that
  injury after equipment changes. This supports preserving historical condition
  bytes and rejecting M28's current-max equality as a journey invariant.
- `Party::changeTime` performs condition processing across 480-minute boundaries,
  and per-charge Confused/Paralyzed processing. With only Good, Unconscious and
  Dead admitted and minutes remaining in `[480,960)`, M26's checked additions
  remain sufficient. Death severity does not advance merely from redraw or an
  item operation.
- `Interface::chargeStep/stepTime` separates ten-minute outdoor charges, pending
  monster opportunities and ctr24. Keep the existing explicit-pulse adaptation;
  do not turn cosmetic deadlines into elapsed gameplay time.
- CHR parsing shows ordinary `parseRoster` omits physical inputs and XP, whereas
  `parseCombatInputs` reads offsets 20/21, 28/29, 30/31, 34 and LE u32 at 348.
  PTY parsing supplies the distinct context fields documented by M26/M27.

The existing read-only `XeenEncounterSmoke` probe was inspected and run against
the external installation. It reported 90 MON records, 27 original actor
identities, 64 isolation configurations and four approach traces passing.
Its assertions cover the four-cell terrain/event domain and all bystanders,
including a stronger forced-activation movement control. This corroborates the
accepted M26 boundary; the existing binary was not rebuilt for this prose task.
It does not certify new M29 behavior, post-victory rendering or physical controls.

The post-victory isolation conclusion is an inference from that evidence and the
movement/classification kernels: removing record 5 cannot bring a bystander
inside the four-cell/four-facing influence union. M29 must add an explicit
defeated-anchor matrix to test that conclusion. No route to the Whistle, broader
map survey, dependency change or commercial resource extraction is part of this
investigation.

## Production entry and connected witness

### Entry interface

Proposed supported syntax:

```text
mmodern --journey-skeleton [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>]
mmodern --load-game <game-directory> <save-path>
```

`--journey-skeleton` names a bounded content entry, not a milestone diagnostic.
Reuse the normal gameplay window and controls. Fresh entry loads the original
30-character roster, original ordered active owners `[0,18,14,11,1,6]`, original
quest/game state, initial context and map resources once. It sets the declared
camera once, initializes actors once, classifies the initial view and starts
with no pending movement or active combat. Record 5 is activated by the initial
North view but does not move without an opportunity. No actor is suppressed.

Without `--save-file`, fresh entry has no save destination. Normal `--load-game`
retains its existing save-target behavior and selects the domain from the file.
Reject duplicate, reordered, malformed and conflicting options before opening
paths. No camera overrides, encounter-26/27 mixing or seed override on load.
Only fresh entry creates Journey state; v4 loads restore it directly. Existing
v1/v2/v3 files remain in their original domains.

The entry screen must state the four-cell boundary, surviving-combat objective,
time limit, inventory/save controls and absence of recovery. Show position,
facing, relevant actor approach and readable boundary feedback; the original
partly occluded Skeleton pixels alone are insufficient threat feedback.

### Connected acceptance sequence

Use fresh original entry with seed 56 and a protected save target outside the
commercial installation. This sequence intentionally retains M28's combat
prestate while changing its attachment and continuation lifetime:

1. At `(13,1)` North, minute 480/ctr24 0, open the existing inventory. Transfer
   Seymour (owner 6) Accessories physical slot 1 `{86,1,0,0}` to Arturius
   (owner 0) slot 1, then equip it as `{86,1,0,8}`. Both slot-0 necklaces remain
   exact. Close inventory. The +3 Speed changes initiative/AC. Verify that no
   combat coordinator existed during these mutations and no time/RNG advanced.
2. Press period once. The charged Wait advances to minute 490/ctr24 1, moves
   record 5 to the camera, publishes engagement, then attaches combat without
   resource party replacement or an Enter/Begin action. Block for the six
   displayed owners in the first round, then attack with Arturius and Tyro.
3. After genuine lethal publication and successful End, require minute 492,
   ctr24 1, active HP `[12,16,12,10,-17,5]`, SP `[2,0,2,0,7,9]`, XP
   `[100,100,100,100,0,100]`. Rebecca (owner 1) retains Unconscious=1 and Dead=1;
   all other condition bytes remain original. Her armor slots 0/1 become
   `{0,2,128,3}` and `{38,10,128,9}`. Record 5 is defeated, inactive, HP 0 at
   `(-128,-128)` and accounted. Verify successful End runtime authority before
   guarded retirement; post-retirement observation verifies defeated/accounted
   consequences rather than a per-actor End field.
4. After the new gameplay frame is presented, turn Right to East, move Forward
   to `(14,1)`, turn Left to North, move Forward to `(14,2)`. Allow actual pending
   pulses to settle. Expected minute 512/ctr24 5. No actor or injury changes.
   Open inventory, transfer the Speed ring from Arturius slot 1 back to Seymour
   slot 1 (frame resets to 0), equip it on Seymour (frame 8), and close inventory.
   Assert exact preservation of Rebecca's injuries and broken armor, every XP,
   SP and unaffected slot. Save at this quiet boundary.
5. Exit the actual process. Start a separate process with `--load-game`. Before
   any gameplay action, require the saved camera `(14,2)` North, minute 512,
   ctr24 5, exact owners/items/progression/world facts, closed inventory and no
   active encounter. Remove Seymour's ring with E, close inventory, turn Left
   to West and move Forward to `(13,2)`. After pending work settles, require
   minute 522/ctr24 7, Seymour slot 1 `{86,1,0,0}`, unchanged injuries/XP and no
   respawn. Save again, exit and load again without replay.
6. Run an independent fresh no-load journey control: original ring on Seymour
   unequipped, original HP/SP/conditions and XP, minute 480, original actors and
   no accounting or active combat. Also keep the unchanged Diagnostic27 fresh control.

North increases y in `XeenMovement`; the route above uses that convention.
The combat numbers are inherited original-data oracles only because item
prestate, context, seed, action order and random consumption are unchanged.
Implementation evidence must compare those assumptions before reusing them.
The new navigation/context/slot expectations are derived separately above.
If the sequence diverges, diagnose it against rules and the action trace; do not
change expected numbers to match new capture output or inject a winning state.

## Ownership, initialization and detached values

### Three different lifetimes

| Lifetime | Authoritative location and contents |
| --- | --- |
| Durable journey | Party/roster character fields, exact items, quest state, owner supplements/XP and gameplay context; world content-domain identity, live actors, per-identity accounting and the single Skeleton combat seed; existing committed camera/game-flag owners. |
| Active encounter | A noncopyable `XeenCombat` borrows those same owners plus its boundary from attachment through successful End/retirement or terminal failure. Current target identity, phase, rounds, candidates and RNG cursor are encounter work. |
| Transient authority | Owner incarnations/lifetime controls, retained operation preimages, revisions, tickets, callback guards, modal/input generations, busy guards and presentation leases. None is serialized or transferable through a value copy. |

Add an explicit Journey domain/entry discriminator; leave Ordinary,
Diagnostic26 and Diagnostic27 meanings intact. `hasEncounterState` must remain a
conservative protection against ordinary replacement, not become synonymous with
an active coordinator. Add separate queries for journey validity, active/pending
work and save eligibility. Never reset the diagnostic marker to permit saving.

World-owned journey facts outlive Flow and combat. Party supplements outlive
combat. Keep marked roster copy/move/replacement refusal for journey owners too;
ordinary item/character mutation through existing operations is distinct from
wholesale owner replacement. Retiring one encounter releases its exclusive
combat borrow and callbacks, not the party's identity or durable fields.

`XeenEncounterState` retains the transient approach phase, pending countdown and
coordination revision under the existing Flow/coordinator model. World remains
the owner of actors/lifecycle and durable Journey facts; it does not acquire the
exact countdown or a duplicate authoritative pending counter.

Use a world-visible runtime activity guard with an owner binding and generation
to block direct capture while approach work, attachment, combat, End, retirement
or presentation is outstanding. Acquire the guard before starting such work;
only current coordination may release its matching generation after proving
pending == 0 and no remaining active work. Transition between blockers without
an observable capture-eligible gap. World revisions used for publication safety
remain distinct from the transient approach countdown/revision contract.

Fresh entry and v4 restore bind fresh transient approach coordination at a quiet
pending-zero boundary. Binding itself performs no classification, movement,
time or event work; fresh content initialization separately supplies its one
initial classification, while restore uses saved activation unchanged. Pending,
coordination revisions and activity guards are never serialized. Unexpected
adapter/coordinator loss closes or invalidates the session even if its last
count was zero; destruction cannot release a blocker into save eligibility.
Diagnostic26/27 retain their existing approach-state contracts and markers.

Reuse `XeenEncounterFlow` as coordination inside `XeenEventFlow`; distinguish its
journey exploration activity from the optional combat coordinator. Do not create
a parallel mutable party, second world, alternate SDL loop or second diagnostic
framework. Temporary restore candidates are unpublished owners with no gameplay
borrow, and never remain as a second live gameplay graph after publication.

### Initialization and complete presence

Keep ordinary loading and v1/v2 ordinary behavior unchanged. At explicit fresh
journey initialization, install all 30 owner-keyed combat supplements from CHR,
using `parseCombatInputs`, and the initial context from PTY. All 30 are retained
because the roster owns progression even when a character is inactive; no new
recruitment control is implied. Context is present for the entire journey,
including before attachment and after retirement.

Presence is explicit, independent of zero: zero XP, zero temporary inputs and
empty item records are existing values. A journey requires complete supplements
and context. A present record is authoritative and is never replenished from
resources. Partial in-memory initialization or a partial v4 payload is invalid,
not an invitation to fill gaps during capture or restore.

Supported v1/v2 files remain ordinary saves and v3 remains completed
Diagnostic27. None can implicitly or explicitly become Journey in M29.
Fresh `--journey-skeleton` initializes a new journey from compatible original
resources; v4 restoration installs every saved mutable value directly and
never supplies missing supplements, XP or context from CHR/PTY. No existing
file is automatically rewritten or upgraded, and no migration interface or
general migration framework is added. Invalid entry/restoration fails without
a fresh-game fallback or relocation.

### Copy/consumer audit

The targeted consumer audit found owner copies in the composition branch and
combat preimage below, fresh ordinary loader/restore value transfers, and
explicit field-array preimages in completed/restore guards. Inventory rendering
and `CloudsUiComposer` consume const owner references; item transfer/equipment
use individual character candidates and publish only item arrays/frame bytes.
Recheck these consumers during integration. Specifically replace
`XeenGameplay.cpp`'s non-Diagnostic27 `observedParty = party` composition path for
journeys, and `XeenCombat::Impl::expected(p)` on journey attachment. Use const
owner reads protected across callbacks, or explicit detached observation structs
containing character arrays, supplements/presence, membership, context, quests
and metadata. No observation struct may install authority back into an owner.

`XeenRestoreGuard`/`xeen_state` already demonstrate explicit complete preimages;
extend them to journey fields and activity rather than invent incomplete copies.
Audit inventory rendering, portrait/rule composition, capture, save preflight,
resource-provider return values and retained failure preimages. Private fresh
candidate publication must move all durable supplement/context values explicitly
and bind fresh authority; `swapOrdinary` cannot publish a journey. Public owner
protection is not relaxed to make that operation convenient.

## Mutable domain and admission

Two predicates are required. **Journey-valid** means the durable graph can be
preserved, inspected and used by its supported ordinary operations.
**Melee-ready** additionally means all current combat-consumed inputs are
implemented and safe. Fresh startup must be both. Item management can leave a
journey-valid but melee-unready loadout; report the specific unsupported equipped
record and permit removal/rearrangement. Before a live-target navigation/Wait
action or attachment, require melee-ready without publishing that attempted
action. The player can repair the loadout through supported controls.

Do not construct a coordinator and assume its construction legitimizes arbitrary
state. Admission is a guarded, side-effect-free current-value check, followed by
nonthrowing attachment after all fallible resource/sprite preparation succeeds.
Revalidate at engagement because item state can differ from startup. During
combat, retained exact preimages admit only published combat deltas; do not keep
rerunning initial-party or completed-party validation on injured owners.

### Party and rule envelope

| Category | M29 treatment |
| --- | --- |
| Membership/identity | Exactly `[0,18,14,11,1,6]`, distinct owners, roster slot IDs 0..29, first/effective counts 6. No aliases, recruitment or reordering in this domain; ordinary saves keep their accepted alias behavior. |
| Active modeled inputs | Valid supported class/race/sex enum values; permanent level 1..255, temporary level/age and permanent/temporary attributes 0..255; birth year retains u16. Physical supplement fields remain 0..255, XP u32. Run checked `validateForUse`, all physical-stat, AC and attack-count consumers before attachment. No initial-CHR equality for current values. |
| Inactive owners | Preserve every base field, name, raw item/condition byte and supplement exactly under the existing save structural bounds. Do not require initial CHR equality or treat inactive data as active combat input. |
| HP/SP | Exact signed i16. No cap to current or original maxima, no healing/clamping on transfer/equip, capture or restore. Positive HP can exceed a later reduced maximum. SP is preserved; Attack/Block does not spend it. |
| Active conditions | Only indexes 12/13 (Unconscious/Dead), each 0 or 1; all others zero. Good requires HP > 0; either injury flag requires HP <= 0. Allow both flags simultaneously. Require at least one `canAct()` owner. Do not compare historical injury against today's max HP, and do not impose M28's -23 lower bound. |
| XP | Any existing u32, including nonzero and near-overflow values. Use the checked additive `xeenCombatExperience` with current XP, current permanent level and eligibility at lethal publication. |
| Quest counters/flags and game flags | Preserve all existing independent fields and bounds. They are not inputs to this Skeleton's combat. No inferred relationship to item possession or encounter completion. |
| Disabled objects/events | Preserve complete original-identity sets, including compatible maps outside this footprint, and validate against resources. No event exists inside the certified footprint even before overlay filtering. |
| Context/camera | Present bounded context below; camera stays inside the four cells with any of four facings. Restored context is saved state, not fresh PTY. |

The broader modeled numeric range admits supported saved values and composition
controls; it adds no way to change class, level, age or statistics through new
gameplay. Values outside this rule envelope are unsupported; invalid enums,
owner mismatches, partial presence and arithmetic overflow are malformed/unsafe.
Unconscious or dead owners remain inspectable and can participate in the same
item operations already allowed by M24/M25; those operations have no canAct gate.
No recovery or resurrection is added.

### Item storage versus combat effects

Retain all four nine-slot arrays, exact order, holes and material/ID/state/frame
bytes. A carried miscellaneous reward remains data; it does not become an item
spell, potion effect or combat bonus. Unknown unequipped records are preservable.
Transfer uses its existing tail-capacity, curse refusal, frame reset and stable
compaction rules. Equipment uses M25 proficiency, conflicts, ring/medal limits,
one-frame publication and cursed-removal refusal. Never normalize equipment on
entry or load and never enforce the original global item multiset.

Melee-ready uses the actual consumer predicates, not just occupied-slot labels:

- Melee frames 1/13 require an implemented M27 weapon ID `{2,6,7,8,12,15}`,
  material 0 and state 0, with legal equipment arrangement. Bare melee is allowed.
  The original bow ID 30/material 0/state 0/frame 4 remains admitted but supplies
  no melee dice; no ranged command is added. Other equipped weapon effects,
  broken/cursed melee weapons and counters are unsupported before publication.
- Equipped armor admits IDs 1..13, materials 0/38 and state 0 or 128. Reuse
  implemented AC strengths and broken-item suppression. Removed or transferred
  broken armor remains broken and may be equipped again under existing item
  rules; there is no repair. Unknown or unsupported good armor contributions
  refuse combat before the AC helper can throw during an action.
- Equipped accessories admit IDs 1..10, materials 0/38/42/86 and state 0 or 128.
  The original owner-1 material-42 ID-5 frame-8 legacy exception remains exact until an
  explicit item operation changes it. Normal arrangements follow M25 frames and
  capacity. Material 86 supplies the existing +3 Speed. Do not add other
  modifier families to combat admission merely because inventory presentation
  already models them. The Skeleton path checks current physical statistics,
  initiative, AC, weapon dice and max HP where damage actually consumes it,
  using the existing bounded M27 rules and checked character calculations.
- Frame-zero items and all miscellaneous records are not consumed by these
  physical rules and are not rejected for unrelated IDs/materials/state bytes.
  Existing preserved nonzero empty metadata needs special care: the current
  attribute scan has no ID-nonzero check, the weapon helper selects frames 1/13,
  and AC selects frame/nonbroken state. Validate any such consumed raw record
  by the same contribution envelope; do not silently skip it because ID is zero.
  An ID-zero melee-frame record is melee-unready. An ID-zero armor record can
  contribute strength zero with admitted material/state, and an accessory empty
  record can contribute an admitted attribute bonus. Preserve the raw bytes.

Unrelated inventory data does not invalidate a journey save. Pre-encounter
navigation with a live target requires melee-ready; post-victory navigation does
not impose weapon effects on a world with no live relevant enemy. Domain tests
must distinguish those predicates rather than globally banning inventory that
this encounter never consumes.

Keep synthetic HP-accessory coverage only as a focused test of historical injury
preservation when existing equipment operations change a modeled maximum. Such
records remain preservable Journey data under existing item operations; they
do not widen melee readiness or create a new original-content combat witness.
Remove or rearrange an unsupported equipped contribution before attachment.
Do not require unrelated HP/SP modifier-family combat matrices for M29.

### Progression and once-only consequences

Replace the journey use of a session-wide accounting boolean with world-owned
accounting keyed by original monster identity. Keep Diagnostic27's existing
boolean semantics. A lethal action prepares all eligible XP additions with
`xeenCombatExperience`; any overflow rejects the entire lethal publication and
terminates active combat safely, without removing the actor or partially
awarding owners. Earlier damage and other published facts remain.

Publish actor defeat/removal and all XP deltas atomically, consuming that identity
once. An Unconscious owner remains eligible; a Dead owner does not, using the
existing worst-condition predicate. Successful End is separate runtime authority
required for retirement, not a durable per-actor fact.
Neither retirement nor load recomputes XP as initial XP plus 100/82, derives
eligibility from later state, or allocates rewards again. Do not infer XP totals
from the accounting set when validating a journey. A save is not a cryptographic
history proof; validate coherent values and lifecycle, not a fabricated transcript.

For the reusable seam, encounter-local entered/work/acted flags begin fresh for a
new admitted identity while owner XP/injuries and earlier accounted identities
remain. Synthetic domain controls compose two distinct supported identity
instances and nonzero XP through the same arithmetic/publication kernel. They
must not expose a second original encounter or a target injection control in
the production entry. No leveling, training or generic progression system.

## Entry, exit and publication state machine

| State | Authorized work and transition |
| --- | --- |
| Unpublished entry candidate | Validate resources, fresh complete initialization, current party, all actors and initial presentation against retained destination/candidate guards. Publish once into fresh owners, or reject startup without a window/fallback. |
| Journey exploration, no pending work | Inventory/party inspection, supported transfer/equipment, bounded navigation/facing/Wait, no-event interaction and eligible F9. No combat borrow exists. |
| Approach opportunity pending | Existing action/pulse kernel owns countdown 1..3. Refuse inventory, event dispatch and F9 without draining work. Navigation may service the inherited old-opportunity rules. |
| Engaged, attachment pending | A current classify boundary selects original record 5 in same-cell slot 0, retires approach pending work, and invalidates exploration/modal input. Hold gameplay/save closed while guarded current-state/sprite admission and combat attachment complete. |
| Active combat | Existing player-ready, preparing action, pending enemy and pending round states. Borrow fixed owners, target and camera; only Attack/Block and required automatic work. No inventory/events/navigation/save. |
| Victory awaiting End | Lethal actor/XP publication has happened. Required End is pending; all ordinary actions and saving remain closed. Failure here preserves consequences but does not become success. |
| Victory ended, retirement pending | End has charged its minute and established current successful-End runtime authority; no candidate/work remains. After service busy guard unwinds, verify exact owners/preimages and consume a current retirement ticket. |
| Journey return awaiting frame | Nonthrowing retirement releases encounter callbacks/borrow, invalidates old tickets/generations, and preserves live party/world/context. A world-owned presentation lease keeps ordinary input/save closed until the matching gameplay frame is uploaded and presented. |
| Journey exploration after retirement | A fresh displayed generation opens the existing mutable controls. Target remains defeated/accounted; no immutable completed preimage governs later legitimate mutation. |
| Defeat / SupportStopped / integrity or fatal failure | Preserve already published facts, retire pending authority and terminate the M29 session. Escape/window close is available; no save, recovery, movement, implicit reload or autosave. |

Retirement must prepare fallible bookkeeping before its final checked,
nonthrowing publication. It does not mutate HP, XP, inventory, camera, actor
coordinates or time. Do not invoke `retireCompletedVictory` or construct
`XeenCompletedEncounterAuthority` for the journey. Share the guarded retirement
principles, not the Diagnostic27 immutable-result contract.

The successful End requirement is consumed by guarded retirement. Save capture
remains forbidden after lethal publication, during pending End and before
retirement. An eligible post-combat v4 therefore represents a retired quiescent
journey through its defeated/accounted actor and resulting party/context/world
values. It stores no per-actor End bit. Restore neither infers nor replays End;
it validates that quiescent save domain and establishes fresh runtime authority.

Lifetime/ABA guards cover world, party, roster, camera and game flags even if an
object is destroyed and reconstructed at the same address. Every provider,
observer, reporting callback, frame return/copy and exception path checks the
retained entry authority. A stale operation cannot stop, publish over, clear a
lease belonging to, or reacquire authority from a newer operation. Resource
validation includes nested map/MOB providers and warm-cache preimages.

Unsupported loadout before an attempted action is a recoverable no-publication
refusal, not an active-combat failure. Invalid resources fail startup; unexpected
integrity/rule failure after an action has published is terminal. A content-edge
move is a readable no-publication refusal, preserving existing pending work and
time; unlike Diagnostic26 it need not kill an otherwise valid journey. Actual
terrain collision retains its existing feedback and separate supplied pulse.
Time-limit failure is terminal as specified below.

Every supported exploration mutation must use the current owner/boundary guard,
consume its input certificate before publication and explicitly adopt its own
fixed result before fallible feedback. Update the operation preimage only for
that authorized publication. This includes item actions, navigation, context
charges and the existing event owner mutations. An arbitrary changed owner must
not become legitimate just because Flow is between encounters. Keep permanent
integrity latches separate from fresh preimages for later legitimate operations.

For unchanged-authority composition/report failure, permit one guarded rebuild
of the current durable facts while retaining a world-owned presentation lease.
Only actual successful SDL upload/present of its matching frame releases it.
No retained gameplay input is replayed. Integrity violation, failed recovery,
lost/failed upload or shutdown keeps the graph unavailable. Destroying Flow or
combat cannot reopen it. Existing completed diagnostic failure behavior remains
unchanged.

Extend displayed-input generations and poll-batch/key-release guards across both
engagement and return. A key queued for combat cannot become a gameplay event,
and old movement/Enter/F9 cannot be accepted on the returned frame. Input repeat
never supplies another attack or retirement. Mapping Space to Attack is
activity-specific; outside combat it remains the existing interaction action.
Enter does not start or restart an encounter. R remains a completed-diagnostic
control and refuses in Journey.

## Durable actors and reconstruction

Keep the world-owned complete original-order actor collection. Immutable identity,
spawn metadata and MON statistics come from compatible original resources;
live x/y, HP, activation, status, lifecycle and accounting are
authoritative state. Occupancy, selected slots, visibility, cosmetic frames and
draw commands are disposable derivatives.

M29 supports exactly one initialized actor map, Clouds 20, all 27 identities and
one mutable identity, record 5. Before combat, that actor can move among the four
cells and retain activation even when facing changes. After defeat it has the
canonical removed coordinates and cannot reappear. Bystanders remain fully
resource-initialized, including any Disabled/Unresolved metadata. Validate all
their live fields, not just positions. The original no-bystander-influence proof
must hold for every admitted camera/facing and every reachable anchor position,
and separately after defeat. An effective Remove overlay cannot hide an original
event or actor to manufacture isolation.

The minimum durable representation is therefore an explicit anchor live-state
record plus resource reconstruction of proven-unchanged bystanders. A defeated
identity alone is insufficient for pre-encounter saves after delayed approach.
The keyed record includes coordinates, HP, activation, status, lifecycle,
and accounting. No sparse inference from a renderer or
an empty occupancy slot is allowed. Require this record even when its values
match initialization, so absence cannot mean an uninitialized journey.

| Operation | Actor/context behavior |
| --- | --- |
| Fresh entry | Initialize original actors and complete context/supplements once; classify the declared initial North view once. Bind pending-zero transient coordination without repeating that initialization. |
| Live action/pulse/combat | Mutate existing authoritative actors through guarded publication. Reuse immutable metadata checks without resetting actors. |
| Cache discard/reconstruction | Load compatible resource metadata and rebuild derived caches against live state. Never call the new-entry initializer or replace live actors from MOB. No activation, time or RNG advancement from reconstruction. |
| True map entry/revisit | Not offered in M29 Journey. Reject map-changing movement/teleports and explicit re-entry before publication; R cannot relocate the party. M28's completed R remains separate. |
| Process restoration | Build all original actors in an unpublished world, apply the saved live anchor record, validate bystanders/content/party, then publish fresh owners. Preserve the saved camera and activation; do not classify for activation, move actors, dispatch initial events or replay combat. |

Environment validation is split into resource topology/isolation and permitted
live-actor state. The first may use immutable spawn records for its proof; the
second accepts either a present live target or a coherent defeated target. Do
not pass a defeated collection through the current Present-anchor validator.
There is no general map actor manager or arbitrary Clouds actor admission.

## Context, time, randomness and quiet boundaries

The party retains one `XeenGameplayContext` throughout the journey. Profile is
WorldOfXeenClouds, difficulty Adventurer, day 1/year 610, minute `[480,960)`,
ctr24 `[0,24)`, effects and light/resistance arrays zero, rested/newDay false.
Current year used by rules comes from this admitted context. No PTY reread may
reset it after entry. Initialization metadata does not become a second clock.

Apply M26 action charging before and after combat: successful Forward/Backward
and Wait cost ten minutes and increment ctr24 modulo 24; turns increment ctr24
without minutes; terrain blocks do neither. A step flushes old movement at its
candidate destination and arms count 3; the separate supplied pulse normally
leaves 2. Wait consumes old and immediate opportunities. Due 100 ms idle pulses
service pending work once without backlog. Post-victory the same charges and
countdown semantics apply even though no influencing live actor remains.

Opening/using/closing inventory costs no gameplay time or RNG. Permit opening
only at pending=0 without a due engagement/attachment. No modal operation freezes
an outstanding approach opportunity; refuse it instead. Cosmetic clocks can
continue under inventory. Manual interaction at the certified cells returns
the existing no-event result without time or actor work. Original EVT validation
proves there are no scripts there; synthetic preservation/event composition
tests do not claim new original event coverage.

Keep M27 combat scheduling: individual commands have no minute charge, Round
and End each add one, combat does not increment ctr24. A ten-minute charge from
950 or later, or a Round/End from 959, stops before the attempted charge and its
dependent work. Earlier lethal consequences remain if End fails. No handling of
960's condition processing, rest, food, day rollover, healing or general recovery.

Content contract 1 owns one nonzero-u32 **combat seed for its admitted Skeleton
encounter**, retained in the world-owned Journey facts. Fresh entry samples it
once using Application's existing startup mechanism (`std::random_device`, with
sampled zero replaced by 1), unless `--combat-seed` supplies the validated value.
An explicit seed bypasses sampling. A pre-combat v4 save persists that exact
seed; restoration preserves it without sampling or advancing RNG.

Successful attachment initializes this single combat from the stored seed and
does not change the durable seed. Keep M27's actual xorshift32/rejection-sampling
behavior inside active combat, including candidate cursor adoption only with
publication. Its cursor/draw position remains transient because active combat
is unsaveable. After guarded retirement the stored seed stays exact but is inert
for content contract 1. Thus seed 56 remains 56 before attachment, after
retirement and across every save/restart.

Redraw, inventory, capture, restore/preflight and post-victory navigation consume
no combat RNG. M29 defines no seed succession for hypothetical later encounters.
M30 may define a broader seeding/RNG policy only after its route evidence.
Diagnostic seed behavior is unchanged.

An eligible F9 requires a journey-valid graph with no combat/attachment/End/
retirement, pending approach count 0, no unserviced classify/engagement work, no
dispatch/event/reward/modal/selection/confirmation/save operation, no unresolved
frame lease, no integrity/fatal/shutdown state, and a current presented frame.
Before victory the target must be Present, HP 20, unaccounted and not
at the camera; after victory it must be Defeated/accounted and the runtime must
have completed guarded retirement into quiescent Journey, or have been freshly
bound from validated post-retirement v4 state. Restore does not fabricate an End
ticket to satisfy this boundary.
Melee readiness is not required to preserve a quiet rearrangeable inventory.

F9 cannot drain pulses, force End, close inventory, retire combat, acknowledge a
frame or queue a later save. Refusal occurs before capture/providers/path work;
the user must issue a fresh F9 after the boundary becomes eligible. A label,
idle-looking frame or absent coordinator alone establishes none of these facts.

## Save representation and compatibility

### Version decision

Introduce **v4 exclusively for Journey**, retaining the v2 base payload exactly
and appending a separately discriminated journey extension. V3 remains the exact
M28 completed layout and semantics. Ordinary eligible saves still write v2;
completed Diagnostic27 writes v3; admitted journey saves write v4. Reader accepts
supported v1/v2/v3/v4. Never encode journey fields in v2/v3, and reject a snapshot
containing both completed and journey extensions.

Keep the existing magic/envelope, little-endian encoding, 4 MiB bound, payload
length and CRC32, including the extension. Archive compatibility still compares
Clouds length/CRC and Dark presence/length/CRC; Journey requires the matching
World of Xeen archives. No installation path or commercial resource payload is
serialized. Existing valid supported targets may be replaced after structural
decode; unknown/malformed targets remain protected.

### V4 extension schema 1

Immediately after the v2 disabled-event list, encode fields in this order:

| Field | Wire representation and requirement |
| --- | --- |
| Domain | u8 = 3 (Journey); distinct from existing entry values 0/1/2. |
| Schema | u16 = 1. |
| Content contract | u16 = 1 (Clouds map-20 four-cell Skeleton journey). Unknown contracts reject. |
| Context presence | u8 = 1, mandatory. |
| Context | u8 profile, u8 difficulty; u16 ctr24/day/year/minutes; nine u8 effects; six u16 light/resistance values; u8 rested/newDay. Same field meanings as M28, with journey minute bounds. |
| Supplement count | u8 = 30, mandatory complete presence. |
| Supplements | Thirty records in owner order 0..29: u8 owner, seven LE i32 Might/Speed/Accuracy permanent/temporary and temporary AC values, then u32 XP (33 bytes each). |
| Skeleton combat seed | u32 nonzero; retained unchanged for content contract 1. |
| Initialized map | u8 side = Clouds/0, u16 map = 20. |
| Original actor count | u16 = 27 for this content contract. |
| Live record count | u16 = 1 for this content contract. Structural bound 1..107 before allocation; domain validation requires 1. |
| Live records | Sorted unique identity (u8 side/u16 map/u32 original record index), i16 x/y, i32 HP, u8 activated, u8 lifecycle, u8 status, u8 accounted (19 bytes per record). |

Measured from extension start B, the exact offsets are:

| Offset from B | Field | Bytes |
| --- | --- | --- |
| 0 | Domain | 1 |
| 1..2 | Schema | 2 |
| 3..4 | Content contract | 2 |
| 5 | Context presence | 1 |
| 6..38 | Context | 33 |
| 39 | Supplement count | 1 |
| 40..1029 | Thirty supplements | 990 |
| 1030..1033 | Skeleton combat seed | 4 |
| 1034..1036 | Initialized map | 3 |
| 1037..1038 | Original actor count | 2 |
| 1039..1040 | Live record count | 2 |
| 1041..1059 | One live record | 19 |

Context is `2 + 4*2 + 9 + 6*2 + 2 = 33` bytes; each supplement is
`1 + 7*4 + 4 = 33` bytes. A live record is `7 + 2*2 + 4 + 4 = 19` bytes:
identity at record offsets 0..6, x at 7..8, y at 9..10, HP at 11..14,
activation/lifecycle/status/accounted at 15/16/17/18. The suffix is therefore
exactly **1060 bytes**: `6 + 33 + 1 + 30*33 + 4 + 3 + 2 + 2 + 19`.
Require exact EOF at B+1060, not trailing padding or optional unknown data.
Use explicit wire enum mappings, independent of compiler enum layout:
lifecycle Present=0/Disabled=1/Unresolved=2/Defeated=3;
status Physical=0/Unsupported=1. Boolean bytes must be 0/1.

Structural validation checks counts, lengths, overflow before allocation,
identity ordering/uniqueness, enum/boolean encodings, i16 coordinates in
`[-128,31]`, HP in `[0,65535]`, supplement input bounds `[0,255]`, complete item
state and mutually exclusive domains. Base fields retain existing limits.
Domain/resource validation further requires:

- Exactly the declared map, 27 original records, original target metadata/type
  and admitted MON combat statistics, four-cell terrain/event/isolation proof.
- Live record identity exactly Clouds/20/5. Present requires HP 20, x/y inside
  the footprint, activated, Physical, accounted=false,
  and not the saved camera cell. Defeated requires HP 0, `(-128,-128)`, inactive,
  Physical, accounted=true. This Defeated/accounted record is admitted only as a
  quiescent post-retirement save, never as suspended lethal-but-End-incomplete
  work. There is no injured-live or incomplete-End v4 representation.
- Fresh North entry activates the target; admitted approach retains activation
  until defeat. A Present inactive target is outside content contract 1. Restore
  validates and preserves the saved activation without inferring or replaying it.
- All omitted actors are verified unchanged at capture and reconstructed from
  the matching original records at restore. No other mutable actor may be
  silently omitted. Per-identity accounting derives from the explicit live
  record, never from XP or initial actor absence. No actor End set is stored.
- Journey-valid party, complete 30-owner supplement presence, declared context,
  camera and independent overlay validation. Do not require current base fields,
  XP, inventory or inactive owners to equal initial CHR. No initial XP-plus-award
  formula is a restore condition.

First/effective serialized membership counts are fixed at 6 by this contract and
checked on capture; they need no extra wire bytes. Loader diagnostics remain
non-gameplay metadata. Runtime activity/revisions, pending work, capabilities,
combat RNG/candidates, UI/cosmetic state, maps/MOB/MON payloads and caches are
not serialized. Save eligibility proves their required quiescent absence.

### Capture, preflight, write and restoration

Reuse Application's checked operation sequence: early domain/Flow refusal,
callback-free capture with a retained source guard, exclusive save lease,
detached restore/preflight, retained source/UI checks after every callback and
exception, and final source check immediately before `XeenSaveFile::write`.
Extend the guard to context, all supplement presence/values, complete actors,
accounting, seed, transient End authority and activity guards. Do not reacquire a
newer source preimage to legitimize an older snapshot. Nested callbacks cannot write, advance combat,
replace owners or consume another F9. Expected own lease transitions are
explicitly admitted; owner mutations are not.

Preserve existing checked directory identity/path-alias protection, installation
containment refusal, sibling temporary creation, full write/flush/close,
protected replacement and old-file preservation on handled failures. Encode and
preflight failures occur before destination replacement. Failed observers do not
authorize stale writes. Do not broaden crash/concurrent-writer/network guarantees.

Journey restore is startup-only into fresh, unborrowed destinations. Its order is:

1. Decode structural shape and version; verify archive identity and required
   providers. Retain destination identity/value/cache guards before callbacks.
2. Create unpublished party/world/camera/flag candidates. Install exact base
   saved fields, context, all supplements and the Skeleton combat seed. Resources
   supply only immutable metadata; v4 has no missing mutable fields to initialize.
3. Validate/reconstruct compatible map/MOB/MON/EVT and independent disabled
   overlays; construct all original actor records and apply the one live record.
   Validate resource isolation, durable lifecycle and current journey party.
4. Compose detached first-frame preflight using saved camera and live actors,
   without activation, action, event dispatch or combat construction. Retain
   candidate preimages at intentional preparation phases and check candidates
   and destinations after every provider, nested callback and exception.
5. Publish all prepared values through a private nonthrowing path, rebind fresh
   owners and pending-zero transient approach coordination with fresh activity
   guards, then present the real startup frame under its lease. Gameplay becomes
   available only after matching handoff.

No candidate incarnation, old pointer, ticket, callback or modal generation crosses
publication. Provider return storage must be detached from aliases. Failure
publishes none of the candidate; externally introduced mutation is detected,
never rolled back or hidden. Invalid restoration fails startup without silently
starting fresh. Do not run initial events, approach, combat, damage, XP,
transfer/equip or seed advancement to reconstruct a save.

Plain v1/v2 loads remain ordinary and write v2 on explicit save. V1's exact
missing-ID/miscellaneous policy remains intact; v2 explicit empties win. V3
continues to restore terminal completed Diagnostic27 with exactly six supplements
and its canonical defeated overlay, read-only inspection and bounded R. It
cannot become Journey by version absence, a CLI seed or a mutable Flow adapter.
Neither v1/v2 nor v3 has an explicit or implicit Journey conversion path in M29.
Only fresh Journey entry and direct v4 restoration create this domain.
No in-session load, suspended input, automatic upgrade/rewrite or broad migration
framework is added.

## Validation and acceptance contract

Expected values must come from literal independent cases, original resource
fields plus explicitly checked action deltas, or pinned-reference arithmetic.
Capture/encode/decode/restore round trips alone are insufficient. Do not build an
expected graph by calling the new actor initializer, capture or restore and then
compare it to itself. Existing M27/M28 replay wrapping is a starting point;
extend coverage to the new attachment and journey initialization seams.

### Required automated/domain cases

| Boundary | Concrete evidence |
| --- | --- |
| Initialization/presence | Ordinary remains without context/supplements; fresh Journey installs 30 exact owner records; present zero XP is not absence; v4 partial presence refuses. V1/v2/v3 loading retains original domains with no conversion or automatic rewrite. Compare every saved base/supplement/context field across v4 restore, including inactive owners and quest/game state. |
| Mutable pre-combat admission | Transfer/equip before any combat object exists; current loadout determines initiative/AC/dice. Nonzero XP, altered SP, carried rewards, holes and empty metadata survive. Test unsupported equipped contributions refusing before action publication, then legal removal and successful admission. |
| Historical injuries | Good positive HP above changed max; Unconscious HP <= 0 with max raised/lowered by modeled HP accessory; simultaneous Unconscious/Dead; exact negative SP. Item operations never heal, clamp or reclassify. Test unsupported Poison/Disease/etc., malformed condition bytes/sign combinations and all-unable refusal. |
| Equipment/raw consumers | Broken armor after removal, transfer and re-equip; unchanged state bits/slots; bare melee; bow ignored for dice; unknown unequipped/misc records preserved; ID-zero frame/material records exercised through actual scan predicates. The focused HP-accessory history control does not widen combat admission. |
| Progression composition | Nonzero XP on all six owners, Unconscious eligible and Dead ineligible; separate distinct-identity awards retain prior values/injuries; repeat same-identity award refuses; near-u32 overflow produces no partial lethal/XP publication. No additional original-content coverage claimed. |
| Attachment/End/retirement | Same live owners through the entire chain; no initial party replacement. Stop after lethal but before End, fail End at 959, retire exactly once only after busy unwinds; successful return allows real transfer/equip/navigation and a fresh capture. Retained old tickets cannot act. |
| Actors/reconstruction | Original 64 approach controls plus every four-cell/facing view after defeat; all 27 actors compared field by field. Save a moved Present anchor at quiet separation from camera, reconstruct caches, restart, then engage from the saved position. Changed/missing/reordered bystanders refuse rather than reset. Journey true-entry/R/map-change requests refuse. |
| Timing/modal/save | Exact action versus pulse counts, old pending flush, rapid Wait, delayed idle without backlog, zero-time item actions, cosmetic-only frames, pending=1/2/3 F9/modal/direct-capture refusal through the activity guard, no work drained by save, 950 charge and 959 Round/End boundaries. Fresh transient binding does not replay work; adapter destruction cannot manufacture eligibility. |
| Format/domain | Literal v4 suffix offsets/counts and independent disk-byte assertions; truncated/extra suffix, CRC/length failure, unknown domain/schema, duplicate owners/identities, invalid coordinates/booleans, absent context, incoherent defeat/accounting and incompatible archives/resources. Lethal-but-End-incomplete capture refuses; no per-actor End byte is encoded. Structural-valid/domain-invalid controls are separate. |
| Single-encounter seed | Explicit seed 56 bypasses sampling and stays 56 through pre-combat save, attachment, retirement and restart. Default startup samples once with existing zero fallback; v4 restore/preflight samples nothing. Active random cursor remains transient and M27 draw behavior is unchanged. No future-encounter seed sequence is tested or exposed. |
| Callback authority | Existing M28 matrix adapted to every journey provider/observer/compose seam: exceptions, exact-value replacement, ABA destruction, stale nested calls, altered candidate/source supplement/context/actor/flag/cache, reentrant F9/input and exceptions after mutation. Neither preimage nor callback can install newer authority. |
| Presentation/failure | Lethal/End/retirement facts survive report/composition failure; no gameplay/save until authorized frame handoff. Wrong lease cannot clear current work; Flow destruction, upload failure and fatal/shutdown never open saving. Stale combat Space cannot dispatch a returned event. |
| File behavior | Failed detached preflight and write/short-write/flush/close/replace preserve prior valid bytes; malformed/unknown targets protected; path and alias checks unchanged. Refused F9 invokes neither provider nor I/O. |
| No replay/compatibility | Repeated pre/post journey save-load-save in separate owners/processes; replay probes observe zero initialization-as-gameplay, combat, damage, XP, event, transfer/equipment or seed advancement during v4 load/preflight. Unchanged ordinary v1/v2 and Diagnostic26/27/M28 tests, formats, copy restrictions and completed R. |

The moved-anchor pre-encounter control uses the existing East trace: turn Right,
move to `(14,1)`, let the delayed opportunity finish, leaving actor `(13,1)`
and camera `(14,1)` East at minute 490/ctr24 2. Save at pending=0; restart with
those exact coordinates/activation, then Wait to engage at minute 500/ctr24 3.
Derive its combat expectations separately; the connected seed-56 trace above
cannot be copied to this different time/prestate without adjustment.

Concrete synthetic controls include a +4 HP accessory (material 105) on a
base-max-12 owner: max 16 and HP -13/Unconscious=1 can become max 12 after Remove
while HP -13/Unconscious=1/Dead=0 stay exact, even though the new max-plus-HP is
negative. Keep current SP exact too; no additional SP-modifier combat control is
required. This Journey-valid fixture is not melee-ready while that unsupported
accessory is equipped; removal permits a separate current-state admission check
without reclassifying the retained injury.
For XP, start every owner at 1000 with Rebecca Dead and the other five eligible:
base 250 and permanent level 1 yield 1100 on those five, Rebecca 1000; a distinct
synthetic target yields 1200 on those five without changing Rebecca or previous
accounting. Use a separate permanent-level-15 arithmetic control for the existing
non-doubled branch. These are domain oracles, not extra original encounters.

Preservation/composition controls should obtain Root/Q2, game-flag changes,
disabled objects/events and carried `{10,37,1,0}` rewards through existing
supported event/reward operations in controlled domain fixtures, then compare
exact values after combat/retirement/restart. These fixtures establish current
Journey owners directly for domain testing, not through a production conversion
interface or saved-file migration. Keep the admitted footprint event-free.
Also cover independent combinations of those
categories without assuming one quest flag implies a particular inventory.

### Original-data, process and physical acceptance

Run the connected production sequence above with original resources and real
controls, including genuine process exit and a new `--load-game` process.
Use the existing inventory browser and readable journey observation output to
inspect exact owners, raw physical item slots, HP/SP/all condition bytes,
supplements/XP, camera, context, actor identity/live state, accounting and
save eligibility. Extend that observation through existing facilities; do not
create another diagnostic entry framework. Emit observations at startup, item
inspection, successful combat return and restore so the comparison does not
depend only on a victory caption or image.

Automated original-data producer/consumer checks must compare all 30 base
characters and supplements, 144 item bytes per owner, all 27 actor metadata/live
records, complete flags/quests/overlays and context/seed. Independently inspect
wire bytes. Include the moved-anchor pre-save, repeated post-save restart and
fresh-session controls. Test actual CLI argument routing and SDL generation/
handoff behavior, beyond injected callbacks alone.

Closure requires an appropriate build, complete CTest suite, original-data/process
evidence, independent implementation review and maintainer-performed physical
acceptance of the connected seed-56 sequence and fresh control. Automated/domain
evidence, original-resource/process evidence, independent review and physical
acceptance are separate claims. This planning task performs none of that future
M29 acceptance. A failure or scope contradiction is corrected or explicitly
replanned; it is not waived by successful codec tests.

## Proposed implementation boundaries

These are independently testable boundaries, not separate speculative designs.
Each needs explicit implementation authorization; completing one does not
authorize the next.

| Substage | Scope and dependencies | Acceptance boundary and exclusions |
| --- | --- | --- |
| M29A - Journey domain and encounter lifetime | Implement explicit domain/owner fields, complete supplements/presence, detached-value audit, current-state admission, persistent context/actor consequences, per-identity accounting, transient approach coordination with world-visible activity guards, attachment/End/retirement and the single Skeleton seed. Depends on this approved contract and M26-M28. | Domain tests demonstrate mutation before attachment, retained injuries/nonzero XP, genuine End and retirement followed by mutation, moved actors/cache reconstruction and stale authority refusal. No production CLI unlock, future RNG succession or save-format claim yet. |
| M29B - Durable journey capture and restoration | Implement v4, fresh guarded restore, save eligibility/source authorization and failure protection. Depends on the domain boundary. | Independent wire/owner comparisons, complete fault/replay/compatibility matrix and repeated detached/process restoration. No legacy-domain conversion, mid-combat save or new original route. |
| M29C - Connected production journey | Wire bounded fresh entry/v4 load, existing Flow inventory/navigation/events, automatic attachment, return frame handoff, observation and SDL/CLI controls. Depends on both prior boundaries. | Connected original seed-56 and moved-anchor save/restart controls, fresh controls, full regression suite, independent review and maintainer physical acceptance. No additional content or recovery. |

At eventual closure, follow [AGENTS.md](../AGENTS.md): only after required review
and acceptance update stable status/history, condense this plan, and remove
completed roadmap scope. README changes then describe the implemented public
entry and save behavior. This candidate changes none of those durable records.

## Exclusions and M30 handoff

Exclude arbitrary map-20 movement, map transitions/true journey revisit,
normal Vertigo startup, travel to or collection of the Bone Whistle, additional
physical encounters/species, Zombie/Disease, wider grouping, ranged/spell
combat, Run/disengagement, recovery/rest/food/healing, loot expansion, item use,
shops/services, leveling/training, recruitment/reordering, general world clocks
or RNG, arbitrary mid-combat saves, in-session load and Darkside gameplay.

The reusable M30 seam is current-owner admission -> identity-bound combat borrow
-> per-identity lethal accounting -> successful End -> guarded mutable return,
with context/progression and keyed live actor records preserved across saves.
It permits a later reviewed content contract to account for more influencing
actors without rebuilding party/world ownership. The v4 structural record seam
is bounded, but contract 1 still admits exactly record 5; changing that content
contract requires new validation/evidence and compatibility discrimination.

After accepted M29 closure, verify its committed baseline and perform the
roadmap's route-evidence gate before detailed M30 planning. Recovery/Run,
Zombie/Disease, grouping and Bone Whistle suitability remain questions for that
future investigation. Broader encounter seeding and group-completion semantics
also require that evidence; M29 prescribes neither. No such route work is
performed here.

No unresolved implementation-blocking alternatives are intentionally deferred.
This revised candidate still requires architecture/specification and maintainer
approval. It authorizes no implementation, commit or broader expedition scope.
