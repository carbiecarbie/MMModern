# Milestone 76 plan - Buffs, information, item and travel spells

**Tier A. Status: planned.** Fourth and last milestone of step 6 of the
[systems arc](roadmap.md#next-arc-finishing-the-games-systems), after
[M75](milestone-75-plan.md).

## Goal

The 22 spells still missing, from the spell book in exploration and combat,
Quick Fight and charged items, so that all 76 Clouds spells work:

| Group | Spells | Item powers |
| --- | --- | --- |
| Party buffs | Bless, Heroism, Holy Bonus, Power Shield, Protection from Elements, Day of Protection, Day of Sorcery | 22, 30, 25, 29, 14, 49, 50 |
| Senses and movement | Light, Levitate, Walk on Water, Clairvoyance | 1, 20, 32, 18 |
| Information | Detect Monster, Identify Monster | 34, 23 |
| Items | Enchant Item, Recharge Item, Item to Gold | 63, 53, - |
| Travel | Etherealize, Time Distortion, Super Shelter, Teleport, Lloyd's Beacon, Town Portal | 58, 39, 48, 42, 28, 55 |

Each spell also needs the state it sets to **do** what the original does:
the party buffs change combat, the senses change movement, terrain and the
screen, and the border shows the original indicators. The spell dialogs
come with them: Select Element, Detect Monster, Identify Monster, the
Items dialog modes, Teleport, Lloyd's Beacon and Town Portal.

Out of scope: combat Equip/Use and the other missing screens (step 7), map
admission and the dark view of dark maps (step 11, decision 4), audio,
Darkside-only behavior.

## Why Tier A

- **Save format.** Lloyd's Beacon stores a beacon per character (map,
  position, side). MMModern reads those fields into the immutable original
  character details and does not save them, so the format changes:
  envelope v15, older saves rejected as usual. Schema 9 and content 14 stay;
  changing either needs a plan amendment first. Party buffs, light count
  and the party resistances are already saved
  (`XeenGameplayContext::effects` and `lightAndResistances`), so they add
  no format change.
- **Combat rules.** Bless, Heroism, Power Shield and the party resistances
  enter the hit and damage formulas used by every fight.
- **Timing and position.** Super Shelter runs Rest under a different mode;
  Etherealize, Time Distortion and Teleport relocate the party on its map,
  and Lloyd's Beacon and Town Portal can change map.

## Design decisions

1. **DOS check first (Part 0).** Read-only static analysis of
   `WORLD/XEEN.DAT`, calibrated as in M74 and M75, before any code. It
   settles, for the 22 handlers and their consumers:
   - each handler's guard, value, prompt, refund on cancel and combat
     admission, from the book and from items (the item list refuses powers
     16, 21, 28, 42, 48, 55 and 58 in combat in the pin; Town Portal's item
     entry differs in DOS);
   - where each buff is read: Bless in armor class, Heroism in the hit
     chance, Power Shield after monster damage, the party resistances in
     monster and trap damage, and **Holy Bonus**, which the pin only shows
     and never applies;
   - Rest's order: the temporary reset clears every buff first, whatever
     the food, so the pin's later per-member clearing of four buffs is
     redundant; and the dawn reset;
   - Super Shelter against ordinary Rest: one 480-minute call or
     interruptible steps, the map restriction, the dying-risk warning and
     cancel, food and reset order, and arrival after Levitate ends;
   - Light's stepped mark, and whether mainland 23 or Vertigo has
     light-drain cells (the pin decrements the light count on them,
     whether or not the map is dark);
   - Walk on Water and Levitate in movement and terrain arrival;
   - Clairvoyance's statues, the border indicators and their frames;
   - the Detect Monster and Identify Monster screens;
   - the Enchant, Recharge and Item to Gold formulas, refusals and draws;
   - for each travel spell, **relocation or map reload**: the pin's Time
     Distortion and Teleport only set a position on the current map, and
     Lloyd's Beacon reloads only when map or side change; plus direction,
     arrival processing, combat continuation, the dialogs, destinations
     and map restriction flags.
   Results go to [magic](rules/magic.md), [evidence](rules/evidence.md) and
   [original quirks](original-quirks.md). DOS overrides the pin; cases that
   stay unsettled go to the maintainer.
2. **One generic owner per mechanic.** Buffs live on the existing gameplay
   context; their readers go into the existing combat, movement, terrain and
   presentation rules, not into the spell handlers. Item spells reuse the
   M61 Items dialog modes. A same-map relocation never reloads the map, so
   live monsters keep their HP and status; a map change reuses the existing
   transition used by the mirror and terrain falls, with the M69 per-map
   state.
3. **Lloyd's Beacon is mutable roster state.** Each character's beacon
   moves out of the immutable original details into the roster owner, with
   save capture and restore, state equality and mutation protection, so
   stale prepared work cannot publish over a changed beacon.
4. **Darkness moves to step 11** (maintainer decision, 2026-10-18).
   Mainland 23 and Vertigo are not dark, so the dark view changes nothing
   playable yet and is tested best on a real dark map when one opens. On
   maps that are not dark, Light is complete as a counter: it increments
   the light count and marks the party stepped, as in the pin. If Part 0
   finds light-drain cells on the open maps, their decrement (a rule, no
   rendering) stays in M76, since Info shows the count; otherwise drain
   cells go to step 11 with the dark view.
5. **Destinations not open yet refuse** before any SP, gem, charge, time or
   random draw is spent, with "not supported yet", as Jump and the mirror
   do. Town Portal reaches Vertigo; the other four towns refuse. Lloyd's
   Beacon can be set anywhere the original allows, but returning to a map
   not open yet refuses.
6. **Combat items wait for step 7.** Combat Use is not available yet, so
   item powers in combat (admission and effects) are covered by automated
   tests only; learned casting in combat, Quick Fight and item Use in
   exploration are play-tested.
7. **Census.** Regenerate [the coverage report](coverage.md) after each
   part; at closure all 76 spells are supported.

## Parts

Each part ends with a passing complete suite, its own commit and a report;
a session may cover more than one part.

- **Part 0 - DOS check** (read-only): the questions above, recorded in the
  rules chapters. Stop rule: any finding that changes this plan's scope is
  reported before Part A1 starts, and the plan is amended first.
- **Part A1 - party buffs** (7): Bless, Heroism, Holy Bonus, Power Shield,
  Protection from Elements with Select Element, Day of Protection and Day
  of Sorcery, their combat and trap effects and the border indicators.
- **Part A2 - senses and information** (6): Light (decision 4), Levitate,
  Walk on Water, Clairvoyance and its statues, Detect Monster and Identify
  Monster.
- **Part B - item spells** (3): Enchant Item, Recharge Item, Item to Gold,
  through the original member prompt and Items dialog modes.
- **Part C1 - shelter and relocation** (4): Super Shelter, Etherealize,
  Time Distortion and Teleport.
- **Part C2 - map travel and save v15** (2): Lloyd's Beacon and Town
  Portal, with the beacon's roster state and save format.

## Risks

- **Digest changes.** Two kinds, kept apart. Gameplay changes: buffs can
  move a recorded scenario only if it casts or receives one (Temple
  Donation sets four). Format changes: the baseline digests hash raw save
  files, so v15 changes them in Part C2 even with identical gameplay.
  Either kind needs a byte, trace and field audit against a rebuilt
  baseline and the maintainer's approval of replacement digests before
  the part's suite can pass and be committed (lesson from M75).
- **Rest and travel.** Super Shelter must not change ordinary Rest (M51,
  M72); a wrong reload would reset monsters, and no move may publish partly.
- **Scope.** 22 spells is large; a part that finds more work than planned
  stops and is split, as in M74.

## Acceptance

- All 22 spells work from the book in exploration and in combat where the
  original allows, and from Quick Fight, with the original prompts,
  dialogs, refusals, refunds and costs. Their item powers work in
  exploration; in combat they are proved by automated tests (decision 6).
- Bless, Heroism, Power Shield, Protection from Elements and Holy Bonus
  (as Part 0 settles it) change combat and traps as in DOS; Rest and dawn
  clear them in the DOS order; the border shows the original indicators.
- Walk on Water and Levitate change movement and terrain as in DOS.
- Super Shelter tests: its Rest against ordinary Rest's interruption, the
  map restriction, cancel at the dying-risk warning, food and reset order,
  and arrival after Levitate ends.
- Same-map relocation keeps live monsters' HP and status; cross-map
  Lloyd's Beacon and Town Portal keep the M69 per-map state.
- Each character keeps their own beacon across save and load, exactly, with
  no replayed events, time or random draws; stale prepared work over a
  changed beacon is rejected; v14 saves are rejected with the older-build
  message.
- Unit tests for every rule; original-data end-to-end tests for a buffed
  fight, Super Shelter, Time Distortion on the mainland and a beacon return
  across save and load; a developer test save (README) with casters who
  know all 22 spells and charged items for their powers.
- Complete `build-rel` suite passes for each part; independent review of
  the implementation; maintainer play-test.
