# Milestone 111 plan - Xeen Slayer selector and whole-Event discard

**Tier A. Status: in progress; Part 0 settled (2026-11-03).** The two Tier A
items the [roadmap](roadmap.md#darkside-arc) kept waiting for a fitting
milestone, chosen by the maintainer on 2026-11-03. Amended after the
independent plan review (2026-11-03) and after
[Part 0](rules/evidence.md#m111-part-0) (empty shots keep the selector). Save v26 and Journey schema 9 / content 14 stay
unchanged: nothing in either Part is saved. The M44 digests should not change.

## Goal

Two remaining differences from DOS go away:

1. **Xeen Slayer Sword and physical spells.** In DOS, after a character
   attacks with the Xeen Slayer Sword, the next physical spell (Pain, Flying
   Fist, Shrapmetal, Dancing Sword and other physical damage) also ignores the
   target's physical resistance, because spell delivery leaves the attack
   weapon selector DS `0x96ee` as the last weapon attack set it. MMModern
   already keeps that selector for sound effects but still reduces physical
   spell damage by resistance.
2. **A refused Event leaves nothing behind.** DOS never stops in the middle of
   an Event. When MMModern has to stop one (an instruction or presentation it
   does not support yet, or a presentation that keeps failing), the whole
   Event is discarded, as the maintainer decided on 2026-10-31: the party,
   items, flags, position, time and random state are as they were before the
   Event's first instruction. Today the effects already shown are published
   and the rest is dropped, so a reward can stay without the flag that
   prevents it from repeating. No Event in the original data reaches this
   today (sweep of 2026-11-02); this is a safety rule for unsupported content.

## Evidence already settled

- [Slayer Sword bypass](original-quirks.md#sv-slayer-sword-exact-id): only
  weapon ID 34 bypasses physical resistance (`0x2beab..0x2bf2b`); spell
  delivery `0x2cf94..0x2cf9f` keeps DS `0x96ee`; physical type 0 is prepared
  by Pain (`0x2f5ef`) and Dancing Sword (`0x30a35`). Calibrated DOS static,
  high confidence.
- [Selector writers known so far](original-quirks.md#sv-unarmed-weapon-selector):
  the weapon helper clears DS `0x96ee` at `0x2dfff` and assigns the equipped
  ID at `0x2e0ee`, so unarmed melee leaves 0. The second bypass, nonzero
  range with a nonzero effect selector DS `0x972e`, is cleared by melee and
  Shoot ([combat rules](rules/combat.md)). Neither selector is saved.
- [Part 0](rules/evidence.md#m111-part-0) (calibrated DOS static, high):
  those are the only gameplay writers of DS `0x96ee`; only process start
  zeroes it, not New Game, Load or combat. Shoot and Poison Volley replace it
  (missile ID or 0) only when target delivery reaches the weapon helper; a
  Shoot with no eligible shooter, or a Shoot or Poison Volley with no target,
  keeps the previous ID, including 34. Every physical cast writes effect 0, so
  only the exact-ID test can bypass for it; the six physical routes (Flying
  Fist, Pain, Shrapmetal, Deadly Swarm, Dancing Sword, Prismatic Light's type
  0) match MMModern's list, items included.
- MMModern keeps DS `0x96ee` as `XeenProgramState::attackWeaponFx` (process
  lifetime, app layer), written only through `XeenConsequenceDraw::weaponSelected`,
  which all eight construction sites already bind.
- The pinned reference also bypasses physical resistance for spells after
  ID 34 (with `>=34`); [scummvm-differences](scummvm-differences.md) wrongly
  says MMModern already does.
- Saving is impossible while an Event runs or waits (`XeenEventFlow::canSave`
  requires no `_transition` and no `_pending`); only the Inn's forced save
  bypasses that.
- Travel, TeleportAndContinue and Continue already stay in the Event
  candidate, and a refusal discards them ([world-events](rules/world-events.md));
  only final publication publishes the camera.

## Parts

### Part 0 - DOS check of the selectors (read-only)

Done 2026-11-03: [evidence](rules/evidence.md#m111-part-0); results in
"Evidence already settled" above. Its scope was calibrated static analysis of `WORLD/XEEN.DAT`, recorded in
[evidence](rules/evidence.md) and [combat rules](rules/combat.md):

- Every write to DS `0x96ee`, including program start, new game, load and
  combat start or end. If a writer beyond the two known ones exists, Part A
  reproduces it.
- Every write to DS `0x972e` on the spell path: whether a physical spell
  sets it to 0 or keeps it, so that Poison Volley followed by Pain is settled.
- Every spell and item cast that reaches attack2 with damage type 0, checked
  against MMModern's list in `XeenSpellDamageCandidate` (Shrapmetal, Flying
  Fist, Pain, prepared type 0).
- That nothing between a weapon attack and a later spell's damage writes
  either selector except another weapon attack or Shoot (monster turns,
  Block, Run, Use, multi-target delivery).

If Part 0 contradicts the settled evidence, stop and report before Part A.

### Part A - Physical spells after the Slayer Sword

- `XeenConsequenceDraw` gets a reader paired with `weaponSelected`; every
  construction site binds it to the same program value. The games layer still
  never receives `XeenProgramState`.
- `XeenSpellDamageCandidate` reads the selector at attack2, in the step where
  it reduces physical damage today, and applies `xeenPhysicalAttackDamage`
  (ID exactly 34 bypasses; effect 0, as Part 0 settled), so melee and spells
  share one rule.
- The selector is written only where DOS reaches the weapon helper: melee and
  delivered Shoot and Poison Volley shots. Part A checks that a Shoot with no
  eligible shooter and a Shoot or Poison Volley with no target write nothing
  in MMModern, and fixes any site that does. Spell candidates built ahead of time (combat's
  next targets) therefore see the value current at damage time. No constructor
  argument; the `XeenPhysicalPlayerCandidate` constructor that
  `tests/XeenConsequenceOriginal.cpp` wraps by its mangled name stays as is.
- Rename `attackWeaponFx` to `attackWeapon` (DS `0x96ee`): it is now a rule
  input, not only a sound cue. It stays process-lifetime state, never saved and
  not reset by Load or New Game unless Part 0 finds such a writer; a loaded
  party behaves as the running program left it, as in DOS.
- Part A confirms that item casts reach damage only through the combat and
  journey (`src/app/XeenJourneyConsequences.cpp`) sites and need no third one.
- Docs: rewrite the `sv-slayer-sword-exact-id` implementation column, correct
  the scummvm-differences item, update [combat](rules/combat.md) and
  [magic](rules/magic.md) rules.

### Part B - Whole-Event discard

**Mechanism: nothing an Event does reaches the live owners before it
completes.** The live owners are then the pre-Event state, and a refusal only
drops the candidate; no second rollback path is added.

- Automatic Events keep their sequence prefix detached, as manual Events
  already do (`publishEventSequencePrefix` else-branch, `retainManualPrefix`),
  instead of calling `XeenEventPublication::publishSequencePrefix`. Their NPC
  animation draws go to the candidate, not through `npcAnimationFrame` to the
  live RNG.
- Every refusal path (`XeenEventExecutionError`, unsupported special
  interactions, a refused transition, `abandonDetachedPresentation`) drops the
  candidate without calling `publishManualPrefix`. Notices keep their texts.
- The journey RNG returns with everything else, including draws made only for
  presentation during the Event (NPC animation frames): they belong to the
  discarded Event. This is a refusal rule, not a Load; the save restore rule is
  unaffected.
- `publishPrelude` and `prepareVertigoPrelude` have no caller; Part B removes
  them if the inventory confirms it, along with any publication made unused.

Part B starts with an inventory, in its report, of every stage below with its
owner and code site, and stops to ask if one does not fit this table:

| Stage | Rule | Reason |
| --- | --- | --- |
| The step, arrival, Wait or Space that triggered the Event | Kept | The player's own action, before the Event's first instruction |
| Script effects, travel and Continue, sequence prefix, falls and their damage until the last impact | Discarded | Inside the Event; all already detached or made detached here |
| Post-script reward delivery (items, currency) and its receipt | Discarded on a refusal or permanent receipt failure | Part of the Event; delivery stays staged until the receipt completes |
| Service, Training, Smithy, Tavern, Bank, Guild, Temple suspension | Kept (whole Event published) | Must be terminal; publishes the whole candidate first |
| Ending publication and forced save (opcodes 57/58) | Kept | The Event has completed |
| Inn sign-in forced save inside an Event, if reachable | As the inventory finds | The only save that bypasses `canSave` |
| Combat started inside an Event and followed by more script | Decision below | Not yet inventoried |

Process-lifetime state with a base (Event call skipped, door text) returns to
that base; sounds and portraits already played are not undone, and the notice
replaces the Event's windows as today.

Docs: [`dev-event-presentation-abandon`](original-quirks.md#dev-event-presentation-abandon)
stays an approved deviation (DOS has no failure path) and now discards the
whole Event; the runtime-refusal half (an unsupported instruction) gets the
same treatment in that entry or its own approved-deviation anchor. Update its
scummvm-differences item, [world-events rules](rules/world-events.md) (the
"already published prefix remains authoritative" passages) and the roadmap
paragraph.

## Decision for the maintainer (only if the inventory finds the case)

**A combat the player fought inside an Event that later refuses.** Recommended:
the fight and its results stay, and only the script after it is discarded;
undoing a won fight would be stranger than the refusal. If the inventory finds
no combat followed by more script in the same Event, this decision is not
needed.

## Risks

- The Event publication model is the most delicate code in the project
  (detached candidates, retained guards, manual and automatic sequences).
  Moving automatic prefixes to the detached path must keep their portrait,
  light-word and continuation behavior.
- Unreachable with original data: every Part B case is a synthetic script or
  a fault injection, as `xeen_fall_defeat_original` already does.
- Part A changes damage once a Slayer Sword has struck; it changes no random
  draws (the reduction draws nothing). The Darkside audit invariants and the
  M44 digests must stay green.
- The selector is process-lifetime: test executables that run several
  scenarios must reset it explicitly, or results depend on test order.

## Tests

- **Part A.** Unit tests on `XeenSpellDamageCandidate` with the reader: ID 34
  bypasses; 0 and the 35 boundary do not; non-physical spells unaffected; 100%
  resistance; Clouds Lord Xeen (resource 89) still refused before damage.
  Original-data cases: a Slayer Sword melee round, then Pain on a physically
  resistant monster deals unreduced damage; the same cast after unarmed melee,
  another weapon or a delivered Shoot is reduced; after a Slayer attack, an
  empty Shoot (no eligible shooter, or no target) or a no-target Poison Volley
  leaves Pain unreduced; a delivered Poison Volley then Pain is reduced;
  multi-target Pain through the pre-built candidates; a journey
  (out-of-combat) physical spell after a Slayer fight; Poison Volley's own
  damage unchanged.
- **Part B.** Synthetic Events that grant gold, items, quest items, quest and
  game flags, time and a draw, then refuse (execution error, unsupported
  interaction, refused transition, permanent presentation failure), in
  automatic and manual form where both exist; afterwards the live owners equal
  the pre-Event state, the Event repeats identically, and a save made after the
  refusal round-trips exactly. Add the cases the quirk entry lists as not
  fault-tested: NPC, numeric and WhoWill waits, a failure during the reward
  receipt, combat-interrupted Events, later fall images and cross-side travel.
- Existing tests that assert the old rule change with it:
  `XeenEventFoundationTests.cpp` (`sessionAndRefusal`, the published XP
  prefix), `XeenSaveStateTests.cpp` (late refused published-prefix forms),
  `xeen_fall_defeat_original`, `xeen_reward_execution` and
  `XeenRestOriginalTests.cpp`.
- Complete CTest suite per [tools/AGENTS.md](../tools/AGENTS.md).

## Reviews and acceptance

- Independent review of each Part's implementation (Tier A); this plan was
  reviewed on 2026-11-03.
- One short maintainer play-test after Part B, with a DOSBox comparison save.
  Item: a Slayer Sword attack, then Pain on a monster with 50% physical
  resistance. Pain deals 8, halved on a save, so 8 or 4 means bypass and 4 or 2
  means reduction; seeing 8 settles it. Reason: the tests prove MMModern's
  rule, but only DOS play confirms the static reading in the real program.
  Part B cannot be play-tested with original data; its tests settle it.
