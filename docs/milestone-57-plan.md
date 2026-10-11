# Milestone 57 plan - Generic Event effects and world state

**Tier A, in two parts. Status: approved; implementation not started.**
Planning baseline: `main = origin/main = 454eea60fa1a49f5432784007c3e5e2e35f90374`;
the working tree was clean. Implementation requires the maintainer's start.

## Goal and evidence

Complete Joe's proof and Gunther's reward through generic Event mechanics,
then deliver Event treasure through shared item rules and RNG. This implements
[roadmap step 3](roadmap.md#near-term), bringing one container family and the
trees forward from step 5 as original-data proofs.
Addresses below identify evidence, never runtime admission or special cases.

Authority: the installed World of Xeen CD resources and pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` ([dependencies](dependencies.md)),
especially `scripts.cpp`, `party.cpp`, `character.cpp` and object presentation.
Maintainer DOSBox CD evidence, 2026-10-10:

- Gunther first adds "Rid Vertigo of the plague of pests and report back to
  Mayor Gunther (Vertigo,14,5)". Only opening the proof box advances the quest.
- Joe disappears, his shop is reported closed, and monsters stop respawning;
  Gunther grants 5,000 XP each, 4,000 gold and 50 gems, then the mines hook.
- Searched crates become open crates; beds do not change. Searching again
  says "Nothing here." Each tree pays only once.
- Display-case failure costs one year, moves the party to `(27,11)` and grants
  "Convicted Thief"; the party walks out. This is evidence for step 5, not M57.

## Part A - Joe, Gunther and durable world effects

- Decode and execute `MoveObj` using original object-record identity, signed
  byte coordinates and the original same-position flip behavior. Moving an
  object and turning it in place are distinct; neither rewrites commercial MOBs.
  Rendering, object selection and interaction must use the same effective state.
  Retain changes across cache rebuilds, travel and save/load, preserving actual
  reference resets; loading a save is not a reset.
- Add `SetChar` semantics required by these scripts (`0`: entire active party),
  retaining WhoWill's selected member and cancellation. Other selector forms
  remain unsupported until implemented from the reference, not guessed.
- Implement generic TakeOrGive modes 15 (award), 16 (XP), 20 (game flags),
  34/35 (currency) and 104 (quest flags), and GiveMulti's fixed 34/35 rewards.
  Admit neutral-take grants and neutral-give flag/award clears, with a neutral
  third pair; refuse unimplemented HowMuch and randomized exchange forms.
  Preserve widths, character/party effects, arithmetic, ordering and termination.
  XP stays in existing combat/Training state. Mutable roster-owned awards
  initialize all 30 characters from CHR, retaining ID remapping and award counts.
- First route: proof box `(9,22)`, including WhoWill, MoveObj,
  proof text, game flag 9 and AfterEvent edits; Gunther `(14,5)`, including
  first-talk quest flag 29, all-member XP, game flag 10, currency, award 6,
  clearing quest flag 29 and setting quest flag 1 for the Mad Dwarf Clan hook.
  Repeated conversations must neither repeat rewards nor erase the next quest.
- Follow flag 9 through Joe `(8,18)`, his sign `(11,18)` and town exit `(15,0)`:
  the script selects Joe's absent/closed branch and skips the respawn call.
  Do not invent a second Joe-removal or monster-reset mechanism.
  Retain ordinary travel and exit `SetVar(84)` facing; Gunther has no SetVar.
- The well `(14,17)` remains deferred by maintainer decision: AlterHed, typed
  Damage, max-HP comparison/healing and world flag 23 belong to step 5.
  Both flag-9 branches must refuse visibly before any head, HP, flag or RNG
  change. Do not bypass an unsupported instruction to expose a partial well.

## Part B - Shared treasure generation and delivery

- Generalize the existing `XeenMerchantItemCandidate` makeItem adaptation into
  common item-generation rules used by merchants and Event mode 66. Reuse the
  same tables, item categories, level semantics and conditional draw sequence;
  no Vertigo generator, independent seed or new RNG owner. Preserve established
  monster-drop behavior when sharing applicable generation/delivery helpers.
- Support GiveMulti's three ordered mode/value pairs, including generated
  items (66) and random gold (100), alongside Part A's fixed currency modes.
  Share delivery rules for weapons, armor, accessories and miscellaneous items:
  selected recipient, party order, condition eligibility, category capacity,
  sorting, overflow/loss and original full-pack behavior. Do not route all
  treasure through the current miscellaneous-only Event queue or treat every
  generated item as purchasable/usable; later item services remain deferred.
- Prove the crate family represented by `(9,28)`, including its complete
  script, opening appearance and repeat search. CD object 8, like proof object
  0, is sprite 95 initially at `(-128,-128)`: MoveObj reveals the open overlay.
  This avoids GiveMulti's separate locked-chest path; that remains unsupported.
- Prove the trees through game flags 232-255/57/58 and GiveMulti mode 100
  (`1..10` gold). Keep `(75,76)` and flag 231's initialization exactly as the
  resources prescribe; M56 already executes this prelude before Tavern entry.
  Revisit, Tavern entry, departure and reload must not mint another payment
  within the same initialized cycle; explicit year-grant resets remain with jail.

## Ownership, RNG and publication

- Extend `XeenWorld`/session state with resource-relative object overrides;
  keep immutable source records and session removals distinct. Extend existing
  Party/roster, Event continuation, reward and Flow owners; no new coordinator.
  Update mutation watches, restore guards, snapshots and equality checks together.
- Every gameplay draw uses the retained Journey RNG through the existing
  consequence-draw mechanism. Preserve script pair order, party iteration and
  makeItem's conditional category/enchantment/charge draws, including draws for
  eventual capacity loss. Fixed rewards and preflight consume no random draws.
  Retain partial generation across draw-budget suspension; retries cannot reroll.
- Extend M56's detached candidate and capability preflight to all new effects,
  including called scripts and terminal services. Validate supported dependent
  paths, source identity, ownership and resources before publishing a prefix.
  Stage XP, purse, awards, flags, object/Event edits, inventory and RNG together;
  publish a completed dependent reward group once through callback-free stores.
- User choice cancellation before publication abandons the candidate without
  changing live state or RNG. A gameplay refusal/loss follows the reference's
  outcome; infrastructure failure is not permission to silently lose a reward.
  Retain prepared results for recoverable presentation/publication retries.
  After publication, acknowledgments only advance the receipt; repeated/stale
  input cannot regrant, reroll or undo it. No save during generation, selection,
  reward pages, handoff or settlement; only the settled quiet state is saveable.
- Unknown opcodes, modes, destinations or dependent effects still show "not
  supported yet" and return safely without partial rewards or consumed flags.
  Preserve M56 terminal-service semantics and existing mainland Event behavior.

## Save change and risks

- Part A increments the envelope **v8 -> v9**, retaining **schema 9/content 14**.
  Store mutable awards for all 30 roster owners and bounded object overrides
  keyed by side/map/original record (signed position and flip, not animation
  frames). Existing flags, Event edits, XP, purse and RNG remain authoritative.
  Reject old envelopes clearly; no legacy reader or replay. Part B retains v9.
- Event treasure staging is transient: the reference clears it after delivery.
  Settle delivery/loss and currency before saving; persist final inventories
  and purse. Amend this plan before any later save layout/meaning change.
- Validate resource binding, bounds, duplicate overrides and award data before
  restoration; rebuild visuals from restored state without replaying scripts.
  Risks: reward/flag separation, stale publication, RNG drift and treating object
  state as cosmetic animation. Record retained oddities in [quirks](original-quirks.md).
- Deferred: the well's effects, display cases/jail, broader step-5 content,
  mines exploration, full Quests/Awards screens and the original treasure UI.
  Original text stays resource-driven. Open DOS questions: none for this scope;
  ask only if implementation exposes contradictory or unverifiable evidence.

## Acceptance

- Independent Tier A plan review before approval and implementation review after
  both parts; resolve findings. Maintainer start and final acceptance are separate.
- Rules tests cover decoding/mode widths, selectors, awards, object moves/flips,
  shared generation draw tapes and delivery capacity/order. Exercise cancellation,
  failures, stale callbacks and repeated acknowledgment for exactly-once publication.
- Generic save round-trip covers changed objects, flags, awards, rewards and RNG,
  including malformed-state refusal and exact restore without Event/time replay.
- Original-data Joe route from New Game through proof, Gunther, repeat visits,
  exit/re-entry and save/load; container and tree runs; both well refusal branches.
- M44 digest gate follows [M55](milestone-55-plan.md): preserve baseline executables,
  reproduce all three v8 hashes, compare complete traces and every common decoded
  field at all checkpoints, account for every v9 byte/CRC delta, and prove exact
  reload/re-save. Obtain maintainer audit approval before replacing any digest.
- Build and run the complete `build-rel` suite after each final production task
  through the [test-runner protocol](../tools/AGENTS.md); Debug closure runs
  `ctest -L fast` plus all three M44 scenarios on `build-m44`.
- Maintainer Debug play-test against the DOSBox CD evidence, including visuals,
  repeat searches/rewards and save/load. Only then record formal acceptance.
