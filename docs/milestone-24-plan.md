# Milestone 24 - Usable party/item inspection and character-to-character transfer

## Goal, authority and baseline

**24A accepted; 24B remains pending separate authorization and implementation.**
M23 remains the latest fully completed milestone; M24 is not complete. This plan
records the accepted bounded-catalog foundation and preserves the future 24B
transfer, interface and persistence specification. The maintainer authorizes
stages separately; acceptance of 24A does not authorize 24B or M25.

The standalone application will display active characters' modeled condition and
current/maximum HP/SP, list all nine slots in each of four inventory categories,
identify supported records from resources, and move one selected item to another
active roster owner. Success and refusal must be visible. Explicit disk save and
a separate process restart must preserve exact resulting ownership.

M24 builds on the accepted M23 architecture at MMModern baseline
`a320dd0478998373b328f8610c88c34ae125eff8`. Actual code/tests establish implemented
behavior; [project status](project-status.md) owns stable capabilities and
[roadmap](roadmap.md) owns future direction. References below use pinned ScummVM
`6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The supported build configuration and
reproduction procedure belong in [dependencies](dependencies.md).

## Existing architecture and missing responsibilities

All MMModern paths in this table are relative to the verified repository root.
They describe current interfaces; proposed responsibility names are not new API
requirements.

| Concern | Existing owner and evidence | M24 delta |
| --- | --- | --- |
| Items | `src/games/xeen/XeenCharacter.h/.cpp`: `XeenItem`, four `XeenItemCategory` arrays, `xeenItemHasTailCapacity`, `xeenCompactItems` | Reuse exact four-byte records and nine-slot arrays; the 24A catalog uses a distinct four-valued `XeenInventoryCategory` discriminator and adds no inventory owner or storage format |
| Identity | `XeenParty.h/.cpp`: `XeenRoster::at`, `XeenParty::activeRosterIds`, `member`, `fromRosterIds`; `XeenPartyState` | Resolve active references to roster owners at action time; reject self-owner transfers including aliases |
| Loading | `XeenPartyLoader::loadInitialCloudsParty/loadFromResources`; `src/formats/xeen/XeenCharacterFormat.cpp` | No change to CHR offsets 166/202/238/274, all 30 owners, or alias loading |
| Rules | `XeenCharacterRules.cpp`: `validateForUse`, effective intellect/personality/endurance, `maxHp/maxSp`; `XeenCharacter::worstCondition` | Read current rules; recompute both affected owners after mutation, including empty-metadata removal effects |
| Diagnostics | `XeenItemRewards.cpp`: `xeenInventoryInspection`, `xeenInventorySummary`; `Application::playGameplay` in `src/app/XeenGameplay.cpp` | Retain complete numeric diagnostics; add actual player presentation |
| Rewards | `XeenItemRewards.cpp`, `XeenEventInterpreter.cpp`, `XeenEventSystem.cpp`: queue, `xeenDeliverRewards`, insertion, receipt lifecycle | No producer/delivery framework changes; manual transfer has a different eligibility policy |
| Input | `src/core/PlayerAction.h`, `src/platform/sdl/SdlWindow.cpp::playerAction/showLoop` | Add bounded inventory actions and Escape ownership; preserve one event loop and ignored key repeats |
| Session and modal flow | `src/app/XeenGameplay.cpp`, `XeenGameplayServices.h`, `XeenEventFlow.h/.cpp`: `handle`, `blocksGameplay`, `refresh`, `updatePresentation`, `handlesEscape` | Flow owns transient inventory state alongside mutually exclusive event presentation; Application retains save ownership |
| Drawing | `XeenEventPresenter`, `XeenTextRenderer`, `CloudsUiComposer`, `CloudsUiLayout.cpp`, `CloudsMapComposer` | Small inventory renderer using existing font/window/frame facilities; no fake script opcode or character-sheet subsystem |
| Resources | `XeenAssetSource`, `src/compat/scummvm/ScummVmXeenBridge.cpp`: checked byte streams, lazy Dark archive metadata access | 24A adds the specifically named, read-only `DARK.CC/mae.xen` read and immutable catalog loader |
| Saves | `XeenSaveState::capture/restoreBeforeGameplay`, `XeenSaveFormat`, `src/platform/XeenSaveFile.cpp` | Existing arrays suffice; no wire change, catalog fingerprint or serialized UI |

Storage accepts every uint8 item byte. ID zero means empty even when the other
three bytes are nonzero. Only slot 8 decides capacity. Explicit compaction is
stable in occupied order and zeroes all remaining empty records. Loading,
inspection, save capture and restore do not compact. Active membership is at
most six references to 30 authoritative owners; duplicates alias the same arrays.

Current rules use material 59..130, nonzero frame and neither curse nor breakage
for equipment bonuses. They intentionally do **not** require nonzero item ID.
Miscellaneous arrays contribute no effects. Endurance receives no equipment
bonus; intellect/personality and direct HP/SP bonuses already exist. Catalog
support must never become an extra gate on these accepted rules.

The remaining 24B responsibilities are one synchronous validated character-item
move and transient modal selection/rendering wired into production Flow/Application.
Put item legality beside character/party rules in `src/games/xeen`; parsing belongs under
`src/formats/xeen`; resource access stays behind the compatibility boundary.
Use a small view helper if necessary to keep Flow readable. Do not introduce a
service registry, generic transaction engine or persistent selection identity.

Accepted 24A implements bounded read-only catalog composition: source generation,
material parsing, immutable structured descriptions, the explicit asset read and
test/smoke consumers. It does not modify `PlayerAction`, SDL, Flow, Application,
live inventory mutation, save formats or production save/resume behavior.

## Catalog source and delivery decision

### Verified source chain

Primary pinned sources:

- [resources.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/resources.cpp)
  `Resources::Resources/loadData`, and
  [resources.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/resources.h)
  `ResFile`: English language ID 7, `CONSTANTS_7` arrays and material-name loading.
- [en_constants.h](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/en_constants.h)
  `EN::{WEAPON_NAMES,ARMOR_NAMES,ACCESSORY_NAMES,MISC_NAMES,SPECIAL_NAMES,
  BONUS_NAMES,ITEM_BROKEN,ITEM_CURSED,ITEM_OF}` and
  [constants.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/constants.cpp)
  `LangConstants::writeConstants` and free `writeConstants` supply generated text.
- [clouds.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/clouds.cpp)
  `writeCloudsData` copies commercial `dark.cc/mae.xen` verbatim to `mae.cld`.
  English `EN::CLOUDS_MAE_NAMES()` is an empty 131-entry placeholder, not a usable
  substitute. The special Russian table is outside this English contract.
- [create_xeen.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/devtools/create_mm/create_xeen/create_xeen.cpp)
  writes version `1.3` and invokes constants, Clouds extraction and Swords
  extraction. `engines/mm/xeen/files.cpp::FileManager::setup` loads `mm.dat`,
  subfolder `xeen`, version 1.3. This is packaged engine data, not a CC archive.

The checked-in `devtools/create_mm/files/xeen/CONSTANTS_7` is Git blob
`455b2eb3900a60be910e4d045d103a73586e73b0`, 35,065 bytes, SHA-256
`a3022d378e7570a56332f30128942afe02ae2bfdef70c307b2de997eb07a9e77`.
Its catalog block is byte range [20680,22438), comprising three NUL-terminated
strings followed by six tagged NUL-string arrays in the order above: bonus 7,
weapon 41, armor 14, accessory 11, misc 22, special 74. Array tags are the four
bytes `00 00 00 count` at this pin; the reference reads them as little-endian
MKTAG values. These offsets corroborate the source getters; M24 will not implement
a parser for the entire unversioned constants stream.

Read-only original-data evidence found `DARK.CC/mae.xen` exactly equal to pinned
`files/xeen/mae.cld`: 1,093 bytes, 131 NUL-terminated entries, maximum entry length
13, no trailing fragment, SHA-256
`78f3ec8421fd46619a4aba63dca1042c914b0f3437d91ce37a47f81676bd4156`.
Examples include material 0 empty, 38 leather, 42 silver, 69 clever, 105 vigor
and 110 spell. Equality certifies this installation's Clouds lead; it does not
make the extracted file redistributable.

### Chosen architecture

**Build-generated embedded English name tables plus a runtime commercial
material-name resource.** A build-only PowerShell generator reads the exact
pinned `CONSTANTS_7` Git blob and emits only the six arrays and three
decoration/connective strings (169 array entries plus three strings) into an
ignored generated C++ include. Compile that immutable data into MMModern. The
generator does not compile or preprocess ScummVM headers and does not link or
execute ScummVM code.

The build-only generator uses explicit category/count metadata, emits a
schema-1/English-7/exact-source-revision marker, escapes bytes as fixed-width
octal data, and replaces its output atomically only after complete validation.
It does not emit material names or any commercial bytes. The generated include
is build-tree-only, ignored and never checked in or installed as a companion file.

Configure time and every build require the exact repository revision, tree entry
and blob identity. The generator reads the object with `git cat-file`, checks its
size and SHA-256, then validates the bounded block. Worktree and compiler inputs
do not supply catalog bytes. The pinned-blob design replaced compiler-based
derivation because it gives a smaller deterministic provenance boundary.
Generation uses Windows PowerShell/.NET and Git; the reproduction and atomic
publication contract is in
[dependencies](dependencies.md#build-generated-english-item-catalog).

At runtime, the explicit Dark-archive seam exposes **only `mae.xen` from
`DARK.CC`** through `XeenAssetSource` and the existing `ScummVmXeenBridge` archive
owner. This optional member uses checked extent/read handling; parsing into owned
immutable strings stays outside the compatibility layer. The catalog uses no
engine `Resources`, `File`, `g_vm`, `g_resources`, or Xeen engine linkage.

The existing game-directory argument locates commercial material text. The
executable carries the source-derived tables when moved outside the checkout;
**no new runtime CLI option, search path, environment variable or companion
catalog file** is required. A Clouds-only installation without Dark material
data still runs with the explicit fallback below. Acceptance uses the verified
World of Xeen installation and requires complete names.

### Provenance and distribution

The selected getter definitions have explicit ScummVM GPLv3-or-later source
headers. Preserve their contributor attribution, source paths, exact revision,
license notice and corresponding generator/input source availability in future
dependency/distribution documentation. Generated text retains that attribution;
shipping an executable does not remove corresponding-source obligations.
Do not infer a license for every member of `mm.dat` from its container.
Commercial `mae.xen`, its extracted `mae.cld`, original archives and probe output
containing resource payloads remain external. Do not bundle them, generated
commercial tables or original-data test fixtures. M24 must not copy the full
ScummVM source tree.

### Naming and bounds

24A exposes structured descriptions and raw fields. References to rows, selected
details or player feedback below are requirements for their future 24B presentation.

Follow the English list-description rules in pinned
[item.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/item.cpp),
`WeaponItems/ArmorItems/AccessoryItems/MiscItems::getFullDescription`.
This is list identification by readable name, not the game's paid identification
operation or `getIdentifiedDetails/getAttributes` screen.

| Category | Supported naming domain and composition |
| --- | --- |
| Weapons | ID 1..40, material 0..130. Material prefix + base; low state counter 1..6 appends `BONUS_NAMES`, 0 appends nothing. Naming the inherited Elder entries does not enable their gameplay. |
| Armor | ID 1..13, material 0..130. Material prefix + base. Low counter is stored information, not a naming-table index. |
| Accessories | ID 1..10, material 0..130. Material prefix + base. No bonus-name suffix. |
| Miscellaneous | Material 1..11 selects container; ID 1..73 selects special name. Container + resource `ITEM_OF` + special. Material 12..21 are upstream `bogus` placeholders and are deliberately unsupported, not advertised as meaningful names. |

Status bits are counter=`state & 63`, cursed=`state & 64`, broken=`state & 128`.
Equipment curse/breakage suppresses material text; weapon bonus suffixes remain
as in the reference. Misc curse/breakage suppresses `of` and special text. Show
both broken and cursed when both bits are set, in that order. Retain a separate
explicit status field even when name decoration repeats it.
Strip only the known embedded color directives from the two decoration strings;
never execute resource text as font commands or printf formats. Trim/join token
whitespace and capitalize the first visible letter. This presentation adaptation
replaces the original backspace/color hacks.

For occupied weapons/armor/accessories show `Equipped` iff frame is nonzero,
with raw frame in details; this reports storage, not equipment legality. For misc
show charges 0..63 even if cursed/broken, and raw frame as stored metadata rather
than claiming usable equipment. For equipment show counter 0..63; weapon counters
7..63 are unknown suffix values, not charges or unchecked table indexes.

An unsupported field retains the known base/container where possible and gets
an explicit unknown-field annotation. Never invent a material, suffix or effect.
The detail area always exposes category, slot and decimal M/ID/S/F. Entirely
unknown records use `Unknown item` and those bytes. If suppression legitimately
hides a modifier, do not reveal its name through another player detail field;
raw numeric diagnostics remain available. Known base names do not imply effects,
prices, class legality, armor class, damage or spell statistics.

ID-zero slots display `Empty`; do not compose their names or call them equipped.
Selected empty details can show preserved raw metadata, but transfer is disabled.
Catalog lookup and rendering never normalize bytes or clear metadata.

Concrete bounds:

- Build generator: exactly six declared counts and three nonempty strings.
  Every array's reserved index zero is an empty NUL-terminated token; all other
  entries are required and nonempty. Each token is at most 63 bytes; aggregate
  emitted text is at most 16 KiB. Reject identity/hash mismatch, malformed tags,
  invalid entries, oversized tokens, truncation or an incorrect block end.
- Runtime material resource: reject length above 8,192 before copying into catalog
  storage (the bridge's absolute CC bound remains 65,535). Exactly 131 NUL-terminated
  entries, each at most 63 bytes, exact EOF after entry 130, entry zero structurally
  empty before sanitation, and entries 1..130 nonempty after sanitation/trim. No
  unchecked `readString` loop.
- Material bytes outside printable ASCII become `?` in this English interface.
  Generated decoration strings strip their two known color directives. Text
  remains literal, including percent signs.
- Composed descriptions are capped at 192 bytes with deterministic `...`
  truncation. Measured-width row elision, wrapped details and rectangle clipping
  belong to the still-unauthorized 24B renderer rather than the 24A catalog.

The material stream has no version header. Its version contract is the declared
131-entry schema in a fingerprinted original installation; do not invent a
version byte or accept arbitrary `mae.xen` from a search path. Counts/truncation
mismatches fail the provider. The measured hash identifies certified original
acceptance; structurally valid different English material text can be displayed
without claiming that installation certified. Immutable build tables carry the
schema/source marker; unknown markers are an error, not a migration path.

### Failure policy

A missing or changed pinned source dependency or generator failure fails the build;
no incomplete generated catalog is published. Compile-time assertions enforce the
generated schema, language, revision and counts. Tests exercise an explicitly
unavailable catalog and its bounded fallback without replacing the built-in tables.

Missing, malformed or unreadable optional material data returns typed availability
and a load diagnostic. Descriptions retain known base names and numeric material
fallbacks; an explicitly unavailable catalog returns `Item catalog unavailable`
with raw fields. Availability is distinct from an unknown record and is not a
save/gameplay compatibility field. No material table is partially published.
Future 24B must present diagnostics without repeated loads on redraw, allow
byte-based transfer independently of naming availability, and preserve these
fallbacks through opening/reconstruction. Complete supported names remain required
for original acceptance; fallback cannot pass Myra or material-equipped anchors.
For a valid CC index, the existing Dark archive owner exposes the `mae.xen` entry
metadata so its extent and read are checked before the upstream fatal short-read
member path. Truncated indexed payloads become the typed read-error fallback;
readable invalid material bytes become malformed. This does not suppress global
ScummVM fatal errors or alter required `clouds.dat`/archive-index failure behavior.

## Transfer rules and publication

The following transfer contract is future 24B scope, pending separate authorization.

Pinned [dialogs_items.cpp](https://github.com/scummvm/scummvm/blob/6814ee9ba54582f5b5adcffab49efbbd8f589edd/engines/mm/xeen/dialogs/dialogs_items.cpp)
`ItemsDialog::execute`, F1-F6 branch around lines 398..447, is the ordinary
non-shop/non-combat oracle. With an item selected it checks cursed state, then
`InventoryItems::isFull`, writes the last destination slot, clears source,
resets destination frame, and sorts both arrays. `item.cpp::InventoryItems::isFull`
checks the last-slot ID; `InventoryItems::sort` preserves occupied order.
That branch has no source/destination `canAct`, HP, SP or equipment-class test.
Its combat/shop mode gates are not rules to import into an idle MMModern move.
The ordinary entry in `engines/mm/xeen/dialogs/dialogs_char_info.cpp` restores
the prior mode and calls `ItemsDialog::show(..., ITEMMODE_CHAR_INFO)` for its item
key, without a character-condition check. M24 adapts that entry to idle I and
explicit confirmation, not the original immediate F-key move.

Validate against live owners in this order. Refusal produces a typed reason,
not a partial mutation or an exception used as ordinary control flow.

| Order / situation | Contract |
| --- | --- |
| 1. Boundary | Require active session, current inventory confirmation generation, no event pending/dispatch or reentrant action. Cancellation stops before the move. |
| 2. Participants | Require nonempty active membership and in-range source/destination active indexes. Resolve through current `activeRosterIds` and checked roster access; identities must match the displayed confirmation. Inactive owners cannot be directly addressed. |
| 3. Same owner | Refuse `Same owner`, including distinct active aliases. No compaction or frame reset. This deliberate safe adaptation prevents using self-transfer as unequip; the original active-party copies cannot represent MMModern aliases. |
| 4. Source | Validate four-category discriminator and slot 0..8; reject empty ID. Validate transient generation and selected location still current. |
| 5. Curse | Any occupied item with bit 6 set refuses, equipped or not, including cursed+broken. Curse wins over destination capacity. |
| 6. Capacity | Destination category slot 8 must have ID zero. Earlier holes do not override an occupied tail. No pre-validation compaction. |
| 7. Rule safety | Preflight resulting source/destination with existing checked derived rules before publication; an unrepresentable result refuses without mutation. This protects extreme saved values, not class/equip legality. |
| 8. Move | Publish the bounded operation exactly once, then establish a fixed typed result before formatting or redraw. |

Conditions of either owner, including asleep/dead/stoned/eradicated, do not bar
inspection or manual transfer. Broken alone is allowed. Unknown occupied item
bytes are transferable under the same rules. No identification, class, spell,
reward-recipient eligibility or equipment-effect validation is added.

Successful mutation, independently defined:

1. Copy the selected four-byte value locally; put it in destination slot 8,
   completely clear the selected source slot, then reset the destination frame
   to zero. No callback/allocation/fallible lookup occurs between these writes.
2. Explicitly stable-compact source category, then destination category with
   `xeenCompactItems`. The destination item follows all previously occupied
   destination records. Its final slot equals prior occupied destination count.
3. Preserve material/ID/state of the moved record and every occupied bystander
   byte/order. Empty metadata in these two arrays is intentionally zeroed. All
   other categories, inactive owners and unrelated state are unchanged.
4. Recompute derived displays and party HUD from current owners. Transfer may
   remove a source equipment bonus; the recipient receives an unequipped item.
   Clearing ID-zero modifier metadata during explicit compaction can change
   either owner's derived values too. Do not add an ID gate to hide this case.
5. Preserve both characters' current HP/SP **exactly**, including negative or
   above-new-maximum saved values. No healing, clamping, condition change or
   implicit equip occurs. Save existing bytes and recompute derived values on load.

Rule preflight may use short-lived candidate character/category values to call
`validateForUse` with year 610. Allocation/copy failure occurs before mutation.
They are not live owners or a UI inventory copy. Publish only the two resulting
arrays with nonthrowing fixed-size assignment; do not assign copied character
names/conditions back. This implementation form must be equivalent to the exact
ordered move/compaction above. No general rollback framework is warranted.

Each move consumes its confirmation token before invoking the operation. After
success clear source-item/destination selections and advance the transient
inventory generation **before** rendering/report callbacks. A delayed or repeated
confirmation cannot select the next identical potion automatically. Identical
records are separate quantities; byte/name equality never identifies an item.
A changed membership, category contents or owner invalidates an armed selection.
Flow owns all authorized live mutation routing; any explicit reconstruction or
test-side owner replacement cancels an armed transfer before accepting more input.
Revalidate the selected live record as an extra stale guard, not a unique ID.

Cancellation/refusal/open/inspection never compact. Refusal consumes the armed
confirmation, retains a still-valid source slot for browsing, clears destination,
and requires fresh transfer initiation. Success keeps source owner/category
but clears item selection; a new number/up/down action must select another item.

A post-publication catalog, feedback or redraw failure cannot undo, replay or
requeue the move. Clear armed transient state first, then attempt clean scene
reconstruction and bounded error reporting. Recoverable inventory rendering
failure closes the panel; a later new open reads actual owners. If base scene
reconstruction also fails, use the existing fatal rendering path, with no implicit
save and no further mutation. Never report an item as still at the source merely
because the last framebuffer predates publication.

## Concrete player interface and modal integration

This interface is specified for 24B and is not implemented by 24A.

This is a small keyboard interface, not the original character sheet. Use one
opaque parchment-style panel inside the native 320x200 frame, approximately
`(4,4)..(315,145)`, leaving portraits/HP indicators at y=150/182 visible. Use the
existing reduced 8-pixel glyphs and drawWindow facility, with an 8-pixel row pitch.
Final pixel offsets may fit measured glyph widths but must satisfy the fixed
content and no-overlap tests below.

```text
F1 Arturius [owner 0]     HP 12/12  SP 2/2
Condition: Good          Source: Arturius
< Weapons | Armor | Accessories | Misc >
 1 [E] Sabre             Selected item details
 2 Empty                Bounded name (wrapped)
 ...                    Status / counter or charges
 9 Empty                M / ID / S / F
                        To: F2 Tyro [owner 18]
Feedback: ...
F1-F6 owner; arrows category/slot; 1-9 slot; T transfer
Enter confirm; Esc back/close; close panel before F9
```

Use two columns: nine slots at x=10..149, details/destination at x=154..309.
Rows at y=37,45,...,101; header/category occupy y=10..34; feedback/help y=111..139.
Show active F-key, resource-loaded name and roster identity; aliases must be
visible as the same owner, not distinct backpacks. Row marks are `E` equipped,
`C` cursed, `B` broken, plus selection highlight; detail labels disambiguate them.
Elide long row names; detail shows wrapped name, statuses and charges/counter,
with M/ID and S/F on separate lines if needed. Use state-specific short help.
Header displays the selected owner's worst modeled condition using the existing
predicate. Do not imply it is the only nonzero condition or a transfer gate.
Keep signed current HP/SP and complete derived maxima readable; prioritize numeric
fields over elidable names, using the second header line for overflow. A full
condition matrix and full character statistics are not required.

| State | Input | Transition and visible result |
| --- | --- | --- |
| Closed, event-idle | I | Open Browse; source active index 0, Weapons, no selected item. Also print existing full live diagnostics. No world action. |
| Closed, event pending/dispatch | I or inventory-only key | Ignore; preserve event page/generation/timing and owners. No queued open. |
| Browse | F1-F6 | Select current active source; retain category, clear item/destination; unused F-key gives bounded feedback. |
| Browse | Left/Right or A/D | Cycle categories in fixed order; clear item/destination. |
| Browse | 1-9 | Select physical slot, including Empty; display details. |
| Browse | Up/Down or W/S | Previous/next physical slot, wrapping 0..8; from none select last/first respectively. |
| Browse, occupied selection | T | Enter ChooseDestination, freeze displayed source/category/slot, no mutation. |
| Browse, no item/Empty/empty party | T | Stay Browse, `Select an occupied item` or `No active characters`. |
| ChooseDestination | F1-F6 | Resolve/display destination, enter Confirm. Invalid F-key stays with feedback. |
| ChooseDestination | Escape | Cancel to Browse retaining valid source slot. |
| Confirm | F1-F6 | Replace destination; remain Confirm with explicit Enter prompt. |
| Confirm | Enter | Consume token, validate/move once, show success/refusal and return Browse as specified above. |
| Confirm | Escape or N | Cancel to Browse, no mutation, retain valid source slot. |
| Browse | Escape or I | Close, clear inventory selections, restore current scene. |
| Any open state | Space, Y, unrelated actions | Consume without world dispatch or transfer; only Enter in Confirm commits. |
| ChooseDestination/Confirm | Category/slot/movement/T/I | Consume; instructions say Escape to cancel first. |
| Any open state | F9 | Refuse visibly and in status/console; retain selection. No deferred save. |
| Any state | Window close | Clear transient state and quit; no save. |

Empty categories show nine Empty slots. Empty party opens an empty panel with
close controls and no rule/portrait/member indexing. Invalid selections are
cleared before read/draw. Duplicate names do not affect owner identity.

I changes from diagnostic-only to open/close UI while retaining its idle diagnostic
print and `InspectInventoryAction` observability. Setup/resume retain all-owner
raw diagnostics. Do not log all 1,080 slots on each redraw/navigation. Add semantic
slot and transfer-initiation actions to `PlayerAction`; map T and 1-9 in SDL.
Reuse `SelectMemberAction`, navigation and Enter/Escape by modal context. New
inventory-only actions outside inventory return without clearing retained event
labels or stepping ordinary animation.

Routing priority: Application active/reentrant guard; F9 handling; Flow event
pending/dispatch guard; open inventory input; closed inventory-open action;
ordinary event/navigation routing. The event branch returns after one action,
even if it finishes an event, and cannot fall through into inventory. Flow's
gameplay/save blocking query includes every inventory state and synchronous move;
event-pending-specific checks remain distinguishable internally. Direct
`initial/acceptManual/acceptAutomatic/respond` seams cannot start/replace events
while inventory is active. No nested SDL loop.

`handlesEscape()` is true for every open inventory state, in addition to existing
event cases, so SDL forwards Escape rather than quits. Keep WhoWill F1-F6/cancel,
NPC/reward Space/Enter/Escape and ordinary idle quit behavior. Application passes
the predicate to `showInteractive`. Teardown/error paths clear inventory transient
state even if its first draw failed.

### Scene reconstruction and timing

Flow retains the current composed underlay; opening clears passive labels via
presenter cleanup and forces one neutral reconstruction. Inventory is a replaceable
overlay over a clean base, never another event layer pushed per input.
`refresh(true)` while open rebuilds world/HUD then inventory from live owners.
Explicit cache reconstruction preserves browsing owner/category/valid slot but
cancels an armed destination/confirmation. This safe adaptation avoids confirming
a stale view after reconstruction; ordinary idle underlay rebasing does not cancel.

Keep M22's 100 ms outdoor deadline and phase while open. SDL idle calls the same
Flow update; recompose/rebase underlay and redraw inventory without changing
selection. Inventory actions/open/close/move/refused save are neutral redraw causes:
no action-driven phase step or deadline rearm. An idle callback due afterward may
advance once normally. No gameplay time, per-item clocks or indoor animation.
NPC cannot be pending simultaneously; its 150 ms timing remains unchanged when
an inventory request is blocked by an event. Closing reveals current phase and
updated HUD without stale labels, duplicate panels, portrait corruption or world
cache reset. Cache loss never resets ownership or reconstructs arrays from labels.

## Persistence and compatibility

24A adds no persistence state or save-format change. The inventory save-boundary
and transfer behavior below specify future 24B integration.

All durable mutation fits existing arrays. Keep writer v2 and reader v1/v2,
144-byte character item block, 4 MiB bound, CRC, archive fingerprints and v1
absent-field resolution unchanged. Preserve installation protections and existing
encode/preflight/temporary-file/flush/close/replace publication.

**F9 is refused in every open inventory state**, including Browse and post-success
feedback. This chooses one visible idle save boundary without serializing or
implicitly cancelling confirmation. Refusal performs no capture, preflight, I/O,
deferred save, transfer, event response or animation update at the handler boundary.
Only designated status/feedback may change. Close inventory and press a new F9.
Application active/dispatching/event guards remain; inventory cannot create an
idle gap in a pending reward lifecycle.

Eligible saving captures actual owners after transfer. Preflight stays independent
and phase zero; catalog availability is not a restore precondition or compatibility
field. Existing archive fingerprints cover material bytes in `DARK.CC`. Loading
does not evaluate names or enforce naming IDs. A fresh process builds fresh owners
and catalogs, starts inventory closed and ordinary phase zero, and does not replay
initial automatic events on resume. Reopening defaults to first active owner and
Weapons and reads restored arrays. Never serialize catalog strings, derived stats,
selection, token/generation, feedback, resource bytes, caches or pages. In-session
loading remains out of scope.

## Original acceptance anchors and exact outcomes

Evidence uses an unmodified World of Xeen installation.
Slots below are zero-based; UI labels add one. Initial active references are
`[0,18,14,11,1,6]`, with 35 occupied records, no miscellaneous records and empty
category tails. An existing inspector and read-only API/resource probes confirmed
these facts. All initial active characters have Good condition.

### Primary: genuine Myra exchange, then transfer

Reuse `tests/SaveResumeIntegrationTest.cpp` and `tests/XeenCheckpointTestSupport.h`.
Start Root index 17=0/Q2=false. Position camera only at Myra map 23 `(9,11)` West:
Space, two original request acknowledgments; five instructions set Q2. Position
Phirna map 23 `(8,2)` North: Space, Y, Enter; original 18-instruction collection
grants one Root and removes object `{Clouds,23,13}` and event records 125..135.
Position back at Myra. Space opens return text at line 7/file offset 244; final
acknowledgment reaches consume-Root line 8, clear-Q2 line 9 and GiveEnchanted
lines 10..14 (`46 25 00 01`), natural end at absent line 15. Nine instructions;
five `{10,37,1,0}` misc records go to owner 0 slots 0..4. Receipt acknowledgment
finishes. Original records 21..35, offsets 182..315, contain this Myra path.
The genuine exchange is accepted M21 behavior. The 24A catalog assertion describes
the delivered reward without changing that exchange. Transfer remains 24B scope.

Future 24B delta: I, Misc (three category-right inputs), slot 1, T, F2, Enter. Show
**Potion of antidotes**, charges 1, unbroken/uncursed, source Arturius owner 0,
destination Tyro owner 18. Owner 0 misc slots 0..3 retain four identical records,
4..8 are zero; owner 18 misc slot 0 contains one exact record, 1..8 are zero.
All other arrays/fields are unchanged. Root=0/Q2=false and only established
Phirna removals remain. Close, new F9, fully terminate, then start a separate
production load process and reopen both owners to see four/one.

Negative controls: cancel before moving and compare full state; target F1 refuses
self-owner; I/T during actual return/receipt cannot mutate; F9 while either
interaction or inventory is open refuses. Repeated Enter after success leaves
four/one. Only after resumed inspection may Myra be revisited; its new request sets
Q2 without another potion. Do not inject a Root, claim a route or test antidote use.

### Original equipment controls

Run each from fresh original owners, or explicitly independent expected states.
Catalog expectations derive from `EN` plus the checked material resource.

| Anchor | Original bytes and label | Transfer to owner 18 and exact category post-state | Negative / visible acceptance |
| --- | --- | --- | --- |
| Equipped weapon | Owner 11 (F4), weapon slot 0 `{0,12,0,1}`, Dagger; slot 1 `{0,12,0,0}` is a second Dagger | Source slot 0 becomes old unequipped dagger; rest zero. Destination retains `{0,2,0,1}` at slot 0, gets `{0,12,0,0}` at slot 1; rest zero | Equipped source mark before; destination mark absent. Same-owner/cancel preserve both distinct daggers. No equip command. |
| Material armor | Owner 0 (F1), armor slot 3 `{38,10,0,9}`, Leather boots | Source retains `{0,3,0,3}`, `{0,8,0,2}`, `{0,13,0,6}` at slots 0..2 then zero. Destination retains its four original armor records, appends `{38,10,0,0}` at slot 4; 5..8 zero | Existing destination equipped boots remain a distinct quantity; matching type does not prevent transfer. Cancel preserves frame 9. |
| Material accessory | Owner 11 (F4), accessory slot 1 `{42,1,0,8}`, Silver ring | Source retains `{38,2,0,12}` at slot 0, rest zero. Destination retains leather belt `{38,2,0,12}` at slot 0, receives `{42,1,0,0}` at slot 1, rest zero | Visible silver prefix, equipped source/unequipped recipient; no class/ring-limit gate. Cancel/self-owner preserve arrays. |

All three require exact disk/restart comparisons and restored inspection through
the production interface. Automated connected coverage runs independently;
maintainer acceptance covers all categories, with Myra separate restart as the
primary continuous sequence. These original items do not change currently modeled
HP/SP maxima; frame reset is not modifier-effect evidence.

### Explicit synthetic contrasts

- Nonzero-frame equipment material 69/77 proves intellect/personality removal;
  105/110 proves direct HP/SP removal. Test current values above new maximum and
  negative saved values without clamping. Counter 63 and unknown ID do not disable
  existing rules. No synthetic value is called original loot.
  Those materials contribute +2 to their respective attributes. For concrete
  direct-stat controls, change original Arturius weapon slot 0's material to 105:
  max HP is 16 before transfer and 12 after. Set synthetic current HP=16 and
  require 16/12 afterward. In an independent material-110 case max SP is 6 before
  and 2 after; synthetic current SP=6 remains 6/2. Destination Tyro gains no
  modifier from either unequipped record. These literal expectations do not come
  from the transfer helper.
- Source/destination empty metadata `{69,0,0,1}` in touched equipment contributes
  under accepted rules until explicit compaction; test its removal and both owners'
  refresh. Misc equivalents contribute nothing.
- Curse `state|64`, broken `state|128`, both, occupied destination tail with holes,
  alias membership, inactive indexes, unknown bytes, malformed providers and
  integer-boundary preflight are synthetic controls.
- Cursed+full destination reports curse first; broken transfers preserving
  broken/counter bits. Tail ID zero with nonzero metadata permits insertion.

## Stage structure and acceptance gates

M24 has **two stages requiring separate authorization**. The catalog's generation, commercial
resource and failure contracts are accepted independently; transfer must be
implemented and accepted together with its production lifecycle.

| Stage | Objective, owners and dependencies | Tests / definition of done | Exclusions and successor |
| --- | --- | --- | --- |
| 24A - Bounded catalog foundation | Accepted: pinned-blob generation, immutable structured lookup, material parser and explicit asset read through existing owners, with test/smoke consumers. | Build, automated and original/reference validation passed; independent technical approval and maintainer acceptance recorded below. | No transfer/player inventory UI, new application CLI, commercial bundles or save changes. |
| 24B - Usable inspection and transfer | Depends on accepted 24A. Validated operation, transient Flow state, renderer, SDL actions, Application guards and connected harness extensions. | Transfer invariants, combined UI/cache/save/fresh-process tests, original anchors, recovery, full build/CTest, original-data regressions, independent review and maintainer physical SDL acceptance all pass. | No equip/unequip, effects or M25 implementation. Production wiring, save policy and recovery are not deferred beyond this stage. |

**24A accepted; 24B remains pending separate authorization and implementation.**
M24 as a whole remains incomplete. Acceptance of one stage does not authorize its
successor.

### 24A final acceptance

The bounded item catalog foundation received independent technical approval
(`APPROVE 24A FOR MAINTAINER ACCEPTANCE`) and maintainer acceptance.

- Normal configure/build and `BUILD_TESTING=OFF` configure/build passed; the full
  CTest suite passed 64/64, including catalog lookup, optional archive reads and
  generation/publication regressions.
- Independent reference validation matched all 169 embedded array entries and
  three scalars against pinned `CONSTANTS_7`. Runtime commercial `mae.xen` matched
  the certified material reference; the catalog smoke also passed outside the
  source checkout with only the executable and normal runtime libraries deployed.
- Original Dagger, Leather boots and Silver ring descriptions retained exact item
  bytes and equipped status. The genuine Myra exchange's delivered `{10,37,1,0}`
  reward described as `Potion of antidotes`, charges 1, through the existing
  save/resume integration harness.
- Validation established immutable descriptions, bounded unknown/material
  fallback, recoverable optional payload reads and atomic generated publication.
  It introduced no inventory UI, transfer, item mutation or save-format change.

These results accept the foundation only. Player-facing inspection, transfer,
ownership after restart and maintainer physical UI acceptance remain 24B work.

## Discriminating automated verification

Normal CTest uses test-owned synthetic bytes and generated source-derived tables,
never commercial files or a developer's installed ScummVM data directory. The
documented pinned build dependency is sufficient. External-data smokes separately
require the installation. Do not copy extracted resources into fixtures.

### Catalog tests

- Independent literal expectations: Dagger, Leather boots, Silver ring, Potion of
  antidotes; synthetic Burning dagger Dragon Slayer, Broken cursed dagger with
  retained valid weapon suffix, Broken potion with hidden special. Category
  minimum/maximum IDs, reserved zero, weapon 41, armor 14, accessory 11, misc
  material 0/11/12/21/22, special 0/73/74, equipment material 0/130/131/255 and
  counter 0/6/7/63. Do not assemble expected strings with the production formatter.
- Exact suppression, frame status, charges/counters; unknown fields retain bytes.
  All 256 state/frame values read without mutation; unknown IDs/materials cannot
  index tables. Compare embedded output with the independently
  parsed pinned constants block as an external provenance check, not the only oracle.
- Material fixtures: missing, empty, 130/132 entries, absent final NUL, trailing
  bytes, 63/64-byte entry, 8,192/8,193-byte envelope, nonempty entry zero, empty
  required entry, controls/high bytes/percent signs; bounded fallback with no
  partial publication. Generated schema/revision/count assertions and bounded
  token validation enforce the build-time catalog contract.
- Build-generation fixtures use test-owned Git repositories and binary blobs.
  They cover exact revision and blob identity, deterministic output and unchanged
  output timestamps, exact schema/count/scalar and representative octal literals,
  malformed counts, oversized tokens and truncation. Publication cases cover
  initial output, unchanged no-op, replacement through paths with spaces and
  failed replacement preserving old output with temporary-file cleanup.
  Mutable worktree/compiler artifacts cannot influence generation. The production
  generated include has an independently pinned full-file SHA-256, while the
  catalog reference smoke separately parses the
  pinned `CONSTANTS_7` block and compares it with compiled tables.
- Production-path synthetic archives cover ready, readable-malformed, missing and
  indexed-but-truncated `mae.xen`; the latter returns a typed read error instead of
  terminating. They also prove no partial material publication, retained built-in
  names and unchanged successful `clouds.dat` access.
- Description cap 192, sanitized long material names and typed distinction among
  unavailable catalog, unavailable material table and unknown individual fields.
  Renderer width, nine-row layout, condition/HP and clipped feedback tests belong
  to 24B.
- The delivery smoke launches from a fresh directory containing only the smoke
  executable and its normal runtime DLLs; generated/source/reference inputs are
  not deployed. Runtime material comes only from the selected installation,
  without `mm.dat`, current-directory or environment lookup. A separate explicit
  reference mode compares the pinned constants block and `mae.cld`.

### 24A connected regression boundary

The existing save/resume smoke retains its established producers, serializers,
oracles and process topology. Stage 24A adds only a narrow post-delivery assertion:
after the genuine five-item Myra reward is already delivered, the actual
`{10,37,1,0}` record must describe as `Potion of antidotes`, charges 1. It adds no
transfer, UI action, persistence field or alternate save route. All transfer and
fresh-process ownership work below remains 24B.

### Transfer tests

Use test-authored pre/post arrays, never expected values obtained from the new
helper or production compactor. Cover all categories, first/middle/tail source,
full source, ordered destination holes, occupied tail with holes, empty-tail
metadata, same reference, distinct aliases, shared destination aliases, invalid
indexes/category/slot, empty party/source, inactive owner and all condition
contrasts. Preserve inactive owners, unrelated categories and quest/world state.

Verify occupied-record multiset conservation, accounting for exactly one frame
reset and empty-metadata clearing. Identical potions remain distinct quantities.
Preserve occupied bystander bytes/order. Refusal/cancel compare full durable
snapshots. Repeated confirmation/SDL keydown, stale membership/location/generation
and reconstruction cancellation cannot move a second record. Test curse-first
precedence, broken success, unknown nonzero frame reset and no class/condition gate.

Derived tests use independent numeric expectations: direct HP/SP and mental
attribute changes, ID-zero metadata compaction, no misc effect and exact current
HP/SP. Test checked-result refusal on extreme saved inputs without partial writes
or a new equipment-legality rule.

### Connected production lifecycle

Extend existing `XeenGameplayServices`/Application seams and
`tests/SaveResumeIntegrationTest.cpp`. Observations borrow state; they do not
supply another serializer or transfer route. At least one normal synthetic
integration and the original exchange extension execute:

```text
Application startup -> I -> owner/category/physical slot -> T -> destination
-> Enter -> refresh/reconstruction -> close -> production F9 -> real disk file
-> producer exit -> separate process --load-game/fresh Application owners
-> I -> source and destination inspection
```

Compare test-owned full expected snapshots, actual disk bytes/decoded values and
fresh owners. Never create expectations by capturing mutated producers. Original
acceptance first runs genuine request/collection/return. Disconnected helper tests
are insufficient. Preserve producer file bytes through consumers/revisits.

Include actual pending NPC return, WhoWill, message and reward warning/receipt;
I/slot/T/F9 cannot bypass them and final event input cannot transfer. Exercise
Escape in every state through SDL, window close, unused F-keys, empty party/slot,
aliases and repeated confirmation. Fail formatting/report/draw after success,
then reopen/rebuild and prove exactly one moved quantity; failures before
publication prove none moved. Block reentrant save/open/event callbacks during
mutation/refresh. Recovery must not rely on the failed feedback callback.

Inject the existing clock for stationary animation while open, input-neutral
timing, rebuilt underlay and correct close frame. Cover retained labels, Myra's
animated flag, pending portrait blocking, indoor static objects and changed HUD.
Direct synthetic and SDL dummy/software tests are automated evidence, not physical
acceptance.

Current regression seams: `XeenCharacterFormatTests`, `XeenCharacterRulesTests`,
`XeenPartyVisualStateTests`, `XeenItemRewardTests`, `XeenRewardExecutionTests`,
`XeenRewardFlowTests`, `XeenRewardGameplayTests`, `XeenWhoWillTests`, `XeenNpcTests`,
`XeenEventUiTests`, `XeenNavigationFlowTests`, `SdlInputTests`, save format/state/
file/Flow/SDL/CLI tests, `XeenOutdoorAnimationTests`, `XeenIndoorComposerTests` and
`XeenVisualRemoveTests`. Use actual CMake registrations; retain M22 rebasing/timing
and M23 indoor composition/occlusion regressions.

## Future commands and maintainer physical acceptance

The physical inspection/transfer gate below belongs to 24B. The accepted 24A
catalog validation commands are maintained in
[dependencies](dependencies.md#build-generated-english-item-catalog).

Existing commands, with build paths discovered from configuration:

```text
cmake --build <build> --parallel 4
ctest --test-dir <build> --output-on-failure
cmake --build <build> --target mmodern_save_resume_smoke mmodern_myra_smoke mmodern_phirna_smoke
mmodern_save_resume_smoke <game-directory> <new-output-directory> [sdl]
mmodern_save_resume_smoke --manual-myra-exchange <game-directory> <new-save-path.mmsave>
mmodern --render-map <game-directory> 23 9 11 west --save-file <save-path.mmsave>
mmodern --load-game <game-directory> <save-path.mmsave>
```

Current manual exchange mode ends at five owner-0 rewards. **Proposed 24B addition:**
extend that executable with a distinct `myra-transfer` checkpoint and
`--manual-myra-transfer <game-directory> <new-save-path.mmsave>` mode, retaining
M21's oracle unchanged. Its new independent oracle expects four/one; its physical
loop waits for player inspection/transfer before F9. These additions do not exist
at this baseline. Add equipment cases to the same harness where practical; no
new acceptance service framework. Use an existing output parent outside the game
installation and an absent save target; never delete/reuse unknown files.

Physical gate, performed by the maintainer in one continuous real SDL producer
loop without injected keys, automatic acknowledgment/F9 or automatic closer:

1. Complete genuine camera-positioned Myra request/Phirna collection/return.
   Observe inventory-open and F9 refusal during return/receipt. Finish receipt,
   open I. Camera positioning is a disclosed checkpoint, not normal navigation.
2. Observe Arturius, condition/HP/SP, all categories/nine slots. Select first
   genuine Potion of antidotes; see charges 1.
3. Arm owner-18 transfer, cancel, verify five remain. Retry/Enter; see success,
   four source/one destination and correct names. Another Enter cannot transfer.
   Observe inventory F9 refusal, then close.
4. New F9 must save. Exit producer completely. Start separate production load,
   reopen both owners and see four/one before Myra interaction. No implicit resave.
5. Independent equipment cases show equipped Dagger, Leather boots and Silver
   ring moving via the same UI, with destination unequipped. Check cancel/self
   feedback and unrelated equipment. Observe indoor dismissal/outdoor animation.

Automated assertions verify full disk/state equality and secondary restarts.
The maintainer supplies physical observations; images, automated Windows SDL
and reviewer inspection do not substitute. Skipping a phase or early exit cannot
pass. No normal-route/playable-region certification follows.

## Risks, exclusions and definition of done

The accepted 24A contracts cover the fixed `CONSTANTS_7` block and object identity,
bounded descriptions, suppression and raw-byte preservation. Remaining 24B review
risks include self-owner adaptation, mutation before fallible presentation, stale
selection after compaction and derived effects of empty metadata. The specification
above bounds those risks; their implementation and acceptance remain future work.

Replan before expanding scope if the pinned constants object or bounded catalog
schema changes, required material text is absent/differently structured in a
claimed supported installation, UI requires a new durable owner, save compatibility
must change or all nine slots/details cannot be made usable. Cross-platform host
generation and wider languages/games are separate decisions. Pixel-fit adjustments
within the declared interface are not scope expansion.

Exclude player equip/unequip, item use/consumption/spell effects, discard, paid
identification, repair, shops/NPC trade, recruitment/reordering, quest journal,
combat/full statistics, new equipment effects, indoor animation/wall items,
normal-route certification, Darkside gameplay, localization and detailed M25
planning. Resetting the moved frame is the only equipment-state action specified
for future 24B transfer; 24A performs no mutation.

M24 completes only after both stages pass, production interaction works in all
categories, genuine Myra four/one ownership survives disk save/separate restart,
required regressions/original-data checks pass, independent review has no unresolved
blocking findings and the maintainer performs physical SDL acceptance. Then update
durable closure documents under AGENTS rules. Stage 24A acceptance alone does not
authorize 24B implementation or any Git publication operation.
