# Milestone 59 - Vertigo content

**Tier A, Parts A and B. Status: completed and accepted (2026-10-11).**
Plan `7e44908`; Part A `384eb13`, `5166cab`; Part B and portrait feedback
`4d39d79`. The maintainer formally accepted
both parts after Debug/DOSBox CD play-testing and confirmed the portrait
sparkle correction in a further play-test.

## Goal and final scope

Complete the remaining city searches, display cases, jail, well and Vern/Guild
entry through the existing Event, Service, World and Party owners
([roadmap](../roadmap.md#near-term) step 5). Coordinates identified original-data
evidence, never runtime admission or per-cell behavior. Authority: World of Xeen
CD resources, maintainer DOSBox evidence and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Display cases and jail.** Event action 102 reads the selected member's
  existing Thievery, without a lockpick roll or XP. Success gives generated
  treasure. Failure advances the year once, grants Convicted Thief to the
  active party and uses generic TeleportAndContinue to reach `(27,11)`; the
  party walks out normally. The year jump resets yearly flags 55/155/222/231,
  temporary stats and effects, and marks the party rested while preserving
  day/minutes. It does not simulate daily ticks, stock or bank interest.
- **Well and heads.** Before Joe's proof, the selected member takes 5
  poison-type damage through the existing combat resistance/death rules and
  retained Journey RNG, without a message or the Poison condition. After proof,
  HP at or below the normal maximum is restored to that maximum with
  "You feel refreshed"; already-above-maximum HP uses the original alternate
  text without granting more HP. World flag 23 follows script order. AlterHed
  uses World's primary-map head cache at `(x % 16, y % 16)`, reloaded from the
  original HED on an ordinary map load.
- **City searches.** Original MoveObj, Spawn, treasure and AfterEvent combine
  generically across all 46 monster/item/empty crate, rubbish and bed sites.
  Monster crates retain their actor slot and enter normal combat; crates open,
  beds and rubbish retain their appearance, and repeat searches say
  "Nothing here." Signs and talk retain original text and directions.
- **Vern and Guild.** Vern charges 25 carried gold and grants award 0 to the
  selected character atomically. Cancellation and insufficient funds change
  nothing; repeat membership payments follow the original script. Guild entry
  uses the current character's membership, original art/DAT text and mouse/key
  controls. Non-members see the original refusal; members see Buy Spells and
  Spell Info, both visibly unsupported. Departure uses the shared service-day
  settlement once.
- **Portrait feedback.** Skill, award and XP grants use the original four-frame
  sparkle for their actual recipients, including all quest XP recipients.
  Convicted Thief is the reference's explicit exception. Feedback starts only
  after publication and does not replay on retries or load.
- **Publication.** Calendar, heads, damage, flags, objects, actors, rewards and
  camera remain staged until their guarded publication boundary. Cancellation,
  stale input and settlement retries cannot reapply effects or random draws.
  Saves remain unavailable during transient Event and Service states.

## Save decision

**v11, schema 9, content 14**, with one bounded world-flag state replacing the
special flag-16 owner and a bounded head-cache override set. Head serialization
preserves map-reload lifetime. Awards, skills, year, temporary state, actors,
objects, Event changes and RNG reuse existing owners; no legacy reader.

The maintainer approved Part A's M44 digest replacement on 2026-10-11: all
traces and common decoded fields were identical. Each save grew by exactly
128 bytes: 127 additional world-flag bytes, preserving flag 16, and one zero
head-override count; version, payload length and CRC changes were explained.
Every checkpoint matched the independent projection. Part B and the portrait
correction retained v11 and produced zero M44 audit differences.

## Results

- Independent Tier A plan and implementation reviews, including the portrait
  correction; findings resolved.
- Rules, original-data and save tests cover Thievery, year/reset semantics,
  typed damage/RNG, healing boundaries, flags/heads, all search families,
  Vern/Guild, cancellation, stale guards, retries and exact restoration.
- Final `build-rel` fast suite 149/149 and complete suite 183/183, run through
  the lightweight single-job runner required by [tools/AGENTS.md](../../tools/AGENTS.md).
- Debug closure fast suite 149/149 and three M44 scenarios 3/3 passed. The final
  portrait correction also passed the affected Debug event test and rebuilt
  the play-test executable.
- Maintainer play-test accepted both parts and confirmed the corrected sparkle.
  Retained quirks and reference-defined edges are in
  [original quirks](../original-quirks.md).

## Deferred

Guild spell purchase/catalogue and Spell Info functionality belong to roadmap
step 6. Locked-chest lock/trap/XP mechanics are absent from Vertigo CD's MOB.
The mirror, audio, animated shopkeepers and remaining map-23 content stay
deferred; M59 does not complete the mainland or the rest of the world.
