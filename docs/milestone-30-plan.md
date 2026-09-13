# Milestone 30 - Bounded expedition encounter integration

**Status: planned; not implemented.** This is the M30 implementation contract,
subject to architecture review and separate implementation authorization.
The investigated baseline is `cb28307d2898be77b1a425c334c357bd5a211e02`
(`Close Post-M29 route-selection gate`). The [roadmap](roadmap.md) owns the
closed Bone Whistle route decision. This plan does not reopen that decision or
claim production acceptance of the expedition.

## Objective and acceptance boundary

Extend the existing mutable Journey from one Skeleton to the selected original
Skeleton/Zombie expedition. The connected production boundary is:

```text
prepared entry -> bounded navigation/actor approach -> grouped combat
-> successful End -> guarded mutable return -> successive encounter
-> quiet explicit save -> process restart -> further navigation/item management
```

Enter Clouds map 20 once at `(0,14)` East. The player may occupy exactly
`(0..5,14)`, with all four facings, use Forward/Backward, turns and Wait, reach
`(5,14)` North, and return to `(0,14)` West. Terrain/content-edge refusal is
readable and commits no attempted movement or charge. Returning to the endpoint
does not terminate Journey; supported navigation and inventory remain mutable.

M30 must demonstrate original activation and movement, mixed and same-species
groups, joining, successive encounters, accumulated injury/Disease/XP/breakage,
survivors outside contact, and meaningful quiet save/restart. Killing all four
influencing actors is a useful control, not a prerequisite for every valid return.
No automatic healing, retraining, actor reset or resource injection occurs after
entry, between encounters, on End, or on restoration.

**M31 owns objective collection.** M30 leaves the original objective visible and
the objective cell reachable. Space at its event address reports that objective
interaction is not available in this milestone; it does not start WhoWill or
publish a grant/Remove. Entry/return feedback must say that encounter integration
is available and collection remains pending. No quest-completed claim is allowed.

Excluded: Run/disengagement, ranged attacks, spells including First Aid, Disease
cure, item use, rest/food/healing, services/training, recruitment/reordering,
map transitions, normal Vertigo progression/travel, broad world time, loot
expansion, autosave, mid-combat save, in-session load and Darkside gameplay.

## Foundations and evidence authority

Unchanged detail remains in [M26](milestone-26-plan.md) (classification, movement,
action/pulse timing), [M27](milestone-27-plan.md) (melee arithmetic, damage,
publication, random draws), [M28](milestone-28-plan.md) (completed-domain and
restore authority), and [M29](milestone-29-plan.md) (Journey ownership, inventory,
retirement, presentation and quiet persistence). M24/M25 item operations remain
unchanged. The M29 final handoff's historical route-gate wording is superseded by
the committed roadmap; its implemented content contract is unchanged.

Evidence keys below distinguish current implementation (I), original resource
observations (O), pinned-reference inspection (R), and bounded research (P).
Line ranges refer to the stated revisions, not future edited files.

| Key | Evidence and conclusion |
| --- | --- |
| I1 | `src/games/xeen/XeenActorApproach.cpp:17..164,185..267,420..468`: reusable original-order vector, 26 classification slots and capacity-three movement; hardcoded envelope/record-5 environment; current engagement clears pending work. |
| I2 | `src/games/xeen/XeenCombat.cpp:110..263,482..708`: seven-entry scheduler, direct actor-5 selection, one-target candidate, immediate lethal-to-End, source-style candidate publication and guarded retirement. `XeenJourneyProgression.h:14..38` already prepares identity-keyed lethal/XP/accounting atomically. |
| I3 | `src/games/xeen/XeenJourneyRules.cpp:20..84`, `XeenCharacterRules.cpp:122..139,191..258,263..279`, `XeenCharacter.cpp:25..44`: day-1 and injury-only admission; Disease already affects INT/PER/END and maxima; physical helper rejects it; canAct already permits Disease. |
| I4 | `src/games/xeen/XeenSaveSnapshot.h:52..79`, `XeenSaveState.cpp:25..129,298..307`, `XeenJourneyCapture.h:31..51`, `src/formats/xeen/XeenSaveFormat.cpp:196..231`: keyed actor vector already exists, but admission/capture/restore require one record and retain a seed, not continuation. |
| I5 | `src/games/xeen/XeenRestoreGuard.h:12..84`, `XeenStateEquality.h:8..31`, `XeenCombat.cpp:28..49`: retained values, optional supplements, lifetime/revision and equality checks must include new authority. |
| I6 | `src/app/XeenJourneyFlow.cpp:14..66,283..338`, `XeenEventFlow.cpp:552..608`, `XeenGameplay.cpp` (`Application::playGameplay`), `src/platform/sdl/SdlWindow.cpp:164..278`: current owners, automatic attachment, read-only Journey interaction, startup-only binding, displayed-generation input and F9 safeguards. |
| I7 | `src/games/xeen/XeenOutdoorScene.cpp:171..213`, `CloudsMapComposer.cpp:10..19,66..81`, `src/formats/xeen/XeenAssetSource.cpp:30..59`: ordered composition and typed shared sprite cache; actor-5/four-placement/global-appearance limitations. |
| I8 | `tests/XeenJourneyPersistenceTests.cpp:15..139`, `XeenJourneyTests.cpp`, `XeenJourneyTestSupport.h`, `XeenJourneyGameplayTests.cpp`, `XeenCombatTests.cpp`, `XeenActorApproachTests.cpp`; `CMakeLists.txt:135..192,485..497`: executable regression, authority, SDL, rules and process-test seams. |

Reference revision is **`6814ee9ba54582f5b5adcffab49efbbd8f589edd`**, verified
with clean source state in the checkout selected by CMake's
`SCUMMVM_SOURCE_DIR`; configuration/provenance belongs to
[dependencies](dependencies.md). Reference paths below are under
`engines/mm/xeen/`, except the named constants source.

| Key | Pinned reference inspected |
| --- | --- |
| R1 | `combat.cpp:464..755`, `interface_scene.cpp:3124..3403`: movement, occupancy, activation, selected slots and drawing. Stable M26 kernels are reused, not reimplemented from a research model. |
| R2 | `combat.cpp:807..963,1084..1165`, `interface.cpp:1591..1640,1708..1717,1833..1866,1922..1993`: attack count/reselection, speed/ties, acted state, pending entry movement, round-before-End and target controls. |
| R3 | `combat.cpp:288..364` and remaining `doCharDamage` injury call; `character.cpp:350..408,431..469,633..673`: physical saving throw, Luck, Disease before injury, effective attributes and Dead suppression. `character.h:149` declares condition values as runtime `int`, despite byte serialization. |
| R4 | `character.cpp:168..248,915..953`, `locations.cpp:1405..1423` and `BaseLocation::show`; `party.cpp:399..447,509..549`: CHR layout, residual experience/training, calendar and 480-minute processing boundary. |
| R5 | `interface_scene.cpp:38..170,528..560,3124..3403`; `devtools/create_mm/create_xeen/constants.cpp:405..414,480..501`: original projection anchors/scales, ATT relocation, spell mapping. Class XP bases are at constants lines 265..267. |

O1 uses unchanged `maze.chr` (30 x 354 bytes) and `maze.pty` from the initial
Clouds resources through `XeenAssetSource`/`XeenPartyLoader`. O2 uses
`maze0020.dat`, `maze0020.mob`, `maze0020.evt`, `DARK.CC/xeen.mon` (90 x 60 bytes),
and `XEEN.CC/008.mon`, `008.att`, `009.mon`, `009.att`. No resource payload is
embedded in the plan or redistributed.

P1 reuses the prior route probes and cumulative protocol. Their baseline
`45efc118a1a05418f6ec55e1c5896d8f812e6f91` differs from this baseline only in
`docs/roadmap.md`; production code/tests are identical. Four selected prepared,
no-healing replays were rerun: schedules 0/3/5 with seed 1 and schedule 5 with
seed 78. These are hand-written reference-backed group reductions using actual
MMModern resources and rule helpers, **not ScummVM engine execution or M30
production acceptance**. Their built-in 128 single-Skeleton comparisons and
literal primitive controls passed, as did the existing combat and approach
test executables. P2 is a focused new resource/derived-rule probe: prepared
values, four actor records, the 27-cell movement rectangle, objective events
and checked MON/ATT loading. P3 extracts unchanged `Combat::allHaveGone` and
`Interface::nextChar` into a minimal stub harness: empty contact after a final
player kill leaves that player unmarked and skips Round, whereas a live-contact
last-player turn services the enemy and exhausts the round. This corrects P1's
extra-Round assumption for a final-player lethal. A disposable corrected copy of
the reduction reran the same four selected traces; their listed outcomes did not
change. No broad route sample was rerun. P3 is an extracted-function control,
not a linked original-engine test.

## Bounded content admission

Use one immutable **Journey content descriptor**, selected at fresh entry or
from the explicit save discriminator. Keep `XeenEncounterEntry::Journey` as the
domain. Descriptor data defines entry camera, navigation cells, influencing
identities, prepared-entry policy, context bounds, resource predicates, event
disposition and compatible persistence representation. It owns no mutable party,
actor, camera, flag or random state. Do not create a separate Bone Whistle mode.

Contract 1 retains every closed M29 restriction, including its seed and pending
entry semantics. New **content contract 2** admits the expedition below. Core
algorithms consume capabilities/identities; record-specific knowledge belongs
in these content admissions, not duplicated branches in every consumer.

| Original record | Type/image | Original position | Initial HP | XP | AC/speed | Attacks/strikes/die | Preferred class/special |
| ---: | --- | --- | ---: | ---: | --- | --- | --- |
| 9 | Skeleton 8/8 | `(6,14)` | 20 | 250 | 5/10 | 1 / 2 / d6 | Cleric 3 / none 0 |
| 17 | Zombie 9/9 | `(8,15)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric 3 / Disease 7 |
| 18 | Zombie 9/9 | `(8,15)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric 3 / Disease 7 |
| 25 | Zombie 9/9 | `(1,13)` | 30 | 300 | 2/4 | 2 / 2 / d4 | Cleric 3 / Disease 7 |

All 27 world actors remain in original MOB order with exact original metadata.
The other 23 retain resource-initialized coordinates, HP, lifecycle and status,
remain unactivated/unaccounted, and continue to contribute to occupancy. Validate
their exclusion from the six-cell/four-facing activation union and movement scan;
do not remove them from the vector or mark them defeated. An unexpected influencing
identity is a content-support failure before its attempted publication.

Navigation is six cells; actor movement is larger. A conservative closed movement
region for these four actors is `x=0..8,y=13..15`. Movement toward a camera at
`y=14,x=0..5` cannot increase an actor's distance along either coordinate beyond
its initial/party extrema. Retain the original 32x32 occupancy representation and
7x7 scan centered on the camera; do not clip actors to the navigation cells.
Already activated actors outside that scan remain activated and stationary until
a later camera/work boundary includes them. No elapsed-time catch-up is allowed.

O2/P2 terrain in this rectangle: ordinary dirt surface 1, middle 3 (`rawWord=0x31`)
or middle 0 (`0x01`), attributes/flags zero, except `(5,15)` and `(7,15)` which
are water surface 0, middle 0, raw word 0 and attributes/flags `0x40`.
Those water cells are **blocked** destinations for nonflying actors, not fatal
unknown terrain. The camera's `(0..4,14)` cells are forest middle 3 and `(5,14)`
is middle 0. Reuse original terrain fallback, not party collision or an all-ground
approximation. Admission verifies this resource-backed bounded terrain and no
additional map behavior. Movement outside the supported region fails before
publication; the geometric invariant means legitimate admitted movement will
not reach that refusal.

Separate four checks currently conflated by `supportsApproach()`/`validateCombat()`:

1. Movement: nonflying, physical-status actors, no ranged movement behavior,
   supported destination terrain and existing occupancy/order rules.
2. Rendering: valid image, ordinary animation/effect profile, required normal
   and attack frames, supported selected projection slot.
3. Combat statistics: positive HP/speed/dice/hit ranges, bounded attack count,
   physical damage/resistance and supported rewards.
4. Special application: none or Disease, with an admitted physical saving throw.

Special-attack zero is not a movement/rendering requirement. Positively admit
the two resource profiles above; do not thereby admit every physical monster.
Both have physical resistance 50, physical attack type 0, category 4, no range,
flight or animation effect, and zero gold/gems/item-drop fields. Zombie elemental
resistances are not new player attack capabilities. MON offsets are XP 16..19
u32, HP 20..21 u16, AC/speed/attacks/preference 22..25 u8, strikes 26..27 u16,
die/type/special/hit/range/category 28..33 u8, physical resistance 40 u8,
gold 42..43 u16, gems/item 44/45 u8, flight/image/loop/effect 46..49 u8.

## Prepared entry and continuing values

Fresh construction loads all original owners/items/flags once, applies the
declared preparation once to active owners, derives full HP/SP, initializes all
27 actors once, and classifies the initial view once without moving. Record 25
is activated at `(1,13)`; initial contact is empty and pending is zero. This
entry is disclosed preparation, not a fabricated completed Vertigo world.

| Active index / roster owner | Name/class | Level | Stored residual XP | Current=max HP | Current=max SP | Permanent Luck |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 0 / 0 | Arturius / Paladin | 3 | 1000 | 36 | 6 | 12 |
| 1 / 18 | Tyro / Knight | 3 | 2000 | 48 | 0 | 14 |
| 2 / 14 | Badger / Ranger | 3 | 1000 | 36 | 6 | 10 |
| 3 / 11 | Zippo / Robber | 4 | 1000 | 40 | 0 | 17 |
| 4 / 1 | Rebecca / Cleric | 3 | 2000 | 21 | 21 | 14 |
| 5 / 6 | Seymour / Sorcerer | 3 | 1000 | 15 | 27 | 15 |

The 5,000 gross XP premise is backed by `maze0028.evt`, `(14,5)` West,
line 12, TakeOrGive parameters `00 00 10 88 13 00 00`. R4 training does not
store gross XP: for level L below 13, current total is
`CLASS_EXP_LEVELS[class] * 2^(L-2) + storedXP` for L>=2; at L=1 it is storedXP.
Thus `storedXP=5000-base*2^(L-2)` gives the table. Relevant bases are
Paladin/Ranger/Sorcerer 2000, Knight/Cleric 1500, Robber 1000. These are the
highest levels afforded by that XP premise; XP gains during M30 change the
residual u32 field only. No implicit training, max-resource refill or minimum
initial-XP equality test follows a kill. Zero residual XP is coherent for a
trained character; ongoing admission validates its u32 domain, not the fresh
derivation. Training costs for these increases sum to 390 gold, below the
original 800; no economy capability is implied or added.

Prepared values have the following provenance and lifetime:

| Values | Classification and policy |
| --- | --- |
| Membership `[0,18,14,11,1,6]`, counts 6, names/classes/races/sex, birth years, permanent attributes, modeled skills, hasSpells and item arrays | Original-loaded. All 30 owners retained; inactive owners unchanged. No equipment upgrade or fresh item normalization. |
| Levels and residual XP above | Reference-derived preparation from the explicitly selected gross-XP premise. No training replay. |
| HP/SP table | Runtime-derived after preparation using existing checked max rules and year 610; assigned only at fresh entry. |
| Day 8, year 610, minute 480, ctr24 0 | Explicit bounded-entry choice. Six first-character training day charges plus a training departure day explain a later morning, but day 8 is not proof of a traversed progression history. |
| WorldOfXeenClouds, Adventurer; nine effects and six light/resistance values zero; rested=false, newDay=false | Explicit prepared context, consistent with a settled morning after preparation. No pending new-day work. |
| All initial conditions, temporary attributes/level/age/AC | Zero preparation state; original-loaded zeros where already zero. Temporary reset is an entry premise, never replayed. |
| Might/Speed/Accuracy, temporary AC, XP supplements | Original-loaded for every owner, with only active preparation changes above. New permanent/temporary Luck comes from CHR 32/33, unsigned bytes. |
| Quest counters, quest flags, game flags, object/event overlays | Original-loaded/empty overlays remain deliberately unchanged. Do not set invented Vertigo quest flags, mark travel, consume food/gold, or disable the objective to justify preparation. |
| Food/gold/gems and unmodeled service/calendar/spell fields | Not new authoritative M30 consumers. Observed original supplies support the premise; no full post-Vertigo snapshot is constructed. |

All active temporary Luck values are zero. CHR supplement offsets remain
Might 20/21, Speed 28/29, Accuracy 30/31, temporary AC 34, XP 348..351 LE u32.
Prepared permanent M/S/A by active order remain
`17/16/15; 19/16/16; 15/15/12; 14/15/18; 12/14/13; 8/14/15`.
Original INT/PER/END remain
`13/13/19; 10/8/19; 15/15/16; 12/14/17; 10/20/16; 20/13/17`.

Original occupied item records below are `material/ID/state/frame`, with zero-based
physical slots. All unspecified slots, ID-zero metadata and the complete
Miscellaneous arrays are loaded verbatim, not synthesized from this summary.
Common accessory slot 0 is `38/2/0/12` for all six.

| Owner | Weapons by slot | Armor by slot | Additional accessories |
| ---: | --- | --- | --- |
| 0 | 0=`0/6/0/1` | 0=`0/3/0/3`, 1=`0/8/0/2`, 2=`0/13/0/6`, 3=`38/10/0/9` | None |
| 18 | 0=`0/2/0/1` | 0=`0/2/0/3`, 1=`0/9/0/5`, 2=`0/13/0/6`, 3=`38/10/0/9` | None |
| 14 | 0=`0/8/0/1`, 1=`0/30/0/4` | 0=`0/2/0/3`, 1=`0/13/0/6`, 2=`38/10/0/9` | None |
| 11 | 0=`0/12/0/1`, 1=`0/12/0/0` | 0=`0/2/0/3`, 1=`38/11/0/10`, 2=`38/10/0/9` | 1=`42/1/0/8` |
| 1 | 0=`0/15/0/1` | 0=`0/2/0/3`, 1=`38/10/0/9` | 1=`42/5/0/8` |
| 6 | 0=`0/7/0/1` | 0=`0/1/0/3`, 1=`38/10/0/9` | 1=`86/1/0/0` |

Keep the exact accepted Rebecca medal exception and M29 consumed-item readiness
rules. Inventory/equipment remains mutable at quiet boundaries. Current HP/SP are
signed i16 historical values; no normalization against changed equipment or
Disease maxima. The initial equipment constraint is not an ongoing equality
constraint. Unknown unequipped records remain preservable; unsupported equipped
contributions can make melee unavailable without making a quiet save invalid.

Spell evidence is separate from `hasSpells`: CHR offsets 121..159 hold 39
class-list knowledge bytes. Nonzero indexes in active order are `[21]`, `[]`,
`[1,20]`, `[]`, `[1,14,21]`, `[0,22,25]`. Current spell at 164 is respectively
`21,255,20,255,21,22`; quick option 165 and adventuring/combat selections 352/353
are zero. Rebecca's class-list index 14 maps to spell 26, First Aid, through R5
and `Spells::executeSpell` (`spells.cpp:65..76`).
M30 consumes only the already modeled hasSpells/max-SP inputs. It adds neither
spell knowledge ownership nor a spell menu; learning/casting and their save
contract belong to a later authorized consumer. No claim that hasSpells grants
First Aid is permitted. First Aid is **excluded**, with no infrastructure-only
stage: Disease damage/saving has no spell-system dependency.

Continuing/restored admission retains M29 byte-origin rule-input domains and
complete owner presence; it adds Luck presence and Disease and uses contract 2's
day 8/year 610. Active levels remain positive and inputs must satisfy their live
consumers; valid persisted XP/injury is not forced back to the prepared table.
No level/attribute-changing action is introduced. Condition index 4 is `0..255`,
12/13 are independently `0..1`, all other active condition bytes zero. Good or
Disease-only requires positive HP; either injury byte requires nonpositive HP.
At least one active owner must be able to act for a quiet continuation. Inactive
owners preserve existing representable values, not active-condition restrictions.

All fresh, continued and restored contexts require `480 <= minutes < 960`,
`ctr24 < 24`, day/year and zero effects/resistances as above, no rested/newDay
work. R4's 480-minute branch includes attribute death and time-condition/RNG work,
and its Disease conditional is not a basis for silently skipping that work.
Keep the inherited precharge stop: ten-minute charge at >=950 or Round/End at
>=959 fails before that operation's time/movement/effects. This applies after
restart as well. No crossing, rollover or delayed boundary debt is admitted.

For Round, perform this precharge check before clearing acted/blocked or
selecting the new round's first participant, including any enemy attacks that
selection makes due. At minute 959 no new-round enemy attack or RNG publication
may precede the refusal. This is the inherited MMModern support-boundary
adaptation, not the reference's location of `changeTime`. At an admitted minute,
new-round enemy attacks publish individually before movement/minute as specified
below; a later Round preparation failure preserves those publications and closes
continuation. Defeat during that selection prevents subsequent Round movement,
charge and player input.

## Group ownership, targeting and scheduling

World remains the sole owner of each actor's identity, coordinates, activation,
live HP, lifecycle/status and once-only accounting. Roster owns all characters,
conditions, equipment and XP. One noncopyable `XeenCombat` borrows these owners
for an entire connected contact episode. It persists across deaths and joining;
it is not destroyed/recreated for each monster. Camera/game flags keep their
existing owners. Copied characters/actors in an operation candidate are detached
preparation values, never a replacement combat party.

### Membership and participants

Contact membership is precisely the live Physical Present identities selected
by same-cell slots 0..2, in original record order. Adjacent/distant slots
activate and present threats but confer no melee attack. Capacity is **three
simultaneous monsters**, not four; the reusable scheduler therefore has six
party participants plus up to three enemy participants (nine entries with empty
enemy slots omitted from the speed table). R1 occupancy caps a destination at
three. P1 seed 78 reaches Zombies `[17,18,25]` together while 25 retains HP 17,
after Skeleton 9 dies; the third slot is an actual route consumer.

Actor identity and slot are different: slots 6..8 in the initiative participant
space describe the current original-ordered contact slots, while all retained
work is keyed by `XeenMonsterIdentity`. Reclassification must not transfer HP,
acted state, a target certificate or an animation from an old slot occupant to
its replacement. Remove dead identities from the current table; retain acted
state and current participant identity for surviving participants within the
round. New members are unacted in the new round; attachment joining occurs
before displayed player input. An inactive/absent slot never receives a turn.

Same-cell survivors cannot depart in this admission: combat fixes the camera,
movement's center delta is zero, and Run/rotation/status displacement are absent.
Normal reclassification compacts slots after death and adds incoming actors at
movement boundaries. If future work makes a living identity leave, it must keep
world HP and must not acquire a second action by leaving/rejoining; that broader
behavior is not silently admitted by M30. Off-contact actors remain world state,
not frozen copies or hidden independent duels.

### Target interaction

Use production keys **1/2/3** to select the displayed contact row; Space attacks
and B blocks for the displayed acting character. Preserve inventory's existing
numeric keys through phase-specific routing, with a distinct combat target
intent. No general UI overhaul or selection modal is required. At attachment
select the first contact identity. Keep selection across player turns and rounds
while that identity remains eligible, including when its row changes. A newly
joined lower-index actor must not silently steal selection.

After a target dies/leaves, choose the first eligible identity for the **next
displayed generation**, with explicit updated feedback. A command bearing the
old displayed generation/actor identity refuses; it never attacks the replacement
in that row. Selecting a row consumes no turn/time/RNG, but invalidates old input
and requires a matching presented selection frame before Attack. Each Attack
certificate binds combat/owner incarnation, actor identity, acting roster owner,
membership/table revision and displayed generation. Empty/invalid/dead/stale
rows refuse without RNG or turn loss. Block has no dependency on a stale target.
SDL batch and held-key rules apply to target selection as to acting-character
changes; a queued Space in the selection batch cannot attack the new row.

### Mandatory ordering

Retain M27 speed arithmetic and positive-speed ordering: descending speed, ties
by active party index then current original-ordered enemy slot. No initiative
RNG. Current prepared players have speeds `[16,16,15,15,14,14]`, Skeleton 10 and
Zombies 4; Disease changes none of those speeds. The supported Speed ring adds
3 through the existing equipment scan. Do not implement the engine as a hardcoded
all-players-then-monsters loop; service the actual current table and test ties,
multiple enemies and preserved participant identity. Combat readiness requires
positive effective Speed for each eligible participant; a representable quiet
state with an unsupported consumed value remains savable but cannot attach
combat. Do not tighten persisted byte-origin fields to the fresh-entry table.

Contract 2's attachment retains whether contact interrupted a nonzero approach
countdown. R2 initializes the combat party/table and selects the first participant,
then, if `_tillMove` was nonzero, performs **one** owed movement opportunity and
reclassifies before player input. Transfer that obligation atomically from
approach to combat; do not erase it, convert the count to several moves, charge
time for it, or execute it twice. For the prepared party and admitted equipment
changes no enemy precedes the first player. If valid current inputs put enemies
first, service their mandatory attacks during initial participant selection,
before the owed move, following `nextChar`; never silently reorder the table.
M29 contract 1 retains its accepted clear-pending behavior, whose isolated
same-cell actor made the distinction immaterial there.

At every completed player Attack/Block, mark that owner acted once. At an enemy
participant, execute each resource attack in order, reselection included, before
marking the enemy acted. No player input may interleave a Zombie's two attacks.
After each operation, reclassify as needed and update the table without resetting
surviving HP or acted facts. Determine party defeat before further actionable
work. A lethal player operation publishes only that identity's removal/accounting;
it does not automatically declare group victory.

At an exhausted round, use the inherited source sequence: clear acted/blocked,
rebuild table, reset/select the next participant, perform one original movement
opportunity over all actors, classify/activate and reconcile membership, then
charge one minute. Initial participant selection may itself require enemy
attacks with their ordinary individual publications, as at attachment; movement
and the minute form the subsequent bounded Round publication. Complete all of
that mandatory work before displaying the next player's turn. Joined actors
participate in this new round; surviving
actors retain HP. The source performs no Medusa reset for these species.

**Preserve the reference's completion checkpoints, including its empty-contact
shortcut.** `attack2` removes and redraws/reclassifies the dead target before
`nextChar`. That function checks `allHaveGone`, then exits on empty contact before
marking the just-acting player gone. Thus even a final eligible player's lethal
does **not** itself force a fresh Round. An internal implementation that marks
acted at publication must preserve this ordering explicitly instead of deriving
a new Round from its eagerly updated bits. P3 resolves this difference from the
old research reduction. Ordinary round exhaustion while contact remains does
require Round/movement and may join actors into the same coordinator. Already
owed entry/Round/candidate work cannot be canceled by a UI completion check.
Once contact is empty with no such obligation, End charges one minute and
performs the final classification check; only then can it succeed. Activated
off-contact survivors do not prevent End merely by existing. End adds no
unsolicited movement opportunity.

Compact deterministic transition controls (source-derived, not new resource
claims):

| Prestate/trigger | Required next result |
| --- | --- |
| Contact `[9,25]`, Attack kills 9 while another party owner is unacted | Account 9 only; retain 25 HP; compact contact to `[25]`; continue same round/coordinator. |
| Contact `[25]`, selected 25; 17/18 join during Round | Selection remains identity 25 as it changes row in `[17,18,25]`; old generation refuses. |
| Last contact killed by a non-final eligible player | No new Round; one End minute, then retirement/presentation. |
| Last contact killed by the final eligible player; 17/18 one move away | Reference empty-contact shortcut: no manufactured Round; separate End minute, retain off-contact survivors. |
| Round exhausted with live contact; 17/18 one move away | Mandatory Round moves/classifies them, adds one minute and continues the same coordinator; no End or save. |
| `[17,18,25]` with 25 damaged, after earlier deaths and joining | Three ordered enemy participants; retained selection/HP and per-identity accounting; no reset to base HP. |

### Publication and lifecycle

Player Attack remains one candidate/publication, including all character strikes.
For lethal damage reuse `xeenPrepareJourneyLethal`: actor HP 0, position
`(-128,-128)`, inactive Defeated, identity accounting and every eligible XP addition
publish together. XP is `floor(monsterXP/eligibleCount) * (level<15 ? 2 : 1)` per
eligible owner, with checked u32 addition. Disease and Unconscious remain eligible;
Dead does not. With six eligible prepared owners Skeleton awards 82 each and
Zombie 100 each. Overflow rejects that complete lethal publication; earlier
damage and other identities' XP remain committed.

An **enemy resource attack** is the atomic operation unit (one or two physical
damage applications on one selected character). The Zombie turn is two such
operations under one retained participant, with transient attack ordinal and
mandatory continuation. This reuses the existing two-injury/nine-armor result
capacity; it does not need a full-party candidate or a four-application aggregate.
Publish each attack's resulting HP, Disease/injury bytes, armor changes and RNG
cursor together. The second attack reads the first's published facts. Failure
in the second preserves the first, closes continuation and permits no save.

In the lifecycle table, W=world, P=party/roster, C=combat, F=Flow. Every operation
retains actual owner identities/preimages, boundary leases and current tickets.

| Transition | Owner and publication unit | Retained authority / next mandatory work | Navigation / save |
| --- | --- | --- | --- |
| Attachment | F binds one C to W/P/camera; consumes approach obligation | Same owners, membership and owed entry move; resource/readiness validation then first frame | No / no |
| Target selection | C/F transient selected identity and new generation | New presented identity certificate; same turn | No / no |
| Nonlethal Attack | W actor HP plus gameplay RNG, acted state and fixed result | Current table/identity; next participant or Round | No / no |
| Lethal Attack | W defeated identity/accounting + P XP + RNG, atomically | Other actors and partial rewards remain; reconcile using source completion checkpoints | No / no |
| Enemy attack | P one target's ordered injury/Disease/armor + RNG | Same enemy's next attack, next participant, Round or Defeat | No / no |
| New-round selection | C acted/blocked reset and table selection after precharge validation; any due enemy attacks use their individual publication units | Retained Round obligation; no player input before movement/minute | No / no |
| Round/joining | W movement/activation and P minute atomically; C membership reconciliation | Same coordinator; joined identities, next participant or End | No / no |
| Group completion candidate | C observes empty contact after mandatory work | Pending End, not durable completion | No / no |
| Successful End | P final minute + W/C current completion facts | Unique runtime End authority; no candidate/contact/owed work | No / no |
| Retirement | F/C guarded release and authority consumption only | Rebind same Journey owners in Presentation; invalidate combat tickets | No / no |
| Frame handoff | F matching successful SDL presentation | New quiet generation only if all guards still match | Yes / yes if quiet predicates pass |
| Failure/defeat | Current authority latches terminal/unavailable state | Preserve earlier publications; stale work cannot latch failure on newer owners | No / no |

## Zombie and Disease rules

Use M27 player dice/hit/resistance arithmetic unchanged, parameterized by the
selected identity's statistics. Prepared levels still yield one attack each;
retain the existing class/level formula rather than hardcoding that result.
Zombie damage is **two attacks**, each with **2d4** physical applications, hit
parameter 5, preferred class Cleric. It is not doubled 2d4 or one 4d4 attack.

For each resource attack, search active owners in order for an eligible Cleric.
If none, request `U[0,5]`; if that owner is disabled, form the ascending eligible
list and request `U[0,n-1]`, including a request when n=1. No eligible character
means defeat. Reselect for attack 2; do not reselect between the two applications
of a natural-20 attack even if application 1 incapacitates or kills the target.

For a selected awake target:

```text
r = U[1,20]
if r == 1: miss; end this resource attack
else:
    if r == 20: applyDamage()             # complete first application now
    hit = r + floor(hitParameter/4) + U[1,hitParameter]
    threshold = currentAC + (blocked ? floor(currentLevel/2)+15 : 10)
    if hit >= threshold: applyDamage()   # separate application, same target

applyDamage():
    clear Asleep                         # zero in admitted state
    damage = max(sum(strikes x U[1,die]) - powerShield, 0)
    if damage > 0 and special == Disease:
        v = statBonus(effectiveLuck) + currentLevel
        saved = U[1,v+20] <= v            # inclusive, exactly one request
        if not saved: Disease += 1        # before subtractHitPoints
    subtractHitPoints(damage)             # M27 injury/death/breakage order
```

For Zombie, the hit score is `r+1+U[1,5]`. A physical save prevents Disease,
**not damage**; no cold/poison/elemental resistance input or recursive resistance
roll occurs. Physical damage is not reduced for Adventurer. Skeleton special=0
must not acquire a saving-throw draw. The ordinary post-critical check uses AC
after any armor breakage from the first application.

New consumed input is **permanent and temporary Luck**, CHR offsets 32/33 u8,
widened using `XeenAttributeValue`. Store it as an explicitly present optional
member of the existing roster-owned combat supplement, required for all 30
contract-2 owners. Legacy supplement presence does not imply Luck presence.
Ordinary/v3/contract-1 owners keep it absent; their encoders must reject attempted
loss of a present new authoritative field. Do not add an always-authoritative
ordinary character field with an invented default that legacy saves omit.

`effectiveLuck=max(permanentLuck+temporaryLuck+itemBonus(category 6),0)` in this
admission. Luck has no age adjustment; Disease changes no Luck. Reuse the checked
attribute item scan across all nine weapon/armor/accessory slots: nonzero frame,
good state (`state & 0xc0 == 0`), admitted attribute material, including the
existing ID-zero behavior. Current original equipment/allowed rearrangements
give no Luck bonus. Existing material admission stays bounded; a future item
bonus cannot bypass readiness because Luck is now consumed. Current level,
range `v+20` and intermediates must be checked before random requests.

Audit parse/admission, equipment postvalidation, effective rules, combat's local
equality, `xeen_state::sameInputs`, all roster/snapshot copies, retained
`XeenRestoreGuard`, capture, codec and fresh-owner restore together. Required tests
mutate each new value during callbacks and through copied/ABA owners; it cannot
escape guards because the old equality ignores it. Equipment recomputes derived
values without writing the underlying Luck or clamping HP/SP.

Disease uses existing **condition index 4**, serialized at CHR offset 327
(`323+4`) and in the existing save condition array. Each failed save increments
severity, including repeat applications. R3 has runtime int conditions: the
`if (!++severity) severity=-1` source text is **not uint8 saturation at 255**.
M30 preserves byte storage and admits `0..255`; requiring 256 is an explicit
support failure before that resource attack publishes any of its candidate.
Do not wrap, saturate, clear Disease or silently widen the legacy condition
wire layout. Ordinary prepared play without healing cannot approach that limit
before incapacitation, but malformed/high-history states and arithmetic limits
still require deterministic controls. No general status-effect framework is needed.

Existing derived rules already subtract Disease severity from INT/PER/END,
floor the effective attributes at zero and recalculate max HP/SP. They suppress
condition modifiers for Dead/Stoned/Eradicated; retain this sometimes surprising
behavior. Add Disease to physical-helper admission without subtracting it from
Might/Speed/Accuracy/AC. No current HP/SP clamp or injury reinterpretation follows
a maximum change. In particular a current SP above the new maximum is valid.
Injury compares `maxHP + resultingHP` after the Disease write and before setting
new Dead; once historical Dead is set, later max recomputation never revives it.
Set Unconscious when that sum is positive and Dead otherwise; preserve both bytes
when both occurred. Break all occupied equipped armor on death or HP<=-10, with
exact original frame/other bytes retained. Weapons/accessories do not break here.

P2 controls for Rebecca at prepared level 3: D=0/1/2/4 gives max HP and SP
`21/21/18/15`; D=255 gives 3. With D=2, current HP 12 and SP 21 remain exactly
12/21. Disease-only is actionable, targetable and XP-eligible. With Unconscious
or Dead, action/target eligibility follows those worse conditions; Unconscious
still receives XP. Party defeat is no eligible acting member, not all Dead.

Required literal Zombie tape, assuming eligible level-3 Rebecca, unchanged Luck
14 (bonus 1), enough positive test HP and ordinary AC: per resource attack
`[1,20]->20; [1,4]->4; [1,4]->4; [1,24]->24; [1,5]->5;
[1,4]->4; [1,4]->4; [1,24]->24`. Repeat for attack 2: four damage
applications, total 32 HP and D+4, sixteen requests, two publication units.
Also test equality success at save value 4 versus failure 5, natural 1 (no
subsequent draws), ordinary miss, critical-first-only at high AC, changed AC
after breakage, and first-attack incapacitation causing second-attack reselection.

## Multi-actor presentation

Extend `XeenOutdoorScene::actorCommands` to emit commands from **selected slots**,
not one command for every actor with a placement. Reuse the 26-slot classifier,
ordered terrain/object/actor stream, `CloudsMapComposer`, typed MON/ATT appearances
and existing sprite cache. Projection availability is independent of species
admission. The existing four-placement switch is insufficient: distant paired
Zombies are already a current consumer of the remaining outdoor table entries.

Required table below is R1/R5, with arrays corresponding to the listed slots in
order. X values are defaults for one/three selected occupants; the final column
overrides the first two for exactly two occupants in that query group. All
commands use scene clipping; only same-cell commands also use bottom clipping.

| Query | Selected slots | Normal draw orders | X anchors | Y / scale | Two-occupant X |
| ---: | --- | --- | --- | --- | --- |
| 2 | 0,1,2 | 118,112,115 | -5,-67,58 | 2 / 0 | 31,-36 |
| 7 | 3,4,5 | 94,92,93 | -7,-38,25 | 34 / 8 | 8,-23 |
| 5 | 12 | 90 | -112 | 34 / 8 | - |
| 9 | 13 | 91 | 98 | 34 / 8 | - |
| 14 | 6,7,8 | 75,73,74 | -8,-24,9 | 53 / 12 | 0,-16 |
| 12 | 14,20 | 69,70 | -65,-85 | 53 / 12 | unchanged |
| 16 | 15,21 | 71,72 | 49,65 | 53 / 12 | unchanged |
| 27 | 9,10,11 | 52,50,51 | -9,-17,-1 | 59 / 14 | -5,-13 |
| 25 | 16,22,24 | 44,42,43 | -34,-41,-26 | 59 / 14 | -27,-37 |
| 23 | 18 | 48 | -58 | 59 / 14 | - |
| 29 | 17,23,25 | 47,45,46 | 16,-16,23 | 59 / 14 | 20,-12 |
| 31 | 19 | 49 | 40 | 59 / 14 | - |

Same-cell ATT relocates the actually animated slot's command from its normal
order to 121 with that slot's anchor/scale/clipping, leaving other actors in
their own normal orders. R5 companion orders 122/123 are power/overlay entries;
do not duplicate the monster there or invent new effects. Use an identity-keyed
transient appearance selection, not one ATT frame applied to every actor. Keep
the inherited sequential M27 cosmetic timing and bounded feedback pause, so one
resource-attack result is presented before the next, with no gameplay RNG.
Player positive-hit reaction and enemy attack feedback bind the result's identity;
dead/reclassified actors cannot lend their animation to a replacement slot.

O2/P2 validates 008.MON/ATT sizes 35946/22004 and 009.MON/ATT sizes 22076/14892:
eight normal frames and four attack frames each through the current checked
sprite path. Validate both sets at admission/rebuild; use normal 0..7 and typed
attack 0..3, never treat an ATT index as an ordinary frame. Missing/malformed
sprites fail guarded preparation; no placeholder grants gameplay admission.

Forest occlusion is retained. The prior original-resource entry frame confirms
dense forest at `(0,14)` East; it is a terrain-only composition, not proof of
M30 actor rendering. Actor activation/targeting must not depend on visible pixels.
Do not enlarge sprites, clear trees, suppress actors or alter coordinates for
readability. Test the original stream/occlusion and add readable UI outside it:
current cell/facing and time, pending/quiet/combat state, nearby classified
threats, numbered contact rows with name and live HP, unmistakable selected and
acting identities, displayed party actor, Attack/Block/target controls, and
ordered damage/Disease severity/injury/breakage/XP results. Identity numbers may
appear in diagnostics; gameplay must remain understandable through names/rows.
Inventory shows exact HP/SP versus recomputed maxima and Disease alongside
Unconscious/Dead, even when another condition is the worst-condition label.

## Durable RNG continuation

For content contract 2, **world/session owns one gameplay random continuation**
for the whole Journey. Keep the M27 xorshift32 algorithm and inclusive rejection
mapping unchanged: algorithm discriminator 1, nonzero u32 state, checked u64 raw
draw count. Seed once at fresh entry (explicit nonzero `--combat-seed` or one
sample with zero mapped to one); initial count is zero. Attachment borrows the
current continuation. No encounter-number-derived streams, reseeding, replay
from the original seed, or RNG reset at retirement are allowed.

Algorithm/state alone determine subsequent random values. The count is retained
as authority for the existing cursor's checked exhaustion behavior and exact
committed-position observation, not as another PRNG input or a replay history.
Contract 2 fixes that limit to u64 independently of host `size_t`. Restore retains
the count exactly: a quiet count of `UINT64_MAX` is representable, but any later
operation requiring a raw draw fails before publication; draw-free operations
do not overflow it. Do not infer or validate a seed/state/count history relation.

The existing cloneable `XeenCombatRandom` remains the calculation cursor. An
accepted operation copies the current continuation into its candidate; successful
publication advances the world continuation atomically with that operation's
effects. Include it in retained world preimages, equality, revisions and capture.
Round/End consume no random values within the admitted time window. Joining
does not construct another RNG owner. Contract 1 still stores its original
Skeleton seed unchanged and keeps its active combat cursor transient.

Candidate prefixes remain uncommitted across the existing maximum 64 raw/provider
draws per service call. Retain rejected conversion draws and exploding-d20
prefixes; never reroll a prefix on the next service. A stale/refused command
consumes no live RNG. A current preparation failure leaves the live cursor at
the preceding published operation, marks continuation unavailable and permits
no retry/save. Earlier published enemy attack 1 is retained if attack 2 fails.
Raw draw-count overflow fails before publication; no wrap. Immutable test tapes
are transient diagnostics, not serialized or accepted as production RNG state.

Rendering, animation, target selection, inspection, inventory, saving, preflight
and restoration consume zero gameplay RNG. Preserve original request order and
inclusive endpoints; deterministic restart equivalence is an MMModern contract,
not a claim of matching ScummVM's PRNG bitstream or seeds. Separate per-encounter
streams would change the reference-compatible request sequence and need an
unnecessary encounter-stream protocol; a saved seed alone loses continuation.
The existing value cursor plus a world-owned continuation is the smallest
reusable extension with a present consumer.

## Persistence and compatibility

### Chosen format policy

Keep the v4 envelope and unchanged v2 base payload. Add **Journey extension
schema 2 / content contract 2**, explicitly paired. Do not change the meaning or
length of schema 1 / contract 1. Compare the alternatives:

| Alternative | Decision |
| --- | --- |
| Another content contract using schema 1 unchanged | Insufficient: four actor records alone fit the vector, but Luck and continuation/count change authoritative fields and wire layout. Reusing the seed field as a cursor silently reinterprets it. |
| v4 with schema 2 / contract 2 | Selected. The existing extension is already discriminated; the base ordinary character layout need not change because Luck belongs to the explicitly extended optional supplement. Old readers fail closed on schema 2. |
| New v5 | Unnecessary for this bounded supplemental change. Required reconsideration if implementation instead changes common character encoding or cannot retain legacy presence/encoding independently. |
| Resource-reconstruct Luck and seed/replay RNG | Rejected: hides missing authoritative inputs, cannot preserve current values, requires gameplay replay or a new stream design. |

Only `(version,domain,schema,contract)=(4,3,1,1)` and `(4,3,2,2)` are supported
Journey combinations. Unknown/crossed discriminators reject, including schema 1
with contract 2 and schema 2 with contract 1. Never select the expedition from
coordinates, level, Disease presence or absent legacy data. No implicit upgrade,
conversion or on-read rewrite. Saving a contract-1 session still writes its
exact 1060-byte schema-1 suffix. Mixed completed/Journey extensions reject.

| Input | Restore and subsequent explicit save |
| --- | --- |
| v1/v2 ordinary | Existing ordinary semantics, v1 missing-item policy, v2 explicit arrays; no new Luck/expedition authority. Eligible F9 writes v2. |
| v3 completed Diagnostic27 | Existing completed terminal/read-only/R behavior and six legacy supplements; F9 writes v3. |
| v4 schema 1 / contract 1 | Exact closed record-5 M29 Journey, original seed semantics and four-cell/day-1 admission; F9 retains that representation. |
| v4 schema 2 / contract 2 | Prepared expedition current-value domain, four keyed actor records, full optional Luck supplements and world RNG continuation; F9 writes schema 2. |

Schema 2 starts immediately after the unchanged v2 disabled-event list; all
multibyte values are little-endian. Exact suffix length is **1366 bytes**:

| Offset | Field | Encoding |
| ---: | --- | --- |
| 0 | Domain | u8=3 |
| 1 | Extension schema | u16=2 |
| 3 | Content contract | u16=2 |
| 5 | Context presence | u8=1 |
| 6 | Context | Same 33-byte context encoding as M29 |
| 39 | Supplement count | u8=30 |
| 40 | Owner-ordered supplements | 30 x 41 bytes |
| 1270 | RNG algorithm | u8=1 (M27 xorshift32) |
| 1271 | Current RNG state | nonzero u32 |
| 1275 | Committed raw draw count | u64 |
| 1283 | Initialized map | u8 Clouds=0, u16=20 |
| 1286 | Original actor count | u16=27 |
| 1288 | Live-record count | u16=4 |
| 1290 | Identity-ordered actor records | 4 x 19 bytes, identities Clouds/20/9,17,18,25 |

Each supplement is the schema-1 33-byte owner/seven-i32/XP record followed by
permanent and temporary Luck as two LE i32 values. Presence is mandatory and
implicit only within schema 2; no per-owner presence byte. All nine numeric
supplement inputs validate `[0,255]`; XP remains full u32. Runtime optional Luck
must be present for these records and absent for legacy encoding. Schema 2
does not append a legacy Skeleton seed or encode a second RNG owner.

Actor record encoding is unchanged: side u8, map u16, original record u32,
x/y i16, HP i32, activation/lifecycle/status/accounted each u8, for 19 bytes.
Use existing enum encodings and canonical booleans. Bound allocations before
reading, require exact counts, increasing unique identities and exact EOF, and
retain the 4 MiB length/CRC/archive fingerprint checks. Generic wire ranges remain
coordinates `[-128,31]`, HP `[0,65535]`; resource/domain checks are stricter.

### Durable versus reconstructed state

| Fact | Classification / validation |
| --- | --- |
| Four actors' coordinates, activation, HP, lifecycle/status, accounted state | Durable keyed records, always all four, including unchanged/unactivated members. Required exact identities/order/count; no four ad hoc named fields. |
| Original MOB identity/order/spawn/type metadata; MON stats, images, events and terrain | Immutable/resource-reconstructed, checked against compatible archives and content descriptor. All 27 actors reconstructed in original order, then four saved records applied. |
| Other 23 actors | Resource-reconstructed; capture proves exact resource-initialized state and no activation/accounting. No silent loss of a changed bystander. |
| Damage history | Live HP is authoritative; no redundant cumulative-damage scalar or transcript. Partial live HP survives every active-group reconstruction. |
| Party, equipment, conditions including Disease, context, residual XP and Luck | Durable current values on existing owners; complete base payload plus thirty extended supplements. Derived maxima are recomputed, current values preserved. |
| Gameplay RNG | Durable algorithm/state/raw-count; no replay. |
| Ordinary quest/game flags, counters and disabled object/event sets | Existing durable independent categories, preserved exactly. M30 does not publish objective mutations; no invented quest-completion bit. |
| Contact slots, initiative, acted/blocked, enemy attack ordinal, selected target, prefixes, End ticket, pending approach work, leases, frames/generations | Transient. They cannot be serialized to manufacture an eligible save; quiet capture proves their absence/completion. |
| Quiet group facts | Reconstruct empty contact from saved actors/camera. No persistent coordinator, group identifier, victory counter or serialized End capability. |

For a Present quiet survivor require Physical, HP equal to resource base (20/30),
unaccounted, within that actor's reachable coordinate envelope, not at the camera
cell, and consistent activation. A never-activated actor must still be at its
original position/full HP. An activated actor may have stayed at spawn or moved;
leaving view does not clear activation. Per-actor geometric envelope is Skeleton
9: `x=0..6,y=14`; Zombies 17/18: `x=0..8,y=14..15`; Zombie 25:
`x=0..5,y=13..14`, restricted by admitted terrain. These are conservative
current-value bounds, not claims that every Cartesian combination is reachable.
Reject same-cell contact, water placement, more than three co-occupants, missing
activation for currently classified actors, malformed identities or changes to
immutable metadata. Defeated requires HP 0, `(-128,-128)`, inactive Physical,
accounted; accounting is exactly the set of defeated saved identities.

**Damaged living survivors are not reachable at a supported quiet boundary.**
Only same-cell melee can damage actors; combat holds the camera fixed; these
actors never leave the center cell; no Run/displacement/healing exists. Therefore
a damaged survivor keeps contact/combat active until defeat and cannot be saved.
Do preserve damaged HP while membership changes *within* combat. Saving full-HP
moved/activated survivors outside contact is required, including 17/18 at
`(8,14)` or `(7,14)` in P1; it must not require all four accounted. A later
capability that makes damaged quiet survivors reachable must revise content
admission explicitly, not introduce mid-combat save now.

Do not enforce a replay-derived XP total or recompute injuries against current
maxima. Archive compatibility and content checks are validation, not cryptographic
authentication of an entire play history. Existing unrelated ordinary overlays
remain independently validated and preserved, without synthetic relationships
between quest counters, flags and actors. M30's known objective events must remain
available under its deferral contract: reject disabled Clouds/20 event records
1..5 and disabled Clouds/20 object record 1 at capture and restoration. Validate
this alongside immutable event/object topology, before any first-frame or
interaction admission. Preserve unrelated overlays independently; do not clear a
conflicting overlay to repair the save. Collection changes belong to M31 admission.

### Capture and restart

Retain M29 startup-only fresh-owner candidate restoration, retained source and
destination preimages, restore-only handoff, archive compatibility and first-frame
presentation gate. Extend their comparisons to content identity, Luck, all four
actor records/accounting and RNG continuation. Restore sequence:

1. Decode/version/domain/structure/archive checks; build unpublished current-value
   party/camera/flag candidates with all saved supplements/context.
2. Reconstruct immutable resources and all 27 original actors; apply the four
   keyed records and RNG state, validate domain, quiet configuration and overlays.
3. Prepare a first frame at the saved camera using pure classification/drawing,
   under retained owner/resource guards. Do not publish activation from this query.
4. Publish once to fresh final owners as Unbound Journey, consume the existing
   restore-only binding before EventFlow's gameplay borrow, and admit exactly
   that borrow revision through the same guard.
5. Require the matching real presented frame before new input/save eligibility.

No CHR/PTY preparation fallback, heal/retrain, movement, activation, attack,
Disease application, XP award, event dispatch, RNG draw or action replay occurs
on load. Reading immutable compatibility metadata is distinct from replacing
saved mutable fields. Invalid successor data fails startup without fresh fallback.
Derived contact/appearance may be recomputed; runtime authority is newly bound,
never deserialized from old pointers or tickets.

## Quiet boundaries, failure and objective deferral

F9 requires a current successfully presented quiet generation, valid current
owners, pending approach zero, empty contact, no combat/candidate/attachment/
Round/End/retirement work, no modal/inventory/certificate/event/reward/save lease,
no unresolved presentation, and no terminal/integrity/fatal state. The capture
authority reads actual coordination; a caller cannot assert that it is quiet.
Refused F9 does no capture/provider/preflight/destination/file work and never
queues a later save. Closing a modal or finishing combat requires a **new** F9.
Keep installation-containment/alias protection, checked target, sibling temporary,
flush/close/replace and old-file protection unchanged.

Successful End is runtime authority for one guarded retirement after service
busy work unwinds. Retirement changes coordination only, preserving all party,
actor, progression, RNG and context facts; the return frame opens the next
mutable input generation. Defeating a group member, deleting a coordinator or
showing a victory-like frame grants none of these permissions.

Defeat (no eligible party actor), unsupported content/time/condition boundary,
failed End, fatal presentation, shutdown or integrity violation leaves the graph
unsaveable and unavailable for gameplay continuation. Explain the particular
failure, preserving already-published damage, Disease, breakage and partial
lethal/XP accounting even while another enemy survives. For unchanged-authority
recoverable composition failure, retain the published result and allow the
inherited one guarded rebuild; capture stays closed until the proper frame.
Do not replay the action to regenerate feedback. Integrity latches are monotonic;
byte restoration cannot revive authorization. Stale/reentrant callbacks cannot
publish, fail or release newer work. Recovery is process restart from the last
successful explicit save; M30 adds no in-session recovery.

At present, environment validation forbids all events in the M29 footprint,
and Journey interaction calls ordinary navigation inside `journeyRead`, then
requires `NoEvent` (I1/I6). That is unsound for the objective: its first WhoWill
can suspend, and later grant/Remove publish mutations which the read preimage
must reject. **Do not run the script and then catch the guard error.**

Descriptor event disposition replaces only the content-specific event-free
assumption. Contract 1 stays event-free. Contract 2 validates the unchanged
16-record `maze0020.evt` topology with exactly records 1..5 in its footprint:
`(5,14)`, direction All, lines 0..4, WhoWill `00 03`, DisplayBottom `00`,
If2 `2C 01 03`, TakeOrGive `00 00 15 64`, Remove. They are manual (no automatic
cell flag) and the object is original record 1. Direction All means deferral
applies on Space at that cell in **every facing**, although the acceptance
route faces North. Preserve event bytes/order and first-match semantics.

M30's disposition is **Deferred**: after current-owner/input/quiet checks, inspect
the admitted event address without dispatch and present the unavailable-objective
notice. No script modal, quest grant, object/event disable, camera/flag publication,
gameplay time or RNG occurs. A notice may use ordinary presentation ownership;
if modal, it holds a presentation lease and blocks save until acknowledgment and
a new frame. No-event Space elsewhere keeps the read-only path. Navigation and
actor work have priority; Space never drains pending approach or skips contact.

M31 can add an authorized Event activity to this same Journey coordination:
acquire an exclusive event/modal lease at a presented quiet boundary, bind
EventFlow to the same owners, authorize specific immediate grants/Remove on the
existing party/world publication paths, adopt only those verified changes, and
retain the lease through WhoWill, reward/notice acknowledgment and recomposition.
Camera/game flags commit on successful event completion; earlier party/world
consequences survive later event failure under their existing semantics. Resume
Journey only after no pending event/combat/actor work and the guarded new frame;
quiet saving remains unavailable during a suspended script. This is the seam
M30 must leave intact, **not authorization to implement M31's mutation capability**.
Do not broadly weaken `journeyRead`, ordinary marked-owner checks or publication
preimages to anticipate M31. Deferral is a content admission of existing
coordination, not a replacement event engine.

## Reusability and implementation stages

Every foundation below is either a reusable engine capability (E) or a
content-specific admission of one (C). No slice-specific workaround is selected.

| Existing seam -> minimal change | Bounded first admission | Dependencies | Class |
| --- | --- | --- | --- |
| Journey entry/environment -> immutable content descriptor | Contract 2's six cells, four influencing identities, prepared entry and Deferred events | Resource/domain evidence | C |
| Original actor vector/classify/move -> capability-specific predicates and identity lookup | Skeleton/Zombie profiles; original occupancy and larger movement region | Descriptor, terrain | E + C |
| `XeenCombat` single target/table -> three contact identities, nine participants, identity-bound work/selection | This expedition's groups and joins | Actors, existing borrowed owners | E |
| Existing enemy candidate -> resource-attack ordinal and required continuation | Two Zombie attacks, each at most two applications | Group participant scheduler | E + C |
| Roster supplements/derived rules -> optional Luck and physical-save/Disease application | Special 7 and condition 4 only | Explicit data presence, checked arithmetic | E + C |
| `XeenJourneyLethal` -> use selected identity throughout all consumers | Four independently accounted actors, group-level End | Group lifecycle | E |
| Cloneable combat cursor -> world-owned Journey continuation | Contract 2, algorithm 1 | Atomic action publication, equality/guards | E + C |
| Snapshot actor vector/v4 extension -> schema 2 and extended supplements/cursor | Exactly four live records, quiet survivors | Complete domain validation | E + C |
| Ordered composer/cache -> selected-slot command table and identity appearance | Images 8/9, required outdoor slots | Resource checks and current group identities | E + C |
| Flow/SDL generations -> combat target intents and Deferred interaction routing | Keys 1..3, readable threats/consequences | Group state, frame handoff | E + C |

Select **two stages**. Splitting by individual mechanic would leave a partially
converted combat owner model; combining everything would put SDL/process work
on unreviewed group and persistence contracts. Stage authorization is separate
from plan approval and from prior-stage completion.

### M30A - Coherent grouped domain and durable representation

**Objective:** implement the complete reusable group lifecycle and successor
data contract before production expedition exposure. It must leave all accepted
legacy domains coherent and operable, with one combat implementation.

**Authorized scope when this stage is separately approved:** descriptor-based
admission/fresh prepared construction; capability split; group identity/table/
target-command domain; owed attachment movement and joining; per-attack Zombie
continuation; Luck/Disease; group completion/End/retirement; world RNG; schema-2
snapshot/codec and guarded capture/restore domain services. Implement actor,
party/roster, combat, equality and save owners together; do not merge a state in
which one consumer still treats an expedition as actor 5 or silently omits Luck.

**Architectural decisions/dependencies:** all preceding contracts are binding.
Reuse current Journey and diagnostic policies on shared algorithms. New data
presence stays explicit; legacy codecs retain exact layout. Stage tests may use
the existing headless service/provider seams to bind/retire/restore real owners.
Do not create a new diagnostic gameplay path or copy the research combat model.
No production command may create/restore a successor session until M30B's
required presentation and controls exist; fail before publication in Application
if successor startup is not yet enabled. Ordinary, diagnostic and contract-1
production entry/load remain available. Internal successor codec/domain tests
do not constitute public expedition support.

**Affected authority:** existing world actor/accounting and new continuation;
roster-owned optional Luck; party context/conditions/XP; one combat borrow and
existing Flow boundary; snapshot transfer values and source/destination guards.
No replacement graph or serialized runtime capability.

The M30A read-only Journey service refuses the Deferred objective address before
invoking a script-capable callback, in every facing. The player-facing notice
and its presentation handoff remain M30B work; this domain refusal creates no
modal state or gameplay mutation.

**Reusable value:** shared identity-based group combat, ordered multiattack and
condition application, durable current-state continuation and a content admission
seam. Existing Skeleton production behavior exercises the shared engine under
its preserved policy. Production expedition presentation is the dependent stage.

**Bounded evidence/acceptance:** deterministic group/target/initiative/owed-move/
round controls, literal Zombie/Disease tapes, pure derived-stat and overflow
controls, per-identity lethal publication/failure tests, RNG cursor adoption,
schema-2 exact layout/malformed rejection and owner-guard round trips. Original
resource controls must reproduce O1/O2 prepared values, species profiles and
terrain; use R1/R2/P3 for scheduling, not P1's corrected extra-Round assumption.
Run relevant existing combat, approach, character, Journey and persistence
regressions plus a configured build. Run full CTest when practical; stage
acceptance cannot carry failing tests.

**Exclusions:** public expedition CLI/SDL exposure, new spell/event mutations,
M31 collection, mid-combat serialization and milestone closure. Headless tests
must not be represented as physical or connected production acceptance.

**Handoff/review gate:** independent technical review is required before M30B
builds on the group ownership, scheduler checkpoints, atomic operation units,
Luck presence, RNG lifetime and wire policy. These are expensive to reverse.
Provide precise revision, focused diffs, deterministic/reference controls and
legacy regression evidence. Resolve required corrections before dependent work;
acceptance of M30A alone does not authorize M30B.

### M30B - Production expedition, presentation and restart acceptance

**Objective:** expose the same domain through the existing production Journey,
with readable groups/consequences and genuine quiet process continuation.

**Scope:** resource-validated multi-slot MON/ATT composition, identity-based
feedback and target controls, fresh-entry CLI, displayed-generation routing,
automatic group attachment/retirement, Deferred objective notice, inventory
continuity, Application F9 and startup-only successor load. Complete production
source/preflight/destination and first-frame gates using M30A's domain services.
No headless shortcut may supply completion or quiet authority to SDL.

Use **`--journey-expedition`** as a fresh-entry selector for the descriptor in the
existing Journey domain. Retain `--journey-skeleton` unchanged. Support the same
strict optional nonzero `--combat-seed`, game-directory and `--save-file` grammar,
duplicate/conflict rejection and one-time sampling policy. `--load-game` derives
the descriptor from the save and accepts no seed/preparation/entry override.
This is one domain with two explicitly admitted entries, not a new gameplay mode.

**Dependencies/authority:** accepted M30A and its independent review; existing
Application/EventFlow/EncounterFlow/SDL loop, composer and sprite cache. Flow
retains exclusive busy/modal/presentation/save authorization; consumers do not
invent quiet state from an empty contact list or completed-looking frame.

**Production value:** the player can navigate out and back, identify/select
group targets, resolve automatic enemy/round/joining work, observe Disease and
injuries, rearrange items at quiet boundaries, explicitly save and resume the
same consequences in a separate process. Reaching the objective produces honest
deferral with no grant/Remove; survivors remain part of the world.

**Evidence/acceptance:** production integration/SDL/CLI tests; original MON/ATT
frame and forest occlusion checks; connected group/successive/survivor traces;
real producer/consumer process save-restart equivalence; no-replay probes;
malformed/legacy file regressions; stale target/key/F9 and callback controls;
post-publication composition failure. Use current service/test seams named in I8.
Run the configured build and full CTest suite at milestone closure.

Independent implementation review must accept the integrated result before
**maintainer-performed physical SDL acceptance**. Automated/dummy-SDL/image
evidence does not substitute for that physical gate. Required manual checks:
fresh prepared entry; all six cells and return; visible target selection between
multiple enemies; automatic multiattack/joining; readable Disease/current versus
maximum HP/SP and broken items; quiet inventory/F9; separate-process restart and
further mutation; objective-facing Space deferral; unaltered legacy entry/load.

**Exclusions:** all milestone exclusions and M31 objective integration.
**Handoff:** accepted group/current-state/capture/event-disposition seams for
M31, exact resulting verified committed revision when closure is authorized,
and concise final acceptance record. Update stable status/history/roadmap/README
only at that future accepted closure under AGENTS.md; this planning task changes
none of them.

## Required validation matrix

The implementation must record exact request/state traces for complementary
deterministic controls. A favorable seed alone is not acceptance. P1/P2/P3 are
specification evidence; future tests must execute production owners/services.

| Area | Required evidence |
| --- | --- |
| Prepared/current state | All six table rows and thirty complete supplements; original item arrays/knowledge observations; no fresh equality on accumulated XP/HP/SP; separate fresh entry stays fresh. |
| Admission | Every record-5/four-cell path in I1..I7 switched to explicit policy where needed; non-influencing actors unchanged; Zombie special does not reject movement/rendering; water blocks/fallback; no hidden actor clipping. |
| Groups/scheduling | Solo `[25]`, solo `[9]`, mixed `[9,25]`, pair `[17,18]`, triple `[17,18,25]`; original order, occupancy-three, HP-preserving joins and reclassification; entry pending zero/nonzero; speeds/ties and no initiative RNG; P3 final-lethal shortcut and ordinary exhausted-round movement. Enemy-first new-round selection at 959 refuses before attacks/draws; at 958, later movement failure retains published enemy attacks, and selection-time defeat prevents movement/charge. |
| Targeting | Select every occupied row; retain identity across turns/slot compaction/joining; dead/empty/replaced/stale row and batch input refuse; Block coexists; no numeric inventory action leaks into combat. |
| Zombie | Both resource attacks including per-attack reselection, critical-first/ordinary-second sequence, first attack disabling Cleric, fallback requests including singleton, intermediate armor/HP and defeat; skeleton draws unchanged. |
| Disease | Save equality success/failure, repeat, two critical attacks, zero-damage suppression, before-injury maximum, Dead suppression, D=255 increment refusal, full byte round-trip, positive HP/above-max SP preservation, physical attributes unchanged, XP/canAct/defeat predicates. |
| Publication/failure | Nonlethal HP; per-identity lethal/XP atomicity and overflow; no repeated award; enemy attack-1 publication retained if attack 2 fails; lethal while another survives; failed End, terminal defeat and no artificial quiet retirement. |
| RNG | Exact inclusive request sequence and raw conversion rejection; retained >64-draw prefix, singleton draw; candidate failure/staleness/no reroll; world cursor advances only on publication; consecutive encounters and join use one stream. Quiet maximum-count round-trip and next-draw refusal; count exhaustion partway through an attack rejects its entire candidate and retains the preceding world state/count. |
| Saves | Exact 1366-byte suffix, order/duplicates/counts/boolean/enums/ranges/EOF/CRC/discriminators; no missing Luck, no seed reinterpretation; reject damaged/contact quiet actor, inconsistent activation/accounting, bystander mutation and time boundary. Preserve ordinary overlays independently. |
| Restart | Uninterrupted versus quiet-save/separate-process-restart: same subsequent RNG requests/state/count, actor outcomes, party HP/SP/conditions/XP/items/context. Test before first combat, between successive groups with survivors, and after return. No preparation, activation, pulse, combat, Disease, XP or event replay. |
| Authority | Every new field included in equality/capture/restore preimages; owner destruction/replacement/ABA, reentrant providers and stale tickets; saves blocked through attachment, enemy ordinal, round, End, retirement and frame handoff; stale/queued F9 performs no provider/I/O. |
| Presentation | All admitted selected slots/1-2-3 arrangements, same-name enemy distinction, correct ATT identity and order 121, cache discard/rebuild, original forest occlusion and readable threat panel; result failure retains publications and closes capture. |
| Event seam | Objective `(5,14)` all facings: original records/object unchanged, no dispatch/grant/Remove/flags/time/RNG; quiet notice and new frame needed; pending actor/combat work wins; M29 NoEvent path unchanged. Reject each disabled objective event and the disabled objective object on restore/capture while preserving unrelated valid overlays. |
| Legacy | v1/v2 ordinary semantics, v3 completed Diagnostic27 and v4 schema-1/content-1 exact bytes/seed behavior; existing M27/M28/M29 literal controls, CLI conflicts and process restoration pass. |

Connected production witnesses must include both a group/condition route and a
successive-encounter route with a save between encounters; a survivor-return
variant must preserve activated 17/18 outside contact. Reproduce the source-backed
structural checkpoints below with explicit input/pulse schedules. Exact P1
numbers are useful differential controls, not permission to inject results:

| Prepared no-healing research control | Checkpoints |
| --- | --- |
| Seed 1, one post-action pulse plus drain before each next navigation input | 25 contact at `(1,14)` minute 490, End 491; 9 contact at `(4,14)` minute 521, End 522; 17/18 survive activated at `(8,14)` on return, minute 582. |
| Seed 1, same schedule plus three East Waits at `(5,14)` | Pair 17/18 contact minute 562; End 565; Rebecca HP 5, max HP 18, SP 21, Disease 3; return minute 615. |
| Seed 1, immediate next navigation after post pulse, retained entry move | Mixed 9/25 at `(5,14)`; End 532; Rebecca HP 12/max 18, SP 21, D2; 17/18 at `(7,14)` retained through return. |
| Seed 78, same fast schedule | Mixed contact, ordinary rounds join 17/18; triple `[17,18,25]` with 25 HP 17 at the minute-533 round; End 536. |

These research scripts also invoked the already-supported ordinary event in
separate research owners. It charged no time/RNG or injury/XP change, so the
encounter checkpoints remain useful with M30's no-dispatch deferral; they do not
establish mutable Journey event acceptance. The corrected final-kill branch
does not change these four controls. Add deterministic unfavorable controls for
death/breakage and total defeat; do not promise that every prepared seed returns.
The final connected production witness must include real accumulated consequences
and post-restart item/navigation mutation, while clearly leaving collection to M31.

## Constraints, review and replanning

Use current abstractions and one authoritative home per field. Keep English
identifiers/diagnostics/docs and resource-driven game content. Preserve original
commercial resources, pinned reference checkout, legacy domain meanings and
existing safety/ownership guards. Internal type names, factoring and UI wording
may vary; group capacity, source checkpoints, target identity safety, operation
units, Disease arithmetic, RNG lifetime, compatibility and acceptance gates may
not be left implementation-defined.

No load-bearing evidence gap remains for this contract. The final-player Round
correction and byte-condition overflow boundary above are explicit decisions,
not reasons to reopen route selection. Independent architecture review should
pay particular attention to these, optional Luck presence, group retirement and
the schema-2/content-2 pair before implementation authorization.

Replan narrowly if current production/reference/resource evidence shows another
influencing actor, required movement outside the admitted region, different
group capacity, a condition/time boundary required for ordinary completion,
unpreservable legacy authoritative data, or a concrete reason objective deferral
cannot safely precede dispatch. Engineering effort to generalize record-5
consumers is not such a contradiction. Do not add Run, recovery, a spell system
or M31 collection merely to simplify testing. At M30 closure, hand M31 the same
Journey owners, quiet authority, per-identity actor/accounting state, continuous
RNG, current-value saves and event-disposition seam; collection still requires
its own specification/authorization and connected acceptance.
