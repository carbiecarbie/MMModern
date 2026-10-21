# World Events: map changes, script values, travel and input

**Evidence: DOS static**, installed English CD `WORLD/XEEN.DAT`, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
These rules were checked on Windows on 2026-10-15 using bounded 16-bit
disassembly at the file offsets below. The Event dispatcher and signed-byte
operand reads agree with the calibration in [the M70 plan](../archive/milestone-70-plan.md).
They establish code paths, not a claim of DOSBox observation.

| Operation | Rule and original file offsets |
| --- | --- |
| MoveWallObj (34) | Read slot, signed x and signed y; replace position only. Repeating the same position does not flip the wall item. `0x2860e..0x2863a`. |
| MakeNothingHere (46) | Clear every opcode at the party's physical cell, across all directions and lines. CallEvent changes the logical execution cell, not this deletion cell. Continue with ordinary line advancement: `0x2793b..0x279ec`, `0x28356..0x28372`, `0x28d30..0x28d3a`. The reference exits instead. |
| AlterMap (19, census name AfterMap) | Signed x/y, direction, wall nibble. Directions 0–3 select north/east/south/west. Direction 4 writes all four faces of that cell; no opposite face is implicitly changed. Resolve the physical DAT first. `0x27b41..0x27ba3`, helper `0x234dd..0x23550`. |
| AlterCellFlag (35) | Signed x/y and surface. Outdoors replace the low four wall-word bits; indoors replace the low three attribute bits, preserving the other bits. Indoor coordinates are masked to 0–15 after neighbour selection, so logical `(25,5)` aliases physical `(9,5)` in the east DAT. `0x2863d..0x28721`. |
| AlterEvent (24, census name AfterEvent) | Scan physical cell, current/all direction and selected line, replacing every matching opcode, including with a nonzero value. No match is a no-op. The line operand is sign-extended against lines stored zero-extended (`0x2827f..0x28287`; index builder `0x21e75..0x21e7e`), so operands 128-255 never match. If the effective matched opcode is PlayCD (60), also replace the next record in storage order. `0x28189..0x28356`, paired-record branch `0x282c0..0x28304`. The reference lacks that paired-record branch. |
| Spawn (16) | For a slot below the map's MOB monster count: reuse the slot's type and bind its MON statistics; write coordinates, physical damage status, asleep/not attacking and full MON HP. A revived slot loses its defeated/accounted state. `0x27a59..0x27b1c`; frame draw `0x27a85..0x27aa1`. A slot at or beyond the count is written in DOS but stays inert, as described below. |

AlterCellFlag's outdoor branch does **not** mask x/y after lookup, unlike its
indoor branch. No valid original operand witness establishes outdoor neighbour
writes. Such operands remain explicitly refused; an indoor-style alias is not
assumed. Ordinary original surfaces fit their respective four/three-bit fields.

AlterMap ignores the lookup's missing-neighbour return. The last resolved DAT
index remains selected and the write masks both coordinates to 0–15. Map 54
has no neighbours, so its operand `(25,5)` writes its own `(9,5)`. The DOS
lookup retains the DAT index at `0x11db7..0x11def` and
`0x11e1e..0x11e43`; the write uses it at `0x234f5..0x23549`.
This rule does not manufacture a neighbour resource or open missing geometry.

Spawn never changes the monster count byte `0x9c59`, which map load sets to
the number of MOB monster records (`0x0fb79..0x0fb84`). Every monster loop
stops at that count (`0x1099c`, `0x155a6`, `0x1fe98`, `0x20ce7`, `0x2ea7d`,
the Event scan at `0x287be`), and the `.mob` writer stores x/y only below it
(`0x266e3..0x2671c`). A slot at or beyond the count is therefore never drawn,
moved, attacked, fought, counted or saved, and map load zeroes the whole
107-slot array (`0x0f5be..0x0f5c7`). MMModern keeps such a Spawn inert and
never stores it. Saves from older builds that hold such slots load without
them. Vertigo's exit script spawns slots 50 and 51 past its 46 records, so
they stay invisible. Only the Warzone arena prompt changes the count
(`0x2b9d5..0x2ba0b`), and a Warzone load sets it to 0 (`0x0fb90..0x0fba9`).
The Battle Master generates that visit's table; every load clears it after
the normal MOB load. Leaving discards it, including explored cells, and no
Warzone state is saved.

The Battle Master (`DoTownEvent` action 6) offers a level from 1 to 10 and
1 to 20 monsters. Zero or Escape cancels. An unfinished fight refuses a new
one; a completed fight increments each active member's Warzone award byte,
without saturation. With no fight, any active member's count of 99 or more
prevents another. Victory and the maximum-count refusal load Vertigo at its
run position facing East; declining loads map 6 at `(12,4)` facing West.
Other outcomes use the Warzone's run position facing East.

Generation draws five species slots before any monster's position, slot,
frame and conditional effect phase, all from gameplay randomness. The DOS
species adjustments and ordinal suffixes differ from the pin; see the
[Arena evidence](evidence.md#4-arena-dialog-generation-and-exits) and
[recorded quirks](../original-quirks.md#sv-arena-species-pool). The original
map-6 entrance at `(12,4)` facing West is a manual Event: its attribute byte
is zero, so Space opens its confirmation after walking there.

Spawn may also stack monsters: it has no occupancy test, so a cell can
hold four or more (Winterkill's waves, Tower of High Magic level 1). Only a
moving monster refuses a destination already holding three (`0x1fc24`, count
built at `0x1fe64..0x1fe9f`), and a monster may still leave a stacked cell.
Each 3D redraw (`0xd0b1..0xd0d3`) rebuilds the attacker slots in monster slot
order (indoors `0x14406`, outdoors `0x1692c`; also from combat frames at
`0x1106c` and `0x11495`). Every monster on a watched cell is woken, and the
first ones take that cell's display and contact slots (three on the front
cell, `0x169c0..0x169f4`). The rest stay hidden until a slot frees, when they
join combat (front slots 0-2, `0x2e282..`). Ranged monsters are picked per
monster outside slots 0-2, with no occupancy condition (`0x1ff7c..0x1ff98`).
The row-or-column test (`0x1ff27..0x1ff38`) admits the party's own cell, so a
hidden fourth ranged monster stacked there fires, indoors and outdoors; only
the three front-slot monsters are excluded. The `.mob` writer saves every
stacked position.

AlterEvent and MakeNothingHere end at `0x28356`, which also sets the
interpreter's script-executed flag (`-0x4f`), as every operation ending at
`0x28b2b` does, Spawn, AlterMap, MoveWallObj and AlterCellFlag included. The
script epilogue reads that flag (`0x28e97`, `0x28f03`) to suppress the
"Nothing here" window and its sound when the party faced an object.
Unlike the reference's `_scriptExecuted`, which every dispatched instruction
sets, only operations that reach these writes count. MMModern has no
"Nothing here" path yet; one should follow this flag.

The map changes above consume no random draw. Spawn alone calls the bounded
random helper once with `0..7`, for the animation frame, at `0x27a89`. The same
helper is calibrated by RndDamage's `1..max` call at `0x285f8`. MMModern retains
the approved M48/M50 deviation: Spawn's frame uses cosmetic randomness, leaving
the Journey generator untouched. Cache reloads, serialization and restoration
do not re-execute these Events.

## Ownership and lifetime

Wall items and Events belong to the logical MOB/EVT map and original record.
Walls and surfaces belong to the resolved physical DAT tile and cell. The
save records carry the resulting positions, opcodes and effective
word/attribute pairs.

M69's lifetime decisions still apply: indoor neighbour DAT changes are retained;
outdoor neighbour DAT changes expire when leaving the logical map and are
excluded from saves, preserving any older retained tile underneath. Clouds map
85 (the Warzone) is visit-only. Monster HP and wake state rebuild on map load;
positions, type binding, lifecycle and accounting retain their defined lifetime.
Shared operations preserve the side in every logical and physical identity.

## Worked resource examples

- `maze0047.evt` offsets 150/159 move wall records 0 to `(-128,-128)` and 4
  to `(13,13)`; offset 175 changes surface `(12,7)` to 6. The reverse switch
  restores surface 5 and exchanges the visible wall records.
- `maze0054.evt` offset 701 writes west wall 8 at logical `(25,5)`, physical
  `(9,5)` in the current DAT because its east neighbour is absent. The adjacent cell's opposite face is a separate
  instruction at offset 711.
- `maze0079.evt` offsets 472–502 separately write four wall faces along a
  corridor. They are not a symmetric-door operation.
- `maze0027.evt` offset 464 clears the ten Event records at physical `(30,30)`.
- `maze0029.evt` offset 1572 spawns slot 22 at `(1,1)` after opening the crate.
  That slot uses its original MOB monster type, not the opcode's unused suffix.

## Script values: conditions, writes and random draws

**Evidence: DOS static**, same `WORLD/XEEN.DAT`, bounded 16-bit disassembly on
2026-10-16 (work files outside the repository). The dispatch table at `0x29320`
routes opcodes 8-10 to `0x2712d`, 12/28/29/38 to `0x27322`, 15 to `0x279ef`,
20 to `0x27ba6`, 23 to `0x28131`, 27 to `0x283fc`, 33 to `0x285cc`, 42 to
`0x28765`, 43 to `0x287d8`, 44 to `0x287ee` and 45 to `0x289d7`. The shared
value routines are ifProc (`0x2162e..0x21bd8`, action table `0x21beb`),
giveTake (`0x2200c..0x23315`, take table `0x23415`, give table `0x23331`),
setValue (`0x23945..0x23c93`, table `0x23c95`) and giveExt
(`0x2358e..0x23917`). The random helper `0x45b9a` returns `min..max`
inclusive; MMModern draws the same range from the Journey generator at the
same place. There is one stream: every Event draw, GiveMulti's included, comes
from the current Journey state in program order, and a GiveMulti generator
lives only while that instruction's generation is suspended. Code paths, not
DOSBox observation.

| Operation | Rule |
| --- | --- |
| If1/If2/If3 (8/9/10) | Operand width by mode: 32-bit for 16, 34, 100; 16-bit for 25, 35, 101, 106; a byte otherwise (table `0x29304`). Compare `>=`, `==`, `<=` as unsigned 32-bit values (`0x21b84..0x21bd8`). With the whole party selected (SetChar 0) every member is tested and the results are OR-ed, without stopping early (`0x2721c..0x2726e`); otherwise the selected member. Action 44 always asks once. A true test jumps to the target line. |
| TakeOrGive (12) | `giveTake(mode1, value1, mode2, value2)` for the selected member, or for each member in turn when the whole party is selected (`0x27808..0x278d3`). A failure for one member ends the Event (`0x278ff`). In the party loop, a failure after taking gold, gems or food ends the Event, a success after giving an item, gold, gems or food ends the loop (the reward is given once), and take modes 8 and 82 are cleared after the first member (`0x27877`, `0x278b4`). |
| TakeOrGive_2/_4 (28, 38) | Test pair 1 with `==` (28) or `<=` (38) for each selected member; on success give pair 2 with no take; stop after a give of mode 82 (`0x27602..0x276f2`). TakeOrGive_3 (29) tests pairs 1 and 2 and gives pair 3 (`0x2770d..0x277f3`); the reference gives pair 2's mode with pair 3's value. |
| GiveMulti (20) | A locked chest is picked first (below). Then three pairs through giveExt: 34/35 add treasure gold/gems, 100/101 add a random amount of treasure gold/gems and 106 a random amount of food straight to the party (the operand's low word, below), 66 generates an item, anything else is `giveTake(0,0,mode,value)` for the selected member (or the first when the whole party is selected, `0x27e71`). A failed give ends the Event. |
| SetVar (27) | setValue for the selected member or every member, same widths as If. Plain stores of the original byte, word or dword; modes 95 and above do nothing. |
| SetChar (15) | Signed selector (`0x279ef`): 0 selects the whole party; 1..6 only name the combat target used by selection 8; 7 draws `1..party size` and selects that member; 8..127 open WhoWill (not supported yet). Member selection 8 (every member except the combat target) is not implemented generically either: no Clouds script produces it, so both wait for the Darkside arc. |
| SelectRandomChar (43) | One `1..party size` draw selects that member; no reroll for a disabled member (`0x287d8..0x287eb`). |
| JumpRnd (23) | Signed range, match and line. One `1..range` draw; jump only when it equals the match (`0x28131..0x2815d`). |
| RndDamage (33) | One `1..maximum` draw, then ordinary typed damage of that amount to the selection, with its own draws (`0x285cc..0x2860b`). The third operand byte is not read. |
| IfMapFlag (42) | A monster is present while both position bytes are below 32. Monster 255 tests every slot below the MOB count. Present continues; absent jumps (`0x28765..0x287d5`). |
| ItemType (45) | Stores the special-event item type for the rest of the run (`0x289d7`, reset at `0x26789`). |
| NPC (5) mode 0 | The NPC window with the Yes/No buttons of the confirmation (global sprite 7 at (232,74), confirm buttons at (235,75) and (260,75), `0x25950..0x259ac`) on its last page. Yes jumps to the target line; No or Escape continues. |

**Selection and run-local values.** Each run starts with member 1 selected,
combat target 1, damage type 0 and item type 0 (`0x26aaa..0x26ab3`,
`0x26789`). These values belong to the run and are never saved.

**Value modes.** ifProc reads, setValue stores, giveTake subtracts (take) and
adds (give) the same fields:

| Modes | Field |
| --- | --- |
| 3, 4, 5 | Sex, race, class bytes |
| 8, 9 | HP and SP words (ifProc sign-extends) |
| 10, 77 | AC bonus byte; ifProc 10 reads the computed armor class instead |
| 11, 12, 64 | Level bonus, age bonus and level bytes; ifProc 12 reads the computed age |
| 13 | Skill byte (`0x27 + skill`): take clears, give increments |
| 15 | Award nibble; see below |
| 16 | Experience dword |
| 17, 70..73 | Party poison, light, fire, electricity and cold words |
| 18 | Condition byte; see below |
| 19 | Learned-spell flag of the class book |
| 20, 103, 104 | Game, world and quest flags (Clouds banks on Clouds) |
| 21 | Item by ID: weapons below 35, armor below 49, accessories below 60, Misc by material below 82; quest counters from 82 |
| 25, 76, 85 | Minutes, day and year words |
| 34, 35, 65 | Party gold, gems (dwords) and food (word) |
| 37..43, 45..51 | Temporary, then permanent Might, Intellect, Personality, Endurance, Speed, Accuracy, Luck bytes |
| 52..63 | Permanent, then temporary fire, electricity, cold, poison, energy, magic resistance bytes |
| 69, 79, 94 | Levitate, Wizard Eye and Walk on Water bytes |
| 74 | Every party effect above at once |
| 78, 81 | ifProc: 1 when the maximum HP/SP is at least the current value, compared as unsigned words; give: refill |
| 83, 84 | setValue: Event damage type and facing |
| 86..92 | ifProc: the full attribute (sheet value) |
| 93 | ifProc: the day modulo 10 |
| 99, 102, 105 | ifProc: party skill, thievery, Megacredits counter |

Byte grants of modes 37..64 saturate at 255; every take and the other grants
wrap. Grants of 3..5, 9..11, 13, 15 (not award 8), 16, 19 (when the spell is in
the book), 34, 35, 37..65, 77, 78 and 81 end with the member's portrait
sparkle (`0x23278`), once per member per run; give 8 sparkles before adding
the HP (`0x226c5`). Give 74 also activates Walk on Water (`0x231df..0x232b5`); the
reference clears it. setValue 74 stores the low byte in every field. ifProc 69
reads Levitate as 0 or 1 (`0x21a36`), and 93 the remainder of the day (`0x21ad1`).

**Uninitialised test values.** A failed skill (13), award (15), condition (18)
or spell (19) test, and every unhandled action, leave ifProc's local value
unset (`0x216c0`, `0x216d3`, `0x21703`, `0x21729`, `0x21b84`). Before an
instruction's first test the frame holds the interpreter's saved SI:DI from the
per-instruction message copy (`0x26c26..0x26c4d`, `0xacd6..0xacda`): DI is the
record's offset in the Event file, SI is unknown. In the party loop each later
member sees the previous member's value. A give in between makes the value
unknown. MMModern applies exactly this: a value that differs from the operand
in its low word fails an `==` test, a `>=` operand at most the offset passes,
and any outcome that would depend on SI refuses as not supported. All 91 Clouds
uses are If2 `==` tests of skills 5/7 and awards whose record offsets differ
from their operands, so they behave as plain failures.

**Awards.** CHR byte `0x39 + n` holds award `n` in its low nibble and award
`n + 64` in its high nibble; 73 and 81 use bytes 62 and 63 (`0x3ba44..0x3bb05`).
A grant ORs 1 into the nibble. A removal ORs `0xf` into the *other* nibble
instead of clearing (an original defect); no Clouds script removes an award.

**Conditions.** setValue and give 18: 16 clears all sixteen, 6 (In Love) sets
1, any other value increments; 13..15 also zero positive HP (`0x22782`,
`0x239ed`). Take 18 clears the byte.

**Items and treasure.** Give 21 queues a weapon, armor or accessory by ID in
the treasure; a quest ID first increments its counter, then shows as a weapon
(`0x22883..0x22967`). Take 21 removes the first matching item and sorts the
category (`0x221fa..0x22343`). Quest counters are bytes in both directions on
every execution path: give 21 (`0x22967..0x2296e`) and GiveEnchanted's quest
branch (`0x289c2..0x289d1`) turn 255 into 0, take 21 (`0x22339..0x22340`) turns
0 into 255, and If 21 reads the byte (`0x217ce..0x217db`). The save keeps its
existing counter width.
Give 66 generates an item; after ItemType it is a special-event item whose
category and ID come from the type, still drawing the category and
subcategory first (`0x25cbc..0x25d42`), and it needs a free Misc treasure slot
(`0x2300e`). A full category fails the give.

**Locked chests.** When the selected object is not object 0, its sprite is
16, 58 or 73 (15 too on Darkside, table `0x235c2`) and no lock was picked yet
this run, giveExt opens the chest (frame 1), then draws `1..4`; on 1 it draws a
type `0..6` and deals the map's trap damage to the recipient as ordinary typed
damage (`0x23618..0x23644`). It then draws `1..20`: thievery plus the draw at
least the map's box difficulty picks the lock, adds level x difficulty x 10
experience (a 16-bit product) and shows "picks the lock"; otherwise the chest
closes, "was unable to pick the lock" is shown and the Event ends
(`0x23647..0x23726`). A test against 20 that follows the sprite table reads
the same sprite byte and so never skips the lock; the reference reads it as
the object's x position. The open frame lasts until the map is loaded again,
as in DOS, and is not saved. The original lock sounds (10 and 21) are part of
the deferred audio.

**Taking gold, gems or food.** Party::subtract (`0x1e941`) refuses a shortfall
with the original "not enough" message for that consumable and ends the Event.

**Time.** Give 76 calls addTime with `days * 1440` minutes as a 16-bit
argument (`0x23218`, `0x263b3`): the script clock advances, the weekly
merchant/bank step runs when it crosses its day, and the dawn rest step is
skipped and cleared while a script runs (`0x26425..0x2648a`).

**Random draw inventory.** JumpRnd, RndDamage, SelectRandomChar and SetChar 7
draw once each at the instruction. A locked chest draws `1..4`, a trap type
`0..6` with the trap's damage draws, then `1..20`. giveTake draws `1..value` when its two
modes and values are equal (the value then becomes the take, `0x22089`), give
100 draws party gold, GiveMulti 100/101/106 draw treasure gold, gems and food,
give 66 runs the item generator, give 76 runs the time step's merchant
regeneration, and typed damage keeps its own draws. No other value mode draws.
Draws with a word argument (giveTake's equal-pair draw `0x22091`, give 100
`0x232bc`, GiveMulti 100/101/106 `0x23770..0x237de`) pass only the operand's low
word, also for 32-bit operands: GiveMulti 100 with 65537 draws `1..1`. The
helper divides by that word as a signed value (`0x45ba3..0x45bac`), so a word
from `0x8000` draws `1..(0x10000 - word)`, and the result is sign-extended
before a 32-bit addition (`0x232c9`, `0x23791`, `0x237e6`): 32768 subtracts.
A zero word divides by zero in DOS; MMModern refuses it before drawing.

**Not supported yet.** These refuse with a notice rather than being guessed:
HowMuch (a take of value 0), give 8/82 damage and take 8, take 25/76 and give
25 (time going back), Misc item grants by give 21, a non-caster's spell test,
grant or removal (DOS uses whichever book the program selected last,
global `0xf604`), SetChar 8..127 (WhoWill), Darkside character flags (107)
and GiveMulti 67. Every skill grant and removal is supported: it only writes
the skill byte, which the save keeps. Arms Master, Direction Sense, Merchant,
Mountaineer, Swimmer, Tracker, Spot Secret Doors and Danger Sense are granted
and saved. Direction Sense, Spot Secret Doors and Danger Sense light their
border indicators ([rules](magic.md#border-and-information-screens)), Merchant
sells at full value and Swimmer lets the whole party onto water. Mountaineer's
passage over blocking terrain is not implemented yet; the reference gives Arms
Master and Tracker no passive effect.

### Part B worked examples

- `maze0050.evt` @574/@595: If1 gold `>= 2000` (32-bit) and gems `>= 20` (16-bit).
- `maze0029.evt` @4112: If1 minutes `>= 301` (16-bit).
- `maze0082.evt` @790: If1 mode 37 reads temporary Might, also on map 82.
- `maze0035.evt` @2109: JumpRnd `1..10`, jump to line 10 on 10.
- `maze0034.evt` @2720: RndDamage type 5, `1..20`.
- `maze0080.evt` @3183: SelectRandomChar.
- `maze0079.evt` @2746: SetChar 2 names the combat target only.
- `maze0032.evt` @180 and `maze0050.evt` @566: IfMapFlag any monster / monster 0.
- `maze0025.evt` @1516: ItemType 3 before GiveMulti level 4 (a special-event Club).
- `maze0055.evt` @335: SetVar 74 value 0 clears every party effect.
- `maze0073.evt` @39: TakeOrGive_2 `(77,0)` then give `(77,20)`.
- `maze0030.evt` @2292: TakeOrGive give 18 value 3 poisons the selected member.
- `maze0027.evt` @3096: NPC mode 0, Yes jumps to line 12.

### Gold Veins (Dwarf and Deep Mines)

The 58 Gold Vein scripts on maps 33-41 share one mechanism; their tiers are
lines that AlterEvent turns off. Each attempt asks WhoWill, draws JumpRnd
`1..10` (10 is a cave-in: damage, "A cave in!", no tier change), and otherwise
runs If1 SP `>= 0` (always true: CHR word +0x158 sign-extended, `0x21685`,
`0x21b46`) chains to the first tier line still active. A tier is a GiveMulti
of `1..100` gold (mode 100; `1..1000` in the 5000 tier) plus a fixed 5000,
1000, 250 or 100 (mode 34), followed by AlterEvent(tier line, 0). The last tier gives `1..100`, disables
line 0 with AlterEvent(0, 0), so later attempts exit at line 1, and moves the
vein wall item to (-128,-128) with MoveWallObj. Veins start at different tiers:
a single tier (for example map 33 (2,30), map 34 (1,12), map 35 (11,6)) up to
the 5000 tier (maps 39-41).

- `maze0034.evt` @3403, (14,14), all directions: tiers 250, 100, last. The
  recording of the original shows 255, 135, a cave-in, then 17 and a plain wall.
- `maze0035.evt` @2073, (11,6) North: a single tier; its wall item 3 faces West
  (see [original quirks](../original-quirks.md)).
- `maze0035.evt` @2226 and @3665: two copies of the (12,2) West script. Each
  line runs its first match in file order (the search restarts for every line,
  `0x26afa..0x26bf9`, `0x28d37..0x28d3a`), so copy one's 100 tier is followed by
  copy two's line 23, which disables line 0: one payout of 101..200.

A disabled line keeps its record and operands; opcode 0 dispatches to the
default case, which only advances the line (`0x26c50..0x26c61`,
`0x28d30..0x28d3a`). GiveMulti reads each pair's operand as 4 bytes for modes
16, 34, 100, 2 bytes for 25, 35, 101, 106 and 1 byte otherwise (table
`0x2924c`, handlers `0x27bd1`, `0x27c40`).

See [original quirks](../original-quirks.md) for deliberate reference differences
and [evidence](evidence.md) for executable address mapping and research limits.

## Travel and typed input

**Evidence: calibrated DOS static**, the installed English CD executable and
file-offset calibration above, checked 2026-10-16. The two residue/bounds
choices below are explicit maintainer decisions, not DOSBox observations.

TeleportAndExit (7) and TeleportAndContinue (31) read a byte map ID. A nonzero
ID reads signed-byte x/y and keeps facing. Zero consumes the mirror record
selected by ConfirmWord in this Event run, without another prompt. Mirror
direction -1 keeps facing; other directions replace it. Mirror `(0,0)` becomes
x=-1; explicit x=-1 also uses the destination map's run coordinates. Evidence:
`0x27018..0x27025`, `0x2702b..0x270a1`, `0x27095..0x2710f`.

Same-map travel skips map loading (`0x270be..0x270f9`), retaining wounds,
wake state and temporary outdoor-neighbour changes. An actual map change uses
the existing exit lifetime, clears that temporary layer, stages/reloads the
destination monsters and reloads head indicators before destination Events.
The foundation's shared transition result checks admission before candidate
changes. Unavailable travel discards unpublished map, party, rewards and Journey
RNG changes. An already published presentation/reward prefix remains authoritative
under the foundation's publication rule.

Exit terminates. Continue redraws/reselects destination objects and restarts at
physical destination line 0 (`0x27117..0x27127`). It discards CallEvent frames
and resets member selection to 1 (active index 0), all-party selection off,
and damage type to physical: initializer `0x26a8e..0x26ab8`, Call/Return storage
`0x28375..0x283f6`. It never returns to the old caller. Object selection is the
ordinary redraw lookup (`0x170c1..0x171d4`, `0x156ee..0x15839`).
Destination messages, input and other visible suspensions compose the retained
destination world at the interpreter's working camera. Drawing does not publish
the travel camera or candidate effects: a later refusal still discards them,
while an already published reward/presentation prefix retains its authority.

ConfirmWord (21) reads input type, success line, alternate text index and title
index. Type 0 looks up a mirror; other values name primary map text (the
dispatch message, `0x26c26..0x26c48`). Alternate index 0 reuses the primary;
a nonzero index selects that resource string. Title index 0 uses the original
password title; others select map text (`0x27f15..0x27fa6`). Strings are compared
directly, case-insensitively, primary first, without the reference's Vowelless
Knight rewrite (`0x24023..0x24067`, comparator `0xa39f..0xa3df`). A match jumps
to the success line; wrong, empty or cancelled input advances normally.

**Maintainer decision, 2026-10-16:** an alternate-only match also takes ordinary
success. DOS passes an uninitialized local marker and can instead use operand 3
when residue equals 55: `0x27f15` skips `0x27f09`'s assignment, `0x27fba` passes
it, `0x2405e..0x24067` tests it and `0x280cc..0x280e6` branches. The only Clouds
alternate-only use is map 69 `(9,15)` West line 0. See
[original quirks](../original-quirks.md#approved-deviations).

Missing byte-indexed primary, alternate or title text aliases text[0]: DOS clears
all 280 offsets (`0x21d26..0x21d34`) before filling actual strings
(`0x21d40..0x21d81`). An unused slot retains offset zero into the resource.
Map 69's alternate 21 is such a slot; its primary is text 8. MMModern preserves
this alias instead of the reference's empty missing alternate.

Clouds mirror lookup starts at record index 0x26 on physical map numbers above
32 (`0x2406c..0x2407c`), otherwise index 0; result is index+1. The CD
`xeenmirr.txt` in DARK.CC has 62 records of 32 bytes. DOS's inclusive bound
(`0x240d5..0x24118`) permits index 62, reading stale shared scratch beyond the
table; loading does not clear the tail (`0x45ef8..0x45f33`). **Maintainer decision,
2026-10-16:** MMModern bounds by actual records and the original list bound.
Travel reads the selected record directly, including Dragon Tower's map 5
`(10,12)` and preserved facing, without the reference's map-13 correction.

Clouds mirror result 59 (`i lost it`) puts weapon 34, material/state zero, in
the first free ten-slot treasure-weapon entry and exits (`0x28076..0x280ca`).
A full category instead follows ordinary success. The existing reward owner
delivers the sword. Ending/cinematic codes 40, 60, 61 and 62, opcode 30, audio
and further town services remain excluded from M70; Darkside-only operations
and durable banks remain separate.

The shared text input uses original window 6, length 30 and width 200
(`0x24008..0x24014`, `0x1e00f..0x1e179`). Width is formatting, not a character
rejection threshold. It accepts original printable input, ignores leading
spaces, capitalizes the first letter of each word and lowers later letters.
Backspace/Delete removes a character; Enter accepts; Escape clears and returns
an empty answer. Left clicks are consumed without editing/submitting
(`0x1e01c..0x1e021`, `0xe315..0xe328`); right/both-button input maps to Escape
and drains buttons (`0xe0d5..0xe0f6`). Original title/window text comes from the
installed executable. While the prompt owns input, movement/other owners get
no keys; M63's frame, context, focus and queue fences still apply. Cursor
animation is presentation-only and consumes no Journey time or RNG.

Worked original Clouds scripts:

- `maze0028.evt`, `(14,10)` West: line 2 ConfirmWord, line 7 map-zero Exit.
  `vertigo` uses map 28's run location `(18,4)` West; `toad meadow` goes to
  map 23 `(7,2)` and `ever blossom orchard` to map 23 `(5,12)`, keeping facing.
  `magic delta` names map 23 `(9,6)`, in the mainland component connected
  through water. It is admitted along with the other mainland destinations;
  nonzero-map names targeting maps that are not open yet refuse.
- `maze0069.evt`, `(9,15)` West line 0: type 8, success line 4, alternate 21,
  title 10. Primary `There wolf`; alternate 21 aliases text[0]. Either match
  takes line 4 under the approved marker decision.
- `maze0011.evt`, `(13,7)`, all directions: line 3 Continue to `(14,7)` on
  map 11, line 4 Continue to `(12,7)`; restart destination scripts at line 0.
- `maze0028.evt` jail paths, line 16 Continue to map 28 `(27,11)`, keep facing
  and skip a map reload.
- `maze0049.evt`, `(7,14)` North line 1 map-zero Exit is a dead suffix:
  line 0 DoTownEvent(12) unconditionally exits (`0x27b1f..0x27b3b`). It does
  not establish another prompt or a durable mirror selection. The census lists
  it separately from missing mechanics, alongside caller/safety limits.

Travel and input need no durable fields.
Prompt text, mirror selection, call frames and pending rewards are transient;
saving is unavailable while those owners are active. Quiet capture and restore
do not replay an Event, reward, elapsed time or draw.

## Entrance scenes

**Evidence: DOS static and maintainer DOSBox captures (2026-10-20)**,
with the frame/coordinate tables from the pinned `locations.cpp` Clouds
branches. [M80 evidence](evidence.md#entrance-scenes-and-the-arena-2026-10-20)
settles the sites, timing, random draws and DOS corrections.

`DoTownEvent` actions 8-11 terminate the entire Event, including its call
stack. A shared table selects the scene, quest counter and destination:

| Action | Site map | Quest counter | Destination and facing |
| --- | --- | --- | --- |
| 8 Reaper | 12 / 15 / 24 | 3 / 2 / 1 | 55 `(3,8)` E / 59 `(11,8)` W / 51 `(7,12)` S |
| 8 Reaper | 7 / 13 | 46 / 45 | Darkside 113 / 117 `(7,4)` N; always keyless in Clouds |
| 9 Golem | 8 / 12 / 20 | 6 / 5 / 7 | 81 `(1,17)` E / 80 `(29,16)` W / 79 `(5,16)` E |
| 9 Golem | 19 | 50 | Darkside 121 `(18,0)` N; always keyless in Clouds |
| 10 Dwarf Mine | 14 | none | 37 `(1,4)` E |
| 10 Dwarf Mine | 18, x=9 / other x | none | 35 `(1,12)` E / 36 `(7,1)` N |
| 10 Dwarf Mine | 23, x=5 / other x | none | 33 `(7,1)` N / 34 `(7,30)` S |
| 11 Sphinx | 5 | 4 | 82 `(7,5)` N |
| 11 Sphinx | 2 | 51 | Darkside 125 `(7,6)` N; always keyless in Clouds |

A key is held when its party quest counter is nonzero. Clouds keeps only
counters 0-34; indices 45, 46, 50 and 51 are never held (maintainer decision,
2026-10-20). A keyed entrance loads its destination; refusal plays its full
scene, then reloads the current map at the party's own position and facing.
Both use the Event map-load owner: monsters are rebuilt, defeated monsters
stay defeated, and stepped is set. There is no service time charge.
Actions 6 (Arena), 12 (Pyramid) and 13 remain outside Part A and refuse
before effects.

One presenter uses Clouds `tower.vga`/`freap.vga`,
`golmback.vga`/`golem.vga`, `dwarf1.vga`/`dwarf3.vga`/`dwarf2.vga`
and `sphinx.vga`, saving the screen and restoring gameplay on return.
Reaper zooms 13..0, slides from 200 by -16, speaks twice, and retreats
0..13 only on refusal. Golem zooms 11..0, slides from -200 by +16,
speaks twice with its intervening growl, and retreats 0..11 on refusal.
Dwarf zooms 12..0, rises 15..0 and speaks three phases. Sphinx zooms
8..0, speaks three keyed or two refusal phases and retreats 0..8 on refusal.

Explicit waits use the existing 18.2065-Hz BIOS clock: zooms and slides
with explicit waits use one tick, Reaper/Dwarf speech two, and Reaper's
inter-speech pause seven. Dwarf's three-tick phase check uses the phase's
existing mark, not a new pause after speech. Loops without explicit waits
use two 70-Hz retraces per iteration, the maintainer's DOSBox-derived rate.
Each speech phase follows its original VOC duration silently through the
existing duration loader and passive voice-audio notice. No captions appear.
VOC descriptor text does not gate timing: DOS plays the structurally intact
`sphinx13.voc` in full despite its damaged descriptor
([evidence and reference difference](../original-quirks.md#sv-sphinx13-descriptor)).
Effects-off captions wait for audio. The approved omission of the load-time
"Please Wait..." box also applies here.

Frames and jitter consume gameplay randomness. Reaper draws frame 0..4
once per speech iteration. Golem's 13 slide iterations each draw Y 0..9
minus 5, then X 0..9 minus 5; each speech iteration draws Y 0..9 minus 3,
then X 0..5 minus 3. Dwarf draws 0..9 and Sphinx 2..10 per speech iteration.
Direct counts are respectively `N1+N2`, `26+2*(N1+N2)`, and the sum of
speech iterations for Dwarf/Sphinx. Sample durations and acquisition cadence
determine those iterations; ordinary map-load draws remain additional.
The cursor, screens and speech intervals are transient; the save remains
v16, schema 9, content 14.

## Quests and Awards screens

**Evidence: DOS static (2026-10-18), high confidence**; offsets and
calibration are in
[M77 evidence](evidence.md#m77-combat-items-quests-awards-and-skill-indicators).
The pinned reference agrees unless a difference is stated.

**Quests** (`0x3390f..0x3402c`) opens from the main screen's Quests button
(`0x0d4d8`) and from the Items dialog's Quest button (`0x3b40a`), including
in combat. It loads `quest.icn` and the note texts `qnotes.bin` from the
Darkside archive (file set 1) on every open, saves the mode byte and restores
it at exit. Six buttons at y 109, 24x20: Items (12), Quests (80), Auto Notes
(148), Up (216), Down (250), Exit (284), with keys I, Q, A, Up, Down, Escape.
Each page shows nine lines (three for Current Quests) from a top row that the
page buttons reset to 0:

| Page | Source | Lines |
| --- | --- | --- |
| Quest Items (first page) | 85 party quest-item counters, Clouds 0-34 then Darkside 35-84 (DS `0xf077`) | A "Clouds of Xeen" title line before the first Clouds entry and a "Darkside of Xeen" line before the first Darkside entry; items 17, 26 and 79-84 show "N name" with an `s` when N exceeds 1 (`0x34056`); unused lines are "\b \b*" so each row keeps its bullet; no items: "No Quest Items" alone |
| Current Quests | Quest flags 1-56 of the 60-bit party array (DS `0xf06f`), Clouds 1-29, Darkside 30-56 | Note text `flag - 1`; title lines as above; no flags: an empty line, then "No Current Quests" |
| Auto Notes | World flags 1-128 of the 128-bit party array (DS `0xf05f`), Clouds 1-72, Darkside 73-128 | Note text `flag - 1 + 56` (the World edition's offset); title lines as above; no flags: an empty line, then "No Auto Notes" |

Scrolling: Up and Down move one row, Page Up and Page Down three, Home to the
first row, End to the last; the top row stays within `0..count-1`. The DOS
key table (`0x3402e`) has no U or D letters; the pin accepts them in
addition ([quirk](../original-quirks.md#sv-quest-award-scroll-keys)). F1-F6
and other keys are ignored. Nothing passes time. Flag 128 of the Auto Notes
loop reads the bit after the world array, which is quest flag 0; no Clouds
script sets that flag.

**Awards** (`0x36c41..0x37072`) opens from the character sheet's Awards
button for the displayed member (`0x384b0..0x384b6`); it has no member
selection of its own and ignores F1-F6. It loads `award.icn` and the award
texts `award.bin` from the Darkside archive. Three buttons at y 109, 24x20:
Up (216), Down (250), Exit (284), with keys Up, Down and Escape (`0x4f6f2`);
the pin scrolls with U and D instead ([quirk](../original-quirks.md#sv-quest-award-scroll-keys)).
Awards 0-87 are tested through the shared award lookup (`0x3ba44`, the
nibble layout [above](#script-values-conditions-writes-and-random-draws));
each held award adds one line: award 9 formats its text with the full count
byte (Warzone wins), award 17 with the member's race name, the rest their
plain text. With no awards the second line reads "No Awards". The header is
"Awards for %s the %s" with name and class; nine lines show from the top
index, Up moves up one, Down moves down one while nine lines still fit.

All four sources are already in save v15: the 35 Clouds quest counters, the
30 Clouds quest flags, the 128 world flags and each character's 128 award
nibbles with the full Warzone byte. The Darkside counters and flags are only
written by Darkside scripts and read as zero here.
