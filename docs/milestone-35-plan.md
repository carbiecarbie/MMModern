# Milestone 35 - Connected Myra quest and local recovery

## Final scope and acceptance boundary

M35 completes a bounded production route in fresh `--journey-region` content
contract 6: explicit Myra request at `(9,11)` West, ordinary mainland travel and
encounters, Phirna collection at `(8,2)`, connected return and Myra exchange,
quiet save and fresh-process restart, further travel, the selected `(7,7)` well,
use of an actually delivered antidote, and continued gameplay. The same live
Journey, world, roster, party, camera, RNG and consequence owners persist. This
does not establish unrestricted map-23 or Clouds gameplay. Gold and XP remain
durable without spending or progression consumers.

M35 inherits [M21 endpoint/reward rules](milestone-21-plan.md),
[M31 Event authority](milestone-31-plan.md), [M32 regional admission](milestone-32-plan.md),
[M33 consequences](milestone-33-plan.md) and [M34 disengagement](milestone-34-plan.md).
The original commercial installation supplied read-only data; [dependencies](dependencies.md)
records the pinned ScummVM reference revision and configuration.

## Original evidence and regional admission

**ORIGINAL:** Clouds map 23's checked DAT/MOB/EVT manifest defines the connected
121-cell mainland and all 19 actor identities. Myra is EVT 21..35, MOB object 1
at `(9,11)` West; Phirna is EVT 125..135, object 13 at `(8,2)` All; the well
is EVT 57..66, object 4 at `(7,7)` All. The inherited sign is record 56 at
`(5,9)` North and is the only admitted automatic mainland event. Fresh
`maze.pty` world flag 16 is false; miscellaneous inventories start empty.
Myra's five `{material=10,id=37,state=1,frame=0}` items arise only after exchange.

**REFERENCE:** The pinned ScummVM interpreter gives the well's Action 78 value
1 when current HP is at or below live maximum HP; mode 8 adds 25 HP without
clamping and mode 103 sets world flag 16. The item path checks source eligibility,
debits before target selection, clears/sorts an exhausted record and services a
monster opportunity. Item spell 37 clears Poison with the bounded
`addHitPoints(0)` side effect. These are source findings, not observed DOS play.

**IMPLEMENTED:** Contract 6 retains contract 5 terrain, actors, combat/Shoot/Run,
conditions, treasure and time behavior. Exact descriptors bind physical camera,
first matching original EVT record, direction, object, logical closure and
resource preimages. Disabled records become effective None without deletion,
reordering or a second lookup; topology remains validated. Myra requires West;
Phirna and the well accept all directions. Called scripts, cross-address
transfers and out-of-closure lines refuse. Manual Space uses camera/facing;
automatic dispatch retains the original bit. Entering or restoring at Myra
does not request the quest. Other map-23 scripts and transitions remain excluded.

**INFERENCE:** Well voice opcode 0x28 index 2 is counted as an original cosmetic
instruction without audio playback. Original text, selection and HP feedback
remain visible. This is a bounded presentation adaptation.

## Connected quest and publication

- With no Root, Myra follows original lines 0,1,4,5,6. Final NPC acknowledgment
  sets Q2. Paging alone grants nothing; the SP comparison has no cost.
- Phirna presents Yes/No. No and already owning a Root produce no grant or
  Remove. Yes with no Root acknowledges success text, then line 6 grants one
  Root (counter 17). Line 7 independently removes object 13 if present and
  disables EVT 125..135 at the physical cell. Remove restarts logical line 0;
  disabled records run as None to natural completion. The successful original
  north-facing chain dispatches 18 instructions; it is not shortened after
  Remove. Grant-only partial state remains independently durable.
- Root possession, rather than Q2 or a completion flag, selects Myra's return
  branch. Final NPC acknowledgment consumes one Root at line 8, clears Q2 at
  line 9, then lines 10..14 each enqueue one exact `{10,37,1,0}` reward.
  Existing warning, synchronous delivery, paginated receipt and final
  acknowledgment apply even when capacity/eligibility loses every item.
  Delivery follows active order, current `canAct()` and miscellaneous tail
  capacity; it neither displaces items nor refunds the Root.
- A later Root legitimately permits another exchange. With no Root, another
  explicit request may set Q2 again after Phirna's removal. Neither is replay.

`XeenEventFlow` remains the noncopyable continuation owner. Its stack-bound
publication capability admits only exact quest-flag, counter, Remove, reward,
delivery and well sites. Prepared deltas adopt retained preimages rather than
callback-mutated state. Root count, Q2, overlays, reward insertion and receipt
are separate ordered publications. Later failure retains published effects;
stale frames/reports cannot replay an instruction or response. Event rewards
are transient, distinct from party-owned monster treasure: they never set
pending monster masks/gold or reactivate dormant Orc items. Due collectable
monster receipts settle before a new interaction.

Retained Event resource authority includes the complete map-23 text preimage,
not only EVT/MOB. A changed admitted value permanently invalidates the graph;
ordinary unavailable resources before admission publish nothing. Trusted
recoverable manual presentation failures retain prior effects and may redraw
only the already owned observation, while automatic/integrity failures close
gameplay and capture. Boundary leases transfer directly between Event,
Presentation and Journey work without a Quiet gap.

## Selected well and bounded item use

The `(7,7)` well is the only newly admitted recovery site. Original WhoWill
selects an eligible active member with F1-F6; Escape before selection cancels
without mutation. Original text indices 17, 18 and 11 provide selection,
success and refusal. The live maximum-HP predicate permits use while current HP
is **at or below** maximum. It adds exactly 25 HP without clamping, including
at maximum. Repetition remains effective while the predicate permits it,
including after later damage. Above maximum, original refusal/refreshing-water
text appears with no gain or flag write. There is no daily quota. HP publishes
before success acknowledgment; world flag 16 publishes separately afterward.
An interrupted chain can retain HP with flag false. The flag is a persistent
marker, not eligibility. The well does not restore SP, clear conditions,
resurrect, repair gear, charge time or consume gameplay RNG.

Only fresh contract-6 exploration admits native **U** on an explicitly selected
occupied Miscellaneous record with material 10, ID 37, 1..63 charges and no
cursed/broken bits. The source must satisfy current `canAct()`. Other items
remain inspectable/storable but have no use effect; combat-time use and legacy
contracts do not gain U. Confirmation names the item and warns that a charge
is spent even if target selection is cancelled. Escape before Enter confirmation
is free. Enter debits once, then a fresh F1-F6 may choose any active target,
including source or Dead member; Escape here retains debit with no target effect.

An accepted target loses Poison only. The `addHitPoints(0)` side effect may
clear Unconscious for a living positive-HP target; numeric HP, SP and other
conditions do not change. A Dead target remains Dead; a healthy target still
spends the charge. At zero charges, all four source bytes clear and only its
miscellaneous category is stably compacted, preserving surviving bytes/order
and frames. With charges remaining, other slots and holes stay unchanged. No
SP/gem/gold cost or refund applies. Target selection or cancellation owes one
existing regional actor opportunity before Quiet, even without a target effect.
Selection/debit/effect costs no calendar minutes, ctr24 or direct RNG; the
opportunity may consume normal ranged-damage RNG. Pre-use refusal/cancellation
owes no opportunity.

Inventory certificate/epoch authorizes the source. Flow owns confirmation and
target UI; EncounterFlow owns retained debit, effect, settlement and opportunity.
Publication is ordered and exactly once at each boundary. After debit, failure
cannot refund or replay U; abandonment must settle exhaustion and the owed
opportunity or close gameplay/capture. Held/stale keys cannot cross phases.
Modal and owed work cannot be saved.

The target selector's presented concrete frame authorizes its F1-F6 or Escape
response. A debit alone, an unpresented or stale selector, a copied observation
or batched input cannot select/cancel a target or open F9 capture. Response
generation is consumed before fallible callbacks. Retained item feedback
survives immediate contact and can be redrawn without repeating debit, effect,
compaction or the owed opportunity.

## Consequences, resources and time

Existing HP/SP, conditions, equipment, inventory, XP, purse, actor wounds and
casualties, activation, monster gold and dormant/ready treasure retain M33/M34
owners and semantics. Myra rewards neither replace monster treasure nor credit
forfeited Run gold. The well and antidote create no treasure. Clearing Poison
may change later condition-tick RNG use; subsequent play follows the live stream.

The DAT/MOB/EVT/statistics manifest remains authoritative. Contract 6 preflights
and retains complete parsed `aaze0023.txt`, including valid empty entries,
before fresh or restored gameplay. Reload/cache values must match retained text
and identity. Wrong, malformed or changed admitted resources cannot become
successful empty interactions. Original FAC/scene composition uses existing
presentation authority. Item-name material fallbacks remain separate.
Commercial text is not copied into source or saves.

Inherited year/day/minute/ctr24 domain, movement, Shoot, combat, Run and tick
rules remain. Quest instructions, well actions, receipts and target input add
no gameplay time or RNG. Antidote opportunity uses existing bounded scheduler
and world RNG. No Rest, daily well reset or unsupported dusk/dawn processing
is inferred.

## Persistence and compatibility

Save envelope remains v4. Fresh Regional Journey uses **schema 6/content
contract 6**; its 5/5 suffix gains one boolean, world flag 16. Preparation
reads checked original `maze.pty` byte 725 bit 0; restore uses the saved value
without rereading PTY or running startup events. Optional party-owned recovery
projection exists only for contract 6 and participates in guarded copy, capture
and restore. Other world flags gain no meaning.

Legacy Journey 1/1 through 5/5 retain saved rules, controls and wire meaning.
In particular 5/5 retains M34 Run/dormancy but no M35 events, well or U; loading
does not upgrade it. Unknown/crossed pairs refuse. Quiet checkpoint saves
across request, collection, return, exchange, well and item use restore exact
owners, overlays, items, consequences and RNG without replaying any effect.
Event/modal/item work, combat, receipts and owed work are unsaveable.

## Exclusions and final acceptance

General questing, magic, item spells/effects beyond the selected antidote,
combat-time item use, general recovery/Rest, resurrection, services, broader
map-23 travel, normal Clouds startup and general presentation/audio fidelity
remain outside M35. Missing original well audio and portrait sparkle/glow are
nonblocking presentation fidelity omissions, neither M35 defects nor automatic
successor requirements.

The final corrected implementation passed four distinct acceptance classes:

1. **Automated deterministic validation:** full build and **96/96 CTest**,
   including bounded rules, ownership, failure/publication and compatibility.
2. **Genuine original-resource/process integration:** real executable/CLI
   entry and original scene/resource composition on the connected production
   route; fresh-process restarts at required checkpoints; all **10** checkpoint
   save-byte comparisons, **81** intermediate owner/RNG comparisons and **14**
   Event observations. Focused publication/failure controls and retained M34
   original-resource and M33 Snake/Toad/undead condition/restart families passed.
3. **Independent technical review:** initial review found material defects that
   were corrected. Focused independent re-review returned **ACCEPT** with no
   remaining material findings.
4. **Maintainer physical native-SDL acceptance:** the maintainer personally
   completed request, travel/encounters, Phirna Yes/collection, connected
   return/exchange/reward, full exit and `--load-game` restart, well recovery,
   antidote selection/use, correct Poison result, further save/restart and
   continued play. No blocking input, visual or gameplay issue was observed.
   Repeated well use followed its live maximum-HP predicate and original
   refusal behavior, so it is expected behavior. Well audio and portrait
   sparkle/glow were absent as scoped above.

M35 closes the approved M33-M35 arc. The next planning activity is a separately
authorized, evidence-based [post-M35 roadmap reassessment](roadmap.md#near-term),
not an already chosen or authorized successor implementation.
