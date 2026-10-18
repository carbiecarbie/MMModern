# Milestone 70 - World Events (Clouds)

**Tier A, a foundation and three parts. Status: completed and accepted by the
maintainer (2026-10-16)** after independent plan and implementation reviews
and maintainer play-tests. Commits: foundation `bf49eb3`, Part A `a590e30`,
Part B `c8f1a19`, Part C (this closure's parent). Baseline `3b4dab9` (v0.2.0).
Step 4 of the [systems arc](../roadmap.md#next-arc-finishing-the-games-systems).

## Scope

Every Event operation the **Clouds** scripts use, other than audio, town
services and the ending, runs as in the original, and map changes persist
through M69's per-map state. Darkside-only operations and Darkside durable
state (per-character flags, Darkside quest and game flag banks) wait for a
later Darkside arc (maintainer decision, 2026-10-15); shared operations are
generic. Out of scope: audio (`PlayFX`, `PlayEventVoc`, `PlayCD` keep their
timing), `DoTownEvent` services (arc steps 5 and 8), the Clouds ending and its
ConfirmWord codes, and admitting maps (step 9).

Authority: calibrated static analysis of `WORLD/XEEN.DAT` (dispatch
`0x26c50..0x26c69`, table `0x29320`), maintainer DOSBox evidence, and pinned
ScummVM `6814ee9b` (`scripts.cpp`) where DOS was not checked. Rules and
offsets are in [world Events](../rules/world-events.md); kept oddities are in
[original quirks](../original-quirks.md).

## Results

- **Foundation.** Durable-state inventory (no save change needed), live map
  owners for Event opcodes and wall items, a shared transition result whose
  refusal leaks no map, party or RNG change, and the `--event-fixture`
  developer entry (any Clouds cell, no saving).
- **Part A - map changes and Spawn.** MoveWallObj, MakeNothingHere (clears the
  physical cell and continues, as DOS), AlterMap, AlterCellFlag, AlterEvent
  and generic Spawn outside Vertigo, with M69's lifetimes. A Spawn slot beyond
  the map's monster count stays inert, as DOS (maintainer, 2026-10-15). A
  hidden fourth ranged monster on the party's cell fires, as DOS.
- **Part B - flow, conditions, rewards and draws.** If1/If2/If3 with the
  original widths and member selection (mode 37 reads temporary Might),
  IfMapFlag, ItemType, SetVar through the original setValue, every Clouds
  TakeOrGive/GiveMulti/SetChar/GiveEnchanted/NPC mode, skill grants, and
  JumpRnd, RndDamage and SelectRandomChar drawing once at the original place.
  Gold Veins follow the original tiers, checked against a CD recording.
- **Part C - travel and typed input.** TeleportAndExit and TeleportAndContinue
  for direct, mirror and run-location destinations; same-map travel skips the
  reload; Continue resets the script context and shows the destination before
  its Events. ConfirmWord uses the original window, mirror table, passwords
  and titles, keyboard and mouse; the Xeen Slayer Sword path works. Amended by
  the maintainer (2026-10-16): an alternate-only match takes ordinary success,
  mirror lookup is bounded by the real records, and missing text aliases
  text[0], as DOS.

## Key decisions

- DOS over the reference: no map-82 Might workaround, no Dragon Tower mirror
  correction, MakeNothingHere continues, no Vowelless Knight answer rewrite.
- Save v13, schema 9, content 14 unchanged; condition-counter edge cases follow
  the repository's ScummVM fallback policy.
- Destinations on maps not admitted refuse through the shared transition
  result; most mirror names therefore refuse until step 9 opens areas.
- makeItem's 1..100 category draws differ from the reference; left for a
  separate maintenance task (recorded in original quirks).

## Validation

- Census: zero unresolved in-scope Clouds Event rows (14919 of 15919
  instructions supported); the rest are audio, services, the ending,
  destination admission and 13 listed census limits.
- M44 gate: Part A replaced the temple and roundtrip digests with approval
  (Spawn and ranged fire as DOS); Parts B and C showed zero differences.
- Complete `build-rel` suite through the runner after each part (Part C
  223/223); exact v13 round trips with no replayed Event, reward, time or draw.
- Maintainer play-tests: Spawn, wall and map changes, rewards, random Events
  and Gold Veins through the fixture; the Vertigo mirror, a password, the jail
  teleport and save/load in Part C.
