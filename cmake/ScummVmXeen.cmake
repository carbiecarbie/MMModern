include_guard(GLOBAL)

function(add_mmodern_scummvm_xeen_bridge target_name)
	if(NOT EXISTS "${SCUMMVM_SOURCE_DIR}/engines/mm/shared/xeen/cc_archive.cpp")
		message(FATAL_ERROR "SCUMMVM_SOURCE_DIR does not point to a ScummVM source tree: ${SCUMMVM_SOURCE_DIR}")
	endif()
	if(NOT EXISTS "${SCUMMVM_BUILD_DIR}/config.h")
		message(FATAL_ERROR "SCUMMVM_BUILD_DIR is not configured: ${SCUMMVM_BUILD_DIR}")
	endif()

	set(scummvm_link_files
		"${SCUMMVM_BUILD_DIR}/image/libimage.a"
		"${SCUMMVM_BUILD_DIR}/graphics/libgraphics.a"
		"${SCUMMVM_BUILD_DIR}/common/formats/po_parser.o"
		"${SCUMMVM_BUILD_DIR}/common/libcommon.a"
	)
	foreach(link_file IN LISTS scummvm_link_files)
		if(NOT EXISTS "${link_file}")
			message(FATAL_ERROR
				"Missing ScummVM build artifact: ${link_file}. Build devtools/xeen_probe first.")
		endif()
	endforeach()
	set(scummvm_platform_compile_flags)
	if(APPLE)
		if(CMAKE_OSX_SYSROOT)
			list(APPEND scummvm_platform_compile_flags -isysroot "${CMAKE_OSX_SYSROOT}")
		endif()
		foreach(architecture IN LISTS CMAKE_OSX_ARCHITECTURES)
			list(APPEND scummvm_platform_compile_flags -arch "${architecture}")
		endforeach()
		if(CMAKE_OSX_DEPLOYMENT_TARGET)
			list(APPEND scummvm_platform_compile_flags "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
		endif()
	endif()

	if(WIN32)
		find_program(SCUMMVM_OBJCOPY NAMES objcopy REQUIRED)
	endif()
	set(sprite_source "${SCUMMVM_SOURCE_DIR}/engines/mm/shared/xeen/sprites.cpp")
	set(sprite_object_dir "${CMAKE_CURRENT_BINARY_DIR}/scummvm_bridge")
	set(sprite_full_object "${sprite_object_dir}/sprites.full.o")
	set(sprite_stream_object "${sprite_object_dir}/sprites.stream.o")

	# SpriteResource's stream decoder shares a translation unit with engine-only
	# path loading. MinGW resolves those unused references before gc-sections, so
	# build a private filtered object exactly as the proven xeen_probe does.
	if(WIN32)
	add_custom_command(
		OUTPUT "${sprite_stream_object}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${sprite_object_dir}"
		COMMAND "${CMAKE_CXX_COMPILER}"
			-std=gnu++11 ${MMODERN_SCUMMVM_DEFINE_FLAGS}
			-ffunction-sections -fdata-sections
			-I "${SCUMMVM_BUILD_DIR}" -I "${SCUMMVM_SOURCE_DIR}"
			-I "${SCUMMVM_SOURCE_DIR}/engines"
			-c "${sprite_source}" -o "${sprite_full_object}"
		COMMAND "${SCUMMVM_OBJCOPY}"
			"--remove-section=.text$_ZN2MM6Shared4Xeen14SpriteResourceC2ERKN6Common4PathE"
			"--remove-section=.xdata$_ZN2MM6Shared4Xeen14SpriteResourceC2ERKN6Common4PathE"
			"--remove-section=.pdata$_ZN2MM6Shared4Xeen14SpriteResourceC2ERKN6Common4PathE"
			"--remove-section=.text$_ZN2MM6Shared4Xeen14SpriteResource4loadERKN6Common4PathE"
			"--remove-section=.xdata$_ZN2MM6Shared4Xeen14SpriteResource4loadERKN6Common4PathE"
			"--remove-section=.pdata$_ZN2MM6Shared4Xeen14SpriteResource4loadERKN6Common4PathE"
			"--remove-section=.text$_ZN2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.xdata$_ZN2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.pdata$_ZN2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.text$_ZTv0_n24_N2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.debug_frame$_ZN2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.debug_frame$_ZTv0_n24_N2MM6Shared4Xeen4FileD1Ev"
			"--remove-section=.rdata$.refptr._ZTTN2MM6Shared4Xeen4FileE"
			"--remove-section=.rdata$.refptr._ZTVN2MM6Shared4Xeen4FileE"
			"${sprite_full_object}" "${sprite_stream_object}"
		COMMAND "${CMAKE_COMMAND}" -E rm -f "${sprite_full_object}"
		DEPENDS "${sprite_source}"
		VERBATIM
	)
 else()
 add_custom_command(OUTPUT "${sprite_stream_object}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${sprite_object_dir}"
		COMMAND "${CMAKE_CXX_COMPILER}" ${scummvm_platform_compile_flags} -std=gnu++11 ${MMODERN_SCUMMVM_DEFINE_FLAGS}
   -ffunction-sections -fdata-sections -I "${SCUMMVM_BUILD_DIR}" -I "${SCUMMVM_SOURCE_DIR}"
   -I "${SCUMMVM_SOURCE_DIR}/engines" -c "${sprite_source}" -o "${sprite_stream_object}"
  DEPENDS "${sprite_source}" VERBATIM)
 endif()
	set_source_files_properties("${sprite_stream_object}" PROPERTIES
		GENERATED TRUE
		EXTERNAL_OBJECT TRUE
	)

	# Keep only Nuked's plain OPL3 core. The class adapter in the same source
	# requires ScummVM's mixer and g_system; neither is used by MMModern audio.
	set(nuked_source "${SCUMMVM_SOURCE_DIR}/audio/softsynth/opl/nuked.cpp")
	set(nuked_full "${sprite_object_dir}/nuked.full.o")
	set(nuked_core "${sprite_object_dir}/nuked.core.o")
	if(WIN32)
	add_custom_command(OUTPUT "${nuked_core}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${sprite_object_dir}"
		COMMAND "${CMAKE_CXX_COMPILER}" ${scummvm_platform_compile_flags} -std=gnu++11 ${MMODERN_SCUMMVM_DEFINE_FLAGS}
			-O2 -ffunction-sections -fdata-sections
			-I "${SCUMMVM_BUILD_DIR}" -I "${SCUMMVM_SOURCE_DIR}"
			-c "${nuked_source}" -o "${nuked_full}"
		COMMAND "${SCUMMVM_OBJCOPY}"
			"--remove-section=*3OPL5NUKED3OPL*"
			"--remove-section=*5Audio*" "--remove-section=*_ZT*"
			"--remove-section=*refptr*" "${nuked_full}" "${nuked_core}"
		COMMAND "${CMAKE_COMMAND}" -E rm -f "${nuked_full}"
		DEPENDS "${nuked_source}" "${SCUMMVM_SOURCE_DIR}/audio/softsynth/opl/nuked.h"
		VERBATIM)
 else()
 add_custom_command(OUTPUT "${nuked_core}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${sprite_object_dir}"
		COMMAND "${CMAKE_CXX_COMPILER}" ${scummvm_platform_compile_flags} -std=gnu++11 ${MMODERN_SCUMMVM_DEFINE_FLAGS}
   -O2 -ffunction-sections -fdata-sections -I "${SCUMMVM_BUILD_DIR}" -I "${SCUMMVM_SOURCE_DIR}"
   -c "${nuked_source}" -o "${nuked_core}"
  DEPENDS "${nuked_source}" "${SCUMMVM_SOURCE_DIR}/audio/softsynth/opl/nuked.h" VERBATIM)
	endif()
	set_source_files_properties("${nuked_core}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

	add_library(${target_name} STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/src/core/ReadOnlyData.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/core/GameInstallation.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/ScummVmRuntime.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/ScummVmXeenBridge.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/formats/xeen/XeenRussianTranslation.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/formats/xeen/XeenObjectSpriteSafety.cpp"
		"${SCUMMVM_SOURCE_DIR}/engines/mm/shared/xeen/cc_archive.cpp"
		"${sprite_stream_object}"
		"${nuked_core}"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/XeenOpl3.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/ScummVmPictureFilter.cpp"
	)
	target_include_directories(${target_name}
		PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
		PRIVATE "${SCUMMVM_BUILD_DIR}" "${SCUMMVM_SOURCE_DIR}"
			"${SCUMMVM_SOURCE_DIR}/engines"
	)
	target_compile_definitions(${target_name} PRIVATE ${MMODERN_SCUMMVM_DEFINES})
	target_compile_options(${target_name} PRIVATE -ffunction-sections -fdata-sections)
	target_link_libraries(${target_name} PUBLIC mmodern_core PRIVATE
		${scummvm_link_files}
		ZLIB::ZLIB
	)

	if(CMAKE_EXECUTABLE_FORMAT STREQUAL "ELF")
  target_link_options(${target_name} INTERFACE -Wl,--gc-sections)
 elseif(APPLE)
		target_link_options(${target_name} INTERFACE -Wl,-dead_strip -Wl,-undefined,error)
 endif()
	target_link_libraries(${target_name} PRIVATE mmodern_platform_libs)
endfunction()
