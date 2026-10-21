# MMModern and ScummVM: behavior differences

MMModern is an open-source reimplementation of the World of Xeen engine (Might
and Magic IV: Clouds of Xeen and V: Darkside of Xeen) that aims to play exactly
like the original DOS game. It builds on ScummVM: it links ScummVM's Xeen
archive and sprite code and its Common, Graphics and Image libraries, and
ScummVM's Xeen engine, the fruit of years of reverse-engineering by its
contributors, is MMModern's reference for how the game works.

Where a rule is unclear or the original seems to differ, MMModern checks the
English World of Xeen CD edition itself: by playing it in DOSBox, and by
read-only static analysis of the original program (`WORLD/XEEN.DAT`), accepted
only when calibrated against known behavior and of high confidence. This page
lists, in player terms, every place where that evidence made MMModern behave
differently from ScummVM. It is not a criticism: ScummVM supports far more
games, editions and platforms, and several items are deliberate workarounds
documented in its source. Each item links to its entry in
[original quirks](original-quirks.md), with the evidence, dates and offsets.

**Revisions.** MMModern pins ScummVM `6814ee9b` (2026-09-06). Every item was
rechecked against ScummVM master `55895ab0` on 2026-10-16, except the new
terrain/Tavern/Temple/recovery/spell/movement/entrance entries explicitly marked "pin: differs; master not
rechecked". At that check, since the pin the
Xeen engine has had only a code-style edit and the Xeen data in `mm.dat` is
unchanged apart from its version number, so the earlier entries are marked
"master: differs". MMModern currently plays Vertigo and the surrounding
mainland; many items apply there, others wait for areas not open yet.

## Combat and conditions

- **Holy Bonus adds weapon damage.** DOS applies it to melee and Shoot,
  before Adventurer's damage multiplier; the pin only displays it.
  [Evidence](original-quirks.md#sv-holy-bonus-damage). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Day-spell counters.** DOS copies current caster level's low byte into
  the Day spells' flags/counts, including Levitate; the pin uses true/1.
  [Evidence](original-quirks.md#sv-day-spell-counters). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Spell effect counter limits.** DOS stores the four combat buffs as bytes
  and Light as a wrapping word; the pin's live integers retain larger values.
  [Evidence](original-quirks.md#sv-spell-effect-widths). Pin: differs; master not rechecked. Extreme-value reachability in ordinary play unverified.
- **Clairvoyance's right statue.** The DOS hint-0 cycle uses frames 8..11;
  the pin uses 9..12.
  [Evidence](original-quirks.md#sv-clairvoyance-right-frames). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Enchant's level and empty result.** DOS uses the caster's level and
  accepts a generated enchantment with no properties; the pin uses the
  initially selected recipient's level and adds an error for an empty result.
  [Evidence](original-quirks.md#sv-enchant-caster-and-copy). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Special weapon IDs in item spells.** DOS Enchant and Item to Gold
  exclude ID exactly 34; the pin also excludes every higher ID.
  [Evidence](original-quirks.md#sv-item-spell-special-id). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Recharge on broken items.** DOS fills an eligible broken item to 63
  charges; the pin adds its normal 1..6 charges with the same cap.
  [Evidence](original-quirks.md#sv-recharge-broken). Pin: differs; master not rechecked. Rule established; implemented in M76.
- **Town Portal items in combat.** DOS continues from the refusal notice
  into its normal town dialog and can load the selected town; the pin returns.
  [Evidence](original-quirks.md#sv-town-portal-item-combat). Pin: differs; master not rechecked. Implemented in M77 Part A; DOSBox presentation remains unobserved. Combat ends at the first attacker re-evaluation after arrival, with its ordinary minute, by the maintainer's decision.
- **Rest's cell restriction.** DOS checks cell bit `0x40`, including on
  the mainland; the pin's cell test is ineffective. MMModern reads the DOS
  attribute bit, and Super Shelter bypasses both ordinary Rest restrictions.
  [Evidence](original-quirks.md#sv-rest-cell-flag). Pin: differs; master not rechecked.
- **Suppress Disease without Disease.** DOS still applies zero-point healing,
  which can clear Unconscious at positive HP; the pin skips it. Both subtract
  3 from Disease counts of at least 4. Implemented in M75 Part A.
  [Evidence](original-quirks.md#sv-suppress-disease-zero). Pin: differs; master not rechecked.
- **Suppress Poison's refund and reduction.** DOS refunds 4 SP on cancel and
  subtracts 3 from counts of at least 4; the pin refunds 1 and subtracts 2.
  Implemented in M75 Part A.
  [Evidence](original-quirks.md#sv-suppress-poison). Pin: differs; master not rechecked.
- **Resurrection cancellation.** DOS refunds 125 SP/20 gems, including from
  its item prompt; the pin uses Raise Dead's 50 SP/10 gems. Implemented
  in M75 Part B.
  [Evidence](original-quirks.md#sv-resurrection-refund). Pin: differs; master not rechecked.
- **Revival at zero Endurance.** DOS byte arithmetic changes 0 to 255 before
  testing the minimum; the pin gives 1. Ordinary-play reachability is not
  established. Implemented in M75 Part B, including this settled edge case.
  [Evidence](original-quirks.md#sv-revival-endurance-byte). Pin: differs; master not rechecked.
- **Create Food overflow.** DOS wraps its food counter modulo 65536; the pin
  retains a wider total in memory. Implemented in M75 Part A.
  [Evidence](original-quirks.md#sv-create-food-wrap). Pin: differs; master not rechecked.
- **Poison Volley and negative hit totals.** DOS treats the final accuracy
  total as an unsigned word, so a negative total can become a hit; the pin's
  signed comparison misses. This belongs to the shared player accuracy
  helper. **Implemented** for melee and Shoot (M74 Part B) and Poison Volley
  (M74 Part C).
  [Evidence](original-quirks.md#sv-hit-total-unsigned). Pin: differs; master not rechecked.
- **Holy Word against the wrong species.** DOS enters the miss path for a
  non-undead target; the pin skips it. Neither damages the target or draws
  a damage-stage save. The call difference is confirmed; its audible result
  remains unplayed. **Implemented (M74 Part B)**; audio remains deferred.
  [Evidence](original-quirks.md#sv-holy-word-miss). Pin: differs; master not rechecked.
- **Frostbite's initial sound call.** The pin adds FX8 before the learned
  spell's projectile; DOS makes no such initial call. The item variant
  matches. Both Frostbite projectiles are implemented; audio remains deferred
  and unverified.
  [Evidence](original-quirks.md#sv-frostbite-initial-fx). Pin: differs; master not rechecked.
- **Incinerate from an item.** DOS uses different sound/projectile selectors
  from the learned spell, while the pin shares the learned handler. Both
  prepare 250 Fire damage. MMModern uses item projectile 2 and learned
  projectile 1. Audio remains deferred; DOS presentation has not been played.
  [Evidence](original-quirks.md#sv-item-incinerate-selectors). Pin: differs; master not rechecked.
- **Controlled monsters recover on the DOS roll.** Recovery succeeds at a roll
  at most the monster resource ID; the pin reverses the inequality. Equality
  succeeds in both. The core scans every map slot before queued shots; learned
  casts and item Use apply controls (M73 Parts A/B). Pin: differs; master not rechecked.
  [Evidence](original-quirks.md#sv-monster-recovery).
- **Terrain damage starts fresh for each character.** Lava and surface falls
  use their full starting damage for every party member in DOS. The pin
  carries earlier members' damage reductions into later members.
  [Evidence](original-quirks.md#sv-terrain-damage-start). Pin: differs; master not rechecked.
- **Darkside fall boundaries.** The original uses the direct map-minus-88
  fall only for maps 89-112 and excludes 130-132 from its special dispatch.
  MMModern keeps these bounds; Darkside play remains outside the admitted area.
  [Evidence](original-quirks.md#sv-darkside-fall-bounds). Pin: differs; master not rechecked.
- **First save against poison and other elements.** The original's first save
  against a monster's non-physical attack (such as the poison of Vertigo's
  Slime and Doom Bug) uses Luck and level; only later saves use the element's
  resistance, which ScummVM uses for both. The original also draws one extra,
  invisible random number in that loop.
  [Evidence](original-quirks.md#sv-poison-first-save),
  [extra draw](original-quirks.md#sv-element-save-extra-draw). Master: differs.
- **Poison and disease wearing off.** Every eight hours in the original, a
  poisoned or diseased character may shake it off with a resistance roll, or
  it worsens. ScummVM runs the check only for characters who are *not*
  poisoned or diseased, so time never cures them.
  [Evidence](original-quirks.md#sv-eight-hour-poison-disease). Master: differs.
- **In Love, Insane and Drunk.** In the original, In Love raises every
  attribute, Insane raises Might and Speed but lowers Intellect, Personality
  and Accuracy, and Drunk raises Personality and Luck but lowers the rest.
  ScummVM lowers every attribute for In Love and Drunk, and Might and Speed for
  Insane. [Evidence](original-quirks.md#sv-condition-modifiers). Master: differs.

## Monsters

- **Break Weapon** also unequips the broken weapon in the original; ScummVM
  deliberately leaves it equipped (a workaround noted in its source).
  [Evidence](original-quirks.md#sv-break-weapon). Master: differs.
- **Eradication** also unequips the victim's weapons, armor and accessories,
  which stay in the pack, broken; ScummVM keeps them equipped.
  [Evidence](original-quirks.md#sv-eradicate-items). Master: differs.
- **Monsters scripted into slots the map lacks** are written but never shown or
  used in the original; ScummVM makes them live. After the party leaves
  Vertigo, ScummVM's town gains two monsters the original never has.
  [Evidence](original-quirks.md#sv-spawn-beyond-count). Master: differs.
- **Outdoor terrain.** The original never lets a monster, flying or not, onto
  five terrain types; ScummVM compares them with the map's limit instead. This
  matters on one Clouds outdoor map.
  [Evidence](original-quirks.md#sv-outdoor-monster-terrain). Master: differs.
- **Walls.** A monster passes a wall only below the map's limit, the party's
  own rule; ScummVM also lets it through walls equal to the limit, on several
  Clouds and Darkside maps but none in Vertigo.
  [Evidence](original-quirks.md#sv-monster-wall-threshold). Master: differs.

## Items and treasure

- **Loading leaves dormant loot in the running DOS process.** Pending loot is
  absent from DOS saves. Loading clears its currency/readiness but retains the
  current staged items, even if acquired after that save; a fresh launch starts
  empty. The pin clears all staged items on Load.
  [Evidence](original-quirks.md#sv-loot-load-process-state). Pin: differs; master
  not rechecked. Not implemented yet.
- **Event treasure and pending monster loot.** In the original, an Event reward
  delivers staged monster items too, and with monster gold pending it delivers
  them twice; MMModern keeps this original bug. The pin saves and restores
  pending treasure around Events only when the bitwise `gold & gems` is nonzero,
  so for example 5 gold and 2 gems are not saved. [Evidence](original-quirks.md#sv-event-loot-stash). Pin:
  differs; master not rechecked.
- **Unused monster drop fields.** No Clouds monster uses drop fields 7-21; the
  pin gives them a zero chance, while MMModern refuses such a record as not
  supported yet, before any draw. No Clouds record is affected; Darkside's Doom
  Knight and Sandro use field 7 and are refused until Darkside gameplay.
  [Evidence](original-quirks.md#sv-unused-drop-fields). Pin: differs; master
  not rechecked.
- **Miscellaneous monster drops.** A monster can drop a miscellaneous item,
  such as a wand, in the original; ScummVM stages a copy of an accessory slot
  instead and loses it. [Evidence](original-quirks.md#sv-misc-monster-loot). Master: differs.
- **Who receives loot.** When no suitable member has room, the original also
  gives items to sleeping, unconscious and dead members; ScummVM skips
  disabled and dead members.
  [Evidence](original-quirks.md#sv-loot-recipients). Master: differs.
- **One kind of pack full.** If no one has room for another weapon, say, the
  original replaces the first character's last weapon with it; ScummVM
  discards the new item.
  [Evidence](original-quirks.md#sv-category-full-replace). Master: differs.
- **Generated items.** Shops, treasure and drops use the original's random
  draws, and charged items get 3 to 10 charges. ScummVM numbers categories and
  subtypes from 0, skips a draw and gives 1 to 8 charges, so different items
  appear. [Evidence](original-quirks.md#sv-makeitem-draws). Master: differs.
- **Identify** lists the classes that can use the item in the original;
  ScummVM's list tests another item and the owner's class.
  [Evidence](original-quirks.md#sv-identify-classes). Master: differs.
- **Broken and cursed weapon names** show only the prefix and base type in the
  original (Identify still shows the real properties); ScummVM keeps the
  suffix. [Evidence](original-quirks.md#sv-broken-weapon-names). Master: differs.
- **Discarding the Xeen Slayer Sword.** The original refuses to discard it
  with its own message; ScummVM lets it go. Rule established in M77 Part 0;
  implemented in M77 Part A in exploration and combat.
  [Evidence](original-quirks.md#sv-discard-xeen-slayer). Pin: differs; master not rechecked.

## Magic

- **Pathfinder movement.** DOS requires two Pathfinder holders to cross outdoor
  wall types 3 and 6; the pin permits them without the skill. MMModern implements
  the DOS wall consumer and counts holders regardless of condition.
  [Evidence](original-quirks.md#sv-pathfinder-movement). Pin: differs; master not rechecked.
- **Pain** does 8 physical damage to the first monster group in the original,
  with normal resistance and save; in ScummVM it does nothing.
  [Evidence](original-quirks.md#sv-pain). Master: differs.

## Events

Most of the scripts concerned are in areas MMModern has not opened yet.

- **The Vowelless Knight.** ScummVM accepts either spelling of one riddle
  answer, a documented workaround for a data error; the original, and
  MMModern, accept only the spelling in the data.
  [Evidence](original-quirks.md#sv-vowelless-knight). Master: differs.
- **The Dragon Tower mirror.** The original mirror entry points to the wrong
  place; ScummVM corrects it, MMModern keeps it.
  [Evidence](original-quirks.md#sv-dragon-tower-mirror). Master: differs.
- **Password prompts naming a missing text** use the map's first text in the
  original, an empty text in ScummVM.
  [Evidence](original-quirks.md#sv-confirmword-text0). Master: differs.
- **Locked chests.** The original decides from the chest's picture and never
  locks the map's first object; ScummVM uses another test, so some chests
  differ, three of them in Clouds.
  [Evidence](original-quirks.md#sv-chest-locks). Master: differs.
- **Clearing an Event spot.** A script that clears the party's spot keeps
  running in the original; ScummVM stops it.
  [Evidence](original-quirks.md#sv-make-nothing-here). Master: differs.

## Services

- **Rivercity Food** fills to 60 rations per active member for 50 gold in DOS;
  the pin fills to 50, with the same offer text.
  [Evidence](original-quirks.md#sv-rivercity-food). Pin: differs; master not rechecked.
- **Newcastle's Temple prices** retain the last Temple's surcharges and
  Donation price in the DOS process, even across New Game or Load; a fresh
  process starts with all five zero. The pin resets them on every visit.
  [Evidence](original-quirks.md#sv-temple-map74-carry). Pin: differs; master not rechecked.
- **Food on an untabled Clouds map** skips the offer assignments in DOS;
  the pin assigns 15 rations per member for 10 gold and text 75. No original
  Clouds Tavern uses this path, and its exact DOS text is not established.
  [Evidence](original-quirks.md#sv-untabled-food). Pin: differs in control flow; master not rechecked.
- **Tavern tips cycle through the original list.** DOS advances the map's
  index modulo 15 on Clouds; the pin divides and repeats the early tips.
  The original also selects the proper tip group for Clouds town 30.
  [Evidence](original-quirks.md#sv-tavern-tip-cycle). Pin: differs; master not rechecked.
- **Repeated Temple purchases each add a day.** Two successful Heal/Uncurse
  actions cost three days on departure in DOS; the pin charges only two.
  Very large visit totals also keep DOS's word arithmetic and signed stock
  regeneration test.
  [Evidence](original-quirks.md#sv-temple-visit-time). Pin: differs; master not rechecked.
- **Raising the dead.** The original leaves the hit-point part out of the
  Temple's Heal price below zero HP, so raising a dead character usually costs
  10 gold per level less than in ScummVM.
  [Evidence](original-quirks.md#sv-temple-negative-hp). Master: differs.
- **Attribute-material prices.** The original adds the material's attribute
  bonus; ScummVM reads another table out of range, so buying, selling and
  repairing these items cost a different amount.
  [Evidence](original-quirks.md#sv-attribute-material-price). Master: differs.
- **Fix and Identify prompts** appear swapped in ScummVM, whose English text
  orders the two verbs unlike the CD.
  [Evidence](original-quirks.md#sv-fix-identify-order). Master: differs.
- **Huge Bank amounts** are refused in the original; ScummVM reads the entry as
  a signed number and handles them differently.
  [Evidence](original-quirks.md#sv-bank-amount-entry). Master: differs.

## Presentation

- [Sphinx refusal sample](original-quirks.md#sv-sphinx13-descriptor): DOS
  plays all of `sphinx13.voc` despite its damaged descriptor, preserving its
  2.67-second speech interval. The pin rejects its header and skips that
  interval. Pin: differs; master not rechecked.
- [Entrance speech cadence](original-quirks.md#sv-scene-speech-cadence):
  Reaper and Dwarf speech use two BIOS ticks per iteration; loops without
  explicit waits use the DOSBox-observed two retraces at 70 Hz. The pin
  uses `WAIT(1)`. DOS retreats on Reaper refusal and draws Sphinx retreat
  indices 0..8; the pin retreats on Reaper success and draws Sphinx 0..7.
  Installed DOS defaults show no captions; Effects-off captions remain
  deferred with audio. Pin: differs; master not rechecked.

- **Control panel and saves.** MMModern uses the original panel, Save As
  included, and its ten-slot chooser; ScummVM rearranges the panel and uses its
  own save dialog, shared by all its games.
  [Evidence](original-quirks.md#sv-control-panel). Master: differs.
- **Combat health colors** change at a quarter of maximum HP in the original,
  at half in ScummVM. [Evidence](original-quirks.md#sv-combat-name-colors). Master: differs.
- **Danger Sense animation** runs in phase with the Spot Secret Doors
  indicator in the original; ScummVM draws it one frame ahead. Rule
  established in M77 Part 0 and implemented in M77 Part C.
  [Evidence](original-quirks.md#sv-danger-sense-frame). Pin: differs; master not rechecked.
- **Quests and Awards scrolling** uses the Up and Down arrows (and keypad 8
  and 2) and the on-screen buttons in the original; ScummVM also accepts U
  and D in Quests and only U and D in Awards. Rule established in M77 Part 0
  and implemented in M77 Part B.
  [Evidence](original-quirks.md#sv-quest-award-scroll-keys). Pin: differs; master not rechecked.
- **Choosing a member who cannot act** brings a refusal and a new choice in the
  original; ScummVM waits silently.
  [Evidence](original-quirks.md#sv-whowill-refusal). Master: differs.
- **Reward sparkles** also mark Event gifts of gold, gems, food, SP, armor
  class and levels, and changes of sex, race or class; ScummVM sparkles for
  fewer gifts. [Evidence](original-quirks.md#sv-event-sparkles). Master: differs.
- **Speaking shopkeepers.** The Clouds Smith and trainer animate while
  speaking; ScummVM omits the Smith's animation and ties the trainer's to the
  opposite sound state.
  [Evidence](original-quirks.md#sv-service-speaking-animation). Master: differs.
- **Talking faces** move exactly as long as the CD speech lasts; ScummVM uses a
  countdown and reads the cue times differently.
  [Evidence](original-quirks.md#sv-cd-speech-timing). Master: differs.
- **Pacing.** Treasure messages, portrait damage flashes and the party's death
  keep the original's pauses and order; ScummVM's differ in length or order.
  [Treasure](original-quirks.md#sv-treasure-waits),
  [damage](original-quirks.md#sv-damage-waits),
  [death](original-quirks.md#sv-death-fire-pacing). Master: differs.
- **Spell lists** have no dead line between rows: a click between two spells
  selects one of them, while ScummVM's rows leave a one-line gap.
  [Evidence](original-quirks.md#sv-spell-row-height). Pin: differs; master not rechecked.
- **Indoor minimap edges.** The original fills the minimap's partial left
  column and top row along their whole length and keeps the party arrow on top
  of the party's own cell walls; ScummVM draws only one edge tile on each side
  and draws the arrow under those walls.
  [Evidence](original-quirks.md#sv-indoor-minimap-edges). Pin: differs; master not rechecked.
- **Scenery animation** includes its last frame, which ScummVM skips.
  [Evidence](original-quirks.md#sv-scenery-terminal-frame). Master: differs.
- **Original text and layout.** Item charge titles (without a stray glyph),
  the spell list's first row and balance, the Info screen's title and Walk on
  Water line and the Bank's wording come from the CD program; ScummVM's source
  itself notes its "Withdrawl" misspelling.
  [Charges](original-quirks.md#sv-charges-glyph),
  [spells](original-quirks.md#sv-spell-list-format),
  [Info](original-quirks.md#sv-info-title),
  [Bank](original-quirks.md#sv-bank-wording). Master: differs.

## Rare cases

These follow the original too, but are rarely or never visible in play; all
still differ in master. Event values: award removal, Walk on Water and Wizard
Eye values, Levitate and day tests, negative HP/SP tests, TakeOrGive's third
pair, and AlterEvent operands above 127 or after a CD cue
([1](original-quirks.md#sv-award-removal), [2](original-quirks.md#sv-script-values),
[3](original-quirks.md#sv-hp-sp-unsigned), [4](original-quirks.md#sv-takeorgive3),
[5](original-quirks.md#sv-alter-event-operand), [6](original-quirks.md#sv-alter-event-playcd)).
Wrapping counters: skills past 255 ([skills](original-quirks.md#sv-skill-byte-wrap)),
age after 255 Aging hits ([aging](original-quirks.md#sv-aging-wrap)) and a
Temple Eradicated counter above 65 ([Temple](original-quirks.md#sv-temple-eradicated-wrap)).
Curse Item against the Xeen Slayer Sword ([Curse](original-quirks.md#sv-curse-item-slayer)).

## Approved deviations

Deliberate differences from the original, approved by the maintainer and kept
apart from the fidelity differences above.

- **No "phantom" steps.** Releasing a movement key stops the party after the
  step in progress; the original queues up to ten repeats and keeps walking. A
  tap still moves one step. [Details](original-quirks.md#dev-no-phantom-steps)
- **Saves record changes, not whole maps**, with the same result in play.
  Saves are not compatible with the original games or ScummVM.
  [Details](original-quirks.md#dev-map-state-deltas)
- **CD speech timing** uses exactly the interval the original requests, with
  no guessed drive delay. [Details](original-quirks.md#dev-cd-speech-delay)
- **Spawn animation.** A spawned monster's first frame is chosen without
  affecting the game's random sequence.
  [Details](original-quirks.md#dev-spawn-frame-random)
- **Mirror list bounds.** The mirror accepts only the CD list's destinations;
  the original could read one entry past it.
  [Details](original-quirks.md#dev-mirror-bound)
- **One password prompt.** A Clouds prompt with only an alternate answer always
  takes the ordinary success path; in the original an uninitialized value
  could rarely pick another. [Details](original-quirks.md#dev-confirmword-alternate)
- **No "Please Wait..." box.** Maps load without the original loading box,
  as in ScummVM. [Details](original-quirks.md#dev-no-please-wait)
- **When the treasure recipient choice clears.** The member a WhoWill dialog
  chose clears on arrival over an Event cell, on Space and in every Event run,
  but not on each later command while standing on an Event cell or after
  combat, as in the original and ScummVM.
  [Details](original-quirks.md#dev-whowill-event-passes)

## What ScummVM has that MMModern does not yet

The [M80 DOS check](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)
also establishes two differences for the Arena, which awaits Part B.
Comparison is with pinned `6814ee9b`, not a new master check:

- [Arena species pool](original-quirks.md#sv-arena-species-pool): DOS chooses
  five species first, then assigns monsters to those slots; the pin chooses
  a species for each monster.
- [Arena ordinal suffix](original-quirks.md#sv-arena-ordinal): DOS displays
  21st/22nd/23rd, while the pin uses `th` for every count above nine.

ScummVM plays the whole game today; MMModern is young. The full list of gaps is
in [project status](project-status.md#known-gaps):

- **The whole game:** only Vertigo and the surrounding mainland are playable;
  the rest of Clouds of Xeen and all of Darkside of Xeen are not.
- **Sound:** no music, effects or CD speech yet (speech keeps its duration in
  silence); no intros, ending or Mr Wizard's help.
- **Services and party:** no Inn, recruiting, character creation, Tavern
  Sign In or Dismiss.
- **World:** no entrance scenes (Reaper, Golem, Dwarf Mine, Sphinx, Pyramid);
  Run does not work indoors; not every monster drops items yet; Mountaineer
  has no effect.
- **Platforms and editions:** MMModern runs on 64-bit Windows with the English
  World of Xeen CD; ScummVM covers many platforms, editions and languages, and
  Swords of Xeen.

Next steps are in the [roadmap](roadmap.md).
