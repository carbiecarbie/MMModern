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

	find_program(SCUMMVM_OBJCOPY NAMES objcopy REQUIRED)
	set(sprite_source "${SCUMMVM_SOURCE_DIR}/engines/mm/shared/xeen/sprites.cpp")
	set(sprite_object_dir "${CMAKE_CURRENT_BINARY_DIR}/scummvm_bridge")
	set(sprite_full_object "${sprite_object_dir}/sprites.full.o")
	set(sprite_stream_object "${sprite_object_dir}/sprites.stream.o")

	# SpriteResource's stream decoder shares a translation unit with engine-only
	# path loading. MinGW resolves those unused references before gc-sections, so
	# build a private filtered object exactly as the proven xeen_probe does.
	add_custom_command(
		OUTPUT "${sprite_stream_object}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${sprite_object_dir}"
		COMMAND "${CMAKE_CXX_COMPILER}"
			-std=gnu++11 -DWIN32 -DUNICODE -D_UNICODE
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
	set_source_files_properties("${sprite_stream_object}" PROPERTIES
		GENERATED TRUE
		EXTERNAL_OBJECT TRUE
	)

	add_library(${target_name} STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/ScummVmRuntime.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/compat/scummvm/ScummVmXeenBridge.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/formats/xeen/XeenObjectSpriteSafety.cpp"
		"${SCUMMVM_SOURCE_DIR}/engines/mm/shared/xeen/cc_archive.cpp"
		"${sprite_stream_object}"
	)
	target_include_directories(${target_name}
		PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
		PRIVATE "${SCUMMVM_BUILD_DIR}" "${SCUMMVM_SOURCE_DIR}"
			"${SCUMMVM_SOURCE_DIR}/engines"
	)
	target_compile_definitions(${target_name} PRIVATE WIN32 UNICODE _UNICODE)
	target_compile_options(${target_name} PRIVATE -ffunction-sections -fdata-sections)
	target_link_libraries(${target_name} PUBLIC mmodern_core PRIVATE
		${scummvm_link_files}
		ZLIB::ZLIB
	)

	if(MINGW)
		target_link_libraries(${target_name} PRIVATE
			winmm gdi32 ole32 uuid winspool sapi
		)
	endif()
endfunction()
