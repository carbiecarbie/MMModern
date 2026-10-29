# Milestone 96 plan - Opened chests stay open

**Tier A. Status: planned (not started).**
Found by roadmap
[step 2](roadmap.md#next-arc-release-quality-and-the-darkside-start), the
DOS save investigation, which pauses until this milestone closes. Baseline:
`80a475b`, equal to `origin/main`.

## Goal and scope

As in DOS, a locked chest whose lock is picked becomes an open chest for
good. When the Event run ends, after its treasure, the chest takes the open
chest picture of its map. It is drawn open indoors and outdoors and stays
open through MoveObj, Remove, leaving the map, returning, Save and Load.
While the lock is being picked, the lid's frames follow the DOS animation.
37 Clouds chests on 18 maps can be picked open; 12 more are trap chests that
never are. Today MMModern shows a picked chest's open frame until the party
leaves the map, and the save cannot hold the change.

Out of scope:
- the DOS save converter and the rest of the investigation;
- GiveMulti mode 67, which no script on either side uses; it stays "not
  supported yet";
- Darkside play;
- ExchObj, which is not supported yet; when it is, it swaps only x and y;
- two notes from the plan review, for later: map 12's object 0 animates from
  sprite 85 in DOS (`0xff8d`, `0x10d0e..0x10d21`), and the pin skips Ali
  Baba's chest on Darkside (`party.cpp:1513-1515`).

## Current state

At `80a475b`:
- **Lock picking.**
  - GiveMulti picks the lock of a selected object with sprite 16, 58 or 73
    (15 too on Darkside), sets `lockCounter` (DOS `0x9755`) and sets the
    object's frame to 1 (`XeenEventInterpreter.cpp:1372-1432`).
  - That frame is presentation state (`XeenScenePresentation.h:78-80`). It is
    drawn in every view (`XeenIndoorScene.cpp:522`,
    `XeenOutdoorScene.cpp:163`) until the party leaves the map
    (`XeenVertigoWorld.cpp:126-127`) or loads a game (`XeenWorld.cpp:199`).
  - Nothing changes the object's picture when the run ends (`finish`,
    `XeenEventInterpreter.cpp:392`).
  - The TeleportAndContinue restart does not reset `lockCounter`
    (`XeenEventInterpreter.cpp:1742-1745`).
  - The selection is cleared on every teleport and map exit
    (`XeenEventInterpreter.cpp:485`, `:603`, `:1721-1722`).
  - A run that ends at a Town service settles on the live Journey world
    (`XeenEventInterpreter.cpp:719-723`, `XeenEventPublication.h:115-126`).
- **Object state.**
  - `XeenObjectOverride` holds a position and a flip
    (`XeenObjectState.h:7-15`).
  - `XeenWorld::effectiveObject` applies the position, then the fixed map
    rules (`XeenWorld.cpp:236`, `XeenScriptedRules.h:24-32`).
  - MoveObj rebuilds the override as `{id,x,y,flip}` on the detached
    candidate (`XeenWorld.cpp:252`, `:264-265`).
  - Remove erases the override and keeps the record only in
    `removedObjects`. It does so live (`XeenWorld.cpp:411-415`) and in the
    Journey publication (`XeenEventPublication.h:225-246`).
  - Restore validates overrides (`XeenWorld.cpp:274`).
- **Drawing.** Scenes draw objects through `XeenObjectVisualResolver`. It
  takes the picture, the frames and the flip from the original MOB record
  (`XeenObjectVisual.cpp:36-59`; `XeenIndoorScene.cpp:505`, `:521`;
  `XeenOutdoorScene.cpp:154`, `:162`), not from `effectiveObject`.
- **Save v21.**
  - Each object override is record, x, y and flip, in five bytes
    (`XeenSaveSnapshot.h:78-88`; `XeenSaveFormat.cpp:77-80`, `:226-228`,
    `:272-276`; count bound `:273`; size `:304`).
  - Capture writes a removed record as `{record,-128,-128,false}`
    (`XeenSaveState.cpp:391`).
  - Restore turns every `(-128,-128)` entry without a flip into a removal
    (`XeenSaveState.cpp:162-166`).
  - Equality: `XeenObjectState.h:11-13`, `XeenSaveSnapshot.h:85-87`,
    `XeenStateEquality.h:58`.
  - `kJourneyVersion` is 21 (`XeenSaveFormat.h:27-28`). Inn, ending and
    forced saves all go through `XeenSaveState::capture`.
- **Docs and tests.**
  - [`sv-chest-locks`](original-quirks.md#sv-chest-locks) and the "Locked
    chests" paragraph of the
    [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws)
    say that the open frame lasts until the map loads and that the chest is
    then closed, "as in DOS". Both are wrong.
  - [`dev-map-state-deltas`](original-quirks.md#dev-map-state-deltas) and
    its ScummVM item say that the overrides cover every map change the game
    produces.
  - `lockPicking` in `tests/XeenEventValueTests.cpp` pins only the frame, and
    its picture table has no open chest.
  - The test dump prints only `.removed` for removed entries
    (`tests/XeenSaveDump.cpp:80`, `:123-126`).
  - The test helpers treat `(-128,-128)` as removal
    (`tests/XeenSaveTestSupport.h:84-108`).
  - An independent test encoder writes five-byte entries
    (`tests/XeenRegionalPersistenceTests.cpp:43`).

## Evidence

DOS static, high confidence, calibrated on a real DOS save and verified by
the independent plan review. Offsets and scripts are in
`D:/Projetos/MModern/MModern-re/m96/evidence-notes.md` and
`D:/Projetos/MModern/MModern-re/m96/review/`.
- **The rule.** Every Event run ends with the defeat check (`0x28d6b`) and
  the treasure delivery (`0x28d70..0x28d8d`). Then, if `0x9755` is positive
  and an object is selected (`0x98d9`), the chest changes
  (`0x28d92..0x28e87`):
  - Sprite 16 (15 on Darkside) takes the first entry of the map's 16-entry
    object picture table that holds sprite 62 (57 on Darkside).
  - Sprite 73 takes the first entry that holds 119.
  - The table index and the sprite both change. Without such an entry
    nothing does, and a 16 or 15 chest never falls through to the 73 test.
  - `0x9755` then returns to zero, and the tail draws (`0x28e8c`).
- **Triggers.**
  - A picked lock increments `0x9755` (`0x23673`).
  - A failed pick ends the run with it at zero (`0x2371b`,
    `0x27eaf..0x27ebb`).
  - GiveMulti mode 67 sets it to one (`0x27ed8`).
  - Each run starts with it at zero (`0x26a96`), and so does each
    TeleportAndContinue restart (`0x27127` -> `0x26a8e`).
  - The selection is rewritten by every scene build (indoor
    `0x156f0`/`0x15836`, outdoor `0x170c3`/`0x171d1`) and zeroed by a map
    load (`0xf59a`). A same-map TeleportAndExit keeps it
    (`0x270be..0x270c6`, `0x2711c`).
- **Persistence.**
  - The map writer saves the table index with x and y (`0x26698..0x266ce`).
  - Loading a map rebuilds the sprite from the index (`0xf989`, `0xf9b1`)
    and sets the frame to 100 (`0xf9d5`).
  - MoveObj and Remove write only x and y (Remove: `0x27913..0x27937`).
  - Two sweeps found only three writers of the index byte: the map-load
    clear (`0xf5d5`), the loader and the rule.
- **Drawing.** DOS takes the picture from the index byte, through the
  per-index resource table (`-0xc16`; indoor `0x1577e..0x1578c`, outdoor
  `0x17119..0x17127`). It takes frames and flip from the sprite byte
  (`0x10d0e..0x10d21`, `0x10d87..0x10db6`). For an opened chest both name the
  open sprite.
- **Frames.**
  - The animation update (`0x10c32..0x10dc7`, the pin's `animate3d`)
    increments each object's frame every tick (`0x10c83`).
  - While `0x9755` is positive, the selected lock object cycles frames 1..4;
    sprite 58 holds frame 1 (`0x10c8a..0x10cf6`).
  - Otherwise a frame is kept while the view's end frame is at least the
    frame, and is reset to the view's start frame past it
    (`0x10cf9..0x10d55`).
  - Every chest sprite is static in each view (CD `clouds.dat`). So after a
    change, the opened chest shows the open sprite's start frame from the
    next tick. Without a change (a failed pick, which left frame 0, or sprite
    58), the object counts up to its start frame or resets to it.
- **Witness.** In slot 01 of the maintainer's DOS play, `maze0009.mob`
  object 1 at (15,11) went from index 3 (sprite 16) to index 4 (sprite 62)
  (`D:/Projetos/MModern/MModern-re/dos-save-20261028/blocker.json`).
- **Reference.** The pin does the same: `scripts.cpp:213-237`; a failed pick
  reaches the tail at zero (`scripts.cpp:944-945`); `map.cpp:398-411`, `:435`
  and `:479-485`. No ScummVM difference arises.
- **Census.**
  - 49 closed chests on 20 Clouds maps (75 on 12 Darkside maps); every one
    has its open sprite in its table.
  - 37 Clouds chests on 18 maps can be picked: 29 have a GiveMulti on their
    own cell, and eight on map 30 reach one through CallEvent. The maps are 9,
    30, 33, 34, 36, 41-46, 52, 54, 60, 62, 66, 70 and 81.
  - The other 12 are trap chests that their Events move away without a
    GiveMulti.
  - No pick script contains a Remove, ExchObj, teleport or service.
  - Other cells' Events MoveObj opened chests: on maps 42-45, sprite-73
    chests are moved onto their own cells, which flips them; on map 60,
    (11,22) moves record 0, the chest picked at (11,8), to `(-128,-128)`.
  - Two records index an empty (0xFF) table slot: record 0 of maps 47 and 48.
  - Map 28 has no chest, so only the mainland M44 scenario could open one.

## Maintainer decisions

- **Tier A with save v22** (2026-10-28), before the DOS save investigation
  resumes. v21 saves are then refused with the existing message, as the 0.x
  policy allows; the next release's notes say so.
- **Independent plan review** (2026-10-28,
  `D:/Projetos/MModern/MModern-re/m96/plan-review.md`): approved with
  changes, all applied here. It settled what Part 0 was to answer, so M96
  has no Part 0.
- Static evidence first; DOSBox only for what this evidence cannot settle.

## Design decisions

- **Owners.** No new owner.
  - The Event interpreter applies the rule once per run, in the run's tail:
    inside `finish`'s `!tailDone` block, after the treasure delivery and
    before the tail's `eventDraw` and "Nothing here".
  - The rule runs on the detached candidate, through one candidate-only world
    write beside `applyMoveObject`, with the same mutation-watch and
    publication protections.
  - A refused or abandoned run publishes nothing.
- **Rule.**
  - Generic, as in DOS: both sides, sprites 16 and 15 and 73, the first
    matching table entry, no change without one. It draws nothing from the
    RNG.
  - The TeleportAndContinue restart zeroes `lockCounter`, as in DOS.
  - While `lockCounter` is positive, the run is refused as "not supported
    yet" at any of these:
    - any other teleport;
    - a map change;
    - a Town service;
    - a Remove of the selected object;
    - any MoveObj or ExchObj, whatever record it names.

    This avoids applying the rule to a selection DOS may not have: every
    draw3d selects again the lowest-index object on the party's cell
    (`0x156f0..0x15836`, `0x170c3..0x171d1`), so moving any object could
    change it. No Clouds pick script does any of these (amended after the
    independent implementation review, which asked for the wider MoveObj
    refusal).
  - Mode 67 stays refused; its effect on `lockCounter` is recorded for when
    it is supported.
- **State.** The index belongs to the record and survives every position
  write, as in DOS.
  - The live override holds an optional picture-table index; absent means
    the record's own.
  - The rule creates or updates the override and keeps the position and the
    flip:
    - x and y come from the existing override or from the MOB record, never
      from `effectiveObject`, which applies the map rules;
    - the flip comes from `effectiveObjectFlip`, as MoveObj reads it.
  - `effectiveObject` substitutes the sprite from the map's table only when
    an index is present, before the fixed map rules. GiveMulti's lock test
    then sees the open sprite, as in DOS.
  - MoveObj replaces x, y and the flip and keeps the index.
  - Remove of an opened object, live and in the Journey publication, keeps an
    override that holds the index at `(-128,-128)` without a flip, instead of
    erasing it. A removed record can therefore now also have an override;
    the rule that `removedObjects` and `objectOverrides` never share a record
    ends.
- **Drawing.** The resolver takes the object's effective table index. When
  it differs from the record's own, the picture, frames and flip come from
  the table entry at that index. DOS draws from the index byte and animates
  from the sprite byte, and the rule sets both to the same sprite. Other
  objects keep today's lookup, so the map 12 rule stays logic-only.
- **Frames.** For the object giveExt framed, MMModern keeps the DOS frame
  byte instead of a fixed frame. Every animation tick applies the DOS update
  to it:
  - frames cycle 1..4 while it is the selection and `lockCounter` is
    positive; sprite 58 holds frame 1;
  - otherwise the frame counts up while at or below the view's end frame, and
    returns to the start frame past it.

  The entry ends once the object shows its sprite's ordinary frame. Leaving
  the map and Load still drop it. An opened chest therefore shows the open
  sprite's start frame from the first tick after its run.
- **Save v22** (v21 -> v22). Each object entry adds its picture-table index,
  removed entries included, which makes six bytes.
  - Capture writes the explicit index of every entry. Restore stores the
    index in the live override only when it differs from the record's own.
  - Restore still turns `(-128,-128)` without a flip into a removal. When
    that entry's index is the open one, it also keeps the override.
  - The format check accepts 0..15.
  - The restore check accepts two values only: the record's own index (even
    when its slot is 0xFF), or the first slot holding the open sprite of the
    record's own closed sprite on its side. The open index is accepted in any
    position shape: moved, removed, or the hidden flipped object of D01.
  - Schema 9 and content 14 stay.
  - Restore is exact: no Event, reward, time or draw is replayed.
  - v21 saves are refused clearly, and no v21 reader is kept.
  - Places to update:
    - `XeenSaveFormat.cpp`: validation, encoding, the decoder's count bound
      (5 -> 6) and the size;
    - the `XeenSaveFormat.h` comment, `XeenSaveSnapshot.h` and
      `XeenSaveState.cpp`;
    - the three equality sites;
    - the test dump (it must print the index of every entry, removed ones
      included), the test helpers and the independent test encoder.
- **Format audit.** An audit-only build, never committed, that writes the
  v21 layout must reproduce the four M44 baseline digests. The v22 saves
  must decode to the same state and the same traces. Bytes differ only in
  three places:
  - the version;
  - the payload length and checksum;
  - one index byte per object entry, equal to the record's own index unless
    the mainland scenario opens a chest.

  The maintainer then approves the new digests.
- **Docs.**
  - Correct `sv-chest-locks` and the "Locked chests" paragraph, for both the
    frames and the persistence.
  - Add the rule, its frames and its evidence to the world Event rules.
  - Name the index in `dev-map-state-deltas` and its ScummVM item.
  - Regenerate `docs/coverage.md` with `mmodern_census`; its guard should
    pass unchanged.
  - Add the preset to development.
  - The rule adds no ScummVM difference.
- **Decided now.** Everything above is settled. The implementation asks the
  maintainer only for a stop rule.
- **Stop rules.** Stop for:
  - a Journey schema or content change;
  - any other change to what the save holds or means;
  - a new owner;
  - an RNG draw;
  - a digest difference the audit does not explain;
  - a reachable Clouds case where the rule's selection or `lockCounter`
    would differ from DOS;
  - evidence that contradicts this plan;
  - scope beyond this plan.

## Risks

- Every v21 save stops loading: the maintainer's play-test saves and
  players' saves from 0.10.1. Presets regenerate as v22.
- Each save with object overrides changes length, not just its version, so
  the audit compares decoded state, not only bytes.
- Drawing has its own path: a fix to `effectiveObject` alone leaves the chest
  drawn closed.
- Any position write that rebuilds or erases an override loses the index.
  MoveObj and Remove on maps 42-45 and 60 reach this in Clouds.
- The rule must run exactly once per run, also when the treasure receipt or
  "Nothing here" suspends the run, and never on a refused one.

## Parts and sequence

| Part | Content | Gate |
| --- | --- | --- |
| A | Rule, restart and refusals, object state, drawing, frames, save v22, format audit, tests, rules and quirk docs, census, preset `m96-chest` | Independent implementation review, complete suite, digest approval |
| Play-test | Short check of the main result | Maintainer acceptance |
| Closure | Status, history, roadmap | Plan condensed |

## Acceptance

1. **Hook point.**
   - With pending treasure, the chest is still closed when the run suspends
     for the receipt: sprite 16, no override.
   - The world handed to the tail's `eventDraw` already has it open.
   - A later run on the opened chest sees sprite 62, so it picks no lock and
     makes no lock draw.
2. **Once.** A run that suspends for "Nothing here" after the change still
   has one override after resuming, and resuming does not apply the rule
   again.
3. **No change** without a selection, after a failed pick, without an open
   slot, for sprite 58, for a Darkside sprite 16, on a refused run (the
   candidate is discarded), or after a TeleportAndContinue restart.
4. **Index kept.**
   - Each of these keeps the index live and across Save and Load, with a
     byte-identical re-save:
     - MoveObj to another cell;
     - MoveObj to its own cell (flip);
     - MoveObj to `(-128,-128)` with and without a flip;
     - Remove.
   - Restore refuses each of these without partial publication:
     - another index;
     - an index of 16 or more;
     - an opened object that is not a chest;
     - an opened chest whose map has no open slot;
     - an opened Darkside sprite 16;
     - v21 and other malformed saves.
5. **Frames and drawing.**
   - The lid cycles 1..4 after a pick, and sprite 58 holds frame 1.
   - A failed pick counts up to the start frame.
   - The opened chest is drawn open, indoors and outdoors (rendered and
     inspected).
6. **Original data.**
   - Pick the lock of a chest on map 70 (2,14) or map 9 (15,11), receive the
     treasure and see the chest open.
   - Leave the map and return, then save and load: it is still open, and the
     save re-saves byte-identically.
   - Exercise one MoveObj case: map 60 record 0 followed by the Event at
     (11,22), or a map 42-45 chest followed by its MoveObj Event.
7. **Suite.** The complete suite passes, and the M44 digests change only
   through the audit.
8. **Play-test** with preset `m96-chest`: pick the lock, watch the lid, leave
   and return, save and load. It stays because the open picture and its
   frames can only be checked against DOS by eye, without a capture.
