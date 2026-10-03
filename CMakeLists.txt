cmake_minimum_required(VERSION 3.20)

project(MMModern VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(SCUMMVM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../scummvm-master"
	CACHE PATH "Path to the ScummVM source tree")
set(SCUMMVM_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build-xeen-probe-sdl"
	CACHE PATH "Path to the configured minimal ScummVM build tree")

find_package(SDL2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(mmodern_core INTERFACE)
target_include_directories(mmodern_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ScummVmXeen.cmake")
add_mmodern_scummvm_xeen_bridge(mmodern_scummvm_xeen)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/MMModernItemCatalog.cmake")
add_mmodern_item_catalog_generation()

add_library(mmodern_xeen STATIC
	src/formats/xeen/XeenMonsterFormat.cpp
	src/formats/xeen/XeenGameplayContextFormat.cpp
	src/games/xeen/XeenActorApproach.cpp
    src/games/xeen/XeenCombat.cpp
    src/games/xeen/XeenCombatRandom.cpp
	src/games/xeen/XeenCombatRules.cpp
	src/games/xeen/XeenMonsterTreasure.cpp
	src/games/xeen/XeenServiceEconomy.cpp
	src/games/xeen/XeenMerchantGeneration.cpp
	src/games/xeen/XeenServiceDay.cpp
	src/games/xeen/XeenTraining.cpp
	src/games/xeen/XeenTempleHeal.cpp
	src/games/xeen/XeenEquipmentPurchase.cpp
	src/games/xeen/XeenJourneyRules.cpp
	src/games/xeen/XeenGameplayContext.cpp
	src/games/xeen/XeenRegionalRules.cpp
	src/formats/xeen/XeenAssetSource.cpp
	src/formats/xeen/XeenCloudsVisualMetadata.cpp
	src/formats/xeen/XeenMaterialNames.cpp
	src/games/xeen/XeenObjectVisual.cpp
	src/formats/xeen/XeenCharacterFormat.cpp
	src/formats/xeen/XeenQuestItemFormat.cpp
	src/formats/xeen/XeenQuestFlagFormat.cpp
	src/formats/xeen/XeenSaveFormat.cpp
	src/games/xeen/XeenSaveState.cpp
	src/formats/xeen/XeenEventFormat.cpp
	src/formats/xeen/XeenEventTextFormat.cpp
	src/formats/xeen/XeenFontFormat.cpp
	src/formats/xeen/XeenGameFlagsFormat.cpp
	src/formats/xeen/XeenMapFormat.cpp
	src/games/xeen/XeenCharacter.cpp
	src/games/xeen/XeenCharacterRules.cpp
	src/games/xeen/XeenLearnedSpellRules.cpp
	src/games/xeen/XeenEventDecoder.cpp
	src/games/xeen/XeenEventDiagnostics.cpp
	src/games/xeen/XeenEventInterpreter.cpp
	src/games/xeen/XeenItemRewards.cpp
	src/games/xeen/XeenItemCatalog.cpp
	src/games/xeen/XeenItemTransfer.cpp
	src/games/xeen/XeenAntidoteUse.cpp
	src/games/xeen/XeenEquipment.cpp
	src/games/xeen/XeenInventoryView.cpp
	src/games/xeen/XeenEventPresenter.cpp
	src/games/xeen/XeenEventLoader.cpp
	src/games/xeen/XeenEventTextLoader.cpp
	src/games/xeen/XeenEventScript.cpp
	src/games/xeen/XeenEventSystem.cpp
	src/games/xeen/XeenEventTrigger.cpp
	src/games/xeen/XeenTextRenderer.cpp
	src/games/xeen/XeenGameFlags.cpp
	src/games/xeen/XeenGameFlagsLoader.cpp
	src/games/xeen/XeenPartyVisualState.cpp
	src/games/xeen/XeenParty.cpp
	src/games/xeen/XeenPartyLoader.cpp
	src/games/xeen/XeenMapLoader.cpp
	src/games/xeen/CloudsUiComposer.cpp
	src/games/xeen/CloudsUiLayout.cpp
	src/games/xeen/CloudsMapComposer.cpp
	src/games/xeen/XeenOutdoorScene.cpp
	src/games/xeen/XeenIndoorScene.cpp
	src/games/xeen/XeenMovement.cpp
	src/games/xeen/XeenWorld.cpp
	src/games/xeen/XeenVertigoWorld.cpp
	src/games/xeen/XeenVertigoRoute.cpp
	src/games/xeen/XeenInstallationDetector.cpp
)
target_include_directories(mmodern_xeen PRIVATE "${MMODERN_ITEM_CATALOG_GENERATED_DIR}")
add_dependencies(mmodern_xeen mmodern_item_catalog_data)
target_link_libraries(mmodern_xeen PUBLIC mmodern_core PRIVATE mmodern_scummvm_xeen ZLIB::ZLIB)

add_library(mmodern_platform_sdl STATIC
	src/platform/sdl/SdlWindow.cpp
)
target_link_libraries(mmodern_platform_sdl PUBLIC mmodern_core PRIVATE SDL2::SDL2)

add_library(mmodern_app STATIC
	src/platform/XeenSaveFile.cpp
	src/app/XeenGameplay.cpp
	src/app/XeenEncounterFlow.cpp
	src/app/XeenJourneyFlow.cpp
	src/app/XeenJourneyConsequences.cpp
	src/app/XeenCastingFlow.cpp
	src/app/XeenSmithFlow.cpp
	src/app/XeenTrainingFlow.cpp
	src/app/XeenCastingUi.cpp
	src/app/Application.cpp
	src/app/XeenEventFlow.cpp
	src/app/XeenInventoryFlow.cpp
	src/app/XeenNavigationFlow.cpp
)
target_link_libraries(mmodern_app PUBLIC mmodern_core PRIVATE
	mmodern_xeen
	mmodern_platform_sdl
)

add_executable(mmodern src/main.cpp)
target_link_libraries(mmodern PRIVATE
	shell32
	mmodern_app
	mmodern_xeen
	mmodern_scummvm_xeen
	mmodern_platform_sdl
	SDL2::SDL2
)

if(MINGW)
	target_compile_options(mmodern PRIVATE -Wall -Wextra -Wpedantic)
	target_link_options(mmodern PRIVATE -static-libgcc -static-libstdc++ -Wl,--gc-sections)
endif()

include(CTest)
add_executable(mmodern_regional_original EXCLUDE_FROM_ALL tests/XeenRegionalOriginal.cpp)
target_link_libraries(mmodern_regional_original PRIVATE mmodern_app mmodern_xeen)
add_executable(mmodern_regional_event_original EXCLUDE_FROM_ALL tests/XeenRegionalEventOriginal.cpp tests/XeenRestoreReplayProbe.cpp)
target_link_libraries(mmodern_regional_event_original PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
add_executable(mmodern_vertigo_original tests/XeenVertigoOriginal.cpp)
target_link_libraries(mmodern_vertigo_original PRIVATE mmodern_app mmodern_xeen)
function(xeen_restore_replay target)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/XeenRestoreReplayProbe.h")
	file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/tests/XeenRestoreReplayProbe.h" xeen_replay_symbols REGEX "^#define XEEN_REPLAY_")
	foreach(xeen_replay_definition IN LISTS xeen_replay_symbols)
		string(REGEX MATCH "\"([^\"]+)\"" xeen_replay_match "${xeen_replay_definition}")
		target_link_options(${target} PRIVATE "LINKER:--wrap,${CMAKE_MATCH_1}")
	endforeach()
endfunction()
xeen_restore_replay(mmodern_regional_event_original)
if(BUILD_TESTING)
	add_executable(mmodern_equipment_purchase_rules_tests tests/XeenEquipmentPurchaseRulesTests.cpp)
	target_link_libraries(mmodern_equipment_purchase_rules_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_equipment_purchase_rules COMMAND mmodern_equipment_purchase_rules_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_equipment_purchase_rules PROPERTIES TIMEOUT 300)
	add_test(NAME xeen_equipment_purchase_reference COMMAND mmodern_equipment_purchase_rules_tests "F:/Games/gog/Might and Magic 4-5" --reference "${SCUMMVM_SOURCE_DIR}")
	set_tests_properties(xeen_equipment_purchase_reference PROPERTIES TIMEOUT 300)
	add_executable(mmodern_purchase_flow_tests tests/XeenPurchaseFlowTests.cpp)
	target_link_libraries(mmodern_purchase_flow_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_purchase_flow COMMAND mmodern_purchase_flow_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_purchase_flow PROPERTIES TIMEOUT 300)
	add_executable(mmodern_purchase_allocation_tests tests/XeenPurchaseAllocationTests.cpp)
	target_link_libraries(mmodern_purchase_allocation_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_purchase_allocation COMMAND mmodern_purchase_allocation_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_purchase_allocation PROPERTIES TIMEOUT 600)
	add_executable(mmodern_purchase_input_tests tests/XeenPurchaseInputTests.cpp)
	target_link_libraries(mmodern_purchase_input_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_purchase_input_tests PRIVATE "LINKER:--wrap,SDL_GetTicks")
	foreach(mode authority native controls)
		add_test(NAME xeen_purchase_input_${mode} COMMAND mmodern_purchase_input_tests "F:/Games/gog/Might and Magic 4-5" ${mode})
		set_tests_properties(xeen_purchase_input_${mode} PROPERTIES TIMEOUT 120)
	endforeach()
	add_executable(mmodern_training_rules_tests tests/XeenTrainingRulesTests.cpp)
	add_executable(mmodern_temple_flow_tests tests/XeenTempleFlowTests.cpp)
	add_executable(mmodern_temple_rules_tests tests/XeenTempleRulesTests.cpp)
	target_link_libraries(mmodern_temple_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_temple_rules COMMAND mmodern_temple_rules_tests)
	target_link_libraries(mmodern_temple_flow_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_temple_flow COMMAND mmodern_temple_flow_tests "F:/Games/gog/Might and Magic 4-5")
	add_executable(mmodern_temple_original_tests tests/XeenTempleOriginalTests.cpp)
	target_link_libraries(mmodern_temple_original_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_temple_original COMMAND mmodern_temple_original_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_temple_original PROPERTIES TIMEOUT 600)
	add_executable(mmodern_m43_cli_witness src/main.cpp tests/XeenM43CliWitness.cpp)
	target_link_libraries(mmodern_m43_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32 bcrypt)
	target_link_options(mmodern_m43_cli_witness PRIVATE
		"LINKER:--wrap,_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
		"LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_executable(mmodern_m43_process_tests tests/XeenM43ProcessTests.cpp)
	target_link_libraries(mmodern_m43_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl bcrypt)
	add_dependencies(mmodern_m43_process_tests mmodern_m43_cli_witness)
	add_test(NAME xeen_temple_process COMMAND mmodern_m43_process_tests $<TARGET_FILE:mmodern_m43_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_temple_process PROPERTIES TIMEOUT 600)
	add_executable(mmodern_m43_depleted_process_tests tests/XeenM43DepletedProcessTests.cpp)
	target_link_libraries(mmodern_m43_depleted_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl bcrypt)
	add_dependencies(mmodern_m43_depleted_process_tests mmodern_m42_cli_witness mmodern_m43_cli_witness)
	add_test(NAME xeen_temple_depleted_process COMMAND mmodern_m43_depleted_process_tests
		$<TARGET_FILE:mmodern_m42_cli_witness> $<TARGET_FILE:mmodern_m43_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_temple_depleted_process PROPERTIES TIMEOUT 600)
	target_link_libraries(mmodern_training_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_training_rules COMMAND mmodern_training_rules_tests)
	add_executable(mmodern_training_flow_tests tests/XeenTrainingFlowTests.cpp)
	target_link_libraries(mmodern_training_flow_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_training_flow COMMAND mmodern_training_flow_tests "F:/Games/gog/Might and Magic 4-5")
	add_executable(mmodern_training_allocation_tests tests/XeenTrainingAllocationTests.cpp)
	target_link_libraries(mmodern_training_allocation_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_training_allocation COMMAND mmodern_training_allocation_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_training_allocation PROPERTIES TIMEOUT 300)
	add_executable(mmodern_training_input_tests tests/XeenTrainingInputTests.cpp)
	add_executable(mmodern_native_responsiveness_tests tests/XeenNativeResponsivenessTests.cpp)
	target_link_libraries(mmodern_native_responsiveness_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_native_responsiveness_tests PRIVATE "LINKER:--wrap,SDL_GetTicks" "LINKER:--wrap,SDL_PumpEvents" "LINKER:--wrap,SDL_WaitEventTimeout")
	add_test(NAME xeen_native_responsiveness COMMAND mmodern_native_responsiveness_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_native_responsiveness PROPERTIES TIMEOUT 60)
	add_executable(mmodern_input_scheduling_tests tests/XeenInputSchedulingTests.cpp)
	target_link_libraries(mmodern_input_scheduling_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_input_scheduling_tests PRIVATE "LINKER:--wrap,SDL_UpdateTexture" "LINKER:--wrap,SDL_RenderCopy")
	add_test(NAME xeen_input_scheduling COMMAND mmodern_input_scheduling_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_input_scheduling PROPERTIES TIMEOUT 120)
	target_link_libraries(mmodern_training_input_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_training_input_tests PRIVATE "LINKER:--wrap,SDL_GetTicks")
	foreach(mode authority native controls)
		add_test(NAME xeen_training_input_${mode} COMMAND mmodern_training_input_tests "F:/Games/gog/Might and Magic 4-5" ${mode})
		set_tests_properties(xeen_training_input_${mode} PROPERTIES TIMEOUT 60)
	endforeach()
	add_executable(mmodern_training_original_tests tests/XeenTrainingOriginalTests.cpp)
	target_link_libraries(mmodern_training_original_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_training_original COMMAND mmodern_training_original_tests "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_training_original PROPERTIES TIMEOUT 600)
	add_executable(mmodern_m41_cli_witness src/main.cpp tests/XeenM41CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m41_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32 bcrypt)
	target_link_options(mmodern_m41_cli_witness PRIVATE "LINKER:--wrap,_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
		"LINKER:--wrap,_ZN7mmodern23xeenPrepareBankInterestERKNS_16XeenBankBalancesE")
	xeen_restore_replay(mmodern_m41_cli_witness)
	target_link_options(mmodern_m41_cli_witness PRIVATE "LINKER:--wrap,SDL_UpdateTexture" "LINKER:--wrap,SDL_RenderCopy")
	add_executable(mmodern_training_process_tests tests/XeenTrainingProcessTests.cpp)
	target_link_libraries(mmodern_training_process_tests PRIVATE mmodern_app mmodern_xeen bcrypt)
	add_dependencies(mmodern_training_process_tests mmodern_m41_cli_witness)
	add_test(NAME xeen_training_process COMMAND mmodern_training_process_tests $<TARGET_FILE:mmodern_m41_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_training_process PROPERTIES TIMEOUT 600)
	add_executable(mmodern_m42_cli_witness src/main.cpp tests/XeenM42CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m42_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32 bcrypt)
	target_link_options(mmodern_m42_cli_witness PRIVATE "LINKER:--wrap,_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
		"LINKER:--wrap,_ZN7mmodern23xeenPrepareBankInterestERKNS_16XeenBankBalancesE"
		"LINKER:--wrap,_ZN7mmodern24XeenEnemyAttackCandidateC1ERKSt5arrayINS_13XeenCharacterELy6EERKS1_INS_16XeenCombatInputsELy6EERKNS_17XeenMonsterRecordEjjRKS1_IbLy6EE"
		"LINKER:--wrap,_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjb")
	xeen_restore_replay(mmodern_m42_cli_witness)
	target_link_options(mmodern_m42_cli_witness PRIVATE "LINKER:--wrap,SDL_UpdateTexture" "LINKER:--wrap,SDL_RenderCopy")
	add_executable(mmodern_purchase_process_tests tests/XeenPurchaseProcessTests.cpp)
	target_link_libraries(mmodern_purchase_process_tests PRIVATE mmodern_app mmodern_xeen bcrypt)
	add_dependencies(mmodern_purchase_process_tests mmodern_m42_cli_witness)
	add_test(NAME xeen_purchase_process COMMAND mmodern_purchase_process_tests $<TARGET_FILE:mmodern_m42_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_purchase_process PROPERTIES TIMEOUT 1200)
	add_executable(mmodern_service_economy_tests tests/XeenServiceEconomyTests.cpp)
	target_link_libraries(mmodern_service_economy_tests PRIVATE mmodern_xeen bcrypt)
	add_test(NAME xeen_service_economy COMMAND mmodern_service_economy_tests)
	add_executable(mmodern_service_day_initialization_tests tests/XeenServiceDayInitializationTests.cpp)
	target_link_libraries(mmodern_service_day_initialization_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_service_day_initialization COMMAND mmodern_service_day_initialization_tests "F:/Games/gog/Might and Magic 4-5")
	add_executable(mmodern_m40_cli_witness src/main.cpp tests/XeenM40CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m40_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32 bcrypt)
	target_link_options(mmodern_m40_cli_witness PRIVATE "LINKER:--wrap,_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
		"LINKER:--wrap,_ZN7mmodern23xeenPrepareBankInterestERKNS_16XeenBankBalancesE")
	xeen_restore_replay(mmodern_m40_cli_witness)
	target_link_options(mmodern_m40_cli_witness PRIVATE "LINKER:--wrap,SDL_UpdateTexture" "LINKER:--wrap,SDL_RenderCopy")
	add_executable(mmodern_service_day_process_tests tests/XeenServiceDayProcessTests.cpp)
	target_link_libraries(mmodern_service_day_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl bcrypt)
	add_dependencies(mmodern_service_day_process_tests mmodern_m40_cli_witness)
	add_test(NAME xeen_service_day_process COMMAND mmodern_service_day_process_tests $<TARGET_FILE:mmodern_m40_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_service_day_process PROPERTIES TIMEOUT 600)
	add_executable(mmodern_combat_clock_process_tests tests/XeenCombatClockProcessTests.cpp)
	target_link_libraries(mmodern_combat_clock_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl bcrypt)
	add_dependencies(mmodern_combat_clock_process_tests mmodern_m40_cli_witness)
	add_test(NAME xeen_combat_clock_process COMMAND mmodern_combat_clock_process_tests $<TARGET_FILE:mmodern_m40_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_combat_clock_process PROPERTIES TIMEOUT 300)
	add_test(NAME xeen_vertigo_original COMMAND mmodern_vertigo_original "F:/Games/gog/Might and Magic 4-5")
	add_executable(mmodern_journey_resource_tests tests/XeenJourneyResourceTests.cpp)
	target_link_libraries(mmodern_journey_resource_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_journey_resources COMMAND mmodern_journey_resource_tests)
	add_executable(mmodern_regional_persistence_tests tests/XeenRegionalPersistenceTests.cpp)
	target_link_libraries(mmodern_regional_persistence_tests PRIVATE mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_regional_persistence COMMAND mmodern_regional_persistence_tests)
	add_executable(mmodern_mutation_tests tests/XeenMutationTests.cpp)
	target_link_libraries(mmodern_mutation_tests PRIVATE mmodern_app mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_mutation_boundary COMMAND mmodern_mutation_tests)
	add_executable(mmodern_mutation_value_tests tests/XeenMutationValueTests.cpp)
	target_link_libraries(mmodern_mutation_value_tests PRIVATE mmodern_core)
	add_test(NAME xeen_mutation_values COMMAND mmodern_mutation_value_tests)
	add_executable(mmodern_antidote_rules_tests tests/XeenAntidoteRulesTests.cpp)
	target_link_libraries(mmodern_antidote_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_antidote_rules COMMAND mmodern_antidote_rules_tests)
	add_executable(mmodern_consequence_original EXCLUDE_FROM_ALL tests/XeenConsequenceOriginal.cpp)
	target_link_libraries(mmodern_consequence_original PRIVATE mmodern_app SDL2::SDL2)
	target_link_options(mmodern_consequence_original PRIVATE "-Wl,--wrap=_ZN7mmodern16XeenCombatRandom4drawEjj"
        "-Wl,--wrap=_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjb")
	add_executable(mmodern_resource_cold_original tests/XeenConsequenceOriginal.cpp)
	target_compile_definitions(mmodern_resource_cold_original PRIVATE MMODERN_M45_COLD_CACHE)
	target_link_libraries(mmodern_resource_cold_original PRIVATE mmodern_app SDL2::SDL2)
	target_link_options(mmodern_resource_cold_original PRIVATE "-Wl,--wrap=_ZN7mmodern16XeenCombatRandom4drawEjj"
        "-Wl,--wrap=_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjb")
	add_test(NAME xeen_resource_cold_original COMMAND mmodern_resource_cold_original "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_resource_cold_original PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
    add_executable(mmodern_consequence_sprite_tests tests/XeenConsequenceSpriteTests.cpp)
    target_link_libraries(mmodern_consequence_sprite_tests PRIVATE mmodern_xeen)
    add_test(NAME xeen_consequence_sprites COMMAND mmodern_consequence_sprite_tests)
	add_executable(mmodern_consequence_rules_tests tests/XeenConsequenceRulesTests.cpp)
	target_link_libraries(mmodern_consequence_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_consequence_rules COMMAND mmodern_consequence_rules_tests)
	add_executable(mmodern_regional_rules_tests tests/XeenRegionalRulesTests.cpp)
	target_link_libraries(mmodern_regional_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_regional_rules COMMAND mmodern_regional_rules_tests)
	add_executable(mmodern_journey_event_tests tests/XeenJourneyEventTests.cpp)
	target_link_libraries(mmodern_journey_event_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_test(NAME xeen_journey_event COMMAND mmodern_journey_event_tests)
	set_tests_properties(xeen_journey_event PROPERTIES TIMEOUT 45)
	add_executable(mmodern_expedition_gameplay_tests tests/XeenExpeditionGameplayTests.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_expedition_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	xeen_restore_replay(mmodern_expedition_gameplay_tests)
	add_test(NAME xeen_expedition_gameplay COMMAND mmodern_expedition_gameplay_tests)
	set_tests_properties(xeen_expedition_gameplay PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 45)
	add_executable(mmodern_expedition_tests tests/XeenExpeditionTests.cpp)
	target_link_libraries(mmodern_expedition_tests PRIVATE mmodern_app mmodern_xeen)
	target_link_options(mmodern_expedition_tests PRIVATE "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_test(NAME xeen_expedition COMMAND mmodern_expedition_tests)
	add_executable(mmodern_journey_tests tests/XeenJourneyTests.cpp)
	target_link_libraries(mmodern_journey_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_journey COMMAND mmodern_journey_tests)
	add_executable(mmodern_journey_gameplay_tests tests/XeenJourneyGameplayTests.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_journey_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_test(NAME xeen_journey_gameplay COMMAND mmodern_journey_gameplay_tests)
	add_test(NAME xeen_journey_sdl COMMAND mmodern_journey_gameplay_tests sdl)
	set_tests_properties(xeen_journey_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 30)
	xeen_restore_replay(mmodern_journey_gameplay_tests)
	add_executable(mmodern_consequence_cli_witness EXCLUDE_FROM_ALL src/main.cpp tests/XeenConsequenceCliWitness.cpp tests/XeenM35CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_consequence_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32)
	target_link_options(mmodern_consequence_cli_witness PRIVATE "-Wl,--wrap=_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE")
	xeen_restore_replay(mmodern_consequence_cli_witness)
	add_executable(mmodern_m36_cli_witness EXCLUDE_FROM_ALL src/main.cpp tests/XeenM36CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	add_executable(mmodern_m39_cli_witness src/main.cpp tests/XeenM39CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m39_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32)
	target_link_options(mmodern_m39_cli_witness PRIVATE "-Wl,--wrap=_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE")
	target_link_options(mmodern_m39_cli_witness PRIVATE
        "-Wl,--wrap=_ZN7mmodern10XeenCombat9beginCastERNS0_12CastResponseERKSt8functionIFNS_21XeenLearnedSpellNamesEvEE"
        "-Wl,--wrap=_ZN7mmodern10XeenCombat11respondCastERNS0_12CastResponseENS_19XeenCombatCastInputEjRKSt8functionIFNS_21XeenLearnedSpellNamesEvEE"
        "-Wl,--wrap=_ZN7mmodern10XeenCombat11serviceCastERKNS0_6TicketEy")
	target_link_options(mmodern_m39_cli_witness PRIVATE "-Wl,--wrap=SDL_UpdateTexture" "-Wl,--wrap=SDL_RenderCopy")
	target_link_options(mmodern_m39_cli_witness PRIVATE "-Wl,--wrap=SDL_FilterEvents" "-Wl,--wrap=SDL_PollEvent" "-Wl,--wrap=SDL_WaitEventTimeout")
	xeen_restore_replay(mmodern_m39_cli_witness)
	add_executable(mmodern_combat_casting_process_tests tests/XeenCombatCastingProcessTests.cpp)
	target_link_libraries(mmodern_combat_casting_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl)
	add_dependencies(mmodern_combat_casting_process_tests mmodern_m39_cli_witness)
	add_test(NAME xeen_combat_casting_process COMMAND mmodern_combat_casting_process_tests $<TARGET_FILE:mmodern_m39_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_combat_casting_process PROPERTIES TIMEOUT 300)
	add_executable(mmodern_combat_casting_control_tests tests/XeenCombatCastingControlTests.cpp)
    target_link_libraries(mmodern_combat_casting_control_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl)
    add_dependencies(mmodern_combat_casting_control_tests mmodern_m39_cli_witness)
    add_test(NAME xeen_combat_casting_controls COMMAND mmodern_combat_casting_control_tests $<TARGET_FILE:mmodern_m39_cli_witness> "F:/Games/gog/Might and Magic 4-5")
    set_tests_properties(xeen_combat_casting_controls PROPERTIES TIMEOUT 180)
    add_test(NAME xeen_combat_player_ready_input COMMAND mmodern_combat_casting_control_tests $<TARGET_FILE:mmodern_m39_cli_witness> "F:/Games/gog/Might and Magic 4-5")
    set_tests_properties(xeen_combat_player_ready_input PROPERTIES TIMEOUT 60 ENVIRONMENT "MMODERN_M39_ONLY_INPUT=1")
	add_executable(mmodern_m38_cli_witness src/main.cpp tests/XeenM38CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m38_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32)
	target_link_options(mmodern_m38_cli_witness PRIVATE "-Wl,--wrap=_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE")
	target_link_options(mmodern_m38_cli_witness PRIVATE "-Wl,--wrap=SDL_UpdateTexture" "-Wl,--wrap=SDL_RenderCopy")
	xeen_restore_replay(mmodern_m38_cli_witness)
	add_executable(mmodern_smith_process_tests tests/XeenSmithProcessTests.cpp)
	target_link_libraries(mmodern_smith_process_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl)
	add_dependencies(mmodern_smith_process_tests mmodern_m38_cli_witness)
	add_test(NAME xeen_smith_process COMMAND mmodern_smith_process_tests $<TARGET_FILE:mmodern_m38_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_smith_process PROPERTIES TIMEOUT 300)
	add_executable(mmodern_m37_cli_witness src/main.cpp tests/XeenM37CliWitness.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_m37_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32)
	target_link_options(mmodern_m37_cli_witness PRIVATE "-Wl,--wrap=_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE")
	xeen_restore_replay(mmodern_m37_cli_witness)
	add_executable(mmodern_vertigo_process_tests tests/XeenVertigoProcessTests.cpp)
	target_link_libraries(mmodern_vertigo_process_tests PRIVATE mmodern_app mmodern_xeen)
	add_dependencies(mmodern_vertigo_process_tests mmodern_m37_cli_witness)
	add_test(NAME xeen_vertigo_process COMMAND mmodern_vertigo_process_tests $<TARGET_FILE:mmodern_m37_cli_witness> "F:/Games/gog/Might and Magic 4-5")
	set_tests_properties(xeen_vertigo_process PROPERTIES TIMEOUT 150)
	target_link_libraries(mmodern_m36_cli_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32)
	target_link_options(mmodern_m36_cli_witness PRIVATE "-Wl,--wrap=_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE")
	target_link_options(mmodern_m36_cli_witness PRIVATE
		"-Wl,--wrap=_ZN7mmodern17XeenEncounterFlow14confirmCastingERKNS0_6TicketEyyyRKSt10shared_ptrIKNS_12IndexedFrameEE"
		"-Wl,--wrap=_ZN7mmodern17XeenEncounterFlow20respondCastingTargetERKNS0_6TicketESt8optionalIyEyRKSt10shared_ptrIKNS_12IndexedFrameEE"
		"-Wl,--wrap=_ZN7mmodern17XeenEncounterFlow13publishAwakenERKNS0_6TicketE"
		"-Wl,--wrap=_ZN7mmodern17XeenEncounterFlow14serviceCastingEv")
	xeen_restore_replay(mmodern_m36_cli_witness)
	add_executable(mmodern_combat_tests tests/XeenCombatTests.cpp)
	add_executable(mmodern_combat_gameplay_tests tests/XeenCombatGameplayTests.cpp)
	target_link_libraries(mmodern_combat_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_combat_gameplay_tests PRIVATE "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_test(NAME xeen_combat_gameplay COMMAND mmodern_combat_gameplay_tests)
	add_test(NAME xeen_combat_sdl COMMAND mmodern_combat_gameplay_tests sdl)
	set_tests_properties(xeen_combat_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	add_test(NAME xeen_combat_cli COMMAND mmodern_combat_gameplay_tests cli $<TARGET_FILE:mmodern>)
	target_link_libraries(mmodern_combat_tests PRIVATE mmodern_app mmodern_xeen)
	target_link_options(mmodern_combat_tests PRIVATE "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_test(NAME xeen_combat COMMAND mmodern_combat_tests)
	add_executable(mmodern_combat_authority_tests tests/XeenCombatAuthorityTests.cpp)
	target_link_libraries(mmodern_combat_authority_tests PRIVATE mmodern_app mmodern_xeen)
	target_link_options(mmodern_combat_authority_tests PRIVATE "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_test(NAME xeen_combat_authority COMMAND mmodern_combat_authority_tests)
	add_executable(mmodern_combat_persistence_tests tests/XeenCombatPersistenceTests.cpp)
	target_link_libraries(mmodern_combat_persistence_tests PRIVATE mmodern_app mmodern_xeen)
	target_link_options(mmodern_combat_persistence_tests PRIVATE "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_test(NAME xeen_combat_persistence COMMAND mmodern_combat_persistence_tests)
	add_executable(mmodern_resource_admission_tests tests/XeenResourceAdmissionTests.cpp)
	target_link_libraries(mmodern_resource_admission_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_resource_admission COMMAND mmodern_resource_admission_tests)
	add_executable(mmodern_completed_restore_tests tests/XeenCompletedRestoreTests.cpp tests/XeenRestoreReplayProbe.cpp)
	add_executable(mmodern_journey_persistence_tests tests/XeenJourneyPersistenceTests.cpp tests/XeenRestoreReplayProbe.cpp)
	target_link_libraries(mmodern_journey_persistence_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_journey_persistence COMMAND mmodern_journey_persistence_tests)
	target_link_libraries(mmodern_completed_restore_tests PRIVATE mmodern_app mmodern_xeen)
	xeen_restore_replay(mmodern_completed_restore_tests)
	xeen_restore_replay(mmodern_journey_persistence_tests)
	add_test(NAME xeen_completed_restore COMMAND mmodern_completed_restore_tests)
	add_executable(mmodern_encounter_flow_tests tests/XeenEncounterFlowTests.cpp)
	target_link_libraries(mmodern_encounter_flow_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_encounter_flow COMMAND mmodern_encounter_flow_tests)
	add_executable(mmodern_encounter_gameplay_tests tests/XeenEncounterGameplayTests.cpp)
	target_link_libraries(mmodern_encounter_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_test(NAME xeen_encounter_gameplay COMMAND mmodern_encounter_gameplay_tests)
	set_tests_properties(xeen_encounter_gameplay PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
	add_executable(mmodern_monster_format_tests tests/XeenMonsterFormatTests.cpp)
	target_link_libraries(mmodern_monster_format_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_monster_format COMMAND mmodern_monster_format_tests)
	add_executable(mmodern_actor_approach_tests tests/XeenActorApproachTests.cpp)
	target_link_libraries(mmodern_actor_approach_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_actor_approach COMMAND mmodern_actor_approach_tests)
	add_executable(mmodern_encounter_save_tests tests/XeenEncounterSaveTests.cpp)
	target_link_libraries(mmodern_encounter_save_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_encounter_save COMMAND mmodern_encounter_save_tests)
	add_executable(mmodern_item_transfer_tests tests/XeenItemTransferTests.cpp)
	target_link_libraries(mmodern_item_transfer_tests PRIVATE mmodern_app mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_item_transfer COMMAND mmodern_item_transfer_tests)
	add_executable(mmodern_inventory_gameplay_tests tests/XeenInventoryGameplayTests.cpp)
	target_link_libraries(mmodern_inventory_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	add_test(NAME xeen_inventory_gameplay COMMAND mmodern_inventory_gameplay_tests)
	add_test(NAME xeen_inventory_sdl COMMAND mmodern_inventory_gameplay_tests sdl)
	set_tests_properties(xeen_inventory_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	add_executable(mmodern_equipment_gameplay_tests tests/XeenEquipmentGameplayTests.cpp)
	target_link_libraries(mmodern_equipment_gameplay_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	add_test(NAME xeen_equipment_gameplay COMMAND mmodern_equipment_gameplay_tests)
	add_executable(mmodern_item_catalog_tests tests/XeenItemCatalogTests.cpp)
	target_link_libraries(mmodern_item_catalog_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_item_catalog COMMAND mmodern_item_catalog_tests)
	add_executable(mmodern_item_catalog_asset_tests tests/XeenItemCatalogAssetTests.cpp)
	target_link_libraries(mmodern_item_catalog_asset_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_item_catalog_assets COMMAND mmodern_item_catalog_asset_tests)
	add_test(NAME xeen_item_catalog_generation
		COMMAND "${MMODERN_ITEM_CATALOG_POWERSHELL}"
			-NoProfile -NonInteractive -ExecutionPolicy Bypass
			-File "${CMAKE_CURRENT_SOURCE_DIR}/tests/XeenItemCatalogGenerationTests.ps1"
			-Generator "${MMODERN_ITEM_CATALOG_GENERATOR}"
			-TestPowerShell "${MMODERN_ITEM_CATALOG_POWERSHELL}"
			-Git "${GIT_EXECUTABLE}"
			-TestRoot "${CMAKE_CURRENT_BINARY_DIR}/item-catalog-generation-tests"
			-ProductionOutput "${MMODERN_ITEM_CATALOG_GENERATED_DIR}/XeenItemCatalogEnglish.inc"
			-ProductionOutputSha256 "${MMODERN_ITEM_CATALOG_GENERATED_SHA256}")
	add_executable(mmodern_outdoor_animation_tests tests/XeenOutdoorAnimationTests.cpp)
	target_link_libraries(mmodern_outdoor_animation_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	add_test(NAME xeen_outdoor_animation COMMAND mmodern_outdoor_animation_tests)
	add_test(NAME xeen_outdoor_animation_sdl COMMAND mmodern_outdoor_animation_tests sdl)
	set_tests_properties(xeen_outdoor_animation_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	foreach(suite item_reward reward_execution reward_flow reward_gameplay)
		if(suite STREQUAL "item_reward")
			set(test_source ItemReward)
		elseif(suite STREQUAL "reward_execution")
			set(test_source RewardExecution)
		elseif(suite STREQUAL "reward_flow")
			set(test_source RewardFlow)
		else()
			set(test_source RewardGameplay)
		endif()
		add_executable(mmodern_${suite}_tests tests/Xeen${test_source}Tests.cpp)
		target_link_libraries(mmodern_${suite}_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
		add_test(NAME xeen_${suite} COMMAND mmodern_${suite}_tests)
	endforeach()
	add_test(NAME xeen_reward_sdl COMMAND mmodern_reward_gameplay_tests sdl)
	set_tests_properties(xeen_reward_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	add_executable(mmodern_save_resume_smoke EXCLUDE_FROM_ALL tests/SaveResumeIntegrationTest.cpp)
	target_link_libraries(mmodern_save_resume_smoke PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	add_dependencies(mmodern_save_resume_smoke mmodern)
	add_executable(mmodern_save_cli_tests tests/XeenSaveCliTests.cpp)
	target_link_libraries(mmodern_save_cli_tests PRIVATE mmodern_app mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_save_cli COMMAND mmodern_save_cli_tests $<TARGET_FILE:mmodern>)
	set_tests_properties(xeen_save_cli PROPERTIES TIMEOUT 60)
	add_executable(mmodern_save_sdl_tests tests/XeenSaveSdlTests.cpp)
	target_link_libraries(mmodern_save_sdl_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	add_test(NAME xeen_save_sdl COMMAND mmodern_save_sdl_tests)
	set_tests_properties(xeen_save_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	add_executable(mmodern_save_file_tests tests/XeenSaveFileTests.cpp)
	target_link_libraries(mmodern_save_file_tests PRIVATE mmodern_app mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_save_file COMMAND mmodern_save_file_tests)
	add_executable(mmodern_save_flow_tests tests/XeenSaveFlowTests.cpp)
	target_link_libraries(mmodern_save_flow_tests PRIVATE mmodern_app mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_save_flow COMMAND mmodern_save_flow_tests)
	add_executable(mmodern_armor_repair_tests tests/XeenArmorRepairTests.cpp)
	target_link_libraries(mmodern_armor_repair_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_armor_repair COMMAND mmodern_armor_repair_tests)
	add_executable(mmodern_save_format_tests tests/XeenSaveFormatTests.cpp)
	target_link_libraries(mmodern_save_format_tests PRIVATE mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_save_format COMMAND mmodern_save_format_tests)
	add_executable(mmodern_save_state_tests tests/XeenSaveStateTests.cpp)
	target_link_libraries(mmodern_save_state_tests PRIVATE mmodern_app mmodern_xeen ZLIB::ZLIB)
	add_test(NAME xeen_save_state COMMAND mmodern_save_state_tests)
	add_executable(mmodern_quest_flag_tests tests/XeenQuestFlagTests.cpp)
	target_link_libraries(mmodern_quest_flag_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_quest_flags COMMAND mmodern_quest_flag_tests)
	add_executable(mmodern_npc_tests tests/XeenNpcTests.cpp)
	target_link_libraries(mmodern_npc_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_test(NAME xeen_npc COMMAND mmodern_npc_tests)
	add_test(NAME xeen_npc_sdl COMMAND mmodern_npc_tests sdl)
	set_tests_properties(xeen_npc_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 15)
	add_executable(mmodern_quest_grant_tests tests/XeenQuestGrantTests.cpp)
	add_executable(mmodern_who_will_tests tests/XeenWhoWillTests.cpp)
	target_link_libraries(mmodern_who_will_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_test(NAME xeen_who_will COMMAND mmodern_who_will_tests)
	add_test(NAME xeen_who_will_sdl COMMAND mmodern_who_will_tests sdl)
	set_tests_properties(xeen_who_will_sdl PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 20)
	target_link_libraries(mmodern_quest_grant_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_quest_grants COMMAND mmodern_quest_grant_tests)
	add_executable(mmodern_quest_item_tests tests/XeenQuestItemTests.cpp)
	target_link_libraries(mmodern_quest_item_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_quest_items COMMAND mmodern_quest_item_tests)
	add_executable(mmodern_phirna_smoke EXCLUDE_FROM_ALL tests/PhirnaIntegrationTest.cpp)
	add_executable(mmodern_myra_smoke EXCLUDE_FROM_ALL tests/MyraIntegrationTest.cpp)
	target_link_libraries(mmodern_myra_smoke PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_executable(mmodern_who_will_smoke EXCLUDE_FROM_ALL tests/WhoWillIntegrationTest.cpp)
	target_link_libraries(mmodern_who_will_smoke PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	target_link_libraries(mmodern_phirna_smoke PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
	add_executable(mmodern_visual_remove_tests tests/XeenVisualRemoveTests.cpp)
	target_link_libraries(mmodern_visual_remove_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_visual_remove COMMAND mmodern_visual_remove_tests)
	add_executable(mmodern_outdoor_object_smoke EXCLUDE_FROM_ALL tests/OutdoorObjectIntegrationTest.cpp)
	target_link_libraries(mmodern_outdoor_object_smoke PRIVATE mmodern_xeen)
	target_include_directories(mmodern_outdoor_object_smoke PRIVATE
		"${SCUMMVM_BUILD_DIR}" "${SCUMMVM_SOURCE_DIR}" "${SCUMMVM_SOURCE_DIR}/engines")
	target_compile_definitions(mmodern_outdoor_object_smoke PRIVATE WIN32 UNICODE _UNICODE)
	add_executable(mmodern_outdoor_composer_tests tests/XeenOutdoorComposerTests.cpp)
	target_link_libraries(mmodern_outdoor_composer_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_outdoor_composer COMMAND mmodern_outdoor_composer_tests)
	add_executable(mmodern_outdoor_object_tests tests/XeenOutdoorObjectTests.cpp)
	target_link_libraries(mmodern_outdoor_object_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_outdoor_objects COMMAND mmodern_outdoor_object_tests)
	add_executable(mmodern_object_visual_tests tests/XeenObjectVisualTests.cpp)
	target_link_libraries(mmodern_object_visual_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_object_visual COMMAND mmodern_object_visual_tests)
	add_executable(mmodern_object_sprite_tests tests/XeenObjectSpriteTests.cpp)
	target_link_libraries(mmodern_object_sprite_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_object_sprite COMMAND mmodern_object_sprite_tests)
	add_executable(mmodern_object_visual_smoke EXCLUDE_FROM_ALL tests/ObjectVisualIntegrationTest.cpp)
	target_link_libraries(mmodern_object_visual_smoke PRIVATE mmodern_xeen mmodern_scummvm_xeen)
	target_include_directories(mmodern_object_visual_smoke PRIVATE
		"${SCUMMVM_BUILD_DIR}" "${SCUMMVM_SOURCE_DIR}" "${SCUMMVM_SOURCE_DIR}/engines")
	target_compile_definitions(mmodern_object_visual_smoke PRIVATE WIN32 UNICODE _UNICODE)
	add_executable(mmodern_remove_tests tests/XeenRemoveTests.cpp)
	target_link_libraries(mmodern_remove_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_remove COMMAND mmodern_remove_tests)
	add_executable(mmodern_session_persistence_tests tests/XeenSessionPersistenceTests.cpp)
	target_link_libraries(mmodern_session_persistence_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_session_persistence COMMAND mmodern_session_persistence_tests)
	add_executable(mmodern_remove_smoke EXCLUDE_FROM_ALL tests/RemoveIntegrationTest.cpp)
	target_link_libraries(mmodern_remove_smoke PRIVATE mmodern_app mmodern_xeen mmodern_scummvm_xeen mmodern_platform_sdl SDL2::SDL2)
	add_executable(mmodern_session_identity_tests tests/XeenSessionIdentityTests.cpp)
	target_link_libraries(mmodern_session_identity_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_session_identity COMMAND mmodern_session_identity_tests)
	# Parser tests link only standard C++; no ScummVM, SDL, or game installation.
	add_executable(mmodern_map_format_tests
		tests/XeenMapFormatTests.cpp
		src/formats/xeen/XeenEventFormat.cpp
		src/formats/xeen/XeenMapFormat.cpp
	)
	target_link_libraries(mmodern_map_format_tests PRIVATE mmodern_core)
	add_test(NAME xeen_map_formats COMMAND mmodern_map_format_tests)
	add_executable(mmodern_event_format_tests
		tests/XeenEventFormatTests.cpp
		src/formats/xeen/XeenEventFormat.cpp
	)
	target_link_libraries(mmodern_event_format_tests PRIVATE mmodern_core)
	add_test(NAME xeen_event_formats COMMAND mmodern_event_format_tests)
	add_executable(mmodern_event_loader_tests
		tests/XeenEventLoaderTests.cpp
		src/formats/xeen/XeenEventFormat.cpp
		src/games/xeen/XeenEventDiagnostics.cpp
		src/games/xeen/XeenEventLoader.cpp
		src/games/xeen/XeenEventScript.cpp
	)
	target_link_libraries(mmodern_event_loader_tests PRIVATE mmodern_core)
	add_test(NAME xeen_event_loader COMMAND mmodern_event_loader_tests)
	add_executable(mmodern_event_text_tests
		tests/XeenEventTextTests.cpp
		src/formats/xeen/XeenEventTextFormat.cpp
		src/games/xeen/XeenEventTextLoader.cpp
	)
	target_link_libraries(mmodern_event_text_tests PRIVATE mmodern_core)
	add_test(NAME xeen_event_text COMMAND mmodern_event_text_tests)
	add_executable(mmodern_event_script_tests
		tests/XeenEventScriptTests.cpp
		src/games/xeen/XeenEventDiagnostics.cpp
		src/games/xeen/XeenEventScript.cpp
		src/games/xeen/XeenEventTrigger.cpp
	)
	target_link_libraries(mmodern_event_script_tests PRIVATE mmodern_core)
	add_test(NAME xeen_event_script COMMAND mmodern_event_script_tests)
	add_executable(mmodern_event_decoder_tests
		tests/XeenEventDecoderTests.cpp
		src/games/xeen/XeenEventDecoder.cpp
	)
	target_link_libraries(mmodern_event_decoder_tests PRIVATE mmodern_core)
	add_test(NAME xeen_event_decoder COMMAND mmodern_event_decoder_tests)
	add_executable(mmodern_event_interpreter_tests
		tests/XeenEventInterpreterTests.cpp
	)
	target_link_libraries(mmodern_event_interpreter_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_event_interpreter COMMAND mmodern_event_interpreter_tests)
	add_executable(mmodern_event_presentation_tests
		tests/XeenEventPresentationTests.cpp
	)
	target_link_libraries(mmodern_event_presentation_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_event_presentation COMMAND mmodern_event_presentation_tests)
	add_executable(mmodern_event_ui_tests
		tests/XeenEventUiTests.cpp
	)
	target_link_libraries(mmodern_event_ui_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_event_ui COMMAND mmodern_event_ui_tests)
	add_executable(mmodern_event_system_tests
		tests/XeenEventSystemTests.cpp
	)
	target_link_libraries(mmodern_event_system_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_event_system COMMAND mmodern_event_system_tests)
	add_executable(mmodern_manual_event_tests
		tests/XeenManualEventTests.cpp
	)
	target_link_libraries(mmodern_manual_event_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_manual_event COMMAND mmodern_manual_event_tests)
	add_executable(mmodern_navigation_flow_tests
		tests/XeenNavigationFlowTests.cpp
		src/app/XeenNavigationFlow.cpp
	)
	target_link_libraries(mmodern_navigation_flow_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_navigation_flow COMMAND mmodern_navigation_flow_tests)
	add_executable(mmodern_sdl_input_tests tests/SdlInputTests.cpp)
	target_link_libraries(mmodern_sdl_input_tests PRIVATE
		mmodern_platform_sdl SDL2::SDL2)
	target_link_options(mmodern_sdl_input_tests PRIVATE "-Wl,--wrap=SDL_GetTicks")
	add_test(NAME sdl_input COMMAND mmodern_sdl_input_tests)
	set_tests_properties(sdl_input PROPERTIES ENVIRONMENT
		"SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
	add_executable(mmodern_game_flags_tests
		tests/XeenGameFlagsTests.cpp
		src/formats/xeen/XeenGameFlagsFormat.cpp
		src/games/xeen/XeenGameFlags.cpp
	)
	target_link_libraries(mmodern_game_flags_tests PRIVATE mmodern_core)
	add_test(NAME xeen_game_flags COMMAND mmodern_game_flags_tests)
	add_test(NAME xeen_events_cli_negative
		COMMAND mmodern --inspect-events unused -1)
	add_test(NAME xeen_events_cli_trailing
		COMMAND mmodern --inspect-events unused 31abc)
	add_test(NAME xeen_events_cli_zero
		COMMAND mmodern --inspect-events unused 0)
	add_test(NAME xeen_events_cli_out_of_range
		COMMAND mmodern --inspect-events unused 10000)
	add_test(NAME xeen_events_cli_missing_argument
		COMMAND mmodern --inspect-events)
	add_test(NAME xeen_events_cli_excess_argument
		COMMAND mmodern --inspect-events unused 31 2 9 north extra)
	add_test(NAME xeen_events_cli_x_without_y
		COMMAND mmodern --inspect-events unused 31 2)
	add_test(NAME xeen_events_cli_negative_x
		COMMAND mmodern --inspect-events unused 31 -1 9)
	add_test(NAME xeen_events_cli_coordinate_overflow
		COMMAND mmodern --inspect-events unused 31 2 256)
	add_test(NAME xeen_events_cli_coordinate_trailing
		COMMAND mmodern --inspect-events unused 31 2abc 9)
	add_test(NAME xeen_events_cli_bad_direction
		COMMAND mmodern --inspect-events unused 31 2 9 northeast)
	set_tests_properties(
		xeen_events_cli_negative
		xeen_events_cli_trailing
		xeen_events_cli_zero
		xeen_events_cli_out_of_range
		xeen_events_cli_missing_argument
		xeen_events_cli_excess_argument
		xeen_events_cli_x_without_y
		xeen_events_cli_negative_x
		xeen_events_cli_coordinate_overflow
		xeen_events_cli_coordinate_trailing
		xeen_events_cli_bad_direction
		PROPERTIES WILL_FAIL TRUE
	)
	add_executable(mmodern_outdoor_scene_tests
		tests/XeenOutdoorSceneTests.cpp
	)
	target_link_libraries(mmodern_outdoor_scene_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_outdoor_scene COMMAND mmodern_outdoor_scene_tests)
	add_executable(mmodern_movement_tests
		tests/XeenMovementTests.cpp
		src/games/xeen/XeenMovement.cpp
		src/games/xeen/XeenWorld.cpp
	)
	target_link_libraries(mmodern_movement_tests PRIVATE mmodern_core)
	add_test(NAME xeen_movement COMMAND mmodern_movement_tests)
	add_executable(mmodern_world_tests
		tests/XeenWorldTests.cpp
		src/games/xeen/XeenWorld.cpp
	)
	target_link_libraries(mmodern_world_tests PRIVATE mmodern_core)
	add_test(NAME xeen_world COMMAND mmodern_world_tests)
	add_executable(mmodern_indoor_scene_tests
		tests/XeenIndoorSceneTests.cpp
	)
	target_link_libraries(mmodern_indoor_scene_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_indoor_scene COMMAND mmodern_indoor_scene_tests)
	add_executable(mmodern_indoor_composer_tests tests/XeenIndoorComposerTests.cpp)
	target_link_libraries(mmodern_indoor_composer_tests PRIVATE mmodern_app mmodern_xeen)
	add_test(NAME xeen_indoor_composer COMMAND mmodern_indoor_composer_tests)
	add_executable(mmodern_character_format_tests
		tests/XeenCharacterFormatTests.cpp
	)
	target_link_libraries(mmodern_character_format_tests PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	add_test(NAME xeen_character_formats COMMAND mmodern_character_format_tests)
	add_executable(mmodern_character_rules_tests
		tests/XeenCharacterRulesTests.cpp
		src/games/xeen/XeenCharacter.cpp
		src/games/xeen/XeenCharacterRules.cpp
	)
	target_link_libraries(mmodern_character_rules_tests PRIVATE mmodern_core)
	add_test(NAME xeen_character_rules COMMAND mmodern_character_rules_tests)
	add_executable(mmodern_combat_casting_rules_tests tests/XeenCombatCastingRulesTests.cpp)
	target_link_libraries(mmodern_combat_casting_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_combat_casting_rules COMMAND mmodern_combat_casting_rules_tests)
	add_executable(mmodern_learned_spell_rules_tests tests/XeenLearnedSpellRulesTests.cpp)
	target_link_libraries(mmodern_learned_spell_rules_tests PRIVATE mmodern_xeen)
	add_test(NAME xeen_learned_spell_rules COMMAND mmodern_learned_spell_rules_tests)
	if(WIN32 AND EXISTS "F:/Games/gog/Might and Magic 4-5")
		add_executable(mmodern_regional_action_original tests/XeenRegionalActionOriginal.cpp)
		target_link_libraries(mmodern_regional_action_original PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2)
		add_test(NAME xeen_regional_action_original COMMAND mmodern_regional_action_original "F:/Games/gog/Might and Magic 4-5")
	endif()
	add_executable(mmodern_equipment_tests
		tests/XeenEquipmentTests.cpp
		src/games/xeen/XeenEquipment.cpp
		src/games/xeen/XeenItemTransfer.cpp
		src/games/xeen/XeenCharacter.cpp
		src/games/xeen/XeenCharacterRules.cpp
		src/games/xeen/XeenParty.cpp
	)
	target_link_libraries(mmodern_equipment_tests PRIVATE mmodern_core)
	add_test(NAME xeen_equipment COMMAND mmodern_equipment_tests)
	add_executable(mmodern_party_visual_state_tests
		tests/XeenPartyVisualStateTests.cpp
		src/games/xeen/XeenCharacter.cpp
		src/games/xeen/XeenCharacterRules.cpp
		src/games/xeen/XeenPartyVisualState.cpp
	)
	target_link_libraries(mmodern_party_visual_state_tests PRIVATE mmodern_core)
	add_test(NAME xeen_party_visual_state COMMAND mmodern_party_visual_state_tests)
	# Explicit target: requires a user-supplied real installation when executed.
	add_executable(mmodern_graphics_smoke EXCLUDE_FROM_ALL tests/GraphicsSmokeTest.cpp)
	target_link_libraries(mmodern_graphics_smoke PRIVATE
		mmodern_app mmodern_xeen mmodern_scummvm_xeen mmodern_platform_sdl SDL2::SDL2)
	if(MINGW)
		target_link_options(mmodern_graphics_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_party_smoke EXCLUDE_FROM_ALL tests/PartyIntegrationTest.cpp)
	target_link_libraries(mmodern_party_smoke PRIVATE mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_party_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_item_catalog_smoke EXCLUDE_FROM_ALL
		tests/ItemCatalogIntegrationTest.cpp)
	target_link_libraries(mmodern_item_catalog_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen bcrypt)
	if(MINGW)
		target_link_options(mmodern_item_catalog_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_indoor_map_smoke EXCLUDE_FROM_ALL
		tests/IndoorMapIntegrationTest.cpp)
	target_link_libraries(mmodern_indoor_map_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_indoor_map_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_indoor_object_smoke EXCLUDE_FROM_ALL
		tests/IndoorObjectIntegrationTest.cpp)
	target_link_libraries(mmodern_indoor_object_smoke PRIVATE
		mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 ZLIB::ZLIB)
	if(MINGW)
		target_link_options(mmodern_indoor_object_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_event_script_smoke EXCLUDE_FROM_ALL
		tests/EventScriptIntegrationTest.cpp)
	target_link_libraries(mmodern_event_script_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_event_script_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_event_text_smoke EXCLUDE_FROM_ALL
		tests/EventTextIntegrationTest.cpp)
	target_link_libraries(mmodern_event_text_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_event_text_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_game_flags_smoke EXCLUDE_FROM_ALL
		tests/GameFlagsIntegrationTest.cpp)
	target_link_libraries(mmodern_game_flags_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_game_flags_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_event_interpreter_smoke EXCLUDE_FROM_ALL
		tests/EventInterpreterIntegrationTest.cpp)
	target_link_libraries(mmodern_event_interpreter_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_event_interpreter_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_event_system_smoke EXCLUDE_FROM_ALL
		tests/EventSystemIntegrationTest.cpp)
	target_link_libraries(mmodern_event_system_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_event_system_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_manual_event_smoke EXCLUDE_FROM_ALL
		tests/ManualEventIntegrationTest.cpp)
	target_link_libraries(mmodern_manual_event_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_manual_event_smoke PRIVATE -Wl,--gc-sections)
	endif()
	add_executable(mmodern_navigation_flow_smoke EXCLUDE_FROM_ALL
		tests/NavigationFlowIntegrationTest.cpp
		src/app/XeenNavigationFlow.cpp)
	target_link_libraries(mmodern_navigation_flow_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_navigation_flow_smoke PRIVATE -Wl,--gc-sections)
	endif()
	# M44 original-data scenarios; digests and regeneration are in tests/XeenM44BaselineDigests.h.
	add_executable(mmodern_m44_baseline_witness EXCLUDE_FROM_ALL src/main.cpp tests/XeenM44BaselineWitness.cpp)
	target_link_libraries(mmodern_m44_baseline_witness PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl SDL2::SDL2 shell32 bcrypt)
	target_link_options(mmodern_m44_baseline_witness PRIVATE "LINKER:--wrap,_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE" "LINKER:--wrap,_ZN7mmodern16XeenCombatRandom4drawEjj")
	add_executable(mmodern_m44_baseline_restart EXCLUDE_FROM_ALL tests/XeenM44BaselineRestart.cpp)
	add_executable(mmodern_m44_scenario_tests tests/XeenM44ScenarioTests.cpp)
	target_link_libraries(mmodern_m44_scenario_tests PRIVATE mmodern_app mmodern_xeen mmodern_platform_sdl bcrypt)
	add_dependencies(mmodern_m44_scenario_tests mmodern_m44_baseline_witness mmodern_m44_baseline_restart mmodern_m43_cli_witness)
	foreach(scenario mainland services temple)
		if(scenario STREQUAL "temple")
			set(m44_generator $<TARGET_FILE:mmodern_m43_cli_witness>)
		else()
			set(m44_generator $<TARGET_FILE:mmodern_m44_baseline_witness>)
		endif()
		add_test(NAME xeen_m44_scenario_${scenario} COMMAND mmodern_m44_scenario_tests ${scenario} ${m44_generator}
			$<TARGET_FILE:mmodern_m44_baseline_witness> $<TARGET_FILE:mmodern_m44_baseline_restart> "F:/Games/gog/Might and Magic 4-5")
		set_tests_properties(xeen_m44_scenario_${scenario} PROPERTIES TIMEOUT 1800)
	endforeach()
	# Every test is labelled `fast` (default iteration: ctest -L fast) or `process`
	# (full run: plain ctest). `process` = runs 30 s or more, or spawns child
	# processes / drives a CLI witness executable.
	set(xeen_process_tests
		xeen_resource_cold_original
		xeen_purchase_flow xeen_purchase_allocation xeen_purchase_process
		xeen_temple_flow xeen_temple_process xeen_temple_depleted_process
		xeen_training_flow xeen_training_allocation xeen_training_original xeen_training_process
		xeen_input_scheduling xeen_service_day_process xeen_combat_clock_process
		xeen_journey_resources xeen_journey_persistence xeen_regional_persistence
		xeen_combat_casting_process xeen_combat_casting_controls
		xeen_combat_player_ready_input
		xeen_smith_process xeen_vertigo_process xeen_save_format xeen_save_cli
		xeen_combat_gameplay xeen_equipment_gameplay xeen_inventory_gameplay
		xeen_m44_scenario_mainland xeen_m44_scenario_services xeen_m44_scenario_temple)
	get_property(xeen_all_tests DIRECTORY PROPERTY TESTS)
	foreach(xeen_test IN LISTS xeen_process_tests)
		if(NOT xeen_test IN_LIST xeen_all_tests)
			message(FATAL_ERROR "Process-labelled test does not exist: ${xeen_test}")
		endif()
	endforeach()
	foreach(xeen_test IN LISTS xeen_all_tests)
		if(xeen_test IN_LIST xeen_process_tests)
			set_tests_properties(${xeen_test} PROPERTIES LABELS process)
		else()
			set_tests_properties(${xeen_test} PROPERTIES LABELS fast)
		endif()
	endforeach()
endif()
