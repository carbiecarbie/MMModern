# Items and equipment

This chapter covers inventory, equipment, Smith transactions, usable charges,
generation and reward delivery. Evidence labels follow the [reference index](README.md).
The edition and baseline are identified in [evidence](evidence.md). Rules marked
Reference only are not promoted to DOS facts by their presence in this guide.

## Representation and inventory order

**Evidence: Reference only**, with the Misc record layout independently confirmed
by the DOS item-use and monster-loot paths described below. Sources:
[item.h][itemh], `XeenItem`, `ItemState`, `InventoryItems`;
[item.cpp][item], `synchronize`, `sort`, `removeItem`.

Each character has four separate nine-slot categories: weapons, armor,
accessories and miscellaneous. A record has material, ID, state and frame.
For equipment, ID is its base type and material selects metal, element or
attribute bonus. For Misc, material is the container and ID is the usable power.
Power, container, learned-book slot and global spell ID are different namespaces.

State has a six-bit counter, a cursed bit (`0x40`) and a broken bit (`0x80`).
The counter is an effectiveness selector for weapons and charges for Misc.
An equipment frame denotes a worn slot; zero is unequipped. `isEquipped`
does not itself mean usable or unbroken. A caller may test those separately.
Generic empty-item tests use ID; the original Misc treasure-capacity check instead
uses its container field, as the staging evidence shows.

Inventory sorting is stable compaction: move each next occupied record into the
earliest hole, preserving relative item order. It is not alphabetical or by value.
Removing equipment clears its frame unless cursed. Discard clears the record and
compacts; cancellation leaves the record. The reference's `discardItem` returning
true after refusal/cancellation is a caller convention, not proof that an item
was removed.

## Equipment slots and restrictions

**Evidence: Reference only.** Source: [item.cpp][item],
`WeaponItems::equipItem`, `ArmorItems::equipItem`, `AccessoryItems::equipItem`,
`InventoryItems::passRestrictions`; [character.cpp][character], `itemScan`.

| Category and IDs | Slot/frame | Capacity and conflict |
| --- | --- | --- |
| Weapons 1-17 | One-handed melee / 1 | Refuse if frame 1 or 13 already occupied |
| Weapons 30-33 | Missile / 4 | One missile weapon; independent of melee slot |
| Other admitted weapons | Two-handed melee / 13 | Refuse existing melee or shield frame 2 |
| Armor 1-7 | Body / 3 | One body armor |
| Armor 8 | Shield / 2 | One; conflicts with two-handed weapon frame 13 |
| Armor 9 | Helm / 5 | One |
| Armor 10 | Boots / 9 | One |
| Armor 11-12 | Cloak, cape / 10 | One shared slot |
| Armor 13 | Gauntlets / 6 | One |
| Accessory 1 | Ring / 8 | Two |
| Accessory 2 | Belt / 12 | One |
| Accessories 3-7 | Medal / 7 | Two across these types |
| Accessories 8-10 | Neck / 11 | One shared slot |

Class restrictions are table-driven and tested for the specific item/category.
Do not derive proficiency from price, character level or a material name.
The original Identify display's proficiency computation is independently
confirmed to enumerate classes for the selected ID/category; the pinned
Identify helper passes the wrong arguments. See [original quirks](../original-quirks.md).

Equipping does not automatically remove a conflicting item. The original asks
the player to remove it first. Passive attribute/resistance scanning generally
requires an equipped, non-bad item, but weapon damage and effectiveness have
their own tests; see [combat](combat.md). Do not replace all these call-site rules
with one broad claim that a broken item has no effect. **Implemented (M67):**
combat, Shoot and Rest accept every material 0-130, effectiveness counter 0-6 and
broken/cursed state; the Journey check refuses only records outside those tables.

## Smith prices

**Evidence: DOS static (retained), high confidence.** The original price routine
is file `0x3c67d`; category and mode dispatch, tables and DOSBox calibration are
recorded in [original quirks](../original-quirks.md), including the
[attribute-material check](../original-quirks.md#sv-attribute-material-price). Source comparison:
[ItemsDialog::calcItemCost][dialogs].

For weapons, armor and accessories, start with the category's base price `B`.
For material 37 divide `B` by 10; for 38 or 40 divide by 4; for 39 divide by 2.
For materials 41-58 multiply by `METAL_BASE_MULTIPLIERS[material-37]`.
Keep each integer division at that position. Then:

- Elemental material 0-36 adds `100 * ELEMENTAL_DAMAGE[material]`.
- Attribute material 59-130 adds `100 * ATTRIBUTE_BONUSES[material-59]`.
- Metal materials have no separate elemental/attribute addend.

The attribute addend is the DOS rule. The pin incorrectly indexes
`ELEMENTAL_DAMAGE[material-52]`, eventually outside that table. It is unsafe
to reproduce the pin's price routine verbatim.

For Misc, value is `container cost[material] + power cost[id]`.
Charges do not affect price. Broken/cursed flags and a weapon's effectiveness
counter do not change the computed value; action admission can still refuse it.

| Action | Final gold price |
| --- | --- |
| Buy | `max(1, value)` |
| Sell without effective Merchant | `max(1, floor(value/2))` |
| Sell with effective Merchant | `max(1, value)` |
| Identify | `max(1, floor(value/100))` |
| Fix | `max(1, floor(value/10))` |

Effective Merchant is `(skill byte & 0x7f) != 0`: 0 and 128 receive half;
1, 127 and 255 receive full value. Fix and Identify ignore Merchant.
The DOS and reference Fix callers differ, but both price routines overwrite
the argument with the fixed divisor selection.

Examples calibrated against retained Smith observations:

- Plain weapon ID 1 has value 50: Buy 50, ordinary Sell 25, Merchant Sell 50,
  Identify 1 and Fix 5.
- Plain armor ID 1 has value 20 and Fix 2.
- Armor ID 10 has base 40; material 38 gives `40/4=10`, then Fix `10/10=1`.
- A material-1 weapon adds 200 to its base price before the action divisor.
  A 50-gold base becomes 250; ordinary Sell is 125, Identify 2 and Fix 25.

These formulas describe admitted records and the verified price ranges.
They do not establish DOS behavior for corrupted IDs, invalid array indices or
arbitrary arithmetic overflow. Table addresses: weapon `0x4fa2e`, armor
`0x4fa74`, accessory `0x4fa90`, container `0x4faa6`, power `0x4fb7c`,
attribute bonus `0x4f9e6`; action divisors at `0x4d880`.

## Ironworks stock by map

**DOS static (2026-10-20), high confidence.** Clouds stock selection scans
the four entries of `BLACKSMITH_MAP_IDS`: map 28 -> shop 0, 30 -> shop 1,
73 -> shop 2, 49 -> shop 3. Any other map (including 29, 31, 32 and 74)
falls back to shop 0. The same selection is used when copying stock into
the merchant's temporary character and when writing it back.

Buy, Sell, Identify and Fix use the shared price routine above, with no map
factor. The map changes the available item records, not the price of an
identical record with identical action and Merchant skill. Pinned
`party.cpp::BlacksmithWares::getSlotIndex` and
`dialogs/dialogs_items.cpp::calcItemCost` agree on map selection and price
independence; the previously documented attribute-material discrepancy
still applies. [Offsets](evidence.md#other-town-services-2026-10-20).

**Darkside: calibrated DOS static, high confidence (2026-10-31).** Maps
29/31/37/43 select stock slots 0/1/2/3 respectively, from the second row of
the same table; an unmatched map selects 0. Both stock selectors use the
side (`0x326c0..0x32709`, `0x32944..0x32985`, table `0x4d6d0`). Prices
remain the shared rules above. This agrees with the pin. M102 Part A implements Darkside purchases and
exact restoration of depleted stock in all four shops, without regenerating it.
[Darkside evidence](evidence.md#darkside-towns-m102-part-0).

## Buying, selling, identifying and repairing

**Evidence: Reference only** for transaction sequencing and refusals unless
explicitly qualified below. Source: [ItemsDialog::doItemOptions][dialogs].

| Action | Admission and confirmation | Successful mutation |
| --- | --- | --- |
| Buy | Refuse full destination category; confirm quote; require carried gold | Debit once, transfer the entire stock record with frame reset to 0, clear source, compact both inventories |
| Sell | Refuse cursed item; also refuse weapon ID at or above Xeen Slayer Sword (34); confirm quote | Remove and compact, add quote to carried gold; the reference does not append the sold record to shop stock |
| Identify | Confirm and require carried gold | Debit and display detailed properties; no durable identified flag |
| Fix | Require broken; confirm and require carried gold | Debit and clear only broken, preserving curse, counter, material, ID and frame |

**DOS play (retained, 2026-10-12):** broken items can be sold at normal value,
identified with their full properties and repaired. No lasting identified
state was observed. Cursed Sell refusal remains reference-only (Discard's is
[DOS static](#discard-recharge-and-enchant)); the maintainer's ordinary play
did not reach a cursed item.

**DOS static (retained):** broken/cursed weapon names hide both material and
effectiveness suffix while retaining bad-state prefix and base name; Identify
still shows actual properties. The pin hides material but retains the suffix.
The DOS confirmation-verb table is ordered Identify then Fix; the pinned English
table swaps them. See [original quirks](../original-quirks.md) for formatter and
proficiency offsets.

**DOS play (maintainer screenshots, 2026-10-19):** item lists draw the Cursed
prefix in yellow and Broken in red, the pin's `\f09` and `\f32` before the line's
own color (`getFullDescription`). The Discard prompt uses the same formatter
with color 4: "Broken" red and the rest of the name white, and a plain item's
whole name white. **Reference only:** the pin passes it with color 15 to the
Buy, Sell, Fix/Identify, Identify, No Need of This and No Special Abilities
prompts; MMModern follows it there too (`XeenItemDescription::coloredName`).

**Reference only:** Smith departure costs one day even when the player cancels
all transactions. Individual transactions do not themselves advance time.
Source: [BaseLocation::show][locations], after the Smith dialog returns. This is a service-exit
charge, not one day per purchase.

## Item Use and cancellation

**Evidence: DOS static (2026-10-12), high confidence for the traced Use path;
DOS play (retained)** confirms no time advance, failed Jump spending a charge
and disappearance on the last charge. Full anchors and calibration are in
[spell and item control flow](evidence.md#spell-and-item-control-flow).

The normal sheet action requires a nonzero current map ID and an actionable user. Its condition
dispatch refuses Asleep, Paralyzed, Unconscious, Dead, Stoned and Eradicated.
The combat-use guard is separate from the spell-specific combat restrictions.
Require a nonzero power, positive low-six-bit charge count and no broken/cursed
bit. Selecting Use is not equivalent to learning or casting a book spell.

After admission, decrement the charge, select the item owner as caster, close
the item windows and dispatch the power. If the charge is exhausted, remove
and compact the item after the effect returns. Clear shooting state and perform
the normal monster response. A supported failure does not restore the charge.
There is no ordinary learned-cast time charge or initial SP/gem debit here.
See [all 73 power mappings](magic.md#item-power-lookup).

**An original cancellation quirk:** use a First Aid item (power 4), then Escape
at its member prompt. The charge remains spent and the original adds **1 SP to
the item user**, despite not having charged SP. This addition has no maximum-SP
clamp. The last-charge item still disappears. Its gem addition is zero because
First Aid costs zero gems.

The traced chain is admission `0x313ce`, charge `0x3145c`, caster assignment
`0x3146d..0x31470`, dispatch `0x3149e -> 0x39e4b -> 0x39e79 -> 0x2f3f5`,
First Aid's prompt ID 26 at `0x2f3fa`, Escape `0x2f146`, refund `0x2f34b`.
This shares the learned-spell refund helper without a check for item origin.
It is not evidence that every cancelled item effect grants the same amount:
each effect can supply a different prompt/refund ID or use a different dialog.
Other routes require their own trace. Cancellation before choosing an item
does not enter this sequence.

**Implemented (M61 Part C):** First Aid item Escape grants this uncapped 1 SP
to the user and spends the charge. Other item powers gain no new refund; their
complete original cancellation paths were not established by M61. The later
[M75 DOS checks](evidence.md#party-healing-spells) establish refunds for
powers 26, 27, 37, 41, 44, 56, 57 and 66: each uses its spell's own ID and
cost, including gems; powers 51 and 73 have no member prompt. These refunds
are implemented in M75 for all those member-prompt powers; Create Food (51)
and Divine Intervention (73) have no member prompt. M76 also implements
own-ID refunds for Protection from Elements (14), Enchant (63) and Recharge
(53), including their gems, while retaining the spent item charge. The
[item spell actions](magic.md#item-spell-actions) distinguish early member
cancellation from later Items cancellation.

## The Items dialog in combat

**Evidence: DOS static (2026-10-18), high confidence** for every path below;
offsets and calibration are in [M77 evidence](evidence.md#m77-combat-items-quests-awards-and-skill-indicators).
The pinned reference agrees unless a difference is stated.

The Items dialog is one routine with a mode argument. Two callers reach it in
combat: the main-screen **Use** button (combat loop handler `0x2e8e3`, mode 7)
and the character sheet's **Items** button (mode 0, `0x3a462..0x3a486`). Mode
7 only selects the Misc category first and sets an internal "combat Use" flag;
the dialog then behaves as mode 0 (`0x3a669..0x3a695`). Both therefore show
the same buttons: Weapons, Armor, Accessories, Misc, Equip (Use on the Misc
page), Remove, Discard, Quest and Exit, and the same key table (`0x3b921`).

| Action | Combat rule | Refusal and text |
| --- | --- | --- |
| Equip (`E`) | Allowed for Weapons, Armor and Accessories; ignored on the Misc page. Same routine as in exploration (`0x37b83`): class restriction, slot conflicts, two-handed and ring/medal limits. No weapons-only or other combat restriction exists. | The ordinary proficiency and "remove X to equip Y" texts |
| Remove (`R`) | Allowed; clears the equipped frame (`0x31119`). | Cursed item: "You cannot remove a cursed item!" |
| Discard (`D`) | Allowed; confirmation prompt, then removal and compaction (`0x31018`). | Cursed item: "You cannot discard a cursed item!"; weapon ID 34 (Xeen Slayer Sword): "Don't you think you'll need the Xeen Slayer to kill Lord Xeen!" |
| Transfer (select a row, then F1-F6) | From the character sheet only, including in combat: the count and order come from the **combat party** (`0x971f`, `0x9720`) while the mode byte is 2, otherwise from the active party. The main-screen Use dialog ignores F1-F6 (`0x3b4b1..0x3b4b7`). Source cursed bit refuses; a full destination category refuses; otherwise the item lands unequipped in the destination's last slot and both lists compact. | "You cannot remove a cursed item!"; "%s's weapons/armor/accessories/misc backpack is full." |
| Use (`U`) | Only from the main-screen Use dialog on the acting member's turn. From the character sheet in combat the internal flag (`0x97c6`, set with the sheet while the mode byte is 2 and cleared at sheet exit) refuses after the map and condition tests. | Map 0: "What's your hurry? Wait till you get out of here!"; Asleep, Paralyzed, Unconscious, Dead, Stoned or Eradicated: "%s is not in any condition to perform actions!"; sheet in combat: "To use an item in Combat, invoke the Use command on your turn!"; no power, no charge or broken/cursed: "%s has no special abilities!" |
| Quest (`Q`) | Opens the Quests screen from either caller (`0x3b401`). | - |
| Exit, Escape | Returns to the caller with nothing changed. | - |

**Turn cost.** Only an admitted Use ends the acting member's turn: the item
routine clears shooting, runs the monster response and sets the turn cursor
to "nobody" (`0x314c9..0x314dd`), and the combat handler then restores the
saved index and advances to the next actor (`0x2e909..0x2e91b`). Equip,
Remove, Discard, transfer, Quest and Exit leave the cursor untouched, so the
same member still acts; they pass no time and give monsters no opportunity.
Depressed and Confused members may still Use (`0x31fee`). A Use refused at
admission, or a dialog left without Use, costs nothing. An admitted Use spends
the charge before the effect and ends the turn even if the effect is
cancelled; the dialog never refunds the charge, and there is no SP or gem cost
([Item Use](#item-use-and-cancellation)).

**Town Portal from an item in combat** passes its combat notice and continues
into the town dialog and the load ([M76](../original-quirks.md#sv-town-portal-item-combat)).
After the load the item routine finishes as any Use: monster response on the
new map, turn cursor cleared, dialog closed, next actor. The combat loop ends
only when its attack-slot test finds no attackers (`0x2ea99..0x2eaf0`); those
slots are rebuilt by the scene setup (`0x14413`, `0x16939`), not by the map
load, so the exact moment combat ends after arrival is **Unknown** statically.
Maintainer decision (2026-10-19): MMModern ends combat at the first attacker
re-evaluation after the load ([M77 evidence](evidence.md#open-questions-from-this-check)).

## Discard, recharge and enchant

**Evidence: DOS static (2026-10-18), high confidence** for Discard's refusals
and confirmation (`0x31018..0x31110`), with retained **DOS play** for ordinary
Discard's selection, permanent-discard confirmation, Yes removal and No
preservation. **Reference only** for Enchant's source listing below. Sources:
[InventoryItems::discardItem/enchantItem][item],
[ItemsDialog::doItemOptions/itemToGold][dialogs].

Discard refuses cursed items before confirmation (state bit `0x40`,
`0x31034..0x31054`). A weapon with ID exactly 34, the Xeen Slayer Sword, also
refuses with the fixed text "Don't you think you'll need the Xeen Slayer to
kill Lord Xeen!" (`0x31061..0x3106f`); the pin lacks this test
([quirk](../original-quirks.md#sv-discard-xeen-slayer)). Broken alone is not a
refusal. Declining the confirmation changes nothing. Removing an equipped
cursed item also refuses (`0x31119..0x311ad`).

**DOS static, high confidence; implemented (M76 Part B):** the three item
spells use their own-ID member prompts, then Items modes 6/4/11. Member
Escape refunds SP/gems, including for a spent item charge; later Escape or
eligibility refusal keeps the cost. See [item spell actions](magic.md#item-spell-actions)
and [M76 evidence](evidence.md#m76-spells).

Enchant draws `R(1, floor(caster current level/5)+1)`, capped at 5, before
eligibility. Weapons and armor require material and state zero; equipped
items qualify, and only weapon ID exactly 34 is excluded. The shared DOS
generator runs in forced weapon/armor mode, including the category,
subcategory and base-ID draws. Only material and state are copied; even a
zero-property result succeeds. Accessories and Misc refuse after the level draw.

Recharge admits nonempty Misc containers at material IDs at most 9, except
power 53. One `R(1,6)` is added, capped at 63. The DOS comparison uses
`state & 0xbf`, so a broken item fills to 63; a cursed one adds normally.
Both flags are preserved, and there is no break/destruction draw.

Item to Gold converts immediately without confirmation or an equipped,
broken or cursed refusal. Weapon ID exactly 34 refuses; Quest counters are
not selectable. It credits `max(1, full DOS item value)`, ignoring charges
and condition, then clears and compacts the category. The Smith price
formula above is shared. These DOS rules supersede the pin's Enchant
recipient-level and zero-property checks, ID-at-least-34 exclusions, and
six-bit-only Recharge comparison.

## Generation and reward delivery

**Evidence: DOS static (2026-10-16), high confidence** for `makeItem` draws,
selection and table ranges, with **DOS static, high confidence** for the staging
and delivery rules below. Source program: `WORLD/XEEN.DAT` SHA-256
`8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`,
`makeItem` file `0x25ca8..0x26382`, random helper `0x45b9a..0x45bb4` calibrated
against the Event currency draw `0x23770..0x23791`. Reference sources:
[Character::makeItem][character],
[Combat::attack2][combat], [Party::giveTreasure][party]. The exhaustive low-tier
generation scope is summarized in [M61's catalogue](../archive/milestone-61-plan.md#goal-and-final-scope).

Generation level 1 permits weapons 1-33, armor 1-7 and Misc containers 1-9;
no accessories. Level 2 expands armor through 13 and includes accessories 1-10.
Generated Misc powers are 1-15 at level 1 and 16-30 at level 2, with charges
`R(1,8)+2`. Ordinary level-1 equipment has no material or state bonus. Level-2
equipment may receive one of the material/effectiveness branches; generation
does not simply draw uniformly over all material IDs. Generation consumes its
draws before later capacity-related loss.

`makeItem` first draws category `R(1,100)`, then subcategory `R(1,100)` at
levels 1-5 or `R(1,80)` at levels 6-7 (`0x25cbc..0x25ce3`). At level 1,
category rolls 1-40 select weapons, 41-85 armor, and 86-100 Misc. At higher
levels, 1-35 select weapons, 36-60 armor, and 61-100 use subcategory thresholds
to select armor, accessories or Misc. Weapon subcategory bands end at 30, 60
and 85; the mixed branch ends at 10, 20, 35, 45, 55, 65, 75 and 80.
The comparisons are inclusive (`0x25d45..0x25e38`), so Misc is unreachable
through the ordinary random path at levels 6-7. An Event ItemType override
still consumes both initial draws (`0x25ce5..0x25d45`).

Every item then consumes an enchantment `R(1,100)` roll. Eligible equipment
consumes a selector `R(1,100)` and a table-bounded value roll; level-7 material
items consume the selector even though it does not change their fixed material
row (`0x25e4a..0x26216`). Enchanted weapons test `R(1,20)==10` for an
effectiveness roll `R(1,6)` (`0x262ae..0x262c6`). Misc items draw a
level-bounded power and charges `R(1,8)+2` (`0x26218..0x2625a`). There is no
curse draw in `makeItem`. The original interval tables at segments `0x47b4`,
`0x47bb`, `0x47c5`, `0x47c8` and `0x47c9` match the shared numeric generation
tables; the pin differs at the four draw rules recorded in
[original quirks](../original-quirks.md).

**DOS static (2026-10-16), high confidence; implemented (M71):** combat loads
the monster drop field (record offset `0x2d`) at `0x2c36c..0x2c370`, checks
its chance with `R(1,100)` at `0x2c38b..0x2c39a`, then reloads that same
field at `0x2c3e2` and passes it as makeItem's level. The far call at
`0x2c3e7` resolves to file `0x25ca8`. Staging capacity is checked afterward
(`0x2c402` onward). All supported sources share `XeenItemCandidate`, including
level-1 monster drops. **Implemented (M81 Part C):** every monster record
generates at its own drop field, levels 1-6, accessories included.

<a id="monster-drop-draws"></a>**Monster drop draw counts (DOS-confirmed, M81).**
Every nonzero drop field consumes one chance draw, even at 100%; field zero
none. After a successful chance draw makeItem consumes its draws as above:
fixed IDs skip the base-ID draw, singleton random ranges still consume one.
Level-1 equipment costs 4 draws and Misc 6; at levels 2-6 armor and
accessories cost 5 or 6, weapons 7 or 8 and reachable Misc 6. Each target's
death and drop finish before the next target's; capacity loss never refunds
a draw.

Stored merchant Misc records accept 1-10 charges so saves keep their exact
stock. Generation produces
3-10. Restore does not regenerate items, deliver rewards or move the RNG cursor;
old stock changes only through normal purchases and restock. Party inventory's
charge domain is unchanged.

**DOS static (2026-10-12), high confidence: monsters stage Misc correctly.**
The original category-3 branch copies the generated Misc container, power and
state into its own ten-slot treasure array. The pin instead copies the temporary
character's accessory record into the accessory array and loses the Misc result.
Consequently the old assertion that DOS monster drops cannot deliver Misc is
unsupported and contradicted by this staging trace. Normal capacity/delivery
refusals still apply; this is not a guarantee that every staged item reaches a
character. See [monster treasure evidence](evidence.md#monster-treasure-staging).

The character-generator and treasure category are established together:
category 3 selects file `0x2c4f5`; generated fields `0x112/0x113/0x114` become
treasure container/power/state at `0x9970/0x9971/0x9972` relative to the data
segment. The generator writes these fields and returns category 3. This rule
is generic, not specific to the Orc witness or one map. **Implemented (M61
Part C):** Misc monster drops use their own pending array and the existing
shared delivery rules, preserving all generation draws. As in DOS, pending Misc survives Run, an
in-process Load and New Game as dormant running-program state and is never
saved (M81); a fresh process starts empty; see
[the calibrated persistence trace](evidence.md#m81-monster-treasure).
M61 Part D implements DOS delivery as follows.

**DOS static, high confidence; implemented (M61 Part D):** Event rewards and
monster loot share one delivery rule, in staged category/slot order. Global
fullness is checked before each slot, including empty slots in DOS. Only
remaining items are discarded; gold and gems are still credited
(`0x2433e..0x2437b`, `0x2486a..0x24884`).

A valid preferred recipient is tried with capacity and `canAct` eligibility,
then other eligible members in party order. If neither succeeds, or the preferred
index is absent/invalid, scan capacity alone: sleeping, disabled and dead members
can receive items (`0x243d5..0x24565`).

**DOS static (M81 Part C), high confidence; agrees with the pin; implemented:**
the preferred recipient is the WhoWill byte, which every delivery reads,
monster loot included. A WhoWill dialog sets it when the party has two or more
members, inside an Event or when unlocking a grate; Cancel and a one-member
party leave it alone. Every call of the Event routine clears it at entry, even
when no record runs: each main-loop pass over a cell with the Event flag,
Space or a 3D-view click not taken by a grate, and two post-combat calls. Run,
Load, New Game, map changes and deliveries keep it, and no save holds it.
MMModern keeps it in the running program's staging value, outside the Event
stash; a refused or rolled-back candidate leaves it unchanged. Every published
Event call clears WhoWill, including a skipped call, every flagged input pass,
and either conditional post-combat caller.
[Evidence](evidence.md#preferred-recipient-after-the-events).

If every member's received category is full while another category has space,
replace the first member's last item in that category (`0x24567`). Preserve the
old destination frame/equipped mark: the four category branches write only
material, ID and state (`0x245aa..0x245cc`, `0x2462b..0x2464d`,
`0x246a0..0x246c2`, `0x24714..0x24736`). Sorting leaves a full nine-slot
category unchanged. Generation draws/order remain unchanged and precede delivery.
[Evidence and limits](evidence.md#treasure-delivery) distinguish this ordinary
loot rule from the separate special-sword branch.

**DOS static (M63 Part B):** clear input at treasure entry. Publish one transfer,
wait one interruptible tick on its blank screen, then sort that recipient's
inventory and publish the found line before its interruptible ten-tick hold.
Continue in category/slot order. Only the separate final acknowledgment credits
currency and clears staged treasure; interrupting a timed hold does neither
(`0x24128..0x248f2`, helper `0x264c5`). The full-pack modal/capacity rules above
remain shared by Event and monster rewards.

**DOS static (M81 Part 0), high confidence:** all four monster staging arrays
have ten slots. Generation precedes every capacity check; a full category loses
the new item without refunding any draws or changing readiness. Fixed gold and
gems accumulate separately in 32-bit fields before the item-chance roll, then
credit the 32-bit purse after final acknowledgment. Arena map 85 bypasses both
currency and ordinary item draws. Full-party Run clears staged currency and
hasItems, retaining the arrays. Ordinary Save restrictions do not inspect loot;
the synchronous receipt cannot open Save. A save holds purse and received
inventory, but excludes all pending loot. In-process Load clears currency and
readiness while retaining the current arrays as dormant items, even when they
were staged after Save. [Offsets and boundaries](evidence.md#m81-monster-treasure).
**Implemented (M81 Part A)** for the state, lifetime and save, and **(M81
Part C)** for every monster record's currency, chance and generation, with the
Arena exclusion.

**DOS static (M81 Part B), high confidence; implemented:** Event treasure uses
the same arrays. Each Event run stashes the staging when currency or readiness
is pending and clears only those; Event items take the first empty slot of
their category, so pending and dormant monster items count against Event
capacity. After the Events, a pending staging is delivered with the shared rule
above, monster items included, whatever monsters are in view (the Event loop
runs in mode 9); the stash then returns over the arrays. With pending monster
currency the monster items are therefore delivered twice and their currency
once, an original bug kept on purpose
([quirk](../original-quirks.md#sv-event-loot-stash)). The cell's Events run
before ordinary delivery, and an aborted Event leaves the staging as it found
it. [Evidence](evidence.md#event-processing-stash).

Quest grants participate in this sequence after their quest count has already
changed. Their display record must not grant the quest again or copy it into
ordinary inventory. The DOS formatter indexes raw quest IDs 82..116 in its
shared far-pointer name table (`0x245f8..0x24610`, table
`[0x4fd5c,0x4fde8)`), and adds the quest star (`0x2476e`). Names remain read
from the installed DOS module. A retry retains the current transfer, phase and
random prefix; later items cannot appear before earlier items' holds complete.
Removing the Event object disables future dispatch, while the current treasure
sequence retains its already executed grants and original selection identity.

## Limits

The most useful remaining original checks are the cursed Sell refusal, item
cancellation routes not traced above, equipment class/slot edge cases and
high-tier generation. Do not extend the observed price/item-use evidence
to these branches without tracing them.

[itemh]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.h
[item]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp
[dialogs]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[locations]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp
[combat]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp
[party]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp
