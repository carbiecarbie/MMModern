# Milestone 25 - Bounded equipment management and existing-rule feedback

## Status, goal and authority

**ACTIVE specification; investigation/planning only. Neither stage is
implementation-authorized.** M24 is complete. This plan specifies player-triggered
equip and remove for bounded Clouds equipment through M24's inventory, with
original legality and feedback limited to existing character rules. It records
no M25 implementation, technical acceptance or physical acceptance.

Verified planning baseline: MMModern `main`, HEAD, `origin/main` and direct remote
main all equal `2c9ef4fac1665ddc1650134bc827807a3209973a`; working tree and staging
were empty. The M24 configuration (`build/24a/CMakeCache.txt`) selects the clean
ScummVM checkout at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Local directory
names are configuration details, not dependency contracts. Original investigation
used `F:\Games\gog\Might and Magic 4-5` read-only.

Current architecture/storage authority is [project status](project-status.md),
the [closed M24 plan](milestone-24-plan.md), [M21 item/persistence
contract](milestone-21-plan.md#21a-item-ownership-and-storage),
[dependencies](dependencies.md) and [AGENTS.md](../AGENTS.md). This plan resolves
the [roadmap proposal](roadmap.md#proposed-m25---bounded-equipment-management-and-existing-rule-feedback)
against that code baseline. In particular, M24 transfer compacts; equipment does
**not**. The roadmap's general compaction wording does not require equipment to
compact. Catalog support does not imply equip support or complete item effects.

All following ScummVM references are pinned to the verified revision:

| Reference | Symbols / authority |
| --- | --- |
| [item.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.h) | `ItemCategory`, `XeenItem`, `ItemState`, `INV_ITEMS_TOTAL`, `XEEN_SLAYER_SWORD=34`; equipped means nonzero frame |
| [item.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp) | `InventoryItems::passRestrictions/removeItem/discardItem`; `WeaponItems/ArmorItems/AccessoryItems::equipItem`; primary mutation oracle |
| [dialogs_items.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp) | `ItemsDialog::execute/doItemOptions`, `ItemSelectionDialog::execute`; ordinary `ITEMMODE_CHAR_INFO` actions 0 equip, 1 remove, distinct 3 discard |
| [dialogs_char_info.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_char_info.cpp) | Item-key entry calls `ItemsDialog::show(..., ITEMMODE_CHAR_INFO)` without condition/HP/SP eligibility |
| [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp) | `LangConstants::RESTRICTION_OFFSETS`, `ITEM_RESTRICTIONS`, `writeConstants`; numeric restriction authority |
| [resources.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/resources.cpp) | `Resources::loadData` reads those four offsets and 86 masks in the matching stream order |
| [en_constants.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/en_constants.h) | `EN::WEAPON_NAMES/ARMOR_NAMES/ACCESSORY_NAMES`; names/subtype domains, not legality predicates |
| [character.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.h), [character.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/character.cpp) | Class numbering, `Character::itemScan`, maximum HP/SP; distinction between legality and modeled consequences |

**Verified reference behavior** below describes these source paths, not an
independent claim of running the original executable. **Adaptation decisions**
are explicit where MMModern bounds unsafe inputs or changes interaction.

## Existing owners and implementation seams

Paths below are repository-relative; proposed new files are labeled.

| Concern | Existing owner/interface and M25 responsibility |
| --- | --- |
| Record/storage | `src/games/xeen/XeenCharacter.h/.cpp`: four-byte `XeenItem`, `XeenItemCategory = array<XeenItem,9>`, four character arrays. `XeenInventoryCategory` in `XeenItemCatalog.h` is a discriminator, not another array/owner. Preserve all bytes except the selected frame. |
| Identity | `XeenParty.h/.cpp`: `XeenRoster::at/characters`, `XeenParty::activeRosterIds/member/fromRosterIds`, `XeenPartyState`. Resolve one active reference to its authoritative roster owner; aliases observe the same mutation. No same-owner refusal for equipment. |
| Loading | `XeenPartyLoader::loadInitialCloudsParty/loadFromResources`, `src/formats/xeen/XeenCharacterFormat.cpp`: all 30 CHR records, offsets 166/202/238/274, nine slots each. No loading normalization. |
| Consequences | `XeenCharacterRules::validateForUse`, effective intellect/personality/endurance, `maxHp/maxSp`; context `{kCloudsInitialYear}` (610). Reuse; no new statistic model. |
| Category access | `XeenItemTransfer.h/.cpp`: `xeenInventoryItems`, `xeenSameItem`. Reuse checked access and equality. `xeenTransferItem` remains the independent M24 transfer operation, with its existing eligibility and compaction. |
| Catalog | `XeenItemCatalog::describe`, `loadXeenItemCatalog`; read-only bounded names/status and raw fields. Never consult naming availability to decide legality. Preserve M24 generated catalog and optional `mae.xen` boundary. |
| Transient interaction | `src/app/XeenEventFlow.h/.cpp`, `XeenInventoryFlow.cpp`: `handle/handleInventory`, `blocksGameplay`, `_inventoryEpoch`, confirmation, `validInventorySource`, `invalidateInventory`, `refresh`, recovery. Flow remains noncopyable and owns selection, result and input consumption. |
| View | `XeenInventoryView.h/.cpp`: `XeenInventorySelection`, `xeenInventoryLayout`, `drawXeenInventory`; retain the opaque native panel, nine physical slots and existing renderer. |
| Input | `src/core/PlayerAction.h`, `src/platform/sdl/SdlWindow.cpp::playerAction/showLoop`; add one inventory-only action on E. Keep one SDL loop, ignored repeat events and Escape predicate. |
| Gameplay/save | `Application::playGameplay` in `src/app/XeenGameplay.cpp`, `XeenGameplayServices`: Application owns session/save routing, active and dispatch guards. Flow never captures or writes a save. Existing borrowed observation/configuration seams suffice. |
| Persistence | `XeenSaveState::capture/restoreBeforeGameplay`, `src/formats/xeen/XeenSaveFormat.cpp`, `src/platform/XeenSaveFile.cpp`; capture copies actual roster characters; v2 writes every frame; restore preflights unpublished owners. No format change. |
| Verification | `XeenItemTransferTests`, `XeenInventoryGameplayTests`, `XeenCharacterRulesTests`, save format/state/Flow/SDL/CLI tests; `PartyIntegrationTest`, `ItemCatalogIntegrationTest`, `SaveResumeIntegrationTest` and shared snapshot/child-process support. Extend these boundaries rather than inventing another runtime/harness architecture. |

The new domain helper belongs beside character/item rules: proposed
`src/games/xeen/XeenEquipment.h/.cpp`. It is a synchronous operation over borrowed
`XeenPartyState`, not a service or equipment owner. An unpublished character copy
for checked rule preflight is a temporary value, never a copied live inventory.

## Exact reference legality and mutation

### Domains and frame/conflict matrix

**M25 adaptation:** equip supports Weapons IDs **1..34**, Armor **1..13** and
Accessories **1..10**. ID zero is empty. Unknown material/counter bytes do not
bar these IDs. Remove supports **every occupied ID 1..255** in those three
categories, including unknown/Elder IDs and unknown nonzero frames. Misc is
outside both operations.

The catalog's Weapons 35..40 are inherited Elder names. M25 deliberately does
not promote their gameplay: equip refuses `UnsupportedItem`, although inspection,
M24 transfer, persistence and uncursed remove still work. This bounds Clouds
equipment without changing M24 naming. ID 34, Xeen Slayer Sword, is included:
its ordinary equip rule is two-handed and unrestricted. It gets no spell,
damage, combat, automatic uncurse or special activation behavior.

**Verified reference:** the functions themselves are more permissive than the
supported naming domains. Weapon `else` includes 18..29 and everything above 33;
armor `else` includes every ID above 12; accessory `else` includes every ID
above 7, and its medal branch also catches zero. There is no safe general
unknown-ID contract: restriction/name indexing can exceed tables. In particular
weapon IDs 35..40 use masks `[0,0,68,100,116,125]` from the shared restriction
array and all take frame 13, even the entry named Elder LongBow. Do not infer
legality from those names or reproduce unchecked arbitrary-ID indexing.

In the following table **scan means all nine physical slots, ascending 0..8,
including the selected slot, ID-zero metadata, unknown IDs and cursed/broken
records**. Compare the exact frame only, without testing ID, material or state.
Report the first conflict in the stated scan order. Proficiency runs before scans
where specified. No candidate is removed automatically.

| Selected category / ID | Subtype | Proficiency | Ordered conflict checks | Frame on success |
| --- | --- | --- | --- | --- |
| Weapons 1..17 | One-handed, including spear 17 | Weapon mask | Weapons frame 1 or 13 | 1 |
| Weapons 18..29, 34 | Two-handed, including Xeen Slayer Sword | Weapon mask | Weapons frame 1 or 13; then Armor frame 2 | 13 |
| Weapons 30..33 | Short bow, long bow, crossbow, sling | Weapon mask | Weapons frame 4 | 4 |
| Armor 1..7 | Robes, scale, ring, chain, splint, plate mail, plate armor | Armor mask | Armor frame 3 | 3 |
| Armor 8 | Shield | Armor mask | Armor frame 2; then Weapons frame 13 | 2 |
| Armor 9 | Helm | None | Armor frame 5 | 5 |
| Armor 10 | Boots | None | Armor frame 9 | 9 |
| Armor 11..12 | Cloak/cape share one slot | None | Armor frame 10 | 10 |
| Armor 13 | Gauntlets | None | Armor frame 6 | 6 |
| Accessories 1 | Ring | None | Count Accessories frame 8; refuse if count >= 2 | 8 |
| Accessories 2 | Belt | None | Accessories frame 12 | 12 |
| Accessories 3..7 | Brooch, medal, charm, cameo, scarab share medal capacity | None | Count Accessories frame 7; refuse if count >= 2 | 7 |
| Accessories 8..10 | Pendant/necklace/amulet share one slot | None | Accessories frame 11 | 11 |

From an ordinary consistent loadout: at most one melee weapon (one- or
two-handed), plus one missile weapon; one each of body armor, shield, helm,
boots, cloak/cape and gauntlets, subject to shield/two-handed exclusion; two
rings, two medal-group items, one belt and one necklace-group item. These are
**frame-group** bounds, not an extra global equipment-count check. Nine storage
slots per category remain unchanged. There is no body-armor versus weapon
conflict, no one-handed versus shield conflict, and no missile versus shield
or melee conflict. No dual-wield exception is present.

### Exact class restrictions and source adaptation

Class IDs match MMModern: Knight 0, Paladin 1, Archer 2, Cleric 3, Sorcerer 4,
Robber 5, Ninja 6, Barbarian 7, Druid 8, Ranger 9. Knight and Paladin always
pass proficiency. For class `c` in 2..9, the item is forbidden iff:

```text
ITEM_RESTRICTIONS[id + RESTRICTION_OFFSETS[category]] & (1 << (c - 2)) != 0
```

`RESTRICTION_OFFSETS[4] = {0,35,49,60}`; `ITEM_RESTRICTIONS[86]` has index domain
0..85. M25 reads the equivalent of indices 1..34 for weapons and 36..43 for
armor IDs 1..8. Armor 9..13 would map to 44..48, accessories 1..10 to 50..59;
these masks are zero, but their equip branches do **not** call proficiency.
Do not add that call or a class rule to accessories/armor 9..13. Invalid classes
are handled safely by the operation/rule preflight below, not indexed unchecked.

| Category / IDs | Decimal mask | Forbidden classes (Knight/Paladin never included) |
| --- | --- | --- |
| Weapons 1..6, 27..29 | 86 | Cleric, Sorcerer, Ninja, Druid |
| Weapons 7, 24, 34 | 0 | None |
| Weapons 8, 17 | 6 | Cleric, Sorcerer |
| Weapons 9..11, 26 | 239 | Archer, Cleric, Sorcerer, Robber, Barbarian, Druid, Ranger |
| Weapons 12 | 2 | Cleric |
| Weapons 13..16, 25 | 4 | Sorcerer |
| Weapons 18..21, 23, 30..33 | 70 | Cleric, Sorcerer, Druid |
| Weapons 22 | 94 | Cleric, Sorcerer, Robber, Ninja, Druid |
| Armor 1 | 0 | None |
| Armor 2 | 68 | Sorcerer, Druid |
| Armor 3 | 100 | Sorcerer, Barbarian, Druid |
| Armor 4 | 116 | Sorcerer, Ninja, Barbarian, Druid |
| Armor 5 | 125 | Archer, Sorcerer, Robber, Ninja, Barbarian, Druid |
| Armor 6..7 | 255 | All eight classes 2..9 |
| Armor 8 | 85 | Archer, Sorcerer, Ninja, Druid |

**Data/provenance decision:** MMModern has matching class IDs and catalog name
domains, but no proficiency masks or equipment conflict evaluator. Add only
34 weapon masks and eight armor masks as bounded source-derived `constexpr`
data in the equipment helper, plus the small predicates above. This follows
the existing `XeenCharacterRules.cpp` numeric-constant approach. Do not copy
all 86 masks, unrelated constants, ScummVM headers or the source tree; do not
extend M24's catalog generator/blob schema or introduce runtime `mm.dat`.
Retain the exact upstream revision/path/symbol, indexing explanation, ScummVM
attribution and GPL-3.0-or-later notice/source availability with the adaptation.
Distribution carries corresponding adapted source under the project's license.
Commercial archive/material bytes and extracted fixtures remain external.

For independent review, the weapon sequence for IDs 1..34 is:

```text
86,86,86,86,86,86,0,6,239,239,239,2,4,4,4,4,6,
70,70,70,70,94,70,0,4,239,86,86,86,70,70,70,70,0
```

Armor IDs 1..8: `0,68,100,116,125,255,255,85`. Tests must independently express
allowed/refused class sets, not obtain expectations by calling this lookup.

### Equip edge cases and ordering

For an occupied supported record, reference order is proficiency (when called),
same-category scan/count, cross-category scan (only two-handed/shield), then
assignment of **selected frame only**. No material, low counter, curse, broken,
character-condition, current HP/SP, `canAct`, sex, race, capacity or spell test
is an equip-legality gate. Existing broken/cursed equipped items still conflict.
Equipping does not create/clear curse or breakage, consume charges or heal.

There is no reference-wide `AlreadyEquipped` refusal. Direct Equip on a normally
equipped singleton conflicts with itself, after proficiency. A ring already at
frame 8 with total count 1 succeeds idempotently; count 2 refuses even when the
selected record is one of those two. Medal frame 7 is analogous. An anomalous
nonzero selected frame can be reassigned if target-group checks pass. The pure
helper must preserve these distinctions; it must not insert a blanket nonzero
frame rejection or skip the selected slot. The contextual UI chooses Remove
for a nonzero frame, so those direct Equip contrasts belong in synthetic tests.

Ordinary UI selection only accepts occupied items; an absent selection opens
the reference item picker, whose Escape returns without action. `doItemOptions`
also exits early when slot 0 is empty, reflecting its compact-array assumption.
M24 exposes physical holes: M25 checks the **selected** ID and permits an occupied
later slot even when slot 0 is empty. It never runs the raw equip method on an
empty record. This is an explicit defensive adaptation, not implicit compaction.

### Remove is not discard

Reference `InventoryItems::removeItem`: check selected curse bit `0x40`; if set,
refuse; otherwise set selected frame to zero. Broken alone does not prevent it.
Curse wins even when the frame already equals zero. There is no proficiency,
condition, HP/SP, race/sex or `canAct` check. Unknown ID/frame does not need a
type-table lookup. An uncursed already-zero frame is an idempotent no-change
result. M25 rejects empty/unselected before this operation and rejects Misc.

No other record changes. Material, ID, all state bits, physical slot and all
other categories remain exact. No compaction, item deletion, quantity change,
automatic replacement or clearing of ID-zero metadata occurs. In contrast,
reference `discardItem` asks permanent-discard confirmation, clears the entire
record and sorts. That separate operation is excluded; no discard path or key
is added. `ItemsDialog::setEquipmentIcons` operates on dialog stock/icon data
and is not the character equip oracle (including its accessory ID assignment).

## Compatibility with modeled consequences

`XeenCharacterRules::itemBonusFrom` and pinned `Character::itemScan` agree on
the relevant equipped predicate: any nonzero frame, neither cursed nor broken,
material 59..130; no nonzero-ID requirement. Misc contributes nothing.

| Existing consequence | M25 compatibility / feedback boundary |
| --- | --- |
| Intellect | Materials 69..76 contribute existing bonuses; recompute effective intellect and any resulting max SP. |
| Personality | Materials 77..84 contribute existing bonuses; recompute effective personality and any resulting max SP. |
| Endurance | No equipment bonus category exists. Existing age/condition/base calculations remain; equip/remove must not change effective endurance. |
| Direct max HP | Materials 105..109 contribute existing +4/+6/+10/+20/+50. |
| Direct max SP | Materials 110..115 contribute existing +4/+8/+12/+16/+20/+25, subject to existing `hasSpells`/max-SP calculation. |
| Curse/broken | Either bit suppresses modeled bonuses even with an equipped frame. Do not confuse suppression with legality or free equipment capacity. |
| Frame coverage | All produced frames `{1,2,3,4,5,6,7,8,9,10,11,12,13}` are recognized. No legitimate produced frame is misinterpreted by these modeled rules. |

Thus truthful M25 feedback requires **no new derived-stat model**. It can show
equip/remove result, existing equipped state, current/max HP/SP and changes to
effective intellect/personality or direct maxima. A material such as original
86 affects an unmodeled attribute; successful equip may leave all displayed
values unchanged. Say `Equipped`/`Removed`, not `No effect` or a claimed damage,
AC/resistance improvement. Legality needs none of those unmodeled statistics.

ID-zero modifier metadata remains a supported storage/rules oddity: it may
contribute a bonus and block a frame group while the inventory displays Empty.
M25 neither equips/removes that empty selection nor fixes the bystander. Existing
over-capacity or anomalous frames are preserved on load; no whole-loadout
normalization/validation is added. Scan exact frame predicates, not inferred IDs.

## Mutation API and publication safety

Proposed public domain boundary in `XeenEquipment.h`:

```text
XeenEquipmentOperation { Equip, Remove }
xeenSetEquipment(XeenPartyState&, activeIndex, XeenInventoryCategory,
                 physicalSlot, XeenEquipmentOperation) -> XeenEquipmentResult
```

The explicit operation keeps direct reference behavior testable; SDL exposes
only contextual E. A fixed-size typed result records status, operation, resolved
owner/category/slot, before/after item bytes, optional first conflict
category/slot or ring/medal count, and before/after effective intellect,
personality, endurance, max HP and max SP on success. No pointers into replaceable
owners, strings, callback or UI lifetime belong to the helper. Distinguish
`Success` (frame changed), `NoChange` (reference idempotent success), structural
refusals, `UnsupportedItem`, `NotProficient`, `Conflict`, `RingLimit`, `MedalLimit`,
`Cursed` and `UnsafeRules`. A singleton self-conflict can be presented as
already equipped, but remains a conflict after the reference's proficiency check.

Exact operation ordering:

1. Flow requires active gameplay, Browse, no pending event/reentrant dispatch,
   and a current selection token. Consume the attempt before calling the helper.
   Token checks are UI safety and precede all domain rules.
2. Helper checks nonempty membership; active index; resolved roster ID < 30 and
   matching character `rosterId`; valid operation; category discriminator and
   equipment category; slot 0..8; selected ID nonzero. Return the first failure.
   Active aliases are permitted and touch one owner once. No direct inactive
   owner API is introduced.
3. Equip: check supported ID domain, then the class-mask rule where applicable
   (invalid class in that branch returns `UnsafeRules`), then matrix conflicts
   in their exact order. Remove: check curse, then propose frame zero; no
   supported-ID/class-proficiency check. Cursed unknown items still refuse removal.
4. Prepare an unpublished copy of the single character, replacing only its
   selected frame. Run existing `validateForUse` with year 610 on original and
   candidate before using unchecked derived getters; obtain complete before/after
   modeled values. Invalid race/class, arithmetic overflow/underflow or unsafe
   candidate returns `UnsafeRules` with no writes. This is MMModern's numeric
   safety envelope, not an original class/condition restriction. Do not reject
   otherwise safe negative/above-maximum current HP/SP. Preparation allocation
   failures propagate before publication and leave all live state unchanged.
5. Construct the fixed result completely. If the frame is equal, return
   `NoChange` with exact full-state equality. Otherwise the single nonthrowing
   assignment `liveSelected.frame = candidateSelected.frame` is the publication
   point. Do not assign the character copy or arrays back. No callbacks,
   allocations, formatting or rendering occur between last validation and that
   byte store. Statically verify nonthrowing result return/storage.
6. Flow stores the result and clears/disarms selected action state before any
   observer, catalog, feedback formatting, scene composition or drawing. Rebuild
   current HUD/underlay with existing neutral redraw cause. Never roll back,
   retry or invert a published operation because presentation failed.

There is one record/one byte of durable mutation on every changing success,
and zero on failure/no-change. The operation can be completely preflighted.
Current HP/SP are never assigned, clamped or healed, even after maxima fall.
Unrelated owners, alias membership, all items, quests, flags, world overlays and
camera must have exact full-state equality on refusal; successful comparison
permits only the selected frame difference. Transient feedback/selection may
change on a refusal. There is no transfer-shaped two-owner publication.

Use M24 recovery: clear transient inventory and attempt one clean scene rebuild.
If recovery succeeds, reopening reads the published owner. If clean recovery
fails, mark fatal and prevent F9; do not retry recovery through another catch.
Failures before publication preserve everything; failures after it preserve the
new frame. Read-only catalog fallback is not a mutation failure. Optional
`reportEquipment` observation follows stored result/disarming and is never a
second mutation path. Reentrant action/save/event callbacks remain blocked by
the existing Application/Flow dispatch scopes throughout preparation and redraw.

## Modal, input and feedback contract

**Adaptation decision:** add `EquipmentInventoryAction` on **E**. E is unused in
the current SDL mapping. In Browse it selects Equip when stored frame is zero,
Remove otherwise. Label it `E equip/remove`, with selected detail indicating
`E equip` or `E remove`. It has no confirmation: reference equip/remove is
immediate, the selected record and current curse/broken status are visible, and
neither action discards. Equipping a cursed record is allowed and may make
subsequent removal refuse; do not silently substitute a safety confirmation.

| Context / input | Required result |
| --- | --- |
| Closed inventory, E | Ignore; no open, movement, script, label clearing or animation step. |
| Pending NPC/message/Yes-No/WhoWill/reward warning or receipt, E | Ignore, preserving pending page/generation/timing/state. No queued action or fallthrough when a later response completes. |
| Browse, valid occupied equipment selection, E | Validate selected facts; run contextual operation synchronously once; show result; remain on same owner/category. Clear selected slot/action token after the attempt, so a second non-repeat E cannot toggle back. Select a slot explicitly for another action. |
| Browse, no selection / selected ID zero / empty party, E | Bounded `Select an occupied item` / `No active characters`; no byte changes or compaction. |
| Browse, Misc selected, E | `Misc equipment is unsupported`; no use, charge change or remove of raw Misc frame. Category rejection precedes selected-ID handling. |
| ChooseDestination or Confirm, E | Consume with existing `Escape to cancel` guidance; preserve transfer token and source/destination. No equipment action or transfer. |
| Browse, Escape or I | Close inventory; no undo. There is no pending equipment confirmation to cancel. |
| Transfer modes, Escape / Confirm N | Existing M24 cancellation only, with no equipment mutation. |
| Any open mode, F9 | Application refusal plus existing inventory feedback; no capture/preflight/I/O, cancellation, deferred save or equipment action. Close then press a new F9. |
| Window close | Existing teardown; no autosave. Published frames remain in memory until session destruction. |

All M24 owner/category/physical-slot controls, T/Enter transfer, event keys and
idle Escape-to-quit remain unchanged. SDL ignores key-repeat events before
dispatch; additionally clear the selection after a completed E attempt to stop
duplicate ordinary keydowns from toggling the same selected quantity. A stale
attempt clears selection and reports `Selection changed; select again`.

Reuse `_inventoryEpoch`, its exhaustion policy and explicit `invalidateInventory`
notification. At each explicit Browse slot selection, arm a bounded equipment
selection token with current epoch, complete active membership (at most six),
source active index/owner, category/slot and four selected bytes. This is a
selection certificate, not another modal phase or durable item identity. Validate
all facts immediately before consuming E. Owner replacement, including identical
bytes, must use existing explicit invalidation. Do not infer safety from byte
equality across reconstruction. Any epoch advance disarms this equipment token;
owner/category navigation or T cannot leave it reusable. `refresh(true)` and
explicit invalidation disarm it, though M24 may retain a valid highlighted slot
for viewing; a new slot selection is required. Ordinary timed underlay rebasing
preserves it. Conflict frames and class are read live during the synchronous
helper; there is no cached equipment-legality registry.

Keep the panel `(4,4)..(316,149)`, nine-pixel rows, all nine physical slots,
two-line selected name, raw M/ID/S/F, status/counter, identity/condition and
**full-width signed current/max HP and SP lines**. Do not sacrifice complete
numeric fields for a new attributes screen. Add only contextual help and result
formatting. M24 `_inventoryFeedback` currently points to static text: dynamic
equipment text must have owned lifetime or be formatted directly from the fixed
Flow-owned result during `xeenInventoryLayout`; never retain a temporary `c_str`.
Chosen approach: pass an optional borrowed `XeenEquipmentResult` to the existing
layout/draw path and format it there after publication; keep static M24 feedback
for transfer/F9 and clear equipment-result display on subsequent browsing/input.

The full-width feedback row reports concise success/refusal. On a modeled
attribute change prefer `Equipped; INT 11 -> 13` or `Removed; PER 13 -> 11`;
otherwise show changed direct max HP/SP similarly, or plain `Equipped`/`Removed`.
Existing HP/SP lines always reflect all resulting maxima, including indirect
SP changes. Endurance should never produce an equipment delta. Use full before/
after numbers (not subtraction that could overflow), bounded labels and measured
width; important refusal reasons/numbers must not be elided. A conflict can use
`Remove Weapons slot 1 first` rather than needing catalog names; ring/medal
capacity has its own reason. Do not add a blocking acknowledgment page or hide
curse/broken status behind feedback. Row equipped marks prove frame state after
selection is cleared. HUD and dismissal must use the recomposed current maxima.

Retain M22's ordinary 100 ms outdoor phase/deadline and M23 indoor composition.
Equipment uses `OrdinaryCause::None`; an independently due idle callback may
advance normally. The opaque panel does not expose underlying animation to human
observation. No second menu framework, input loop, event script or clock is added.

## Save and restart policy

**No save-format change.** The only durable change is an existing uint8 frame.
Writer v2 already stores 144 item bytes per character; reader v1/v2, legacy
absent-ID/Misc resolution, opaque item bytes, CRC, archive fingerprints and
protected disk publication remain exactly as [M21](milestone-21-plan.md#save-v2-and-legacy-v1-compatibility)
and [M24](milestone-24-plan.md#persistence-and-compatibility) specify.

No equipment owner/registry, inferred loadout, serialized modal state, derived
maximum, result/token, catalog text or resource payload is necessary. Capture
reads authoritative arrays; load preserves frames, including unsupported IDs
and inconsistent original frames. Recompute rules/HUD, never clamp current HP/SP
or run equip on restore. Resume starts inventory closed with fresh transient
state and no initial automatic-event replay. Reopening reads actual owners.

Required acceptance path is Application action -> mutation -> close inventory
-> **new production F9** -> actual disk file -> producer exits -> distinct
consumer plus production `mmodern --load-game` process -> I -> inspect exact
owner/category/slot/frame. Neither a same-process recapture nor a serializer
unit round trip alone can satisfy this boundary. Consumers leave disk bytes
unchanged. Any newly required durable state is a replanning trigger.

## Original-data evidence and acceptance anchors

Investigation decoded the original initial archive in memory only, following
`ScummVmXeenBridge::InitialCloudsArchive/initial` and pinned
[cc_archive.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/shared/xeen/cc_archive.cpp).
It used the original reset block order, XOR-decoded outer members, inner index
and CHR offsets, without writing extracted resources. `maze.chr` is 10,620
bytes, `maze.pty` 812; active order `[0,18,14,11,1,6]`. Across all 30 owners there
are 35 occupied items, all on those six owners, with no Misc, cursed, broken,
two-handed weapon or frame-7 medal item. There are two actual ID-1 rings and
one ID-5 charm; no other medal-group item. Existing prebuilt party/catalog smokes
also passed read-only checks: all 1,080 item slots matched original CHR bytes,
and Dagger/Leather boots/Silver ring labels and raw fields matched. This is
planning evidence only, not M25 acceptance or a new-build claim.

`DARK.CC/mae.xen` has 1,093 bytes and 131 names, SHA-256
`78f3ec8421fd46619a4aba63dca1042c914b0f3437d91ce37a47f81676bd4156`;
material 38 is leather and 42 silver. Original labels remain catalog/resource
driven. All slot numbers below are **zero-based**; UI keys are slot + 1.

| Case | Original prerequisites | Independently specified action and outcome |
| --- | --- | --- |
| Primary dagger | Zippo, owner 11/F4, Robber: Weapons slot 0 `{0,12,0,1}`, slot 1 `{0,12,0,0}` | E on slot 1 refuses: weapon slot 0 frame 1 conflicts; Robber passes mask 2. Reselect slot 0/E removes to `{0,12,0,0}`. Reselect slot 1/E equips to `{0,12,0,1}`. Slots do not exchange or compact. All other bytes exact. |
| Armor | Arturius, owner 0/F1, Paladin: Armor slot 3 `{38,10,0,9}` Leather boots | Select/E removes to frame 0; reselect/E restores frame 9. Separate transfer of these boots to Tyro's armor slot 4 (M24) then E refuses against Tyro slot 3 frame 9. |
| Accessory | Zippo Accessories slot 1 `{42,1,0,8}` Silver ring; slot 0 `{38,2,0,12}` Leather belt | Select/E removes only ring frame; reselect/E restores 8; belt remains exact. |
| Class refusal | Transfer Zippo's unequipped dagger slot 1 to Rebecca, owner 1/F5, Cleric, via existing T/F5/Enter | Rebecca retains Cudgel `{0,15,0,1}` at Weapons slot 0 and receives `{0,12,0,0}` at slot 1. E on dagger refuses proficiency (mask 2), before the Cudgel conflict. No mutation on refusal. |
| Additional weapon frame | Badger, owner 14/F3: Weapons slot 0 `{0,8,0,1}` hand axe, slot 1 `{0,30,0,4}` short bow | Remove/re-equip short bow sets 0 then 4 while hand axe stays frame 1; Ranger passes bow mask 70. |
| Raw frame anomaly | Rebecca Accessories slot 1 `{42,5,0,8}` Silver charm | This is an equipped charm with ring frame, not a third actual ring. Remove gives `{42,5,0,0}`; reselect/E gives `{42,5,0,7}`. Loading/viewing alone must never change 8 to 7. |
| Ring capacity using originals | Fresh Rebecca retains belt slot 0 and charm slot 1/frame 8. Transfer Zippo Silver ring to Rebecca slot 2, frame reset to 0; equip it to 8. Transfer Seymour/owner 6/F6 Accessories slot 1 `{86,1,0,0}` to Rebecca slot 3 | Rebecca has two frame-8 entries (charm + Silver ring). E on slot 3 refuses RingLimit with exact arrays. Remove charm (frame 0), then equip slot 3 to 8; now two true rings. Re-equip charm to frame 7 and it coexists with both. Never fabricate a ring or change material 86. |

Use independent fresh states for these controls so setup from one does not
contaminate another. The ring control's final Rebecca Accessories slots 0..3
are `{38,2,0,12}`, `{42,5,0,7}`, `{42,1,0,8}`, `{86,1,0,8}`, then zeros;
Zippo and Seymour retain only their original leather belts in Accessories.
M24 transfer is the only setup mutation between owners; it keeps its own
compaction semantics. Primary dagger requires no transfers and is the smallest
restart/physical anchor. No listed original sequence changes modeled maxima;
material 86's unmodeled attribute is not invented feedback.

**Synthetic/source-reference evidence, not original loot:** cursed and broken
states; two-handed/shield conflicts in both directions; third medal-group item;
full class/mask/domain matrices; unknown bytes, empty metadata and modifier
contrasts. Initial roster data cannot establish those original acceptance cases.
Do not search later loot, recruit inactive owners, forge an original record or
extend encounter/event gameplay to obtain them.

## Stage structure and separately authorized boundaries

Two stages are proposed because legality/mutation has an independently testable
contract, while input/publication recovery and real process restart form a
second integration boundary. A standalone data-generation stage is unnecessary
for 42 numeric masks. Both stages require separate explicit authorization;
approval of this plan or 25A completion does not authorize 25B.

### 25A - Equipment legality and one-byte mutation foundation

- Objective: implement the exact bounded masks, predicates, explicit Equip/Remove
  operation and typed, fully preflighted publication result.
- Dependencies: stable M24; approved architecture review of this plan.
- Proposed file responsibilities: new `XeenEquipment.h/.cpp`, new
  `tests/XeenEquipmentTests.cpp`, focused additions to `XeenCharacterRulesTests`
  where useful, and minimal `CMakeLists.txt` target/source registration.
  Reuse existing category/identity access; no owner/interface redesign.
- Exclusions: PlayerAction/SDL/Flow/view/Application/save production changes,
  player-accessible equipment, new catalog generation, new effects and transfer
  behavior changes. Update only stage-relevant plan/provenance documentation;
  stable status does not claim completed player equipment.
- Automated acceptance: complete class/ID/frame and edge-order matrix below,
  independently authored post-bytes, rule/HP/SP safety, alias/empty-metadata
  contrasts, relevant existing character/transfer/save regressions and build.
- Original-data acceptance: extend existing `PartyIntegrationTest.cpp` with
  in-memory operations on independently loaded original owners for dagger,
  boots, ring, charm and bow controls. Compare each against initial bytes plus
  literal expected frame changes; this is domain evidence, not UI acceptance.
- Physical acceptance: none claimed or required for a non-player-accessible
  foundation. Independent technical review must accept the domain contract and
  source-derived masks before closure. Completion means that reviewed foundation
  and its tests pass; M25 remains incomplete until 25B acceptance.

### 25B - Existing modal, feedback and production persistence lifecycle

- Objective: connect contextual E, selection safety, truthful existing-rule
  feedback and all original/synthetic production acceptance through M24 owners.
- Dependencies: accepted 25A and separate explicit authorization.
- Proposed file responsibilities: `PlayerAction.h`, `SdlWindow.cpp`,
  `XeenEventFlow.h/.cpp`, `XeenInventoryFlow.cpp`, `XeenInventoryView.h/.cpp`;
  existing inventory/SDL/rules/save tests, `SaveResumeIntegrationTest.cpp` and
  its existing snapshot/child-process support; minimal CMake registrations.
  `XeenGameplay.cpp/Services` may receive only a narrowly demonstrated integration
  adjustment; existing save ownership/guards must remain. No serializer change
  is anticipated. README controls update only with implemented capability;
  status/history/roadmap/closed-plan updates only after milestone acceptance.
- Exclusions: new equipment rules beyond 25A, altered M24 transfer, new save
  fields, broader events/route acceptance, or any non-goal below.
- Automated acceptance: domain regressions plus E routing, token invalidation,
  UI bounds, failure/reentrancy, pending events, SDL, synthetic save compatibility
  and process-restart tests below. Full build and complete CTest required at
  milestone closure, with no failing tests.
- Original-data acceptance: all selected anchors above, independent full-state
  oracles, producer/disk/separate-consumer/actual CLI reopen. Retain M24 transfer,
  genuine Myra exchange/reward, M22 timing and M23 indoor regressions.
- Physical acceptance: maintainer sequence below, separate from automated SDL,
  screenshots and independent technical review. Completion requires all required
  automated/original/physical gates plus independent review; then durable closure
  documentation, without implying Git authorization.

## Discriminating automated acceptance

### Domain and numerical tests

1. Independently test all ten classes against every supported weapon 1..34 and
   armor 1..8 with otherwise empty arrays; compare explicit allowed/refused sets.
   Exercise armor 9..13/accessories 1..10 under all classes to prove absence of
   proficiency gates. Cover every subtype boundary, especially spear 17,
   18/29/30/33/34 and cloak/cape 11/12. Weapons 35..40 refuse equip despite names.
2. Each matrix row: successful equip and remove; exact one-byte difference at
   first/middle/tail physical slots; all bystanders, holes and ID-zero metadata
   unchanged. No compaction, array exchange, implicit unequip or record clear.
   Refusals compare complete typed state, not only selected record or item counts.
3. For every relevant scan, conflict at each physical position; selected self
   included; first conflict chosen ascending. Weapon same-category conflict wins
   over shield conflict; shield conflict wins over two-handed weapon conflict.
   Proficiency wins over either. Bow coexists with melee/shield; body armor,
   helm/boots/cloak/gauntlets add no weapon conflict. Cursed/broken and ID-zero
   bystanders still block/count; unknown frame 255 is not frame 1/13/2/etc.
4. Rings/medals: counts 0,1 permit equip; 2,3,9 refuse; count by raw frame even
   with a different ID/empty ID/bad state. Direct Equip on an already equipped
   ring/medal is NoChange at count 1 and capacity refusal at count 2. Singleton
   direct Equip conflicts with itself; wrong nonzero frame can remap on explicit
   Equip. Contextual E instead removes. No general AlreadyEquipped shortcut.
5. Equipping cursed, broken and both is allowed when other rules pass; state
   remains exact and bonuses suppressed. Remove curse-first, including frame
   zero/unknown ID; broken-only remove succeeds; uncursed zero frame is NoChange.
   No character condition or negative/zero HP/SP gate; cover all disabled/worst
   conditions without healing or condition changes.
6. Structural invalid operations/category/slot/active index/owner, empty party,
   nonzero metadata with empty ID, unsupported equip IDs at 0/35/40/41/255,
   Armor 14/255 and Accessories 11/255. Unknown occupied IDs can be removed.
   Unknown materials 131/255 and weapon counters 7/63 do not prevent supported
   IDs from equipping. Misc frame stays opaque. Aliases mutate one roster owner,
   no duplicate capacity/counts; inactive owners remain exact.
7. For every produced frame 1..13, test an independently valid category/ID with
   material 69 (+2 intellect), 77 (+2 personality), 105 (+4 HP), 110 (+4 SP),
   curse/broken suppression and remove. Use numeric literal expectations; no
   expectation calls the equipment helper. A Human Paladin, level 1, year 610,
   birthYear 592, endurance 19, personality 13, spells=true has max HP/SP 12/2;
   material 105 yields 16/2, material 110 yields 12/6. Current HP=16 and SP=6
   remain exact after removal, as do separate negative signed controls.
8. Independent intellect/personality fixtures start effective 11 and become 13
   on equip, return 11 on remove; verify indirect SP using existing rule oracle.
   No endurance delta, no Misc effect, no bonus lost from untouched ID-zero
   metadata. Safe extreme saved inputs that would overflow on equip or underflow
   on removal refuse before publication, including negative attribute/condition
   combinations whose previous item bonus kept arithmetic safe.
9. Preparation failure before store preserves all state; result construction and
   return are nonthrowing after store. No fallible observer is inside the helper.
   Keep class/rule safety separate from original proficiency and storage validity.

Normal CTest uses synthetic authored data, not commercial resources. Numeric
source provenance can be cross-checked against the pinned constants separately;
tests must not compare the production table only with itself or reproduce the
same lookup implementation as their sole oracle.

### Flow, rendering, Application and SDL

- Drive E through actual Application action routing for all equipment categories,
  empty/unselected/Misc cases, each main refusal and successful feedback. Confirm
  row marks, raw frame and complete current/max HP/SP; distinguish reference
  bad-item legality from bonus suppression. Verify full numeric feedback at int
  bounds, nine-pixel descenders, wrapped selected names and unchanged panel bounds.
- Two ordinary E keydowns after one selection act once; repeated SDL keydown acts
  zero times; explicit reselect/E can remove/equip again. E never confirms a
  transfer, acknowledges an event, opens a closed inventory or dispatches Space.
- Stale epoch, changed full membership, changed selected bytes, changed owner ID,
  identical-owner replacement plus `invalidateInventory`, `refresh(true)`, close/
  reopen and epoch exhaustion all prevent a stale attempt. Timed rebase preserves
  valid selection and does not rearm a consumed one. Aliases show live same-owner
  changes without retaining character copies.
- Pending automatic/manual message, Yes/No, NPC, WhoWill, reward warning and every
  receipt page block equipment input. Preserve generation/page/pixels/timing at
  handler boundary; completion never falls through. Direct event-start seams
  reject while inventory is open or synchronous equipment dispatch is active.
- Escape closes Browse/cancels transfer as before; I behavior and unused F-keys
  remain intact. F9 in all inventory/transfer/pending states does no capture/I/O
  and does not queue a save. Reentrant callbacks cannot E/T/open/save/start events.
- Inject layout/catalog/report/compose failures before and after publication.
  A prepublication failure changes nothing; a postpublication failure followed by
  reopen/rebuild shows exactly one changed frame. Failure of the one clean-base
  recovery is fatal and unsaveable. Observers are not required for recovery.
- After changed maxima, force recomposition and close; current HUD and underlay
  use new maxima without current-HP/SP clamp. Preserve stationary outdoor timing,
  opaque-overlay semantics, NPC timing when E is blocked, and Nightshadow's
  static gravestone/ordinary clue interaction after inventory closes.

### Save/restart and independent original oracle

Extend the existing save/resume coordinator with distinct equipment checkpoint
names; retain all current transfer checkpoint meanings. A primary `equipment`
checkpoint uses the dagger sequence, then removes Arturius's boots and Zippo's
Silver ring and leaves them removed for saving. Its expected snapshot starts
from independently loaded original defaults at a disclosed existing diagnostic
camera (reuse the harness Myra camera without invoking its event). Change only:

```text
owner 11 Weapons slot 0: {0,12,0,0}
owner 11 Weapons slot 1: {0,12,0,1}
owner 0 Armor slot 3:    {38,10,0,0}
owner 11 Accessories slot 1: {42,1,0,0}
```

Everything else, including membership, signed HP/SP, quest/game flags and world
overlays, equals the fresh control. First exercise reversible remove/re-equip
success for boots/ring before leaving them removed. Independent secondary
original controls cover proficiency, bow, charm remapping and ring limits.
Persist/restart the ring-control final arrays separately to cover frame 7 and
two coexisting frame-8 items. Synthetic connected save cases cover new frame 13,
bad states, numeric changes, holes/opaque bytes, aliases/inactive owners and
current values above maxima; existing v1/v2 compatibility remains required.

Use existing full-character/party/snapshot comparators and real
`XeenSaveFile::read` on disk. Build expected states from fresh independent inputs
and literal listed changes, never by running the new helper, capturing mutated
producer owners, or treating the actual decoded save as expected. Assert no
hidden save while inventory is open, close then new F9, and wait for producer
termination before the consumer. Consumers reconstruct fresh owners, reopen
inventory and perform cache reconstruction; actual CLI `--load-game` also
reopens via I and checks emitted/raw state. A fresh process without load retains
original equipped defaults. All consumers/fresh controls preserve producer bytes.

## Physical maintainer acceptance

Add a bounded `--manual-equipment <game-directory> <new-save-path.mmsave>` mode
to the existing save/resume smoke, reusing its continuous real SDL loop. No
synthetic inventory setup, injected keys, automated acknowledgment/save/closer
or alternate persistence route. Only diagnostic camera positioning is setup;
this does not certify travel or safe encounters. Save parent exists outside the
commercial installation; existing protected-target policy remains.

1. Open I, F4 Weapons: see both Daggers and initial equipped mark. Select slot 2
   (UI key 2), E: observe conflict and unchanged marks. Reselect key 1/E removes
   first; key 2/E equips second. A further E without selection changes nothing.
2. F1 Armor: remove/re-equip Leather boots, then leave removed. F4 Accessories:
   remove/re-equip Silver ring, then leave removed. Verify visible statuses/raw
   frames and unchanged current/max HP/SP for these original nonmodifier items.
3. While open, press F9 and observe refusal. Close with Escape, verify intact
   scene/HUD, then press a **new F9** and observe successful save. Exit completely.
4. Start separate production `mmodern --load-game <game-directory> <same-save>`.
   Reopen inventory: first dagger unequipped, second equipped, boots/ring removed
   at their original physical slots. No modal/confirmation/action is replayed.

The harness must reject early exit or skipped required operations/F9 observations.
Automated original controls cover the more involved ring/proficiency sequences;
synthetic tests establish modifier changes and missing curse/two-handed/medal
cases. Physical acceptance need not claim those unavailable original cases.
Separately observe Nightshadow inventory close and ordinary clue interaction if
the shared view/input changes affect that existing regression boundary. Record
maintainer physical observations, independent review, typed automated assertions
and image inspection separately; none substitutes for the others.

## Risks, exclusions and completion

Architecture review must specifically confirm the explicit adaptation decisions:
Clouds equip IDs 1..34 rather than catalog Elder coverage; removal of occupied
unknown IDs; raw-frame anomalies/empty metadata retained; E contextual removal
and reselection discipline; bounded source-derived masks; checked one-byte
publication; and two separately authorized stages. These are proposed decisions,
not unresolved original-rule questions.

Replan before implementation if review establishes missing authoritative durable
state, a legality prerequisite requiring unmodeled statistics/effects, required
Elder/game expansion, incompatible save validation, or a source mismatch. Do not
silently fix anomalous originals or treat malformed catalog inputs as illegal
equipment. Test feedback at numeric bounds before accepting any layout change;
retain complete HP/SP and M24 detail wrapping. Newly discovered production defects
outside this bounded contract require separately scoped work.

Explicit non-goals: transfer behavior changes; Misc/potion/item use; discard;
repair; shops/trading, paid identification, enchant/recharge or gold conversion;
spell/effect activation; combat/monster state; damage, attack bonus, armor class,
hit chance, resistances or full attributes/character sheet; new currencies;
recruitment/reordering; new save format; Darkside/Swords gameplay; mouse UI;
broader original route certification; and any work beyond M25. No later milestone
is designed here. Source tables needed only for those features are not imported.

Definition of done for eventual M25 closure: 25A and 25B separately authorized
and accepted; exact reference/adaptation tests and original controls pass;
build/full CTest pass; real disk/separate-process/CLI evidence matches independent
oracles; physical maintainer acceptance and independent review are complete;
durable documentation is updated according to AGENTS.md. This planning candidate
does not fulfill those implementation gates and authorizes no commit/push/tag.

**Unresolved original behavior/prerequisites: none within the specified bounded
contract.** Proposed adaptation decisions remain subject to architecture review.

**Implementation-readiness verdict: READY FOR ARCHITECTURE REVIEW.** Evidence is
sufficient to review the specification and prepare implementation tasks. It does
not authorize implementation of either stage.
