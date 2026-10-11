# Milestone 58 plan - Skills and mapping

**Tier A. Status: completed and accepted.** Plan `7326067`; implementation
`a39ec95` (with the reusable digest audit tool `ba1b416` as preparation).
The maintainer's Debug play-test was accepted.
Baseline: `ba1b41676e0b6c585e5c0b542c445254814a27b8`.

## Goal and final scope

Learn Cartography from Mylo and Pathfinder from Rialdo, use one character skill
state for rules and display, and explore with the original automap and Map
([roadmap](roadmap.md) step 4). Teacher addresses were evidence only, never
runtime admission. Authority: the World of Xeen CD resources, maintainer
DOSBox evidence of 2026-10-10/11 and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`.

- **Skills.** Each of the 30 roster characters has one mutation-tracked array of
  18 skill bytes initialized from CHR bytes 39..56; the duplicate max-stat skill
  booleans and immutable skill copies were removed. Maximum HP/SP, Thievery,
  Navigator's Rest effect and the character sheet read this state. Party skill
  checks follow the reference: two holders for Pathfinder/Mountaineer, all
  members for Swimming/Crusader, one otherwise, without a condition filter.
  Pathfinder does not gate forest movement, as in the reference.
- **Events.** NPC mode 2 (offer, then WhoWill), skill tests 13 (selected
  character) and 99 (party) and mode-13 grant/removal run generically. Grants
  increment one byte and wrap 255 -> 0 ("follows the original byte storage; not
  confirmed in DOS"). Unimplemented skill effects refuse before charging.
- **Teachers.** Mylo teaches Cartography for 100 gold; Rialdo offers Pathfinder
  for 2,500 but charges 2,000. Both charge again for repeat lessons and increment
  the skill, as their scripts have no learned-skill test. Debit and grant
  publish once; cancellation, stale input and insufficient gold change nothing.
- **Mapping.** With Cartography in the party, the occupied cell is recorded at
  the reference's stepped-update boundary. Exploration lives in the session world
  per physical map tile (256 bits per 16x16 tile), seeded by the original
  stepped bits and kept across services, combat, travel and save/load. The 7x7
  automap (`=` toggles it, using the saved PTY automap byte) and the 16x16 Map
  (M or the Map button) use original sprites, text and controls; viewing spends
  no time, turn or RNG.

## Decisions

1. **Skill byte policy** (maintainer, 2026-10-11): one byte per skill live and
   saved, wrapping at 256; ScummVM's wider live counter with save-time
   truncation is not reproduced. Repeat paid lessons make the boundary
   reachable, but only after 256 lessons.
2. **Repeat lessons follow the scripts** (maintainer clarification), superseding
   the plan's once-only assumption.
3. Rialdo's price, repeat lessons, party skill checks, Event skill sentinels,
   minimap/Map reference placement and sky/space Map visibility are recorded in
   [original quirks](original-quirks.md).
4. **Save v10** (schema 9, content 14): all 18 skill bytes per roster owner and
   bounded explored-tile bitsets. M44 digests were replaced after the
   maintainer-approved audit with `tools/m44-digest-audit.ps1`: traces and all
   common decoded fields identical; 420 skill bytes plus 37/107 exploration
   bytes, version, length and CRC explained.

## Results

- Unit, original-data and save tests for skills, teachers, exploration masks,
  Map/automap rendering, input parity and exact round-trips of all 30 owners.
- Independent Tier A plan and implementation reviews; findings resolved.
- Full CTest 183/183 on `build-rel` (two stale test fixtures corrected), Debug
  closure check (`-L fast` and the three M44 scenarios) passed.
- Maintainer Debug play-test accepted: both teachers and prices, repeat lessons,
  Map and automap, travel and reload. Mapping without Cartography could not be
  observed because a starting character already has it; the maintainer judged
  the test sufficient.

## Deferred

Vern's guild membership and spell purchase (steps 5-6), Wizard Eye, effects of
the other skills, and movement abilities (Swimming, Mountaineer, Walk on Water).
