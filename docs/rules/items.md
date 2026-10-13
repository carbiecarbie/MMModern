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
| Armor 9 | Cloak / 5 | One |
| Armor 10 | Helm / 9 | One |
| Armor 11-12 | Boots / 10 | One shared slot |
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
with one broad claim that a broken item has no effect.

## Smith prices

**Evidence: DOS static (retained), high confidence.** The original price routine
is file `0x3c67d`; category and mode dispatch, tables and DOSBox calibration are
recorded in [M61 price evidence](../milestone-61-plan.md#original-program-price-evidence-and-confidence)
and [original quirks](../original-quirks.md). Source comparison:
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
state was observed. Cursed Sell/Discard refusal remains reference-only;
the maintainer's ordinary play did not reach a cursed item.

**DOS static (retained):** broken/cursed weapon names hide both material and
effectiveness suffix while retaining bad-state prefix and base name; Identify
still shows actual properties. The pin hides material but retains the suffix.
The DOS confirmation-verb table is ordered Identify then Fix; the pinned English
table swaps them. See [original quirks](../original-quirks.md) for formatter and
proficiency offsets.

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
complete original cancellation paths remain unverified.

## Discard, recharge and enchant

**Evidence: Reference only**, with retained **DOS play** for ordinary Discard's
selection, permanent-discard confirmation, Yes removal and No preservation.
Sources: [InventoryItems::discardItem/enchantItem][item],
[ItemsDialog::doItemOptions/itemToGold][dialogs].

Discard refuses cursed items before confirmation. Broken alone is not a refusal.
Removing an equipped cursed item also refuses. The reference allows Item to Gold
to convert an occupied ordinary item using full-value pricing and rejects
slayer-sword weapon IDs; this is distinct from Sell's cursed-item admission.
Original DOS equivalence for this distinction remains unknown.

Recharge admits nonempty Misc containers at material IDs at most 9, except the
Recharge Item power itself (53). It adds one `R(1,6)` to the counter and caps at
63. This check does not independently reject a broken/cursed item. The exact DOS
behavior and charge spending around the selecting item are not verified here.

Enchant first draws `R(1, floor(L/5)+1)` and caps the result at 5. The selected
category's handler then decides eligibility. Weapons and armor
require material 0 and empty state; weapons also reject slayer IDs. The generator
supplies enchantment properties; the action does not replace the selected base
item with a random base type. Accessories and Misc use the failure handler. The
draw can therefore occur before a later rejection; do not prevalidate it away
without DOS evidence. These are reference rules, not a fully traced DOS enchant
algorithm or a promise of current MMModern support.

## Generation and reward delivery

**Evidence: Reference only** for generic generation, with **DOS static, high
confidence** for the staging and delivery rules below. Sources: [Character::makeItem][character],
[Combat::attack2][combat], [Party::giveTreasure][party]. The exhaustive low-tier
generation ranges are recorded in [M61's catalogue](../milestone-61-plan.md#reachable-catalogue-exhaustive-generation-table-ranges).

Generation level 1 permits weapons 1-33, armor 1-7 and Misc containers 1-9;
no accessories. Level 2 expands armor through 13 and includes accessories 1-10.
Generated Misc powers are 1-15 at level 1 and 16-30 at level 2, with charges
`R(1,8)`. Ordinary level-1 equipment has no material or state bonus. Level-2
equipment may receive one of the material/effectiveness branches; generation
does not simply draw uniformly over all material IDs. Generation consumes its
draws before later capacity-related loss. The full high-tier generator and its
complete conditional RNG trace remain outside this chapter's DOS coverage.

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
shared delivery rules, preserving all generation draws. Pending Misc survives
Run and v12 save/load. Part D implements DOS delivery as follows.

**DOS static, high confidence; implemented (M61 Part D):** Event rewards and
monster loot share one delivery rule, in staged category/slot order. Global
fullness is checked before each slot, including empty slots in DOS. Only
remaining items are discarded; gold and gems are still credited
(`0x2433e..0x2437b`, `0x2486a..0x24884`).

A valid preferred recipient is tried with capacity and `canAct` eligibility,
then other eligible members in party order. If neither succeeds, or the preferred
index is absent/invalid, scan capacity alone: sleeping, disabled and dead members
can receive items (`0x243d5..0x24565`). Monster loot has no preferred recipient.

If every member's received category is full while another category has space,
replace the first member's last item in that category (`0x24567`). Preserve the
old destination frame/equipped mark: the four category branches write only
material, ID and state (`0x245aa..0x245cc`, `0x2462b..0x2464d`,
`0x246a0..0x246c2`, `0x24714..0x24736`). Sorting leaves a full nine-slot
category unchanged. Generation draws/order remain unchanged and precede delivery.
[Evidence and limits](evidence.md#treasure-delivery) distinguish this ordinary
loot rule from the out-of-scope special-sword and quest-item branches.

## Limits

The most useful remaining original checks are cursed-item refusals, complete
Recharge/Enchant and Item to Gold settlement, all non-First-Aid cancellation
paths, equipment class/slot edge cases and high-tier generation. Do not extend the observed price/item-use evidence
to these branches without tracing them.

[itemh]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.h
[item]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp
[dialogs]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp
[character]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp
[locations]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/locations.cpp
[combat]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/combat.cpp
[party]: https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/party.cpp
