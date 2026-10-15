# Milestone 68 - Monster abilities

**Tier A, in two parts. Status: approved after the independent plan review;
implementation not started.**

## Goal and boundary

Step 2 of the [systems arc](roadmap.md#next-arc-finishing-the-games-systems):
every monster attack, secondary effect and movement kind the Clouds and
Darkside data use works as in the original. Today [the coverage report](coverage.md)
lists 33 unsupported Clouds and 35 Darkside monster rows: flying movement
(32 Clouds maps), nonphysical and ranged elemental attacks, and secondary
effects other than Poison, Disease and Sleep.

Tier A: monster damage, movement, approach and random draws change, and the
M44 scenarios may change. Independent plan (done, 2026-10-15) and
implementation reviews are mandatory. No map is admitted and Darkside gameplay
stays closed. The monster status system (sleeping, paralyzed or controlled
monsters) belongs to the spells step: these attacks affect characters, and an
elemental attacker keeps Physical actor status.

Authority: calibrated static analysis of `WORLD/XEEN.DAT` (SHA-256 `8bf78b48...`,
file offsets below, from the plan review), the [combat rules](rules/combat.md),
and pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd` where DOS was not
checked. DOS overrides the reference; every difference goes to the rules and
[original quirks](original-quirks.md) in the same change.

## Part A - attacks, secondary effects and consequences

1. **Monster damage, DOS sequence** (`0x2d22d`): clear Sleep, roll the damage
   dice; for a nonphysical attack the initial save uses the **Physical**
   selector (`0x2d2e4..0x2d2f7`), not the element, halving on success; subtract
   party protection (zero until its spells exist); the element save loop then
   draws once unconditionally on entry, even at zero damage (`0x2d35e..0x2d374`),
   halving and repeating while it succeeds and damage stays nonzero; then Power
   Shield (zero for now) and the zero clamp; then the secondary save only for
   positive damage and a nonzero special. This corrects the pinned
   `doCharDamage` and the current Poison implementation, which already-playable
   Vertigo and F3 monsters use: that change is intended, not a regression.
   `giveCharDamage` stays a separate algorithm.
2. **Secondary effects**, after a failed Physical save: every existing rules
   row, plus:
   - **Special 11** (Harpy, Sand Flower): the save is drawn, no effect follows,
     HP damage applies (DOS table `0x2d6d9`, entry `0x2d6e5`); no "In Love"
     condition is invented.
   - **Break Weapon**: DOS-confirmed unequip (`0x2d52c..0x2d532`).
   - **Eradicate** (`0x2d5c6..0x2d60e`, helper `0x2bd6a`): increment the
     condition; in all nine slots of every category, set the broken bit;
     clear weapon frames except weapon ID 34; clear armor and accessory
     frames; keep miscellaneous frames and charges; empty slots are written
     too; positive HP becomes zero before the ordinary HP subtraction.
   - **Curse Item** (`0x2d671..0x2d6ab`): all nine slots of all categories
     except weapon ID 34 get the cursed bit; frames stay.
   - **Aging** increments the temporary-age byte with DOS byte behavior
     (`0x2d592..0x2d59b`).
3. **Ranged attacks** of every damage type, in combat and in exploration,
   keeping the existing volley rule: the first non-Magic-Arrow projectile in
   the queued volley is drawn, and each source then applies its own attacks
   and damage type in queue order with the existing staged travel, portrait
   and injury timing.
4. **Full consequence publication.** Combat and exploration currently publish
   only HP, conditions and armor. Carry and publish every changed character
   field (SP, temporary age, all inventory bytes and frames) at the injury
   publication, in the retained expected state used by authority checks, and
   after the operation settles; cover contact, exploration ranged attacks and
   Rest's ranged opportunity. Reuse the current owners.
5. **Legacy medal.** Curse Item keeps its frame-8 arrangement with the cursed
   bit; admit exactly that faithful state in the completed-equipment check,
   without relaxing other arrangement rules.
6. **Developer fixture**, introduced in Part A: an explicit developer CLI
   entry, absent from the title menu, that builds F3 from the original data
   plus an in-memory fixture MOB placing one chosen original species at a
   legal spawn. The same fixture bytes feed parsing, manifest validation,
   reconstruction and renewal, so every existing validation runs. Saving is
   disabled in the fixture; round-trip tests use the same provider. No change
   to commercial files, saves or the save schema.

## Part B - monster movement, including flight and species 59

Reference evidence (`Combat::canMonsterMove`, `combat.cpp:700-755`), checked in
`WORLD/XEEN.DAT` (`0x1fc91`): the surface rule matches; DOS differs on the
threshold, and the maintainer approved following DOS (2026-10-15, see the
[combat rules](rules/combat.md#monster-movement-terrain)).

1. **Outdoors**, for middle values {0,2,3,4,5,6,8,11,13,14} the destination's
   resolved surface decides: Water and Deep Water admit flyers and species 59
   (Clouds Water Dragon, Darkside Vulture Roc); Space admits flyers; other
   surfaces admit every species on Darkside and all but species 59 on Clouds.
   Middle values {1,7,9,10,12} always refuse, flyers too; middle 15 uses the
   strict wall threshold (below no-pass).
2. **Indoors** has no flight rule: the strict wall threshold applies to everyone,
   walkers included.
3. **Shared rules stay**: the two approach passes, actor scan order, facing
   primary and fallback directions, pre-movement ranged scheduling, the shared
   three-actor occupancy cap with no flight layer, contact entry and drawing
   order. The existing float stays the only drawing difference.
4. Remove the ground-only and species-59 refusals from the regional rules,
   approach and contact (`XeenRegionalRules.cpp:147,162`, `XeenActorApproach.cpp:145`,
   `XeenCombat.cpp:561`), keeping status, lifecycle and identity checks.
   Mechanic tests cover both sides without admitting Darkside play.

## Census, RNG and the M44 gate

- Every actual Clouds and Darkside attack tuple and live movement kind becomes
  supported, special 11 included. The one allowed exception is the zero-HP,
  zero-use `dark.mon 0` record, which is not a live actor. Resistance rows are
  classified against the M67 behavior. The census must query the capability
  runtime uses, not just the flying bit. Any other remaining gap amends scope.
- No save-format change: HP/SP, temporary age, conditions and every inventory
  byte already persist, and monster species rebuild from resources; envelope,
  schema and content stay 12/9/14. A needed change stops work for a plan
  amendment.
- Run `tools/m44-digest-audit.ps1 -Step Preserve` before production edits and
  `-Step Compare` after each part. The DOS Poison correction may change the
  digests: explain each difference from its first divergence and ask for
  approval before any digest changes; never keep the old Poison behavior just
  to keep a digest.

## Acceptance

- Deterministic tests for: draw intervals, order and count (nonphysical skips
  the hit roll; Physical natural 1 and 20; sleeping-target bypass and wake;
  the Physical initial save and the zero-damage loop draw); target selection
  per attack including disabled or dead hated targets; fresh dice per
  recipient; secondary save only on positive damage; every secondary effect
  including special 11, Eradicate and Curse Item frames on empty and occupied
  slots, the legacy medal, Aging at the byte limit and M67 contributions right
  after the change; one mixed ranged volley in combat and in exploration;
  walker regressions next to flyers (pass order, terrain versus occupancy
  failure, shared cap, contact, Rest activation, no new movement draws); the
  outdoor terrain matrix on both sides; exact save round trips after each new
  effect, with no replayed effect, time, reward or draw.
- A few gameplay integration tests prove the full consequence publication.
- The census regenerated and checked; complete `build-rel` suite through the
  runner after each part; M44 gate; independent implementation review.
- Maintainer play-test through the fixture: a nonphysical ranged attacker in
  exploration and contact; a visible inventory effect and Drain SP; a Giant
  Bat crossing water and entering contact; the usual F3 and Vertigo route
  unchanged apart from the corrected Poison rule.
