# Time, rest and conditions

Reference: ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. **Reference only**
means source-verified, not DOS-confirmed. **DOS static (retained)** denotes
calibrated analysis recorded in [original quirks](../original-quirks.md),
not fresh executable analysis by this chapter's author. **DOS play** is
retained maintainer evidence. **Unknown** identifies unverified original
behavior. These distinctions matter: the pin contains intentional changes
and bugs in precisely these rules.

## Real play time and panel clock

**DOS static, high (M92, 2026-10-25),** calibrated against the known slot-05
counter and score and the slot-06 before/after capture. The maintainer's
DOSBox check confirms the panel's hidden default, G/O/B/R activation,
12-hour civil clock, advancing clocks and reset on reopening. It does not
extend runtime confirmation to the arithmetic/DST edge cases below.
This counter is
independent of calendar minutes, BIOS presentation ticks and Journey RNG.
Offsets below are in the calibrated CD `WORLD/XEEN.DAT`; see
[M92 evidence](evidence.md#m92-clouds-ending-evidence).

The saved dword at DS `f01a` (party offset 654) counts seconds. Query
`0xf2fd..0xf324` returns `(time() - startMark + savedSeconds) mod 2^32`.
The start mark is DS `98b0`; `0xf2e9` samples it at DAT entry (`0xb3b3`)
and after each snapshot (`0xe9d0`). Load, including Inn Load, does not
reset it. Saving stores the query result (`0xe945..0xe94e`) and resets
the mark after copying the archives. New Game zeros savedSeconds before
its first snapshot (`0x20c08..0x20c38`), including time already spent in
that DAT at the title or difficulty selection. Menus and dialogs count.
EXE scenes run outside that DAT interval: their return starts a fresh DAT;
viewing an ending from Other Options does not write a saved counter.
There is no monotonic clamp: moving the DOS clock backwards can reduce
the total, or wrap to a large unsigned value if the total goes below zero.
Save-time I/O between the query and the reset is not accumulated again.

`time()` (`0x6ccc`) reads the date and time through the DOS Get Date and
Get Time services and converts through Borland's runtime (`0xa4ac`). The
host reader accepts DOS civil years 1980 through 2107, using Borland's `year & 3` leap rule, including 2100.
An out-of-range host date reports an error. Without `TZ`, `0xa7da..0xa8cb` selects
EST/EDT, a 18,000-second standard offset and enabled legacy US DST. The
rule (`0xa9c8..0xaaa2`) uses the first Sunday in April from 1987 onward
(last Sunday in April earlier), and the last Sunday in October, with
the runtime's 02:00 spring and 01:59 autumn boundary tests. The conversion
subtracts an hour in the DST interval. Thus this is not a claim that DOS
uses the host's current timezone database or modern March/November rules.
A civil-clock jump and a change of this DST classification both affect
the queried seconds; no separate gameplay adjustment compensates them.

The clock is hidden by default. The panel's key table at `0x1f332` maps
G, O, B, R to the four gate stages (`0x1f2a5..0x1f2d5`); pressing those
keys in order sets runtime display flag DS `97fa`. Repeated O is allowed.
The longer word `GOOBER` also reaches that gate, but its E uses the ordinary
Efx toggle; the minimal sequence avoids that extra change. This mapping
is calibrated by the same table's Efx and Save entries. Leaving the panel
clears the flag (`0x1f30b`), so reopening hides the clocks again.
The panel calls `0xf325` only while this flag is set
(`0x1efde..0x1eff3`). Its DS `4be3` format puts the civil clock at the
upper left and accumulated play time at the upper right. The clock comes
through Borland `ctime`/`localtime` (`0xf1ce`, `0x8b32`, `0x8ad1`), then
converts to a 12-hour display with a leading space for a one-digit hour,
minutes, seconds and lower-case `a`/`p`; noon and midnight show 12. The
inverse timezone/DST conversion normally reproduces DOS's local civil
time; do not apply its five-hour offset a second time to the host clock.
The calendar helper clamps a signed-negative adjusted dword to the epoch
(`0x08872..0x08885`), so injected seconds zero displays midnight. This
presentation clamp does not clamp the unsigned play-time counter.
The idle loop resets its BIOS timer at `0x1ef91`, draws the clock, waits
for a tick or input, and redraws. It is live while the panel is open.

Play-time formatting (`0xf286..0xf2e8`) uses an hours:minutes:seconds
unsigned format with minimum width two for each field. Preserve its
16-bit intermediate: it narrows `seconds/3600` to 16 bits and subtracts
only the low word of `hours*3600` from the 32-bit total before dividing
the remainder into minutes and seconds. The minutes quotient also narrows
to a 16-bit local (`0xf2b7`, passed as a word at `0xf2cd`); a rollback
total of `0xfffffff9` displays `13398:16412:09`. At 62,545 seconds the result is
`17:22:25`; at 68,400 seconds (19 actual hours) it is `19:1092:16`.
This is calibrated arithmetic evidence, not a 19-hour DOSBox capture.
The score has separate arithmetic described in
[ending score](cutscenes.md#clouds-ending-score).

## Bash

**Evidence: DOS play and calibrated DOS static, high confidence (2026-10-15).**
The maintainer's default and high-Might parties cannot break Vertigo's torch
wall at `(8,4)` West. The installed CD program and map data explain the refusal:
the forward cell `(7,4)` has the whole wall word `0x8888`, which collides with
the original `INVALID_CELL` sentinel. The [artifact identity and offset
mapping](evidence.md#exact-baseline) apply; all locations below are file offsets.

- B first charges one minute indoors, ten outdoors (`0x0d516`); outdoors the
  Bash routine returns without damage (`0x32d4f..0x32d56`). A passable forward
  wall moves instead of entering Bash and charges a second indoor minute
  (`0x0d51b..0x0d547`).
- A blocking indoor attempt selects the first two eligible active members in
  party order, or one if only one is eligible. Asleep and Paralyzed through
  Eradicated are skipped (`0x32d6c..0x32daf`, table `0x32f71`). Each selected
  member loses 2 HP and gets the injury portrait (`0x32daf..0x32e2c`).
- Next, `mazeLookup(forward, 0, 0xffff)` returns the **effective whole forward
  wall word**, not a boolean cell-presence result (`0x11edb..0x11ee4`). A return
  of `0x8888`, whether missing geometry or a real cell with that value, skips
  all subsequent wall handling and consumes no Bash random draw
  (`0x32e4f..0x32e5c`). No cell flag or surface rule adds a Bash prohibition;
  the lookup's surface/stepped bookkeeping does not change its returned word.
- Otherwise, wall 7, 14 or 15 changes the original wall animation counters and
  skips the roll (`0x32e5f..0x32e84`). Every other blocking wall can reach the
  roll. There is no wall-type whitelist.
- Sum the selected members' effective Might **after the HP costs** and add one
  `R(1,30)` draw (`0x32e87..0x32ece`, inclusive range helper `0x45b9a`).
  Attribute 0 reads the unsigned permanent/temporary Might bytes, adds age,
  equipment and condition modifiers, and clamps at zero (`0x21454..0x215a9`).
  The Bash sum and comparison use 16-bit words; there is no 8-bit sum truncation
  or cap explaining refusal for two members with Might above 100.
- The primary map's difficulty byte 5 is used for wall 9; byte 6 for every
  other rolled wall (`0x32ed0..0x32ef3`). These are DAT offsets 821 and 822,
  memory `ds:0xc101` and `ds:0xc102`, both **unsigned**. Vertigo has 30 in both,
  matching the original initial state and inspected DOS `.CUR` files.
  `R(1,30)` plus the Might sum succeeds when greater than or equal to the
  difficulty (`0x32ef5..0x32ef7`).
- Success writes wall 3 on the current face and the opposite forward face
  (`0x32ef9..0x32f4c`); the writer has no additional eligibility check
  (`0x234dd..0x23550`). Failure keeps the walls and position. Both outcomes
  check party death, wait two interruptible BIOS ticks and redraw the party
  (`0x32f4f..0x32f63`). The early `0x8888` refusal shares that tail. Normal
  charged-time condition draws and subsequent world presentation remain owned
  by their existing paths; zero draws here means zero **Bash-roll** draws.

Calibration: the already DOS-confirmed M50 grate behavior uses the same
member selection, HP cost, `R(1,30)`, threshold and two-face wall writer;
Vertigo grates retain difficulty 30. Pinned ScummVM `6814ee9b`
`Interface::bash` and `Map::mazeLookup` agree about the sentinel collision.
Its signed loading of difficulty bytes 5/6 differs from the zero-extension in
DOS; MMModern preserves the stored metadata and interprets those bytes as
unsigned when Bash consumes them. Synthetic tests cover 127/128/255 thresholds,
and current cell overrides participate in the forward-word check.

## Calendar and kinds of time advance

**Evidence: Reference only.** Source: [Party::changeTime/addTime][party].
Time is measured in minutes, with 1,440 minutes per day and 100 days per
year. Internal days range 0-99. Midnight subtracts 1,440, increments day,
and wraps day 100 to 0 while incrementing year. Dawn is 05:00 (minute 300);
night is before 05:00 or at/after 21:00 (minute 1,260).

`changeTime(n)` and `addTime(n)` are different operations:

| Order | `changeTime(n)` | `addTime(n)` alone |
| --- | --- | --- |
| 1 | Before changing clock, process one eight-hour condition block if `floor((minutes+n)/480) != floor(minutes/480)`. | Skip condition block. |
| 2 | Run `addTime(n)`: calendar, possible merchant regeneration/interest, dawn. | Perform these same calendar/economy/dawn operations. |
| 3 | Process Confused then Paralyzed for each member, whether or not an eight-hour boundary was crossed. | Skip both recovery checks. |
| 4 | Refresh night/day sky if necessary. | No equivalent final check inside this routine. |

The eight-hour test is a boolean, not a count. A single 2,000-minute call
has one block even when it crosses several eight-hour boundaries. Splitting
it into many calls is observably different: conditions and RNG can change
more often. The stat-death check also uses the old year because it precedes
calendar rollover. A call starting at day 99, 23:59 can age a character into
a lower attribute bracket only after that call's death check.

**DOS static (retained):** ordinary movement changes position, calls
`chargeStep`, then `stepTime` (`0x0d39f..0x0d410`). Charge costs 1 minute indoors
or 10 outdoors. Condition/calendar/economy work precedes servicing the prior
pending monster opportunity at the reached position; the countdown is then
reset to three. `stepTime` performs arrival bookkeeping and advances the
modulo-24 counter. Turns call `stepTime` without acquiring a successful-step
charge; blocked attempts acquire no charge. The next ordinary draw services
one countdown pulse and checks contact before input (`0x0d1c3..0x0d1c9`).

A simple countdown does not bar another navigation action. A rapid successful
step consumes its prior opportunity once; an idle drain instead services it
when the countdown reaches zero. Retained ranged/combat/Event work still fences
subsequent navigation until its frames and suffix are acquired. `journeyMutable`
and save admission continue to require Quiet. The approved navigation queue
exception is documented in [original quirks](../original-quirks.md#approved-deviations).

Held navigation is not a new tap on every draw. The fresh press is admitted
without a repeat delay; only held repeats wait for 500 ms from the platform's
observation of that press. Thereafter a repeat needs both its nominal 11/120 s
typematic period and ordinary draw/input readiness. Thus sustained pace is
bounded by the slower of typematic generation and draw3d (about 9.1/s under the
standard BIOS clock). The rate is the maintainer-authorized IBM keyboard
fallback, not a measurement of the DOSBox device. [Evidence and video limits](evidence.md#navigation-typematic-evidence-2026-10-13)
distinguish the two. Pending distinct taps retain their FIFO obligations; release
discards only held demand. Modifier changes do not restart the physical press's
delay. Idle drawing/actor work continues during the initial pause, and a late
wake dispatches at most one repeat, resetting its period at dispatch rather
than after feedback or gameplay work. No new gameplay scheduler owns this wait.

Real wall-clock waiting and animation do not call `changeTime`. The common
presentation counter uses absolute monotonic time at 18.2065 Hz, with independent
reset samples; an N-increment threshold can first occur after roughly N-1..N
periods. Ordinary draw3d uses a two-increment threshold; its outer one-increment
wait overlaps it. The UI counter/water orientation updates before composition
(`0x10bda..0x10bf4`); overall/floating phases, actor animation and splat lifetimes
advance afterward (`0x107b6`). Reconstruction reads the retained presentation
values that produced the displayed frame, preserving the live next-draw values
without consuming them. Failed composition retries retain the same pre-draw
update and perform the post-draw update once on success. Fades use the separate
70 Hz retrace clock. [Capture calibration](evidence.md#presentation-clock-captures-2026-10-13)
provides observations and limits. Input, cursor repaint and a repeated acquisition
of the same frame must not replay gameplay, RNG or draw advancement. Navigation
already waiting only for draw completion returns to the input poll
before another semantic draw can begin, including a threshold crossed between
admission and idle (`0x0d1c3..0x0d1c9`). Retained work keeps its existing idle owner.

### Synchronous presentation intervals

**DOS static, calibrated against the title, dream, death and Gunther captures.**
These waits use the shared presentation clock; they never advance calendar time.
An interruptible wait ends at its threshold or an admitted input poll. An
uninterruptible wait retains input until the original poll, while the host
continues servicing close/focus events. A late wake advances one acquired phase,
without replaying missed draws.

| Caller | Interval and causal order |
| --- | --- |
| World YesNo/WhoWill | Outer one-tick wait; locations use three. These outer waits are interruptible (`0x24bd7..0x24c16`, `0x251a3..0x25262`). |
| NPC | Text on uses three ticks, draw, reset, then input poll; ordinary Text off chooses its face before a one-tick hold ([audio rules](audio.md#text-off-byte-readers-and-presentation)). The inner wait is uninterruptible. Page changes retain the running CD interval (`0x25b8f..0x25bad`). Each fresh invocation resets its local playback guard (`0x256ec`, `0x25ae8..0x25af3`). |
| CD speech | Decode MMSSFF as 75-Hz frames; admitted starts use `(end-start+25)/75` seconds from the call. Refused absolute starts have no busy interval; missing/undecodable tracks retain timing with visible text ([audio rules](audio.md#cd-disc-addressing-and-boundary-failures)). Mode 2 starts/replaces playback and returns without stopping (`0x25aa8`); ordinary final dismissal stops (`0x25c94`). An all-zero cue neither starts nor stops (`0x3da98..0x3daa7`). WhoWill keeps the passive face; Event cleanup closes its window without an audio stop. Busy polls choose `R(0,3)` from the existing Journey RNG; idle uses face zero without a random request. See [capture limits](evidence.md#death-and-cd-speech-captures-2026-10-13). |
| Shops | Poll input before the three-tick wait, so input may end it early. While the current sample is active, Bank chooses `R(7,11)`, Smith `R(5,7)`, Temple `R(2,4)`, Training `R(8,12)`. Smith replaces base art while its logical base index still advances. Guild and Tavern have no Clouds speaking choice. |
| Sparkle | Complete four frames for one recipient before the next. Each frame waits two outer ticks in window 11, otherwise one plus nested draw3d. Suspend and restore the actor countdown (`0x21087..0x21186`). Window 11 directly publishes the sparkle without nested draw3d, so NPC face activity also waits until this helper returns. A per-member latch skips a recipient already sparkled in the same Event run (`0x210bc`, set `0x2117f`); Event start and end clear it (`0x267a6`, `0x291e0`), as does HP addition (`0x2ee43`, `0x2eeca`). |
| Damage | Monster recipients each have a two-tick uninterruptible hold. Generic Event/trap damage publishes overlay and injury in recipient order, then shares one final two-tick hold (`0x23f73`, `0x2d6c4`, `0x264ed`). |
| Title | Thirteen phases; phase zero is background alone. The first CD fade uses step four. Publish phase one after the acquired final fade level, then use interruptible four-tick idle waits (`0x0be9a..0x0bf8d`). |
| Party death | Eighteen fire publications have retrace cost and no explicit BIOS hold. Laugh iterations use two ticks and sample status, one restart at index ten, maximum executed frame 21, and no sound tail. Closing hands use one-tick interruptible waits; fade out with step four, then return to the World title established by capture. |

VOC gates use INTRO `prosnd`'s fixed PCM offset +`0x20`, low length word at
+`0x1b` minus 10 modulo 65536 plus one sample, and fixed 68-microsecond
period, independently of header rate ([audio rules](audio.md#installed-digital-driver-command-and-length-rules)); a new sample replaces the previous interval,
and stop/completion clears activity without requiring an audio mixer. Greetings
do not impose an uninterruptible delay. Action/refusal samples replace greetings;
Tavern goodbye retains the last screen until its sample completes, without
advancing a shop animation. Speaking draws occur before later service-day
generation, so an early input can change how many visual RNG requests precede
that gameplay work. Retain chosen results across composition failure/retry.

## Eight-hour condition block

**Evidence: DOS static, high confidence.** Same DAT as below; routine
`0x2533e..0x256cc`, member loop `0x2536b..0x255fe`, rechecked 2026-10-22 and
calibrated against the Poison/Disease and Weak/Drunk sites below. The pin
([Party::changeTime][party]) has the same order without step 3 and keeps
live `int` counters. MMModern implements the DOS block in
[XeenConditionTimeCandidate::service](../../src/games/xeen/XeenCombatRules.cpp).

Process active members in order. Finish all these steps for one member
before beginning the next:

1. If not Dead, Stoned or Eradicated, inspect all seven effective attributes.
   Any value below 1 sets Dead to 1. Do not infer an HP assignment here.
2. Increment nonzero Heart Broken; if the incremented count exceeds 10,
   clear it and set Depressed to 1.
3. **DOS:** clear Confused, Asleep and Paralyzed, and Weak, which step 7
   replaces (`0x253fb..0x25424`).
4. Process existing Poison, then existing Disease, using the DOS rule below.
5. Increment nonzero Insane. Increment each nonzero Dead, Stoned and
   Eradicated counter.
6. Increment nonzero In Love; above 10, clear it and set Heart Broken to 1.
7. **DOS:** replace Weak with Drunk, then clear Drunk, unconditionally.
8. Advance nonzero Depressed to `(count+1) % 4`.

Consequences of order: Heart Broken 10 becomes Depressed 1 and then
Depressed 2 in the same block. In Love 10 becomes Heart Broken 1 after the
Heart Broken step, so it does not immediately advance again. A new Dead 1
from the attribute check reaches Dead 2 later in that same block.
Terminal members skip only the attribute-death test, not the clear,
Poison/Disease or other counter steps. After the block, the per-call
Confused/Paralyzed recovery finds nothing to draw for.

**Byte boundaries.** Every counter is a byte tested for zero before its
increment, and the comparisons with 10 read the stored byte. Insane, Heart
Broken and In Love wrap 255 to 0, so a wrapped Heart Broken or In Love
triggers nothing; Dead, Stoned and Eradicated store 255 when the increment
wraps, so they saturate (`0x253c7..0x253f6`, `0x254c7..0x25569`,
`0x25574..0x255a6`). The pin's live counters reach 256 instead and cross the
threshold ([quirk](../original-quirks.md#sv-condition-counter-bytes)).

### Poison and Disease

**Evidence: DOS static (retained), high confidence.** `WORLD/XEEN.DAT`
SHA-256 `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`;
analysis rechecked 2026-10-11. Condition routine file offsets `0x25349`,
`0x25425`, `0x25473`, member-loop tail `0x255ff`; save calculations
`0x2d164`, `0x2d18a`, `0x2d200..0x2d21f`; inclusive draw helper
`0x45b9a`. Calibration used character attribute order at `0x21454` and
known byte layout. [Evidence record](../original-quirks.md).

For each present condition, draw `U(1,10)`. Only a result of 1 permits a
saving throw. Poison uses **electrical** resistance; Disease uses **cold**
resistance. Let `R` be permanent + temporary + equipped resistance for that
kind. Draw `U(1,R+40)` and succeed at `<= R`. Success clears the condition;
every other outcome doubles its unsigned byte modulo 256. An absent
condition consumes no draw. Terminal members still participate.

Thus Poison 128 can disappear on a failed gate (`128*2 mod 256 = 0`), and
255 becomes 254. A resistance-10 character gets a cure probability of
`1/10 * 10/50 = 1/50` per present-condition check. Failed gates consume one
draw; attempted saves consume two. Draw order is member 1 Poison gate/save,
member 1 Disease gate/save, then member 2, never all Poison checks first.

The pin incorrectly tests absent conditions and uses `U(0,9)` for Disease.
Historical milestone prose describing that behavior is superseded by the
DOS evidence. Its formulas must not be reintroduced from an old plan.

### Weak, Drunk and attribute modifiers

**Evidence: DOS static (retained), high confidence.** Same DAT; eight-hour
replacement `0x255ab`; dawn increment `0x26452`; condition modifiers
`0x1f9a8..0x1fbe4`, selector table `0x1fbe6`, calibrated against known
attribute order `0x21454`. [Evidence record](../original-quirks.md).

Weak is replaced by Drunk even when Drunk is zero and even when either is
255. Therefore ordinary fatigue can vanish at an eight-hour boundary
without Rest. The pin deliberately changes replacement into addition and
later adds a Rest-specific cure; neither workaround describes the original.
Dawn increments Weak with saturation at 255, not a signed -1 exemption.

Condition modifiers use unsigned counts and one attribute point per count.
Dead, Stoned or Eradicated suppress this entire modifier calculation.
For nonterminal members, the entries below add together:

| Condition | Might | Intellect | Personality | Endurance | Speed | Accuracy | Luck |
| --- | --- | --- | --- | --- | --- | --- | --- |
| In Love | +count | +count | +count | +count | +count | +count | +count |
| Heart Broken / Weak | -count | -count | -count | -count | -count | -count | -count |
| Insane | +count | -count | -count | 0 | +count | -count | 0 |
| Drunk | -count | -count | +count | -count | -count | -count | +count |
| Poison | -count | 0 | 0 | 0 | -count | -count | 0 |
| Disease | 0 | -count | -count | -count | 0 | 0 | 0 |
| Cursed | 0 | 0 | 0 | 0 | 0 | 0 | -count |

Weak 255 therefore subtracts 255. Attribute calculation finally floors at
zero in the current rules; that floor does not prevent the subsequent
attribute-death test. Full age/equipment/base-stat ordering belongs to the
character calculation, not to this additive condition table.

## Per-call recovery checks

**Evidence: Reference only.** Sources: [Party::changeTime][party],
[Character::charSavingThrow][character]. After calendar/dawn processing,
for every active member:

- If Confused, draw `U(0,2)`. On 1, perform the physical character save:
  `V = statBonus(effective Luck) + L`, draw `U(1,V+20)`, pass at `<= V`.
  Success clears Confused; failure decrements it by one. Other first-draw
  values leave it unchanged. This is not the monster's resource-ID save.
- If Paralyzed, draw `U(0,4)`; on 1 decrement by one. No second save.

These draws occur once per `changeTime` call, including calls that do not
reach eight hours. Absent conditions skip their draws. `addTime` alone
performs neither check. For Confused + Paralyzed, the order is Confused
gate, optional physical save, then Paralyzed gate for each member.
Original behavior for artificially invalid save intervals is **Unknown**;
MMModern's validation is not evidence of a DOS clamp.

## Dawn and temporary effects

**Evidence: Reference only** for the dawn mode/latch control flow;
**DOS static (retained)** confirms the temporary-reset field selection in
the direct-year Event path (`0x2329c`, `0x21043`, `0x1e828`) and Weak
increment (`0x26452`). Sources: [Party::addTime/resetTemps][party].

A changed final day number sets the pending `newDay` latch. At minute 300
or later, a pending latch is consumed once. Normally this resets temporary
effects, then either clears `rested` (if already rested or currently
sleeping), or increments all members' Weak and shows the need-rest message.
In script mode or `MODE_INTERACTIVE7`, the latch is still consumed but the
reset and fatigue/message block is skipped. Merely waiting until another
call therefore does not replay that skipped dawn.

The reset clears:

| Owner | Fields cleared |
| --- | --- |
| Each active character | Temporary values of all seven attributes, level, AC, and six personal resistances (fire, electrical, cold, poison, energy, magic). |
| Party | Fire/electrical/cold/poison protection, light count, levitate count, Walk on Water, Wizard Eye, Clairvoyance, Heroism, Holy Bonus, Power Shield, Bless. |

It does **not** clear temporary age, torch count, automap-on, permanent
attributes or resistances, conditions, HP, SP or inventory. Wizard Eye
clears automap-on when cast; its later expiry does not restore that flag.
A reset alone is not a heal, age cure or clock advance. See
[magic](magic.md) for the spells that set these values.

## Rest

Dream presentation has separate **DOS static** evidence (`0x1e605..0x1e72a`):
four step-4 retrace fades surround two sequential sample holds and two 7-BIOS-tick
waits. **DOS play**, maintainer 2026-10-13, confirms that Escape does not locally
skip the dream. [Capture measurements and limits](evidence.md#presentation-clock-captures-2026-10-13)
support the nominal clock rates and sequential header-derived sample durations.
Capture A/V displacement does not establish extra emulated sound holds. This presentation
evidence is separate from the mechanical checks below.

**Evidence: DOS static, high confidence (2026-10-18)** for the Rest sequence
below, including Super Shelter and food/terminal ordering
(`0x1e50d..0x1e826`); retained **DOS play** accepted ordinary Rest in M51.
Static branches do not claim DOSBox observation of every synthetic party
state. Weak/Drunk ordering retains its earlier evidence.
Source comparison: [Interface::rest][interface],
`chargeStep`, `doStepCode`; [Party::changeTime/resetTemps][party].

The primary map's Rest restriction refuses ordinary Rest. DOS cell lookup
also populates a cell restriction latch from attribute bit `0x40`
(`0x11fe7..0x11fec`); Rest tests it at `0x1e50d`. The pin's cell check is
ineffective because its loader cannot populate the flag it tests; see
[the restriction discrepancy](../original-quirks.md#sv-rest-cell-flag).
Super Shelter uses a
special entry mode that bypasses this ordinary refusal, after its own map
restriction check.

MMModern reads the DOS cell bit in `XeenEncounterFlow::beginRest`
(`XeenRestFlow.cpp`). Its Super Shelter
mode reuses that Rest continuation, with a single 480-minute sleeping call.
`XeenTravelSpellOriginalTests.cpp` checks the mainland refusal and shelter
bypass, warning cancellation and exact restoration; this is implementation
evidence rather than DOS play.

Before sleeping, inspect every member's seven effective attributes. Any
below 1 triggers the dying-risk confirmation; declining starts no Rest.
On admission set Asleep on all active members. Ordinary Rest then performs
ten `chargeStep` opportunities, followed by 470 more minutes indoors or
380 outdoors. Total is 480 minutes: `10*1+470` or `10*10+380`. Monsters can
move/attack during those ten opportunities. If interrupted, retain elapsed
time and attack consequences, and skip the suffix, dream, food and refill.
Do not wake untouched sleepers merely because the action was interrupted.
An eight-hour boundary crossed during those opportunities wakes everyone
through the [eight-hour block](#eight-hour-condition-block)'s clear.

Super Shelter instead calls `changeTime(480)` once, without those ten
ordinary opportunities. The difference in call partitioning can matter for
Confused/Paralyzed recovery even though total elapsed minutes match.
Its own map restriction is `0x200` (`0x305ef`); the handler enters mode 12
at `0x30605`, calls Rest at `0x30612` and restores its prior mode.
It bypasses ordinary map/cell Rest refusal, not the dying-risk warning.
Declining that warning returns before Asleep or time writes, but an
already-admitted Super Shelter retains its SP/gems or item charge.
Neither that decline nor a failed map guard invokes a spell refund.

After an uninterrupted time sequence:

1. Draw `U(1,20)`; on 1 show the dream.
2. Reset temporary effects before calculating recovery maxima.
3. Iterate active members in order and clear Asleep.
4. Check food **before** terminal eligibility. If food is zero, set the
   starving flag and give this member no refill. Otherwise set `rested`.
5. With food available, Dead/Stoned/Eradicated members consume none and get
   no refill. Every other member consumes one food, clears Unconscious and
   receives maximum HP and SP using the now-reset temporary values.
6. Apply terrain arrival, display completion/food-consumed and check defeat.

The DOS reset call is `0x1e72d -> 0x1e828`, before the food check
`0x1e754`, terminal test `0x1e770..0x1e778`, ration debit `0x1e77a` and
HP/SP refill `0x1e79b..0x1e7b8`. Its party-field clears at
`0x1e890..0x1e8b6` cover all the listed buffs even with no food. The later
Heroism/Holy Bonus/Power Shield/Bless clears at `0x1e784..0x1e78d` are
therefore redundant, as in the pin. The dream check precedes reset
(`0x1e5f1..0x1e605`); completed Super Shelter shares the same dream,
food and terrain suffix. Levitate has ended before `0x1e7df` calls arrival.
See [M76 spell evidence](evidence.md#m76-spells).

This is partial recovery in party order, not an all-or-nothing ration
check. With two food and six living members, only the first two refill.
With exactly six food and six living members, everyone refills and the
message is not starving, despite ending at zero. A trailing terminal member
visited after food runs out can set starving because the food test comes
first. If food is positive but all members are terminal, the loop can set
`rested` without consuming food.

There is no separate original post-refill Weak cure. The eight-hour block
already replaced Weak with Drunk. A member who entered Rest with Drunk 2
can therefore recover with Weak 2; computing maxima before that replacement
or applying the pin's later cure changes the result.

**DOS static (2026-10-17), high confidence:** ordinary Rest has no
surface-specific refusal (`0x1e50d..0x1e52e`). The original calls the shared
arrival routine after refill (`0x1e7df`); resetting temporary effects first
removes Levitate, so resting on cloud falls afterward.

## Outdoor movement skills

**DOS static, high confidence (2026-10-19).** The outdoor movement dispatch
requires two Mountaineer holders for wall types 1/7/9/10/12, and two
Pathfinder holders for wall types 3/6. Wall 5 uses the Mountaineer check
only on Darkside; Clouds uses its surface check. These are raw wall types,
separate from the resolved arrival surfaces below. The shared skill check
counts nonzero skill bytes on active members regardless of condition: dead
and unconscious holders count too. Navigator is skill 10, needs one holder,
and prevents the outdoor desert's extra 170 minutes; Pathfinder is skill 11.

Failing either movement skill check plays FX 21 and returns false.
The four movement callers skip position writes, time charging, the explicit
action-driven monster opportunity and terrain arrival on false. Ordinary
redraw/idle monster scheduling still runs; a refusal does not freeze it.
The pin agrees on Mountaineer and skill aggregation
but omits the Pathfinder wall consumer. MMModern implements both consumers in
`XeenMovement`, using `XeenSkills` for holder counts; the refusal plays FX 21
(M82). Fixed area admission still applies after a successful skill check.
See [offsets and calibration](evidence.md#outdoor-movement-skills)
and [the original/reference difference](../original-quirks.md#sv-pathfinder-movement).

### Stepped and lighting

**DOS static, high confidence (2026-10-19):** turns and waits mark stepped
through shared terrain arrival, even without changing cells. Refused movement
skips that suffix and does not mark stepped. Scene preparation performs the
lighting check; completed redraw clears stepped. A subsequent redraw derives
obscurity but skips drain unless another action or map load has marked stepped.
The rule is per lighting check with a pending step, not simply per displacement
or per final destination: Event Continue redraws intermediate destinations,
and later outer arrival can mark stepped again. Learned travel reaches the
ordinary successful-cast arrival; item travel retains its separate wrapper.
See the [travel-family offsets and restore boundary](evidence.md#travel-lighting-checks).

With a pending step, each scene check drains first, marks the occupied physical
cell with an active Cartographer, then derives darkness. Darkness does not
suppress exploration. A second draw without a new step changes neither value.

Current-format restoration is byte-exact before presentation. Gameplay marks
one pending step before the first Load/New Game frame; that frame can drain
and mark. Inn entry skips this arrival: leaving the Inn reloads the map and
marks its step. See [the restore boundary](evidence.md#restore-boundary).

## Terrain arrival

**DOS static (2026-10-17), high confidence.** Shared routine `070d:0194`,
`WORLD/XEEN.DAT` file `0x0c924..0x0ca48`, calibrated against ordinary Rest
and its known time/recovery order. Outdoor cells select a surface slot
from the wall word's low nibble (`0x11afb..0x11b01`); indoor cells use the
attribute byte's low three bits (`0x11b30..0x11b45`). Both resolve that slot
through the primary map's surface-types table (`0x0c93d..0x0c945`). Raw
slot numbers are map-dependent. Indoor raw slot 4 has a separate swimming
movement check; it must not be confused with resolved type 4, swamp.

| Resolved type | Name | Arrival effect |
| --- | --- | --- |
| 0 | Water/default floor | None |
| 1 | Dirt | None |
| 2 | Grass | None |
| 3 | Snow | None |
| 4 | Swamp | None |
| 5 | Lava | 100 fire damage per active member |
| 6 | Desert | Outdoors only, `addTime(170)` unless the party has Navigator |
| 7 | Road | None |
| 8 | Deep water | None |
| 9 | Tile floor | None |
| 10 | Sky | Fall, then 100 physical damage per member |
| 11 | Cloud road | None |
| 12 | Sewer | None |
| 13 | Cloud | Fall, then 100 physical damage unless Levitate is nonzero |
| 14 | Scorched | None |
| 15 | Space | Set party dead |

Type names are established by the resource-name table
`0x52677..0x5270a`. Lava is `0x0c95f..0x0c968`; sky
`0x0c96a..0x0c978`; desert `0x0c97a..0x0c996`; cloud
`0x0c998..0x0c9ac`; space `0x0c958..0x0c95d`. Swimming, Walk on Water,
Mountaineer and Pathfinder govern movement and give no arrival protection.
Levitate protects cloud only; it does not protect sky, lava or space.
Darkside game flag 118 suppresses the fall/damage suffix, including lava
damage (`0x0c9af..0x0ca27`). Space death and desert time are set before this
gate and remain effective. Arrival makes no direct random draw.

The original calls arrival from the step-time suffix (`0x0d411`), the
successful Cast/Shoot suffix (`0x0d5ba`) and completed Rest (`0x1e7df`).
Consequently turns, waits and successful stationary casts/shots can repeat
a hazard. Jump and teleport spells reach the same successful-cast suffix.
Internal TeleportAndContinue chains finish before the outer interaction's
step-time suffix: only their final destination receives that terrain call.
Manual Space first completes its Event chain, then charges 1 minute indoors
or 10 outdoors and runs the ordinary monster opportunity before arrival
(`0x0d468..0x0d482`). Event-free Space shares this sequence. Blocked Bash
skips arrival (`0x0d547..0x0d54f`, jump to `0x0d5c7`). Handled Space doors
and grates also skip the suffix: openGrate clears the current key at
`0x0c911` before returning (`0x0c917..0x0c91a`), and dispatch tests it at
`0x0d263`. Bash pass-through retains its charged movement and arrival.
Terminal services
consume their separate farewell and clear the current key before returning
(`0x1d782..0x1d7d3`, key clear `0x1d7c8`), so they do not receive an extra
Space charge, monster opportunity or arrival.
Automatic Events do not repeat the movement call that already reached them.
Rest arrival follows temporary-effect reset and recovery; falling occurs
before its fixed damage, using the destination map's ordinary load lifetime.

Clouds surface fall destinations (`0x0ca60..0x0cd20`, table `0x0cdf4`):

| Source maps | Map, position | Facing |
| --- | --- | --- |
| 25, 51-54 | 24, (11,9) | Preserve |
| 26, 55-58 | 12, (6,15) | Preserve |
| 27, 59-62, 76-78 | 15, (4,12) | South |
| 65-66 | 14, (8,3) | South |
| 69-70 | 1, (8,7) | North |
| 74 | 12, (11,13) | South |
| 82-84 | 5, (12,7) | North |
| 85 | 6, (12,3) | North |
| Other | 23, (12,10) | North |

Explicit Event falls use the script's own destination and signed damage
byte (`0x0cc93..0x0ccb4`, `0x0cd00..0x0cd0b`), rather than this fixed
surface-fall damage.
They preserve facing, load before outdoor edge normalization and redraw the
destination before damage. Surface arrivals normalize outdoor edges too.
For valid neighbors, normalization adjusts Y/South-or-North first, then
X/West-or-East through that selected map, by 16 per flagged axis, preserving
facing. See [integration offsets and limits](evidence.md#fall-and-outdoor-edge-integration).

Darkside surface falls retain the original generic mapping even though its
maps are not admitted to play. Sources 89-112 fall to map minus 88 with their
coordinates and facing retained (`0x0cb70..0x0cb86`). The remaining special
dispatch only covers 25-129 (`0x0cb93..0x0cb98`, table `0x0cd22`); sources
outside it use Darkside 29 `(25,21)` North. The pin additionally includes 113
in the subtraction range and 130-132 in its special cases.

For each terrain-damage recipient the original starts again at 100
(`0x23de0..0x23de8`) and clears Asleep (`0x23dfa..0x23e04`), including
terminal members. Fire subtracts party fire resistance, then repeatedly
draws `U(1,R+40)`, where `R` is permanent + temporary fire resistance plus
equipment scan 11. A roll at most `R` halves positive damage and repeats;
even reaching zero incurs a final draw (`0x23e84..0x23eb5`, save helper
`0x2d13d..0x2d161`, `0x2d1fa..0x2d21d`). Physical damage skips saving
throws. Finally subtract Power Shield and clamp at zero
(`0x23ee5..0x23f10`). Example: party fire resistance 20, `R=10`, shield 3,
rolls 5 then 20 from 1..50: `(100-20)/2-3 = 37` damage. Each subsequent
member starts from 100. A physical fall with shield 3 deals 97, with no
saving draw. Pinned ScummVM carries reductions between party members;
terrain follows the original fresh value.

### Falling presentation

**DOS static, high confidence (2026-10-23):** the terrain transition retains
its original destination, fixed 100 damage and RNG rules. Before publishing
that arrival, it displays 15 accelerated scene scrolls and four impact images
from the original source/destination views. Source offsets are
`0,3,7,12,18,25,33,42,52,63,75,88,102,117,132,127,132,129,132` in the 216x132
scene. Each waits one 70-Hz VGA retrace; the enclosing full draw also has its
two-BIOS-tick minimum. Scream stops after offset 132, before the four impact
images; [audio](audio.md#voc-inventory-boundaries) gives the cue sites.
`XeenEventFlow::beginFall` is the single presentation entry used by the
existing terrain transition, including its manual-step path. Input follows
the existing blocking-presentation policy; repaint and unacquired frames do
not advance it. No save fields or gameplay clock ticks are added.
[Original offsets and pin difference](../original-quirks.md#sv-fall-presentation).

M86 covers terrain falls only. The original Clouds Events do not use
`FallToMap` (opcode 52); its Darkside uses remain unsupported. A future caller
can reuse the presentation without changing these terrain rules.

## Calendar economy and direct year changes

**Evidence: DOS static, high confidence** within the playable word-sized call
domain. Calendar `0x263b3..0x26491`, stock `0x26401`, interest `0x26408`,
latch `0x2640b..0x26411`, interest arithmetic `0x26492..0x264c4`; calibrated
against Weak/Drunk and direct-year behavior. Reference comparison:
[Party::addTime/giveBankInterest][party]. After advancing the clock, if the
final day differs from the starting day and either `final day % 10 == 1`
or the requested increment exceeds 1,440 minutes, regenerate merchant
wares once and then apply bank interest once. Gold and gems independently
increase by `floor(balance/100)`. These are not per-crossed-day loops.
A balance of 99 gains 0; 199 gains 1. Merchant regeneration precedes the
interest and dawn steps and consumes its own RNG.

A call of exactly 1,440 minutes does not satisfy the long-call clause; it
needs to end on the ordinary `day % 10 == 1` schedule. Current 1,440/2,880-minute
service calls agree with DOS. The final-day comparison gates both economy and
setting the pending dawn latch; equality does not clear an existing latch.
Pending dawn still processes at minute >= 300, including mode-based consumption.
A synthetic 144,000-minute (100-day) call can return to the same day number in
the reference, but is not representable by DOS's word-sized argument. Do not
claim that example as a DOS observation or extend this confirmation to arbitrary
host-width increments. [Retained evidence](evidence.md#other-playable-rules-confirmations).

**DOS play + DOS static (retained):** the jail's direct year penalty is a
different operation. It changes the year while preserving day and minute,
resets yearly flags 55/155/222/231 and temporary effects, and does not
simulate all intervening days. DOS file offsets `0x2329c`, `0x21043`,
`0x1e828`, retained 2026-10-11 analysis calibrated against the maintainer's
jail observation. No invented daily rolls, merchant regeneration or bank
interest belongs in this path. Temporary age and torch count survive.

## Training

**DOS static (2026-10-20), high confidence.** Clouds map 28 permits training
to permanent level 10, map 30 to 15, and every other map to 20 (including
Newcastle 73). The cap is the only map-dependent numeric Training rule.
Both the displayed quote and payment use `10 * L * L`, with `L` the
current permanent level; XP requirements and the level increase use the
same character routines everywhere ([level thresholds](combat.md#level-thresholds));
the cap and missing experience are checked before the condition refusal,
which precedes payment (`0x1c8d1..0x1c933`, `0x1ab5b..0x1abb9`, condition
helper `0x1e3bc..0x1e41c`).
From level 12 each level needs and debits 1,024,000 XP. The pinned
`locations.cpp::TrainingLocation` agrees on caps and cost, but its thresholds
make Training from 12 to 13 free. Vertigo's existing cap 10 supplies the
gameplay calibration.
[Offsets and comparison](evidence.md#other-town-services-2026-10-20).

## Tavern Food

**DOS static (2026-10-20), high confidence for the tabled Clouds maps.**
The offer fills food to the listed amount times the active-party count,
without filtering disabled or terminal members. The price is for the whole
refill, not per member or missing ration. Text indices address `tavern.bin`.

| Clouds map | Food per active member | Gold | Offer text index | Pin 6814ee9b |
| --- | --- | --- | --- | --- |
| 28 Vertigo | 15 | 10 | 75 | Same |
| 30 Rivercity | 60 | 50 | 76 | 50 / 50 / 76 |
| 49 Shangri-La | 120 | 100 | 77 | Same |

After Yes, compare the unsigned food word with the target: `food >= target`
shows Full without payment. Otherwise, successful payment replaces food
with the target; it does not add that amount. The pin agrees on this test
and settlement.

On an untabled Clouds map (29, 31, 32, 73, 74 or any other), DOS skips all
three offer assignments: the target and price globals retain their prior
values, and the automatic text selector has not been initialized on this
path. **Reference only** for a usable fallback: the pin explicitly supplies
15 / 10 / 75, but an exact DOS text index is not established. No original
Clouds Tavern uses this path. [Evidence and limits](evidence.md#other-town-services-2026-10-20).

## Temple Heal price

Paid Heal clears all six temporary resistances (`0x1ade7..0x1ae00`), as well
as temporary attributes and level before assigning HP.

**DOS static (2026-10-16), high confidence.** Price routine
`WORLD/XEEN.DAT` `0x1c9ce..0x1cd3e`, calibrated against the reference
formula it shares and against the Vertigo prices of the original-data Temple
tests. `L` is the permanent plus temporary level byte, never negative
(`0x21fc9`). Totals are 32-bit. The Heal price is the sum of:

| Term | Price | Offsets |
| --- | --- | --- |
| HP below maximum | `10*L + heal surcharge`; compared as unsigned words, so negative HP pays nothing | `0x1cab3..0x1cb02`, comparison `0x1cadb` |
| Each of conditions 1..12 (Heart Broken through Unconscious) with a nonzero counter | `10*L` | `0x1cb08..0x1cb40` |
| Dead, Stoned (each) | `100*L + 50*n + resurrect surcharge`, `n` the counter | `0x1cb4e`, `0x1cb93` |
| Eradicated | `1000*L + 500*n + eradicate surcharge`; `500*n` is a signed 16-bit product | `0x1cbd8..0x1cc1e` |

Cursed and cursed items enter only the Uncurse price (`20*L + uncurse
surcharge`, `0x1cc22..0x1cce4`). Surcharges come from the current map
(`0x1c9f5..0x1cab3`; Clouds switch table at file offset `0x1ce87`,
corrected by the 2026-10-20 calibration):

| Price selector | Heal | Resurrect | Eradicate | Uncurse | Donation |
| --- | --- | --- | --- | --- | --- |
| Clouds 28 (Vertigo), Darkside 29 | 0 | 0 | 0 | 0 | 10 |
| Clouds 30, Darkside 31 | 10 | 50 | 500 | 100 | 25 |
| Clouds 73, Darkside 37 | 20 | 100 | 1000 | 200 | 50 |
| Clouds 49, any other Darkside map | 100 | 500 | 5000 | 300 | 100 |

Clouds temples are on maps **28, 30, 49 and 74**; map 73 is a price-table
case, not an original Temple site. Map 74 reaches the routine with
`DS:ed99 = 74` and misses every case. **DOS static (2026-10-20), high
confidence:** it keeps five process globals: `DS:f656` Heal, `f654`
Resurrect, `f652` Eradicate, `f650` Uncurse and `f64e` Donation. Entry resets
quotes and the visit blessing latch, not these globals.

At map 74 the selected member pays the formulas above with the last
assigned surcharges; Donation uses the last fixed price:

| Last Temple visited in this process | Heal surcharge | Resurrect surcharge (each Dead/Stoned term) | Eradicate surcharge | Uncurse surcharge | Donation |
| --- | --- | --- | --- | --- | --- |
| None since program start | 0 | 0 | 0 | 0 | 0 |
| Clouds 28 | 0 | 0 | 0 | 0 | 10 |
| Clouds 30 | 10 | 50 | 500 | 100 | 25 |
| Clouds 49 | 100 | 500 | 5000 | 300 | 100 |

Another visit to 74 preserves the same row. The same assignments occur at
Darkside 29, 31, 37 and other Darkside maps, respectively selecting the
zero/low/middle/high rows of the first table. With no previous assignment,
Heal and Uncurse still charge their base formulas; a zero Donation quote
does nothing, as the Donation action refuses a zero quote.

Startup zeroes all five. Normal New Game, Load, Save, map changes and other
locations do not reset or write them. They are **absent from the original
save**. A fresh process followed by New or Load and no Temple visit gives
the zero row; a Load or New inside a process that already visited a Temple
retains that process's row, even if the loaded party visited a different
Temple when saved. The pin instead constructs a new `TempleLocation` for
each visit and initializes its corresponding members to zero: map 74 always
gets the zero row there. This corrects the previous claim of equivalent
carry-over. [Evidence and save bounds](evidence.md#other-town-services-2026-10-20).

Donation is 0 after this visit's blessing
latch is set (byte at `0x47ad:0`), independently of an existing party
Blessed effect. On the tabled maps the pinned reference agrees except for
the signed HP comparison and the full-width Eradicated product; see
[original quirks](../original-quirks.md). Worked examples: level 3 at HP -15,
Unconscious and Dead 1 at Vertigo costs `30 + 350 = 380`; the same character
at HP 0 costs 410. Implementation: `xeenTempleSurcharges` and
`xeenQuoteTempleHeal` in
[XeenTempleHeal.cpp](../../src/games/xeen/XeenTempleHeal.cpp), tested by
[XeenTempleRulesTests.cpp](../../tests/XeenTempleRulesTests.cpp).

## Tavern and Temple options

**DOS static (2026-10-17), high confidence.** Calibrated against the Temple
price routine above and the original resource strings. Service actions
below add no action time; normal departure still advances time and may
regenerate merchant stock, apply interest and consume the associated RNG.

- **Drink:** an acting member pays 1 carried gold, receives the original
  Good Stuff status and sets a visit-local bought-drink latch. Exactly one
  `U(1,100)` draw follows payment; rolls 1..25 increment the selected
  member's Drunk byte, including byte wrap (`0x1b077..0x1b192`). A disabled
  member or failed payment makes no draw. Selecting a member clears the latch.
- **Tip:** an acting member without that latch gets the original Have a
  Drink status. With the latch, clear it first; Drunk refuses without payment,
  otherwise pay 1 gold and show `tavern.bin[base+tipIndex]`. Then store
  `(byte(tipIndex+1)) % 15` on Clouds, `%10` on Darkside
  (`0x1b349..0x1b521`). No random draw. The cursor belongs to the map and
  survives revisits and saves. Clouds town bases: 28 -> 30, 30 -> 45,
  49 -> 60; Darkside: 29 -> 30, 31 -> 40, others -> 50. The pin uses
  division instead of remainder and checks Clouds map 45 instead of 30.
- **Rumors:** free, no random draw and no persistent change. Show
  `tavern.bin[base + day%10]`, so a day's rumor repeats
  (`0x1b626..0x1b6ad`). Clouds bases: 28 -> 0, 30 -> 10, 49 -> 20;
  Darkside: 29 -> 0, 31 -> 10, others -> 20.
- **Donation:** pay the current temple's fixed price, then increment a
  visit-local counter modulo 10. When it equals `day%10`, set Clairvoyance,
  Light=1, and Blessed, Power Shield, Holy Bonus and Heroism to that counter
  (10 for zero). Set the visit-blessed latch so further Donation quotes are
  zero (`0x1ae8f..0x1af73`). Counter resets on entry and character selection;
  the blessed latch resets on entry. No random draw or persistent donation
  counter. Worked values: day 8 requires eight successful donations from a
  fresh counter; day 0 requires ten. At Vertigo that costs 80 or 100 gold.
- **Uncurse:** when the selected member is Cursed or any carried item's
  state has bit `0x40`, charge `20*L + uncurse surcharge`. After payment
  clear Cursed and only that bit from all nine item slots in all four
  categories (`0x1cc1e..0x1cce0`, `0x1af79..0x1b01b`). Preserve the items,
  their equipped status, other state bits and even empty-slot metadata.
  No random draw. A level-3 member costs 60 in Vertigo or 160 in Clouds
  temple 30, regardless of the number of cursed slots.

Each paid Heal or Uncurse **adds** 1,440 to a visit-local 16-bit farewell
accumulator (`0x1ae68`); the common exit adds another 1,440 and passes the
word to `addTime` (`0x1d78e..0x1d798`). Thus zero, one and two successful
purchases cost one, two and three days. Addition wraps as an unsigned word;
Donation contributes no paid-day increment. Pinned ScummVM instead assigns
one paid day, so repeated purchases do not accumulate there.

The called DOS `addTime` also adds the current minutes and charge in a word
before normalizing unsigned (`0x263b3..0x263e5`, addition `0x263be`). For
example `1300 + 65000` wraps to minute 764 on the same day. Its merchant
regeneration test requires a changed day and either a resulting day ending
in 1 or the original charge interpreted as signed greater than 1,440
(`0x263e7..0x2640b`, signed branch `0x263f9`). Starting at day 0 minute
604, charge 32,767 reaches day 23 minute 251 and regenerates; charge 32,768
reaches minute 252 and does not. These service-only word edges are retained
without changing the ordinary interactive time owner.

Sign In enters the original inn party manager, after writing town/side and
advancing a day (`0x1b6b0..0x1b79a`, outer dispatch `0x0b819..0x0b839`,
inn guard/list `0x394b7..0x395d5`). Its rules are in
[Sign In and time](party.md#sign-in-and-time).

## Executable witnesses and limits

Implementation evidence, not independent DOS proof:
[XeenCombatRules.cpp](../../src/games/xeen/XeenCombatRules.cpp)
(`XeenConditionTimeCandidate`, temporary reset functions),
[XeenRestRules.cpp](../../src/games/xeen/XeenRestRules.cpp),
[XeenCharacterRules.cpp](../../src/games/xeen/XeenCharacterRules.cpp),
[XeenDailyTimeOracles.h](../../tests/XeenDailyTimeOracles.h),
[XeenRestTests.cpp](../../tests/XeenRestTests.cpp), and
[XeenRestOriginalTests.cpp](../../tests/XeenRestOriginalTests.cpp).
They cover calendar rollover, one-block long calls, ordered condition
transitions/draws, byte boundaries, food distribution and original-data
Rest integration. An original-data automated test runs MMModern with those
resources; it is not a DOS play observation.

The major remaining DOS questions are remaining script-mode dawn behavior,
unusual condition-byte edges. Super Shelter's call partition and shared terrain
suffix have the M76 static evidence above. The established DOS
Poison/Disease and Weak/Drunk findings supersede conflicting old milestone
prose; retain that distinction when implementing adjacent mechanics.

[party]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[interface]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp
