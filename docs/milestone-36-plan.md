# Milestone 36 - Learned exploration casting

## Final scope and acceptance boundary

**Completed and accepted.** M36 adds learned First Aid and Awaken in fresh
Regional Journey exploration on the admitted map-23 mainland. It establishes
roster-owned knowledge, bounded spell identity and resource admission, modal
selection, exact SP/effect/time/actor publication, and quiet restart. First Aid
and Awaken are distinct learned effects, not item-use shortcuts.

Fresh `--journey-region` selects content contract 7. Contract 7 inherits the
contract-6 prepared party, mainland, actors, events, well, antidote, Shoot, Run,
consequences and treasure. It adds learned exploration casting and knowledge
persistence. The [roadmap](roadmap.md#near-term) records M37 as the next planning
unit; M36 completion grants no authorization to implement it.

Inherited authority lives in [M28](milestone-28-plan.md#completion-authority-and-retirement),
[M29](milestone-29-plan.md#durable-ownership-and-authority),
[M31](milestone-31-plan.md#ownership-and-journey-integration),
[M32](milestone-32-plan.md#regional-navigation-and-admission),
[M33](milestone-33-plan.md#conditions-and-the-minimum-time-extension),
[M34](milestone-34-plan.md#surviving-actors-xp-and-treasure) and
[M35](milestone-35-plan.md#selected-well-and-bounded-item-use).
These remain authoritative for their domains; M36 does not re-specify them.

## Original-data and reference provenance

The pinned ScummVM revision and configuration are in
[dependencies.md](dependencies.md#pinned-scummvm-revision). Original
`XEEN.CC/maze.chr` contains thirty 354-byte character records. Each owner's
39 raw learned flags occupy offsets 121..159; the class-category slot is
separate from a global spell ID and from any miscellaneous item spell ID.
`SPELLS_ALLOWED` maps Clerical, Wizardry and Druidic slots 0..38 to global IDs
0..75; slot 39 and global ID 76 are UI sentinels. Original current-spell and
quick-option bytes are selection convenience, not durable knowledge.

For the admitted World-of-Xeen Clouds profile, original spell names come from
`DARK.CC/spells.xen`: exactly 77 NUL-terminated entries, 937 bytes, CRC32
`0x63568f11`. Entry 1 is Awaken and entry 26 is First Aid. Contract-7
admission validates this resource and retains its immutable preimage. Commercial
names and character bytes remain external; no extracted data is stored here.
Original learned Light and Magic Arrow remain visible but unsupported.
Learned global First Aid (26) is distinct from item spell First Aid (4), and
the M35 antidote remains a separate miscellaneous item action.
The selected numerical rules follow the pinned reference's learned spell
mapping and costs, while MMModern's bounded Flow/EncounterFlow/SDL owners provide
runtime authority. Reference source is evidence of intended algorithms, not a
claim of independently observed DOS behavior.

## Knowledge, identity and resource model

### Authoritative representation

`XeenLearnedSpells` is a fixed `array<uint8_t,39>` value with an optional
`learnedSpells` member on `XeenCharacter`. **Present** means an explicitly loaded
or saved spellbook, including all-zero; **absent** means this domain does not
model knowledge. Preserve raw flags, including noncanonical nonzero values;
query `raw[slot] != 0` for knowledge. This follows the reference boolean use
without losing original bytes or conflating absence with ignorance. There is
no parallel spellbook map on Flow, party or world and no acquisition API in M36.

`XeenCharacterFormat::parseLearnedSpells(bytes, owner)` is a bounded inert parser:
require exactly `30*354` bytes and owner 0..29, return offsets 121..159 without
changing `hasSpells` or any other field. Ordinary `parseRoster` behavior remains
unchanged. Fresh contract-7 Journey preparation explicitly loads all thirty
books from the same retained CHR bytes already used for supplements, prepares
them in the detached candidate and publishes them in the guarded initialization.
All other fresh entry domains retain absent knowledge. Do not load only active
casters or initialize a book on opening the UI.

Four separate queries govern casting:

1. Capability: existing `hasSpells` and a valid class category.
2. Knowledge: optional present and the selected slot nonzero.
3. Implementation support: mapped global ID is 1 or 26 in contract 7.
4. Current eligibility: active owner, `canAct()`, legal exploration boundary,
   current resources, checked rules and charge support.

No `maxSp()>0`, highest-condition approximation, item possession or default UI
selection can establish knowledge. Use existing `canAct()` exactly, including
its worst-condition precedence for simultaneous conditions; do not introduce an
independent incapacity list or impose HP>0 on the pure caster query. Production
Journey's existing condition/HP validation remains in force.

Category+slot identifies lookup; a typed global identity governs effect dispatch.
Bounds are explicit: category 0..2, slot 0..38, effect ID 0..75. Reject sentinel
39/76 and out-of-range identities before indexing. A learned supported spell
does not become castable for a class whose mapping lacks it. Unknown/invalid
class records and learned bytes on noncaster records remain losslessly inert;
they grant no category or effect. Preserve learned-but-unsupported entries and
show them truthfully. Never silently convert, discard or activate them.

All queries resolve active index through `activeRosterIds` to the authoritative
roster owner. A self-target and a different active alias for that owner are the
same target. Party-wide effects prepare one delta per distinct roster owner in
first active-reference order. Inactive books persist but inactive characters are
not selectable or affected. Contract 7 retains the fixed original active order;
alias handling is tested at shared query/effect boundaries, not used to expand
Regional membership admission.

`xeen_state::sameCharacter`, test snapshot equality and hand-written
character comparisons include optional presence and all 39 bytes. Existing
guarded copy/move/swap restrictions remain; copied values carry knowledge, never
casting authority. Full-character combat/consequence candidate copies must retain
knowledge; narrow HP/condition/SP publishers must not overwrite unrelated fields.
Include active and inactive books in preimages, capture, provider guards and
fresh-owner restoration, including ABA, equal-value replacement and mutation
followed by reversion tests.

### Selected spell and bounded catalog

Do **not** make original `_currentSpell`, `_quickOption` or `_lastCaster` durable.
The inspected use is selection convenience; none is needed to reproduce the
admitted effects. M36 has no Quick Cast. Opening C starts with explicit caster
selection; opening a caster's list highlights its first learned row and requires
fresh confirmation. Closing/restarting discards selection. Original current
spell `255` or invalid values are never indices or errors in the new book parser.

A small immutable learned-spell catalog/rules module adapts precisely the
three 39-entry mapping rows from pinned `SPELLS_ALLOWED` (exclude the sentinel),
with GPL attribution and pinned path/SHA. It includes only the two supported effect
descriptors and their numeric costs from `SPELL_COSTS`/`SPELL_GEM_COST`.
This is a finite lookup, not a survey/implementation of other spell effects.
No ScummVM engine construction, new linked library, full `mm.dat` decoder or
additional build dependency is admitted.

An explicit read-only `DARK.CC/spells.xen` accessor uses the existing
asset/bridge owner, analogous to monster statistics/material names. Parse exactly
77 terminated strings with no trailing bytes; enforce archive extent and bounded
nonempty display names (at most 63 bytes each). Retain original bytes and use the
existing font/text renderer; do not copy commercial names into source or saves.
Contract-7 resource admission checks size 937 and CRC32 `0x63568f11` through
existing zlib support; No SHA-256 runtime dependency is added.
Retain the exact parsed/raw name preimage across cache reconstruction. Missing,
malformed or incompatible names prevent contract-7 startup/restore; a changed
previously admitted resource latches integrity failure. No fallback empty book
or successful unnamed cast is permitted. Older domains do not require this new
resource. Costs and supported effect identity never come from rendered strings.

## Supported spells and exact rules

| Property | First Aid | Awaken |
| --- | --- | --- |
| Global identity | 26 | 1 |
| Learned slots | Clerical 14; Druidic 11; absent in Wizardry | Clerical 1; Wizardry 0; Druidic 1 |
| Eligible caster classes | Paladin, Cleric, Druid, Ranger, when actually learned/capable | All six spellcasting classes, when actually learned/capable |
| Production initial casters | Rebecca | Rebecca, Seymour, Badger |
| Cost | Exactly 1 current SP, 0 gems | Exactly 1 current SP, 0 gems |
| Target | One selected active reference, including self or terminal member | All distinct active roster owners; no target prompt |
| Effect | Living target: bounded +6 HP; possible Unconscious clearing | Clear Sleep; HP>0 also clears Unconscious |
| Exploration admission | Contract 7 mainland only; no combat | Same |

Neither cost depends on class, level, maximum SP or number of targets. SP is the
existing signed i16 current value, not recalculated maximum SP. Require SP>=1;
negative/zero SP refuses before subtraction. Current SP above maximum is allowed
and is decremented exactly, without clamping. The carried u32 gems remain exact,
including zero and values above signed-int maximum. These spells work with zero
gems. Do not borrow gold, pending monster gold or banked resources.

**Nonzero gem debit/refund and level-scaled costs are deferred.** Preserve and
display the zero-gem cost and existing purse, test zero/high purse invariance,
and reject unsupported effects before any cost operation. Do not add a gem spell,
generic negative-coefficient evaluator or acquisition to exercise an unused
abstraction. A later cost consumer must specify checked unsigned comparisons and
its own scaling/refund contract; do not inherit the reference's signed gem cast.

### HP and condition semantics

For First Aid, terminal means any nonzero Dead (13), Stoned (14) or Eradicated
(15), matching `isDead()` over this condition ordering. Such a target is valid
to select but produces **Spell failed**, no target mutation and no refund.
It still owes the exploration charge. It never resurrects or repairs equipment.

Otherwise calculate live `M = XeenCharacterRules::maxHp(target, currentYear)`
under checked existing rule inputs, including level, age, Disease and equipment.
Let signed current HP be H:

| Before | HP afterward | Unconscious afterward |
| --- | --- | --- |
| H <= M | min(H+6, M) | Clear only if resulting HP>0; otherwise preserve |
| H > M | H, unchanged | Clear if H>0 |
| Terminal condition | H, unchanged | Unchanged |

Use a widened intermediate and check the resulting i16 before stores; never
wrap or saturate to storage bounds. Zero/negative HP is healable if no terminal
condition; a still-nonpositive result remains Unconscious. First Aid does not
clear Sleep, Poison, Disease or any other condition. An asleep recipient can
gain HP and remain asleep. At maximum, a healthy target spends SP for no HP
gain. Above maximum, including an M35 well surplus or a lowered Disease maximum,
First Aid neither adds HP nor destroys the surplus. Repeats apply this predicate
to the new live state; no once-per-day marker or remembered maximum exists.

Awaken clears condition 8 on every active owner, including a dead/simultaneously
conditioned owner. It clears condition 12 iff that owner's HP>0, **without**
First Aid's terminal check. HP/SP of recipients and every other condition are
unchanged. Dead/Stoned/Eradicated remain; removing Sleep never revives them.
Unconscious at HP<=0 remains. Fully awake targets and an already awake party
still consume SP and owe time. The unusual positive-HP terminal/Unconscious
combination belongs in synthetic rule tests; M36 does not relax Journey's
existing saved-state constraints merely to make it reachable.

## Cast lifecycle, costs and cancellation

Flow owns presentation of `ChooseCaster -> BrowseLearned -> ConfirmCast` and,
for First Aid after commitment, `ChooseTarget`. EncounterFlow owns one retained
casting continuation and authoritative publications. A phase cannot be entered
by assigning a public observation. No callback can submit a prepared delta as
authority.

| Response/boundary | SP/effect | Time, ctr24 and actor obligation |
| --- | --- | --- |
| Open, browse, choose caster/spell | None | None |
| Incapable/no book/unknown effect/insufficient SP refusal | None; retain truthful selector | None |
| Escape before confirmed cast | None | None |
| Valid ConfirmCast Enter | Debit 1 SP once; install continuation | Ten-minute charge becomes owed |
| First Aid invalid active index or wrong/stale response | None further; prompt stays | Existing obligation remains |
| First Aid valid living target | Publish exact HP/condition delta | Settle charge and pending=3 |
| First Aid valid terminal target | Failed effect; retain debit | Same |
| First Aid Escape from presented target prompt | Refund exact debited 1 SP once; no target effect | Same; cancellation is not a free exploration action |
| Awaken confirmed | Debit, then publish party effect | Same |
| Healthy/no-op/repeated cast | Normal debit and bounded/no-op effect | Same |
| Explicit quit/fatal failure after debit | No invented refund or effect | No Quiet/capture; prior disk save survives |

`SpellOnWho` refunds only explicit target Escape; `firstAid` failure does not.
An insufficient-resource `castSpell` result loops inside `CastSpell::show`, and
outer cancellation returns -1, so no `chargeStep` is due from that refusal.
The reference setting `_moveMonsters=1` alone is not a new MMModern scheduled
opportunity. These conclusions derive from the caller chain, not M35.

Commit protocol:

1. Require current presented confirmation, same owners/membership/caster slot,
   class, complete book, capability, SP, purse, context and resource preimages.
   Revalidate supported identity and `canAct()`. Preflight the pure ten-minute
   calendar support boundary before debit. Reaching/crossing unsupported dusk
   1260 refuses without resource/effect mutation. Do not precompute tick RNG.
2. Prepare bounded continuation/result/guard storage and generation increments;
   acquire/retain exclusive Casting work. Consume confirmation authority before
   callbacks. Publish the SP debit and the retained cost/owed-work record together
   with nonthrowing stores. This is the commitment boundary. Record original SP
   and actual debit; a later refund does not recompute cost from current level.
3. First Aid awaits a newly composed, successfully presented target frame.
   Accept F1-F6 or Escape only under that frame's authority. Consume the response
   before preparing the target effect or refund. Invalid indices do not consume
   a valid target choice; stale authority never re-arms it. Awaken requires no
   post-debit selection and proceeds directly to its bounded effect unit.
4. Prepare the exact target delta(s) from the post-debit owner graph, so self
   targeting cannot copy an old character over the new SP. Publish only allowed
   HP/condition fields, or the exact refund SP field. Adopt the prepared successor
   preimage and fixed result before fallible feedback. Awaken publishes its small
   complete distinct-owner effect set as one nonthrowing unit.
5. Service owed charge and actor/presentation work as specified below. Close the
   selector; return through guarded presentation/attachment/receipt ownership.
   No second Enter/acknowledgment is necessary to make a successful effect real.

Technical rule/overflow/owner failures after commitment are not game-level
`Spell failed`: preserve the debit and any published prefix, stop gameplay and
capture. Never disguise them as a refundable target cancellation. A trusted
presentation-only failure may reconstruct the owned prompt/result; it cannot
re-execute confirmation, refund, effect or work. Window close has no automatic
refund/save. M36 needs no persisted in-progress casting because every such state
is unsaveable.

## Input and native presentation authority

C is `CastSpellAction`. It is admitted only from a current presented quiet
contract-7 exploration boundary: no combat or same-cell contact, pending actor
work, projectiles, attachment/round/end/retirement, Event, inventory, item use,
receipt, dispatch/save operation, unresolved presentation or integrity failure.
Already owed work retains priority; an early C is discarded and a fresh C is
required afterward. It cannot queue a cast across contact/combat. Combat and all
legacy domains explicitly refuse the typed action as well as the physical key.

Accepted C immediately acquires Casting work/activity and creates the uncommitted
continuation before rendering ChooseCaster. The lease covers precommit browsing
as well as committed work; a displayed modal never makes the world capture
predicate quiet. Precommit cancellation transfers it directly to guarded return
Presentation, with no resource/time obligation. Confirmation retains that same
exclusive lease rather than briefly releasing/reacquiring it.

Use the existing Flow/SDL loop and scene/text composition. A compact scrollable
learned list is justified now because the original casters know unsupported
Light/Magic Arrow as well as supported spells. It is bounded to 39 learned rows,
six visible at once, sorted by class slot; this is not the complete magic UI.

| Phase | Controls and feedback |
| --- | --- |
| ChooseCaster | F1-F6 selects the displayed active character. Show names, current SP and incapacity/no-book/no-supported-spell reason. Ineligible selections stay here. Escape closes. |
| BrowseLearned | Up/Down selects a row and scrolls; Enter opens confirmation only for an eligible supported learned spell. Unsupported rows show original name and `Not supported`; no cost/effect is invented. F1-F6 changes caster through the same eligibility check. Escape returns to ChooseCaster. |
| ConfirmCast | Show caster, spell, exact 1 SP/0 gems, Single member or Whole party, and ten-minute cost. First Aid explicitly warns that target Escape refunds SP but still spends time. Fresh Enter commits; Escape returns to list for free. |
| ChooseTarget | First Aid only: F1-F6 shows/selects live named recipients with current/max HP and conditions; include caster and terminal members. Warn that terminal targets fail without refund. Escape refunds SP and settles the attempt. |
| Result/settlement | Retain caster/spell, spent or refunded SP, exact target HP changes/cleared conditions or no-op/failure. Show that actor work is pending. Carry the result into immediate combat feedback if contact follows. |

No navigation, I/U, Space events, F Shoot, R Run, new C or F9 can leak through
these phases. Up/Down in the list must be consumed before navigation routing;
Enter is not Begin/acknowledgment outside its current phase. Extend
`canCancelInteraction` so Escape inside casting cancels the relevant phase;
after a target/party effect or refund response is consumed, Escape is absorbed
while mandatory settlement runs and cannot cancel it. Window close can still
terminate the unsaved session. Outside a modal the existing exit behavior remains.
Cosmetic scene updates may
continue, but modal polling is not gameplay time or an actor opportunity.

Bind every response to Flow incarnation, EncounterFlow ticket, casting generation,
phase, selected owner/book/slot, active membership, displayed-input generation
and the **concrete frame token that actually completed SDL presentation**.
Prepared, copied, returned, merely uploaded, or equal-pixel frames are not enough.
`sealFrame`, `framePresented`, SDL's fixed batch generation and all-key held/repeat/
timestamp protection remain the model. C shares that protection; F keys, arrows,
Enter and Escape must also require a fresh key press after a phase change.
One batch cannot choose caster, confirm a spell and pick a target. Direct typed
responses, reentrant callbacks and test adapters get no weaker authority.

A presentation lease spans every modal-to-modal and terminal handoff. Presenting
the target prompt admits only its target response, not saving or ordinary input.
Redraw replaces frame authority but not semantic work. Consume generations before
external callbacks. Stale callbacks cannot release a newer lease, mark newer work
failed or manufacture Quiet. Final output remains readable across immediate
contact; an observation cache is not the continuation owner.

## Publication, exploration time and failure boundaries

### Owners and publication sites

Keep live knowledge/HP/SP/conditions on roster characters and gems on the existing
party treasure/purse projection. World owns actors, accounting and the Journey
RNG; party owns calendar context; Application retains camera/game flags.
EncounterFlow coordinates a noncopyable Casting continuation under a new
`XeenJourneyActivity::Casting` and `XeenCombatBoundary::Work::Casting`; Flow owns
UI phase and concrete input authorization. These are transient coordination,
not another magic engine or gameplay owner.

Use pure `XeenLearnedSpellRules` queries/preparation and a fixed result describing
already published facts. The narrow interface responsibilities are:
`categoryForClass`, `spellForSlot`, `known`, `eligibility`, `prepareFirstAid`,
`prepareAwaken`; EncounterFlow provides guarded begin/confirm/respond/service
methods accessible through Flow, not public mutation by detached results.

There are four separately retained publication boundaries: **cost commitment**,
**target effect or explicit refund**, **calendar/condition/RNG charge**, and
**ordinary actor opportunity/attachment**. These are not one rollback transaction.
All allocations, validation and successor-preimage preparation for each unit
precede its live stores. After stores, adopt exactly the authorized delta; never
recapture callback-mutated state. M31's prepared guard adoption and M35's
continuation ownership are precedents; a new generic transaction framework is
unnecessary. The two actual target shapes justify a shared bounded result/effect
interface, not an open-ended spell VM or general temporary-effect registry.

The continuation retains committed/refunded/effect-settled/time-settled state,
current response authority and any bounded RNG continuation. Repeated service,
redraw, reporting failure, stale cancellation, Flow destruction or cache eviction
cannot repeat a completed unit. Failure after an effect preserves that effect;
failure during an unpublished time/opportunity candidate preserves the earlier
spell publications but publishes none of that candidate's character/actor/RNG
delta. A failed integrity check is monotonic even if bytes are restored later.

### Charge and scheduler sequence

The exact supported order from an admitted Quiet boundary is:

1. Caster/spell/confirmation selection, then SP debit.
2. First Aid target effect/refund/failure, or complete Awaken party effect.
3. Ten-minute `XeenConditionTimeCandidate` on the **post-effect** characters,
   using the existing world RNG continuation. Preserve ctr24. The admitted
   mainland introduces no additional terrain hazard/navigation effect here;
   casting dispatches no manual/automatic Event and does not move the camera.
4. Publish the checked time/condition/RNG unit and arm approach pending=3 if
   survivors remain. Continue with the existing distinct supplied pulse and
   100 ms idle cadence: three countdown decrements produce one normal regional
   movement/ranged opportunity. Do not add the antidote's immediate opportunity
   or call Wait/stepTime as a shortcut.
5. Publish ordinary ranged/movement/classification/activation through existing
   Approach authority. If contact results, transfer straight to Attachment and
   the existing combat lifecycle; caster SP/effect/time are already settled.
   Do not charge again on attachment, replay the cast or reopen its selectors.
6. Settle projectile presentation and any newly eligible existing monster receipt
   before the final presented Quiet frame. A ready collectable receipt existing
   before C must already have settled; ready treasure blocked by selected threats
   and dormant items retain M33/M34 meanings. The spell creates no loot, XP,
   gold or treasure readiness and does not awaken dormant items.

Do not freeze actors to make casting safe: selection occurs only after previous
work has settled, and a committed attempt creates its normal owed opportunity.
Pure spell effects, selection, refunds and refusal draw no RNG. The ten-minute
charge may cross the inherited 480/960 tick; its condition draws precede any new
ranged opportunity draws. Reuse the existing tick quirks and checked domains.
If the tick leaves no surviving targetable member, publish its consequences and
terminal Defeat without arming an opportunity. Required failures remain unsaveable.
No elapsed-time backlog, extra minute, ctr24 increment or automatic sign dispatch
is inferred from modal duration.

The result is nonmodal feedback after settlement; it does not require an extra
acknowledgment to release Quiet. Retain it until the next accepted gameplay
action, and combine it with contact feedback when the cast's opportunity attaches
combat. This bounded presentation adaptation does not reproduce the reference's
blocking error scroll, sound or spell sparkle.

Existing Shoot already demonstrates this charge/pending split. Reuse its checked
time-candidate machinery and ordinary Approach settlement; do not call Shoot or
antidote as fake inputs. A narrow shared charge helper is acceptable if it preserves
the different owners and ordering. Preserve M35's zero-time immediate-opportunity
behavior unchanged. Returning from casting must retain exclusive ownership until
Presentation/Approach/Attachment/Reward has taken over; no saveable Quiet gap
between releasing Casting and creating the successor work.

## Persistence and compatibility

### Version and exact suffix

Retain **envelope v4** and add **Journey schema 7 / content contract 7**. A new
schema is justified by explicit durable spellbooks; a new content contract is
needed so legacy knowledge absence and exploration rules are not reinterpreted.
The envelope framing and common character payload are unchanged. This is the
next schema revision, not a version chosen from milestone number 36.

Keep the complete 6/6 Journey suffix, changing its pair to 7/7, and append:

| Offset from new knowledge block K | Wire field |
| --- | --- |
| 0 | u8 owner count, exactly 30 |
| 1 + 40*i, i=0..29 | u8 owner ID, exactly i in ascending roster order |
| 2 + 40*i .. 40 + 40*i | 39 raw u8 learned flags in original class-slot order |

Let B be the existing Journey suffix start after the base disabled-event list,
and N the existing packed monster weapon+armor record count. World flag 16
remains at `B+1820+5*N`. Knowledge begins at `K=B+1821+5*N`, occupies exactly
**1201 bytes**, and the full schema-7 suffix is **3022+5*N bytes** (N<=12).
The known 7/7 pair itself requires every book present; no per-owner optional
marker, duplicate learned array in `XeenSaveJourney`, current spell or cast state
is serialized. Decode the suffix into the optional members of
`XeenSaveSnapshot::characters`; these are detached transfer values, not owners.
All integers retain existing endian/framing rules; raw learned bytes are not
canonical booleans and may be 0..255. Preserve them exactly.

Require fixed count/order, exact extent/EOF, bounded indices and complete
presence. Existing resistance records, purse, dormant/ready treasure and recovery
flag presence all remain required in 7/7. Adjust treasure remaining-length checks
to include the new block before reading any records. Keep archive fingerprints,
CRC32, size limits, canonical booleans elsewhere, ownership and file-replacement
checks. Unknown/crossed schema/content pairs reject, as do missing/partial books,
duplicate/reordered owners, unexpected knowledge in another domain or extra bytes.

The shared `writeCharacter/readCharacter` prefix must stay byte-identical. Snapshot
validation requires all thirty books present only for 7/7, all absent elsewhere;
encoding a book-bearing snapshot as v2/v3 or old v4 must fail rather than silently
dropping knowledge. Live legacy/ordinary initialization likewise retains absence.
This avoids accidentally changing ordinary saves merely by adding a character
member. `hasSpells` remains its established independent field and wire meaning.

### Legacy domain matrix

| Supported input domain | Required behavior |
| --- | --- |
| Ordinary v1 | Existing narrow original-data fallback for missing item fields only; absent knowledge remains absent. No learned casting. |
| Ordinary v2 | Exact existing payload and ordinary continuation; absent knowledge; no implicit upgrade. |
| Completed Diagnostic27 v3 | Terminal quiescent completion, read-only inspection/R revisit, existing owner safeguards; no learned casting or book backfill. |
| Journey v4 1/1 | Existing Skeleton seed/domain and 1060-byte suffix; absent knowledge. |
| Journey v4 2/2 | Existing expedition/Event collection, Luck/RNG and 1366-byte suffix; absent knowledge. |
| Journey v4 3/3 | Existing 1651-byte regional suffix and support stops; absent knowledge. |
| Journey v4 4/4 | M33 consequences and strict treasure source semantics; absent knowledge. |
| Journey v4 5/5 | M34 Run, casualties and dormant items; absent knowledge. |
| Journey v4 6/6 | M35 Event/well/antidote and recovery flag; absent knowledge. |
| Journey v4 7/7 | Exact explicit thirty-owner books plus all 6/6 consequences; learned exploration casting enabled. |

No migration/relearning option is added. An explicit all-zero saved book in 7/7
stays empty even if original CHR knows First Aid. Saved nonzero unsupported slots
and inactive-owner bytes likewise remain exact; do not compare them to original
knowledge as a restore eligibility condition. The immutable resource manifest
validates mappings/names and original content, not a demand to reseed the saved
mutable owners.

### Capture, fresh restoration and continuation

Capture extends full-graph save authorization without a second path. Unsafe
F9 refuses **before** capture, providers, fingerprint/target work or file I/O.
It cannot finish a cast, refund it, close its UI, settle owed work or queue a save.
Every casting phase, retained cost/response/effect/time/opportunity obligation,
unpresented return, pending actor work, combat, Event and receipt is unsaveable.
Only a fresh F9 at a successfully presented, otherwise valid Quiet boundary saves.

Fresh-owner restore follows `XeenSaveState::restoreJourney`: decode/validate,
prepare unpublished party/world/camera/flags, reconstruct original immutable
map/MOB/EVT/statistics/names, install **saved** books/HP/SP/conditions and other
durable fields, retain provider preimages, validate topology/current state,
preflight presentation, publish once and bind new runtime capabilities. Never
invoke fresh Journey preparation over a saved graph. No original CHR knowledge
provider is needed to restore 7/7; saved books remain authoritative if a
provider is consulted. Original resources still supply immutable content.

Do not serialize caster/target/list selection, selected/current spell, phases,
debited amounts/refund obligations, time/opportunity continuations, results,
leases, frames, revisions or catalog text. A quiet restored Journey begins with
no cast underway. Rebuilding catalog/UI/map caches neither relearns nor casts.
Exact encoded bytes and every owner field are checked before first gameplay input;
uninterrupted and fresh-process branches match after further casts and play. Restoration must not heal, clamp, wake, refund, charge
time, draw RNG, activate/move actors, replay events/treasure or reconstruct a cast
from reduced SP.

## Exclusions and final acceptance

Excluded: combat and quick casting; guild/spell acquisition; Vertigo and new
transitions; Light, durations, buffs and other world-changing or offensive
spells; nonzero gem/scaled costs; new item spells; Rest, resurrection, SP
recovery, services and unrestricted Clouds gameplay. Unsupported learned spells
remain preserved data. General magic is not established.

The complete build and unfiltered CTest suite (97/97) passed, with focused
casting authority, publication and persistence controls. Genuine original-resource
First Aid and Awaken routes, real F9 save, full process exit, fresh-process restore
and uninterrupted/resumed comparisons passed; the relevant M35 process regression
also passed. Independent technical review accepted after focused corrections, with
no remaining BLOCKER, MAJOR or MINOR finding. The final design retains casting
protection through combat/projectile completion and presentation of the resulting
concrete frame; completed Event presentation does not hide casting, and stale
casting feedback does not outlive its intended interval.

Separately, the maintainer personally completed native-SDL physical acceptance
with ordinary keyboard controls: both production spells and their effects,
precommit cancellation, postdebit target cancellation/refund with owed settlement,
unsupported Light refusal, combat C refusal, held/batched input fencing, readable
phases, mandatory settlement, F9 save, full exit, fresh `--load-game`, restored
HP/SP/conditions, and continued casting and ordinary play. Automated/process
evidence, independent technical review and maintainer physical acceptance are
separate acceptance classes.
