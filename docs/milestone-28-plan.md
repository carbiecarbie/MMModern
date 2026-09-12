# Milestone 28 - Durable bounded encounter completion and revisit

**Planning verdict: READY FOR REVIEW. Implementation is not authorized.**

## Baseline, objective and scope

The architecture/planning investigation used local commit
`f63bc3d0e577b2310a9e2ae86a5797387fa3d108`, the accepted M27 closure. Before
investigation, branch `main`, HEAD, `origin/main` and direct remote
`refs/heads/main` all matched that SHA; the working tree and index were empty.
The focused revision reverified the same four Git identities, an empty index,
and only this untracked plan as the permitted working-tree difference. No fetch
or baseline repair was performed.

This plan specifies the immediate successor in the [roadmap](roadmap.md#m28---durable-bounded-encounter-completion-and-revisit).
The [stable status](project-status.md) and complete [closed M27 contract](milestone-27-plan.md)
remain the baseline, not descriptions of M28 implementation. Planning approval,
implementation authorization and final acceptance are separate decisions.

The required result is a genuine supported Skeleton Victory, successful End
publication, explicit retirement into a quiescent completed encounter, production
F9 save, producer process exit, separate-process resume into fresh owners, and
bounded true re-entry with record 5 still defeated and XP awarded exactly once.
Current HP/SP, simultaneous conditions, progression and all physical item slots
must remain exact throughout. A visually empty scene is insufficient evidence.

Retain Diagnostic27, World of Xeen Clouds/Adventurer, map 20, all 27 original
monster records, selected original record 5/type 8, original placement `(13,2)`,
entry camera `(13,1)` North and the four-cell M27 envelope. Only record 5 is an
admitted combatant. Completed sessions remain a bounded diagnostic, with no
route back to combat or ordinary exploration. Ordinary sessions and Diagnostic26
keep their current behavior, except shared persistence safeguards strengthened
where necessary to preserve the same ownership guarantees.

## Focused evidence and design consequences

Repository links below refer to files inspected at the verified baseline.
Symbols identify the relevant boundary; future edits must check the actual code.

| Evidence | Consequence for M28 |
| --- | --- |
| [XeenCombat.cpp](../src/games/xeen/XeenCombat.cpp), `Impl`, `service`, `current`, `fail`, constructor/destructor | `_combatAccounted` is set with lethal removal/XP. `phase=Victory` and `work=None` are published only by successful End, with its minute increment. That phase lives in `Impl`, not the world. The default destructor does not establish completion or clear world pointers. |
| [XeenWorld.h](../src/games/xeen/XeenWorld.h), `XeenSessionWorldState` | `_encounterTerminal` means approach ended, including engagement or stop; `_combatEntered` means handoff consumed. Neither proves Victory. `_combatOwner` and `_combatApproachState` are borrowed runtime authorization, not durable outcome records. |
| [XeenActorApproach.cpp](../src/games/xeen/XeenActorApproach.cpp), `actorsFromResources`, `initialize`, `validateDomain`; [XeenActor.h](../src/games/xeen/XeenActor.h) | Original metadata and current actor values are distinct. Initialization constructs every original record. Cache eviction does not call initialization. The Good-only domain validator cannot restore injured victors. |
| [XeenParty.h](../src/games/xeen/XeenParty.h), [XeenParty.cpp](../src/games/xeen/XeenParty.cpp), marked-roster copy/move/swap guards | Supplemental inputs belong to the roster. Detaching it from its original world cannot remove the persistence prohibition. Ordinary swaps exchange neither the supplement nor its marker. |
| [XeenSaveSnapshot.h](../src/games/xeen/XeenSaveSnapshot.h), [XeenSaveState.cpp](../src/games/xeen/XeenSaveState.cpp) | The snapshot omits encounter/context/supplemental data. Capture rejects any world marker, context or marked roster. Restore guards destination and candidate after providers; ordinary publication intentionally omits encounter authority. |
| [XeenSaveFormat.cpp](../src/formats/xeen/XeenSaveFormat.cpp), `readCharacter`, `encode`, `decode`, `validate` | Existing version branches explicitly use `version == 2` for IDs/miscellaneous. A version-constant edit alone would misread v3. Ordinary aliases and opaque item bytes are valid storage. |
| [XeenSaveFile.cpp](../src/platform/XeenSaveFile.cpp), `resolve`, `read`, `write` | Existing-target replacement uses structural decode, not resource compatibility with the new session. Encode precedes destination I/O. Checked directory identity, sibling temporary, flush/close and protected replacement already exist. |
| [XeenGameplay.cpp](../src/app/XeenGameplay.cpp), `Application::playGameplay`; [Application.cpp](../src/app/Application.cpp), `encounter27`, `gameplay` | Startup rejects explicit encounter+resume; Application rejects encounter save targets. F9 unconditionally refuses encounter authority before capture, preflight and write. Resume currently composes ordinary gameplay. |
| [XeenEncounterFlow.cpp](../src/app/XeenEncounterFlow.cpp), `current`, `fail`, `acceptCombatResult`, `scheduleCombat`; [XeenEventFlow.cpp](../src/app/XeenEventFlow.cpp), rendering, dispatch and initial routes | Flow retains combat results, a boundary lease owner and frame/input generations. `blocksGameplay()` includes the mere existence of an encounter, so it is not the new save predicate. Presentation failure is separately latched. |
| [main.cpp](../src/main.cpp), strict encounter parser; [SdlWindow.cpp](../src/platform/sdl/SdlWindow.cpp), action mapping and poll-batch tickets | Existing `--encounter-27` cannot be combined with `--save-file`. New completion/revisit inputs must use the existing one-loop, repeat and generation protection. |
| [XeenPartyLoader.cpp](../src/games/xeen/XeenPartyLoader.cpp), [XeenCharacterFormat.cpp](../src/formats/xeen/XeenCharacterFormat.cpp), `parseCombatInputs`; [XeenGameplayContextFormat.cpp](../src/formats/xeen/XeenGameplayContextFormat.cpp) | Ordinary initial loading does not attach combat inputs/context. CHR supplies seven unsigned-byte inputs and u32 XP; PTY supplies context. Saved earned XP must override initial XP. |

Persistence inheritance is limited to the [M20 wire contract](milestone-20-plan.md#4-concrete-format-mmmodern-clouds-save-v1)
and [M21 v1/v2 policy](milestone-21-plan.md#save-v2-and-legacy-v1-compatibility).
M27 already settles formulas, traces, identity, removal coordinates, appearance
and the [map-load versus cache HP policy](milestone-27-plan.md#victory-defeat-xp-and-persistence-safety).
The focused revision rechecked actor initialization, activation, approach and
combat publications, presentation recovery and synchronous save callbacks.
No unresolved reference question required inspecting commercial payloads or
rescanning ScummVM. The inherited pin/configuration authority remains
[dependencies](dependencies.md); this plan changes neither dependencies nor
resource files. No claim here comes from a newly executed original-data fight.

### The missing durable boundary

The baseline has no world-owned proof that End succeeded. At minute 959 a lethal
action legitimately removes record 5 and awards XP, then End refuses 960 and the
session becomes SupportStopped. Therefore absence, HP zero, accounting, the
approach terminal latch, coordinator destruction and a Victory-looking frame
must never confer save authority.

Add a small private completion record to `XeenSessionWorldState`, with runtime
states **None**, **VictoryEnded**, **VictoryQuiescent**. These are new names for
this specification, not claims about existing symbols. Only successful combat
End can publish VictoryEnded, in the same nonthrowing publication as the final
minute, combat Victory and no pending End. It records the selected original
monster identity and the already-consumed accounting fact. It does not award XP.
Before successful End, every other exit leaves completion None. Subsequent
failure preserves the completed outcome while revoking save eligibility as
specified below. Do not generalize this into an outcome ledger or a persistent
combat state machine.

After `service` has returned and its busy guard has unwound, a ticket-guarded
`XeenCombat::retireCompletedVictory` operation consumes a current Victory ticket.
It mutates world completion only through private access and requires
the same world, party, roster, camera and boundary owner, exact accepted live
preimages, VictoryEnded, no candidate, `work=None`, no pending approach/handoff,
and a quiet boundary. Flow calls it only after adopting the End result, with
inventory/certificates/event/reward work absent. A direct headless caller must
use this same operation and owner-bound boundary; a boolean assertion of quiet
state or a copied result is not authority.

Retirement preallocates any necessary values, rechecks all retained authorization,
then nonthrowingly publishes VictoryQuiescent and a new world revision. It
invalidates combat/approach tickets and clears the retired world coordinator
pointers/check callbacks through this authorized transition. The coordinator
becomes inert before it may be destroyed. Neither destruction nor clearing any
marker substitutes for retirement. Preserve `_entry=Diagnostic27`, encounter
marked/initialized/terminal, combat-entered and accounting; preserve the roster
marker, all supplemental values, context and every gameplay effect. Do not turn
this graph into an ordinary graph.

Completed authority is bound privately to the actual `XeenPartyState`, its roster
and committed camera owner for this world. Runtime bindings and a new revision
are rebuilt on restore, never deserialized. Public setters/copies/swaps cannot
install or move completion authority. Reusing a completed world with another
party/camera, a marked roster with an ordinary world, or a matching-looking
snapshot/result cannot satisfy the predicate. Ordinary world overlay restoration
continues to reject marked worlds. Marked public copy/move/swap protections remain.

Flow retains a completed presentation adapter, not an active combat coordinator.
It obtains fresh world-bound completed tickets; `current`, frame handoff and
failure routing explicitly handle this mode. It cannot fall through to approach
when `_combat` disappears. Historical Victory and XP survive feedback failure.
Separate irreversible integrity failure from recoverable presentation work as
specified below; coordinator/presentation disposal alone clears neither guard.
Already-written valid saves remain valid. Startup/re-entry/preflight failures
never manufacture a completed graph.

### Minimum durable actor representation

Persist the **completed/accounted original monster identity**, with no actor
array or live actor fields in the snapshot/wire extension. This is sufficient
for the admitted completed boundary, for concrete reasons in the current code:

- `XeenActorApproach::actorsFromResources` creates every original record in order,
  with resource-derived original metadata, coordinates, statistics presence/base
  HP and lifecycle; activation starts false and status Physical.
- `validateDomain` checks every non-record-5 actor at its original coordinates
  and excludes it from both the activation union over all four cells/facings
  and the movement scan. Approach activates only the classified union and moves
  only activated relevant actors. Thus no bystander field changes in this domain.
- `XeenCombat::Impl::exact` compares every retained actor field and metadata;
  round publication rejects any difference between the movement candidate and
  retained actors. Combat damage changes only record 5.
- Successful lethal publication puts record 5 at HP0, coordinates `(-128,-128)`,
  inactive and Defeated, retaining Physical status and immutable metadata.
  M28 requires successful End and retirement after this publication. No wounded,
  living selected actor or additional defeated identity is saveable.

Consequently no authoritative live actor category can vary at an admitted M28
save boundary beyond the explicit completion fact. Guarded restore reconstructs
all 27 records from compatible MOB/MON and applies the canonical defeated overlay
to the accounted identity. This materializes state; it never executes a lethal
action or grants XP. Preserve Disabled/Unresolved bystanders according to resource
initialization, not by treating them as kills. Saved camera remains separate.

Retirement and capture must verify the entire live collection is exactly
reconstructible under this rule before omitting it. A moved, injured, activated,
retyped, missing or otherwise changed bystander, or a noncanonical target, is an
integrity failure, not information to silently discard. Use the retained admitted
metadata/preimages for the no-provider capture guard; detached resource preflight
independently checks compatibility with immutable sources. Cache reconstruction
must continue reading live actors during unsafe/living combat; it must never use
this completed-only reconstruction shortcut. If a legitimate completed state
violates these invariants, reopen the schema decision rather than losing it.

### Failure classification and authorized recovery

The baseline `XeenEventFlow::renderEncounter` already permits one reconstruction
attempt from live facts, suppresses repeat reporting on that attempt, and fails
fatally if recovery fails. `XeenCombat::fail` preserves terminal Victory. Current
`XeenEncounterFlow::fail` also holds PresentationFailure and latches `_failure`,
but all M27 states are unsaveable; that blanket latch is not evidence that a
completed M28 graph must be permanently unsaveable after a cosmetic exception.

| Failure/boundary | Owner and disposition |
| --- | --- |
| Current integrity/authority/preimage violation | World/session latches irreversible runtime unsafe authority. Preserve already-published facts; refuse capture permanently for this graph. Copying, Flow replacement or presentation recovery cannot clear it. A stale operation may only reject its own work; it cannot mark a newer graph unsafe. |
| Composition, text/reporting or frame-copy exception with unchanged completed facts | Hold a recoverable presentation lease bound to this graph and operation generation. Capture/F9 refuse while unresolved. Do not change completion, actors, party or accounting, and do not mark integrity unsafe merely because an exception occurred. |
| Successful authorized reconstruction | Recheck retained owners, completion revision and exact gameplay preimages after every callback; reconstruct presentation once from current completed facts, without replaying failed reporting or gameplay. Install a valid current frame and complete its normal handoff, then release only the matching presentation lease. Saving becomes eligible after dispatch unwinds and normal UI/quiescence guards pass. |
| Recovery failure, unverified handoff/upload, fatal Application or shutdown | Keep the graph unavailable for capture under the unresolved/fatal session guard and close the Application. No fallback, deferred save or successful recovery claim. Do not conflate this lifecycle failure with a changed gameplay outcome. |

Retain presentation leases with the graph's runtime boundary so discarding Flow
cannot remove an unresolved failure. A replacement presentation owner may clear
only a matching recoverable lease after the same guarded reconstruction and
handoff; it cannot clear integrity or fatal/closed-session state. Direct capture
checks these domain guards even without Flow. A headless graph has no UI lease
unless presentation work was actually attached. No such guard is serialized.
If a callback changes gameplay while throwing, classify the preimage violation
as integrity failure before considering presentation recovery. If it merely
invalidates the retained ticket, reject the stale operation without overwriting
newer authority. A failure at VictoryAwaitingEnd still has no completed Victory;
this recovery allowance never completes End or legitimizes its abandoned work.

## Ownership and field disposition

**S** means serialize exact authoritative value. **R** means reconstruct from
the same fingerprinted immutable resources and, for the selected actor, the
explicit durable completion fact. **T** means transient, with the
specified retirement/fresh-owner rule. The lifecycle table below distinguishes
live cache preservation from resource-based true entry; no admitted completed
actor value changes through that entry policy.

| Value and current source/owner | Disposition, identity and presence |
| --- | --- |
| `XeenActor::id` / world's original-order `_actors` | R all 27 identities, including disabled/unresolved records, from original MOB record order on Clouds map 20. S only the completed/accounted identity Clouds/20/5. Never sprite ID, table index, draw index or coordinate identity. |
| `XeenActor::original` (`XeenMapEntity::x,y,tableIndex,direction,resourceId`) | R from the corresponding unfiltered initial MOB record. These are immutable spawn metadata and resolved type, including absent-resource semantics. Record 5 must still be type 8 with spawn `(13,2)`. Do not replace these with saved live coordinates. |
| `XeenActor::statistics` presence and complete MON record | R by original resolved type through the existing monster parser. No MON bytes in the save. Preserve source absence for unresolved records, source base HP and other statistics; only record 5 requires combat admission. |
| `XeenActor::x,y` | R bystanders at original coordinates and record 5 at canonical `(-128,-128)` from completion. Live approach movement still survives caches, but has no successful M28 save representation. The target's last occupied cell is no longer retained and is not invented as a death position. Never restore it at spawn; camera is separate. |
| `XeenActor::hp` | R bystander HP from source initialization and selected HP0 from completion. All actual live HP remains world-owned, including partial combat HP through caches. True entry/process entry uses resource base HP for surviving Present actors; this equals every admitted completed bystander's existing HP. |
| `activated`, `status`, `lifecycle` | R initialized bystander values and inactive/Physical/Defeated target from completion. Verify all live values at capture. Do not reduce Disabled/Unresolved to Defeated; original absence is not an XP event. No activation pass or status processing on revisit. |
| `_combatAccounted`, new completion and selected identity / world | S as completed-Victory outcome plus explicit accounted identity/fact. The existing boolean has meaning only for the selected record; no additional defeated monster or reward accounting is admitted. Removal, accounting and completed outcome must agree. |
| `_entry`, `_diagnostic27`, encounter marked/initialized/terminal, `_combatEntered` / world | S entry kind; the remaining booleans are reconstructed as the mandatory completed-Diagnostic27 invariants of that entry/outcome. Legacy absence reconstructs none of them. None is independently writable authorization. |
| `XeenPartyState::encounterContext` presence | Required exactly for completed Diagnostic27; absent for ordinary/legacy. S every field below. It is never fabricated to promote a legacy file into an encounter. |
| `XeenGameplayContext::profile,difficulty` | S explicit WorldOfXeenClouds and Adventurer codes. Resource availability never selects rules. |
| Context `ctr24,day,year,minutes` | S all four u16s; retain earned minute and approach counter. Bounded profile day 1/year 610, `ctr24<24`, completed minutes 491..959. No restart/revisit time charge. |
| Context `effects[9]`, `lightAndResistances[6]`, `rested,newDay` | S in array order, including explicit zeros/false. Domain requires zero arrays and false booleans. Do not infer `newDay` afresh from saved minutes. |
| `XeenRoster::_combatInputs[30]` and marker | S exactly six owner-keyed records, independent of active position. Their fixed required identities establish presence on owners 0,1,6,11,14,18 and absence on all others; no arbitrary optional shape is accepted. Legacy/ordinary has no supplement. Reconstruct the irreversible marker only within the validated completed graph. |
| `XeenCombatInputs::might.permanent,temporary`; `speed.permanent,temporary`; `accuracy.permanent,temporary`; `temporaryAc` | S all seven integers even though immutable within M27. Resource/domain validation checks the initial CHR values; absent input is distinct from explicit zero. No second copy of existing INT/PER/END or levels. |
| `XeenCombatInputs::experience` | S exact u32 by original roster owner. Domain checks initial XP plus the one accepted allocation; this validation does not perform an award. Ineligible owners retain initial XP, not an absent supplement. |
| Every `XeenCharacter` field, all 30 owners | S with the existing v2 payload: ID/name/sex/race/class; INT/PER/END permanent/temporary; level/temp level/temp age; four max-stat skills/hasSpells; signed HP/SP; all 16 condition bytes; birth year. Include inactive owners and preserve simultaneous Unconscious/Dead. Derived maxima/AC are computed, never saved in place of inputs. |
| Four nine-slot item arrays / roster characters | S all 144 bytes per owner in physical category/slot order. Frame carries equipped state; state carries broken and other bits. Preserve holes, explicit empty-slot metadata and all opaque bytes. No compaction, re-equip, transfer replay, ID repair or healing on v2/v3 restore. |
| `XeenParty::activeRosterIds` | S order and aliases under ordinary rules. Completed encounter separately requires the unique M27 sequence. Do not impose that restriction on ordinary snapshots. |
| Quest counters/quest flags, Application camera/game flags, world disabled object/event sets | S unchanged existing categories and independent identities, including uncached maps. Monster defeat never adds ordinary Remove entries. Completion/revisit does not mutate quest state or flags. Camera remains saved exactly on process resume; explicit revisit alone repositions it. |
| Party loader counts/diagnostics; source map cells/geometry/tables, scripts and catalog data | R existing loading metadata/resources. `firstSerializedCount`/`effectiveSerializedCount` are not new progression fields; completed source must yield six/six. `seen`/`stepped` and other loaded geometry metadata are not newly mutable M28 state. |
| Combat admission/preimage copies, `Impl::expected`, inputs/actor preimages and fixed XP results | T validation evidence/observations, not owners. Before retirement they certify the live publications. Afterward use completed-domain validation and immutable resources; do not serialize a second character array or full combat transcript. |
| Combat phase beyond the completed outcome, RNG/cursor/tape, candidate/injury work, participant/order/turn, acted/blocked, selected target, pending work and approach countdown/reason | T. Required work must actually finish before retirement. Clear/invalidate only through retirement; none can be restored or replayed. Stop/failure phases have no save representation. |
| Revisions, tickets, integrity/fatal guards, recoverable presentation leases, Flow/display generations, owner pointers, providers and callbacks | T authorization. Consume old capabilities; generate fresh runtime binding/revision on restore or completed map entry. Unresolved guards refuse capture; only authorized presentation recovery clears its own lease. Pointer values never appear in the wire format. An old capability cannot target the new graph. |
| Notices, last damage/XP receipts, inventory selections/certificates, interpreter execution/rewards, dispatch and modal state | T. Pending authoritative work forbids retirement/save. Retained cosmetic observations can be discarded and rebuilt from completed live facts; reopening inspection creates no action certificate. |
| MON/ATT appearance/frame/step, ordinary/NPC animation phase/deadlines, occupancy, placements, sprites, composition/map/script/text caches and SDL resources | T/R. Cache reconstruction uses live authority; restart starts new presentation at phase zero with fresh timing. No cosmetic time or cache load advances combat, XP, activation or gameplay minutes. |

All authoritative values remain with world/session or party/roster. Snapshot
extensions and codec records are temporary transfer representations. The wire
representation does not become a second owner and cannot authorize gameplay.

## Save format and compatibility

### Version policy

Introduce **v3**, retaining the 20-byte envelope, 4 MiB maximum, little-endian
encoding, explicit signed conversion, CRC32 and existing identity/count limits.

| Session/input category | Read/restore policy | Writer |
| --- | --- | --- |
| Ordinary v1 | Existing missing-item-field policy only; no encounter/context/supplement | Eligible recapture writes v2 |
| Ordinary v2 | Exact complete items; no encounter/context/supplement | v2, byte layout unchanged |
| Completed Diagnostic27 v3 | Required complete extension below, fresh completed authority after resource validation | v3 |
| Diagnostic26 or unsafe Diagnostic27 | No successful capture or restoration representation | Refuse |
| Unknown version, v3 missing extension, partial completion | Reject, no fallback | Refuse |

V1 restores only missing weapon/armor/accessory IDs and the entire missing
miscellaneous arrays from matching initial roster slots. Saved triples and all
other saved values win. Unresolved v1 snapshots remain structurally decodable
for reads/existing-target checks, but cannot be encoded directly. V2/v3 explicit
empty values never acquire initial items. Legacy absence means an ordinary
session with no claimed encounter history, not a live or defeated Skeleton, not
zero XP attached to six owners, and not an implicit Diagnostic27 resume.

An M28 reader accepts v1/v2/v3. Older binaries continue to read newly written
ordinary v2 and reject v3. No automatic rewriting on read. A valid existing
v1/v2/v3 target can be replaced by either supported writer category after normal
new-snapshot preflight; existing-target decode is structural and does not require
that target's resource signature/session kind match the replacement. Malformed,
unknown-version or unsupported-extension targets remain protected. This is the
existing explicit-target replacement contract, not an in-session migration.

### Exact v3 extension

V3 contains the **complete v2 payload unchanged**, from archive signature through
the last disabled-event identity, followed immediately by this extension. There
is no alignment, padding, raw C++ layout, trailing data or extension directory.
Let `B` be the byte immediately after the existing disabled-event list. Offsets
below are relative to B. Enum wire codes are explicit, not native enum dumps.

| Offset/order | Representation and meaning |
| --- | --- |
| 0 | u8 extension-present, exactly 1. Zero is invalid in v3; v1/v2 encode absence by having no extension. |
| 1 | u8 entry kind, exactly 2 = Diagnostic27 (0 ordinary/1 Diagnostic26 are not v3 encounter values). |
| 2 | u8 completion, exactly 1 = completed Victory. No other combat phase is encoded. |
| 3 | u8 accounting-consumed, exactly 1. |
| 4..10 | Accounted/selected monster identity: u8 side = 0 (Clouds), u16 map = 20, u32 original record index = 5. This same identity identifies the unique defeated actor. |
| 11..12 | u8 profile = 0 (WorldOfXeenClouds), u8 difficulty = 0 (Adventurer). |
| 13..20 | Four u16s: ctr24, day, year, minutes. |
| 21..29 | Nine u8 effects in `effects` order. |
| 30..41 | Six u16 light/resistance values in `lightAndResistances` order. |
| 42..43 | u8 rested, u8 newDay, canonical booleans. |
| 44 | u8 supplemental owner count, exactly 6. |
| 45..242 | Six records, exactly 33 bytes each, in ascending original-owner order `[0,1,6,11,14,18]`. Each: u8 owner ID; seven i32s (Might permanent/temporary, Speed permanent/temporary, Accuracy permanent/temporary, temporary AC); u32 experience. No active-index addressing. |
| After 242 | End of payload. No actor count, actor records or optional per-owner payload follows. |

Record `k` begins at `45 + 33*k`: owner at +0, seven integers at +1,+5,+9,+13,
+17,+21,+25, XP at +29. Presence on the six owners is explicit through the
required ID sequence; absence on the other 24 owners is part of this bounded
format. Present zero XP is distinct from no supplemental record. Missing,
extra, duplicate, reordered or substituted owners are structural errors, not
alternate presence shapes. Ordinary membership aliases retain their v1/v2 rules.

Supplemental integers use i32 to match the accepted owner type, with structural
range 0..255 reflecting their unsigned-byte origin; resource/domain validation
requires the actual immutable CHR values. XP retains its u32 representation and
is checked against the single encounter allocation before restore. Context
booleans are canonical; the bounded profile requires zero effects/light arrays,
false rested/newDay, day1/year610, ctr24<24 and minutes491..959. Check these known
constraints structurally as well as on live capture; they need no resource reads.

The v3 extension is exactly **243 bytes**: 44 bytes through context + 1 owner-count
byte + 6*(1 owner ID + 7*4 input bytes + 4 XP bytes). The file's payload length
and CRC include this entire suffix. This independent size/offset oracle is
complemented by literal field fixtures. There is no broader actor-count or
supplement-presence wire domain reserved for hypothetical future milestones.

### Structural and resource validation

The codec validates size before allocation, checked remaining-length/count
arithmetic, exact EOF, envelope, CRC, the fixed 243-byte extension, discriminators,
bounded context/scalars and exact six-owner sequence. Require entry2/completion1/
accounting1 and the sole identity Clouds/20/5. Partial outcomes, other identities,
extra actor payload and absent v3 extension are rejected. Retain existing ordinary
field and disabled-object/event identity validation. Codec validation does not
invent resource mappings or apply combat item legality to ordinary item bytes.

Resource/domain validation additionally requires:

- Matching complete xeen.cc fingerprint and present matching dark.cc fingerprint;
  original geometry/MOB/EVT identity and the existing four-cell isolation rules.
  Exactly all 27 original monster identities, with no omissions/substitutions.
  Reconstruct original metadata/statistics by each index, not by sprite or camera.
  The selected type and supported resource contract must still match M27.
- Every bystander has exactly its initialized source position, HP, activation,
  lifecycle/status. Disabled/unresolved entries are retained if supplied by the
  accepted original list; do not require all retained records to be combat-capable.
  Apply HP0/Defeated/(-128,-128)/inactive/Physical only to record 5 from the explicit
  completion fact. Verify the full resulting collection before publication.
  Unexpected live bystander/target states are rejected by capture and re-entry,
  not silently reconstructed away. No such state is encoded in v3. Ordinary
  overlay identities are validated independently.
- Saved camera is Clouds map 20, x=13..14/y=1..2, any accepted facing. Context
  satisfies the bounds in the field table. The save may precede or follow an
  explicit revisit; neither needs a surviving combat coordinator.
- Membership is exactly the M27 owner sequence; six/six loader metadata;
  supplement present on precisely those owners. Seven immutable input fields
  equal parsed initial CHR values; saved XP is retained and checked, not loaded
  over from CHR. All inactive characters equal the immutable baseline in the
  admitted diagnostic, while ordinary inactive values retain their broader policy.
- Immutable active character inputs/SP remain as admitted by M27. HP lies from
  -23 through that owner's initial HP; condition bytes other than Unconscious
  and Dead remain zero. Good requires positive HP; Unconscious=1 without Dead
  requires HP<=0 and maxHP+HP>0; Dead=1 requires maxHP+HP<=0 and Unconscious 0
  or 1. At least one active owner can act. Preserve both bits when both were set.
  Use existing safe rule/max-stat calculations, not `validateOriginal` or the
  Good-only `XeenActorApproach::validateDomain` as a restored-party predicate.
- The occupied item multiset across the six owners conserves original items
  and categories after ignoring only permitted armor bit7 acquisition. All
  non-break bits/material/ID remain exact; breakage is limited to equipped Armor.
  Frames include both untouched original frames (including the original medal's
  frame 8) and supported M24/M25 results. Reuse/extract accepted preparation
  constraints; do not apply a new canonical-frame rule to legacy initial frames.
  Validate legal supported arrangements while preserving every physical slot
  and empty-slot byte. In this completed domain, empty slots remain the admitted
  zero records: reject invented ID-zero effects rather than normalizing them.
  This restriction never applies to ordinary v1/v2 storage. For occupied records,
  accept frame zero or the supported category/ID equipment frame and enforce
  existing class/proficiency, subtype, two-handed/shield and capacity rules.
  Preserve the original owner-1 medal's frame-8 exception at any physical slot
  reached by same-owner compaction; transfer to another owner resets it to zero
  and subsequent Equip uses the existing medal frame. No attempt to recreate
  the preparation action history or execute equipment operations during restore.
- Check XP against initial resource XP plus the single M27 award for the final
  eligible roster owners, using the settled division/doubling order. Final
  conditions are valid evidence of eligibility because nothing changes them
  after lethal publication in this scope. Ineligible owners receive no increment.
  This is consistency checking only; never call the mutating award path.

Live capture/retirement must retain exact preimage authorization, not just pass
these broad reachable-state checks. A file is untrusted transfer data checked
for structural/domain consistency, not cryptographic proof of its play history.
CRC32 and consistency checks do not authenticate edits. No save transcript,
signed certificate or general accounting framework is required.

Keep the current whole-archive signature unchanged: it already covers initial
MOB/CHR/PTY through xeen.cc and the Clouds MON source through dark.cc, as well as
the presentation resources. No new path-based signature, per-member manifest,
hash cache or resource survey is justified. Code/schema compatibility is the
version/domain contract; CRC32 remains accidental-corruption detection only.

Audit all version-specific branches: `kVersion` usage, header acceptance,
`readCharacter` ID and miscellaneous branches, writer selection, exact EOF,
legacy presence, encoded-size fixtures, protected-target decode and CLI tests
which currently use version 3 as their unknown-version example. Select explicit
v1 versus complete-item v2/v3 decoding only after the supported-version gate.

## Fresh-owner restoration and failure-safe publication

Extend `XeenSaveState::Resources` with narrowly typed immutable encounter
resource providers needed for CHR supplemental fields, PTY context validation
and MON records. Reuse map/object/event loaders and parser abstractions. These
providers must return resource values, not a pre-marked party or prepared combat
coordinator. Ordinary loading does not invoke encounter-only providers.

Restore remains startup-only. The destination must have no encounter authority,
context, marked supplement, Flow or borrowed runtime capabilities; even a valid
completed destination is not an in-session load target. The ordinary public
restore/swap paths keep that prohibition. Add a private publication path for a
fully prepared completed graph; do not relax `swapOrdinary` into a generic marker
exchange or make marked objects publicly movable.

Required order:

1. Validate structural transfer data and resource signature before providers.
   Own a stable local copy of transfer values across callbacks. Retain destination
   owner addresses, authority/revisions and relevant preimages before preparation.
2. Load an ordinary initial-party candidate solely for metadata, resource checks
   and v1 missing fields. Reject a provider that supplies context/marked inputs.
   Install exact saved characters/membership/quest state and flags/camera locally;
   validate safe existing character/portrait use. No initial events execute.
3. Prepare a separate candidate world with guarded resource providers. Validate
   ordinary overlay identities before encounter installation. For v3, reconstruct
   all original actors through the pure resource-to-actor constructor, then apply
   the canonical defeated overlay for the saved completed/accounted identity.
   Attach saved context and the six owner-keyed inputs, validate the completed
   domain and process-entry semantics below. The pure `actorsFromResources`
   conversion is permitted; combat/approach initialization, activation and
   lethal-action/XP publication are not.
4. Build candidate completed authority privately with candidate-local bindings
   and fresh revision. Candidate status is unpublished and cannot authorize
   public capture, actions or re-entry. Validate the complete graph, including
   expected marker/supplement/context/completion combinations and their bindings.
5. Preflight the actual completed presentation from those candidates, including
   required original sprites/scene/portraits and completed inspection values.
   Ordinary preflight stays ordinary. No fallback first frame is composed.
6. After every initial/CHR/PTY/MON/map/MOB/EVT provider and preflight callback,
   including nested providers, verify both destination and candidate graphs
   before another callback or mutation. Detect detached/mixed graphs, unexpected
   marker or lease changes, direct field changes, replacement/ABA through
   authorized mutation seams and stale revisions. Expected candidate marking is
   phase-specific, not a blanket exemption for any marked candidate. The guard
   must also run on exception paths before deciding how to report failure.
7. Prepare all allocations/results before a final guard. Publish characters,
   supplements/marker, context, membership, flags/camera, actor/overlay state and
   completed authority together through checked nonthrowing stores/swaps. Rebind
   completion to destination addresses, generate fresh capabilities, and never
   transfer candidate-capturing lambdas or temporary pointers. Keep loaders owned
   by the destination; restore any temporary guarded wrappers before lifetime ends.
8. Only then create completed Flow references and the first visible frame.
   Resume omits initial automatic events; completed mode also blocks later event
   dispatch because there is no supported exploration route. Ordinary resume's
   later navigation/event behavior is unchanged.

A handled failure publishes none of the candidate, including caches/metadata,
and starts no gameplay window. If an adversarial callback itself changed a
destination, preserve that externally published change rather than rolling it
back or concealing it; reject the old candidate immediately. Tests distinguish
this case from the unchanged-destination guarantee for ordinary provider errors.
Invalid/incompatible saves fail startup; there is no new-game fallback.

No restore path reapplies damage, XP, equipment/transfer actions or rewards,
heals/clamps characters, clears simultaneous conditions, fills explicit empty
items, or replaces earned XP with initial values. New runtime references are
the sole authority after publication; old pointers/tickets are never revived.

## Save eligibility and callback boundaries

Provide one read-only domain predicate for **ordinary** or **coherent completed
Diagnostic27** capture. Completed authorization uses world-owned VictoryQuiescent,
accounting/removal invariants, actual party/roster/camera binding, retained
context/supplement, fresh revision and no unsafe/session-operation latch. Both
Application and direct capture use it. The predicate does not drive work, load
resources or change markers. Ordinary capture still rejects any stray encounter
component. Restore uses the same completed-domain rules for candidates while
requiring fresh destinations, not a live capture permission.

| State/work | F9 and direct capture | Authoritative reason/owner |
| --- | --- | --- |
| Ordinary idle | Existing eligibility | No encounter world/context/marked roster; Application/Flow idle guards |
| Diagnostic26, including any terminal state | Refuse | No supported durable completion |
| Diagnostic27 Preparation or open preparation inventory | Refuse | Marked graph, completion None; inventory/certificate leases |
| Approach/Engaged/handoff | Refuse | Approach or handoff work; completion None |
| PlayerReady, PreparingAction, PendingEnemy, PendingRound | Refuse | Combat session still live, even when `work=None` in PlayerReady |
| VictoryAwaitingEnd | Refuse | Removal/XP can be final but required End has not succeeded |
| VictoryEnded before authorized retirement | Refuse | End succeeded but coordinator/boundary retirement is incomplete |
| VictoryQuiescent with retained cosmetic notices | Eligible with lifecycle/UI guards | Completed world proof, exact bound graph; notices carry no pending action |
| Defeat, SupportStopped, Failed; kill at unsupported time boundary | Refuse | No successful completed Victory; earlier effects do not confer eligibility |
| Dispatch/resource preflight/re-entry/save already in progress | Refuse reentrant requests | Retained operation guard/revision in domain plus Application/Flow dispatch |
| Pending event/reward/modal/inventory/transfer/equipment certificate | Refuse | Actual owning Flow/boundary lease; do not discard it to permit saving |
| Current authority/integrity/preimage failure | Permanently refuse for this graph | World/session integrity latch; disposal or presentation recovery cannot clear it |
| Recoverable completed presentation failure/reconstruction pending | Refuse until authorized recovery succeeds | Matching runtime presentation lease; unchanged published Victory remains authoritative |
| Successfully reconstructed completed presentation | Eligible with normal lifecycle/UI guards | Exact live preimages, valid current frame/handoff and release of matching lease after dispatch |
| Fatal recovery failure, shutdown/closed session, failed startup | Refuse | Runtime fatal guard and Application/Flow lifecycle; no new-game fallback |
| Completed snapshot restored into fresh owners | Eligible only after successful startup | New bound completed authority; no inherited coordinator or UI work |

F9 also requires configured target, successful startup, active Application,
current frame/handoff, no dispatch and no modal/inspection page. Keep exploration
blocked in completed mode; add a dedicated `canSave`/save-boundary query rather
than making `blocksGameplay()` false merely to pass the old save branch.

Unsafe F9 must return before target formatting/handling, capture/provider calls
and any disk/temporary-file I/O. It cannot service End, close inventory, retire
combat, acknowledge text or queue a save for later. Issue a fresh F9 after the
boundary is reached. Configuring/resolving a target and fingerprinting resources
at startup is permitted; it is not save I/O caused by an unsafe F9.

For an eligible save, retain bound owners, world revision, UI/boundary generation
and exact values before save preparation or its first observer. Keep that source
authorization through capture, candidate restore/preflight and write-stage
observers. Recheck after each actual `observeSaveStage`, composition/resource/
preflight callback, and immediately before calling `XeenSaveFile::write`. Do not
reacquire newer authority to legitimize an obsolete snapshot. The detached
preflight candidate cannot detect a changed live source by itself.

Keep the scoped dispatch/save operation guard through synchronous file writing.
The current single-threaded Application loop does not pump SDL or invoke gameplay
callbacks inside Windows file I/O; the production call supplies no file fault
callback. Therefore the final source recheck immediately before write is the
publication authorization boundary. Do not expand `XeenSaveFile` with gameplay
callbacks or intra-I/O authority machinery solely for fault-injection tests.
Existing file fault hooks continue testing file failures and preservation of old
bytes independently. Test changed-source refusal at the real save-stage and
resource/preflight callbacks. A future actual reentrancy/concurrency path would
require replanning this boundary. Post-publication feedback cannot replay the write.
An isolated save-candidate preflight or I/O failure is a failed save, not a new
live presentation failure: preserve live completed authority and permit a fresh
F9 when otherwise safe. A changed live graph retains its integrity guard;
failed live feedback/handoff follows the recoverable-presentation or fatal policy
above, according to whether authorized reconstruction succeeds.

Keep `XeenSaveFile` safety and its scope: existing parent, `.mmsave`, protected
unknown/unsupported target, device/stream/path restrictions, commercial-installation
containment including aliases, checked Windows directory identity, unique sibling
temporary, checked writes/flush/close and replacement. Preserve previous valid
bytes on handled write/replacement failures. Cleanup failure may leave its own
temporary and reports it. No broader concurrent-writer, network-filesystem,
running-resource-replacement, arbitrary-crash or power-loss guarantee is added.

## Lifecycle and bounded production revisit

| Boundary | Actor/world effects | Party/context and coordination |
| --- | --- | --- |
| Disposable cache reconstruction | Re-read immutable metadata/resources as needed; preserve all live coordinates, HP, activation/status/lifecycle and completion/accounting | No time, XP, injury or input changes. Existing live cosmetics can remain; cache miss/discard alone is not entry. |
| Explicit completed map re-entry | Validate the live completed collection, then construct a new active map-entry incarnation from compatible MOB/MON resources and the retained completed/accounted identity. Reconstruct all 27 records and apply the canonical defeated overlay. Surviving Present actors have resource base HP; bystander coordinates/status/lifecycle and target removal remain exact. | Reposition camera to map20 `(13,1)` North; preserve every party/context value, including minute/ctr24. Retire old completed tickets, create new ones, reset entry presentation. No action/approach/combat/events. |
| Process resume | Reconstruct all 27 records from compatible resources, apply the saved completed/accounted identity's canonical defeated overlay before first frame. No saved live actor array exists. Keep saved camera exactly. | Restore exact party/context/XP; new runtime bindings, generations and presentation. No initial events, RNG or coordinator. |
| Fresh new game | No prior completion/accounting/actor overlay. Ordinary new game follows existing rules; new Diagnostic27 goes through real Preparation/Begin with original actors. | Initial resources supply characters/XP/context at the accepted initialization points. It does not read a prior save. |

In accepted completed saves, bystanders were never injured or moved and have
base HP; the target is defeated. Consequently true-entry resource initialization
changes no legitimate completed actor value. M27's living-combat cache test proves
live wounded HP survives cache reconstruction; M28's entry tests independently
check resource base HP and canonical defeat. Do not introduce a general living
entry/reset operation or successful wounded save merely to demonstrate a numeric
HP difference. Unexpected wounded/moved bystanders in a completed live graph
must refuse capture/re-entry, rather than being silently healed by reconstruction.

Introduce one bounded domain operation, `reenterCompletedEncounter`, owned by
world/session and borrowing the bound party/camera under a completed boundary
lease. It accepts only a current, safe VictoryQuiescent graph. It prepares a new
map-entry candidate with fresh map-20 geometry/MOB caches from the loaders and
compatible MON resources plus the retained completion identity/overlay. Validate
the existing live collection first so reconstruction cannot conceal corruption.
It validates and preflights that candidate, then atomically replaces the active entry's map-20
caches, actor storage and camera and advances a nonserialized entry generation.
Unrelated ordinary overlays remain exact. This operation runs even if the old
map's caches were populated; a cache hit must not suppress the entry policy.
It deliberately ends one logical active-map incarnation and begins another,
even though both are map 20. That is the disclosed diagnostic true-load boundary;
`discardMapCache()` alone remains a different operation.

Allocation/resource/composition failure before publication leaves the old entry
and camera intact and cannot reset accounting. Apply retained graph/preimage
checks after callbacks. A nonmutating candidate/preflight failure leaves the live
completed graph eligible once the operation unwinds; current integrity failures
latch unsafe authority. Post-publication presentation failure follows the matching
lease/recovery/fatal policy above without undoing the published entry.
Only successful publication reports a typed MapReentered result containing old/
new entry generation, destination and preserved defeated identity. The result
is observation, not a replay capability. Tests require both generation change
and actual entry-policy execution/provider evidence; cache load counters alone
cannot prove re-entry. Do not serialize entry counts or keep a map-travel history.

Repeated successful re-entry is allowed from the completed checkpoint and has
the same durable result. Repeated save/resume remains idempotent except for the
explicit camera repositioning on first revisit. No new combat constructor,
activation pass, pulse, time transition or reward path may be called.

### CLI and keyboard contract

Extend the existing strict form, preserving its current no-save usage:

```text
mmodern --encounter-27 [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>]
mmodern --load-game <game-directory> <save-path>
```

The save option is trailing, singular and requires a nonempty path. Retain strict
seed parsing; reject duplicate/misordered/conflicting options, extra positions,
camera overrides, `--encounter-26`/`--render-map` combinations and explicit
encounter flags or a seed on `--load-game` before path access. Add no M28 entry
flag or separate load syntax. Loaded content selects Ordinary versus completed
Diagnostic27 only after successful decode/resource validation. Resume uses the
loaded path as its F9 target under existing replacement policy.

Application must allow a save target for fresh Diagnostic27 and provide the
encounter resource/preflight/compose services for a v3 load even though the CLI
entry argument is ordinary. Resolve the effective mode from restored authority,
not a caller-supplied rendering flag. Construct a completed Flow adapter directly;
do not call `prepareCombat`, `XeenCombat`'s fresh constructor or approach
initialization. Keep the initial automatic-dispatch suppression explicit.

Production sequence:

1. Start the first form with a target outside the commercial installation.
   Use existing I/F1-F6/category/slot/T/E preparation, close it, Enter to Begin,
   period for Wait/engagement, then displayed Space=Attack/B=Block with automatic
   enemy/round/End work. Existing preparation Escape semantics remain unchanged.
2. After End and retirement, show **Victory completed - F9 save, I inspect,
   R revisit, Escape exit**. Show exact current HP/conditions and owner XP, not
   a notice inherited as proof. No acknowledgment is required to finish End.
3. F9 writes through existing capture, detached resource preflight and file
   publication. Confirm the saved path only on success. Escape/window close
   exits without save-on-exit or a final gameplay pulse; wait for process exit.
4. Launch the second form in a separate process. Its first frame already shows
   completed Victory and restored facts, with no initial default-party flash.
5. I opens completed **read-only** inspection. Reuse existing item/owner display
   and raw M/ID/S/F reporting, with explicit supplement presence/XP, HP/SP and
   all condition bytes, context, original/live monster identity/coordinates,
   lifecycle, accounted identity and completion status. Disable T/E/confirmation
   certificates in this mode. I closes inspection; Escape still exits the
   diagnostic. F9/R refuse while inspection is open.
6. Fresh R (a typed completed-revisit action through SDL and Flow) invokes the
   genuine bounded entry operation. Show the re-entry receipt and entry camera,
   unchanged owner values and defeated/accounted record. This is not navigation
   or Run. Space/B/Enter/movement/Wait/event dispatch remain refused.
7. Save again, exit, resume again and repeat R. No respawn or XP duplication.

Use completed world/Flow generations for R and inspection handoff, including
poll-batch protection and key-repeat rejection; stale combat inputs cannot acquire
completed capabilities. Repeated R requires a fresh displayed ticket and press.
Keep one SDL loop. Inspection/pages are read-only UI and block saving until
closed; nonmodal retained Victory/re-entry observations do not block it. Provide
concise in-frame durable feedback and full read-only console detail so physical
acceptance can distinguish exact facts from missing pixels.

## Implementation units and dependencies

Use **three stages**. Successful-End authority and transfer encoding can be
reviewed independently of fallible fresh-owner publication; both must be stable
before integrating the production lifecycle/UI and process evidence. These are
clean ownership and risk boundaries in the existing capture/codec, restore, and
Application/Flow layers. Combining them in 28A would couple three high-risk
changes before their individual invariants were testable. Neither this split nor
a stage's acceptance authorizes the next stage automatically.

| Stage | Objective, ownership and scope | Validation/acceptance boundary and exclusions |
| --- | --- | --- |
| **28A - Completed authority and v3 capture/codec** | World-owned successful-End proof, guarded retirement into bound quiescent authority, typed integrity/presentation guards and capture eligibility; minimal completion snapshot and six-owner supplements; exact v3 codec, ordinary v2 writer and v1 compatibility. Depends on accepted M27, with no restoration or UI dependency. | Build and focused synthetic authority/capture/format tests, including independent wire oracles and legacy behavior. Review the authority/schema seam. It establishes transfer correctness only: no new production F9 route, fresh-owner restore, restart or M28 completion. Until 28B, decoded v3 must explicitly refuse unsupported restoration; never partially apply it through ordinary restore. |
| **28B - Fresh-owner restoration and bounded true re-entry** | Depends on accepted 28A. Resource/domain validation, guarded candidate construction/preflight/publication, pure resource reconstruction plus defeated overlay, fresh bindings, cache-versus-entry semantics and world-owned completed re-entry. Exercise presentation preflight through existing resource/compose seams without adding a production route. | Build and focused headless encode/restore/re-entry tests, both graph callback guards and failure-safe publication, all reconstructed actor fields and exact owner values. Review private publication and lifecycle boundaries. It establishes coherent fresh authority and entry semantics, not production CLI/SDL/F9, separate-process acceptance or milestone completion. |
| **28C - Production save/restart/revisit and complete acceptance** | Depends on accepted 28A and 28B. CLI/save-target selection; Application/F9 source guards, completed Flow/presentation/recovery/inspection, SDL R input and entry feedback; deterministic production-path child evidence, real executable startup/load checks and original-data controls. | Build, full CTest, complete disk/producer/consumer/revisit/fresh evidence, independent review and maintainer physical acceptance. Only this combined boundary closes M28. No new combat rules, traversal, recovery gameplay or native full-fight automation system. |

Each authorized implementation task must reference this plan and its exact
verified baseline, scope and acceptance boundary rather than reproduce combat
research. A later implementation closure follows AGENTS documentation/Git rules;
this planning task updates only this file and authorizes no closure edits.

## Acceptance evidence

### Inspected coverage and required comparison changes

Inspected the focused save suites: [format](../tests/XeenSaveFormatTests.cpp),
[state](../tests/XeenSaveStateTests.cpp), [file](../tests/XeenSaveFileTests.cpp),
[flow](../tests/XeenSaveFlowTests.cpp), [CLI](../tests/XeenSaveCliTests.cpp),
[combat persistence](../tests/XeenCombatPersistenceTests.cpp),
[encounter saves](../tests/XeenEncounterSaveTests.cpp),
[combat authority](../tests/XeenCombatAuthorityTests.cpp),
[combat gameplay](../tests/XeenCombatGameplayTests.cpp) and
[encounter gameplay](../tests/XeenEncounterGameplayTests.cpp).
They establish useful test seams; inspection is not a new test execution.

[SaveResumeIntegrationTest](../tests/SaveResumeIntegrationTest.cpp) constructs
expected values from initial resources plus explicit expected effects, checks
actual disk files in distinct producer/consumer/fresh processes, and separately
launches the real CLI. Reuse that pattern and
[XeenChildProcessTestSupport](../tests/XeenChildProcessTestSupport.h). Its
ordinary direct camera assignment and cache-discard helpers do not establish
M28 true re-entry. Its older per-batch SDL drive helper is not a substitute for
M27's continuous loop/displayed combat ticket controls.

[XeenSaveTestSupport::sameSnapshot](../tests/XeenSaveTestSupport.h) currently
compares only existing fields. [checkSameCharacter](../tests/XeenPartySnapshotTestSupport.h)
does compare every modeled character/item field, but neither it nor the old
party snapshot includes context or supplements. Extend comparisons explicitly
for completion/accounting identity, context and the exact six-owner presence shape
and all eight input members. Compare every reconstructed actor live field and
immutable metadata/statistics on authoritative owners, not nonexistent snapshot
actor records. Add negative controls that mutate each compared field/presence and
prove the comparison fails; independently prove unexpected live actor mutations
refuse capture/re-entry. Do not claim persistence from an old helper or
encoder/decoder agreement.

### Automated synthetic requirements

| ID | Required evidence |
| --- | --- |
| M28-S01 | Independent literal v3 wire construction, exact 243-byte suffix and every offset, encoder equality and decode values. Distinct asymmetric values for all seven i32 fields/u32 XP across each of the six records, including explicit zero; verify the other 24 owners remain absent. Check fixed context fields/discriminators and identity. Nonzero variable context bounds get independent cases. Retain independent v1/v2 fixtures and CRC calculation. Actor reconstruction has independent resource/overlay oracles in S07; no actor wire fields exist. |
| M28-S02 | Truncation at each new boundary, trailing bytes, count other than six (including 0/27/30/255), invalid booleans/enums, wrong/reordered/duplicate/missing supplemental owner, wrong side/map/record, incomplete record, negative/out-of-range inputs, false completion/accounting and malformed v3 absence. Legacy versions mean absent extension; v3 has no optional owner shape or actor count. Reject unknown versions. Separate structurally accepted values that fail resource/domain checks from malformed payloads. |
| M28-S03 | Successful real domain fight -> lethal publication -> End -> retirement -> capture. Refuse at every earlier phase, including all pending kinds, and at Defeat/support/failure. Reach 959 via existing real-round control and prove removed/awarded SupportStopped cannot retire/save. Destroy transient owners both before and after legitimate retirement; only the latter can remain coherent. |
| M28-S04 | Wrong world/party/roster/camera, detached marked roster, partially attached inputs, copied receipts, old tickets, expired leases, duplicate retirement, mixed completed and ordinary graphs, byte-identical replacement notification and reentrant F9/re-entry. No callback can upgrade old authority by recapturing a new ticket in a catch path. |
| M28-S05 | Provider/preflight faults and mutations on both destination and candidate at initial party, CHR/PTY/MON/map/MOB/EVT, compose and final publication boundaries. Validate no candidate stores, unchanged destination caches for nonmutating failures, no leaked temporary callbacks and no successful publication after an external mutation. Startup never shows a frame or falls back. |
| M28-S06 | Post-End composition/report failure preserves Victory and exact gameplay values; capture/F9 refuse while the presentation lease is pending, including after Flow disposal. Successful authorized reconstruction releases only its matching lease and permits a fresh save after dispatch/UI guards; it never repeats reporting, combat, approach, RNG, XP, equipment or events. Changed preimages permanently refuse this graph; stale recovery cannot clear another lease; fatal recovery/shutdown stays unavailable. Fresh restore creates independent bindings; repeated save/resume preserves values without rewards. |
| M28-S07 | Existing M27 wounded live-actor cache preservation remains covered and unsaveable. Independently construct expected 27 resource records plus canonical target overlay and compare every metadata/live field after fresh restore, cache rebuild and completed re-entry; no actor state is sourced from capture output. Changed live bystander/target fields refuse capture/re-entry. Genuine entry advances generation and executes resource entry construction even with warm caches; preserve accounting and refuse stale/reentrant/unsafe input. |
| M28-S08 | Full v1 missing-field restoration, v2 exact unknown/empty item bytes/holes/inactive owners/ordinary aliases, ordinary initial/later event behavior and existing animation-save boundary. Ordinary writer remains v2; unsafe encounter never falls into it. |
| M28-S09 | Real-file valid v1/v2/v3 replacement, decode-only target checks, unknown/malformed protection, old bytes preserved for handled Open/Write/ShortWrite/Flush/Close/Replace faults, cleanup-failure reporting and protected/Unicode/alias paths. F9 refusal performs zero save-stage/provider/I/O calls and queues nothing. |
| M28-S10 | Strict new/old CLI syntax, v3 mode selection without explicit encounter flag, missing/incompatible/unsafe save startup errors, first-frame restored values, completed inspection gates and R poll-batch/fresh-press handling. Source authority changes during detached save preflight or stage callbacks prevent write/replacement. |

### Original-data controls and independent expectations

Use accepted deterministic controls without formula reinvestigation. Producer
actions must use accepted preparation, displayed player input and real automatic
enemy/round/End continuation. Never set actor HP, character conditions, XP,
completion or items directly to create a victory fixture.

| Control | Inputs and required independent expected facts |
| --- | --- |
| **Injured victory (existing M27)** | Seed 1, original equipment, Enter then period, six first-round Blocks, then Attack for every eligible displayed owner. M27's accepted result is minute 493, ctr24=1; active-order HP `[12,16,12,10,-4,5]`, Rebecca Unconscious=1/Dead=0, +82 XP each. It covers injury/unconscious XP, **not broken armor**. |
| **Broken equipped armor victory (M28 control)** | Seed 56, original equipment, Enter then period, six Blocks, then Arturius Attack and Tyro Attack after round continuation. Independent arithmetic from M27's specified xorshift/rules predicts enemy rolls `20,6,6,1,6,6`, Rebecca HP `7 -> -5 -> -17`, Unconscious=1 and Dead=1; both originally equipped Armor records gain bit7 without frame changes. Arturius dice `2,2,2,1`, hit12 give damage12; Tyro dice `3,3`, hit12 give damage11, removing the remaining8 HP. End completes at492/ctr24=1; active-order HP `[12,16,12,10,-17,5]`; XP `[100,100,100,100,0,100]`. This retained planning arithmetic is a proposed literal oracle, not an executed engine result; 28C must confirm it through production before claiming acceptance. |
| **Prepared equipment/transfer victory** | Seed56; in Preparation transfer Seymour's owner6 Accessories slot1 Speed ring to Arturius/owner0, then equip it. Expected source slot1 becomes `{0,0,0,0}` and destination Accessories slot1 becomes `{86,1,0,8}`; both slot0 necklaces stay exact. Arturius remains first in initiative, now Speed19, and the first enemy still targets Rebecca. Use the same six Blocks/two Attacks as the breakage control, with the same predicted HP/XP/time and Rebecca armor deltas. All other item bytes stay initial. Confirm through production, with expectations independent of capture/codec output. |
| **Fresh control** | Independent no-load process starts Diagnostic27 Preparation with original six owners/items/XP, then Begin creates all27 original actors with record5 Present/HP20/spawn `(13,2)`. No saved completion/accounting. Stop without writing over producer evidence. |

For seed56, original Rebecca Armor slots 0 and 1 are `(0,2,0,3)` and
`(38,10,0,9)`; expected saved records are `(0,2,128,3)` and `(38,10,128,9)`.
All her other slots/categories and all other owners' items are unchanged except
for the explicit ring transfer/equip in the prepared variant. SP in
active order remains `[2,0,2,0,7,9]`; both seed controls retain every other condition
byte at zero. The initial CHR supplies all unchanged owner values independently
of the capture path. If connected production evidence contradicts this new
control, resolve the smallest input/oracle error under the settled M27 rules;
do not alter combat to fit it or call breakage covered by seed1.

For each completed producer, compare live state against independent expected
values **before capture**, actual disk decode/wire fields after F9, and fresh
consumer owners before any visible frame. Include selected and all bystander
identities/original metadata/live fields; owner-indexed XP and all supplement
fields/presence; all30 character payloads, all144 item bytes each, conditions,
HP/SP, membership, context, camera, all quest/game flags and independent ordinary
removal sets. Do not compare only the target and six XP totals.

Use genuine disk files, distinct producer/consumer PIDs and normal producer exit
before consumer launch. The smallest sufficient automated evidence is the
seed-1 injured path and seed-56 prepared-item/breakage path, plus a fresh control.
The original-equipment seed-56 row supplies the independent breakage oracle;
it need not add a third full restart run if the connected prepared variant
confirms the same combat result and its explicit item differences.

Extend existing deterministic child/harness infrastructure to drive actual
Application/Flow/SDL handling in the continuous gameplay loop, using displayed
input tickets and real automatic continuations. It supplies live owner assertions,
F9 through capture/preflight/file writing, separate-process restore and R through
the genuine entry operation. No private direct completion/camera mutation or
test-only gameplay completion/exit CLI option may substitute for these paths.
Detailed harness observations must connect the literal expectations to live
production publications before capture, rather than feed expected outcomes into
gameplay. Separately exercise the real `mmodern` executable's CLI combinations,
fresh save-target startup, v3 load/first-scene and invalid-load failures, reusing
bounded startup/close support. Maintainer keyboard acceptance below covers its
complete fight/F9/exit/resume/R sequence. A new native Windows keyboard-injection
system for a full executable fight would duplicate those boundaries and is not
required. Harness-only evidence without executable checks and physical acceptance
cannot close M28.

Check cache reconstruction before save, after restore and after R; separately
prove R's true entry generation/policy. Repeat F9 and process resume after revisit
and repeat R at least twice, retaining removed record5 and exact once-only XP.
Consumer inspection/revisit alone leaves disk bytes unchanged; explicit F9 may
replace them. Include normal original metadata and all bystanders, not deletion
or freezing introduced to isolate the test. Failure controls preserve a previous
valid file. Ordinary original-data restart controls remain passing regressions.

### Maintainer physical acceptance and closure

Use one primary end-to-end maintainer run of the connected seed-56 prepared-item
path once automated evidence confirms it. This combines genuine victory, broken
equipped armor, simultaneous injury conditions, ineligible XP, exact ring transfer/
equip persistence, I inspection, successful F9, full process exit, a new
`--load-game` process, restored first-frame/inspection checks, R true revisit,
another save, exit and resume/revisit. Add a short independent fresh-new-game
control to observe original state without overwriting the evidence save. The
automated seed-1 injury/unconscious-XP control need not be repeated physically.
If the prepared path cannot cover the stated combination, resolve the control
before closure and add only the smallest distinct physical check required.
Read-only inspection must expose identity 20/5,
immutable spawn versus removed live coordinates, completed/accounted facts,
minute/ctr24, exact XP by roster owner, signed HP/SP, simultaneous condition bytes
and physical item slots/bytes. In-frame feedback must make completion, saving
and diagnostic re-entry clear; console detail supplements it.

Within the primary run, physical checks include unsafe F9 during Preparation/combat,
inspection-open refusal, exit without automatic save, and no enabled combat or
exploration controls after completion. Revisit must visibly report its genuine
entry transition, not merely show an empty scene. Precise short-lived pending-End
and callback boundaries remain deterministic automated checks; do not require
the maintainer to time a key into them. Automated input, screenshots, image
comparison and independent review do not stand in for the maintainer's physical
result.

M28 closure requires the build, full CTest suite, original-data/process evidence,
independent review and maintainer physical acceptance of the combined outcome.
No schema-only, headless-only or screenshot-only result establishes completion.

## Exclusions, risks and replanning triggers

Excluded: Run/disengagement, rest/healing/recovery/resurrection, successful
Defeat or living/mid-combat saves, general map travel, repeated encounters,
random loot, new combat formulas/rules, additional admitted monsters/maps,
Darkside gameplay, original save compatibility, in-session load, autosave,
save-on-exit and M29 planning. No commercial data is copied into the repository
or changed. A diagnostic same-map re-entry does not certify traversal.

No material architectural contradiction remains unresolved in this plan.
Implementation risks requiring focused review are the new marked-graph private
publication path, lifetime-safe retirement and completed bindings, nested
provider/Flow callback guards, acceptance of reachable injury/legacy frames,
and connected production confirmation of the prepared armor-breakage oracle.
Treat these as required validation, not presumed passing behavior.

Replan and report the smallest affected contract if accepted M27 states cannot
be represented/validated without losing values; a bystander is actually mutable
in this envelope; true entry requires unsupported gameplay/time/activation;
retirement cannot preserve authority without a parallel gameplay owner; the
existing signature omits a newly required source; callback/publication guarantees
cannot be maintained; or requested acceptance needs living saves, recovery,
wider travel or another encounter. A material unresolved contradiction blocks
implementation-ready status rather than authorizing silent scope expansion.

## Planning validation record

This document is based on source/test inspection, the complete closed M27 plan,
targeted M20/M21 contracts and the verified Git baseline. The prior seed56 numeric
oracle is retained without combat recalculation or redesign in this revision.
No build, CTest suite, game execution, original-resource extraction or physical acceptance
was performed for this documentation-only task. All implementation tests and
acceptance results above are requirements for future authorized work.

The complete untracked contents were reviewed in sections, including field
coverage, compatibility branches, ownership/lifecycle consistency and scope.
All local Markdown links and heading anchors were checked, as were fence balance
and the independent 243-byte extension calculation. `git diff --check` and the
explicit untracked-file whitespace scan passed; the no-index check against NUL
reported no whitespace diagnostics (exit 1 denotes the new-file difference).
An empty tracked diff alone was not used as validation. The planning handoff
must confirm that only this plan is added, HEAD is unchanged and the index empty.
