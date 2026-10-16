# Milestone 70 - World Events (Clouds)

**Tier A, a foundation and three parts. Status: approved after the
independent plan review (maintainer, 2026-10-15); implementation not
started.** Baseline `3b4dab9`
(v0.2.0). Step 4 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems).

## Goal and boundary

Every Event operation the **Clouds** scripts use, other than audio, town
services and the ending, runs as in the original, so Clouds Event rows stop
blocking maps in [the coverage report](coverage.md). Map changes persist
through M69's per-map state.

**Clouds only** (maintainer decision, 2026-10-15). Darkside-only operations
(ChooseNumeric, ExchObj, FallToMap, GotoRandom, FlipWorld, DisplayLarge,
ConfirmWord_2, Darkside GiveEnchanted forms) and Darkside durable state
(per-character flags of mode 107, Darkside quest and game flag banks) belong
to a later Darkside arc. Operations shared by both sides are implemented
generically: no Clouds-only assumption where the original does not make one,
so Darkside operands can be added later without restructuring.

Out of scope: audio (`PlayFX`, `PlayEventVoc`, `PlayCD` keep their timing and
stay partial); `DoTownEvent` services (arc steps 5 and 8); the Clouds ending
(`CutsceneEndClouds`, opcode 30, and the ending entry paths inside
`ConfirmWord`); admitting maps (step 9).

Tier A: Events change persistent map state, draw random numbers and move the
party; the M44 scenarios may change. Independent plan (done, 2026-10-15) and
implementation reviews are mandatory.

Authority: calibrated static analysis of `WORLD/XEEN.DAT` (SHA-256
`8bf78b48...`; file offsets; read-only, work copies outside the repository),
maintainer DOSBox evidence where reachable, and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` (`scripts.cpp`) where DOS was not
checked. Event dispatch is calibrated at `0x26c50..0x26c69` (table `0x29320`).

## Durable state first

Before implementation, inventory every durable value the Clouds operations
read or write: map state (wall items, cells, Event opcodes, objects,
monsters), party and character fields, the Clouds game flags, quest flags and
quest counters, party effects. M69's v13 already carries the map fields on
the wire; the live owner still needs arbitrary Event-opcode and wall-item
override storage, mutation guards, effective rendering and script reads,
capture, resource-bound restore and removal of the now-obsolete refusals
(`XeenSaveState.cpp:81-82`). If any Clouds value has no durable owner, stop
and amend this plan before changing the save format.

## DOS differences from the reference, decided now

Follow DOS in each case and record it in [original quirks](original-quirks.md):

1. **If mode 37** reads temporary Might (CHR +0x15) with no map check
   (`0x218d7`, `0x21b44`); the reference substitutes full Might on map 82 as a
   workaround.
2. **Mirror destinations**: on Clouds maps above 32 the scan starts at entry
   0x26 (`0x2406c..0x2407c`) and covers the bounded list
   (`0x240d5..0x24118`); the teleport reads the selected record directly
   (`0x2702b..0x270a1`), without the reference's Dragon Tower correction.
3. **MakeNothingHere** clears every Event opcode at the party's physical cell
   (`0x2793b..0x279ec`) and continues with ordinary line advancement
   (`0x28356..0x28372`, `0x28d30..0x28d3a`); the reference exits instead. A
   script called at another logical cell can continue.
4. **Password comparison** matches the input directly against the primary
   and alternate resource strings (`0x24023..0x24067`); the reference's
   rewrite of a Vowelless Knight answer is not imported. Original resource
   mistakes stay.

## Foundation - developer fixture and publication contract

Built before Part A. An isolated developer session using the normal loader,
interpreter, Journey RNG and candidate/publication owners, with a chosen
Clouds map, position, direction and reproducible party prerequisites, absent
from the title menu, saving disabled. Allowing the fixture's selected map is
separate from resource, ownership and mutation validation, which all still
run. A shared transition result handles unavailable destinations: failed
travel leaks no candidate map, party or RNG change, and the plan for already
published script prefixes is stated and tested. Separate tests exercise the
real v13 serializer and restore for every newly persistent change, including
map revisit and refused travel.

## Part A - map changes and Spawn

1. **MoveWallObj** (34): slot plus signed x/y, position only, no flip
   (`0x2860e..0x2863a`); persisted per logical MOB map and record.
2. **MakeNothingHere** (46) as decided above, with a CallEvent witness that
   separates physical deletion from logical execution.
3. **AlterMap** (opcode 19, census name `AfterMap`): x/y, direction, wall
   value; direction 4 sets all four faces of the selected cell only
   (`0x27b41..0x27ba3`, helper `0x234dd..0x23550`); persisted on the physical
   DAT tile.
4. **AlterCellFlag** (35): a surface change, outdoors in the low wall-word
   bits, indoors in the low attribute bits of the physical tile
   (`0x2863d..0x28721`), keeping other bits; indoor neighbour aliasing tested.
5. **AlterEvent** replacements, including nonzero opcodes and the physical
   line lookup.
6. **Generic Spawn** outside Vertigo: existing, revived and new slots,
   type and statistics binding, outdoor coordinates, accounting and
   publication, keeping the approved cosmetic Spawn-frame RNG policy.
   *Amended (maintainer, 2026-10-15):* a slot at or beyond the MOB monster
   count stays inert and unsaved, following DOS (`0x9c59`), including
   Vertigo's exit-script slots 50 and 51. Saves holding such slots load
   without them; v13 is unchanged.
7. Lifetime rules from M69: outdoor neighbour DAT changes are not written
   back, indoor neighbours are, Clouds map 85 is visit-only.

## Part B - script flow, conditions, rewards and random draws

1. **If1/If2/If3** (8/9/10) compare `>=`, `==`, `<=` with mode-specific
   operand widths (32-bit for 16/34/100; 16-bit for 25/35/101/106; byte
   otherwise) and the selected member, all members or all but the combat
   target (`0x2712d`, `0x21b84..0x21bd8`); `IfMapFlag` (42) and `ItemType`
   (45).
2. **SetVar** (27) through the original `setValue` semantics, not a raw byte
   store (mode 74 sets several party effects, condition mode 18 is special,
   83 is the script damage type, 84 the facing); decoding is widened
   explicitly. Condition-counter edge cases follow the repository's ScummVM
   fallback policy.
3. **TakeOrGive**, **GiveMulti**, **TakeOrGive_2** modes, **SetChar**
   selectors (including 7, random), the Clouds **GiveEnchanted** forms and
   **NPC** modes, reusing the reward, sparkle and publication rules.
4. **Random operations**: JumpRnd (23) draws `1..N` once and jumps only on
   equality (`0x28131..0x2815d`); RndDamage (33) draws `1..max` once before
   ordinary typed damage (`0x285cc..0x2860b`); SelectRandomChar (43) draws
   `1..party size` once with no reroll (`0x287d8..0x287eb`). Every draw of
   SetChar, reward generation and damage is inventoried; draws use the
   Journey generator at the original placement.

## Part C - travel and typed input

1. **Teleports**: TeleportAndExit (7) and TeleportAndContinue (31) to every
   Clouds destination form. Destination 0 consumes the mirror selection made
   by ConfirmWord; direction -1 keeps facing; the run location sentinel
   (`0x27095..0x2710f`); signed-byte coordinates; same-map teleports do not
   reload the map; Continue restarts at the destination, Exit terminates.
   Destinations on maps not admitted refuse through the shared transition
   result. Audit the partial reasons for TeleportAndContinue with an active
   call stack and object selection.
2. **ConfirmWord** (21): mirror lookup for input type 0 and expected-string
   matching otherwise, primary and alternate strings, title selection,
   success and failure fall-through; the original input window (length 30,
   width 200, `0x24008..0x24014`), keyboard and mouse, Escape and Enter,
   without leaking keys into movement (M63 input rules). The sword reward
   path is included; ending codes are excluded.
   *Amended (maintainer, 2026-10-16):* opcode 21 alternate-only matches take
   ordinary success, rather than modeling its uninitialized DOS marker. Mirror
   lookup bounds by actual resource records instead of the stale scratch read
   at index 62. Missing byte-indexed text aliases text[0], following DOS.

## Census limits versus gaps

WhoWill, Exit, Display0x01, CallEvent, Return and MoveObj already have
implementations; their partial rows come from the census's starting camera.
Re-probe them in valid contexts and classify each reason individually,
including the AlterEvent line lookup, TeleportAndContinue context and the
1024-instruction loop limit.

## Save, RNG and the M44 gate

No save-format change is expected for Clouds; the durable-state inventory
decides. New draws sit exactly where the original makes them and are named in
any M44 explanation. Run `tools/m44-digest-audit.ps1 -Step Preserve` before
production edits and `-Step Compare` after the foundation and each part;
explain every difference from its first divergence and ask for approval
before any digest changes.

## Acceptance

- Zero unresolved in-scope Clouds Event gaps in the regenerated census, with
  its guard and calibration checks passing. Residual census limits are listed
  separately from the functional exclusions (audio, services, ending), the
  Darkside rows and the map-admission barrier.
- Tests per operation and mode with worked examples from original Clouds
  scripts, including nonzero AlterEvent, Spawn outside Vertigo, called-script
  deletion, refused travel with state and RNG preserved, several typed-input
  outcomes, every persistence lifetime and exact v13 round trips with no
  replayed Event, reward, time or draw (monster HP and wake reset on reload
  and Warzone's visit-only state are expected, not differences).
- Complete `build-rel` suite through the runner after the foundation and each
  part; M44 gate; independent implementation review.
- Maintainer play-test: the Vertigo mirror in ordinary play (Vertigo's only
  ConfirmWord use); a password prompt, a moved wall item, a MakeNothingHere
  cell, a random Event and a refused teleport through the developer fixture;
  save, load and return checks.
