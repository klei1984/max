include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${JSONSCHEMA_FILE})
	file(${JSONSCHEMA_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${JSONSCHEMA_FILE} JSONSCHEMA_FILE_HASH)

	if(${JSONSCHEMA_FILE_HASH} STREQUAL ${JSONSCHEMA_HASH})
		set(JSONSCHEMA_URI file://${PROJECT_SOURCE_DIR}/dependencies/${JSONSCHEMA_FILE})
	endif()
endif()

find_package(Patch)

if(NOT Patch_FOUND)
	message(FATAL_ERROR "Patch tool is required.")
endif()

set(JSONSCHEMA_PATCH ${Patch_EXECUTABLE} -p0 < ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/patches/jsonschema.patch)

FetchContent_Declare(
	JSONSCHEMA
	TIMEOUT 60
	URL ${JSONSCHEMA_URI}
	URL_HASH ${JSONSCHEMA_HASH_TYPE}=${JSONSCHEMA_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
	PATCH_COMMAND ${JSONSCHEMA_PATCH}
	UPDATE_DISCONNECTED TRUE
)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(JSONSCHEMA)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})
