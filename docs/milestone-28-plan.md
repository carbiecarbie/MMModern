# Milestone 28 - Durable bounded encounter completion and revisit

**Status: completed and accepted.**

## Final scope

Milestone 28 completes the bounded World of Xeen Clouds/Adventurer Skeleton
diagnostic introduced by M26 and made playable by M27. A genuine successful
combat End retires into a quiescent completed encounter; F9 can save it, a new
process can resume it into fresh owners, and R can perform the admitted same-map
true re-entry with the Skeleton still defeated and XP awarded exactly once.

The envelope remains Clouds map 20, original monster record 5/type 8, original
spawn `(13,2)`, entry camera `(13,1)` North, all 27 original monster records and
the original six active roster owners. Exact HP/SP, simultaneous conditions,
items and equipment/breakage bytes, owner XP, context, camera, flags and existing
world overlays survive restart. Completed mode permits read-only inspection but
cannot resume combat, ordinary events or exploration.

Diagnostic26 and every incomplete or unsafe Diagnostic27 state remain
unsaveable. The milestone does not add Run/disengagement, recovery/healing,
repeatable encounters, general traversal, additional monsters/maps, random loot,
new combat rules, autosave, save-on-exit, in-session load or Darkside gameplay.

## Completion authority and retirement

Successful combat End publishes `VictoryEnded` only after the final gameplay
minute, Victory phase and absence of pending End work are committed. Removal or
HP zero, awarded XP, a terminal approach latch, a Victory-looking frame or
coordinator destruction cannot establish completion by themselves. An End that
fails at the time boundary remains unsaveable even if lethal effects were already
published.

`XeenCombat::retireCompletedVictory` consumes a current Victory ticket after the
combat service busy guard has unwound. It requires the same world, party, roster,
camera and combat boundary, exact retained preimages, `VictoryEnded`, accounted
record 5, no candidate or pending work and a quiet boundary. Retirement
prepares the completed record before its nonthrowing publication, changes the
state to `VictoryQuiescent`, invalidates combat/approach capabilities and clears
the retired coordinator pointers. It never awards XP or changes gameplay state.

Completed authority is privately bound to the actual `XeenPartyState`, roster,
camera and world incarnation. Public copy, move, swap or value setters cannot
install or transfer it. Runtime revisions, tickets, operation/presentation
leases, owner-lifetime controls, Flow generations and retained callback preimages
are capabilities rather than durable state. Restore creates fresh bindings; old
pointers and tickets never revive.

Capture and re-entry verify the complete live graph against the retired
preimage. A changed character, supplement, context, camera, flag, overlay or
actor field latches the graph unsafe instead of silently recapturing newer
authority. Destroyed/reconstructed owners are distinguished even at reused
addresses through gameplay-borrow lifetime/incarnation state.

## Durable representation

The save stores the completed/accounted original monster identity, not an actor
array or combat transcript. This is sufficient for the admitted boundary because
all bystanders retain their resource-initialized values and record 5 has one
canonical completed overlay:

- all 27 actors are reconstructed in original MOB record order with original
  metadata and MON statistics;
- every bystander retains its initialized coordinates, HP, activation, status
  and lifecycle, including Disabled or Unresolved records;
- record 5 retains immutable metadata/statistics but becomes HP 0, coordinates
  `(-128,-128)`, inactive, Physical and Defeated;
- capture and re-entry reject any moved, injured, activated, retyped, missing or
  otherwise changed bystander and any noncanonical record-5 state.

The completed extension also stores every encounter-context field and exactly
six owner-keyed supplements for roster owners `[0,1,6,11,14,18]`. Each supplement
contains Might, Speed and Accuracy permanent/temporary values, temporary AC and
XP. Absence on the other 24 owners is required. Existing character payloads
retain all 30 characters, all four nine-slot item arrays, HP/SP, conditions and
other modeled fields. Existing membership, quest items/flags, game flags, camera
and disabled object/event identities retain their established representations.

Resource payloads, actor arrays, combat phase/work/RNG, approach state,
certificates, notices, presentation state, caches and runtime capabilities are
transient or reconstructed. Restoration never performs damage, XP allocation,
equipment/transfer operations, events, healing or normalization.

## Save format and compatibility

The reader accepts supported v1, v2 and v3 files. Ordinary eligible capture
continues to write v2. Completed Diagnostic27 capture writes v3. V1 restoration
retains its narrow missing-item-field policy; v2/v3 preserve explicit item bytes,
including holes and ID-zero records. Legacy files have no implicit encounter,
context or supplement state and cannot be promoted to completion by absence.

V3 retains the complete v2 payload unchanged and immediately appends a fixed
243-byte extension. Let `B` be the first byte after the existing disabled-event
list:

| Offset | Representation |
| --- | --- |
| 0 | u8 extension-present, exactly 1 |
| 1 | u8 entry, exactly 2 = Diagnostic27 |
| 2 | u8 completion, exactly 1 = completed Victory |
| 3 | u8 accounting-consumed, exactly 1 |
| 4..10 | monster identity: u8 Clouds side 0, u16 map 20, u32 record 5 |
| 11..12 | u8 WorldOfXeenClouds profile 0, u8 Adventurer difficulty 0 |
| 13..20 | four u16 values: ctr24, day, year, minutes |
| 21..29 | nine u8 context effects |
| 30..41 | six u16 light/resistance values |
| 42..43 | canonical u8 rested and newDay booleans |
| 44 | u8 supplemental owner count, exactly 6 |
| 45..242 | six 33-byte owner records in order `[0,1,6,11,14,18]` |

Each owner record is one u8 owner ID, seven little-endian i32 input values and
one u32 XP value. Record `k` starts at `45 + 33*k`, with input fields at offsets
`+1,+5,+9,+13,+17,+21,+25` and XP at `+29`. No padding, actor count, actor data,
optional presence shape or trailing bytes are permitted. The envelope payload
length and CRC include the extension.

Structural validation requires the fixed discriminators, identity, owner count
and sequence, canonical booleans, exact EOF and bounded values. The completed
profile requires day 1/year 610, `ctr24 < 24`, minutes 491..959, zero effects and
light/resistance arrays, false rested/newDay, and supplemental input values in
their admitted 0..255 range. Unknown versions and malformed or partial v3
extensions are rejected.

Resource/domain validation requires the matching archive signature and compatible
map/MOB/MON/EVT data; exactly 27 original actor identities; the admitted camera,
profile and membership; precisely six matching CHR-derived input records; and XP
equal to initial resource XP plus the single accepted M27 allocation for eligible
owners. Completed item validation preserves physical slots and bytes while
allowing only admitted transfer/equipment arrangements and equipped-armor bit-7
breakage. Existing valid v1/v2/v3 targets may be replaced after structural decode;
unknown or malformed targets remain protected.

## Fresh-owner restoration

Restore is startup-only and requires fresh destination owners without encounter
authority, context, supplements or borrowed gameplay capabilities. It first
validates transfer structure and archive identity, then constructs unpublished
party/world/camera/flag candidates from immutable providers. The party candidate
receives exact saved values; the world candidate reconstructs all original actors
and applies the canonical record-5 defeated overlay.

After every initial-party, CHR, PTY, MON, map, MOB, EVT and presentation callback,
retained guards verify both the destination and candidate graphs, including
nested provider calls and exception paths. Candidate completed authority is
bound only to candidate owners. Publication swaps the fully checked values into
the destination through the private nonthrowing path, rebinds completion to the
destination addresses and creates fresh capabilities before any visible frame.

A nonmutating preparation failure publishes none of the candidate and opens no
gameplay window. If a callback externally mutates the destination, that mutation
is not concealed or rolled back; the candidate is rejected. Invalid or
incompatible completed saves do not fall back to a fresh game.

## Save eligibility and source authorization

One read-only domain predicate admits either an ordinary graph or a coherent,
bound `VictoryQuiescent` Diagnostic27 graph. Diagnostic26, Preparation, Approach,
combat and pending automatic work, `VictoryAwaitingEnd`, `VictoryEnded` before
retirement, Defeat, SupportStopped and Failed remain ineligible. Capture also
refuses active dispatch, save/re-entry, modal work, open inventory/inspection,
unresolved presentation leases, integrity failure, fatal failure and shutdown.

Unsafe F9 returns before capture, provider/preflight work and file I/O. It cannot
service End, close UI, retire combat, acknowledge presentation or queue a future
save. A fresh F9 is required after eligibility is reached.

For an eligible F9, Application retains the exact source owners, values, world
revision and UI generations across capture, detached restore/preflight and the
save-stage observers. It rechecks them after every callback and immediately
before `XeenSaveFile::write`; it never reacquires newer authority to legitimize
an obsolete snapshot. The synchronous save operation blocks reentrant F9/R/UI
work. Existing checked sibling-temporary, flush/close, protected replacement,
path/alias and old-file preservation behavior remains unchanged.

## Presentation and failure policy

Completed presentation is an adapter over the authoritative completed graph,
not a combat coordinator. It composes from current facts and holds a world-owned
lease until the matching frame crosses the actual SDL upload/present handoff.

| Failure class | Durable behavior |
| --- | --- |
| Integrity or source-preimage violation | Preserve published gameplay facts, latch the graph permanently unsafe and refuse capture/re-entry. |
| Composition, reporting or return-copy failure with unchanged facts | Preserve Victory and values; hold a recoverable presentation lease and refuse capture until one authorized reconstruction is uploaded and presented. |
| Successful authorized reconstruction | Recheck owners/preimages, install a current frame, complete its handoff and release only the matching lease without replaying reports or gameplay. |
| Recovery failure, lost/failed upload, fatal Application error or shutdown | Keep the graph unavailable and close the session; no fallback or successful recovery claim. |

Discarding Flow cannot clear a pending presentation lease. A replacement adapter
may clear only the matching recoverable lease after guarded reconstruction and
handoff. Stale recovery cannot clear a newer lease, and no recovery path can
clear integrity or fatal guards.

## Cache reconstruction, revisit and resume

| Boundary | Result |
| --- | --- |
| Cache reconstruction | Rebuild disposable maps/sprites/presentation while preserving the current live actors, camera, party state and completion generation. |
| Completed R revisit | Validate the live completed graph, reload compatible map/MOB/MON/EVT resources even with warm caches, reconstruct all 27 actors plus the defeated overlay, move the camera to `(13,1)` North and advance a transient entry generation. |
| Process resume | Reconstruct fresh actors and completed authority before the first frame while preserving the saved camera and all exact durable values. |
| Fresh Diagnostic27 | Start Preparation with original owners/items/XP and no completion; Begin creates the original actor collection with record 5 Present at `(13,2)` with HP 20. |

Repeated R and save/resume are idempotent apart from R's explicit camera
repositioning and transient generation. They do not respawn record 5 or replay
approach, combat, RNG, time, XP, events or equipment. Changed live actors refuse
re-entry rather than being silently reset.

## Production interface

The supported forms are:

```text
mmodern --encounter-27 [--combat-seed <nonzero-u32>] <game-directory> [--save-file <path>]
mmodern --load-game <game-directory> <save-path>
```

The save option is trailing, singular and requires a nonempty path. Seed parsing
remains strict. Duplicate, reordered, conflicting or extra arguments, camera
overrides and encounter/seed options on `--load-game` are rejected before path
access. Restored authority selects ordinary versus completed presentation after
successful validation; no caller flag can manufacture completed mode.

After successful End and retirement, the frame reports completed Victory and
offers F9 save, I inspect, R revisit and Escape exit. Completed I prints identity,
spawn/live coordinates, lifecycle, completion/accounting, context, owner XP,
HP/SP, all condition bytes and physical item bytes. It uses the existing browsing
controls but cannot arm transfer/equipment certificates or mutate state. F9 and
R refuse while inspection is open.

R is a typed completed action wired to
`XeenWorld::reenterCompletedEncounter`. SDL displayed-input generations, current
frame checks, poll-batch admission, key-release and repeat rejection prevent
stale or held input from repeating it. Space, B, Enter, movement, Wait and event
dispatch remain unavailable in completed mode. Escape/window close exits without
autosave or a final gameplay pulse.

## Durable original-data acceptance oracles

The accepted production controls derive expectations from original initial
resources plus explicit M27 outcomes rather than from capture, codec or restore
output:

- **Seed 1 injured victory:** Enter, period, six first-round Blocks, then Attack
  for each eligible displayed owner. Completion is minute 493, ctr24 1; active
  HP `[12,16,12,10,-4,5]`; Rebecca has Unconscious 1/Dead 0; each owner receives
  82 XP.
- **Seed 56 prepared equipment/breakage victory:** transfer Seymour owner 6
  Accessories slot 1 `{86,1,0,0}` to Arturius owner 0 slot 1 and equip it as
  `{86,1,0,8}`; both slot-0 necklaces remain exact. Six Blocks followed by
  Arturius and Tyro attacks complete at minute 492, ctr24 1; active HP is
  `[12,16,12,10,-17,5]`; Rebecca has Unconscious 1 and Dead 1; XP is
  `[100,100,100,100,0,100]` in active order. Her armor slots change from
  `(0,2,0,3)` and `(38,10,0,9)` to `(0,2,128,3)` and `(38,10,128,9)`.
- **Fresh control:** a separate no-load Diagnostic27 starts with original party
  values and no completion/accounting; Begin constructs all 27 original actors
  with record 5 Present, HP 20 and spawn `(13,2)`.

The connected evidence compares all 30 character payloads and all 144 item bytes
per owner, six-owner supplement presence and fields, HP/SP/conditions, context,
camera/flags/overlays and every metadata/live field on all 27 actors before
capture, on disk and after fresh-owner restore. Separate producer/consumer
processes, repeated save/resume/R, warm-cache true entry, unsafe F9, callback
mutation, presentation handoff/failure, real executable startup/load and ordinary
v1/v2 restart controls establish the production boundary without replacing the
maintainer's physical check.

## Final acceptance

Milestone 28 closed after the final build and complete automated suite passed;
the required original-data producer/consumer/revisit/fresh and executable
lifecycle evidence passed; the independent implementation reviewer approved the
28C implementation; and the maintainer physically completed the connected seed-56
save, process exit, resume, read-only inspection, revisit, re-save and independent
fresh-session control. Automated and process evidence did not substitute for
that maintainer-performed physical acceptance.

The accepted result is the bounded completed Diagnostic27 lifecycle described
above. It establishes no broader exploration, recovery or subsequent milestone.
