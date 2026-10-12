# Milestone 60 - Magic

**Tier A. Status: Part A completed and accepted by the maintainer on 2026-10-12;
Part B remains pending.** Part A implementation baseline:
`a6003643cbc3053282ba328ef9c653ec62e61955` on `m60-part-a`. The Part A acceptance
and implementation are recorded together in this branch's commit.

Part A delivers original Cast/New and Guild Buy/Info dialogs, common spell rules,
shared Quick Fight selection and no-charge unsupported casts. The maintainer
accepted the Debug play-test after the Guild member-selection and Spell Info
layout corrections. Independent Tier A plan and implementation reviews passed;
`build-rel` fast tests passed 149/149 and the complete suite passed 183/183 through
the prescribed runner. All three M44 digests remain unchanged; save stays v11,
schema 9, content 14. Part B's eight effects and common projectile/arrival
integration are not implemented or accepted by this result. The remaining plan
retains their scope and acceptance criteria.

## Goal and authority

Replace the temporary casting screens with the original Cast dialog, buy and
inspect spells at Vertigo's Guild, and implement Wizard Eye, Shrapmetal, Energy
Blast and Jump plus Cure Wounds, Flying Fist, Revitalize and Pain
([roadmap](roadmap.md#near-term), step 6). The maintainer chose this option
"(a+)" on 2026-10-11. The prerequisite condition-rule
correction recorded in [original quirks](original-quirks.md) (eight-hour
Poison/Disease and condition modifiers) landed before Part A implementation.
Read with [project status](project-status.md), the closed [M56](milestone-56-plan.md),
[M57](milestone-57-plan.md), [M58](milestone-58-plan.md), [M59](milestone-59-plan.md)
plans and [original quirks](original-quirks.md).

Read-only investigation used the clean pinned ScummVM checkout at
`6814ee9ba54582f5b5adcffab49efbbd8f589edd` ([provenance](dependencies.md)):
`engines/mm/xeen/dialogs/dialogs_spells.cpp` (CastSpell, SpellsDialog, SpellOnWho,
SelectElement), `dialogs_query.cpp::Confirm`, `locations.cpp::GuildLocation`,
`spells.cpp`, `combat.cpp::rangedAttack/attack/attack2`, `party.cpp`,
`interface.cpp`, `interface_minimap.cpp`, `character.h`, and
`devtools/create_mm/create_xeen/constants.cpp` (costs, books and Guild catalogue).
Installed `WORLD/XEEN.DAT` was inspected read-only; no commercial bytes are copied
into the repository. Normal DOSBox evidence overrides this reference.

## Investigation: current engine

- `XeenLearnedSpellRules` maps three 39-slot books to global spell IDs. Cleric/
  Paladin use Clerical, Sorcerer/Archer Wizardry, Druid/Ranger Druidic; other
  classes cannot cast. CHR learned bytes are retained, with any nonzero byte
  meaning learned. Resource spell names come from `DARK.CC/spells.xen`.
- `XeenCastingUi` is a project-made keyboard-only caster/list/confirmation/result
  flow with six visible rows and technical messages. It is a stopgap, including
  the extra confirmation and enemy-selection stages, not the original dialog.
  Its exploration text still says ten minutes even in Vertigo. `XeenCastingFlow`
  actually charges one minute on map 28 and ten otherwise; replace the map-number
  distinction with the reference indoor/outdoor property when integrating M60.
- Exploration supports Awaken and First Aid; combat supports those plus Magic
  Arrow. All cost zero gems; Awaken/First Aid cost 1 SP, Magic Arrow 2 SP.
  First Aid selects a party member, heals 6 up to normal maximum, clears
  Unconscious with positive HP, and fails on Dead/Stoned/Eradicated. Awaken
  clears Sleep for the active party (and Unconscious with positive HP).
  Magic Arrow currently requires a selected contact enemy and deals 8 after
  its magic gate. Its restriction to combat/contact is a stopgap: the reference
  projectile path also works in exploration and searches forward rows.
- Before paid commitment, Escape changes no costs/time/RNG. First Aid target
  Escape refunds SP but still spends the combat action or exploration time;
  a dead target spends the cost and action/time. Combat uses its acting member
  and participants; no separate per-cast minutes. Exploration settles time,
  condition/economy consequences, Cartography and actor work through existing
  continuations. Healing effects themselves use no RNG. Magic Arrow skips its
  `1..100+level` gate if magic resistance is zero, otherwise requires roll >=
  resistance; it then draws `1..50+monster resource ID` even though that saving
  throw does not change its 8 damage. Retry guards retain draws and publication.
- M55 remembers the selected book slot in mutable `currentSpell` (255 means
  none), shared with Quick Fight. Manual list selection already updates it.
  Quick Fight has a separate full cost table and checks SP, gems and forbidden
  contexts before support; ordinary casting conflates support/SP refusals.
  Consolidate these rules, preserving original supported-spell refusal order.
- Learned books for all 30 roster owners, `currentSpell`, gold/gems, actors,
  position, calendar and RNG already serialize. `XeenGameplayContext.effects`
  stores levitation, automap, Wizard Eye, clairvoyance, water walking, blessing,
  shield, holy bonus and heroism (indices 0..8); `lightAndResistances` stores
  light, torches and four party resistances. M51 reset logic already clears Eye
  and temporary effects. M58 minimap only reads exploration today; Eye rendering
  is missing. M59 Guild entry/membership/departure exist; Buy/Info are stopgaps.

## Investigation: original dialogs and Guild

- Cast uses window 10, `cast.icn`, C/Cast, N/New and Escape, with original
  button rectangles `(234,108)-(259,128)`, `(261,108)-(285,128)` and
  `(288,108)-(312,128)`. Show caster, remembered spell, SP/gem cost and current
  SP. Exploration starts with the last runtime caster, otherwise the last
  character with spells; F1-F6 and portraits switch caster in both Cast and
  New. Combat fixes the acting caster and ignores caster-switch input.
- New uses window 25, text window 37, `scroll.icn` and ten rows, in learned-book
  order. Digits 1..9/0 or matching row clicks select; S/Enter/keypad Enter or
  Select accepts; Escape cancels selection. Up/Down scroll one row, PageUp/
  PageDown and their mouse regions scroll ten. Keep original row colors,
  scroll limits, pressed frames and nested Escape behavior. Selection stores
  the book slot, even without casting; opening/cancelling the list does not.
  Last-caster is runtime UI state; do not create a second saved spell selection.
- The installed DAT supplies Cast `[0x546da,0x54760)`, Guild text/options
  `[0x54760,0x547e2)`, list headings/rows `[0x54af3,0x54bdd)`, Info
  `[0x54bec,0x54c12)`, purchase `[0x54c12,0x54c4a)`, learned-all
  `[0x54a63,0x54a8e)`, noncaster `[0x54a8e,0x54aa4)`, no-known-spells
  `[0x54ab4,0x54ad1)`, and SP/gold labels `[0x54bdd,0x54bec)`.
  Extend the bounded DOS reader with validated formats/control streams, not
  hardcoded text. The DOS list's first row uses tab 011 and its balance `%lu`,
  unlike the reference constants (tab 010 / `%u`); preserve DOS layout.
  Names and descriptions remain resource-driven (`spells.xen`, `spldesc.bin`).
- Guild B/Buy Spells and S/Spell Info reuse the list, filtered by class and
  the map's catalogue. Buy omits learned spells; Info includes learned ones.
  A row immediately opens purchase/description, rather than Cast's Select flow.
  Confirm uses window 22 and `confirm.icn`, Y/N and mouse buttons at
  `(120,133)` / `(176,133)`; Info is acknowledgement only, never a purchase.
  Membership is per selected character (Vertigo award 0), rechecked after
  portrait changes, not a party-wide unlock. Buy entry also tests `noActions`;
  Info entry does not. Retain noncaster/max-SP, no-offers and insufficient-gold
  behavior. No invented level, alignment or caster-condition restriction on
  individual offers beyond the reference's entry and refresh checks.
- Vertigo's 18 IDs are `1,10,20,26,27,38,40,42,45,50,55,59,60,61,62,68,72,75`;
  the two 77 entries in `CLOUDS_GUILD_SPELLS[0]` are sentinels. Use this original
  order and book intersection, not a list limited to implemented spells.
  Purchase price is `100 * fixed SP cost`, or `500 * abs(negative cost)` for
  level-scaled spells, doubled for Paladin/Archer/Ranger; no gem purchase fee
  and no level scaling of gold price. Purchase sets that owner's learned slot
  to true and debits carried gold once. Books already persist; there is no stock
  depletion shared across characters. No transaction RNG/time; the existing
  one-day Guild departure still settles time, possible stock RNG and interest.

## Option (a+): two implementation parts

**Part A - original Cast and Guild, common rules.** Replace both stopgap Cast
screens, share list rendering/input with Buy/Info, and keep Party/roster as book
owner, Service as Guild transaction owner, Combat as combat owner, Event/Flow as
interaction and arrival owner. Extend existing candidates and guards; no new
coordinator. Centralize global ID/book-slot conversion, current-level SP/gem
costs, context/support checks and target descriptors. Preserve M55 Quick Fight
selection and ensure unsupported Quick Cast also spends no turn. Purchases and
selection changes are guarded against stale input; disabling Save spans all
dialogs, payment, effects, projectiles, arrival and settlement, until quiet.

Every unimplemented spell remains purchasable and selectable under original
rules. Attempting it says **"not supported yet"**, without effect, SP/gem debit,
time, combat turn or spell-caused RNG; do this before paid commitment, including Quick Fight.
This is the project's explicit incomplete-engine behavior, not an original
spell failure. A remembered selection may change independently of casting.
For implemented spells, insufficient SP precedes insufficient gems; forbidden
combat spells return to selection with no net charge/turn. SpellOnWho uses
window 16, F1-F6/portraits, and original refunds on Escape; remove the extra
project confirmation/result menus. Supported failed casts and post-cost target
cancellation still spend one action (or 1 indoor/10 outdoor minutes); opening
and cancelling Cast costs nothing. `Interface::perform` may settle already-due
monster movement before opening Cast (`_tillMove`); retain that entry work
independently. Measure no-charge/no-draw refusals from the settled dialog entry,
not by erasing previously due actor work. Keep scheduling and no redraw RNG.

**Part B - eight effects and common projectile/arrival integration.** Share ranged
spell delivery with combat/actor scene lanes and the existing shooting flow;
extend Magic Arrow to the same original exploration/forward-target behavior.
This extends the existing Magic Arrow effect through common infrastructure. Preserve
original projectile/splat resources, target order, XP/loot and combat admission.

| Effect | Rules, contexts, costs and failure paths |
| --- | --- |
| Cure Wounds (10) | Exploration and combat, 3 SP + 0 gems; selected member heals 15 up to the normal maximum, with First Aid's target, dead-state failure and Escape-refund rules (parameterize the existing First Aid path). No draws. |
| Revitalize (59) | Exploration and combat, 2 SP + 0 gems; selected member loses Weak and gets `addHitPoints(0)` (positive-HP Unconscious repair). No resurrection, no invented dead-target failure. No draws. |
| Flying Fist (27) | Exploration and combat, 2 SP + 0 gems; single target, 6 physical damage through the shared ranged path, with the magic gate and saving throw below. |
| Pain (50) | Exploration and combat, 4 SP + 0 gems; 8 physical damage against the first group (up to three monsters), like Shrapmetal's traversal. ScummVM prepares zero damage; DOS prepares 8 (original program analysis, `WORLD/XEEN.DAT` `0x33558`, `0x2f5e9`, `0x2cd81`). |
| Wizard Eye (75) | Exploration only, 5 SP + 2 gems, party-wide, no target or intrinsic draw. Set `effects[2]=1`, `effects[1]=0`. Reveal the local minimap without granting Cartography or writing M58 explored bits; full Map stays exploration-based. Toggling automap on temporarily suppresses Eye display without clearing its flag. It lasts until `resetTemps` (including Rest, the eligible new-day reset at/after 05:00, and year advance), not a fixed elapsed-minute timer. Recast sets true, not a stacked duration. Normal cast settlement may record only the occupied cell through existing Cartography rules; Eye reveal itself must never record cells. |
| Shrapmetal (60) | Exploration and combat, current level SP + 0 gems; `2 * currentLevel` physical damage, `RT_GROUP`. Hit up to three monsters in the first populated reachable forward row, then stop, even if all resist; never all rows/groups. Use the selected contact ordering in combat, original wall/terrain projectile blockers and forward row traversal outside contact. No base-damage roll; per target magic gate, physical reduction and saving throw as below. No target/blocked projectile still spends cost and action/time. |
| Energy Blast (20) | Exploration and combat, current level SP + 1 gem; `2 * currentLevel` energy damage, logical `RT_SINGLE`: first selected/reachable monster only. Same projectile admission and empty/blocked paid-cast behavior. No damage roll; magic gate and saving throw still apply because ranged delivery calls `attack(..., RT_GROUP)` even for a single projectile. Do not reuse character damage resistance as a monster spell rule. Elemental resistance does not reduce it (DOS-confirmed). |
| Jump (40) | Exploration only, 4 SP + 0 gems, party moves exactly two cells forward, facing unchanged, no intrinsic RNG or target prompt. Outdoors: sample offset 7 (one ahead), reject wall 1; sample offset 14 (two ahead), require nonzero surface ID and wall != 1. Indoors: both current and one-ahead directional `MONSTER_GRID_BITMASK` lookups must be zero. It is not two ordinary walks and must not acquire extra swimming/mountaineering checks. Blocked cast says Spell failed, retains cost and normal cast time; combat refusal spends neither. |

For the damaging spells, evaluate targets in reference order. A nonzero magic
resistance draws `1..100+currentLevel` and blocks when roll < resistance; zero
skips that draw. Accepted nonzero damage reaches the resource-ID saving throw
`1..50+ID`, succeeds at <= ID, and halves Shrapmetal, Energy Blast, Flying Fist
and Pain damage. For the physical spells, apply physical resistance before
saving-throw halving. Keep
the draw even when physical immunity has reduced damage to zero after entry
to `attack2`. Retain detached RNG candidates across yields and publish impacts,
deaths and rewards once. The pin's `getMonsterResistance` starts its spell-path
damage contribution at zero: energy resistance does not reduce the base there.
Original program analysis (2026-10-11) confirms this DOS rule; see
[original quirks](original-quirks.md).

Jump must publish relocation once and enter the ordinary post-cast destination
sequence: time/actor work in reference order, `doStepCode` terrain consequences,
physical map transition, automatic Event/contact combat, light/Cartography
stepped update. Do not trigger the intermediate cell. Successful and blocked
casts leave `ctr24` unchanged: Cast never calls walking `stepTime`. Audit ordering
against `Interface::perform/chargeStep/doStepCode`
and the existing navigation/Event continuations. Unsupported destination effects
use the existing visible unsupported behavior; no per-cell admission lists.
If faithful arrival on the admitted maps requires a missing hazard system beyond
this scope, stop and amend the plan instead of silently dropping consequences.

## Remaining catalogue

The remaining seven Vertigo spells stay deferred, all
purchasable and visibly "not supported yet" when cast: **Light** (needs darkness
and drain rules), **Sleep** (needs a monster sleep status and a save change),
**Insect Spray**, **Protection from Elements**, **Sparks**, **Toxic Cloud** and
**Suppress Poison** (DOS refunds 4 SP on cancel and subtracts 3, unlike the
reference; see [original quirks](original-quirks.md)). Sparks and Toxic Cloud
follow the same DOS-confirmed resistance rule as Energy Blast.

## Save, risks and acceptance

- **Option (a+) retains envelope v11, schema 9, content 14.** Eye is already
  `effects[2]`, including its reset semantics; no invented duration field.
  Books, selection, purse, positions and damage all reuse existing saved meaning.
  Keep exact generic save/load round-trip, including an active Eye, automap mode,
  purchased unsupported spell, all 30 books, calendar and next RNG continuation.
  Do not serialize transient dialogs/projectiles or replay arrival on load.
- If implementation discovers a necessary layout/meaning change, amend this plan
  before changing format; increment v11 to v12, reject older saves clearly, and
  use [tools/m44-digest-audit.ps1](../tools/m44-digest-audit.ps1) for the M44 gate.
  Explain decoded-field and trace differences and obtain the required digest
  replacement approval; never regenerate expected hashes without the audit.
- Independent Tier A **plan and implementation reviews** are mandatory. Review
  the combined casting/Guild ownership and each implemented part, including
  changes from DOS evidence, before calling implementation complete.
- Rules tests: all catalogue/book intersections/prices, level-scaled SP, exact
  SP/gem balances, disabled/noncaster/member cases, learned filtering, no offers,
  refusals and cancellation, chosen member/enemy/group, zero targets, blocked
  projectiles, exact draw intervals/order/count and half-damage boundaries.
  For Flying Fist and Pain, verify physical resistance before saving-throw
  halving; with zero physical resistance, successful saves reduce 6 to 3 and
  8 to 4 respectively.
  Test Quick Fight support/refusal parity and currentSpell with changed party order;
  successful/blocked Jump at `ctr24=23` must leave it 23.
- UI tests: resource-backed Cast/New/Buy/Info/On Who, all row/scroll/select/exit
  mouse and keyboard paths, portrait caster switching in exploration, fixed
  caster in combat, nested cancellation, refresh after purchase and original
  DAT formatting. No debug technical messages in the player flow.
- Exactly-once tests: stale/double input, resource preparation failure, retries,
  projectile impact, grouped deaths/rewards, purchase debit+learn, SP+gem debit,
  refund, Jump relocation/Event/encounter, time settlement and Save exclusion.
  Reuse existing casting, service, mapping and generic persistence tests; no
  process-continuation witness per spell.
- A few original-data runs: New Game -> Vern -> Guild -> buy an unlearned spell
  -> inspect learned/unknown entries -> depart -> save/load; cast each selected
  effect in every allowed context; exercise first-group versus farther enemies,
  one-target Energy Blast, blocked/successful Jump and Eye expiry without reveal
  bits; buy/cast an unimplemented spell and prove unchanged costs/turn/RNG.
- During implementation: affected tests and fast suite on `build-rel`, then full
  build and complete suite after each task's final production build through
  [tools/AGENTS.md](../tools/AGENTS.md). Debug
  closure uses its fast suite and three M44 scenarios, with logs. No build/test
  is authorized by this planning task. Maintainer Debug/DOSBox play-test and
  explicit acceptance are required before changing the status to completed.

## DOS questions

None open. Original program analysis on 2026-10-11 settled monster elemental
resistance (unchanged base damage), Pain (8 physical, group) and the Magic Arrow
saving-throw draw (kept). The maintainer's acceptance play-test should still cast
Eye before/after Rest and across 05:00, toggle `=` and inspect Map; cast
Shrapmetal and Pain with near and far groups; Jump with each of the two edges
blocked and onto an Event cell; heal with Cure Wounds and Revitalize; and
compare Guild offers/prices for a full caster and an eligible hybrid.
