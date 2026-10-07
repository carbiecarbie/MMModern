# Milestone 52 plan - Normal start in Vertigo

**Tier A. Status: approved; not started (2026-10-07).** Independent plan review amendments incorporated; implementation requires explicit start.

## Goal and baseline

A new Clouds game starts with original party/state, using existing Vertigo/mainland combat, services, Rest and exact save/restore.
Keep prepared Journey for developers/M44. No new content/coordinator; combat changes only for difficulty.
Baseline (2026-10-06, clean): `main`, HEAD, `origin/main`, live `ls-remote` = `143f397c4cbe3ff3fded09115eb3855170e392db`.
Reference clean at `6814ee9ba54582f5b5adcffab49efbbd8f589edd`; `R/` = its `engines/mm/xeen/`, `M/` = this repo's `src/`.
Read-only data: `F:/Games/gog/Might and Magic 4-5/XEEN.CC`. Scope: Clouds/World, both difficulties; no standalone Clouds/Darkside support.

## Initialization evidence and required result

- Archive reset: `R/files.cpp:161` concatenates resources `2a0c,2a1c,2a2c,2a3c,284c,2a5c`;
  `R/saves.cpp:212` resets combat/selects Clouds. MMModern reconstructs initial data without player saves:
  `M/compat/scummvm/ScummVmXeenBridge.cpp:36,311` and `M/games/xeen/XeenPartyLoader.cpp:13`.
- Thirty CHR records then PTY active IDs: `R/files.cpp:148`, `R/party.cpp:56,278`, `R/character.cpp:168`;
  `M/formats/xeen/XeenCharacterFormat.cpp:129,184`, `M/games/xeen/XeenPartyLoader.cpp:20`. Retain original items/equipment.
- Read-only decoding confirmed CHR = 10,620 bytes and PTY = 812 bytes. Active order:
  Arturius/0, Tyro/18, Badger/14, Zippo/11, Rebecca/1, Seymour/6; all level 1, XP 0,
  HP `12,16,12,10,7,5`, SP `2,0,2,0,7,9`. Preserve these values, do not refill.
  Offsets level/HP/SP/XP=35/342/344/348; MM `:83,129`, `R/character.cpp:190,243`, `tests/PartyIntegrationTest.cpp:30,178`.
- PTY byte coverage begins: 0..1=`6,6`; 2..7=active IDs above; unused member slots 8..9=`00,00`.
  Bytes 10..13=`3,18,4,28`: West, `(18,4)`, Vertigo/map 28; 14..16=`1,1,0` ignored configuration;
  17=prior map 28; 18..26=zero effects; 27=0/Adventurer; 28..603=zero Clouds stock.
  `R/party.cpp:278,286,292,294,304,306`; roadmap confirmed. Add typed location parsing alongside
  `M/formats/xeen/XeenCharacterFormat.cpp:184`; camera is currently external (`M/app/XeenGameplay.cpp:119`).
- PTY words 610/612/614/616/618: ctr24=0, day=1, year=610, minutes=480, food=90 (`R/party.cpp:311`).
  `R/saves.cpp:239` resets stock/totalTime/year/day; MM: `M/formats/xeen/XeenGameplayContextFormat.cpp:7`, `M/games/xeen/XeenPartyLoader.cpp:29`.
- PTY dwords 638/642/646/650: gold=800, gems=10, bank gold/gems=0, all original; retain original purse/equipment
  (`R/party.cpp:325`; `M/formats/xeen/XeenCharacterFormat.cpp:105,116`).
- Remaining PTY spans: 604..609=zero completion flags; 610..619=time/food above;
  620..631=zero light/torch/resistances; 632..637=zero death/win/loss counters; 638..653=purse/bank above;
  654..657=totalTime 0; 658=rested false; 659..690/691..722=Clouds/Darkside game flags,
  723..738=world flags, 739..746=quest flags, 747..811=first 65 quest items, all zero.
  PTY ends inside 85 quest items: remaining 20, Darkside stock and character flags read as zero by truncation
  (`R/party.cpp:308,331,336,339,343`). Oracle accounts for every byte and absent tail; readers must accept 812 bytes.
  MM subsets: `M/formats/xeen/XeenGameFlagsFormat.h:17`, `XeenQuestFlagFormat.h:9`, `XeenQuestItemFormat.h:10`.
- Empty shop slots require regeneration of both sides' four shops (`R/party.cpp:114,1630`). Reuse
  `M/games/xeen/XeenMerchantGeneration.cpp:64` and `XeenActorApproach.cpp:278` there, once before publication.
  Stock uses the first gameplay draws from the seed, before map load (`R/saves.cpp:241`, `R/xeen.cpp:286`).
  Bank stays zero: no startup interest/day advance. Map-load draws use M48/M50 presentation RNG.
- `R/xeen.cpp:280` clears/loads the map; `R/interface.cpp:294` draws scene/party/buttons.
  `R/map.cpp:666,794,844,1129,1406` loads DAT/MOB/Events,
  HP (`R/map.cpp:524`) and night; `:509` draws monster frame, `:527` draws cosmetic `_effect3`.
  `R/xeen.cpp:327` checks arrival Events/treasure; neither cosmetic draw advances gameplay RNG in MMModern.
  MM: `M/games/xeen/XeenMapLoader.cpp:11,40`, `XeenVertigoWorld.cpp:19` there, `M/app/XeenGameplay.cpp:215`.
  Load fresh resource state: no disabled Events/objects, defeated actors, wall overrides or pending treasure.
  At `(18,4)`, no West/all-direction line-0 Event exists; the sole East line-0 Event is opcode 2 (DoorTextSml).

## Prepared mode and ownership decisions

1. Keep `--journey-region` Adventurer, seed handling and M44 inputs unchanged. Its camera is
   map 23 `(9,11)` West (`M/games/xeen/XeenJourneyContent.h:20`); initialization sets
   day 8, levels `3,3,3,4,3,3`, XP `1000,2000,1000,1000,2000,1000`, clears temporary
   fields/conditions and refills HP/SP (`M/games/xeen/XeenActorApproach.cpp:265,315`).
   No gold/bank/item injection (`M/app/XeenGameplay.cpp:158,162`): correct status prose at closure, not roadmap coordinates.
2. Application selects original/prepared construction; Party/Roster retain state ownership, with no saved start profile.
   Both starts share World/EventFlow/EncounterFlow, guards and cosmetic RNG; isolate prepared overrides from parsing.
3. Generalize fresh map-23 assumptions: camera-derived EVT and mainland sprite loop
   (`M/app/XeenGameplay.cpp:143,198`); bound EVT/mainland manifest (`M/app/XeenJourneyFlow.cpp:32,70`);
   outdoor bounds/EVT/geometry/(9,11) component, entry equality, 19-actor construction/validation
   (`M/games/xeen/XeenActorApproach.cpp:197,245,292`). Bind Vertigo EVT, city statistics and city sprites;
   still validate staged mainland. Keep detached preflight/atomic publication (`M/app/XeenJourneyFlow.cpp:54`).
   `xeenJourneyContent()` remains prepared/mainland policy: initializedMap, (9,11) anchor and Run destination;
   never use it as the new-game camera or initialize prepared play then teleport/undo bonuses.
4. World stages 46 Vertigo/19 mainland actors in existing collections; only the current map is activated/classified.
   Use the indoor classifier; mainland preparation consumes no movement, Event, reward or gameplay RNG.
   Reuse `M/games/xeen/XeenVertigoWorld.cpp:10,19` and `M/app/XeenEventFlow.cpp:759`;
   ordinary entry/exit scripts remain responsible for later actor resets and overlays.
5. Audit missing state explicitly: prior maze, completion/death/win/loss/totalTime,
   full world/Darkside/character flags, remaining quest items, mutable map seen/stepped
   bits, Lloyd/current-spell/quick-fight fields are not complete live MMModern owners
   (`R/party.cpp:294,308,322,331,339`; `R/character.cpp:214,230,241`;
   `M/games/xeen/XeenSaveSnapshot.h:58,74`, `M/formats/xeen/XeenCharacterFormat.cpp:137`).
   Retain/test source-backed details/defaults; missing mutable state affecting play requires plan/save amendment, not discarded effects.
6. Difficulty audit across pinned `R/`: only gameplay rule is `R/combat.cpp:1725`, Adventurer +5 hit/x3 weapon damage,
   neither for Warrior; other references only declare, initialize, serialize or select/display difficulty.
   Pass party difficulty at both constructions: `M/games/xeen/XeenCombat.cpp:660` (melee),
   `M/app/XeenJourneyConsequences.cpp:104` (Shoot), to shared `XeenPhysicalPlayerCandidate`;
   condition `M/games/xeen/XeenCombatRules.cpp:301,310`, preserving damage order and RNG draws.
   Admit both enum values in `M/games/xeen/XeenGameplayContext.cpp:37` and `XeenJourneyRules.cpp:30`
   in that directory; reject invalid values. Keep prepared-start selection fixed to Adventurer.

## Entry/UI and reachable content

- Clouds title: New/Load/Credits/conditional ending; World adds Other Options (`R/worldofxeen/worldofxeen_menu.cpp:409,559`).
  New asks difficulty/loads resources; Load chooses a save (`:349`). The pin's ScummVM chooser (`R/saves.cpp:258`)
  does not establish DOS save/load visuals; use DOSBox evidence.
- **Approved temporary deviation (2026-10-06):** direct launch; the static party screen is removed.
  Exact usage (bare entry accepts the same options; `M/main.cpp:200`, `M/app/XeenGameplay.cpp:87`):
  `mmodern --new-game <game-dir> [--difficulty adventurer|warrior] [--save-file <path>]`
  `mmodern <game-dir> [--difficulty adventurer|warrior] [--save-file <path>]`
  `--combat-seed` is rejected for both new-game forms; retain it for prepared Journey, seed tests via services.
  Omission defaults to Adventurer (PTY[27]=0, `R/party.cpp:220`), also part of this deviation:
  the original dialog requires a choice (`R/dialogs/dialogs_difficulty.cpp:70`, `R/worldofxeen/worldofxeen_menu.cpp:356`).
  Reject invalid difficulty/overrides on Journey/load; restore saved difficulty. Keep F9/`--load-game`, no implicit target.
- Original title/difficulty/save-load dialogs are **the next milestone**. In-game Load is out: slot UI/live replacement
  exceed initialization (`R/dialogs/dialogs_control_panel.cpp:194,207`; `M/games/xeen/XeenSaveState.cpp:307`).
  At closure document direct entry/omitted-difficulty default in project-status, usage text and README.
- DAT traversal (`M/formats/xeen/XeenMapFormat.cpp:94`, `tests/XeenVertigoOriginal.cpp:102,124`) finds the same 424-cell
  component from `(18,4)`: `(17,4),(17,3),(16,3),(16,2),(16,1),(15,1),(15,0)` reaches
  the existing exit, then map 23 `(10,12)` South (`tests/XeenVertigoOriginal.cpp:147,163`).
  Whole Vertigo, existing barriers/services and mainland are reachable; level-1 survival needs gameplay/play-test.
- Keep “not supported yet” for map 22/other exits, Guild/Bank/Tavern/food, GiveMulti/chests, mirror/fountains/missing effects
  (`M/games/xeen/XeenEventInterpreter.cpp:143,479`; `XeenActorApproach.cpp:470` in that directory).
  Do not bypass these gaps, add areas or certify a per-cell route to make a test pass.

## Save, work order, tests and acceptance

- **Retain v6:** existing snapshot represents day/level 1, Vertigo camera/actors, mainland, food/flags/economy/RNG
  (`M/games/xeen/XeenSaveSnapshot.h:58,74`; `XeenSaveState.cpp:39,252` in that directory;
  `M/formats/xeen/XeenSaveFormat.h:27`). Difficulty already encodes/decodes 0/1 in
  `M/formats/xeen/XeenSaveFormat.cpp:333,462`; admitting Warrior needs no format change.
  If layout or meaning must change, stop, amend to v7 with one reader, reject v6 clearly.
- **Part A:** after explicit start, add resource/difficulty oracles, isolate prepared overrides, admit Warrior, publish owners/verify restore.
  **Part B:** wire public CLI/default entry, exercise gameplay/refusals and finish acceptance.
  Preserve prepared-mode draw order and lazy city initialization; no unrelated refactoring.
- Compare every PTY span above and all 30 CHR records before/after round-trip: items/books/supplements,
  Lloyd beacon (160..162,310), current spell/quick option (164..165,352..353), townUnknown/savedMaze (339..341)
  (`R/character.cpp:214,218,230,241,247`; `M/formats/xeen/XeenCharacterFormat.cpp:137`), including resource-backed details.
  Also active order, camera/calendar/food/purse/bank/flags/clean overlays/treasure, actors and generated stock.
  Compare stock against reference controlled draws; check prepared stats/calendar/camera/money/items/RNG too.
- Both difficulties: oracle exact first-frame view slots and activation for all 46 Vertigo records,
  notably record 35 (Slime at (15,4)); no mainland actor activated. Immediate F9 must round-trip exactly
  (`R/xeen.cpp:289,299`; `M/games/xeen/XeenSaveState.cpp:244,245`), without a preliminary move/pulse.
- Test both difficulties' melee/Shoot hit thresholds and weapon damage against reference draw tapes:
  +5/x3 only for Adventurer, before melee Might/resistance processing; unchanged draw order.
  Cover unarmed: x3 of zero weapon damage stays zero; Adventurer +5 still applies. Warrior new game must
  land real melee and Shoot hits proving neither +5 nor x3, with unchanged attack draw count for controlled hits.
  Test Warrior round-trip/CLI/continuation; replace `tests/XeenSaveFormatTests.cpp:366` rejection with acceptance/invalid-enum coverage.
- Short gameplay test turns East at (18,4) to exercise its city Event, then walks, fights, Rests, uses an affordable
  service, exits to mainland and returns; earn any required XP/damage through normal actions.
  Save-round-trip immediately/after play: identical re-save, exact RNG/actors, no regenerated stock or replay.
  Failed preparation leaves owners unpublished. Extend existing harnesses; no per-feature process witnesses.
- M44 digests in `tests/XeenM44BaselineDigests.h` must remain unchanged. Any difference
  stops work: reproduce baseline, compare complete M49 traces and every changed save byte,
  identify the first divergence and request approval; never silently update expected hashes.
- Iterate affected tests and `ctest --test-dir build-rel -L fast --output-on-failure`.
  After final build, one runner (Codex: `gpt-6-luna`, low, `fork_turns: none`; Claude Code: Haiku, low, no context) runs
  `tools/run-full-ctest.ps1 -BuildDir build-rel`, default single job; repeat once on `build-m44` at closure.
  Runner uses longest process waits; main freezes checkout/build, waits via maximum native completion wait without polls/progress messages.
  Follow AGENTS.md rerun rules; report model/effort/command/wait method; unavailable runner leaves validation pending.
- Risks: indoor publication/guards, unintended actor/RNG changes, day-1 restock, low-level progression,
  missing persistent state and difficulty propagation. Stop on original ambiguity; ask the maintainer, no buffs.
- Acceptance: passing tests, independent implementation review, maintainer DOSBox comparison in both difficulties
  (initial state/view, walking/combat/Rest/services, exit/re-entry) and exact MMModern reload.
  At closure update Entry modes, usage and README; roadmap names title menu/save-load as next (maintainer decision).
  Update status/history and condense this plan. Planning needs diff/link checks only, no build/CTest.
