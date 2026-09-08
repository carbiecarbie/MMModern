# Milestone 20 - Save and resume supported Clouds progress

**Status: approved specification; 20A independently approved; 20B implemented and validated, awaiting review.**

Prepared and approved on 2026-09-08. M19 remains the latest stable milestone.
Following commit `4d65e34`, independent review returned **APPROVE MILESTONE 20A**.
The user then explicitly authorized 20B only. 20C remains unstarted. Historical
20A evidence below is preserved; 20B evidence is recorded separately.

## 1. Goal, baseline and authority

Preserve currently modeled Clouds gameplay across an actual process exit and
restart: party/roster state, counted quest items, quest requests, game flags,
committed camera and all session world removals. Resume through the production
application and existing resource providers without replaying earlier effects.

Verified local checkout for this planning task:

- Project: `D:/Projetos/MModern/mmodern`.
- Branch: `main`; HEAD: `fd94e91fac626663c44407e30c709a9dc89b10d9`.
- Initial `git status --short`: empty; no preexisting changes.
- Latest commits: `fd94e91` (manual validation/language policy), immediately
  preceded by `59d3016` (M19 completion). No branch change, pull, reset, commit,
  push, tag or history rewrite was performed.
- Original installation: `F:/Games/gog/Might and Magic 4-5`, read-only. No
  original resources were inspected or modified for this planning task.

Read [AGENTS.md](../AGENTS.md), [project status](project-status.md), the
[approved roadmap](roadmap.md), [dependencies](dependencies.md), M19's current
specification/completion records, and relevant M17/M18 ownership, scope and
acceptance sections. M15 ownership and M16 reconstruction contracts were
consulted selectively. Earlier incomplete-stage statements are historical.

The current M19 record reports a successful Debug build, 23/23 focused tests,
47/47 full CTest, the 18-case Myra matrix plus revisits in each of direct and
SDL dummy/software modes, collection regressions and inspected native frames.
[M19 section 19](milestone-19-plan.md#19-post-completion-physical-window-manual-validation)
subsequently records the user's passing physical-window observations for Myra,
Phirna and the observed Bone Whistle flow. That later record supplements the
earlier evidence; it does not claim the entire matrix was manually retested.
These are **historical results, not tests run during M20 planning**.

The configured `build/19a/CMakeCache.txt` identifies Debug, MSYS Makefiles,
`C:/msys64/ucrt64/bin/c++.exe`, ScummVM source
`D:/Projetos/MModern/scummvm-known-good-candidate` and library build
`D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64`. The authoritative dependency
commit remains `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with the exact
configuration in dependencies.md. The cache was read; dependency checkout,
compiler versions and old binaries were not revalidated. No dependency update
or ScummVM save-system investigation is needed.

No concrete roadmap replanning trigger was found. Private membership/removal
storage needs narrow import/export access, not a new live session owner.
M20 remains the default successor; M21-M23 ordering and review cadence remain.

## 2. Inspected architecture and state inventory

Source paths below are repository-relative. Fields grouped in a row share the
same persistence policy. **Mutable** includes modeled value state exposed to
callers even where current production scripts only read it. Saving all modeled
character values avoids making future or fixture changes disappear on resume.
An encoded snapshot is a short-lived value transfer, never a second authoritative
state store retained beside the gameplay owners.

| State category and fields | Current authoritative owner | Classification | M20 decision | Validation and restoration destination | Current evidence |
|---|---|---|---|---|---|
| Committed side/map number, x, y, direction | Application's `XeenCamera`, passed by reference | Mutable | Save explicitly | Clouds; map 1..9999 and loadable with matching internal identity; x/y 0..15; direction 0..3. Restore camera before flow construction. Do not require a passable cell: current diagnostic positioning and teleports are not walking. | `src/games/xeen/XeenNavigation.h`, `XeenMapIdentity.h`, `XeenMapLoader.cpp`; event-system/navigation/teleport tests |
| Active roster IDs in order | `XeenPartyState::party` / `XeenParty::_activeRosterIds` | Mutable membership value | Save exact vector | 0..6 entries, each 0..29; preserve duplicates and order. Do not silently deduplicate or restore the initial party. Checked construction in existing party abstraction. | `XeenPartyLoader.cpp`, `XeenParty.cpp`; `XeenCharacterFormatTests.cpp` explicitly accepts/diagnoses duplicates and skips absent serialized slots |
| Thirty roster slots; `rosterId`, `name`, `sex`, `race`, `characterClass` | `XeenPartyState::roster`, each `XeenCharacter` | Modeled value state; slot identity fixed | Save all thirty, including inactive characters | Slot index 0..29 is identity; stored roster ID must equal it. Names remain bounded original-encoding character names. Active race/class must be safe for current rules; unsupported portrait handling remains explicit. Restore into the same roster abstraction. | `XeenCharacter.h`, `XeenCharacterFormat.cpp`, character-format tests; shared party snapshot |
| `currentHp`, `currentSp`, all 16 condition bytes | Each roster character | Mutable modeled state | Save explicitly, without normalization | Signed 16-bit HP/SP; each condition 0..255. Preserve negative HP/SP, over-maximum HP, simultaneous conditions and severity bytes. No healing or recomputation of current values. | Character format/rules/party visual tests; WhoWill worst-condition eligibility; interpreter Action 9 |
| Intellect/personality/endurance permanent and temporary values; permanent/temporary level; temporary age | Each roster character | Mutable modeled rule inputs | Save all nine integer fields | Signed 32-bit encoding, matching current Windows `int`; no byte cap or unsigned conversion. Validate safe evaluation before use as described below. Restore exact fields. | `XeenCharacterRules.cpp`; rules tests include negative temporary level, attributes above 255 and a 70000 stat |
| Birth year; astrologer/bodybuilder/prayerMaster/prestidigitation; hasSpells | Each roster character | Modeled rule inputs | Save explicitly | `uint16` birth year, five independent booleans. Restore exact values. | Character loader/rules and snapshot helper |
| Weapons, armor, accessories: nine `{material,state,frame}` entries per category | Each roster character | Modeled modifier inputs, not inventory | Save all 81 bytes per character | Each byte 0..255, category/slot order fixed. Do not synthesize item IDs, miscellaneous items, capacity or rewards. | `XeenItemModifierSource`, character format/rules; snapshot helper |
| All 35 Clouds quest counters (item IDs 82..116) | `XeenPartyState::questItems` | Mutable, immediate grants | Save every `uint32` counter | 0..UINT32_MAX, preserving 256 and larger counts. Use existing value constructor. No possession-to-removal inference. | `XeenQuestItemFormat.cpp` widens original bytes at 747..781; `XeenQuestItemTests.cpp`, `XeenQuestGrantTests.cpp` |
| All 30 Clouds quest flags | `XeenPartyState::questFlags` | Mutable, immediate set | Save independently | Exactly 30 booleans, no inferred request from Root possession. Use existing value constructor, not scripts. | `XeenQuestFlagFormat.cpp`: eight-byte field at 739, Clouds bits 0..29; `XeenQuestFlagTests.cpp`, Myra integration |
| All 256 Clouds game flags | Application's `XeenGameFlags` | Mutable, event completion commits | Save separately from quest flags | Exactly 256 booleans; use existing `Storage` constructor. | `XeenGameFlagsLoader.cpp`, `XeenGameFlagsFormat.cpp`, game-flag and event-system tests |
| Disabled original object identities, across every affected map | `XeenWorld::_sessionState._objects` | Mutable immediate overlay | Export/save exact set | Clouds map + original zero-based object record index; resource present, index in original object array. Restore exact set, including valid already-base-disabled entries. | `XeenRecordIdentity.h`, `XeenWorld::disableObject/applyRemove`; Remove/session/Phirna tests |
| Disabled original event identities, across every affected map | `XeenWorld::_sessionState._events` | Mutable immediate overlay | Export/save exact set, independently of objects | Clouds map + original zero-based event record index; original EVT present and index in range. Restore exact identities, not all events sharing a saved cell. | `XeenWorld::disableEventsAtCell/effectiveEvent`; Remove/session/Bone Whistle tests |
| Original map geometry, neighbors, collision/surface data, base object records, scripts/text and visual metadata | Read-only resources, disposable world/event/asset caches | Immutable/resource-derived | Reconstruct through existing providers | Resource compatibility plus current parser checks. Base disabled objects remain resource-derived; only session overlay sets are saved. No cached `XeenMap::instructions` mutation store. | `XeenMapLoader`, `XeenEventLoader`, `XeenAssetSource`; M15/M16 lifecycle contracts |
| Maximum HP/SP, effective attributes, current level, worst condition, canAct, portrait resource/frame, draw commands | Rules/composers calculated from state | Derived | Recompute | Current rules and composition; never substitute maxima for saved HP/SP. Current year remains fixed `kCloudsInitialYear` (610), not a new saved clock. | `XeenCharacterRules`, `XeenPartyVisualState`, `CloudsUiComposer`, `CloudsMapComposer` |
| `firstSerializedCount`, `effectiveSerializedCount`, loader diagnostics | `XeenPartyState` loading metadata | Resource-derived metadata, not gameplay | Reconstruct from initial loader | Keep original loader metadata/diagnostics; they are not active membership authority. Compare durable fields separately in persistence tests. | `XeenPartyLoader`; shared snapshot currently includes these fields as unrelated-state regression checks |
| Pending execution, logical address, working camera/flags, call stack, selected character/object, instruction budget, generations/responses | Interpreter/EventFlow | Transient | Exclude | No pending state at save boundary; new owners on startup. | Event system/flow; quest-grant, quest-flag, WhoWill, NPC tests |
| Dialog pages, retained labels, portrait random/timing/rest state, underlays, frames, rendered-camera/count detector, cache contents/pointers and counters | Presenter/flow/assets/SDL | Transient/derived | Exclude and rebuild | New presenter and forced initial composition; no copied callback or cache reference. | `XeenEventFlow::refresh`, NPC and visual Remove tests |

Only resource-derived, derived and transient rows are rebuilt or reinitialized.
No modeled character
field, party membership, counter, flag or session removal is reloaded from its
original default in place of a saved value. Names are typed character state;
no narrative text tables or raw CHR/PTY payload is embedded.

### Validation details at the character boundary

The original loader casts enum bytes without checking them, and the rules index
class/race tables directly. Do not feed hostile save bytes into those rules.
For active members require race 0..4 and class 0..9; retain raw uint8 sex values
(the rules do not index by sex). Inactive slots may retain any enum byte, as the
loader currently does; they are not evaluated. Do not invent a general roster
recruitment feature. Active characters must pass current portrait/composition
preflight, including its existing unsupported roster-portrait behavior.

The codec can round-trip the full signed-32 domain of rule inputs. Publication
also requires every arithmetic intermediate used by current level, age,
attribute and HP/SP rules for active members to fit its existing result type.
Check sums/products in widened arithmetic before calling the ordinary rules;
reject unsafe combinations, never clamp or wrap them. Prefer a narrow checked
preflight adjacent to `XeenCharacterRules`, sharing its constants/calculations,
over duplicate rule tables in a save parser. This is input validation, not a
new stat rule or a byte-range restriction. Include safe negative/large values,
and separate codec boundary tests from unsafe-publication rejection tests.

## 3. Save boundary and smallest production interface

Extend the existing `--render-map` gameplay path and add startup resume:

```text
mmodern --render-map <game-directory> [<map> <x> <y> <north|east|south|west>] [--save-file <path>]
mmodern --load-game <game-directory> <save-path>
```

Existing invocations retain their behavior. `--load-game` has no position
override: the saved camera wins. The load path also becomes that session's F9
save target. `--save-file` is optional for new sessions; with no configured
target F9 reports how to supply one and writes nothing. No implicit default
save, slot menu, autosave, save-on-exit or in-session load is required.

**F9** requests a synchronous save. Add a `SaveGameAction` to the existing
`PlayerAction` route and SDL repeat suppression; intercept it in Application
before `flow.handle()`. Do not turn it into an interpreter instruction or let
the generic gameplay handler clear labels/advance dialogs. F1-F6 remain WhoWill
controls. Save success/failure is printed in English to stdout/stderr with the
resolved path and exposed in the SDL window title through a small existing-window
status hook. This avoids a new dialog/presenter mode. Status remains until the
next save attempt; input controls, including contextual Escape, are unchanged.

Saving is allowed only after startup succeeded, between completed synchronous
input callbacks, with no active dispatch, no pending execution/presentation
(`flow.blocksGameplay() == false`), and no shutdown/fatal presentation failure.
The application owns this decision; an event-report callback is not a save
boundary. Retained nonblocking labels after a completed event are permitted;
they are omitted from the save and first resumed frame.

| Situation at F9 | Required behavior |
|---|---|
| Idle after completion, no event, or recoverable manual error | Capture actual authoritative state; save if values/resources validate |
| Display/page, acknowledgment, Yes/No, NPC or WhoWill pending | Refuse with `Cannot save while an interaction is pending.`; no file I/O, queued save, response, cancellation or script advancement |
| Repeated F9 keydown | Ignore repeats, as for existing input |
| A genuine later user acknowledgment/cancellation finishes interaction | A subsequent fresh F9 may save; the earlier refused request does nothing |
| SDL_QUIT / ordinary exit | Exit under existing rules; never save implicitly or consume queued input after quit |
| Startup failure / fatal automatic-event exception | No running session to save; preserve existing failure outcome |

Saving is not a new transaction around gameplay. EventSystem publishes camera
and game flags only on completion. Event errors or abandonment discard working
camera/flags, not movement/rotation committed before dispatch. **WhoWill
cancellation completes like Exit and therefore commits its current working
camera/game flags**; it is not equivalent to abandonment. Party grants, quest
sets and successful world Remove remain immediate and can survive subsequent
errors, abandonment or cancellation. A later permitted save captures those
surviving effects independently. Never infer flags/removals from possession or
replay scripts to reconstruct mutations.

### Paths and overwrite contract

Resolve relative paths against the process working directory once; display the
absolute path. Require an existing parent and a regular file target (or absent
leaf), with `.mmsave` suffix; reject directories, devices and alternate streams.
Use native wide filesystem paths on Windows and test spaces/non-ASCII names.
No directories are created implicitly. Refuse targets within the detected
commercial installation, including resolved aliases/junctions. Generated saves,
temporary files, logs and images for development belong under ignored `build/20*`
or a user data directory, never tracked source files or the installation.

The startup message states `F9 saves and replaces <absolute-path>`. Choosing the
path opts into replacement of that MMModern save. Refuse overwriting an existing
file that fails the supported save decoder/validation; report that another path
is needed. Do not overwrite an unknown-format or unsupported-version file.
Errors leave gameplay usable and do not report success or change gameplay state.

## 4. Concrete format: MMModern Clouds save v1

Use a small **uncompressed binary format**, explicitly encoded little-endian,
with extension `.mmsave`. Reuse existing zlib CRC32 for accidental corruption
detection; no new third-party parser, generic serializer, raw struct dumps,
padding, host `size_t`, native `bool` layout or C++ object representations.

The total file is at most **4 MiB (4,194,304 bytes)**. Reader checks size before
allocation, bounds every read/count, uses checked size arithmetic, and requires
exact end-of-file. The same limits apply to capture/encoding; exceeding one is
an explicit save failure, never a truncated successful save.

### Wire layout, in order

`u8/u16/u32/u64` denote fixed-width unsigned integers; `i16/i32` use explicitly
encoded two's-complement values. Arrays have no implicit counts unless listed.
CRC32 means zlib's standard `crc32` initialized with `crc32(0L, Z_NULL, 0)`
and updated over the byte stream in order. Every presence/boolean byte is 0 or 1.

| Field | Representation / rule |
|---|---|
| Magic | Eight literal bytes `MMMSAVE\0` |
| Format version | u16 = 1 |
| Game side | u8 = 0 (Clouds only) |
| Reserved | u8 = 0 |
| Payload length | u32; exactly file size minus 20; header is 20 bytes |
| Payload checksum | u32 zlib CRC32 over payload only |
| Resource signature (start of payload) | Required xeen.cc: u64 byte length + u32 CRC32; dark.cc: u8 present, u64 length + u32 CRC32. Absent dark.cc requires zero length/checksum. |
| Camera | u8 side, u16 map, u8 x, u8 y, u8 direction |
| Active membership | u8 count, followed by count u8 roster IDs; count 0..6 |
| Roster count | u8 = 30 |
| Roster records | Thirty character records in roster-index order, as below |
| Quest counters | 35 u32 values in item-ID-minus-82 order |
| Quest flags | 30 u8 values, each exactly 0 or 1 |
| Game flags | 256 u8 values, each exactly 0 or 1 |
| Disabled objects | u32 count (0..65536), then count identity records |
| Disabled events | u32 count (0..262144), then count identity records |

An identity record is **u8 side + u16 map + u32 original record index**.
Objects/events are separate arrays sorted strictly by `(side,map,index)`;
duplicates or unsorted input are rejected. Check index representability before
conversion to `size_t`, then against actual original records. These bounds are
storage limits, not hardcoded checkpoint whitelists; every affected supported
map must be exported, including maps absent from caches and the current view.

Each character record is exactly this ordered sequence:

1. u8 rosterId; u8 name length 0..16; that many original-encoding name bytes,
   no embedded NUL, UTF-8 conversion or trailing terminator.
2. u8 sex, u8 race, u8 class.
3. Six i32 values: intellect permanent/temporary, personality permanent/temporary,
   endurance permanent/temporary; then three i32 values: permanent level,
   temporary level, temporary age.
4. Five u8 booleans in order: astrologer, bodybuilder, prayerMaster,
   prestidigitation, hasSpells; each exactly 0 or 1.
5. Weapons, armor, accessories, in that order: nine entries each, each entry
   three u8 values in material/state/frame order.
6. i16 currentHp, i16 currentSp, sixteen u8 condition values in enum order,
   u16 birthYear.

No original archives, CHR/PTY blobs, map/script blobs, extracted narrative text,
images or sprites are serialized. Character names and modifier triples are
only their existing typed modeled fields, not original resource records.

### Compatibility and semantic validation

Recommend a conservative **same archive contents** policy for v1: byte length
and streaming zlib CRC32 of the complete detected xeen.cc and of dark.cc when
present must match. Paths, timestamps, installation directory and filename case
are not compatibility keys. Including dark.cc accounts for the existing Clouds
visual-metadata provider without claiming Darkside gameplay. Presence changes
are incompatible. Compute signatures once per startup from read-only files;
resource replacement while a process is running is unsupported.

This deliberately rejects another release/localization or a modified archive,
even if a particular checkpoint would happen to work. It avoids a resource
manifest/tracking system or broad resource survey in M20. CRC32 detects ordinary
accidental changes, **not malicious substitutions or cryptographic identity**;
the original installation is trusted input. No archive bytes are copied to the
save. A bounded chunked stream avoids loading entire archives for hashing.

After structural decoding/checksum/version checks, validate against resources:

- Load initial party through the existing loader to reconstruct metadata and
  establish thirty source roster slots. Overlay all saved modeled values,
  including membership, instead of applying initial gameplay defaults afterward.
- Validate camera map and every map referenced by either removal set through
  `XeenMapLoader`; map side/number and internal identity must agree. No blanket
  survey of every installed map is necessary.
- Validate disabled object/event indices against the **original**, unfiltered
  MOB objects / EVT records through existing loaders. Missing optional resources
  remain legal where no corresponding identity requires them; a saved identity
  in a missing resource is invalid. Do not confuse original index with sprite
  ID, coordinate, visible index, line number or file offset.
- Preserve object and event sets independently, including event-only removals.
  Do not require an associated item, quest flag, or paired object removal.
- Validate membership, booleans and active character safety; preflight the
  ordinary composition/resources before exposing gameplay. Unknown opcodes in
  a valid base script are not a load rejection: retain existing unsupported
  execution boundaries rather than conducting an opcode census.

Unsupported versions fail clearly before payload interpretation; no fallback
to v1 or fresh game. Future persistent categories require a deliberate version
bump, written field/validation policy and tests. M21 must decide explicitly
whether to read v1 under documented defaults or reject it. M20 implements only
v1; it provides no generic migration framework or future inventory payload.

## 5. Decode, restore and first presentation

Separate three operations: decode into an inert value snapshot; validate it
against existing resource/rule contracts; prepare the existing owners. No decoder
mutates a running party, camera, flags or world as it consumes bytes.

Recommended narrowly scoped additions:

- `XeenSaveSnapshot` value type for capture/transfer, codec in
  `src/formats/xeen/XeenSaveFormat.{h,cpp}` and resource-aware preparation in
  `src/games/xeen/XeenSaveState.{h,cpp}`. Names are proposed, not existing APIs.
- Checked membership construction on `XeenParty`, reusing its loader domain.
- Read-only enumeration/export of the two sets from `XeenSessionWorldState`;
  a validated startup import on `XeenWorld` builds complete replacement sets
  locally and swaps only after all identities pass. It must not re-execute
  `applyRemove` or expand event identities by cell.
- File operations outside the interpreter, e.g.
  `src/platform/XeenSaveFile.{h,cpp}`, with a small fault-injectable file boundary.

Startup sequence in the existing Application gameplay construction:

1. Parse options and detect original installation. For resume, read/decode and
   verify resource compatibility before creating any SDL gameplay window.
2. Create an unpublished candidate party/camera/game flags and an `XeenWorld`
   with the ordinary map/object providers. Validate everything, including the
   complete overlay sets, and prepare it before any gameplay dispatch.
3. Move prepared party values into their final lifetime location; finish camera,
   flags and world initialization. Only now construct EventSystem, font,
   composer callbacks and EventFlow references. Providers/assets outlive them.
4. The EventFlow constructor already calls `refresh(true)`. For **resume**, use
   its freshly composed `frame()` without calling `initial()`. For **new session**,
   keep the current initial loaders/default camera and call `initial()` as today.
5. Show only the completed first frame. Resume success feedback is issued only
   after preparation/composition succeeds. No default-party or unremoved-world
   frame may flash first. Ordinary subsequent navigation still calls
   `processNavigationAction`, including its existing automatic-event dispatch;
   there is no persistent “skip automatic events” switch.

Any missing, truncated, malformed, oversized, unsupported, incompatible or
semantically invalid file, resource failure or failed initial composition aborts
startup with an English reason and nonzero exit status (recommend 3, consistent
with current startup errors). There is no partially published gameplay graph and
no implicit new game. An explicit `--render-map` invocation starts a new session.
Invalid CLI syntax retains exit 1. File read failures are distinct from bad data.

Fresh process/flow construction prevents stale references, response generations,
callbacks, selection, portrait timing and retained underlays. No in-session state
replacement is proposed. Thus the existing disabled-object-count change detector
remains valid for monotonic mutations within a session. Nevertheless, test two
saved states with equal disabled counts but different identities in separate
fresh graphs: each must compose its own correct first frame. Never optimize
startup by reusing an old graph/frame because the counts match.

## 6. Safe writing on the Windows toolchain

Use the current MSYS2 UCRT64 Windows build, native wide Win32 file operations
and checked results. M20's write contract targets local Windows filesystems;
network shares and concurrent writers to the same save are unsupported and
should be rejected where detectable. Do not use delete-then-rename or assume
`std::filesystem::rename` overwrites identically across platforms.

20A approval clarification for future 20B: local Windows files, Unicode paths,
safe temporary-file replacement and preservation of an existing valid save are
the core requirements. Junction/device/network/concurrent-writer edge cases
must not grow into a generic filesystem subsystem or expand this milestone.

Recommended write protocol:

1. Capture, validate and encode the complete bounded snapshot before opening
   a destination for replacement. Validate an existing save before overwriting.
2. Create a uniquely named sibling temporary file with `CreateFileW(CREATE_NEW)`;
   never truncate the destination or reuse an existing temporary file. The
   sibling is on the same volume. Loop over `WriteFile`, checking byte counts.
3. Check `FlushFileBuffers`, then close successfully before publication. Any
   creation/write/flush/close failure leaves the old destination untouched.
4. Publish using `MoveFileExW(temp, target, MOVEFILE_REPLACE_EXISTING |
   MOVEFILE_WRITE_THROUGH)`. Do not enable cross-volume copy/delete or delayed
   reboot behavior. Handle failure explicitly; never remove the target to make
   a retry succeed. Test preservation of its exact old bytes on real Windows
   replacement failures, including a target held open without delete sharing.
5. Report success only after publication succeeds. Clean up only the temporary
   file created by this attempt on failure; cleanup failure is reported with its
   path, does not erase the valid save, and must not become a false success.
   A leftover temporary file is never auto-loaded as the requested save.

The required tested guarantee is that ordinary handled open/write/flush/close
and replacement failures preserve an existing valid save, and success publishes
a complete file. Preserve the old bytes and verify decoding after each injected
failure. Test absent-target creation and successful replacement separately.
Do not claim transactional guarantees across arbitrary filesystems, concurrent
external edits, process kill at any OS instruction, hardware failure or power
loss. A pre-publication process exit can leave a temporary file; a successful
API return is not evidence of absolute power-loss durability.

The API choices follow Microsoft's
[MoveFileExW documentation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw)
and [FlushFileBuffers documentation](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers).
The flags and flushing narrow ordinary failure exposure; they do not establish
an application-level crash-safety proof. M20B must establish the stated failure
contract on the actual toolchain/filesystem before acceptance.

## 7. Numbered acceptance criteria and test mapping

Proposed new test names below are implementation deliverables, not current
passing targets. Keep data-free CTest separate from explicit original-data smokes.
20A can close A01-A05 at the format/state layer when their tests pass. Evidence
for A08-A10 is only partial owner/flow evidence in 20A; production startup, file,
SDL/Application and cross-process portions remain for 20B/20C. In particular,
A09 cannot close before 20B validates production new-session versus resume setup.

| ID | Requirement | Required evidence / test owner |
|---|---|---|
| M20-A01 | Complete durable state round-trips deterministically, independent of initial defaults | `xeen_save_format`: all 30 characters/all fields, inactive entries, membership order/duplicates, empty/one/six parties; compare typed values, not diagnostics |
| M20-A02 | Numeric/encoding domains preserved | Codec tests: HP/SP -32768/-1/0/32767, condition 0/255, counters 0/1/255/256/UINT32_MAX, each flag bit, first/last IDs, signed rule inputs including -2 and 70000, full wire integer boundaries, name 0/16 bytes and high-bit bytes |
| M20-A03 | Independent categories, multi-map complete sets | `xeen_save_state`: disjoint counter/quest-flag/game-flag patterns; camera independent of all effects; inactive roster change; object-only/event-only sets, off-camera maps and equal-count/different-identity cases; no inferred effects |
| M20-A04 | Bounded structural rejection | Bad magic/reserved/version/side/checksum; every truncation point, trailing bytes, wrong fixed counts, oversized file/count, impossible lengths, invalid boolean, duplicate/unsorted identities; no partial output |
| M20-A05 | Semantic/resource rejection before publication | Invalid membership/roster IDs, camera/domain, active enum/unsafe arithmetic, missing or changed archives, map mismatch, missing MOB/EVT for an identity, index exactly at/past record count; candidate failure leaves any sentinel owners unchanged and starts no window/event |
| M20-A06 | Safe file replacement | `xeen_save_file`: injected short/failed write, open/flush/close/replace/cleanup failures; actual Windows locked target; exact old-byte comparison and fresh decode; successful create/overwrite, path spaces/Unicode, unknown file refusal |
| M20-A07 | Stable save eligibility and input | `xeen_save_flow`, `xeen_save_sdl`, `sdl_input`: F9 idle, repeat suppression, every pending presentation kind/page, invalid WhoWill retry, no configured path, quit ordering; refusal changes neither gameplay nor generation/page/selection and performs zero writes |
| M20-A08 | Existing mutation/error policies survive saving | Extend quest-grant/quest-flag/Remove fixtures: earlier immediate effects then later error/abandon; save once idle; camera/game flags rolled back only as before. WhoWill cancellation commits working camera/flags; save/reload that result. Automatic failure still preserves prior movement at the owner level. |
| M20-A09 | Resume performs zero initial automatic dispatch; new session retains it | Synthetic automatic grant/flag/teleport trigger at saved cell; shared production startup helper, fresh frame before event, no invocation of `initial()` on resume; explicit new-session control dispatches once; later movement/rotation retains current automatic policy |
| M20-A10 | Restored owners precede presentation, with no stale state | `xeen_save_state/flow`: first composed frame contains saved party/removals/camera; fresh generations/timing, no retained label; equal-count different-state graphs; genuine provider reload counters and post-reload state/frame assertions |
| M20-A11 | Production CLI and startup failures are unambiguous | `xeen_save_cli`: syntax/conflicting arguments; missing/unreadable/invalid save returns nonzero, no fresh fallback; new-session interface unchanged; load uses saved position and target; production Application/SDL smoke |
| M20-A12 | Phirna disk restart checkpoint A | Original interaction, F9/shared production save request, terminate producer, new consumer process resumes ownership/removal/appearance; repeat cannot grant; cache reload and fresh-session controls |
| M20-A13 | Bone Whistle disk restart checkpoint B | Original WhoWill/display/acknowledgment, then independent process resumes exact counter/object/event sets and repeat behavior, without pending selection; reload/fresh controls |
| M20-A14 | Myra disk restart checkpoint C | Original no-root request, process restart, Q2 persists, original request repeats; root-owned branch still errors at unsupported consumption with no reward or state change |
| M20-A15 | Cumulative multi-map persistence | Genuine Myra request then Phirna and Bone Whistle effects with disclosed controlled positioning; serialize together, restart, inspect/revisit each and reload providers. No injected checkpoint grants/flags/removals. |
| M20-A16 | Existing regressions and evidence separation | Relevant navigation/movement/teleport, event, presentation, collection, NPC and WhoWill tests; full CTest; direct original, dummy/software, native-frame inspection and physical-window observations recorded separately |

Reuse `XeenPartySnapshotTestSupport.h` for all modeled character/membership
comparisons and explicit quest collection comparisons. Add a durable-state
comparison that excludes loader metadata; do not weaken historical full-party
invariance assertions. Existing integration helpers and scripts remain the
source of checkpoint behavior; avoid three replacement smoke implementations.

## 8. Original-data cross-process acceptance

Extend `PhirnaIntegrationTest`, `WhoWillIntegrationTest`, `MyraIntegrationTest`
helpers where appropriate. A small `mmodern_save_resume_smoke` coordinator may
reuse extracted test-only helpers to drive producer/consumer child processes.
It must use the production codec/file operations, save eligibility, startup
choice and composition path. A narrow shared Application startup/save helper
is justified for this testability; a broad Application rewrite is not.

For every checkpoint, the producer loads original resources, obtains effects
through ordinary line-0 interactions and completes all required responses,
saves and **exits**. The coordinator waits for exit and launches a distinct
consumer PID that reads the disk file and uses the production resume path.
No shared assets, owner graph or in-memory snapshot can cross that boundary.
Logs record commands, paths, PIDs, exit codes and state assertions. Also exercise
the actual `mmodern --load-game` CLI through Application/SDL, not only a test
reconstruction. A data-only format round trip does not close these criteria.

| Checkpoint | Obtain in producer | Assert in newly started consumer |
|---|---|---|
| A: Phirna, Clouds 23 `(8,2)` North | Optional No control; then original Yes/ack harvest, 18 instructions, item 99/index 17 +1 | Object `{23,13}` absent from selection/draw commands; event records 125..135 effectively None; counter retained; repeat runs eleven None without another grant or dialogue replay; compare clean resumed scene rather than retained pre-save success text |
| B: Bone Whistle, Clouds 20 `(5,14)` North | Original WhoWill, valid F1-F6 member, display and acknowledgment; ten instructions; item 100/index 18 +1 | Object `{20,1}` and events 1..5 disabled; correct first frame; repeat five None and no grant; no carried selection; cancellation/retry remains a separate fresh control |
| C: Myra, Clouds 23 `(9,11)` West | Without Root, original two-page request then final acknowledgment, five instructions; only Q2 becomes true | Q2 true, all other state preserved; original request repeats (Q2 does not suppress it). Preserve final Escape semantics. Root-owned return still stops at line 8/offset 255, three instructions, `UnsupportedOperationMode`, no consumption/clear/reward. |

For the cumulative case, start without a Root, complete Myra's request, then
position at Phirna and collect, then position at Bone Whistle and collect, save
on map 20 and exit. On resume assert both maps' sets plus both counters and Q2.
Controlled position changes use test harness camera setup only, without script
effects or new production teleport keys. Revisit both removals and Myra with
the genuinely acquired Root; the latter remains at unsupported consumption.
This supplies the real root-owned control. Retain the existing Myra count-1/3,
Q2-false/true matrix as separately disclosed controlled-fixture regressions;
do not describe injected setup as persistence checkpoint acquisition.

After loading, discard geometry/object, script, text and sprite caches separately
and together, then recompose/revisit through genuine providers. Require loader
and sprite construction counters to increase. A removed-cell revisit may need
no text; use the established independent text control to prove actual text
reload. Reusing a cache entry or only reconstructing a flow is insufficient.

Run a distinct **fresh process without any load option**: original counters and
quest/game flags, visible original objects, base events, no inherited selections
or generations. Include both automatic-start synthetic control and original
fresh-session visual/state controls. Positioning does not certify travel between
checkpoints or a generally playable region.

## 9. Implementation stages and exact validation commands

Commands below are **future acceptance commands**, not work performed in this
planning task. Proposed targets/modes must be implemented before they can run.
Use an MSYS2 UCRT64 shell, per dependencies.md, from the repository root:

```sh
GAME='F:/Games/gog/Might and Magic 4-5'
B='build/20a'
SC_SRC='D:/Projetos/MModern/scummvm-known-good-candidate'
SC_BUILD='D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64'
git -C "$SC_SRC" rev-parse HEAD
git -C "$SC_SRC" status --porcelain
cmake -S . -B "$B" -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Debug \
  -DSCUMMVM_SOURCE_DIR="$SC_SRC" -DSCUMMVM_BUILD_DIR="$SC_BUILD"
```

Dependency HEAD must match dependencies.md, with the documented configuration;
do not update it. Later stages reuse this build and put outputs under their own
ignored directories. Full CTest is mandatory at milestone completion; run the
specified focused tests during each stage and keep failures as blockers.

### 20A - State transfer, format and validation

Objective: implement complete state coverage, deterministic codec, capture and
resource-aware unpublished restoration. Depends only on the approved M19 baseline
and explicit authorization of 20A.

In scope: sections 2, 4 and 5's value/preparation boundaries; checked membership
construction and world set export/import; narrow rule-input preflight; synthetic
tests for M20-A01-A05, A08-A10 at the owner/flow level. Likely files: new
`XeenSaveSnapshot/State/Format` files, `XeenParty`, `XeenWorld`, a narrow
`XeenCharacterRules` validation helper if needed, save tests and CMake registration.
Resource signatures reuse zlib. Out of scope: file writes, F9, CLI changes,
cross-process original acceptance, any next milestone gameplay.

```sh
cmake --build "$B" --parallel 4
ctest --test-dir "$B" --output-on-failure -R 'xeen_(save_(format|state)|character_|party_visual_state|quest_|game_flags|session_|world|remove|event_system|navigation_flow|who_will|npc)'
git diff --check
```

Stop with an independently testable value/preparation layer and documented
acceptance evidence. A same-process restoration test here is not disk persistence
or M20 completion. Do not begin 20B automatically.

### 20B - File persistence and production save/resume

Objective: make F9 and startup load usable with safe failure behavior. Depends
on approved/completed 20A and explicit 20B authorization.

In scope: Windows file boundary/fault injection, CLI parsing, F9 routing/status,
new/resume initialization distinction, construction before presentation; tests
M20-A06-A11 and regression coverage of A08-A10. Likely files: `XeenSaveFile`,
`src/main.cpp`, `Application.{h,cpp}`, `src/core/PlayerAction.h`,
`SdlWindow.{h,cpp}`, shared narrow app helper, new file/flow/SDL/CLI tests,
`GraphicsSmokeTest.cpp`, CMake. Change EventFlow only if needed for a clear
initialization API; its constructor/frame already support resume composition.
Out of scope: in-session load, generic session controller, slot menu/autosaves,
checkpoint effect shortcuts or declaring full original-data acceptance.

```sh
cmake --build "$B" --parallel 4
ctest --test-dir "$B" --output-on-failure -R 'xeen_(save_|quest_|game_flags|session_|remove|visual_remove|event_|manual_event|navigation_flow|who_will|npc|character_|party_visual_state)|sdl_input'
cmake --build "$B" --parallel 4 --target mmodern_graphics_smoke
git diff --check
```

Stop when production new/save/resume and failure paths work and focused tests
pass. Do not call M20 complete or begin 20C without authorization.

### 20C - Cross-process original acceptance and stabilization

Objective: close M20-A12-A16, preserving all earlier criteria. Depends on
approved/completed 20B and explicit 20C authorization.

In scope: reuse/extract existing integration helpers, coordinator/child modes,
full original matrix, CLI/Application dummy input, native first-frame inspection,
short physical-window validation, documentation and narrowly necessary fixes.
Likely files: three existing integration tests and shared test support,
`SaveResumeIntegrationTest.cpp`, `GraphicsSmokeTest.cpp`, CMake registrations,
README/project status/this plan's new evidence section. Out of scope: normal
route certification, M19 UI polish, future gameplay, dependencies.

Proposed coordinator interface: `mmodern_save_resume_smoke <game> <output>
[sdl]`, runs A/B/C/cumulative plus fresh controls in child processes. Proposed
GraphicsSmoke additions: `save-phirna <save-path>` drives a fresh real Application
harvest and F9 then exits; `resume <save-path>` enters the same path as
`--load-game`, verifies resumed startup and exits. Both also receive the ordinary
existing `escape` exit argument. The coordinator must verify state, not merely
child exit codes; production-path checks must not bypass eligibility/initialization.

```sh
cmake --build "$B" --parallel 4
cmake --build "$B" --parallel 4 --target mmodern_save_resume_smoke \
  mmodern_phirna_smoke mmodern_who_will_smoke mmodern_myra_smoke \
  mmodern_manual_event_smoke mmodern_navigation_flow_smoke mmodern_graphics_smoke
ctest --test-dir "$B" --output-on-failure -R 'xeen_(save_|quest_|game_flags|session_|remove|visual_remove|event_|manual_event|navigation_flow|movement|world|who_will|npc|character_|party_visual_state|outdoor_|object_)|sdl_input'
"$B/mmodern_save_resume_smoke.exe" "$GAME" build/20c/restart-direct
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_save_resume_smoke.exe" "$GAME" build/20c/restart-sdl sdl
"$B/mmodern_phirna_smoke.exe" "$GAME" build/20c/phirna-direct
"$B/mmodern_who_will_smoke.exe" "$GAME" build/20c/whistle-direct
"$B/mmodern_myra_smoke.exe" "$GAME" build/20c/myra-direct
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_phirna_smoke.exe" "$GAME" build/20c/phirna-sdl sdl
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_who_will_smoke.exe" "$GAME" build/20c/whistle-sdl sdl
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_myra_smoke.exe" "$GAME" build/20c/myra-sdl sdl
"$B/mmodern_manual_event_smoke.exe" "$GAME" build/20c/manual
"$B/mmodern_navigation_flow_smoke.exe" "$GAME"
mkdir -p build/20c/application
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_graphics_smoke.exe" "$GAME" save-phirna escape build/20c/application/phirna.mmsave
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_graphics_smoke.exe" "$GAME" resume escape build/20c/application/phirna.mmsave
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_graphics_smoke.exe" "$GAME" manual-no escape
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  "$B/mmodern_graphics_smoke.exe" "$GAME" manual-yes escape
ctest --test-dir "$B" --output-on-failure
git diff --check
```

Capture and inspect native 320x200 frames for each resumed checkpoint and fresh
control: saved party HUD, effective object presence/absence and clean NPC/WhoWill
layers. Keep native-frame inspection distinct from automated pixel assertions
and SDL dummy/software execution. Full CTest count is whatever is registered at
completion; do not predetermine it from the historical 47 tests.

### Short Windows physical-window procedure

From PowerShell with UCRT64 runtime DLLs on PATH, no dummy video driver, create
an ignored output directory and launch a real window:

```powershell
New-Item -ItemType Directory -Force build/20c/manual-window
& .\build\20a\mmodern.exe --render-map 'F:\Games\gog\Might and Magic 4-5' 23 8 2 north --save-file 'build/20c/manual-window/phirna.mmsave'
& .\build\20a\mmodern.exe --load-game 'F:\Games\gog\Might and Magic 4-5' 'build/20c/manual-window/phirna.mmsave'
```

In the first process: Space opens the question; F9 must refuse while pending
without advancing it. Choose Yes and complete acknowledgment, observe removal,
press F9 and verify title/console success, then exit. Run the second command
only after the first process ends. Verify the first visible frame has the saved
position/party and no plant, Space cannot collect again, and ordinary controls
work. Record the two process launches separately.

Repeat using map 20 `(5,14)` North and `whistle.mmsave`: F9 during WhoWill refuses,
F1 selects, complete the original acknowledgment, save/exit/relaunch, verify
absence/no repeated grant. Repeat map 23 `(9,11)` West with `myra.mmsave`: finish
both request pages, save/exit/relaunch, verify clean initial scene and original
revisit dialogue. Q2 ownership is established by state assertions/logs; visual
repetition alone cannot prove a flag that the script does not check. Finally
launch fresh `--render-map` controls with no load. Record any root-owned return
seen separately from the automated cumulative test. Do not expand this short
procedure into claims about unobserved matrix cells or ordinary travel.

Stage stopping point: all criteria, build/full CTest, cross-process original
acceptance and required physical-window validation pass; update project status
and README for the new public interface and prepare a concise commit message.
Do not mark complete if manual validation remains pending. No push or milestone
tag without explicit approval; no next milestone/sub-stage starts automatically.

## 10. Risks, review decisions and exclusions

Concrete risks and proposed resolutions:

- Public character fields permit values beyond original bytes; serializing only
  resource bytes would lose modeled state. Use full typed fields and distinguish
  encoding limits from active-rule safety preflight. Check arithmetic without
  weakening existing large/negative-value rule tests.
- Membership loader diagnostics are permissive, including duplicates/empty
  slots. Preserve supported values; do not invent uniqueness, six-member minimum
  or portrait-range restrictions on inactive roster entries.
- A world import that calls Remove would invent event effects. Import validated
  independent identity sets with original ordering, never cells or sprite IDs.
- Constructor composition runs before `initial()`. All restored values must be
  in final owners before constructing flow; both initial dispatch and first-frame
  order need explicit production-path tests.
- File replacement is platform-sensitive. Establish locked-target and injected
  failure preservation on Windows; do not substitute a POSIX-only test or claim
  power-loss safety. Concurrent writers/network filesystems are outside v1.
- Whole-archive signatures are conservative and add sequential startup I/O.
  Recommend this bounded implementation over resource-manifest infrastructure;
  record measured cost during 20B and revisit only if it is material. CRC32 is
  accidental-compatibility checking, not security authentication.
- Original integration tests currently prove same-process reconstruction.
  Distinct PIDs, file handoff and actual CLI/Application coverage are required;
  test helpers must not inject the effects being accepted.

The user approved this scope/format/interface and explicitly authorized **20A
only**. Ordinary engineering choices above are resolved by the approved plan,
not a list of blockers for the user. There is no
identified external prerequisite requiring a roadmap change. Stage approval,
physical validation and eventual release actions remain separate decisions.

Excluded: original Xeen/ScummVM save compatibility; suspended-script/dialog
persistence; in-session load; inventory, Myra Root consumption/quest clearing/
rewards; generic NPC services, shops, combat, Darkside gameplay; world animation,
new indoor objects, post-M19 UI fidelity; new game clock; generic serialization
or migration systems; broad translations; detailed M21-M23 planning. M19's
documented visual notes remain non-blocking and are not reopened.

## 11. Precise first implementation task and planning handoff

After explicit approval, the recommended first task is:

> Implement Milestone 20A only according to sections 2, 4, 5, 7 and 9 of this
> plan. Reverify checkout and preserve preexisting changes. Add the complete
> typed snapshot/codec, checked resource-aware preparation and the narrow
> existing-owner export/import access required for it. Add and run synthetic
> format/state coverage including all numeric domains, independent categories,
> original identity validation and rejection without partial publication. Build
> and run the specified focused regressions. Record results and stop before
> file writing, production CLI/SDL save controls or 20B implementation.

Planning deliverables are this proposed document, a minimal current-status link
and the roadmap's corrected next action. Planning checks: baseline/read-only
source/test/configuration inspection, bounded official Win32 API documentation
review, documentation diff inspection, `git diff --check`, and a separate
`git diff --no-index --check -- /dev/null docs/milestone-20-plan.md` check for
the new untracked document (no whitespace diagnostics). No build, CTest,
original-data smoke, rendering, physical-window test or implementation was
performed for this documentation-only task. Historical validation records are
unchanged. No commit, push, tag or branch operation is part of this handoff.

## 12. 20A implementation and validation evidence

**2026-09-08: implemented and validated; awaiting review.** This records only
20A. M20 remains incomplete and M19 remains the latest stable milestone.

Checkout reverified before implementation: `main`, HEAD
`fd94e91fac626663c44407e30c709a9dc89b10d9` (following M19 completion `59d3016`).
Preexisting changes were the modified project-status/roadmap documents and the
untracked M20 plan; these were preserved and extended. The dependency checkout
was read-only verified at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with no
working-tree changes. The source/build configuration from dependencies.md was
used without dependency updates. No commercial resources were changed.

### Implemented contracts

- `XeenSaveSnapshot` is a temporary typed transfer value. Capture reads existing
  party/roster, camera, game flags and world owners. It carries all 30 modeled
  characters, membership including duplicates/empty parties, 35 uint32 counters,
  30 quest flags, 256 game flags and independent sorted original identity sets.
  Loading metadata and interpreter/presentation/cache state are absent.
- `XeenSaveFormat` implements exactly the section 4 v1 little-endian layout,
  20-byte header, payload CRC32, canonical booleans, bounded names/counts, exact
  length/EOF and the 4 MiB limit. Signed fields retain their full representation;
  quest counters are never narrowed to original resource bytes. Unknown versions
  are rejected. Archive fingerprinting consumes a caller-supplied stream in
  64 KiB chunks; no file/path API is implemented. The library and test targets
  calling crc32 explicitly link `ZLIB::ZLIB`.
- `XeenSaveState::restoreBeforeGameplay` first checks the snapshot and actual
  resource signatures supplied by a trusted caller. Initial party loading
  rebuilds metadata, then all durable values replace the initial defaults in
  candidate owners. Checked active-character arithmetic shares the existing
  rule calculations and tables, with no normalization or new byte-range limits.
  Unsupported active portraits retain their existing rejection policy.
- Candidate world loading validates the committed map and every affected map,
  original object/event record and resource presence. Imports do not call Remove,
  infer flags or replay scripts. A required presentation-preflight callback
  checks the candidate's needed resources. Only after every throwing operation
  succeeds are party/camera/flags/world published through nonthrowing swaps or
  assignments. Failed preparation leaves destination values and caches intact.
- `XeenParty::fromRosterIds`, read-only world-set access and validated world import
  are the narrow owner additions. No new live state store or production flow
  was added. Snapshot capture deliberately requires its caller to enforce idle
  eligibility; production enforcement is 20B work.

### Acceptance bookkeeping

| Criterion | 20A status and evidence |
| --- | --- |
| M20-A01 | **Closed.** Typed all-field deterministic round trips; all 30 slots, inactive records, independent defaults, ordered duplicate/empty/one/six membership; independent wire fixture verifies layout rather than only codec symmetry. |
| M20-A02 | **Closed.** Full signed wire limits, HP/SP limits, all condition bytes and counter slots including UINT32_MAX, each quest/game flag, name 0/16/high-bit bytes, modifier fields and identity boundaries. Safe large/negative rule inputs survive unchanged. |
| M20-A03 | **Closed.** Independent owner categories, off-camera object/event sets, base-disabled objects, event-only/object-only states, same-cell distinct original records and equal-count/different-identity states. |
| M20-A04 | **Closed.** Every truncation point with original and repaired envelopes, corruption, reserved/version/side/CRC/length errors, booleans, names, fixed counts, ordering/duplicates and maximum/over-limit counts; decoder returns no partial result. |
| M20-A05 | **Closed at the state/preparation layer.** Signature mismatch, malformed initial resources, map mismatch/missing resources, invalid record boundaries, membership/active enum/portrait/arithmetic rejection, missing callbacks and throwing composition preflight. Sentinel values, metadata and destination caches remain unchanged. No startup window or event is created by this API. Production startup error reporting remains A11. |
| M20-A08 | **Partial, owner/flow only.** Immediate quest-item/quest-flag/Remove effects survive later errors; abandonment retains immediate party effects but rolls back event camera/game flags; WhoWill cancellation commits working camera/game flags. Prior movement survives automatic failure. Each resulting authoritative state survives codec/candidate reconstruction after the flow becomes idle. Production save eligibility and disk paths remain 20B/20C. |
| M20-A09 | **Partial, not closed.** Fresh flow construction after restoration does not dispatch; explicit new-session `initial()` dispatches grant/flag/teleport; later navigation still dispatches. The production Application new-session/resume initialization path must be implemented and validated in 20B. |
| M20-A10 | **Partial, owner/flow only.** Synthetic composition uses restored camera, ownership, HP/portrait placements and removals; fresh presentation state, independent equal-count graphs, and actual map/object/script/text provider reloads are checked. This is not native scene/sprite-cache, Application/SDL first-frame or physical-window evidence. |
| M20-A06/A07/A11 | **Pending 20B.** No filesystem replacement, F9/save eligibility or production CLI/startup implementation. |
| M20-A12-A15 | **Pending 20C.** No original-data disk restart or cumulative cross-process acceptance was performed. |
| M20-A16 | **Partial milestone evidence.** Current full CTest passes; the milestone's original-data, native-frame and physical-window portions remain pending. |

### Commands actually run and results

PowerShell, with the existing UCRT64/MSYS executables on PATH:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
cmake -S . -B build/20a -G 'MSYS Makefiles' -DCMAKE_BUILD_TYPE=Debug -DSCUMMVM_SOURCE_DIR=D:/Projetos/MModern/scummvm-known-good-candidate -DSCUMMVM_BUILD_DIR=D:/Projetos/MModern/build-scummvm-6814ee9b-ucrt64
cmake --build build/20a --parallel 4 --target mmodern_save_format_tests mmodern_save_state_tests
ctest --test-dir build/20a --output-on-failure -R 'xeen_save_'
cmake --build build/20a --parallel 4
ctest --test-dir build/20a --output-on-failure -R 'xeen_(save_(format|state)|character_|party_visual_state|quest_|game_flags|session_|world|remove|event_system|navigation_flow|who_will|npc)'
ctest --test-dir build/20a --output-on-failure
git diff --check
```

Final results: configure and complete Debug build succeeded (GNU 16.2.0,
UCRT64, zlib 1.3.2); new tests **2/2**, specified focused selection **19/19**
(4.28 s), complete CTest **49/49** (5.67 s). Existing synthetic SDL input,
WhoWill and NPC dummy/software regressions are included; no production save/SDL
claim follows from them. Outputs remain under ignored `build/20a`.

During iteration, a target requested before regenerating the MSYS makefiles
reported no rule; reconfiguration resolved it. The first state-test link lacked
`mmodern_app` for existing EventFlow symbols; explicit test linkage resolved it.
The final build and all test runs pass. Documentation/code diffs and whitespace
were checked, including each untracked new file separately with
`git diff --no-index --check -- /dev/null <path>`; no whitespace errors.
Historical test results above are not counted as new 20A executions.

### Review handoff and remaining boundaries

No scope deviation or new roadmap trigger was found. The checked rule helper is
the narrow preflight contemplated in the plan: existing rule tables/calculations
are shared, and existing runtime behavior is preserved. The preparation API's
remaining integration obligations are explicit: 20B must supply independently
computed archive signatures, actual presentation-resource preflight, enforce an
idle save boundary, and construct flow references only after restoration. It
must validate the production distinction between new-session and resume startup.

No file writes, F9, CLI save/load, Application changes, in-session load or
cross-process work was added. Original checkpoints, native-frame inspection and
physical-window runs were not repeated for 20A. README remains unchanged because
there is no new public production interface. The historical M17/M18 stable-status
statements now explicitly name their historical boundaries; M19 evidence is
preserved. Stop here for 20A review; 20B/20C require separate authorization.
No commit, push, tag, dependency update or branch change was performed.

## 13. 20B implementation and validation evidence

**2026-09-08: implemented and validated, awaiting review.** Independent review
returned **APPROVE MILESTONE 20A** after its commit. The user then authorized
20B only. Section 12 remains the historical 20A implementation record; its
then-pending production evidence is supplemented here, not rewritten.
M19 remains the stable milestone; M20 is incomplete and 20C has not started.

### Verified baseline and scope

- Branch `main`; HEAD and local `origin/main` both
  `4d65e34564452647e15a7d7b9c9144cc449a6a1b` (`Implement Milestone 20A save state foundation`).
- Initial working tree was clean. No reset, clean, pull, branch operation,
  commit, push, tag or history rewrite was performed.
- Pinned ScummVM source was read-only verified at
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, with empty status. Configuration
  remains the existing `build/20a` Debug/MSYS Makefiles/UCRT64 environment and
  dependency paths recorded in section 12. No dependency changed.
- Original installation `F:/Games/gog/Might and Magic 4-5` was used read-only
  for bounded production smoke checks. All generated files are under ignored
  build directories, outside the commercial installation and tracked sources.

### Production integration

`Application::renderMap` and `loadGame` bind ordinary archive/map/object/event,
party/flag, font/composer and SDL providers, then call the same
`Application::playGameplay` implementation in `XeenGameplay.cpp`.
`XeenGameplayServices` holds only borrowed providers; it owns no session or save
state. This small extraction lets synthetic tests execute Application's actual
startup decision and input handler, without creating another session controller.

The old render-map forms retain their camera defaults and initial automatic
event. Optional trailing `--save-file <path>` configures F9. `--load-game <game>
<path>` uses the saved camera and uses that path for subsequent saves. Native
Windows command-line decoding preserves Unicode before converting paths with
`u8path`; invalid new syntax returns 1. Missing installation retains return 2;
load/preparation failures return 3 without a new-session fallback. SDL failure
retains return 4. No save/load menu, autosave or save-on-exit was introduced.

For resume, file decoding is inert. Production signatures and the existing 20A
preparer validate candidates, including an actual `CloudsMapComposer::compose`
preflight with the installed assets. The resulting owners are final before
EventSystem/flow callbacks reference them. Application explicitly chooses
`flow.frame()` for resume and `flow.initial()` for a new session. The SDL window
is created only after startup composition succeeds; normal later navigation
retains automatic dispatch. The 20A snapshot, codec and owner APIs are unchanged.

F9 maps to `SaveGameAction` through existing SDL repeat filtering. Application
intercepts it before `flow.handle`, preserving retained labels and refusing
pending display/pages, acknowledgment, Yes/No, NPC and WhoWill (including invalid
selection retry) without preparation or writes. It also rejects reentrant event
callbacks and requests after a fatal dispatch. Refusals are not queued. After a
real completion/cancellation, a fresh F9 captures current authoritative values,
validates them on disposable candidates, and writes them. Success/failure and
absolute path are reported on stdout/stderr and through the existing SDL window
title; the status lasts until the next attempt. Quit stops input consumption and
never saves implicitly.

### Windows file and signature boundary

`XeenSaveFile` isolates native file operations from gameplay/interpreter code.
Resolution uses the process working directory once, requires an existing parent
and `.mmsave`, resolves the parent, and rejects the detected commercial root,
directories/reparse leaves, alternate streams, common device names and detectable
network targets. It creates no directories. Windows wide APIs support spaces and
Unicode names. These are bounded local-path checks, not a generic filesystem
security or concurrent-writer subsystem.

Write encodes completely before destination I/O and decodes any existing target
as a supported v1 save before replacing it. It creates a unique sibling using
`CreateFileW(CREATE_NEW)`, writes checked chunks, rejects short writes, checks
`FlushFileBuffers` and close, then publishes with
`MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`. It never
truncates/deletes the old target to make publication succeed. Failure cleans up
only this attempt's temporary; cleanup failure includes that path in the error.
Reads use the requested file only, check size before allocation and invoke the
approved bounded decoder. Abandoned temporaries are never substituted.

The fault seam is a small enum/callback around these operations, not an abstract
filesystem. Tests perform real local writes and inject open/write/short-write/
flush/close/replace/cleanup failures; the close-failure injection occurs after
closing the real test handle so the seam does not leak handles. Actual Windows
sharing locks also exercise unreadable files and failed replacement. Every failed
replacement preserves the old bytes, which are decoded again. This proves the
specified handled-failure contract, not arbitrary kill/hardware/power-loss
safety. Concurrent external writers and network filesystems remain unsupported.

Archive signatures use read-only binary filesystem streams and 20A's 64 KiB
streaming CRC32 helper. They contain the detected xeen.cc size/CRC and dark.cc
presence/size/CRC, never paths, timestamps or archive payloads. They are computed
once at startup when save/resume is configured. Measured original-data runs:
127 ms on the first save smoke, 11 ms on its resume, then 12 ms each for a repeat
save/resume pair. No manifest or signature cache was justified. Resource changes
while running remain outside the contract.

### Acceptance matrix after 20B

| Criteria | Status and evidence |
| --- | --- |
| M20-A01-A05 | **Remain closed from approved 20A.** Both original format/state tests pass unchanged. No wire/owner redesign. |
| M20-A06 | **Implementation evidence ready for independent reconsideration after the second correction.** Both reviews left A06 partial: first for alias containment, then for changing the checked directory identity by removing the extended prefix. `xeen_save_file` now proves exact literal-directory identity through destination/temp creation, replacement and failure cleanup, alongside all previous alias and safe-replacement regressions. Independent closure/20B approval is not claimed. |
| M20-A07 | **Closed.** `xeen_save_flow` uses the Application input boundary for all seven pending fixtures (paginated display, acknowledgment, Yes/No, NPC, WhoWill, invalid retry, main display), compares frame/page/generation/timing, proves no queued write and fresh-F9 success. `xeen_save_sdl` proves F9 repeat suppression, pending refusal, native title updates, contextual cancellation, and quit-before-save ordering. No-target and reentrant/fatal boundaries are covered. |
| M20-A08 | **Production save/resume responsibilities closed.** Application saves and resumes synthetic states after immediate quest-item/quest-flag/Remove effects followed by error, acknowledgment abandonment and WhoWill cancellation, preserving camera/game-flag policies. A fatal automatic failure preserves prior movement but production shuts down and refuses subsequent saving; no new recoverable-auto-error policy was invented. Original checkpoint certification remains A12-A15 in 20C. |
| M20-A09 | **Closed at the production startup layer.** Application's common startup path is exercised with an automatic grant/flag/teleport at the saved cell: new session dispatches exactly once, resume dispatches zero initial events, and later rotation dispatches normally. This is beyond the 20A flow-constructor-only test. |
| M20-A10 | **20B production portions closed; milestone visual evidence remains partial.** Restored first composition/first shown frame, absence of default frames, fresh presentation, equal-count/different-identity startup graphs and genuine map/object reloads pass through Application. Real composer preflight and idle SDL resume pass. Full native/sprite-cache/original-checkpoint/physical observations remain 20C. |
| M20-A11 | **Closed.** `xeen_save_cli` launches the real executable using synthetic original-format archives: invalid/conflicting syntax, old/new render forms reaching Application, Unicode load path, missing/locked/malformed/version/incompatible/semantic/first-composition failures, exact failure exit and no resume/fresh fallback. Application tests additionally inject throwing/invalid-frame preflight and show that save failure leaves gameplay usable. |
| M20-A12-A15 | **Pending 20C.** No Phirna/Bone Whistle/Myra disk checkpoint or cumulative multi-map certification was performed. |
| M20-A16 | **Partial milestone evidence.** Build/full CTest and bounded original SDL controls pass; the full original matrix, native frame inspection and physical-window milestone procedure remain pending. |

Synthetic scenes/archives are fixtures, not copied original resources. CLI child
processes validate CLI failures; synthetic Application reconstruction uses fresh
graphs in the same test process. The original idle save/resume smoke uses two
separate invocations but certifies no quest checkpoint or ordinary travel.

### Commands and results actually obtained

PowerShell commands used (the build directory retains the approved configuration):

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
cmake -S . -B build/20a
cmake --build build/20a --parallel 4
cmake --build build/20a --parallel 4 --target mmodern_save_file_tests mmodern_save_flow_tests mmodern_save_sdl_tests mmodern_save_cli_tests
ctest --test-dir build/20a --output-on-failure -R 'xeen_save_'
ctest --test-dir build/20a --output-on-failure -R 'xeen_(save_|quest_|game_flags|session_|remove|visual_remove|event_|manual_event|navigation_flow|who_will|npc|character_|party_visual_state)|sdl_input'
ctest --test-dir build/20a --output-on-failure
cmake --build build/20a --parallel 4 --target mmodern_graphics_smoke
$env:SDL_VIDEODRIVER = 'dummy'
$env:SDL_RENDER_DRIVER = 'software'
& ./build/20a/mmodern_graphics_smoke.exe 'F:\Games\gog\Might and Magic 4-5' save-idle escape build/20b/idle.mmsave
& ./build/20a/mmodern_graphics_smoke.exe 'F:\Games\gog\Might and Magic 4-5' resume-idle escape build/20b/idle.mmsave
```

The existing GraphicsSmoke modes `map`, `event`, `manual-no` and `manual-yes`
were also run with the same game path, dummy/software environment and `escape`.
All six modes passed. `build/20b` was explicitly created by the validation command;
the application/file implementation does not create directories.

Configure/complete Debug build and smoke target build passed. Save selection:
**6/6**; specified stage-focused selection: **33/33**; full CTest: **53/53**.
New tests were also run separately during iteration. Final whitespace/diff
checks include `git diff --check` and a separate no-index whitespace check for
every new untracked file. No original native-frame inspection or physical-window
validation was performed in 20B; historical M19/20A results are not new runs.

During iteration, the Yes/No fixture initially used a main-display opcode and the
Remove fixture initially removed its own following error instruction. Both were
corrected to established event contracts, without production opcode changes.
The CLI fixture initially lacked the nested initial-state archive; the shared
synthetic archive name helper gained the pinned reader's four-hex-digit identity
rule and the test now supplies the original nested-container structure. These
were test-fixture corrections; the final suite has no failures.

### Changes, deviations and stopping point

Production changes are confined to Application startup/input composition,
SaveGameAction, the SDL title/input hook, native CLI argument handling and the
small save-file boundary. CMake registers four tests and links the native
command-line helper. One existing unsupported-portrait diagnostic in
`CloudsUiLayout.cpp` was changed to English because it is now surfaced by resume;
no portrait policy changed. README documents the newly usable interface.
The roadmap is unchanged: no replanning trigger was found.

There is no 20A format/state correction or architectural deviation. Providers
must outlive the synchronous gameplay call; snapshots remain temporary values.
The existing initial automatic failure remains fatal, and in-session load is
still excluded. File durability is limited to the documented handled-failure
contract. No inventory, quest consumption/reward, save slots, autosave, physical
validation or 20C checkpoint certification was started. Stop for 20B review;
20C requires separate authorization. No commit, push or tag was performed.

### Independent review correction: Windows directory aliases

This subsection records the first correction and its validation boundary. The
second review and identity-preserving correction below supersede its path
representation and A06 readiness assessment; neither review approved 20B.

The first independent 20B review returned **REQUEST CHANGES**. Its only blocking
finding was an installation-containment bypass: under the configured UCRT64
toolchain, `std::filesystem::canonical` could retain a trailing-dot directory
alias or a junction path. Comparing those strings with the installation string
did not reliably compare the directories Windows would actually access. A06
was therefore partial at that review boundary; the earlier test results above
did not demonstrate alias protection.

The correction is confined to `XeenSaveFile.cpp`, `XeenSaveFileTests.cpp` and
these two current-status/evidence documents. `resolve` now opens the existing
installation and destination-parent directories with `CreateFileW`,
`OPEN_EXISTING` and `FILE_FLAG_BACKUP_SEMANTICS`, without
`FILE_FLAG_OPEN_REPARSE_POINT`. `GetFileInformationByHandle` verifies directory
attributes. `GetFinalPathNameByHandleW(FILE_NAME_NORMALIZED | VOLUME_NAME_DOS)`
returns the resolved directory paths, following junctions and normalizing
aliases through Windows. The implementation retains local DOS paths, strips
only the verified local DOS extended-path prefix, and compares equal/descendant
paths with case-insensitive `CompareStringOrdinal` and a separator boundary.
The returned save path uses that resolved parent, not the input alias.
Directory resolution creates neither a destination nor a temporary file.
These APIs use [normalized final paths](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfinalpathnamebyhandlew)
and [ordinal Windows case comparison](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-comparestringordinal).

The real Windows regression creates a synthetic protected directory and a
mount-point junction using `DeviceIoControl(FSCTL_SET_REPARSE_POINT)` entirely
under ignored `build/20a/save-file-tests`. Junction creation failure fails the
test with its Windows error; it is not skipped or simulated. Six save paths
are refused: direct, direct child, trailing-dot, trailing-dot child, external
junction and junction child. Each checks that `resolve` refuses before the
write call and that the protected tree contains no destination or temporary
files. Additional cases resolve aliases in the installation argument itself.
External targets with a similar directory-name prefix, spaces and Unicode each
pass new-save and valid-save replacement/decode. An allowed trailing-dot parent
returns the same resolved path as its ordinary spelling and saves successfully.
The real junction is removed with `RemoveDirectoryW`, with checked success.

Correction validation used the unchanged configured build and dependency:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
cmake --build build/20a --parallel 4 --target mmodern_save_file_tests
ctest --test-dir build/20a --output-on-failure -V -R '^xeen_save_file$'
cmake --build build/20a --parallel 4
ctest --test-dir build/20a --output-on-failure -R 'xeen_save_'
ctest --test-dir build/20a --output-on-failure -R 'xeen_(save_|quest_|game_flags|session_|remove|visual_remove|event_|manual_event|navigation_flow|who_will|npc|character_|party_visual_state)|sdl_input'
ctest --test-dir build/20a --output-on-failure
cmake --build build/20a --parallel 4 --target mmodern_graphics_smoke
git diff --check
git diff --no-index --check -- /dev/null src/platform/XeenSaveFile.cpp
git diff --no-index --check -- /dev/null tests/XeenSaveFileTests.cpp
```

Results: focused file test **1/1**, save selection **6/6**, stage-focused
selection **33/33**, full CTest **53/53**; both requested builds and the graphics
smoke target build passed. Whitespace checks passed and the correction diff
was inspected against copies of the preexisting uncommitted 20B files. The
commercial installation was not used for writes or original-data validation
in this correction. No graphics smoke execution, 20C checkpoint matrix or
physical-window procedure was run anew.

A06 now has passing evidence for the corrected local Windows contract and
returns for independent closure/re-review; this is not independent approval
of 20B. All other acceptance statuses remain as recorded above. The safe-write
algorithm and its limits are unchanged: concurrent external changes, network
filesystems and absolute crash/power-loss durability remain outside the
contract. HEAD and `origin/main` remain `4d65e34564452647e15a7d7b9c9144cc449a6a1b`
on `main`; all preexisting 20B changes remain uncommitted. No 20C work began.

### Second review correction: retain the checked extended-path identity

The second independent review returned **REQUEST CHANGES**. It confirmed the
ordinary trailing-dot/junction correction and its six refusal cases, but found
that removing `\\?\` from the final directory path re-enabled ordinary Win32
normalization. With distinct synthetic `protected` and literal `protected.`
directories, a junction could resolve to the latter while the subsequently
written ordinary path accessed the former. A06 remained partial solely for this
identity defect. The newly added isolated regression failed against the first
correction with `resolved path changed checked directory identity`, before any
save write in that reproduction.

`XeenSaveFile` now retains the local extended DOS path returned by
`GetFinalPathNameByHandleW`. Containment still uses ordinal case-insensitive
comparison with component boundaries. Drive-root handling retains the root
separator. Parent resolution never strips trailing dots/spaces or converts the
verified path to ordinary DOS form. Other final-path namespaces remain refused.
Local extended paths copied from feedback or returned by `resolve` can be
resolved again without lexical normalization; non-native separators and relative
`.`/`..` components in such input are refused rather than rewritten.

The existing native target-attribute check now also returns whether the target
exists, replacing the `std::filesystem::exists` call before existing-save
validation. The redundant `std::filesystem::is_directory` precheck is removed;
the directory handle's attributes remain authoritative. This avoids routing
save existence/parent checks through the CRT filesystem layer. All actual save
I/O now consumes the preserved path: `GetFileAttributesW`, `CreateFileW` for
existing saves and sibling temporaries, `MoveFileExW` for publication, and
`DeleteFileW` for cleanup. The temporary suffix is appended to the verified
destination path without changing its parent. `Application` stores and passes
that same `filesystem::path`; UTF-8 conversion is used only for logging/title
text and does not feed back into I/O. No Application/flow code changed.
The [Win32 namespace rules](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
explain why removing the prefix changes semantics;
[`MoveFileExW`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw)
supports the extended-path representation used by both operands.

The exact Windows reproduction creates both distinct directories and a real
mount-point junction under ignored `build/20a/save-file-tests`. Native handle
volume/file IDs prove that the junction and returned parent refer to literal
`protected.`, distinct from `protected`. The production `resolve -> write`
sequence creates and replaces the save only in the literal directory. Native
enumeration during injected write/replacement failures observes the temporary
in that same directory; successful cleanup leaves no temporary. A preexisting
protected save remains byte-identical, with no unexpected file in the normal
sibling. The literal destination is decoded after success/failure, and a
malformed existing literal save is refused without modification. Both ordinary
and extended drive-root containment, case-insensitive containment, unchanged
leaf spelling and 32 repeated accepted/refused resolutions without a process
handle-count increase also pass. The junction is removed with checked success.

During test development, UCRT64 `std::filesystem::directory_iterator` enumerated
the normalized sibling for the literal extended directory. The identity fixture
therefore uses `FindFirstFileW`/`FindNextFileW`/`FindClose` for enumeration and
native APIs for raw bytes/existence, avoiding an ambiguous test oracle. The
existing relative-input check now supplies an actual relative input directly,
instead of deriving it from an extended output with `filesystem::relative`.
All first-correction alias tests and existing fault/locked-file tests remain.

Commands actually run using the unchanged configured build/dependency:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;C:\msys64\usr\bin;' + $env:PATH
cmake --build build/20a --parallel 4 --target mmodern_save_file_tests
Push-Location build/20a
./mmodern_save_file_tests.exe --identity-only
Pop-Location
cmake --build build/20a --parallel 4
ctest --test-dir build/20a --output-on-failure -V -R '^xeen_save_file$'
ctest --test-dir build/20a --output-on-failure -R 'xeen_save_'
ctest --test-dir build/20a --output-on-failure -R 'xeen_(save_|quest_|game_flags|session_|remove|visual_remove|event_|manual_event|navigation_flow|who_will|npc|character_|party_visual_state)|sdl_input'
ctest --test-dir build/20a --output-on-failure
cmake --build build/20a --parallel 4 --target mmodern_graphics_smoke
git diff --check
git diff --no-index --check -- /dev/null src/platform/XeenSaveFile.cpp
git diff --no-index --check -- /dev/null tests/XeenSaveFileTests.cpp
```

The isolated identity regression passed, followed by file CTest **1/1**, save
selection **6/6**, stage selection **33/33**, and full CTest **53/53**. All builds
passed. Whitespace checks passed; correction diffs were inspected against the
preexisting uncommitted files. These are new stabilization results, separate
from both earlier validation records. No original-data smoke execution,
commercial-installation writes, physical-window validation or 20C certification
was performed. Only `XeenSaveFile.cpp`, `XeenSaveFileTests.cpp`, this document
and `project-status.md` changed in this correction. Format, owners, F9/CLI
syntax, startup/presentation behavior and safe-replacement ordering are unchanged.

A06 implementation evidence is ready for independent reconsideration, not
independent approval. Other criteria keep their recorded status: A01-A05,
A07-A09 and A11 closed; A10 production satisfied/overall partial; A12-A15 pending
20C; A16 partial. M19 stays stable and 20C unstarted. Concurrent filesystem
changes, network filesystems and absolute crash/power-loss guarantees remain
outside the existing contract. No commit, push, tag or branch operation occurred.
