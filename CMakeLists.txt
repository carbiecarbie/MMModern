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

add_library(mmodern_xeen STATIC
	src/formats/xeen/XeenAssetSource.cpp
	src/formats/xeen/XeenCloudsVisualMetadata.cpp
	src/games/xeen/XeenObjectVisual.cpp
	src/formats/xeen/XeenCharacterFormat.cpp
	src/formats/xeen/XeenQuestItemFormat.cpp
	src/formats/xeen/XeenQuestFlagFormat.cpp
	src/formats/xeen/XeenEventFormat.cpp
	src/formats/xeen/XeenEventTextFormat.cpp
	src/formats/xeen/XeenFontFormat.cpp
	src/formats/xeen/XeenGameFlagsFormat.cpp
	src/formats/xeen/XeenMapFormat.cpp
	src/games/xeen/XeenCharacter.cpp
	src/games/xeen/XeenCharacterRules.cpp
	src/games/xeen/XeenEventDecoder.cpp
	src/games/xeen/XeenEventDiagnostics.cpp
	src/games/xeen/XeenEventInterpreter.cpp
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
	src/games/xeen/XeenInstallationDetector.cpp
)
target_link_libraries(mmodern_xeen PUBLIC mmodern_core PRIVATE mmodern_scummvm_xeen)

add_library(mmodern_platform_sdl STATIC
	src/platform/sdl/SdlWindow.cpp
)
target_link_libraries(mmodern_platform_sdl PUBLIC mmodern_core PRIVATE SDL2::SDL2)

add_library(mmodern_app STATIC
	src/app/Application.cpp
	src/app/XeenEventFlow.cpp
	src/app/XeenNavigationFlow.cpp
)
target_link_libraries(mmodern_app PUBLIC mmodern_core PRIVATE
	mmodern_xeen
	mmodern_platform_sdl
)

add_executable(mmodern src/main.cpp)
target_link_libraries(mmodern PRIVATE
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
if(BUILD_TESTING)
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
		src/games/xeen/XeenIndoorScene.cpp
		src/games/xeen/XeenWorld.cpp
	)
	target_link_libraries(mmodern_indoor_scene_tests PRIVATE mmodern_core)
	add_test(NAME xeen_indoor_scene COMMAND mmodern_indoor_scene_tests)
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
	add_executable(mmodern_indoor_map_smoke EXCLUDE_FROM_ALL
		tests/IndoorMapIntegrationTest.cpp)
	target_link_libraries(mmodern_indoor_map_smoke PRIVATE
		mmodern_xeen mmodern_scummvm_xeen)
	if(MINGW)
		target_link_options(mmodern_indoor_map_smoke PRIVATE -Wl,--gc-sections)
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
endif()
