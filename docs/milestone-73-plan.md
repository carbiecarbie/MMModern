# Milestone 73 plan - Monster control and first spell expansion

**Tier A. Status: completed and accepted by the maintainer (2026-10-17)**
after independent plan and implementation reviews and a maintainer
play-test. Plan `d58d097`; implementation `4792a3d` (Part A) and `972256b`
(Part B), review fixes `6724171`, play-test fix `2f34569`. First milestone of
step 6 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems).

## Scope

A monster status system and the eight spells that need it or share its
delivery: Sleep, Beast Master, Hypnotize and Dragon Sleep (control), Insect
Spray, Turn Undead, Sparks and Toxic Cloud (group damage), as learned spells
in exploration and combat, in Quick Fight and from charged items. The other
57 spells are split into M74 (monster damage, Tier B), M75 (party healing and
conditions, Tier B) and M76 (buffs, information, item and travel, Tier A).

Out of scope: monster paralysis, fear or confusion (Xeen has none), new map
admission, combat inventory, audio.

## Key decisions

- **One status per monster**, overwritten on success: Physical, Sleep, Beast
  Master, Hypnotize or Dragon Sleep. Controlled monsters keep their cell,
  visibility and targeting, keep combat engaged, but neither move nor attack,
  and are 20 easier to hit. Sleep and Dragon Sleep wake on nonzero damage
  before physical resistance and the save; Beast Master and Hypnotize survive
  hits and hold the hit pose until they recover.
- **Recovery** (DOS `0x20ca9..0x20cf1`, high confidence): every original slot
  of the map, any lifecycle or position, before the first queued shot
  (`0x20167` before `0x20171`), on `R(1, ID + 50) <= ID`; the pin reverses
  the comparison. Dragon Sleep never recovers; damage or a map load clears it.
- **Save v14 kept** (maintainer decision): DOS clears status on every map and
  game load, so the saved byte stays Physical and the format and its meaning
  do not change. Map reload and Spawn clear every slot.
- **Dragon Sleep accepted by tests only** (maintainer decision): no playable
  map has a dragon; a fixture with Fire Dragon (68) statistics proves it.
- **Item powers**: the M61 table misrouted several powers and stopped at 30;
  it now holds the original mapping for all 73 (DOS dispatch `0x39e4b`).

## Results

- Cast order as in the pin and DOS: magic-resistance gate, species gate
  (Animal or Humanoid; Dragon for Dragon Sleep), then the control save
  (status on `> ID`). Insect Spray and Turn Undead deal 25 to insects or
  undead and draw a save that never halves; Sparks (2 x level) and Toxic
  Cloud (10 poison) halve on the ordinary save.
- Pending item Use keeps its Journey publication when the monsters' response
  is staged (found while testing items; covered by a pre-M73 regression test).
- Play-test fix, a defect since M63: a monster treasure receipt with an item
  closed the game ("Stale monster treasure hold") because drawing the receipt
  renewed the Encounter ticket. The hold now follows the reward lease, item
  cursor and phase. Original-data tests now cover item drops from melee and
  from Sparks killing three Orcs, closing the gap left by M71.
- Developer test save `monster-control`
  ([development guide](development.md#developer-test-saves)) starts in
  Vertigo with the eight spells, 1,000 SP per caster and 1,000 gems.

## Validation

- Complete `build-rel` suite for each production change, unsandboxed:
  224/224 (Part A), 225/225 (Part B), 225/225 with one hang rerun (treasure
  fix; the `xeen_save_file` cleanup hang was fixed separately in `2cb6e73`).
- Independent reviews: plan (approve with changes, applied), implementation
  (accept with fixes: two regression tests and one rules sentence, applied).
- Maintainer play-test: Guild catalogue and costs, Insect Spray and Sleep on
  Doom Bugs, control, recovery and waking on mainland Orcs, Turn Undead,
  group damage, Dragon Sleep refusal, save/load and map re-entry clearing
  statuses, and treasure receipts after the fix.
