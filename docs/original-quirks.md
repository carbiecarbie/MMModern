# Original quirks

MMModern reproduces the original DOS game, including its oddities. This page
records behavior that looks like a bug but is kept on purpose, so that nobody
"fixes" it by accident, and lists the cases where the pinned ScummVM reference
itself differs from the original and is not followed. Entries that could later
become opt-in quality-of-life fixes (always off by default) are marked
**QoL candidate**. Add an entry when a milestone deliberately keeps or rejects
such a behavior, with its evidence.

## Original behavior reproduced on purpose

| Behavior | Evidence | Where | Notes |
| --- | --- | --- | --- |
| AlterHed aliases indoor megamap coordinates through one primary-map head table indexed by `(x % 16, y % 16)`. | Pinned `scripts.cpp::cmdAlterHed` and `map.cpp::Map::load`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; ordinary DOS comparison pending. | M59 proposal, World head cache | Unlike exploration, head indicators are not keyed by neighboring physical tile. Reload the original HED on an ordinary map load. |
| Vern's guild offer has no membership check before charging 25 gold and granting membership again. | CD `maze0028.evt` `[648,715)`, selected-character debit 34/25 then award 15/0; pinned `party.cpp::giveTake`. Initial price/selection confirmed by maintainer DOSBox CD play, 2026-10-10; repeat payment awaits ordinary DOS comparison. | M59 proposal, existing generic Event debit/award rules | Preserve the script's repeat behavior; do not invent a repeat-membership refusal. |
| Jail's year penalty directly advances the year and resets yearly flags and temporary effects; it does not simulate a year's daily ticks. | Pinned `party.cpp::giveTake` mode 85, `resetYearlyBits`, `resetTemps`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Maintainer DOSBox CD play, 2026-10-10 confirms one year, jail destination and ordinary exit; detailed reset/time comparison remains pending. | M59 proposal, generic Event year advance | Keep day/minutes; no invented daily condition rolls, merchant regeneration or bank interest during the jump. |
| Rialdo advertises 2,500 gold for Pathfinder but charges 2,000. | Maintainer DOSBox CD play, 2026-10-10/11; CD `maze0028.evt` at `(25,26)` North debits mode 34 value 2000, then grants mode 13 skill 11. | M58, generic teacher Event rules | Preserve the resource text and actual charge. |
| Mylo and Rialdo charge for repeat lessons and increment the skill again. | CD `maze0028.evt` byte ranges [514,581) (Mylo) and [581,648) (Rialdo), each containing Event lines 0..6, have no learned-skill test; pinned `scripts.cpp::cmdTakeOrGive` and `party.cpp::giveTake` mode 13 increment and continue. Maintainer scope clarification, 2026-10-11: follow these scripts, superseding the M58 plan's once-only assumption. | M58, `XeenEventInterpreter.cpp`, `XeenWorld.cpp::applyEventGrant` | No idempotent learned flag or repeat-lesson refusal. Ordinary DOS repeat-lesson comparison remains pending. |
| Party skill checks count holders regardless of condition. Pathfinder needs two holders, but the reference does not test it in normal movement. | Pinned `party.cpp::checkSkill`, `scripts.cpp::ifProc` action 99, and `interface.cpp::canMove`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. | M58, `XeenSkills.h`, party skill queries | No invented forest restriction or discount. These reference rules await ordinary DOS comparison. |
| Event skill comparisons use the original numeric sentinels: selected-character action 13 returns the skill ID or 0; party action 99 returns the ID or unsigned -1. | Pinned `scripts.cpp::ifProc` initializes `v` to 0 and compares the unsigned result directly. | M58, `XeenEventInterpreter.cpp` | An absent skill 0 can satisfy action 13 equality to 0; an absent party skill can satisfy a greater-than comparison. Preserve the script comparisons; ordinary DOS comparison remains pending. |
| The indoor minimap draws its partial left/top wall loop once, uses frame 24 for the left TFLR edge, and the indoor Map marker tests row `14 - arrowY / 10` despite eight-pixel tiles. | Pinned `interface_minimap.cpp::draw` and `dialogs_map.cpp::execute`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. | M58, `XeenMapView.cpp` | Preserve the reference placement and loop bounds; compare the visuals during the maintainer's DOS play-test. |
| Map lookup treats sky and space surfaces as visible without recording exploration. | Pinned `map.cpp::mazeLookup` sets its disposable stepped lookup result for these surfaces. | M58, `XeenMapView.cpp::lookup` | Both views preserve this display rule without writing explored cells or importing cached seen bits. |
| In the save/load chooser, the scroll arrows show their pressed frame but do nothing, and the bar between them does nothing; only ten slots exist. | Maintainer DOSBox CD play, 2026-10-09 (an eleventh slot pair is ignored). | M54 chooser | QoL candidate (scrolling or more slots). |
| After a member runs from combat, the Quick Fight Options dialog keeps the old party indexing: F1-F5 and the portraits select the members by their pre-Run positions, so the visible portrait and the selected member differ; F6 does nothing. | Maintainer DOSBox CD play, 2026-10-10; matches `dialogs_quick_fight.cpp:97-99` in the reference. | M55, `XeenCharacterDialogFlow.cpp::handleQuickFightOptions` | QoL candidate. Combat actions still use the real actor. |
| A valid Bank transfer adds to its destination using unsigned 32-bit arithmetic, so the destination can wrap. This is distinct from refusing an entered amount above `UINT32_MAX`. | Pinned `locations.cpp::BankLocation::depositWithdrawl` adds to the party's unsigned balances; M56 retains that balance arithmetic. | M56, `XeenFoodBank.cpp::xeenTransferBank` | No saturation or destination-capacity refusal. |
| `MoveObj` at an object's current position toggles its flip, but the next scene draw replaces that flip with the directional animation metadata, including for off-screen objects. | Pinned ScummVM `scripts.cpp::cmdMoveObj` and `interface_scene.cpp::drawScene` (revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`); maintainer confirmed following these resets on 2026-10-10. | M57, `XeenWorld.cpp::applyMoveObject`, `XeenIndoorScene.cpp`, `XeenOutdoorScene.cpp` | Position changes remain durable. Flip observations are disposable; saves retain the effective flip without serializing animation frames. |
| Event XP and carried currency additions use unsigned 32-bit arithmetic and can wrap. Award IDs 73 and 81 name slots 126 and 127; granting an award sets its count to 1 rather than incrementing it. | Pinned `party.cpp::giveTake`, `character.cpp::fixAwardId/setAward`. | M57, `XeenWorld.cpp::applyEventGrant`, `XeenCharacter.h::awardIndex` | CHR counts are retained for all 30 owners, including the full-byte Warzone count in slot 9. |
| Full backpacks discard the remaining staged treasure, including gold and gems. Capacity is checked at every category slot, even an empty slot; filling the very last miscellaneous slot has no following check. | Pinned `party.cpp::giveTreasure` at revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; M57 explicitly retains its full-pack behavior. | M57, `XeenItemRewards.cpp::xeenDeliverRewards` | Items are generated before capacity loss, so their RNG draws still count. The fallback described as allowing unconscious members still checks `isDisabledOrDead()`. |

## Unconfirmed in DOS, explicit maintainer policy

| Behavior | Evidence and decision | Where |
| --- | --- | --- |
| Skills use one byte each; incrementing 255 wraps immediately to 0, with identical live and restored values. | CHR/DOS save byte storage; explicit maintainer policy, 2026-10-11: "follows the original byte storage; not confirmed in DOS". Repeat paid lessons follow the scripts under the later scope clarification above; the overflow boundary itself remains unconfirmed in DOS. ScummVM's `int _skills[18]` increment followed by `syncAsByte` truncation is not reproduced. | M58, shared skill grant site (quoted marker) and v10 skill serialization. |

## Unconfirmed in DOS, following the reference

Cases the original cannot be checked for in normal play; they follow the
pinned ScummVM reference under the AGENTS.md condition-counter rule.

| Behavior | Where | Notes |
| --- | --- | --- |
| A condition byte `0xFF` counts as the `-1` sentinel: it suppresses the Weak/Drunk change and the dawn Weak increment, and adds 1 in stat modifications. | M51, `XeenCombatRules.cpp`, `XeenCharacterRules.cpp` ("follows ScummVM; not confirmed in DOS") | Only reachable with extreme counters. |
| The eight-hour Poison and Disease checks only draw for members who do *not* have the condition; poisoned or diseased members neither worsen nor recover over time. | M51 daily time | Looks inverted in the reference; a DOSBox check of a poisoned member over several hours would settle it. |

## Reference behavior not reproduced

Places where the pinned ScummVM code differs from the DOS original or contains
a port defect; MMModern follows the original instead.

| Reference behavior | MMModern | Where |
| --- | --- | --- |
| `FMT_CHARGES` carries a redundant alignment letter that draws a stray glyph in item titles. | Uses the DOS bytes from `WORLD/XEEN.DAT`, without the glyph (DOSBox-confirmed). | M47, M53 |
| `ItemsDialog::setEquipmentIcons` sets an accessory frame through `_id = 8`. | Uses the item's own frame. | M47 |
| `CharacterInfo::execute` forces combat mode after every stat popup. | Restores the caller's mode. | M47 |
| Repair's `calcItemCost` passes `actionIndex` as the Merchant skill. | Keeps the M38 Armor Repair prices. | M47 |
| The control panel drops Save As and rearranges the buttons; saves use ScummVM's own chooser. | DOS panel and chooser from `WORLD/XEEN.DAT` and DOSBox evidence. | M54 |
| Info chooses its title by engine game ID, and its Walk on Water format takes three arguments. | CD Clouds Info uses the World title (maintainer DOSBox play, 2026-10-10). The bounded DOS fields at `[0x53673,0x53679)` and `[0x535d4,0x535e6)` supply that title and the two-argument Walk on Water format. | M55 Part A, `XeenDosText.cpp`, `XeenDialogView.cpp` |
| The pinned English bank constants spell Withdrawal as `Withdrawl` and use a different refresh control prefix. | Preserve the installed CD DAT wording at `[0x51b51,0x51b5c)` and refresh template at `[0x52b32,0x52b79)`, verified read-only on 2026-10-10 against `en_constants.h::DEPOSIT_WITHDRAWL/GOLD_GEMS_2`. | M56, `XeenDosText.cpp`, `XeenDialogView.cpp` |
| Bank amount entry parses with signed `atoi`. | DOS scans `%lu`; depositing `4294967296` or `4294967306` with more than 10 gold gives the "not enough" refusal, with no wrap (maintainer DOSBox CD play, 2026-10-10). Such amounts are refused as excess. | M56, `XeenFoodBank.cpp::xeenBankAmount/xeenTransferBank` |
| `_currentCantRest` tests a flag that the cell loader can never set, so only the map-wide flag refuses Rest. | Same effective behavior: only the primary map's flag refuses Rest. | M51 |
