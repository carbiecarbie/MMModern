# World Events: map changes and Spawn

**Evidence: DOS static**, installed English CD `WORLD/XEEN.DAT`, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
These rules were checked on Windows on 2026-10-15 using bounded 16-bit
disassembly at the file offsets below. The Event dispatcher and signed-byte
operand reads agree with the calibration in [the M70 plan](../milestone-70-plan.md).
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
(`0x2b9d5..0x2ba0b`), and a Warzone load sets it to 0 (`0x0fb90..0x0fba9`);
neither is implemented yet.

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
stacked position. Saves from older builds
that hold pending treasure from a slot past the MOB count load without that
treasure (pending gold bit and items), with a console warning.

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
existing v13 save records carry the resulting positions, opcodes and effective
word/attribute pairs; no format, schema or content change is needed.

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

See [original quirks](../original-quirks.md) for deliberate reference differences
and [evidence](evidence.md) for executable address mapping and research limits.
