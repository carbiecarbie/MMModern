# Milestone 58 plan - Skills and mapping

**Tier A. Status: approved; implementation not started.**
Baseline: `6e4d26693e1c789e3aaac36e3496860140967ad6` (`main`).

## Goal and scope

Learn Cartography from Mylo and Pathfinder from Rialdo, use one character skill
state for rules and display, and explore with the original automap and Map
screen ([roadmap](roadmap.md), step 4). Keep whole-map loading and generic
Events; teacher addresses below are evidence, never runtime admission rules.
Authority: World of Xeen CD resources, maintainer DOSBox evidence of
2026-10-10/11, and ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`
([dependency provenance](dependencies.md)). The reference checkout was verified
at that revision and clean when planning.

M57 already supplies staged Event publication, carried currency, awards, XP,
flags, object changes and treasure. Extend those paths, not a parallel trainer
system. Vern's 25-gold dues and per-character guild membership remain step 5;
guild spell purchase and Wizard Eye remain step 6. No new playable regions,
character creation, general spell support or service catalogue is included.

## Character state and skill effects

- Give each of the 30 roster-owned `XeenCharacter`s one mutation-tracked array
  of 18 skill bytes, initialized from CHR bytes 39..56. Preserve imported
  values. Increment modulo 256: 255 becomes 0, identically live and after load.
  Mark the grant site "follows the original byte storage; not confirmed in DOS".
  This is explicit maintainer policy (2026-10-11), not a DOS observation.
- Remove the duplicate `maxStatSkills` authority and skill storage/readers in
  immutable `originalDetails`. Derive maximum HP/SP, Thievery, Navigator's
  existing Rest effect, the skill count and the character-sheet list from the
  mutable array. Update copies, equality, combat snapshots and mutation guards.
  Learning changes the skill; it does not refill current HP/SP or train a level.
- Preserve the existing Bodybuilder, Astrologer, Prayer Master,
  Prestidigitation, Thievery and Navigator rules. Add Cartography and Pathfinder
  through shared character/party queries. `Party::checkSkill` counts the active
  party without a condition filter: two holders for Pathfinder/Mountaineer,
  all members for Swimming/Crusader, one for other skills. Empty parties fail.
- Pathfinder is skill 11. The pinned `Interface::canMove` does not consult it:
  do not invent a forest restriction or movement discount. Its two-holder
  predicate is available to Event action 99. Cartography is skill 4; one holder
  enables recording. Neither skill automatically enables the automap switch.
- Implement Event skill tests (13 for the selected character, 99 for the
  party) and mode-13 grant/removal for supported skills, preserving original
  comparison and selection semantics. Grant increments; removal clears the
  value. Do not replace these with an idempotent learned flag.
- Other skill bytes remain intact and visible. Acquisition or an attempted
  effect of an unimplemented skill says "not supported yet" before charging
  gold or publishing dependent effects. A shared support classification covers
  preflight and execution; it does not gate maps or individual teachers.
  Broader skill effects are deferred, rather than silently treated as working.

## Teachers and existing owners

- Read teacher names, offers, portraits and WhoWill text from the original
  resources. Mylo at logical map 28 `(8,16)` grants skill 4 for 100 carried
  gold. Rialdo at `(25,26)`, facing North, grants skill 11 for 2,000 carried
  gold although his offer says 2,500. Retain both text and charge; see
  [original quirks](original-quirks.md).
- Both scripts use NPC mode 2, WhoWill, an existing current-SP branch,
  a separate mode-34 debit, then mode-13 grant. Implement mode 2 as
  `LocationMessage::execute` does: present the offer and continue to WhoWill,
  without adding a Yes/No prompt. Their If1 action 9 tests current SP >= 0;
  preserve that branch, WhoWill cancellation and insufficient-gold refusal.
- Reuse `XeenEventSystem`/interpreter/publication and `XeenEventFlow` for this
  sequence, existing dialog rendering for the original offer/WhoWill windows,
  and existing purse ownership in the Journey. F1-F6 and portrait clicks select
  the recipient; preserve `WhoWill::execute` eligibility and the single-member
  shortcut. The normal six-member prompt is "Who will learn? F1-F6".
- Debit and grant publish once through the M57 candidate transaction. Fresh
  recipient selection is required; stale input, cancellation and unsupported dependent
  operations cannot spend or teach. Per maintainer clarification, teachers grant
  once; do not treat repeat paid lessons as a supported gameplay path.
- These are Event NPCs, not `DoTownEvent` locations. Reuse Service/Flow owners
  where shared presentation is needed, but do not add a trainer service owner,
  departure day, merchant regeneration or Bank interest. Preserve ordinary
  interaction scheduling and the existing CD speech limitation.

## Automap, Map and exploration

- Use `Party::handleLight`, `Interface::perform`, `InterfaceMinimap`,
  `MapDialog` and `Map::mazeLookup` as the rules/rendering references. Record
  the occupied cell at the reference's stepped-update boundary only when the
  party has Cartography, including entry and other supported stepped actions.
  Do not infer exploration from rendered visibility, or retroactively reveal
  the route walked before learning. Turning/repainting must not grow a radius.
- Keep exploration in `XeenSessionWorldState`, keyed by physical map tile
  identity and local cell (256 bits per 16x16 tile). Resolve Vertigo's 32x32
  logical coordinates through existing world sampling. Original `stepped`
  resource bits seed the effective state; cached `seen` bits are distinct.
  Reuse effective cells and barrier overrides so opened doors appear correctly.
  Preserve explored cells through cache reload, services, actor resets, travel
  and save/load; never edit original MAZ resources.
- Render the 7x7 automap with original tile/global sprites, terrain/wall layers,
  direction marker, clipping and explored-cell masks, indoors and outdoors.
  `=`/keypad `=` toggles it; the off state shows the original logo. Reuse
  `XeenGameplayContext::effects[1]` (PTY automap byte), already saved, instead
  of introducing another switch. Respect original modal-window suppression.
- Map button and M open the original 16x16 viewport, with its edge positioning,
  map name, coordinates, facing, frame and animated marker. Use original
  resource text and the DOS text reader, not embedded English or new controls.
  There is no new scrolling, zoom or skill-gated opening: the reference gates
  recorded cells, not access to Map. Match key/mouse dismissal and caller-mode
  restoration, including combat; no turn, RNG or time is spent on viewing.
- Both views consume the same effective exploration. Drawing is read-only;
  schedule recording outside render code. Save only at the existing quiet
  boundary, not inside a teacher or Map modal. Wizard Eye stays unsupported;
  do not let its presentation path reveal additional cells in M58.

## Save decision

**Change envelope v9 to v10; retain schema 9 and content 14.** This is required:
v9 writes only four max-stat skill booleans and no exploration overlay. Replace
those booleans with all 18 skill bytes for every roster owner and add bounded,
deterministically ordered explored-tile bitsets. The existing automap byte needs
no new field. Use one byte per skill in memory and in the save, so restoration
retains the complete live value, including 0 after a wrapping increment.

Validate tile identities, bounds, duplicates and record counts before publishing
a restore. Restore into fresh owners, merge with resource defaults without
replaying exploration, and retain the RNG, purse and clock exactly. Reject v9
and earlier clearly through the current older-save policy; add no legacy reader.
Update the generic round-trip, rejection and intentional save-digest expectations
after auditing differences; unchanged gameplay traces must remain unchanged.

## Risks, tests and acceptance

- Unit tests: CHR skill initialization, grant/removal and predicates,
  active/inactive owners, condition eligibility, derived stats and display,
  debit boundaries, once-only teaching, cancellation and stale publication;
  unsupported skills must leave purse, skills, world, time and RNG unchanged.
  Synthetic boundary tests cover 254 -> 255 -> 0 and exact save/load at each
  value; overflow is not an original-data teacher scenario.
- Mapping tests: with/without Cartography, before/after learning, automap off,
  initial resource bits, indoor quadrant seams, outdoor edges, opened barriers,
  entry/turn/movement update boundaries, and drawing that cannot mutate state.
  Verify shared masks, original sprite placement, input and modal restoration.
- A few original-data end-to-end tests cover Mylo, Rialdo and a mapping route
  through Vertigo/mainland with save/load and revisit. Assert the real debit and
  selected skill, cancellation/insufficient funds, once-only teaching and retained
  explored cells. Extend one generic exact round-trip for all 30 owners and
  exploration; do not add process-continuation witnesses per feature.
- Independent Tier A plan review precedes maintainer approval; independent
  implementation review precedes acceptance. Reviews check ownership, reference
  fidelity, transactional learning, physical tile identity and save necessity.
  Resolve findings before the respective approval; do not implement on review
  approval alone. Run focused tests and `ctest -L fast` on `build-rel` while
  implementing, then the full suite and Debug closure check under
  [tools/AGENTS.md](../tools/AGENTS.md). No build/CTest for this plan-only change.
- Maintainer Debug play-test: both trainers and quoted/charged prices, original
  dialogs, mapping before/after learning, indoor/outdoor visuals, controls,
  travel and exact reload. Only explicit acceptance closes M58. Implementation
  must stop for contradictory DOS evidence or scope expansion.

## Evidence limits and settled policy

The maintainer settled counter width on 2026-10-11: skills use original byte
storage and increments wrap modulo 256. Teachers grant once and there is no
repeatable skill grant in normal play, so overflow is unreachable there.
ScummVM's wider live counters and save-time byte truncation are not reproduced.
Record "follows the original byte storage; not confirmed in DOS" at the grant
site and in [original quirks](original-quirks.md). No further DOS question remains.

Pathfinder not gating forest movement and condition-independent party checks
remain explicit reference rules, recorded in the quirks ledger. Compare them
and Map visuals during play-test. Plan approval is still required before
implementation; the counter policy does not authorize commit/push.
