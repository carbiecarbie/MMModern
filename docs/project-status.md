# MMModern - Project Status

This describes what can be played and done now. **M72 is the latest completed
and accepted milestone** ([plan](milestone-72-plan.md)): every surface works
as in the original (lava burns, sky and clouds drop the party to the land
below, space kills, the desert costs time, and Rest runs anywhere), pressing
Space passes time and lets monsters act, and the Vertigo Tavern and Temple
offer Drink, Tip, Rumors, Donation and Uncurse; saves are now v14. M71
([plan](milestone-71-plan.md)) made shops, Event treasure,
display cases and monster drops generate items with the original draws, so
wands and other charged items come with 3 to 10 charges as in DOS. M70
([plan](milestone-70-plan.md)) made every Event operation
the Clouds scripts use runs as in the original, other than audio, town services
and the ending, including moved walls and objects, monster spawns, conditions,
rewards, random Events, teleports, the magic mirror and password prompts; most
of them live in areas not open yet. M69 ([plan](milestone-69-plan.md)) made the save keep every
map's state in one generic form (save v13), ready for the areas still to open,
and monsters recover full HP and fall asleep whenever their map is loaded
again, as in the original. M68 ([plan](milestone-68-plan.md)) made every monster attack,
secondary effect and movement kind in the original data works as in DOS,
including flying monsters, though most of them live in areas not open yet;
the death scene and the Shoot refusal were corrected (release v0.1.3). M67
([plan](milestone-67-plan.md)) made equipped metal,
elemental and enchanted items, rings and amulets count in combat, Shoot and
Rest as in the original (release v0.1.2). M66 ([plan](milestone-66-plan.md))
cut the complete test suite from 29 minutes to about 3, and M65
([plan](milestone-65-plan.md)) keeps a census of every mechanic the original
data uses, with its support status, in [the coverage report](coverage.md). M64 ([plan](milestone-64-plan.md)) fixed the last
play-through findings in Vertigo and published the first public releases,
[v0.1.0 and v0.1.1](https://github.com/carbiecarbie/MMModern/releases), a
Windows zip whose launcher finds the GOG installation. M63
([plan](milestone-63-plan.md)) put the game on the original clock: the party
walks about nine steps per second as in DOS, and NPC speech, shopkeepers,
treasure, sparkles, Shoot, the dream, the title and party death follow the
original sequences and timing. M62
([plan](milestone-62-plan.md)) gave Event, NPC, WhoWill, combat-panel and
treasure screens the original presentation and animated the six shopkeepers. M61
([plan](milestone-61-plan.md)) made the Ironworks buy,
sell, identify and fix every item category, and charged items can be used
outside combat. M60 ([plan](milestone-60-plan.md)) added the original Cast dialog,
Guild spell purchases and Spell Info, and eight new spells (Wizard Eye,
Shrapmetal, Energy Blast, Jump, Cure Wounds, Flying Fist, Revitalize, Pain).
M59 ([plan](milestone-59-plan.md)) made display cases, jail, the well, city
searches, Vern's guild membership and Guild entry work. M58 ([plan](milestone-58-plan.md)) added teachers, skills and Cartography for
the original automap and Map. M57 added shared Event rewards and object changes;
M56 added Tavern food and Bank transfers.
The original title, save slots, main screen, character dialogs, Rest and combat
remain available. Direction is in the [roadmap](roadmap.md); completed work is in
[project history](project-history.md). Build and dependency setup is in
[dependencies](dependencies.md).

MMModern aims to be a faithful reimplementation of the original games; any
approved deviation is noted where it applies. It is not yet a general
replacement for them. A **new game** of Might and Magic IV: Clouds of Xeen,
read from the World of Xeen two-CD talkie edition, starts as in the original, and the
playable area is the whole town of Vertigo and the connected mainland of
map 23. Darkside gameplay is not supported.

## What can be played

Launch `mmodern <game-dir>` (see [Entry modes](#entry-modes)) to reach the
original title screen. Escape shows or hides the menu; Credits shows the four
original pages, and Other Options lists the intros (not supported yet).
**New** asks for a save slot (confirming before replacing an occupied one), a
name and the difficulty: Adventurer (weapons hit harder) or Warrior. As in the
original, the six level-1 Clouds characters then stand in Vertigo at `(18,4)`
facing West at 08:00 on day 1 of year 610, with 800 gold, 10 gems, 90 food and
an empty bank. **Load** continues a saved game from the same slot list.

The **prepared Journey** (`--journey-region`) remains as a test mode: the same
party with prepared levels and XP at map 23 `(9,11)` on day 8, in Adventurer;
the automated scenarios use it.

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
- **Magic** uses the characters' learned books. Awaken, First Aid, Cure Wounds,
  Revitalize, Magic Arrow, Flying Fist, Energy Blast, Shrapmetal and Pain work
  in exploration and combat; Wizard Eye and Jump in exploration. Missiles fly
  forward to the first reachable monsters, with the original magic resistance
  and saving throws. Wizard Eye shows the surroundings on the automap without
  marking them explored, until the next temporary-effect reset (Rest, dawn).
  Jump moves two cells forward; destinations on space, lava, sky, cloud or
  desert, or outside the walkable area, say "not supported yet" for now.
  Cast uses the original dialog: C casts, N opens the ten-row New list, and
  Escape backs out. F1-F6 or portraits switch caster in exploration; combat
  keeps the acting caster. New's saved selection is shared with Quick Fight.
  Other spells remain selectable and say "not supported yet" without spending
  SP, gems, time, a combat turn or spell-caused random draws, including Quick
  Fight. Supported exploration casts cost one minute indoors and ten outdoors.
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
Space to leave. Leaving runs the town's original exit script, which respawns
and heals the city monsters, and every map load rebuilds its monsters at full
HP and asleep, as the original does.

- **Monsters.** All Doom Bugs, Slimes and Breeder Slimes are active and fight
  under the original combat rules. Shoot works indoors, and so do enemy
  ranged attacks where a monster has them.
- **Grates and doors.** Bash (B) breaks walls and grates; Space at a locked
  grate or door asks who will pick the lock, with the original trap, Thievery
  roll and XP. Opened barriers stay open across saves.
- **Events.** Signs, NPC conversations and other supported Events work
  anywhere in town, with original text, directions and proof-dependent branches.
  The magic mirror at `(14,10)` facing West asks for a destination name and
  teleports there; names whose map is not open yet refuse without changing
  anything, and the original "I lost it" answer gives the Xeen Slayer Sword.
- **Display cases and jail.** A selected member with positive Thievery takes
  generated treasure without a lockpick roll or XP. A member with zero Thievery
  sends the active party to jail at `(27,11)` with Convicted Thief and a one-year penalty.
  Day/time stay unchanged, temporary bonuses reset, and the party walks out
  normally. Consumed cases give no further treasure.
- **Well** `(14,17)`: before Joe's proof, only the selected member takes
  5 poison-type damage, subject to resistance, without a message or the Poison
  condition. After proof, the selected member is healed to the normal maximum
  with "You feel refreshed"; above-maximum HP is preserved with alternate text.
- **Quests and treasure.** Joe's proof box and Gunther's reward work: all
  members gain XP, the party receives gold, gems and an award once, and Joe's
  shop closes. Searching crates reveals the open crate and gives generated
  items or gold through the same generation and random draws as merchants;
  repeat searches find nothing. Each tree pays 1-10 gold once per cycle. With
  full packs the remaining items are lost but gold and gems still arrive; when
  only one category is full, the new item replaces that category's last item on
  the first character, as in the original.
- **City searches.** Monster crates open and trigger normal combat using their
  original actor slot. Item/empty crates open; rubbish and beds keep their
  appearance. Searches use the original member selection and item delivery;
  repeat searches say "Nothing here."
- **Membership.** Vern `(16,11)` facing East enrolls the selected member for
  25 carried gold. Cancellation or insufficient gold changes nothing; repeat
  payments charge again, as in the original script.
- **Teachers and skills.** Mylo `(8,16)` teaches Cartography for 100 gold and
  Rialdo `(25,26)` teaches Pathfinder for 2,000 (his offer says 2,500), to the
  member chosen with F1-F6 or a portrait. As in the original scripts, a repeat
  lesson charges again. The character sheet lists the current skills.
- **Mapping.** While a member knows Cartography, the cells the party steps on
  are recorded. `=` toggles the 7x7 automap; M or the Map button opens the
  original 16x16 Map with the map name, coordinates and facing. Explored cells
  are kept across travel, services, combat and saves.
Services use the original location screens: the town art stays visible with
the original text panel, and buttons work by mouse or key.

- **Ironworks** (Space at `(8,4)`): *Buy*, *Sell*, *Identify* and *Fix* for
  weapons, armor, accessories and miscellaneous items at the original prices
  (Merchant lets a member sell at full value). Broken items can still be
  identified and sold, and Fix repairs them. Stock is generated and depleted as
  in the original.
- **Using items.** On the character sheet, charged items cast their effect when
  it is one of the supported spells or Myra's antidote, without spending time;
  each use costs a charge, even if it fails or the target prompt is cancelled,
  and the item vanishes at zero. Other powers say "not supported yet" without
  spending a charge. Discard asks for confirmation.
- **Training** (Space at `(10,11)`): permanent levels from earned XP and gold,
  with the Vertigo level cap; it resets temporary bonuses and refills the
  trained member's HP/SP.
- **Temple** (Space at `(15,28)`): Heal and resurrection at original prices for a
  selected member, including Unconscious or Dead ones. Conditions are cleared
  and HP restored; SP and equipment are not touched. Donation (D) charges per
  gift and blesses the party when the count matches the day; Uncurse (U)
  clears the Cursed condition and the curse of carried items. Each paid Heal
  or Uncurse adds a day to the stay, plus one on leaving.
- **Tavern** (Space at `(24,5)`): Food (F) offers a refill to 15 food per
  active member for a flat 10 carried gold. Full or overfull packs cost nothing
  and are not reduced; insufficient gold gives the original refusal. No/Escape
  cancels, and the selected member's condition does not prevent buying food.
  Drink (D) costs 1 gold and may make the member Drunk; Tip (T) needs a drink
  bought this visit and shows the next of the town's tips; Rumors (R) is free
  and changes daily. Sign In says "not supported yet" until the Inn exists.
- **Bank** (Space at `(26,17)`): D/W selects deposit/withdrawal, then O/E
  selects gold/gems. Enter up to ten digits, edit with Backspace/Delete and
  confirm with Enter. Zero, empty input and cancellation transfer nothing;
  an exact balance is allowed, excess is refused without a partial transfer.
  Escape backs out one level. Mouse buttons perform the same actions.
- **Guild** (Space at `(20,13)`): F1-F6 or portraits select the current member.
  Non-members see "You have to be a member to shop here"; members can Buy Spells
  or inspect Spell Info from Vertigo's original 18-spell catalogue, filtered by
  their class book. Prices are 100 times fixed SP cost, or 500 times the absolute
  level-scaled cost, doubled for Paladin, Archer and Ranger. Purchases debit
  carried gold and teach only the selected member, including unsupported spells.
  The original dialogs support mouse and keyboard; departure costs one day.
- **Time.** Each service visit costs a departure day (two after a paid Temple
  Heal). Training adds a day per newly trained member. Merchant stock
  regenerates and bank interest is applied on the original schedule, with
  exact random-number continuation. Food and Bank transactions add no separate
  time; departure still costs a day without a transaction. Scheduled 1% bank
  interest uses the final balances once, after stock generation.

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
  The Tavern refills it after Rest or other consumption.
- **Days and years.** Time runs without limit: conditions change on the
  original eight-hour schedule, each dawn without rest shows the original
  message and can make members Weak, the sky darkens at night, and ages
  follow the year.

### Saving

Tab or the gem opens the original control panel when nothing else is open:
**Save** writes the current game's slot (chosen by New, Load or the last Save
As), **Save As** picks a slot and a new name, **Load** replaces the running
game with a saved one, and **Quit** asks for confirmation. Saving is refused in
combat and on maps that forbid it. Saving and the control panel are unavailable
throughout service visits, including confirmation and amount entry, until the
party returns to exploration. Efx, Music, Speech, Text and Mr Wizard say
"not supported yet". Slots live under Local AppData (`MMModern/Saves/`); a
damaged or foreign slot file is shown as unavailable and never overwritten.
Loading continues exactly: positions, wounds, casualties,
conditions, food, treasure, items, levels, purchases, merchant and bank state, quest
progress and the random sequence. Nothing is replayed: no events, rewards,
time or combat.

## Controls

| Key | Action |
| --- | --- |
| Up / Down | Move forward/back outside combat |
| Left / Right | Turn, also during combat |
| Ctrl+Left / Ctrl+Right, keypad 4 / 6 | Strafe sideways with the facing unchanged; ignored in combat |
| Space | Interact outside combat; acknowledge text |
| A | Attack in combat |
| Enter | Confirm (transfer, cast, purchase, Training, Temple); acknowledge |
| Y / N | Yes / No |
| F in the Tavern | Buy food |
| D / W in the Bank, then O / E | Deposit / Withdraw, then Gold / Gems |
| Digits, Backspace/Delete, Enter in Bank amount entry | Enter up to ten digits, edit, confirm; Escape cancels |
| F1-F6 | Open the character sheet; in dialogs choose a member (item recipient, WhoWill, spell or healing target) |
| 1-9 | Select an item or stock row; 1-3 pick a combat target |
| . | Wait |
| S | Shoot in exploration |
| B | Bash in exploration; Block in combat; Buy in the Ironworks lobby |
| R | Rest in exploration; Run in mainland combat; Armor Repair in the Ironworks lobby |
| C | Open learned spells (exploration or the acting member's book in combat) |
| I | Info (date, time and active effects); in the character sheet, open Items |
| Q | Quick Reference (party summary), also from the character sheet |
| E in the character sheet | Exchange: choose a member with F1-F6 or a portrait (refused in combat) |
| F / O in combat | Quick Fight (the acting member's configured action) / Quick Fight Options |
| W/A/C/M in Items | Weapons, Armor, Accessories, Misc |
| E / R / U in Items | Equip / Remove / Use; select a row, then F1-F6 to move it to another member |
| M | Map (original 16x16 view) |
| = | Toggle the automap |
| Tab | Control panel (Save, Save As, Load, Quit) |
| F9 | Save to the developer `--save-file` target |
| Escape | Back, cancel or acknowledge; quit when nothing is open |

**Mouse.** On the main screen, left clicks work as in the original: the
action and movement buttons, the combat buttons and targets 1-3, and the 3D
view (Interact in exploration, Attack in combat). Buttons whose action is not
implemented yet (Dismiss and View Quests) show "not supported yet"; the Map
button opens the Map; the
lower corner buttons strafe and the gem opens the control panel; portraits
open the original character sheet, whose Items dialog is also clickable.
Buttons briefly show their original pressed frame. The right button does
nothing. Cast, New and Guild spell dialogs support mouse and keyboard. Clicks
follow the same buffering as keys.

**Presentation.** Terrain, sky and water alternate, wall items and torches
animate, and monsters animate from their own data, on every Clouds map
(`--render-map` shows any map's monsters and wall art). In combat, missiles
fly together and damage, death and rewards apply when they arrive, hits
show the original splats, portraits show damage and healing effects, the strip
shrinks when members run, and the acting member is highlighted. Skill, award
and XP grants sparkle on their recipients' portraits (except Convicted Thief).
The original mouse cursor is used.

**Character sheet and items.** The original character sheet and Items dialog
replace the project's inventory: stats with their popups, equip, remove,
transfer and antidote use. In combat they open for viewing only; equipping
and transferring say "not supported yet", and Use and Exchange give the
original refusals. Outside combat, Exchange swaps two members and Quick
Reference opens the party summary. Discard, Quest and Awards are not
supported yet.

In exploration and combat, keys pressed while the
game is still busy (redraws, animations, enemy turns) are buffered and applied
in order when the game is ready. Every distinct movement or turn tap counts;
a held key repeats after half a second at the original pace (about nine steps
per second), and releasing it adds no further step, an approved deviation from
DOS, which can add steps after release. The buffer is cleared when the context
changes (combat ends, a panel or dialog opens, map changes). Services, dialogs, inventory and casting accept only fresh presses.

## Entry modes

All modes need the path to a World of Xeen CD installation: at a GOG root,
MMModern reads the CD archives inside the disc images and the installed
`WORLD/XEEN.DAT` for the DOS interface text. A copy of the CD's `GAME` folder
also works, with `--ui-data <path-to-XEEN.DAT>`. The floppy archives are not
used, and saves made with them are rejected. Original data is never copied or
modified, and saves must not be written inside any of these directories.

| Command | Purpose |
| --- | --- |
| `mmodern <game-dir>` | Open the original title screen (New, Load, Credits, Other Options). |
| `mmodern --new-game <game-dir> [--difficulty adventurer\|warrior] [--save-file <path>]` | Developer: start a new game directly (default Adventurer). Without `--save-file`, F9 writes nothing. |
| `mmodern --journey-region [--combat-seed <u32>] <game-dir> [--save-file <path>]` | Prepared Journey test mode. The optional nonzero seed fixes combat randomness. |
| `mmodern --load-game <game-dir> <save>` | Developer: continue a save file; it becomes the F9 target. |
| `mmodern --render-map <game-dir> [<map> <x> <y> <dir>]` | Explore any Clouds map the loader can render. Unsaveable: F9 refuses. Not a supported gameplay mode. |
| `mmodern --inspect-map\|--inspect-party\|--inspect-events <game-dir> ...` | Developer inspection output. |

The earlier `--encounter-26`, `--encounter-27`, `--journey-skeleton` and
`--journey-expedition` modes were removed in M44 and print usage.

## Save format

There is **one current save format**: envelope v14, schema 9, content 14. It
keeps one generic state per visited map (monster positions and lifecycle,
removed or moved objects, changed cells, disabled Events, explored cells and
the Tavern tip index),
plus skills, bounded world flags and head-cache overrides with their original
map-reload lifetime. Monster HP and wake state are not saved: every map load
rebuilds them. During 0.x, older formats are not read:
- A save from an older build is rejected before anything is restored, with "This
  save was created by an older MMModern build and is no longer supported."
- A save from a newer or unrecognized build is rejected with a "newer or
  unsupported" message, and the file is left alone.
- F9 may overwrite a file that is a recognizable older MMModern save. Unknown
  or corrupt files, and newer saves, are protected.

Saves are tied to the original archives (checked by fingerprint), are
written atomically, and are not compatible with the original games or ScummVM.
There is no autosave. Any change to the layout or
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
  Heal, Food, Bank transfers, learned spells, antidotes, service days and
  regional movement.
- **Events**: `XeenEventSystem` decodes and runs the original Event scripts;
  supported preludes publish once before terminal town services; unsupported
  dependent effects refuse visibly without publishing a partial prelude.
- **Flows** (`app`): `XeenEventFlow` is the gameplay session coordinator and
  presenter for input, dialogs and inventory. `XeenEncounterFlow` hosts the
  Journey: encounters, combat, Smith, Training, Temple, Tavern, Bank, Guild, casting
  and consequences.
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

- **Start and saves.** The intros, audio and Mr Wizard's help are not available.
- **Areas.** Only the map-23 mainland and Vertigo are playable. Other maps
  appear only through `--render-map`, without save support. Run refuses
  indoors.
- **Missing services.** Inn/recruitment is not available; the Temple handles
  only the modeled conditions.
- **Items and magic.** Item powers backed by unimplemented spells say "not
  supported yet"; cursed-item refusals follow the reference. Most spells
  are missing, and monsters cannot yet be put to sleep, paralyzed or
  controlled; Light, Sleep, Insect Spray, Protection
  from Elements, Sparks, Toxic Cloud and Suppress Poison can be bought but say
  "not supported yet" when cast. Mainland fountains remain
  unsupported. Locked-chest lock/trap/XP mechanics are deferred; Vertigo CD's
  searchable objects use the crate, case, bed and rubbish mechanics.
- **Presentation.** There is no audio: where the CD plays speech, a short
  notice appears. Speech, shop voices, the dream and the death laugh keep their
  original duration silently, and NPC faces move for as long as the CD speech
  would play.
- **Services.** Tavern Sign In shows "not supported yet" until the Inn.
- **Combat items.** Equipping during combat and the combat Use button are not
  supported yet (the original allows both).
- **Skills.** Only Cartography and the skills used by existing rules
  (Thievery, Navigator, Bodybuilder and others affecting HP/SP) have effects.
  Arms Master, Direction Sense, Merchant, Mountaineer, Swimmer, Tracker, Spot
  Secret Doors and Danger Sense can be learned from Events and are saved, but
  their passive effects are not implemented yet.
- **Events.** Clouds Event audio, town-service Events and the ending's
  ConfirmWord answers are not implemented; teleports to maps not open yet refuse.
- **Darkside** gameplay is not supported.

Next steps are in the [roadmap](roadmap.md).
