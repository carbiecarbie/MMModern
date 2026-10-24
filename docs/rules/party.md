# The Inn, membership and party size

Checked against the English World of Xeen CD program on 2026-10-24, at
MMModern `e862107` (Create/Delete); the retained M87 checks are identified below.
This describes DOS rules, not implemented coverage.
**Evidence: DOS static, high confidence**, except where labeled otherwise.
File offsets refer to `WORLD/XEEN.DAT`; calibration, provenance and runtime
limits are in [M87 evidence](evidence.md#m87-inn-and-party-size) and
[M88 evidence](evidence.md#m88-create-and-delete). Create and Delete share
the Inn's persistence boundary; their record and screen rules are below.

## Roster and list

**Resource inspection, high confidence:** the initial `maze.chr` has 30 records
of 354 bytes. Named records are 0 Arturius, 1 Rebecca, 6 Seymour, 11 Zippo,
14 Badger and 18 Tyro. Each has side 0 and saved map 28. The other records
have empty names, side 0 and saved map 0. CHR byte 339 is an unknown byte;
bytes 340–341 are the little-endian lodging-map word, not a byte at 341.
DOS writes that word at `0x1ecae`. The pinned reference instead groups
339–340 as an unknown word and reads 341 as the map; MMModern follows DOS
([layout difference](../original-quirks.md#sv-chr-lodging-layout)). Initial active order is
`{0,18,14,11,1,6}`. Side byte 0 denotes Clouds and 1 Darkside; this chapter
does not certify Darkside play. Clouds supplies `char01.fac` through
`char24.fac`, but not `char25.fac` through `char30.fac`.

The Inn scans roster IDs 0–23, requiring a nonempty name, the current side
and a saved map equal to the Inn's town (the party's prior map). Records
24–29 cannot be recruited even if given names. The list is in ascending
roster-ID order, not party order. Active members remain listed, with colored
“In Party” replacing their names; race, sex and class remain visible.
Offsets: list `0x39576..0x395d5`, member marker `0x38e9c..0x38f32`.
**Reference comparison:** the pin lists named characters by side without
the DOS town filter; both restrict ordinary recruitment to the first 24 slots.

Four list faces are shown. Face slot `j` maps to `list[start+j]`, then that
roster ID's portrait (`0x38ffa..0x39090`). Up and Down move four entries;
the last page starts at `4 * floor((listCount-1)/4)`, or zero for an empty
list (`0x398bb..0x398d4`). Empty rows are blank and unclickable; there is no
separate empty-list message (`0x38e40..0x38e86`, `0x38fdc..0x38fe6`).

Membership consists of distinct recruitable roster IDs in the chosen order,
with at most six members. Remove can leave zero inside the Inn; Exit requires
at least one. Recruitment has no health-condition gate. Six physical character
lanes do not imply six active characters; the consumer bounds are below.

## Sign In and time

**Resource inspection + DOS static, high confidence:** these are the Tavern
action-3 placements and their Sign In exits. Maps 29, 31, 32, 73 and 74 have
no Tavern placement; unused exit-table entries are not additional taverns.

| Map | Tavern Event cell | Exit cell | Facing | Evidence |
| --- | --- | --- | --- | --- |
| 28, Vertigo | (24,5) | (18,4) | West | `maze0028.evt` offset 16, line 1 after CallEvent; exit table `0x506ca` |
| 30, Rivercity | (24,16) | (19,16) | West | `maze0030.evt` offset 7, line 0; exit table `0x506ce` |
| 49, Shangri-La | (14,13) | (11,12) | West | `maze0049.evt` offset 45, line 0; explicit override `0x1b723..0x1b734` |

The Tavern's S dispatch enters Sign In directly at `0x1b6b0`. It does not
apply the selected-member action check used by Drink. Dead, Stoned or
Eradicated members do not cause refusal; an entirely defeated party remains
subject to the ordinary defeat flow before it can reach the service.

Sign In sets the exit coordinates/facing and prior map, writes the current
town to each active CHR's saved-map word at `+0x154` and the side to `+0x12`,
then adds 1440 minutes and sets current map to 0 (`0x1b6b0..0x1b794`). The
subsequent forced Save copies these active records into the lodged roster.

The time addition advances the calendar by one day, preserving time of day.
When the final day satisfies `day % 10 == 1`, merchant regeneration runs
once, then bank interest once; exactly 1440 minutes does not qualify for
the separate longer-call rule. See [calendar economy](time-and-conditions.md#calendar-economy-and-direct-year-changes).
In Tavern mode 17 the dawn path skips temporary-effect clearing and fatigue.
Before 05:00 the new-day latch remains pending; at or after 05:00 it is
consumed without those clears. This is `addTime`, not the condition-ticking
`changeTime` path: no food consumption, Rest, HP/SP recovery or eight-hour
condition/death checks occur (`0x263b3..0x26491`). Consequently this addition
cannot itself kill the last viable member through those omitted checks.
See [calendar](time-and-conditions.md#calendar-and-kinds-of-time-advance)
and [dawn](time-and-conditions.md#dawn-and-temporary-effects).

## Saves, Load and return

Sign In performs a forced Save: `0x1d7a4` calls `0x1ebc2` with argument 1.
The saved state has current map 0, the prior tavern map, its corrected exit
position/facing, the post-1440-minute calendar, and membership as it stands
before the Inn opens. The full roster is saved, including the active records
just lodged. Loading this state does not add another day.

Ordinary panel Save passes argument 0 and has an outer combat refusal
(`0x1f1cb..0x1f1e5`). Forced Save bypasses the map restriction, progress
prompt and final acknowledgement (`0x1ebc8..0x1ec2a`, `0x1ed46..0x1ed98`).
Both share the live-party preservation, roster reload/copy, map flush,
snapshot and DOS disk-reset path. Forced Save is not merely an in-memory copy.

The snapshot writes `maze.nam`, `maze.chr` (10620 bytes) and `maze.pty`
(1530 bytes) into `DARK.CUR`, then copies the working archive to the selected
`DARKnn.WOX`. Normal World play also copies `XEEN.CUR` to `XEENnn.WOX`
(`0xe938`, `0xec85`, `0xe9b8`). Thus it affects both working archives and
numbered user-save files. The slot is the existing selected number; forced
Save opens no slot chooser. A missing numbered file can be created. There is
no separate unusable-slot fallback in this path: open failure terminates
through `0x21ecd`. Archive seek/write carry errors reach `0x45c8a..0x45cfd`
and terminate with status 2. Some short writes are unchecked: the archive
writer does not compare a successful write's byte count, and the archive-copy
loop does not check its write result (`0xedb1..0xee09`). These paths do not
provide an atomic-save or rollback guarantee; physical failure outcomes need
the [runtime checks](evidence.md#m87-runtime-checks).

Load reconstructs the roster and active members from the working save, then
tests current map 0 and opens the Inn (`0x39460..0x394bc`). Exit restores
the prior map, retains the saved exit coordinates/facing, copies the active
party to the roster and **saves again** (`0x39b1b..0x39b2b`). Add and Remove
alone do not snapshot; their changes reach the numbered slot on Exit, before
the next manual Save. Confirmed Delete (`0x39a2e..0x39a32`) and return from
Create (`0x39a96..0x39a9a`, including return without creating a character)
also snapshot while map is 0. Those M88 paths can capture earlier membership
changes, including an empty party; the ordinary Exit path refuses zero.

The call order is farewell voice and completion wait, prior-song restoration,
forced Save, service return and Event completion, closing scroll, then Inn
loading/song, screen/portraits and opening presentation. Relevant offsets:
`0x1d688..0x1d6cf`, `0x1d763..0x1d7a4`, `0xd5de..0xd60f`,
`0x394d4..0x394f4`, `0x39738..0x39745`.

**DOS static, high confidence (2026-10-24):** the Clouds scroll uses 22
steps in either direction. Each step resets the BIOS counter before restoring
the saved screen and drawing; it waits for a nonzero elapsed tick before
flipping (`0xadba..0xaebd`, `0xaecc..0xafd2`, counter routines
`0x440cd..0x440f4`). Opening uses hand/marble indices 0..21; closing uses
22..1 and leaves the fully closed image. The Inn first calls
`fadeIn(0x81)` (`0x39738`): `0x3fe2c..0x3fe71` skips the first palette write,
then restores the full palette at one VGA retrace (`0x3fe76..0x3fe81`),
without a gradual fade. Both scroll calls pass false for their own fade flag.
MMModern uses the existing BIOS/retrace presentation clocks and composes the
opening's Inn underlay once. After each new step's acquisition its next deadline
is `max(after(previous deadline,1), after(acquisition,1))`. A stall longer than
a tick loses that tick; two steps never appear in one BIOS tick. Re-presenting
the same acquired frame does not restart its wait. The death closing scroll
uses this same DOS routine and rule.

After each flip the loops poll input (`0xaea1..0xaeb4`, `0xafbd..0xafcb`). Any
key or click jumps to `0xafd5..0xb01c`, which draws and flips the final image
without a timed hold: the Inn background for opening, hand00 and the first
marble for closing. The closing final image is acquired before Inn loading and
opening. The interrupting key does **not** reach the Inn menu: `0xe149` calls
the consuming reader `0x45436` (queue/current-code update at
`0x4549c..0x454e6`); the Inn polls again at `0x39893` before reading the new
code at `0x3989c`. Clicks are popped by `0x43877..0x4389a`. The scroll tail's
`0xe6bf` restores button context, not the interrupted input. No blanket keyboard
flush is implied; later queued input is separate. These skip rules also apply
to the death scroll. **Unknown:** exact audiovisual overlap and partial
framebuffer updates within DOS drawing; static waits do not specify those.

**DOS static, high confidence (2026-10-24):** the Inn Items dialog refuses all
F1–F6 keys, including member switching without a selected item: the map-0 test
at `0x3b4de..0x3b4e5` precedes member lookup and selection/transfer branches.
The character sheet itself has a separate F-key handler at `0x3a4f9..0x3a546`
which permits switching among active members. Inn entry calls that sheet at
`0x39bf1..0x39c0b`. Item Use retains its map-0 hurry refusal.

Developer sessions without a save target run the same deferred Dismiss lodging
publication at Sign In and Exit, then make no snapshot or write. Departed
members therefore appear in the Vertigo Inn just as with a current save slot.
This does not enable ordinary saving in a developer fixture.

The service returns status 2. Its Event caller goes to the shared completion
tail, rather than executing later Event lines (`0x27b1f..0x27b3b`). After Inn
Exit, gameplay entry unconditionally loads the map and rebuilds actors, even
for the same map (`0xd086..0xd0d3`), then reaches the ordinary flagged-cell
Event pass (`0xd18c/0xd1a5`). The Tavern Event continuation does not resume at
the door. This is a Sign In/Inn restart, distinct from the initiating Load or
New Game paths documented in [Event dispatch](world-events.md#event-dispatch),
though they eventually share gameplay-entry machinery. **Reference comparison:**
the pin has no corresponding Sign In/Exit slot writes and reloads only a
different map on return (`locations.cpp:973`). Its Sign In exit lookup also
lacks the DOS Shangri-La correction.

## Inn input and refusals

The key table is at `0x39c59`; mouse rectangles use the same coordinate-only
dispatch as [M84](world-events.md#event-dispatch), for either mouse button.
Coordinates below are half-open bounds expressed as `(x,y,width,height)`.

| Action | Keys | Mouse rectangle |
| --- | --- | --- |
| Previous page | Up | (16,100,24,20) |
| Next page | Down | (52,100,24,20) |
| Delete | D | (87,100,24,20) |
| Remove | R | (122,100,24,20) |
| Create | C | (157,100,24,20) |
| Exit | E, X, Escape, Space | (192,100,24,20) |
| Add displayed character | 1–4 | (16,16,32,32), (117,16,32,32), (16,59,32,32), (117,59,32,32) |
| Active character sheet | F1–F6 | Existing active portraits in the bottom party bar |

Enter is unmatched. Inactive F-keys and missing faces do nothing. Active
portrait hits are count-guarded at `0xe2cc..0xe313`; displayed roster-face
hits use the list mapping above.

Add first ignores an absent row or an already-active ID silently, then checks
capacity. At six members it says “Your party is full!” and calls FX21
(`0x39b33..0x39beb`). Otherwise it appends the chosen ID and copies that
complete roster record into the new active lane. It does not reject a
character because of terminal conditions.

Remove ignores an empty party. Otherwise “Remove which character?” accepts
an active F-key/portrait or Escape to cancel; its Escape button is
`(225,84,24,20)`. It copies active records to the roster before compacting
membership (`0x39a3a..0x39a73`, `0x392be`). It has no last-member, health or
Elder-weapon refusal, and changes no CHR fields. Removing the last member is
allowed. Exit with zero members says “You have no one to adventure with”
(`0x39ac1..0x39ad0`); Escape and Space take that same path. No explicit FX
call accompanies this refusal, and the ErrorScroll helper supplies none.
Exit checks count, not viability; ordinary defeat handles terminal members.

## Create and Delete

**DOS static, high confidence (2026-10-24)** unless a paragraph explicitly
says Resource inspection, Reference comparison or Unknown. All offsets are
file offsets in the hash-checked CD `WORLD/XEEN.DAT`. These are original
rules, including its oddities, rather than permission to expand M88's scope.

### Create entry, free slots and return

The Inn C dispatch (`0x39c59`, handler `0x39a76`) tests the **displayed Inn
list count against 24**, not active membership or the total named roster.
Six active members therefore do not prevent Create. At count 24 or greater,
it displays “Your Roster is full!” (`0x5552a`) through ErrorScroll, with no
explicit FX call (`0x39ab8..0x39abe`). Characters lodged elsewhere can fill
the underlying roster while this local-list test still admits the screen.

Entry fades out with argument 4, closes Inn window 11, disables active
portrait input and calls Create (`0x39a7c..0x39a91`). Create scans IDs 0–23
in ascending order and admits a slot exactly when its first name byte is
zero; there is **no side or lodging test** (`0x35e40..0x35e90`). The first
free slot is selected. If none exists, it returns through `0x368d3` without
rolling or displaying an additional full-roster message. Slot race is
`floor(id/4) % 5`, sex is `id & 1`; the face is that roster ID's portrait,
frame 0 at (27,102) (`0x35e95..0x35ecb`, `0x361b5..0x361dd`). Thus IDs
20–23 are Human again. Up/Down move one free slot without wrapping; moving
past either end does nothing. They preserve the rolls and chosen class
(`0x3637a..0x363f7`). **Resource inspection:** Clouds has all 24 required
portraits; the earlier [roster census](#roster-and-list) remains applicable.

Every normal return from Create, including Escape, no free slots and return
after creating nobody, copies active records to the roster and calls the
known snapshot (`0x39a96..0x39a9a`). Creating a character stays on the screen:
success restarts at `0x35e3c`, rebuilding the free list, selecting its first
slot, clearing class choice and rolling again if any slot remains
(`0x36369..0x36377`). Filling the final slot instead returns immediately.
No new character is automatically added to the active party.

After the snapshot, the Inn fades out again, reloads its state through
`0x39460`, rebuilds the list and resets its page to zero (`0x395dc`). It
redraws `back.raw`, the party bar and window 11, loads `mm4.pal`, and uses
the return branch `0x3976f..0x39785` to fade in with argument 4, **without
replaying the opening scroll**. The existing Inn song continues: Create
contains no music replacement, and return skips song startup because
`DS:8d28` remains set (`0x394cb..0x394f4`). First Create display fades in
once, after its first dice draw (`0x3627b..0x36290`); later restarts do not.
The shared fade loop makes 33 palette writes at retraces for argument 4,
scaling 0..128 or 128..0 by 4 (`0x3fe0f..0x3fe81`). **DOSBox observation
(2026-10-24):** each of these fades takes about 0.4-0.5 s, the return shows
no scroll and the Inn song plays on ([runtime checks](evidence.md#m88-runtime-checks)).
The title menu also sets `DS:8d28` (`0xbcee`). Every gameplay entry calls
the Inn routine (`0xb824`), which clears the flag on its way out when the
party is not inside the Inn (`0x39c13..0x39c47`). Only a save loaded into the
Inn from the title therefore opens it in silence, as DOSBox shows; the map's
song returns after Exit. Sign In after any entry, and a panel Load into the
Inn, play the Inn song ([evidence](evidence.md#m88-runtime-checks)).

### Rolls and class selection

One attempt clears seven attributes, then makes three passes in the order
Might, Intellect, Personality, Endurance, Speed, Accuracy, Luck. Each pass
adds `floor(R(10,79)/10)` to each attribute: three contributions of 1..7,
giving 3..21. It makes **21 bounded draws per attempt**. If no class
qualifies, it discards all seven values and repeats; each retry costs another
21 draws (`0x359f2..0x35a93`). The bounded helper `0x45b9a..0x45bb4` calls
the generator once, masks to 15 bits and reduces modulo the inclusive range;
there is no rejection loop inside that helper. DOS clock mixing in
`0x45be5..0x45c1f` is the already-deferred generator difference in the plan,
not a new generator to implement.

| Class, in ID order | Required rolled attributes | DOS check |
| --- | --- | --- |
| Knight | Might 15 | `0x358eb` |
| Paladin | Might, Personality, Endurance 13 | `0x35900` |
| Archer | Intellect, Accuracy 13 | `0x35925` |
| Cleric | Personality 13 | `0x35941` |
| Sorcerer | Intellect 13 | `0x3594f` |
| Robber | Luck 13 | `0x3595d` |
| Ninja | Speed, Accuracy 13 | `0x3596b` |
| Barbarian | Endurance 15 | `0x35981` |
| Druid | Intellect, Personality 15 | `0x35990` |
| Ranger | Intellect, Personality, Endurance, Speed 12 | `0x359a6` |

“Required” means at least that value. All ten flags are recomputed by
`0x358d2..0x359dc`. Race does not change these thresholds. Initially there
is **no chosen class**, although the arrow points to the first qualifying
class in ID order (`0x35a9c..0x35ab6`, `0x35bc1..0x35bf1`). Left/Right
move the arrow among qualifying classes without wrapping; Enter commits
the highlighted class. Clicking an eligible class square commits it
directly. Arrow movement alone does not replace an already chosen class;
Create uses the committed class (`0x3630b`, `0x3635e`, `0x3689c..0x368be`).

R and Space reroll all seven attributes, repeat the qualification retry if
needed and clear the chosen class (`0x36314..0x36329`). They do not rebuild
the free-slot list or reset the dice. A completed exchange recomputes the
classes and clears the chosen class, but does **not** reroll even if no class
now qualifies (`0x36818..0x36853`). Escape from exchange preserves both the
values and class choice. The exchange's own selection loop ignores choosing
the same attribute (`0x35182..0x3518f`).

The audited Create, exchange, name, Delete and confirmation paths have no
generator call beyond the roll site's `0x35a2b`. The dice, UI waits and
name typing consume none. Name entry sets mode 255, which prevents its
shared key helper from drawing the gameplay scene (`0x3632c`,
`0x1df68..0x1df9b`); ErrorScroll uses its input-only branch. This is a
bounded call-path audit, not an absence claim about the whole executable.

### Create screen, input and dice

**Resource inspection + DOS static:** `create.raw` is the 64,000-byte
320×200 background. `create.icn` has 66 frames, `create2.icn` 16 and
`dice.vga` eight. The initial composition (`0x35f19..0x36263`) and ordinary
detail redraw (`0x363fd..0x36744`) draw these `create.icn` frames in window 0:

| Purpose | Frames and positions |
| --- | --- |
| Seven attribute buttons | `10+2*i` at (168,19+24*i), i=0..6 |
| Ten class squares | `24+2*i` at (227,19+11*i), i=0..9 |
| Attribute-value backgrounds | `50+i` at (195,31+24*i), i=0..6 |
| Race, sex and class backgrounds | 57,58,59 at (62,148), (62,158), (62,168) |
| Arrow and skill backgrounds | 61 at (220,19), 64 at (220,155), 65 at (220,170) |
| Roll, Create, Escape | 0,2,4 at (132,98), (132,128), (132,158) |
| Slot Up and Down | 6,8 at (86,98), (86,120) |

The selected slot's face and text follow those backgrounds. Arrow redraw
clears with frame 61 and draws frame 63 at (220,21+11*classId)
(`0x35cb5..0x35cfa`). Exchange highlights an attribute with its odd frame
`11+2*i`; cancellation restores the even frame. Window 26 displays
“Exchange %s with...” and frame 0 of `create2.icn` at (118,58)
(`0x35084..0x351ae`, `0x36777..0x36888`).

Text is loaded from the [program strings below](#create-and-delete-text-locations),
including its control codes. Normal text is color 4, hotkeys color 37;
unavailable class names use color **2**, available classes 4, and after a
class is committed the selected class index uses 12 (`0x35bb0..0x35bfd`);
DOSBox shows them gray, white and blue.
Values are right-aligned at x=215, y=31+24*i. Class names begin at
(242,20+11*i); race/sex/class labels start at x=22, y=148; the chosen class
name is at (62,168). Skills appear at (223,155) and (223,170), with the
“Skills” heading centered at (265,142). Redraw clears these areas using
the frames above rather than reloading the entire background.

Bounds below are half-open `(x,y,width,height)`. The installed arrays are
at `0x4f5ba` (x words), `0x4f5e8` (y), `0x4f5fe` (width), `0x4f614`
(height), `0x4f62a` (action) and `0x4f640` (button-feedback flag).
The 26-entry key table is `0x368e7`, with handlers at `0x3691b`.
Keyboard translation and either-button mouse behavior reuse
[M84's input evidence](evidence.md#event-dispatch).

| Action | Physical keys | Mouse bounds |
| --- | --- | --- |
| Roll | R, Space | (132,98,24,20) |
| Create/name | C | (132,128,24,20) |
| Return to Inn | Escape | (132,158,24,20) |
| Previous/next free slot | Up/Down, including keypad equivalents | (86,98,24,20), (86,120,24,20) |
| Highlight previous/next eligible class | Left/Right, including keypad equivalents | None |
| Commit highlighted class | Enter | None |
| Exchange Might/Intellect/Personality/Endurance/Speed/Accuracy/Luck | M/I/P/E/S/A/L | (168,19+24*i,24,20) |
| Commit class i=0..9 | No direct letter or digit shortcut | (227,19+11*i,12,10) |

Class mouse actions are internal codes 97..106, not physical lowercase
letter shortcuts: the ordinary keyboard translator emits uppercase letters.
PageUp/PageDown are **not** class controls in DOS. Unmatched translated
keys reach FX21 (`0x368c1`); ineligible class-square clicks and slot/arrow
limits are silent. Shared intercepted keys retain M84's behavior.
Exchange admits only the seven attribute keys/squares and Escape, whose
button is (118,58,24,20); other inputs and the same attribute leave it open.
Its arrays start at `0x4f656`; key table `0x351af`.

The dice have deterministic state initialized once per screen invocation:
frames (0,2,4), positions ((20,17),(112,35),(61,50)), velocities
((10,-10),(-10,-10),(-10,10)) (`0x35d12..0x35dba`). Each draw first
advances each frame modulo 7, adds its velocity, clamps x to
`[13,163-width(frame)]` and y to `[12,93-height(frame)]`, and reverses
the corresponding velocity only when outside that interval. Frame widths
are (27,27,27,31,27,27,27), heights (26,20,27,30,26,20,27), at
`0x4d750/0x4d760`; these match the inspected resources. These are
**per-frame** dimensions, not one fixed die size.

Each pass resets the BIOS counter, draws background frame 7 (152×83) at
(12,11) in window 32, moves/draws the three dice, updates that window,
then polls until input or a nonzero elapsed BIOS tick
(`0x351d0..0x355db`, clocks `0x440cd..0x440f4`). On M63's clock this is
one BIOS-tick acquisition cycle, nominally about 54.9 ms, without catch-up;
input can end the wait early. It is not a fixed game-time increment. Rerolls,
slot changes and successful creation preserve animation state; modal name
and exchange windows suspend this dice loop. Exact framebuffer/retrace
overlap remains Unknown and belongs to the runtime checks.

### Creating the record

Without a committed class, C shows “Select a Class before saving.” through
ErrorScroll and returns without mutation (`0x355e2..0x355f8`). Otherwise
window 6 asks “Enter a Name for this Character”, with maximum length 10 and
input width 200 (`0x355fb..0x35641`). The shared input accepts bytes
32..127, refuses a leading space, and converts each space-delimited word
to initial uppercase followed by lowercase. Backspace or Left removes the
last character. Enter returns the length; Escape returns zero. Empty Enter
or Escape leaves the roster untouched (`0x1e00f..0x1e179`, case helpers
`0x6d24/0x6d50`). There is no duplicate-name check in this writer.

Success zeroes **all 354 bytes** before copying the name and setting the
fields below (`0x3564a..0x358cb`; clearing helper `0xac93..0xacd5`).
Nothing in the old CHR is preserved. The slot identity/portrait is external
to that byte record and stays the same. Byte offsets here are within CHR:

| CHR field/bytes | Created value; DOS writer |
| --- | --- |
| Name 0–15 | Accepted name followed by zeroes; `0x3565d..0x35671` |
| Sex 16, race 17, class 19 | Selected slot's sex/race, committed class; `0x35687..0x35698` |
| Side 18; lodging word 340–341 | Current side (Clouds 0); prior town map; `0x35673..0x35683` |
| Seven attribute pairs 20–33 | Rolled permanent values, zero temporaries; `0x3569c..0x356f8` |
| Temporary AC 34, level temporary 36, temporary age 38 | Zero |
| Permanent level 35 | 1 on Clouds; `0x356fc..0x3570c`. Nonzero archive side gives 5: Darkside-only, out of scope |
| Birthday 37; birth year 346–347 | Low byte of current day; current year minus 18; `0x35782..0x35790` |
| Skills 39–56 | Zero except class/race assignments below, set to 1; `0x35851..0x3589c` |
| Awards 57–120 | All zero (including both packed nibbles) |
| Learned spells 121–159 | Zero except the listed class-book slots set to 1; `0x357a0..0x3584e` |
| Beacon map/x/y 160–162 and side 310 | All zero |
| Has-spells 163 | 1 if the starting table has a spell, otherwise 0 |
| Current spell 164 | First listed starting slot; 255 for noncasters; `0x35794..0x3579a`, `0x35818..0x35843` |
| Quick option 165 | Zero (Attack) |
| Weapons 166–201, armor 202–237, accessories 238–273, miscellaneous 274–309 | Every field in every slot zero, including item frame/state |
| Resistance pairs 311–322 | Race permanent values below; all temporary values zero; `0x35710..0x3577d` |
| Conditions 323–338; unknown byte 339 | All zero |
| HP 342–343, SP 344–345 | Current maximum for the initialized record; calls `0x358a7/0x358bb` |
| Experience 348–351; adventuring/combat spell 352–353 | Zero |

The class skill IDs and copy lengths are at `0x4d770/0x4d7a0`; the
four-entry-per-class starting spell table is at `0x4d7aa`. Spell numbers
below are **class-book slots**, not global spell IDs. Every unlisted flag is
zero. The current spell is always the first listed slot, even when several
flags are set. The skill column also specifies the exact display substring:

| Class | Starting book slots | Class skill ID; display source and copy length |
| --- | --- | --- |
| Knight | None | 1 Arms Master; `0x51ca0`, 11 bytes |
| Paladin | 21 | 5 Crusader; `0x51cd1`, 8 |
| Archer | 22 | None |
| Cleric | 21,1,14 | None |
| Sorcerer | 22,0,25 | 4 Cartographer; `0x51cc4`, 12 |
| Robber | None | 0 Thievery; `0x51c93`, 8 (excludes following formatting) |
| Ninja | None | 0 Thievery; `0x51c93`, 8 |
| Barbarian | None | None |
| Druid | 20,1,11,23 | 6 Direction Sense; `0x51cda`, 9, displaying “Direction” |
| Ranger | 20,1 | 11 Path Finder; `0x51d12`, 11 |

Race skill IDs and display offsets are at `0x4d780/0x4d790`. Human has
skill 14 Swimmer (`0x51d3c`, offset 0); Dwarf 17 Danger Sense (`0x51d5e`,
offset 0); Gnome 16 Spot Secret Doors (`0x51d4c`, offset 5), displaying
“Secret Doors”. Race substrings run to the terminator. Elf and Half-Orc
have none. Skills are not randomized. The class substring is copied into
a zeroed 14-byte buffer (`0x35acd..0x35b28`); race slicing is
`0x35b5c..0x35bab`.

| Race | Fire | Cold | Electricity | Poison | Energy | Magic |
| --- | --- | --- | --- | --- | --- | --- |
| Human | 7 | 7 | 7 | 7 | 7 | 7 |
| Elf | 0 | 0 | 0 | 0 | 5 | 5 |
| Dwarf | 2 | 2 | 2 | 2 | 2 | 20 |
| Gnome | 5 | 5 | 5 | 20 | 5 | 0 |
| Half-Orc | 10 | 10 | 10 | 0 | 0 | 0 |

Those five-entry tables start at `0x4f926` (Magic), `0x4f92b` (Fire),
`0x4f930` (Electricity), `0x4f935` (Cold), `0x4f93a` (Energy),
`0x4f93f` (Poison). The initialized character is age 18, level 1, without
equipment, conditions, temporary bonuses or HP/SP skills. The DOS capacity
functions (`0x20182..0x20255`, `0x3bb30..0x3bcce`) match the existing
`XeenCharacterRules::maxHp/maxSp` level-1 formulas:
HP is `max(class HP + B(Endurance) + race HP,1)`; a caster's pass is
`max(B(relevant attribute)+3+race SP,1)`, halved for a hybrid. Druid averages
the Personality and Intellect passes; Ranger halves each before averaging.
Noncasters have zero SP. Class HP, race HP and race SP tables at
`0x4f148/0x5022a/0x50220`, and the bonus tables at `0x50638/0x50668`,
match the current code. The full formulas remain in
[character capacity](combat.md#attribute-bonus-hp-sp-and-armor-class).

### Delete and empty-party saves

D does nothing when the displayed list is empty. Otherwise window 28 asks
“Delete which character?” and accepts 1–4 or those displayed face rectangles;
Escape cancels via its (225,120,24,20) button. Active F-keys/party portraits
do not select for Delete (`0x398d7..0x398f3`, `0x39109..0x39290`). An active
member is nevertheless deletable through its **listed face**. Characters
lodged in another town cannot be selected from the town-filtered list.

Before confirmation, DOS checks the selected **roster** record's nine weapon
IDs for 34 (Xeen Slayer Sword), without testing equipped, broken or other
item state (`0x3991b..0x3992f`, `0x325f5..0x3261a`). Carrying it refuses
deletion. Pooled quest items are not read by this guard and are not cleared.
The refusal text starts at `0x554b7`.

Confirmation is “Are you sure you want to delete %s the %s?” (name and
class), in window 21. Y or its button (129,112,24,20) confirms; N, Escape
or the No button (185,112,24,20) cancels. Enter/Space do not confirm
(`0x39932..0x39982`, `0x1ea3a..0x1ebc1`). Buttons use `confirm.icn` frames
0/2 and their shared pressed feedback.

Confirmed Delete clears all 354 CHR bytes (`0x39985..0x39995`). Every field
in the creation table is zero, including sex/race/class, level, every Beacon
field, awards, inventories, conditions, skills, spellbook, HP/SP and birth
data. Current spell becomes **0**, not 255. No CHR field is preserved.
**Resource inspection:** all 24 unnamed original `maze.chr` records are
entirely zero, so the deleted record matches their contents exactly.

DOS rebuilds the ascending 0–23 named list, testing town but **omitting the
side test** used by normal Inn entry (`0x39998..0x399fb`). It removes the
deleted ID from active membership if present, compacts the remaining party,
then copies active records and snapshots (`0x399fd..0x39a32`). There is no
last-member restriction. It returns to `0x39787` with **the old page index**;
there is no page-zero reset or last-page clamp on this path. Deleting the
last entry on the last page can therefore leave an empty visible page until
Up, as DOSBox shows. Return from Create, unlike Delete, does reset to page
zero. There is no
explicit Delete success/refusal FX call in these paths or the confirm helper.

Both known snapshots write the roster and party even with active count zero:
the copy loop in `0x392be..0x392f6` skips zero members, while `0xe938`
still writes `maze.chr` and `maze.pty` (`0xe961..0xe982`). Load reads those
blocks without a nonempty-party gate (`0xf013..0xf076`); the active copy
loop also skips zero, then map 0 opens the Inn (`0x39460..0x394bc`). Exit
still refuses until a member is added, using `0x5553f` and the
[existing Exit rule](#inn-input-and-refusals). **DOSBox observation
(2026-10-24):** both paths reopen the Inn empty after Load; the Load menu
shows the slot's level as 0, and the empty party bar keeps the yellow
selection frame on its first slot.

The two sites call the snapshot directly, not the ordinary Save wrapper's
Dismiss-tail settlement. The earlier Sign In forced Save has already lodged
any dismissed characters and consumed that tail (`0x1ec97..0x1ed2e`).
Remove/Delete are not Dismiss. Their new snapshots can retain the old second
party-header count while the first count is zero; Load uses the first count
for membership. Inn Exit later sets the second count to the first
(`0x39b1b..0x39b1e`). This stale header count is not evidence of a newly
pending Dismiss operation or permission to resurrect cleared characters.
The CHR clear/initialization audit found no field absent from the current
character, supplement, learned-spell or award save representation.

### Create and Delete text locations

Load the original strings with their formatting, rather than replacing them
with translated or reformatted text. All entries below are null-terminated
unless a skill slice above specifies a length. Resource-rendered button
art comes from the named ICN files, not additional program strings.

| Text or format | XEEN.DAT file offset |
| --- | --- |
| Full roster refusal | `0x5552a` |
| No class selected refusal | `0x54c82` |
| Name prompt | `0x54ca8` |
| Name input layout, populated input layout | `0x53101`, `0x5310f` |
| Shared ErrorScroll wrapper | `0x5240a` |
| Create labels/hotkeys plus detail placeholder | `0x54e5d` |
| Stats, race/sex labels, all ten class labels, skill heading/placeholders | `0x54cd7` |
| Chosen class-name placement | `0x54ccc` |
| Exchange prompt | `0x54c6e` |
| Delete-selection format; “Delete” substitution | `0x553c1`; `0x519f6` |
| Xeen Slayer Sword refusal | `0x554b7` |
| Delete confirmation | `0x554ff` |
| Empty-party Exit refusal | `0x5553f` |
| Class names, IDs 0–9 | `0x5191f`, `0x51926`, `0x5192e`, `0x51935`, `0x5193c`, `0x51945`, `0x5194c`, `0x51952`, `0x5195c`, `0x51962` |
| Race names, IDs 0–4 | `0x51bbb`, `0x51bc1`, `0x51bc5`, `0x51bcb`, `0x51bd1` |
| Male, Female | `0x51bd7`, `0x51bdc` |
| Might, Intellect, Personality, Endurance, Speed, Accuracy, Luck for exchange | `0x51975`, `0x5197b`, `0x51985`, `0x51c4b`, `0x51991`, `0x51997`, `0x519a0` |

Class, race and sex far-pointer tables are at `0x500fa`, `0x50724`,
`0x50744`; exchange uses entries 1–7 of the table at `0x50790`.
Skill-name sources and slice lengths are in the creation tables above.
Filenames are `create2.icn` at `0x54c62`, `dice.vga` at `0x54e3e`,
`create.icn` at `0x54e47`, `create.raw` at `0x54e52`, and `confirm.icn`
at `0x53244`; the Inn retains `inn.m` (`0x55453`), `back.raw`
(`0x55459`) and `mm4.pal` (`0x554af`).

**Reference comparison:** the pin offers 30 free slots, checks the Inn list
against 30, initially highlights the last eligible class, uses disabled
color 0, maps class navigation to PageUp/PageDown, uses a fixed dice size,
selects the last starting spell and resets the page after Delete. Its
character clear leaves Beacon x/y untouched; DOS clears them. Its rebuilt
Delete list uses side and lacks the DOS town restriction. The already-known
missing Create/Delete snapshots and lodging-layout difference still apply.
These comparisons do not authorize Darkside implementation.

## Dismiss and deferred lodging

Exploration D dispatches to Dismiss (`0xd45b..0xd460`). The combat key table
at `0x2ecad` has no D action; it follows unmatched-key dispatch, not Dismiss.
Window 31 displays “Dismiss whom?” (`0x20e81..0x20e88`). F1–F6 and active
portraits select, with a count guard; Escape closes, including its button
`(225,120,24,20)`. Last-member refusal is “You cannot dismiss your last
character!” (`0x20f34..0x20f6d`), without an explicit refusal FX call.
No Clouds item blocks dismissal. The pin's Elder-weapon guard applies to
Swords of Xeen, so its presence is not a Clouds discrepancy.

Dismiss adds no time, does not save and changes no CHR fields immediately.
It compacts active members, retaining the departed full CHR and ID at the new
inactive tail (`0x20f70..0x21004`). The old count remains available to Save. The first party-header byte
(`DS:ed8c`) is the active count; the second (`DS:ed8d`) is the retained count.
Save later walks those departed records, sets their saved map to `28+side`
and their side, and copies them to the roster (`0x1ec97..0x1ed1e`). In Clouds
this lodges them at **Vertigo**, regardless of the dismissal map. This deferred
write differs from Remove, which keeps the character lodged at its Inn town.
Save clears each departed ID to `0xff` (`0x1ece7`) and sets the retained
count to the active count (`0x1ed23..0x1ed26`) before the snapshot
(`0x1ed2e`). The tail is consumed, rather than serialized into `maze.pty`.

An explicit inactive-read witness is dismissing from six to five and then
saving: Save reads lane 5's departed CHR, updates lodging and persists it.
Its source is the retained full character and roster ID, not uninitialized
memory. The canonical saved result is the roster record, not a separate
serialized active-tail array. The portrait compactor's reads of handle slots
6/7 (`0x389ff..0x38a44`) concern a larger resource cache populated by the
24-face loader, not reads beyond the six physical character lanes.

Dismiss keeps drawing the exploration scene while window 31 is open: its
poll loop calls `0x10bc3(0)` at `0x20eb8..0x20ec4`. This advances the
shared four-phase drawing counter (`0x10bda..0x10be7`). The argument is
separate from global scene mode `DS:9a1c`; it controls the optional
post-draw update at `0x117d2`. **DOS static, high confidence.** Monster
movement from the draw routine happens only in scene modes 1, 2 and 5
([evidence](evidence.md)), and the argument does not gate it. MMModern
keeps ordinary scene animation running as it does for Info, without
advancing exploration time or requesting monster movement.

**Unknown: Poison Volley visuals with one to five members.** DOS sets all
six fixed shot flags (`0x1d899..0x1d8bb`), and its hit loop is bounded by
active count (`0x2cb46..0x2ccdb`). The retained listings do not settle the
projectile renderer's lane count. MMModern currently draws active lanes;
a smaller-party DOS recording or the renderer's loop must settle whether
inactive missiles are also visible. See [magic rules](magic.md).

## Shared state and consumer bounds

Add, Remove and Dismiss do not divide or clear pooled gold, gems or food.
Sign In can apply bank interest through time advancement, but does not eat
or buy food. The full 30-record roster is saved in `maze.chr`; Load does not
reconstruct modified characters from the original resource.

Pending treasure is separate from those pools and from the saved CHR/party
payload. Membership operations do not repartition it. The forced Sign In
Save precedes Event-tail delivery/cleanup, and gameplay entry clears pending
totals/hasItems (`0xcfe3..0xcff7`). Do not promise that pending treasure
survives the complete Inn round trip. This reuses the
[M81 save boundary](evidence.md#save-and-load-boundary), including stale item
array bytes; it does not add a new persistence owner.

**DOS static, high confidence for the cited bounds**, including retained
evidence for the underlying rules. The inventory is limited to M87's consumer
categories and original call sites; it is not an absence proof for the entire
program. Combat count can be smaller than active count after Run.

| Consumer | Verdict and offsets |
| --- | --- |
| Combat order/participants | Population copies active count and ordered indexes, `0x2e6e2..0x2e703`; turn/cursor work uses compacted combat participants, not a fixed six-member population. |
| Monster targets | Preferred scan `0x2d89c..0x2d99e` and eligible fallback `0x2da3a..0x2da90` use combat participants. The [count-dependent draws](combat.md#monster-targeting-and-hit-resolution) include the special six-member path. |
| XP | Kill caller `0x2c27d` enters `0x2c5b2`; bounded passes `0x2c5d3..0x2c66e` use combat count in combat, active count otherwise, and divide among eligible recipients excluding Dead/Stoned/Eradicated. |
| Poison Volley | Six shot flags are enabled at `0x1d899..0x1d8be`; actual character hit loop `0x2cb46..0x2ccd9` stops at active count. Fixed flags do not imply inactive CHR reads; the smaller-party visible missile count remains Unknown. |
| Rest and food | Rest's recovery/food loops `0x1e50d..0x1e826` use active count; Tavern Food is the [calibration witness](evidence.md#m87-inn-and-party-size). |
| Event conditions/give/take | Whole-party If `0x27266..0x2726e`, Take/Give `0x278c3..0x278d3`, SetChar random `0x27a01..0x27a14` and SelectRandomChar `0x287d8..0x287eb` use count. This does not certify arbitrary literal selectors. |
| Member prompts/WhoWill | Common prompt `0x2f162..0x2f179` bounds selection by combat/active count. WhoWill's one-member path returns 1 without writing its stored selection (`0x25082..0x2508c`); other selections are guarded at `0x25294..0x252b6`. Retain M85's selection lifetime. |
| Treasure | Capacity `0x2493c..0x249be`, preferred-recipient guard `0x243d5..0x243f2` and fallback `0x24502..0x24565` respect membership. |
| Training/Temple | F-key selection rejects inactive indexes at `0x1ad0b..0x1ad36` / `0x1b020..0x1b04e`. |
| Town Portal | `0x30654..0x30819` enumerates five destinations, not party members; selected-caster admission belongs to the casting path. |
| Lloyd's Beacon | `0x2fc9a..0x2ffb1` uses the selected caster pointer and its beacon fields, not a fixed party loop. |
| Mr Wizard | `0x1f23e..0x1f296` relocates the party and clears pooled gold; no character iteration. |
| Everyone dead | `0x24f29..0x24f8f` uses combat/active count and condition helper `0x20256`. |

The Dismiss-to-Save tail read above is the established inactive-character
exception. Preserve its data provenance separately from ordinary active-member
access and from unrelated fixed-size tables.
