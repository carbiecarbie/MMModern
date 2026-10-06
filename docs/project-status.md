# MMModern - Project Status

This describes what can be played and done now. **M51 is the latest completed
milestone** ([plan](milestone-51-plan.md)): the party can Rest as in the
original, with food, recovery and monster interruption, and play continues
across days and years. M50 ([plan](milestone-50-plan.md)) made the whole town
of Vertigo playable, with Bash, unlocking and indoor Shoot. M49 ([plan](milestone-49-plan.md)) made monster targeting, damage at missile
impact and turning in combat follow the original. M48
([plan](milestone-48-plan.md)) made scene and combat presentation generic for
all Clouds content, with the original keys and cursor. M47 ([plan](milestone-47-plan.md)) brought the original character
sheet, items dialog and service dialogs. M46
([plan](milestone-46-plan.md)) made the original main screen clickable. M45 ([plan](milestone-45-plan.md)) made keyboard input reliable and
removed integrity failures from on-demand resource loading. M44
([plan](milestone-44-plan.md)) left one save format and one Journey
configuration. Direction is in the
[roadmap](roadmap.md); completed work is in [project history](project-history.md).
Build and dependency setup is in [dependencies](dependencies.md).

MMModern aims to be a faithful reimplementation of the original games; any
approved deviation is noted where it applies. It is not yet a general
replacement for them. The playable
scope is one **prepared Journey** in Might and Magic IV: Clouds of Xeen (read
from a World of Xeen installation): the connected mainland of map 23 and the
whole town of Vertigo. Darkside gameplay is not supported.

## What can be played

Start it with `--journey-region` (see [Entry modes](#entry-modes)). The party is
the prepared six-character Clouds party standing at map 23 `(9,11)` facing West.
It starts at minute 480 of day 8, year 610, with prepared levels, gold and bank
balances rather than the original new-game state.

### Mainland, map 23

- Walk the connected mainland. Terrain, walls, objects and original text come
  from the original resources; outdoor objects animate while you stand still and
  during dialogue.
- All 19 original actors on the mainland are alive in the world. They wake,
  approach and attack on the original rules. The five monster kinds present are
  admitted, and some monsters also attack at range.
- **Combat** starts on contact and is turn-based per party member: Attack, Block,
  Run (outdoors), Shoot (exploration only), and learned spells. Monsters choose their
  targets and number of attacks from their original data, and the party can
  turn left and right during combat. Wounds, Poison, Sleep,
  Disease, broken armor, XP, gold and generated equipment are kept on the
  characters, party and world. Monster treasure appears when the fight is over
  and must be acknowledged page by page.
- **Run** is per member. Remaining members keep fighting. If the whole party
  leaves, it is moved to the original fixed destination `(10,12)` with its
  facing unchanged. Survivors keep their wounds and can be met again.
- **Magic** uses the characters' learned books. In exploration, First Aid and
  Awaken can be cast. In combat, Magic Arrow, First Aid and Awaken work.
  Other learned spells, such as Light, are listed but cannot be cast.
- **Quests and recovery.** Myra's request, the Phirna collection and Myra's
  return exchange run end to end across normal mainland travel. The selected
  well restores HP. The Myra-delivered antidote can be used from the inventory
  in exploration and removes Poison.
- The original sign at `(5,9)` is shown automatically. Events the engine does
  not support yet show a clear "Unsupported event" notice and the game keeps
  running.

### Vertigo

Enter from mainland `(10,13)` facing North with Space and Yes. The whole town
is loaded from the original resources. Face South at the entrance and use
Space to leave; the mainland monsters and the town's actors are reset on exit
under the original rules.

- **Monsters.** All Doom Bugs, Slimes and Breeder Slimes are active and fight
  under the original combat rules. Shoot works indoors, and so do enemy
  ranged attacks where a monster has them.
- **Grates and doors.** Bash (B) breaks walls and grates; Space at a locked
  grate or door asks who will pick the lock, with the original trap, Thievery
  roll and XP. Opened barriers stay open across saves.
- **Events.** Signs, NPC conversations and other supported Events work
  anywhere in town. Treasure (chests), moving objects, fountains, the magic
  mirror and trap teleports show "not supported yet".
Services use the original location screens: the town art stays visible with
the original text panel, and buttons work by mouse or key.

- **Ironworks** (Space at `(8,4)`): *Buy* plain Weapons and Armor from the
  generated stock, paid from carried gold and delivered unequipped, with stock
  depletion; *Armor Repair* at original prices. Stock stays visible even when it
  cannot be bought yet.
- **Training** (Space at `(10,11)`): permanent levels from earned XP and gold,
  with the Vertigo level cap; it resets temporary bonuses and refills the
  trained member's HP/SP.
- **Temple** (Space at `(15,28)`): Heal and resurrection at original prices for a
  selected member, including Unconscious or Dead ones. Conditions are cleared
  and HP restored; SP and equipment are not touched.
- **Time.** Each service visit costs a departure day (two after a paid Temple
  Heal). Training adds a day per newly trained member. Merchant stock
  regenerates and bank interest is applied on the original schedule, with
  exact random-number continuation.

### Rest, food and time

- **Rest** (Rest button or R, outside combat): the party sleeps for eight
  hours, eats one food per member who can recover, and recovers HP and SP;
  temporary bonuses end. The original warning appears when a member may die,
  and the original dream can appear. Without food, time still passes but
  nobody recovers.
- **Interruption.** Nearby monsters approach and attack while the party
  sleeps; the Rest stops without recovery, and members who were not hit stay
  asleep until a completed Rest or Awaken.
- **Food** starts at the original 90 and is shown on the character sheet.
  There is no way to buy more yet.
- **Days and years.** Time runs without limit: conditions change on the
  original eight-hour schedule, each dawn without rest shows the original
  message and can make members Weak, the sky darkens at night, and ages
  follow the year.

### Saving

F9 saves at a quiet moment (no dialogue, combat, inventory or service open).
Restarting with `--load-game` continues exactly: positions, wounds, casualties,
conditions, treasure, items, levels, purchases, merchant and bank state, quest
progress and the random sequence. Nothing is replayed: no events, rewards,
time or combat.

## Controls

| Key | Action |
| --- | --- |
| Up / Down | Move forward/back outside combat |
| Left / Right | Turn, also during combat |
| Space | Interact outside combat; acknowledge text |
| A | Attack in combat |
| Enter | Confirm (transfer, cast, purchase, Training, Temple); acknowledge |
| Y / N | Yes / No |
| F1-F6 | Open the character sheet; in dialogs choose a member (item recipient, WhoWill, spell or healing target) |
| 1-9 | Select an item or stock row; 1-3 pick a combat target |
| . | Wait |
| S | Shoot in exploration |
| B | Bash in exploration; Block in combat; Buy in the Ironworks lobby |
| R | Rest in exploration; Run in mainland combat; Armor Repair in the Ironworks lobby |
| C | Open learned spells (exploration or the acting member's book in combat) |
| I | Info (not supported yet); in the character sheet, open Items |
| W/A/C/M in Items | Weapons, Armor, Accessories, Misc |
| E / R / U in Items | Equip / Remove / Use; select a row, then F1-F6 to move it to another member |
| F9 | Save |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

**Mouse.** On the main screen, left clicks work as in the original: the
action and movement buttons, the combat buttons and targets 1-3, and the 3D
view (Interact in exploration, Attack in combat). Buttons whose action is not
implemented yet (Dismiss, View Quests, Map, Info, Quick Ref, Quick
Fight, the control panel and strafing) show "not supported yet"; portraits
open the original character sheet, whose Items dialog is also clickable.
Buttons briefly show their original pressed frame. The right button does
nothing; casting still uses the keyboard. Clicks
follow the same buffering as keys.

**Presentation.** Terrain, sky and water alternate, wall items and torches
animate, and monsters animate from their own data, on every Clouds map
(`--render-map` shows any map's monsters and wall art). In combat, missiles
fly together and damage, death and rewards apply when they arrive, hits
show the original splats, portraits show damage and
healing effects, the strip shrinks when members run, and the acting member
is highlighted. The original mouse cursor is used.

**Character sheet and items.** The original character sheet and Items dialog
replace the project's inventory: stats with their popups, equip, remove,
transfer and antidote use. In combat they open for viewing only; equipping
and transferring say "not supported yet", and Use and Exchange give the
original refusals. Discard, Quest, Quick Reference, Awards and Exchange are
not supported yet.

In exploration and combat, keys pressed while the
game is still busy (redraws, animations, enemy turns) are buffered, up to five,
and applied in order when the game is ready, as in the original; holding a
movement key walks and adds at most one step after release. The buffer is
cleared when the context changes (combat ends, a panel or dialog opens, map
changes). Services, dialogs, inventory and casting accept only fresh presses.

## Entry modes

All modes need the path to a World of Xeen installation. Original data is never
copied or modified, and saves must not be written inside the installation.

| Command | Purpose |
| --- | --- |
| `mmodern --journey-region [--combat-seed <u32>] <game-dir> [--save-file <path>]` | Play the Regional Journey. The optional nonzero seed fixes combat randomness. Without `--save-file`, F9 writes nothing. |
| `mmodern --load-game <game-dir> <save>` | Continue a save; the loaded file becomes the F9 target. |
| `mmodern --render-map <game-dir> [<map> <x> <y> <dir>]` | Explore any Clouds map the loader can render. Unsaveable: F9 refuses. Not a supported gameplay mode. |
| `mmodern --inspect-map\|--inspect-party\|--inspect-events <game-dir> ...` | Developer inspection output. |
| `mmodern <game-dir>` | Static party screen. |

The earlier `--encounter-26`, `--encounter-27`, `--journey-skeleton` and
`--journey-expedition` modes were removed in M44 and print usage.

## Save format

There is **one current save format**: envelope v6, schema 9, content 14, written
by the Regional Journey. Until a public release, older formats are not read:
- A save from an older build is rejected before anything is restored, with "This
  save was created by an older MMModern build and is no longer supported."
- A save from a newer or unrecognized build is rejected with a "newer or
  unsupported" message, and the file is left alone.
- F9 may overwrite a file that is a recognizable older MMModern save. Unknown
  or corrupt files, and newer saves, are protected.

Saves are tied to the original archives (checked by fingerprint), are
written atomically, and are not compatible with the original games or ScummVM.
There is no autosave and no in-session load. Any change to the layout or
meaning of a save bumps the format version and rejects older saves.

## Architecture overview

Source is under `src/`: `formats/xeen` (original resource and save formats),
`games/xeen` (rules and state), `app` (coordination and presentation),
`platform` (SDL window, save file I/O) and `compat` (the selected ScummVM
components the project links).

Ownership, from state outward:

- **Party and roster** (`XeenParty`, `XeenRoster`): characters, inventory,
  conditions, quest items and flags, purse and the learned books. One owner
  for all thirty roster characters; everything else borrows it.
- **World** (`XeenWorld`, `XeenSessionWorldState`): map and object caches, the
  live actors of both regions, session removals, merchant and bank economy, the
  calendar and the world random state.
- **Rules** (pure, in `games/xeen`): combat (`XeenCombat` and its rules),
  actor approach, monster treasure, equipment and purchase, Training, Temple
  Heal, learned spells, antidotes, service days and regional movement.
- **Events**: `XeenEventSystem` decodes and runs the original Event scripts;
  unsupported opcodes refuse visibly.
- **Flows** (`app`): `XeenEventFlow` is the gameplay session coordinator and
  presenter for input, dialogs and inventory. `XeenEncounterFlow` hosts the
  Journey: encounters, combat, Smith, Training, Temple, casting and consequences.
  `XeenNavigationFlow` handles movement. `Application` wires resources, SDL and
  the entry modes.
- **Save** (`XeenSaveState`, `XeenSaveFormat`, `XeenSaveFile`): captures a value
  snapshot at a quiet boundary and restores it into fresh owners before gameplay
  starts. `XeenRestoreGuard` and `XeenMutation` detect state that changed
  while work was prepared, so stale work cannot publish.
- **Journey content** (`XeenJourneyContent`): an immutable description of the
  single Regional Journey: the map-23 mainland and the whole of Vertigo.

Presentation draws into a 320x200 indexed frame that SDL scales. Presented
frames carry an identity so input from a stale frame is dropped.

## Tests

CTest covers rules and synthetic fixtures without original data, and
original-data integration and process tests that need a legally obtained
installation. Every test is labelled `fast` or `process`: use `ctest -L fast`
while iterating and plain `ctest` before closing a milestone. Three
original-data scenarios (mainland, Vertigo services, Temple) compare their final
saves with digests recorded before M44 and re-save byte-identically after a
reload, guarding against unintended behavior changes. Build and run
instructions are in [dependencies](dependencies.md).

## Known gaps

- **Not a new game.** The party is prepared and injured or leveled only through
  the services above and Rest. There is no original new-game start, and food
  cannot be bought (Tavern) yet.
- **Areas.** Only the map-23 mainland and Vertigo are playable. Other maps
  appear only through `--render-map`, without save support. Run refuses
  indoors.
- **Missing services.** The bank menu, Inn, Tavern and Guild spell purchase do
  not exist; the Temple handles only the modeled conditions.
- **Items and magic.** There is no general item use. Item effects, most spells
  and most monster abilities are missing; Light and other learned spells are
  visible but cannot be cast. Treasure Events (GiveMulti, chests), moving
  objects, fountains, the mirror, trap teleports and Thievery Event checks
  show "not supported yet".
- **Presentation.** Service art is static (no animated shopkeepers). There is
  no audio. Event, casting and treasure screens still use project layouts.
- **Services.** Sell, Identify, buying Accessories/Misc, Fix of non-armor
  items, Temple Donation and Uncurse show "not supported yet".
- **Combat items.** Equipping during combat and the combat Use button are not
  supported yet (the original allows both).
- **Rest details.** Resting on unsupported terrain (lava, sky, cloud, space)
  refuses; none is reachable today. The dream has no audio, so it is shorter
  than the original.
- **Unconfirmed rules.** The condition byte `0xFF` follows ScummVM's `-1`
  sentinel (Weak/Drunk, dawn and stat modifications); the eight-hour
  Poison/Disease branch follows ScummVM, which only draws for members without
  the condition. Neither is confirmed in the DOS original.
- **Darkside** gameplay is not supported.

Next steps are in the [roadmap](roadmap.md).
