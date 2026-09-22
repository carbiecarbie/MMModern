# MMModern

MMModern is an open-source reimplementation of the engine used by
Might and Magic IV: Clouds of Xeen and
Might and Magic V: Darkside of Xeen / World of Xeen.

## Status

**Milestone 34 is the latest completed milestone.**

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
Casualties and dormant or ready treasure survive restart. Recovery, Myra quest
execution and travel beyond this mainland remain excluded.

The Diagnostic26 entry presents an original outdoor Skeleton, supports its
activation and approach, and stops at terminal same-cell engagement. Diagnostic27
continues that bounded encounter through playable Attack/Block combat with
original MON/ATT appearance, injury, armor breakage, victory/defeat and once-only
XP. A successfully ended victory becomes a saveable completed checkpoint that
can resume in a new process, expose read-only inspection and perform a bounded
true revisit with the Skeleton still defeated. Neither diagnostic enables
normal-start gameplay.

MMModern remains incomplete and experimental. It is not yet a generally playable
replacement for the original games: general combat, item use, complete item effects and Darkside
gameplay remain unsupported, and travel between validated checkpoints is not certified.

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
  indoor objects, with persistent removal after interactions. Indoor ordinary
  animation remains unsupported.
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
  presentation, and exact continuation at quiet boundaries.

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

`--journey-region` starts a Regional Journey at Clouds map 23 `(9,11)` West,
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
Save files include living actor wounds, casualties, conditions and dormant/ready
treasure, without storing combat, projectiles or UI work. Loaded legacy Journeys
retain their original rules and controls, including their existing support stops;
Run is available only in contract-5 regional combat.

Recovery, Myra quest execution and travel beyond the mainland are unavailable.
Defeat and unsupported time processing close further gameplay/save admission.
Restart a quiet save with `--load-game`. The closed scope and acceptance contract
are in [M34](docs/milestone-34-plan.md).

| Key | Action |
| --- | --- |
| W/Up, S/Down | Move forward/backward; browse physical slots in inventory |
| A/Left, D/Right | Turn left/right; browse categories in inventory |
| Space, Enter | Interact (Space) or advance/acknowledge text; Enter confirms an armed transfer |
| Y / N | Answer Yes/No; N cancels a transfer confirmation |
| F1-F6 | Select inventory owner or transfer recipient; outside inventory, select an eligible member during WhoWill |
| 1-9 | Select a physical inventory slot while browsing; 1-3 select a displayed target during a ready expedition or regional combat turn |
| F | Shoot in contract-4/5 regional exploration; unavailable during contact combat |
| T | Begin transfer of the selected occupied slot |
| E | Equip or remove the explicitly selected occupied weapon, armor or accessory |
| . | Wait during the bounded encounter diagnostics and Journey; no action in ordinary gameplay |
| F9 | Save an eligible idle ordinary session, completed Diagnostic27 or quiet presented Journey; refused while blocking work/UI is active |
| I | Open inventory while idle; in completed Diagnostic27 open read-only inspection; close while browsing and print live diagnostics on opening |
| R | Run for the displayed member in contract-5 regional combat; revisit the completed Diagnostic27 checkpoint in that separate mode |
| Escape | Exit either diagnostic session; otherwise back/cancel transfer or close inventory, cancel WhoWill, acknowledge NPC/reward pages, or exit |

Movement and ordinary interaction are blocked while a response is required;
repeated keydown events are ignored. NPC dialogue and reward pages accept
Space/Enter/Escape, including final acknowledgment with Escape.
Inventory navigation applies while browsing; during transfer selection/confirmation,
Escape returns to browsing before changing category or slot. Each equipment
attempt consumes its selection; select the slot again before another E action.
Misc item use and general item effects are not provided by this panel.

F9 refuses during a response-requiring interaction, open inventory/inspection,
pending Journey approach, combat/End/retirement, unresolved frame handoff or an unsafe session,
without advancing work or scheduling a later save. Close or finish the blocking
work, then issue a new F9. Without a configured path, it writes nothing. Save
results appear in the console and window title. Existing supported valid MMModern
saves can be replaced; there is no autosave, save-on-exit or in-session load.
Ordinary eligible saves write v2, completed Diagnostic27 writes v3, and Journey
saves write v4. Fresh Regional Journey saves use schema/content 5/5 and preserve
all 19 actor records, living wounds, casualties, conditions, purse and
dormant/ready treasure with exact continuation. Legacy 4/4 and earlier Journey
saves retain their original rules and controls; loading does not upgrade them.
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
