# Milestone 111 plan - Xeen Slayer selector and whole-Event discard

**Tier A. Status: completed and accepted by the maintainer (2026-11-04).**
Plan `a8edc40`, Part 0 `68fbd78`, Part A `1673d03`, Part B `bc467ee`. These
are the two Tier A items the [roadmap](roadmap.md#darkside-arc) kept waiting
for a fitting milestone; the maintainer chose them on 2026-11-03. Save v26
and Journey schema 9 / content 14 are unchanged, because nothing in either
Part is saved. The M44 digests did not change.

## Goal and scope

1. **Xeen Slayer Sword and physical spells.** In DOS, the attack weapon
   selector DS `0x96ee` keeps the last weapon attack's ID. After a Xeen Slayer
   Sword (ID 34) attack, later physical spells also ignore physical
   resistance. These are Flying Fist, Pain, Shrapmetal, Deadly Swarm, Dancing
   Sword and Prismatic Light's type 0. MMModern used to reduce them.
2. **A refused Event leaves nothing behind.** DOS never stops in the middle of
   an Event. When MMModern has to stop one, the whole Event is discarded, as
   the maintainer decided on 2026-10-31 ([approved deviation](original-quirks.md#dev-event-presentation-abandon)).
   This covers an unsupported instruction, an unavailable destination, a door
   or grate, or a presentation that keeps failing. Party, items, flags,
   position, time and the Journey random state return to their pre-Event
   values. No Event in the original data reaches this today (sweep of
   2026-11-02); the rule is a safety net for unsupported content.

## Key decisions

- **Part 0 evidence** ([evidence](rules/evidence.md#m111-part-0), calibrated
  DOS static, high confidence):
  - The weapon helper's clear (`0x2dfff`) and assignment (`0x2e0ee`) are the
    only gameplay writers of DS `0x96ee`. Process start zeroes it; New Game,
    Load and combat do not.
  - A Shoot with no eligible shooter, and a Shoot or Poison Volley with no
    target, keep the previous ID, 34 included.
  - Every physical cast writes 0 to the effect selector DS `0x972e`, so only
    the exact-ID test can bypass resistance for a spell.
- **One rule for melee and spells.** `XeenConsequenceDraw` has a reader
  paired with `weaponSelected`, and every construction site must bind it.
  `XeenSpellDamageCandidate` reads the selector when the damage is applied,
  so spell targets built ahead of time see the current value.
  `XeenProgramState::attackWeapon` stays process-lifetime state and is never
  saved.
- **Discard without a second rollback path.** In Journey play nothing an Event
  does reaches the live owners before it completes:
  - automatic sequence prefixes and NPC animation draws stay in the detached
    candidate;
  - terrain death and damage feedback are applied only at final publication;
  - a refusal only drops the candidate.

  The prefix publication path, `ObservedPrefix` and the prefix light
  projection were removed. A permanent logic error while composing the
  destination is treated as a presentation defect, but only after both the
  candidate and live owner guards pass.
- **Stages kept**:
  - the step, arrival, Wait or Space that triggered the Event, with any input
    it consumed;
  - terminal services, which publish the whole candidate first;
  - the ending and its forced save;
  - sounds and portraits already played.

  The inventory found no map change published early and no combat followed by
  more script in the same Event, so the combat decision was not needed.

## Results

- Original-data and unit tests cover the Slayer cases:
  - the bypass after a Slayer melee, in combat and out of combat;
  - reduction after unarmed melee, another weapon or a delivered Shoot;
  - empty Shoot and no-target Poison Volley;
  - multi-target Pain;
  - 100% resistance;
  - Clouds Lord Xeen;
  - Save and Load.
- Synthetic Events grant and then refuse in manual and automatic form:
  - refusals: execution error, unavailable map, door or grate, permanent
    presentation failure;
  - waits: NPC, numeric and WhoWill;
  - reward receipt failure;
  - a post-combat caller;
  - cross-side travel;
  - later fall images;
  - a refusal repeated in the same session.

  Afterwards the live owners equal the pre-Event state, the Event repeats
  identically, and a save made after the refusal round-trips exactly.
- Both Part implementations were reviewed independently, and their findings
  were fixed. The last complete suite passed 416 of 416.
- **No maintainer play-test item.** The planned check, Pain after a Slayer
  attack, needs a visible damage number, and the game shows none for spells.
  The maintainer dropped it on 2026-11-04 and accepted the milestone on the
  Part 0 evidence and the tests. Part B is unreachable with original data.
