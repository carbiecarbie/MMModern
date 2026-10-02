# Milestone 39 - Bounded already-learned combat casting

Historical record; not current rules or scope.

## Completion and scope

**Completed and accepted.** M39 connects existing learned books and finite
current SP to **Magic Arrow, First Aid and Awaken in combat** on the already
admitted Regional Journey mainland and Vertigo encounters. Acceptance includes
subsequent turns, wounds/lethals and existing consequences, quiet save, complete
process exit, fresh restore and further play. Fresh Regional Journey uses
**envelope v4 / schema 8 / content 10**. Older content retains its behavior,
including combat C refusal.

The [roadmap](../roadmap.md#near-term) retains the accepted M39-M41 sequence.
M40 planning/specification is next; completion does not authorize its implementation.

Inherited contracts remain in their natural homes:

- [M36 knowledge and mappings](milestone-36-plan.md#knowledge-identity-and-resource-model)
  and [recovery effects](milestone-36-plan.md#hp-and-condition-semantics).
- [M33 combat lifecycle](milestone-33-plan.md#regional-ownership-attachment-and-combat-lifecycle),
  [time](milestone-33-plan.md#conditions-and-the-minimum-time-extension) and
  [treasure](milestone-33-plan.md#gold-monster-treasure-and-equipment-closure).
- [M34 participation](milestone-34-plan.md#individual-run-and-partial-party-rules),
  [non-victory lifecycle](milestone-34-plan.md#lifecycle-and-non-victory-completion)
  and [identity accounting](milestone-34-plan.md#surviving-actors-xp-and-treasure).
- [M37 regional ownership](milestone-37-plan.md#durable-ownership-and-transition-publication)
  and [city reset](milestone-37-plan.md#exact-scripted-actor-reset).
- [M38 route](milestone-38-plan.md#exact-route-event-graph-and-city-consequences),
  [service time](milestone-38-plan.md#time-departure-and-scheduling) and
  [schema-8 wire layout](milestone-38-plan.md#exact-successor-wire).

Exploration First Aid/Awaken retain their existing effects, refund and settlement:
ten mainland minutes or one Vertigo minute, followed by the ordinary actor
opportunity. Combat casting does not use that exploration continuation.

## Original behavior and provenance

Selected numerical behavior follows ScummVM revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`; configuration and attribution belong to
[dependencies](../dependencies.md#pinned-scummvm-revision). Decisive reference areas
are Xeen `spells.cpp`, `dialogs/dialogs_spells.cpp`, `interface.cpp`, `combat.cpp`,
`character.cpp`, and the spell tables in `devtools/create_mm/create_xeen/constants.cpp`.
This is pinned-reference interpretation, not independently observed DOS behavior.
Stable party slots, explicit confirmation/result acknowledgment and the bounded
contact projectile are MMModern adaptations.

Original initialization supplies the thirty 39-byte learned books; M39 adds no
spell acquisition, refill or preparation adjustment. Names come from validated
`DARK.CC/spells.xen`. Magic resistance is byte 39 of the retained original
`DARK.CC/xeen.mon` record, distinct from physical resistance at byte 40.
Admitted Slime, Giant Snake, Orc, Skeleton, Zombie and Giant Toad profiles have
zero magic resistance. Skeleton/Zombie physical resistance remains 50%.
Combat uses the existing Regional Journey profile admission, not Diagnostic27's
Skeleton-only `validateCombat` restriction.

The reference learned `magicArrow()` selects `POW_ARROW=11` (`XEEN.CC/pow11.icn`),
not `POW_MAGIC_ARROW=12`. Its ranged caller uses the selected contact's
`RT_GROUP` damage/resistance path before stopping after one target; physical
`RT_SINGLE` damage rules would be incorrect. Resources remain external and
unmodified; no commercial payload is included in the repository.

## Spell rules and support admission

| Spell / global ID | Clerical slot | Wizardry slot | Druidic slot | SP / gems | Combat target |
| --- | ---: | ---: | ---: | --- | --- |
| Awaken / 1 | 1 | 0 | 1 | 1 / 0 | All distinct active roster owners |
| First Aid / 26 | 14 | absent | 11 | 1 / 0 | One remaining participant, including self |
| Magic Arrow / 45 | absent | 25 | 23 | 2 / 0 | One identity-bound live same-cell contact |

Paladin/Cleric use Clerical, Archer/Sorcerer Wizardry, and Druid/Ranger Druidic
slots. Knowledge requires an explicitly present book and a nonzero mapped byte;
raw values 1..255 remain lossless. Sentinel/out-of-range slots are not effects.
Effect recognition and context admission are separate: only content-10 combat
admits all three; eligible content-7/8/9/10 exploration admits only First Aid and
Awaken. Light and other unsupported learned rows remain visible and inert.

Commit requires the presented PlayerReady acting participant, unchanged
slot/roster binding, hasSpells, valid learned class slot, canAct(), positive
effective combat Speed, supported encounter/equipment inputs and enough signed
current SP. There is no caster switching, maximum-SP test or additional HP-positive
requirement. Checked widened subtraction debits fixed cost without clamping;
above-maximum SP is preserved except for the debit. Gems, purse, treasure and
item charges do not pay for these spells.

### Participation and recovery

The six stable active slots remain bound to roster owners. Combat's participation
mask records escape, not roster removal or death. Joining preserves it; only a
new episode after proper retirement reconstitutes participation.

First Aid accepts any remaining participant, including sleeping, paralyzed,
unconscious or terminal members, but rejects escaped/inactive targets. F1-F6
retain stable slots with escaped slots disabled. Awaken affects all distinct
active owners, including escaped and terminal owners, without restoring
participation. Shared effects deduplicate aliases by roster identity; this does
not widen production membership admission.

- First Aid fails on any nonzero Dead(13), Stoned(14) or Eradicated(15): no target
  mutation, no refund, and the action is consumed. Otherwise, current HP H at or
  below checked maximum M becomes `min(H+6,M)`; H above M stays exact. Widened
  arithmetic must fit i16. Clear Unconscious(12) iff resulting HP is positive;
  preserve Sleep, Poison, Disease and all other conditions. Healthy healing is
  a paid no-op.
- Awaken clears Sleep(8) and clears Unconscious(12) iff that owner's current HP
  is positive, without First Aid's terminal-condition predicate. HP, other
  conditions and noncaster SP stay exact. It does not resurrect; an awake party
  is a paid no-op.

Effects draw no RNG and publish only the prepared post-debit HP/condition delta;
self-target healing cannot restore pre-debit SP. Recovery preserves acted and
blocked bits: an eligible unacted participant can receive its remaining turn,
while an already-used turn never repeats. Enemy targetability, full-active defeat,
participant/condition XP eligibility and full-active treasure delivery retain
their separate existing predicates.

### Magic Arrow and exact RNG order

Enemy selection precedes debit. Keys 1/2/3 choose from at most three live contacts,
starting at the current selection. Confirmation binds side/root map/original
record, actor HP/state/statistics/resource identity, combat incarnation/revision
and contact membership. Reordering/replacement cannot redirect the operation.
Off-contact, distant and exploration targets remain excluded.

The detached `XeenMagicArrowCandidate` uses `XeenConsequenceDraw` and the existing
world-owned RNG in this order:

1. Validate magic resistance R in 0..100 and checked
   `L=max(permanentLevel+temporaryLevel,0)`. Validate `100+L` and `50+I` as
   supported positive signed-int intervals before debit, where I is the original
   monster resource/statistics index, not actor record, contact row or sprite.
2. R=0 skips the resistance draw. Otherwise request inclusive `U[1,100+L]`;
   a result less than R resists, yields zero damage and skips the saving throw.
   Equality passes; R=100 is not automatic immunity.
3. On passing, consume inclusive `U[1,50+I]`. The reference saving comparison is
   `draw<=I`, but both outcomes leave damage exactly **8**. No hit roll, damage
   die, critical roll, physical resistance or equipment mitigation applies.
4. Publish checked `max(HP-8,0)`, or unchanged HP on resistance. Existing lethal
   XP/drop/accounting preparation follows the spell draw before enemy, movement,
   round or End draws.

Thus the six admitted profiles each consume one accepted spell draw: Slime
[1,50], Snake [1,53], Orc [1,56], Skeleton [1,58], Zombie [1,59], Toad [1,63].
Even Slime consumes its saving throw. Algorithm 1, rejection conversion and the
64-raw-attempt service budget remain unchanged; rejected raw values advance the
detached cursor, and yields retain accepted prefixes. Menus, cancellation,
refund, acknowledgment and animation draw nothing.

## Combat action, turn and time

| Boundary | Publication | Turn consequence |
| --- | --- | --- |
| C, browsing, enemy selection, refusal | No durable change | Same acting turn reserved |
| Precommit Escape | No cost/effect | Return to newly presented same-actor ready frame |
| Final confirmation | Exact SP debit once | One action obligation, successor held |
| First Aid target Escape | Exact recorded SP refund once | Action consumed |
| Effect, resisted Arrow, terminal healing failure or no-op | Effect/outcome, debit retained | Action consumed |
| Invalid/stale/wrong-phase response | No new publication | Existing obligation remains |
| Technical failure after debit | Committed prefix retained | Terminal unsaveable failure |

Settlement marks only the caster acted, grants no Block, and retains the
participation mask and other acted/blocked bits. Derived initiative uses live
post-effect state and existing selectNext/contact reconciliation. The successor
stays private until projectile/result presentation and acknowledgment finish.
No new actor action or owed enemy work runs through a result-reading gap.

Casting adds **zero cast-specific minutes or ctr24 change**. Existing round and
successful End charges remain one minute, with full-active condition processing.
Existing moveDue, chargeRound, attack ordinals, joining, no-action cycles, Run,
attrition, defeat, support-stop and retirement rules remain intact. Mandatory
successor work uses bounded idle service, never a nested loop. Unsupported owed
time preserves committed spell/kill consequences and stops unsaveably.

## Ownership, authority and publication

`XeenCombat` owns the noncopyable casting continuation, reservation, phases,
private response capability, detached effect/RNG work and reserved successor.
`XeenEncounterFlow` coordinates ticket/frame handoffs and mandatory service;
`XeenEventFlow` and casting UI compose selection and settled feedback.
Application/Gameplay/SDL route phase-specific input through the existing loop.
No exploration CastingContinuation or parallel gameplay owner is created.

Roster owns books, HP/SP, conditions and supplements; party owns membership,
context, purse and treasure; world owns actors, accounting, RNG and lifecycle;
Application owns camera/flags. Public cast/result/RNG observations are detached
values and cannot authorize debit, effects, refunds or successor publication.

Journey activity stays Combat. The combat owner acquires the exclusive boundary
Work::Casting lease; cast operations validate exactly that lease, while ordinary
commands retain their quiet checks. Phase/lease changes mint fresh full tickets,
including boundary generation. Coordinator responses consume concrete-frame,
Flow/input and combat-phase authority before callbacks and pass a one-use opaque
capability to combat. Copied tickets, UI state and direct typed calls cannot mint it.

Publication units are:

1. Reservation/selection binds immutable resources, book/caster/target and authority
   without durable mutation. Ordinary precommit preparation failure can retry via
   a newly presented response; integrity failure cannot.
2. Cost publishes only exact SP debit and retained original-SP/action obligation.
3. Recovery or explicit First Aid refund publishes post-debit HP/conditions or
   exact original SP, caster acted bit, result and reserved successor together.
4. Arrow publishes HP/canonical lethal, once-only XP/accounting/drop/treasure,
   detached RNG cursor, acted bit, result and successor as one complete unit.
5. Projectile/result handoff advances presentation authority only; existing
   enemy/round/End/treasure work follows under fresh tickets.

All fallible preparation precedes stores. Final callback checks compare complete
guarded owners to the private reservation; stores and exact expected-delta guard
adoption have no intervening callback. No arbitrary live-state recapture or
full-character overwrite renews authority. Incomplete effect/drop/XP/RNG work
publishes nothing; post-debit technical failure retains the debit and any earlier
committed unit without retry/refund or further gameplay/save admission.

Guards include all thirty owners/books/items/supplements, membership, context,
purse/treasure, camera/flags, overlays, both regions, actor statistics, RNG,
lifecycle and the immutable name/resource union. Immediate mutation/reversion,
reentrant callbacks and nested newly inserted/reconstructed cache storage remain
observed before exposure. Matching retries or cache reconstruction never clear
monotonic integrity failure.

Presentation-only recovery may recompose the same owned prompt/projectile/result
with fresh frame authority, never repeat payment/effect or advance the successor.
A recovered post-debit First Aid target prompt can still accept explicit Escape;
a technical effect failure cannot be relabeled as that prompt. Fatal rendering
failure or window close preserves the previous disk save and ends the unsaved session.

## Consequences and native presentation

Wounds retain the same actor identity. Lethals publish once-only existing XP,
accounting and Orc drop/pending treasure; Slime retains 50 base XP and no treasure.
Delivery, receipts, gold credit, capacity/loss and DirectRun versus attrition
remain the existing consequence path. City reset alone creates a new admitted
Slime life; casting, revisit and restore do not respawn it.

C opens the acting member's book. Up/Down select, Enter reviews/confirms, and
Escape backs out before debit. Arrow uses precommit 1/2/3 enemy choice; First Aid
uses post-debit F1-F6 target choice with its refund/action warning. Known Light
remains visible and unusable. Six visible learned rows scroll within the 39-slot
book. Result feedback reports SP/refund, target, HP/conditions, resistance/failure/
no-op and pending successor; a fresh Enter/Space/Escape acknowledges once.
Six-owner feedback uses the existing right roster panel.

Arrow uses one scene-clipped POW11 row-0/frame-0, scale-0 projectile at (72,43),
order 124 outdoors or 162 indoors, before interface layers in the existing
renderer. It remains for a successfully presented frame plus a 100 ms cosmetic
interval, then gives way to the result. Damage already committed does not wait
for animation; replacement contacts cannot inherit feedback. This bounded
same-cell adaptation adds no distant flight, collision simulation or indoor Shoot.

### Input, saving and successor authority

Each actionable phase requires its own successfully presented concrete
IndexedFrame capability. Equal pixels, copied/prepared frames or texture upload
without completed presentation cannot authorize input. An idle cosmetic redraw
that retains the **exact same combat ticket preserves semantic input authority
and epoch**, but seals a new concrete frame and blocks responses until that frame
is successfully presented. Genuine semantic/ticket transitions invalidate prior
input. SDL retires queued keydowns at semantic handoffs; released fresh keys
remain usable on the unchanged acting turn after presentation.

Early, stale, held, repeated and batched inputs remain rejected; a batch cannot
open/select/confirm/target/acknowledge or act for a successor. Modal input cannot
leak into Attack/Block/Run, inventory, events or navigation. Precommit cancellation
and final successor handoff require their newly presented ready frames.

The lease persists through projectile/result and transfers to owed work without
a Quiet gap. F9 refuses before capture, providers, path/fingerprint preparation
or I/O throughout casting, combat/End, receipts, retirement and unresolved
presentation. It never settles or queues a save. Only a fresh F9 at the final
presented eligible Quiet boundary can save.

## Persistence and compatibility

**Envelope v4 / schema 8 / content 10** adds no durable field. Existing HP/SP,
conditions, thirty raw books/supplements, actor wounds/death/accounting, treasure,
context, both regions and RNG encode the settled state. Reservations, participant/
acted/blocked bits, selected targets, modal/projectile state, leases, revisions,
incarnations and input/frame authority are transient and unsaveable.

Accepted Journey pairs are exactly 1/1 through 8/8, plus 8/9 and 8/10; crossed or
unknown pairs refuse. Fresh --journey-region selects 8/10. The
[M38 wire layout](milestone-38-plan.md#exact-successor-wire) remains unchanged
apart from the explicit content value: suffix lengths are `3114+5*N` without
city, `3992+5*N` with 46 city slots and `4106+5*N` with 52, N<=12. Learned flags
remain raw bytes, not normalized booleans. Envelope checks, fingerprints,
canonical booleans, ordering and exact EOF remain mandatory.

Content 10 inherits content 9's sixteen-city-cell route, year 610/day 8..10,
daytime/ctr24 limits, retained-city requirement after day 8, actor/reset provenance
and treasure constraints. Quiet saved wounds need no spell-damage multiple or
historical kill-method proof. Saved SP and books, including inactive/all-zero
books, remain authoritative; there is no spell ledger or original-book backfill.

| Domain | Continued behavior |
| --- | --- |
| Ordinary v1/v2; completed Diagnostic27 v3 | Existing domain, no learned combat casting |
| Journey 1/1..6/6 | Existing knowledge absence and support limits |
| Journey 7/7 | Mainland exploration recovery, combat C refusal, no city |
| Journey 8/8 | Eleven-cell city route, exploration recovery, combat C/service refusal |
| Journey 8/9 | Sixteen-cell route, repair/day-10 boundary, exploration recovery; combat C refuses |
| Journey 8/10 | Same inherited services/route plus the three bounded combat spells |

Capture uses existing guarded Quiet authority. Restore validates detached state,
prepares fresh unpublished owners/resources, installs exact saved fields,
preflights the first frame and publishes once with fresh runtime authority.
Before first input there is no initialization, cast/effect/refund, movement,
combat, time/RNG, reward, service or reset replay. Loading/recapture never upgrades
legacy content merely because schema 8 is shared.

## Exclusions

No acquisition/guild purchases, new learned books, general spells, area attacks,
new monster statuses, offensive exploration/off-contact casting, Light/durations,
rest/SP replenishment, new routes/services, calendar expansion, Training,
Buy/Sell/merchant stock, new quests/species, unrestricted Clouds/startup or
Darkside gameplay. M40 service-day continuation and M41 progression remain future
work. No new ScummVM dependency, nested event loop or commercial-data modification.

## Final acceptance

The normal all-target build, focused rules/combat/save/guard/authority/process
validation, publication-authority adversarial controls, original-resource A/B/C
process witnesses and content-9/content-10 Ironworks regressions passed. The final
complete **unfiltered CTest passed 110/110**, and `git diff --check` passed.
The pinned ScummVM checkout remained clean at the revision above; commercial
resources were not modified. Independent technical review and required focused
re-review completed with no remaining material findings.

Production witnesses established useful healing, Orc and Vertigo Slime Arrow
lethals, Toad wounds, combat Awaken/recovery without repeated turns, and Run with
retained wounds. Actual quiet F9 saves, complete process exits, fresh loads and
identical continuation preserved encoded state, owner fields and RNG traces with
zero pre-input replay. Legacy combat refusal and inherited exploration/service
behavior remained isolated. Synthetic fault/authority controls are separate from
these original-initialization process witnesses.

Separately, **maintainer physical native-SDL acceptance passed**: useful combat
casting and projectile/results, unsupported Light, free precommit cancellation,
First Aid refund with action consumption, modal F9 denial, held/batched input,
recovery/Run/re-engagement, and save/full exit/fresh load/further play. Fresh
Attack/Block input at stable PlayerReady boundaries executed on every eligible
press, including across cosmetic redraws.
Automated, process and independent-review evidence does not substitute for this
maintainer-performed physical acceptance.
