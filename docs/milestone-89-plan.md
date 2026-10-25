# Milestone 89 plan - Clouds scripted kills and map rules

**Tier B. Status: planned (not started).** Roadmap
[step 15](roadmap.md#next-arc-finishing-the-games-systems), after
[M88](milestone-88-plan.md). Save v20, schema 9 and content 14 unchanged.
The maintainer pre-authorized one independent implementation review
(2026-10-24); work stops if an M44 digest changes.

## Goal

Reproduce the Clouds rules that live in the original program rather than in
the Events, so the main quest reaches the Sixth Mirror and three side quests
can finish: Lord Xeen's fight and death reward, the three scripted death
hooks, the Ice Troll regeneration and the map load and leave rules. The
ending itself (`CutsceneEndClouds`) stays "not supported yet" until step 18.
Evidence: the completion audit's DOS static findings
(`D:/MModern-re/clouds-completion/findings.md` §3-4, high confidence) and the
maintainer's DOSBox run of the ending (`dos-ending-observations.md` there).

## Tasks

### Part A - Lord Xeen and the death hooks

- [ ] **Slayer Sword bypass.** Physical resistance is skipped only for
  attack weapon ID == 34 (`0x2beb6`), not `>= 34` as the pin and the current
  code do (`XeenCombatRules.cpp` player attack); with a nonzero range
  argument DOS also skips it when `[0x972e]` is nonzero (`0x2beaf`): find
  what that word is before deciding whether it is reachable. Quirk row and
  ScummVM item.
- [ ] **Lord Xeen's damage** (species 89, Clouds side, `0x2be01..0x2be76`):
  nonzero damage with a nonzero range argument becomes zero (Shoot passes
  3, `0x2cbb0`; spells and Poison Volley already refuse); otherwise +100 to
  nonzero damage, unconditionally (the intended `!weapon == 34` test never
  holds), before physical resistance. Confirm where effectiveness is applied
  relative to the +100. Quirk row (original bug kept).
- [ ] **Death hooks** (`0x2c191..0x2c237`, after HP reaches zero, before XP
  and treasure), keyed by map and MOB slot (`XeenMonsterIdentity::recordIndex`):
  map 41 slot 28 (Clan King) sets game flag 11; map 78 slot 8 (Lord Xeen)
  sets flag 60, clears quest flag 23 and gives award 42 to every active
  member (confirm whether dead members get it); map 29 slot 27 (Count Draco)
  sets flag 104. One shared rule for the three death paths: melee
  (`XeenCombat::serviceConsequences`), spells and Poison Volley
  (`XeenCombat::serviceCast`) and Shoot outside combat
  (`XeenJourneyConsequences.cpp`), published atomically with the XP. Use the
  existing flag, quest flag and award setters (`XeenEventValues.cpp`).
  ScummVM item: the pin uses slot 20 on map 41.
- [ ] **Scepter** (`0x2c30b..0x2c337`, outside map 85, Clouds, species 89):
  treasure weapon slot 0 becomes {material 0, ID 90, state 0} without an
  empty-slot test (so it can overwrite Lord Xeen's own drop; confirm the
  order against the drop), hasItems is set and quest item 8 is incremented.
  The shared staging (`XeenTreasureStaging`) and the ordinary treasure
  screen then deliver it, to the first member even when dead, as the DOSBox
  run shows. Quirk row for the overwrite.
- [ ] **Ice Troll** (species 53, Clouds side, `0x2ea4a..0x2ea85`): at every
  new combat round each one's HP returns to its maximum (maps 8 and 63), in
  the round branch of `XeenCombat::serviceConsequences`. Confirm the exact
  point in the round and the side test.
- [ ] **Sixth Mirror.** Today the whole Event at map 78 (4,9) W is refused
  by the Event preflight, because its line 7 (`CutsceneEndClouds`, opcode
  30) does not decode; even the force-field text never shows. Decode opcode
  30 as a known operation that ends the Event with "Clouds ending: not
  supported yet", so lines 0-6 run as in DOS and the game keeps running.
  Step 18 replaces the notice with the ending.
- [ ] Update [combat rules](rules/combat.md#lord-xeen-and-scripted-death-hooks)
  (evidence to DOS static; map-41 index) and the applying-damage section.

### Part B - Map load and leave rules

Every rule applies on each main-map load (travel through
`XeenWorld::loadEventDestination`, Load and New Game) or on leaving
(`XeenWorld::leaveMap`). The object and sprite rules derive from saved flags
at load or draw time; they add no saved state (`XeenObjectOverride` has no
sprite field).

- [ ] **Map 15 Water Dragons** (MOB slots 0-2): leaving map 15 with all three
  dead sets flag 56 (`0x26558..0x26590`); loading map 15 as the main map with
  all three dead clears it (`0xfc7f..0xfcbd`). ScummVM item (the pin sets on
  load and on save).
- [ ] **Flag 25 cave rule**: with flag 25 set, clear the low three cell bits
  on maps 42, 43 and 44 at load (`0xf80b..0xf859`). ScummVM item (the pin
  has map 4).
- [ ] **Quest-done objects**: the nine-entry table (`0xfd1f..0xfe63`) moves
  map objects to (-128,-128) at load when their flag is set. ScummVM item.
- [ ] **Map 12 castle sprite**: object sprite 118 becomes 85 once flag 43 is
  set (`0xff50..0xff94`).
- [ ] **Map 50 stepped map**: every stepped bit set at load
  (`0xf7f4..0xf808`); confirm the operand is the map number. ScummVM item
  (the pin applies it to Darkside only).

### Both parts

- [ ] Tests: a unit test per rule; original-data tests that kill Lord Xeen
  with the Xeen Slayer Sword on map 78 (Scepter staged and delivered, quest
  item 8, award 42, flags 60 and 23, the Sixth Mirror text and then the
  "not supported yet" ending notice), kill the Clan King and reach Gunther's
  completion text, and apply the map-15 rule; the generic exact-restore test
  unchanged. Regenerate the coverage census (the Sixth Mirror row changes).
- [ ] Play-test presets in `tools/XeenTestSave.cpp`, written to new paths:
  `m89-lord-xeen` (map 78 (5,8) N, a strong party with the sword),
  `m89-clan-king` (map 41), `m89-draco` (map 29), `m89-dragons` (map 15),
  `m89-trolls` (map 8). Rebuild `mmodern_test_save` before generating them.
- [ ] Docs: quirk rows and ScummVM items in the same change; project status,
  history and roadmap at closure.

## Rules for the implementer

- Re-read each cited DOS range before implementing it; the audit's offsets
  are the authority, the pin is comparison only. Stop if a range disagrees
  with the audit.
- Stop and report if a task needs a saved field, changes an RNG draw or its
  order, changes combat entry or exit, or changes an M44 digest.
- Use the existing owners: combat rules and flow for the fight and death,
  the shared treasure staging (M81) for the Scepter, the Event-side flag,
  quest and award setters, and map load for Part B. Add no new owner.
- Part A and Part B may be one task or two commits. The pre-authorized
  independent review covers the whole implementation before push; reuse its
  session for fix rounds.

## Acceptance

All tasks done; affected tests and `ctest -L fast` while iterating; one
complete `build-rel` suite passing as [tools/AGENTS.md](../tools/AGENTS.md)
requires; M44 digests unchanged; the independent implementation review
approved. The maintainer play-tests the five presets (Lord Xeen to the Sixth
Mirror, Gunther after the Clan King, the Nightshadow well after Draco, Medin
after the dragons, the trolls) and accepts the milestone.
