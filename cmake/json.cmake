include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${JSON_FILE})
	file(${JSON_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${JSON_FILE} JSON_FILE_HASH)

	if(${JSON_FILE_HASH} STREQUAL ${JSON_HASH})
		set(JSON_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${JSON_FILE})
	endif()
endif()

FetchContent_Declare(
	JSON
	TIMEOUT 60
	URL ${JSON_URI}
	URL_HASH ${JSON_HASH_TYPE}=${JSON_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

set(JSON_GlobalUDLs OFF)
set(JSON_ImplicitConversions OFF)

FetchContent_MakeAvailable(JSON)
