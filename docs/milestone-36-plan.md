# Milestone 36 - Learned exploration casting

## Status, objective and acceptance boundary

This is the implementation contract for M36, not an implementation or completion
record. Specification baseline: `e2cabd4209ecefc475f753204c17dd5fa1f9626a`
(Record post-M35 roadmap direction). The planning gate verified `main`, HEAD,
local `origin/main` and direct remote `main` at that SHA, with a clean worktree
and index. Implementation requires separate authorization and a fresh baseline
check under [AGENTS.md](../AGENTS.md).

Implement learned **First Aid and Awaken**, during exploration in a fresh
Regional Journey. Establish reusable character knowledge, class/slot-to-spell
identity, eligibility, SP charging, selection, target response, effect publication,
owed exploration work and exact quiet restart. The result must be playable
through the ordinary native SDL controls on the admitted map-23 mainland.
First Aid is one consumer of this foundation, not a privileged character/item
shortcut. Awaken adds a real party-target consumer and recovery from already
admitted monster-inflicted Sleep without duration or combat-magic machinery.

Fresh `--journey-region` selects content contract 7. Contract 7 inherits all of
contract 6, including its prepared party, mainland, actors, events, well, antidote,
Shoot, Run, consequences and treasure; only learned exploration casting and its
knowledge persistence are new. No new CLI entry, city, original-data mutation,
spell grant, SP grant or recovery service is needed. The accepted
[roadmap](roadmap.md#near-term) supersedes historical successor pointers: M36 is
independently acceptable here; M37 retains Vertigo entry/traversal.

Inherited authority lives in [M28](milestone-28-plan.md#completion-authority-and-retirement),
[M29](milestone-29-plan.md#durable-ownership-and-authority),
[M31](milestone-31-plan.md#ownership-and-journey-integration),
[M32](milestone-32-plan.md#regional-navigation-and-admission),
[M33](milestone-33-plan.md#conditions-and-the-minimum-time-extension),
[M34](milestone-34-plan.md#surviving-actors-xp-and-treasure) and
[M35](milestone-35-plan.md#selected-well-and-bounded-item-use).
These remain authoritative for their domains; M36 does not re-specify them.

## Targeted evidence and provenance

Evidence labels below distinguish commercial bytes (**ORIGINAL**), pinned
ScummVM algorithms (**REFERENCE**), current MMModern behavior (**IMPLEMENTED**)
and deliberate bounded adaptations/derived conclusions (**INFERENCE**).
Reference source is not independently observed DOS behavior.

The [dependency contract](dependencies.md#pinned-scummvm-revision) pins ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The actual `build/CMakeCache.txt`
configuration resolves source to
`D:/Projetos/MModern/scummvm-known-good-candidate` and external build to
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`; source HEAD and clean status
were checked. Those paths describe this investigation, not required directory
names. Dependency revision, four linked artifacts and configuration are unchanged.

### Character bytes, names and initial consumers

**ORIGINAL:** Read-only `F:/Games/gog/Might and Magic 4-5/XEEN.CC`, through the
initial-container reconstruction used by
[`ScummVmXeenBridge::Impl::initial`](../src/compat/scummvm/ScummVmXeenBridge.cpp), supplies
`maze.chr`: 10,620 bytes, thirty 354-byte records, inner offset 2441, SHA-256
`c1bb681d2a9c5b3b29b2b75ad314328debfd6f512d942d699c4891a5f6da227a`.
The outer block order is `2a0c,2a1c,2a2c,2a3c,284c,2a5c`; outer payloads use
XOR `0x35`, inner payloads are plaintext. No extracted data is added to the repo.
`maze.pty` is 812 bytes; header bytes 0/1 are 6/6, active references at bytes
2..7 are `[0,18,14,11,1,6]`; carried gold/gems at 638/642 are 800/10.

**REFERENCE:** `engines/mm/xeen/character.cpp`, `Character::synchronize`, stores
39 byte-valued learned flags at record offsets **121..159**, Lloyd data at
160..162, `_hasSpells` at 163, signed `_currentSpell` at 164 and `_quickOption`
at 165. The learned index is a **class-category slot**, not a global spell ID
or miscellaneous item spell ID. `getSpellsCategory` maps Paladin/Cleric to
Clerical (0), Archer/Sorcerer to Wizardry (1), Druid/Ranger to Druidic (2), and
other classes to Invalid. `devtools/create_mm/create_xeen/constants.cpp`,
`SPELLS_ALLOWED[3][40]`, maps slots 0..38; slot 39 is the UI sentinel 76
(`None Ready`), not learned storage. `spells.h`/`Spells::executeSpell` use
global IDs 0..75. The exact pinned sources are available at the
[ScummVM commit](https://github.com/scummvm/scummvm/tree/6814ee9ba54582f5b5adcffab49efbbd8f589edd).

**ORIGINAL + REFERENCE mapping:** Nonzero learned slots in the installation:

| Active key / owner | Class | Learned slot -> global ID / name | Original HP/SP | Prepared Regional HP/SP |
| --- | --- | --- | --- | --- |
| F1 / 0 Arturius | Paladin | 21 -> 42 Light | 12/2 | 36/6 |
| F2 / 18 Tyro | Knight | none | 16/0 | 48/0 |
| F3 / 14 Badger | Ranger | 1 -> 1 Awaken; 20 -> 42 Light | 12/2 | 36/6 |
| F4 / 11 Zippo | Robber | none | 10/0 | 40/0 |
| F5 / 1 Rebecca | Cleric | 1 -> 1 Awaken; 14 -> 26 First Aid; 21 -> 42 Light | 7/7 | 21/21 |
| F6 / 6 Seymour | Sorcerer | 0 -> 1 Awaken; 22 -> 42 Light; 25 -> 45 Magic Arrow | 5/9 | 15/27 |

All other original roster records have zero learned flags. The original current
spell bytes for owners 0/1/6/11/14/18 are 21/21/22/255/20/255: defaulting to
them would select unsupported Light for every caster. Original known flags here
are exactly 1. **IMPLEMENTED:** `XeenActorApproach::initializeJourney` retains
the admitted preparation: active levels `[3,3,3,4,3,3]` in active order, year
610/day 8/minute 480/ctr24 0, current HP/SP initialized from existing live maximum
rules. This is existing production preparation, not an M36 test grant. Original
knowledge must survive it unchanged.

**ORIGINAL:** `DARK.CC/spells.xen`, resource ID `0x64b2`, archive offset 391041,
contains 77 NUL-terminated names, 937 bytes, CRC32 `0x63568f11`, SHA-256
`3e64cc674720401c58dcf5c57ddaaffd46fc6eb4af2b9a78934c26fa0f7bec1e`.
Entries 1/26/76 are Awaken/First Aid/None Ready. This World-of-Xeen installation
has no `spells.cld` resource in the two gameplay archives. **REFERENCE:**
`Spells::load` reads `spells.xen` from archive side 1 for this profile; the
Clouds-only profile uses `spells.cld`. M36 admits the existing WorldOfXeenClouds
profile only and uses `DARK.CC/spells.xen` explicitly.

### Algorithms and integration precedents

| Label | Evidence site | Decision established |
| --- | --- | --- |
| REFERENCE | `spells.cpp`: `subSpellCost`, `addSpellCost`, `castSpell` | Check SP then carried gems; subtract before dispatch. Negative SP coefficients scale by current level. Neither selected spell uses that scaling. |
| REFERENCE | `dialogs/dialogs_spells.cpp`: `CastSpell::show/execute`, `SpellOnWho::execute` | Incapacity blocks casting; insufficient cost returns to selection. Escape at a target prompt refunds SP/gems; returning from that spell still counts as a completed cast attempt. |
| REFERENCE | `spells.cpp`: `firstAid`; `character.cpp`: `isDead`, `addHitPoints` | First Aid targets one active member; terminal conditions fail after debit; otherwise bounded +6 HP and conditional Unconscious clearing. |
| REFERENCE | `spells.cpp`: `awaken` | Clear Sleep on every active member; clear Unconscious when HP>0, even with another terminal condition. No `isDead` test here. |
| REFERENCE | `interface.cpp`: exploration `KEYCODE_c`, `chargeStep`, `doStepCode`, `stepTime` | Completed attempt calls `chargeStep` plus `doStepCode`, not `stepTime`: ten outdoor minutes, delayed actor work, no ctr24 increment. Cancelling the cast dialog before dispatch has no such charge. |
| IMPLEMENTED | `XeenCharacterFormat::parseRoster`, `XeenCharacter`, `XeenPartyLoader` | Only `hasSpells`, SP and existing rule inputs are modeled; learned flags and current spell are currently discarded. All thirty characters belong to roster; active references are aliases. |
| IMPLEMENTED | `XeenJourneyFlow` item-use continuation; `XeenInventoryFlow`; `XeenEventFlow::sealFrame/framePresented/handle`; SDL | Existing leases, generations and concrete presented-frame authority are reusable; M35 cost/opportunity rules are not reusable spell semantics. |
| IMPLEMENTED | `XeenEncounterFlow::serviceShoot` in `XeenJourneyConsequences.cpp`, `XeenActorApproach::regionalTransition`, `XeenRegionalActionCandidate` | Existing checked ten-minute tick, world RNG and pending=3 scheduler can settle casting without another clock or actor owner. |
| IMPLEMENTED | `XeenStateEquality`, `XeenRestoreGuard`, `XeenJourneyCapture`, `XeenSaveState/Format` | Exact character preimages already flow through gameplay, save and fresh-owner restore. The common character wire prefix has no learned state. |

**INFERENCE:** The two selected spells are sufficient. First Aid gives an
affordable, useful HP consumer; Awaken exercises party effects, three existing
learned categories and a currently unrecoverable admitted condition. Light would
require duration/world-light state; Magic Arrow would introduce offensive spell
targeting/damage. Neither is necessary. Do not substitute the antidote's
miscellaneous spell ID 37 or First Aid's item spell ID 4 for learned global ID 26.

## Knowledge, identity and resource model

### Authoritative representation

Add `XeenLearnedSpells` as a fixed `array<uint8_t,39>` value and an optional
`learnedSpells` member on `XeenCharacter`. **Present** means an explicitly loaded
or saved spellbook, including all-zero; **absent** means this domain does not
model knowledge. Preserve raw flags, including noncanonical nonzero values;
query `raw[slot] != 0` for knowledge. This follows the reference boolean use
without losing original bytes or conflating absence with ignorance. There is
no parallel spellbook map on Flow, party or world and no acquisition API in M36.

`XeenCharacterFormat::parseLearnedSpells(bytes, owner)` is a bounded inert parser:
require exactly `30*354` bytes and owner 0..29, return offsets 121..159 without
changing `hasSpells` or any other field. Keep ordinary `parseRoster` behavior
unchanged. Fresh contract-7 Journey preparation explicitly loads all thirty
books from the same retained CHR bytes already used for supplements, prepares
them in the detached candidate and publishes them in the guarded initialization.
All other fresh entry domains retain absent knowledge. Do not load only active
casters or initialize a book on opening the UI.

Keep four separate queries:

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

Use category+slot for lookup and a typed global identity for effect dispatch.
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

Update `xeen_state::sameCharacter`, test snapshot equality and any hand-written
character comparisons to include optional presence and all 39 bytes. Existing
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

Add a small immutable learned-spell catalog/rules module. Adapt precisely the
three 39-entry mapping rows from pinned `SPELLS_ALLOWED` (exclude the sentinel),
with GPL attribution and pinned path/SHA. Include only the two supported effect
descriptors and their numeric costs from `SPELL_COSTS`/`SPELL_GEM_COST`.
This is a finite lookup, not a survey/implementation of other spell effects.
Test table extents, identity bounds, per-category uniqueness and selected mappings.
No ScummVM engine construction, new linked library, full `mm.dat` decoder or
additional build dependency is admitted.

Add an explicit read-only `DARK.CC/spells.xen` accessor through the existing
asset/bridge owner, analogous to monster statistics/material names. Parse exactly
77 terminated strings with no trailing bytes; enforce archive extent and bounded
nonempty display names (at most 63 bytes each). Retain original bytes and use the
existing font/text renderer; do not copy commercial names into source or saves.
Contract-7 resource admission checks size 937 and CRC32 `0x63568f11` through
existing zlib support; SHA-256 above is provenance, not a new runtime dependency.
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

Add C as `CastSpellAction`. It is admitted only from a current presented quiet
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
timestamp protection remain the model. Add C to that protection; F keys, arrows,
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
already published facts. Suggested narrow interface responsibilities are:
`categoryForClass`, `spellForSlot`, `known`, `eligibility`, `prepareFirstAid`,
`prepareAwaken`; EncounterFlow provides guarded begin/confirm/respond/service
methods accessible through Flow, not public mutation by detached results.
Keep exact names consistent with local conventions during implementation.

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

Extend current full-graph save authorization, not a second capture path. Unsafe
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
provider is necessary to restore 7/7; test that a provider cannot replace the
saved books. Original resources still supply the existing immutable content.

Do not serialize caster/target/list selection, selected/current spell, phases,
debited amounts/refund obligations, time/opportunity continuations, results,
leases, frames, revisions or catalog text. A quiet restored Journey begins with
no cast underway. Rebuilding catalog/UI/map caches neither relearns nor casts.
Verify exact encoded bytes and every owner field before the first gameplay input,
then compare uninterrupted and fresh-process branches after identical further
casts and ordinary play. Restoration must not heal, clamp, wake, refund, charge
time, draw RNG, activate/move actors, replay events/treasure or reconstruct a cast
from reduced SP.

## Implementation boundaries and likely files

Implement in dependency order, with targeted deterministic checks at each boundary.
These are engineering steps within M36, not authorization to start another milestone.

1. **Knowledge and resource identity:** character optional value and bounded CHR
   parser; immutable mapping/descriptors and explicit name accessor/parser;
   equality and provider guards. Likely files: `XeenCharacter.h`,
   `XeenCharacterFormat.*`, `XeenStateEquality.h`, asset/bridge files and new
   `XeenLearnedSpellRules.*`/small catalog support. Keep legacy parser behavior.
2. **Pure rules and prepared publications:** two target shapes, checked HP/SP
   behavior and fixed results. Reuse `XeenCharacterRules` and existing condition
   fields. Introduce no abstract effect hierarchy, item-spell unification or RNG.
3. **Contract-7 lifecycle/persistence:** explicit content predicates in
   `XeenJourneyContent`, fresh initialization, party-domain validation, guards,
   `XeenSaveSnapshot/State/Format`, capture and fresh-owner restore. Audit exact
   `contract==6` and `schema==6` gates in Event, item use, text admission, world,
   treasure and Application so 7 inherits 6 while every legacy pair stays closed.
   Use explicit supported-pair checks, not unbounded `>=7` future admission.
4. **Casting coordination and SDL:** `XeenEncounterFlow`, `XeenJourneyFlow`,
   `XeenEventFlow`, `XeenJourneyConsequences`, `PlayerAction`, `XeenGameplay` and
   `SdlWindow`; add the transient work/activity, presentation/selection state,
   cast/refund/effect publications and existing time/actor settlement. A dedicated
   `XeenCastingFlow.cpp` implementation unit within these owners is reasonable;
   no second live Flow or nested loop. Verify direct APIs as well as SDL routing.
5. **Production integration and acceptance:** CMake test registration, existing
   CLI/resource/process witness extensions and restore replay probes. Add a
   focused M36 witness adapter analogous to `XeenM35CliWitness`, with contract 7
   explicitly selected; the retained M33 adapter forces 4 and is not an M36 test.
   Update public controls/durable docs only at authorized implementation closure.

No public callable grants of learned spells, debug cast commands, test hooks in
production owners or new runtime dependencies are required.

## Acceptance and planned verification

The four acceptance classes are separate. Nothing below claims future M36 tests,
review or physical acceptance have passed.

### 1. Automated deterministic validation

Use the character, Journey gameplay/Event, publication/authority, regional
persistence and save families registered by [CMakeLists.txt](../CMakeLists.txt).
Add focused learned-rule and casting-flow tests where those families do not
express the boundary. Preserve the M35 antidote suite as a distinct regression.

| Area | Required controls |
| --- | --- |
| Parsing/knowledge | Exact 30x354 extent and offsets; owners 0/29 and invalid owner; present zero book versus absent; raw 1/2/255; inactive books; all class categories, every slot bound, unsupported learned slots, current byte 255/invalid ignored; aliases resolve to the same owner. |
| Eligibility/cost | Capability, learned and supported predicates independent; every disabling worst condition and simultaneous-condition precedence; SP -32768/-1/0/1/32767 and above maximum; level/class changes do not scale either cost; gems 0/10/u32 maximum unchanged; invalid domain/caster/slot never debits. |
| Effects | First Aid self/other/alias; negative/zero HP, min(H+6,M), H=M, H>M, live equipment/Disease maximum, Unconscious threshold, Sleep retained, all terminal conditions/no refund, repeated and healthy casts, checked storage overflow. Awaken all active owners, inactive exclusion, alias deduplication, Sleep+Poison/Disease/Unconscious/Dead combinations, HP-positive conditional clearing, all-awake no-op. |
| Cancellation and units | Escape at each precommit phase; postdebit target Escape exact once-only refund but ten-minute charge; invalid target retains prompt; repeated refund/effect responses; failure before debit versus after debit/effect/time; allocation and reporting faults; no transaction-wide rollback or lost cost obligation. |
| Authority/input | Owner replacement/destruction/ABA, book/class/membership/SP/purse/context mutation, reentrant providers/reporters, generation overflow, copied observations, copied/equal-pixel/merely uploaded frames, wrong frame/phase/owner, stale F9, batched C/F5/Enter/Enter/F6, held/repeat/timestamp input, stale callbacks against newer work. |
| Settlement | Exactly +10 minutes and unchanged ctr24 on success, failure, no-op and refunded target cancellation; zero on precommit refusal/cancel; 950->960 tick after effect/refund and before ranged RNG; 1250+10 refused before debit; checked draw continuation/failure; pending=3 and one opportunity, immediate contact, postcast combat uses changed state, defeat, receipts and dormant treasure, no automatic Event invented. |
| Exclusions/restore | Explicit C refusal in combat, legacy/diagnostic domains, Event, inventory/U, receipt, approach, save and unresolved presentation; no provider/I/O on unsafe F9; save/restore/cache rebuild never executes casting; continued mutation after restart. |
| Wire | Independent literal schema-7 offsets/length/count/order/CRC; thirty full books including inactive/unknown flags; partial/missing/duplicate/reordered owner records, every truncation boundary, extra bytes, crossed/unknown pairs, wrong optional presence; unchanged common character prefix and legacy golden bytes/domains. |

Include the existing M28/M29 save preimage/reentry tests, M31 Event publication,
M32 regional admission, M33 Shoot/tick/ranged/condition/treasure, M34 Run/legacy
consequence and M35 quest/well/antidote families. Full build and complete CTest
are required at implementation closure, with no failing test accepted as closure.

### 2. Genuine original-resource and process evidence

Use the legal external installation and real Application/CLI/Flow/scene/resource
construction. Witness adapters supply typed input and a controlled presentation
clock, not owner mutations, fabricated injuries, spell/resource grants, frozen
actors or substitute effect functions. They must complete the concrete-frame
handoff. Original DOS behavior is not claimed by these MMModern witnesses.

The planning investigation read original CHR/PTY/names in memory and inspected
the existing source/test chains. Read-only runs of the existing
`mmodern_consequence_cli_witness` established these **inherited contract-4
gameplay prefixes**, using its normal lowest-record contact target/Attack policy,
normal automatic work, 100 ms pulses, and no equipment changes:

| Seed / ordinary input prefix | Observed settled state before magic |
| --- | --- |
| 1 / `UFU` | `(7,11)` West, minute 511, ctr24 2, RNG state 3323190024/count 18; Seymour HP 11/15, SP 27; Rebecca HP 21, SP 21; actor 9 defeated. |
| 7 / `LUUURUULURUULUUU` (16 inputs) | `(5,4)` South, minute 592, ctr24 16, RNG state 3210387611/count 62; Rebecca HP 4/SP 21/Sleep 1, Badger HP 21/SP 6/Sleep 1; Seymour HP 15/SP 27/able; Toad record 15 defeated, record 14 alive at `(5,2)`. |

Here U=Forward, L/R=TurnLeft/TurnRight and F=Shoot; each input waits for ordinary
work and combat to settle before the next. In combat, select the lowest original
record among contacts and Attack for each ready member; acknowledge receipts.
These probes supplied no save target and wrote no saves: their final snapshot
footer refused `Unsupported Journey durable state` because Application omits
archive fingerprints without a target. They are resource/gameplay setup evidence,
not a passed process/save test or proof of the new contract. Rebuild the witness
against the implementation and verify these prefixes under contract 7; unchanged
pre-casting behavior is required. No acceptance claim rests on a prebuilt binary.

**Required primary route (First Aid):**

1. Start the real executable with
   `--journey-region --combat-seed 1 <installation> --save-file <external-save>`.
   Verify contract 7, original thirty books and the prepared HP/SP table. The
   save path is outside the commercial installation. Reach the first prefix
   above with ordinary controls; no Myra/Phirna/well interaction is needed.
2. C, F5 Rebecca; browse to First Aid (slot 14, global 26), Enter to confirmation,
   a fresh Enter to commit, then a newly presented F6 target choice. Observe
   Rebecca SP **21->20**, Seymour HP **11->15**, gems **10->10**, and no unrelated
   spell effect. The result must identify the selected caster/target.
3. Settle the ten-minute charge **511->521**, ctr24 remaining **2**, and the
   normal opportunity/projectile/contact/treasure work. Capture effect-time and
   post-opportunity observations separately so any later damage is not mistaken
   for a failed heal. Require a presented Quiet checkpoint; finish any genuinely
   attached combat normally before F9.
4. F9, full process exit, then launch a fresh process with
   `--load-game <installation> <external-save>`. Before first input compare exact
   save bytes/full semantic state, including books, SP, HP, conditions, all actors,
   treasure, context and RNG against the uninterrupted branch. Probes must show
   no initialization/casting/refund/time/RNG/event/treasure replay.
5. In the resumed process cast First Aid again on the same active target, using
   its live HP predicate, then make an ordinary turn and forward/backward movement
   within the mainland, settle work and save again. Execute identical input in
   the uninterrupted control and compare bytes/state after this continuation.
   The further cast may be a no-op heal; it must still spend exactly one SP and
   service its ten-minute/opportunity obligations. No reseeding or SP refill.

**Required party-effect route (Awaken):**

Start independently with seed 7 and the second prefix, stopping after the 16th
input, before the next southward step. Use C, F6 Seymour, Awaken, confirmation.
Observe SP **27->26**, both original Sleep bytes **1->0**, their HP unchanged at
effect publication, and the whole active condition delta. Settle **592->602**,
unchanged ctr24 **16**, and the ordinary opportunity. The living nearby Toad
must remain active; if it contacts, the healed/woken owners enter normal combat
and later Sleep/damage is a new consequence. Finish mandatory combat/receipts,
reach Quiet, save, exit and fresh-load; compare with an uninterrupted control.
Continue with Rebecca's now-eligible First Aid if she remains able, otherwise
Seymour's or Badger's eligible learned cast after ordinary settlement. The
separate primary route already requires First Aid after fresh restore; this
route must prove the party effect and ensuing genuine actor consequences.

For both routes, the implementation witness records the complete post-casting
literal trace and saves, not just the two displayed scalars. Extend the existing
`XeenM35CliWitness` style with distinct new stages and the
[`XeenRestoreReplayProbe`](../tests/XeenRestoreReplayProbe.h) linker approach to
observe new cost/effect/settlement sites. Require real file capture and separate
process invocations via the existing process-test approach; helper round-trips
alone are insufficient. Keep synthetic failure injections, alias fixtures,
overflow, terminal recipients and artificial tick/contact setups explicitly
separate from these resource-derived routes.

### 3. Independent technical review

Before closure, an independent reviewer checks the implementation and tests
against this contract and exact candidate SHA, especially learned versus capable,
target refund versus time charge, Awaken's terminal-condition quirk, self-target
SP preservation, concrete-frame input, partial publication, all-thirty-owner
persistence, legacy absence and fresh-process no-replay continuation. Correct
material findings and obtain acceptance before recording stable completion.
Planning approval and this document do not substitute for that review.

### 4. Maintainer native-SDL physical acceptance

The maintainer personally runs the primary route with ordinary keyboard controls,
reads the caster/list/cost/target/result panels, observes actual injury recovery,
waits for actor settlement, saves with F9, exits the process, fresh-loads and
casts/plays again. Also exercise the party-effect route's Sleep clearing and
nearby actor continuation, precommit Escape, First Aid target Escape (SP refund
but time spent), unsupported Light selection, held/batched keys and combat C
refusal. Check panel readability at 320x200, highlight/scroll behavior, changing
phase controls and result visibility across contact. Automated SDL, screenshots
or reference reading cannot be recorded as physical acceptance.

## Exclusions and closure criteria

Excluded: combat magic and quick casting; guild/spell acquisition; Vertigo or
any new map transition; Light/duration/buffs and general temporary effects;
movement/world-changing/offensive spells; nonzero gem costs and scaled-cost
consumers; new item spells; Rest/temple/resurrection/SP-recovery services;
progression/economy; a complete magic dialog, original audio/sparkle fidelity,
Clouds-only packaging/localization and unrestricted Clouds gameplay.
Original learned unsupported spells remain data, not newly enabled behavior.

M36 closes only when both supported spells work through the production path,
the primary save/fresh-process/further-casting route and party-effect route pass,
required regressions/full build/CTest pass, independent technical review accepts,
and maintainer physical acceptance is recorded separately. Then, under closure
authorization, update stable status/history/public controls and condense this
plan to durable decisions/results. There are no unresolved maintainer product
choices in this contract. Do not mark M36 complete or start M37 from planning
completion; no staging, commit, push or tag is authorized by this specification.
