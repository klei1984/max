include(versions)
include(FetchContent)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE})
	file(${XOSHIRO_HASH_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE} XOSHIRO_FILE_HASH)

	if(${XOSHIRO_FILE_HASH} STREQUAL ${XOSHIRO_HASH})
		set(XOSHIRO_URI file://${CMAKE_CURRENT_SOURCE_DIR}/dependencies/${XOSHIRO_FILE})
	endif()
endif()

FetchContent_Declare(
	XOSHIRO
	TIMEOUT 60
	URL ${XOSHIRO_URI}
	URL_HASH ${XOSHIRO_HASH_TYPE}=${XOSHIRO_HASH}
	DOWNLOAD_EXTRACT_TIMESTAMP FALSE
	OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(XOSHIRO)

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
