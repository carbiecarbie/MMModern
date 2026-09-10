include_guard(GLOBAL)

set(MMODERN_ITEM_CATALOG_SCUMMVM_REVISION
	"6814ee9ba54582f5b5adcffab49efbbd8f589edd")
set(MMODERN_ITEM_CATALOG_BLOB_PATH
	"devtools/create_mm/files/xeen/CONSTANTS_7")
set(MMODERN_ITEM_CATALOG_BLOB_OID
	"455b2eb3900a60be910e4d045d103a73586e73b0")
set(MMODERN_ITEM_CATALOG_BLOB_SHA256
	"a3022d378e7570a56332f30128942afe02ae2bfdef70c307b2de997eb07a9e77")
set(MMODERN_ITEM_CATALOG_GENERATED_SHA256
	"feb9c081a65c79faf1ad1937df515121d944836a0224670e8ac93819dec66b32")
set(_MMODERN_ITEM_CATALOG_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(add_mmodern_item_catalog_generation)
	if(CMAKE_CROSSCOMPILING)
		message(FATAL_ERROR
			"MMModern's item catalog generation currently requires a native Windows build; cross-compilation is unsupported")
	endif()
	find_package(Git REQUIRED)
	find_program(item_catalog_powershell powershell REQUIRED)
	set(generator
		"${_MMODERN_ITEM_CATALOG_MODULE_DIR}/../tools/GenerateXeenItemCatalog.ps1")
	set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
	set(generated_include "${generated_dir}/XeenItemCatalogEnglish.inc")
	set(generate_command
		"${item_catalog_powershell}" -NoProfile -NonInteractive -ExecutionPolicy Bypass
		-File "${generator}"
		-Source "${SCUMMVM_SOURCE_DIR}"
		-Revision "${MMODERN_ITEM_CATALOG_SCUMMVM_REVISION}"
		-Git "${GIT_EXECUTABLE}"
		-ExpectedBlobOid "${MMODERN_ITEM_CATALOG_BLOB_OID}"
		-ExpectedBlobSha256 "${MMODERN_ITEM_CATALOG_BLOB_SHA256}"
		-Output "${generated_include}"
	)

	# Generate during configuration for an immediate dependency failure, and run
	# the same immutable-blob generation on every build so stale output cannot
	# bypass the source/revision gate.
	execute_process(COMMAND ${generate_command} RESULT_VARIABLE generate_result)
	if(NOT generate_result EQUAL 0)
		message(FATAL_ERROR "ScummVM item-catalog generation failed")
	endif()
	add_custom_target(mmodern_item_catalog_data ALL
		COMMAND ${generate_command}
		BYPRODUCTS "${generated_include}"
		VERBATIM)
	set_source_files_properties("${generated_include}" PROPERTIES GENERATED TRUE)
	set(MMODERN_ITEM_CATALOG_GENERATED_DIR "${generated_dir}" PARENT_SCOPE)
	set(MMODERN_ITEM_CATALOG_POWERSHELL "${item_catalog_powershell}" PARENT_SCOPE)
	set(MMODERN_ITEM_CATALOG_GENERATOR "${generator}" PARENT_SCOPE)
endfunction()
