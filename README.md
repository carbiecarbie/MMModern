# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Status

**M43 is the latest completed and accepted milestone.**

The engine supports a bounded Clouds quest loop: request a quest, collect an
item, return it for character-held rewards, and save/resume the resulting progress.
Active-party inventory inspection shows resource-driven item descriptions and
supports transfer between active roster owners and contextual equip/remove for
bounded Clouds weapons, armor and accessories, preserving frames on restart.
Original maps, text, portraits and supported objects appear through a standalone
SDL application, with ordinary outdoor objects animating while stationary and
during dialogue. Static ordinary indoor objects use original directional
appearances, placement and wall occlusion; the bounded Nightshadow gravestone
interaction displays its original clue through the existing event flow.

The production `--journey-skeleton` entry connects inventory/equipment,
four-cell navigation, the original map-20 Skeleton encounter and durable
continuation. Players may arrange the party before combat, engage automatically,
fight with Attack/Block through a genuine End, then return to mutable gameplay
on the same owners and save/restart the resulting injuries, XP, items, context
and defeated actor consequence. This is a bounded Journey, not general map-20
exploration or normal original-game startup.

The production `--journey-expedition` entry adds a prepared six-cell map-20
route with grouped Skeleton/Zombie encounters, identity-bound target selection,
automatic joining and Zombie multiattack, Disease and accumulated injury,
equipment and XP consequences. The first connected Clouds vertical slice runs
from that prepared entry through original Bone Whistle collection and return.
WhoWill, discovery, acknowledgment, grant and removal preserve accumulated
consequences. Quiet F9 saves before or after collection restore the same current
state and RNG continuation in another process, allowing further navigation and
item management without replay. The supported route remains six cells; it adds
no general map-20 exploration, Vertigo travel, normal startup or Whistle use/turn-in.

The production `--journey-region` entry explores the connected mainland
containing `(9,11)` on Clouds map 23. Its 19 original actors retain position,
wounds and defeat accounting across Journey saves. The five admitted species
use contact combat and Orc ranged attacks; F fires equipped missile weapons.
Poison, Sleep, Disease, ordinary generated equipment, monster gold and delayed
treasure remain attached to their character, party and world owners. Inventory,
the original sign and saving are available at the admitted quiet boundaries.
Individual Run permits partial-party combat and non-victory disengagement to the
original fixed destination, followed by return/re-engagement with wounded survivors.
Casualties and dormant or ready treasure survive restart. Fresh Regional Journey
also connects explicit Myra request, Phirna collection and Myra exchange across
ordinary mainland travel, with selected well recovery and narrow exploration
use of Myra's delivered antidote. It now connects the original Vertigo entrance,
a bounded forty-nine-cell town route with Slime combat, Ironworks Armor Repair,
original Training and selected Temple Heal/resurrection, plus exit/reset/revisit
behavior. Training consumes earned XP and carried gold for permanent levels, resets active temporary bonuses and
refills the selected member's HP/SP. Each distinct trainee costs one day per
visit, with a separate departure day. Both regions retain consequences through
exact save/restart. Fresh content 14 supports repeated purchase/repair/Training/
Temple visits across bounded same-year service days, with durable merchant wares, shared bank
balances and exact RNG continuation. Already-learned Magic Arrow, First Aid and
Awaken remain usable after progression. Actual generated Ironworks stock supports
bounded ordinary Weapons/Armor purchases with carried-gold payment, unequipped delivery and durable depletion. Full Vertigo
and services beyond bounded purchase/repair/Training/Temple Heal remain unsupported.

The Diagnostic26 entry presents an original outdoor Skeleton, supports its
activation and approach, and stops at terminal same-cell engagement. Diagnostic27
continues that bounded encounter through playable Attack/Block combat with
original MON/ATT appearance, injury, armor breakage, victory/defeat and once-only
XP. A successfully ended victory becomes a saveable completed checkpoint that
can resume in a new process, expose read-only inspection and perform a bounded
true revisit with the Skeleton still defeated. Neither diagnostic enables
normal-start gameplay.

MMModern remains incomplete and experimental. It is not yet a generally playable
replacement for the original games: general combat, general item use/effects and Darkside
gameplay remain unsupported, and travel outside admitted routes is not certified.

See the [technical snapshot](docs/project-status.md),
[completed milestones](docs/project-history.md) and [future direction](docs/roadmap.md).

## Goals

- Reimplement the Xeen engine using legally obtained original resources.
- Preserve original behavior while keeping unsupported boundaries explicit.
- Build maintainable, testable gameplay and rendering with clear state ownership.

## Current capabilities

- Original Clouds resource loading, outdoor/indoor rendering, navigation and collision.
- Resource-derived outdoor monster state, normal sprite and delayed approach at
  one bounded Skeleton checkpoint, plus playable Attack/Block combat with original
  attack sprites and outcomes in Diagnostic27.
- Supported static and ordinary animated outdoor objects and static ordinary
  indoor objects, with persistent removal after interactions; the bounded Vertigo
  route adds its visible animated object, town sky and Slime composition.
- Bounded event execution, teleports, original text, choices, character selection
  and animated NPC dialogue portraits.
- Party/character state, quest items and flags, and deterministic item rewards.
- Active-character condition and current/max HP/SP, four-category inventory
  inspection and character-to-character transfer, with nine slots per category.
- Contextual equip/remove for bounded Clouds weapons, armor and accessories,
  including class, conflict, capacity and curse feedback for modeled rules.
- Local Windows save/resume for ordinary progress and completed Diagnostic27,
  plus Journey v4 restart with transferred ownership, exact combat outcomes,
  continued bounded navigation/item management and live diagnostics.
- A bounded production expedition with up to three simultaneous contacts,
  readable multi-actor MON/ATT combat, original Bone Whistle collection and
  return, and schema-2 separate-process continuation.
- Resource-derived map-23 mainland exploration through `--journey-region`, with
  all 19 original actors, physical combat/Shoot, conditions and monster treasure,
  individual Run/disengagement and survivor re-engagement, automatic sign
  presentation, connected Myra/Phirna quest and exchange, selected well recovery,
  bounded antidote use, learned First Aid/Awaken exploration casting,
  bounded mainland/Vertigo travel, repeated Ironworks Armor Repair across service
  days, bounded Ironworks Weapons/Armor purchase, original Training/permanent
  progression, selected Temple Heal/resurrection and combat Magic Arrow/First Aid/Awaken,
  with exact 9/14 continuation of both regions,
  progression, merchant/bank state and RNG at quiet boundaries.

## Running and controls

Build/dependency setup is documented in [dependencies.md](docs/dependencies.md).
Run from a terminal to see diagnostics; quote paths containing spaces:

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --encounter-26 <game-directory>
mmodern --encounter-27 [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --journey-skeleton [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --journey-expedition [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
```

Use an existing save directory outside the original game installation. Relative
paths resolve against the working directory. Resume uses the loaded file as the
subsequent save target; invalid/incompatible saves fail without starting a new game.

`--encounter-26` opens the bounded World of Xeen Clouds map-20 diagnostic at
`(13,1)` North. Period (`.`) is Wait; the initial Skeleton may be almost completely
hidden by original forest occlusion, becoming identifiable at engagement. The
entire session is unsaveable: F9 is refused, as are ordinary events and inventory.
Engagement or a support stop terminates supported exploration before combat;
Escape/window close exits. This mode accepts no save options or camera overrides.
Ordinary `--render-map` behavior remains unchanged.

`--encounter-27` adds the bounded Attack/Block diagnostic. Preparation starts
without live actors: I opens inventory for normal transfer/equipment operations,
and Enter begins only with inventory closed. In transfer selection/confirmation,
I cancels to Browse; another I closes. N also cancels confirmation. Escape always
exits the session. After approach engagement, Space attacks and B blocks for the
displayed character; enemy and round work continues automatically. Preparation,
approach, combat, incomplete victory, defeat and failure remain unsaveable. After
a successful End, the completed checkpoint permits F9 saving to the configured
target, read-only I inspection and bounded R re-entry at `(13,1)` North. Resume
that checkpoint with `--load-game`; it cannot resume combat or exploration, and R
does not mean Run or general navigation. The optional nonzero 32-bit seed
reproduces diagnostic RNG. Camera overrides and other entry modes cannot be
combined with this entry.
Combat uses the original normal and attack sprites with bounded source-derived
sequences, while the live roster panel retains injuries and terminal XP results.

`--journey-skeleton` starts bounded production gameplay at Clouds map 20
`(13,1)` North with the original party, all 27 actors and one retained Skeleton
combat seed. I permits normal transfer/equipment before combat. Arrows or WASD
and period use the four-cell movement/Wait contract; same-cell engagement
attaches combat automatically, so Enter never begins Journey combat. Space
interacts outside combat and attacks during combat; B blocks. A successful End
retires combat and returns to mutable inventory/navigation on the same owners.
F9 saves only at a presented, quiet Journey boundary with inventory closed and
no pending approach or combat work. R has no action in this Skeleton Journey.

`--journey-expedition` starts the prepared contract-2 Journey at `(0,14)` East.
Movement is bounded to six party cells along `x=0..5,y=14`; original actor work
may form successive, mixed or three-monster contacts. During a ready combat turn,
1/2/3 selects the corresponding named contact row, Space attacks and B blocks.
Selection, joining and enemy/round work use new presented generations, so batched,
held or stale keys cannot attack a replacement identity. Disease, exact current
versus maximum HP/SP, broken armor and XP remain visible. Quiet inventory and F9
work as in the Skeleton Journey. At a quiet `(5,14)` boundary, Space starts the
original Bone Whistle interaction from any facing. F1-F6 chooses an eligible
member; Escape cancels WhoWill and permits a fresh retry. Space/Enter acknowledges
the discovery, grants the party's Whistle and removes the bones. Movement,
inventory, combat controls and F9 remain blocked while the interaction is pending.
Escape after WhoWill exits without acknowledging acquisition. After success,
discovery text remains readable, repeat interaction grants nothing, and a fresh
F9 may save at the presented quiet boundary. Turn West and return to `(0,14)`;
the Journey remains mutable after return and restart. There is no autosave or
healing requirement for the accepted seed-1 route.

`--journey-region` starts a content-14 Regional Journey at Clouds map 23 `(9,11)` West,
minute 480, with the prepared party and all 19 original actors. Movement follows
the resource-derived mainland. Contact opens Attack/Block/Run combat; 1-3 selects a
live contact. R attempts Run for the displayed member and consumes that turn,
even on failure. Escaped members leave the current combat participation; the
remaining members continue fighting. Orcs can fire during movement opportunities.
F initiates an exploration volley from eligible equipped missile users, then charges ten minutes.
Wounds, Poison/Sleep/Disease, broken armor, XP, gold and generated ordinary items
persist. Ready monster treasure waits while an actor remains in the selected view;
when collection becomes eligible, acknowledge every reward page before continuing.
Inventory shows condition severities, HP/SP, derived Speed/AC and purse state.

Non-victory disengagement relocates to `(10,12)` with facing unchanged. It can
leave abandoned casualties; the destination is not guaranteed safe and may
immediately start another encounter. Surviving enemies keep their wounds and
identities for return/re-engagement. A direct Run exit forfeits undelivered monster
gold and retains stored items dormant until later Orc treasure reactivates them.
An exit caused by later attrition after escape preserves ready treasure. Escaped
members receive no subsequent XP in that encounter and rejoin at retirement;
no injuries or conditions are healed.

Space refuses unsupported interactions without executing their scripts. Facing
North at `(5,9)` automatically displays the original sign; it can also be
requested manually. It requires no acknowledgment, and ordinary eligible controls
remain available while its text is visible. Inventory/equipment and F9 require
presented quiet boundaries.

At Myra `(9,11)` West, Space explicitly requests the original quest. Travel to
Phirna `(8,2)` and use Space from any facing, answer Yes, and acknowledge the
collection. Return by ordinary mainland travel, use Space at Myra and acknowledge
the exchange and receipt for up to five one-charge antidotes. Root possession
controls the return branch; another Root can support another exchange. This is
one bounded connected quest, not general questing.

At the selected well `(7,7)`, Space opens WhoWill; F1-F6 chooses an eligible
member and Escape cancels before selection. It adds 25 HP while that member's
current HP is at or below live maximum, including at maximum; repeated use may
work until HP exceeds maximum, when the original refusal text appears. It does
not clear conditions or restore SP. In quiet Regional Journey exploration, open I,
select a supported miscellaneous antidote and press U. Enter confirms spending
one charge before a fresh F1-F6 target choice; Escape before Enter is free,
while Escape at target choice still spends the charge. A chosen target loses
Poison only. The item action then services one ordinary actor opportunity,
which can lead to an encounter. General and combat-time item use remain unavailable.

At a presented quiet contract-7/8/9/10/11/12/13/14 boundary, C opens learned exploration casting.
Choose an eligible caster with F1-F6, browse that character's learned spells with
Up/Down, press Enter to review the cost and Enter again to cast. First Aid heals
one active member selected with a fresh F1-F6; Awaken clears Sleep across the
active party. Each costs one current SP and ten mainland minutes (one minute indoors),
followed by one ordinary actor opportunity in the active region. Escape before
confirmation costs nothing. Escape at
First Aid's target prompt refunds the SP, but the time and actor obligation still
settle. Learned unsupported spells such as Light remain visible but unusable.
Exploration casting is unavailable during combat, events, open inventory and
other pending work; C cannot queue a cast for later.

In content-10/11/12/13/14 combat, C opens the **acting member's** learned book. Up/Down
select a spell and Enter reviews/confirms it. Magic Arrow costs 2 SP and targets
one admitted same-cell contact: select with 1-3, Enter to review, then a fresh
Enter to commit. First Aid and Awaken cost 1 SP each; First Aid uses F1-F6 for
a remaining participant after confirmation, while Awaken affects the active
party. Escape before commitment is free; Escape at First Aid target choice
refunds SP but consumes the combat action. A paid effect, failure or no-op also
uses the action, with no extra cast-specific time. Acknowledge the result with
a fresh Enter/Space/Escape before combat continues. Light remains unusable.
Magic Arrow has bounded projectile/result feedback and is unavailable in
exploration or against distant/off-contact enemies. Older content, including
8/9 saves, retains combat C refusal.

Save files include living actor wounds, casualties, conditions and dormant/ready
treasure, without storing combat, projectiles or UI work. Loaded legacy Journeys
retain their original rules and controls, including their existing support stops;
Run is available in contract-5/6/7/8/9/10/11/12/13/14 mainland combat.

In contracts 8/9/10/11/12/13/14, approach mainland `(10,13)` facing North, press Space and answer
Yes to enter Vertigo at `(15,0)`. The admitted town route is `x=15,y=0..4`,
`x=16,y=1..4`, and `x=8..14,y=4` in contents 9/10/11/12/13/14; legacy 8/8 stops at `(13,4)`.
Contents 12/13/14 add the original Training approach described below;
content 14 extends the northbound corridor to the Temple at `(15,28)`.
Resolve the entrance Slime through normal combat; face West at `(13,4)` to see the automatic Ironworks outside-door label.
Return to `(15,0)`, face South and use Space. No stays in town after the original
flag prelude; Yes returns to mainland `(10,12)` South and runs the original
monster reset while game flag 9 is clear. A later entry then has a new entrance
Slime. F9 at a quiet boundary and a fresh `--load-game` preserve the current
state on either side; loading never repeats that reset. Movement/Wait cost one
minute indoors; Shoot, Run, cells outside the bounded route and other services
remain unavailable.

In content-9/10/11/12/13/14 Regional Journey, continue west five cells from `(13,4)` to
`(8,4)` and press Space to enter Ironworks Armor Repair. Choose an owner with
F1-F6, Enter to browse armor, 1-9 to select a slot and Enter to quote/confirm;
N or Escape cancels a quote. Supported Armor IDs 1..13, materials 0 and 38,
use original repair prices. Escape backs out to the lobby and then departs.
Every admitted visit costs one day on departure, even without a repair:
fresh content 14 and legacy contents 11/12/13 permit entry on days 8..98 in year 610 and departure to
days 9..99. Entry on day 99 refuses; daytime play and quiet saving still work.
Departures to days 11,21,...,91 replace generated merchant wares and apply bank
interest without healing or replaying skipped actor work. Legacy contents 9/10
permit day 8 to 9 and day 9 to 10 only; a new day-10 visit still refuses. F9 is
available after departure and return to quiet exploration. Generated wares and
shared bank balances persist, with Buy admitted for contents 13/14. Sell,
Identify and bank menus remain unavailable; numeric side-1 stock does not enable Darkside gameplay.

In fresh content 14, from `(15,4)` face North, move three cells, turn left,
move five, turn right and move four to original Training at `(10,11)`.
Acknowledge the original label on the approach, then press Space from any facing.
F1-F6 selects an active member; Enter opens a one-level quote, a fresh Enter
confirms, and Enter acknowledges the result/refusal. Escape cancels a quote
or requests departure from the menu. Eligibility uses permanent level and stored
XP, with Vertigo's level-10 cap; payment uses carried gold. Successful Training
resets active temporary bonuses and assigns only the selected member's maximum
HP/SP, preserving conditions and other members' current HP/SP. The first
successful level for each distinct member in a visit costs one day; further
levels for that same member add no member-day. Departure always costs its own
day, including refusal-only visits. F9 remains unavailable until settlement
returns to quiet presented exploration.

In fresh content 14, Space at Ironworks opens the shared service lobby. B opens
Buy and R/Enter opens Armor Repair. In Buy, Left/Right selects a category,
F1-F6 selects the recipient and 1-9 selects an actual physical stock row.
Enter quotes; a fresh Enter/Yes confirms and Escape/No cancels. Supported offers
are plain Weapons 1..33 and Armor 1..13 with material/state/frame zero; other
stock remains visible with truthful unsupported feedback. Tail capacity is
separate from later equipability. Success atomically pays carried gold, delivers
unequipped and depletes stock. Enter/Escape acknowledges the result; browser
Escape returns to the lobby, and lobby Escape requests the one-day departure.
Repeated purchases and Repair share that departure. F9 remains blocked until
settlement returns to a presented Quiet frame. Smith and Training illustrations
are static; broader terrain/wall-item/indoor scenery animation remains outside
scope, while admitted object, portrait and combat animations remain supported.

In fresh content 14, from `(15,4)` face North and move 24 cells to the Temple
at logical `(28,15,28)`, crossing the northern tile seam and original outside-door
label at `(15,21)`. Space enters from any facing with a surviving party. F1-F6
selects a member, including one who is Unconscious or Dead; Enter quotes Heal,
a fresh Enter confirms and Enter acknowledges the result/refusal. Escape cancels
the quote or unpublished Heal preparation; menu Escape departs. Original prices
use level, missing HP and supported conditions, paid from carried gold. Heal
resets only the selected member's temporary bonuses, assigns HP before clearing
supported conditions and preserves SP and equipment. Zero-price quotes are
no-ops; Disease can leave HP below the final healthy maximum.

Any paid Heal makes the visit owe one two-day departure, regardless of how many
members are healed; unpaid visits owe one day. Paid departure replaces all
merchant stock and applies bank interest once with exact RNG continuation.
Paid visits can depart from days 8..97 to 10..99; day-98 entry permits only the
one-day refusal/cancellation visit. Quiet F9 and a fresh-process restart preserve
exact recovery, purse, date, stock and RNG without replay. The static Temple
illustration is fully obscured by its opaque text panel; presentation work is
deferred with broader art/animation improvements. Audio and animated service
characters are outside this boundary.

General recovery/Rest, other quests and travel outside the admitted mainland
and bounded Vertigo route are unavailable.
Defeat and unsupported time processing close further gameplay/save admission.
Restart a quiet save with `--load-game`. The closed scope and acceptance contract
are in [M35](docs/milestone-35-plan.md). Learned casting and its boundary are
in [M36](docs/milestone-36-plan.md); the town route and regional continuation are
in [M37](docs/milestone-37-plan.md). Armor Repair is bounded by
[M38](docs/milestone-38-plan.md); combat casting is bounded by
[M39](docs/milestone-39-plan.md); service-day continuation is bounded by
[M40](docs/milestone-40-plan.md); Training/progression is bounded by
[M41](docs/milestone-41-plan.md); equipment purchase is bounded by
[M42](docs/milestone-42-plan.md); Temple Heal/resurrection is bounded by
[M43](docs/milestone-43-plan.md).

| Key | Action |
| --- | --- |
| W/Up, S/Down | Move forward/backward; browse physical slots in inventory |
| A/Left, D/Right | Turn left/right; browse categories in inventory |
| Space, Enter | Interact (Space) or advance/acknowledge text; Enter confirms transfer, antidote use, a learned cast, purchase, Temple Heal or one quoted Training level in its current phase |
| Y / N | Answer Yes/No; N cancels a transfer confirmation |
| F1-F6 | Select inventory owner or transfer recipient, an eligible member during WhoWill, an antidote target, a learned caster/First Aid target, the Training member, the Temple recipient, or the Smith recipient |
| 1-9 | Select a physical inventory or Buy stock slot while browsing; 1-3 select a displayed target during a ready expedition or regional combat turn |
| F | Shoot in contract-4/5/6/7/8/9/10/11/12/13/14 mainland exploration; unavailable during contact combat |
| C | Open learned exploration casting at a presented quiet contract-7/8/9/10/11/12/13/14 boundary, or the acting member's book during a presented content-10/11/12/13/14 combat turn |
| U | Use a selected eligible antidote from inventory in quiet contract-6/7/8/9/10/11/12/13/14 exploration; confirm with Enter, then choose a target with a fresh F1-F6 |
| T | Begin transfer of the selected occupied slot |
| E | Equip or remove the explicitly selected occupied weapon, armor or accessory |
| . | Wait during the bounded encounter diagnostics and Journey; no action in ordinary gameplay |
| F9 | Save an eligible idle ordinary session, completed Diagnostic27 or quiet presented Journey; refused while blocking work/UI is active |
| I | Open inventory while idle; in completed Diagnostic27 open read-only inspection; close while browsing and print live diagnostics on opening |
| R | Run for the displayed member in contract-5/6/7/8/9/10/11/12/13/14 mainland combat; revisit the completed Diagnostic27 checkpoint in that separate mode |
| Escape | Exit either diagnostic session; otherwise back/cancel transfer or close inventory, cancel WhoWill or antidote selection, back out of precommit casting for free or refund First Aid at target choice while settlement remains owed, acknowledge NPC/reward pages, cancel a Training/Temple quote or unpublished Temple preparation/request service departure, or exit |

Movement and ordinary interaction are blocked while a response is required;
repeated keydown events are ignored. NPC dialogue and reward pages accept
Space/Enter/Escape, including final acknowledgment with Escape.
Inventory navigation applies while browsing; during transfer selection/confirmation,
Escape returns to browsing before changing category or slot. Each equipment
attempt consumes its selection; select the slot again before another E action.
Only the bounded contract-6/7/8/9/10/11/12/13/14 antidote has inventory use; general item effects are unavailable.

F9 refuses during a response-requiring interaction, open inventory/inspection,
pending Journey approach, combat/End/retirement, unresolved frame handoff or an unsafe session,
without advancing work or scheduling a later save. Close or finish the blocking
work, then issue a new F9. Without a configured path, it writes nothing. Save
results appear in the console and window title. Existing supported valid MMModern
saves can be replaced; there is no autosave, save-on-exit or in-session load.
Ordinary eligible saves write v2, completed Diagnostic27 writes v3, and Journey
saves write v4. Fresh Regional Journey saves use schema 9/content 14 and preserve
all 19 mainland actors, optional retained Vertigo actors and explicit reset
results, purchased items/depleted stock, repaired items, progressed levels/XP,
exact reset/current HP/SP and Temple recovery, individual service dates, merchant wares/shared bank balances and
world RNG continuation, living wounds, casualties, conditions,
purse/treasure, flags, quest/item consequences, and exact learned books and poison inputs for all thirty owners.
Continuation does not replay purchases, Events, Training, Heal/resurrection, refill, payment or time, reset
actors or relearn spells. Legacy
7/7 and earlier Journeys retain their original domains and controls; loading
does not upgrade them or grant Vertigo entry. Legacy 8/8 retains its shorter
town route and does not gain Armor Repair. Legacy 8/9 retains M38 behavior
and does not gain combat casting. Legacy 8/10 retains M39 combat casting;
both 8/9 and 8/10 keep their day-10 service limit without economy state or upgrade.
Legacy 9/11 retains M40's route and service behavior without Training or an
expanded route. Legacy 9/12 retains Training and complete-generation stock
semantics without Buy or purchase depletion. Legacy 9/13 retains its twenty-eight-cell
route and Buy/repair/Training services without Temple recovery or two-day departure.
The reader accepts supported v1/v2/v3/v4 in their
distinct domains. `--load-game` selects Journey directly from v4 and restores fresh owners
without replaying fresh Journey initialization, approach, combat, objective
grant/removal or item actions. Existing expedition saves remain readable
and can collect the Whistle through a fresh explicit interaction.
Saves require matching game archives and are not compatible with original Xeen
or ScummVM saves. See the
[persistence model](docs/project-status.md#persistence-model) for details.

## Requirements and original game data

A legally obtained original installation is required; no commercial game data is
included or modified. Current original-data validation uses World of Xeen resources
for Clouds gameplay. Reading Clouds visual metadata from `DARK.CC` does not enable
Darkside gameplay.

The validated development setup is Windows x86-64 with MSYS2 UCRT64, CMake, SDL2,
zlib and separate ScummVM source/build trees. MMModern reuses selected ScummVM
Xeen and support components; it does not instantiate ScummVM's engine or use its
SDL backend. The exact pin and configuration live in
[dependencies.md](docs/dependencies.md).

## Documentation

- [Milestone 43 plan](docs/milestone-43-plan.md): closed bounded Temple Heal/resurrection,
  forty-nine-cell route, two-day departure, exact 9/14 continuation and acceptance.
- [Milestone 42 plan](docs/milestone-42-plan.md): closed bounded equipment purchase,
  depleted-stock admission, exact 9/13 continuation and acceptance.
- [Milestone 41 plan](docs/milestone-41-plan.md): closed bounded Training,
  progression, individual service days, exact 9/12 continuation and acceptance.
- [Milestone 40 plan](docs/milestone-40-plan.md): closed bounded service days,
  merchant/bank state, exact 9/11 continuation and acceptance.
- [Milestone 39 plan](docs/milestone-39-plan.md): closed combat casting rules,
  authority, content-10 compatibility and acceptance.
- [Milestone 38 plan](docs/milestone-38-plan.md): closed Ironworks Armor Repair,
  bounded departure and 8/9 continuation contract.
- [Milestone 37 plan](docs/milestone-37-plan.md): closed bounded Vertigo travel,
  retained regional state, 8/8 continuation and acceptance contract.
- [Milestone 35 plan](docs/milestone-35-plan.md): closed connected quest,
  selected well, antidote use, 6/6 continuation and acceptance contract.
- [Milestone 36 plan](docs/milestone-36-plan.md): closed learned casting,
  7/7 continuation and acceptance contract.
- [Project status](docs/project-status.md): current stable technical capabilities,
  ownership, persistence and boundaries.
- [Project history](docs/project-history.md): concise completed milestones and plan links.
- [Roadmap](docs/roadmap.md): future direction and planning review cadence.
- [Milestone 34 plan](docs/milestone-34-plan.md): closed Run, partial-party
  disengagement, survivor/treasure continuation and acceptance contract.
- [Milestone 33 plan](docs/milestone-33-plan.md): closed mainland combat, Shoot,
  conditions/rewards, schema/content 4/4 and acceptance contract.
- [Milestone 32 plan](docs/milestone-32-plan.md): closed regional navigation,
  actor/resource authority, schema-3 persistence and acceptance contract.
- [Milestone 31 plan](docs/milestone-31-plan.md): closed connected collection,
  event authority, publication/failure, restart and acceptance contract.
- [Milestone 29 plan](docs/milestone-29-plan.md): closed mutable Journey,
  encounter-continuity, v4 persistence and acceptance contract.
- [Milestone 30 plan](docs/milestone-30-plan.md): closed grouped expedition,
  Disease, presentation, schema-2 continuation and acceptance contract.
- [Milestone 28 plan](docs/milestone-28-plan.md): closed completed-encounter
  authority, persistence, restoration, revisit and acceptance contract.
- [Milestone 27 plan](docs/milestone-27-plan.md): closed bounded combat rules,
  ownership, original appearance and acceptance results.
- [Milestone 26 plan](docs/milestone-26-plan.md): closed actor/approach, timing,
  engagement and unsaveable-session contracts, with acceptance results.
- [Milestone 25 plan](docs/milestone-25-plan.md): closed bounded equipment
  contracts, architectural decisions and acceptance results.
- [Milestone 24 plan](docs/milestone-24-plan.md): closed item catalog, inspection
  and transfer contracts.
- [Dependencies](docs/dependencies.md): supported toolchain and ScummVM setup.
- [Agent instructions](AGENTS.md): development, documentation and Git rules.

## Disclaimer and license

MMModern is unofficial and is not affiliated with or endorsed by Ubisoft,
New World Computing or the ScummVM project. Might and Magic names and assets
belong to their respective copyright holders.

MMModern is distributed under the GNU General Public License version 3 or, at
your option, any later version (GPL-3.0-or-later). Reused ScummVM code is copyright
its respective contributors and licensed under GPLv3 or later.
