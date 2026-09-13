# Milestone 31 - First connected Clouds vertical slice

**Status: implementation-ready specification; not implemented.**
Baseline: `dfacac7076a7ae7714229b691eb80afea0f70347`,
`Complete M30 production expedition and milestone closure`.
Implementation requires separate authorization. This plan owns the M31 contract;
[M30](milestone-30-plan.md) owns the inherited expedition, combat and schema-2
details, and [project status](project-status.md) remains the stable baseline.

## Objective and acceptance boundary

Close the original Bone Whistle collection loop in production
`--journey-expedition`, on the same contract-2 party/world/camera owners:

```text
prepared (0,14) East -> accepted six-cell expedition and influencing encounters
-> quiet (5,14) -> original WhoWill / discovery / acknowledgment / grant / Remove
-> mutable return to (0,14) West -> explicit F9 -> process restart
-> preserved collection and consequences -> further navigation/item management
```

Collection and return constitute the slice. There is no invented turn-in,
exit teleport, reward at the starting cell or terminal Journey completion mode.
Success leaves the existing mutable Journey available. Influencing actors must
resolve through the accepted scheduler whenever they engage; surviving actors
remain authoritative. Collection has no all-actors-defeated prerequisite and
never suppresses survivors or retires an active encounter.

Keep M30's prepared party, six party cells, actor bounds, four influencing
identities, grouped combat, targeting, joining, Disease/injuries/breakage/XP,
inventory/equipment rules, clock and RNG continuation unchanged. Normal entry
and startup resume retain their existing CLI meanings. This milestone certifies
neither general map-20 exploration nor travel from Vertigo.

Use **one implementation unit**, with focused internal tests followed by connected
acceptance. Event authority, modal handoff and persistence admission must agree
before collection is usable; separate A/B release boundaries add no useful
independent capability here.

## Original interaction and address contract

The accepted original-resource evidence is the
[M18 Bone Whistle checkpoint](milestone-18-plan.md#10-exact-original-bone-whistle-checkpoint),
implemented in `XeenEventInterpreter`/`XeenEventSystem` and exercised by
[`WhoWillIntegrationTest.cpp`](../tests/WhoWillIntegrationTest.cpp).
M30 independently admits the same topology in
[`XeenActorApproach::validateEnvironment`](../src/games/xeen/XeenActorApproach.cpp).
Do not extract or bundle original resources or replace their text with literals.

`maze0020.evt` contains 16 original records. These five records are at Clouds
map 20 `(5,14)`, direction `All(4)`:

| Record | Line | File offset | Opcode | Operands (hex) | Required meaning |
| ---: | ---: | ---: | --- | --- | --- |
| 1 | 0 | 7 | `20 WhoWill` | `00 03` | Search; title from text index 3 |
| 2 | 1 | 15 | `29 DisplayBottom` | `00` | Discovery text index 0 |
| 3 | 2 | 22 | `09 If2` | `2C 01 03` | Action 44, value 1, acknowledgment to line 3 |
| 4 | 3 | 31 | `0C TakeOrGive` | `00 00 15 64` | Neutral take, mode-21 grant of quest item 100 |
| 5 | 4 | 41 | `0E Remove` | none | Existing physical-cell removal and logical restart |

Text comes from `aaze0020.txt`. Object identity is `{Clouds,20,1}`, at
`(5,14)`, original direction North, table index 1/resource 26 (`026.obj`).
Selection uses the first eligible original object at the physical cell,
independently of facing, projection or visible pixels. Retain and validate that
identity; sprite ID is never mutation authority.

The physical camera and logical script address happen to share map/X/Y for
this chain. They remain distinct values: dispatch starts logical line 0 using
the current physical facing; `All` matches all four facings. Neither turning
North nor changing camera position is part of collection. There is no Call,
teleport, Action 9 SP test or quest-possession condition in this chain.
`Remove` targets the working physical cell and selected object, then restarts
**logical line 0**, keeping lookup direction and continuation context. It does
not advance to line 5 or delete event records. The restarted records 1-5 are
effective `None`; missing line 5 completes naturally. Successful collection
executes ten instructions; later interaction executes five effective `None`
instructions, with no new prompt or grant.

Pinned reference: ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`, Xeen `scripts.cpp`
(`cmdWhoWill`, `cmdRemove`, `cmdMakeNothingHere`, Action 44 and dispatcher),
with the dependency/provenance policy in [dependencies](dependencies.md).
M18/M15 already establish the source-to-MMModern continuation adaptation; M31
preserves it rather than introducing another script implementation.

## Ownership and Journey integration

`XeenEventFlow` remains the sole noncopyable live event continuation owner.
Its existing pending execution owns temporary character/object selection,
logical address, working camera/flags, request, pages and response generation.
`XeenEventSystem` and `XeenEventInterpreter` continue decoding and executing the
original records. `XeenPartyState::questItems` owns the counter; `XeenWorld` /
`XeenSessionWorldState` owns disabled identities and live actors. Application
owns committed camera/game flags. No new quest owner, event engine, Journey
mode, completion flag, save system or commercial-data override is permitted.

Replace contract 2's Deferred disposition with bounded event admission, not an
unrestricted script callback. Admission must check the actual owner graph,
contract, immutable event/object contract, physical address and current presented
input before script-capable work. All other route cells remain event-free;
contract 1 and encounter diagnostics retain their existing exclusions. Initial
and movement processing must not start this manual event or bypass actor work.

Add an exclusive **Event activity within Journey**, coordinated by the existing
`XeenEncounterFlow`, using `XeenCombatBoundary::Work::Event` and the existing
ticket/preimage/generation discipline. Acquire it only from a presented quiet
boundary: pending approach zero, Exploring, no contact/combat, attachment,
Round/End/retirement, inventory/certificate, save or unresolved frame work.
Actor work has priority; a refused interaction is not queued for later.

The Event lease spans dispatch, all automatic `Presented` continuations,
WhoWill, discovery pagination, Action-44 acknowledgment, immediate mutations,
execution completion/error/cancellation and the final composition handoff.
Presenting an intermediate modal frame admits only its matching response; it
must not turn Event into Quiet. On terminal event handling, transfer authority
to the existing Presentation boundary without a quiet gap. Only its matching
successful SDL presentation opens mutable input and capture again.

Extend the existing production seams in
[`XeenJourneyFlow.cpp`](../src/app/XeenJourneyFlow.cpp),
[`XeenEventFlow.cpp`](../src/app/XeenEventFlow.cpp) and
[`XeenRestoreGuard.h`](../src/games/xeen/XeenRestoreGuard.h).
Today `journeyRead` refuses the objective, Flow rejects encounter responses,
Journey composition bypasses event rebasing, and quiet preimages reject any
grant/Remove. Simply removing these checks is insufficient.

Use a bounded publication authorization tied to the live Flow continuation,
Journey ticket/lease and expected operation. Reuse the existing interpreter
operations and party/world publishers; any added guard interface must authorize
their publication, not reproduce their semantics in Flow. In M31 it admits
only the chain above and its effective-None/cancellation paths. Successful
grant authorizes precisely counter 100's checked increment; Remove authorizes
only the selected object and this physical cell's five event identities.
Completion preserves the existing camera/flag publication rule; for this chain
their values are unchanged.

Check retained authority before and after external providers, reporting,
composition and presentation callbacks, including event/text reloads and warm
caches. Admit each known internal publication into the retained preimage before
another callback can run. Never recapture arbitrary live state after a callback
and call it authorized. All other counters, flags, characters, membership,
supplements, items, context, actor/accounting state and RNG remain exact.
Prepare successor guard storage before each publication so adopting its known
effect cannot itself allocate or throw after gameplay has changed.
Reentrant, wrong-owner, copied-result and stale-generation calls cannot publish,
resume or release a newer lease. Public injection of detached event results
must remain unavailable in Journey; direct responses need the same current
continuation and successfully presented modal authority as SDL responses.

## Mutation ordering and failure semantics

The discovery message precedes acquisition. WhoWill and discovery presentation
change no durable values. F1-F6 resolves the displayed active index to the same
live roster owner and rechecks `canAct()`: Disease alone is eligible;
Unconscious/Dead are not. Invalid/ineligible selection retains the prompt and
performs no grant. Escape at WhoWill completes like Exit in one instruction,
leaving collection unchanged and allowing a fresh retry after frame handoff.

DisplayBottom's `Presented` response is automatic for one page; pagination uses
the existing presenter. It then reaches the separate Action-44 acknowledgment.
Only an accepted current acknowledgment advances to line 3:

1. Consume the response/continuation authority before resumption.
2. The existing checked mode-21 operation increments quest item 100 immediately
   (`q -> q+1`, original fresh `q=0`). It is party-wide, not an inventory item
   for the selected character, and creates no reward queue or receipt.
3. Execute ordinary Remove against object 1 and events 1-5; publish those world
   overlays before further interpretation or fallible presentation.
4. Execute the effective-None continuation and complete through EventSystem.
5. Recompose the changed world, retain valid discovery text, and present the
   result before opening a quiet boundary.

These are separate authoritative effects, **not one transaction**. Counter
overflow refuses the grant without wrap and does not run Remove. If a later
Remove preparation/provider failure occurs after the grant, the grant survives.
If reporting, continuation or drawing fails after Remove, removal survives.
Do not refund, rerun the acknowledged operation, restore a pre-event snapshot
or infer a quest flag from either effect.

For the admitted Remove publication, prepare both resulting identity sets and
all fallible validation/storage before changing the world, then publish without
throwing. Keep this in the existing world Remove operation. The baseline inserts
into sets incrementally; M31 must make this publication boundary explicit so
allocation failure cannot leave untracked partial removal. This operation-level
preparation does not combine or roll back the preceding grant.

Known manual script/presentation failures may return to mutable Journey only
after pending continuation is consumed/discarded, already-published effects are
retained and validated, and a fresh recovery frame succeeds. A recoverable
composition retry is presentation-only; it never redispatches or acknowledges
again. If a grant succeeded but Remove did not, active original events remain:
a new explicit interaction follows them and can grant again. There is no
Bone Whistle-specific duplicate guard; normal successful repeat prevention is
the disabled event overlay. Do not label a partial failure as completed collection.

Owner/integrity violations, inability to establish a trusted post-publication
state, fatal SDL failure or shutdown keep the graph unavailable for saving and
further gameplay. Stale work cannot revive it or fail newer work. No cleanup,
exception or destructor manufactures quiet authority. Published in-memory
effects survive cleanup, but survive process termination only if explicitly
saved at a later eligible boundary.

## Modal presentation and controls

Use the existing presenter, original fonts/text and the single SDL loop. Event
layers must rebase over production Journey composition of the committed camera,
world and party, including actors. During suspension, cosmetic animation/cache
reconstruction may refresh that base without advancing actor work, gameplay
time or RNG, dismissing the modal, losing its page/selection, or acknowledging it.
The Journey notice must not cover the prompt, discovery text or response controls.

Show F1-F6/eligible choices and Escape cancellation for WhoWill. Discovery/Action-44
acknowledgment uses Space/Enter and must be visibly acknowledgment-only: adapt
the existing confirmation widget by its response requirement instead of showing
misleading Yes/No controls. Actual Yes/No events keep their behavior. Escape
after WhoWill retains the ordinary non-NPC acknowledgment checkpoint's exit
semantics; it is not an acquisition acknowledgment or a rollback command.

Navigation, Wait, Attack/Block, target selection, inventory, transfer/equipment,
another interaction dispatch and F9 are blocked while modal. Space at the
acknowledgment is a response, not combat or a second interaction. A selecting
F-key cannot acknowledge the next phase. Repeated, held, stale or same-SDL-batch
keys cannot cross generations, including cancel-to-retry and acknowledgment-to-
save. Recomposition preserves valid semantic pending state; each changed frame
still needs a matching upload/presentation before accepting new input.

After success the object draw command is absent, transient response controls
are gone, and the original discovery text remains readable through the existing
retained-layer behavior. A new ordinary gameplay action clears that retained
text as usual. A nonblocking retained message alone does not prohibit F9.

## Save eligibility and restoration

**No new durable field, envelope version, content contract or schema is needed.**
Keep Journey v4, schema 2/content 2, its 1366-byte suffix and all M30 validation
of party, actor, context and RNG state. The existing v2 base already serializes
all 35 quest counters and independent disabled object/event identity sets.
Object/event additions change the variable base length, not the Journey suffix.
Do not serialize a completion Boolean, WhoWill selection, event lease, request,
page, instruction cursor or retained message.

Remove M30's objective-overlay prohibition in
[`XeenSaveState::validateJourneyValues`](../src/games/xeen/XeenSaveState.cpp) and
the corresponding environment check in `XeenActorApproach`. Separate immutable
original EVT/MOB topology admission from mutable overlay admission. Continue
validating the original records even after removal, including on restored
navigation, survivor attachment and cache reconstruction; do not compare an
effective-None script to the immutable opcode contract or require fresh bones.

Counters and identity sets remain independent categories with existing bounds,
uniqueness and resource-identity validation. Do not impose `count == 1`, derive
removal from possession, reconstruct possession from removal, or require the six
objective identities to occur as an all-or-nothing save encoding. Valid existing
independent overlays and a grant-only failure state round-trip exactly. They
are not certificates of successful collection; the connected execution witness
establishes that result. Invalid identities/resources still reject without
normalization or fresh fallback. M31 broadens contract-2 admission only as
needed to preserve these objective effects; it does not enable other scripts.

| Boundary | F9 / restart contract |
| --- | --- |
| Quiet before interaction, including at `(5,14)` | Save exact current consequences; restore with objective still available and no automatic dispatch. |
| Dispatch, WhoWill, discovery pages or acknowledgment | Refuse before capture/providers/preflight/file I/O; no queued save. |
| Grant/Remove published, completion or recovery frame pending | Still refuse; publication alone is not quiet authority. |
| Cancellation/success/recoverable error, new quiet frame presented | Fresh F9 may save the exact surviving state. No pending UI is serialized. |
| Fatal/integrity failure, shutdown or combat work | Existing refusal remains; last valid disk save is unchanged. |

Restore through existing unpublished validation, compatible-resource loading,
overlay application and the one-time fresh-owner binding. Before the first
presented frame it remains unsaveable. Restore never dispatches the objective,
grants/removes, replays combat/XP/Disease, advances RNG/time, heals/retrains or
reinitializes party/items. After successful collection, the resumed bones remain
absent and interaction runs effective None. Before collection, a fresh explicit
interaction may collect. Closing during suspension and loading the prior quiet
save restarts from that saved state, not from a suspended cursor.

Existing M30 schema-2 saves remain readable without conversion and may collect
under M31. M30 binaries may reject newly admitted removed-object saves; backward
execution support is not promised. Ordinary v1/v2, completed Diagnostic27 v3
and Journey schema-1/content-1 retain their distinct meanings and behavior.

## Required automated evidence

Extend existing event/WhoWill, Journey/expedition, persistence, Application and
SDL harnesses. Synthetic tests remain independent of commercial data; original
integration uses external unmodified archives. Test observable facts and
publication boundaries, not only a new activity enum or isolated helper calls.

| Evidence | Required assertions |
| --- | --- |
| Original chain in Journey | Exact table/identities above; all-facing manual admission; live eligible non-first selection, ineligible refusal and cancellation/retry; no durable changes before acknowledgment; success `q+1`, object 1/events 1-5 disabled, ten instructions; repeat five None. |
| Exclusive authority | Pending approach/contact/attachment/combat/retirement/save/inventory refuse dispatch before providers; modal blocks incompatible controls; stale/copied/wrong-phase/direct responses, same-batch/held/repeated SDL keys and F9 cannot publish or release. Test reentrancy/owner replacement in event/text/map/MOB/report/compose callbacks and on cache-hit paths. |
| Publication and failure | Overflow; failure before grant, after grant/before Remove, after Remove/before completion/report/frame; counter and overlays survive exactly once; Remove preparation failure publishes no partial sets; recovery does not replay; integrity/fatal cases cannot save. Independent counters/overlays remain independent. |
| Presentation/reconstruction | WhoWill, discovery pagination, acknowledgment and retained success survive individual and combined scene/script/text/sprite cache rebuilds with real reload evidence; committed camera only, correct bones visibility, readable controls, no modal overlap or gameplay/RNG/time advance; failed and stale frame handoffs stay closed. |
| Persistence/compatibility | Production F9 before and after collection, cancellation and recoverable failure; exact counters/sets plus all 30 owners/items/supplements, 27 actors/accounting, context/camera/flags and RNG; malformed identities/topology reject; legacy v1-v4 domains and existing M30 schema-2 saves retain behavior. |
| Connected restart | Fresh production entry through encounters, collection, return, disk save, distinct-process `--load-game` and further real mutation; compare all persisted bytes and RNG continuation with uninterrupted execution. Also split before collection and immediately after collection, then continue the return in a new process. Startup performs zero event/grant/Remove/combat/XP/RNG replay; a later explicit repeat executes only the five None records. |

The main original connected witness extends schedule 1 in
[`XeenExpeditionGameplayTests.cpp`](../tests/XeenExpeditionGameplayTests.cpp):
seed 1, settle approach after each forward step, Attack against the first
original-ordered contact, five eastward moves, then three Wait actions to resolve
the Zombie pair. Existing checkpoints are End minutes 491/522/565, with Rebecca
at current HP 5, Disease 3, maximum HP 18 and SP 21/18. Turn North, cancel once,
retry/select an eligible member, acknowledge, then turn West and return five
cells. With the same movement schedule, collection/cancellation adds no time or
RNG, so return remains minute 615. Assert unchanged M30 consequences around
each objective step rather than rederive combat formulas here.

Also extend a connected surviving-actor schedule (M30 schedule 0 or 2) through
collection and subsequent movement/attachment as applicable. This catches a
remaining fresh-object restriction in environment validation. Keep M30 grouped,
triple, unfavorable/defeat and legacy controls as regressions without duplicating
their complete combat oracle. A relocated objective fixture or an injected
post-collection save is useful unit coverage but cannot replace the connected
witness. Include real SDL input routing and separate-process Application/F9/load
evidence; distinguish those from uninstrumented CLI and physical validation.

Build and run focused changed-subsystem tests during implementation; full CTest,
relevant original-resource controls and the connected process witnesses must
pass at milestone closure. Inspect native frames for the objective phases.

## Implementation review and maintainer acceptance

Independent implementation review must inspect the final candidate and evidence,
especially publication authorization,
surviving effects on failure, modal/frame authority, immutable-versus-effective
events, exact persistence and connected no-replay evidence. Closure requires an
accepted verdict, full automated validation and separate maintainer physical
SDL acceptance; images or synthetic input do not substitute for the latter.

The maintainer should use the unchanged prepared expedition with a reproducible
seed-1 successful route, resolve influencing encounters using normal controls,
and inspect accumulated HP/SP, Disease, equipment and XP. At a quiet objective
boundary, save and restart before collection; verify the original WhoWill prompt,
cancel and retry, choose a live member, and acknowledge the original discovery.
Check blocked movement/inventory/F9 while pending, readable acknowledgment-only
controls, disappearing bones and recovery of mutable controls. Reinteraction
must not grant again. Return to `(0,14)` West, inspect consequences, explicitly
save, close, load in a new process and confirm persistence plus further ordinary
navigation/item management. Include a quiet save immediately after collection
and resume the return from it. An unfavorable run may still lose; acceptance
does not promise recovery from every seed or input schedule.

First Aid is excluded: M30's original schedule-1 witness already survives the
complete route with the stated injury/Disease values, and the objective has no
HP/SP, combat, time or RNG cost. There is no demonstrated recovery dependency.
If implementation reveals a contradiction, report it and replan before adding
spells, altered preparation or healing; do not silently change the boundary.

At accepted closure, update stable status/history and condense this plan under
[AGENTS.md](../AGENTS.md). Update README for the production collection capability
and controls, and remove completed future scope from the roadmap. Planning
alone changes none of those stable completion claims.

## Explicit non-goals

General map-20 exploration, map transitions/Vertigo travel, original-game startup,
generic quests or unrestricted event admission, Myra/Phirna travel, Bone Whistle
use or turn-in, new rewards, broader combat/RNG rules, First Aid/spells/recovery,
Disease cure, rest/services/time systems, recruitment, item use, general UI/audio
fidelity, Darkside gameplay, autosave, mid-event/mid-combat saves, in-session load,
new save formats and original game save compatibility remain outside M31.
