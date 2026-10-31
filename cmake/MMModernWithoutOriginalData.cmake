include_guard(GLOBAL)

# Data-free configuration (MMODERN_WITHOUT_ORIGINAL_DATA=ON, off by default).
# Included only when the option is on, so a default configuration is unchanged.
#
# Every external test input (original data, CD audio, captures, MT-32 ROMs)
# is replaced by one marker path that never exists. A test whose command line
# mentions the marker needs one of those inputs and is not registered, and
# property calls on such a test are ignored. Every other test registers
# exactly as in a default configuration.

set(MMODERN_ORIGINAL_DATA_MARKER
	"${CMAKE_CURRENT_BINARY_DIR}/original-data-not-configured")
foreach(mmodern_external_input
		MMODERN_XEEN_DATA_DIR MMODERN_XEEN_AUDIO_DIR MMODERN_PROMUS_CAPTURES_DIR
		MMODERN_M94_CAPTURES_DIR MMODERN_MT32_ROM_DIR)
	if(NOT "${${mmodern_external_input}}" STREQUAL "")
		message(STATUS "MMODERN_WITHOUT_ORIGINAL_DATA ignores ${mmodern_external_input}")
	endif()
	# A normal variable shadows the cache entry for this configuration only.
	set(${mmodern_external_input} "${MMODERN_ORIGINAL_DATA_MARKER}")
endforeach()
set(MMODERN_XEEN_UI_DATA "")
set_property(GLOBAL PROPERTY MMODERN_UNREGISTERED_TESTS "")

# Arguments are forwarded one by one, bracket-quoted, so empty arguments and
# arguments containing semicolons (ENVIRONMENT lists) reach the command intact.
function(add_test)
	if(ARGV0 STREQUAL "NAME" AND "${ARGV}" MATCHES "original-data-not-configured")
		set_property(GLOBAL APPEND PROPERTY MMODERN_UNREGISTERED_TESTS "${ARGV1}")
		return()
	endif()
	set(arguments "")
	math(EXPR last "${ARGC} - 1")
	foreach(index RANGE 0 ${last})
		string(APPEND arguments " [==[${ARGV${index}}]==]")
	endforeach()
	cmake_language(EVAL CODE "_add_test(${arguments})")
endfunction()

function(mmodern_remove_unregistered_tests list_variable)
	get_property(unregistered GLOBAL PROPERTY MMODERN_UNREGISTERED_TESTS)
	set(result ${${list_variable}})
	if(unregistered)
		list(REMOVE_ITEM result ${unregistered})
	endif()
	set(${list_variable} ${result} PARENT_SCOPE)
endfunction()

function(set_tests_properties)
	get_property(unregistered GLOBAL PROPERTY MMODERN_UNREGISTERED_TESTS)
	set(arguments "")
	set(names_done FALSE)
	set(kept_names 0)
	math(EXPR last "${ARGC} - 1")
	foreach(index RANGE 0 ${last})
		set(argument "${ARGV${index}}")
		if(NOT names_done AND argument STREQUAL "PROPERTIES")
			set(names_done TRUE)
		elseif(NOT names_done AND argument IN_LIST unregistered)
			continue()
		elseif(NOT names_done)
			math(EXPR kept_names "${kept_names} + 1")
		endif()
		string(APPEND arguments " [==[${argument}]==]")
	endforeach()
	if(kept_names GREATER 0)
		cmake_language(EVAL CODE "_set_tests_properties(${arguments})")
	endif()
endfunction()
