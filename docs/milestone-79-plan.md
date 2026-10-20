# Milestone 79 plan - Services of the other towns

**Tier B, except Part C (Tier A). Status: planned.** Step 9 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M78](milestone-78-plan.md). Baseline `093d517`.

## Goal

Every Clouds service outside Vertigo works as in the original, on the
existing Smith, Training, Temple, Tavern, Bank and Guild owners, chosen by
the camera map rather than by map 28. The original Events reach them only
through unconditional `DoTownEvent` records at line 0, facing All:

| Map | Services (action) |
| --- | --- |
| 29 Nightshadow, 31 Asp, 32 Winterkill | Guild (2) at `(12,4)`, `(2,1)`, `(7,1)` |
| 30 Rivercity | Ironworks `(24,9)`, Tavern `(24,16)`, Training `(12,12)`, Guild `(6,30)`, Bank `(25,3)`, Temple `(6,21)` |
| 49 Shangri-La | Temple `(15,3)`, Ironworks `(14,8)`, Tavern `(14,13)`, Guild `(0,7)` |
| 73 Newcastle Level 1 | Ironworks `(4,7)`, Training `(11,7)` |
| 74 Newcastle Level 2 | Temple `(4,4)`, Bank `(5,8)` |

Per place (DOS, Part 0): Training cap 10 at 28, 15 at 30, 20 elsewhere,
cost `10*L*L`; Ironworks shop 0-3 for maps 28/30/73/49, otherwise 0, with
map-independent prices; Guild rows and awards 0-4 for maps 28-32, every
class spell and award 5 at Shangri-La, Spell Info including learned spells;
Tavern Food 15 per member for 10 gold at 28, 60/50 at 30 (the pin has 50),
120/100 at 49, with offer texts 75-77. Temple surcharges and Tavern Tip/Rumor
groups were already per map. Map 74's Temple uses the last Temple prices of
the running program: zero at program start, replaced at each tabled Temple,
kept across New Game and Load, never saved. Bank interest and accounts are
global. Out of scope, each keeping its "not supported yet" refusal before
any effect: entrance scenes, the Arena and the Pyramid (`DoTownEvent` 6 and
8-12, including map 49 `(7,14)`), Sign In and Dismiss, audio.

**Save:** stays v16 (schema 9, content 14). Stock for all four shops is
already generated, regenerated and saved; Tavern tip cursors are per map.
Accepting depletion in shops 1-3 widens restore validation only. The map 74
prices are process state outside the save.
**M44:** the services digest is expected unchanged; any change stops the
work for the maintainer.

## Maintainer decisions (2026-10-20)

- Map 74's Temple reproduces DOS (option A): the prices live for the
  running program, outside the game session and the save. This new owner is
  Part C, Tier A, with independent plan and implementation reviews of that
  part only.
- Depletion in shops 1-3 widens restore validation only; the save stays v16
  without an independent review.
- Scope adds Shangri-La and Newcastle to roadmap step 9's four towns.
- Part C keeps the Temple price revision check, and its cleanup of map-28
  defaults goes in a separate commit (after the independent plan review).

## Tasks

- [x] **Part 0 - DOS check.** Calibrated static analysis of
      `WORLD/XEEN.DAT` settled every question; see
      [service evidence](rules/evidence.md#other-town-services-2026-10-20).
- [ ] **Part A - rules by place.** Training cap, Ironworks shop selection
      and purchase offers for every shop, Guild rows and membership, Food
      offer, each in the existing rule owners. No per-town copies. Correct
      the Temple table offset cited in `XeenTempleHeal.cpp` (`0x1ce87`).
- [ ] **Part B - admission.** Remove the map-28 gates from the Event
      preflight and runtime, `xeenRegionalService`, `XeenEventPublication`,
      `beginSmith`, `beginTraining` and the service preflights, which bind
      the camera map's Events and text through the generic text admission.
- [ ] **Part C - Newcastle Level 2 Temple (Tier A).** A short design note
      in this plan, reviewed before code: the owner of the program-lifetime
      Temple prices, how the Temple Flow reads and replaces them, and why New
      Game, Load and restore leave them alone. Then implement it; map 74
      refuses until Part C lands.
      - **Authority.** Implement option A from the
        [settled charges](rules/time-and-conditions.md#temple-heal-price),
        [DOS evidence](rules/evidence.md#other-town-services-2026-10-20) and
        [carry-over quirk](original-quirks.md#sv-temple-map74-carry).
      - **Owner.** Add one noncopyable `XeenTemplePriceState` in the service
        rules layer: a zero-initialized `XeenTempleSurcharges` row and an
        unsaved revision. `main` (or a member of `Application`, outside
        `run()` and `gameplay()`) creates it once per process, before the
        title loop, so title return, New Game and panel Load never reset it.
        Sessions borrow it by reference through `XeenGameplayServices`,
        `XeenEventFlow` and `XeenEncounterFlow`; no `mutable`, per-session
        fallback or second copy. Each test case or fixture creates its own
        owner (one owner = one process); the census creates one per run.
        Existing Party, World and Flow owners end with a game session, so
        none can own this DOS program state. This is the reason for the owner.
      - **Prepare.** `beginSmith` computes the visit's row before calling the
        preflight: tabled Clouds 28/30/49/73 and every Darkside map use
        `xeenTempleSurcharges`; an untabled Clouds Temple copies the owner's
        current row. It stores that row and the owner's base revision in
        `SmithContinuation`. Staging changes neither owner nor revision.
      - **Publish.** In `serviceSmithPreparation`, check every capacity,
        including the revision's, before `_boundary.hold`. After the hold and
        before `checkSmithBoundary(AfterAdmission)`, write row and revision in
        the same nonthrowing, callback-free step as `_smith=std::move(...)`
        and the Service activity. Every fallible check and hook that can
        refuse comes before that step; a refusal, exception or stale
        preparation leaves row and revision unchanged. A failure in
        `AfterAdmission` cannot undo it: the row stays published and the
        visit stays usable.
      - **Read.** One accessor serves every Temple read (text, Heal
        quote/prepare/confirm, Uncurse, Donation): before admission it returns
        the visit's prepared row; after admission the same row, checking that
        the owner's row equals it and the revision matches. Redraws during
        preparation therefore show the visit's own prices. Rendering never
        writes. Zero surcharges keep the Heal and Uncurse base charges; zero
        Donation returns NoAction before counter, blessing, gold, time or
        sound. `xeenTemplePricesKnown`'s table gate becomes state-backed Temple
        pricing; `xeenTownServiceKnown` admits Clouds Temple carry-over, while
        Food and every other untabled refusal stays. Darkside selection is a
        rule only, without admitting play.

        Departure to settlement: `departSmith` destroys `SmithContinuation`, but
        the Temple screen lives until the Event settlement completes and may
        be redrawn there (for example a `BeforeEventSettlement` retry). `SmithUi`
        keeps the visit's published row and revision from admission until
        settlement ends; in that window the accessor returns them after
        checking that the owner's row and revision still equal them. No quote,
        payment, Uncurse or Donation is possible then, and settlement
        completion discards them with `SmithUi`. Before admission the prepared
        row is the source, as already described.
      - **Lifetime and stale work.** Load and New Game do not touch the owner,
        so its revision cannot detect old-session work. That protection is the
        existing one: `playGameplay` destroys the Flow and session owners
        before the next session, plus the session tickets. The revision only
        catches a publication between a visit's preparation and its use,
        which with one exclusive visit at a time would be a defect. Developer
        entry modes start at zero in a fresh process; inspection performs no
        assignment. Row and revision stay outside `XeenSaveState`, save
        bytes, restore guard preimages and their `XeenMutation` ranges.
        Restore never replays Temple admission. Save v16/schema 9/content 14,
        M44 digests, RNG order and timing stay unchanged.
      - **Tests.** Unit: zero initialization, every table assignment,
        carry-over and stale revision rejection. Flow: fresh-owner map 74 at
        zero; Rivercity and Shangri-La carry-over through New Game and panel
        Load; redraw during preparation showing the visit's row; quotes and
        text agreeing; Uncurse charging `20*L` with the zero row; zero
        Donation changing no counter, gold, time or sound; refused, failed
        and stale admission leaving the row unchanged; a failing
        `AfterAdmission` hook leaving the row published and the visit usable.
        Save bytes from the same restored snapshot with and without a prior
        Temple visit (excluding the visit's own effects) are identical. One
        original-data Temple test at 74 `(4,4)`; the census marks it
        supported. Title, New Game and the Temple go into the implementation
        review checklist if `Application`'s loop is not testable.
      - **Docs in the same change.** Update the `XeenTempleHeal.h` comment
        that says MMModern does not model the carry-over, and point the
        `sv-temple-map74-carry` quirk at `XeenTemplePriceState`.
      - **Separate cleanup commit.** Remove the `XeenMapIdentity(28)` default
        arguments in `XeenDialogView.h`, `XeenFoodBank.h`,
        `XeenLearnedSpellRules.h` and `XeenTraining.h` (callers pass the map;
        `XeenRestoreGuard`'s map 28 text admission is not a service rule and
        stays). `xeenTrainingCap` throws `invalid_argument` outside Clouds and
        loses `noexcept`, with a test. Remove README's Part C Temple caveat.
- [ ] Developer test save `m79-playtest`, starting in Rivercity: members
      below each cap with the XP to train, partial spell books, guild
      awards for some members only, one cursed item, one broken item and
      injuries, 100,000 gold and 5,000 gems, Town Portal and Lloyd's Beacon
      for travel to 49, 73 and 74; README entry.
- [ ] Regenerate the census; update its calibration if needed.

## Acceptance

- Unit tests: each per-place rule, the fallbacks, Guild membership
  refusal, depleted stock in shops 1-3 restoring exactly.
- Original-data tests: Rivercity Training to the cap; a purchase in the
  Shangri-La Ironworks from its own shop; the Bank at 74; a
  Guild purchase in Nightshadow, Asp or Winterkill.
- Map 74's Temple: zero prices in a fresh program, the last tabled Temple's
  prices after a visit, kept across New Game and Load, absent from the save.
- Sign In, Dismiss and actions 6 and 8-12 still refuse without changing
  state, time or random draws.
- The census lists every action 0-5 site as supported. One complete suite
  per `tools/AGENTS.md`; M44 digests unchanged.
- Maintainer play-test with `m79-playtest`: each town and each service kind
  through the original screens, with mouse and keyboard.
