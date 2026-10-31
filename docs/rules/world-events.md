# World Events: map changes, script values, travel and input

## Event dispatch

**Evidence: DOS static, high confidence (2026-10-21 and 2026-10-23)**,
calibrated against known movement, door labels, character sheets and saving.
The maintainer also confirmed the mouse rule in DOSBox on 2026-10-23, and
the rubble and gas-trap behavior on 2026-10-27.
See [the offset and calibration record](evidence.md#event-dispatch).
These describe DOS rules; implemented coverage remains in project status.

Exploration checks the party's current cell after each completed outer input.
If that cell has the Event flag, it starts a full Event call, clears WhoWill,
stages pending treasure and scans from line 0, even when no record matches.
Records must match position, requested line and facing (or All). The original
exception at `(0,0)` facing North rejects every record. A quiet frame does not
make another pass. The pass precedes ordinary treasure delivery and the next
idle draw that can start combat.

Closing a character sheet, Items, Info, Quick Reference, Map or Quests completes
the original input's one pass. Item Use stays inside the sheet's input, including
travel. Refused and cancelled actions, blocked movement and unmatched nonzero
keys also complete a pass. A panel visit has one pass when it closes: Save,
Save As, sound options and their acknowledgements are nested responses, however
many saves are made. Mr Wizard returns to the same continuation at its resulting
cell. Successful Load replaces the old continuation; Quit, death and terminal
transitions leave exploration instead of making an ordinary pass.

Title Load, panel Load and New Game load the map, rebuild monsters and objects,
and draw the arrival view before their entry pass. This pass is gameplay after
restoration, not an effect of copying saved state. The approved MMModern F9 Save
has its own following pass; DOS recognizes F9 as an unmatched nonzero key.

| Exploration input | DOS pass rule |
| --- | --- |
| Letters, digits, punctuation, Enter, Tab, Backspace, Escape, arrows | One pass after the action or dialog completes, even without a mapped action |
| F1-F6, F7, F8, F9 | One pass; sheets, unmatched F7/F9, CD replay F8 respectively |
| F10 | Consumed by the temporary DOS-prompt screen; no exploration pass |
| F11, F12, Delete/keypad decimal | No code, no pass |
| Insert, Home, End, Page Up/Down | Nonzero unmatched code, one pass |
| Shift, Ctrl, Alt alone | No pass; Caps/Num/Scroll Lock instead produce unmatched codes |
| Alt plus a key | Same exploration code as the key; MMModern Alt+Enter is a host action |
| Ctrl plus a key | Translated before dispatch: Ctrl+Left/Right strafe; Ctrl+Up/Down are unmatched; Ctrl+F10 is unmatched and does make a pass |
| Keypad | Nonzero translated keys make a pass; keypad 4/6 turn in DOS, and NumLock does not select a second translation table |
| Either mouse button on a button, live portrait or accepted 3D area | Same coordinate-only click; one outer pass |
| Empty panel click, key release, mouse motion, nested dialog response | No independent pass |
| MMModern Alt+Enter, resize, window close | Host actions, no gameplay pass |

MMModern follows these DOS mappings: period is an unmatched key (W waits),
keypad 4/6 turn, and Ctrl+Down does not turn around.
All still have a pass obligation when delivered as outer exploration inputs.
Shift changes punctuation; Ctrl translates the plain code rather than combining
with Shift. For example, Ctrl+I opens the panel and Ctrl+grave produces Space.
Standard keyboard translation is settled; unusual host layouts are not measured.
The pass test uses the low-byte code after the wrapper removes Alt's high-byte
state; a nonzero raw return containing only that Alt state is not an input.

Either mouse button is indistinguishable to this CD build: the callback stores
coordinates only, the reader returns 0 or 1, and the supposed right-click Escape
branch at `0xe0dc` is unreachable. The maintainer's DOSBox check confirms equal
left/right behavior on buttons and in the 3D view.

Icon and portrait bounds are lower inclusive and upper exclusive. The DOS 3D
input rectangle instead includes x 8 through 224 and y 8 through 135. Its outer
manual-interaction test extends through y 140, after a nonzero input exists.
A stationary click on the panel icon's top four rows (y 137 through 140) can
therefore run manual Events before opening the panel. Empty pixels below the
accepted view do not create a code merely because they satisfy the outer test.

Space and a 3D-view click first try the indoor grate/door interaction. A handled
interaction clears the key and skips the manual Event and Wait; a refusal can
leave the ordinary manual route available. Otherwise they call Events manually,
including on an unflagged cell. The loop then dispatches the **current key left
by that call** and finally makes the ordinary flagged-cell pass. With no intervening
input or clearing, Space waits and a 3D click does not: both can run a flagged
cell twice. Teleport can clear Space; a dialog can replace it. A click followed
by a Space or W acknowledgement can consequently wait. The original does not
preserve an unconditional Space-versus-click continuation across every script.

Wait advances time and monsters and clears door text. The monster step moves
actors; a draw rebuilds attack slots. An Event call tests the most recently
built slot 0 before **every line**, including line 0. If occupied, it sets the
skipped-call flag and proceeds to cleanup. Entry still clears WhoWill and stages
treasure; cleanup, the tail below and stash restoration still apply. Each
admitted line scan clears the skipped flag, even without a matching record.
The conditional post-combat callers are separate from the per-input rule.

DOS accepts typematic makes for non-navigation keys as well as movement. It has
a ten-key FIFO; ordinary releases do not remove queued makes, while NumLock
release clears it. Exact keyboard/emulator repeat delay and rate are **Unknown**
from static evidence. MMModern retains the approved
[movement buffering deviation](../original-quirks.md#dev-no-phantom-steps).

Executing an Event does **not** unconditionally flush the keyboard in DOS.
Teleport, map loading, text-page continuation, NPC and treasure presentations
have particular clearing sites; a non-modal script can leave subsequent taps
queued. Keyboard clearing drops pending makes and the current code, not physical
modifier state or future repeats. Mouse draining is a separate operation.
The NPC's pre-wait mouse drain also zeros the current code (`0x25ac7` to
`0xe090`); its keyboard clear at `0x25ad9` runs when more text remains.
The final accepted acknowledgement leaves its new code for the outer dispatch;
mode 2 returns without these waits or clears.

Frames drawn while an Event waits show the script's world changes so far,
including the small map and the border indicators. Indoor map tiles appear
when explored or while Wizard Eye reveals them. MMModern draws these from the
pending Event without publishing it; dialog layers retain their usual order.
The wait redraw is **calibrated DOS static, high confidence**, not a DOSBox
capture of the small map ([evidence](evidence.md#event-wait-drawing)). In
DOSBox on 2026-10-27, the maintainer saw the map 33 gas trap's object while it
hurt the party.

A scene-drawing Event wait consumes the pending step on the detached candidate:
it drains light, marks the occupied cell with an active Cartographer, and
derives darkness. A refused/abandoned candidate publishes none of these changes.
A reconstruction or repeated wait draw cannot spend the same step twice.
Window 11, location artwork and animation-only waits keep the step; text input
checks only when its idle loop actually draws. Follow the
[Event-wait gate table](evidence.md#event-waits-and-pending-steps).
A scene-drawing prompt followed by a teleport marks both cells; a silent
teleport, or one behind an NPC with Text on, marks only the destination.
NPC Text off allows scene preparation during its wait. Mode 2 has no wait and
keeps its art until Event cleanup; later Yes/No, acknowledgment and numeric-choice
prompts that test the location-art handle animate that art and keep the step.
Opening a prompt does not dismiss this layer. Continue checks each intermediate stop.
Entrance scenes check after their reload, then the Event caller marks stepped
again for the later arrival check; on a drain cell these are two drains.

The Event tail has two alternatives, for manual and automatic calls alike:

| State at completion | Result |
| --- | --- |
| No executed marker, a selected object, no other-facing candidate | Window 38: "Nothing here", requiring acknowledgement |
| No executed marker, a selected object, an other-facing candidate, cell Event flag clear | Sound 21 |
| Otherwise | Neither tail effect |

The window does not require Space and does not test the Event flag. Sound 21
is suppressed on a flagged cell. An other-facing candidate must match the
current script position and requested line; an unrelated record is insufficient.
An explicit jump or CallEvent whose target has no record reaches the same normal
Event tail (`0x26afa..0x26b1f`, `0x28d41..0x28d50`); a missing call does not
resume its caller. **Evidence: DOS static, high confidence.**
The marker persists across scans. The executed marker is set by instruction
paths, not merely by finding a record: even a failed If sets it, and CallEvent
sets it before its target scan (`0x283c0`, `0x28a2b`, `0x28b2b`). Zero opcodes,
Exit (`0x28d50`), opcode 47 (`0x28d30`), PlayCD (`0x28b40..0x28b5a`) and the
Clouds ending (`0x28b98`) never set it. TakeOrGive and its test variants set it
only when they continue (`0x2790d`); a failed take ends the Event first
(`0x2788d`, `0x278ff`). ConfirmWord's special mirror results 40, 59 and 62 end
the Event unmarked (`0x2801b..0x280c0`); other results and cancellation mark
(`0x280e9`). A SetChar selection that aborts before its marking path can
leave it clear. The per-opcode sweep is in the external DOS findings of
2026-10-27. Nothing-here acknowledgement consumes input without a general
keyboard flush.

The selected-object byte is one-based: zero means no selection, and MOB record
0 is selectable. The nearest object is selected at the party's logical cell;
searching only the next map cell gives the wrong tail census. MMModern runs
the full call and this tail for manual and automatic calls alike.

The read-only Clouds sweep finds **300 flagged cells**, after rejecting archive
hash aliases and resolving indoor continuation tiles into their owning maps.
There are records at 299; the remaining cell, map 25 `(27,3)`, has no object
selected and produces neither tail effect. Of 135 flagged cells with objects,
18 have a line-0 record for only one facing: their other 54 facing cases have
neither window nor sound. The other matched paths set the executed marker,
directly or in their called script. Thus unchanged resources with attack slot 0
free supply no flagged-cell tail-window witness, and the flag suppresses the
tail sound. This corrects the earlier 376/187/189 census, which mixed archive
hash aliases and physical DAT coordinates with logical Event coordinates.

**Unknown:** an exhaustive list of reachable tail windows after scripts mutate
Events or objects, or when combat makes a call skip its scan. Static predicates
do not establish every reachable saved state. A focused DOSBox check can watch
attack slot 0 while waiting on an object-bearing flagged cell, then verify
whether the skipped call shows window 38 before combat resumes.

Combat can start inside any ordinary draw whose admission tests pass: a held
slot among 0-2, mode 1 or 5, neither missile gate held, and exploration enabled.
Matching Event instructions use mode 9, so their draws can build slots without
starting combat. Spawn changes actor records; it issues no draw and does not build
slots. In DOSBox on 2026-10-27, the maintainer searched the map 33 rubble twice:
the monsters appeared only on the first search and the second reported nothing.
TeleportAndContinue draws before restarting at line 0; label draws and the
fall/restart path can also expose contact to the next line-head test. Map load
and the restart alone do not rebuild slots. Completed indoor and outdoor builds
pack contact into slots 0, 1, 2 in order; no ordinary outside-combat writer was
found that leaves only slot 1 or 2 occupied.

There are two direct combat entry sites: the ordinary draw and exploration
Shoot. Event-started fights and Arena generation eventually use the draw site;
Arena does not call combat directly. **Caller 4** belongs to combat's return:
if door text was up at entry and the current cell is flagged, call Events there.
**Caller 3** belongs to the draw after combat returns: if the skipped flag is
still set, call Events at the current position, without a cell-flag test.
Direct Shoot has no caller-3 suffix. Caller 4 precedes caller 3; a scanning
caller 4 clears the flag and suppresses caller 3, while a skipped caller 4 does
not. Neither caller uses the original fight address after relocation.

| Combat result or origin | Continuation and Event calls |
| --- | --- |
| Win; partial Run followed by win | Normal return at the fight cell; partial escape does not relocate the winners. Conditional caller 4, then caller 3 for draw entry. |
| Full-party Run | Relocate to the current map's Run coordinates before the normal return; callers test that landing and its current facing. |
| Defeat | Not a blanket caller exclusion: the exit still reaches the conditional caller-4 test and returns to the entry site. The early no-active-character branch bypasses the normal redraw/treasure prefix. Exploration's death exit does not owe a surviving-input pass. |
| Combat panel Load / Quit | Load is refused in mode 2 and leaves combat running. Confirmed Quit terminates the program without either caller. |
| Combat relocation | Learned Town Portal, Lloyd's Beacon and Teleport, and item Beacon/Teleport, are refused. Item Town Portal continues despite its refusal notice. Time Distortion is allowed. Permitted relocation returns to the combat loop; it does not itself force combat retirement. Later slot rebuilds and exit tests decide whether fighting continues; eventual callers use the resulting position. |
| Mr Wizard in combat | The panel zeroes the gems and combat ends at once. After an earlier escape with contact remaining, the party first lands on the current map's Run position and disabled members are abandoned, as in attrition, without forfeiture. Caller 4 runs at that position, then combat loads Vertigo facing East at its Run coordinates, and a draw-origin caller 3 calls there if the skipped flag remains. The load builds no attack slots, so caller 3 tests the slot 0 last built at the fight cell or landing: at the fight cell the monster usually still holds it, so caller 4 and caller 3 both skip and the flag stays set. A scanning caller 4 suppresses caller 3. |
| Idle-draw origin | Return to the exploration input reader; no extra ordinary pass before that next input. |
| Draw inside a surviving input handler, or direct Shoot | Resume that handler, then its one ordinary pass before the next outer input. A manual Event resumes current-key dispatch, so an admitted fight can take control before Space's Wait. Terminal returns retain their established exclusions. |

Normal combat completion closes the combat window, clears queued keys, redraws
in combat mode and calls treasure delivery before caller 4. Delivery still
defers while any attack slot is occupied: mode 2 does not bypass its entry
gate; only Event mode 9 does. Deferred treasure stays staged and does not
prevent caller 4. Caller 3
follows return to the originating draw. Ordinary delivery after an exploration
pass is later, at that pass's existing delivery site. Arena rewards are issued
by the subsequent Arena service interaction, not by a special combat-return
callback; its monster-treasure exclusion remains the M81 rule.

The exploration input reader has no independent occupied-slot guard. If a
draw's combat gate is closed, it can reach input with contact still recorded;
Space and view clicks retain the manual/current-key rules above. Combat entry
does not unconditionally clear keys queued since the monster step: combat can
consume them. Particular dialogs can clear them, and ordinary combat exit does
clear them. Exact projectile-frame input timing remains **Unknown**; a DOSBox
check should record the gates, queued key and next reader at an engaged draw.

In DOS, a skipped call's window-38 draw can start combat and its callers before
acknowledgement and before that call restores its stash. The pinned ScummVM
also draws before acknowledgement and restores saved treasure afterward.
**Approved deviation (2026-10-23):** MMModern acknowledges the skipped call's
window and finishes the call before starting combat, for automatic and manual
calls alike. The nested-stash outcome is deliberately outside this evidence.

Door text is set by label opcodes 2, 3, 4 and 39. Its drawing requires the flag
and an empty slot 0. Movement/turn completion, Wait, gameplay entry and combat
entry clear it. Text and numeric input temporarily clear it and restore the
saved value. Dialogs, Rest, stationary Bash, item Use and map load have no
generic clear. Cast and Shoot clear it when their completed exploration action
takes the movement/terrain continuation; refused/cancelled paths do not gain
that clear. Bash that moves uses the movement clear. Travel can clear the text
buffer without clearing this flag. Combat saves the entry value only to decide
caller 4; it never restores the flag. A label executed afterward sets it anew.

| Boundary | Skipped flag | Door-text flag |
| --- | --- | --- |
| Save / Load payload | Not saved or restored | Not saved or restored |
| Gameplay entry after Load or New Game | Kept until a scan clears it | Cleared before the first view |
| Map load within play | Kept | Kept; text resources/buffer can change separately |
| Return to title in the same process | Kept | Kept until gameplay entry clears it |
| Fight without caller 3 | Kept unless another Event scan clears it | Cleared at combat entry; only later writers change it |
| Fresh process | Zeroed with runtime globals | Initially zero |

**Static Clouds witness, high confidence for movement and dispatch:** Vertigo
`(13,4)` West is flagged and has a line-0 label. Original monster 35 starts at
`(15,4)`. Once awakened by viewing it, its ordinary AI can move west through
`(14,4)` onto the party at `(13,4)`; both source-wall checks pass. The labelled
western wall does not prevent entry from the east. A Wait can make that last
move while slots still describe the preceding draw. The pass executes the
label, its mode-9 redraw fills slot 0, and the next line-head test skips. Combat
then saves the label flag, providing caller 4 after a win. This uses no placed
actor. After Run, caller 4 instead tests the landing: Vertigo `(18,4)` has its
label facing East, so retaining West does not redraw that label. The precise
interactive setup has not been played in DOSBox; watch the original actor,
slot rebuild and line-head test to reproduce it without altering actor state.

## Map and script instructions

**Evidence: DOS static**, installed English CD `WORLD/XEEN.DAT`, SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`.
These rules were checked on Windows on 2026-10-15, the Battle Master and
Warzone generation on 2026-10-20, using bounded 16-bit disassembly at the
file offsets below. The Event dispatcher and signed-byte
operand reads agree with the calibration in [the M70 plan](../archive/milestone-70-plan.md).
They establish code paths, not a claim of DOSBox observation.

| Operation | Rule and original file offsets |
| --- | --- |
| MoveWallObj (34) | Read slot, signed x and signed y; replace position only. Repeating the same position does not flip the wall item. `0x2860e..0x2863a`. |
| MakeNothingHere (46) | Clear every opcode at the party's physical cell, across all directions and lines. CallEvent changes the logical execution cell, not this deletion cell. Continue with ordinary line advancement: `0x2793b..0x279ec`, `0x28356..0x28372`, `0x28d30..0x28d3a`. The reference exits instead. |
| AlterMap (19, census name AfterMap) | Signed x/y, direction, wall nibble. Directions 0–3 select north/east/south/west. Direction 4 writes all four faces of that cell; no opposite face is implicitly changed. Resolve the physical DAT first. `0x27b41..0x27ba3`, helper `0x234dd..0x23550`. |
| AlterCellFlag (35) | Signed x/y and surface. Outdoors replace the low four wall-word bits; indoors replace the low three attribute bits, preserving the other bits. Indoor coordinates are masked to 0–15 after neighbour selection, so logical `(25,5)` aliases physical `(9,5)` in the east DAT. `0x2863d..0x28721`. |
| AlterEvent (24, census name AfterEvent) | Scan physical cell, current/all direction and selected line, replacing every matching opcode, including with a nonzero value. No match is a no-op. The line operand is sign-extended against lines stored zero-extended (`0x2827f..0x28287`; index builder `0x21e75..0x21e7e`), so operands 128-255 never match. If the effective matched opcode is PlayCD (60), also replace the next record in storage order. `0x28189..0x28356`, paired-record branch `0x282c0..0x28304`. The reference lacks that paired-record branch. |
| Spawn (16) | For a slot below the map's MOB monster count: reuse the slot's type and bind its MON statistics; write coordinates, physical damage status, asleep/not attacking and full MON HP. A revived slot loses its defeated/accounted state. `0x27a59..0x27b1c`; random animation-frame draw `0x27a85..0x27aa1`; no 3D draw is issued. A slot at or beyond the count is written in DOS but stays inert, as described below. |

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
never stores it. Vertigo's exit script spawns slots 50 and 51 past its 46
records, so they stay invisible. Only the Warzone arena prompt changes the
count (`0x2b9d5..0x2ba0b`), and a Warzone load sets it to 0
(`0x0fb90..0x0fba9`).
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
sets, only operations that reach these writes count. MMModern's Event tail
reads this marker; see [Event dispatch](#event-dispatch).

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

Loading and leaving Clouds maps also apply fixed scripted rules. They are
recorded with their evidence in original quirks: the
[lake dragons](../original-quirks.md#sv-lake-dragons-away),
[drained caves](../original-quirks.md#sv-drained-cave-maps),
[quest-completed object hides](../original-quirks.md#sv-quest-done-objects),
Newcastle's castle graphics in the same table, and
[Castle Xeen's removal after a win](../original-quirks.md#sv-clouds-castle-removal).

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

**Items and treasure.** Give 21 stages a weapon, armor or accessory by ID in
the first empty slot of the shared treasure arrays (a full category fails the
give); a quest ID first increments its counter, then shows as a weapon
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

**Locked chests.** When an object is selected (any object, the map's first
included: the selection byte `0x98d9` is one-based and giveExt `0x23597` skips
only when it is zero, calibrated 2026-10-23 in
[Event dispatch evidence](evidence.md#event-dispatch)), its sprite is
16, 58 or 73 (15 too on Darkside, table `0x235c2`) and no lock was picked yet
this run, giveExt opens the chest (frame 1), then draws `1..4`; on 1 it draws a
type `0..6` and deals the map's trap damage to the recipient as ordinary typed
damage (`0x23618..0x23644`). It then draws `1..20`: thievery plus the draw at
least the map's box difficulty picks the lock, adds level x difficulty x 10
experience (a 16-bit product) and shows "picks the lock"; otherwise the chest
closes, "was unable to pick the lock" is shown and the Event ends
(`0x23647..0x23726`). A test against 20 that follows the sprite table reads
the same sprite byte and so never skips the lock; the reference reads it as
the object's x position. A pick plays FX 10, a failure FX 21. giveExt sets the
object's frame byte to 1 when it starts and to 0 after a failed pick
(`0x235f6`, `0x23703`); the lid frames and the opened chest follow.

**Opened chests (M96).** Every Event run ends with the defeat check and the
treasure delivery (`0x28d6b`, `0x28d70..0x28d8d`). Then, while a picked
lock's counter `0x9755` is positive and an object is selected (`0x98d9`), the
selected chest changes (`0x28d92..0x28e87`): sprite 16 (15 on Darkside) takes
the first of the map's 16 picture-table slots that holds sprite 62 (57 on
Darkside), and sprite 73 the first that holds 119. The table index and the
sprite change together (`0x28e04`, `0x28e1f`, `0x28e69`, `0x28e77`); without
such a slot nothing changes, and a 16 or 15 never falls through to the 73 test;
sprite 58 is never changed. The counter then returns to zero (`0x28e87`),
before the run's tail and "Nothing here". A pick increments it (`0x23673`); a
failed pick ends the run with it at zero (`0x2371b`, `0x27eaf..0x27ebb`); each
run starts with it at zero (`0x26a96`), and so does each TeleportAndContinue
restart (`0x27127` to `0x26a8e`). GiveMulti mode 67 would set it to one
(`0x27ed8`); no script uses that mode and it stays not supported yet. The rule
draws nothing from the random stream. The index belongs to the record: the map
writer saves it with x and y (`0x26698..0x266ce`), the loader rebuilds the
sprite from it (`0xf989`, `0xf9b1`), and MoveObj and Remove write only x and y
(Remove `0x27913..0x27937`), so a picked chest stays open through MoveObj,
Remove, leaving the map, Save and Load; a later run on it sees the open sprite
and picks no lock. DOS draws its picture from the index (indoor
`0x1577e..0x1578c`, outdoor `0x17119..0x17127`) and its frames and flip from
the sprite (`0x10d0e..0x10d21`, `0x10d87..0x10db6`); both name the open sprite.
37 Clouds chests on 18 maps can be picked open; 12 trap chests never are.
The selection is the lowest-index object on the party's own cell, rebuilt at
every draw3d: the object builders clear it, then take the first object whose
position is the party's while the near draw slot is still free (indoor
`0x156f0`/`0x15836`, outdoor `0x170c3`/`0x171d1`; the position tables name the
party's cell for every facing, and draw3d frees the slot first, `0x111e8`,
`0x10dd9`). The object need not have a picture: on Clouds only map 47's
record 0 at (8,0) has none, no Event record lies on its cell and no lock is
picked there, so a search there shows "Nothing here"
([picture-less selection](../original-quirks.md#sv-pictureless-selection)).
MMModern selects once per run by the same rule.
MMModern applies the rule once per run on its
detached candidate, so a refused run changes nothing, and refuses a run as not
supported yet when, with the counter positive, it reaches a teleport other
than a same-map TeleportAndContinue, a map change, a Town service, a Remove of
the selection or any MoveObj; ExchObj is outside the supported subset in every
run. After any of these DOS's next draw3d could select another object. None
can follow a pick in the 41 Clouds runs that can pick a lock (the 37 chests
and map 80's four sprite-58 objects), no other object can stand on those
cells, and a TeleportAndContinue draws before its restart (`0x27121`), so the
restarted run's selection is the new cell's, as in MMModern. Witness: in the
maintainer's DOS save, `maze0009.mob` object 1 at (15,11) went from index 3
(sprite 16) to index 4 (sprite 62).

**Lid frames.** draw3d updates every object's frame byte before it composes
the scene (`0x10bc3`: interface `0x10bda..0x10bf4`, objects
`0x10c32..0x10dc7`, then the scene from `0x10dcf`). The byte goes up by one
(`0x10c83`). While the counter is positive the selected 16 (15 on Darkside),
58 or 73 cycles frames 1..4, and a 58 holds frame 1 (`0x10c8a..0x10cf6`).
Otherwise the frame stays while the view's end frame is at least the frame and
returns to the view's start frame past it (`0x10cf9..0x10d55`). Every chest
sprite is static in each view (CD `clouds.dat`), and so are 58 and the open
62 and 119: a picked lid cycles 1..4 until the run ends, and the opened chest
shows the open sprite's start frame from the next draw, which the main loop
makes right after the run (run `0xd1a5`, treasure `0x24128`, draw3d `0xd1c3`);
after a failed pick the closed chest counts up to its start frame or returns
to it. MMModern keeps this byte only for the object giveExt framed, applies
the update once per animation tick before the composition, and drops it once
the object shows its sprite's ordinary frame, which these static views keep
for good; the composition right after a run that opened a chest updates it
first, as that draw3d does. Leaving the map and Load drop it too, and it is
not saved, as in DOS.

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
HowMuch (a take of value 0), take 8 and give 82 (damage), an HP grant through
GiveMulti, take 25/76 and give 25 (time going back), SetVar 3..5, SetVar
25, 74, 76 or 84 with an out-of-range value, Misc item grants by give 21, a
non-caster's spell test, grant or removal (DOS uses whichever book the
program selected last, global `0xf604`), SetChar 8..127 (WhoWill), Darkside
character flags (107) and GiveMulti 67. Every skill grant and removal is
supported: it only writes the skill byte, which the save keeps. Arms Master,
Direction Sense, Merchant, Mountaineer, Swimmer, Tracker, Spot Secret Doors
and Danger Sense are granted and saved. Direction Sense, Spot Secret Doors
and Danger Sense light their border indicators
([rules](magic.md#border-and-information-screens)), Merchant sells at full
value, Swimmer lets the whole party onto water and two Mountaineer holders
pass outdoor wall types 1/7/9/10/12
([rules](time-and-conditions.md#outdoor-movement-skills)). Arms Master and
Tracker have no passive effect in DOS, as in the reference (**DOS static,
high confidence**, 2026-10-19): the party skill check `0x215b9..0x21609` is
never called with skill 1 or 15, and the traced to-hit path
`0x2ded8..0x2e1c0` reads no skill byte
(`D:/Projetos/MModern/MModern-re/skills-20261019/findings.md`).

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
under the foundation's publication rule. Its light word tracks successful Event
Set/Take/Give operations (including the year reset), independently of detached
scene drains. Repeated prefix publication therefore preserves Event effects
without leaking a drain from a subsequently refused run; the final candidate
publication retains the actual checked light word.

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
delivers the sword. Results 40 (`showtime`) and 62 (`bloopers`) and opcode 30,
which ends Clouds, follow the
[ending entry rules](cutscenes.md#clouds-ending-entry-and-save). Results 60 and
61 (the Darkside and World endings) and town action 13 refuse with a
not-supported notice. Action 12 follows the Pyramid crossing below.

The shared text input uses original window 6, length 30 and width 200
(`0x24008..0x24014`, `0x1e00f..0x1e179`). Width is formatting, not a character
rejection threshold. It accepts original printable input, ignores leading
spaces, capitalizes the first letter of each word and lowers later letters.
Backspace removes a character; Enter accepts; Escape clears and returns
an empty answer. This CD keyboard table gives Delete no code. Either mouse
button is consumed without editing/submitting (`0x1e01c..0x1e021`,
`0xe315..0xe328`); the supposed right-Escape branch is unreachable, as the
[Event dispatch audit](#event-dispatch) establishes. Original title/window text comes from the
installed executable. While the prompt owns input, movement/other owners get
no keys; M63's frame, context, focus and queue fences still apply. Cursor
animation is presentation-only and consumes no Journey time or RNG.

Worked original Clouds scripts:

- `maze0028.evt`, `(14,10)` West: line 2 ConfirmWord, line 7 map-zero Exit.
  `vertigo` uses map 28's run location `(18,4)` West; `toad meadow` goes to
  map 23 `(7,2)` and `ever blossom orchard` to map 23 `(5,12)`, keeping facing.
  `magic delta` names map 23 `(9,6)`, in the mainland component connected
  through water.
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

A key is held when its party quest counter is nonzero. The four entrances
using counters 45, 46, 50 and 51 remain deferred: a held key gives a
"not supported yet" notice before presentation or publication. Without
those keys the original Clouds refusal scene still plays. An admitted keyed
entrance loads its destination; an ordinary key refusal plays its full
scene, then reloads the current map at the party's own position and facing.
Both use the Event map-load owner: monsters are rebuilt, defeated monsters
stay defeated, and stepped is set. There is no service time charge.
Action 6 is the Battle Master described above; action 12 is the Pyramid
crossing below, while action 13 refuses before effects.

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
existing duration loader and passive voice-audio notice; with Effects off the
VOC wait is skipped and the caption appears instead (M82, see
[audio](audio.md#efx-voc-waits-and-entrance-captions)).
VOC descriptor text does not gate timing: DOS plays the structurally intact
`sphinx13.voc` in full despite its damaged descriptor
([evidence and reference difference](../original-quirks.md#sv-sphinx13-descriptor)).
The approved omission of the load-time
"Please Wait..." box also applies here.

Frames and jitter consume gameplay randomness. Reaper draws frame 0..4
once per speech iteration. Golem's 13 slide iterations each draw Y 0..9
minus 5, then X 0..9 minus 5; each speech iteration draws Y 0..9 minus 3,
then X 0..5 minus 3. Dwarf draws 0..9 and Sphinx 2..10 per speech iteration.
Direct counts are respectively `N1+N2`, `26+2*(N1+N2)`, and the sum of
speech iterations for Dwarf/Sphinx. Sample durations and acquisition cadence
determine those iterations; ordinary map-load draws remain additional.
The cursor, screens and speech intervals are transient and never saved.

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

All four sources are in the current save format: 85 shared quest counters,
both 30-bit quest-flag banks, the 128 world flags and each character's 128
award nibbles with the full Warzone byte. The journal reads both sides.

## Two-sided World state and ordinary Darkside maps

**Reference and DOS resource layout:** Event mode 20 selects one of two
256-bit game-flag banks by camera side; mode 104 selects one of two 30-bit
quest-flag banks. Quest item IDs 82..166 address one shared set of 85
counters. Megacredits is counter 26. Mode 107 addresses 32 flags on the
selected roster record, shared across sides and independent of party order.
The annual reset always clears Clouds game flags 55, 155, 222 and 231.
The 128 world flags remain shared, including the Auto Notes 128 alias above.

**Calibrated DOS static, high confidence (2026-10-30):** ordinary Darkside
maps select `dark.dat`, `dark.mon` and `darkpic.dat` at `WORLD/XEEN.DAT`
`0xf6fd..0xf778`. The installed New Game archive has 113 ordinary main-map
DAT/MOB/EVT triples (maps 1..112 and 129); maps 113..128 use mixed resources
and are refused. Object/picture and monster art follow the loaded side;
World species 91 uses Clouds art, with Darkside statistics
(`0xffeb..0x10010`). `darkpic.dat` is a sprite resource; the pin loads it
but does not consume it. Picture animation uses the actual picture's frames.

Castleview's Event at map 29 `(30,20)` temporarily forces text even when
the Text option is off (`0x26a61..0x26a88`, restoration
`0x291cf..0x291d8`). MMModern carries this on the Event presentation,
leaving the durable option unchanged.

**Pyramid: DOS static, high confidence.** action 12 bypasses the
ordinary service scene and return (`0x1cea5..0x1ceba`); crossing
`0x295f2..0x29679` has no separate scene, speech, hold, time cost,
character-side write or Pyramid-specific draw. The normal destination
load still selects music. Clouds map 49 leads to Darkside 52 `(2,2)` North;
other Clouds Pyramids lead to Darkside 29 `(25,21)` North. Darkside 52
leads to Clouds 49 `(7,14)` South; Darkside 29 to Clouds 23 `(8,10)` North.
The caller terminates the source script, then closes windows and settles
treasure (`0x27b1f..0x27b3b`, `0x28d50..0x28f22`). The next ordinary input
performs the destination Event pass; no recursive pass is made on crossing.
Action 12 uses the existing detached travel/arrival path and the Town
handoff preconditions. It publishes once, retains visited history, marks
arrival stepped and clears input once. Darkside arrival selects one map
song; Clouds arrival selects none. Failure before publication retains the
committed source. No disc-change prompt or wait is shown (approved
[deviation](../original-quirks.md#dev-no-disc-prompt)). Darkside town services
remain refused before their gameplay publication; their values and return
music belong to M102.

## Opening doors and service borders

**DOS static, high confidence for control flow (2026-10-23):** an admitted
indoor forward move through front-wall type 4 or 13 requests FX47, draws the
whole open-door view and holds at least two BIOS ticks before the step
(`0x0cf93..0x0cfb7`, `0x11873..0x11880`). The frame mapping follows the pin,
not confirmed directly in DOS: the nearest `fwl2` door uses frame 10 for
city wall kind 0, frame 1 otherwise, and its second sky layer uses the open
day/night sky. Blocked, backward, strafe and outdoor movement skip this view.
Keyboard and 3D movement share the admitted movement path.

Services clear Levitate, Spot Secret Doors and Danger Sense animation counters
and statue frames at setup (`0x1d4c4..0x1d4d2`) and on a full background
animation redraw (`0x1c402..0x1c410`, guarded by its redraw argument).
Assembly draws statues 0/8 and advances the three zeroed counters once.
There is no additional exit reset; returning to the map keeps the last
assembled phase. These are disposable presentation values.
