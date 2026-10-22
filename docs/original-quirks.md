# Original quirks

MMModern reproduces the original DOS game, including its oddities. This page
records behavior that looks like a bug but is kept on purpose, so that nobody
"fixes" it by accident, and lists the cases where the pinned ScummVM reference
itself differs from the original and is not followed. Entries that could later
become opt-in quality-of-life fixes (always off by default) are marked
**QoL candidate**. Add an entry when a milestone deliberately keeps or rejects
such a behavior, with its evidence. Entries tagged **ScummVM:** are summarized
for players in [ScummVM differences](scummvm-differences.md).

## Original behavior reproduced on purpose

| Behavior | Evidence | Where | Notes |
| --- | --- | --- | --- |
| <a id="sv-confirmword-text0"></a> Missing ConfirmWord primary, alternate or nonzero title indices alias text[0]. | Calibrated static `WORLD/XEEN.DAT` analysis, 2026-10-16: `0x21d26..0x21d34` clears all 280 text-offset slots, then `0x21d40..0x21d81` fills actual strings. Dispatch (`0x26c26..0x26c48`) and ConfirmWord (`0x27f15..0x27fa6`) add those offsets to the resource start. | `XeenEventInterpreter` ConfirmWord; [travel and typed input](rules/world-events.md#travel-and-typed-input) | Clouds map 69 `(9,15)` West line 0 names missing alternate 21, selecting text[0]. Pinned ScummVM instead leaves a missing alternate empty. **ScummVM:** differs. |
| <a id="sv-dragon-tower-mirror"></a> Clouds mirrors above map 32 start at record index 0x26; travel reads the selected record directly, including the incorrect Dragon Tower destination. | Static `WORLD/XEEN.DAT`, 2026-10-16: scan start `0x2406c..0x2407c`, record use `0x2702b..0x270a1`. The original Dragon Tower record names map 5 `(10,12)`, direction -1. | `XeenEventInterpreter` ConfirmWord and teleport | The reference's correction to map 13 `(10,5)` South is not imported. Unadmitted maps still refuse through the shared transition result. **ScummVM:** differs. |
| <a id="sv-vowelless-knight"></a> Password strings are compared directly, without the Vowelless Knight answer rewrite. | Static `WORLD/XEEN.DAT`, 2026-10-16: primary then alternate comparison `0x24023..0x24067`, ASCII case-insensitive comparator `0xa39f..0xa3df`. | `XeenEventInterpreter` ConfirmWord | Follows the M70 plan's DOS decision; original resource mistakes remain. **ScummVM:** differs. |
| TeleportAndContinue discards the active CallEvent stack and restarts at line 0 with member 1 and physical damage selected. Same-map travel does not reload the map. | Static `WORLD/XEEN.DAT`, 2026-10-16: Continue `0x27117..0x27127` jumps to initializer `0x26a8e..0x26ab8`; Call/Return storage `0x28375..0x283f6`; same-map load skip `0x270be..0x270f9`. | `XeenEventInterpreter`, `XeenWorld::loadEventDestination` | Destination objects are reselected by redraw. Same-map wounds and temporary outdoor-neighbour changes remain; actual map changes apply normal exit lifetimes. |
| AlterMap still writes a wall after neighbour lookup reports a missing tile. | DOS lookup retains the last DAT index on a missing neighbour (`0x11db7..0x11def`, `0x11e1e..0x11e43`); setWall ignores that return and masks x/y for the write (`0x234f5..0x23549`). Original Clouds map 54 has no neighbours, yet EVT offsets 701–731 and 772–802 name x=23–25. Static check, 2026-10-15. | `XeenWorld::applyAlterMap`; [world Event rules](rules/world-events.md) | Those operands alias x=7–9 in map 54 itself. A valid neighbour is still resolved normally. |
| <a id="sv-spawn-beyond-count"></a> A Spawn into a slot at or beyond the map's MOB monster count writes the slot but nothing ever uses it: it is never drawn, moved, attacked, fought, counted or saved, and the next map load clears it. | Static `WORLD/XEEN.DAT` analysis, 2026-10-15. Map load sets the count byte `0x9c59` to the number of MOB monster records (`0x0fb79..0x0fb84`) after zeroing the 107-slot array (`0x0f5be..0x0f5c7`). Spawn (`0x27a59..0x27b1c`) writes only the slot's fields and never the count. Monster loops stop at the count: `0x1099c`, `0x155a6`, `0x1fe98`, `0x20ce7`, `0x2ea7d`, the Event scan at `0x287be` and the limit loaded at `0x1696b`. The `.mob` writer stores x/y only below it (`0x266e3..0x2671c`). The only other count writer is the Warzone arena's monster-count prompt (`0x2b9d5..0x2ba0b`); a Warzone load also forces the count to 0 (`0x0fb90..0x0fba9`). Pinned `scripts.cpp::cmdSpawn` instead resizes the monster list and makes the slot live. | `XeenWorld::applySpawn`; `XeenSaveState` restore; `XeenMapEventTests.cpp`, `XeenVertigoProcessTests.cpp` | Applies to Vertigo's exit-script slots 50 and 51 and to Clouds Spawns past their map's count. Saves from older builds that hold such slots load with those slots dropped, the state DOS itself would reload. Warzone's count-0 load and arena prompt are not implemented yet (map 85 is not admitted). Not checked in DOSBox play. **ScummVM:** differs. |
| A cell may hold four or more monsters when a script spawns them there; only a moving monster respects the three-per-cell limit. The first three on a watched cell, in monster slot order, take its display and contact slots; the others are hidden, still woken, and step in when a slot frees. | Static `WORLD/XEEN.DAT` analysis, 2026-10-15. Spawn (`0x27a59..0x27b1c`) has no occupancy test. Movement builds a per-cell count from the MOB-count monsters with y below 32 (`0x1fe64..0x1fe9f`) and refuses only a destination already holding three (`0x1fc24`, `jge`); a monster may leave a stacked cell. Every 3D redraw (`0xd0b1..0xd0d3`) rebuilds the attacker slots (`0x98b4..`), indoors through `0x14406` and outdoors through `0x1692c`, walking monsters in slot order: each monster on a watched cell gets its +4 byte set (the reference's `_isAttacking`), then the first free slot of that cell (`0x169c0..0x169f4` for the three front slots). Combat redraws re-run it (`0x1106c`, `0x11495`). Combat reads front slots 0-2 (`0x2e282..`). Ranged monsters are chosen per monster, excluding slots 0-2, with no occupancy condition (`0x1ff7c..0x1ff98`). The `.mob` writer has no cap (`0x266e3..0x2671c`). Pinned `cmdSpawn`, `setIndoorsMonsters` and `moveMonsters` agree. | `XeenActorApproach::classify`, `XeenIndoorScene::classifyActors`, `XeenCombat` contact reconciliation and `XeenActorApproach::move` already follow this; M70 Part A removed the actor-table validators' per-cell cap of three. `XeenMapEventTests.cpp`, `XeenSaveStateTests.cpp` | Winterkill (map 32) and Tower of High Magic level 1 (map 55) stack their waves this way. Not checked in DOSBox play. |
| <a id="sv-award-removal"></a> Removing an award ORs `0xf` into the other nibble of its CHR byte instead of clearing it; granting ORs 1 into the award's nibble rather than setting it to 1. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: setAward `0x3ba9e..0x3bb05` (`or 0xf0`/`or 0x0f` on removal, `or 0x01`/`or 0x10` on grant), hasAward `0x3ba44..0x3ba9d`; byte `n` of CHR `+0x39` holds award `n` (low) and `n + 64` (high), 73 and 81 use bytes 62 and 63. Pinned `Character::setAward` clears and sets. | M70 Part B, `xeenEventTake`/`xeenEventGive` mode 15; [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws) | No Clouds script removes an award. A grant equals the reference while the count is 0 or 1. **ScummVM:** differs. |
| <a id="sv-script-values"></a> Give 74 activates Walk on Water and setValue 74 stores the value's low byte in Walk on Water and Wizard Eye; ifProc 69 reads Levitate as 0 or 1 and ifProc 93 the day modulo 10. | Static analysis, 2026-10-16: give `0x231df..0x232b5`, setValue `0x23c38..0x23c57`, ifProc `0x21a36..0x21a42` and `0x21ad1..0x21add`. Pinned `giveTake` clears Walk on Water, `setValue` stores `value != 0`, `ifProc` returns the Levitate count and `day / 10`. | M70 Part B, `XeenEventValues.cpp`; [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws) | Clouds If2 69 compares with 0 or 1; no Clouds If uses 93. **ScummVM:** differs. |
| <a id="sv-hp-sp-unsigned"></a> ifProc 78 and 81 compare the maximum HP/SP with the current value as unsigned words, so a negative current value is not at or below the maximum. | Static analysis, 2026-10-16: `0x21a6c..0x21a84`, `0x21a8e..0x21aa6` (`cmp`, `jb`). Pinned `ifProc` compares signed values. | M70 Part B, `xeenEventProbe`; [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws) | Affects only negative HP or SP. **ScummVM:** differs. |
| <a id="sv-takeorgive3"></a> TakeOrGive_3 (29) gives its third pair's mode and value. | Static analysis, 2026-10-16: `0x27669..0x27680` pushes the third mode (`-0x14`) and value. Pinned `cmdTakeOrGive` passes the second mode with the third value. | M70 Part B, `XeenEventInterpreter` | No Clouds script uses opcode 29. **ScummVM:** differs. |
| <a id="sv-chest-locks"></a> GiveMulti never picks a lock on object 0, and a chest whose sprite is in the lock table always needs picking: the test against 20 that follows reads the same sprite byte. | Static analysis, 2026-10-16: giveExt `0x23597..0x2359e` skips when the selected object byte `0x98d9` is 0; sprite table `0x235c2` (15 Darkside only, 16, 58, 73); the compare with 20 at `0x2360d` reads `-0x2004 + 8 * object`, the sprite byte read at `0x235b6`. Pinned `giveExt` tests `_objNumber != -1` and the object's x position. | M70 Part B, `XeenEventInterpreter` GiveMulti; [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws) | Three Clouds GiveMulti chests are object 0. The open chest frame is not saved and returns closed when the map is loaded again, as in DOS. **ScummVM:** differs. |
| Taking a quest item the party does not hold wraps its byte counter to 255 instead of being refused. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: giveTake take 21 decrements the counter byte with no floor (`0x22339..0x22340`). The pinned `_questItems[...]--` behaves the same on its byte storage. | M70 Part B, `XeenCloudsQuestItems::eventGrant`/`eventTake`, used by `xeenEventTake` and give 21 (the save keeps its counter width) | Original scripts test possession first. Every Event applies the same byte rule, which also wraps a grant from 255 to 0 (`0x22967..0x2296e`). |
| The Gold Vein at Dwarf Mine 3 (11,6) runs its Event facing North, but its vein wall item faces West, so the vein is drawn on the West wall and the North wall the party mines looks plain. | Original resources, 2026-10-16: `maze0035.evt` @2073 has direction 0 and line 18 MoveWallObj(3, -128, -128); `maze0035.mob` wall item 3 is (11,6) direction 3. Every other one of the 58 Clouds Gold Veins has matching Event and wall-item directions (or an all-direction Event). Wall items are drawn for the party's facing (pinned `setIndoorsWallPics`), as the original recording of map 34 confirms. | Data only; `XeenIndoorScene` draws wall items as the reference does | The vein still pays and disappears from the West wall when mined. |
| Random Event amounts from a word argument use only the operand's low word, divide by it as a signed word and sign-extend the result: GiveMulti 100 with 65537 gives 1, a word from `0x8000` draws `1..(0x10000 - word)`, and a drawn 32768 subtracts 32768 from gold or gems. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: low-word pushes at `0x22091`, `0x232bc` and `0x23770..0x237de`; helper `0x45b9a` divides `idiv` by `max + 1 - min` (`0x45ba3..0x45bac`); `cwd` before the 32-bit additions at `0x2209e`, `0x232c9`, `0x23791`, `0x237e6`. | M70 Part B, `xeenEventWordDrawBound`/`xeenEventSignedWord` (equal-pair take, give 100, GiveMulti 100/101/106) | No Clouds script uses a word from `0x8000`. A zero word divides by zero in DOS; MMModern refuses it before drawing. |
| Original MOB records may park monsters or unbound slots at `(100,100)`, or place records beyond the available DAT footprint. | Original Clouds census, 2026-10-15: maps 42-45, 64-66, 72, 80 and 82 have typed parking slots; maps 71, 73, 74 and 85 have unbound slots; map 49 has an unbound disabled sentinel. Maps 33 and 35 contain original placements outside their decoded DAT footprint. Pinned `combat.cpp::moveMonsters`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, bounds the occupancy grid to coordinates below 32; source resources establish the slot values. Not confirmed in DOS play. | `xeenValidateIndoorActors`, M70 developer session | Retain and validate exact untouched resource slots, including their MON binding and HP/wake defaults. Scripts park monsters the same way: Cave of Illusion (maps 42-45) Spawns slots to `(100,100)`, which is accepted for any slot below the MOB count. This does not permit a changed actor to go anywhere else off valid geometry. Missing geometry remains unavailable to movement. |
| Bash treats a real forward cell whose whole wall word is `0x8888` as `INVALID_CELL`: HP damage and the pause remain, but there is no Might roll or wall opening. | Maintainer DOSBox CD play, 2026-10-15: Vertigo `(8,4)` West, wall 12, refuses even with Might above 100. Calibrated read-only `WORLD/XEEN.DAT` analysis: lookup/compare `0x32e4f..0x32e5c`, whole-word return `0x11edb..0x11ee4`; the forward cell `(7,4)` is `0x8888`. [Bash rules](rules/time-and-conditions.md#bash). | `XeenBarrierCandidate`; `XeenBarrierTests.cpp`, `XeenBarrierOriginalTests.cpp` | Pinned `Interface::bash`/`Map::mazeLookup` already preserve this collision; MMModern had reduced it to a cell-existence check. This is not a restriction on wall type. DOS also zero-extends the two difficulty bytes (`0x32ee9..0x32ef3`), unlike the pin's signed metadata; MMModern uses unsigned values at the Bash comparison. |
| The World CD Clouds map-120 MOB contains an entity table index outside the 16-slot format. | Original-data census, 2026-10-14: the existing `XeenMapFormat::parseMob` rejects the member at offset 52. Pinned ScummVM `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, `engines/mm/xeen/map.cpp` around line 819, explicitly skips invalid World map-120 monster data when loading surrounding maps. Not checked in DOS play. | `XeenMapFormat`, `tools/XeenCoverageMain.cpp`, generated `coverage.md` | Keep the strict decoder. The census marks this resource unclassified rather than repairing bytes or treating absent decoded encounters as supported. |
| Item Use spends its charge before the effect, including Escape at an admitted target prompt and supported failures; an exhausted item disappears. It adds no time and gives monsters one normal response. | Pinned `dialogs_items.cpp` Use decrement-before-`castItemSpell`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`, and the established antidote cancellation witness (`XeenRegionalActionOriginal.cpp`). Maintainer DOSBox CD play, 2026-10-12, confirms no time, failed Jump spending and last-charge removal. | M61 Part B, `XeenJourneyFlow.cpp`, `XeenJourneyConsequences.cpp`, shared M60 effect rules | Unsupported powers and Jump terrain retain the approved pre-admission refusal. |
| Sell and Discard refuse cursed items; Sell also refuses weapons at or above the Xeen Slayer Sword ID. | Pinned `dialogs_items.cpp::doItemOptions` and `item.cpp::discardItem`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Cursed items were not reachable for the maintainer's 2026-10-12 DOSBox check. Discard's cursed refusal is DOS static since 2026-10-18 (`0x31034..0x31054`), and DOS Discard also refuses weapon ID exactly 34 ([M77 finding](#sv-discard-xeen-slayer)); Sell still follows ScummVM provisionally. | M61 Part A, `xeenQuoteItemService`, `xeenDiscardItems`, `XeenCharacterDialogFlow.cpp` | Refuse before money or inventory changes. |
| Broken items retain their full Identify details and normal Sell price. Identify has no durable known flag; Fix only clears the broken bit. | Maintainer DOSBox CD play, 2026-10-12: broken cudgel retained its electrical damage in Identify; broken equipment could be sold or repaired. Pinned `dialogs_items.cpp` confirms no identified-state write. | M61 Part A, shared item service rules and Identify view | Keep curse, counters, charges, material and frame bytes intact. |
| A valid Guild class book can buy spells even with zero maximum SP. Its list hides the row numbers but retains selectable offers; New instead shows the noncaster message with no learned rows. | Pinned `dialogs_spells.cpp::SpellsDialog::setSpellText/execute`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`: Guild builds offers before the maximum-SP check; New checks first. Ordinary DOS comparison pending. | M60 Part A, `XeenLearnedSpellRules::rows`, `drawXeenSpellList`, `XeenSmithFlow.cpp` | Do not add a maximum-SP purchase restriction. |
| AlterHed aliases indoor megamap coordinates through one primary-map head table indexed by `(x % 16, y % 16)`. | Pinned `scripts.cpp::cmdAlterHed` and `map.cpp::Map::load`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; ordinary DOS comparison pending. | M59, `XeenWorld::alterHeads/reloadHeads`, `XeenHeadState.h` | Unlike exploration, head indicators are not keyed by neighboring physical tile. Reload the original HED on an ordinary map load. |
| Original HED resources contain trailing bytes beyond the 16-by-16 head table. | CD `aaze0023.hed` and `aaze0028.hed` are 2,048 bytes; pinned `map.cpp::HeadData::synchronize` reads 256 left/right byte pairs. | M59, `XeenMapLoader.cpp::loadGeometryMap` | Read the first 512 bytes, as the reference does; do not reject the original trailing data. |
| Vern's guild offer has no membership check before charging 25 gold and granting membership again. | CD `maze0028.evt` `[648,715)`, selected-character debit 34/25 then award 15/0; pinned `party.cpp::giveTake`. Initial price/selection confirmed by maintainer DOSBox CD play, 2026-10-10; repeat payment confirmed in the accepted M59 play-test, 2026-10-11. | M59, generic Event debit/award rules; `XeenWorldEventTests.cpp` | Preserve the script's repeat behavior; do not invent a repeat-membership refusal. |
| Jail's year penalty directly advances the year and resets yearly flags and temporary effects; it does not simulate a year's daily ticks. | Pinned `party.cpp::giveTake` mode 85, `resetYearlyBits`, `resetTemps`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Maintainer DOSBox CD play, 2026-10-10/11 confirms one year, preserved day/time, jail destination and ordinary exit. Original program analysis, 2026-10-11 (`WORLD/XEEN.DAT` file offsets `0x2329c`, `0x21043`, `0x1e828`) confirms the reference's flags 55/155/222/231 and temporary resets; temporary age and torch count are not cleared. | M59, `XeenWorld::applyEventYear`, `XeenEventInterpreter.cpp` | Keep day/minutes; no invented daily condition rolls, merchant regeneration or bank interest during the jump. |
| Award grants show the portrait sparkle except Convicted Thief (award 8). | Pinned `party.cpp::giveTake` mode 15 explicitly excludes value 8 from `Interface::spellFX`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. Maintainer DOSBox CD play confirms sparkle for guild membership, skills and quest XP, 2026-10-11. Original program analysis, 2026-10-14: the give dispatch (`WORLD/XEEN.DAT` `0x22689`, table `0x23331`) skips the shared sparkle tail `0x23278` only for award value 8 (`0x22738..0x2275d`). | M59; M70 Part B `xeenEventGive`; `XeenWorldEventTests.cpp`, `XeenEventValueTests.cpp` | Keep the jail award silent; skill and XP grants sparkle for their actual recipients. |
| Equip refusals print the lowercase base item names from the original name tables. "You must remove great axe to equip spear!" backs over the trailing space, but the proficiency refusal keeps it: "Sorcerers are not proficient with a long sword !". | Original program analysis, 2026-10-14: `WORLD/XEEN.DAT` formatters `0x37a9b..0x37b28` (template `0x54fe5`, conflicting item first) and `0x3c564..0x3c5d4` (template `0x55c93`, no backspace) read id-indexed name tables without material; pinned `item.cpp::equipError/passRestrictions` agree. | M64, `XeenInventoryFlow.cpp`, `XeenItemCatalog::foundName`; `XeenDialogTests.cpp` | Do not substitute the decorated inventory names or trim the space. |
| Rialdo advertises 2,500 gold for Pathfinder but charges 2,000. | Maintainer DOSBox CD play, 2026-10-10/11; CD `maze0028.evt` at `(25,26)` North debits mode 34 value 2000, then grants mode 13 skill 11. | M58, generic teacher Event rules | Preserve the resource text and actual charge. |
| <a id="sv-skill-byte-wrap"></a> Skills use one byte each; incrementing 255 wraps immediately to 0, live and after restore. | Original program analysis, 2026-10-11: mode 13 increments the byte at character offset `0x27 + skill` (`WORLD/XEEN.DAT` `0x23345`, `0x2272b`, `0x22731`). Earlier explicit maintainer policy (2026-10-11) chose the same rule. ScummVM's `int _skills[18]` live counter with save-time byte truncation is not reproduced. | M58, shared skill grant site and v10 skill serialization | The site marker in `xeenGrantSkill` now cites this evidence (M70 Part B). **ScummVM:** differs. |
| Monster elemental resistance does not reduce spell damage: the spell path's elemental contribution starts at zero, so Energy Blast, Sparks and Toxic Cloud keep their base damage (the saving throw can still halve it). | Original program analysis, 2026-10-11 (`WORLD/XEEN.DAT` `0x2c675`, `0x2c70c`, `0x2c873`, `0x2bf8a`) matches pinned `combat.cpp::getMonsterResistance`. | M60 Part B, `XeenSpellDamageCandidate` | Looks like a bug, but it is the original rule. |
| Zero-level Energy Blast, Shrapmetal and Sparks still draw a nonzero magic-resistance gate but skip the saving throw because their base damage is zero. Physical immunity after nonzero base damage still retains the saving throw. | Pinned `combat.cpp::attack/attack2`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; ordinary DOS comparison pending for these target-stage boundary cases. Energy Blast's separate preparation draw is established below. | M60 Part B, `XeenSpellDamageCandidate` | Preserve draw order even at zero damage. |
| <a id="sv-energy-blast-roll"></a> Energy Blast prepares `U(2,6) * current caster level`, once per cast, before its sound and projectile. | Calibrated DOS static, 2026-10-22, high confidence: ID 20 table entry `0x33853` reaches `0x333ab` and handler `0x2f584`; RNG `0x2f588`, level `0x2f599`, multiply `0x2f5a4`, effect 16 `0x2f5b7`, projectile 13 `0x2f5bf`. RNG calibrated against Power Cure/Recharge and level helper against Sparks. The draw remains at level zero and without a target. Pinned `Spells::energyBlast` instead uses fixed `2L`. | `XeenSpellDamagePreparation`, shared by learned and item casting in combat and exploration; existing target gate/save order retained. **ScummVM:** differs; master not rechecked. | Preserve the preparation draw independently of target availability and Efx. |
| Insect Spray and Turn Undead draw the damage save for a compatible 25-damage target but ignore its result. Wrong-type Spray skips damage handling; wrong-type Turn Undead enters with zero and draws no save. Control spells do zero damage and do not change allegiance. | Reference only: pinned `combat.cpp::attack/attack2`, `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; no new DOS claim. | M73 Part B, `XeenSpellDamageCandidate::service`; gate precedes species, and each target settles before the next gate. |
| Magic Arrow draws the monster saving throw (`1..50 + resource ID`) although the result never changes its fixed 8 damage. | Original program analysis, 2026-10-11 (`WORLD/XEEN.DAT` `0x30fa8`, `0x2cde4`, `0x2c037`, `0x2c59a`) matches the reference. | Existing combat casting; M60 | Keep the draw: it advances the random sequence. |
| Mylo and Rialdo charge for repeat lessons and increment the skill again. | CD `maze0028.evt` byte ranges [514,581) (Mylo) and [581,648) (Rialdo), each containing Event lines 0..6, have no learned-skill test; pinned `scripts.cpp::cmdTakeOrGive` and `party.cpp::giveTake` mode 13 increment and continue. Maintainer scope clarification, 2026-10-11: follow these scripts, superseding the M58 plan's once-only assumption. | M58, `XeenEventInterpreter.cpp`, `XeenEventValues.cpp::xeenEventGive` | No idempotent learned flag or repeat-lesson refusal. Ordinary DOS repeat-lesson comparison remains pending. |
| Party skill checks count holders regardless of condition, including dead and unconscious members. | Calibrated DOS static analysis, 2026-10-19: complete helper `0x215b9..0x21609` reads only active-party size and indexed skill bytes; table `0x2160a` requires two holders for Mountaineer/Pathfinder and all members for Crusader/Swimming. High confidence; [evidence](rules/evidence.md#outdoor-movement-skills). | M58, `XeenSkills.h`, party skill queries | Agrees with pinned `party.cpp::checkSkill`; do not add an eligibility filter. |
| Darkness hides scene/monsters but leaves the minimap, full Map, exploration recording and border indicators usable. | Calibrated DOS static analysis, 2026-10-19: flag/light selection `0x12077..0x12093`; independent obscurer `0x3fd4a..0x3fe0e`; layer order `0x1161d..0x1162b`, `0x117b1`. High confidence; [evidence](rules/evidence.md#darkness-and-lighting-continuations). | `CloudsMapComposer`, `XeenWorldPresentation`; original dark map 33 in `XeenSensesOriginalTests`. | Ordinary dark-map lighting is black/clear, not a partial-light gradient. Three partial renderer levels belong to a distinct UI fade. Agrees with the pin. |
| Turns and waits mark stepped without displacement; refused movement does not. A pending light check can therefore drain on a turn. | Calibrated DOS static analysis, 2026-10-19: turn/wait suffix `0x0d38d..0x0d411`, `0x0d468..0x0d482`; mark `0x0c92b`; refusal branches `0x0d31c`, `0x0d35a`, `0x0d3a5`, `0x0d3dd`; draw clear `0x1184f`. High confidence; [travel checks](rules/evidence.md#travel-lighting-checks). | Movement/arrival and presentation continuation owners in M78. | Refusal charges no action time or explicit monster opportunity; ordinary redraw scheduling continues. No initial Clouds DAT cell has the drain bit; this is a generic consumer rule, not a claimed natural play-test witness. |
| <a id="sv-pathfinder-movement"></a> Outdoor wall types 3 and 6 require two Pathfinder holders in DOS. | Calibrated DOS static analysis, 2026-10-19: outdoor dispatch `0x0cec4..0x0cedf`, table `0x0cfc1` entries 3/6, skill-11 call `0x0cf38..0x0cf44`, shared two-holder check `0x215e9`, refusal FX 21/false `0x0cf46..0x0cf50`. High confidence; [evidence](rules/evidence.md#outdoor-movement-skills). | `XeenMovement`, `XeenSkills`; synthetic holder/wall tests and original map-23 crossing in `XeenSensesOriginalTests`. | Raw wall types, not an invented forest restriction or discount. Navigator is skill 10 and separately prevents desert arrival time. **ScummVM:** differs: pinned `interface.cpp::canMove` has no cases 3/6 and permits them through its default path. |
| A failed If test of a skill, award, condition or spell, and an unhandled If action, compares an uninitialised value: before the instruction's first test it is the interpreter's saved SI:DI from the per-instruction message copy (DI is the record's offset in the Event file); later members of a party loop see the previous member's value. Party skill tests (99) use unsigned -1. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: ifProc leaves its local unset on these paths (`0x216c0`, `0x216d3`, `0x21703`, `0x21729`, `0x21b84`) and sets -1 only for 20, 21, 99, 103, 104, 107 (`0x21b7a`); the dispatcher's `_fstrncpy` (`0x26c26..0x26c4d`, `0xacd6..0xacda`) is the last call at the same stack depth and saves SI then DI in those slots; no call intervenes before the If handler's test (`0x2712d..0x272ab`). Pinned `scripts.cpp::ifProc` starts from 0 (conditions: 0xffffffff). | M70 Part B, `xeenEventProbe`/`xeenEventCompare`; [world Event rules](rules/world-events.md#script-values-conditions-writes-and-random-draws) | All 91 Clouds uses are If2 `==` tests whose operand differs from their record offset, so they fail as in the reference. An outcome that would depend on the unknown SI refuses as not supported. An interrupt between the copy and the test could also change the slot; not modelled. |
| The indoor minimap draws its partial left/top wall loop once, uses frame 24 for the left TFLR edge, and the indoor Map marker tests row `14 - arrowY / 10` despite eight-pixel tiles. | Pinned `interface_minimap.cpp::draw` and `dialogs_map.cpp::execute`, revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. | M58, `XeenMapView.cpp` | Preserve the reference placement and loop bounds; compare the visuals during the maintainer's DOS play-test. |
| Map lookup treats sky and space surfaces as visible without recording exploration. | Pinned `map.cpp::mazeLookup` sets its disposable stepped lookup result for these surfaces. | M58, `XeenMapView.cpp::lookup` | Both views preserve this display rule without writing explored cells or importing cached seen bits. |
| Fix ignores the Merchant skill even though the callers pass different values: the pinned reference passes `actionIndex` where Sell passes Merchant, and the DOS service callers pass the character skill byte. | Both the DOS routine and the reference overwrite that argument with Fix divisor index 3, so the price is the same in both: `max(1, value / 10)`. Read-only `WORLD/XEEN.DAT` analysis, 2026-10-12: price mode dispatch `0x3c6ab-0x3c6d5` (the Merchant byte is masked with `0x7f` at `0x3c6ab` and tested at `0x3c6c2`), service callers `0x3ab29-0x3ab32`, `0x3ac78-0x3ac81`, `0x3adbc-0x3adc5`, `0x3aee0-0x3aee9`, divisors `0x4d880`; calibrated against M38 DOSBox prices. | M38 Armor Repair, M61 plan | Equivalent behavior; no correction to the reference formula is needed. |
| In the save/load chooser, the scroll arrows show their pressed frame but do nothing, and the bar between them does nothing; only ten slots exist. | Maintainer DOSBox CD play, 2026-10-09 (an eleventh slot pair is ignored). | M54 chooser | QoL candidate (scrolling or more slots). |
| After a member runs from combat, the Quick Fight Options dialog keeps the old party indexing: F1-F5 and the portraits select the members by their pre-Run positions, so the visible portrait and the selected member differ; F6 does nothing. | Maintainer DOSBox CD play, 2026-10-10; matches `dialogs_quick_fight.cpp:97-99` in the reference. | M55, `XeenCharacterDialogFlow.cpp::handleQuickFightOptions` | QoL candidate. Combat actions still use the real actor. |
| Retreat leaves disabled members still in the combat party Dead. | Reference only: pinned `interface.cpp::Interface::doCombat`, lines 1870-1880, moves to the Run location and sets Dead on remaining `isDisabled()` members. Observed in the existing MMModern rule while repairing M71's ordinary Temple witness; not newly confirmed in DOS. | `XeenCombat::finishDisengagement`; M43/M44 ordinary resurrection preconditions | Existing behavior retained without a production change. Successfully fled members have already left the combat party. |
| A valid Bank transfer adds to its destination using unsigned 32-bit arithmetic, so the destination can wrap. This is distinct from refusing an entered amount above `UINT32_MAX`. | Pinned `locations.cpp::BankLocation::depositWithdrawl` adds to the party's unsigned balances; M56 retains that balance arithmetic. | M56, `XeenFoodBank.cpp::xeenTransferBank` | No saturation or destination-capacity refusal. |
| Vertigo's exit reset respawns slots 0-11 at their own MOB positions, slot 12 at `(4,12)`, and each slot 13-40 at the position of the MOB record before it, so the list runs one record behind from slot 12 on; slots 41-45 are not respawned. The script's last two Spawns, slots 50 and 51 at `(7,24)` and `(6,27)`, are past the 46 MOB records and stay inert (see the Spawn row below). | CD `maze0028.evt` Spawn records 788-830 at `(100,100)`, lines 0-42, compared with the city MOB's 46 monster records (read 2026-10-15 through the project loaders). | M69, `XeenWorld::applySpawn`; `XeenVertigoProcessTests.cpp` reset table | The order comes from the script data; keep it. Not separately checked in DOS play. Before M70 Part A, slots 50 and 51 became live type-0 monsters, as in the reference. |
| `MoveObj` at an object's current position toggles its flip, but the next scene draw replaces that flip with the directional animation metadata, including for off-screen objects. | Pinned ScummVM `scripts.cpp::cmdMoveObj` and `interface_scene.cpp::drawScene` (revision `6814ee9ba54582f5b5adcffab49efbbd8f589edd`); maintainer confirmed following these resets on 2026-10-10. | M57, `XeenWorld.cpp::applyMoveObject`, `XeenIndoorScene.cpp`, `XeenOutdoorScene.cpp` | Position changes remain durable. Flip observations are disposable; saves retain the effective flip without serializing animation frames. |
| Event XP and carried currency additions use unsigned 32-bit arithmetic and can wrap. Award IDs 73 and 81 name slots 126 and 127; granting an award never increments its count (DOS ORs 1 into its nibble; see [award removal](#sv-award-removal)). | Pinned `party.cpp::giveTake`, `character.cpp::fixAwardId/setAward`. | M57, `XeenWorld.cpp::applyEventGrant`, `XeenCharacter.h::awardIndex` | CHR counts are retained for all 30 owners, including the full-byte Warzone count in slot 9. |
| Completely full backpacks discard remaining ordinary items but still credit gold and gems. | DOS `WORLD/XEEN.DAT` four 40-byte item clears `0x2433e..0x2437b`, currency credit `0x2486a..0x24884`; [delivery evidence](rules/evidence.md#treasure-delivery), high confidence. Supersedes M57's reference-based currency-loss entry. | **Implemented (M61 Part D)**, shared `XeenItemRewards.cpp` delivery and monster treasure | Generation draws remain consumed; currency is not inventory capacity. |
| <a id="sv-loot-recipients"></a> With no valid preferred recipient, delivery goes directly to a capacity-only scan, including sleeping, disabled and dead members. A valid preference first tries eligible recipients, then the same unrestricted fallback. | DOS `0x243d5..0x24565`, actual switch tables and condition helper checked; [delivery evidence](rules/evidence.md#treasure-delivery). | **Implemented (M61 Part D)**, Event and monster shared delivery | Do not filter the final scan by condition. **ScummVM:** differs. |
| <a id="sv-category-full-replace"></a> When this category alone is full, treasure replaces the first member's last item in that category, retaining its equipped mark. | DOS selects member zero at `0x24567`; weapon writes `0x245aa..0x245cc` replace material/ID/state but not frame; corresponding writes for all four categories and sort confirmed. | **Implemented (M61 Part D)**, shared `xeenDeliverRewardItem` | **QoL candidate.** Global fullness instead discards remaining items. **ScummVM:** differs. |
| Natural 20 applies enemy damage before the ordinary hit comparison; armor broken by that application affects the second AC check. | DOS `0x2db45`, `0x2db55`, `0x2db65`, fresh AC `0x2dbd7`, injury `0x1f8a7`, bad-armor gate `0x21275..0x21281`; [combat](rules/combat.md). | Existing `XeenCombatRules.cpp` | DOS-confirmed, high confidence. |
| Broken weapons can retain base dice while losing material effects; effectiveness scans equipped bad-state and missile weapons. | DOS bad-state gate `0x2e057..0x2e05d`, base dice `0x2e0e9..0x2e134`, effectiveness `0x2c950`, `0x2c96f`, `0x2caed..0x2cb00`. | **Implemented (M67)**, `XeenCombatRules.cpp::XeenPhysicalPlayerCandidate`; [combat](rules/combat.md) | DOS-confirmed scan: a broken or cursed effective weapon still triples the melee total, and an equipped missile weapon's counter triples melee damage. |
| Hatred 1 bypasses class preference; Run succeeds only for draw below the map threshold. | DOS `0x2d84b..0x2d852` to `0x2d9a3`; Run `0x2e1c0..0x2e1d4`. | Existing targeting and Run rules | Threshold 100 accepts outcomes 1..99, not 100; no DOS RNG uniformity claim. |
| Long time advances apply economy once using the final day, rather than per crossed day. | DOS calendar `0x263b3..0x26491`, stock `0x26401`, interest `0x26408`, latch `0x2640b..0x26411`. | [Calendar economy](rules/time-and-conditions.md#calendar-economy-and-direct-year-changes) | DOS-confirmed within word-sized calls. A synthetic 144,000-minute call is not representable in DOS. |
| <a id="sv-temple-negative-hp"></a> The Temple Heal price compares current HP with maximum HP as unsigned words, so a character at negative HP pays nothing for the HP term. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: price routine `0x1c9ce..0x1cd3e`, comparison at `0x1cadb` (`cmp`, `jbe`) against the maximum-HP helper `0x20182`. Pinned `TempleLocation::createLocationText` compares signed values and charges it. Maintainer decision, 2026-10-16: follow DOS. | `xeenQuoteTempleHeal`; [Temple Heal price](rules/time-and-conditions.md#temple-heal-price) | A Dead character usually has negative HP, so resurrection costs 10*L less than in ScummVM. Not checked in DOSBox play. **ScummVM:** differs. |
| <a id="sv-temple-eradicated-wrap"></a> The Temple Heal Eradicated term multiplies the counter by 500 in 16 bits and sign-extends it, so counters above 65 wrap. | Static `WORLD/XEEN.DAT` analysis, 2026-10-16: `0x1cbd8..0x1cc1e` (`imul` by 500 on the counter byte, `cwd` before the 32-bit sum). Pinned `createLocationText` uses full integers. | `xeenQuoteTempleHeal` | Unreachable in ordinary play: Eradication sets the counter to 1. A wrapped total below zero is refused as unsupported; DOS behavior for a negative total is not traced. **ScummVM:** differs. |
| <a id="sv-temple-map74-carry"></a> Newcastle Level 2's Temple (map 74) keeps the last Temple prices used in the process, including across New Game or Load. | Calibrated DOS static recheck, 2026-10-20: selector `0x1ca30..0x1ca4d`, corrected table `0x1ce87` (not `0x1d187`), globals `DS:f64e..f656`; startup `0x5762..0x577d` zeroes them. The actual Clouds temples are on 28, 30, 49 and **74**; 73 is only a price-table case. Save/Load buffers exclude these globals. [Full evidence and bounds](rules/evidence.md#other-town-services-2026-10-20). | `XeenTemplePriceState` (process owner), `XeenEncounterFlow::serviceSmithPreparation` (admission publication); [exact charges](rules/time-and-conditions.md#temple-heal-price). The prices are process state: never saved, kept across New Game and Load. | **ScummVM:** differs; `LocationManager::doAction` creates a new `TempleLocation` for every visit, whose constructor zeroes all five members. Thus its map 74 always uses zero surcharges and Donation 0. This corrects the former claim that the pin preserves DOS carry-over. |
| <a id="sv-rivercity-food"></a> Rivercity Food fills to 60 rations per active member for 50 gold, using Tavern text 76. | Calibrated DOS static analysis, 2026-10-20: Clouds selector `0x1b202..0x1b247`, Rivercity multiply by 60 at `0x1b21f`, price at `0x1b225`, text selection `0x1b248..0x1b28b`. | `xeenBuyFood`; [Tavern Food](rules/time-and-conditions.md#tavern-food) | **ScummVM:** differs; the pin fills to 50 per member at the same price and text index. Vertigo 15/10/75, Shangri-La 120/100/77 and the unsigned Full test agree. |
| <a id="sv-untabled-food"></a> Untabled Clouds Food skips assigning the target, price and local text selector. | Calibrated DOS static analysis, 2026-10-20: `0x1b202..0x1b248`; prologue `0x1ab24` allocates but does not initialize the selector. Target/price globals retain previous values; the exact resulting text is not established. No original Clouds Tavern uses this path. | [Tavern Food](rules/time-and-conditions.md#tavern-food). `XeenFoodBank::xeenTavernFoodOffer` returns no offer and Event admission refuses before any effect. The usable fallback remains **Reference only**; no deterministic DOS offer or extra history is selected. | **ScummVM:** differs in control flow; it explicitly assigns 15/10/75. This is an unreachable original-data path, not newly implemented behavior. |
| <a id="sv-tavern-tip-cycle"></a> Tavern tips advance by byte increment followed by remainder modulo 15 on Clouds or 10 on Darkside; Clouds map 30 uses text base 45. | Calibrated read-only `WORLD/XEEN.DAT` analysis, 2026-10-17: tip text/index `0x1b4c2..0x1b521`; town selector `0x1b49e..0x1b4bb`. The pin divides instead of taking the remainder and checks Clouds map 45 instead of 30. | Existing Tavern rule/Flow owners and per-map `tavernTips`; the save retains the cursor, initialized from original DAT. | A successful Drink sets a visit latch; Tip consumes it before checking Drunk or payment. Repeated paid tips progress through the original text. **ScummVM:** differs. |
| <a id="sv-temple-visit-time"></a> Every paid Heal or Uncurse adds one day to the Temple visit; the common exit adds another day. | Calibrated DOS static analysis, 2026-10-17: shared paid tail adds 1,440 to a word at `0x1ae68`; Uncurse joins it at `0x1b01d`; common exit adds 1,440 and calls addTime at `0x1d78e..0x1d798`. The called routine wraps minutes plus charge as a word (`0x263be`), normalizes unsigned (`0x263e5`), and compares the charge signed for stock regeneration (`0x263f9`). The pin assigns a single paid day and uses wider arithmetic. | Existing `XeenServiceDayCandidate` and Temple Flow retain the visit's word-sized charge; merchant generation/interest still run once at departure. | Two successful paid actions cost three days; Donation adds no extra day. Preserve both word additions and the signed regeneration threshold; see worked edges in [time rules](rules/time-and-conditions.md#tavern-and-temple-options). **ScummVM:** differs. |
| <a id="sv-terrain-damage-start"></a> Lava and surface-fall damage starts at 100 independently for every active member, including terminal members. | Calibrated DOS static analysis, 2026-10-17: arrival `0x0c924..0x0ca48` selects all-party damage; original damage saves its input at `0x23d63..0x23d66` and reloads it for each member at `0x23de0..0x23de8`. The pin carries reductions between members. | Shared terrain arrival reuses a fresh single-target damage candidate for each member, in active order. | Fire saves include the final zero-damage draw; physical falls make no save draw. This evidence does not silently widen M72 into changes to existing Event damage. **ScummVM:** differs. |
| Uncurse clears only the Cursed condition and item-state bit `0x40` in every slot of every carried category, without destroying items or requiring an equipped mark. | Calibrated DOS static analysis, 2026-10-17: condition clear `0x1afa2`; four nine-slot loops `0x1afb0..0x1b01b`; original Temple pricing provides calibration. The pin agrees. | Existing Temple rules and dialog; all other item bytes, including empty-slot metadata, remain exact. | One price covers any number of cursed slots. |
| Donation's counter and blessed latch belong to one Temple visit, not to saved party history. | Calibrated DOS static analysis, 2026-10-17: counter update/comparison `0x1aee4..0x1af01`; effect writes `0x1af04..0x1af28`; visit latch `0x1af5c..0x1af73`; counter resets on entry `0x1d14b` and member selection `0x1b04e`, latch resets `0x1cee4..0x1cef1`. | Existing saved party effects retain Clairvoyance, Light and the four bonuses; the dialog owns the transient counter/latch. | An existing Blessed effect does not make a new visit's Donation free. Day 0 needs ten donations from counter zero; day 8 needs eight. |
| Clouds Tavern text groups are assigned only for maps 28, 30 and 49; an untabled map would retain the previous group. | Calibrated DOS static analysis, 2026-10-17: Tip selection `0x1b49e..0x1b4c2`, Rumors `0x1b626..0x1b6ad`. No original Clouds tavern uses an untabled map. | `xeenTavernTipText`, `xeenTavernRumorText` refuse unknown Clouds maps instead of inventing a group. | Darkside explicitly assigns its fallback group. No extra saved history is introduced for this unreachable Clouds case. |
| Rest has no surface-specific refusal; after reset and recovery it applies the same terrain routine as steps and successful casts/shots. | Calibrated DOS static analysis, 2026-10-17: Rest `0x1e4f5..0x1e827`, ordinary restriction `0x1e50d..0x1e52e`, arrival call `0x1e7df`; other callers `0x0d411`, `0x0d5ba`. | Existing Rest and shared terrain arrival owners. | Rest removes Levitate before cloud arrival. Sky ignores Levitate even outside Rest; Swimming and Walk on Water are movement permissions, not lava/sky/space protection. |
| Learned Jump applies terrain through the successful-cast suffix; item Jump does not. | Calibrated DOS static analysis, 2026-10-17: all direct shared-arrival callers are `0x0d411`, `0x0d5ba`, `0x1e7df`; the item-use dispatch `0x39e4b` returns through its ordinary monster response without that suffix. The pin's `jump` handler only moves and sets stepped. | Existing learned-cast and item-use continuations, shared arrival owner. | Neither an ordinary monster response nor the ten sleeping Rest opportunities invents an arrival call. |
| Ordinary manual Space, including an empty cell, charges time and runs a monster opportunity before arrival; a terminal service does not repeat that suffix. | Calibrated DOS static analysis, 2026-10-17: Space dispatch `0x0d468..0x0d482`; common service farewell `0x1d782..0x1d79d` and current-key clear `0x1d7c8` before return `0x1d7ce..0x1d7d3`. | Existing Event, regional opportunity and service farewell owners. | The entire manual Event chain finishes first. Service farewell remains its original separate charge, without an extra minute/opportunity/hazard on return. |
| Space that handles a door or grate does not run the ordinary Space arrival suffix. | Calibrated DOS static analysis, 2026-10-17: openGrate starts at `0x0c794`, clears the current key at `0x0c911`, then returns at `0x0c917..0x0c91a`; outer dispatch tests that key at `0x0d263`. | Existing barrier owner retains its own effects, with no extra Space charge, monster opportunity or terrain arrival. | Bash pass-through still reaches charged movement and arrival; blocked Bash does not. |
| <a id="sv-darkside-fall-bounds"></a> Darkside surface falls use the DOS dispatch bounds, which differ from the pin at maps 113 and 130-132. | Calibrated DOS static analysis, 2026-10-17: `WORLD/XEEN.DAT` `0x0cb70..0x0cb86` maps 89-112 to map minus 88; dispatch `0x0cb93..0x0cb98` includes only 25-129, table `0x0cd22`. | `xeenTerrainFallDestination` uses these original bounds; other maps use Darkside 29 `(25,21)` North. | The pin includes 113 in the subtraction range and 130-132 in later special cases. Darkside gameplay remains unadmitted. **ScummVM:** differs. |
| <a id="sv-spell-row-height"></a> Spell list rows are nine lines tall, so a click anywhere between two spells selects one of them; the reference's eight-line rows leave a dead line under each spell. | Calibrated static `WORLD/XEEN.DAT` analysis, 2026-10-18: the spell dialog button table at `0x4f19a` stores x words, then y, width, height and key bytes, for 16 buttons; the ten rows have x 40, y 28 + 9k, width 147, height 9. The mouse decoder `0x0e32b..0x0e396` tests `x0 <= x < x0 + width` and `y0 <= y < y0 + height`; its portrait test `0x0e2d8..0x0e301` (`[150,182)` by `[face, face + 32)`) calibrates the half-open rule against the known party buttons. Pinned `SpellsDialog::loadButtons` uses `Rect(40, 28 + 9k, 187, 36 + 9k)`. | `xeenSpellListInput` (Cast New, Guild and combat spell lists); `XeenServiceDialogTests.cpp` | The scroll, page, Select and Exit buttons in the same table match the pin. **ScummVM:** differs. |
| Item list rows are eight lines tall at a nine-line pitch and service rows (Tavern and similar) eight lines tall at a ten-line pitch, so a click on the line between two rows selects nothing. | Same static analysis, 2026-10-18: the item dialog table at `0x4f4ca` stores the nine rows as x 8, y 20 + 9k, width 255, height 8; the Tavern table at `0x4f2dc` stores x 234, y 54/64/74/84, width 74, height 8. The pin's rectangles are equivalent. | `xeenItemsInput`, `xeenLocationInput`; `XeenServiceDialogTests.cpp` | **QoL candidate:** close the gaps as an opt-in. |

<a id="sv-loot-load-process-state"></a>**Pending treasure is process state,
not save data (M81 Part 0, 2026-10-20).** Calibrated DOS static analysis, high
confidence: normal Save checks combat (`0x1f1cb..0x1f1e6`) and the maze
restriction (`0x1ebcf`), not pending loot. Its three party/roster source ranges
(`0xe951..0xe982`, writer `0x45e36..0x45eae`) exclude DS:`9970..9a19`:
all forty staged items, currency and hasItems are absent from the file.
An open receipt cannot dispatch Save. In-process Load returns through
`0xd2fa..0xd305`, `0xb819..0xb839`; party reload replaces saved ranges and
characters, and play entry clears currency/hasItems (`0xcfe3..0xcff7`) but
leaves the current four arrays dormant. These may contain items staged after
the chosen save. Startup clears DS:`[8c4c,fd78)` at `0x576c..0x577c`, so a
fresh process starts empty; no save recovers its pending
loot. New Game behaves like Load: its path (`0x20a2b..0x20c43`) writes none of
the arrays and reaches the same play entry. Full-party Run has the same currency/flag-only clear
(`0x2e932..0x2e946`), and a later currency/readiness trigger can deliver the
retained arrays. [Full trace and boundaries](rules/evidence.md#m81-monster-treasure).
**ScummVM:** differs on Load: pinned `SavesManager::loadGameState` calls
`Treasure::reset`, which also clears the arrays. Run and New Game agree.
**Implemented (M81 Part A):** `XeenProgramState` (created by `main`) owns the
staging for the whole process; save v17 holds only the purse. A successful
entry (`Application::playGameplay`) clears currency and readiness and keeps
the arrays; a failed or cancelled entry restores the owner exactly.

<a id="sv-event-loot-stash"></a>**Event treasure shares the monster staging, and
pending monster currency makes it deliver monster items twice (M81, 2026-10-20).**
Calibrated DOS static analysis, high confidence: every Event run (`0x26774`)
stashes pending currency, readiness and the forty staged slots when any of them
is set, clears only currency and readiness, stages Event items in the same arrays,
delivers after the Events (never deferred by monsters, since the Event loop runs
in mode 9) and restores the stash over the arrays. With stashed monster currency,
an Event reward hands the monster items over, and the restore stages them again
with their currency, so the items are delivered twice and the currency once.
Without stashed currency, an Event reward also delivers dormant monster items.
[Trace](rules/evidence.md#event-processing-stash). Kept on purpose as an
original bug (maintainer, 2026-10-20); **QoL candidate** for a later opt-in
correction. **Implemented (M81 Part B):** each Event run keeps its stash and
entry snapshot in `XeenEventExecutionState`, stages into the shared arrays and
delivers with the shared monster/Event delivery; the cell's Events run before
ordinary monster delivery, and an aborted run leaves the staging unchanged.
**ScummVM:** differs: the pin stashes only when the bitwise `gold & gems` is
nonzero.

<a id="sv-unused-drop-fields"></a>**Unused monster drop fields and currency
widths (M81 Part 0).** No Clouds monster uses drop fields 7-21. The DOS chance
lookup (`0x2c37a..0x2c39a`, base `0x4d650`) has no bound: 7-17 read zero and
18-21 read 5/4/5/5 from the bytes after the table (`0x4d661..0x4d664`); the pin
uses zero. Neither establishes item generation at those levels, so MMModern
refuses such a record (`XeenMonsterRecord::dropLevel`, checked with the attack
capabilities before any draw); no Clouds record is affected. Darkside's Doom
Knight and Sandro use field 7 and are refused until Darkside gameplay. Staged and purse gold and gems are 32-bit add/carry sums without
saturation (`0x2c2c2..0x2c307`, `0x2486a..0x24884`); MMModern keeps the same
widths, wrapping modulo 2^32. [Evidence](rules/evidence.md#m81-monster-treasure).
**ScummVM:** differs only on fields outside the original Clouds records.

## Unconfirmed in DOS, following the reference

Cases the original cannot be checked for in normal play or by program analysis;
they follow the pinned ScummVM reference under the AGENTS.md condition-counter
rule, or follow the reference where it has not been checked.

**Counter-boundary discrepancy:** current `XeenConditionTimeCandidate::service`
wraps Insane/Heart Broken/In Love as bytes and saturates terminal counters at 255
(`XeenDailyTimeOracles.h` checks these choices). The pin stores live `int`
counters (`character.h:149`, `party.cpp::changeTime`) and truncates only on
serialization (`character.cpp::synchronize`); its behavior at 255 is different.
These current choices must not be called literal reference equivalence or DOS
confirmation. The established DOS Poison/Disease and Weak/Drunk results do not
settle these other counters. See [the precise boundary](rules/time-and-conditions.md#eight-hour-condition-block).
This research records the mismatch without changing behavior or granting an
exception to the root condition-counter fallback rule.

<a id="per-map-write-back"></a>**Per-map write-back (M69):** the pinned reference writes a map's EVT, its
primary DAT (indoors also its eight neighbour tiles) and its MOB into the save
archive on every map load and save (`map.cpp::Map::load/saveMaze`,
`saves.cpp::saveGame`), but never its HED: AlterHed changes last until the next
map load and are lost on reload. MMModern's save keeps only the current
primary map's head cache (v11 layout, unchanged in v13) and drops it on a map
load, as the reference does. The reference also never writes Clouds map 85
(The Warzone) back: its state lasts for one visit. MMModern drops the
Warzone's map state and monster accounting when the party leaves it, so
re-entry starts from the original resources, never writes it into a save and
refuses a save that contains it (M69, `xeenRetainsMapState`). The Warzone is
not admitted yet. Not confirmed in DOS.

**Remove hides an object only by its position (2026-10-19):** the pinned
reference's Remove writes the selected object's position as `(128,128)`, the
signed byte -128 in a save, and keeps nothing else (`scripts.cpp::cmdRemove`);
a later MoveObj on that object writes a new position and the object shows up
again (`cmdMoveObj`). MMModern does the same: Remove replaces any MoveObj
position, MoveObj clears the removal, the object keeps its flip, and a save
holds one position per MOB record. An original-data scan found no Clouds or
Darkside script that reaches MoveObj after Remove on the same object (Remove
exits, and every object a Remove can select is moved, if at all, only earlier
in that script, to `(-128,-128)` or `(100,100)`); no AlterEvent writes either
opcode. A MoveObj to `(-128,-128)` on a removed object does not toggle its
flip, because no signed-byte operand equals `(128,128)`; the next scene draw
replaces the flip in any case. Where:
`XeenWorld::applyMoveObject/applyRemove`, `XeenEventPublication::prepareRemove`;
`XeenSaveStateTests.cpp::removeThenMoveRoundTrip`. Not confirmed in DOS.

**Equipped item contributions (M67):** the following follow the pinned
reference (`combat.cpp::attack/attack2/getWeaponDamage/getMonsterResistance`,
`character.cpp::itemScan`, `interface_scene.cpp`) and are not confirmed in DOS
beyond the retained scans listed above; the maintainer's DOSBox comparison in
the M67 play-test is the intended check.

- The elemental weapon contribution is added once per action, after the
  accumulated attempts, effectiveness and physical resistance, and is never
  tripled; Shoot halves the combined amount on a monster save.
- Elemental-only damage shows no splat: `attack2` arms the splat duration only
  for a positive physical share, then draws the elemental frame beside it with
  its own scale (`XeenScenePresentation::Splat::elementalFrame`).
- Effectiveness triples once per matching equipped weapon, so two matching
  weapons give nine times; Shoot never applies it.
- A weapon with ID 34 or above (Xeen Slayer Sword) bypasses physical
  resistance; Lord Xeen's `+100` and his immunity to other damage are not
  implemented (no reachable monster).
- `getWeaponDamage` assigns a metal weapon's damage modifier instead of adding
  it; with one equipped melee or missile weapon this is unobservable, and
  MMModern assigns as the reference does.
- Heroism adds to the hit modifier per matching slot. M76 Part 0 confirms
  this DOS rule, including Shoot and Poison Volley; see [the consumers](rules/magic.md#party-effects-and-movement).

## Original findings and current discrepancies

These findings include calibrated DOS analysis and explicitly labelled
reference/resource checks. Implementation status is explicit in each row. The exact executable hash,
calibration, retained REA records and limitations are in
[rules evidence](rules/evidence.md).

| Original behavior | Evidence and comparison | Implementation boundary |
| --- | --- | --- |
| <a id="sv-cd-promus-fm-banks"></a> The CD `promus` driver enables OPL3 and uses both FM register banks with opposite stereo outputs. | Calibrated DOS static analysis, high, 2026-10-21: INTRO.CC resource offsets `0x2cb..0x2d0` enable NEW mode, `0x2d1..0x33c` set routing, and `0x4d0..0x54a` write both banks. DAT startup/resolver `0xb4d2..0xb503`, `0x45f4c..0x45fa6` establishes INTRO.CC, with a loose-file override first. Two 11-byte patches are used per voice; FX definitions are 22 bytes and FX opcode 0 is a no-op. FX opcode 6 retains a faulty byte-indexed frequency-cache write at resource `0x722`. [Full semantics and identities](rules/audio.md#promus-commands-and-voices). **ScummVM:** the pin requests one OPL2, reads `blastmus`, uses 11-byte FX definitions and an FX call opcode; master not rechecked. | Maintainer decision, 2026-10-21: M82 reproduces CD promus on pinned Nuked in OPL3 stereo; admus/blastmus belong to roadmap step 16. Implemented by `PromusDriver` over the filtered plain Nuked core in `XeenOpl3`; the effect opcode 6 cache defect is preserved, including music opcode 8's `0x1fff` write mask (`0x762..0x771`) after the corruption; FX opcode 7 instead uses `0xdfff`. No listening-equivalence or dual-OPL2 hardware compatibility claim. |
| <a id="sv-cd-fm-tick-fade"></a> Promus ticks at approximately 72.80 Hz and advances fade on byte carry; music-due ticks skip the FX countdown. | Calibrated DOS static, high: resource `0x266..0x290`, `0x693..0x6e9`, `0x9cc..0xa19`, `0xac5..0xadb`. Command 207 takes 78..80 eligible ticks depending on persistent phase; stream-ending ticks skip postprocessing. Music stop keys off only voices 0..6 in both banks. [Timing formula and exceptions](rules/audio.md#tick-scheduling-and-fade-length). **ScummVM:** pin truncates 72.8 to 72 Hz, uses integer/sign tests for fade and slide, and differs in octave correction; its skipped FX countdown agrees with DOS. Master not rechecked. | `PromusDriver` retains the countdown priority, byte carry, terminal-tick exclusions and persistent phase. Each fade carry emits the previous cached level (loads at `0x9f4/0xa04`, increments at `0x9fd/0xa0d`), and patch loading substitutes fade steps in bank zero only without refreshing the level-command caches; `patch` and `postprocess` retain those asymmetries. `XeenAudio` predicts pending loads from the presentation clock without waiting on device output. |
| <a id="promus-write-spacing"></a> Promus spaces every OPL register write in real time, so a tick's key-off and following key-on reach the chip at separate moments. | DOS static, high: the delay loops of the [I/O audit](rules/audio.md#tick-scheduling-and-fade-length) exist (three PIT reads after the address, twenty after the data). Inferred from that I/O count, high: about 23 bus reads per write, roughly one 20.1 µs native sample or more apart. DOSBox capture `promus-fx20-alone`: 36 writes within one millisecond of its second burst, an upper bound of about 1.4 samples on the average interval; its first millisecond holds 176 writes, so DOSBox I/O timing is not hardware timing. Unspaced writes left effect 20 (Efx on) almost silent. | Follows the pinned reference's spacing: `XeenOpl3` calls `OPL3_WriteRegBuffered`, two native samples per write, as the pin's `OPL::writeReg` does; the exact DOS interval is not reproduced. Maintenance fix, 2026-10-21; `xeen_promus` compares effect 20's rendered energy with the capture. |
| <a id="sv-promus-pas-mixer"></a> Promus writes card-mixer registers separately from OPL attenuation. | DOS static, high: INTRO promus `0x23c..0x265` initializes mixer 2e/22/26 to 255 and 04 to 153 when enabled; command 0x100 at `0x40f..0x438` computes the byte for 26. DRO captures record OPL writes only and establish no card-mixer gain or listening equivalence. **ScummVM:** the pin uses its own software volume scaling rather than this PAS mixer interface; master not rechecked. | Card-mixer gain is unmodelled in Part A: `PromusDriver::mixerLevel` retains the command value without changing PCM gain. The plain Nuked core has no card mixer. Maintainer-approved limitation, 2026-10-21: DOSBox does not emulate the PAS mixer, so neither the captures nor the maintainer reference include it; no gain curve is invented. FM/PCM balance will be revisited in M83 against DOSBox by ear. This is separate from the implemented per-bank OPL carrier levels/fade. |
| <a id="sv-cd-audio-party-options"></a> The four CD sound/text options travel with the saved party; New Clouds has Efx/Music on, Text off and Speech on. Efx off gates new FM/VOC and VOC status queries without stopping existing sound. | Calibrated DOS static/resource, high: party offsets +`0x0e`, +`0x0f`, +`0x5f8`, +`0x5f9`; New loads the Darkside template then overlays the 812-byte Clouds block (`0x20b7c..0x20be5`). Gates `0xdea9..0xdee6`; panel coupling `0x1ee00..0x1ee32`. Missing XEEN.CFG exits in EXE `0x11daf..0x11e1a` and DAT `0x3d320..0x3d33b`. [Option rules](rules/audio.md#party-options-and-new-or-load). **ScummVM:** pin stores global configuration, stops FM FX when disabling effects, and has only two panel audio toggles; master not rechecked. | `XeenPartyState::soundOptions` is saved in v18; panel toggles and silent sample/CD waits use those values. A missing XEEN.CFG refuses startup with a clear message, as DOS does (maintainer, 2026-10-21). The plan separately approves keeping text visible temporarily until M83; this does not change the established DOS default. |
| <a id="sv-cd-song-process-state"></a> Song names/buffers are process state. Same-name map music continues even when Music is off; fades do not block input. Music-on cancels a pending load and restarts whichever buffer is already loaded. | Calibrated DOS static, high: map `0x10276..0x104fc`, request `0x3790f..0x37955`, input `0xe0a3..0xe0c2`, deferred loader `0xe590..0xe5ee`, panel `0x1f07b..0x1f0a5`. Names change before buffers, so On during a pending fade/CD wait can leave new name/old buffer. Load with Music off sends no stop solely because that byte changed; the main Load return bypasses `0xb8b5`, and ordinary command wrapper `0xde65` is Music-gated. Wall kind 5 selects adjacent-table `outday1.m`; a missing song exits through `0x45c8a..0x45cfd`. [Names, title/Inn and transitions](rules/audio.md#song-names-and-asynchronous-changes). **ScummVM:** pin requires the same song to be playing for continuation, ignores the fade argument, does not defer loading for CD speech, and clears the current name on stop. Master not rechecked. | `XeenAudio` owns current name, loaded buffer and pending requests across sessions, with title/preload and map/location selection. Panel On deliberately cancels pending and restarts the loaded buffer; restoring an off byte sends no stop. Inn music is exposed and tested; the sign-in screen stays unsupported (maintainer, 2026-10-21). |
| Entrance captions advance one character per qualifying two-BIOS-tick interval, including long records; Efx off removes VOC-busy waits but leaves cursor/fixed scene waits. | Retained calibrated DOS evidence, high: renderer `0xdca3..0xdd52`, VOC gate `0xdea9..0xdecd`; Dwarf/Sphinx early exits `0x2b67b..0x2b6aa`, `0x2992d..0x29940`. [Lengths and timing](rules/audio.md#efx-voc-waits-and-entrance-captions), including the Dwarf video-erasure finding. | `XeenEntranceAnimation` advances one character per qualifying interval; Dwarf/Sphinx skip later sample phases in caption mode. Dwarf video frames 136..960 show the box; frame 961 erases it in the final redraw. Other guardians' erasure is not video-confirmed. The tagged entrance-cadence entry below owns the comparison with ScummVM. |
| A Run that empties the combat party lands on the current map's run position, indoors and outdoors alike, facing unchanged. | Static `WORLD/XEEN.DAT` analysis, 2026-10-19: the combat exit (`0x2eb11..0x2eb1f`) calls the run-location routine `0x1691f`, which copies the loaded map's run X/Y into the party position; nothing tests indoor or outdoor. The landing cell's automatic Event runs only when door text was up as combat began: combat entry saves and clears that flag (`0x2e62d..0x2e634`, set by the door-text opcodes at `0x26f7a`), and the exit's cell lookup and Event check (`0x2ec18..0x2ec37`) depend on it. The pin's `Party::moveToRunLocation` and `upDoorText` agree. | Implemented for every admitted map by `xeenRunDestination`. The door-text flag is not modelled, so Run says "not supported yet" when the landing cell has an automatic Event for the party's facing; if the party turns after an admitted Run, it lands without that Event, as DOS does when no door text was up. F3's `(10,12)` has none; Vertigo's `(18,4)` has an East-facing door-text Event, so there only Run facing East refuses. |
| Mr Wizard loads Vertigo facing East at its run position and zeroes the party's gems, without passing time. In combat it ends the fight first and then does the same. | Static `WORLD/XEEN.DAT` analysis, 2026-10-19, calibrated on the Run exit above, which calls the same routine `0x1691f`. The panel handler (`0x1f218..0x1f29b`) confirms with `MR_WIZARD` (`0x1f228`). Outside combat it plays FX 51, loads map 28 (`0x1f26f`), faces East (`0x1f282`) and moves to the run position (`0x1f285`); a flag set at startup when the other game is installed (`0x0b3d5`) selects Vertigo and East over map 29 and South. Both paths zero the 32-bit gems (`0x1f28a..0x1f290`) and return 2. In combat, the caller leaves combat on 2 (`0x2e98e..0x2e996`) and the exit tail repeats FX 51, map 28, East and the run position (`0x2ec3c..0x2ec7e`). The pin's combat `reloadMap` branch (`interface.cpp` ~1910) omits `moveToRunLocation`, so there the party keeps its coordinates on the new map. | Outside combat it is implemented in `XeenEncounterFlow::mrWizard`; FX 51 is requested before the transition. In combat the panel says "not supported yet". When combat support is added, follow DOS and move the party to the run position. |
| AlterEvent with no matching physical cell/direction/line silently changes nothing. | DOS scan `WORLD/XEEN.DAT` `0x28189..0x28356`, bounded static check, 2026-10-15, confirms the pinned `scripts.cpp::cmdAlterEvent` no-op. Original Darkside map 33 records at offsets 698 and 722 request absent lines 144 and 200. | `XeenWorld::applyAlterEvent` follows DOS generically on both sides. The map-116 record at offset 90 has a South-facing target and remains a resolved census context limit. |
| <a id="sv-misc-monster-loot"></a> Generated Misc monster loot is staged in its own treasure array. | DOS category-3 branch `0x2c4f5`, power/container/state writes `0x2c513`, `0x2c524`, `0x2c535`; generator and record layout independently agree. The pin instead copies an accessory into accessory treasure. [Full trace](rules/evidence.md#monster-treasure-staging). | **Implemented (M61 Part C).** `XeenMonsterTreasure.cpp` stages and delivers Misc through the existing shared rules, preserving draws and order. Pending Misc stays in the running program's staging across Run, Load and New Game and is never saved, as the [M81 persistence finding](#sv-loot-load-process-state) establishes. Since M81 Part C every monster record's drop, at levels 1-6, stages Misc this way. M61 Part D implements the separately confirmed DOS delivery rules below. **ScummVM:** differs. |
| Escaping a First Aid item's target prompt spends the charge and grants the user 1 SP without an initial SP debit or maximum-SP clamp. | DOS item caller `0x313ce..0x314ce`, power-4 branch `0x39e79`, First Aid prompt ID 26 at `0x2f3fa`, Escape `0x2f146`, shared refund `0x2f34b` and SP write `0x2f386`. [Full trace](rules/evidence.md#spell-and-item-control-flow). | **Implemented (M61 Part C)** in `XeenJourneyFlow.cpp::settleItemUse`, only for First Aid item cancellation. Rechecked read-only in the persistent Ghidra project, same image hash and offsets. M75 separately establishes eight more targeted item paths in the healing evidence; implementation support is unchanged by that analysis. **QoL candidate** for a future opt-in correction to the original exploit. |
| Every map load rebuilds each live monster at its MON HP and asleep, with its status cleared; the save keeps monster positions only. A wounded or woken monster is healed and asleep after the party leaves its map and returns, and after a game is loaded. | DOS map load `0x0f542` zeroes the 107-by-20-byte monster array (`0x0f5be..0x0f5c7`, helper `0x96a7`/`0x9683`), wake byte +4 and status +7 included, then sets HP from the MON record's word `+0x14` (`0x0fa99..0x0faa8`). The `.mob` writer stores only x/y (`0x266e3..0x2671c`). Game load copies the slot archive (`0x0ee4c`) and re-enters the main loop (`0x0b819..0x0b839`), which loads the current map (`0x0d086..0x0d08c`). Calibrated against the Spawn handler `0x27a6b..` (the pin's `cmdSpawn` field order) and recovery `0x20ce1`; high confidence, same as the pin. The maintainer's DOSBox check (wound a monster, leave its map and return; the same across a save and load) is pending. | **Implemented (M69 A2)**: map entry (`XeenWorld::reloadMapMonsters`) and game load (`XeenSaveState::restoreJourney`) refill HP and clear wake before the arrival view wakes what it sees; saves retain positions/lifecycle; `xeenRefillLoadedMonster` rebuilds HP and wake state. **M73 Part A:** status resets on every slot before the arrival view, including Disabled, Unresolved and Defeated slots. M73 Part B supplies the learned and item spell producers. |
| <a id="sv-monster-recovery"></a> Monster recovery clears status when `R(1, resource ID + 50) <= resource ID`; the pin instead uses `>= ID`. | DOS caller `0x20ca9..0x20cf1`, resource ID `0x20cd2`, helper compare/branch `0x2e1b2..0x2e1b4`, clear `0x20ce1`; high confidence. Equality succeeds in both. The 2026-10-17 bounded check scans the MOB count (`0x9c59`) in slot order, testing only status 0/13, with no HP, lifecycle or position filter; calibrated against status +7 and species +6. Recovery call `0x20167` precedes ranged dispatch `0x20171` (queue `0x3694f`, 36 entries `0x36958..0x369a6`). | **Implemented (M73 Part A)** in `XeenRegionalOpportunityCandidate::service`: resumable all-slot recovery before queued shots; Physical and Dragon Sleep never draw. M73 Part B applies these statuses through learned casts and item Use. **ScummVM:** differs. |
| <a id="sv-break-weapon"></a> Break Weapon marks the weapon broken and unequips it; the pin deliberately retains the frame. | DOS `0x2d52c..0x2d532`; [combat](rules/combat.md), high confidence. | **Implemented (M68 Part A)** in `xeenApplySpecialAttack`: equipped, nonempty weapons other than ID 34 lose their frame. **ScummVM:** differs. |
| <a id="sv-poison-first-save"></a> A nonphysical monster attack's first saving throw uses the Physical selector (`B(Luck) + L`, draw `1..v + 20`), not the element; the pin draws the element save twice. | DOS `0x2d2e4..0x2d2f7` pushes selector 0 before the type-specific protection switch; the element selector is first used by the loop at `0x2d35e`. 2026-10-15, high confidence. | **Implemented (M68 Part A)**, `XeenEnemyAttackCandidate::InitialSave`. This corrects the Poison attacks already playable in Vertigo (Slime, Doom Bug): their first save now draws `1..v + 20` instead of `1..poison resistance + 40`. **ScummVM:** differs. |
| <a id="sv-element-save-extra-draw"></a> The element save loop draws once on entry even when the damage is already zero, then halves and repeats while the save succeeds and the damage stays nonzero. | DOS `0x2d35e..0x2d374`: call, `je` exit, `sar di,1`, `or di,di`, `jne` back; the pin tests `damage > 0` before each draw. High confidence. | **Implemented (M68 Part A)**, `ElementSave`. One extra draw per nonphysical application whose initial save reduced the damage to zero. **ScummVM:** differs. |
| <a id="sv-eradicate-items"></a> Eradicate clears the equipped frames of weapons, armor and accessories (not miscellaneous) and skips a weapon slot holding ID 34 entirely; empty slots receive the broken bit too. | DOS helper `0x2bd6a..0x2bdbc`, called from `0x2d5e8`; the pin keeps every frame ("WORKAROUND") and breaks weapons below 34. High confidence. | **Implemented (M68 Part A)**. The unequipped items stay in the pack as broken; the AC and attack contributions drop immediately. **ScummVM:** differs. |
| <a id="sv-curse-item-slayer"></a> Curse Item leaves a weapon slot holding ID 34 untouched; every other slot of every category, empty ones included, gets the cursed bit. | DOS `0x2d671..0x2d6ab`; the pin clears that weapon's cursed bit (`_cursed = curse && id < 34`). High confidence. | **Implemented (M68 Part A)**. Not observable with current content (no Slayer Sword). **ScummVM:** differs. |
| <a id="sv-aging-wrap"></a> Aging increments the temporary-age byte without saturation: 255 wraps to 0. | DOS `0x2d592..0x2d59b`: `inc al` and store, no zero test; the pin increments an `int`. High confidence. | **Implemented (M68 Part A)**. Unreachable in ordinary play (255 Aging hits). **ScummVM:** differs. |
| Special attacks 1-4, 6 and 11 ("In Love" in the pin's enum) have no handler: the secondary save is drawn and nothing else happens. | DOS table `0x2d6d9` indexes `special - 5`; entry `0x2d6e5` (special 11) and the special-6 entry hold the HP-subtraction address; 1-4 fall outside the table. High confidence. | **Implemented (M68 Part A)**: Harpy and Sand Flower (special 11) do HP damage only, and no In Love condition is invented. |
| <a id="sv-resurrection-refund"></a> Resurrection cancellation refunds its own 125 SP and 20 gems. | DOS target prompt supplies ID 58 at `0x30bf3`; the pin supplies Raise Dead 56, refunding only 50 SP/10 gems. Retained calibrated evidence; M75 confirms item power 66 reaches the same handler at `0x3a0db`. [Full trace](rules/evidence.md#party-healing-spells). | Implemented in M75 Part B through the shared member prompt, including item power 66. **ScummVM:** differs; master not rechecked. |
| <a id="sv-suppress-disease-zero"></a> Suppress Disease still runs healing with amount zero when Disease is absent, so positive HP can clear Unconscious. | DOS zero branch `0x2f86c..0x2f872` joins the unconditional helper call `0x2f8a4`; the pin encloses that call in its nonzero-condition block. Both subtract 3 at counts 4+. Calibrated static check, 2026-10-17, high confidence. | `XeenLearnedSpellRules`, implemented in M75 Part A. **ScummVM:** differs; master not rechecked. |
| <a id="sv-revival-endurance-byte"></a> Raise Dead and Resurrection decrement permanent Endurance as a byte before applying the minimum: input 0 becomes 255; inputs 1/2 become 1. | DOS `0x30975..0x30986`, `0x30c56..0x30c6b`; the pin decrements an integer and clamps input 0 to 1. High-confidence static arithmetic, 2026-10-17; reachability of zero through ordinary play is not established. | `XeenLearnedSpellRules`, implemented in M75 Part B, including the explicitly settled zero edge case. **ScummVM:** differs; master not rechecked. |
| <a id="sv-create-food-wrap"></a> Create Food adds every active member to a 16-bit food counter, wrapping at 65536. | DOS `0x30642..0x30653`, count DS:`ed8c`, word addition at `0x30647`, no cap/filter. The pin adds to a wider unsigned in-memory field (its save still uses 16 bits). Calibrated static check, 2026-10-17, high confidence. | `XeenLearnedSpellRules`, implemented in M75 Part A. **ScummVM:** differs; master not rechecked. |
| Cancelling targeted healing item powers spends the charge and grants the spell's SP/gem refund without an initial SP/gem debit. | Powers 26/27/37/41/44/56/57/66 reach their learned handlers and own-ID prompts; caller charge/caster/dispatch `0x3145c..0x3149e`, shared refund `0x2f34b..0x2f393`. Power Cure uses user level. Food/Divine powers 51/73 have no member prompt. [All branch offsets](rules/evidence.md#party-healing-spells), calibrated 2026-10-17, high confidence. | Implemented in M75 Part A for powers 26/27/37/41/44/56, and no-prompt Food power 51, in `XeenJourneyFlow.cpp`. M75 Part B adds powers 57/66 and no-prompt Divine Intervention power 73 through the same paths. Resurrection's separate pin discrepancy is above. |
| Party cures run healing before clearing their named condition; Resurrection leaves HP unchanged. | DOS Cure Poison/Disease/Paralysis/Stone calls `0x30013`/`0x302d5`/`0x305bf`/`0x30859` precede clears; Resurrection calls `0x30c47` before clearing Eradicated at `0x30c50`. Divine Intervention preserves Eradicated and refills only other active members (`0x30d91..0x30dde`); Raise Dead clears Dead/Unconscious before healing 1 (`0x3094a..0x3096c`). Power Cure draws once after refusal (`0x2fb52..0x2fb95`). These match the pin. [Full calibrated rules and offsets](rules/evidence.md#party-healing-spells), high confidence. | Implemented in `XeenLearnedSpellRules`: cures and Power Cure in M75 Part A, Raise Dead, Resurrection and Divine Intervention in Part B. |
| Re-enabling a combat member after its skipped slot does not grant a late action that round. | DOS cursor/action-flag advance `0x2dd6d..0x2dd9f`, condition table `0x2de5e`, completion/reset `0x2ea0f..0x2ea47`; high-confidence static check calibrated against Run, 2026-10-17. Before its slot arrives, a re-enabled member can act this round. A final disabled tail ends the round without a further player action. HP alone does not deny admission: condition-only defeat gate `0x24f29..0x24f8f`, command dispatch `0x2e808..0x2e824`, Block/Run `0x2e205`/`0x2e1c0`. The pin agrees. [Combat rule](rules/combat.md#initiative-block-run-and-monster-recovery). | `XeenCombat.cpp::selectNext` marks skipped disabled participating party slots done; implemented in M75 Part A, including existing Awaken and Cure Paralysis. Monster slots retain their existing handling. M75 Part B removes the old HP/Unconscious consistency gate in `XeenJourneyRules`, so revival states remain admitted in exploration and saves too. |
| <a id="sv-moon-ray-preparation"></a> Moon Ray prepares all member healing before the retained portrait sequence. | Calibrated DOS static, 2026-10-22, high: DAT `0x30aa1..0x30adf` interleaves cue 30, U(1,30), and healing per party member; healing `0x2ee3a..0x2eed6` calls sparkle `0x21087..0x21185`, including shared cue 20 and frame waits. **ScummVM:** the pin also interleaves each draw and healing; master not rechecked. | Existing preparation gap in `XeenMoonRayHealing` and its combat/exploration publication, recorded without changing it at the maintainer's request (2026-10-22), not an approved deviation. C2 queues each member's cue through the existing portrait owner so audible 30/20 and animation ordering is retained; RNG and healing remain batched. |
| <a id="sv-item-incinerate-selectors"></a> Item Incinerate and learned Incinerate prepare the same 250 Fire/single damage but use different sound/projectile selectors. | DOS item handler `0x30b58..0x30b7a` uses 13/2; learned handler `0x30f0d..0x30f2f` uses 22/1. Reconfirmed 2026-10-17 against the pinned `castItemSpell` power-64 mapping to the shared learned handler; high confidence. [Dispatch evidence](rules/evidence.md#item-power-dispatch-and-variants). | `xeenSpellProjectile` selects learned projectile 1 and item-power-64 projectile 2; `xeenSpellLaunchFmEffect` selects 22 and 13 respectively. The selectors' actual presentation remains uncalibrated. **ScummVM:** differs; master not rechecked. |
| <a id="sv-frostbite-initial-fx"></a> Learned Frostbite has no initial FX8 call; the pin adds one. | Calibrated DOS static analysis, 2026-10-17: complete handler `0x30eac..0x30ec6` sets 35 Cold/Single, calls projectile selector 8 and returns, without calling sound helper `0xdece`. Pinned `Spells::frostbite` calls `playFX(8)` first. Item power 33 instead matches `frostbite2`: sound 15/projectile 9 at `0x2fc77..0x2fc99`. High confidence for calls; no DOSBox audio observation. | `xeenSpellProjectile` selects learned projectile 8 and item-power-33 projectile 9; `xeenSpellLaunchFmEffect` emits no learned launch cue and selects 15 for the item. Shared impact sounds remain; this is not a claim of a silent cast. [Magic rules](rules/magic.md#item-power-lookup). **ScummVM:** differs; master not rechecked. |
| <a id="sv-hit-total-unsigned"></a> A negative final player hit total can hit in DOS because the comparison treats it as an unsigned word. Poison Volley uses this shared accuracy helper too. | Calibrated DOS static analysis, 2026-10-17: missile caller `0x2cb6b..0x2cb7f`; Cursed subtraction `0x2df5f..0x2df69`; status bonus `0x2df75..0x2df7c`; unsigned comparison with AC+10 at `0x2dfb3..0x2dfc4`. A final -2 is 65534 and beats AC5+10. Pinned `Combat::hitMonster` compares signed `int chance` and misses. High confidence from reached instructions, not a play observation. | **Implemented** in `XeenPhysicalPlayerCandidate` for melee and Shoot (M74 Part B), and Poison Volley (M74 Part C), shared by exploration and combat casting. Its shot count, 10 base damage plus missile elemental contribution, and halving save otherwise follow the traced missile branch. [Magic rules](rules/magic.md#damage-preparation-and-resolution). **ScummVM:** differs; master not rechecked. |
| <a id="sv-holy-word-miss"></a> Holy Word on a non-undead target enters zero-damage/miss handling in DOS; the pin skips damage handling. | Calibrated DOS static analysis, 2026-10-17: zero preparation `0x30bcb`; species check `0x2cf4c..0x2cf50` branches through `0x2cf91` to `0x2cf9c`, then zero-damage test `0x2be33..0x2be3b` reaches miss calls `0x2c16c..0x2c181`. Pinned `Combat::attack` calls `attack2` only for Undead. High confidence for control flow; audible result unplayed. | **Implemented (M74 Part B)** in `XeenSpellDamageCandidate`; M82 uses the shared FX6 miss cue. Neither path damages, wakes or draws a damage-stage save for the incompatible target; the earlier magic-resistance gate still applies. [Magic rules](rules/magic.md#monster-specific-magic). **ScummVM:** differs; master not rechecked. |
| M74 ordinary preparation and special saves retain the DOS rules, including random-type draws before targeting and saves that can increase a small Finger of Death/Mass Distortion amount to 5. | Calibrated static analysis, 2026-10-17: all 23 learned entries match prepared fields/costs; scalar draws `0x30263`, `0x30a14`, `0x30043`, `0x2fc0f`; type draws `0x30b85`, `0x30a5c`; special dispatch `0x2cfab`, save dispatch `0x2c59a`, save helper `0x2e19b`. [Rules and boundaries](rules/magic.md#damage-preparation-and-resolution); external per-spell report `D:/MModern-re/m74-damage-spells/findings.md`. Calibration reproduced Pain, elemental zero and item dispatch on the [verified image](rules/evidence.md#exact-baseline). | The 18 ordinary spells use `XeenSpellDamagePreparation` and `XeenSpellDamageCandidate`; the four special damage spells use the same candidate with current target HP (M74 Part B); Poison Volley uses the shared party volley and physical accuracy candidate (M74 Part C). No empty-target refund: learned debit/dispatch `0x3327b..0x332c7`; Moon Ray still heals after delivery `0x30aa1..0x30ad4`. |
| All-range spell projectiles cover at most four groups of three positions; each target completes death/treasure generation before the next, with party delivery after the projectile. Clouds resource ID 89 refuses spell damage before the damage save. | DOS static, high confidence, 2026-10-17: four-tier iteration `0x1d98a..0x1de91`; outdoor blocking values 1/3/6/7/9/10/12 and indoor no-pass comparisons `0x1db3e..0x1ddc4`; synchronous damage/death return `0x2cf94..0x2cf9f`, `0x2c112..0x2c591`; delivery `0x1debf`. ID89 refusal `0x2bdf8..0x2be33` applies to nonzero attack-range arguments, including Single spells and Poison Volley. [Magic rules](rules/magic.md#damage-preparation-and-resolution). | Implemented through the shared spell candidate, including the ID89 refusal (M74 Part B). Matches the pin for these rules; four groups is a finite bound, not unlimited travel until a wall. This does not certify the boss's separate melee/Slayer Sword behavior or override recorded treasure differences. |
| <a id="sv-event-sparkles"></a> The Event give dispatch sparkles the recipient for sex, race and class (3-5), SP, AC and level bonuses (9-11), gold and gems (34/35) and food (65), besides the pin's sparkle modes (HP 8, skills 13, awards 15 except 8, XP 16, 25, 37-64, 77, 78, 81). | DOS give table `0x23331` dispatched at `0x22689`; these handlers reach the shared tail `0x23278..0x2327f`, game flags (20) and world/quest flags (103/104) leave through `0x23308` without one; 2026-10-14, high confidence. Mode 19 sparkles only when the spell is in the class book (`0x2280b..0x22869`, rechecked 2026-10-16). The pin's `giveTake` omits spellFX for 3-5, 9-11, 34, 35 and 65. | **Implemented (M70 Part B)** in `xeenEventGive`, once per member per Event run; GiveMulti currency still goes to treasure without a sparkle. **ScummVM:** differs. |
| A Knight, Robber, Ninja or Barbarian's spell test, grant or removal uses the spell book of whichever class the program computed last. | Static analysis, 2026-10-16: the class switches at `0x21729`, `0x22182` and `0x2280b` leave the global `0xf604` unchanged for these classes. Pinned `getSpellsCategory` returns invalid and skips. | **Not supported yet** (M70 Part B, `xeenEventProbe`/`xeenEventTake`/`xeenEventGive` mode 19): MMModern does not own that global and refuses. A non-caster's learned flags are never shown. |
| <a id="sv-makeitem-draws"></a> `makeItem` draws category and subcategory from 1, not 0. It also consumes a material selector at level 7, tests weapon effectiveness with `R(1,20)`, and gives Misc items `R(1,8)+2` charges. The pin uses 0-based category/subcategory and effectiveness rolls, skips the level-7 selector, and stores the Misc roll without 2. | Calibrated DOS static analysis, 2026-10-16, `WORLD/XEEN.DAT` SHA-256 `8bf78b48...`: helper `0x45b9a..0x45bb4` computes inclusive min..max, as at the known Event currency draw `0x23770..0x23791`. `makeItem` `0x25cbc..0x25ce3` draws category `R(1,100)` and subcategory `R(1,100)` or `R(1,80)` at level 6+; comparisons at `0x25d45..0x25e38` retain the same thresholds. Selector `0x26165..0x26187` is drawn before the level-7 material override; effectiveness `0x262ae..0x262c6`; Misc charges `0x2624b..0x2625a`. High confidence. | **Implemented** in the shared `XeenItemCandidate` and merchant support validation. Event GiveMulti, display cases and merchant stock use it. Combat reloads the monster's drop-level field (`+0x2d`) at `0x2c3e2` and passes it to makeItem at `0x2c3e7` (far target `1b98:4c68`, file `0x25ca8`); the chance check at `0x2c38b..0x2c39a` precedes generation and capacity checks follow it. `XeenMonsterDropCandidate` shares the generator for every record's drop field, levels 1-6 (M81), retaining chance, accounting, staging and delivery. Stored merchant stock with Misc charges 1-10 remains valid without regeneration; new generation yields 3-10. **ScummVM:** differs. |
| Town Portal's item combat branch shows its prohibited-use notice, then reaches the spell handler. | DOS `0x3a058..0x3a075`; the pin returns after the notice. [M76 downstream check](#sv-town-portal-item-combat) traces the dialog and load. | Static travel reachability is settled by M76; DOSBox presentation remains unobserved. |
| <a id="sv-outdoor-monster-terrain"></a> Outdoors, monsters never enter a cell whose middle value is 1, 7, 9, 10 or 12, flyers included, whatever the map's no-pass threshold; the pin compares these middles with the threshold. | DOS `canMonsterMove` `0x1fc91`: jump table `0x1fda3` sends these middles straight to the refusal at `0x1fd8b`; threshold byte calibrated as described in [combat](rules/combat.md#monster-movement-terrain). 2026-10-15, high confidence. Observable only on Clouds outdoor map 2 (no-pass 6, 17 middle-1 cells); map 23 has no-pass 0, where both agree. Maintainer approved following DOS, 2026-10-15. | **Implemented (M68 Part B)**, `xeenOutdoorMonsterTerrain`. **ScummVM:** differs. |
| <a id="sv-monster-wall-threshold"></a> A monster's wall threshold is strict: a wall passes only below the map's no-pass value (the player's own rule), indoors and for outdoor middle 15; the pin admits a wall equal to no-pass. | DOS `0x1fd8f..0x1fd9b`: `mov al,[0xc0fc]`, `cmp ax,dx`, `jle` refuses. 2026-10-15, high confidence. Original-data scan: no-pass-valued (7) interior walls on Clouds maps 51, 55, 59 and Darkside maps 53, 57, 61, 65, 69, 113, 117, 163, 164, 213 now block monsters. Vertigo's only such wall is an off-map edge, so the playable route is unchanged. Maintainer approved following DOS, 2026-10-15. | **Implemented (M68 Part B)**, `xeenMonsterPassesWall`, shared by walkers and flyers. **ScummVM:** differs. |

### M76 spell findings

The following are **DOS static, high confidence (2026-10-18)**, against pin
`6814ee9b`; master was not rechecked. They establish original rules, not
implemented M76 support or milestone acceptance. Calibration and the external
finding ledger are in [M76 evidence](rules/evidence.md#m76-spells).

- <a id="sv-holy-bonus-damage"></a> **Holy Bonus contributes weapon damage.**
  The shared weapon calculation adds Heroism to hit at `0x2e0c8..0x2e0d2`
  and Holy Bonus to damage at `0x2e0d6..0x2e0db`, before Adventurer triples
  damage. Melee and Shoot use it; a spell's prepared nonzero damage replaces
  the weapon base. **ScummVM:** differs; its weapon routine omits Holy Bonus.
- <a id="sv-day-spell-counters"></a> **Day spells copy the caster's level byte
  into flags and counts.** Day of Protection writes Walk on Water as that
  byte (`0x30e34`); Day of Sorcery does the same for Clairvoyance, Wizard Eye
  and Levitate (`0x30e6d..0x30e73`). Light and party elemental words receive
  the zero-extended low byte too. All use current level including temporary
  level (`0x21fc9`). **ScummVM:** differs; its flag assignments are true and
  Levitate is 1; its level assignments retain the full integer level.
- <a id="sv-spell-effect-widths"></a> **Spell effect writes retain DOS field
  widths.** Bless/Heroism/Holy Bonus/Power Shield store only the low level
  byte (`0x2f8cd`, `0x2fc65`, `0x2fb0c`, `0x2ffdb`); Light increments a
  word and wraps 65535 to 0 (`0x2f39a`). **ScummVM:** differs; its live
  integer fields retain larger values, although save serialization uses
  byte/word fields. Reachability of these extremes in ordinary play was
  not established; the static writes are high confidence.
- <a id="sv-clairvoyance-right-frames"></a> **The right statue's hint-0 cycle
  is frames 8..11.** DOS subtracts 4 after its modulo-4-plus-12 calculation
  (`0x1badd..0x1bafd`). **ScummVM:** differs; subtracting 3 gives 9..12.
  These are numeric frame identities, not a DOSBox animation capture.
- <a id="sv-enchant-caster-and-copy"></a> **Enchant uses the caster's level
  and copies even an empty generated enchantment.** The global caster is
  passed to current-level helper at `0x31c91..0x31c99`, before the level
  draw/cap. Eligible weapon/armor branches copy material/state and call the
  success sound selector without testing the generated properties
  (`0x31cfa..0x31da3`). **ScummVM:** differs; it uses the initially selected
  recipient's level and adds an error on a zero-property result. Generator
  draw differences already recorded for M71 also apply to this shared call.
- <a id="sv-item-spell-special-id"></a> **Enchant and Item to Gold exclude
  weapon ID exactly 34.** DOS equality checks are `0x31cef..0x31cf7` and
  `0x31df8..0x31dfe`; higher weapon IDs are not refused by these tests.
  **ScummVM:** differs; both tests reject IDs at least 34. Quest counters
  themselves are separate from selectable inventory records.
- <a id="sv-recharge-broken"></a> **Recharge fills an eligible broken item
  to 63 charges.** The cap comparison masks the state with `0xbf`
  (`0x31c0d..0x31c1f`), which keeps broken bit `0x80`, so a broken item
  always takes the cap branch. Cursed bit `0x40` is dropped for that
  comparison, so a cursed item recharges normally; both flags stay in the
  record. Bit values: Curse Item sets `0x40` (`0x2d685`), Break Weapon
  `0x80` (`0x2d52c`). One 1..6 draw still occurs; no break/destruction
  draw occurs. **DOS play (2026-10-18):** on a save whose item state bytes
  were edited, a broken charged item went to 63 and stayed broken, a cursed
  one gained 1..6 and stayed cursed, and Enchant refused a cursed armor
  with no material.
  **ScummVM:** differs; it compares the six-bit charge count alone.
- <a id="sv-town-portal-item-combat"></a> **Town Portal items continue after
  the combat refusal notice.** The branch `0x3a058..0x3a075` reaches the
  ordinary map guard, town dialog and load (`0x30654..0x30819`). With the
  map restriction clear, a selection can load the town even in combat.
  Cancellation does not refund the charge. **ScummVM:** differs; it returns
  after the notice. Static reachability is settled; the anomalous route's
  complete DOSBox presentation remains unobserved. **M77 Part 0** traced the
  rest of the route: the handler restores the saved combat mode after its
  dialog (`0x30762`), the item routine finishes as any Use (monster response,
  turn cursor cleared, `0x314c9..0x314dd`), the Items dialog closes, and the
  combat loop ends once its attack-slot test finds no attackers
  (`0x2ea99..0x2eaf0`), which the scene setup decides after the load
  ([evidence](rules/evidence.md#open-questions-from-this-check)). Maintainer
  decision, 2026-10-19: M77 Part A ends combat at the first attacker
  re-evaluation after the load; the exact DOS presentation in between stays
  unobserved. Implemented in M77 Part A in `XeenCombat::settleCast` and
  `XeenEncounterFlow::useCombatItem`, through the existing map transition.
  M82 C2 requests FX21 at the engaged notice (`0x2f1d3`), even when the
  later town dialog is cancelled; successful travel retains FX51. The
  request uses `XeenEventFlow::beginTravelDialog` and the existing audio owner.
- <a id="sv-rest-cell-flag"></a> **DOS ordinary Rest honors cell bit 0x40.**
  Lookup stores attribute `&0x40` in DS:`f488` (`0x11fe7..0x11fec`), and
  Rest checks it before the primary map flag (`0x1e50d..0x1e521`). Mode 12
  bypasses both checks for Super Shelter. **ScummVM:** differs; its cell
  lookup masks the cell flags with map restriction `0x4000`, which cannot
  occur in the byte. The CD mainland 23 contains such cells; Vertigo's four
  component maps do not. MMModern follows DOS in `XeenRestFlow::beginRest`:
  it samples the current cell, including resident neighbors, and refuses on
  attribute bit `0x40`. Super Shelter bypasses both ordinary refusals.

- **Item travel arrival timing is only partly confirmed in DOS.** Item
  Jump, Etherealize, Time Distortion, Teleport, Lloyd's Beacon and Town
  Portal return through the existing
  item-use monster response, without an immediate terrain-arrival call.
  The calibrated call sites are `0x39e4b`, `0x0d411` and `0x0d5ba`; see
  [M76 travel evidence](rules/evidence.md#m76-spells). The exact timing of
  automatic destination Events after an item cast is not confirmed in DOS.
  `XeenEncounterFlow::settleItemUse/serviceItemUse` retains item Jump's
  destination-Event scheduling, following the pin within this evidence limit.

The same check also settles these rules that agree with the pin, within the
listed boundaries:

- The 22 spell costs, learned combat list and item branches other than Portal
  agree; admitted handler failures spend resources, while explicit member/
  element cancellation refunds use the supplied spell ID. Late Items/travel
  cancellation does not refund. [Handler and refund offsets](rules/evidence.md#handlers-costs-and-admission).
- Bless's AC addition has no condition test. Heroism applies in the shared
  weapon accuracy path, including Shoot/Poison Volley. Shield comes after
  damage saves and before the secondary save, with a zero clamp. The prior
  M68/M72 damage-sequence differences remain. [Consumer locators](rules/evidence.md#consumer-and-dialog-locators).
- Rest resets all temporary buffs before food/recovery, so its later four
  clears are redundant. Super Shelter makes one 480-minute call, bypasses
  ordinary monster interruption opportunities, still asks about dying risk,
  and shares dream/reset/food/arrival ordering. Levitate has ended before
  arrival. [Rest sequence](rules/time-and-conditions.md#rest).
- Light increments/marks stepped; a stepped drain check decrements before
  darkness. No drain cells occur on the five open map components. Levitate
  has no per-step decrement and protects cloud, not unconditional falls/sky;
  Walk on Water handles shallow/deep movement without extra arrival immunity.
  The requested buff/element/Levitate border positions and per-call cycles
  agree. Detect/Identify Monster screens and their empty-target costs agree.
  [Senses and screens](rules/magic.md#border-and-information-screens).
- Recharge has no break/destruction draw. Enchant's eligibility tests occur
  after its initial draw and its generator is shared with the existing DOS
  generation rules. Item to Gold uses full DOS item value, no additional
  confirmation, and no equipped/broken/cursed refusal. The differences above
  qualify these agreements. [Item actions](rules/magic.md#item-spell-actions).
- Etherealize, Time Distortion and Teleport mutate position without directly
  loading; Beacon reloads only across map/side changes; Portal always loads.
  Facing is preserved on Clouds. Restrictions, dialog cancellation, Beacon's
  per-character default/Set/Return fields and five town run destinations are
  recorded with offsets. [Travel](rules/magic.md#travel-handlers-and-callers).
  Exact post-item automatic-Event timing remains Unknown; the normal learned
  terrain suffix retains the earlier Jump evidence.

### M77 combat items, Quests, Awards and skill indicator findings

Calibrated **DOS static** findings of 2026-10-18 (M77 Part 0, read-only
Capstone analysis of the installed `WORLD/XEEN.DAT`; anchors in
[M77 evidence](rules/evidence.md#m77-combat-items-quests-awards-and-skill-indicators)).
They settle rules for M77 Parts A-C: the combat item paths are implemented
in Part A, Quests and Awards in Part B and the indicators in Part C.

- <a id="sv-discard-xeen-slayer"></a> **Discard refuses the Xeen Slayer
  Sword.** The Discard routine tests weapon ID exactly 34 after the cursed
  test and shows the fixed text "Don't you think you'll need the Xeen Slayer
  to kill Lord Xeen!" (`0x31061..0x3106f`, text `0x54316`); declining the
  confirmation returns false. **ScummVM:** differs; `discardItem` has no
  weapon test and returns true either way. Implemented in M77 Part A in
  `xeenDiscardItems` and `XeenCharacterDialogFlow.cpp`, with the original
  resource text at `0x54316`; applies in exploration and combat.
- <a id="sv-danger-sense-frame"></a> **Danger Sense animates from its own
  counter, in phase with Spot Secret Doors.** DOS keeps three border counters
  (`0x974d` Levitate, `0x974e` Spot Doors, `0x974f` Danger Sense), advances
  each by one modulo 12 per border assembly and zeroes them together
  (`0x1ba1e..0x1baab`, `0x1c402..0x1c410`, `0x1d4c4..0x1d4d2`). **ScummVM:**
  differs; `assembleBorder` draws Danger Sense with the Spot Doors counter
  after incrementing it, one frame ahead. Frame identities, not a DOSBox
  animation capture. MMModern follows DOS in M77 Part C
  (`CloudsMapComposer::drawInterfaceLayers`).
- <a id="sv-quest-award-scroll-keys"></a> **Quests and Awards scroll with the
  arrow keys and buttons, not with U and D.** The Quests key table
  (`0x3402e`) holds Escape, I, Q, A, Home, End, Page Up, Page Down and the
  Up/Down codes `0xf2`/`0xf3`; the Awards loop (`0x36fdd..0x37019`) handles
  Escape, `0xf2` and `0xf3`; the key translation table (`0x456e8`) maps Up
  and keypad 8 to `0xf2` and Down and keypad 2 to `0xf3`, and the two
  dialogs' Up/Down buttons carry the same codes. **ScummVM:** differs; its
  Quests dialog also accepts U and D, and its Awards dialog accepts only U
  and D (its buttons are `KEYCODE_u`/`KEYCODE_d`). MMModern follows DOS in
  M77 Part B (`XeenJournalView.cpp`: `xeenJournalInput`, `XeenJournalUi::key`).
- **Auto Notes flag 128 reads quest flag 0.** DOS's loop at
  `0x33d8c..0x33f3c` reads one bit beyond the 128-bit world array, into the
  following quest array; no Clouds script sets that bit. The display adapter
  in `XeenJournalView.cpp` preserves this alias from the existing v15 state.
  See [M77 evidence](rules/evidence.md#m77-combat-items-quests-awards-and-skill-indicators).
- **Spot Secret Doors keeps its last indoor evaluation outdoors.** The
  thin-wall flag `0xf496` is written only by the indoor scene setup
  (`0x12303..0x1231e`); outdoor scenes never clear it. The pin's `_thinWall`
  behaves the same. Static only, not observed in DOSBox; see the
  [open question](rules/evidence.md#open-questions-from-this-check). Follow
  the code path unless the maintainer decides otherwise. M77 Part C keeps it:
  only `XeenIndoorScene::build` writes the scene presentation's `thinWall`.
- **Combat Items dialog rules match the pin** apart from the Discard test
  above: Equip, Remove, Discard, transfer and Quest are free actions in
  combat; only a successful Use ends the turn; the main-screen Use dialog
  ignores F1-F6; transfers from the character sheet in combat use the combat
  party; Use from the sheet in combat is refused with the original text
  ([rules](rules/items.md#the-items-dialog-in-combat)).

## Presentation findings (2026-10-13)

These are calibrated **DOS static** findings, not implemented corrections or
DOSBox observations. [M62's proposed plan](archive/milestone-62-plan.md) separates
resource/layout work from M63 timing and publication work. Evidence is the
external [UI report](D:/MModern-re/ui-presentation-20261013/findings.md) and
[claim index](D:/MModern-re/ui-presentation-20261013/evidence-index.md), using
DAT SHA256 `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`
and pin `6814ee9ba54582f5b5adcffab49efbbd8f589edd`. The known six-phase cursor
calibrates the analysis (`0x3f3be`, `0x43f08..0x43f43`); offsets below are DAT
file offsets. Exact runtime clocks, pixels and post-wait input remain bounded
by the report's capture requirements.

| Original behavior and reference difference | Evidence | Current implementation boundary |
| --- | --- | --- |
| <a id="sv-scenery-terminal-frame"></a> Ordinary scenery includes the directional terminal frame; the pin excludes it. | `0x10d21..0x10d55`, indexed initial map 28/23 placements and resource headers | `XeenObjectVisualResolver` keeps the end frame in the cycle: frames start..end, static when end <= start (indexed Vertigo 008/009 and map 23 009/102/105/109 are tested against the original `clouds.dat`). Static views, facing flips and the shared phase source are unchanged. **ScummVM:** differs. |
| draw3d updates water/UI before rendering, but advances shared scenery/monster phases afterward; repainting does not consume either update. | Calibrated UI findings, `0x10bda..0x10bf4`, `0x107b6`; two-counter completion `0x11873..0x1187b` | M63 Part A splits the existing `XeenScenePresentation` pre-/post-draw work. `XeenEventFlow` retains the chosen presentation values for reconstruction; indoor/outdoor scene commands read that retained view on a pure refresh. Failed composition does not repeat the water update or consume post-draw animation. |
| <a id="sv-combat-name-colors"></a> Combat names use color 15 at full HP, otherwise 9 at or above floor(maxHP/4), 32 below; the pin uses half HP. | `0x2e211..0x2e272`, templates `0x54001..0x54048`; maintainer long-play evidence, 2026-10-13, confirms wounded names turn yellow (selector 9). | `XeenDialogView::drawXeenCombatPanel`, consumed by `XeenEventFlow`. The quarter-health boundary follows calibrated DOS program evidence; no health-color capture was supplied. **ScummVM:** differs. |
| <a id="sv-whowill-refusal"></a> WhoWill shows the incapacitated-member refusal and returns to selection; the pin silently retries. Intermediate NPC Escape/right click advances a page, while final confirmation declines. | `0x2507a..0x252f6`, refusal `0x50aae`; door/grate action pointers `0x5023c..0x5024c`, selection `0x250ff..0x25109`, callers `0x0d25b..0x0d2b4`; NPC `0x256cd..0x25ca7` | `XeenEventFlow` and door/grate callers retain the window-6 refusal and reselection; `XeenEventPresenter` uses the original window-11 writer continuation. Supported NPC modes remain 1/2; final mode-0 confirmation is evidence for future admission only. **ScummVM:** differs. |
| Combat row clicks use `[239,312)` × `[27,37)`, `[37,47)`, `[47,57)`; Escape/right click leaves combat active. | Combat button tables `0x4f20c`, `0x4f236`, `0x4f24a`, `0x4f25e`, entries 17..19, installed at `0x2e686` through `0x38904`; lower-inclusive/upper-exclusive decoder `0x0e32d..0x0e396`, calibrated against YesNo's known 24×20 records at `0x4f4b0`. Dispatch table `0x2ecad..0x2ecd6` omits Escape; right maps to Escape at `0x0e0dc`. Maintainer capture `monsters test.mkv`, 2026-10-13 (SHA256 `3846733d16461ad0ba0be8305df29cf631b50bff45a80c27ad9bcd97fea17abc`), corroborates row selection around 3/4/5 and 8/9 seconds. | `XeenMainScreenInput`, unchanged rectangles; `XeenEventFlow::handlesEscape` and SDL caller routing. Exact bounds are settled by calibrated program evidence; pixels alone do not identify the pressed key or mouse button. |
| <a id="sv-service-speaking-animation"></a> Clouds Smith replaces base art with blck2 frames 5..7 while speaking; Training overlays trng2 8..12 while speaking. The pin omits Smith's branch and reverses Training's sound condition. | `0x1c03b..0x1c12c`, `0x1c309..0x1c390`; sound status calibrated against dream/death loops | `XeenServiceAnimation` and existing Service/Flow continuations use VOC-header intervals, three-tick input-interruptible waits and retained Journey RNG choices. Bank 7..11, Temple 2..4 and Training 8..12 overlay base art; Smith 5..7 replaces it while its logical index advances. Guild/Tavern have no Clouds speaking overlay. Greeting/action/refusal replace the active interval; Tavern goodbye holds the last screen. Merchant generation stays at departure, after eligible speaking requests. **ScummVM:** differs. |
| <a id="sv-cd-speech-timing"></a> NPC face activity follows MSCDEX busy, not the pin's word countdown. Normal CD cues parse MMSSFF and request end minus start plus 25 sectors; the pin treats the last digits as centiseconds. | `0x1c417..0x1c45c`, `0x3d5fd`, `0x3daec..0x3dbbf`, command packet `0x3d9b4..0x3d9e8`; maintainer Gunther capture identifies track 19/cue 370..2161, duration 18.213333 s. [Measurements and limits](rules/evidence.md#death-and-cd-speech-captures-2026-10-13). | `XeenEventFlow` retains the exact requested interval from the original playback call and samples it at three-tick polls; pages do not restart it. Each fresh NPC invocation restarts a valid cue; mode 2 starts it and returns without stopping, ordinary final dismissal stops it, and an all-zero cue is a no-op (`0x256ec`, `0x25ae8..0x25af3`, `0x25aa8`, `0x25c94`, `0x3da98..0x3daa7`). WhoWill retains window 11; Event completion closes it and ends visible face activity (`0x251a8..0x251bf`, `0x28e92`, `0x0dd80`). Original face choices consume the existing Journey RNG once per eligible call. Maintainer approved this boundary on 2026-10-13: additional driver/capture latency could not be isolated, so no fixed startup delay or instrumented capture is required. **ScummVM:** differs. |
| <a id="sv-treasure-waits"></a> Treasure blank/found waits are 1/10 BIOS ticks, not the pin's 5/8; item transfer precedes its own message and currency follows final acknowledgment. | `0x24128..0x248f2`, `0x264c5..0x264ed`; quest name table `[0x4fd5c,0x4fde8)`, raw-ID indexing `0x245f8..0x24610` and star `0x2476e` | Existing Event/Encounter/Flow owners publish each transfer before its blank hold, sort before its found hold, and credit currency only at the separate final acknowledgment. Input interrupts a hold without becoming the final acknowledgment. Quest counts are already granted; their retained display records never grant again or enter ordinary inventory. M61 full-pack mechanics remain shared. **ScummVM:** differs. |
| Clouds death uses fire pairs 0..35, rising death pairs at the DOS Y table, final 34/35 and death1 frames 0..21; stored death1 frame 22 is not executed. | Calibrated UI report, DAT routine `0x0b02f..0x0b39a`, Y table `0x47a44`; original CD sprite headers corroborate capacities; maintainer `Morte.mkv`. | The existing `XeenEventFlow` publishes the fire/final/laugh sequence, then closing hands and fade-out. `Application` retires the gameplay owner and returns to World title idle, as the capture shows. The closing-hands poll can end early (`0x0afbd..0x0afc4`); fire and laughter have no local skip. [Capture measurements](rules/evidence.md#death-and-cd-speech-captures-2026-10-13). |
| <a id="sv-damage-waits"></a> Both monster and Event/trap portrait damage wait 2 ticks; the pin's generic path waits 5. Recipient sparkles complete synchronously, not concurrently. | `0x23f73..0x23f7d`, `0x2d6c4..0x2d6ce`, `0x21087..0x21186` | `XeenEventFlow` retains recipient progress and chosen results, publishes each overlay/injury in order, and owns the acquired-frame hold. `XeenScenePresentation` composes the retained effect; host refresh does not advance it. Generic multi-recipient damage shares its final hold. A member already sparkled in the same Event run is skipped by the helper's latch (`0x210bc`/`0x2117f`, cleared at Event start/end `0x267a6`/`0x291e0`); M64 corrected M63's per-grant repeat (`XeenEventInterpreter.cpp`, maintainer capture `recompensa gunther.mkv`). Window-11 sparkles freeze NPC face activity because that branch omits draw3d (`0x2110f..0x21169`). **ScummVM:** differs. |
| <a id="sv-indoor-minimap-edges"></a> The indoor minimap's partial left column and top row cover all seven rows and columns, bottom row first; a left-column wall of type 9 uses tile frame 20; the direction arrow is drawn after the party cell's walls and before cell (4,4). The pin draws one left and one top tile (its loop ends after one pass, and its first left tile takes the top row's map coordinate), uses frame 24 for type 9 there, and draws the arrow before the party cell's walls. | Static `WORLD/XEEN.DAT` analysis, 2026-10-19 (D:/MModern-re/minimap-20261019): indoor branch `0x13d6b..0x1438c`, edge loop `0x13fcc..0x1411c` (seven 10-pixel steps, left lookup `(x-4, y-3+i)` at `(222, 60-8i)`), left table `cs:0x1308` (frame 20 for type 9; base calibrated by its 15 jump targets), arrow test `0x1413c..0x14169`; main tables `cs:0x12cc`/`cs:0x12ae`, base, surface and unexplored passes and the maze lookup (`0x11d40`) match the pin. Calibrated by the minimap's known origin (237,12), 10x8 tiles and the far-call mapping to this routine (`0x1161d`). High confidence. Prompted by the maintainer's 2026-10-19 DOSBox comparison. | `XeenMapView` indoor minimap. The full Map dialog's edge loop is a separate DOS routine and is unchanged. **ScummVM:** differs. |
| Indoor maps draw a cell whose raw wall word is 0x8888 (Vertigo's solid building blocks) as a missing cell: no ground or surface, its wall strips from the shifted word, then the unexplored tile over them, so blocks look solid. The original lookup returns 0x8888 for a missing cell and the map routines compare against it; the pin does the same. | Static `WORLD/XEEN.DAT` analysis, 2026-10-19: lookup `0x11d40` returns 0x8888 before shifting for a missing cell; minimap ground `0x13db2`, surface `0x13e30` and unexplored `0x142fd` compare the unshifted word; maintainer DOSBox screenshot of Vertigo, 2026-10-19. Pinned `INVALID_CELL` tests in `interface_minimap.cpp` and `dialogs_map.cpp`. | `XeenMapView` minimap and full Map. |
| A closing marble sprite can expand a compressed row to 321 pixels although its declared visible width is 320. | Original loose/CD `marb04.vga`, cell `0x16`, row sources `0x3d6`/`0x54a`; CD resource offset `0xa1efe2`. The pinned sprite drawer uses a 960-entry work line starting at entry 320 and clips to declared width. | `XeenObjectSpriteSafety` validates expansion against that bounded work storage instead of rejecting all padding beyond visible width. Normal/flipped publication still clips the extra pixels; row/operand and scratch-edge checks remain enforced. |
| Keyboard buttons have no explicit tick hold; mouse buttons wait for release. Fades synchronize to VGA retrace. World YesNo/WhoWill outer waits use 1 tick, location waits 3; NPC polling follows its 3-tick interval. | `0x0e1ca..0x0e2b1`, `0x0e41e..0x0e430`, `0x3fe0f..0x3ff02`, `0x43fca..0x43fdf`, `0x24bd7..0x24c16`, `0x251a3..0x25262`, `0x25b8f..0x25bad` | M63 Part A: `SdlWindow` restores keyboard feedback without a tick dwell and retains mouse feedback until physical release, servicing focus/close events. `PresentationClock` uses the capture-supported standard BIOS and separate 70 Hz retrace timelines; `XeenEventFlow` also retains the original dialog waits and NPC draw/reset/input order. |
| <a id="sv-death-fire-pacing"></a> Death fire has no explicit 1-tick delay or final sound-wait tail; the pin adds both. CD title phase 0 draws background alone, and its first fade uses step 4. Dream waits for two sequential samples between its 7-tick holds and four step-4 retrace fades. | Death `0x0b02f..0x0b39a`; title `0x0be9a..0x0bf8d`; dream `0x1e605..0x1e72a` | `XeenTitleFlow` retains every acquired fade level, publishes phase one immediately after the initial fade, then waits four ticks per idle phase. `XeenEventFlow` uses retrace publication for death fire and sampled two-tick laugh iterations, with no added tail. `XeenRestFlow` waits 1.664504 s for dreams2 then 1.966764 s for laff1 between single-reset seven-tick holds; recovery follows the final acquired fade exactly once. Escape/right click advances each original credits page. **ScummVM:** differs. |

The maintainer's 2026-10-13 normal DOSBox Staging CD play (`cpu_cycles 80000`,
`Sonho.mkv`) confirms that **Escape does not interrupt the Rest dream**. This
agrees with the no-local-input-poll path at DAT `0x1e605..0x1e72a`; the video
has no input overlay, so it does not timestamp the press or settle later queued
input. [Capture measurements](rules/evidence.md#presentation-clock-captures-2026-10-13)
support standard BIOS and approximately 70 Hz retrace timing. Updated recordings
include the sequential dream samples at their header durations; a common A/V
displacement must not be added to the emulated silent holds.
`XeenRestFlow` preserves that no-skip path and defers gameplay input until the
original post-dream poll.

## Approved temporary restrictions

- <a id="sv-m82-visible-text"></a> **M82 Text off remains visible until M83.**
  The maintainer approved this temporary restriction on 2026-10-21: store,
  show, couple and save Text normally, but keep Event and dialog text visible
  until M83 provides CD speech. New Clouds therefore retains the original
  Efx/Music on, Text off, Speech on values without leaving the player with
  neither text nor speech. `XeenSoundOptions`, `XeenTitleFlow` and the existing
  Event presenter own these parts. **ScummVM:** the pin has speech playback
  and separate global subtitle settings; this temporary restriction belongs
  to MMModern. Pin `6814ee9b`; master not rechecked.

- Jump destinations outside the current saved mainland remain refused before
  charge. M72 applies the original surface effects to admitted destinations;
  a fall to an unadmitted map is also refused before publication. Read-only CD
  map 23 inspection confirms `(11,11)` East to `(13,11)` passes walls `0,0`
  and lands on space, but that destination remains outside the admitted area.
  This retains the area restriction approved on 2026-10-12, without the old
  blanket refusal of hazardous surfaces.

## Approved deviations

Deliberate differences from the original that the maintainer approved; each
stays documented here and in the plan that implements it.

- <a id="dev-whowill-event-passes"></a> **WhoWill clears only on arrival and
  on Space, not on every Event-routine call** (maintainer, 2026-10-21, M81 Part
  C review). DOS calls the Event routine `0x26774`, which clears WhoWill at
  entry (`0x26797`), on every main-loop pass over a cell with the Event flag,
  on Space or a 3D-view click not taken by a grate, after combat when attack
  slot 0 skipped an Event, and at combat end when door text was up
  ([trace](rules/evidence.md#preferred-recipient-after-the-events)). MMModern
  runs a cell's Events only on arrival or Space with a matching record, so it
  clears WhoWill on arrival at each new position or facing over a flagged cell, on Space
  not taken by a grate, and in every Event run (`XeenEventFlow::
  prepareJourneyTransition`, `XeenEncounterFlow::clearWhoWill`). Further passes
  while standing on a flagged cell and the two post-combat calls do not clear
  it: a delivery there, in a later pass than its WhoWill dialog, still prefers
  that member. Event dispatch per main-loop pass is separate work.
  **ScummVM:** differs: the pin clears in all four cases, like DOS.
- <a id="dev-confirmword-alternate"></a> **ConfirmWord opcode 21 takes ordinary success on an alternate-only match**
  (maintainer, 2026-10-16). The DOS marker is an uninitialized Event-function
  local: `0x27f15` skips `0x27f09`'s assignment; `0x27fba` passes it;
  `0x2405e..0x24067` compares it with 55; `0x280cc..0x280e6` branches.
  Residue 55 can select operand 3 instead of the ordinary success line. The
  only Clouds alternate-only use is map 69 `(9,15)` West line 0, whose missing
  alternate aliases text[0]. MMModern deliberately takes the ordinary path
  (the result when the marker is not 55), in `XeenEventInterpreter` ConfirmWord,
  tested by `XeenTravelInputTests`. This conditional code path was established
  statically; a runtime divergence was not observed in DOS play. **ScummVM:** approved deviation.

- <a id="dev-mirror-bound"></a> **Mirror lookup stops at the actual resource records** (maintainer,
  2026-10-16). DOS's inclusive bound permits index 62 (`0x240d5..0x24118`),
  although this CD's `xeenmirr.txt` has 62 complete records, indices 0..61.
  The shared loader reads only the member length and does not clear its tail
  (`0x45ef8..0x45f33`), so that last read examines stale scratch, not another
  resource destination. `XeenEventInterpreter` bounds lookup by the actual
  records and the original list bound; no destination is invented from stale
  memory. `XeenEventTextLoader::parseMirrors` retains complete records only. **ScummVM:** approved deviation.

- <a id="dev-spawn-frame-random"></a> **Spawn animation frames use cosmetic randomness** (approved M48/M50 policy,
  retained by M70 Part A). DOS draws once from `0..7` at `WORLD/XEEN.DAT`
  `0x27a89` and writes only the slot's frame (`0x27aa1`).
  `XeenScenePresentation::spawn` keeps that draw off the Journey generator;
  repeated candidate publications retain its cosmetic sequence. Map effects,
  monster type, HP, position and accounting keep their original semantics. **ScummVM:** approved deviation.

- <a id="dev-cd-speech-delay"></a> **CD speech uses its requested DOS interval without a guessed driver delay**
  (maintainer, 2026-10-13; M63 Part B). `NPC falando.mkv` identifies Gunther's
  track/cue and supports the end boundary, but cannot isolate playback-command,
  mixer and capture latency. Use exactly `(end-start+25)/75` seconds from the
  original playback call. The maintainer explicitly accepted this model and
  declined an instrumented capture. [Evidence and limits](rules/evidence.md#death-and-cd-speech-captures-2026-10-13). **ScummVM:** approved deviation.

- <a id="dev-no-phantom-steps"></a> **No queued "phantom" movement steps** (maintainer, 2026-10-13; implemented in
  M63 Part A). The original keeps up to ten keys in its own keyboard
  FIFO (`WORLD/XEEN.DAT` `0x0b3b8..0x0b3bf`, producer `0x45664..0x4568c`,
  consumer `0x4549c..0x454e6`; releases do not purge it, `0x455dd..0x45620`),
  so a held movement key keeps moving the party after release. MMModern: while
  a movement, strafe or turn key is held, its fresh press acts once, then waits
  at least 500 ms from platform observation of the press before repeating.
  Subsequent repeats use the maintainer-authorized IBM enhanced-keyboard fallback
  (nominal 10.9 Hz, an ideal 11/120-second period), capped by the original draw
  pace. A late wake supplies one repeat, never a backlog. This corrects the
  initial Part A implementation's immediate next-draw repeat (maintainer
  play-test, 2026-10-13). The recordings show first-to-second visible-step gaps
  of 467/416 ms but cannot timestamp physical input or isolate the producer's
  rate; DOSBox Staging 0.83.0 actually defaults to 500/33 ms. The fallback is
  explicit, not a claim that its device defaults to 10.9 Hz. See
  [typematic evidence](rules/evidence.md#navigation-typematic-evidence-2026-10-13).
  On release it stops after the step already in progress and queued repeats
  are discarded; each distinct tap still moves exactly one step. Fresh
  navigation taps use an ordered dynamic FIFO; held demand carries the physical
  scancode/press identity, so release also works across a keysym/modifier change.
  Other keys keep their current queueing. Implementation: `SdlWindow.cpp`;
  evidence: D:/MModern-re/movement-fluidity-20261013. **ScummVM:** approved deviation.

- <a id="dev-map-state-deltas"></a> **Per-map state is saved as deltas, not whole map files** (maintainer,
  2026-10-15; M69 decision 7, save v13). The original writes each visited
  map's whole EVT, DAT and MOB into the save archive. MMModern keeps the
  original resources immutable and saves, per map, its complete monster table
  plus sparse overrides against the resources: object positions and flips
  (a removed object is `(-128,-128)`, as Remove writes it), wall-item
  positions, cells as their effective DAT word and attribute byte with the
  original preimage, Event record opcodes and explored bits. Restore rebinds
  every override to its map's resources, so the result is observably the
  same. Monster HP and wake state are not saved (decision 8 above). The
  overrides cover every map change the game produces today: MoveObj and
  Remove (objects), MoveWallObj (wall items), doors, AlterMap and
  AlterCellFlag (cell walls, the unlocked bit, the outdoor surface nibble and
  the indoor surface bits), and AlterEvent, MakeNothingHere and Remove (Event
  opcodes); restore accepts each, and `XeenSaveStateTests` round-trips each
  through a real Event. As in the reference, which writes only the primary
  outdoor DAT, an outdoor write to a neighbouring tile lasts only for the
  visit and is not saved. Restore still refuses cell attribute changes no
  producer makes (bits 3-6, clearing an original unlocked bit, an outdoor
  attribute byte). The reference saves no HED, and AlterHed changes are kept
  only in the current map's head cache ([per-map write-back](#per-map-write-back)).
  Any future mechanic that changes map data some other way must extend these
  overrides. **ScummVM:** approved deviation.

- <a id="dev-no-please-wait"></a> **No "Please Wait..." box on map loads** (maintainer, 2026-10-20). DOS
  draws the original "Please Wait..." box while it loads a map: on outdoor
  region changes and stairs (maintainer's DOSBox play) and on the entrance
  scenes' destination load and keyless reload, where the M80 captures show
  it for 16 and 14 frames at 70 Hz, about 0.2 s set by the emulated load
  time ([capture](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)).
  MMModern loads maps without delay and shows no box, so map changes are
  about that much faster than in DOS; the load itself, its timing in game
  time and every other effect are unchanged. **ScummVM:** approved
  deviation; the pin loads the string but never shows it either.

## Reference behavior not reproduced

Places where the pinned ScummVM code differs from the DOS original or contains
a port defect; MMModern follows the original instead.

| Reference behavior | MMModern | Where |
| --- | --- | --- |
| <a id="sv-alter-event-operand"></a> AlterEvent's line operand matches every stored line 0-255 in the reference. | DOS sign-extends the operand (`0x2827f..0x28287`) and compares it with the index line, which the Event index builder stores zero-extended (`0x21e75..0x21e7e`). Operands 128-255 therefore never match, and the instruction changes nothing. Bounded static check, 2026-10-15. | `XeenWorld::applyAlterEvent`; [world Event rules](rules/world-events.md). **ScummVM:** differs. |
| <a id="sv-make-nothing-here"></a> MakeNothingHere exits the script in the reference. | DOS clears all opcodes at the party's physical cell, then advances the logical line normally. A script called at another cell can continue. `WORLD/XEEN.DAT` `0x2793b..0x279ec`, `0x28356..0x28372`, `0x28d30..0x28d3a`; bounded static check, 2026-10-15. | `XeenWorld::applyMakeNothingHere`, `XeenEventInterpreter`; [world Event rules](rules/world-events.md). **ScummVM:** differs. |
| <a id="sv-alter-event-playcd"></a> AlterEvent replaces only matching records in the reference. | DOS additionally replaces the next stored record when a matched effective opcode is PlayCD (60), regardless of that next record's cell. `WORLD/XEEN.DAT` `0x282c0..0x28304`; bounded static check, 2026-10-15. | `XeenWorld::applyAlterEvent`; [world Event rules](rules/world-events.md). **ScummVM:** differs. |
| <a id="sv-fix-identify-order"></a> `FIX_IDENTIFY` is ordered Fix, Identify in the pinned English constants. | The CD DOS table is ordered Identify, Fix: `WORLD/XEEN.DAT` `[0x55cdf,0x55cec)`, read-only inspection on 2026-10-12. Select each confirmation verb using the DOS order; the maintainer's Part A play-test exposed the swapped prompts. | M61 Part A, `XeenDialogView.cpp::xeenServiceConfirm` **ScummVM:** differs. |
| <a id="sv-attribute-material-price"></a> Item pricing uses `ELEMENTAL_DAMAGE[material - 52]` for attribute materials, which reads past the reference table for materials 89-130. | DOS adds `ATTRIBUTE_BONUSES[material - 59] * 100` before the action divisor. Read-only original program analysis, 2026-10-12: `WORLD/XEEN.DAT` lookup `0x3c7fd`, multiply `0x3c802`, divisor branch `0x3c81f`; table `[0x4f9e6,0x4fa2e)` matches all 72 pinned attribute bonuses. Metal `[0x4f9d0,0x4f9e6)` and elemental `[0x4f969,0x4f98e)` tables match the pin. Calibrated against the plan's Buy and DOSBox Fix witnesses; high confidence. DAT SHA-256 `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`. | M61 Part A, `XeenItemServiceRules.cpp::xeenItemPrice` **ScummVM:** differs. |
| <a id="sv-identify-classes"></a> Identify's proficiency loop supplies the loop index as item ID and uses the owner's class. | DOS enumerates classes using the selected item ID/category independently of the owner. Read-only original program analysis, 2026-10-12: `WORLD/XEEN.DAT` selected-record dispatch `0x32061..0x320a5`, class loop/call `0x320a7..0x3211e`, All substitution `0x32120..0x32134`, restriction helper `0x3c516..0x3c562`. Calibrated against character item offsets and original Item Details formatting; high confidence, same DAT as above. | M61 Part A, `XeenDialogView.cpp::xeenIdentifiedDetails` **ScummVM:** differs. |
| <a id="sv-broken-weapon-names"></a> Broken/cursed weapons retain their effectiveness suffix in the name. | DOS hides both material and suffix for `state & 0xc0`, retaining the Broken/Cursed prefix and base type; Identify still shows the actual properties. Read-only original program analysis, 2026-10-12: `WORLD/XEEN.DAT` formatter `0x3ccb7`, bad-state mask `0x3ccde..0x3cce8`, suffix skip `0x3cd07..0x3cd10`, material skip `0x3cd7c..0x3cd85`. Calibrated against the maintainer's broken-name DOSBox observation and Broken/Cursed strings at `0x55d03`/`0x55d0e`; high confidence, same DAT as above. | M61 Part A, `XeenItemCatalog.cpp::describe` **ScummVM:** differs. |
| <a id="sv-spell-list-format"></a> The spell-list constant places its first row at tab 010 and formats its balance with `%u`. | The installed CD DOS list uses tab 011 for the first row and `%lu` for the balance. Preserve those resource bytes when adding the original spell screens; this is a presentation choice, not a change to spell costs. Read-only `WORLD/XEEN.DAT` inspection on 2026-10-11: `[0x54b30,0x54bdd)`, compared with pinned `en_constants.h::SPELLS_DIALOG_SPELLS`. | M60 Part A, `XeenDosText.cpp`, `drawXeenSpellList` **ScummVM:** differs. |
| <a id="sv-charges-glyph"></a> `FMT_CHARGES` carries a redundant alignment letter that draws a stray glyph in item titles. | Uses the DOS bytes from `WORLD/XEEN.DAT`, without the glyph (DOSBox-confirmed). | M47, M53 **ScummVM:** differs. |
| `ItemsDialog::setEquipmentIcons` sets an accessory frame through `_id = 8`. | Uses the item's own frame. | M47 |
| `CharacterInfo::execute` forces combat mode after every stat popup. | Restores the caller's mode. | M47 |
| <a id="sv-control-panel"></a> The control panel drops Save As and rearranges the buttons; saves use ScummVM's own chooser. | DOS panel and chooser from `WORLD/XEEN.DAT` and DOSBox evidence. | M54 **ScummVM:** differs. |
| <a id="sv-info-title"></a> Info chooses its title by engine game ID, and its Walk on Water format takes three arguments. | CD Clouds Info uses the World title (maintainer DOSBox play, 2026-10-10). The bounded DOS fields at `[0x53673,0x53679)` and `[0x535d4,0x535e6)` supply that title and the two-argument Walk on Water format. | M55 Part A, `XeenDosText.cpp`, `XeenDialogView.cpp` **ScummVM:** differs. |
| <a id="sv-bank-wording"></a> The pinned English bank constants spell Withdrawal as `Withdrawl` and use a different refresh control prefix. | Preserve the installed CD DAT wording at `[0x51b51,0x51b5c)` and refresh template at `[0x52b32,0x52b79)`, verified read-only on 2026-10-10 against `en_constants.h::DEPOSIT_WITHDRAWL/GOLD_GEMS_2`. | M56, `XeenDosText.cpp`, `XeenDialogView.cpp` **ScummVM:** differs. |
| <a id="sv-bank-amount-entry"></a> Bank amount entry parses with signed `atoi`. | DOS scans up to ten digits with `%lu`; on 32-bit overflow its conversion saturates to `2147483647` instead of wrapping, and the unsigned comparison with the balance then refuses it as "not enough" (maintainer DOSBox CD play, 2026-10-10: `4294967296` and `4294967306` refused; original program analysis, 2026-10-11, `WORLD/XEEN.DAT` `0x1c702`, `0x1e26f`, `0x1e941`, `0x67bf`). MMModern refuses such amounts as excess, with the same result for any reachable balance. | M56, `XeenFoodBank.cpp::xeenBankAmount/xeenTransferBank` **ScummVM:** differs. |
| <a id="sv-eight-hour-poison-disease"></a> The eight-hour Poison and Disease checks draw only for members who do *not* have the condition, and Disease draws `0..9`. | **DOS rule implemented.** In active member order, process Poison then Disease, only when present: draw `1..10`; on 1, draw `1..(resistance + 40)` and pass at `<= resistance`, using electricity for Poison and cold for Disease (permanent + temporary + equipped resistance). A pass clears; every failure doubles modulo 256 (`128 -> 0`, `255 -> 254`). Terminal members still participate; absent conditions draw nothing. Read-only original program analysis rechecked 2026-10-11: `WORLD/XEEN.DAT` `0x25349`, `0x25425`, `0x25473`, member loop `0x255ff`; save routine `0x2d164`, `0x2d18a`, `0x2d200..0x2d21f`; inclusive range helper `0x45b9a`. Calibrated against the known character attribute order in `0x21454` and the byte layout; DAT SHA-256 `8bf78b4850d5973cbb063c7dd96b9aaef4e94791d748334187eef74acf528d27`. | `XeenCombatRules.cpp::XeenConditionTimeCandidate::service`, existing `XeenCharacterRules.cpp::damageSaveValue/sheetResistance` **ScummVM:** differs. |
| <a id="sv-condition-modifiers"></a> `Character::conditionMod` and the eight-hour Weak/Drunk step treat a condition byte `0xFF` as the `-1` sentinel; In Love, Insane and Drunk have different attribute signs. | **DOS rule implemented.** The eight-hour step unconditionally replaces Weak with Drunk and clears Drunk, including Weak 255 and Drunk 0/255. Dawn increments Weak up to 255. Attribute modifiers use unsigned counts, one point per count: In Love adds to all seven; Insane adds to Might/Speed, subtracts from Intellect/Personality/Accuracy, leaves Endurance/Luck unchanged; Drunk adds to Personality/Luck and subtracts from the other five. Weak 255 subtracts 255. Dead/Stoned/Eradicated suppress the modifiers. Read-only original program analysis rechecked 2026-10-11 on the same DAT: `0x255ab`, `0x26452`, `0x1f9a8..0x1fbe4` (Insane `0x1f9fa`, In Love `0x1face`, Weak `0x1fb1b`, Drunk `0x1fb68`); selector table `0x1fbe6` agrees with the known attribute order in `0x21454`. | `XeenCombatRules.cpp::XeenConditionTimeCandidate::service`, shared `XeenCharacterRules.cpp::conditionModifier` used by `sheetStat/effectiveAttribute` **ScummVM:** differs. |
| <a id="sv-pain"></a> Pain (spell 50) prepares zero damage and appears to do nothing. | DOS prepares 8 physical damage against the first group, with the normal physical resistance and saving throw. Original program analysis, 2026-10-11, `WORLD/XEEN.DAT` `0x33558`, `0x2f5e9`, `0x2cd81`. | M60 Part B, `XeenSpellDamageCandidate` **ScummVM:** differs. |
| <a id="sv-suppress-poison"></a> Suppress Poison's target dialog passes First Aid's ID, refunding 1 SP on cancel, and reduces Poison by 2 when it is at least 4. | DOS passes ID 68, refunds 4 SP on cancel and subtracts 3. Original program analysis, 2026-10-11, `WORLD/XEEN.DAT` `0x2f62f`, `0x2f146`, `0x2f34b`, `0x2f67b`. Follow-up 2026-10-17 confirms zero unchanged (`0x2f663..0x2f669`), 1..3 set to 1 (`0x2f673`), 4+ subtract 3 (`0x2f683`), all selected-target paths reaching healing(0) at `0x2f69b`. Zero/1..3 and the unconditional helper match the pin; no extra zero-path discrepancy. | `XeenLearnedSpellRules` and own-ID casting refunds, implemented in M75 Part A. **ScummVM:** differs; master not rechecked. |
| The pin's `_currentCantRest` tests a flag its cell loader cannot set. | DOS differs: cell bit `0x40` is loaded and tested. See [Rest cell flag](#sv-rest-cell-flag). | MMModern follows DOS in `XeenRestFlow::beginRest`, including Super Shelter's bypass. |

## World CD protection Events

The copy-protection Event (`ProtectionCheck`, opcode `0x2f`) is a no-op on
all admitted Clouds maps, as it already was in Vertigo. This follows the
pinned reference's default World CD configuration: `Scripts::copyProtectionCheck`
returns success unless copy protection was explicitly enabled. MMModern does
not expose that optional dialog. **Evidence: reference only**; M78 Part B did
not add a DOS observation. Location: `XeenEventInterpreter`, the shared
`XeenEventProtectionCheck` dispatch. The map-28 admission guard is removed;
this does not introduce a second protection policy for other maps.

## Entrance scenes and the Arena

These rules are **DOS static, 2026-10-20**, calibrated against the settled
town-service dispatch in the World CD executable. Full provenance,
offsets and unresolved boundaries: [M80 evidence](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20).
They are implementation rules, not a claim that these features are playable.

<a id="scene-refusal-reload"></a>
### A keyless entrance reloads the map

Reaper, Golem and Sphinx preserve the current destination on refusal but
still call the map loader (`0x2afe1`, `0x2a5db`, `0x29a57`). Ordinary
maze state is saved; runtime monsters are rebuilt, including frame/effect
randomization. This does not imply respawning dead monsters. Map load sets
stepped (`0xf58b`), redraw consumes it (`0x1184f`), and the Event caller
sets it again (`0x27b32`). The scenes bypass the town-service day charge.
Preserve the reload and lighting behavior. High confidence; the reference
also reloads. A Darkside key at a cross-side site is outside the World CD
reference's Clouds play (only Darkside Events grant those keys).

<a id="sv-arena-species-pool"></a>
### The Arena chooses five species before choosing monsters

DOS draws five species slots with the 67/59/28 adjustments, then generates
monsters using a random slot ID 0..4. Species selection precedes all
position/frame/effect draws (`0x2ba4b..0x2bb81`, `0x2bb8c..0x2bd1c`).
Duplicate species slots are allowed; a one-monster fight still draws five.
Direct generation uses `5 + 4*count + nonzeroEffectCount` draws.
High confidence. **ScummVM:** the pin's `ArenaLocation::show` chooses
one species per monster instead. MMModern follows DOS in `XeenArenaRules`
and the detached Battle Master Event continuation. Master not rechecked.

<a id="sv-arena-ordinal"></a>
### Arena ordinals use the units digit except in the teens

DOS uses `th` for 11..19 and the units digit otherwise, including
21st/22nd/23rd (`0x2b830..0x2b87a`). Award index 9's 99 check prevents
starting a fight; victory increments the byte without saturation
(`0x2b818..0x2b82e`, `0x2b891..0x2b89d`). High confidence.
**ScummVM:** the pin uses `th` for every count >=10. MMModern follows DOS
in `XeenArenaRules`; the suffix strings come from the original UI data.
Master not rechecked.

<a id="arena-visit-reset"></a>
### The Clouds Arena fight is discarded when leaving

DOS excludes Clouds map 85 from maze saving (`0x26600..0x26610`) and
resets its species slots and monster count on load (`0xfb8a..0xfba9`).
Generated monsters do not survive a return; a completed fight cannot yield
another award by leaving and returning. Character awards survive normally.
Victory/maxed exits use the newly loaded map 28's run position
(`0x2b8b7..0x2b8bf`, `0x2bd2c..0x2bd36`, `0x1691f..0x1692b`).
High confidence; the reference also excludes map 85 from maze saving.
Map 85's header carries the no-save flag `0x8000` (resource inspection).

<a id="sv-sphinx13-descriptor"></a>
### The Sphinx plays a sample with a damaged VOC descriptor

Clouds `sphinx13.voc`, the first refusal sample, differs from the VOC
descriptor text only at bytes 0..7. Its data offset (26), single type-1 PCM
block (length 39,245, codec 0, time constant 188) and terminator are intact.
The 39,243 audio bytes last 2.668524 seconds; `sphinx14.voc` adds 1.275068
seconds. The maintainer's [DOSBox refusal capture](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)
shows about 4.1-4.4 seconds of speech animation, consistent with both samples
and not with the second alone. DOS plays the first sample in full without
checking the descriptor text; follow DOS (maintainer, 2026-10-20).

`xeenVocDurationNanoseconds` ignores descriptor text while retaining its
minimum-size, data-offset, block-type, length, codec, rate-continuity and
terminator checks. No resource-name exception or invented duration is used.
**ScummVM:** pinned `6814ee9b`, `Audio::checkVOCHeader`, requires `Creative`
at bytes 0..7 and rejects this sample, skipping its speech interval. Master
not rechecked.

<a id="sv-scene-speech-cadence"></a>
### Entrance speech animation consumes gameplay randomness

DOS uses gameplay helper `0x45b9a` for scene frames/jitter. Loop counts
depend on speech and, on some phases, the text cursor; reload can add
further draws. Reaper speech explicitly waits two BIOS ticks
(`0x2ac94..0x2ac9c`, `0x2adf5..0x2adfd`), as does Dwarf
(`0x2b65a..0x2b662`). Conditional `special.bin` text rendering at
`0xdca3..0xdd52` precludes assuming the CD program is always speech-only.

Part 0b rechecks the two-tick calibration and settles the gate: startup
sets Effects (`DS:ed9a`) to 1 at `0xb63c`; the installed `XEEN.CFG`
supplies effects-driver word `DS:8d38=10` via `0x3d343`.
**The installed default therefore shows no entrance captions.** Effects
off enables them. Speech (`DS:f385`) and Text (`DS:f384`) are separate
controls (`0x1f04c..0x1f078`); Speech off forces Text on for its own
uses but does not change these scene captions or their VOC sample gate.
Do not wire the scene text gate to the Speech/Text controls. Efx-off captions
use the record cursor instead of the VOC-busy interval; see the
[Dwarf video finding](rules/audio.md#efx-voc-waits-and-entrance-captions). The M80 DOSBox captures observe the one-retrace
loops as two retraces per iteration (about 35 Hz); M80 uses that rate.

When enabled, the DOS cursor advances one character per two BIOS ticks
(`0xdcba..0xdccf`, `0xdd1a`, `0x4411d..0x44144`). The no-explicit-wait
loops use one VGA retrace per update (`0x4437e -> 0x43fca`), nominally
70 Hz; scene entry disables the mouse so input does not add another
redraw (`0x438f1..0x43913`, `0x43855..0x438ef`). This applies to
Sphinx zoom/speech/retreat, both Dwarf zooms and Reaper/Golem retreats.
Slow execution can miss retraces; actual DOSBox frame rate is not certified.
Record lengths are verified without extracting text: Clouds Dwarf is
107 bytes; the full metadata is in the evidence section.

Part 0b also corrects Part 0's retreat summaries: Reaper retreats **on
refusal**, skipping that animation with the key (`0x2ae50..0x2ae57`);
Sphinx refusal draws indices **0..8** (`0x29a08..0x29a0c`).
**ScummVM:** the pin uses `WAIT(1)`, its own subtitle setting/continuation
logic, two-character steps for the 107-byte Dwarf line, Reaper retreat on
success and Sphinx retreat 0..7. These DOS constraints apply when
implementing the scenes. Exact box-pixel disappearance in Effects-off
mode still requires a capture; default and Speech-off alone cannot expose
it. High confidence for the stated static paths, not pixel equivalence
or a fixed whole-invocation RNG count. Master not rechecked.

### DOS FM call arguments and retained presentation state

**DOS static, high, 2026-10-21.** The calibrated Efx wrapper at DAT
`0xdece..0xdee6` gates every request with the party Effects byte. The
[133-site inventory](rules/audio.md#dos-fm-effect-call-sites) distinguishes
shared helpers from spell-handler cues. Movement refusal at `0xcf48` receives
21 from `0xcf46`, or 46 from `0xcf6a/0xcf6c` for indoor walls and swamp
refusal. The maintainer's Part A DOSBox `promus-wall-bump` capture matches 46.
The step suffix (`0xd42b..0xd448`) toggles its bit before choosing 7/8, even
with Efx off. Passable indoor B skips Bash at `0xd534 -> 0xd3aa`
and reaches this movement suffix; it does not request Bash's 31. Only the
blocked indoor branch calls Bash (`0xd547 -> 0x32d49`), after the first charge,
and reaches 31 at `0x32d5b`; outdoor Bash returns before that cue. Neither the
step bit nor the attack weapon selector is saved.

Positive monster damage requests `60 + original table[attack weapon ID]`
(`0x2c0ef..0x2c105`, table `0x4d660`). The weapon helper clears the selector
at `0x2dfff` and assigns equipped IDs at `0x2e0ee`. Ordinary spell delivery
(`0x2cf94..0x2cf9f`) bypasses that helper and retains the previous selector;
Poison Volley runs the physical helper. Zero/nonpositive attack2 damage
reaches effect 6 (`0x2be33..0x2be3b`, `0x2c06e..0x2c07f`, `0x2c17c`). A
magic-resistance rejection (`0x2cd74 -> 0x2cfa7`) bypasses attack2 and is
silent at this shared site. Monster misses use a separate 6 (`0x2dc0b`), on
natural 1 or failed AC checks, including the second check after a critical
first damage application. Missed player missiles and Poison Volley bypass
attack2 (`0x2cb86 -> 0x2ccc4`); melee submits its zero total and requests 6.
The projectile loop checks the next wall before its spent-arrow exit
(`0x1db3e..0x1db84`): a wall immediately beyond the last consuming hit still
requests 46; a farther one does not. Single/Group spells with a dispatched
target return before this check; All spells continue. C2 owns individual spell cues. MMModern
retains these audio selectors in program state, separate from save v18.
Instruction slices and semantic notes remain in `D:/MModern-re/audio-m82/`.

<a id="sv-dos-fm-effects"></a>
### Mirror confirmation, Stone and monster-death FM cues

**DOS static, high, 2026-10-21.** A valid mirror-name selection requests 51
(`0x240fe`), in addition to the teleport's separate 51 (`0x270a8`). A
successful Stone secondary attack requests 37 (`0x2d62e`); Death requests 38
(`0x2d65e`). Monster death requests 4 after boss-specific quest writes and
before XP/treasure (`0x2c23c`). The calibrated wrapper and bounded caller
branches establish the arguments; the full references are in the audio
inventory. MMModern requests these through the existing Event and combat
owners. **ScummVM:** pin `6814ee9b` uses 61/35 for mirror confirmation,
38 for Stone and omits the monster-death FM request. Master not rechecked.
