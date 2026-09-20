# Milestone 34 - Original disengagement and encounter lifecycle

## Status, objective and acceptance boundary

**Independently reviewed with corrections; unimplemented and pending maintainer
approval.** This document does not authorize implementation or certify milestone
acceptance.
The investigated MMModern baseline is
`ac1399965abe81f245f25e687df4ee760a2f8538`,
`Complete Milestone 33 mainland combat and consequences`.

M34 adds individual Run to regional combat, partial-party participation, explicit
non-victory endings, original fixed relocation, and continued play against the
same surviving actors. The acceptance boundary is:

```text
original prepared regional entry -> production movement/Shoot/contact
-> failed and successful individual Run -> partial-party combat
-> victory OR non-victory finish (including casualties, treasure disposition,
   fixed relocation and destination classification)
-> guarded retirement -> required attachment/reward/event/presentation handoff
-> mutable Journey -> return/re-engagement with unchanged surviving identities
-> quiet save -> fresh-process restore -> further mutation/re-engagement/death
```

No healing, regeneration, respawn or duplication of XP, treasure production or
delivery may occur through this chain. Defeat remains terminal and unsaveable. A Run
success is not itself a party-wide escape, a victory, or permission to save.

Consume [M33](milestone-33-plan.md), especially its contact episode, physical
consequences, time, treasure, publication and concrete-frame contracts. Retain
[M32](milestone-32-plan.md)'s mainland/actor/event separation,
[M30](milestone-30-plan.md)'s grouped initiative and joining,
[M29](milestone-29-plan.md)'s Journey owners and
[M31](milestone-31-plan.md)'s exclusive Event authority. Those specifications
remain authoritative except for the explicit new-content changes below. M34 is
one milestone; the checkpoints here are dependency order, not separate acceptance
or implementation authorizations.

## Evidence and provenance

### Verified baseline and interpretation

The architecture/specification investigation read `AGENTS.md` before checking
origin. Origin was `https://github.com/carbiecarbie/MMModern.git`; branch was
`main`; HEAD, `origin/main` and direct `git ls-remote origin refs/heads/main`
all equalled the baseline above. The required log, porcelain including untracked
files, unstaged diff and staged diff commands succeeded with a clean tree/index.
The gate was repeated successfully after the maintainer reported a PC interruption.

**ORIGINAL** below means bytes read from the commercial installation, not a
physical observation of the DOS executable. **REFERENCE** means algorithms read
in the pinned ScummVM source. **IMPLEMENTED** means inspected baseline code/tests.
**INFERENCE** means a deliberate bounded integration decision specified here.
No M34 DOS or native-SDL behavioral acceptance was performed during planning.

[Dependencies](dependencies.md) owns the reference pin and attribution. The local
ScummVM checkout was verified at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` with empty porcelain status. The build
cache points to that checkout and the separately configured ScummVM build. All
reference paths below are relative to that exact revision; inherited M33 damage,
equipment and generation arithmetic is not reinvestigated or replaced.

| Evidence | Decisive locations and facts |
| --- | --- |
| REFERENCE Run | [combat.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp), `Combat::run` (1621 onward): inclusive 1..100 draw, strict comparison with signed `_chance2Run`, remove only `_whosTurn`, rebuild speed table, set `_partyRan`; no stat/level/monster comparison. |
| REFERENCE input and endings | [interface.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp), `Interface::doCombat` (1591 onward), R branch (1771 onward), post-loop branch (1869 onward), `nextChar` (1921 onward). R calls `run` then `nextChar`; the immediate interactive-return branch clears treasure money/readiness and relocates. The later surviving-monster/`_partyRan` branch rechecks the full party, relocates and sets disabled remaining members' Dead byte to 1. |
| REFERENCE distinct predicates | [character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp), `worstCondition`, `isDisabled`, `isDisabledOrDead`; [party.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp), `checkPartyDead`, `changeTime`, `moveToRunLocation`. Defeat uses combat membership in combat and full active membership outside it; Sleep is not defeat. Relocation assigns position only. Time visits full active membership. |
| REFERENCE partial party | `combat.cpp`, `setupCombatParty`, `setSpeedTable`, `allHaveGone`, `charsCantAct`, both `doMonsterTurn` overloads, `giveExperience`, `monstersAttack`. Contact and in-combat ranged targeting and XP use the remaining combat-party pointers; ordinary exploration reconstitutes full membership. |
| REFERENCE treasure quirk | `interface.cpp` R branch sets `_treasure._gold=0`, `_gems=0`, `_hasItems=false` without clearing item arrays. `party.cpp`, `Treasure::clear/reset`, `giveTreasure` (699 onward); `combat.cpp`, lethal branch of `attack2` (1516 onward). Later gold or item production can reopen delivery of those retained arrays. `giveTreasure` actually clears them only on delivery/capacity processing. |
| ORIGINAL map metadata | Initial `maze0023.dat`: 892 bytes, CRC32 `8f3e28ee`; Run X/Y offsets 815/824 are 10/12; signed difficulty byte at 823 is 100. `MazeDifficulties::synchronize` in reference `map.cpp` identifies difficulty index 7 as signed `_chance2Run`; MMModern `XeenMapFormat::parseDat` already preserves it. |
| IMPLEMENTED combat | `src/games/xeen/XeenCombat.cpp`: `Impl::sortOrder/selectNext/defeated/reconcile`, `command`, `serviceConsequences`, `retireJourney`. Six fixed owner slots currently participate; commands are Attack/Block; retirement requires `Victory`, `ended`, an episode lethal and no owed work. |
| IMPLEMENTED integration | `src/app/XeenEncounterFlow.cpp`, `handleCombat`, `idleCombat`, `acceptCombatResult`, `observeCombat`; `XeenJourneyFlow.cpp`, `attachJourney`, `retireJourney`, `advanceJourney`, `presentJourney`; `XeenEventFlow.cpp`, `prepareJourneyTransition`, `handle`, `framePresented`, save gates. |
| IMPLEMENTED consequences/persistence | `XeenCombatRules.cpp`, `XeenEnemyAttackCandidate` and `XeenConditionTimeCandidate`; `XeenRegionalRules.cpp`, `XeenRegionalOpportunityCandidate`; `XeenJourneyProgression.h`, `xeenPrepareJourneyLethal`; `XeenMonsterTreasure.*`; `XeenJourneyRules.cpp`; `XeenJourneyCapture.h`; `XeenSaveState::validateJourneyValues/restoreJourney/capture`; `XeenSaveFormat::validate/encode/decode`. |

### Original relocation facts and reproducible planning evidence

The original installation at `F:\Games\gog\Might and Magic 4-5` was read-only.
A memory-only probe decoded the existing initial CC blocks in reference
`SaveArchive::reset` order (`2a0c,2a1c,2a2c,2a3c,284c,2a5c`), verified the DAT CRC,
and applied `XeenMovement::outdoorDestination/component`'s admitted collision
rules from anchor `(9,11)`. It wrote no extracted resources or asset dumps.

| Cell | Raw word | Surface index / resolved type | Middle | Attributes | Mainland |
| --- | --- | --- | --- | --- | --- |
| Run `(10,12)` | `0x0007` | 7 / 7 | 0 | 0 | yes |
| `(9,12)` | `0x0002` | 2 / 2 | 0 | 0 | yes |
| `(11,12)` | `0x0022` | 2 / 2 | 2 | 0 | yes |
| `(10,11)` | `0x0007` | 7 / 7 | 0 | 0 | yes |
| `(10,13)` | `0x0807` | 7 / 7 | 0 | 0 | yes |

The component contains 121 cells. The nominal destination therefore needs no
geometry extension, alternate safe tile or movement ability. The destination has
no automatic-event bit. The retained original event-address checks in
`tests/XeenRegionalOriginal.cpp` exclude `(10,12)` for all four facings; the
adjacent `(10,13)` transition remains unadmitted.

To reproduce the additional scalar probe, use the existing bridge's initial
resource reader, `XeenMapFormat::parseDat`, and `XeenMovement::component`; inspect
`runX/runY`, `difficulties[7]` and `cells[12*16+10]`. The CC decoding and DAT offsets
are also explicit in `ScummVmXeenBridge.cpp`, pinned `cc_archive.cpp` and
`XeenMapFormat.cpp`. Compare the CRC before interpreting offsets. These are
read-only observations, not a proposed second production resource reader.

Planning ran the existing `mmodern_regional_original.exe <installation>` with no
save-output argument: the manifest, actor closures, route schedules, restoration,
resource renewal and synthetic unknown-event refusal checks passed. It also ran
the existing consequence CLI witness with seed 3, route `UF`, oracle `wound` and
a save destination under the OS temporary directory. The fixed original-resource
wound oracle passed: camera `(8,11)` West, minute 500, ctr24 1, actor 9 HP 16,
world RNG state `0x4a767d04`, raw count 8, carried gold/gems 800/10, empty treasure.
The original owner HP vector remains `[36,48,36,40,21,15]`. This verifies a useful
M34 starting prefix, not a Run outcome. No build or full CTest was needed for
this documentation task; M33's closed acceptance is inherited evidence.

## Content boundary and ownership

**INFERENCE:** introduce regional content contract **5**, with the same entry,
prepared party, all 19 original actors, terrain, events, profiles and time domain
as contract 4. Fresh `--journey-region` selects 5 after implementation. Explicit
legacy fixtures/load select their saved contract; no implicit upgrade or content
override on load is permitted. Contracts 1..4 do not acquire Run.

| State | Sole owner and M34 treatment |
| --- | --- |
| Actor identity, original metadata, position, current HP, activation, lifecycle/status, defeated accounting, gameplay RNG | Existing world/session; never copied back from base monster statistics at retirement or reattachment. |
| All 30 character records, active membership/order, HP/SP, conditions, items, supplements/XP, purse and stored monster treasure | Existing party/roster. Run never erases/reorders membership or moves character state into another live party. |
| Camera/map/facing and flags | Existing Application-owned values, included in guarded relocation publication. |
| Participation, acted/blocked bits, selected target, initiative, exit reason, owed work and completion proof | Existing noncopyable `XeenCombat`; bounded transient values keyed by original active-owner slot/actor identity. No combat-party owner or serialized escape flag. |
| Notices, receipt pages, animation, input generations, exact frame binding | Existing Flow/SDL; immutable observations do not confer publication authority. |

Use a six-bit participant mask over active slots `[0,18,14,11,1,6]`, initially
`0x3f`. A cleared bit means **escaped during this episode**, not absent/dead.
Incapacitated/dead characters retain set bits until retirement. The ordered
participant view is derived from the mask in active order; it may contain owner
references/slot indexes, never mutable duplicate characters. Preserve acted and
blocked state by those stable slots; do not reproduce reference array-index
compaction artifacts that transfer another member's action flags.

## Individual Run and partial-party rules

### Admission, draw and consumption

Only the currently selected, presented `PlayerReady` member in contract 5 may
attempt Run. Require a set participation bit, `canAct()` and positive effective
Speed, current owner/ticket/frame authority, no competing lease, and checked
original environment. Poison/Disease alone do not forbid it. Sleep, Paralyzed,
Unconscious, Dead, Stoned and Eradicated forbid an individual turn/Run; only the
already admitted M33 condition domain is produced or saved by M34.

Run occupies the member's ordinary initiative position. It is not a reaction,
free party command, target-dependent action or exploration command. Failed and
successful attempts both consume that action. An invalid/stale/wrong-phase
request consumes neither action, world RNG nor time, and creates no future intent.

Read the signed `difficulties[7]` from the guarded current map. Draw exactly one
accepted inclusive `U[1,100]`; success is **draw < chance2Run**. Do not use `<=`,
Luck, Speed, class, monster level, party size or a second success roll. For the
admitted map, 1..99 succeeds and 100 fails. For pure rule controls, signed values
<=1 never succeed and values >100 always succeed, but still perform the draw.
No shortcut may change draw consumption.

Use the existing `XeenCombatRandom` algorithm-1 world continuation and rejection
conversion. Each raw attempt, including a rejected conversion, advances only the
detached candidate until publication. Retain the existing maximum 64 raw attempts
per service call and resume without redrawing a completed prefix. Publish the
accepted result, RNG continuation, acted bit, participant-mask change and next
coordination together. No minute/ctr24 charge belongs to the attempt itself.
Only then select/service subsequent enemies or the existing round operation.

Failure retains the mask. Success clears only the acting slot; it does not
change that owner's HP/SP, conditions, XP, equipment, membership or location.
Rebuild the derived initiative using surviving stable slots and current speeds,
retaining everyone else's acted/blocked bits. Escaped members receive no further
turns this episode. Do not reset a round or replay a faster member because the
participant list became shorter. A failed attempt may be followed by lethal
enemy work; its eventual outcome is evaluated separately from the Run roll.

### The four distinct eligibility domains

1. **Can act:** participating, awake/non-disabled/non-dead, positive effective
   Speed for a ready turn. All-asleep or zero-Speed participation uses M33's
   bounded automatic inner cycle, not escape or a manufactured round charge.
2. **Targetable/continuing life:** M33 `xeenCombatTargetable` (worst condition
   below Paralyzed, or Good). Sleep is targetable. Evaluate over the participant
   view for encounter continuation, and separately over full active membership
   for whole-party defeat and post-retirement save eligibility.
3. **XP eligible:** participating at the lethal publication and worst condition
   other than Dead/Stoned/Eradicated. Sleep and Unconscious qualify. Previously
   escaped members do not receive that kill's XP; earlier XP is never taken away.
4. **Treasure recipient:** full active membership at delivery, `canAct()` and
   category tail capacity, following M33. An escaped member can receive an item
   after reintegration even though it received no XP for the producing kill.

Enemy contact and owed in-combat ranged attacks use the ordered participant
view. Orc's initial random target is `U[0,N-1]`, where N includes incapacitated
and dead participating members. Skeleton/Zombie's preferred Cleric search is
restricted to that view. On an ineligible initial choice, draw from its ordered
targetable subset, including the singleton draw. With N>0 but no targetable
member, retain M33's initial-draw/no-hit fallback behavior in already owed ranged
work. With N=0 no target draw is legal; finish the movement/shot bookkeeping with
an explicit no-participants result, never `U[0,-1]`.

Snake/Toad hates-party attacks visit **all remaining participants**, in active
order, including their incapacitated/dead members; escaped slots receive no hit,
special, wake-up or draw. Finish the complete prepared resource attack before
testing its terminal result. Preserve M33's separate Zombie resource-attack
publications and terminal cutoff before another resource attack. Required ranged
queues retain their source identities and complete their admitted observation
sequence; participant shrinkage does not silently discard a queued shot.

`Party::changeTime` is different: every admitted round/time tick still visits
the full six active owners, including escaped, sleeping and dead owners, in M33
order. Escaping grants no immunity to elapsed-time conditions/stat death.
After successful retirement all six roster members are available to ordinary
Journey predicates again; a new contact episode starts with mask `0x3f`, without
waking, curing, resurrecting or changing any of their bytes.

## Lifecycle and non-victory completion

### Terminal predicates and ordering

After each complete publication, resolve the following order. Never inspect a
partially serviced damage/time candidate as though it were published state.

1. A full active party with no targetable member is terminal **Defeat**. Retain
   every already published consequence; no relocation, reward settlement, capture
   or non-victory completion authority is created.
2. Complete any already owed movement/ranged/charged-round work as described
   below before deciding that a contact episode is over.
3. With no contact and a genuine episode lethal, use M33
   `VictoryAwaitingEnd -> End -> Victory`. End still charges one minute, processes
   its admitted tick and succeeds only if the full party survives. This includes
   victory after other members escaped; no Run relocation or casualty conversion
   follows victory. Empty contact without an episode lethal is an integrity error,
   not a fabricated victory or a generic escape path.
4. With surviving contacts, if at least one participant remains targetable,
   continue combat. Any escaped mask is retained through rounds and joining.
5. With surviving contacts, no targetable participant, at least one escaped
   member and a non-defeated full party, finish **Disengagement**. This covers an
   empty mask and abandoned incapacitated/dead remaining members. Merely having
   nobody immediately able to act (Sleep/zero Speed) does not satisfy this test.
6. The remaining impossible combinations fail closed. In particular, an exhausted
   participating party with no escaped survivor cannot obtain retirement.

The pre-existing full-party defeat cutoff remains terminal: do not generate new
work after it. A movement opportunity already executing is an indivisible M33
candidate and finishes its admitted queue before that cutoff. An unpaid later
obligation in a terminal graph is retained as terminal unresolved work, not
reported as consumed, Quiet or retired.

### Direct Run exit versus later attrition

Retain a transient, immutable exit cause when the exit first becomes eligible:

| Cause | Exact trigger | Pending treasure money | Stored item arrays |
| --- | --- | --- | --- |
| `DirectRun` | A successful Run publication itself leaves no targetable participant (including empty mask), before a subsequent enemy action | Forfeit all undelivered monster gold and its source mask | Retain; become dormant, as specified below |
| `AttritionAfterEscape` | A subsequent enemy attack, owed ranged opportunity or time publication exhausts participants after an earlier success | Retain | Retain normal readiness |
| Victory with prior escape | Contact actually cleared, owed work drained, successful End | Retain/settle under M33 | Retain/settle under M33 |
| Defeat/failure | Whole party defeated or authority/resource/time failure | Preserve published state for inspection | Preserve; no later receipt authority |

**REFERENCE/INFERENCE boundary:** the first row captures `doCombat`'s R-branch
interactive return; the second captures its later `_partyRan`/surviving-contact
exit. In the reference, `nextChar` can return to the R branch immediately after
removing the last targetable character; an enemy lethal inside `nextChar` can
instead leave the loop through the later path. Do not simplify this to “any Run
loses all loot” or “all non-victory endings preserve all loot.” The semantic causes
above deliberately replace incidental mode variables and compacted index bugs;
they are the normative bounded behavior. Ordinary Attack/Block followed by
participant exhaustion after escape takes the second row; a failed Run followed
by enemy exhaustion also takes the second row.

### Owed work and successful non-victory finish

Use the existing coordinator, with a distinct pending disengagement work/phase
and a successful `Disengaged` phase. An exit intent closes player input immediately
and prevents new contact turns/round creation. It does not erase `moveDue`,
`chargeRound`, an attachment opportunity, a ranged candidate or a published
projectile/result awaiting presentation.

Before finish, service pre-existing movement/ranged obligations at the **origin
camera**, using remaining participants, the original retained source/scan order
and the existing owner graph. A transferred attachment opportunity carries no
ordinary-round minute; a previously established ordinary round retains its one
minute after movement/ranged damage. Finish its time tick over all active owners.
Joining during that opportunity updates the authoritative origin contact view
and observations; it cannot reopen turns after a latched disengagement intent.
Full-party defeat overrides finish. No new round is owed merely because the last
character escaped or no action-capable participant remains.

Then prepare one bounded **FinishDisengagement** publication containing:

- the validated exit cause, origin and original fixed destination;
- casualty condition changes for remaining participants;
- direct-exit pending-money forfeiture, when applicable, with source observations;
- camera relocation, destination classification and required activation;
- immutable result facts and a one-use successful-finish proof tied to this combat
  incarnation, current revisions and the actual owners.

For remaining participants only, implement reference `isDisabled()` by **worst
condition**: Asleep, Paralyzed, Unconscious, Stoned or Eradicated sets Dead=1.
Already-Dead as the worst condition does not reset/increment its byte. In M34's
admitted domain, terminal remaining casualties are Unconscious or already Dead;
Sleep alone cannot cause this exit. Preserve HP/SP, all other condition bytes,
armor and inventory. This is not physical injury: no invented HP subtraction,
armor breakage, stat recomputation writeback or XP deduction. Unsupported
conditions remain excluded even though the reference predicate is documented.

Preparation must validate the final full-party/save-domain predicates without
healing or clamping. Allocate results/preimages and preflight destination resources
before nonthrowing stores. Finish itself charges **zero** time/ctr24 and draws
**zero** RNG. The proof is created only by successful publication, never inferred
from a mask, a result enum, copied state or a camera already at `(10,12)`.

Guarded non-victory retirement consumes that proof once, verifies no candidate or
owed work remains, retains the immutable resource union, releases the combat
borrow/owner and returns to existing Journey coordination. It does not call End,
set victory completion/accounting, require a lethal, or loosen `retireJourney`'s
successful-victory-End gate. Both retirement paths leave world actor state and
party consequences authoritative. Old combat input/tickets die with that episode.

### Relocation and authority handoff

Relocate on the same Clouds map to guarded `runX/runY=(10,12)`. Preserve facing.
Never search for safety, nearest free space, initial entry or the previous cell.
The operation is a fixed assignment, not navigation through intervening cells:
no step charge, RNG draw, movement opportunity, ranged scan or transition event
is generated just by relocation. Reclassification and newly required activation
are deterministic follow-up work included in finish, not an actor movement.

Nominal destination conflicts are resolved as follows:

- A live actor on `(10,12)` remains there. After old-combat retirement, transfer
  directly to Journey `Attachment`, then a fresh encounter on the same survivors
  with all six participation bits. No Quiet/capture/input gap and no free attack
  occur between the two incarnations. Repeated Run at that same destination may
  re-engage; there is no safety guarantee or automatic infinite Run loop.
- Selected off-contact survivors defer ready treasure by the unchanged 26-slot
  predicate. Hidden pixels do not make them absent. If no contact remains,
  settlement has priority over an eligible Event and over Quiet.
- The verified destination has no Event address or automatic bit. Validate this
  under the existing manifest/immutable guards; no manual event is synthesized
  and no Myra/transition script is run. Unexpected changed topology or events is
  resource incompatibility, not permission to expand admission.
- An incompatible/out-of-mainland nominal destination refuses preparation before
  relocation. Never substitute a tile. Published Run/earlier consequences remain
  published; the graph fails closed and remains unsaveable.

An origin-cell automatic sign request may be waiting behind contact. Bind that
unstarted request to its physical address/facing instead of letting the existing
`_regionalAutomatic` boolean accidentally dispatch at another address. Victory
without relocation retains the request and M33's Reward-before-Event order.
Successful relocation explicitly supersedes the unstarted origin request and
rechecks the destination's automatic eligibility (none at the verified Run cell).
This follows `Interface::doCombat`'s final `upDoorText`/`cellFlagLookup` check at
the current, possibly relocated position. Record the supersession in the finish
observation; retirement alone is not cancellation authority. No Event lease or
partially executed script can coexist with Run, so no begun Event mutation is
discarded. A preparation failure before relocation retains the origin request
in the unavailable graph. Add a sign-contact-escape control for this handoff.

Journey authority resumes only when the combat borrow has been released by guarded
retirement and the fresh Journey preimage has adopted the exact publication.
Capture remains closed through Attachment, Reward, Event, projectile recovery and
Presentation. Only a matching concrete frame at an otherwise eligible Quiet
boundary opens fresh input/F9. A reattached combat still requires its own matching
frame and automatic-work service before any `PlayerReady` input.

## Surviving actors, XP and treasure

### Actor invariants

Survivors keep their original `{Clouds,23,recordIndex}` identities, HP, coordinates,
activation, Physical/Present lifecycle and unaccounted status. Finish changes
neither contact-origin survivors' positions nor their HP. Classification may
activate additional actors at the destination; activation never resets to false
for a live survivor. All 19 actors, including those outside the party component,
retain M32/M33 movement closures, occupancy and original-order scheduling.

Discarding encounter contact rows, selected target, acted bits and ATT effects
does not remove/freeze an actor. Subsequent genuine Journey opportunities move
those same actors. Reattachment uses their current HP and position, not base HP,
spawn coordinates or reconstructed living instances. A later lethal atomically
canonicalizes that identity and adds accounting exactly once, with the existing
shared consequence resolver. Repeated disengagement, cache discard, saving,
restoration and a new encounter cannot produce another death for an accounted
identity. Defeated actors stay HP=0, `(-128,-128)`, inactive and accounted.

### XP publication

Extend `xeenPrepareJourneyLethal` with an explicit eligible participation mask,
defaulting only at callers that intentionally represent full-party exploration.
All six supplement values remain present; nonparticipants retain their XP.
Compute the eligible count from the current participant/condition intersection.
Keep M33's `floor(baseXP/eligibleCount) * (permanentLevel<15 ? 2 : 1)` and checked
u32 addition. No eligible recipient for a player lethal is an integrity failure.

XP publishes with the lethal, alongside canonical actor death, accounting, drop
production and RNG. It is not deferred to End, finish, receipt or retirement.
Run never refunds XP. A later casualty retains previously credited XP. Delivery
of a dormant item never credits its source's XP or gold again.

### Retained item quirk and smallest consequence extension

The reference's Run branch does **not** call `Treasure::clear` or `reset`. Treating
those items as lost would erase observable future consequences. M34 therefore
preserves the already party-owned weapon/armor queues, including production order
and original source IDs, when `DirectRun` forfeits pending money. It clears
`pendingGold` and its `pendingMask`, not carried gold/gems or stored item records.
Reference pending gems are also zeroed; the admitted monsters produce none, so
M34 adds no pending-gem field and never subtracts carried gems.

Use the existing fields with explicit content-5 meanings:

- `pendingMask` identifies only **uncredited, unforfeited Orc gold obligations**;
  `pendingGold == 10*popcount(pendingMask)` remains mandatory.
- Stored item sources identify already defeated/accounted Orcs independently of
  that mask. A source can therefore retain an item after its gold was forfeited.
- In the admitted monster domain, delivery is **ready** exactly when
  `pendingMask != 0`. Nonempty arrays with zero mask/gold are **dormant**. No
  independent item-ready boolean is necessary: every admitted item-producing
  monster is an Orc that adds ten gold, and Run clears both readiness and money.
- A later Orc lethal adds only its own new gold bit/ten gold and follows unchanged
  M33 generation/capacity rules. Its gold opens delivery of all stored arrays,
  even if it produces no item. A Snake/Toad/Skeleton/Zombie lethal adds neither
  money nor readiness; dormant items remain dormant. Existing dormant records
  occupy real category slots and can cause later category-capacity loss.
- Only later delivery/capacity loss removes stored items. No corpse cache, second
  treasure owner, new loot generation, inferred item reconstruction or gold debt
  for forfeited sources is introduced.

This preserves the source algorithm's quirk within the bounded domain. It extends
M33's queue provenance validation, not its producer/recipient ownership. Do not
claim the reference saved this transient treasure: M34 intentionally makes these
produced consequences durable to meet exact restart continuation, as M33 already
did for delayed ready treasure.

`AttritionAfterEscape` and victory do not clear readiness or money. After
relocation/retirement, ready treasure is delivered only when no selected live
threat remains; otherwise it stays ready and pending. Dormant items do not start
a receipt even when the destination is threat-free. The full active roster is
the recipient domain; escaped members are reintegrated before delivery and
casualty conversion is already visible to eligibility checks.

Retain M33's Weapons-then-Armor, retained slot order, first eligible active owner,
tail-capacity test, exact item bytes, stable insertion, global-full warning and
explicit capacity/recipient losses. Publish all item insertions/removals before
receipt presentation; publish gold credit and clear its mask on the final
acknowledgment only. Forfeiture is a separate finish observation, not a receipt
that can accidentally credit money. Show gold forfeited and items held dormant
truthfully; never label dormant items delivered or irretrievably lost.

## Time, RNG and persistence

### Charges and draw order

| Boundary | Time / ctr24 / RNG |
| --- | --- |
| Rejected/stale Run, target choice, presentation or capture refusal | None; no queued action |
| Accepted Run | One accepted 1..100 draw plus conversion rejections; zero time/ctr24; consume member action |
| Continuing enemy turn | M33 damage draws over the remaining view, after Run's publication |
| Existing ordinary round | Selection/enemy work, one movement/ranged opportunity, then one-minute M33 tick; no extra round for mask shrinkage |
| Transferred attachment opportunity | Existing movement/ranged work; no invented ordinary-round minute |
| Successful victory End | Existing one-minute tick and successful-End gate |
| Non-victory finish / retirement / fixed relocation | Zero time/ctr24/RNG; only already owed work has its original charge |
| New contact attachment | Existing selection and genuinely transferred work; no entry reseed/HP initialization/extra Run charge |
| Subsequent navigation/Shoot/Wait | Existing M33 charges and draw order with full reintegrated active membership |
| Save, restore, inspection, frame/cache recovery | No gameplay time/RNG or consequence replay |

Keep the M33 daytime domain, 480/960 condition tick, checked byte/HP/XP/purse
bounds and refusal before unsupported dusk/dawn/daily/midnight/year work. A Run
at minute 1259 can disengage without a fictitious End minute; a genuinely owed
round crossing the unsupported boundary cannot be cancelled by escape. Failure
of that required work prevents retirement and saving, without undoing an earlier
Run draw, wound, kill, XP or item publication. Never call `changeTime(0)`.

### Version decision

Retain **save envelope v4**. Introduce **Journey schema 5 / content contract 5**.
The schema uses exactly the 4/4 field layout and suffix length
`1820 + 5*(weaponCount + armorCount)` from
[M33's wire contract](milestone-33-plan.md#exact-new-state-and-wire-layout), with
the schema/content discriminators changed to 5/5. No new durable field is needed.

The semantic revision is necessary, not milestone numbering: 4/4 requires every
item source bit in `pendingMask`, requires no items for an empty mask, and assumes
any pending queue is ready. A legitimate direct-exit save can contain a stored
item and **zero pending gold**. Encoding that as 4/4 would either fail its
validator, lose the item, restore forfeited money or change accepted legacy
meaning. Keeping 4/4 intact and admitting only 5/5 also prevents older readers
from accepting a state whose future delivery they cannot preserve.

In 5/5, retain every structural bound and canonical-byte rule from M33. At suffix
offset 1810 the mask means outstanding gold sources; offset 1814 remains pending
gold; offsets 1818/1819 hold W/A and records begin at 1820. Counts stay <=10 per
category and <=12 combined, with exact remaining length before allocation.
Require each mask bit AND each item source, independently, to identify a canonical
defeated/accounted original Orc 0..11. Item sources remain distinct across both
queues. Do not require an item source to be in the gold mask. Empty mask requires
zero pending gold but permits stored items. Preserve checked carried+pending u32
arithmetic, zero material/state/frame, valid category IDs, packed wire order and
canonical unused runtime slots. Loss/credit cannot be inferred backwards from
XP, inventories or actor count.

In the admitted content, a separate readiness flag, escape history, forfeiture
ledger or participation field would be redundant. Empty gold mask plus stored
items represents dormancy; nonempty gold mask represents readiness. Once delivery
or item loss removes a record, world accounting prevents recreating its source.
No partially executed receipt is saveable, so its publication phases need no wire
representation. M35 must revisit this equivalence before admitting another
producer that can create ready item-only treasure; that is not M34 scope.

### Safe capture and exact restoration

Keep the existing owner-bound `XeenJourneyCapture` and Flow save leases. Eligible
post-disengagement capture requires successful finish AND guarded retirement,
successfully presented Quiet, pending zero, no contact, no work/charge/projectile,
no Event/inventory/receipt/save/presentation lease, and no failure latch. Merely
being at the Run tile or having no combat pointer is insufficient.

Ready treasure may be saved only while selected threats make delivery ineligible.
A ready collectable queue must settle before Quiet. Dormant items can be saved
with or without selected threats; they create no immediate delivery obligation.
Validate the complete 19 actors, required activation, no same-cell live contact,
occupancy, closures, mainland camera and full roster using existing current-state
rules. M33 already permits Dead with positive HP and simultaneous Unconscious/
Dead; no death-cause field or HP-sign normalization is necessary for casualties.

Restore 5/5 through the existing unpublished-candidate pipeline: decode, validate
full saved owners, verify original manifest/profiles, derive topology/closures,
apply all saved mutable actor fields/accounting, validate readiness/contact/
activation, retain provider/resource preimages, preflight a concrete first frame,
and publish once into fresh owners. Startup performs no Run, casualty conversion,
time, RNG, movement, activation, attack, XP, production, delivery, relocation or
Event dispatch. New transient bindings start with no combat; later contact starts
a new full participant mask. No CHR/PTY fallback or reseeding can override saved
values. Save bytes and complete semantic state must match the uninterrupted
branch before AND after the same subsequent mutation.

| Existing domain | Required compatibility |
| --- | --- |
| v1/v2 ordinary | Existing missing-item-field v1 restoration and exact ordinary v2 behavior; no Journey inference |
| v3 completed Diagnostic27 | Existing successful victory, read-only inspection and R revisit; no Run or mutable regional conversion |
| v4 Journey 1/1 | Exact 1060-byte suffix and legacy seed/Skeleton domain |
| v4 Journey 2/2 | Exact 1366-byte suffix, expedition/collection, Luck/RNG and legacy combat |
| v4 Journey 3/3 | Exact 1651-byte suffix and regional contact/ranged/time support stops |
| v4 Journey 4/4 | Exact M33 rules/layout/strict treasure-source relation; no Run, dormancy or migration |
| v4 Journey 5/5 | This document's runtime and semantic extension; unchanged physical layout |

Reject unknown/crossed pairs, missing optional fields, extra bytes, noncanonical
records, forged source/accounting combinations and incompatible resources. Retain
existing archive fingerprints, CRC32, 4 MiB limit, exact EOF and atomic file-write
policy. Do not serialize participant masks, exit causes/proofs, scan cursors,
transient attack/work queues, acted/blocked bits, targets, initiative, leases,
revisions, frame IDs or UI state. The durable pending-gold mask and stored item
queues remain serialized exactly as specified above.

## Publication, resource and input contracts

Retain `XeenCombat::Ticket`, busy/reentrancy gates, world encounter revision,
Journey owner/generation, boundary leases and `XeenRestoreGuard`. Extend those
existing preimages and state transitions; introduce no new general authority
framework. A Run result, finish result or retired notice is an owned observation,
not a replayable command or transferable completion ticket.

Every candidate retains the actual world/party/roster/camera/flag owner lifetimes
and incarnation, exact full mutable preimages (including all supplements,
conditions/items, treasure, accounting, actor fields, context and RNG), the
participation/acted/blocked/contact/selection/exit-intent preimage and the boundary
generation. Retain all admitted immutable DAT/MOB/EVT/statistics and required
sprite guards. Run threshold and destination must come from that same retained
map identity, including after cache discard; a reloaded equal pointer is not
evidence and a changed value cannot be legitimized by guard renewal.

The publication units, in order, are:

1. Run candidate: accepted draw/RNG, action consumption, mask/result/next work.
2. Subsequent complete resource attack or owed movement/ranged/time unit, each
   with the inherited M33 atomicity and observation adoption.
3. FinishDisengagement: casualties, applicable money forfeiture, camera,
   destination activation/classification and successful-finish proof together.
4. Guarded retirement: consume proof and transfer coordination/borrow/resource
   union, with no gameplay consequence rewrite.
5. Required new attachment or ready treasure delivery, then existing Event/
   Presentation handoff; only afterward may Quiet/capture be possible.

Consume an input intent before any callback/random/resource work. Preallocate
mask/result storage, source lists, guard replacement and finish observations;
check overflow and all providers before nonthrowing stores. Recheck ticket,
owners, full preimages and leases after every callback and immediately before
publication. Include owner destruction/replacement, byte-equal copy/move/ABA,
stale completion, same-address new combat, changed map internal ID and stale
resource retry in failure controls. Stale work cannot fail, overwrite, retire or
release a newer owner's work. A current integrity failure latches that graph
unavailable; later equal bytes cannot reopen it.

Unpublished failure changes no gameplay owners or world RNG for that unit.
After publication, later failure preserves the entire published prefix and its
obligations. For example, a finish allocation/resource failure cannot put an
escaped member back into the fight or refund its RNG, and a receipt failure
cannot restore forfeited gold or redeliver inserted items. Failed graphs expose
exit/diagnostics but cannot capture or continue gameplay by deleting the combat
object. Recovery is only the existing bounded presentation retry: recompose
retained observations with no new gameplay call. An incompatible resource value
permanently invalidates the graph even if a later provider returns matching data.

Preserve M33's distinction between semantic input generation and exact concrete
frame identity. Every accepted Run (including failure), participant change,
exit-intent adoption, finish, retirement and new attachment invalidates old
semantic input. Cosmetic redraw alone may preserve the epoch. Only the exact
current owner's composed `IndexedFrame` binding successfully uploaded/presented
can open its boundary; no previous-incarnation or reordered frame can do so.

R uses the same SDL held/repeat/timestamp/batch fences as Space, B, targeting and
F9. Multiple R events sampled in one batch must not escape several members, even
if the first publication produces another ready member. R/Space/B/number/F9 held
through partial escape, a receipt, retirement or reattachment cannot act on the
next phase. F9 must refuse before capture, resource/preflight or filesystem work
in every unresolved phase and must never queue a later save. Fresh controls after
a valid matching frame remain responsive; do not block them merely because an
unrelated cosmetic frame changed.

## Concrete implementation seams and presentation

The following changes belong to a future authorized implementation, not this
planning task:

- `XeenJourneyContent.h`: add explicit contract 5 and bounded descriptor queries
  for consequence behavior (4 or 5) and disengagement (5 only). Audit current
  `==4`/`!=4` branches in ActorApproach, Combat, JourneyRules, SaveState/Format,
  Journey/Encounter/Event Flow, Gameplay and CloudsMapComposer. Share M33 logic
  without accidentally admitting Run or relaxed treasure rules to legacy 4.
- `XeenCombat.h/.cpp`: add `XeenCombatCommand::Run`, a Run operation/result with
  acting owner, accepted roll/success and before/after mask; read-only participant
  mask observation; pending disengagement work, successful `Disengaged` phase and
  explicit exit cause. Add a private guarded non-victory retirement path alongside
  victory retirement. Keep the successful-finish proof private to the coordinator.
  `sortOrder/selectNext/defeated/reconcile`, enemy scheduling and all-asleep handling
  must use the correct participation predicate. No movable/copyable combat object.
- `XeenCombatRules.*` and `XeenRegionalRules.*`: pass an explicit bounded target
  mask into `XeenEnemyAttackCandidate` and `XeenRegionalOpportunityCandidate`.
  Keep fixed six-owner storage for detached deltas/results; derive ordered target
  indexes for random selection. Exploration callers use all six. Time candidates
  intentionally remain full-party. `XeenJourneyProgression.h` takes the explicit
  XP mask. Preserve result bounds; smaller participation needs no smaller owner
  arrays or translated roster IDs.
- `XeenMonsterTreasure.*`: make validation content-aware, retaining strict 4/4
  behavior and the independent stored-item/gold-source rules for 5/5. Distinguish
  ready money, stored items and dormant items in queries/inspection. Extend the
  existing candidate preparation for direct forfeiture; do not route it through
  gold credit or erase item arrays. Update lethal, receipt, guard equality and
  snapshot validation callers together.
- `XeenJourneyFlow.cpp`/`XeenEncounterFlow.cpp`: prepare the post-finish guard before
  consuming authority, union both guards' retained resources, adopt exact changed
  camera/party/world values, reset only obsolete encounter cosmetics/selection,
  and retain the finish observation. Select `Attachment` or `Presentation/Reward`
  from the published destination view, never unconditional Exploring/Quiet.
- `XeenEventFlow::prepareJourneyTransition`: dispatch the appropriate retirement
  gate, then service mandatory follow-up without a one-frame Quiet gap. Keep Event
  and receipt leases exclusive and preserve passive-sign semantics. Do not let the
  current victory-only branch silently leave Disengaged terminal forever.
- `PlayerAction.h`/Flow/SDL: introduce typed `RunAction`. Preserve existing native
  R mapping (`RevisitCompletedAction`) and normalize it to `RunAction` only inside
  contract-5 combat after input-generation checks, as contextual Space already
  becomes Attack. Completed Diagnostic27 keeps R revisit; exploration/legacy
  combat refuse Run. Typed test input and native R must reach the same command.
- `XeenSaveSnapshot`, `XeenSaveFormat`, `XeenSaveState`, `XeenJourneyCapture` and
  restore guards: admit exact 5/5 with the same fields and revised provenance/
  readiness validation. Extend serializers' current schema-4-only suffix gates
  explicitly; do not infer schema from map/camera. No transient finish proof enters
  any snapshot. Refresh the fresh-entry default and CLI observations to 5/5 while
  retaining explicit 4/4 regression setup.

The combat panel must show the acting member, R=Run, participants versus escaped
members, remaining conditions and selected actor identity/current HP. Keep the
roster/portrait positions stable, marking escaped members rather than renumbering
their owners. Show a failed attempt and who succeeded; neither may look like an
attack miss or party-wide victory. Show abandoned casualties, destination/facing,
surviving threats, forfeited gold and dormant versus ready/delivered treasure.
Keep notices bounded/paginated using existing presentation; text capacity cannot
truncate mechanical consequences. English is canonical for development feedback.

Reuse original MON/ATT/portrait resources and the existing indexed scene and
100 ms appearance service. An old contact's ATT frame must not transfer to a
destination contact occupying the same row. Pending/projectile observations must
be presented or recovered before cleanup; their damage is never recomputed.
No new audio system, original asset copies, HUD framework or inventory redesign
is required. Escape/window close retain their existing interaction/exit meanings;
Run is R, not Escape. Read-only complete state inspection must expose masks and
exit reason during combat and durable actor/treasure state after retirement.

## Implementation checkpoints

1. **Content and durable semantics:** add contract/schema 5 admission and treasure
   validation/query distinctions with 4/4 regression controls; prove the dormant
   queue codec/restore cases before exposing fresh contract 5 to gameplay.
2. **Participant-aware consumers:** add the mask to initiative, targeting, group
   attacks and XP; preserve full-party time/treasure recipients. Validate six-to-
   zero matrices and unchanged full-mask M33 traces.
3. **Run publication:** connect the typed command and exact signed-threshold draw
   to existing service/tickets, action consumption and semantic input fencing.
4. **Finish and retirement:** add cause-specific disposition, casualties, owed-work
   drainage, fixed relocation, activation and separate successful-finish proof;
   integrate guarded retirement/new attachment/Reward/Presentation recovery.
5. **Connected continuity:** exercise surviving identities, return, repeated escape,
   later death, dormant-item reactivation, explicit save/fresh-process continuation
   and further mutation. Extend existing witness infrastructure and native input
   controls; close authority/resource/publication faults.
6. **Closure evidence:** build, full CTest, original-resource/process witnesses,
   maintainer physical native-SDL acceptance and required independent review.
   Only then perform separately authorized milestone closure documentation/Git
   work. A checkpoint pass is not authorization for a new milestone or a push.

## Acceptance requirements

All requirements below await implementation. Keep three evidence classes separate.
Reuse existing test targets/helpers instead of a parallel acceptance framework:
`XeenConsequenceRulesTests`, `XeenConsequenceOriginal`,
`XeenConsequenceReviewControls.h`, `XeenConsequencePhysicalControls.h`,
`XeenConsequenceCliWitness`, `RunConsequenceWitnesses.py`, combat authority/gameplay/
persistence tests, Journey/regional resource and persistence tests, and save/SDL
authority tests. Production input must travel through `Application::playGameplay`,
displayed input, Event/Encounter Flow, actual actor/consequence owners and F9.

### Automated deterministic correctness

Require bounded matrices and literal draw traces for:

- Threshold -128, 0, 1, 2, 99, 100, 101, 127 and draw endpoints/equality; conversion
  rejection, yield at raw budget, cursor overflow and stale/pre-publication failure.
  Map-23 success 1..99/failure 100 is mandatory.
- Each owner as acting runner; none/some/all bits cleared; all subset sizes and
  stable owner order; initiative ties, already-acted peers, joins and next-round
  reset. Failed Run consumes an action without changing membership or target.
- All relevant action/target/XP/recipient predicates with Good, Poison, Disease,
  Sleep, zero Speed, Unconscious, Dead and simultaneous admitted conditions.
  Include sleeping-only remaining participants versus genuine exhaustion.
- Preferred-class/random/fallback and all-party attacks over masks, including
  singleton target draws, excluded escaped members, dead remaining targets, all
  no-target ranged queues and no invalid empty interval. Preserve full-mask traces.
- DirectRun versus attrition after Attack/Block/failed Run; last capable member
  escaping beside unconscious/dead members; already-Dead counter preservation;
  victory after escape; full-party defeat at an owed tick; no false victory based
  on an older accounted actor. Casualty conversion changes only the Dead byte.
- Owed attachment versus charged-round opportunities, ranged sources that join,
  exit intent during owed work, all-party attack completion and Zombie cutoff.
  Verify no discarded charge/work or invented post-escape round/End minute.
- Destination same as origin, destination occupied, off-contact selected threat,
  all four facings, changed Run metadata/terrain/EVT, and immediate new incarnation.
  Artificial destination-occupancy/resource fixtures are labelled as such.
- XP before/after escape with checked division/overflow; no reward replay on
  repeated retirement/death observation; prior gold credited versus pending gold
  forfeited; stored items retained dormant; later no-item Orc kill reactivating old
  items without old gold/XP; non-Orc kills leaving them dormant; repeated Run,
  capacity loss, all-full tails, no recipient and separate final gold acknowledgment.
- Quiet dormant-item saves without selected threats; ready delayed saves with
  selected threats; rejection of collectable ready saves, contact or missing
  activation. Corrupt/crossed schemas, source-mask independence, forged living
  source, duplicate source, noncanonical arrays, exact lengths and ordinary/legacy
  behavior. Include positive-HP Dead state and exact simultaneous condition bytes.
- Every publication unit's before/after failure, reentrancy and ABA controls;
  cache/resource renewal, changed internal identities and retry failure latches;
  held/repeated/stale/same-batch R/Space/B/target/F9; wrong/missing/reordered/foreign
  or previous-incarnation concrete frame. Assert zero save-provider/file calls on
  blocked F9 and responsiveness to fresh input after a valid handoff.

Artificial tapes/fixtures are appropriate for the rare rule and failure matrix.
They must not be presented as route-reachable original gameplay. Ordinary CTest
must remain independent of commercial resources. Run the full CTest suite at
milestone closure, not only helper tests.

### Genuine production, original-resource and process witnesses

Extend the existing consequence runner with explicit combat action policies,
without altering production RNG/damage/resources or injecting live state. Keep
the M33 route alphabet (`U` forward, `L/R` turn, `F` exploration Shoot) distinct
from typed combat Run; a route R must not ambiguously become Run. A combat-policy
record supplies Run/Block/Attack at actual presented ready boundaries. Automatic
work and receipt acknowledgment use their production paths.

Retain M33's committed recipes in `tests/RunConsequenceWitnesses.py`: seed-3 `UF`
wound and `UFU` contact/generated armor, seed-7 Snake/Poison route,
seed-226 Toad/Sleep route, seed-18 undead route and seed-64 generated missile.
These existing recipes are retained evidence; their M34 variants have not passed
yet. Preserve 4/4 regressions explicitly rather than treating new fresh 5/5 output
as byte-identical legacy saves.

Require the following connected witnesses; fixed prefixes may be followed by a
bounded deterministic selection recipe, then the selected literal transcript is
retained and replayed without search:

| Witness | Production setup/actions and required observations |
| --- | --- |
| Wounded survivor escape/return | Seed 3, genuine `UF` prefix, then forward into actor-9 contact. At ready turns Run until disengagement. Keep actor 9 alive at its inherited wound, verify fixed relocation and exact durable state; save/restart. Return toward that same live identity through ordinary mainland moves and re-engage; run again, then return and kill it. Record actual contact/movement positions, not assumed spawn coordinates. |
| Failed and mixed Run | From the same contact prefix, bounded seeds 1..4096. First ready turn Run, then Run at each new ready turn until a failure and at least one success have occurred. Retain first ascending seed satisfying both while nonterminal. A separate replay branches after the first success to Attack through victory, proving escaped-member XP exclusion and later reintegration. |
| Casualties after escape | Use the retained M33 Snake/Toad/undead route families and actual grouped contacts; try fixed retained seeds first, then ascending 1..4096. Let the first ready owner escape, Block with remaining members until participant exhaustion, bounded by 128 resource attacks/600 player inputs and the existing time boundary. Retain a non-defeated full-party attrition exit, casualty bytes and exact treasure disposition. Also retain a direct Run beside an already unconscious member when found. Rare predicate combinations remain separately artificial; at least one genuine casualty escape is required. |
| Treasure through direct exit | At genuine grouped Orc contact on the retained mainland route families, Attack the lowest original-index contact until a kill produces a retained ordinary item while another contact survives, then Run remaining able members. Search seeds 1..4096 in ascending order with the same input/time bounds. Assert forfeited gold, dormant item/source retention and retained defeated accounting. Save/restart, kill a different Orc through production play, then obtain no-selected-threat delivery; compare item/recipient bytes and only the new gold credit. |
| Ready delayed treasure / attrition | Use the inherited delayed-treasure route in the runner and the casualty policy. Preserve ready pending ownership across a non-DirectRun ending, a selected surviving threat and restart; turn/move until the original selection predicate permits settlement. A separate artificial dormant-item/full-capacity fixture supplements, never substitutes for, the genuine treasure witness. |
| Occupied destination and rare failures | Genuine bounded routes should report when reached; deterministic labelled fixtures establish the full state matrix even if that original route is rare. They cannot replace the genuine wounded-survivor, casualty or treasure chains above. |

For return routing, use a deterministic test-side shortest path over the existing
121-cell mainland to the retained survivor's latest cell: neighbor tie order
North, East, South, West; minimal turns, Right for a 180-degree tie; emit only
ordinary production navigation actions, settle each mandatory boundary, and
replan after each publication. Cap at 64 charged moves, never cross an unadmitted
map/Event/time boundary. This chooses inputs, not actor locations or RNG. From
Run `(10,12)` facing West, the static route toward the original seed-3 contact
cell `(7,11)` is `LURUUU`; actual earlier recontact must be adopted rather than
forcing the endpoint. Log the exact emitted sequence and survivor identity.

The bounded searches are automated preparation, not acceptance by hope: absence
of a required genuine witness at the stated bounds is a failing acceptance gate
to investigate/report, never authority to inject a fixture, extend time, heal or
claim success. Do not make the maintainer hunt seeds or transcribe state. Record
selected seeds, original setup, full typed input/idle policy, literal accepted
and rejected draw intervals/counts, actor/participant transition observations and
each expected semantic checkpoint. Replay selected cases deterministically.

For every required post-retirement save family, create three real process paths:

1. Uninterrupted production input to checkpoint and through a recorded suffix.
2. Production F9 at that checkpoint, process exit, fresh `--load-game`, exact
   comparison before any new input, then the identical suffix.
3. Another save/restart after that suffix and an identical further mutation,
   including re-engagement or final survivor death/treasure settlement.

Compare complete semantic snapshots AND exact save bytes at matching points:
all 30 characters/supplements, active membership/order, current HP/SP, conditions,
all exact item arrays, purse and both gold/item provenance domains, context,
camera/flags/quests/overlays, all 19 actor identities and mutable fields,
accounting, and RNG algorithm/state/raw count. Compare publication observations
for non-saveable intermediate boundaries. Visible HP, screenshots or a hash alone
are insufficient. Account for header CRC/discriminators normally; do not ignore
fields to obtain an equality. Uninterrupted/restored branches must use the same
contract; 4/4 legacy comparison is a separate regression.

Extend `RunConsequenceWitnesses.py` to retain new evidence beneath a dedicated
`<build>/m34-evidence` directory: selected recipes, process commands/exits, full
state comparisons, draw transcripts and saves, excluding commercial data. Retain
the runner's existing 4/4 cases and label artificial outputs explicitly. These
locations are a future acceptance recipe, not a claim that M34 logs exist now.

### Maintainer physical native-SDL acceptance

Automation prepares the selected genuine seeds, short transcripts and quiet saves
above, plus a small launch recipe using the existing executable, legal installation,
`--journey-region --combat-seed <selected>` and an external `--save-file` path.
The seed-3 setup begins with Up, F, Up, settling each boundary. A native Run test
continues from actual combat; never require a combat snapshot or debug teleport.
Provide a direct `--load-game <installation> <prepared-quiet-save>` launch for the
return/restart portion. Clearly label any optional artificial visual control.

The maintainer's short checklist is limited to human judgments:

1. R applies to the named current member; failed/successful feedback and escaped
   versus incapacitated/dead portraits are understandable during partial combat.
2. Relocation, casualty/treasure feedback and any immediate new contact are clear;
   old attack effects do not appear attached to a replacement actor.
3. Fresh movement/combat controls respond after handoff; holding R/Space/F9 across
   it does not cause an unintended action. F9 communicates unavailable versus saved.
4. Restart shows the expected wound/conditions/dormant-or-ready treasure clearly,
   and return/re-engagement remains responsive. Receipt text identifies actual
   delivery/loss without suggesting healing or lost gold has been restored.

No manual seed search, arithmetic verification, invariant transcription or repeated
save-byte comparison belongs in this checklist. Physical native-SDL acceptance is
maintainer evidence, distinct from deterministic automation, headless/original-
resource witnesses, image inspection and independent technical review.

## Exclusions, review notes and M35 handoff

Do not add recovery, Rest, resurrection, services, training, spending, general
magic, item use, new monster profiles/loot levels, party recruitment/reordering,
combat-time inventory mutation, new maps, swimming/mountaineering, disconnected
map-23 geometry, map transitions, new quest scripts, unsupported calendar work,
save-anywhere snapshots, autosave or in-session load. Preserve all current
ordinary/diagnostic controls and the original commercial resources unchanged.

There is no material geometry/objective blocker: the original Run cell is already
admitted. The persistence semantic extension and dormant treasure are the material
findings beyond a naive escape implementation; they fit the approved M34 arc.
No roadmap change or M34 subdivision is proposed. Reference index/mode accidents
are resolved into the explicit stable-identity/exit-cause contract above; this is
identified integration policy, not claimed independent DOS observation. Physical
original-game comparison of those quirks is not available from this planning task
and is a nonblocking provenance limitation for independent review.

M35 retains connected **Myra -> Phirna -> Myra** and selected local recovery/
antidote item use. It receives the same mutable regional owners, exact injuries/
conditions/items/XP/purse, dormant or ready monster consequences and continuous
time/RNG. It must consume those owners rather than reconstructing history from
old encounter UI, and must revisit treasure readiness if adding a new producer.
M34 does not execute that quest or introduce recovery to make its witnesses easier.
Review approval of this candidate is distinct from implementation authorization.
