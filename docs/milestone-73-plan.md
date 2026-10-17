# Milestone 73 plan - Monster control and first spell expansion

**Tier A. Status: approved by the maintainer (2026-10-17); implementation not
started.**
Baseline: `main` at `e104f7c8dc5e1258b66f9e491c8f4f02e9bebd2b` (v0.4.0, save v14).

## Goal and scope

Make monster control usable through Cast, Quick Fight and exploration item Use on shared actors.
This is [roadmap](roadmap.md) step 6's first milestone: eight of the remaining 65 spells.
No new map admission, combat inventory, recruitment, audio or travel system.

Authority: [dependencies](dependencies.md), clean ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`;
[M60](milestone-60-plan.md), [magic rules](rules/magic.md) and [original quirks](original-quirks.md).
Reference functions below are in `engines/mm/xeen/` at that pin.

Xeen has no numbered spell levels or minimum caster-level gate; "none" reflects that.
`L = max(permanent + temporary caster level, 0)`; C = Clerical (Cleric/Paladin),
W = Wizardry (Sorcerer/Archer), D = Druidic (Druid/Ranger). Costs do not change for hybrids.

| Spell (global ID) | Original level | Book | SP | Gems | Cast context | Effect/range |
| --- | --- | --- | ---: | ---: | --- | --- |
| Sleep (61) | none | W, D | 3 | 1 | Both | Sleep; first group |
| Beast Master (2) | none | C, D | 5 | 2 | Both | Beast Master; first group |
| Hypnotize (33) | none | C | 15 | 4 | Both | Hypnotize; first group |
| Dragon Sleep (17) | none | W | 10 | 4 | Both | Dragon Sleep; single |
| Insect Spray (38) | none | W, D | 5 | 1 | Both | 25 to insects; first group |
| Turn Undead (73) | none | C, D | 5 | 2 | Both | 25 to undead; first group |
| Sparks (62) | none | C, D | L | 1 | Both | 2L electrical damage; first group |
| Toxic Cloud (72) | none | W, D | 4 | 1 | Both | 10 poison damage; first group |

Books/costs: `XeenLearnedSpellRules`' pinned tables; contexts: `Spells::castSpell` and the eight handlers.
Both means exploration and combat, not guaranteed success against any species.
The four extra spells reuse dispatch, group delivery and damage rules; no new dialogs or party effects.
Item powers 7/17/31/40 and 12/19/10/13 respectively share these effects through M61's Use path.
[Coverage](coverage.md) records Sleep, Beast Master and Toxic Cloud on map 53,
Hypnotize and Sparks on 79; the other three have no explicit map references.
These references do not establish casting frequency or fully unblocked maps.

## Design decisions

1. **One status, four values.** Extend `XeenActorStatus` with Sleep, Beast Master,
   Hypnotize and Dragon Sleep beside Physical/none; keep invalid/unsupported data
   distinct. One status, success overwrites it; no monster paralysis/fear/confusion or party cures in M73.
2. **Application.** `Combat::attack`: Sleep, Beast Master and Hypnotize accept
   Animal or Humanoid; Dragon Sleep accepts Dragon. All four prepare zero damage
   and use `POW_MAGIC_ORB`, FX18. No HP loss, allegiance change or friendly attacks.
   `rangedAttack` selects the first reachable group (up to three), or first target
   for Dragon Sleep, using current selected-target order and wall limits.
3. **Movement, turns and targets.** `moveMonster`, `moveMonsters`, `doMonsterTurn`
   prohibit movement, ranged selection and melee for nonzero status; keep occupancy,
   visibility and targeting; a controlled monster in a front slot still keeps combat engaged.
   `hitMonster` adds 20 after the exploding d20 sequence, not automatic hits or bonus damage.
   Preserve the turn scheduler: skip the disabled attack without target/damage draws.
4. **Damage and presentation.** `attack2` clears Sleep/Dragon Sleep upon entry with
   nonzero damage, before physical resistance and the save, even if final damage
   becomes zero. A miss, failed magic gate or zero entry damage does not wake it;
   Beast Master/Hypnotize survive hits. Apply this to melee, Shoot and old spells
   too. `InterfaceScene::animate3d` skips the whole frame block for non-Physical
   monsters, including frame 11: hit Beast Master/Hypnotize monsters hold their
   hit pose until recovery; spell-impact effects keep animating. Keep HP name colors.
   `spells.cpp`: Insect Spray/Toxic Cloud use `POW_SPRAY`, FX17; Sparks uses
   `POW_SPARKS`, FX14; Turn Undead uses `POW_ENERGY_BLAST`, FX18 (audio remains deferred).
5. **Expiry.** No duration counter. `moveMonsters` calls `monsterOvercome` after
   both movement passes/ranged selection, before queued ranged damage. Scan the
   current map's original monster slots in order, including offscreen slots and
   retained statuses on inactive/dead slots; do not restrict recovery to contacts.
   Sleep/Beast Master/Hypnotize recover on `U(1,I+50) <= I` (resource ID `I`).
   This is the documented DOS correction, not the pin's reversed comparison.
   Dragon Sleep never draws recovery; damage or map reload clears it. Disabled
   movement or characters shooting skips the whole opportunity, including recovery.
   Use the same opportunity in exploration, combat rounds, item response and Rest.
   `reloadMapMonsters` clears status on every slot before the arrival view, whatever
   its lifecycle; `xeenRefillLoadedMonster`'s non-Present early return must not hide
   this reset (DOS zeroes the whole array). Spawn also resets status (`cmdSpawn`).
6. **Ownership.** World/session actors own the status. Combat owns cast/turn
   continuations; regional opportunities own recovery, actor approach owns movement.
   Party/roster retains books/costs, Event scripts, Flow UI/publication. Extend detached
   candidates and mutation guards; no new owner, coordinator, RNG or timer.
7. **Save v14 retained (maintainer decision).** The saved byte stays Physical;
   the format and its meaning are unchanged, so the save version stays at 14.
   Retain schema 9/content 14. Validate live status values, then canonicalize
   supported saved statuses to Physical; no new durable field or compatibility reader.
   Restore canonical state/RNG without replaying effects/time/rewards/draws; DOS rebuilds status/HP on Load.
   Keep quiet save boundaries and combat-save refusal; activation/asleep is distinct from spell Sleep.

## RNG contract

Use the existing bounded `XeenConsequenceDraw`/Journey RNG and resumable candidates.
For each target in `rangedAttack` order, `Combat::attack` first draws `U(1,100+L)`
only if magic resistance R is nonzero; `< R` fails, equality passes. Then check
species. A compatible control target draws `U(1,50+I)` and gains status only on
`> I` (`monsterSavingThrow`, or Sleep's inline equivalent). No damage-stage draw.
Insect Spray/Turn Undead use the same gate, then `attack2`'s save on eligible
25-damage targets; consume it but do not halve. Wrong-type Insect Spray skips
`attack2`; wrong-type Turn Undead enters it with zero and draws no damage save.
Sparks/Toxic Cloud use M60's ordinary damage save, halving on `<= I`; elemental
resistance contributes zero as DOS establishes. Zero-level Sparks gates but
skips the damage save. No M73 spell has a random base-damage or duration draw.
Finish each target's death/treasure settlement before the next target. Recovery follows
movement selection; recovered monsters cannot retroactively move or join the ranged queue.
Recovery is the first resumable stage of `XeenRegionalOpportunityCandidate::service`,
before the first queued shot; `XeenActorApproach::move` stays pure.
Subsequent melee uses recovered state; each eligible slot draws once, Physical/Dragon Sleep never draw.
No extra hit, initiative, target or animation draws; retain existing combat order.

## Risks and unresolved evidence

Physical-only guard audit (baseline line numbers): ranged selection stays
(`XeenRegionalRules.cpp:281,304`); movement require becomes a skip
(`XeenActorApproach.cpp:145-146`). Contacts (`XeenCombat.cpp:223,329,573`), spell
targets (`:901`), kill progression (`XeenJourneyProgression.h:20`) and validators
(`XeenRegionalRules.cpp:244`, `XeenVertigoWorld.cpp:168`) accept controlled monsters.
Bounded read-only DOS check (2026-10-17), same SHA-256 as the cited quirks:
`WORLD/XEEN.DAT 0x20ca9..0x20cf1` scans the MOB count, testing only status 0/13;
no HP, lifecycle or position filter. Calibrated against known status +7, species
+6 and count `0x9c59`; `0x20167` calls recovery before `0x20171` dispatches the
ranged queue (`0x3694f`, 36 entries at `0x36958..0x369a6`). High-confidence static evidence.
Maintainer decisions retain v14 and accept Dragon Sleep by tests only. Remaining
DOS-play checks: controlled hit-pose persistence and waking on damage; contradictory
observations stop work for resolution. No other inspected M73 branch needs a decision.
Implementation records oddities in original-quirks, with an anchor/ScummVM tag/differences item for recovery.

## Acceptance

- Unit tests cover every status/type gate, overwrite, recovery boundary/order,
  Dragon Sleep exemption, attack skip, +20 hit chance, targeting, frame-11 freeze,
  live impact animation, all-lifecycle map reset, Spawn reset and wake-before-reduction.
  Draw tapes cover resistance equality, zero level, wrong species, offscreen/dead slots and yields.
- Test all eight spell costs/formulas, Cast/Quick Fight parity, exploration/item
  delivery, charges and time/turn settlement. Unsupported spells say "not supported yet"
  with no SP, gems, time, turn, charge or RNG expenditure.
- A few original-data end-to-end scenarios: Vertigo Guild/Doom Bugs and mainland
  Orc control/group damage. A test verifies Doom Bug (2, Insect), Orc (6, Humanoid),
  Skeleton (8) and Zombie (9, both Undead) types against `xeen.mon`.
  Dragon Sleep is accepted by tests only: inject Fire Dragon (68) statistics into
  a fixture to prove success, recovery exemption and waking by damage.
- Generic save/load round-trip proves canonical status reset, unchanged RNG/time,
  byte-identical resave; an existing v14 save loads, and a save taken while statuses
  are live is byte-identical to one taken without them (all other state equal).
- Complete CTest passes on `build-rel`; regenerate coverage when support changes.
- Maintainer: buy Sleep/Insect Spray/Sparks/Toxic Cloud at Vertigo's Guild; cast
  Spray on Doom Bugs, compare Sleep's incompatible target result, and check costs.
  A prepared learned-book fixture adds Beast Master, Hypnotize, Turn Undead and
  Dragon Sleep, which that Guild does not sell. Leave Vertigo for F3 (map 23),
  face an Orc and cast Sleep: check frozen movement, skipped attacks, recovery and
  waking on damage. With Beast Master/Hypnotize, check that damage leaves the
  status and hit pose until recovery. Cast Turn Undead on Skeleton (8)/Zombie (9)
  on map 23 and compare an Orc. Dragon Sleep's play-test checks only its refusal
  by F3 monsters (incompatible species, not an unsupported-spell refusal).
  At a quiet boundary save/load and verify statuses clear; leave/re-enter too.
