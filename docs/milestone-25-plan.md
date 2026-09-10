# Milestone 25 - Bounded equipment management and existing-rule feedback

## Final status and scope

**Milestone 25 is complete.** Stage 25A and stage 25B are accepted. Independent
technical review and maintainer physical acceptance are complete.

M25 adds player-triggered equip and remove for bounded Clouds equipment through
the existing M24 inventory panel. It preserves the existing roster, item,
character-rule, Flow, SDL and save owners. Stage 25A introduced legality and the
synchronous one-byte mutation/result foundation; stage 25B integrated contextual
input, transient authorization, feedback, recovery and production restart.

Current architecture and public behavior are summarized in
[project status](project-status.md). M24 transfer remains governed by the
[closed M24 plan](milestone-24-plan.md), item ownership and v2 persistence by
[M21](milestone-21-plan.md#21a-item-ownership-and-storage), and dependency pinning
and restriction-data provenance by [dependencies](dependencies.md).

## Reference provenance and bounded adaptation

The accepted behavior is adapted from pinned ScummVM revision
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`:

- [item.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.h)
  defines item categories, item state, supported inventory bounds and the
  nonzero-frame equipped representation.
- [item.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp)
  supplies the `passRestrictions`, `removeItem` and category-specific equip
  behavior used as the legality and mutation oracle.
- [dialogs_items.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp)
  establishes contextual item interaction and keeps equip/remove distinct from
  discard.
- [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp)
  supplies the decimal `ITEM_RESTRICTIONS` and `RESTRICTION_OFFSETS` values.

The exact adapted numeric data, licensing and distribution obligations are in
the [dependency provenance record](dependencies.md#bounded-equipment-restriction-data).
Commercial game resources are not copied into these tables.

## Equipment domains and frame contract

Equip supports these bounded category/ID domains:

| Category | Supported Equip IDs | Equipped frame |
| --- | ---: | --- |
| Weapons | 1..34 | subtype frame 1, 4 or 13 |
| Armor | 1..13 | subtype frame 2, 3, 5, 6, 9 or 10 |
| Accessories | 1..10 | subtype frame 7, 8, 11 or 12 |

Weapons 35..40 remain catalog-visible but cannot be equipped. Armor 14 and
Accessories 11 and higher are also unsupported. Miscellaneous remains outside
equipment actions: M25 does not treat its frame as equipment state or item use.

Any occupied equipment record with a nonzero raw frame is contextually removable,
including an unknown ID, unless cursed. This keeps recovery possible for opaque
or future records without extending their Equip domain. ID zero is empty for the
selected-action gate even when its other bytes are nonzero.

Equipment state is the existing raw frame byte. A successful operation changes
only the selected record's frame and preserves its material, ID and state bytes,
its physical slot, every bystander, every hole, membership and unrelated state.
Equipment never compacts, exchanges slots, clears empty metadata or normalizes a
loadout. Loading and viewing preserve anomalous frames exactly.

M24 transfer is a separate operation. It resets the moved frame and stable-compacts
both touched category arrays while preserving occupied order. Transfer retains
its own destination-capacity, alias and confirmation rules and never calls the
equipment helper.

## Legality and conflict contract

Weapon IDs 1..34 and Armor IDs 1..8 apply the accepted source-derived class masks.
Armor IDs 9..13 and Accessories IDs 1..10 have no proficiency mask because their
reference equip branches do not call the proficiency check. The masks and mapping
remain bounded to the accepted Clouds domain; catalog coverage does not broaden
legality.

Class IDs are Knight 0, Paladin 1, Archer 2, Cleric 3, Sorcerer 4, Robber 5,
Ninja 6, Barbarian 7, Druid 8 and Ranger 9. Knight and Paladin always pass. For
classes 2..9, the following exact masks identify forbidden classes:

| Category / IDs | Mask | Forbidden classes |
| --- | ---: | --- |
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
| Armor 6..7 | 255 | All classes 2..9 |
| Armor 8 | 85 | Archer, Sorcerer, Ninja, Druid |

The stored weapon-mask sequence for IDs 1..34 is
`86,86,86,86,86,86,0,6,239,239,239,2,4,4,4,4,6,70,70,70,70,94,70,0,4,239,86,86,86,70,70,70,70,0`;
Armor IDs 1..8 use `0,68,100,116,125,255,255,85`. The adapter imports only
these 42 values, not the complete reference table or a runtime data dependency.

Equip validates structure, selected occupancy, supported domain and proficiency
before category conflicts. Conflict scans use authoritative raw frame values and
ascending physical slots, including cursed, broken, unknown-ID and ID-zero
bystanders when their frame belongs to the relevant group:

- melee weapons conflict with another melee weapon;
- shields conflict with another shield and with an equipped two-handed weapon;
- two-handed weapons conflict with an equipped shield;
- bows coexist with melee weapons and shields;
- body armor, helm, boots, cloak and gauntlets conflict within their subtype;
- ring-frame and medal-frame groups each permit two equipped records and refuse
  another when the group is already full.

Raw frames define these conflicts. M25 does not repair a mismatched ID/frame,
infer intended equipment or add a general equipment-count rule. A direct explicit
Equip of an already equipped singleton can conflict with itself; contextual player
E removes any selected nonzero frame instead.

Cursed or broken items may be equipped if other checks pass. Cursed Remove is
refused; broken-only Remove succeeds. Curse/broken state also suppresses existing
modeled bonuses independently of legality. Character conditions and current HP/SP
do not gate an equipment action.

## Mutation, ownership and result contract

`xeenSetEquipment` is the authoritative synchronous mutation operation over
`XeenPartyState`. It resolves an active membership reference to the authoritative
roster owner, so aliases observe one shared mutation and are not counted twice.
There is no separate equipment owner or loadout registry.

The helper accepts an explicit Equip or Remove request and returns an owned,
typed `XeenEquipmentResult` containing the status, operation, owner, category,
physical slot, exact before/after items, applicable conflict and modeled before/
after values. It validates and prepares every fallible result field before the
store. Success then publishes exactly the selected frame byte. Result adoption,
observation and presentation happen after publication; an observer is never part
of the domain transaction.

Preparation failure publishes nothing. After publication, result construction
and return are nonthrowing. The caller owns post-publication reporting and visual
recovery and must never rerun the mutation to reconstruct a result.

## Player input and transient selection safety

Within inventory Browse mode, E is contextual:

```text
selected raw frame == 0  -> Equip
selected raw frame != 0  -> Remove
```

Every action requires a new explicit physical-slot selection. Selection arms a
transient certificate containing the inventory generation, complete active
membership, resolved owner, category, physical slot and exact selected item.
E consumes the certificate before calling the domain helper, including refused
and failed attempts. A second E without reselection cannot toggle or replay an
operation.

Membership, owner, category, slot, item-byte or generation drift invalidates the
certificate. A formerly occupied record that becomes empty follows stale-selection
cleanup: the domain helper is not called, the slot and displayed record are
cleared, and the player is told to select again. A selection that was originally
empty retains ordinary empty-selection feedback. Recomposition that does not
change authoritative facts may retain a valid selection, but never rearms a
consumed certificate.

Miscellaneous rejection takes precedence over empty-item handling. E cannot
confirm transfer, acknowledge an event, open closed inventory or dispatch Space.
Pending event, NPC, WhoWill and reward presentation continues to block inventory
actions. F9 while inventory is open remains an immediate refusal with no capture,
I/O, cancellation or deferred save; closing inventory requires a new F9.

## Feedback and modeled consequences

Inventory rows and selected details show the raw equipped frame and contextual
Equip/Remove action. Feedback reports the typed operation/refusal and only the
currently modeled consequences:

- effective Intellect and Personality;
- maximum HP derived through existing character rules;
- maximum SP derived through existing character rules.

Successful actions recompute these values from authoritative frame bytes. Current
HP and current SP remain exact even when they exceed a new maximum or are negative;
equipment never heals, clamps or normalizes them. Endurance has no equipment bonus
category and is not reported as an equipment delta.

M25 does not invent weapon damage, attack bonus, hit chance, armor class,
resistance, spell activation or other unmodeled effects. A successful action can
therefore have no displayed numerical delta. Cursed/broken equipped records retain
their bytes but do not contribute modeled bonuses.

## Flow, presentation and recovery invariants

`XeenEventFlow` remains the noncopyable continuation and transient inventory owner.
The existing Application action router and one SDL loop deliver E; no nested loop
or parallel gameplay state is introduced. The inventory view formats borrowed
party state plus the Flow-owned result and feedback.

Synchronous mutation is guarded against reentrant equipment, transfer, inventory,
save and event entry. Publication precedes fallible reporting, catalog formatting,
drawing and composition. A failure before publication changes no durable state. A
failure afterward preserves the one published frame and clears transient inventory
presentation before attempting one clean-base rebuild. Recovery never relies on
the failed observer and never repeats the action. Failure of that clean rebuild is
fatal and remains unsaveable.

Equipment result values own their data; no result or formatted feedback borrows a
destroyed request or callback. Scene refresh and inventory close rebuild from
authoritative owners, so HUD and underlay show current modeled maxima without
changing current HP/SP. Ordinary outdoor and NPC timing remain independent of
equipment input.

## Persistence and compatibility

M25 introduces no save-format or compatibility change. The existing MMModern
Clouds v2 format already stores every material/ID/state/frame byte for all four
item categories and all 30 roster owners. Successful equipment therefore survives
production F9 and process restart through its saved frame byte.

Equipment certificates, selected slots, results, feedback and modal state are
transient and never serialized. Resume reconstructs independent roster owners and
derived presentation from saved values, starts with inventory closed, and does not
rerun Equip, infer conflicts or normalize a loadout. Existing v1 restoration and
v2 validation remain governed by the [M21 persistence contract](milestone-21-plan.md#save-v2-and-legacy-v1-compatibility).

## Accepted original-data behavior

Original World of Xeen data supplied these durable Clouds anchors:

- Zippo, authoritative owner 11, starts with Dagger records in Weapons physical
  slots 0 and 1: `{0,12,0,1}` and `{0,12,0,0}`. The second refuses Equip while
  the first is equipped. Removing the first permits equipping the second without
  moving either slot; duplicate E without reselection does nothing.
- Arturius, owner 0, Leather boots in Armor physical slot 3
  `{38,10,0,9}` complete Remove -> Equip -> Remove as
  `{38,10,0,0}` -> `{38,10,0,9}` -> `{38,10,0,0}`.
- Zippo's Silver ring in Accessories physical slot 1 `{42,1,0,8}` completes
  Remove -> Equip -> Remove in the same slot.
- Original Badger bow, Rebecca proficiency/charm and transferred ring/medal-group
  controls establish additional source-derived frame, class and capacity behavior.
  Synthetic fixtures cover cursed/broken states, two-handed/shield conflicts,
  full class/domain matrices, opaque bytes and modeled modifier contrasts that
  initial original data cannot isolate.

The accepted production checkpoint saves Zippo Weapons slots 0/1 at frames 0/1,
Arturius Armor slot 3 at frame 0 and Zippo Accessories slot 1 at frame 0. Separate
producer, consumer, fresh-session and actual `mmodern --load-game` processes prove
exact slot/frame restoration, no modal/action replay and unchanged save bytes in
read-only consumers.

## Exclusions

M25 does not provide Miscellaneous item use, potions/antidote effects, discard,
repair, shops/trading, paid identification, enchant/recharge or gold conversion,
spell/effect activation, random loot, complete equipment effects, weapon damage,
attack bonus, hit chance, armor class, resistance, combat/monster state, a complete
character sheet, recruitment/reordering, mouse UI, a new save format, Darkside or
Swords gameplay, or broader original-route certification. It does not import
reference tables needed only for those features and performs no general loadout
normalization.

## Final acceptance

- Stage 25A's legality, restriction provenance and synchronous one-byte mutation
  foundation were independently reviewed and accepted.
- Stage 25B's contextual Flow/SDL integration, stale-selection protection,
  feedback, recovery and save/restart lifecycle were independently reviewed and
  accepted after bounded corrections.
- Focused equipment, inventory and SDL regressions passed. The complete configured
  CTest suite passed **69/69**.
- Original-data equipment controls and direct plus dummy/software SDL production
  restart passed through distinct producer, consumer, fresh-session and actual CLI
  process boundaries.
- The maintainer physically completed the Dagger refusal/remove/equip and duplicate
  E control; Leather boots and Silver ring Remove -> Equip -> Remove sequences;
  open-inventory F9 refusal; new post-close F9 save; and separate production
  restart with exact slots/frames and no replay.
- The maintainer also physically confirmed the Nightshadow inventory open/close
  regression and the original map 29 `(4,6)` West gravestone clue interaction.

These gates complete Milestone 25.
