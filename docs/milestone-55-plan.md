# Milestone 55 plan - Interface gaps

**Tier A, in two parts. Status: approved; implementation not started.**

## Goal and scope decision

Make Strafe, Exchange, Quick Reference, Info and Quick Fight work through the
original CD interface, by mouse and keyboard. Excludes Map, Quests, Dismiss,
new spells, combat equipment/Use, party recruitment and audio.
Planning baseline: clean `main`, HEAD/local origin/main/live origin/main all
`e882f34661a3d7952eb58c31d63be48b47bf8136`; reference `6814ee9ba54582f5b5adcffab49efbbd8f589edd`.
Below, `R:` means `D:/Projetos/MModern/scummvm-known-good-candidate/engines/mm/xeen/`;
MMModern paths are repository-relative. DAT offsets are file offsets, end-exclusive,
read in memory from `F:/Games/gog/Might and Magic 4-5/WORLD/XEEN.DAT`; never extract or modify data.

The requested scope is larger than interface-only Tier B: Exchange meets fixed
combat ownership assumptions, and Quick Fight needs persistent mutable settings.
The whole milestone is Tier A; retain existing owners/scheduler. It is
implemented in two parts on working branches:

- **Part A - Strafe, Quick Reference, Info** (branch `m55-part-a`). Read-only
  dialogs and ordinary movement; save stays v7, M44 digests stay unchanged.
  Checkpoint: affected tests, `-L fast` and the complete suite on `build-rel`.
- **Part B - Exchange, Quick Fight, save v8** (branch `m55-part-b`, from Part A).
  The ownership audit lands first; the single v8 change lands before any
  reordered party or mutable Quick Fight setting can be saved. Checkpoint: the
  complete suite plus the M44 digest gate below.

After Part B: one independent implementation review of both parts, the Debug
closure check, then the maintainer's DOSBox play-test of the whole milestone.
Acceptance is recorded only after that play-test.

## Strafe

- [ ] Add Ctrl+Left/Right and keypad 4/6, plus the existing lower corner buttons;
  keep facing unchanged. Reference: `R:interface.cpp:345`, `:455`, `:483`, `:1044`.
  Sprites: exploration `main.icn`, combat `combat.icn`; no new text prompt.
  Combat ignores sideways movement (`R:interface.cpp:1811`); do not turn or spend a turn.
- [ ] Extend movement directions at `src/games/xeen/XeenMovement.cpp:159` and
  input at `src/platform/sdl/SdlWindow.cpp:43`, `src/platform/sdl/XeenMainScreenInput.h:40`.
  Route Journey movement through `src/app/XeenEncounterFlow.cpp:14` and
  `src/games/xeen/XeenActorApproach.cpp:492`, not just the standalone navigation flow.
  Reuse wall/terrain checks, movement publication, `src/app/XeenJourneyFlow.cpp:557` and
  `src/app/XeenEventFlow.cpp:260` automatic Events with the retained facing.
- [ ] Preserve successful-step charge (1 minute indoors, 10 outdoors), ctr24,
  monster opportunities and daily processing; blocked steps take the ordinary
  blocked path. Authority: `R:interface.cpp:712`; existing pipeline: `src/games/xeen/XeenActorApproach.cpp:503`.
  Position/time/actors and consequent RNG change as for ordinary movement;
  no new RNG draws, save fields or timing policy. Preserve original step animation.

## Exchange

- [ ] Character sheet E/button opens window 31, asks whom, accepts F1-F6 or
  portrait clicks, swaps selected and destination positions, and follows the same
  character to its new slot; self/missing selection keeps the dialog open, Escape cancels.
  Combat keeps the original refusal. `R:dialogs/dialogs_char_info.cpp:225`,
  `R:dialogs/dialogs_exchange.cpp:35`, `R:dialogs/dialogs.cpp:81` are the authority.
  DAT: EXCHANGE_WITH_WHOM `[0x55d54,0x55d70)`, existing EXCHANGING_IN_COMBAT;
  `view.icn` Exchange frames and `esc.icn` cancel (`R:dialogs/dialogs_exchange.cpp:71`).
  DOS-confirmed: only portrait faces and the visible cancel button accept clicks;
  surrounding space does nothing. Do not reproduce the reference's y=245 cancel extent.
- [ ] Reuse `src/app/XeenCharacterDialogFlow.cpp:45` and
  `src/games/xeen/XeenDialogView.cpp:104`; publish a tracked party-order mutation
  through the existing Flow boundary, refresh sheet selection/portrait input and invalidate stale selections.
  `src/games/xeen/XeenParty.h:57` already owns ordered roster IDs;
  `src/games/xeen/XeenSaveState.cpp:96`/`:277` and `src/formats/xeen/XeenSaveFormat.cpp:317` save them.
- [ ] Tier A prerequisite: replace the fixed order `kXeenCombatOwners`
  (`src/games/xeen/XeenCombatInputs.h:30`) with the party's current order
  (`activeRosterIds()`), keeping roster identities stable. Allow permutations of
  today's six members, without adding/removing members. Production inventory:

  | Kind | Sites | Rule after Exchange |
  | --- | --- | --- |
  | Order-sensitive (array index = party position) | Combat participants, speed ties, targeting, observations and participant masks (`XeenCombat.cpp:197`, `:229`, `:301`, `:428`, `:646`-`:701`, `:863`); regional opportunities (`XeenActorApproach.cpp:484`); Shoot character/input builders and eligibility, which also feed Shoot time and treasure delivery (`XeenJourneyConsequences.cpp:12`, `:15`, `:32`), and shooter lanes (`:102`-`:114`); Rest scarce-food recovery and condition-tick RNG order (`XeenRestFlow.cpp:9`-`:12`); casting time (`XeenCastingFlow.cpp:24`-`:29`); item-use opportunities (`XeenJourneyFlow.cpp:208`); barrier member selection (`XeenBarrierFlow.cpp:14`, `:37`, `:158`); monster treasure recipients (`XeenMonsterTreasure.cpp:87`) | Build arrays from current order; combat captures it on entry and retains it until combat ends. |
  | Owner-keyed storage (index → owner write-back) | Prepared-state publication (`XeenActorApproach.cpp:345`); experience and consequence write-back (`XeenCombat.cpp:933`-`:1096`, `XeenJourneyConsequences.cpp:145`, `:178`, `XeenRestFlow.cpp:114`-`:143`, `XeenCastingFlow.cpp:215`-`:225`, `XeenActorApproach.cpp:555`, `:590`, `XeenBarrierFlow.cpp:104`, `:198`-`:271`) | Map index to owner through the same captured order as the read side. |
  | Membership validation | `XeenJourneyRules.cpp:32` (rejects reorder), `:49`, `:67` | Accept any permutation of the six; still reject other membership. |
  | Order-independent | Membership-wide skill aggregation (`XeenActorApproach.cpp:271`) | Unchanged. |
  | Prepared initialization | Prepared levels/XP (`XeenActorApproach.cpp:284`-`:291`) | Stays attached to the intended roster identities, not to positions. |
  | Diagnostics | Roster text (`XeenEncounterFlow.cpp:434`, `:458`, `XeenJourneyConsequences.cpp:314`-`:370`); `XeenItemRewards.cpp` already uses current order | Show current order; unchanged where already ordered. |

  Each read uses the same captured order as its consumers and write-back. Do not
  blanket-replace every fixed-owner use with current-position indexing; tests and
  witnesses using `kXeenCombatOwners` keep a named default-order constant.
- [ ] Exchange itself consumes no time/RNG and changes no character data. Subsequent
  combat targeting, speed ties, damage, casting and rewards must resolve current positions
  to the correct owners. Order already has save storage; its newly admitted semantics
  join the single v8 change below. Reference persistence: `R:party.cpp:267`.

## Quick Reference and Info

- [ ] Q/button in exploration/combat and Q on the character sheet opens Quick Reference;
  any fresh key/click closes back to the caller. `R:dialogs/dialogs_quick_ref.cpp:61`:
  window 24, current combat participants when fighting, otherwise active party;
  number/name, three-letter class, permanent level, HP/SP, AC, four-letter worst
  condition, original colors; gold/gems and food days = food / active party size / 3.
  Retain original framed text/underlay; no special sprite.
- [ ] DAT QUICK_REF_LINE `[0x55b81,0x55be8)`, QUICK_REFERENCE `[0x55be8,0x55c93)`;
  reuse CLASS_NAMES, CONDITION_NAMES, DAY_SINGULAR/PLURAL. Reuse sheet rules/colors
  at `src/games/xeen/XeenDialogView.cpp:261`, dialog stack/input at
  `src/app/XeenCharacterDialogFlow.cpp:6`/`:14`/`:68`; add main-screen Q mapping.
- [ ] I/button opens Info in exploration/combat; any fresh key/click closes it.
  DOS-confirmed title in CD Clouds play: World of Xeen, using WORLD_GAME_TEXT.
  `R:dialogs/dialogs_info.cpp:41`/`:122`: window 28 grows with effect rows, scene
  keeps animating; 12-hour clock, day/year and ten-day weekday, then nonzero light,
  fire/electricity/cold/poison resistance, clairvoyance, levitation, walk on water in that order.
  No extra effects, duration estimates or special sprite.
- [ ] DAT GAME_INFORMATION `[0x535f7,0x53673)`, WORLD_GAME_TEXT `[0x53673,0x53679)`,
  weekday strings `[0x519c5,0x519f6)`, effect formats `[0x5355a,0x535e6)` and
  alignment fragments `[0x5354b,0x5355a)`. Use DOS signatures, not ScummVM's:
  DOS WALK_ON_WATER has two arguments; Quick Reference uses a day suffix.
  Reuse `src/games/xeen/XeenGameplayContext.h:14`, parsed at
  `src/formats/xeen/XeenGameplayContextFormat.cpp:14`; `R:party.cpp:295`/`:316`
  maps effects[0/3/4] and lightAndResistances[0/2/3/4/5] to these rows.
- [ ] Both are read-only: no save mutation, gameplay time, combat turn or RNG draw.
  Extend the bounded M54 DAT reader (`src/formats/xeen/XeenDosText.cpp:38`,
  `src/formats/xeen/XeenDosText.h:24`) with verified field/signature/button metadata, and reuse
  `src/games/xeen/XeenDialogView.cpp:34`/`:48`; do not embed original strings.

## Quick Fight, persistence and scheduling

- [ ] F/combat button executes the *current actor's* configured action, then advances
  normally; repeated input reaches the other members. No whole-party auto-batch.
  `R:interface.cpp:1747`, `R:combat.cpp:1597`: Attack, Cast, Block, Run (in that order).
  O/combat Options button opens configuration, not the character sheet
  (`R:interface.cpp:1759`, `R:dialogs/dialogs_quick_fight.cpp:52`).
- [ ] Window 10: current member/action, N/Next cycles immediately; F1-F6/portraits
  select a member with the DOS indexing quirk recorded below; Return or Escape/Exit
  closes without undoing changes. `train.icn`
  Next/Exit, `combat.icn` main buttons. DAT QUICK_FIGHT_TEXT `[0x54a00,0x54a56)`
  and action labels `[0x519fd,0x51a13)`; verify DOS hitboxes/hotkeys before adding them.
- [ ] Promote quickOption and currentSpell from immutable defaults
  (`src/games/xeen/XeenCharacter.h:89`, `src/formats/xeen/XeenCharacterFormat.cpp:180`)
  to tracked state initialized from CHR, saved for all members. Cast remembers a class spellbook slot:
  `R:dialogs/dialogs_spells.cpp:265`/`:367`: cancelling selection preserves the old slot;
  selecting then cancelling outer Cast retains the new slot. Validate option 0..3 and
  slot 0..38 or none (0xFF/-1); reference `R:character.h:135`, `R:character.cpp:218`.
- [ ] Reuse `src/app/XeenEncounterFlow.cpp:320` command/scheduling and combat casting
  (`src/app/XeenEncounterFlow.cpp:163`, `src/app/XeenCastingUi.cpp:167`);
  preserve target prompts, missile settlement, Run consequences and per-action RNG.
  No extra random stream/draw or new scheduler. Configuration is free of gameplay time/RNG.
  Unsupported Cast/Run requirements refuse visibly without effects/time/RNG/turn consumption;
  no-spell skips casting but spends the turn, as do insufficient SP/gems or ordinary
  combat-forbidden casts (`R:combat.cpp:1606`, `R:spells.cpp:177`, `R:interface.cpp:1749`).
  Preserve target-cancel costs/refunds per supported spell; unsupported-system refusal is distinct.
- [ ] Propose envelope v8; retain schema 9/content 14. Extend character save/read,
  equality, capture/restore and validation (`src/formats/xeen/XeenSaveFormat.cpp:100`,
  `src/games/xeen/XeenSaveState.cpp:92`); reject v7 clearly, no legacy reader.
  Current constants: `src/formats/xeen/XeenSaveFormat.h:27`. M54 slot/session behavior stays exact.

## DOSBox evidence and risks

Maintainer's normal CD DOSBox play and three screenshots, 2026-10-10, resolve all
three questions. Exchange accepts only faces/the cancel button, ignores surrounding
space, and self-selection swaps nobody. Info shows World of Xeen; any key/click closes it.
Quick Fight Options retains the original active-party indexing after Run, bounded
by remaining combat count, matching `R:dialogs/dialogs_quick_fight.cpp:97`/`:99`.
With Arturius, Tyro, Badger, Zippo, Rebecca, Seymour initially and Arturius fled,
the five visible portraits are Tyro through Seymour, but F1-F5/the corresponding
portrait clicks select Arturius, Tyro, Badger, Zippo, Rebecca; F6/empty slot does nothing.
Reproduce this observed DOS bug in Options; combat actions still use the actual actor.

Main risks: fixed-owner assumptions beyond combat; modal/stale input changing the wrong
member; duplicate turn advancement; DOS/reference formatting and click-area differences.

## Acceptance checklist

- [ ] Rules: four-facing strafe, walls/blocked time, monster/time/Event parity;
  Exchange self/cancel/permutations, targeting/ties/rewards after reorder; summary colors,
  food rounding, clock boundaries and every effect row; four quick actions and refusals.
- [ ] Mouse/keyboard parity for every entry, selection, cancel and return; stale/held
  input cannot leak across dialogs. Cover the DOS Options-after-Run mapping, Exchange's
  inert surrounding area/self-selection, Info title/dismissal and selected-spell persistence.
- [ ] A few original-data checks: Vertigo strafe into Events/walls, CD dialog
  rendering/options (including Info with/without effects). After a reorder:
  combat, Shoot lanes, scarce-food Rest, condition ticks, barrier selection and
  treasure delivery follow the new positions and write back to the right owners.
  Generic save round-trip after Exchange + Strafe + option changes: exact
  order/position/settings/calendar/actors/RNG, no replay.
- [ ] M44 digest gate (Part B; procedure of `tests/XeenM44BaselineDigests.h:73`).
  Before rebuilding, preserve a v7 executable/witness from the Part A tip; reproduce
  all three old hashes with unchanged inputs and seeds; compare the complete traces and
  every common decoded field (they must be identical); account for every added field
  and the version/length/CRC deltas; verify v8 reload/re-save equality. Stop on any
  unexplained difference. Replace the digests only after the maintainer approves the audit.
- [ ] During implementation: affected tests and `ctest --test-dir build-rel -L fast`;
  complete suite after each task's final production build through [tools/AGENTS.md](../tools/AGENTS.md).
  Closure: Debug fast + three M44 scenarios, independent review, maintainer DOSBox acceptance.
  This planning-only task uses diff/link checks only.
