# Development

This page is for people working on MMModern. Build and dependency setup is in
[dependencies](dependencies.md).

## Game data sources

At a GOG root, MMModern reads the cue-located ISO archives and the installed
`WORLD/XEEN.DAT`, never the co-located floppy archives. One complete disc
suffices. A pre-existing CD `GAME` folder (or its parent) is also accepted with
all three archives and an uncompressed English DOS UI module: supply
`--ui-data <path-to-XEEN.DAT>` or an unambiguous `WORLD` sibling. A bare `GAME`
copy has no UI text. Conflicting or ambiguous sources and packed DATs fail clearly.
By default, the installed `WORLD/XEEN.CFG` must sit beside the UI module;
without it startup fails, and MMModern never creates or writes it. For
play-tests, `--xeen-cfg <path>` selects a different read-only CFG for every
entry mode, including the title and its intros and ending. A missing selected
file fails startup instead of falling back to the installed CFG. Loose game
resources still come from the directory containing the installed UI module.
`WORLD/XEEN.EXE` beside it is
read, never run, for the Clouds and Darkside intros and the Clouds ending; if
it is missing or different, they do not play and the game continues.
Reading Darkside metadata does not enable Darkside gameplay.

## Entry modes

Run from a terminal to see diagnostics, and quote paths containing spaces:
`--ui-data <path>` and `--xeen-cfg <path>` can be supplied with every entry mode.
Developer modes use a fixed DOS wall clock of zero seconds. Supply
`--wall-clock <unsigned-32-bit-seconds>` to select another fixed value;
plain title launch uses the host's local civil clock through the original
Borland conversion. This clock is independent of presentation ticks and
Journey randomness.

```text
mmodern <game-directory>
mmodern --new-game <game-directory> [--difficulty adventurer|warrior] [--save-file <path.mmsave>]
mmodern --journey-region [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path.mmsave>]
mmodern --load-game <game-directory> <path.mmsave>
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>]
mmodern --monster-fixture <species> <game-directory> [--record <n>] [--start <x>,<y>,<facing> | --water-edge] [--combat-seed <nonzero-u32>]
mmodern --event-fixture <game-directory> <map> <x> <y> <north|east|south|west> [--combat-seed <nonzero-u32>]
mmodern --inspect-map|--inspect-party|--inspect-events <game-directory> ...
```

- `mmodern <game-directory>` plays the Darkside intro (the full story the first
  time, a shorter sequence on later launches), then opens the original title
  screen (normal play).
- `--new-game` is a developer shortcut that starts directly in Vertigo
  (`--difficulty`, default Adventurer). Without `--save-file`, F9 writes
  nothing.
- `--journey-region` is a test mode with a prepared party at Clouds map 23
  `(9,11)`. `--combat-seed` fixes combat randomness.
- `--load-game` continues a save file and keeps using it for F9 (developer).
- `--render-map` explores a map for inspection. It cannot save.
- `--monster-fixture` starts the prepared Journey on F3 with one placed monster
  replaced by the chosen original `xeen.mon` species, through an in-memory copy
  of the map's monster list. By default it picks the placement nearest the
  Journey entry that has a walkable start cell with the monster 2-3 cells
  straight ahead on a clear line (no tent, tree or other monster between);
  `--record` fixes the placement and
  `--start <x>,<y>,<north|east|south|west>` fixes the start, which must be a
  walkable F3 cell without an automatic Event or a monster spawn (refused with
  the reason otherwise). `--water-edge` instead starts the party facing water
  and moves the placement (or `--record`) to free land 2-3 cells beyond it, so
  a flyer crosses the water while a walker stays on its bank; for example
  `--monster-fixture 1 <game-directory> --water-edge` for a Giant Bat. Walking
  and flying species are both accepted if they pass the normal mainland checks;
  the terminal prints the species, its cell, the start and the facing. Saving
  and the control panel's Save, Save As and Load are disabled. Not in the
  title menu.
- `--event-fixture` starts an isolated Event session on the chosen Clouds map,
  position and facing, using the original loader, interpreter and Journey
  publication owners. The default seed is 7; `--combat-seed` makes another run
  reproducible. Party prerequisites are the prepared Journey's six original
  members, at levels `3,3,3,4,3,3` with XP `1000,2000,1000,1000,2000,1000`,
  full HP/SP, cleared conditions and temporary bonuses, on day 8 of year 610
  at 08:00 in Adventurer mode. Original food, purse, flags, quest counters,
  inventories and skills are loaded normally; merchant stock uses that seed.
  For example, `mmodern --event-fixture "F:/Games/gog/Might and Magic 4-5" 27 10 10 north`.
  The position must resolve to original geometry without immediate monster
  contact. Resource identity, actor binding, ownership and mutation checks still
  run. Unsupported operations refuse through the normal Event flow. Travel to
  an unavailable map discards unpublished changes; already displayed and
  published script rewards or damage remain. Saving, Save As and Load are
  disabled, including direct snapshot capture. This entry is absent from the
  title menu and does not admit maps to ordinary play.
- `--inspect-*` are developer tools.

Developer save files must be outside all original source directories, including
separately supplied disc-image and DAT directories. Relative paths use the
working directory. Saves from older builds are rejected with a clear message
(and may be overwritten by F9), and saves from newer builds are protected.
Floppy-bound saves are rejected before restoration with a data-edition diagnostic.

## Tests

Every CTest entry is labelled `fast` or `process`. `process` tests take 30 seconds
or more or drive child processes. Configuring the tests requires the original
installation.
Tests are independent, so run them in parallel; the few that assert real-clock
behavior or open a native window are marked to run alone.

```text
ctest -L fast -j 8    # iteration: everything except process tests
tools/run-full-ctest.ps1 -BuildDir build-rel  # full suite with local job preference
```

The complete suite is normally run through `tools/run-full-ctest.ps1`, which
picks the job count and keeps a log and summary per run. An explicit `-Jobs`
overrides the local machine preference; otherwise machines without a preference
use half their logical processors, at most 8 jobs. Tests run on the
optimized `build-rel` build (assertions kept); its setup is in
[dependencies](dependencies.md#optimized-test-build-build-rel).
Configure `MMODERN_XEEN_DATA_DIR` explicitly for required CD validation,
`MMODERN_XEEN_AUDIO_DIR` when the CD audio tracks are elsewhere,
`MMODERN_PROMUS_CAPTURES_DIR` for the six DOSBox music captures `xeen_promus`
needs, `MMODERN_M94_CAPTURES_DIR` for the three mono OPL takes and the rolmus
MIDI capture, `MMODERN_MT32_ROM_DIR` for the MT-32 test ROMs, and, when needed,
`MMODERN_XEEN_UI_DATA` for its installed UI module. See the [MT-32 and capture
requirements](dependencies.md#mt-32-core-and-player-roms) for details.
The development machine's M94 capture directory is
`D:/Projetos/MModern/MModern-re/m94/captures-0c`; captures are not distributed.

The ending presentation test also provides a direct visual check of the two
score pages, using the production segments and installed EXE text:
`build-rel/mmodern_ending_presentation_tests <game-directory> <output-directory> --score-pages`.
A key (including Escape) advances a page; clicks are ignored. It writes PPM
snapshots to the output directory and changes no saves or settings.
`--score-pictures` writes the same snapshots without opening a window.

## DOS save export

Build `mmodern_dos_export` on request. It projects a current MMModern Clouds
save onto a DOS World of Xeen slot pair; the [DOS save rules](rules/dos-saves.md)
describe the projection and its refusals.

```powershell
cmake --build build-rel --target mmodern_dos_export
.\build-rel\mmodern_dos_export.exe <game-dir> <save.mmsave> <output-folder> --slot 1 --name "MMModern Export"
```

`--overwrite` permits replacing an existing slot. `--place clouds|darkside
<map> <x> <y> north|east|south|west` changes only the destination side and
position. Darkside progress remains New Game. In the maintainer's DOSBox
play-test (2026-10-29), a party placed at Castleview `(14,4)` loaded there and
could walk, talk and fight.

`--dosbox <dosbox.exe>` prepares a DOSBox Staging CD profile under the output
folder. Its `pristine/WORLD` contains the exported pair for Load, plus New
Game `XEEN.CUR` and `DARK.CUR` for the live archives; both CUR files are
needed because DOS may read them before a slot is loaded. If `--cfg <file>`
is supplied, its configuration is copied to the seed for the chosen sound
setup. Otherwise the seed has no CFG and DOS reads the installation's own
`WORLD/XEEN.CFG` through the C: mount. The profile mounts both `GAME1.INS`
and `GAME2.INS` as D: and an overlay over C:. Run `start.cmd`; before each
reset it backs up a non-empty `overlay/WORLD` under `runs/<timestamp>/`.

Compare numbered pairs or `cur` in two folders:

```powershell
.\build-rel\mmodern_dos_export.exe --diff <folderA> 1 <folderB> 2 --resave-check
```

The comparison lists every changed offset and flags invalid pair markers.
`--resave-check` accepts only name and live-party play-time changes, so an
immediate DOS Load and Save As can be checked without a separate script.

## Developer test saves

Build the opt-in tool with `cmake --build build-rel --target mmodern_test_save`.
Generated saves use the current save format, including the full roster.
Regenerate older preset files with the rebuilt tool; the game refuses them.
It starts from the normal fresh Regional Journey and writes a validated current
save for `broken-armor`, `train-ready`, `injured-dead` or `poisoned`. The party
stays at map 23 `(9,11)` facing West; world, calendar and RNG retain fresh
Journey values (explicit seed 7). The presets provide equipped broken armor,
enough XP/gold for one Training, an Unconscious member and a Dead member with
four survivors, or Poison and a usable Misc antidote, respectively.
The `monster-control` preset stands in Vertigo `(18,4)` facing West: each
caster knows the M73 spells available in their book (Sleep, Beast Master,
Hypnotize, Dragon Sleep, Insect Spray, Turn Undead, Sparks and Toxic Cloud),
with 1,000 SP each and 1,000 party gems. Generate it with:

```powershell
.\build-rel\mmodern_test_save.exe monster-control "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/monster-control.mmsave"
```

The `monster-damage` preset uses the same Vertigo position, 1,000 SP per
caster and 1,000 party gems. Together the casters know all 23 M74 damage
spells, including Finger of Death, Holy Word, Mass Distortion, Golem Stopper
and Poison Volley. Generate it with:

```powershell
.\build-rel\mmodern_test_save.exe monster-damage "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/monster-damage.mmsave"
```

The `m76-spells` preset starts at Vertigo `(18,4)` facing West. A level-7
Cleric and Sorcerer know the seven M76 party buffs and Light, Levitate,
Walk on Water, Clairvoyance, Detect Monster and Identify Monster between them;
the Sorcerer also knows Enchant Item, Recharge Item, Item to Gold,
Super Shelter, Etherealize, Time Distortion, Teleport, Lloyd's Beacon and Town Portal,
with 1,000 SP each and 1,000 party gems. Their miscellaneous packs hold
five-charge items for the first fifteen powers: the Cleric has the seven buff
powers, Light and Levitate; the Sorcerer has Walk on Water, Clairvoyance,
Detect Monster, Identify Monster, Enchant and Recharge. The Sorcerer's last
three Misc slots hold ordinary, cursed and broken rechargeable items: broken
fills to 63 charges; cursed adds the normal draw. His Recharge-power item is
itself ineligible for Recharge. The third member carries a plain weapon and
armor, an enchanted weapon, broken and cursed weapons, and weapon ID 34.
Enchant refuses the latter four weapons; Item to Gold refuses only ID 34
and converts immediately. Escape at the member prompt refunds SP/gems,
including for an item cast whose charge remains spent; later Escape keeps
the cost. An original Vertigo monster stands one
step west, at `(17,4)`, for combat casting, Quick Fight and item Use.
On mainland map 23, stand at `(12,9)` facing West to try
Walk on Water onto the shallow water at `(11,9)`.

The third member's first six Misc slots hold five-charge Super Shelter,
Etherealize, Time Distortion, Teleport, Lloyd's Beacon and Town Portal items. On mainland map 23,
`(12,10)` has cell attribute bit `0x40`: ordinary Rest refuses there, while
Super Shelter sleeps eight hours without monster interruption opportunities.
Time Distortion returns to `(10,12)` without changing facing. Teleport asks
for one digit (1-9); empty, zero or Escape keeps the cost and cancels.
Etherealize
moves one cell forward, ignoring walls. Only Time Distortion is admitted
from these six travel spells in the learned book or Quick Fight during combat.

The Cleric's beacon starts in Vertigo `(18,4)`, the Sorcerer's on mainland 23
`(9,11)`, and the third member's in Nightshadow (map 29). The third member also
knows Lloyd's Beacon. Transfer the Beacon item to the Cleric to use his stored
Vertigo destination. Set uses S/T or the original Set button; Return uses R
or its button and keeps facing. Same-map Return keeps live monster HP and
status; cross-map Return reloads. Town Portal accepts 1-5 followed by Enter;
Vertigo always reloads, including from Vertigo. Darkside beacons refuse
without spending resources. Escape keeps the
spell or item cost; an unset beacon defaults to Vertigo even on cancel.

```powershell
.\build-rel\mmodern_test_save.exe m76-spells "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m76-spells.mmsave"
```

The `m95-witch-skull` preset starts at Witch Tower Level 4, Clouds map 54
`(10,6)` facing North, beside the skull. The level-30 party has equipped gear,
spell books and enough gems for the tower; the original monsters remain. Cast
Wizard Eye before turning: press C, select a caster whose current spell is
Wizard Eye with F1-F6, then press C again. Press Right to face the skull and
Space to talk to it. Answer Yes and enter `Rosebud` to open the passage.

```powershell
.\build-rel\mmodern_test_save.exe m95-witch-skull "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m95-witch-skull.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m95-witch-skull.mmsave"
```

The `m96-chest` preset starts outdoors on Clouds map 9 `(15,11)` facing
North, on a locked chest; map 9's monsters are defeated. Outdoors an object is
drawn over the terrain only on the party's own cell, so the chest shows at the
bottom of the view. The level-30 `m78-playtest` party includes Zippo, a Robber
(member 4), whose Thievery always beats this lock. Press Space to run the
chest's Event and choose Zippo when it asks who will try: the lid's frames
cycle while the lock is picked, the treasure follows, and from then on the
chest is drawn open, also after walking East into map 13 and back and after
Save and Load.

The `m97-open-chest` preset uses the same level-30 party and location, with
the chest already open. It is intended for the DOS export comparison.

```powershell
.\build-rel\mmodern_test_save.exe m96-chest "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m96-chest.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m96-chest.mmsave"
```

The M84 input/Event presets use the current save format and a healthy travel
party with 1,000 HP per member; monsters on retained maps are defeated so the
input witnesses stay quiet. Loading still makes the normal flagged-cell entry
pass. The presets are:

- `m84-playtest-trap`: Tower of High Magic, map 55 `(7,8)` North, an
  unguarded indoor damage trap. Try W, period, Info then Escape, held keys,
  and either mouse button on the view or panel. (Map 26's traps lie on
  clouds: without Levitate the party falls at once.)
- `m84-playtest-label`: Vertigo, map 28 `(13,4)` West, the door label.
  W clears it and the following pass restores it.
- `m84-playtest-save`: map 25 `(27,3)` North, a flagged cell with no Event
  record or selected object. Use the panel's Save/Load here to inspect the
  entry pass without damage or a modal prompt; for a visible entry pass,
  save and load at the `m84-playtest-label` door label.

Generate outside the repository and game installation, then start one:

```powershell
New-Item -ItemType Directory -Force D:/Projetos/MModern/MModern-playtest
.\build-rel\mmodern_test_save.exe m84-playtest-trap "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m84-playtest-trap.mmsave"
.\build-rel\mmodern_test_save.exe m84-playtest-label "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m84-playtest-label.mmsave"
.\build-rel\mmodern_test_save.exe m84-playtest-save "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m84-playtest-save.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m84-playtest-trap.mmsave"
```

The M86 presentation presets use the current save format:

- `m86-playtest-fall`: Clouds map 26 `(5,1)` North, on original cloud terrain
  with Levitate off and 1,000 HP per member. Load and Wait. The scene scrolls
  down faster over 15 images, with FM11 and a scream. At the first full
  destination image the scream stops; unnh and FX31 accompany four impact
  images. Arrival is map 12 `(6,15)`, with 100 HP lost per member afterwards.
- `m86-playtest-moon-ray`: mainland 23 `(9,11)` West, an original Orc identity
  one cell ahead, an injured travel party, and the second member prepared as
  a Cleric with Moon Ray selected and 1,000 SP. Wait to engage; on the
  Cleric's turn press C then C; Block with earlier members to keep the Orc
  alive until then. After the attack, each recipient's cue,
  HP increase and four-frame sparkle finish before the next recipient's cue.
- `m86-playtest-border`: Vertigo `(10,11)` North, next to Training, with
  Levitate active, Spot Secret Doors and Danger Sense learned, and one nearby
  original slime identity for Danger Sense. Enter Training, wait for its
  animation and leave with Escape. The counters restart during the visit;
  the return preserves the most recent border assembly's phase, with no
  additional exit reset. Spot Secret Doors lights only when facing a thin wall.

A New Game covers door testing. Walk into a closed indoor door with Up or
its 3D movement click: FX47 precedes one full open-door view, held for at least
two BIOS ticks, then the step. The original-data test also checks Vertigo
`(4,3)` East. Backward/strafe/blocked moves do not show that image.

```powershell
.\build-rel\mmodern_test_save.exe m86-playtest-fall "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m86-playtest-fall.mmsave"
.\build-rel\mmodern_test_save.exe m86-playtest-moon-ray "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m86-playtest-moon-ray.mmsave"
.\build-rel\mmodern_test_save.exe m86-playtest-border "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m86-playtest-border.mmsave"
```

The M85 post-combat Event presets use the current save format, 1,000 HP per
member and strong melee statistics. Each keeps one original monster at its
original position; the original-data dispatch tests walk it to the party and
check that the terrain does not make the party fall.

- `m85-playtest-label`: Vertigo `(13,4)` East, original monster 35 at
  `(15,4)`. Turn left twice to West, then Wait until it engages. The door
  label disappears during combat and returns after a win. Reload and Run
  while facing West to check the caller at the landing.
- `m85-playtest-run`: the same starting position. Engage while facing West,
  turn left twice during combat to face East, then Run. The party reaches
  `(18,4)` and the landing's East label appears; this landing is no longer
  refused for having an Event.
- `m85-playtest-trap`: Tower of High Magic, map 55 `(7,8)` East, with
  original monster 8 at `(10,8)`. Wait until it reaches the party, then win.
  The skipped trap call runs once after combat. Earlier Waits can also hurt
  the party; map 55 keeps it on solid terrain.

```powershell
.\build-rel\mmodern_test_save.exe m85-playtest-label "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m85-playtest-label.mmsave"
.\build-rel\mmodern_test_save.exe m85-playtest-run "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m85-playtest-run.mmsave"
.\build-rel\mmodern_test_save.exe m85-playtest-trap "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m85-playtest-trap.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m85-playtest-label.mmsave"
```

The `m80-playtest` preset starts at the Reaper entrance on Clouds map 24
`(10,9)` facing North. It has the party's quest items 1-7, the M78 travel
party and books, 10,000 SP per caster and 5,000 gems. The Sorcerer has
Lloyd's Beacon and Jump in its book, plus a Town Portal item with 63 charges
in its ninth miscellaneous slot. Town Portal is clerical; the Cleric also
has it in its book. Space enters map 51 `(7,12)` facing South.
Quest items belong to the party;
Exchange does not change which keys it holds. For a keyless Clouds scene,
use `--event-fixture` with the desired map, position and facing (for example
`24 10 9 north`); its initial party has no entrance keys. Reaper sites on
maps 7/13, Golem 19 and Sphinx 2 always play the keyless refusal: their
Darkside keys are outside the Clouds quest counters.

```powershell
.\build-rel\mmodern_test_save.exe m80-playtest "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m80-playtest.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m80-playtest.mmsave"
```

The `m80-arena` and `m80-arena-maxed` presets start beside the Warzone
entrance on Clouds map 6 at `(13,4)` facing West, with the level-30 M78
party, equipped Obsidian gear and full HP/SP. Step forward to `(12,4)`,
then press Space and answer Yes. This original entrance is a manual Event;
walking onto it alone does not open the question. It loads map 85 at
`(10,10)`. Walk west to x=7, then south to `(7,6)` and press Space to talk
to the Battle Master. Choose Yes, level 1 and count 1 to try a fight.
After winning, talk again for the award and the return to Vertigo.

`m80-arena` starts every member's Warzone count at zero.
`m80-arena-maxed` starts the first member at 98: one win reaches 99; return
to the Warzone and talk again to check the maximum-count exit. Awards appear
on the character sheet's Awards screen. Also try declining, entering zero,
Escape at either number prompt, and talking before a fight is finished.
Leaving the Warzone discards the fight. Saving there is forbidden; both
presets use the unchanged current save format and start outside it.

```powershell
.\build-rel\mmodern_test_save.exe m80-arena "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m80-arena.mmsave"
.\build-rel\mmodern_test_save.exe m80-arena-maxed "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m80-arena-maxed.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m80-arena.mmsave"
```

The `m81-drops` and `m81-fullpacks` presets start on Clouds map 3 at
`(5,12)` facing West with the level-30 M78 party, equipped Obsidian gear,
full HP and 1,000 SP per caster. Two original Barbarians and a Ninja stand
together two cells ahead at `(3,12)`. A Barbarian always drops an item from
level 4 (accessories included) and 25 gold; a Ninja drops a level-3 item half
of the time and 50 gold. The other map-3 monsters keep their places,
including eight more Barbarians to the south-east and five Ninjas to the east.
The first four members carry bows: press S to Shoot from here, step forward to
fight in melee, or cast a group attack spell in combat. Treasure appears on
the original screens after the fight; outside combat it waits while a monster
is in view.

`m81-drops` leaves at least three free slots in every pack. `m81-fullpacks`
fills every pack except the second member's last weapon slot. A weapon drop
fills that slot. While it is free, an armor, accessory or Misc item for which
no member has room replaces the first member's last item of that category, as
the original does. Once no slot is left, the full-pack warning appears and the
remaining items are lost, but gold and gems are still credited.

```powershell
.\build-rel\mmodern_test_save.exe m81-drops "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m81-drops.mmsave"
.\build-rel\mmodern_test_save.exe m81-fullpacks "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m81-fullpacks.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m81-drops.mmsave"
```

The `m81-event` preset starts on mainland map 23 at `(8,2)` facing North,
before the Root Event, with the same party. Only two original Orcs remain: one
at `(6,5)` in view, which keeps ordinary treasure delivery waiting, and one
two cells ahead at `(8,4)`. The saved random seed makes the first Shoot (S)
after loading kill the near Orc with a generated item and 10 gold; any other
action first changes the draws, and an Orc then drops an item only 10% of the
time. Press Space at once and answer the Event: its treasure screen shows the
roots and the Orc's item without the gold. Turn away to get the ordinary
delivery: the same item again, with the 10 gold. The doubled item is an
original bug kept on purpose ([quirk](original-quirks.md#sv-event-loot-stash)).
For the dormant case, after the Shoot press `.` (wait) until the other Orc
attacks, Run with the whole party (Run can fail; repeat it), walk back to
`(8,2)` facing North and press Space: the item arrives once with the Event's
own and the gold never.

```powershell
.\build-rel\mmodern_test_save.exe m81-event "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m81-event.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m81-event.mmsave"
```

The `m79-playtest` preset starts at Rivercity Training, map 30 `(12,12)`
facing North, with 100,000 gold, 5,000 gems and one ration. Permanent levels
repeat 9, 14 and 19, with enough XP for the three Training caps. Members 1,
3 and 5 have all six Clouds Guild memberships; the others have none. Books
are partial, members are injured, member 1 carries a cursed weapon and member
2 a broken armor item. Members 5 and 6 are Sorcerers with Town Portal and
Lloyd's Beacon: their beacons reach Shangri-La Ironworks `(14,8)` on map 49
and Newcastle Training `(11,7)` on map 73. Use Town Portal for the five towns;
use the two beacons for Shangri-La and Newcastle, then the original stairs to
Newcastle Level 2's Bank and Temple. The Temple keeps the prices of the last
Temple visited in the running program, including across New Game and Load;
in a fresh program it has zero surcharges and no Donation charge. Interact
with Space at a service, and use its original buttons or keys.

```powershell
.\build-rel\mmodern_test_save.exe m79-playtest "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m79-playtest.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m79-playtest.mmsave"
```

The `m78-playtest` preset is a travel party at map 23 `(9,11)` facing West:
the original six at level 30 with full HP and SP, Obsidian weapons and armor
equipped as each class allows, every spell of each caster's list, 100,000
gold and 5,000 gems. Arturius and Tyro hold Mountaineer and Pathfinder,
everyone can swim, and Badger has the information skills. `m78-one-holder`
is the same party with Mountaineer and Pathfinder on Arturius only, so the
mountains and outdoor wall types 3 and 6 stay closed until a teacher adds a
second holder.

```powershell
.\build-rel\mmodern_test_save.exe m78-playtest "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m78-playtest.mmsave"
```

The `party-healing` preset also starts at Vertigo (18,4). A Cleric and a
Druid know the M75 spells allowed by their classes, with 1,000 SP per caster
and 1,000 party gems.
Four patients carry Poison, Disease, Paralysis, Stone, Dead and Eradicated;
the casters carry five-charge items for all ten M75 powers.

```powershell
.\build-rel\mmodern_test_save.exe party-healing "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/party-healing.mmsave"
```

For example, to create the play-test folder, generate the `train-ready` save
and load it:

```powershell
New-Item -ItemType Directory -Force D:/Projetos/MModern/MModern-playtest
.\build-rel\mmodern_test_save.exe train-ready "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/train-ready.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/train-ready.mmsave"
```

The `combat-items` preset starts on the mainland at (9,11), facing West,
near monsters. The party carries weapons and armor to swap, a cursed weapon,
the Xeen Slayer Sword, and charged damage, healing and Town Portal items.
The last member has full packs. Enter combat with Wait, manage equipment
through a character's Items dialog, and use charged items through the combat
Use button (U). The Quests button (V, or Q in Items) shows quest items,
including three Phirna Roots, current quests and Auto Notes. The first member
has more than nine awards, including Warzone wins and a race award: open
Awards on that character's sheet and scroll with Up/Down. The party has
Direction Sense, Spot Secret Doors and Danger Sense: the facing letter shows
below the view (hidden while a text or number prompt is open), and the top
border gem lights after the first step or turn while a monster is within
three cells. Spot Secret Doors lights only indoors, facing a plain wall with
open space behind it; Vertigo has none, so try it in an Event developer
session, whose original party also has the skill:
`.\build-rel\mmodern.exe --event-fixture "F:/Games/gog/Might and Magic 4-5" 30 6 7 north`
(Rivercity).

```powershell
.\build-rel\mmodern_test_save.exe combat-items "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/combat-items.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/combat-items.mmsave"
```

Choose your own output path in an existing directory outside both this repository
and the game installation. The tool refuses existing files. Generated `.mmsave`
files are local play-test artifacts; never commit them. The game and save format
are unchanged.

### Inn and recruiting

`m87-inn` places the original party outside Vertigo's tavern at (24,4), facing
North. `m87-three` starts there with three active members and the other three
lodged in Vertigo. `m87-shangrila` starts outside Shangri-La's tavern at
(14,12), facing North. Step forward, interact with the tavern (Space or the
view), and choose Sign In (S). The Inn lists local characters, permits Remove
down to zero, and requires at least one member to Exit. Delete clears a listed
character after Y confirms. Create rolls attributes, exchanges them, selects
a class and asks for a name; the new character stays lodged until added to
the party.

`m88-roster` starts at the same Vertigo door with the original six active
members and 22 named characters lodged there. Only slots 12 (Gnome, male)
and 21 (Human, female) are free. Create twice, then C shows the full-roster
refusal. Up and Down choose those free slots; Left and Right move the class
arrow, Enter commits, R or Space rerolls, and the attribute keys exchange
values. Returning to the Inn resets its page and keeps its song playing.

```powershell
.\build-rel\mmodern_test_save.exe m87-inn "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-inn.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-inn.mmsave"
.\build-rel\mmodern_test_save.exe m87-three "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-three.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-three.mmsave"
.\build-rel\mmodern_test_save.exe m87-shangrila "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-shangrila.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m87-shangrila.mmsave"
.\build-rel\mmodern_test_save.exe m88-roster "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m88-roster.mmsave"
.\build-rel\mmodern.exe --load-game "F:/Games/gog/Might and Magic 4-5" "D:/Projetos/MModern/MModern-playtest/m88-roster.mmsave"
```

Sign In, confirmed Delete, every return from Create and Exit save automatically.
In the developer `--load-game` path,
these writes use that explicit file; keep a separate copy to repeat the
starting setup. Managed play uses its current numbered slot. Reload a Sign In
save to reopen the Inn without adding a day. Add and Remove alone do not save.

### Clouds quests and the ending

The M89 presets use the level-30 `m78-playtest` party and original maps,
MOB slots and Events. Each starts beside a walkable fight cell:

| Preset | Start and purpose |
| --- | --- |
| `m89-lord-xeen` | Map 78 `(5,8)` North, first member's Slayer Sword equipped, temporary Speed +100, party Holy Bonus 20 and quest 23 active. Step forward to fight, then visit the Sixth Mirror at `(4,9)` facing West for its text, the win and the original ending. |
| `m89-clan-king` | Map 41 `(30,25)` North, Gunther's dwarf quest active and the Vertigo prerequisites complete. Step forward to the Clan King's group, then return to Gunther. |
| `m89-draco` | Map 29 `(1,13)` North at 21:00, with the three coffin prerequisites set and earlier enemies removed. Step forward and open the coffin to spawn Count Draco; afterwards visit the well at `(7,7)`. |
| `m89-dragons` | Map 15 `(0,12)` North, Medin's lake quest active and Walk on Water enabled for deep water. Kill slots 0-2, leave the map and return to Medin. |
| `m89-trolls` | Map 8 `(2,7)` North, beside the Ice Troll lair. Wound a troll and check its full heal at a new combat round. |

Generate each with `mmodern_test_save <preset> <game-dir> <new-path.mmsave>`.
The tool refuses an existing path; keep an untouched copy for repeated play.

The M90 presets use the same level-30 `m78-playtest` party and original
monsters. Their quest items and currency are limited to the named gate:

| Preset | Start and purpose |
| --- | --- |
| `m90-newcastle` | Map 12 `(11,12)` South, 50,000 gold, 15 King's Mega Credits (item 26), and Rescued Crodo (award 13). Buy the deed, then visit Emerson and the Taxman for each building stage; Artemus supplies the permit. |
| `m90-golem` | Map 20 `(4,4)` South, Yak Stone (7), Stone of a Thousand Terrors (5), Golem Stone (6), no credits. Enter Yak Temple; the other Golems are on maps 12 and 8. Collect credits in 79, 80 and 81. |
| `m90-illusion` | Map 45 `(7,14)` North, 300 gems. Use the skull to buy the Tower of High Magic key. |
| `m90-high-magic` | Map 12 `(6,15)` South, High Magic key (3). Enter through the Reaper and climb from 55 to Darzog's key at 58 `(7,12)`. |
| `m90-darzog` | Map 15 `(4,13)` North, Darzog's key (2). Enter 59 and reach Crodo at 61 `(7,6)`. |
| `m90-clouds` | Map 27 `(15,2)` North, quest 23 and award 13, 11,000 gold and 1,100 gems (ten attempts per game). Slick's first two games cost 100/1,000 gold; the other two cost 10/100 gems. South at this cell returns to map 62; head North into Clouds. |
| `m90-castle` | Map 27 `(16,29)` North, Cupie Doll (28) and first member's Slayer Sword equipped. Open the gate to 75 `(6,0)` and climb through 76 and 77 to the M89 Lord Xeen approach on 78. |
| `m90-winterkill` | Map 32 `(8,13)` North, before Randon's quest. Speak to him, clear each monster wave and ring the bell at `(13,1)`. |

Positions and trigger facings come from each map's original Event records.
All other M90 purses start empty; none of these presets pre-completes its gate.

The M90 Part D spots shorten the play-through: each stands on the original
Event cell with its facing, so the Event runs as the save loads, and carries
only the story state reached at that moment, including the Event lines the
original's AlterEvent has already disabled. Walking between spots is optional.

| Preset | Start and purpose |
| --- | --- |
| `m90-emerson-short` | Emerson, map 64 `(2,7)`, deed owned, 4 credits: "You don't have enough Megacredits." |
| `m90-emerson-walls` | Emerson with the deed and 5 credits: pay for the walls (flag 43, five credits taken). |
| `m90-taxman-walls` | Taxman, map 12 `(11,12)` South, walls built: Stone of a Thousand Terrors (5), teleport to 72 `(7,15)`; outside, map 12 shows the castle. |
| `m90-emerson-keep` | Emerson with stone 5 and 5 credits: pay for the keep (flag 44). |
| `m90-taxman-keep` | Taxman after the keep: Golem Stone (6), teleport to 73 `(7,15)`. |
| `m90-artemus` | Artemus, map 64 `(1,4)`, after his first talk, with Rescued Crodo: Excavation Permit (27). |
| `m90-emerson-dungeon` | Emerson with the permit and 5 credits: pay for the dungeon (flag 54). |
| `m90-taxman-dungeon` | Taxman after the dungeon: congratulations, teleport to 73. |
| `m90-laboratory` | Map 73 `(12,14)` North, dungeon open: the password `Laboratory` leads to 71 `(2,14)`; a wrong word says "Incorrect!". |
| `m90-sword` | The pedestal, map 71 `(7,4)`: the Xeen Slayer Sword; stepping back on says "The pedestal is empty." |
| `m90-mirabeth` | Mirabeth, map 16 `(12,3)` West, first visit: Elixir quest (11) and Yak Stone (7). |
| `m90-mirabeth-elixir` | Mirabeth with the Elixir (10): 250,000 XP, +5 Personality and her award. |
| `m90-elixir` | The Elixir cell, map 79 `(30,25)` East: take it once; it stays gone after Save and Load. |
| `m90-credit-yak` | A Yak Temple credit, map 79 `(27,22)` South: take it, Save, Load, and step back on the empty cell. |
| `m90-golem-terrors` | The map-12 Golem, `(6,6)` South, with stone 5: entrance to 80. |
| `m90-credit-terrors` | A Thousand Terrors credit, map 80 `(19,9)` East, as above. |
| `m90-golem-stone` | The map-8 Golem, `(11,9)` South, with stone 6: entrance to 81. |
| `m90-credit-golem` | A Golem Dungeon credit, map 81 `(4,1)` South, as above. |
| `m90-darzog-key` | Darzog's key, map 58 `(7,12)`, with the High Magic key: chest, 100 gold and key (2). |
| `m90-crodo` | Crodo, map 61 `(7,6)`: Rescued Crodo, 1,000,000 XP and quest 23; the exit is at 62 `(5,5)`. |
| `m90-slick` | Slick's first game, map 27 `(15,24)` North, quest 23, 11,000 gold and 1,100 gems; the others are at `(14,26)`, `(11,25)` and `(12,22)`. |
| `m90-slick-trade` | Slick's trade, map 27 `(12,29)` North, with the four dolls (29-32): Cupie Doll (28). The gate is at `(16,29)`. |
| `m90-generator` | Guard Making Machine, map 78 `(11,11)` North, with the Slayer Sword: destroy it (flag 202, 200,000 XP); the Fire Generator is at `(3,1)`. |
| `m90-winterkill-gong` | The gong, map 32 `(13,1)`, quest 18 accepted: wave 1; ring again once Winterkill is empty to set flag 0. |
| `m90-winterkill-wave2` | Randon, `(8,13)` North, wave 1 done and Winterkill empty: congratulations and wave 2 (flag 72); then the gong. |
| `m90-winterkill-wave3` | Randon after the second gong (flag 3), Winterkill empty: wave 3 (flag 73); then the gong (flag 8). |
| `m90-winterkill-final` | Randon after the last gong (flag 8), Winterkill empty: award, 500,000 XP and the end of quest 18. |

The M92 presets finish Clouds of Xeen with the `m89-lord-xeen` party:

| Preset | Start and purpose |
| --- | --- |
| `m92-lord-xeen` | The `m89-lord-xeen` start. Kill Lord Xeen, take the Scepter from the treasure screen, then press Space at the Sixth Mirror at `(4,9)` facing West. |
| `m92-sixth-mirror` | Map 78 `(4,9)` West, at the Sixth Mirror with Lord Xeen dead: the party holds the Scepter (quest item 8), quest 23 is over, flag 60 is set and every member has the "Defeated Lord Xeen" award (42). Press Space: the mirror's text, the win, the save at Vertigo and the original ending and score follow. |

The win saves over the loaded file, like Sign In; keep an untouched copy.

### Cartographer arrival presets

`m98-load` starts at Clouds map 9, `(14,11)`, North, on a blank cell with
an active Cartographer. `m98-inn` starts inside Vertigo's Inn with five active
members without Cartographer and a lodged Cartographer available to add;
Exit returns to map 28, `(18,4)`, West. Both use the level-30 M78 party and
quiet original maps. The first Load frame marks the former cell; Inn entry
keeps the latter blank until the edited party exits and the scene is checked.

Generate them with `build-rel/mmodern_test_save.exe <preset> <game-dir> <output>`.
The M98 session folder is `D:/Projetos/MModern/MModern-playtest/m98/`.
Its New Game item uses the New Game entry and checks Vertigo `(18,4)`, West.
