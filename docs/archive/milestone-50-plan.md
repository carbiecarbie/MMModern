# Milestone 50 plan - Whole Vertigo

**Tier A. Status: completed and accepted.** Plan `6009ea8`; Part A on `main`
through `2cabd9a`; Part B `ff91d0a`; review fixes `73739e7`.

## Goal

The prepared Journey party explores the whole of Vertigo as defined by the
original resources: it meets the town's actors, triggers its Events, fights
and Shoots indoors and uses the existing services, with no per-cell or
per-address certification. Unsupported operations say "not supported yet"
and change nothing.

## Final scope

- **Whole-map admission (Part A).** The 49-cell list, Vertigo route/manifest
  certification, actor freezes, the Slime-only combat fingerprint
  (`validateAdmittedPoisonCombat`) and slot-specific accounting were removed
  and replaced by structural, resource-identity and owner checks. Indoor
  geometry is sampled through the original neighbor tiles (Vertigo is map 28
  built from `maze0028`, `mazex109-111`). All 46 monsters (Doom Bug, Slime,
  Breeder Slime) are active under the M49 combat rules.
- **Generic Events.** Same-map Events run through the detached Event
  candidate and guarded publication already used for map transitions;
  dispatch is by capability, not address. Unsupported effect chains are
  refused before any change. Each service continuation (Smith, Training,
  Temple) has a single owner. New opcode: 27, small sign text. NPC talk in
  mode 1 works anywhere.
- **Traversal.** Original Bash (`Interface::bash`) and grate/door unlocking
  and toggling (`Scripts::openGrate`): WhoWill selection, 1-in-4 trap with
  generic typed damage (`giveCharDamage`: seven damage types, saving throws,
  resistances, shield), Thievery + d20, XP, two-sided wall and surface flag
  changes. Map difficulties come from the primary map (`mazeData()`), walls
  from the physical tile.
- **Indoor ranged (Part B).** Player Shoot on every indoor map (one-minute
  charge, reference target groups and wall checks, including opened walls)
  and supported enemy ranged attacks, both on the M49 impact protocol.
- **Still "not supported yet":** GiveMulti treasure (including chests),
  MoveObj, fountains, the mirror, trap teleports, SetChar, Thievery Event
  checks, Bank/Guild/Tavern services, indoor Run.

## Decisions

1. **Save v5.** Adds World-owned wall/surface-flag overrides, original actor
   counts and script-slot types (`spawnedType`); v4 saves are rejected.
2. **Spawn animation frame** uses the separate presentation RNG (as decided
   in M48), so gameplay RNG is unchanged; gap slots 46-49 stay unresolved.
3. **Barrier pauses** follow the reference clock: trap damage holds five
   ticks before the unlock roll; Bash waits two ticks.
4. **M44 digests.** Complete traces of all three routes were identical
   before and after; the digests changed only by save-v5 format bytes (2, 94
   and 106 bytes), every byte explained and approved.
5. **Work split.** Part A ran on a working branch with tested checkpoint
   commits; the maintainer's authorizations let guards be generalized from
   address-specific to capability-based, never deleted without an equivalent
   check, with no state published around the candidate.

## Results

- Data-driven tests over all 1,024 cells and four facings, tile seams, all
  847 Event records, 143 objects and 46 actors; typed damage, Thievery, Bash
  and unlocking rules; indoor ranged rays, lanes and impact order; service
  single-consumer and candidate-tampering controls; save v5 round-trips.
- Full CTest 159/159 after Part A, Part B and the review fixes (single job,
  lightweight runner); fast suite 127/127.
- Independent implementation review (REVISE) found barriers reading the
  physical tile's difficulties (zero outside `maze0028`), missing barrier
  pauses and a discarded Spawn cosmetic RNG advance; all fixed, with an
  audit of every map-difficulty read.
- Maintainer play-tests passed for Part A (whole town, monsters, Bash,
  unlocking, Tavern NPCs, services, exit/re-entry, save/reload) and Part B
  (indoor Shoot) after the fixes. A frame-rate stutter while moving the
  mouse during movement was found to exist before M50 and is tracked
  separately.
