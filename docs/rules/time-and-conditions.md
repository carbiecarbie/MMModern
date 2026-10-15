# Time, rest and conditions

Baseline: MMModern `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`;
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. **Reference only**
means source-verified, not DOS-confirmed. **DOS static (retained)** denotes
calibrated analysis recorded in [original quirks](../original-quirks.md),
not fresh executable analysis by this chapter's author. **DOS play** is
retained maintainer evidence. **Unknown** identifies unverified original
behavior. These distinctions matter: the pin contains intentional changes
and bugs in precisely these rules.

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
- Sum the selected members' effective Might **after the HP costs**, then draw
  `R(1,30)` once (`0x32e87..0x32ece`, inclusive range helper `0x45b9a`).
  Attribute 0 reads the unsigned permanent/temporary Might bytes, adds age,
  equipment and condition modifiers, and clamps at zero (`0x21454..0x215a9`).
  The Bash sum and comparison use 16-bit words; there is no 8-bit sum truncation
  or cap explaining refusal for two members with Might above 100.
- The primary map's difficulty byte 5 is used for wall 9; byte 6 for every
  other rolled wall (`0x32ed0..0x32ef3`). These are DAT offsets 821 and 822,
  memory `ds:0xc101` and `ds:0xc102`, both **unsigned**. Vertigo has 30 in both,
  matching the original initial state and inspected DOS `.CUR` files. Sum
  greater than or equal to difficulty succeeds (`0x32ef5..0x32ef7`).
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
| NPC | Three ticks, draw, reset, then input poll. The inner wait is uninterruptible. Page changes retain the running CD interval (`0x25b8f..0x25bad`). Each fresh invocation resets its local playback guard (`0x256ec`, `0x25ae8..0x25af3`). |
| CD speech | Decode MMSSFF as 75-Hz frames; duration is exactly `(end-start+25)/75` seconds from playback call. Mode 2 starts/replaces playback and returns without stopping (`0x25aa8`); ordinary final dismissal stops (`0x25c94`). An all-zero cue neither starts nor stops (`0x3da98..0x3daa7`). WhoWill keeps the passive face; Event cleanup closes its window without an audio stop. Busy polls choose `R(0,3)` from the existing Journey RNG; idle uses face zero without a random request. See [capture limits](evidence.md#death-and-cd-speech-captures-2026-10-13). |
| Shops | Poll input before the three-tick wait, so input may end it early. While the current sample is active, Bank chooses `R(7,11)`, Smith `R(5,7)`, Temple `R(2,4)`, Training `R(8,12)`. Smith replaces base art while its logical base index still advances. Guild and Tavern have no Clouds speaking choice. |
| Sparkle | Complete four frames for one recipient before the next. Each frame waits two outer ticks in window 11, otherwise one plus nested draw3d. Suspend and restore the actor countdown (`0x21087..0x21186`). Window 11 directly publishes the sparkle without nested draw3d, so NPC face activity also waits until this helper returns. A per-member latch skips a recipient already sparkled in the same Event run (`0x210bc`, set `0x2117f`); Event start and end clear it (`0x267a6`, `0x291e0`), as does HP addition (`0x2ee43`, `0x2eeca`). |
| Damage | Monster recipients each have a two-tick uninterruptible hold. Generic Event/trap damage publishes overlay and injury in recipient order, then shares one final two-tick hold (`0x23f73`, `0x2d6c4`, `0x264ed`). |
| Title | Thirteen phases; phase zero is background alone. The first CD fade uses step four. Publish phase one after the acquired final fade level, then use interruptible four-tick idle waits (`0x0be9a..0x0bf8d`). |
| Party death | Eighteen fire publications have retrace cost and no explicit BIOS hold. Laugh iterations use two ticks and sample status, one restart at index ten, maximum executed frame 21, and no sound tail. Closing hands use one-tick interruptible waits; fade out with step four, then return to the World title established by capture. |

Silent VOC gates derive duration from sample count and header rate. The installed
samples use a 68-microsecond period; a new sample replaces the previous interval,
and stop/completion clears activity without requiring an audio mixer. Greetings
do not impose an uninterruptible delay. Action/refusal samples replace greetings;
Tavern goodbye retains the last screen until its sample completes, without
advancing a shop animation. Speaking draws occur before later service-day
generation, so an early input can change how many visual RNG requests precede
that gameplay work. Retain chosen results across composition failure/retry.

## Eight-hour condition block

**Evidence: Mixed.** The order and branches below are verified in
[Party::changeTime][party] and MMModern's
[XeenConditionTimeCandidate::service](../../src/games/xeen/XeenCombatRules.cpp).
Poison/Disease and Weak/Drunk use the stronger DOS evidence below instead
of the pin. Other ordinary branches are **Reference only**. Extreme byte-counter
edges below describe current implementation choices separately; they are
**not confirmed in DOS** or literally equivalent to the pin's live integers.

Process active members in order. Finish all these steps for one member
before beginning the next:

1. If not Dead, Stoned or Eradicated, inspect all seven effective attributes.
   Any value below 1 sets Dead to 1. Do not infer an HP assignment here.
2. Increment nonzero Heart Broken; if the incremented count exceeds 10,
   clear it and set Depressed to 1.
3. Process existing Poison, then existing Disease, using the DOS rule below.
4. Increment nonzero Insane. Increment each nonzero Dead, Stoned and
   Eradicated counter, retaining terminal saturation at 255 in MMModern.
5. Increment nonzero In Love; above 10, clear it and set Heart Broken to 1.
6. **DOS:** replace Weak with Drunk, then clear Drunk, unconditionally.
7. Advance nonzero Depressed to `(count+1) % 4`.

Consequences of order: Heart Broken 10 becomes Depressed 1 and then
Depressed 2 in the same block. In Love 10 becomes Heart Broken 1 after the
Heart Broken step, so it does not immediately advance again. A new Dead 1
from the attribute check reaches Dead 2 later in that same block.
Terminal members skip only the attribute-death test, not the later
Poison/Disease or other counter steps.

**Current MMModern arithmetic; DOS boundary behavior unknown.** Insane 255
becomes 0; Heart Broken/In Love 255 wrap to 0 before their `>10` comparison;
Dead/Stoned/Eradicated saturate at 255. These choices are explicit in
`XeenConditionTimeCandidate::service` and `XeenDailyTimeOracles.h`.
The pin instead stores live `int` counters and truncates them only when
serializing. At 255, Insane and terminal counters become 256; Heart Broken
and In Love cross the threshold and trigger their transitions. The pin's
zero-to-minus-one terminal guard does not run at 255. It therefore does not
establish MMModern's byte boundaries. The root reference-fallback instruction
remains authoritative; this chapter reports the discrepancy without approving
it. The retained DOS findings settle Poison/Disease and Weak/Drunk, not these
other counters. See [original quirks](../original-quirks.md#unconfirmed-in-dos-following-the-reference).

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
evidence does not upgrade the remaining Rest mechanics below to DOS confirmation.

**Evidence: Reference only** for exact sequencing, food edge cases and
interruption; retained **DOS play** accepted ordinary Rest in M51, but does
not establish every synthetic case below. Weak/Drunk ordering uses the
stronger DOS static evidence above. Source: [Interface::rest][interface],
`chargeStep`, `doStepCode`; [Party::changeTime/resetTemps][party].

The primary map's Rest restriction refuses ordinary Rest. The pin's cell
flag check is ineffective because the loader cannot populate that flag;
this is recorded in [original quirks](../original-quirks.md), **Reference
only**, not a separately confirmed DOS cell rule. Super Shelter uses a
special entry mode that bypasses this ordinary refusal, after its own map
restriction check.

Before sleeping, inspect every member's seven effective attributes. Any
below 1 triggers the dying-risk confirmation; declining changes nothing.
On admission set Asleep on all active members. Ordinary Rest then performs
ten `chargeStep` opportunities, followed by 470 more minutes indoors or
380 outdoors. Total is 480 minutes: `10*1+470` or `10*10+380`. Monsters can
move/attack during those ten opportunities. If interrupted, retain elapsed
time and attack consequences, and skip the suffix, dream, food and refill.
Do not wake untouched sleepers merely because the action was interrupted.

Super Shelter instead calls `changeTime(480)` once, without those ten
ordinary opportunities. The difference in call partitioning can matter for
Confused/Paralyzed recovery even though total elapsed minutes match.

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

**Reference only:** outdoor desert arrival adds 170 minutes through
`addTime` unless the party has Navigator. Indoor surfaces do not incur that
extra time. Other terrain can cause damage, death or falling; this chapter
does not establish a safe-Rest exemption for lava/sky/cloud/space. MMModern's
currently approved unsupported-terrain refusals are implementation limits,
not rules of the original game.

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
unusual condition-byte edges and
Super Shelter's full interruption/terrain interaction. The established DOS
Poison/Disease and Weak/Drunk findings supersede conflicting old milestone
prose; retain that distinction when implementing adjacent mechanics.

[party]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[interface]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp
