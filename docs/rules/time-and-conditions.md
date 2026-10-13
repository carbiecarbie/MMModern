# Time, rest and conditions

Baseline: MMModern `9cc4def8c5c0253899c094efbe5b4d7e4513b22c`;
ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. **Reference only**
means source-verified, not DOS-confirmed. **DOS static (retained)** denotes
calibrated analysis recorded in [original quirks](../original-quirks.md),
not fresh executable analysis by this chapter's author. **DOS play** is
retained maintainer evidence. **Unknown** identifies unverified original
behavior. These distinctions matter: the pin contains intentional changes
and bugs in precisely these rules.

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

**Reference only:** `Interface::chargeStep` charges 1 minute indoors or 10
outdoors and offers the normal monster response. Real wall-clock waiting
or animation is not the same as calling `changeTime`. Combat, travel,
services and scripts must retain their actual callers' operation and mode;
this chapter is not a universal action-cost table.

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

**Evidence: Reference only** for ordinary calendar economy. Sources:
[Party::addTime/giveBankInterest][party]. After advancing the clock, if the
final day differs from the starting day and either `final day % 10 == 1`
or the requested increment exceeds 1,440 minutes, regenerate merchant
wares once and then apply bank interest once. Gold and gems independently
increase by `floor(balance/100)`. These are not per-crossed-day loops.
A balance of 99 gains 0; 199 gains 1. Merchant regeneration precedes the
interest and dawn steps and consumes its own RNG.

A call of exactly 1,440 minutes does not satisfy the long-call clause; it
needs to end on the ordinary `day % 10 == 1` schedule. A call spanning an
exact 100-day year can return to the same day number and skip the day-change
economy/newDay tests. These are literal reference control-flow consequences;
**Unknown** whether original DOS has all long-call edge cases unchanged.

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

The major remaining DOS questions are exact multi-day-call economy/latch
behavior, script-mode dawn suppression, unusual condition-byte edges and
Super Shelter's full interruption/terrain interaction. The established DOS
Poison/Disease and Weak/Drunk findings supersede conflicting old milestone
prose; retain that distinction when implementing adjacent mechanics.

[party]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[interface]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/interface.cpp
