# Milestone 49 plan - Combat rules fidelity

**Tier A. Status: proposed; not approved.** Implementation requires explicit maintainer start.

## Goal and boundaries

Reproduce monster targeting/attack counts, projectile impact publication and
combat rotation through the existing combat authority, for all uses of these
mechanics. Follow [AGENTS.md](../AGENTS.md), [roadmap](roadmap.md) and the
[M48 decisions](milestone-48-plan.md); whole Vertigo admission remains M50.
Do not add spells, monster special abilities, new areas, combat equipment use,
audio, a parallel combat system or per-monster exceptions. Unsupported damage
abilities retain explicit safe refusals; generic targeting is independently testable.

## Verified baseline and reference

- Repository main, HEAD, origin/main and live `git ls-remote origin refs/heads/main`
  were all `88117ac5cbd36deb7dff26b08b42052e552a7df5`; initial tree was clean.
- Reference: `D:/Projetos/MModern/scummvm-known-good-candidate`, clean at
  `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, as pinned in [dependencies](dependencies.md).
- Original installation `F:/Games/gog/Might and Magic 4-5` stays read-only.
  Never commit original records, extracted fixtures or generated save bytes.
- Reference locations below are in `engines/mm/xeen`: `map.cpp:107-112` (record fields), `character.h:62` (hatred constants), `combat.cpp:288`
  (`doCharDamage`), `:570` (`monstersAttack`), `:806` (`doMonsterTurn`), `:1197` (`attack`), `:1397` (`attack2`), `:1834` (`rangedAttack`),
  `interface.cpp:1811-1826` (combat rotation).

## 1. Monster attacks first

- Root cause: `src/games/xeen/XeenCombatRules.cpp:80` sets `allParty` for
  `preferredClass()==16`; reference HATES_PARTY is **15**, HATES_NOBODY **16**. The `Step::Next` loop then visits every participant for each attack.
  `src/formats/xeen/XeenMonsterFormat.cpp:57-65` validates original Slime:
  two attacks, hatred 16, one d2 strike, poison damage type 5. Thus the current
  two attacks become two whole-party passes. This is a generic interpretation
  defect, affecting any record with hatred 16 (and mishandling true hatred 15),
  not a Slime-specific damage rule. Two random attacks need not hit distinct members.
- Decode named record semantics once: attacks byte 24, hatred byte 25, strikes word 26, damage die byte 28, damage type byte 29; strikes are dice
  per damage invocation, not attacks or target count.
- Share the reference attack-count loop and target selection between melee and ranged candidates. Melee currently repeats at `XeenCombat.cpp:972`;
  `XeenRegionalRules.cpp:254-264` currently advances after just one attack. Honor zero and multiple attacks without double-counting either caller.
- Per attack: hatred 15 visits the combat party in order, including disabled
  and dead members; recognized class hatred selects the first eligible match;
  hatred 12 selects a dwarf; Paladin value 1 means no preferred-class search.
  Otherwise draw over the entire participating combat party, then draw again
  over eligible members only if the selected member is disabled. No eligible member follows the reference defeat path. Re-evaluate after each injury.
- Preserve physical hit checks, sleeping bypass, waking, per-strike dice, poison saving throws and admitted special effects from `doMonsterTurn` and
  `doCharDamage`, including physical roll 20's damage before the subsequent
  hit check (potentially a second injury). Damage type is not target breadth.
- RNG intentionally changes for corrected hatred, dwarf preference and ranged
  multiplicity: target/fallback draws and each resulting hit/dice/save sequence
  follow reference order. Keep the existing gameplay PRNG algorithm and M48's separate presentation RNG; do not claim sequence identity with DOSBox.

## 2. Publish at impact

- Today `XeenJourneyConsequences.cpp:109-136` changes actor HP/death/XP/drops
  before creating Shoot visuals; `XeenCombat.cpp:835-848` does likewise before
  Magic Arrow's Projectile phase. Enemy movement candidates publish injuries through ActorApproach/combat before `observeRanged` launches the volley.
- Retain resumable candidates in their existing owners: XeenCombat for casting
  and combat opportunities, EncounterFlow/ActorApproach for Journey Shoot and
  enemy opportunities. Flow owns travel/display acknowledgments; guards and
  combat authority own validation and publication. Rendering never mutates HP.
- Split preparation, travel, impact-frame acknowledgment and publication.
  Prepare outcomes with private candidate state/RNG; keep live targets present
  and unchanged through travel and the acknowledged reference hit frame. Then
  publish HP, present/acknowledge the post-HP, pre-reward frame from `attack2`,
  then publish lethal accounting, XP, pending drops and removal exactly once.
  Keep lethal actors drawable until removal; use a guarded transient impact
  state/snapshot without weakening quiet-state actor validation. Block input,
  saves and successor settlement between stages; treasure delivery stays separate.
- Shoot follows reference depth/target/shooter order, including miss continuation
  into farther rows and lane termination on a hit. Publish each resolved impact
  before preparing dependent downstream outcomes; do not defer the whole ray as one aggregate or let visual arrival reorder gameplay draws.
- Enemy volleys follow `monstersAttack`: all visible travel completes, then sources resolve in reference order, with each source's full attack loop.
  Mixed distances must not reorder damage by arrival time. Off-camera sources
  still resolve and show portrait injury feedback; empty visible lanes cannot suppress or indefinitely defer their consequences. Use the appropriate
  portrait impact for enemies: acknowledge it before injury, as `doCharDamage`
  updates the portrait before HP. Do not invent a monster hit-frame requirement.
- Timing alone adds/removes no RNG draw: retain weapon/hit/resistance/save/drop
  order and count, including Magic Arrow's otherwise ineffectual saving throw.
  Publish the corresponding RNG continuation with each authoritative result;
  spell costs remain paid at reservation, with no retry charge/refund change.
- Bind pending work to action identity, generation, target/source identities and
  guarded preimage. Prepare allocations/checks before atomic stores; duplicate
  service/frame callbacks cannot redraw RNG or repay injuries/rewards. Stale
  callbacks fail closed; presentation failure cannot replay a committed impact.
- Serialize input/actions, movement, time charges, successor selection and reward opening until owed impacts settle. A quit during travel abandons
  unpublished work; no quiet save or new action can expose a partial candidate.
  Keep transient impact work unsaved. If persistent layout/meaning must change,
  stop for a plan amendment and version bump; never add a legacy reader.

## 3. Turn during combat

- Route Left/Right (including existing mouse buttons) past the blanket refusal
  in `XeenEventFlow.cpp:1143` to a guarded XeenCombat operation, only when ready.
  Rotate direction modulo four, flip only sky through ScenePresentation's existing combat navigation path, and set the reference party-stepped state.
  Do not charge a movement minute, consume the member's turn or clear Block.
- Keep a transient `_tillMove` countdown in existing Flow/combat scheduling,
  separate from `moveDue` round debt, which drains before PlayerReady. Arm 3 at
  the `chargeStep` equivalent; decrement on qualifying reference draw beats in
  Interactive/Combat, only with movement enabled and no enemy volley underway
  (`interface.cpp:715-719,1346-1349`). At zero invoke movement once; rotation
  flushes nonzero countdown through the same guarded opportunity, then marks stepped.
  Otherwise rotate without movement or RNG. Keep the acting member unchanged
  while eligible; resulting defeat/contact loss uses normal combat settlement.
- Update expected camera and affected actor/resource snapshots atomically; `XeenCombat.cpp:264` must still reject every unauthorized camera mutation.
  Reclassify contacts/targets for the new facing without resetting initiative.
  Do not confuse party `_stepped` with a map cell's visited/stepped flag.

## Evidence, tests and digest policy

- Before implementation, run the three M44 scenarios on the verified baseline
  with the header's fixed seeds; retain local saves and traces outside game data.
  Repeat after each item with identical inputs. Trace turn/source/attack ordinal,
  targets, HP/conditions/armor, RNG draw index/range/value, XP/drops, time, publication boundaries and rotation/movement. Compare decoded save fields
  and raw byte offsets, including checksums and downstream RNG consequences.
- These are intentional rule fixes: digests may change. Explain **every** changed
  byte through those traces; timing-only runs should retain final state absent
  a demonstrated reference-order correction. Unexplained differences stop work. Obtain maintainer approval of the evidence, then regenerate
  `tests/XeenM44BaselineDigests.h`, recording old/new hashes, baseline and reason.
  Keep exact current-format reload/re-save checks; no automatic baseline update.
- Test target/count semantics over every original Clouds monster record, using
  a reference-derived selector/count oracle independent of production helpers;
  cover hatred 1/12/15/16, class matches, absent matches, disabled/escaped members,
  repeated targets, zero/multiple attacks, and lethal first attacks. Unsupported abilities do not exclude their records from selector/count coverage.
- Test Shoot/Arrow/enemy impacts: misses/resistance/zero damage, lethal rewards,
  mixed depths, hit-frame presence, off-camera sources, draw-budget suspension,
  retries, stale tickets, failed presentation, quit and attempted saves/actions.
  Assert before/after live state and exact RNG traces, not only final screenshots.
- Test both rotations in every facing, with/without owed movement, repeated
  turns, PlayerReady with nonzero countdown, blocked movers and ranged consequences; verify sky, stepped, actor
  identity, unchanged acting member/initiative, and guards rejecting tampering.
- Extend existing rules/authority/presentation tests and a few original-data
  gameplay cases, not a separate process-continuation witness for each feature.
  During implementation run affected tests and `ctest -L fast`; after the final
  build delegate the full suite once to `gpt-6-luna`, low, `fork_turns: none`:
  `tools/run-full-ctest.ps1 -BuildDir <build>` (no `-Jobs`). Freeze checkout/build;
  runner waits with maximum tool waits, returns only results and does not rerun.
  Main waits through native agent completion with maximum wait, no intermediate
  checks/log reads/progress messages. Report model, command and waits; if runner
  unavailable, full validation stays pending. Failures prevent completion.

## Work order, risks and acceptance

Independent plan review, then maintainer approval/start; implement items 1, 2, 3 in order. Review digest evidence before accepting new baselines.
Highest risks: RNG changes cascading into scenarios, premature death/turn settlement,
double publication on retry and confusing countdown with round debt. Stop for unexplained differences, ambiguous reference
behavior, missing required authority or scope expansion; do not weaken guards.
Require independent implementation review and passing complete CTest, then
maintainer DOSBox comparison of Slime targeting/count, arrow arrival/lethal
visibility and combat turning. Only after acceptance close/condense this plan
and update project status/history. Planning validation is diff/link checking and `git diff --check` only; no build or CTest for planning.
