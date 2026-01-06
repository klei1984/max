include(versions)
include(FetchContent)

if(EXISTS ${PROJECT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE})
	file(${XOSHIRO_HASH_TYPE} ${PROJECT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE} XOSHIRO_FILE_HASH)

	if(${XOSHIRO_FILE_HASH} STREQUAL ${XOSHIRO_HASH})
		set(XOSHIRO_URI file://${PROJECT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE})
	endif()
endif()

set(XOSHIRO_PATCH ${Patch_EXECUTABLE} -p0 < ${PROJECT_SOURCE_DIR}/dependencies/patches/xoshiro.patch)

FetchContent_Declare(
	XOSHIRO
	TIMEOUT 60
	URL ${XOSHIRO_URI}
	URL_HASH ${XOSHIRO_HASH_TYPE}=${XOSHIRO_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
	PATCH_COMMAND ${XOSHIRO_PATCH}
	UPDATE_DISCONNECTED TRUE
)

# Force Release build without debug info (treat as system library)
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

FetchContent_MakeAvailable(XOSHIRO)

# Restore build configuration
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})

set(XOSHIRO_BINARY_DIR ${PROJECT_BINARY_DIR}/_deps/xoshiro-build)
set(XOSHIRO_SOURCE_DIR ${PROJECT_BINARY_DIR}/_deps/xoshiro-src)

file(GLOB XOSHIRO_FILES CONFIGURE_DEPENDS
	"${XOSHIRO_SOURCE_DIR}/*.hpp"
)

file(COPY ${XOSHIRO_FILES} DESTINATION ${XOSHIRO_BINARY_DIR})

add_library(Xoshiro INTERFACE)

target_include_directories(Xoshiro INTERFACE "${XOSHIRO_BINARY_DIR}")

if(NOT TARGET Xoshiro::Xoshiro)
	add_library(Xoshiro::Xoshiro ALIAS Xoshiro)
endif()
