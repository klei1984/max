include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${JSON_FILE})
	file(${JSON_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${JSON_FILE} JSON_FILE_HASH)

	if(${JSON_FILE_HASH} STREQUAL ${JSON_HASH})
		set(JSON_URI file://${PROJECT_SOURCE_DIR}/dependencies/${JSON_FILE})
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

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(JSON)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
