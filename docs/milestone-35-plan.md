# Milestone 35 - Connected Myra quest and local recovery

## Status and acceptance boundary

**Technical contract candidate for independent review; not implemented or accepted.**

The specification baseline is `b2001711a20bf8aa3ff4fa86a8a5cedc6a69ab22`
(Complete Milestone 34 disengagement lifecycle). The planning gate verified `main`,
HEAD, `refs/remotes/origin/main` and direct remote main at that exact commit,
with an empty working tree/index, including untracked files. This document does
not authorize implementation, commit, push, closure or successor planning.

Implement this continuous production slice:

```text
fresh Regional Journey at Myra -> explicit request -> ordinary mainland travel
and encounters -> Phirna collection -> ordinary return -> Myra exchange/reward
-> quiet save -> fresh-process restart -> further travel -> selected local well
recovery and use of an actually delivered antidote -> further playable mutation
```

Use the same live Journey, world, roster, party, camera and consequence owners
throughout. The quest neither resets them nor creates a terminal completed mode.
Gold and XP remain durable consequences without spending or progression consumers.
The acceptance route includes genuine acquisition, combat injuries and Poison;
it does not assume potions on the outbound trip. Recovery is one selected
mainland well plus the narrow antidote operation defined below.

Inherited ownership, scheduling, combat, time, resources and presentation remain
those of [M29](milestone-29-plan.md), [M30](milestone-30-plan.md),
[M31 Event integration](milestone-31-plan.md), [M32 regional admission](milestone-32-plan.md),
[M33 consequences](milestone-33-plan.md) and [M34 disengagement](milestone-34-plan.md).
[M21](milestone-21-plan.md) owns the original endpoint/reward rules; this plan
changes their admission and guarded coordination in the new Journey contract,
not their quest meaning. [Project status](project-status.md) remains the stable
implemented baseline; [roadmap](roadmap.md#approved-m33-m35-arc) owns strategic scope.

## Evidence and decisions established during planning

Labels have specific meanings: **ORIGINAL** is decoded commercial resource data;
**REFERENCE** is source behavior at the pinned ScummVM revision; **IMPLEMENTED**
is current MMModern code/tests or an explicitly identified baseline run;
**INFERENCE** is the chosen bounded integration policy. No reference finding is
represented as observed DOS execution. Commercial resources were read-only.

Reference revision: `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, verified locally
with a clean checkout. Configuration, provenance and licensing remain in
[dependencies](dependencies.md). Investigation used the configured external
source/build, existing MMModern resource readers, and an external temporary
metadata probe; no alternative production archive adapter is proposed.

| Evidence | Provenance and decisive finding |
| --- | --- |
| ORIGINAL mainland | Initial `maze0023.dat`, 892 bytes, CRC32 `8f3e28ee`; `maze0023.mob`, 220 bytes, CRC32 `ce08a8c8`; `maze0023.evt`, 1440 bytes, CRC32 `e3128711`. Existing `xeenValidateRegionalManifest` binds these values. The 121-cell component includes Myra `(9,11)`, Phirna `(8,2)` and well `(7,7)`; the only mainland automatic-event cell is `(5,9)`. |
| ORIGINAL interaction identity | EVT records 21..35 are Myra; 125..135 are Phirna; 57..66 are the selected well; 56 is the inherited sign. MOB object identities are respectively 1, 13, 4 and 7, with resource IDs 9, 111, 102 and 54. Record order and original coordinates survive disabled overlays. |
| ORIGINAL recovery | Well record 60 gives mode 8/value 25; record 59 tests Action 78 against zero; record 63 gives mode 103/value 16. Text indices 17, 18 and 11 supply selection, success and refusal text. There is no condition-clear, SP restoration, Remove, time charge, payment or daily-use test in this chain. |
| ORIGINAL initial state | `maze.pty` has 812 bytes; world flag 16 is false (byte 725 bit 0). The original miscellaneous inventories are empty; Myra produces five `{10,37,1,0}` records only after the Root exchange. These are installation facts, not a fresh-state fallback on resume. |
| REFERENCE well | [`scripts.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/scripts.cpp), `ifProc` Action 78 returns 1 for current HP <= maximum, otherwise 0; `cmdWhoWill`, `cmdTakeOrGive`, `cmdPlayEventVoc`. [`party.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp), `giveTake` mode 8 adds HP directly and mode 103 sets a world flag. It does not call healing/clamping or clear Unconscious. |
| REFERENCE item | [`dialogs_items.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp), `ItemsDialog::doItemOptions`, action 2: source worst-condition eligibility, `isBad`, positive charge, decrement before target selection, clear/sort on exhaustion, then `moveMonsters`. |
| REFERENCE effect and target | [`spells.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/spells.cpp), `castItemSpell` maps ID 37 to `curePoison`; it calls `addHitPoints(0)` and clears Poison. [`dialogs_spells.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_spells.cpp), `SpellOnWho`, selects any active member or cancels. Cancellation occurs after the item debit. Its generic spell-cost refund is deliberately not imported into this item-only path. |
| REFERENCE side effects | [`character.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp), `addHitPoints(0)`: no numerical HP change; on a non-dead target with positive HP clears Unconscious. [`item.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp), `XeenItem::clear` and `InventoryItems::sort`, clear all four exhausted bytes and stable-compact the category. [`combat.cpp`](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp), `moveMonsters`, supplies a movement/ranged opportunity without a calendar or ctr24 increment. |
| IMPLEMENTED guard gap | `XeenEventPublication::execution/prepareGrant/prepareRemove` permits only the sign for regional contracts; expedition grant index 18 and map-20 Remove are separate. Quest-flag writes, quest consumption and `xeenDeliverRewards` currently have no regional expected-preimage adoption. Merely admitting Myra/Phirna addresses would fail or bypass these guards. |
| IMPLEMENTED reuse | `XeenEventInterpreter::runInstructions`, `XeenEventSystem`, `XeenEventFlow::journeyEventWork`, `XeenInventoryFlow.cpp`, `XeenJourneyConsequences.cpp`, `XeenRestoreGuard`, `XeenJourneyCapture`, `XeenSaveState` and the codec establish the owners and publication seams used below. `MyraIntegrationTest`, `PhirnaIntegrationTest` and `SaveResumeIntegrationTest` establish endpoint semantics, but their positioned checkpoints do not certify this route. |

### Baseline production route evidence

An existing `mmodern_consequence_cli_witness` run, with original resources,
`--journey-region --combat-seed 7`, M34 `attack` policy and production input,
completed the following recipes. `U` means Forward, `L/R` turn left/right.
Between route keys, finish production owed work; in combat target the lowest
original record among current contacts and Attack each ready member, letting
enemy/round/End work run normally; acknowledge monster receipts. This is the
existing witness policy, not direct combat/helper calls or a freeze of idle work.

| Segment | Recipe and endpoints |
| --- | --- |
| Outbound | `LUUURUULURUULUUUUUUURRUURUUUL`: `(9,11,W)` -> `(9,8)` -> `(7,8)` -> `(7,7)` -> `(5,7)` -> `(5,0)` -> `(5,2)` -> `(8,2,N)` |
| Return | `UULUUURUUURUULURUULUUUL`: `(8,2,N)` -> `(8,4)` -> `(5,4)` -> `(5,7)` -> `(7,7)` -> `(7,8)` -> `(9,8)` -> `(9,11,W)` |
| Continued recovery visit | `LUUURUULU`: Myra -> `(7,7,S)`, by the outbound northern approach |

**IMPLEMENTED observation:** outbound plus return reached Myra at minute **848**,
world RNG raw count **281**, carried gold **810**, with living Poison, Sleep
history and three combat episodes. The continued recovery visit reached `(7,7,S)`
at minute **910**, RNG count **315**, gold **830**, four episodes, and no pending
monster gold/items. Owners 1 and 6 were Dead; owner 14 remained Unconscious/Asleep;
owner 18 remained alive with Poison=1 and HP=13; owner 11 was awake at HP=2;
owner 0 was awake at HP=28. The latter three supply a real antidote target, well
recipient and potential potion user. All 19 actors remained owned/scheduled;
survivors were not reset. No equipment rearrangement, injected inventory,
relocation, quest effect or recovery was used in these runs.

The southern `(5,0)` waypoint engages the original Snake and supplies the actual
Poison needed to test the later antidote. It is an acceptance path within admitted
mainland geometry, not an additional interaction or required quest prerequisite.
The simpler path via `(5,4)` also completed a seed-7 baseline round trip at minute
807, but did not establish living Poison and is not the main antidote witness.

**Limit of evidence:** these runs establish baseline travel/combat feasibility
and available time, not implemented M35 events, item use or recovery. M35 must
replay the retained recipe with the actual interactions and checkpoint assertions.
Quest work below has no time/RNG/scheduling cost, so it must preserve the baseline
route observations up to recovery, except for the explicit quest/item/overlay
changes. Failure of that comparison is a regression to resolve, not grounds to
inject state or silently change the route. The well and antidote effects then
intentionally change subsequent gameplay. Retain new post-effect observations
when executing acceptance.

## Regional content and exact interaction admission

Introduce **Journey content contract 6**. Fresh `--journey-region` selects it;
explicit legacy setup/load retains its saved contract. Contract 6 inherits all
contract-5 terrain, original actor profiles/closures, complete scheduling,
Attack/Block/Shoot/Run, conditions, treasure, preparation and time rules. It adds
only the events and item use below. Use explicit supported-policy predicates,
including `consequences()` and `disengagement()`, instead of leaving `==5` gates
that accidentally remove Run from 6 or broadening arbitrary future contracts.

Party reachability remains the resource-derived component rooted at `(9,11)`.
Actor influence remains the full original 19-actor domain, including actors
outside the party component. Neither grants event authority. Every other
mainland script and every transition continues to refuse before execution.
In particular `(8,10)` is opcode 0x11/location 12 (reference `PYRAMID`), not
a recovery service; `(5,13)` is the mines; `(0,1)` and `(12,12)` are stat/AC
boost interactions, not necessary recovery. They remain excluded.

All addresses in the following table are Clouds map 23. Original EVT identities
are zero-based; offsets and line numbers are decimal. Direction bytes are
North=0, East=1, South=2, West=3, All=4.

| Entry | Original records / offsets | Direction, logical closure and physical selection |
| --- | --- | --- |
| Myra `(9,11)` | 21..35, offsets 182..315; lines 0..14 | West only. Original MOB 1, resource 9. Physical and logical address stay here; allow line 15 only as natural end. Exact operand table is in [M21 original exchange](milestone-21-plan.md#myra-script-and-exchange). |
| Phirna `(8,2)` | 125..135, offsets 1056..1132; lines 0..10 | All directions, not just the north-facing M21 checkpoint. Original MOB 13, resource 111. Physical and logical address stay here; line 11 is the adjacent acknowledged natural end. |
| Well `(7,7)` | 57..66, offsets 502..575; lines 0..9 | All directions. Original MOB 4, resource 102. Physical and logical address stay here; line 10 is the adjacent acknowledged natural end. |
| Sign `(5,9)` | 56, offset 495, line 0, opcode 04, operand `10` hex | North only, MOB 7; unchanged M32 semantics and absent line 1. |

There is **no required CallEvent closure** in these original chains. Retain
existing generic Call/Return for its existing domains, but contract-6 admission
requires an empty call stack and rejects transfer to another logical address,
map, line outside the descriptor or a called script. Decodability is not admission.

Lookup must use the existing first original matching `(x,y,line,direction-or-All)`
record, then apply its disabled overlay as effective `None`. Do not delete,
reorder or skip a disabled record to find another match. Validate immutable
topology and original opcode/operands even for effective None. None retains the
existing sequential behavior; collected Phirna can traverse disabled records
to natural completion without text, grant or Remove replay. Independently valid
partial overlays remain representable; they are not repaired into a quest state.

Manual Space uses the actual camera/facing without an automatic-bit requirement.
Automatic dispatch retains the original bit: among admitted mainland cells only
the sign can run automatically. Fresh Myra entry is a presented mutable scene;
Space starts the request. Walking/turning back to Myra does not open her dialogue.
M34 retirement first settles mandatory attachment/treasure and the existing
automatic-dispatch opportunity. The fixed Run destination `(10,12)` has no event;
it must not summon Myra or replay an abandoned interaction. Restore never performs
startup event dispatch, including at Myra, Phirna, the well or sign.

### Exact Phirna records

The Phirna and well tables use hexadecimal opcode and parameter bytes; each
length field is `5 + parameter count`. The complete regional manifest remains
authoritative.

| Record | Offset | Line | Opcode | Parameters |
| --- | --- | --- | --- | --- |
| 125 | 1056 | 0 | 01 | `1e` |
| 126 | 1063 | 1 | 09 | `2c 00 03` |
| 127 | 1072 | 2 | 12 | none |
| 128 | 1078 | 3 | 09 | `15 63 09` |
| 129 | 1087 | 4 | 01 | `1f` |
| 130 | 1094 | 5 | 09 | `2c 01 06` |
| 131 | 1103 | 6 | 0c | `00 00 15 63` |
| 132 | 1113 | 7 | 0e | none |
| 133 | 1119 | 8 | 12 | none |
| 134 | 1125 | 9 | 29 | `20` |
| 135 | 1132 | 10 | 09 | `2c 01 0b` |

## Quest lifecycle and reward publication

Keep M21's existing interpreter branches and response boundaries:

- With no Root, Myra follows lines 0,1,4,5,6. Opening or paging the original NPC
  dialogue changes nothing; final NPC acknowledgment sets Q2 at line 5. No item
  or reward is produced. The unsigned SP comparison retains its existing meaning;
  do not add combat eligibility, an SP cost or a different request branch.
- Phirna's initial centered text leads to Yes/No at line 1. No exits without a
  grant/removal. Yes checks Root possession at line 3. Already owning a Root
  leads to text 32 and acknowledgment, without grant/removal. Otherwise success
  text 31 is acknowledged at line 5 before line 6 grants one Root (item 99,
  counter index 17). Line 7 independently publishes Remove: selected original
  object 13 if present and all event identities 125..135 at the physical cell.
  Remove never touches actors. The north-facing successful original trace has
  18 dispatched instructions: Remove resets the logical line to 0, then all eleven
  disabled records execute as None. Preserve this reset, interpreter counting and
  effective lookup, not a shortened shortcut.
- Root possession, **not Q2 or a new completion flag**, selects Myra's return
  line 7. Final NPC acknowledgment reaches line 8 (consume exactly one Root),
  line 9 (clear Q2), then five independent GiveEnchanted instructions at lines
  10..14 and natural completion. Original suffix bytes `00 01` remain diagnostic;
  each item is exactly `{material=10,id=37,state=1,frame=0}`. No HP, XP, gold,
  time or RNG bonus is invented.
- M21 warning -> synchronous delivery -> paginated receipt -> final acknowledgment
  remains mandatory, including complete recipient/capacity loss. Myra has no
  preferred recipient; use active order, current `canAct()` and miscellaneous
  tail capacity for each item. No displacement, eligibility reset or Root refund.
  M21 overflow/loss accounting and terminal/error discard policy remain intact.
- Final receipt acknowledgment returns through Presentation to ordinary Journey.
  A later Root permits another exchange. With no Root, a new explicit request
  sets Q2 again even though Phirna remains removed. This is legitimate gameplay,
  not reward replay. Artificial multiple-Root tests supplement the one original
  plant; they do not claim a second original acquisition site.

Root count, Q2, object removal, each disabled event, HP/items and the new well
flag are independent durable facts. A grant-only failure does not imply removal;
Root consumption does not imply a completed receipt. Preserve every published
prefix. Exactly-once applies to an accepted instruction/publication and its
response generation, not globally to the NPC or quest.

### Required extension of existing Event authority

Keep `XeenEventFlow` as the noncopyable continuation owner and `XeenEventPublication`
as its stack-bound capability. Add an immutable admitted-interaction descriptor
selected from the actual first matching record and contract. It binds physical
camera, lookup direction, selected original object (or its valid disabled absence),
complete original EVT, allowed logical control-flow/operand sites, pending
response kind, and reward phase. The sign and expedition retain their distinct
descriptors. It is not a general coordinate allowlist or transferable command.

Extend the existing prepare/expected-adoption pairs with these narrow operations:

| Publication | Exact permitted producer and expected delta |
| --- | --- |
| Quest flag | Myra records 26/30: set/clear Q2 only, including idempotent writes; update retained `questFlags` before any callback. |
| Quest counter | Phirna record 131 grants index 17 with checked u32 overflow; Myra record 29 consumes index 17 with checked underflow. Keep expedition index 18 separate. |
| Remove | Phirna record 132: preallocate both world overlay sets, retain all preexisting entries, add the exact physical identities above, then swap both sets and expected preimages nonthrowingly. No whole-world snapshot replacement. |
| Reward enqueue | Myra records 31..35 each authorize exactly one pending `{10,37,1,0}` production at that instruction. Check the live owned continuation's queue/phase before and after; no record can be replayed by a copied report. |
| Reward delivery | Extend the existing `xeenDeliverRewards` seam with a detached bounded delivery candidate, exact membership/recipient/category preimages, resulting miscellaneous arrays, fixed receipt and cleared queue. Publish all touched arrays, queue consumption and receipt adoption once before formatting/reporting. Retain M21's algorithm, including per-item capacity changes and losses; ordinary endpoints keep the same semantics. |
| Well HP and flag | Exact sites 60 and 63, with the separate effects and ordering below. |

The capability must validate on entry/resume, before a response is consumed,
before finalization (which can precede the interpreter loop), at each instruction,
around providers/reporters and immediately before each write. An expected-state
adoption applies only a prepared delta to the already retained preimage; it must
not bless whatever a callback left in live owners. Pending state, branch/site,
queue, receipt, selection and instruction count must belong to the live Flow
generation across suspensions. Stack-local grant/remove booleans alone cannot
establish cross-response exactly-once execution.

`XeenEventSystem` completion continues to require unchanged working versus
committed camera/game flags for these chains. The new world flag is party-owned
and immediate, not `XeenGameFlags` mode 20 and not a transactional working flag.
Do not remove the current completion checks to accommodate it.

## Selected local recovery: the `(7,7)` well

The selected well is on both route directions. It is the only additional
recovery interaction admitted. Its original chain is:

| Record | Offset | Line | Opcode | Parameters / meaning |
| --- | --- | --- | --- | --- |
| 57 | 502 | 0 | 20 | `02 11`: WhoWill, verb 2, text index 17 |
| 58 | 510 | 1 | 28 | `02`: PlayEventVoc index 2 |
| 59 | 517 | 2 | 09 | `4e 00 08`: if HP is already above maximum, branch to 8 |
| 60 | 526 | 3 | 0c | `00 00 08 19`: selected character HP += 25 |
| 61 | 536 | 4 | 01 | `12`: original success text 18 |
| 62 | 543 | 5 | 09 | `2c 01 06`: acknowledgment before line 6 |
| 63 | 552 | 6 | 0c | `00 00 67 10`: set world flag 16 |
| 64 | 562 | 7 | 12 | Exit |
| 65 | 568 | 8 | 01 | `0b`: original refusal text 11 |
| 66 | 575 | 9 | 09 | `2c 01 0a`: acknowledgment, then natural end |

WhoWill reuses F1-F6 and the existing membership/identity/current `canAct()`
validation; Escape before selection cancels with no mutation. An ineligible
selection keeps the prompt with refusal feedback. It does not select a substitute.
Only the selected authoritative owner receives HP. In contract 6 the fixed
membership is unchanged; helper alias controls still resolve a roster owner once.

Add the bounded interpreter comparison Action 78 and neutral/give-only mode 8.
Compute maximum HP with `XeenCharacterRules::maxHp` and the live year, conditions,
level and equipment. Action 78 evaluates to 1 when current HP <= maximum, 0
otherwise, exactly as the reference. The well adds **25**, without clamping;
at exactly maximum HP it still adds 25. Repetition while HP <= maximum is legal;
once above maximum the refusal branch adds nothing and does not set the flag.
Later damage can make another use effective. There is no daily quota or
well-used lockout. Check int16 HP addition before publication; overflow is an
execution error without that instruction's effect, never saturation or wrapping.

Do not clear Poison, Disease, Sleep, Unconscious or Dead, restore SP, change
maximum HP, repair equipment or touch supplements. The selection restriction
means this is not rescue for an unconscious/dead member. HP gain publishes at
line 3 **before** success acknowledgment. World flag 16 publishes separately at
line 6 **after** acknowledgment. A later text/flag failure preserves the HP gain;
an interrupted chain may therefore save HP gain with the flag still false after
trusted cleanup. The flag is an original persistent marker, not eligibility.

Decode opcode 0x28 with exactly one operand and admit index 2 only through this
descriptor. **INFERENCE, presentation adaptation:** consume/count this original
voice instruction as a cosmetic cue with no audio playback in M35. Use the
existing original text/WhoWill/success presentation and visible HP update; do
not add an audio engine, copy voice data or skip the gameplay instructions around
it. No gameplay RNG, time, ctr24, actor pulse or condition tick belongs to well
selection, effect, acknowledgment, refusal or repetition. Ordinary prior movement
and any later navigation retain their full costs.

### Minimal original world-flag ownership

Introduce `XeenRegionalRecoveryState { bool worldFlag16; }`, held as an optional
value by `XeenPartyState`; it is required only for contract 6 and absent in every
legacy/ordinary/completed domain. It is a bounded projection of original world
state, not a second quest owner, history ledger or all-purpose service state.
Only mode 103/value 16 at the well may set it. Other world flags, clear operations
and conditional Action 103 remain outside production admission.

Extend the existing PTY parser with a checked `parseRegionalRecovery` operation:
the world-flag block begins at `659 + 32 + 32 = 723`, has 16 serialized bytes,
and uses low-bit-first packing; flag 16 is byte 725 bit 0. Require the complete
block through byte 738 before reading. Pass the typed initial value through
`XeenJourneySetup` and guarded fresh preparation, not a hard-coded false. Do not
reload PTY during interaction or restore. Party copy/move/swap, detached candidates,
equality, `XeenRestoreGuard`, capture and restore must include presence and value.
Other 127 serialized world bits and reference-only flag 128 are not newly modeled
or assigned meanings; no M35 operation reads or writes them.

## Narrow antidote item use

### Admission and controls

Use the existing inventory source/active-owner/category/physical-slot selection
and epoch certificates. Add typed `UseItemAction` (native **U**, contextual to the
open inventory), a use confirmation, and a target-selection mode to that flow.
The pure bounded resolver may be named `XeenAntidoteUse`; it operates on typed
source/target/record values and returns prepared deltas/observations, never owns
a party, spell book, random stream or presentation loop. Do not call a ScummVM
engine/spell dispatcher or create a general item-spell framework.

Only presented **contract-6 exploration** with current owner/frame authority and
no combat, owed work, projectiles, attachment, event, monster receipt or competing
lease may begin inventory use. Its own Inventory/Certificate leases remain held
until the explicit transfer below. Existing inventory selection is modal. Combat-time use,
diagnostics, completed checkpoints and contracts 1..5 remain unsupported. A
visible distant actor does not by itself forbid use; its genuine subsequent
movement/ranged opportunity must be honored.

Admit only an occupied **Miscellaneous** record with material **10**, ID **37**.
Low six state bits are charges 0..63; cursed/broken bits are 0x40/0x80. Frame is
opaque, any byte, and confers neither equipment nor use eligibility. Accept
1..63 charges with neither bad bit set. Zero charges, empty slot, cursed/broken,
other category/material/ID, stale source or an ineligible source refuse without
debit, effect, action, time, RNG or actor work. Do not restrict storage/inspection
of other existing opaque records. The sole new genuine producer remains Myra's
one-charge material-10 rewards. Material 11 and other effects remain excluded.

Source eligibility is the existing `canAct()` worst-condition predicate, not a
class, SP, HP-sign, target or inventory-recipient check. Resolve the source owner
from the selected active slot and retain that identity. Poison/Disease alone
permit using an item. No transfer to the target is required. Target may be a
different active member or the same owner.

U first presents source, resource/catalog-derived item name, charge count and
the explicit warning that confirming spends a charge even if target selection
is cancelled. Escape from this **pre-use confirmation** is free. Enter confirms
once; only after its debit frame is presented may a fresh F1-F6 select a target.
Escape in that target selector is an accepted cancellation **after debit**.
This distinction preserves the reference's spent-on-cancel behavior without
hiding it from the player. Held/batched Enter, U or F-keys cannot cross phases.

### Effects, bytes and ordered publication

The item uses no SP, gems or gold. Do not emulate `SpellOnWho`'s generic
`addSpellCost` cancellation refund: no spell cost was charged. This is an
explicit bounded adaptation, not a claim that the reference has a separate
correct item-refund path.

1. **Debit:** after source/record/owner/lease preflight, decrement the low-six-bit
   charge once, preserving material, ID, frame and high state bits. Publish that
   record and adopt a live use continuation in the noncopyable Journey coordinator before
   callbacks/target presentation. Debit also commits the obligation to settle
   exhaustion and service one opportunity, even on later cancellation/abandonment.
   Keep a one-charge exhausted record temporarily at its physical slot with zero
   charge until settlement; this phase is never saveable.
2. **Target effect:** accept any current active slot, including an incapacitated
   or Dead member. Recheck membership and resolve the retained target owner.
   Clear only Poison (condition index 3). Preserve the reference `addHitPoints(0)`
   side effect: if no Dead/Stoned/Eradicated condition and HP>0, clear Unconscious;
   never change numeric HP, SP or any other condition. The normal Journey
   invariant already excludes living positive-HP Unconscious, so that side effect
   is normally inert; it does not justify widening the saved condition domain.
   Dead targets may lose Poison but remain Dead with identical HP; this is not
   resurrection. A healthy target is an accepted no-effect use and still spends
   the charge. Target cancellation publishes no target delta and retains the debit.
3. **Inventory settlement:** if charges reached zero, clear all four bytes of
   the source record, stable-compact **only its miscellaneous category** with
   `xeenCompactItems`, and clear empty-tail metadata exactly as that helper does.
   Preserve every occupied record's bytes/order; no frame reset on moved surviving
   items. With remaining charges, preserve every other slot including holes and
   ID-zero metadata; do not compact. Same source/target must merge these deltas
   into that owner, never overwrite the debit/effect from an old character copy.
   Publish a fixed result (source, target/cancelled, Poison before/after, spent
   charge, exhausted status) and make the one already owed item opportunity ready.
4. **Owed world opportunity:** close inventory and target selection, retain the
   observation and lease, and service one existing regional movement/ranged/
   classification opportunity on all 19 live actors and the full active party.
   Reuse `XeenRegionalOpportunityCandidate`/existing guarded Journey publication;
   no navigation, Shoot charge, Wait charge or counterfeit combat End. Publish
   its complete actor/character/RNG result once. Any resulting contact attaches
   combat through the ordinary path; otherwise settle ready monster treasure and
   presentation before Quiet. Keep item feedback visible through immediate contact
   using the existing retained-observation handoff pattern.

Accepted selection/effect/debit/settlement has **zero minute, ctr24 and direct
RNG cost**. The one subsequent actor opportunity can consume normal ranged damage
RNG and wound/kill characters; it has zero additional minute/ctr24 cost. Do not
create a three-pulse navigation delay or call `changeTime(0)`. Its raw-draw
preparation remains bounded at 64 attempts per service call, with the existing
continuation and atomic opportunity semantics. Subsequent combat has its normal
round/End costs. Cancellation after debit and healthy-target use owe the same
opportunity. Pre-use refusal/cancellation owes none.

A failure before debit changes no gameplay. A failure after debit cannot refund,
restore a potion, add SP/gems or replay U. Bounded target-frame retry only redraws
the retained continuation. Explicit abandonment after debit performs no target
effect, but must settle exhaustion and the already owed opportunity before any
new Quiet boundary; if integrity/resource failure prevents that, latch Failed
and forbid continued gameplay/capture. A shutdown saves nothing. A failure after
effect preserves the effect as well as consumption; a failure after opportunity
cannot repeat movement, damage or RNG. These ordered units are not rolled back
together. Consume any response generation before entering resource/callback work.

Use the existing item catalog (`XeenItemCatalog`, pinned embedded English names
and optional `mae.xen`) for names/fallbacks, and live roster names for source and
target. Feedback distinguishes refusal, cancelled-after-spend, no Poison to clear,
Poison cleared and item exhausted. It reports actual results without claiming
HP restoration or immunity. No permanent poison resistance is granted; later
enemy attacks may reapply Poison. One new U hint and target/confirmation feedback
must fit the existing indexed inventory/presenter layout with bounded wrapping.

The concrete coordination seam belongs on `XeenEncounterFlow`: guarded methods
to begin use from the inventory source certificate, accept one target/cancellation
for the retained use generation, and service settlement/owed opportunity from idle.
Inputs carry the current Journey ticket, inventory epoch, active source index,
resolved roster owner, category/physical slot and all four selected bytes; target
input additionally carries its displayed generation and active index. No method
accepts a caller-supplied final character, charge delta, arbitrary spell ID or RNG.
`XeenEventFlow` owns UI selection/confirmation and routes typed/native actions;
the EncounterFlow continuation owns the debit/effect/settlement phase and owed
world work. Add a bounded ItemUse activity/lease to existing Journey coordination,
using Inventory/Certificate leases for pre-use selection and transferring to
ItemUse before releasing them. Save/capture and input predicates must explicitly
recognize this activity; an open inventory is never used as the only safety gate.

## Consequence continuity, resources and authority

The existing world/session still owns actors, accounting, overlays and gameplay
RNG; party/roster owns all character values, supplements, XP, purse/monster
treasure, quests and the new flag projection. Application retains camera/game
flags. Event/Inventory/Journey coordinators borrow these owners. Their results
are observations of published state, not reusable mutation commands.

Myra delivery is **transient Event reward production**, not an Orc drop. It does
not set monster pendingMask, pendingGold, item source tags or readiness. The well
and antidote produce no treasure. Consequently M34's bounded equivalence
`monster treasure ready iff pendingMask != 0` remains valid: Orcs remain the only
monster-item producer, every such production adds its gold obligation, and
DirectRun retains dormant items exactly as before. No readiness bit or merged
reward owner is necessary. A Myra receipt may deliver while unrelated monster
items remain dormant or ready-but-threat-blocked; it must not awaken, discard,
credit or hide them. Due collectable monster receipts settle before another
interaction begins. After item movement, re-evaluate the existing monster
delivery gate without weakening provenance.

Recovery changes only its specified owner/effect. Everything else remains exact:
HP/SP and injuries; Poison/Sleep/Disease and M34 casualties; all four item arrays,
equipment and breakage; full membership/supplements; XP, gold and gems; wounded
survivors and activation; lethal accounting; pending/dormant treasure; day/minute,
ctr24 and world RNG. Well HP is not general condition recovery. Sleep only changes
through already admitted damage/condition behavior; Disease, incapacitation,
Dead casualties, depleted SP and broken gear have no new recovery path. They may
remain unrecoverable for the rest of this bounded slice.

Reuse the existing `XeenRestoreGuard` owner lifetimes/incarnations, complete
mutable preimages, resource union, monotonic integrity failure and boundary
leases. Include the new optional flag, item-use continuation and Event queue/
receipt phase in their respective authority checks. Owner replacement, byte-equal
copy/move/ABA, stale completion, same-address coordinator replacement or a copied
frame/result cannot grant authority. Stale work cannot fail, overwrite or release
newer work. Preallocate storage/results, check overflow and validate the actual
retained ticket after every provider/reporter, then use nonthrowing publication.
Do not refresh a guard from callback-mutated owners to repair a failed check.

Extend retained Event resource authority to map-23 text values as well as EVT:
retain map identity, resource presence/name, complete parsed strings (including
valid empties), and compare exact values on reload. Add this bounded text preimage
to the existing retained guard/resource union and Event provider wrapper; preserve
it through combat, inventory, cache discard, event retirement and save preflight.
For contract 6, load `aaze0023.txt` through `XeenEventTextLoader` before exposing
fresh/restore gameplay, as a detached resource value in setup/restore resources.
Validate the required indices for the admitted branches and retain that initial
table before callbacks can replace it. This is resource preflight, not script
execution; legacy domains gain no new startup dependency. EventSystem cache hits
and reloads must compare against this retained table, not establish a new authority.
Bind NPC FAC/presentation resources through the existing composition checks, not
an independent resource cache. Required well/quest text absence, malformed data,
wrong identity or changed retained content is an error, never an empty successful
event. Keep optional item-material fallback semantics separate. Do not pin copied
commercial text into source, add a runtime `mm.dat` reader or relax the existing
DAT/MOB/EVT/statistics manifest and archive compatibility policy.

An ordinary unavailable resource before admission publishes nothing. A resource
change after retention permanently invalidates the graph even if the provider
later returns matching bytes. Trusted recoverable manual text/presentation failure
retains earlier effects and follows the inherited cleanup-to-Presentation path;
automatic failures and integrity failures remain fatal/unsaveable. A presentation
retry redraws an already owned observation/continuation only. It never restarts
an instruction, delivery, well effect, debit or world opportunity.

Capture/incompatible input remains closed continuously through Event, WhoWill,
Yes/No, NPC paging, warning, synchronous delivery, receipts, item confirmation/
selection/settlement, owed work and presentation handoff. Transfer lease ownership
before releasing the preceding lease; do not briefly assign Quiet because a
pointer was cleared. For each new semantic phase advance the existing input
generation. Cosmetic animation may retain it, but only successful presentation
of the exact current owner's concrete `IndexedFrame` binding opens its boundary.
F9 refuses before capture, resource/preflight or I/O and never queues a save.
Held/repeat/timestamp/batch fences cover U, confirmation, F1-F6, Escape, Space,
combat controls and F9. A queued old action cannot select a new target, dismiss
the following receipt or attack on immediate reattachment.

## Time and persistence

Keep the M33/M34 supported domain: year u16, day 0..99, minutes 300..1259,
ctr24 0..23, WorldOfXeenClouds/Adventurer, rested/newDay false and the inherited
zero unsupported effects/light/resistances. Fresh preparation remains day 8,
year 610, minute 480. Ordinary travel, Wait, Shoot, rounds, End, Run, retirement
and condition ticks preserve their existing charges/order. No quest/recovery
time reset, rest, dusk/dawn/daily/year handling is added. The demonstrated loop
and later well visit leave room before 960 and 1260; acceptance includes further
mutation across the supported 960 tick and refusal before 1260.

Quest instructions, well actions, receipt/target input and saving add no gameplay
draws. The antidote's owed opportunity and later condition/combat work use the
same world RNG continuation. In particular clearing Poison changes which draws
the inherited 480-minute condition processing consumes; do not force the old
draw count after treatment. Portrait, scene animation and item feedback remain
cosmetic and do not borrow gameplay RNG.

### Version and wire decision

Keep **save envelope v4**, add exactly **Journey schema 6 / content contract 6**.
Do not encode new sessions as 5/5. There is one newly durable value (world flag
16), and a new continuation contract for regional quest/recovery/item use. An
older reader cannot preserve these meanings; unchanged old field layouts alone
would not justify reusing 5/5. The envelope already delimits and dispatches Journey
schemas and needs no new version.

Retain the entire 5/5 suffix layout and M34 independent money/item provenance
validation, change the two discriminators to 6/6, and append **one canonical u8**
for `worldFlag16` **after** the variable weapon/armor treasure records. Values
are 0 or 1 only. The resulting suffix length is exactly
`1821 + 5*(weaponCount + armorCount)`; the new byte is at suffix offset
`1820 + 5*(weaponCount + armorCount)`. Existing offsets, counts <=10/category,
combined <=12, source identities and canonical unused runtime slots are unchanged.
Validate exact remaining length before allocation. Missing/extra/truncated flag,
nonboolean flag, unknown/crossed schema-content pair or inconsistent optional
presence refuses. Add the optional typed projection to the Journey snapshot;
require it only in 6/6 and forbid it in every other domain.

| Existing domain | Preserved meaning |
| --- | --- |
| Ordinary v1/v2 | Existing v1 missing-item resolution and ordinary v2; no recovery projection or conversion to Journey. |
| Completed v3 | Existing Diagnostic27 completion, read-only inventory and R revisit; no M35 actions. |
| v4 1/1, 2/2, 3/3 | Exact legacy suffixes, rules, resource requirements, support stops and event domains; no implicit upgrade. |
| v4 4/4 | Coupled source/gold treasure semantics, no Run/dormancy, no M35 actions. |
| v4 5/5 | Exact M34 Run/dormancy and sign-only regional event admission; no inferred world flag or M35 actions. |
| v4 6/6 | M34 consequences plus this document's bounded events, well flag and item continuation. |

Legacy F9 continues to encode the same supported legacy pair. No load-time or
save-time upgrade, CHR/PTY defaulting of new fields, format probing fallback,
quest normalization or optional content override is allowed. Keep archive
fingerprints, CRC32, 4 MiB bound, exact EOF and protected atomic file replacement.

### Quiet saves and exact restore

Meaningful save points include fresh entry before Space; after request; quiet
travel with wounds/treasure; at Phirna before interaction; after collection;
return with the Root; after final Myra receipt; after well acknowledgment; and
after item settlement/owed work/receipt/presentation. Save at the actual camera,
with the actual independent quest and overlay facts. Pending dialogue, reward
queue, use continuation and owed item opportunity are not serialized.

Require `XeenJourneyCapture`'s existing authoritative, presented, pending-zero,
no-contact/no-work/no-lease boundary, extended for item-use work. Ready collectable
monster treasure still prevents Quiet; dormant treasure does not. Do not narrow
valid quest/overlay states to a story sequence. HP above maximum from the well
is valid int16 HP, never clamped on capture/restore; existing HP/Unconscious/Dead
consistency and at least one nonterminal member remain mandatory.

Restore through the existing unpublished-candidate pipeline: decode/validate;
load matching immutable resources; derive party mainland and actor closures;
apply all saved characters, supplements, independent quests/overlays/new flag,
actor live fields/accounting, treasure, time and RNG; validate complete current
state; retain owner/resource preimages and preflight the first concrete frame;
publish once into fresh owners and bind fresh transient coordination. Restore
does not request Myra, grant/consume Root, Remove, distribute rewards, heal, cure,
compact items, debit a charge, activate/move actors, relocate, tick, draw or replay
the last event. Subsequent fresh input uses saved contract 6. Invalid files fail
startup without a fresh-session fallback and leave destination owners unpublished.

## Dependency-ordered implementation checkpoints

These are internal dependencies of one milestone, not separately authorized
sub-milestones. No implementation is authorized by this candidate.

1. **Admission and durable projection:** add explicit contract-6 policy, the
   bounded PTY flag reader/party/snapshot/preimage value and exact 6/6 codec/restore
   rules. Preserve all legacy predicates and fixture pairs; add exact interaction
   descriptors against the original manifest. Do not expose incomplete fresh-6
   production entry as accepted gameplay.
2. **Guarded Event integration:** extend prepare/adopt operations and detached
   reward publication; validate continuation/finalizer/text authority; connect
   Myra and Phirna through existing EventFlow. Add selected-owner Action 78,
   give-only HP mode 8, world flag 16 and bounded cosmetic opcode 28; exercise
   well partial-failure prefixes. Complete Event-to-Presentation/capture handoff.
3. **Inventory antidote and owed work:** add the narrow pure candidate/result,
   source certificate/confirmation/target modes and ordered debit/effect/settlement;
   reuse regional opportunity publication and attachment/treasure/presentation
   handoff. Expose U only where supported; finish failure and input fences.
4. **Connected production acceptance:** extend existing original-resource/process
   witness infrastructure with typed event/inventory inputs and route checkpoints;
   run the recipe and all branch comparisons below, full build/CTest and required
   legacy/original controls. Finish independent review and maintainer physical
   acceptance before any closure record. Closure documentation/Git require the
   separate authorization and workflow in [AGENTS.md](../AGENTS.md).

## Acceptance contract

### Automated deterministic correctness and regression

Use artificial resources only for labeled rule, rare refusal, corruption and fault
controls. Extend the relevant Event/quest/reward, inventory/SDL, Journey/resource/
consequence, format/save/capture/restore suites. Required cases include:

- Exact descriptors/first-match/All-facing lookup, wrong-facing Myra, disabled
  first records and partial overlays, no called-script escape, unknown nearby
  interaction and automatic-bit refusal, initial/return/retirement/restore dispatch.
- No-Root Q2 false/true request; one/multiple Roots; repeated exchanges; already
  owned Phirna, No and collected revisit; quest underflow/overflow; all recipient,
  tail capacity, alias, invalid/overflow, warning and receipt paths inherited from
  M21. Verify Root loss is never refunded after later reward loss/failure.
- Well HP below/equal/above max, repeated use, live equipment/condition-derived
  maximum, eligible/ineligible/cancelled selection, int16 overflow, and failure
  before/after HP publication and before/after world-flag acknowledgment. Check
  unchanged SP/conditions/items/XP/purse/time/RNG and independent HP/flag saves.
- Antidote material/ID/category/frame/state domain; bad/empty/zero-charge source,
  source/target differences, same owner, aliases in helper tests, ineligible user,
  all target condition cases, healthy-target spend, pre-confirmation cancellation
  versus post-debit cancellation, 1/2/63 charges, exact compaction/holes/ID-zero
  metadata. Verify the reference's zero-HP numerical change and no SP/gem refund.
- Inject failure at each ordered publication boundary, including target rendering,
  exhaustion, ranged preparation/draws, immediate contact, receipt and concrete
  upload. Compare the entire published prefix, not merely counters. A lost/stale
  observer cannot resurrect spent charge, clear another target or release work.
- Callback reentrancy, byte-equal owner replacement/ABA, stale copied continuations,
  cached then changed EVT/text/MOB/DAT/statistics, changed resources restored to
  equal bytes, held/batched input, obsolete frame upload and retries. F9 must do
  no capture/providers/I/O in each blocked phase, including debit-to-selector and
  settlement-to-world-work boundaries.
- Complete 6/6 wire fixtures, exact size/canonical bytes, optional presence,
  malformed/new-schema rejection by baseline reader, crossed pairs and unchanged
  legacy 1/1..5/5/ordinary/completed acceptance. No legacy session acquires U or
  new regional events. Artificial independent Root/flag/overlay states round-trip.
- M34 dormant/ready treasure and casualties across Event rewards/well/item use:
  Myra potions must not reactivate dormant treasure or credit forfeited gold.
  Item-opportunity contact must use a fresh full combat participant mask and exact
  surviving actor state; damage and Poison reapplication are allowed real effects.

Run focused tests during implementation, then the complete build and full CTest
suite at milestone closure. Preserve the existing M21 endpoint and M31 collection
controls; they remain useful regressions but do not replace connected acceptance.

### Genuine original-resource and fresh-process witnesses

Extend `tests/RunConsequenceWitnesses.py` with an opt-in M35 family, retaining its
process orchestration, state/byte comparison, bounded policies and transcript
style. Reuse `XeenConsequenceCliWitness`, `XeenRestoreReplayProbe`, save test
comparators and existing Application/SDL input hooks. Add typed recipe operations
for Space, Yes/No, fresh acknowledgment, inventory selection/U/confirmation/target
and checkpoints; observations may guide a bounded driver but never mutate owners.
Store retained recipes, seeds, semantic checkpoints, intermediate publications,
raw RNG counts/states and process outcomes outside tracked source. Scripts must
use new save paths outside the installation and avoid overwriting unrelated files.

The main retained **seed-7** recipe is mandatory:

1. Fresh contract 6 at Myra: Space and complete original request; assert Q2=true
   and no other consequence. Follow the outbound recipe above. At `(8,2,N)` use
   Space, Yes and the original acknowledgment; verify Root=1 and exact Remove.
2. Follow the return recipe. The baseline minute/RNG/actor/condition/purse trace
   remains unchanged by these zero-cost events. At Myra explicitly interact,
   acknowledge return and receipt; verify one Root consumed, Q2=false and five
   real potions delivered according to the *current* eligible owners, with no
   assumed original-party eligibility. Save with a fresh F9, fully exit, restart
   through production `--load-game`, and inspect before another interaction.
3. Walk the continued well-visit recipe on those restored owners. Before healing,
   compare the baseline minute-910/RNG-315 facts and complete expected quest/item
   deltas. Select active slot F4 (owner 11) at the well: HP 2 -> 27 on the retained
   baseline path, with the exact independent world-flag acknowledgment boundary.
   Repeated use/refusal must follow the live maximum, never a fixture assumption.
4. Through I and physical miscellaneous selection, use an actually delivered
   potion from eligible owner 0 on F2 (owner 18). Verify Poison 1 -> 0, one potion
   exhausted/compacted, preserved HP/SP/other conditions and exactly one owed
   actor opportunity. Baseline inventory capacity makes owner 0 the expected
   recipient, but acceptance must prove this through the real receipt and array
   comparison. No injected potion/Root is allowed if that expectation fails.
5. Complete the resulting production handoff; navigate again, reach a meaningful
   quiet save, restart in another process and mutate again. Include a bounded
   Wait continuation across minute 960 with full condition/RNG comparison after
   the cure. Never continue a search by resetting time or bypassing 1260.

Fork uninterrupted versus save/fresh-process branches after request, before and
after Phirna, during return with the Root, after final Myra receipt, after well
completion and after item settlement. Each branch must compare the restored
initial state **and** the same subsequent input suffix. Include a second save/
restart with further mutation for the completed exchange/recovery/item family.
Use production F9 and an actual separate CLI process, not only snapshot helpers.

Compare all 30 characters and all raw slots, membership, supplements/XP, camera,
game flags, independent Root/Q2 and overlays/new flag, all 19 actors and accounting,
purse and independent pending money/item sources, context and RNG; compare exact
on-disk bytes including envelope/CRC where the semantic endpoint is the same.
Also compare intermediate instruction/publication/receipt/RNG observations so a
replayed then cancelled effect cannot pass only by reaching the same final state.

Required additional genuine branches are Phirna No then retry; collected revisit;
post-exchange no-Root Myra request with unchanged rewards; real healthy-target
and cancelled-after-debit potion consumption; and gameplay after each. Cache
eviction at quest/restore boundaries must reload original data without resetting
live owners or replaying work. An untouched fresh-process control verifies no
session leakage. Negative artificial fixtures supplement capacity loss, multiple
Roots, inaccessible conditions and partial failures; they are labeled separately.

Retain M34 production witnesses alongside the new family: seed 3 `UFU` with Run
for wounded-survivor return; seed 24 `UFU` mixed Run and first-run-attack; seed 7
Snake first-run-block casualty escape; seed 14 sign-group item-run dormancy and
subsequent Orc reactivation; and seed 7 pending-first-run-block ready attrition.
Their exact selected recipes remain owned by `RunConsequenceWitnesses.py` and
its generated selection record. Keep explicit legacy 4/4 and 5/5 regressions;
retain the runner's M33 Snake/Toad/undead condition families, including Disease.
Also exercise the same rules under fresh 6/6. Include at least one contract-6
continuation from genuine disengagement through an explicit quest interaction
and save/restart, with wounded survivors/dormant or ready treasure preserved.
Use bounded seed/input selection only for this additional combined family:
retain the first valid seed/recipe, cap seeds at 1..256 and the existing attack/
input/return-move bounds, and fail visibly if no witness is found. Never replace
this with actor reset, direct camera placement or an injected quest item.

### Maintainer physical native-SDL acceptance

Provide bounded automated preparation/checkpoint validation and a concise route
card. A prepared save must itself descend from the genuine producer; it is not
a fabricated shortcut. Keep physical input continuous where an interaction or
handoff is being accepted. The maintainer personally checks:

1. Myra request, ordinary travel, Phirna Yes/collection, return/exchange and receipt
   are visible, readable and controllable, with continued navigation afterward.
2. Well selection/refusal and changed HP are understandable; inventory U identifies
   the potion/user/target, warns about charge consumption, and shows the actual
   Poison result without implying general healing or resurrection.
3. Modal F9/held-key refusal and any contact/receipt handoff show no unintended
   second action. A fresh F9, full exit and separate native CLI restart restore
   a playable scene; further travel/item inspection reflects retained consequences.

Automate route/seed preparation, arithmetic, exact state/bytes, resource faults and
rare refusal matrices. Automated SDL/image checks and independent review are
separate evidence from maintainer-performed physical acceptance. Neither this
investigation nor a future harness run claims the latter.

## Exclusions and later reassessment handoff

Exclude general magic/spellcasting; arbitrary item use or general-purpose recovery;
combat-time item use; full Rest; resurrection, temples, shops, inns and broader
services/economy; training/permanent progression; Swimming/Walk on Water/Mountaineer;
disconnected map-23 areas; Vertigo, mines, adjacent maps and Darkside transfers;
unrestricted higher-tier loot/effects; stat/AC wells, unrelated quests; general
world-flag scripting; new audio playback; unsupported calendar work; autosave,
in-session load and suspended-modal saves. No commercial payload is copied into
source or altered. No party recruitment/reordering, actor replacement or new
quest-completed gameplay mode is introduced.

The bounded world-flag projection, semantic 6/6 extension and documented item
cancellation/audio adaptations are explicit design decisions within M35, not
successor planning. Investigation found no route/time blocker to the roadmap
objective. Production acceptance and independent review remain outstanding;
unimplemented behavior is not evidence of acceptance.

At accepted M35 closure, retain for the later roadmap reassessment: the verified
committed SHA/Git handoff required by AGENTS.md; exact content/schema and legacy
compatibility results; retained connected and disengagement recipes/seeds;
uninterrupted/fresh-process comparisons and intermediate publication/RNG evidence;
which local injuries/conditions remain unrecoverable; observed time headroom and
calendar limits; actual reward losses/recipients, item cancellation behavior and
treasure independence; resource/authority failure results; independent review and
the separately attributed physical acceptance. Report any actual scope deviation
or feasibility limit. Do not perform that reassessment or choose a successor here.
