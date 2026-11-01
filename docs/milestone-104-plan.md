# Milestone 104 plan - Mixed World of Xeen maps

**Tier A. Status: plan (2026-11-01); decisions approved by the maintainer,
independent plan review applied.** Step 3 of the
[Darkside arc](roadmap.md#darkside-arc). Save v23 -> v24 expected (decision
3); Journey schema 9 and content 14 unchanged.

## Goal

A World of Xeen party carrying the Darkside keys enters Darkstone Tower,
Dragon Tower, the Dungeon of Death and the Southern Sphinx from their Clouds
sites, and reaches the Dragon Clouds, as in the CD. The sixteen mixed maps
(Darkside 113-128) load with the resources, monsters, music and side rules
DOS uses for each of them, FlipWorld sends the party back to the side DOS
chooses, and Town Portal, Lloyd's Beacon, falls, saves and loads work from
every one of them. Whatever these maps use that is not implemented yet says
"not supported yet" and keeps the game running.

Out of scope: opcodes 57 and 58 and the Darkside and World endings (step 4);
the three waiting Tier A roadmap items (whole-Event discard on a runtime
refusal, the Xeen Slayer Sword selector, the cached night flag; the approved
[current-clock night](original-quirks.md#dev-current-clock-night) and the
four-retrace fall box stay as they are); DOS save import; new services; any
mechanic these maps need beyond the ones listed here, which gets a census row
and a "not supported yet" notice instead.

## What the reference does (research pass and plan review, 2026-11-01)

All sixteen maps are Darkside-archive maps (`MAZEX113..128`, Darkside Event,
text and map-name resources); only some resources are redirected per map.
The pinned reference (`map.cpp` `Map::load`, lines 685-752) chooses:

| Maps | Monster table | Monster art | Object art | Wall pictures | Animation table |
| --- | --- | --- | --- | --- | --- |
| 113-116, 128 | `dark.mon` | Darkside | Clouds | Darkside | `clouds.dat` |
| 117-120, 124 | `xeen.mon` | Clouds | Clouds | Darkside | `clouds.dat` |
| 121-123 | `dark.mon` | Darkside | Darkside | Darkside | `dark.dat` |
| 125-127 | `dark.mon` | Darkside | Clouds | Clouds | `clouds.dat` |

Further reference rules on these maps:

- Species 91 keeps its Clouds art everywhere.
- On 125-127 the indoor load temporarily switches to the Clouds archive:
  sky, ground, tiles and wall sprites come from Clouds, the Clouds horizon
  rule applies, and a Clouds song plays with no music draw. The logical
  archive is restored at the end of the load (line 1046). Every other mixed
  map draws a Darkside song after the monster and effect setup draws.
- Missing cells use Darkside surface slot 6 on 113-120, 125, 126 and 128,
  slot 0 on 121-124 and 127 (lines 1293-1303, 1327, 1363). Map 128 hides the
  horizon and always uses the day sky (lines 1034-1039, 1406-1412).
- A surrounding-map-120 workaround (lines 813-831) names Deep Mine Alpha; it
  is not taken as a Dragon Tower rule.
- Object anchors use the logical side (`interface_scene.cpp:2685-2688`,
  `3391-3394`), not the object-art side; monster voices use the current file
  side (`combat.cpp:842`).
- Town Portal from 113-127 goes to the Clouds towns; 128 uses the Darkside
  selector. The intent is unknown, and DOS decides.
- FlipWorld (opcode 59, Events on 113, 117, 121 and 125) only writes the
  pending load side (`scripts.cpp:1492-1494`). An Event teleport reloads only
  when the map number changes (`scripts.cpp:520-536`), Town Portal always
  overwrites the pending side, Lloyd's Beacon only when the map or side
  differs, and loading a save resets it from the saved side byte
  (`saves.cpp:193-206`).
- The reference's fall from 113 to Clouds 25 is already known to differ from
  DOS ([sv-darkside-fall-bounds](original-quirks.md#sv-darkside-fall-bounds)).
- `tools/XeenDosExport.cpp` already encodes the art table above for the
  DOS export.

The reference was wrong in comparable Clouds cases (M80), so none of this is
adopted before Part 0.

## Key decisions (maintainer, 2026-11-01)

1. **One milestone, three parts, one play-test.** Part 0 DOS analysis, Part
   A loading and saving the mixed maps, Part B the ways in and out. Part B
   depends on Part A; there is no earlier play-test.
2. **Independent selectors per resource family, not one substituted side.**
   Each map identity resolves to a set of separate selectors, built from the
   DOS rules of Part 0: map archive (map, Event, text, map name), monster
   table, monster art, object art, wall pictures and picture metadata,
   wall/sky/ground/tile/surface sprites, animation table, missing-cell
   surface slot, horizon and always-day sky, music class, monster and defeat
   voices, ambient UI/NPC/dialog resources, and Town Portal side. The
   logical map identity, its archive and the gameplay rule side (flag and
   quest banks, roster filter, combat, movement, terrain, chests, services,
   object anchors) stay distinct from the art selectors and change only where
   Part 0 shows DOS changes them. A single substituted side cannot work:
   113 needs Clouds objects with Darkside monsters, 117 Clouds monsters with
   Darkside wall pictures, 125 Darkside monsters with Clouds scenery. The
   existing helper `xeenPictureArchive`, which already returns Clouds for
   125-127 and is reused for voices, defeat, dialogs and combat validation,
   is split by consumer. The two monster tables are immutable and cached by
   table identity, never by the last map's side; species 91 keeps its
   override only for its established art. The review's
   [caller inventory](#caller-checklist) is the required checklist. No new
   owner: the selectors live with the map identity and are read by the
   existing world, scene, combat, audio and save owners; nothing derived is
   serialized.
3. **Save v24.** A mixed map is saved with its side and number as today; the
   valid states widen and their validation changes:
   - maps DOS gives a fixed song require an absent song byte and derive the
     song without drawing; drawing maps require a saved song in the correct
     geometry row;
   - the envelope side stays equal to the camera's logical archive side;
     the Beacon's side and map stay independent and validated;
   - every saved map's actors are rebound to that map's monster table, so a
     history holding both 113 and 117 restores;
   - a pending FlipWorld side is saved only if Part 0 shows normal play
     reaches a persistent pending state.
   v24 is sound under the 0.x policy because validation and meaning change,
   even without new bytes; v23 saves are refused with the usual message. If
   Part 0 finds neither new state nor new meaning, the plan is amended to
   keep v23 before Part A.
4. **FlipWorld as DOS does it.** Part 0 settles the current versus pending
   load side across every load path (question 7). A pending side, if any, is
   staged with the existing detached world and Event candidates and
   published once, and belongs to the world owner beside the camera. Adding
   the side to the teleport's whole-identity comparison must not turn a
   same-number teleport into a reload or a music draw. If DOS drops the
   pending side on Load, that result is reconciled explicitly with the
   exact-restore policy. If DOS cannot represent a retained pending side,
   the DOS export refuses or documents the limitation; an MMModern side byte
   never silently changes DOS's side byte.
5. **Entrance keys.** The M80 rule that treats Clouds keys 35 and above as
   absent existed because only counters 0-34 were saved; since M101 all 85
   are. Part B removes it, so the four sites (Clouds maps 2, 7, 13, 19 with
   quest items 51, 46, 45, 50) play their keyed outcome from the shared
   counters. The counters and destinations are already calibrated
   ([evidence](rules/evidence.md#entrance-scenes-and-the-arena-2026-10-20)); Part 0 only adds the
   side and load behavior.
6. **Town Portal from 128** follows DOS whichever way Part 0 finds; if DOS
   differs from the reference it gets an original-quirks entry with a
   `**ScummVM:**` tag.

## Part 0 - DOS analysis

Calibrated static analysis of `WORLD/XEEN.DAT` (REA through WSL, as in M103),
work files outside the repository, results in
[evidence](rules/evidence.md) and [original quirks](original-quirks.md). It
starts from the known ordinary-map selection at `0xf6fd..0xf778`, the
species-91 switch at `0xffeb..0x10010`, the music draw sites
`0x102b1..0x102bd` / `0x1048d..0x10499` and the M80 entrance evidence, and
reuses settled evidence instead of repeating it. Questions:

1. The per-map table above: every column, keyed on map number or on a map
   flag, the exact ranges (121-123, 124, 128), and root versus neighboring
   map resource selection.
2. Sky, ground, tile and wall sprites, the horizon, the always-day sky and
   every missing-cell slot through 128, and the map name on 125-127;
   whether the reference's surrounding-map-120 workaround touches any mixed
   path.
3. Music: whether 125-127 skip the draw (indoor branch), which song and
   archive they use, the draw's range and table on the other mixed maps, and
   its place among the other load operations, keeping the established split
   between cosmetic and Journey randomness.
4. The ambient side after a mixed-map load and after FlipWorld, as UI,
   dialogs, voices (monster and defeat), object anchors, chest opening and
   combat rules see it; flag and quest banks and the party-side filter,
   especially on 125-127; the side byte DOS writes in a save there.
5. Town Portal and Lloyd's Beacon side from each map, names included; map
   restriction flags.
6. Falls from 113-127 (terrain and scripted, flag 118 included) and the exit
   from 128 to Clouds map 8 `(11,15)`.
7. FlipWorld: its operand, and the current versus pending side before and
   after FlipWorld, Event end, same-number teleport, failed transition,
   ordinary boundary load, entrance and reload, terrain and scripted falls,
   Town Portal, Beacon, and Save and Load; reset and overwrite rules, and
   whether normal play reaches a persistent pending state.
8. The side and load behavior of the four keyed entrances.

Exit: each question answered with offsets, or listed as unresolved. No
dependent Part A or B work starts until the maintainer settles the
unresolved ones. Answers that change a decision amend this plan first.

## Part A - Loading and saving the mixed maps

- Before any change, keep the verified v23 M44 baseline with the existing
  audit protocol (`tools/AGENTS.md`), as M101 did.
- Lift the mixed-map refusals in map identity, Event and text loading, map
  loading, save history validation, event-transition admission and the
  census together.
- The selectors of decision 2, with every caller in the checklist moved to
  the right one; species 91 unchanged.
- Music from the music selector at world load: the draw where DOS draws,
  the DOS song without a draw where it does not, and the save and Load rules
  of [dev-darkside-load-music](original-quirks.md#dev-darkside-load-music).
  Audio consumes the selected song without randomness, including the
  Darkside-camera assumption in the gameplay audio path.
- Save v24 with the validation of decision 3. The generic round-trip
  witness gains a history holding both monster tables, a 125-127 camera
  without a song, a drawing camera with one, nonzero values in both flag
  banks and a mixed-map Beacon; re-save is byte-identical, restore replays no
  RNG, time or reward, and a mismatched envelope, song or pending side is
  rejected without publishing partial state. The DOS export writes the mixed
  maps from the shared selectors instead of its own table.
- Census: the sixteen maps admitted through real engine admission, the
  presets and session callbacks included, coverage regenerated, calibration
  counts updated; no per-cell admission gates.

## Part B - Ways in and out

- FlipWorld (opcode 59) in the Event decoder and interpreter, per Part 0
  and decision 4.
- The four keyed Clouds entrances (decision 5). The Darkside map-19
  action-9 dispatch row to 121 is kept and tested as a generic row; no
  original Event uses it, so no live route is invented
  ([quirk](original-quirks.md#darkside-town-rules-established-from-dos)).
- Town Portal (names and destinations from the same selector, the five
  [fixed Darkside arrivals](original-quirks.md#sv-darkside-town-portal-stride)
  intact), Lloyd's Beacon, Teleport and Jump from every mixed map with the
  DOS side and restriction rules; terrain and scripted falls and the 128
  exit per Part 0, keeping the M103 fall damage and Oops box.
- Rerun the runtime-refusal sweep over the original data for all sixteen
  maps; every refusal it finds becomes a census row with a "not supported
  yet" notice, or is fixed if it belongs to this scope.

## Risks

- **A consumer left on the wrong selector** shows wrong art, voices or
  statistics on some maps only. Mitigation: the caller checklist and the
  parameterized all-map test below; rendered frames inspected for each
  distinct behavior case.
- **Ordinary maps changing by accident.** Ordinary-map gameplay, RNG and
  time traces and common decoded fields stay unchanged. The raw M44 hashes
  necessarily change with v24: every byte or field difference is explained
  and the maintainer approves the replacement digests; if v23 is kept, the
  hashes stay unchanged. No legacy production reader or writer is kept.
- **RNG order.** A skipped or added music draw shifts the Journey tape; the
  draw's place follows Part 0 and is tested together with the next Journey
  draw.
- **Pending FlipWorld side** can be lost or wrongly kept by a save, a
  reload or a failed load; covered by decisions 3 and 4 and their tests.
- **Content the maps use that is still missing** (monster abilities,
  Events); the refusal sweep bounds it before the play-test.

## Acceptance

- Part 0 evidence recorded; this plan amended where DOS differs.
- A parameterized original-data test loads all sixteen maps through
  production admission and asserts every resolved selector and restriction,
  with the boundaries 120/121, 123/124, 124/125, 126/127 and 127/128.
- Music: for every mixed map, zero or one draw as DOS establishes, its
  range, table, song and archive, and the next Journey draw; on first
  entry, revisit and a real same-map reload; no draw during selector
  lookup, preflight, redraw, audio callbacks or exact restore; audio
  enabled, muted and absent; Town Portal learned and from an item, in
  combat too.
- Live-input original-data tests, with input clearing, Event settlement and
  elapsed time, no per-feature process witnesses: a few walks, fights and
  Events across distinct behavior cases; all four keyed entrances with and
  without their key and their FlipWorld returns; Town Portal names and all
  five destinations with coordinates and facings; Beacon set, default,
  return, same-map and cross-side; Teleport and Jump restrictions; terrain
  and scripted falls (flag 118 included) and 128's Clouds exit.
- FlipWorld: flags before the load, same-number teleport without a load,
  replacement and cancellation, save continuation and failed-load rollback.
- The save witness of Part A; ordinary-map traces unchanged and the
  replacement M44 digests approved; census regenerated.
- Independent reviews of this plan and of each part's implementation.
- Complete CTest suite passing on `build-rel`.
- One maintainer play-test after Part B. Routes, keys, destinations, flags,
  RNG and saves are settled by the tests above and resource choices by
  inspected frames and audio, reusing the M80 and M102 entrance evidence.
  The session covers only what remains, with a reason for each item (input
  feel, audible or visible equivalence without a DOS capture), preset saves,
  launchers and coordinate moves per [play-tests](playtests.md); with
  nothing uncertain it is a short check of the main result.

<a id="caller-checklist"></a>

## Caller checklist

From the independent plan review; "logical" keeps today's side unless Part 0
shows DOS uses another. All paths under `src/games/xeen/` unless noted.

| Family | Sites | Treatment |
| --- | --- | --- |
| Admission | `XeenMapIdentity.h:44`, `XeenVertigoWorld.cpp:53-64`, `XeenSaveState.cpp:69-71`, `tools/XeenCoverageMain.cpp:138` | Lift together |
| Map, Event, text, map name | `XeenMapLoader.cpp`, `XeenEventLoader.cpp`, `XeenEventTextLoader.cpp`, `src/formats/xeen/XeenAssetSource.cpp:47-52`, `ScummVmXeenBridge.cpp:488-494` | Logical Darkside archive |
| Monster table | `ScummVmXeenBridge.cpp:453-459`, `XeenSaveState.h:27-36`, `src/app/XeenGameplay.cpp:206-210`, `XeenActorApproach.cpp:225`, `XeenWorld.h:183`, `XeenVertigoWorld.cpp:84,196-270`, `XeenSaveState.cpp:195-249`, `XeenWorldPresentation.cpp:32-40`, `CloudsMapComposer.cpp:87-94` | Monster-table selector; caches keyed by table |
| Animation table | `XeenObjectVisual.cpp:18,34`, `ScummVmXeenBridge.cpp:428-435` | Animation selector |
| Object and monster drawing | `CloudsMapComposer.cpp:14-117`, `XeenIndoorScene.cpp:689`, `XeenOutdoorScene.cpp:265` | Separate object and monster archives |
| Pictures, walls, surfaces, sky | `XeenIndoorScene.h:23-25`, `XeenIndoorScene.cpp:276-360,599`, `CloudsMapComposer.cpp:110` | Picture, sprite, slot and sky selectors |
| Object anchors | `XeenIndoorScene.cpp:534`, `XeenOutdoorScene.cpp:169` | Logical |
| UI, NPC, minimap, dialogs | `src/app/Application.cpp:481-628`, `CloudsMapComposer.cpp:55-131` | Per Part 0 question 4 |
| Voices | `src/app/XeenEncounterFlow.cpp:357`, `src/app/XeenEventFlow.cpp:385-394,459,1060,2758-2816` | Voice selector, defeat on 125-127 included |
| Music, CD, location return | `XeenVertigoWorld.cpp:303-313`, `src/app/XeenGameplay.cpp:594-610`, `src/app/XeenInnFlow.cpp:136-175`, `src/app/XeenAudio.cpp:350,389-390` | Music selector at world load; others classified |
| Save, export, tools | `src/formats/xeen/XeenSaveFormat.cpp:330-516`, `XeenSaveState.cpp:192,383`, `tools/XeenDosExport.cpp`, `tools/XeenCoverageMain.cpp`, `tools/XeenCoverageContext.cpp`, `tools/XeenTestSave.cpp` | Shared selectors; decision 3 |
| Town Portal, Beacon, teleport | `XeenLearnedSpellRules.cpp:15-58`, `src/app/XeenCastingUi.cpp:17`, `XeenEventInterpreter.cpp:1957-1979` | Portal selector for names and targets; Beacon and pending side independent |
| Flags, quests, roster | `XeenEventValues.cpp`, `XeenGameFlags.cpp:18-23`, `XeenParty.cpp:102-109`, `XeenDialogView.cpp:76` | Logical; shared counters stay shared |
| Combat, movement, terrain, chests | `XeenCombat.cpp`, `src/app/XeenJourneyConsequences.cpp:137-139`, `XeenMovement.cpp`, `XeenRegionalRules.cpp:166-193`, `XeenMonsterTreasure.cpp:59`, `XeenTerrainRules.cpp`, `XeenEventInterpreter.cpp:968,1631`, `XeenWorld.cpp` | Logical; Clouds statistics do not imply Clouds rules |
| Entrances, services | `XeenEntranceScene.cpp`, `XeenServiceAnimation.cpp`, `src/app/XeenEventFlow.cpp:2817-2909`, `src/app/XeenInnFlow.cpp`, `src/app/XeenSmithFlow.cpp`, `XeenEquipmentPurchase.cpp:17`, `XeenDialogView.cpp`, `XeenScriptedRules.cpp` | Logical; fix only shown misrouting |
