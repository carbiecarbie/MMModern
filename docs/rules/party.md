# The Inn, membership and party size

Checked against the English World of Xeen CD program on 2026-10-23, at
MMModern `f77720f`. This describes DOS rules, not implemented coverage.
**Evidence: DOS static, high confidence**, except where labeled otherwise.
File offsets refer to `WORLD/XEEN.DAT`; calibration, provenance and runtime
limits are in [M87 evidence](evidence.md#m87-inn-and-party-size).
Create and Delete editing rules belong to M88; their save side effects are
identified here because they share the Inn's persistence boundary.

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
| Delete (M88) | D | (87,100,24,20) |
| Remove | R | (122,100,24,20) |
| Create (M88) | C | (157,100,24,20) |
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
