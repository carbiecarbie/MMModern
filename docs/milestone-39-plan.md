# Milestone 39 - Bounded already-learned combat casting

## Specification status and scope

**Technical specification candidate for independent review.** This document
specifies work; it records neither implementation authorization nor completed
gameplay acceptance. Planning baseline: `main`, commit
`0a76c958d8e77e26572b8bf53e949b7267700265`, whose only changed file is the approved
[roadmap](roadmap.md). Local HEAD, `origin/main` and direct remote main were
verified equal, with an empty index and clean worktree before investigation.

M39 connects existing learned books and finite current SP to **Magic Arrow,
First Aid and Awaken in combat**, on the already admitted Regional Journey
mainland and Vertigo encounters. The useful boundary includes following combat
work, wounds/lethals and existing consequences, quiet save, complete process
exit, fresh restore and further play. Fresh Regional Journey selects **envelope
v4 / schema 8 / content 10**. Existing content 9 and every older domain retain
their existing behavior, including combat Cast refusal.

The accepted order remains M39 casting, M40 service-day continuation, then M41
Training/progression. No investigation finding requires reopening that choice.
The closed M38 plan/status's older next-direction wording describes its closure;
the later approved roadmap establishes the present planning sequence.

Inherited contracts have these natural homes:

- [M36 knowledge, mappings and resource model](milestone-36-plan.md#knowledge-identity-and-resource-model)
  and [pure recovery effects](milestone-36-plan.md#hp-and-condition-semantics).
- [M33 combat lifecycle](milestone-33-plan.md#regional-ownership-attachment-and-combat-lifecycle),
  [condition/time](milestone-33-plan.md#conditions-and-the-minimum-time-extension),
  [treasure](milestone-33-plan.md#gold-monster-treasure-and-equipment-closure)
  and [frame authority](milestone-33-plan.md#semantic-input-and-concrete-frame-authority).
- [M34 participation](milestone-34-plan.md#individual-run-and-partial-party-rules),
  [non-victory lifecycle](milestone-34-plan.md#lifecycle-and-non-victory-completion)
  and [identity accounting](milestone-34-plan.md#surviving-actors-xp-and-treasure).
- [M37 retained-region ownership](milestone-37-plan.md#durable-ownership-and-transition-publication)
  and [city reset](milestone-37-plan.md#exact-scripted-actor-reset).
- [M38 route](milestone-38-plan.md#exact-route-event-graph-and-city-consequences),
  [service boundary](milestone-38-plan.md#time-departure-and-scheduling),
  [authority](milestone-38-plan.md#owners-authority-and-publication) and
  [schema-8 representation](milestone-38-plan.md#exact-successor-wire).

Only the explicit changes below supersede inherited behavior. In particular,
the actual exploration caster charges **ten minutes on the mainland and one in
Vertigo**, followed by its existing settlement. Preserve that code path,
including refunds and time/actor obligations. Historical fixed UI text saying
ten minutes is neither a new time rule nor evidence about combat casting.

## Evidence and provenance

### Evidence classes and decisive sources

**Current MMModern facts** were established from the baseline code/tests, not
inferred from older plans. `XeenCombatCommand` currently has Attack/Block/Run;
`XeenCombat` owns initiative, participation, consequences and tickets.
`XeenCastingFlow.cpp` instead owns the separate exploration continuation.
`XeenLearnedSpellRules::supported/eligible` currently recognizes only global
IDs 1 and 26 and has no context parameter. Simply adding ID 45 there would be
an unsafe support expansion. `XeenJourneyContent.h` admits exactly 1/1 through
8/8 plus 8/9. Combat's `Impl::exact` expects Journey activity **Combat**, and
`current(Ticket)` includes the boundary generation; both facts constrain the
modal design below.

The ScummVM source directory was discovered from configured CMake cache values,
not from its name: `D:/Projetos/MModern/scummvm-known-good-candidate`, with
separate configured build `D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`.
Its HEAD was verified as
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` and its worktree was clean. A
command-local safe-directory exception permitted the read-only Git check under
the sandbox account; no Git configuration was changed. The authoritative
dependency/configuration contract remains [dependencies.md](dependencies.md).

**Pinned-reference behavior** comes from that revision, principally:

| Path relative to ScummVM | Decisive symbols and findings |
| --- | --- |
| `devtools/create_mm/create_xeen/constants.cpp` | `SPELLS_ALLOWED`, `SPELL_COSTS`, `SPELL_GEM_COST`: identities, class slots and fixed costs |
| `engines/mm/xeen/spells.cpp` | `castSpell`, `subSpellCost`, `addSpellCost`, `magicArrow`, `firstAid`, `awaken`: cost, dispatch, effect/failure behavior |
| `engines/mm/xeen/dialogs/dialogs_spells.cpp` | `CastSpell::show/execute`, `SpellOnWho::show/execute`: acting caster, selection cancellation, participant target domain, explicit refund |
| `engines/mm/xeen/interface.cpp` | combat C branch, `nextChar`, round/End `changeTime(1)`, separate exploration `chargeStep`: action consumption and absence of a cast-specific combat minute |
| `engines/mm/xeen/combat.cpp` | `rangedAttack`, `attack`, `attack2`, `monsterSavingThrow` macro, `getMonsterResistance`, `giveExperience`, `setupCombatParty`, `setSpeedTable`: selected contact, resistance/draw order and consequences |
| `engines/mm/xeen/combat.h`, `interface_scene.cpp` | `POW_ARROW=11`, `IndoorDrawList`, `OutdoorDrawList`, projectile frames/anchors: bounded original picture reuse |
| `engines/mm/xeen/character.cpp`, `map.cpp` | `addHitPoints`, condition checks, `MonsterStruct::load`: recovery predicates and raw magic resistance at record offset 39 |
| `engines/mm/shared/xeen/cc_archive.cpp`, `engines/mm/xeen/files.cpp` | CC name/index/payload decoding and `SaveArchive::reset`: read-only original initialization provenance |

The important caller distinction is that `magicArrow` sets `_rangeType=RT_SINGLE`
but `rangedAttack` calls `attack(..., RT_GROUP)` for the selected contact before
stopping after one target. Treating it as physical `attack(..., RT_SINGLE)` would
select the wrong damage and resistance path. `POW_MAGIC_ARROW=12` is **not** the
sprite selected by the learned `magicArrow()` function; that function uses
`POW_ARROW=11`.

This is ScummVM interpretation, not independently observed DOS behavior. The
stable-slot adaptation, explicit confirmations, result acknowledgment and
bounded contact projectile are **MMModern decisions**. Seeds, input injection,
raw RNG tapes, fault seams and synthetic state fixtures are **artificial
controls**, never original-resource facts. Adapted numerical logic must retain
ScummVM attribution and GPL-3.0-or-later provenance.

### Original resource findings obtained during planning

Read-only in-memory decoding of the external installation independently checked
the following. No extracted payload was saved or added to the repository.

| Original member | Size | Decoded CRC32 | Relevant result |
| --- | ---: | --- | --- |
| Initial `maze.chr` | 10620 | `81a2dd16` | Thirty 354-byte records; learned flags at 121..159 |
| `DARK.CC/spells.xen` | 937 | `63568f11` | 77 terminated names; IDs 1/26/45 are Awaken/First Aid/Magic Arrow; 42 is Light |
| `DARK.CC/xeen.mon` | 5400 | `4dec3f50` | Ninety 60-byte records; selected admitted monster profiles retain their checked identities |
| `XEEN.CC/pow11.icn` | 451 | `67f7f690` | Three frames; existing player projectile resource |

Initial CHR means the plaintext inner member of the initial archive assembled
in `ScummVmXeenBridge::initial` from outer blocks
`2a0c,2a1c,2a2c,2a3c,284c,2a5c` in that order, omitting absent blocks as the
existing adapter does. It does **not** mean a direct outer `maze.chr` member or
the player's mutable `XEEN.CUR`.

| Active slot / roster owner | Original class | Original nonzero learned slots | Raw initial HP/SP | Prepared Journey HP/SP |
| --- | --- | --- | --- | --- |
| 0 / 0 Arturius | Paladin | 21 (Light) | 12/2 | 36/6 |
| 1 / 18 Tyro | Knight | none | 16/0 | 48/0 |
| 2 / 14 Badger | Ranger | 1 (Awaken), 20 (Light) | 12/2 | 36/6 |
| 3 / 11 Zippo | Robber | none | 10/0 | 40/0 |
| 4 / 1 Rebecca | Cleric | 1 (Awaken), 14 (First Aid), 21 (Light) | 7/7 | 21/21 |
| 5 / 6 Seymour | Sorcerer | 0 (Awaken), 22 (Light), 25 (Magic Arrow) | 5/9 | 15/27 |

Raw values are original facts; the right column is the inherited MMModern
`XeenActorApproach::initializeJourney` preparation, observed in the diagnostic:
levels `[3,3,3,4,3,3]`, inherited XP preparation, cleared temporary values and
initial maximum HP/SP. M39 adds no new refill or preparation adjustment.

Magic resistance is raw byte 39, distinct from physical resistance at 40.
Original Slime 0, Giant Snake 3, Orc 6, Skeleton 8, Zombie 9 and Giant Toad 13
all have magic resistance zero. Skeleton/Zombie physical resistance is 50;
the other four are zero. Planning rechecked records 0/3/6/8/9/13; Slime CRC is
`4743814e`, Orc `eb3b54b1`, Toad `5636ae25`. This zero magic-resistance observation
removes the first draw in production but **does not remove the later saving
throw draw**. Nonzero resistance belongs in pure synthetic rules tests without
loosening original profile admission.

## Spell rules and support admission

### Identities, costs and eligibility

| Spell / global ID | Clerical slot | Wizardry slot | Druidic slot | Current SP / gems | Combat target |
| --- | ---: | ---: | ---: | --- | --- |
| Awaken / 1 | 1 | 0 | 1 | 1 / 0 | All distinct active roster owners |
| First Aid / 26 | 14 | absent | 11 | 1 / 0 | One remaining participant, including self |
| Magic Arrow / 45 | absent | 25 | 23 | 2 / 0 | One identity-bound live contact |

Keep the existing categories: Paladin/Cleric are Clerical, Archer/Sorcerer
Wizardry, Druid/Ranger Druidic. Other classes grant no category. Knowledge is
an explicitly present 39-byte book and a nonzero byte at the mapped class slot;
raw values 1..255 are equally known and remain lossless. Slots 39 and global
76 are sentinels, not effects; reject out-of-range indexes before access.

Cost commitment requires all of: content 10, existing admitted combat,
reservation from the presented `PlayerReady` acting slot, set participation bit, unchanged
active-owner binding, `hasSpells`, valid class category, learned supported slot,
`canAct()` and positive effective combat Speed, checked rule inputs and enough
signed current SP. Use the existing `canAct()` worst-condition semantics;
do not replace them with an approximate list, maximum-SP test or independent
HP-positive requirement. Journey canonical state remains independently checked.
An unsupported equipment/encounter domain does not become admissible because
the selected spell would ignore a weapon.

Costs do not scale with level. Check `SP >= cost` before widened subtraction
and checked i16 storage. Negative/zero SP refuses. Above-maximum SP is legal
within inherited state constraints and debits exactly without clamping. Gems,
carried/pending gold and all treasure bytes remain unchanged by spell cost or
refund, including zero and `UINT32_MAX` gems. No bank, item charge, equipment,
guild or purchase prerequisite exists.

Split **effect recognition** from **support admission**. The pure catalog may
recognize three effects, but use explicit context/content queries at the UI
and authoritative commit sites:

| Domain | Supported learned effects |
| --- | --- |
| Content 10 combat, mainland or admitted Vertigo | 1, 26, 45 |
| Content 7/8/9/10 eligible exploration | Existing 1 and 26 only, with inherited geographic admission |
| Content 1..9 combat, diagnostics, ordinary gameplay | None |
| Any context: Light or another learned row | Visible when known, inert/unsupported |

Do not let a global `supported(45)` implicitly authorize exploration, old
content or a direct typed-action seam. An explicit `combatCasting()` capability
is true only for content 10. Content 10 explicitly inherits content 9's other
capabilities; audit predicates such as `armorRepair()`, `schema()`, `vertigo()`
and all pair-sensitive code instead of admitting a new version range.

### Distinct membership and target domains

Active membership remains `[0,18,14,11,1,6]`; a combat episode's six-bit mask
records escape, not death or roster removal. Recovery never sets a cleared
participation bit. Group joining retains that mask. Only a new attached episode
after proper retirement reconstitutes participation under M34.

- **Action eligibility** requires participation, `canAct()` and positive Speed.
- **Enemy targetability/continuing combat** still uses `xeenCombatTargetable`
  over participants; full-party defeat evaluates the full active membership.
  A sleeping member remains targetable. Healing targets need not satisfy this
  predicate.
- **First Aid targetability** follows reference `SpellOnWho` in combat mode 2:
  the remaining combat-party view, including sleeping, paralyzed, unconscious
  and terminal participants. Escaped or inactive owners refuse selection. Keep
  stable F1..F6 slots in MMModern, visibly disabling escaped slots instead of
  compacting controls and accidentally selecting another owner.
- **Awaken effect membership** follows `Spells::awaken`'s full active party,
  including escaped and terminal owners. Clearing their conditions grants
  neither participation nor an extra action.
- **XP eligibility** remains the participant/condition intersection at lethal
  publication (`xeenCombatXpEligible`); Sleep and Unconscious may qualify,
  terminal worst conditions and escaped slots do not. Treasure delivery uses
  its separate full-active, `canAct()` and capacity predicates.

Shared pure effects resolve active references to roster identities. Self and
aliases of self refer to one owner; party effects deduplicate owners in first
active-reference order. Aliases remain shared-helper test cases, not new
production membership admission. The First Aid combat adapter validates the
stable participant slot before calling the pure effect helper; do not widen its
target domain just because `prepareFirstAid` accepts any active index.

### First Aid and Awaken

Reuse `prepareFirstAid` and `prepareAwaken`'s existing pure HP/condition rules,
with combat-specific target admission above. For clarity, the decisive
predicates from the [M36 effect contract](milestone-36-plan.md#hp-and-condition-semantics)
remain:

- First Aid fails on **any** nonzero Dead(13), Stoned(14) or Eradicated(15):
  target selection is valid, SP stays spent, no target field changes, and the
  action is consumed. It is not a refundable refusal.
- Otherwise, with current HP H and checked live maximum M, H<=M gives
  `min(H+6,M)`; H>M preserves H. Compute in widened arithmetic, reject an
  unrepresentable i16 result, and clear Unconscious(12) iff resulting HP>0.
  This includes above-maximum positive HP. Negative/zero HP can recover; a
  still-nonpositive result preserves Unconscious. Sleep, Poison, Disease and
  all other conditions remain exact. Healthy/no-op healing still spends SP
  and an action.
- Awaken clears Sleep(8) for every distinct active owner and clears
  Unconscious(12) iff that owner's current HP>0, **without** First Aid's
  terminal-condition test. HP, other conditions and noncaster SP do not
  change. It does not resurrect. An already awake party is a paid no-op.

Positive-HP terminal/Unconscious combinations are pure-rule adversarial tests;
they do not justify relaxing production save constraints. Effects draw no RNG.
Publish only HP/conditions, prepared from **post-debit** owners. A self-target
must never copy a pre-debit character over the caster's reduced SP.

### Magic Arrow and exact RNG order

The enemy picker is **precommit**. It starts at the currently selected contact
and shows the same at-most-three contact identities. Keys 1/2/3 select a live
same-cell contact; Enter moves to confirmation. Confirmation binds side/root
map/original record identity, current actor state, statistics/resource identity,
combat incarnation/revision and contact membership. Visual row, sprite ID or
coordinates alone are insufficient. No targets outside contact, target seeking,
exploration shot, multi-target effect or ranged weapon requirement is admitted.
Cancellation before confirmation costs nothing and consumes no action.

On commitment, target identity is immutable for this operation. A contact
reorder/replacement cannot redirect it. Own-modal suspension prevents legitimate
enemy work/joining while it is selected; an unauthorized change is an integrity
failure. A stale response is rejected before any work, rather than rebound to
the new occupant. After the settled action, normal contact reconciliation and
default target selection may proceed for the next actor.

Use a small detached `XeenMagicArrowCandidate` beside the existing consequence
candidates, serviced by `XeenConsequenceDraw` and **the same world-owned RNG**:

1. Validate monster profile and magic resistance R in 0..100; use raw[39]
   through a named accessor. Let L be `max(permanentLevel+temporaryLevel,0)`,
   computed with checked widened addition under current character rules.
   Check `100+L` fits the supported positive signed-int interval before any
   debit. Do not rely on an overflowing call to unchecked `currentLevel()`.
2. If R=0, skip the resistance draw entirely. Otherwise request inclusive
   `U[1,100+L]`; if its accepted result is **less than R**, resist: damage=0
   and stop this spell's effect draws. Equality passes. R=100 is not an
   unconditional immunity shortcut.
3. On passing the gate, damage is exactly **8**, independent of class, level,
   Might, Accuracy, weapon, AC, physical resistance and elemental equipment.
   `getMonsterResistance(RT_GROUP)` adds zero for DT_MAGIC_ARROW. There is
   no hit roll, damage die, critical roll or physical mitigation.
4. Still request inclusive **`U[1,50+I]`**, where I is the original monster
   resource/statistics index (`original.resourceId`, not contact row, actor
   record index, monster kind raw[33] or a newly assigned display image).
   Compare `draw<=I` as the reference saving throw does, but **both outcomes
   leave Magic Arrow's damage at 8**. Validate the interval before commitment.
   Even Slime I=0 consumes `U[1,50]` although the test cannot succeed.
5. Apply `HP_after=max(HP_before-8,0)` using widened subtraction and checked
   actor storage, or preserve HP on resistance. Living wounds change only
   authoritative actor HP. On lethal, prepare the existing XP/accounting/drop
   operation. Its draws follow the spell's last draw, before any enemy,
   movement, round or End draws.

All six original admitted profiles therefore request exactly one **accepted**
spell draw: Slime `[1,50]`, Snake `[1,53]`, Orc `[1,56]`, Skeleton `[1,58]`,
Zombie `[1,59]`, Toad `[1,63]`. They take eight damage despite Skeleton/Zombie's
50% physical resistance. These are semantic request counts; rejected raw
conversions consume the existing RNG cursor too. Keep algorithm 1, its exact
rejection conversion and the 64-raw-attempt service budget, retaining candidate
state across budget yields without redrawing an accepted prefix. No draw occurs
in menus, refunds, invalid inputs, acknowledgment or animation.

No new monster status or generic spell-resistance framework is required. The
admitted monsters have Physical status; do not import unreachable reference
special-monster, debug-super-strength, status-effect or slayer branches. Reuse
the **lethal/publication machinery**, not `XeenPhysicalPlayerCandidate`, whose
weapon and saving-throw damage rules are different.

## Combat action, turn and time

| Boundary | Resource/effect publication | Initiative consequence |
| --- | --- | --- |
| C from presented eligible PlayerReady | Reserve that acting slot; no debit | Suspend the same turn |
| Browse, precommit enemy selection, refusal | None | Same reserved turn |
| Precommit Escape | None | Return through presentation to the same PlayerReady |
| Valid final confirmation | Debit exact cost once; retain obligation | Reserve exactly one action; no other actor may run yet |
| First Aid target Escape | Refund recorded 1 SP exactly once; no effect | Consume the reserved action |
| Valid effect, resisted Arrow, terminal First Aid failure or no-op | Publish effect/outcome; retain debit | Consume the reserved action |
| Invalid index, escaped First Aid target, stale/wrong-phase input | No new publication | Existing selection/obligation remains |
| Technical failure after debit | Preserve debit and any committed effect | Unsavable retained failure; no fabricated cancel/refund |

The reference combat C caller invokes `nextChar` if `CastSpell::show()!=-1`.
An outer precommit cancellation returns -1. `SpellOnWho` Escape refunds but
returns through `firstAid` to successful `castSpell`, so it still reaches
`nextChar`. Insufficient SP loops/refuses before commitment. `spellFailed`
does not refund. `castSpell` sets `_moveMonsters=1` and restores `_tillMove`,
but adds no calendar charge. Follow the existing MMModern combat scheduler;
do not turn that flag into an extra opportunity.

On effect/refund settlement, mark **only** the acting slot acted, preserving
all other acted and blocked bits and the participant mask. Casting does not
grant Block. Recompute derived speeds/initiative from live post-effect state,
reconcile contacts by identity and use the existing `selectNext` algorithm.
A recovered participant whose acted bit is false can get its normal remaining
opportunity in the current cycle; a recovered member already acted cannot act
again. An escaped owner affected by Awaken remains excluded. Do not reset an
initiative cycle, compact member slots or manufacture a round on recovery.

Keep the computed successor phase/work/actor reserved inside the cast
continuation while its result is awaiting presentation/acknowledgment. Do not
expose live `PlayerReady`, service another enemy or let `pending()==None`
authorize input simply because effect computation has already selected a
successor. The final owned handoff installs that successor once.

Retain `moveDue`, `chargeRound`, enemy resource-attack ordinals, grouped joining,
no-action inner cycles and M34 lifecycle priority. Settlement of the cast's
presentation precedes new action input and automatic successor work. Once
handed back, mandatory enemy/round work runs through existing bounded idle
service, never a nested input/combat loop. Last-contact lethal owes ordinary
End after any already owed work; unsuccessful Run, prior escape, attrition,
defeat and support-stop retain their existing meanings.

There is **zero cast-specific time or ctr24 change** in either region. Existing
ordinary rounds and successful End each retain their one-minute charge and
post-effect full-active condition processing. Precommit menus and result reading
do not tick time. Do not call exploration `serviceCasting`, `stepTime`, Wait,
Shoot, antidote settlement or `changeTime(0)`. Do not introduce exploration's
pending=3, extra ranged/movement work or a ten/one-minute preflight for a combat
cast. If later genuinely owed round/End time is unsupported, preserve committed
spell/kill consequences and stop unsaveably under the existing time contract.

## Ownership, authority and publication

### Smallest extension and interface responsibilities

`XeenCombat` owns one noncopyable combat-casting continuation, with a new combat
Casting phase/subphases and cast result type. Use bounded private operations
equivalent to `beginCast`, `selectCastSpell`, `selectCastEnemy`, `confirmCast`,
`respondCastPartyTarget`, `cancelCast`, `serviceCast` and `finishCastPresentation`.
Names may follow local conventions; their responsibilities and checks may not
be delegated to the exploration caster. Pure candidates have no publication
capability. The continuation stores actor slot/owner, class-slot/global ID,
bound target, original SP/debit, response generation, exact result, detached
RNG/effect/drop prefix and completed-unit flags. These are transient values.

`XeenEncounterFlow` routes current combat requests, coordinates frame/lease
handoffs, presents already published results and services mandatory work.
`XeenEventFlow`/casting UI composes the bounded modal and owns UI selection
presentation. It does not mutate combat owners. Application/Gameplay/SDL route
C and phase-specific controls through the existing loop. Do not create an
exploration `CastingContinuation` while combat exists.

Roster remains sole owner of all thirty books, HP/SP, conditions and supplements;
party owns context, purse and pending treasure; world owns both regional actor
collections, accounting, Journey RNG and lifecycle; Application owns camera and
flags. Preimages and effect candidates are detached values, not a second live
combat party, resource store, RNG owner or general spell service.

### Own-modal lease and tickets

Keep Journey activity **Combat** throughout the cast, matching existing combat
authority. Use the existing boundary `Work::Casting` slot as an exclusive
**combat-owned** lease; do not set activity to exploration Casting or make
`boundary.quiet()` true during a modal.

At begin, validate the old ready ticket/frame with a quiet boundary and consume
its response before providers. In a callback-free checked transition, install
the reserved combat phase, advance its generation and acquire its Casting
lease. Return a **new ticket** containing the resulting boundary generation.
The predecessor ticket is intentionally stale. Every modal operation still
uses full `current(Ticket)` equality, including boundary generation, phase,
work, world incarnation and combat owner/revision.

Add the narrow boundary query needed to verify that the continuation holds
exactly its own Casting lease and no competing leases. Only cast operations
may use this predicate instead of quiet; Attack/Block/Run, ordinary service,
capture and other modals retain their existing quiet checks. A foreign lease
or wrong lease generation is not authorized. The combat owner, not a UI
callback, acquires/releases this lease. If a successor presentation lease is
needed, acquire it before releasing the predecessor, with no callback between
coordination stores; publish fresh tickets for the new generation afterward.
Never ignore the boundary component or reuse a pre-lease ticket.

Expose live cast mutation only through friend coordinator paths carrying a
consumed response capability. The coordinator validates the concrete presented
frame, UI/Flow incarnation, input batch/generation, combat ticket and cast phase
generation, then passes a one-use opaque authorization to combat. Combat checks
it against its retained phase/selection before publication. A public result,
copied ticket or direct typed response cannot manufacture this authorization.
Headless test seams must explicitly model presentation authorization; they must
not become weaker production entry points. Keep rendering types out of pure
effect rules.

### Publication units and failure policy

Prepare allocation, checked values, exact successor preimages, fixed result
storage, required immutable resource admission and nonthrowing publication
capacity before each unit's stores. Check all owners/resources immediately
after the last callback. Consume response capability before calling any
provider, rule callback or observer. Publication and exact guard adoption have
no callback between stores.

1. **Reservation/selection:** owns a turn and lease but changes no durable
   state. Bind names, complete books/class, actor/target and frame authority.
   Ordinary precommit preparation failure may retry the same uncommitted
   phase or cancel through a newly presented frame. Integrity failure cannot.
2. **Cost:** atomically debit caster SP, adopt the exact expected SP in combat's
   full preimage, record original SP/actual debit and consume confirmation.
   No RNG/effect/acted bit publishes in this unit. The action obligation and
   unsaveable lease persist. Gem value is checked/preserved, never cast to a
   signed integer for these zero-gem spells.
3. **Effect or explicit refund, with action settlement:** prepare from the
   post-cost graph. First Aid publishes only its target HP/conditions; Awaken
   publishes its complete bounded distinct-owner HP/condition set together;
   First Aid cancellation checks the exact debited-SP preimage and restores
   recorded original SP. Each also atomically marks the caster acted, records
   the fixed result and retains the successor scheduler state. Terminal
   failure/no-op has the same action-settlement unit with no target mutation.
4. **Arrow effect:** one unit publishes damage, canonical lethal if any,
   once-only accounting/XP, existing drop/pending treasure production, the
   detached RNG continuation, caster acted bit and retained next-work/result.
   `xeenPrepareJourneyLethal`, `XeenMonsterDropCandidate`, `episodeLethal` and
   identity-based reconciliation are reused. Resistance produces an action
   result with unchanged actor and the consumed gate RNG. Never publish cost
   again while resuming an RNG-budget yield.
5. **Presentation/handoff:** projectile and result acknowledgment observe
   settled facts; they publish no cost, effect, XP or RNG. Only their owned
   cursor/response authority advances. Hand off to the already selected
   enemy/round/End/ready continuation under fresh tickets; then existing
   independent combat/treasure publications proceed.

For each unit, update combat's `expected.roster`, actor/accounting/supplement/
treasure/RNG preimages and retained guard authority to the **prepared exact
delta**. Never recapture arbitrary live callback state to renew authority.
Do not assign a full detached character for a recovery/cost result. Existing
`publishCharacters` does not write SP; preserve that property for enemy/time
continuations, and prepare their copies only after the cast publications.
New spell code should use narrower HP/condition/SP publishers rather than
extending that helper into a general full-character writer.

An unpublished Arrow candidate may retain accepted draws and resume after the
64-attempt budget yield; no live RNG changes until its complete unit publishes.
If effect/drop/XP preparation fails technically after debit, discard no committed
prefix, publish none of that incomplete effect/RNG unit, retain the reservation
as failed and prohibit further gameplay/save. An overflow, invalid operand,
owner mismatch or exhausted cursor is not game-level `Spell failed` and is
never refundable. There is no automatic effect retry after such a failure.

Recoverable **presentation-only** failure can recompose the same owned target
prompt, projectile or result with fresh concrete frame authority. It cannot
repeat debit, consume a different target, rewind an animation's completed
prefix, refund, replay damage or advance the next actor early. After cost but
before target response, a trusted recovered First Aid prompt may still accept
explicit Escape once; technical effect failure cannot be relabeled as that
prompt. Fatal rendering failure/window close preserves the previous disk save
and ends the unsaved session without a new refund or save.

### Complete guard domain

Retain all thirty characters and supplements, optional book presence/raw
bytes, membership/order, every item byte including empty-slot metadata,
context, purse/pending treasure, recovery, camera/flags, overlays, both active
and inactive regions, all actor statistics, RNG and lifecycle/coordination
authority. Include catalog/name and projectile resources in the retained
immutable union. An inactive region or owner cannot be omitted because the
modal has no row for it.

Keep existing exclusive gameplay borrows, reentrancy fences, owner incarnations,
monotonic integrity latches and resource preimages across cache eviction and
reconstruction. Full-value equality alone does not detect mutation then
reversion. Newly inserted/reconstructed raw and typed map/MOB/Event/name/sprite
storage, including nested vectors/arrays/parameters, must join existing mutation
observers **before** any reference escapes or any provider can mutate them.
Prepare observer capacity before insertion; enrollment cannot reset prior
history. Test immediate insertion -> nested mutation -> reversion within one
callback, not just mutation of a previously enrolled cache. Do not permit a
matching retry, cosmetic redraw or stale callback to clear failure, release a
newer lease or manufacture Quiet. Check providers before and after even when
they are nominally read-only.

## Consequences and native presentation

### Existing consequence path

An Arrow wound persists on the same monster identity. A lethal is accounted
exactly once at publication, not at animation/acknowledgment/End. XP uses the
current participant/condition mask and existing checked split formula; Orc
drop generation and pending gold use the existing source identity and RNG.
Slime gives its existing 50 base XP and no treasure. Retain actor death
coordinates/lifecycle canonicalization and grouped contact replacement.

Ready/dormant treasure, selected-threat deferral, item delivery before receipt,
gold credit on final receipt acknowledgment, losses/capacity and receipt retry
are unchanged. DirectRun and AttritionAfterEscape retain their different
treasure disposition. No spell-specific reward queue, kill marker, inventory
delivery or accounting store is introduced. Full-party defeat and unsupported
mandatory work remain terminal/unsaveable; only genuine successful End or M34
successful disengagement finish grants retirement authority. City reset, not
casting/revisit/restore, establishes a new admitted Slime life.

### Bounded 320x200 flow

Reuse existing scene, text/font, six-member strip, scrollable learned list and
phase-specific input patterns. C during a presented combat turn goes directly
to the **acting member's** book, with no ChooseCaster or F-key caster switching.
Known unsupported rows remain visible. Six rows at once, at most 39 total,
sorted by class slot; start at first learned row each admission. Show name,
current SP, cost and a concise support/eligibility reason. Arturius can inspect
his known Light but cannot commit it; noncasters/no-book owners get a bounded
refusal without reserving a payable action.

| Phase | Controls and required feedback |
| --- | --- |
| PlayerReady | Existing actor/contacts plus C Cast, alongside Attack/Block/Run. C refuses with a reason if domain/caster is unavailable. |
| Learned list | Up/Down scroll/select; Enter selects supported affordable spell; Escape returns to the same ready actor through presentation. F keys cannot change caster. |
| Arrow enemy choice, before commitment | 1/2/3 select a displayed identity/HP; Enter opens confirmation; Escape returns to list. No free exploration shot or hidden-target fallback. |
| Confirmation | Caster, original spell name, exact SP/gem cost and target shape/identity; Enter commits; Escape returns to prior selector. Say one combat action, no added cast time. First Aid warns that target Escape refunds SP but uses the turn. |
| First Aid target, after debit | F1..F6 selects stable named slots with current/max HP and conditions. Escaped slots are disabled. Self and terminal participants remain selectable; label terminal failure. Escape refunds and consumes the turn. |
| Mandatory preparation/projectile | No gameplay response; Escape is absorbed. Show cast pending/settled facts and SP already spent. |
| Result | Explicit SP before/after or refund, target identity, HP before/after, cleared conditions, resistance/no-op/failed effect and pending combat work. Fresh Enter/Space/Escape acknowledges once. |
| Successor | Only after its concrete presentation, accept the next ready turn; otherwise automatically service owed enemy/round/End and later receipt/retirement. |

Result acknowledgment is a bounded MMModern readability adaptation. The effect
and turn settlement are already committed; acknowledgment is not the cast
commit point and cannot cancel/refund it. Bounded pagination is allowed for
Awaken's six owners; no page advances gameplay. First Aid/Awaken need readable
HP/condition feedback, not new sound, portrait sparkle assets or a general
effect engine. Show a healed-but-still-asleep/nonparticipating member truthfully.

Magic Arrow uses existing checked **pow11.icn**, a single cosmetic projectile
in lane 0 (as reference `_shootingRow[0]`), bound in the result to the selected
monster identity. For this same-cell-only action, show one row-0/frame-0,
scale-0, scene-clipped projectile for at least one successfully presented frame
and one 100 ms cosmetic interval, then clear it and show the result. This
contact-only truncation of reference flight is an explicit bounded adaptation;
there is no distant travel or collision simulation. Reuse outdoor command order
124, anchor `(72,43)`; add the corresponding narrow indoor projectile draw
variant at reference order 162, anchor `(72,43)`, using the same asset adapter.
It is composed in the existing ordered scene before interface layers, not a
second renderer. The indoor extension admits only this contact presentation,
not indoor Shoot or distant missiles. No new resource is required.

Damage may already be committed while the projectile is shown. Never keep a
dead actor alive for the picture or apply the wound when animation ends. Keep
identity-based existing hit feedback for surviving targets and result text for
lethals; a replacement contact cannot inherit the old effect. Preflight the
POW resource and bounded command before debit; reconstruction retains its
immutable preimage. The cosmetic clock cannot skip an unpresented required
frame or create elapsed-time gameplay backlog.

### Input, saving and successor authority

Every actionable phase needs its own concrete successfully presented
`IndexedFrame` capability, not equal pixels, a copied/returned/prepared frame
or successful texture upload without the completed presentation handshake.
Bind Flow incarnation, EncounterFlow ticket, combat incarnation/revision,
casting generation/phase, selected owner/book/slot/target, input generation
and frame identity. A semantic phase change requires a fresh frame. Cosmetic
redraw preserves the same operation but replaces concrete presentation
authority through the existing seal/present protocol.

Retain SDL fixed-batch, held/repeat/timestamp fencing for C, Up/Down, F keys,
1/2/3, Enter, Space and Escape. One batch cannot open, select, confirm, target
and acknowledge; held Enter cannot chain casts. Consume responses before
callbacks. Direct typed input, provider reentrancy and native inputs have the
same checks. Wrong-phase Attack/Block/Run, enemy selection, navigation, item
use, inventory, Event/manual interaction, save and another C cannot leak out
of the modal. Reject early input instead of queuing it for a successor actor.

No empty UI queue, result-cache eviction or modal close creates a quiet gap.
The cast lease/continuation lasts through its projectile and result; transfer
to owned combat/presentation work before release. On precommit cancellation,
the newly presented same-actor ready frame is required before any next command.
On completion, the next actor/result frame cannot inherit the consumed input.

F9 must refuse **before capture, save providers, fingerprint/path preparation
or I/O** throughout selection, debit/target, candidate yields, projectile,
result pages, owed combat work, End, reward receipts, retirement and unresolved
presentation. Refusal never settles, acknowledges, cancels or queues a save.
An existing disk save stays byte-identical. Only a fresh F9 at the final
successfully presented otherwise valid Quiet boundary may save.

## Persistence and compatibility

### Exact pair and representation

Use **v4 / schema 8 / content 10**, with no new durable fields. HP/SP/conditions,
all thirty books/supplements, actor wounds/death/accounting, treasure, context,
both retained regions and the exact RNG cursor already have schema-8 fields.
Menu/target/projectile/action reservations cannot be captured and therefore
require no wire representation. No representation mismatch was found.

Extend `xeenSupportedJourneyPair` with **only `(8,10)`**. Accepted pairs remain
exactly 1/1, 2/2, 3/3, 4/4, 5/5, 6/6, 7/7, 8/8, 8/9, 8/10. Reject 9/9,
9/10, 10/10 and all crossed/unknown pairs; no broad range admission. Fresh
`--journey-region` selects 8/10. Other fresh entries are unchanged.

The [M38 schema-8 wire table](milestone-38-plan.md#exact-successor-wire) is
unchanged except the explicit content u16 is 10. Preserve v4 envelope/header,
base character prefix, member ordering, archive fingerprints, CRC/size checks,
canonical booleans and exact EOF. Suffix lengths remain `3114+5*N` without
city, `3992+5*N` with 46 city slots, `4106+5*N` with 52, N<=12. Learned flags
remain raw bytes, **not normalized booleans**, despite the closed M38 table's
shorthand wording. No spell transcript/current selection/quick slot is added.

Content 10 retains content 9's exact current-state constraints: same mainland,
sixteen city cells, day 8..10/year 610, existing daytime minute/ctr24 limits,
all thirty books and poison/resistance supplements, fixed active membership,
all actor original/reset provenance and treasure source/readiness constraints.
Day>8 still requires a present city; absent city requires mainland camera and
no city overlays; city presence/count/gaps/reset overlay retain M37/M38 checks.
Arbitrary in-range live HP wounds are represented already. Do not require
spell-damage multiples, original SP, a spell expenditure ledger, inferred
historical kill method or an original-knowledge match at restore. Raw saved
books, including inactive/all-zero books, remain authoritative.

### Capture and restore

Use existing guarded Quiet capture, never a casting-specific save path.
Decode/validate into detached snapshot values; prepare fresh unpublished
party/world/camera/flags and immutable resources; install exact saved mutable
fields; check all owners, both regions and canonical constraints; preflight
the first frame; publish once and bind fresh runtime/Flow/input authority.
No fresh Journey initialization, cast reconstruction, classification/movement,
cost/refund, damage, wake/heal, actor turn, End, XP/drop/receipt, Event/service
reset, time or RNG draw may run before first input.

Restore does not infer a partially completed cast from reduced SP. Selections,
response tokens, cast/animation/result continuations, participant/acted/blocked
bits, frame identities, leases, revisions and incarnations are absent from the
save. A fresh later encounter builds its normal participation; saved conditions
and wounds stay exact. Resource caches may reconstruct only immutable data,
without altering owners or relearning original books.

### Compatibility acceptance matrix

| Input domain | Required subsequent behavior and recapture |
| --- | --- |
| Ordinary v1/v2 | Existing absence/fallback rules; no learned combat admission or automatic Journey conversion |
| Completed Diagnostic27 v3 | Existing completed inspection/revisit authority, no new casting |
| Journey 1/1..6/6 | Existing absent knowledge and action/support limits; no book backfill |
| Journey 7/7 | Existing mainland exploration recovery casting, combat C refusal, no city |
| Journey 8/8 | Existing eleven-cell city route, exploration recovery, combat C and service-boundary refusal |
| Journey 8/9 | Existing sixteen-cell route, Armor Repair/day-10 refusal and exploration recovery; **combat C still refuses**, Magic Arrow stays unsupported, recapture remains 8/9 |
| Journey 8/10 | Same inherited route/services plus only the three admitted combat spells; exact new quiet consequences and further casting |

Test bytes **and later gameplay/refusal** in each domain. Loading an older save
in a newer executable does not upgrade its content, wire pair, SP/books,
resources, routes or service-day semantics. In particular do not convert 8/9
to 8/10 on capture merely because schema 8 is shared.

## Production witness and acceptance

### Planning diagnostics actually obtained

A temporary observer derived from `tests/XeenM36CliWitness.cpp` was compiled
against the existing build libraries. It used real original initialization,
current content 9, deterministic CLI seeds and frame-bound typed normal inputs.
It logged state at `PlayerReady`, changed only the input policy to Block at
specified turns, and terminated at the required boundary. No owner writes,
save-file substitution or M39 spell implementation were added. Both final
bounded observations exited successfully. These were headless/provider-frame
diagnostics, **not physical or successful native-SDL M39 acceptance**.

The historical witness needed temporary diagnostic-only content assertions
updated from 7 to current 9 and two ambiguous ternary comparisons explicitly
converted to int to compile with current mutation wrappers. The product/test
sources were not changed. An exploratory overlong Block branch reached defeat;
the useful evidence is the earlier recorded eligible boundary, not survival
under indefinite blocking. Temporary observer source, executable and logs are
removed at handoff. No product build or CTest run is claimed for this prose task.

Use route notation U=forward, D=backward, L/R=turn left/right, F=exploration
Shoot. Every symbol follows a presented valid boundary and drains owed work;
combat inputs below intervene before returning Quiet.

| Branch | Inputs before M39 action | Observed boundary on baseline |
| --- | --- | --- |
| A: injury and lethal opportunity, seed 1 | From fresh `(23,9,11,West)`, `UFU`; Block the first five ready slots 0,1,2,3,4 | `(23,7,11,West)`, minute 510, ctr24=2, RNG state `1495045943`, count 12; Orc record 9/resource 6 at HP7; Seymour ready at HP11/SP27; Rebecca HP21/SP21 |
| B: useful Awaken, seed 7 | `LUUURUULURUULUUU`; first combat cycle attacks lowest record contact at each ready turn; in next cycle Block slots 0,1,3, leaving skipped sleeping slots untouched | `(23,5,4,South)`, minute 591, ctr24=16, RNG state `2283941241`, count 51; Seymour ready HP15/SP27, Badger HP21/Sleep1, Rebecca HP4/Sleep1; Toad record15/resource13 at HP50 |

Branch B's first cycle also reaches Seymour ready at minute590/RNG count29
with that Toad at HP54, providing a separate pre-existing wound opportunity.
The observation does not assert a future trace after substituting a spell.
M39 retains initialization and all preceding actions, so these prefixes provide
reachable prerequisites. Revalidate them in content 10 after implementation.

### Required implemented witness branches

**A, main connected witness:** use seed1 and `UFU`. Block slots 0..3; on
Rebecca's presented turn, C, Down from Awaken to First Aid, Enter to
confirmation, fresh Enter to debit, F6 target Seymour, then acknowledge the
result. Expected immediate effect HP11->15 and Rebecca SP21->20, no RNG/time
change; it consumes the same turn that the diagnostic used for Block. On
Seymour's turn, C, Down twice (Awaken, Light, Magic Arrow), Enter to enemy
choice, select record9 via its displayed contact row, Enter to confirmation,
fresh Enter to commit. Expected immediate SP27->25, HP7->0 and existing Orc
lethal consequences. Its one spell request is `[1,56]`, then existing Orc
drop requests. Observe projectile/result, acknowledge, drain End/receipt and
reach a presented Quiet boundary. With six eligible level<15 participants,
the existing Orc split credits 66 XP each; gold production is 10, delivered
under inherited readiness/receipt rules. Do not assume a particular generated
item or post-cast RNG total from the old Attack trace.

Save with actual F9 outside the installation, exit the entire process, load
the file in a new process and compare exact state **before first input**.
Continue both an uninterrupted branch and the fresh-load branch with identical
inputs. Starting from `(7,11,West)`, `DDLLULUU` reaches the existing mainland
entrance route; Space/Yes performs its ordinary entry. Follow M38's admitted
`URULUUULUUU` city approach, draining work normally. Do not enable the M38
deliberate armor-breaking Block loop. In the first Slime encounter, Block
earlier ready members until Seymour, use his Magic Arrow for the 2-HP lethal,
then drain End. If normal intervening injury needs recovery, use only the
already admitted well/exploration First Aid or combat First Aid, with its
costs and exact inputs recorded in the final witness. No owner edits or refill.
Save quietly with retained mainland and city consequences; fully exit/load
again, then continue navigation and a supported exploration cast. Require
encoded and field-level equality after identical continuation, including both
regions, items, flags/context and RNG. Verify original exit/reset/revisit on
a companion from that legitimate checkpoint using the existing M38 route;
reset changes only the named city lives, not mainland kills or spell SP.

**B, practical waking/recovery:** use the seed7 prefix above, then at minute591
Seymour C/Enter/Enter for Awaken and acknowledge. Expected immediate SP27->26,
Badger and Rebecca Sleep1->0, unchanged HP, unchanged RNG/time/participation.
The existing stable-slot selector now offers their eligible unacted turns;
Block with Badger so the wounded Toad remains for Rebecca's turn, and
demonstrate that slots0/1/3 are never replayed. Rebecca uses combat First Aid on herself
(HP4->10, SP21->20), then normal attacks finish the fight, or legitimate Run
settles a non-victory exit. Drain all consequences and demonstrate quiet
save/restart and further play on this branch too. The final action/RNG trace
and survivability after these changed choices must be established by the
implementation witness; the old M36 completion trace is not reused.

**C, wound and control companions:** from the independent seed7 fresh prefix,
substitute Magic Arrow at Seymour's first-cycle HP54 target boundary, giving
HP54->46/SP27->25, then follow normal combat work. Record subsequent legitimate
wounds/lethal or Run/re-engagement and retained wounded identity; do not splice
this trace into B. On a separate A prefix, exercise list/confirmation/Arrow
target precommit cancellation and unsupported Light with zero durable delta;
exercise First Aid post-debit Escape with exact refund but next turn consumed.
Also cover a paid healthy First Aid/Awaken no-op. Preserve a usable real-save
branch for all full-process comparisons. Seed/input choices are reproducible
controls; no synthetic state is a substitute for A/B/C's production effects.

Finite SP is sufficient for the independent witnesses: A's key operations use
Rebecca 1 SP and Seymour 4 SP across Orc/Slime, B uses 1 each, C uses Seymour
2. Fresh branches are separate original starts, not replenishment. No guild,
new learned book, rest, Training or M40 day advancement is needed. Any failure
to complete these changed-action routes must be resolved with a bounded normal
input/seed branch and documented exact trace before acceptance, not with a
live-state overwrite or an assumed historical RNG trace.

### Commands and evidence separation

Current `src/main.cpp` and `tests/XeenSmithProcessTests.cpp` establish the CLI
syntax (save parent must exist outside the commercial directory):

```powershell
.\build\mmodern.exe --journey-region --combat-seed 1 "F:\Games\gog\Might and Magic 4-5" --save-file "D:\Projetos\MModern\mmodern\build\m39-acceptance\main.mmsave"
.\build\mmodern.exe --load-game "F:\Games\gog\Might and Magic 4-5" "D:\Projetos\MModern\mmodern\build\m39-acceptance\main.mmsave"
```

Seed7 companions use the same fresh syntax with seed7 and separate output.
There is no public `--journey-contract` switch; legacy-domain controls use
existing test fixtures/restore seams. These commands are syntax-verified here,
not evidence that the current executable implements M39. Do not send combat
seeds/contracts with `--load-game`.

Add focused M39 witness/process tests following the current M38 process pattern:
real Application setup, external original resources, normal typed/native input,
actual F9 save pipeline, full child-process termination, distinct process
incarnations and fresh load. Use `XeenRestoreReplayProbe`-style counters extended
to cast costs/effects. Before first input require byte-identical recapture and
field equality for all thirty owners, books, membership, supplements, items,
context/flags, both regions/accounting, treasure and RNG; require zero
initialization/action/cast/time/drop/retirement replay. After identical further
inputs require the same exact comparisons and RNG requests/raw count. Compare
uninterrupted and restored runs, not two loads of a mutated player save.

Automated typed-input diagnostics, native SDL injected-input tests, dummy SDL,
screenshots/image review, independent technical review and **maintainer physical
native-SDL acceptance** are separate evidence classes. Physical acceptance must
use ordinary keyboard controls in a visible native 320x200 presentation (with
normal scaling allowed): useful Arrow/healing/waking, unsupported rows,
cancellations/refund/turn consumption, readable targets/SP/results/projectile,
held/batched controls, F9 denial, consequences, quiet save, full exit/load and
further combat/exploration including retained Vertigo. Automation and images
cannot satisfy that acceptance requirement.

### Focused tests and closure gates

1. **Pure rules/resource validation:** exact three mappings/costs; absent,
   all-zero and raw-nonzero books; capability/class/condition/Speed/SP bounds;
   all recovery predicates, aliases and inactive owners; nonparticipating
   First Aid refusal versus full-active Awaken. Arithmetic overflow fails
   before stores. Arrow tapes prove R=0 skips the gate, equality passes,
   R=100 still draws, failure skips saving throw, saving success/failure both
   keep eight, PR50 does not halve, Slime still draws, and I is the resource
   identity. Include rejected raw values, 64-attempt yields, exhausted count,
   invalid intervals and lethal/drop draw order. Validate original CHR/names,
   selected MON records and decoded POW frames without bundling data.
2. **Combat integration:** casting only on the acting presented participant;
   Attack/Block/Run still work before/after; no extra time/opportunity; one
   acted bit per paid attempt including refund/failure/no-op; unacted recovered
   member versus already-acted/blocked/escaped member; grouped joining and
   re-engagement; each target identity replacement and alias/self publication;
   wound, last/nonlast lethal, XP masks, Orc gold/drop/readiness, city Slime,
   DirectRun/attrition, defeat, owed movement/round/End and support stops.
3. **Failure/publication:** inject before/after debit, target/refund, effect,
   each RNG yield/draw, drop/XP preparation, pre-store check, post-effect
   reporting, projectile/result, successor and receipt. Assert exact committed
   prefix, no live unpublished RNG/effect, no duplicate turn/reward, preserved
   earlier disk save and truthful unsaveability. Overflow in XP/drop/time
   cannot roll back a prior SP debit or completed lethal unit.
4. **Authority/adversarial:** stale/foreign/copied tickets/frames, destroyed and
   reconstructed owners, boundary lease generation changes, forged empty
   modals, same-pixel unpresented frames, failed upload/copy, redraw/resize,
   held/repeat/stale-timestamp and multi-key batches; provider-recursive C,
   confirm/target/cancel/acknowledge/Attack/Run/F9. Mutate then revert active and
   inactive HP/SP/book/presence/class/items/supplements, membership, purse,
   context, flags, RNG, both regions, actor statistics, names and newly inserted
   nested resources. Require monotonic failure after later matching retries.
   Early F9 has zero capture/provider/path/I/O counters at every phase and
   handoff. Restore/catalog reconstruction cannot mint response authority.
5. **Persistence/regression:** explicit pair matrix above, exact schema-8
   extents/field bytes, malformed/truncated/reordered/unknown pairs, canonical
   constraints and all thirty books. Fresh 8/10 and legacy 8/9 process controls
   must prove different combat Cast admission but identical inherited
   exploration time, repair/day boundary, routes and restore semantics.
6. **Closure:** build appropriate targets, run focused tests during iteration,
   then the complete unfiltered CTest suite and original-resource/process
   witnesses. Obtain independent technical review and maintainer physical
   acceptance before recording milestone completion. No failing test or
   unproven required witness may be labeled accepted. Update durable status,
   history, roadmap/closed-plan record only in separately authorized closure.

## Focused implementation touchpoints and exclusions

| Existing area | Bounded change |
| --- | --- |
| `XeenLearnedSpellRules.{h,cpp}` | Catalog identity/cost and explicit context support; retain pure recovery semantics |
| `XeenCombat.{h,cpp}`, `XeenCombatRules` | Combat-owned continuation/lease, Arrow candidate, narrow cost/effect publication, retained scheduler/consequence integration |
| `XeenMonsterFormat.h` | Named checked magic-resistance accessor over existing byte; no new durable statistics |
| `XeenJourneyContent.h`, initialization/validation/capture/codec/restore | Explicit 8/10 capability and fresh selection; retain 8/9 and schema-8 representation |
| `XeenEncounterFlow`, `XeenEventFlow`, casting UI, Gameplay/Application/SDL | Route combat C, own-frame response adapter, fixed acting caster, phase controls/result, early saving refusal and successor handoffs |
| `XeenMonsterAppearance`, indoor/outdoor scene/composer and existing asset adapter | Bound one contact projectile/result identity; narrow indoor ordered command, no new renderer |
| `XeenRestoreGuard`, mutation/resource hooks | Complete preimages and pre-exposure nested-storage observation at newly reached callbacks |
| Combat/learned/authority/save/input tests and new M39 CLI/process witness | Behavior, compatibility, failure and genuine production acceptance above |

No gameplay or test source is changed by this planning task. Implementation
must preserve architecture and share existing consequences/pure effects rather
than introducing parallel owners or a generic spell VM/transaction framework.

Excluded: acquisition, guild membership/purchases, new learned spells/books,
general spell coverage, area attacks, new monster statuses, offensive
exploration casting, Light/durations, rest/SP replenishment, new routes/services,
calendar expansion, Training/progression, Buy/Sell/merchant stock, new quests,
enemy species and unrestricted Clouds/startup. No M40/M41 pre-implementation,
new ScummVM dependency, nested event loop, commercial-data modification or
commercial payload in the repository. The only planning deliverable is this
file; review readiness does not authorize implementation, staging, commit,
push or tag creation.
